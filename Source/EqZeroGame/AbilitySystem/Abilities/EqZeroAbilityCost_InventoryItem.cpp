// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAbilityCost_InventoryItem.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySpecHandle.h"
// #include "Inventory/EqZeroInventoryManagerComponent.h" // TODO: Add this when Inventory is migrated

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAbilityCost_InventoryItem)

UEqZeroAbilityCost_InventoryItem::UEqZeroAbilityCost_InventoryItem()
{
	Quantity.SetValue(1.0f);
}

bool UEqZeroAbilityCost_InventoryItem::CheckCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
#if 0 // TODO: Enable when Inventory system is migrated
	if (AController* PC = Ability->GetControllerFromActorInfo())
	{
		if (UEqZeroInventoryManagerComponent* InventoryComponent = PC->GetComponentByClass<UEqZeroInventoryManagerComponent>())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumItemsToConsumeReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumItemsToConsume = FMath::TruncToInt(NumItemsToConsumeReal);

			return InventoryComponent->GetTotalItemCountByDefinition(ItemDefinition) >= NumItemsToConsume;
		}
	}
#endif
	return false;
}

void UEqZeroAbilityCost_InventoryItem::ApplyCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
#if 0 // TODO: Enable when Inventory system is migrated
	if (ActorInfo->IsNetAuthority())
	{
		if (AController* PC = Ability->GetControllerFromActorInfo())
		{
			if (UEqZeroInventoryManagerComponent* InventoryComponent = PC->GetComponentByClass<UEqZeroInventoryManagerComponent>())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumItemsToConsumeReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumItemsToConsume = FMath::TruncToInt(NumItemsToConsumeReal);

				InventoryComponent->ConsumeItemsByDefinition(ItemDefinition, NumItemsToConsume);
			}
		}
	}
#endif
}
