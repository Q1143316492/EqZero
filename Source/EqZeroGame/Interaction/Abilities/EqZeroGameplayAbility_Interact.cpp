// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_Interact.h"

#include "AbilitySystemComponent.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionStatics.h"
#include "Interaction/Tasks/AbilityTask_GrantNearbyInteraction.h"
#include "NativeGameplayTags.h"
#include "Player/EqZeroPlayerController.h"
// #include "UI/IndicatorSystem/IndicatorDescriptor.h" TODO UI还没写
// #include "UI/IndicatorSystem/EqZeroIndicatorManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Interact)

class UUserWidget;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Interaction_Activate, "Ability.Interaction.Activate");
UE_DEFINE_GAMEPLAY_TAG(TAG_INTERACTION_DURATION_MESSAGE, "Ability.Interaction.Duration.Message");

UEqZeroGameplayAbility_Interact::UEqZeroGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EEqZeroAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UEqZeroGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystem && AbilitySystem->GetOwnerRole() == ROLE_Authority)
	{
		UAbilityTask_GrantNearbyInteraction* Task = UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(this, InteractionScanRange, InteractionScanRate);
		Task->ReadyForActivation();
	}
}

void UEqZeroGameplayAbility_Interact::UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions)
{
	// TODO 一些UI的东西
	// if (AEqZeroPlayerController* PC = GetEqZeroPlayerControllerFromActorInfo())
	// {
	// 	if (UEqZeroIndicatorManagerComponent* IndicatorManager = UEqZeroIndicatorManagerComponent::GetComponent(PC))
	// 	{
	// 		for (UIndicatorDescriptor* Indicator : Indicators)
	// 		{
	// 			IndicatorManager->RemoveIndicator(Indicator);
	// 		}
	// 		Indicators.Reset();
	//
	// 		for (const FInteractionOption& InteractionOption : InteractiveOptions)
	// 		{
	// 			AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(InteractionOption.InteractableTarget);
	//
	// 			TSoftClassPtr<UUserWidget> InteractionWidgetClass = 
	// 				InteractionOption.InteractionWidgetClass.IsNull() ? DefaultInteractionWidgetClass : InteractionOption.InteractionWidgetClass;
	//
	// 			UIndicatorDescriptor* Indicator = NewObject<UIndicatorDescriptor>();
	// 			Indicator->SetDataObject(InteractableTargetActor);
	// 			Indicator->SetSceneComponent(InteractableTargetActor->GetRootComponent());
	// 			Indicator->SetIndicatorClass(InteractionWidgetClass);
	// 			IndicatorManager->AddIndicator(Indicator);
	//
	// 			Indicators.Add(Indicator);
	// 		}
	// 	}
	// 	else
	// 	{
	// 		//TODO This should probably be a noisy warning.  Why are we updating interactions on a PC that can never do anything with them?
	// 	}
	// }
	//
	CurrentOptions = InteractiveOptions;
}

void UEqZeroGameplayAbility_Interact::TriggerInteraction()
{
	if (CurrentOptions.Num() == 0)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystem)
	{
		const FInteractionOption& InteractionOption = CurrentOptions[0];

		AActor* Instigator = GetAvatarActorFromActorInfo();
		AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(InteractionOption.InteractableTarget);

		// 允许目标自定义我们将要传入的事件数据，以防能力需要只有 Actor 才知道的自定义数据。
		FGameplayEventData Payload;
		Payload.EventTag = TAG_Ability_Interaction_Activate;
		Payload.Instigator = Instigator;
		Payload.Target = InteractableTargetActor;

		// 如果需要，我们允许可交互目标操纵事件数据，例如墙上的按钮可能希望指定一个门 Actor 来执行该能力，因此它可以选择将 Target 覆盖为门 Actor。
		// 目前没用
		InteractionOption.InteractableTarget->CustomizeInteractionEventData(TAG_Ability_Interaction_Activate, Payload);

		// 从有效负载中获取目标 Actor，我们将使用它作为交互的 'Avatar'，并将源 InteractableTarget Actor 作为拥有者 Actor。
		AActor* TargetActor = const_cast<AActor*>(ToRawPtr(Payload.Target));

		FGameplayAbilityActorInfo ActorInfo;
		ActorInfo.InitFromActor(InteractableTargetActor, TargetActor, InteractionOption.TargetAbilitySystem);

		const bool bSuccess = InteractionOption.TargetAbilitySystem->TriggerAbilityFromGameplayEvent(
			InteractionOption.TargetInteractionAbilityHandle,
			&ActorInfo,
			TAG_Ability_Interaction_Activate,
			&Payload,
			*InteractionOption.TargetAbilitySystem
		);
	}
}

