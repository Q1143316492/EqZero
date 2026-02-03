// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EqZeroAnimationTypes.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EEqZeroCardinalDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EEqZeroRootYawOffsetMode : uint8
{
	None, // Placeholder, update based on logic
	Accumulate,
	BlendOut,
	Hold
};

USTRUCT(BlueprintType)
struct FAnimStructCardinalDirections
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> Backward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> Right;
};
