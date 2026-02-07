// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAbilityCost_ItemTagStack.h"

// TODO: References to Equipment and Inventory
// #include "Equipment/EqZeroGameplayAbility_FromEquipment.h"
// #include "Inventory/EqZeroInventoryItemInstance.h"
// #include "NativeGameplayTags.h" // TODO: Add this when NativeGameplayTags is migrated

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAbilityCost_ItemTagStack)

// UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost"); // TODO: Define this tag

UEqZeroAbilityCost_ItemTagStack::UEqZeroAbilityCost_ItemTagStack()
{
	Quantity.SetValue(1.0f);
	// FailureTag = TAG_ABILITY_FAIL_COST; // TODO: Assign tag
}

bool UEqZeroAbilityCost_ItemTagStack::CheckCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
#if 0 // TODO: Implement Equipment system
	if (const UEqZeroGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UEqZeroGameplayAbility_FromEquipment>(Ability))
	{
		if (UEqZeroInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

			const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;

			// Inform other abilities why this cost cannot be applied
			if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailureTag);
			}
			return bCanApplyCost;
		}
	}
#endif
	return false;
}

void UEqZeroAbilityCost_ItemTagStack::ApplyCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
#if 0 // TODO: Implement Equipment system
	if (ActorInfo->IsNetAuthority())
	{
		if (const UEqZeroGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UEqZeroGameplayAbility_FromEquipment>(Ability))
		{
			if (UEqZeroInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				ItemInstance->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
#endif
}
