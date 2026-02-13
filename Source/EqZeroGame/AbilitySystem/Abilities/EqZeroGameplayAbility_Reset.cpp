// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/EqZeroGameplayAbility_Reset.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Character/EqZeroCharacter.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "EqZeroGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Reset)

UEqZeroGameplayAbility_Reset::UEqZeroGameplayAbility_Reset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = EqZeroGameplayTags::GameplayEvent_RequestReset;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UEqZeroGameplayAbility_Reset::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UEqZeroAbilitySystemComponent* EqZeroASC = CastChecked<UEqZeroAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

    // 取消技能，但要忽略重生技能
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(EqZeroGameplayTags::Ability_Behavior_SurvivesDeath);
	EqZeroASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

    // 角色执行重置
	if (AEqZeroCharacter* EqZeroChar = Cast<AEqZeroCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		EqZeroChar->Reset();
	}

	// 通知其他人重置已发生
	FEqZeroPlayerResetMessage Message;
	Message.OwnerPlayerState = CurrentActorInfo->OwnerActor.Get();
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(EqZeroGameplayTags::GameplayEvent_Reset, Message);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const bool bReplicateEndAbility = true;
	const bool bWasCanceled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCanceled);
}
