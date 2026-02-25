// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroBotCreationComponent.h"
#include "EqZeroGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/EqZeroExperienceManagerComponent.h"
#include "Character/EqZeroPawnExtensionComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Character/EqZeroHealthComponent.h"
#include "Development/EqZeroDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroBotCreationComponent)

UEqZeroBotCreationComponent::UEqZeroBotCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroBotCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// 监听体验加载完成事件
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnEqZeroExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UEqZeroBotCreationComponent::OnExperienceLoaded(const UEqZeroExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
}

#if WITH_SERVER_CODE

void UEqZeroBotCreationComponent::ServerCreateBots_Implementation()
{
	if (BotControllerClass == nullptr)
	{
		return;
	}

	RemainingBotNames = RandomBotNames;

	int32 EffectiveBotCount = NumBotsToCreate;

	if (GIsEditor)
	{
		const UEqZeroDeveloperSettings* DeveloperSettings = GetDefault<UEqZeroDeveloperSettings>();
		
		if (DeveloperSettings->bOverrideBotCount)
		{
			EffectiveBotCount = DeveloperSettings->OverrideNumPlayerBotsToSpawn;
		}
	}
	
	// 给 URL 参数一个覆盖的机会，格式为 "?NumBots=X"
	if (AGameModeBase* GameModeBase = GetGameMode<AGameModeBase>())
	{
		EffectiveBotCount = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, TEXT("NumBots"), EffectiveBotCount);
	}

	// 创建
	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
	{
		SpawnOneBot();
	}
}

// 随机机器人名字，叫啥其实不重要
FString UEqZeroBotCreationComponent::CreateBotName(int32 PlayerIndex)
{
	FString Result;
	if (RemainingBotNames.Num() > 0)
	{
		const int32 NameIndex = FMath::RandRange(0, RemainingBotNames.Num() - 1);
		Result = RemainingBotNames[NameIndex];
		RemainingBotNames.RemoveAtSwap(NameIndex);
	}
	else
	{
		// PlayerId is only being initialized for players right now
		PlayerIndex = FMath::RandRange(260, 260 + 100);
		Result = FString::Printf(TEXT("Bot %d"), PlayerIndex);
	}
	return Result;
}

void UEqZeroBotCreationComponent::SpawnOneBot()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;
	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	if (NewController != nullptr)
	{
		AEqZeroGameMode* GameMode = GetGameMode<AEqZeroGameMode>();
		check(GameMode);

		if (NewController->PlayerState != nullptr) // 通过 bWantsPlayerState 来设置需要PlayerState
		{
			NewController->PlayerState->SetPlayerName(CreateBotName(NewController->PlayerState->GetPlayerId()));
		}

		// 让AI也能跑 OnGameModePlayerInitialized 的流程
		GameMode->GenericPlayerInitialization(NewController);

		// 这里面会找到 FindPlayerStart 然后生成的流程
		GameMode->RestartPlayer(NewController);

		if (NewController->GetPawn() != nullptr)
		{
			if (UEqZeroPawnExtensionComponent* PawnExtComponent = NewController->GetPawn()->FindComponentByClass<UEqZeroPawnExtensionComponent>())
			{
				PawnExtComponent->CheckDefaultInitialization();
			}
		}

		SpawnedBotList.Add(NewController);
	}
}

void UEqZeroBotCreationComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);

		AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex];
		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);

		if (BotToRemove)
		{
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				if (UEqZeroHealthComponent* HealthComponent = UEqZeroHealthComponent::FindHealthComponent(ControlledPawn))
				{
					HealthComponent->DamageSelfDestruct();
				}
				else
				{
					ControlledPawn->Destroy();
				}
			}

			// Destroy the controller (will cause it to Logout, etc...)
			BotToRemove->Destroy();
		}
	}
}

#else // !WITH_SERVER_CODE

void UEqZeroBotCreationComponent::ServerCreateBots_Implementation()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in EqZeroClient!"));
}

void UEqZeroBotCreationComponent::SpawnOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in EqZeroClient!"));
}

void UEqZeroBotCreationComponent::RemoveOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in EqZeroClient!"));
}

#endif
