// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroAbilityCost.h"
#include "ScalableFloat.h"
#include "Templates/SubclassOf.h"

#include "EqZeroAbilityCost_InventoryItem.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;

class UEqZeroGameplayAbility;
// TODO: Implement UEqZeroInventoryItemDefinition when moving Inventory system
// class UEqZeroInventoryItemDefinition;
class UObject;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;

/**
 * Represents a cost that requires expending a quantity of an inventory item
 */
UCLASS(meta=(DisplayName="Inventory Item"))
class UEqZeroAbilityCost_InventoryItem : public UEqZeroAbilityCost
{
	GENERATED_BODY()

public:
	UEqZeroAbilityCost_InventoryItem();

	//~UEqZeroAbilityCost interface
	virtual bool CheckCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UEqZeroAbilityCost interface

protected:
	/** How much of the item to spend (keyed on ability level) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AbilityCost)
	FScalableFloat Quantity;

	// TODO
	/** Which item to consume */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AbilityCost)
	// TSubclassOf<UEqZeroInventoryItemDefinition> ItemDefinition;
};
