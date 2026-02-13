// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "EqZeroAttributeSet.h"

#include "EqZeroCombatSet.generated.h"

class UObject;
struct FFrame;


/**
 * UEqZeroCombatSet
 *
 * 定义施加伤害或治疗所需属性的类。属性示例包括：伤害、治疗量、攻击力和破盾值。
 */
UCLASS(BlueprintType)
class UEqZeroCombatSet : public UEqZeroAttributeSet
{
	GENERATED_BODY()

public:

	UEqZeroCombatSet();

	ATTRIBUTE_ACCESSORS(UEqZeroCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UEqZeroCombatSet, BaseHeal);

protected:

	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "EqZero|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "EqZero|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;
};
