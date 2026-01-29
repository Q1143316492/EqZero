// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "EqZeroCharacterMovementComponent.generated.h"

#define UE_API EQZEROGAME_API

class UObject;
struct FFrame;

EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);

/**
 * FEqZeroCharacterGroundInfo
 *
 *      Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FEqZeroCharacterGroundInfo
{
	GENERATED_BODY()

	FEqZeroCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};


/**
 * UEqZeroCharacterMovementComponent
 *
 *      The base character movement component class used by this project.
 */
UCLASS(MinimalAPI, Config = Game)
class UEqZeroCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UE_API UEqZeroCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	UE_API virtual void SimulateMovement(float DeltaTime) override;

	UE_API virtual bool CanAttemptJump() const override;

	// Returns the current ground info.  Calling this will update the ground info if it's out of date.
	UFUNCTION(BlueprintCallable, Category = "EqZero|CharacterMovement")
	UE_API const FEqZeroCharacterGroundInfo& GetGroundInfo();

	UE_API void SetReplicatedAcceleration(const FVector& InAcceleration);

	//~UMovementComponent interface
	UE_API virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	UE_API virtual float GetMaxSpeed() const override;
	//~End of UMovementComponent interface

protected:

	UE_API virtual void InitializeComponent() override;

protected:

	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FEqZeroCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};

#undef UE_API
