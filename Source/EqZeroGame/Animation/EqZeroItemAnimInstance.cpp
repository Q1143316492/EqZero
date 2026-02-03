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

    // 链接动画层改变会重新初始化的，缓存没关系
	// if (AActor* OwningActor = GetOwningActor())
	// {
 //        // 获取拥有这个动画实例的网格组件
	// 	if (USkeletalMeshComponent* SkeletalMeshComponent = GetOwningComponent())
	// 	{
	// 		// 如果组件的主动画实例 != 我自己 (this)。这个接口返回的是那个“主”实例。
	// 		if (SkeletalMeshComponent->GetAnimInstance() != this)
	// 		{
	// 			// 说明我是被“链接”进来的子实例。这个接口返回的是那个“主”实例。
	// 			MainAnim = Cast<UEqZeroAnimInstance>(SkeletalMeshComponent->GetAnimInstance());
	// 		}
	// 		else
	// 		{
	// 			// 只有当这个 ItemAnimInstance 被直接作为主动画蓝图设置在某个 Mesh 上时（而不是链接进去时）
	// 			// 不应该这么用
 //                UE_LOG(LogEqZero, Warning, TEXT("UEqZeroItemAnimInstance on [%s] is the main AnimInstance. This is not the intended use case."), *GetNameSafe(OwningActor));
	// 		}
	// 	}
	// }
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
	bool bCondition1 = !RaiseWeaponAfterFiringWhenCrouched && MainAnim->IsCrouching;
	bool bCondition2 = !MainAnim->IsCrouching && MainAnim->GameplayTag_IsADS && MainAnim->IsOnGround;

	if (bCondition1 || bCondition2)
	{
		HipFireUpperBodyOverrideWeight = 0.0f;
		AimOffsetBlendWeight = 1.0f;
        return ;
	}
    
    bCondition1 = MainAnim->TimeSinceFiredWeapon < RaiseWeaponAfterFiringDuration;
    bCondition2 = MainAnim->GameplayTag_IsADS && (MainAnim->IsCrouching || !MainAnim->IsOnGround);
    if (bCondition1 || bCondition2 || GetCurveValue(TEXT("applyHipfireOverridePose")) > 0.f)
    {
        HipFireUpperBodyOverrideWeight = 1.0f;
        AimOffsetBlendWeight = 1.0f;
    }
    else
    {
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