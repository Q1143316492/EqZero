// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/EqZeroAnimInstance.h"
#include "EqZeroAnimationTypes.h"
#include "EqZeroItemAnimInstance.generated.h"

class UAnimSequence;
class UBlendSpace;

/**
 * 
 */
UCLASS()
class EQZEROGAME_API UEqZeroItemAnimInstance : public UAnimInstance // 不和 UEqZeroAnimInstance 继承
{
	GENERATED_BODY()
public:
	UEqZeroItemAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
    void UpdateData(float DeltaSeconds);
	void UpdateBlendWeightData(float DeltaSeconds);
    void UpdateJumpFallData(float DeltaSeconds);
    void UpdateSkeletalControlData(float DeltaSeconds);
	
public:
	// Anim Set - Idle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> Idle_ADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> Idle_Hipfire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TArray<TObjectPtr<UAnimSequence>> Idle_Breaks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> Crouch_Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> Crouch_Idle_Entry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> Crouch_Idle_Exit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Idle")
	TObjectPtr<UAnimSequence> LeftHandPose_Override; // TODO check

	// Anim Set - Starts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Starts")
	FAnimStructCardinalDirections Jog_Start_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Starts")
	FAnimStructCardinalDirections ADS_Start_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Starts")
	FAnimStructCardinalDirections Crouch_Start_Cardinals;

	// Anim Set - Stops
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Stops")
	FAnimStructCardinalDirections Jog_Stop_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Stops")
	FAnimStructCardinalDirections ADS_Stop_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Stops")
	FAnimStructCardinalDirections Crouch_Stop_Cardinals;

	// Anim Set - Pivots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Pivots")
	FAnimStructCardinalDirections Jog_Pivot_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Pivots")
	FAnimStructCardinalDirections ADS_Pivot_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Pivots")
	FAnimStructCardinalDirections Crouch_Pivot_Cardinals;

	// Anim Set - Turn in Place
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Turn in Place")
	TObjectPtr<UAnimSequence> TurnInPlace_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Turn in Place")
	TObjectPtr<UAnimSequence> TurnInPlace_Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Turn in Place")
	TObjectPtr<UAnimSequence> Crouch_TurnInPlace_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Turn in Place")
	TObjectPtr<UAnimSequence> Crouch_TurnInPlace_Right;

	// Anim Set - Jog
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jog")
	FAnimStructCardinalDirections Jog_Cardinals;

	// Anim Set - Jump
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_Apex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_FallLand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_RecoveryAdditive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_StartLoop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	TObjectPtr<UAnimSequence> Jump_FallLoop;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Jump")
	// FName JumpDistanceCurveName; // TODO check

	// Anim Set - Walk
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Walk")
	FAnimStructCardinalDirections Walk_Cardinals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Walk")
	FAnimStructCardinalDirections Crouch_Walk_Cardinals;

	// Anim Set - Aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Aiming")
	TObjectPtr<UAnimSequence> Aim_HipFirePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Aiming")
	TObjectPtr<UAnimSequence> Aim_HipFirePose_Crouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Aiming")
	TObjectPtr<UBlendSpace> IdleAimOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Set - Aiming")
	TObjectPtr<UBlendSpace> RelaxedAimOffset;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector2D PlayRateClampStartsPivots = FVector2D(0.6f, 5.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool RaiseWeaponAfterFiringWhenCrouched;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool DisableHandIK;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool EnableLeftHandPoseOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float RaiseWeaponAfterFiringDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float StrideWarpingBlendInDurationScaled = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float StrideWarpingBlendInStartOffset = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName LocomotionDistanceCurveName = TEXT("Distance");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector2D PlayRateClampCycle = FVector2D(0.8f, 1.2f);

	// Blend Weight Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend Weight Data")
	float HipFireUpperBodyOverrideWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend Weight Data")
	float AimOffsetBlendWeight;

	// Turn In Place
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceAnimTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceRotationDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceRecoveryDirection;

	// Idle Breaks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Breaks")
	bool WantsIdleBreak;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Breaks")
	float TimeUntilNextIdleBreak;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Breaks")
	int32 CurrentIdleBreakIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Breaks")
	float IdleBreakDelayTime;

	// Pivots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pivots")
	FVector PivotStartingAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pivots")
	float TimeAtPivotStop;

	// Jump
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	float LandRecoveryAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	float TimeFalling;

	// Skel Control Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skel Control Data")
	float HandIK_Right_Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skel Control Data")
	float HandIK_Left_Alpha;

	// Stride Warping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingStartAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingPivotAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingCycleAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride Warping")
	float LeftHandPoseOverrideWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride Warping")
	float HandFKWeight;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<UEqZeroAnimInstance> MainAnim;
};
