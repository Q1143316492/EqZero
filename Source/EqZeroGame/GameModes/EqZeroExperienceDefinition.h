// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "EqZeroExperienceDefinition.generated.h"

class UGameFeatureAction;
class UEqZeroPawnData;
class UEqZeroExperienceActionSet;

/**
 * 体验定义
 */
UCLASS(BlueprintType, Const)
class UEqZeroExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEqZeroExperienceDefinition();

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif
	//~End of UPrimaryDataAsset interface

public:
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TObjectPtr<const UEqZeroPawnData> DefaultPawnData;

	UPROPERTY(EditDefaultsOnly, Instanced, Category="Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<UEqZeroExperienceActionSet>> ActionSets;
};
