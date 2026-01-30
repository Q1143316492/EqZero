// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EqZeroSettingsShared.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEqZeroSettingChanged, UEqZeroSettingsShared*);

/**
 * UEqZeroSettingsShared
 *
 * Store settings shared between local players
 */
UCLASS(config=Game, defaultconfig)
class EQZEROGAME_API UEqZeroSettingsShared : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroSettingsShared() = default;

	UFUNCTION()
	bool GetForceFeedbackEnabled() const { return bForceFeedbackEnabled; }
	
	FOnEqZeroSettingChanged OnSettingChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game|Feedback")
	uint32 bForceFeedbackEnabled:1;
};
