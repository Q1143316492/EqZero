// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "EqZeroAimSensitivityData.generated.h"

#define UE_API EQZEROGAME_API

enum class EEqZeroGamepadSensitivity : uint8;

class UObject;

/** Defines a set of gamepad sensitivity to a float value. */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "EqZero Aim Sensitivity Data", ShortTooltip = "Data asset used to define a map of Gamepad Sensitivty to a float value."))
class UEqZeroAimSensitivityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API UEqZeroAimSensitivityData(const FObjectInitializer& ObjectInitializer);

	UE_API const float SensitivtyEnumToFloat(const EEqZeroGamepadSensitivity InSensitivity) const;

protected:
	/** Map of SensitivityMap settings to their corresponding float */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EEqZeroGamepadSensitivity, float> SensitivityMap;
};

#undef UE_API
