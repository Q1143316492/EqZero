// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "EqZeroAbilityCost.generated.h"

class UEqZeroGameplayAbility;

/**
 * UEqZeroAbilityCost
 *
 * 我们自己定义的一个花费的数据结构的基类
 * Base class for costs that a EqZeroGameplayAbility has (e.g., ammo or charges)
 */
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UEqZeroAbilityCost : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroAbilityCost() {}

	virtual bool CheckCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	virtual void ApplyCost(const UEqZeroGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
	{
	}

	bool ShouldOnlyApplyCostOnHit() const { return bOnlyApplyCostOnHit; }

protected:
	/** 只有在技能命中才扣花费的一个标记 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	bool bOnlyApplyCostOnHit = false;
};
