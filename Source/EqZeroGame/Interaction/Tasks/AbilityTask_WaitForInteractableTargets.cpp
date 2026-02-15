// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/IInteractableTarget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitForInteractableTargets)

struct FInteractionQuery;

UAbilityTask_WaitForInteractableTargets::UAbilityTask_WaitForInteractableTargets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_WaitForInteractableTargets::LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params)
{
	check(World);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;
	World->LineTraceMultiByProfile(HitResults, Start, End, ProfileName, Params);

	OutHitResult.TraceStart = Start;
	OutHitResult.TraceEnd = End;

	if (HitResults.Num() > 0)
	{
		OutHitResult = HitResults[0];
	}
}

void UAbilityTask_WaitForInteractableTargets::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch) const
{
	if (!Ability) // Server and launching client only
	{
		return;
	}

	// 机器人怎么办？如果是机器人，后面会被断言，暂不支持
	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	check(PC);

	FVector ViewStart;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewStart, ViewRot);

	// 拿到 ViewPoint 向前MaxRange 这一条向量
	const FVector ViewDir = ViewRot.Vector();
	FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

	// 修正 ViewEnd。如果摄像机位于角色后方很远，简单的 ViewStart + Range 可能导致射线检测的有效范围与实际角色发起的交互范围不一致。
	// 它确保射线检测的终点在以 TraceStart（通常是角色位置）为球心的 MaxRange 范围内。
	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	// 射线检测：看玩家的准星实际上指向了世界中的哪个物体。
	FHitResult HitResult;
	LineTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, TraceProfile.Name, Params);

	// 检测中交互物，并且要在技能范围内
	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (MaxRange * MaxRange));
	
    // 计算修正后的瞄准方向：
    // 从 TraceStart（角色位置/枪口位置）指向 AdjustedEnd（玩家看到的目标点）。
    // 这解决了“越肩视角”造成的视差问题：摄像机看到的点和角色枪口指向的点需要汇聚。
	const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

	// 处理 bIgnorePitch（忽略俯仰角修正）的情况。通常用于某些不需要上下瞄准修正的交互。
	// 只有在不用 Pitch 修正 且 我们确实使用了检测结果时才进入此逻辑。
	// !bTraceAffectsAimPitch 可能是一个笔误或者 context 中定义的成员变量
	// （参数名为 bIgnorePitch，但内部用了 bTraceAffectsAimPitch，这是代码里的一个潜在不一致点，假设它是类成员变量控制是否启用此逻辑）。
	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero())
	{
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult)
	{
		// 计算原始视线（从 TraceStart 到 ViewEnd）的方向。
		FVector OriginalAimDir = (ViewEnd - TraceStart).GetSafeNormal();

		if (!OriginalAimDir.IsZero())
		{
			// 获取原始瞄准方向的旋转。
			const FRotator OriginalAimRot = OriginalAimDir.Rotation();

			// 获取修正后方向的旋转。
			FRotator AdjustedAimRot = AdjustedAimDir.Rotation();

			// 将修正后的 Pitch（俯仰角）强行设置为原始 Pitch。
			// 这意味着我们只修正 Yaw（左右朝向），保留原来的上下朝向。
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			// 重新计算修正后的方向向量。
			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}
	
	// 输出的 TraceEnd 是从起点 TraceStart 沿着修正后的方向 AdjustedAimDir 延伸 MaxRange 距离的点。
	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}

bool UAbilityTask_WaitForInteractableTargets::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition)
{
	/*
	* 如果直接用 CameraPos + ViewDir * Range，当摄像机离角色很远时，射线的终点可能实际上离角色只有 Range - CameraDistance 那么远，导致你的实际攻击距离缩短了。
	* 或者，如果摄像机有角度，直接延伸可能导致射线末端虽然在视野里看起来很远，但实际上在三维空间中已经超出了以角色为中心的 MaxRange 球体。
	*
	* 不好用文字描述，但是看过源项目的射线更好理解
	*/

	// ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd 参数对照

	// 摄像机到角色的向量
	FVector CameraToCenter = AbilityCenter - CameraLocation;

	// 计算 向量 CameraToCenter 在视线方向 CameraDirection 上的投影长度
	// 如果投影 < 0，说明摄像机背对角色看，或者角色在摄像机背后很远且视角相反，这种情况下不进行修正
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);

	if (DotToCenter >= 0)
	{
		/*          <--view
		 * <--------------------Camera
		 * |                 /
		 * |             /  斜边
		 * |         /
		 * |Center/
		 * | /
		 * P
		 */
		// 斜边的平方 - 投影长度的平方 = 另一条边的平方 (勾股)
		// 计算 角色 到 视线射线 的垂直距离的平方
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);

		// Center范围一个球(交互技能的攻击距离)半径是AbilityRange，这样视野方向和这个球相交了
		if (DistanceSquared <= RadiusSquared)
		{
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);
			return true;
		}
	}
	return false;
}

void UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions(const FInteractionQuery& InteractQuery, const TArray<TScriptInterface<IInteractableTarget>>& InteractableTargets)
{
	TArray<FInteractionOption> NewOptions;

	/*
	 * 遍历所有交互物，从中获取能激活技能的Options
	 * 1. 如果Option里直接有技能了，就用这个技能
	 * 2. 如果Option里没有技能，但是有技能类，就先从玩家身上赋予这个技能，再用它
	 * 3. 如果Option里没有技能也没有技能类，那这个Option就没法用了，丢弃
	 * 4. 最后根据玩家当前的状态（例如，是否在CD中，是否满足激活条件等）来过滤掉一些Option
	 */
	for (const TScriptInterface<IInteractableTarget>& InteractiveTarget : InteractableTargets)
	{
		TArray<FInteractionOption> TempOptions;
		FInteractionOptionBuilder InteractionBuilder(InteractiveTarget, TempOptions);
		InteractiveTarget->GatherInteractionOptions(InteractQuery, InteractionBuilder);

		for (FInteractionOption& Option : TempOptions)
		{
			FGameplayAbilitySpec* InteractionAbilitySpec = nullptr;

			if (Option.TargetAbilitySystem && Option.TargetInteractionAbilityHandle.IsValid())
			{
				InteractionAbilitySpec = Option.TargetAbilitySystem->FindAbilitySpecFromHandle(Option.TargetInteractionAbilityHandle);
			}
			else if (Option.InteractionAbilityToGrant)
			{
				InteractionAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(Option.InteractionAbilityToGrant);

				if (InteractionAbilitySpec)
				{
					Option.TargetAbilitySystem = AbilitySystemComponent.Get();
					Option.TargetInteractionAbilityHandle = InteractionAbilitySpec->Handle;
				}
			}

			if (InteractionAbilitySpec)
			{
				if (InteractionAbilitySpec->Ability->CanActivateAbility(InteractionAbilitySpec->Handle, AbilitySystemComponent->AbilityActorInfo.Get()))
				{
					NewOptions.Add(Option);
				}
			}
		}
	}

	// 检查新旧 Options 是否有变化，如果有变化就广播事件
	bool bOptionsChanged = false;
	if (NewOptions.Num() == CurrentOptions.Num())
	{
		NewOptions.Sort();

		for (int OptionIndex = 0; OptionIndex < NewOptions.Num(); OptionIndex++)
		{
			const FInteractionOption& NewOption = NewOptions[OptionIndex];
			const FInteractionOption& CurrentOption = CurrentOptions[OptionIndex];

			if (NewOption != CurrentOption)
			{
				bOptionsChanged = true;
				break;
			}
		}
	}
	else
	{
		bOptionsChanged = true;
	}

	if (bOptionsChanged)
	{
		CurrentOptions = NewOptions;
		InteractableObjectsChanged.Broadcast(CurrentOptions);
	}
}
