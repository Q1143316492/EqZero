// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameMode.h"

#include "EqZeroGameState.h"
#include "EqZeroLogChannels.h"
#include "EqZeroWorldSettings.h"
#include "EqZeroExperienceDefinition.h"
#include "EqZeroExperienceManagerComponent.h"
#include "EqZeroUserFacingExperienceDefinition.h"
#include "System/EqZeroGameInstance.h"
#include "System/EqZeroAssetManager.h"
#include "Player/EqZeroPlayerController.h"
#include "Character/EqZeroPawnData.h"
#include "UI/EqZeroHUD.h"
#include "Player/EqZeroPlayerState.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Development/EqDeveloperSettings.h"
#include "CommonUserSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "GameMapsSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameMode)

AEqZeroGameMode::AEqZeroGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AEqZeroGameState::StaticClass();
	PlayerControllerClass = AEqZeroPlayerController::StaticClass();
	HUDClass = AEqZeroHUD::StaticClass();
	PlayerStateClass = AEqZeroPlayerState::StaticClass();
	DefaultPawnClass = nullptr; // Uses PawnData
	SpectatorClass = nullptr;
}

const UEqZeroPawnData* AEqZeroGameMode::GetPawnDataForController(const AController* InController) const
{
	// See if the pawn data is already set on the player state
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

	// If not, fall back to the default for the current experience
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

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		// 体验已加载但仍然没有 pawn data，暂时回退到默认值
		return UEqZeroAssetManager::Get().GetDefaultPawnData();
	}

	// Experience not loaded yet, so there is no pawn data to be had
	// 体验尚未加载，所以没有可用的 pawn data
	return nullptr;
}

void AEqZeroGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
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
			// TODO: 迁移 UEqZeroPawnExtensionComponent 后，使用 SetPawnData 设置 PawnData
			// if (UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			// {
			//     if (const UEqZeroPawnData* PawnData = GetPawnDataForController(NewPlayer))
			//     {
			//         PawnExtComp->SetPawnData(PawnData);
			//     }
			// }

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
	// TODO: 迁移 UEqZeroPlayerSpawningManagerComponent 后，使用该组件选择玩家生成点
	// if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	// {
	//     return PlayerSpawningComponent->ChoosePlayerStart(Player);
	// }
	
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AEqZeroGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	// TODO: 迁移 UEqZeroPlayerSpawningManagerComponent 后，使用该组件完成玩家重生
	// if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	// {
	//     PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	// }
	
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AEqZeroGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

void AEqZeroGameMode::InitGameState()
{
	Super::InitGameState();

	// Listen for the experience load to complete
	// 监听体验加载完成事件
	UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnEqZeroExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

bool AEqZeroGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void AEqZeroGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AEqZeroGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// TODO: 添加重试逻辑，如果玩家生成失败，在下一帧再次尝试
	// 可以参考 LyraGameMode 中的实现，使用 GetWorldTimerManager().SetTimerForNextTick
	// if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
	// {
	//     if (PlayerCanRestart(NewPC))
	//     {
	//         RequestPlayerRestartNextFrame(NewPlayer, false);
	//     }
	// }
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
		{
			return false;
		}
	}

	// TODO: 迁移 UEqZeroPlayerSpawningManagerComponent 后，使用该组件检查是否可以重生
	// if (UEqZeroPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UEqZeroPlayerSpawningManagerComponent>())
	// {
	//     return PlayerSpawningComponent->ControllerCanRestart(Controller);
	// }

	return true;
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

	// Check URL Options
	// 检查 URL 选项
	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UEqZeroExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	// Check Developer Settings (PIE only)
	// 检查开发者设置（仅限PIE）
	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UEqDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}

	// See if the command line wants to set the experience
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

	// See if the world settings has a default experience
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

	// Final fallback to the default experience
	// 最终回退到默认体验
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// This will start to host as a dedicated server
			return;
		}

		//@TODO: Pull this from a config setting or something
		// 从配置设置中获取默认体验
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("EqZeroExperienceDefinition"), FName("B_EqDefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

bool AEqZeroGameMode::TryDedicatedServerLogin()
{
	// Some basic code to register as an active dedicated server, this would be heavily modified by the game
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

void AEqZeroGameMode::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
{
	// TODO: 完善专用服务器体验加载逻辑
	// 参考 LyraGameMode::HostDedicatedServerMatch 实现
	// 需要加载 UserFacingExperience 资产并创建 CommonSession 主机请求
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	
	UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
	if (!SessionSubsystem)
	{
		return;
	}
	
	// 如果启动时指定了地图，使用那个作为默认体验
	FString MapOrExperience;
	if (!FParse::Value(FCommandLine::Get(), TEXT("ExperienceDefinition="), MapOrExperience))
	{
		FString DesiredMap = UGameMapsSettings::GetGameDefaultMap();
		if (!DesiredMap.IsEmpty())
		{
			MapOrExperience = DesiredMap;
		}
	}
	
	if (MapOrExperience.IsEmpty())
	{
		UE_LOG(LogEqZeroExperience, Warning, TEXT("HostDedicatedServerMatch: No experience or map specified for dedicated server"));
		return;
	}
	
	UE_LOG(LogEqZeroExperience, Log, TEXT("HostDedicatedServerMatch: Starting with experience/map: %s"), *MapOrExperience);
	
	// TODO: 查找匹配的 UserFacingExperience 并创建会话请求
	// UEqZeroUserFacingExperienceDefinition* UserFacingExperience = FindUserFacingExperience(MapOrExperience);
	// if (UserFacingExperience)
	// {
	//     UCommonSession_HostSessionRequest* HostRequest = UserFacingExperience->CreateHostingRequest(this);
	//     if (HostRequest)
	//     {
	//         HostRequest->OnlineMode = OnlineMode;
	//         SessionSubsystem->HostSession(nullptr, HostRequest);
	//     }
	// }
}
