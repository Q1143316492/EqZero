// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "EqZeroAnimationTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "EqZeroAnimInstance.generated.h"

class UAbilitySystemComponent;
class UEqZeroItemAnimInstance;


/**
 * UEqZeroAnimInstance
 */
UCLASS(Config = Game)
class UEqZeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend class UEqZeroItemAnimInstance;

public:

	UEqZeroAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	void UpdateData(float DeltaSeconds);
	void UpdateLocationData(float DeltaSeconds);
	void UpdateRotationData();
	void UpdateVelocityData();
	void UpdateAccelerationData();
	void UpdateWallDetectionHeuristic();
	void UpdateCharacterStateData(float DeltaSeconds);
	void UpdateBlendWeightData(float DeltaSeconds);
	void UpdateAimingData();
	void UpdateJumpFallData();

	/**
	 * Helper Functions
	 */
	UFUNCTION(BlueprintPure, Category = "EqZero|Helper Functions")
	EEqZeroCardinalDirection SelectCardinalDirectionFromAngle(float Angle, float DeadZone, EEqZeroCardinalDirection CurrentDirection, bool UseCurrentDirection);
	
	UFUNCTION(BlueprintPure, Category = "EqZero|Helper Functions")
	EEqZeroCardinalDirection GetOppositeCardinalDirection(EEqZeroCardinalDirection Direction) const;

	/**
	 * Turn In Place
	 */
	UFUNCTION(BlueprintCallable, Category = "EqZero|Turn In Place", meta = (BlueprintThreadSafe))
	void SetRootYawOffset(float InRootYawOffset);
	
	UFUNCTION(BlueprintCallable, Category = "EqZero|Turn In Place", meta = (BlueprintThreadSafe))
	void ProcessTurnYawCurve();
	
	UFUNCTION(BlueprintCallable, Category = "EqZero|Turn In Place", meta = (BlueprintThreadSafe))
	void UpdateRootYawOffset(float DeltaSeconds);
protected:
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<class AEqZeroCharacter> Character = nullptr;
	
	// Rotation Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Data")
	FRotator WorldRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Data")
	float YawDeltaSinceLastUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Data")
	float AdditiveLeanAngle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Data")
	float YawDeltaSpeed; // deg/s

	// Debug
	void UpdateDebugData();
	bool bEnableDebugInfo = true;

	// Location Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location Data")
	FVector WorldLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location Data")
	float DisplacementSinceLastUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location Data")
	float DisplacementSpeed;

	// Velocity Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	FVector WorldVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	FVector LocalVelocity2D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	float LocalVelocityDirectionAngle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	float LocalVelocityDirectionAngleWithOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	EEqZeroCardinalDirection LocalVelocityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	EEqZeroCardinalDirection LocalVelocityDirectionNoOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Data")
	bool HasVelocity;

	// Acceleration Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acceleration Data")
	FVector LocalAcceleration2D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acceleration Data")
	bool HasAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acceleration Data")
	FVector PivotDirection2D;

	// Character State Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	float GroundDistance = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsOnGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool CrouchStateChange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool ADSStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool WasADSLastUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	float TimeSinceFiredWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsJumping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsFalling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	float TimeToJumpApex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State Data")
	bool IsRunningIntoWall;

	// Gameplay Tag Bindings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Bindings")
	bool GameplayTag_IsADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Bindings")
	bool GameplayTag_IsFiring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Bindings")
	bool GameplayTag_IsReloading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Bindings")
	bool GameplayTag_IsDashing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Bindings")
	bool GameplayTag_IsMelee;

	// Locomotion SM Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion SM Data")
	EEqZeroCardinalDirection StartDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion SM Data")
	EEqZeroCardinalDirection PivotInitialDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion SM Data")
	float LastPivotTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion SM Data")
	EEqZeroCardinalDirection CardinalDirectionFromAcceleration;

	// Blend Weight Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend Weight Data")
	float UpperbodyDynamicAdditiveWeight;

	// Aiming Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	float AimPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	float AimYaw;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float CardinalDirectionDeadZone;

	// Linked Layer Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linked Layer Data")
	bool LinkedLayerChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linked Layer Data")
	TObjectPtr<UAnimInstance> LastLinkedLayer;

	// Turn In Place
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	float RootYawOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	FFloatSpringState RootYawOffsetSpringState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	float TurnYawCurveValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	EEqZeroRootYawOffsetMode RootYawOffsetMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	FVector2D RootYawOffsetAngleClamp = FVector2D(-120.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	FVector2D RootYawOffsetAngleClampCrouched = FVector2D(-90.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	bool IsFirstUpdate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	bool EnableControlRig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	bool UseFootPlacement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	bool bEnableRootYawOffset = true;
};
