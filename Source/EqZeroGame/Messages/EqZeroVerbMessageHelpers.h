// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "EqZeroVerbMessageHelpers.generated.h"

#define UE_API EQZEROGAME_API

struct FGameplayCueParameters;
struct FEqZeroVerbMessage;

class APlayerController;
class APlayerState;
class UObject;
struct FFrame;

/**
 * UEqZeroVerbMessageHelpers
 */
UCLASS(MinimalAPI)
class UEqZeroVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EqZero")
	static UE_API APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "EqZero")
	static UE_API APlayerController* GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "EqZero")
	static UE_API FGameplayCueParameters VerbMessageToCueParameters(const FEqZeroVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "EqZero")
	static UE_API FEqZeroVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};

#undef UE_API