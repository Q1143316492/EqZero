// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Engine/DataAsset.h"

#include "EqZeroPawnData.generated.h"

class APawn;
class UEqZeroAbilitySet;
class UEqZeroAbilityTagRelationshipMapping;
class UEqZeroCameraMode;
class UEqZeroInputConfig;
class UObject;


/**
 * UEqZeroPawnData
 *
 * 体验中和Pawn相关的数据资产
 * 包括Pawn类, 默认技能关系映射配置, 输入配置, 默认摄像机模式等
 */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "EqZero Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class UEqZeroPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UEqZeroPawnData(const FObjectInitializer& ObjectInitializer);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pawn")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Abilities")
	TArray<TObjectPtr<UEqZeroAbilitySet>> AbilitySets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Abilities")
	TObjectPtr<UEqZeroAbilityTagRelationshipMapping> TagRelationshipMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Input")
	TObjectPtr<UEqZeroInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Camera")
	TSubclassOf<UEqZeroCameraMode> DefaultCameraMode;
};
