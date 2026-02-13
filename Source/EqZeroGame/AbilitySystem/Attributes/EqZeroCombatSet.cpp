// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCombatSet.h"

#include "AbilitySystem/Attributes/EqZeroAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCombatSet)

class FLifetimeProperty;


UEqZeroCombatSet::UEqZeroCombatSet()
	: BaseDamage(6.0f)
	, BaseHeal(0.0f)
{
}

void UEqZeroCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UEqZeroCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEqZeroCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UEqZeroCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UEqZeroCombatSet, BaseDamage, OldValue);
}

void UEqZeroCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UEqZeroCombatSet, BaseHeal, OldValue);
}
