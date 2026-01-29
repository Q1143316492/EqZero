// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAttributeSet.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAttributeSet)

class UWorld;


UEqZeroAttributeSet::UEqZeroAttributeSet()
{
}

UWorld* UEqZeroAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UEqZeroAbilitySystemComponent* UEqZeroAttributeSet::GetEqZeroAbilitySystemComponent() const
{
	return Cast<UEqZeroAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
