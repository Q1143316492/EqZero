// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "EqZeroGameplayAbilityTargetData_SingleTargetHit.generated.h"

class FArchive;
struct FGameplayEffectContextHandle;

/**
 * 对 SingleTargetHit 进行游戏特定的扩展，增加了 CartridgeID 字段来标识同一弹夹中的多个子弹。
 */
USTRUCT()
struct FEqZeroGameplayAbilityTargetData_SingleTargetHit : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	FEqZeroGameplayAbilityTargetData_SingleTargetHit()
		: CartridgeID(-1)
	{ }

	virtual void AddTargetDataToContext(FGameplayEffectContextHandle& Context, bool bIncludeActorArray) const override;

	/**
	 * 简单来说就是散弹枪多子弹同一个标记，一起计算
	 */
	UPROPERTY()
	int32 CartridgeID;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FEqZeroGameplayAbilityTargetData_SingleTargetHit::StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FEqZeroGameplayAbilityTargetData_SingleTargetHit> : public TStructOpsTypeTraitsBase2<FEqZeroGameplayAbilityTargetData_SingleTargetHit>
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};
