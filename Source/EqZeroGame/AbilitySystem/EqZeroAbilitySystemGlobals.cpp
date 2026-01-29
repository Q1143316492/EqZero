// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroAbilitySystemGlobals.h"

#include "EqZeroGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAbilitySystemGlobals)

struct FGameplayEffectContext;

UEqZeroAbilitySystemGlobals::UEqZeroAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UEqZeroAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FEqZeroGameplayEffectContext();
}
