// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "EqZeroPawnData.generated.h"

class APawn;
class UEqZeroAbilitySet;
// TODO: Uncomment when created
// class UEqZeroAbilityTagRelationshipMapping;
// class UEqZeroCameraMode;
// class UEqZeroInputConfig;
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
    
	// TODO: Uncomment when created
	// TObjectPtr<UEqZeroAbilityTagRelationshipMapping> TagRelationshipMapping;
	TObjectPtr<UObject> TagRelationshipMapping;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Input")
	// TODO: Uncomment when created
	// TObjectPtr<UEqZeroInputConfig> InputConfig;
	TObjectPtr<UObject> InputConfig;

	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Camera")
	// TODO: Uncomment when created
	// TSubclassOf<UEqZeroCameraMode> DefaultCameraMode;
	TSubclassOf<UObject> DefaultCameraMode;
};
