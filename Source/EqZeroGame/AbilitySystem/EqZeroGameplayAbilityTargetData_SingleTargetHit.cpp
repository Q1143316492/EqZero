// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbilityTargetData_SingleTargetHit.h"

#include "EqZeroGameplayEffectContext.h"
#include "GameplayEffectTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbilityTargetData_SingleTargetHit)

void FEqZeroGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(FGameplayEffectContextHandle& Context, bool bIncludeActorArray) const
{
	FGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(Context, bIncludeActorArray);
	
	if (FEqZeroGameplayEffectContext* TypedContext = FEqZeroGameplayEffectContext::ExtractEffectContext(Context))
	{
		TypedContext->CartridgeID = CartridgeID;
	}
}

bool FEqZeroGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);
	
	Ar << CartridgeID;

	return true;
}
