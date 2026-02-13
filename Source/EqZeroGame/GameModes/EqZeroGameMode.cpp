// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameMode.h"

#include "EqZeroGameState.h"
#include "EqZeroLogChannels.h"
#include "EqZeroWorldSettings.h"
#include "System/EqZeroGameSession.h"
#include "EqZeroExperienceDefinition.h"
#include "EqZeroExperienceManagerComponent.h"
#include "EqZeroUserFacingExperienceDefinition.h"
#include "System/EqZeroGameInstance.h"
#include "System/EqZeroAssetManager.h"
#include "Player/EqZeroPlayerController.h"
#include "Character/EqZeroPawnData.h"
#include "Character/EqZeroCharacter.h"
#include "UI/EqZeroHUD.h"
#include "Player/EqZeroPlayerState.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Development/EqZeroDeveloperSettings.h"
#include "CommonUserSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "GameMapsSettings.h"
#include "Character/EqZeroPawnExtensionComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Player/EqZeroPlayerSpawningManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameMode)

AEqZeroGameMode::AEqZeroGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AEqZeroGameState::StaticClass();
	GameSessionClass = AEqZeroGameSession::StaticClass();
	PlayerControllerClass = AEqZeroPlayerController::StaticClass();
	// ReplaySpectatorPlayerControllerClass = AEqZeroReplaySpectatorPlayerController::StaticClass(); 先不搞
	PlayerStateClass = AEqZeroPlayerState::StaticClass();
	DefaultPawnClass = AEqZeroCharacter::StaticClass();
	HUDClass = AEqZeroHUD::StaticClass();
}

////////////////////////////////////////////////////////////////////////////
//
//	体验相关
//
////////////////////////////////////////////////////////////////////////////

const UEqZeroPawnData* AEqZeroGameMode::GetPawnDataForController(const AController* InController) const
{
	// 检查 pawn data 是否已经设置在 player state 上
	if (InController)
	{
		if (const AEqZeroPlayerState* EqZeroPS = InController->GetPlayerState<AEqZeroPlayerState>())
		{
			if (const UEqZeroPawnData* PawnData = EqZeroPS->GetPawnData<UEqZeroPawnData>())
			{
				return PawnData;
			}
		}
	}

	// 如果没有，回退到当前体验的默认值
	check(GameState);
	UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	check(ExperienceComponent);

	if (ExperienceComponent->IsExperienceLoaded())
	{
		const UEqZeroExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData != nullptr)
		{
			return Experience->DefaultPawnData;
		}

		// 体验已加载但仍然没有 pawn data，暂时回退到默认值
		return UEqZeroAssetManager::Get().GetDefaultPawnData();
	}

	// 体验尚未加载，所以没有可用的 pawn data
	return nullptr;
}

void AEqZeroGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void AEqZeroGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;

	// Precedence order (highest wins)
	// 优先级顺序（从高到低）
	//  - Matchmaking assignment (if present) / 匹配分配（如果存在）
	//  - URL Options override / URL 选项覆盖
	//  - Developer Settings (PIE only) / 开发者设置（仅限PIE）
	//  - Command Line override / 命令行覆盖
	//  - World Settings / 世界设置
	//  - Default experience / 默认体验

	UWorld* World = GetWorld();

	// 检查 URL 选项
	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UEqZeroExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	// 检查开发者设置（仅限PIE）
	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UEqZeroDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}

	// 检查命令行是否设置了体验
	if (!ExperienceId.IsValid())
	{
		FString ExperienceFromCommandLine;
		if (FParse::Value(FCommandLine::Get(), TEXT("Experience="), ExperienceFromCommandLine))
		{
			ExperienceId = FPrimaryAssetId::ParseTypeAndName(ExperienceFromCommandLine);
			if (!ExperienceId.PrimaryAssetType.IsValid())
			{
				ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UEqZeroExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromCommandLine));
			}
			ExperienceIdSource = TEXT("CommandLine");
		}
	}

	// 检查世界设置是否有默认体验
	if (!ExperienceId.IsValid())
	{
		if (AEqZeroWorldSettings* TypedWorldSettings = Cast<AEqZeroWorldSettings>(GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	UEqZeroAssetManager& AssetManager = UEqZeroAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogEqZeroExperience, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"), *ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}

	// 最终回退到默认体验
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// This will start to host as a dedicated server
			return;
		}

		// TODO: Pull this from a config setting or something
		// 从配置设置中获取默认体验
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("EqZeroExperienceDefinition"), FName("B_EqDefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

bool AEqZeroGameMode::TryDedicatedServerLogin()
{
	// 一些基本代码用于注册为活动的专用服务器，游戏会对其进行大量修改
	FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	{
		// Only register if this is the default map on a dedicated server
		// 仅在专用服务器上的默认地图时注册
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

		// Dedicated servers may need to do an online login
		// 专用服务器可能需要进行在线登录
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &AEqZeroGameMode::OnUserInitializedForDedicatedServer);

		// There are no local users on dedicated server, but index 0 means the default platform user which is handled by the online login code
		// 专用服务器上没有本地用户，但索引 0 意味着默认平台用户，由在线登录代码处理
		if (!UserSubsystem->TryToLoginForOnlinePlay(0))
		{
			OnUserInitializedForDedicatedServer(nullptr, false, FText(), ECommonUserPrivilege::CanPlayOnline, ECommonUserOnlineContext::Default);
		}

		return true;
	}

	return false;
}

void AEqZeroGameMode::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
{
	FPrimaryAssetType UserExperienceType = UEqZeroUserFacingExperienceDefinition::StaticClass()->GetFName();
	
	// Figure out what UserFacingExperience to load
	FPrimaryAssetId UserExperienceId;
	FString UserExperienceFromCommandLine;
	if (FParse::Value(FCommandLine::Get(), TEXT("UserExperience="), UserExperienceFromCommandLine) ||
		FParse::Value(FCommandLine::Get(), TEXT("Playlist="), UserExperienceFromCommandLine))
	{
		UserExperienceId = FPrimaryAssetId::ParseTypeAndName(UserExperienceFromCommandLine);
		if (!UserExperienceId.PrimaryAssetType.IsValid())
		{
			UserExperienceId = FPrimaryAssetId(FPrimaryAssetType(UserExperienceType), FName(*UserExperienceFromCommandLine));
		}
	}

	// Search for the matching experience, it's fine to force load them because we're in dedicated server startup
	UEqZeroAssetManager& AssetManager = UEqZeroAssetManager::Get();
	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssetsWithType(UserExperienceType);
	if (ensure(Handle.IsValid()))
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> UserExperiences;
	AssetManager.GetPrimaryAssetObjectList(UserExperienceType, UserExperiences);
	UEqZeroUserFacingExperienceDefinition* FoundExperience = nullptr;
	UEqZeroUserFacingExperienceDefinition* DefaultExperience = nullptr;

	for (UObject* Object : UserExperiences)
	{
		UEqZeroUserFacingExperienceDefinition* UserExperience = Cast<UEqZeroUserFacingExperienceDefinition>(Object);
		if (ensure(UserExperience))
		{
			if (UserExperience->GetPrimaryAssetId() == UserExperienceId)
			{
				FoundExperience = UserExperience;
				break;
			}
			
			if (UserExperience->bIsDefaultExperience && DefaultExperience == nullptr)
			{
				DefaultExperience = UserExperience;
			}
		}
	}

	if (FoundExperience == nullptr)
	{
		FoundExperience = DefaultExperience;
	}
	
	UGameInstance* GameInstance = GetGameInstance();
	if (ensure(FoundExperience && GameInstance))
	{
		// Actually host the game
		UCommonSession_HostSessionRequest* HostRequest = FoundExperience->CreateHostingRequest(this);
		if (ensure(HostRequest))
		{
			HostRequest->OnlineMode = OnlineMode;

			// TODO override other parameters?

			UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
			SessionSubsystem->HostSession(nullptr, HostRequest);
			
			// This will handle the map travel
		}
	}
}

void AEqZeroGameMode::OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		// Unbind
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &AEqZeroGameMode::OnUserInitializedForDedicatedServer);

		// Dedicated servers do not require user login, but some online subsystems may expect it
		if (bSuccess && ensure(UserInfo))
		{
			UE_LOG(LogEqZeroExperience, Log, TEXT("Dedicated server user login succeeded for id %s, starting online server"), *UserInfo->GetNetId().ToString());
		}
		else
		{
			UE_LOG(LogEqZeroExperience, Log, TEXT("Dedicated server user login unsuccessful, starting online server as login is not required"));
		}
		
		HostDedicatedServerMatch(ECommonSessionOnlineMode::Online);
	}
}

void AEqZeroGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogEqZeroExperience, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogEqZeroExperience, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void AEqZeroGameMode::OnExperienceLoaded(const UEqZeroExperienceDefinition* CurrentExperience)
{
	// Spawn any players that are already attached
	// 生成所有已经连接的玩家
	//@TODO: Here we're handling only *player* controllers, but in GetDefaultPawnClassForController_Implementation we skipped all controllers
	// GetDefaultPawnClassForController_Implementation might only be getting called for players anyways
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

bool AEqZeroGameMode::IsExperienceLoaded() const
{
	check(GameState);
	UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

UClass* AEqZeroGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UEqZeroPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

////////////////////////////////////////////////////////////////////////////
//
//	体验中 玩家生成相关
//
////////////////////////////////////////////////////////////////////////////

APawn* AEqZeroGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
			    if (const UEqZeroPawnData* PawnData = GetPawnDataForController(NewPlayer))
			    {
			        PawnExtComp->SetPawnData(PawnData);
			    }
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
	}

	return nullptr;
}

bool AEqZeroGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn transform
	// 我们从不想使用起始点，总是使用生成变换
	return false;
}

void AEqZeroGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Delay starting new players until the experience has been loaded
	// (players who log in prior to that will be started by OnExperienceLoaded)
	// 延迟启动新玩家，直到体验加载完成
	// (在体验加载完成之前登录的玩家将由 OnExperienceLoaded 启动)
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

AActor* AEqZeroGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}
	
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AEqZeroGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}
	
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AEqZeroGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

bool AEqZeroGameMode::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		// Bot 版本的 Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			return false;
		}
	}

	if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ControllerCanRestart(Controller);
	}
	return true;
}

void AEqZeroGameMode::InitGameState()
{
	Super::InitGameState();

	// 监听体验加载完成事件
	UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnEqZeroExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void AEqZeroGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AEqZeroGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && Controller)
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
}

bool AEqZeroGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void AEqZeroGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// TODO: 添加重试逻辑，如果玩家生成失败，在下一帧再次尝试
}