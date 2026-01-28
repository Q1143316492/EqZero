// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Engine/DataAsset.h"

#include "EqZeroGameData.generated.h"

class UGameplayEffect;
class UObject;

/**
 * UEqZeroGameData
 *
 *	Non-mutable data asset that contains global game data.
 *    不可变的数据资产，包含全局游戏数据。
 *    目前是一些 GE 的配置
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "EqZero Game Data", ShortTooltip = "Data asset containing global game data."))
class EQZEROGAME_API UEqZeroGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UEqZeroGameData();

	static const UEqZeroGameData& Get();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;
};
