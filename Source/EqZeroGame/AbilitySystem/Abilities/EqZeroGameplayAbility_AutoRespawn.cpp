// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/EqZeroGameplayAbility_AutoRespawn.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Abilities/EqZeroGameplayAbility_Reset.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "EqZeroGameplayTags.h"
#include "Character/EqZeroHealthComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/EqZeroGameMode.h"
#include "GameModes/EqZeroGameState.h"
#include "Messages/EqZeroVerbMessage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_AutoRespawn)

UEqZeroGameplayAbility_AutoRespawn::UEqZeroGameplayAbility_AutoRespawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EEqZeroAbilityActivationPolicy::OnSpawn;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	
	bLogCancelation = true;

	// 如果设置了此项，客户端版本可以取消该能力的服务器端版本。服务器始终可以取消客户端版本
	bServerRespectsRemoteAbilityCancellation = false;
	// 如果为真，并且尝试激活一个已经处于激活状态的实例化能力，就结束它并重新触发它
	bRetriggerInstancedAbility = true;

	bIsListeningForReset = false;
	bShouldFinishRestart = false;
}

AActor* UEqZeroGameplayAbility_AutoRespawn::GetOwningPlayerState() const
{
	if (auto ActorInfo = GetEqZeroAbilitySystemComponentFromActorInfo())
	{
		return ActorInfo->GetOwner();
	}
	return nullptr;
}

bool UEqZeroGameplayAbility_AutoRespawn::IsAvatarDeadOrDying() const
{
	if (auto AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (auto HealthComponent = AvatarActor->FindComponentByClass<UEqZeroHealthComponent>())
		{
			return HealthComponent->IsDeadOrDying();
		}
	}
	return false;
}


void UEqZeroGameplayAbility_AutoRespawn::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);
	ClearDeathListener();

	if (bIsListeningForReset)
	{
		ListenerHandle.Unregister();
		bIsListeningForReset = false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_Respawn);
		World->GetTimerManager().ClearTimer(TimerHandle_ResetRequest);
	}
}

void UEqZeroGameplayAbility_AutoRespawn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	InternalActivateAutoRespawnAbility();
}

void UEqZeroGameplayAbility_AutoRespawn::OnPawnAvatarSet()
{
	Super::OnPawnAvatarSet();
	InternalActivateAutoRespawnAbility();
}

void UEqZeroGameplayAbility_AutoRespawn::InternalActivateAutoRespawnAbility()
{
	if (!bIsListeningForReset)
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		ListenerHandle = MessageSubsystem.RegisterListener(EqZeroGameplayTags::GameplayEvent_Reset, this, &ThisClass::OnPlayerResetMessage);
		bIsListeningForReset = true;
	}

	if (IsAvatarDeadOrDying())
	{
		OnDeathStarted(GetAvatarActorFromActorInfo());
	}
	else
	{
		BindDeathListener();
	}
}

void UEqZeroGameplayAbility_AutoRespawn::OnPlayerResetMessage(FGameplayTag Channel, const FEqZeroPlayerResetMessage& Payload)
{
	if (Payload.OwnerPlayerState == GetOwningActorFromActorInfo())
	{
		OnPlayerReset();
	}
}

void UEqZeroGameplayAbility_AutoRespawn::OnAvatarEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	if (Reason == EEndPlayReason::Destroyed)
	{
		OnPlayerReset();	
	}
}

void UEqZeroGameplayAbility_AutoRespawn::OnPlayerReset()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	AController* ControllerToReset = GetControllerFromActorInfo();
	if (!ControllerToReset)
	{
		return;
	}

	bShouldFinishRestart = false;
	if (auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControllerToReset->PlayerState))
	{
		TArray<FGameplayAbilitySpecHandle> OutAbilityHandles;
		ASC->FindAllAbilitiesWithTags(
			OutAbilityHandles,
			FGameplayTagContainer(EqZeroGameplayTags::Ability_Type_StatusChange_Death), true);

		for (const FGameplayAbilitySpecHandle& Handle : OutAbilityHandles)
		{
			if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
			{
				if (UEqZeroGameplayAbility* AbilityInstance = Cast<UEqZeroGameplayAbility>(Spec->GetPrimaryInstance()))
				{
					AbilityInstance->ForceEndAbility(Handle, AbilityInstance->GetCurrentActorInfo(), AbilityInstance->GetCurrentActivationInfo(), true, false);
				}
			}
		}
	}

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(this, [this, ControllerToReset]()
	{
		if (auto GameMode = GetWorld()->GetAuthGameMode<AEqZeroGameMode>())
		{
			GameMode->RequestPlayerRestartNextFrame(ControllerToReset, true);
		}

		FEqZeroVerbMessage Message;
		Message.Instigator = GetOwningPlayerState();

		if (auto GameState = GetWorld()->GetGameState<AEqZeroGameState>())
		{
			GameState->MulticastMessageToClients(Message);
		}

		if (GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			// Ability.Respawn.Completed.Message
			UGameplayMessageSubsystem::Get(this).BroadcastMessage(
				EqZeroGameplayTags::Ability_Respawn_Completed_Message, Message);
		}
	});
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetRequest, TimerDelegate, 0.1f, false);
}

void UEqZeroGameplayAbility_AutoRespawn::OnDeathStarted(AActor* OwningActor)
{
	ClearDeathListener();

	AController* ControllerToReset = GetControllerFromActorInfo();
	if (!ControllerToReset)
	{
		return;
	}

	if (AActor* Instigator = GetOwningPlayerState())
	{
		FEqZeroInteractionDurationMessage DurationMessage;
		DurationMessage.Instigator = Instigator;
		DurationMessage.Duration = RespawnDelayDuration;

		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.BroadcastMessage(EqZeroGameplayTags::Ability_Respawn_Duration_Message, DurationMessage);
	}

	if (HasAuthority(&CurrentActivationInfo))
	{
		bShouldFinishRestart = true;

		FTimerDelegate RespawnDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (bShouldFinishRestart)
			{
				bShouldFinishRestart = false;
				OnPlayerReset();
			}
		});
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Respawn, RespawnDelegate, RespawnDelayDuration, false);
	}
}

void UEqZeroGameplayAbility_AutoRespawn::BindDeathListener()
{
	ClearDeathListener();

	LastBoundAvatarActor = GetAvatarActorFromActorInfo();
	if (LastBoundAvatarActor)
	{
		LastBoundAvatarActor->OnEndPlay.AddDynamic(this, &ThisClass::OnAvatarEndPlay);
		LastBoundHealthComponent = LastBoundAvatarActor->FindComponentByClass<UEqZeroHealthComponent>();
		if (LastBoundHealthComponent)
		{
			LastBoundHealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
		}
	}
}

void UEqZeroGameplayAbility_AutoRespawn::ClearDeathListener()
{
	if (LastBoundAvatarActor)
	{
		LastBoundAvatarActor->OnEndPlay.RemoveDynamic(this, &ThisClass::OnAvatarEndPlay);
		LastBoundAvatarActor = nullptr;
	}

	if (LastBoundHealthComponent)
	{
		LastBoundHealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::OnDeathStarted);
		LastBoundHealthComponent = nullptr;
	}
}
