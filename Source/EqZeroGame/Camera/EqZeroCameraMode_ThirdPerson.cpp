// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCameraMode_ThirdPerson.h"
#include "EqZeroCameraMode.h"
#include "Components/PrimitiveComponent.h"
#include "EqZeroPenetrationAvoidanceFeeler.h"
#include "Curves/CurveVector.h"
#include "Engine/Canvas.h"
#include "GameFramework/CameraBlockingVolume.h"
#include "EqZeroCameraAssistInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCameraMode_ThirdPerson)

namespace EqZeroCameraMode_ThirdPerson_Statics
{
	static const FName NAME_IgnoreCameraCollision = TEXT("IgnoreCameraCollision");
}

UEqZeroCameraMode_ThirdPerson::UEqZeroCameraMode_ThirdPerson()
{
	TargetOffsetCurve = nullptr;

    /**
     * 前面我们说单条射线检测不准，需要多准备几条，每条的角度不同，并且有不同的影响权重。这个就是那个射线的数组
     */
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+00.0f, +00.0f, 0.0f), 1.00f, 1.00f, 14.f, 0));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+00.0f, +16.0f, 0.0f), 0.75f, 0.75f, 00.f, 3));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+00.0f, -16.0f, 0.0f), 0.75f, 0.75f, 00.f, 3));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+00.0f, +32.0f, 0.0f), 0.50f, 0.50f, 00.f, 5));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+00.0f, -32.0f, 0.0f), 0.50f, 0.50f, 00.f, 5));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(+20.0f, +00.0f, 0.0f), 1.00f, 1.00f, 00.f, 4));
	PenetrationAvoidanceFeelers.Add(FEqZeroPenetrationAvoidanceFeeler(FRotator(-20.0f, +00.0f, 0.0f), 0.50f, 0.50f, 00.f, 4));
}

void UEqZeroCameraMode_ThirdPerson::UpdateView(float DeltaTime)
{
    // 摄像机模式累加权重的时候会调用一次这个函数，我们需要在这里做摄像机的更新

    // 蹲伏改变了摄像机目标的偏移，最后插值在 CurrentCrouchOffset 影响最终摄像机位置
	UpdateForTarget(DeltaTime);
	UpdateCrouchOffset(DeltaTime);

    // 基础数值
	FVector PivotLocation = GetPivotLocation() + CurrentCrouchOffset;
	FRotator PivotRotation = GetPivotRotation();
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);
	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	// 根据俯仰角应用第三人称偏移
	if (!bUseRuntimeFloatCurves)
	{
		if (TargetOffsetCurve)
		{
            // 根据曲线计算位置
			const FVector TargetOffset = TargetOffsetCurve->GetVectorValue(PivotRotation.Pitch);
			View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
		}
	}
	else
	{
        // 这是编辑器BUG，不用看，看上面的if就行，等修

		FVector TargetOffset(0.0f);

		TargetOffset.X = TargetOffsetX.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Y = TargetOffsetY.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Z = TargetOffsetZ.GetRichCurveConst()->Eval(PivotRotation.Pitch);

		View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
	}

	// 调整最终所需的相机位置，以防止任何穿透情况发生
	UpdatePreventPenetration(DeltaTime);
}

void UEqZeroCameraMode_ThirdPerson::UpdateForTarget(float DeltaTime)
{
    // 蹲伏的时候要设置一下摄像机的偏移量

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		if (TargetCharacter->IsCrouched())
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			const float CrouchedHeightAdjustment = TargetCharacterCDO->CrouchedEyeHeight - TargetCharacterCDO->BaseEyeHeight;

			SetTargetCrouchOffset(FVector(0.f, 0.f, CrouchedHeightAdjustment));

			return;
		}
	}

	SetTargetCrouchOffset(FVector::ZeroVector);
}

void UEqZeroCameraMode_ThirdPerson::DrawDebug(UCanvas* Canvas) const
{
	Super::DrawDebug(Canvas);

#if ENABLE_DRAW_DEBUG
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	for (int i = 0; i < DebugActorsHitDuringCameraPenetration.Num(); i++)
	{
		DisplayDebugManager.DrawString(
			FString::Printf(TEXT("HitActorDuringPenetration[%d]: %s")
				, i
				, *DebugActorsHitDuringCameraPenetration[i]->GetName()));
	}

	LastDrawDebugTime = GetWorld()->GetTimeSeconds();
#endif
}

void UEqZeroCameraMode_ThirdPerson::UpdatePreventPenetration(float DeltaTime)
{
	if (!bPreventPenetration)
	{
		return;
	}

    /**
     * 一些概念
     * SafeLocation：摄像机保底位置，一般是角色位置，保底就是贴着角色
     * Desired camera position 期望位置：摄像机算出来的位置，但是由于阻挡，可能需要二次调整
     * 实际位置：计算阻挡后的位置
     * 
     * 【安全】      【实际位置/ 墙】      【期望位置】
     *  ------------------A
     *  -------------------------------------B
     * 一条线，摄像机在期望位置，然后看向角色的地方，有墙，摄像机挪到了墙前面一点的实际位置。
     * 
     * 阻挡百分比：（实际位置-安全位置）/ (期望位置-安全位置) = A / B【0,1】
     * A / B = 1 说明没阻挡
     * A / B = 0.4 说明阻挡距离安全位置0.4，距离理想位置0.6 【安全位置.....0.4墙......期望位置】
     * 
     * 没被阻挡，摄像机应该去理想位置，直到被阻挡或者达到理想位置。
     * 摄像机被阻挡：应该退到安全位置，直到不被阻挡或达到安全位置。
     * 阻挡用射线检测，检测中应该忽略不影响摄像机视线的场景对象，也可以根据对象权重来处理阻挡。
     */

    // 角色
	AActor* TargetActor = GetTargetActor();
	APawn* TargetPawn = Cast<APawn>(TargetActor);

    // 控制器
	AController* TargetController = TargetPawn ? TargetPawn->GetController() : nullptr;

    // 角色和控制器可以实现这个接口，当前项目是Controller实现了
	IEqZeroCameraAssistInterface* TargetControllerAssist = Cast<IEqZeroCameraAssistInterface>(TargetController);
	IEqZeroCameraAssistInterface* TargetActorAssist = Cast<IEqZeroCameraAssistInterface>(TargetActor);

    // 获取防止穿透的目标Actor，如果没设置，一般也就是主角
	TOptional<AActor*> OptionalPPTarget = TargetActorAssist ? TargetActorAssist->GetCameraPreventPenetrationTarget() : TOptional<AActor*>();
	AActor* PPActor = OptionalPPTarget.IsSet() ? OptionalPPTarget.GetValue() : TargetActor;

    // 重新Cast一下接口，如果有就拿到接口，否则就nullptr
	IEqZeroCameraAssistInterface* PPActorAssist = OptionalPPTarget.IsSet() ? Cast<IEqZeroCameraAssistInterface>(PPActor) : nullptr;

    // 拿到根组件，一般是胶囊体 
	const UPrimitiveComponent* PPActorRootComponent = Cast<UPrimitiveComponent>(PPActor->GetRootComponent());
	if (PPActorRootComponent)
	{
		// 首先要选择安全位置，避免瞄准的时候移动过多
		// 我们的相机就是我们的瞄准镜，所以我们希望保持瞄准状态，并尽可能让其稳定且平稳。
        // 拿到 SafeLocation 在  View.Location这个位置前向射线的这条直线 上最近的点
		FVector ClosestPointOnLineToCapsuleCenter;
		FVector SafeLocation = PPActor->GetActorLocation();
		FMath::PointDistToLine(SafeLocation, View.Rotation.Vector(), View.Location, ClosestPointOnLineToCapsuleCenter);

		// 调整安全距离高度，使其与瞄准线相同，但在胶囊体内。
        // 摄像机不能越过头或者低过脚，的一个限制，但是下面马上就覆盖掉了
		float const PushInDistance = PenetrationAvoidanceFeelers[0].Extent + CollisionPushOutDistance;
		float const MaxHalfHeight = PPActor->GetSimpleCollisionHalfHeight() - PushInDistance;
		SafeLocation.Z = FMath::Clamp(ClosestPointOnLineToCapsuleCenter.Z, SafeLocation.Z - MaxHalfHeight, SafeLocation.Z + MaxHalfHeight);

        // 但是这里 SafeLocation 马上被覆盖了，上面的逻辑存演示作用
        // 返回与最近的 Body Instance 表面的距离的平方
		float DistanceSqr;
		PPActorRootComponent->GetSquaredDistanceToCollision(ClosestPointOnLineToCapsuleCenter, DistanceSqr, SafeLocation);
    
		// 往胶囊体里面推一点，不然在边缘可能视野有问题
		if (PenetrationAvoidanceFeelers.Num() > 0)
		{
			SafeLocation += (SafeLocation - ClosestPointOnLineToCapsuleCenter).GetSafeNormal() * PushInDistance;
		}

        // 上面一堆，我们只是找到了 【SafeLocation】

		// 计算瞄准线到  desired camera position (期待摄像机位置)
        // 这里反正是说要进行摄像机前的墙检测, 但是应该是直接修改数值的
		bool const bSingleRayPenetrationCheck = !bDoPredictiveAvoidance; // 要不要单次射线检测，还是多几条射线

        // 这个函数要修改 View.Location 摄像机位置
        // 计算 AimLineToDesiredPosBlockedPct 阻挡百分比。1代表没阻挡摄像机在理想位置，越小摄像机越靠近角色，0就贴在安全点了
		PreventCameraPenetration(*PPActor, SafeLocation, View.Location, DeltaTime, AimLineToDesiredPosBlockedPct, bSingleRayPenetrationCheck);

		IEqZeroCameraAssistInterface* AssistArray[] = { TargetControllerAssist, TargetActorAssist, PPActorAssist };

        // 摄像机是否被挤压得太近了，会在接口里面隐藏角色模型
		if (AimLineToDesiredPosBlockedPct < ReportPenetrationPercent)
		{
			for (IEqZeroCameraAssistInterface* Assist : AssistArray)
			{
				if (Assist)
				{
					Assist->OnCameraPenetratingTarget();
				}
			}
		}
	}
}

void UEqZeroCameraMode_ThirdPerson::PreventCameraPenetration(class AActor const& ViewTarget, FVector const& SafeLoc, FVector& CameraLoc, float const& DeltaTime, float& DistBlockedPct, bool bSingleRayOnly)
{
    // 先看参数 ViewTarget，主角actor
    // SafeLoc 保底安全位置。图：【安全位置/角色】      【实际位置/ 墙】     <-【期望位置/摄像机】
    // CameraLoc 修改这个会参与我们摄像机位置的计算。
    // DistBlockedPct 看下配置没有，默认值是0，而且他都标了 Transient
    // bSingleRayOnly 单射线还是多射线检测？
#if ENABLE_DRAW_DEBUG
	DebugActorsHitDuringCameraPenetration.Reset();
#endif

    // Soft 和 Hard 阻挡百分比？
	float HardBlockedPct = DistBlockedPct;
	float SoftBlockedPct = DistBlockedPct;

    /**
     * 角色(安全位置) _____墙______ 摄像机(期望位置前面计算出来的)
     */

	FVector BaseRay = CameraLoc - SafeLoc;
	FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
	FVector BaseRayLocalUp, BaseRayLocalFwd, BaseRayLocalRight;

    // 获取按该矩阵的缩放比例缩放后的此矩阵的轴
	BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);

	float DistBlockedPctThisFrame = 1.f;

	int32 const NumRaysToShoot = bSingleRayOnly ? FMath::Min(1, PenetrationAvoidanceFeelers.Num()) : PenetrationAvoidanceFeelers.Num();
	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPen), false, nullptr/*PlayerCamera*/);
	SphereParams.AddIgnoredActor(&ViewTarget);

    // 官方注释
	//TODO IEqZeroCameraTarget.GetIgnoredActorsForCameraPentration();
	//if (IgnoreActorForCameraPenetration)
	//{
	//	SphereParams.AddIgnoredActor(IgnoreActorForCameraPenetration);
	//}

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);
	UWorld* World = GetWorld();

    // 进行这么多次射线检测
	for (int32 RayIdx = 0; RayIdx < NumRaysToShoot; ++RayIdx)
	{
		FEqZeroPenetrationAvoidanceFeeler& Feeler = PenetrationAvoidanceFeelers[RayIdx];
		if (Feeler.FramesUntilNextTrace <= 0) // 到了检测的帧数
		{
			// 计算射线目标
			FVector RayTarget;
			{
				FVector RotatedRay = BaseRay.RotateAngleAxis(Feeler.AdjustmentRot.Yaw, BaseRayLocalUp);
				RotatedRay = RotatedRay.RotateAngleAxis(Feeler.AdjustmentRot.Pitch, BaseRayLocalRight);
				RayTarget = SafeLoc + RotatedRay;
			}

			// cast for world and pawn hits separately.  this is so we can safely ignore the 
			// camera's target pawn
			SphereShape.Sphere.Radius = Feeler.Extent;
			ECollisionChannel TraceChannel = ECC_Camera;		//(Feeler.PawnWeight > 0.f) ? ECC_Pawn : ECC_Camera;

			// do multi-line check to make sure the hits we throw out aren't
			// masking real hits behind (these are important rays).

			// MT-> passing camera as actor so that camerablockingvolumes know when it's the camera doing traces

            // 射线检测了
			FHitResult Hit;
			const bool bHit = World->SweepSingleByChannel(Hit, SafeLoc, RayTarget, FQuat::Identity, TraceChannel, SphereShape, SphereParams);

            // DEBUG指令，DEBUG SHOWCAMERA
#if ENABLE_DRAW_DEBUG
			if (World->TimeSince(LastDrawDebugTime) < 1.f)
			{
				DrawDebugSphere(World, SafeLoc, SphereShape.Sphere.Radius, 8, FColor::Red);
				DrawDebugSphere(World, bHit ? Hit.Location : RayTarget, SphereShape.Sphere.Radius, 8, FColor::Red);
				DrawDebugLine(World, SafeLoc, bHit ? Hit.Location : RayTarget, FColor::Red);
			}
#endif // ENABLE_DRAW_DEBUG

			Feeler.FramesUntilNextTrace = Feeler.TraceInterval;

			const AActor* HitActor = Hit.GetActor();

			// 射线检测到了
			if (bHit && HitActor)
			{
				bool bIgnoreHit = false;

				if (HitActor->ActorHasTag(EqZeroCameraMode_ThirdPerson_Statics::NAME_IgnoreCameraCollision))
				{
					bIgnoreHit = true;
					SphereParams.AddIgnoredActor(HitActor);
				}

				// Ignore CameraBlockingVolume hits that occur in front of the ViewTarget.
				if (!bIgnoreHit && HitActor->IsA<ACameraBlockingVolume>())
				{
					const FVector ViewTargetForwardXY = ViewTarget.GetActorForwardVector().GetSafeNormal2D();
					const FVector ViewTargetLocation = ViewTarget.GetActorLocation();
					const FVector HitOffset = Hit.Location - ViewTargetLocation;
					const FVector HitDirectionXY = HitOffset.GetSafeNormal2D();
					const float DotHitDirection = FVector::DotProduct(ViewTargetForwardXY, HitDirectionXY);
					if (DotHitDirection > 0.0f)
					{
						bIgnoreHit = true;
						// Ignore this CameraBlockingVolume on the remaining sweeps.
						SphereParams.AddIgnoredActor(HitActor);
					}
					else
					{
#if ENABLE_DRAW_DEBUG
						DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
					}
				}

				// 确定射线打中一个有效的东西了
				if (!bIgnoreHit)
				{
					float const Weight = Cast<APawn>(Hit.GetActor()) ? Feeler.PawnWeight : Feeler.WorldWeight;
					float NewBlockPct = Hit.Time;
					NewBlockPct += (1.f - NewBlockPct) * (1.f - Weight);

					// 重新计算受阻百分比，将推出距离纳入考量。
					NewBlockPct = ((Hit.Location - SafeLoc).Size() - CollisionPushOutDistance) / (RayTarget - SafeLoc).Size();
					DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame);

					// 这个射线有了反应，所以下一帧再进行一次追踪
					Feeler.FramesUntilNextTrace = 0;

#if ENABLE_DRAW_DEBUG
					DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
				}
			}

			if (RayIdx == 0)
			{
				// 不要向这个方向插值，直接吸附到它上面
				// 假设光线 0 是中心 / 主光线
				HardBlockedPct = DistBlockedPctThisFrame;
			}
			else
			{
				SoftBlockedPct = DistBlockedPctThisFrame;
			}
		}
		else
		{
			--Feeler.FramesUntilNextTrace;
		}
	}

	if (bResetInterpolation)
	{
		DistBlockedPct = DistBlockedPctThisFrame;
	}
	else if (DistBlockedPct < DistBlockedPctThisFrame)
	{
		// interpolate smoothly out
		if (PenetrationBlendOutTime > DeltaTime)
		{
			DistBlockedPct = DistBlockedPct + DeltaTime / PenetrationBlendOutTime * (DistBlockedPctThisFrame - DistBlockedPct);
		}
		else
		{
			DistBlockedPct = DistBlockedPctThisFrame;
		}
	}
	else
	{
		if (DistBlockedPct > HardBlockedPct)
		{
			DistBlockedPct = HardBlockedPct;
		}
		else if (DistBlockedPct > SoftBlockedPct)
		{
			// interpolate smoothly in
			if (PenetrationBlendInTime > DeltaTime)
			{
				DistBlockedPct = DistBlockedPct - DeltaTime / PenetrationBlendInTime * (DistBlockedPct - SoftBlockedPct);
			}
			else
			{
				DistBlockedPct = SoftBlockedPct;
			}
		}
	}

	// 根据阻挡百分比算摄像机位置
	DistBlockedPct = FMath::Clamp<float>(DistBlockedPct, 0.f, 1.f);
	if (DistBlockedPct < (1.f - ZERO_ANIMWEIGHT_THRESH))
	{
		CameraLoc = SafeLoc + (CameraLoc - SafeLoc) * DistBlockedPct;
	}
}

void UEqZeroCameraMode_ThirdPerson::SetTargetCrouchOffset(FVector NewTargetOffset)
{
	CrouchOffsetBlendPct = 0.0f;
	InitialCrouchOffset = CurrentCrouchOffset;
	TargetCrouchOffset = NewTargetOffset;
}


void UEqZeroCameraMode_ThirdPerson::UpdateCrouchOffset(float DeltaTime)
{
	if (CrouchOffsetBlendPct < 1.0f)
	{
		CrouchOffsetBlendPct = FMath::Min(CrouchOffsetBlendPct + DeltaTime * CrouchOffsetBlendMultiplier, 1.0f);
		CurrentCrouchOffset = FMath::InterpEaseInOut(InitialCrouchOffset, TargetCrouchOffset, CrouchOffsetBlendPct, 1.0f);
	}
	else
	{
		CurrentCrouchOffset = TargetCrouchOffset;
		CrouchOffsetBlendPct = 1.0f;
	}
}
