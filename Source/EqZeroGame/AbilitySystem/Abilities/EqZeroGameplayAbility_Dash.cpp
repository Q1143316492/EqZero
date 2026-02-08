// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_Dash.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/EqZeroCharacter.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Dash)

UEqZeroGameplayAbility_Dash::UEqZeroGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UEqZeroGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 只在本地控制端计算方向，确保客户端和服务器使用相同的数据
	if (IsLocallyControlled())
	{
		FVector Facing = FVector::ZeroVector;
		FVector LastMovementInput = FVector::ZeroVector;
		FVector Movement = FVector::ZeroVector;
		bool bPositive;
		GetDirection(Facing, LastMovementInput, Movement, bPositive);

		bool BiasForwardMovement = false;
		UAnimMontage* TempMontage = nullptr;
		SelectDirectionalMontage(Facing, LastMovementInput, TempMontage, BiasForwardMovement);
		Montage = TempMontage;
		Direction = BiasForwardMovement ? Movement : LastMovementInput;

		if (Direction.IsNearlyZero())
		{
			CancelAbility(Handle, ActorInfo, ActivationInfo, true);
			return;
		}
		
        if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
        {
        	CancelAbility(Handle, ActorInfo, ActivationInfo, true);
            return;
        }

		if (AEqZeroCharacter* Character = GetEqZeroCharacterFromActorInfo())
		{
			if (Character->IsCrouched())
			{
				Character->UnCrouch();
			}
		}

		if (HasAuthority(&ActivationInfo))
		{
			// 单机或 Listen Server：直接执行
			ExecuteDash(Direction, Montage);
		}
		else
		{
			// DS 客户端：本地预测执行 + RPC 通知服务器
			ExecuteDash(Direction, Montage);
			ServerSendInfo(Direction, Montage);
		}
	}
}

void UEqZeroGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (EndAbilityTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(EndAbilityTimerHandle);
		EndAbilityTimerHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UEqZeroGameplayAbility_Dash::GetDirection(FVector& OutFacing, FVector& OutLastMovementInput, FVector& OutMovement, bool &OutPositive)
{
	auto Character = GetEqZeroCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}
	
	OutFacing = Character->GetActorForwardVector();

	bool bIsAIController = UAIBlueprintHelperLibrary::GetAIController(Character) != nullptr;
	if (bIsAIController)
	{
		// AI 使用 Navigation 获取移动方向
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(Character->GetWorld());
        FNavLocation ProjectedLocation;
        FVector ActorLocation = Character->GetActorLocation();
        FVector QueryExtent(500.0f, 500.0f, 500.0f);

        if (NavSys && NavSys->ProjectPointToNavigation(ActorLocation, ProjectedLocation, QueryExtent))
        {
            // 忽略Z轴，计算水平面上的导航方向。如果导航系统无法提供有效的方向，则回退到角色的朝向。
            FVector NavDirection = (ProjectedLocation.Location - ActorLocation) * FVector(1.0f, 1.0f, 0.0f);
            NavDirection.Normalize();
            bool bHasNavDirection = NavDirection.SizeSquared() > 0.0f;
            OutLastMovementInput = bHasNavDirection ? NavDirection : OutFacing;

            OutMovement = OutLastMovementInput;
            OutMovement.Normalize();
        }
	}
	else
	{
        // 是玩家控制的
		OutLastMovementInput = Character->GetLastMovementInputVector().GetSafeNormal();
        if (UCameraComponent* CameraComp = Character->FindComponentByClass<UCameraComponent>())
        {
            auto CameraForward = CameraComp->GetForwardVector();
            OutMovement = CameraForward * FVector(1.0f, 1.0f, 1.2f);
        }
	}

    // 是否是主动的
	OutPositive = Character->GetLastMovementInputVector().Length() > KINDA_SMALL_NUMBER || bIsAIController;
}

void UEqZeroGameplayAbility_Dash::SelectDirectionalMontage(const FVector& Facing, const FVector& MovementDirection, UAnimMontage*& DirectionalMontage, bool& BiasForwardMovement)
{
	DirectionalMontage = nullptr;
	BiasForwardMovement = false;

	// 从向量创建旋转（类似蓝图的 Rotation From X Vector）
	FRotator FacingRotation = Facing.Rotation();
	FRotator MovementRotation = MovementDirection.Rotation();
	
	// 计算旋转差值（类似蓝图的 Delta (Rotator)）
	FRotator DeltaRotation = MovementRotation - FacingRotation;
	DeltaRotation.Normalize(); // 规范化到 -180 到 180
	
	float YawDelta = DeltaRotation.Yaw;
	float AbsYawDelta = FMath::Abs(YawDelta); // 类似蓝图的 ABS 节点
	
	// 根据角度选择动画蒙太奇（遵循蓝图逻辑）
	if (AbsYawDelta < 45.0f)
	{
		// Forward: -45° to 45°
		DirectionalMontage = Dash_Fwd_Montage;
		BiasForwardMovement = true;
	}
	else if (AbsYawDelta < 135.0f)
	{
		// Side: 45° to 135°
		if (YawDelta > 0.0f)
		{
			DirectionalMontage = Dash_Right_Montage;
		}
		else
		{
			DirectionalMontage = Dash_Left_Montage;
		}
		BiasForwardMovement = false;
	}
	else
	{
		// Backward: > 135°
		DirectionalMontage = Dash_Bwd_Montage;
		BiasForwardMovement = false;
	}
}

void UEqZeroGameplayAbility_Dash::OnMontageCompleted()
{
}

void UEqZeroGameplayAbility_Dash::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UEqZeroGameplayAbility_Dash::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UEqZeroGameplayAbility_Dash::OnRootMotionFinished()
{
	float DelayEndAbilityTime = AbilityDuration - RootMotionDuration;
	if (DelayEndAbilityTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			EndAbilityTimerHandle,
			[this]()
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			},
			DelayEndAbilityTime,
			false
		);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UEqZeroGameplayAbility_Dash::ServerSendInfo_Implementation(const FVector& InDirection, UAnimMontage* InMontage)
{
    Direction = InDirection;
	Montage = InMontage;
    CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
    ExecuteDash(Direction, Montage);
}

void UEqZeroGameplayAbility_Dash::ExecuteDash(const FVector& InDirection, UAnimMontage* InMontage)
{
	Direction = InDirection;
	Montage = InMontage;

	UE_LOG(LogTemp, Warning, TEXT("[Dash] ExecuteDash called on %s | Direction: %s"), 
		HasAuthority(&CurrentActivationInfo) ? TEXT("Server") : TEXT("Client"),
		*Direction.ToString());

	// Play Montage (确保 Montage 动画不包含 Root Motion，只包含姿势动画)
	if (InMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			FName("PlayDashMontage"),
			InMontage,
			1.0f,
			NAME_None,
			true
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &UEqZeroGameplayAbility_Dash::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UEqZeroGameplayAbility_Dash::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &UEqZeroGameplayAbility_Dash::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	// Apply Root Motion Constant Force
	if (Strength > 0.0f && RootMotionDuration > 0.0f)
	{
		UAbilityTask_ApplyRootMotionConstantForce* RootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this,
			FName("DashRootMotion"),
			InDirection,
			Strength,
			RootMotionDuration,
			true,
			nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,
			1000.0f,
			false
		);

		if (RootMotionTask)
		{
			RootMotionTask->OnFinish.AddDynamic(this, &UEqZeroGameplayAbility_Dash::OnRootMotionFinished);
			RootMotionTask->ReadyForActivation();
		}
	}

	if (HasAuthority(&CurrentActivationInfo))
	{
		// TODO gameplay cue	
	}
}