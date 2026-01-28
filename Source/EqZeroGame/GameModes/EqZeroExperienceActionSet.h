// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Engine/DataAsset.h"
#include "EqZeroExperienceActionSet.generated.h"

class UGameFeatureAction;

/**
 * 技能集, 一组UGameFeatureAction 和 一组名字
 */
UCLASS(BlueprintType, NotBlueprintable)
class UEqZeroExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEqZeroExperienceActionSet();

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
	UPROPERTY(EditAnywhere, Instanced, Category="Actions to Perform")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	UPROPERTY(EditAnywhere, Category="Feature Dependencies")
	TArray<FString> GameFeaturesToEnable;
};
