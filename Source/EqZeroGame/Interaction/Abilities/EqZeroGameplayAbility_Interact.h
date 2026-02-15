// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/EqZeroGameplayAbility.h"
#include "Interaction/InteractionOption.h"

#include "EqZeroGameplayAbility_Interact.generated.h"

class UIndicatorDescriptor;
class UObject;
class UUserWidget;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;

/**
 * UEqZeroGameplayAbility_Interact
 *
 * 角色的交互技能。
 */
UCLASS(Abstract)
class UEqZeroGameplayAbility_Interact : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable)
	void UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions);

	UFUNCTION(BlueprintCallable)
	void TriggerInteraction();

protected:
	UPROPERTY(BlueprintReadWrite)
	TArray<FInteractionOption> CurrentOptions;

	// UPROPERTY()
	// TArray<TObjectPtr<UIndicatorDescriptor>> Indicators;

protected:

	UPROPERTY(EditDefaultsOnly)
	float InteractionScanRate = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	float InteractionScanRange = 500;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UUserWidget> DefaultInteractionWidgetClass;
};
