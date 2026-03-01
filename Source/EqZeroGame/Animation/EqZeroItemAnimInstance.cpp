// Copyright Epic Games, Inc. All Rights Reserved.


#include "Animation/EqZeroItemAnimInstance.h"
#include "Animation/EqZeroAnimInstance.h"
#include "EqZeroLogChannels.h"
#include "Animation/AnimSequence.h"
#include "KismetAnimationLibrary.h"


UEqZeroItemAnimInstance::UEqZeroItemAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroItemAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

    if(auto MeshComp = GetOwningComponent())
    {
	    MainAnim = Cast<UEqZeroAnimInstance>(MeshComp->GetAnimInstance());
    }
}

void UEqZeroItemAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	UpdateData(DeltaSeconds);
}

void UEqZeroItemAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}
	
void UEqZeroItemAnimInstance::UpdateData(float DeltaSeconds)
{
	if (!MainAnim.IsValid())
    {
        return;
    }

    UpdateBlendWeightData(DeltaSeconds);
    UpdateJumpFallData(DeltaSeconds);
    UpdateSkeletalControlData(DeltaSeconds);
}

void UEqZeroItemAnimInstance::UpdateBlendWeightData(float DeltaSeconds)
{
	//  RaiseWeaponAfterFiringWhenCrouched 是默认False的配置
	bool bCondition1 = !RaiseWeaponAfterFiringWhenCrouched && MainAnim->IsCrouching;
	bool bCondition2 = !MainAnim->IsCrouching && MainAnim->GameplayTag_IsADS && MainAnim->IsOnGround;

	// 蹲伏状态 || 非蹲伏状态下在地面瞄准
	if (bCondition1 || bCondition2)
	{
		HipFireUpperBodyOverrideWeight = 0.0f;
		AimOffsetBlendWeight = 1.0f;
        return ;
	}
    
    bCondition1 = MainAnim->TimeSinceFiredWeapon < RaiseWeaponAfterFiringDuration;
    bCondition2 = MainAnim->GameplayTag_IsADS && (MainAnim->IsCrouching || !MainAnim->IsOnGround);

	// 0.5s 内刚刚开火 || 瞄准且蹲伏或不在地面 || 动画资产强制应用瞄准姿势覆盖
    if (bCondition1 || bCondition2 || GetCurveValue(TEXT("applyHipfireOverridePose")) > 0.f)
    {
        HipFireUpperBodyOverrideWeight = 1.0f;
        AimOffsetBlendWeight = 1.0f;
    }
    else
    {
    	// 否则平滑地插值回0
        HipFireUpperBodyOverrideWeight = FMath::FInterpTo(HipFireUpperBodyOverrideWeight, 0.0f, DeltaSeconds, 1.0f);

    	// 静止或存在根偏航偏移时使用瞄准偏移, 正常运动时使用放松瞄准偏移
    	float AimTargetInterpTo = 1.f;
    	if (bool UseHipFireWeight = FMath::Abs(MainAnim->RootYawOffset) < 10.f && MainAnim->HasAcceleration)
		{
    		AimTargetInterpTo = HipFireUpperBodyOverrideWeight;
		}
    	AimOffsetBlendWeight = FMath::FInterpTo(AimOffsetBlendWeight, AimTargetInterpTo, DeltaSeconds, 10.0f);
    }
}

void UEqZeroItemAnimInstance::UpdateJumpFallData(float DeltaSeconds)
{
	if (MainAnim->IsFalling)
	{
		TimeFalling += DeltaSeconds;
	}
	else if (MainAnim->IsJumping)
	{
		TimeFalling = 0.f;
	}
}

void UEqZeroItemAnimInstance::UpdateSkeletalControlData(float DeltaSeconds)
{
	const float BaseAlpha = DisableHandIK ? 0.0f : 1.0f;

	HandIK_Right_Alpha = FMath::Clamp(BaseAlpha - GetCurveValue(TEXT("DisableRHandIK")), 0.0f, 1.0f);
	HandIK_Left_Alpha = FMath::Clamp(BaseAlpha - GetCurveValue(TEXT("DisableLHandIK")), 0.0f, 1.0f);
}