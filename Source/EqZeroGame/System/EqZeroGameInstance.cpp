// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameInstance.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "HAL/IConsoleManager.h"
#include "EqZeroGameplayTags.h"
#include "Misc/Paths.h"
#include "Player/EqZeroPlayerController.h"
#include "Player/EqZeroLocalPlayer.h"
#include "GameFramework/PlayerState.h"
#include "JsEnv.h"

#if UE_WITH_DTLS
#include "DTLSCertStore.h"
#include "DTLSHandlerComponent.h"
#include "Misc/FileHelper.h"
#endif // UE_WITH_DTLS

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameInstance)

namespace EqZero
{
	static bool bTestEncryption = false;
	static FAutoConsoleVariableRef CVarEqZeroTestEncryption(
		TEXT("EqZero.TestEncryption"),
		bTestEncryption,
		TEXT("If true, clients will send an encryption token with their request to join the server and attempt to encrypt the connection using a debug key. This is NOT SECURE and for demonstration purposes only."),
		ECVF_Default);

#if UE_WITH_DTLS
	static bool bUseDTLSEncryption = false;
	static FAutoConsoleVariableRef CVarEqZeroUseDTLSEncryption(
		TEXT("EqZero.UseDTLSEncryption"),
		bUseDTLSEncryption,
		TEXT("Set to true if using EqZero.TestEncryption and the DTLS packet handler."),
		ECVF_Default);

	// 适用于在同一设备（桌面版构建）上对多个游戏实例进行测试
	static bool bTestDTLSFingerprint = false;
	static FAutoConsoleVariableRef CVarEqZeroTestDTLSFingerprint(
		TEXT("EqZero.TestDTLSFingerprint"),
		bTestDTLSFingerprint,
		TEXT("If true and using DTLS encryption, generate unique cert per connection and fingerprint will be written to file to simulate passing through an online service."),
		ECVF_Default);

#if !UE_BUILD_SHIPPING
	/** 
		用于生成测试用 DTLS 自签名证书的控制台命令。
		使用方法：GenerateDTLSCertificate <证书名称>
		证书将导出至 Content/DTLS/<证书名称>.pem
	 */
	static FAutoConsoleCommandWithWorldAndArgs CmdGenerateDTLSCertificate(
		TEXT("GenerateDTLSCertificate"),
		TEXT("Generate a DTLS self-signed certificate for testing and export to PEM."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.Num() == 1)
				{
					const FString& CertName = InArgs[0];

					FTimespan CertExpire = FTimespan::FromDays(365);
					TSharedPtr<FDTLSCertificate> Cert = FDTLSCertStore::Get().CreateCert(CertExpire, CertName);
					if (Cert.IsValid())
					{
						const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / FPaths::MakeValidFileName(FString::Printf(TEXT("%s.pem"), *CertName));
						if (!Cert->ExportCertificate(CertPath))
						{
							UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Failed to export certificate."));
						}
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Failed to generate certificate."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Invalid argument(s)."));
				}
			}));
#endif // UE_BUILD_SHIPPING
#endif // UE_WITH_DTLS
};

UEqZeroGameInstance::UEqZeroGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AEqZeroPlayerController* UEqZeroGameInstance::GetPrimaryPlayerController() const
{
	return Cast<AEqZeroPlayerController>(Super::GetPrimaryPlayerController(false));
}

bool UEqZeroGameInstance::CanJoinRequestedSession() const
{
	if (!Super::CanJoinRequestedSession())
	{
		return false;
	}
	return true;
}

void UEqZeroGameInstance::HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	Super::HandlerUserInitialized(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);

	if (bSuccess && ensure(UserInfo))
	{
		UEqZeroLocalPlayer* LocalPlayer = Cast<UEqZeroLocalPlayer>(GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex));
		if (LocalPlayer)
		{
			// LocalPlayer->LoadSharedSettingsFromDisk();
			// TODO: Add settings loading when EqZeroLocalPlayer has the method
		}
	}
}

void UEqZeroGameInstance::Init()
{
	Super::Init();

	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);
	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(EqZeroGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(EqZeroGameplayTags::InitState_DataAvailable, false, EqZeroGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(EqZeroGameplayTags::InitState_DataInitialized, false, EqZeroGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(EqZeroGameplayTags::InitState_GameplayReady, false, EqZeroGameplayTags::InitState_DataInitialized);
	}

	// 测试用的加密KEY
	DebugTestEncryptionKey.SetNum(32);
	for (int32 i = 0; i < DebugTestEncryptionKey.Num(); ++i)
	{
		DebugTestEncryptionKey[i] = uint8(i);
	}

	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		// 客户端在连接服务器前将 ?EncryptionToken=xxx 添加到 URL
		SessionSubsystem->OnPreClientTravelEvent.AddUObject(this, &UEqZeroGameInstance::OnPreClientTravelToSession);
	}

	// 初始化 TypeScript/JavaScript 环境 (Puerts)
	// TSDebugPort > 0 时启用调试，Chrome浏览器打开: chrome://inspect
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (TSDebugPort > 0)
	{
		// 启用调试模式：创建带调试端口的JsEnv
		GameScript = MakeShared<puerts::FJsEnv>(
			std::make_unique<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")),
			std::make_shared<puerts::FDefaultLogger>(),
			TSDebugPort
		);
		GameScript->WaitDebugger(); // 暂停等待调试器连接
		UE_LOG(LogTemp, Warning, TEXT("[Puerts] Debug mode enabled on port %d, waiting for debugger..."), TSDebugPort);
	}
	else
	{
		// 不启用调试
		GameScript = MakeShared<puerts::FJsEnv>();
	}
#else
	// 发布模式：总是不启用调试
	GameScript = MakeShared<puerts::FJsEnv>();
#endif

	TArray<TPair<FString, UObject*>> Arguments;
	Arguments.Add(TPair<FString, UObject*>("GameInstance", this));
	GameScript->Start("Main", Arguments);
}

void UEqZeroGameInstance::Shutdown()
{
	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->OnPreClientTravelEvent.RemoveAll(this);
	}

	Super::Shutdown();
	GameScript.Reset();
}

// - 服务器 收到客户端的加密请求，返回加密密钥
// 这是一个简单的实现，用于演示如何使用硬编码密钥对游戏流量进行加密。
// 对于完整的实现，你可能希望从安全的来源获取加密密钥，
// 例如通过 HTTPS 从网络服务获取。这可以在该函数中完成，甚至可以异步进行 —— 只需
// 在已知密钥后调用传入的响应委托即可。EncryptionToken（加密令牌）的内容由用户决定，
// 但通常会包含用于生成唯一加密密钥的信息，例如用户 ID 和 / 或会话 ID。
void UEqZeroGameInstance::ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	FEncryptionKeyResponse Response(EEncryptionResponse::Failure, TEXT("Unknown encryption failure"));

	if (EncryptionToken.IsEmpty())
	{
		Response.Response = EEncryptionResponse::InvalidToken;
		Response.ErrorMsg = TEXT("Encryption token is empty.");
	}
	else
	{
#if UE_WITH_DTLS
		if (EqZero::bUseDTLSEncryption)
		{
			TSharedPtr<FDTLSCertificate> Cert;

			if (EqZero::bTestDTLSFingerprint)
			{
				// Generate a unique server certificate for this connection
				// The fingerprint will be written to a file to simulate an online service
				FTimespan CertExpire = FTimespan::FromHours(4);
				Cert = FDTLSCertStore::Get().CreateCert(CertExpire, EncryptionToken);
			}
			else
			{
				// Load a pre-generated certificate from disk (for testing only)
				const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / TEXT("EqZeroTest.pem");

				Cert = FDTLSCertStore::Get().GetCert(EncryptionToken);

				if (!Cert.IsValid())
				{
					Cert = FDTLSCertStore::Get().ImportCert(CertPath, EncryptionToken);
				}
			}

			if (Cert.IsValid())
			{
				if (EqZero::bTestDTLSFingerprint)
				{
					// In production, the fingerprint should be posted to a secure web service
					// Writing to disk here for local testing purposes only
					TArrayView<const uint8> Fingerprint = Cert->GetFingerprint();

					FString DebugFile = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("DTLS")) / FPaths::MakeValidFileName(EncryptionToken) + TEXT("_server.txt");

					FString FingerprintStr = BytesToHex(Fingerprint.GetData(), Fingerprint.Num());
					FFileHelper::SaveStringToFile(FingerprintStr, *DebugFile);
				}

				// Set up the encryption data for the connection
				Response.EncryptionData.Identifier = EncryptionToken;
				Response.EncryptionData.Key = DebugTestEncryptionKey;

				Response.Response = EEncryptionResponse::Success;
			}
			else
			{
				Response.Response = EEncryptionResponse::Failure;
				Response.ErrorMsg = TEXT("Unable to obtain certificate.");
			}
		}
		else
#endif // UE_WITH_DTLS
		{
			// Simple AES encryption without DTLS
			// Just use the hardcoded debug key
			Response.Response = EEncryptionResponse::Success;
			Response.EncryptionData.Key = DebugTestEncryptionKey;
		}
	}

	Delegate.ExecuteIfBound(Response);
}

// - 客户端 收到服务器确认后，设置客户端的加密密钥
// 这是一个简单的实现，用于演示如何使用硬编码密钥对游戏流量进行加密。
// 对于完整的实现，你可能希望从安全的来源获取加密密钥，
// 例如通过 HTTPS 从 Web 服务获取。这可以在该函数中完成，甚至可以异步进行 —— 只需
// 在获知密钥后调用传入的响应委托即可。
void UEqZeroGameInstance::ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate)
{
	FEncryptionKeyResponse Response;

#if UE_WITH_DTLS
	if (EqZero::bUseDTLSEncryption)
	{
		Response.Response = EEncryptionResponse::Failure;

		APlayerController* const PlayerController = GetFirstLocalPlayerController();

		if (PlayerController && PlayerController->PlayerState && PlayerController->PlayerState->GetUniqueId().IsValid())
		{
			const FUniqueNetIdRepl& PlayerUniqueId = PlayerController->PlayerState->GetUniqueId();

			// Rebuild the encryption token from player ID
			// In production, this token would be passed directly rather than rebuilt
			const FString EncryptionToken = PlayerUniqueId.ToString();

			Response.EncryptionData.Identifier = EncryptionToken;

			if (EqZero::bTestDTLSFingerprint)
			{
				// Load the server's fingerprint from the test file
				// In production, this would come from a secure web service
				FString DebugFile = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("DTLS")) / FPaths::MakeValidFileName(EncryptionToken) + TEXT("_server.txt");

				FString FingerprintStr;
				FFileHelper::LoadFileToString(FingerprintStr, *DebugFile);

				Response.EncryptionData.Fingerprint.AddUninitialized(FingerprintStr.Len() / 2);
				HexToBytes(FingerprintStr, Response.EncryptionData.Fingerprint.GetData());
			}
			else
			{
				// Load certificate from disk to get fingerprint
				const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / TEXT("EqZeroTest.pem");

				TSharedPtr<FDTLSCertificate> Cert = FDTLSCertStore::Get().GetCert(EncryptionToken);
				if (!Cert.IsValid())
				{
					Cert = FDTLSCertStore::Get().ImportCert(CertPath, EncryptionToken);
				}

				if (Cert.IsValid())
				{
					TArrayView<const uint8> Fingerprint = Cert->GetFingerprint();
					Response.EncryptionData.Fingerprint = Fingerprint;
				}
				else
				{
					Response.Response = EEncryptionResponse::Failure;
					Response.ErrorMsg = TEXT("Unable to obtain certificate.");
				}
			}

			Response.EncryptionData.Key = DebugTestEncryptionKey;
			Response.Response = EEncryptionResponse::Success;
		}
	}
	else
#endif // UE_WITH_DTLS
	{
		// Simple AES encryption - just use the debug key
		Response.Response = EEncryptionResponse::Success;
		Response.EncryptionData.Key = DebugTestEncryptionKey;
	}

	Delegate.ExecuteIfBound(Response);
}

void UEqZeroGameInstance::OnPreClientTravelToSession(FString& URL)
{
	// Add encryption token to connection URL if test encryption is enabled
	if (EqZero::bTestEncryption)
	{
#if UE_WITH_DTLS
		if (EqZero::bUseDTLSEncryption)
		{
			// Use player's unique ID as the encryption token for DTLS
			APlayerController* const PlayerController = GetFirstLocalPlayerController();

			if (PlayerController && PlayerController->PlayerState && PlayerController->PlayerState->GetUniqueId().IsValid())
			{
				const FUniqueNetIdRepl& PlayerUniqueId = PlayerController->PlayerState->GetUniqueId();
				const FString EncryptionToken = PlayerUniqueId.ToString();

				URL += TEXT("?EncryptionToken=") + EncryptionToken;
			}
		}
		else
#endif // UE_WITH_DTLS
		{
			// Simple test token - server will use the same hardcoded key regardless
			// In production, this could be a user ID or session ID used to lookup a unique key
			URL += TEXT("?EncryptionToken=1");
		}
	}
}
