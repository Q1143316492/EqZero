// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAbilityCost_PlayerTagStack.h"

#include "GameFramework/Controller.h"
#include "EqZeroGameplayAbility.h"
#include "Player/EqZeroPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAbilityCost_PlayerTagStack)

UEqZeroAbilityCost_PlayerTagStack::UEqZeroAbilityCost_PlayerTagStack()
{
	Quantity.SetValue(1.0f);
}

bool UEqZeroAbilityCost_PlayerTagStack::CheckCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (AController* PC = Ability->GetControllerFromActorInfo())
	{
		if (AEqZeroPlayerState* PS = Cast<AEqZeroPlayerState>(PC->PlayerState))
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

			return PS->GetStatTagStackCount(Tag) >= NumStacks;
		}
	}
	return false;
}

void UEqZeroAbilityCost_PlayerTagStack::ApplyCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (AController* PC = Ability->GetControllerFromActorInfo())
		{
			if (AEqZeroPlayerState* PS = Cast<AEqZeroPlayerState>(PC->PlayerState))
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				PS->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}
