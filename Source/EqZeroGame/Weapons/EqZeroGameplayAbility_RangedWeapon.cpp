// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_RangedWeapon.h"
#include "Weapons/EqZeroRangedWeaponInstance.h"
#include "Physics/EqZeroCollisionChannels.h"
#include "EqZeroLogChannels.h"
#include "AIController.h"
#include "NativeGameplayTags.h"
#include "Weapons/EqZeroWeaponStateComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/EqZeroGameplayAbilityTargetData_SingleTargetHit.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_RangedWeapon)

namespace EqZeroConsoleVariables
{
	static float DrawBulletTracesDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletTraceDuraton(
		TEXT("EqZero.Weapon.DrawBulletTraceDuration"),
		DrawBulletTracesDuration,
		TEXT("Should we do debug drawing for bullet traces (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHits(
		TEXT("EqZero.Weapon.DrawBulletHitDuration"),
		DrawBulletHitDuration,
		TEXT("Should we do debug drawing for bullet impacts (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitRadius = 3.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHitRadius(
		TEXT("EqZero.Weapon.DrawBulletHitRadius"),
		DrawBulletHitRadius,
		TEXT("When bullet hit debug drawing is enabled (see DrawBulletHitDuration), how big should the hit radius be? (in uu)"),
		ECVF_Default);
}

// Weapon fire will be blocked/canceled if the player has this tag
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked, "Ability.Weapon.NoFiring");

//////////////////////////////////////////////////////////////////////

FVector VRandConeNormalDistribution(const FVector& Dir, const float ConeHalfAngleRad, const float Exponent)
{
	// ConeHalfAngleRad = 这个手枪是7.5 除一半进来。3.75弧度。大概比PI大一点，
	if (ConeHalfAngleRad > 0.f)
	{
		
		const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRad);

        // 1. 偏离中心的程度 (0.0 ~ 1.0)
        // FMath::FRand() 生成 [0, 1] 之间的随机数
        // Pow(Random, Exponent): 
        //   如果 Exponent 是 2，那么 0.5^2 = 0.25。也就是原本 50% 概率出现的值，现在变成了 25% 的位置。
        //   这意味着生成的随机数经过 Pow 计算后，会变得更小（更接近 0，即更接近中心）。
		const float FromCenter = FMath::Pow(FMath::FRand(), Exponent);

        // 2. 将偏离程度映射到角度
        // 如果 FromCenter 是 0.1，那么偏离角度就是最大半角的 10%。
		const float AngleFromCenter = FromCenter * ConeHalfAngleDegrees;

        // 3. 绕中心旋转的角度 (0 ~ 360度)
        // 决定子弹是偏左、偏右、偏上还是偏下。这部分是完全均匀随机的。
		const float AngleAround = FMath::FRand() * 360.0f;

        // 4. 构建旋转四元数
		// 一条Dir的向量
		FRotator Rot = Dir.Rotation();
		FQuat DirQuat(Rot);

		// DirQuat 绕 Yaw 旋转 AngleFromCenter 度
		FQuat FromCenterQuat(FRotator(0.0f, AngleFromCenter, 0.0f));

		// DirQuat 绕 Roll 旋转 AngleAround 度
		// 这一步非常有迷惑性。它是在转动整个坐标系。
		// 想象你刚才把针往右拨歪了。现在，你捏着这根针的根部（圆心），把它沿着圆周转一个随机角度（0~360度
		FQuat AroundQuat(FRotator(0.0f, 0.0, AngleAround));
		
		// 两个四元数相乘 A * B，意思是：先进行 B 旋转，再进行 A 旋转（或者理解为在 A 的基础上叠加 B 的旋转）。
		// 先绕 Yaw 旋转，再绕 Roll 旋转，最后应用到 DirQuat 上
		FQuat FinalDirectionQuat = DirQuat * AroundQuat * FromCenterQuat;
		FinalDirectionQuat.Normalize();

		return FinalDirectionQuat.RotateVector(FVector::ForwardVector);
	}
	else
	{
		return Dir.GetSafeNormal();
	}
}


UEqZeroGameplayAbility_RangedWeapon::UEqZeroGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// SourceBlockedTags.AddTag(TAG_WeaponFireBlocked); // Standard gameplay tag
}

UEqZeroRangedWeaponInstance* UEqZeroGameplayAbility_RangedWeapon::GetWeaponInstance() const
{
	return Cast<UEqZeroRangedWeaponInstance>(GetAssociatedEquipment());
}

bool UEqZeroGameplayAbility_RangedWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		if (GetWeaponInstance() == nullptr)
		{
			UE_LOG(LogEqZeroAbilitySystem, Error, TEXT("Weapon ability %s cannot be activated because there is no associated ranged weapon (equipment instance=%s but needs to be derived from %s)"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*UEqZeroRangedWeaponInstance::StaticClass()->GetName());
			bResult = false;
		}
	}

	return bResult;
}

int32 UEqZeroGameplayAbility_RangedWeapon::FindFirstPawnHitResult(const TArray<FHitResult>& HitResults)
{
	// 从 HitResults 中找到第一个命中 Pawn 的结果，如果没有则返回 INDEX_NONE
	for (int32 Idx = 0; Idx < HitResults.Num(); ++Idx)
	{
		const FHitResult& CurHitResult = HitResults[Idx];
		if (CurHitResult.HitObjectHandle.DoesRepresentClass(APawn::StaticClass()))
		{
			// 如果直接击中了一个 Pawn，返回这个结果的索引
			return Idx;
		}
		else
		{
			AActor* HitActor = CurHitResult.HitObjectHandle.FetchActor();
			if ((HitActor != nullptr) && (HitActor->GetAttachParentActor() != nullptr) && (Cast<APawn>(HitActor->GetAttachParentActor()) != nullptr))
			{
				// 击中了一个附着在 Pawn 上的物体，也算命中 Pawn
				return Idx;
			}
		}
	}

	return INDEX_NONE;
}

void UEqZeroGameplayAbility_RangedWeapon::AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		// Ignore any actors attached to the avatar doing the shooting
		TArray<AActor*> AttachedActors;
		Avatar->GetAttachedActors(AttachedActors);
		TraceParams.AddIgnoredActors(AttachedActors);
	}
}

ECollisionChannel UEqZeroGameplayAbility_RangedWeapon::DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const
{
	return EqZero_TraceChannel_Weapon;
}

FHitResult UEqZeroGameplayAbility_RangedWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHitResults) const
{
	/*
	 * 根据 SweepRadius 会用射线检测和圆柱体检测，同时避免多条射线打中同一个角色造成多次伤害
	 */
	TArray<FHitResult> HitResults;
	
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetAvatarActorFromActorInfo());
	TraceParams.bReturnPhysicalMaterial = true;
	AddAdditionalTraceIgnoreActors(TraceParams);
	//TraceParams.bDebugQuery = true;

	const ECollisionChannel TraceChannel = DetermineTraceChannel(TraceParams, bIsSimulated);

	// 射线检测和圆柱检测的区别
	if (SweepRadius > 0.0f)
	{
		GetWorld()->SweepMultiByChannel(HitResults, StartTrace, EndTrace, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(SweepRadius), TraceParams);
	}
	else
	{
		GetWorld()->LineTraceMultiByChannel(HitResults, StartTrace, EndTrace, TraceChannel, TraceParams);
	}

	FHitResult Hit(ForceInit);
	if (HitResults.Num() > 0)
	{
		// 过滤输出列表，以防止对同一角色造成多次命中；
		// 这样做是为了防止在使用重叠追踪时，一颗子弹对
		// 单个角色造成多次伤害
		for (FHitResult& CurHitResult : HitResults)
		{
			auto Pred = [&CurHitResult](const FHitResult& Other)
			{
				return Other.HitObjectHandle == CurHitResult.HitObjectHandle;
			};

			if (!OutHitResults.ContainsByPredicate(Pred))
			{
				OutHitResults.Add(CurHitResult);
			}
		}

		Hit = OutHitResults.Last();
	}
	else
	{
		Hit.TraceStart = StartTrace;
		Hit.TraceEnd = EndTrace;
	}

	return Hit;
}

FVector UEqZeroGameplayAbility_RangedWeapon::GetWeaponTargetingSourceLocation() const
{
	// 用 Pawn 作为基准，再添加偏移
	
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	check(AvatarPawn);

	const FVector SourceLoc = AvatarPawn->GetActorLocation();
	const FQuat SourceRot = AvatarPawn->GetActorQuat();

	FVector TargetingSourceLocation = SourceLoc;

	// 从武器实例添加一个偏移量，并根据角色的蹲下、瞄准等状态进行调整……

	return TargetingSourceLocation;
}

FTransform UEqZeroGameplayAbility_RangedWeapon::GetTargetingTransform(APawn* SourcePawn, EEqZeroAbilityTargetingSource Source) const
{
	// 根据枚举。决定子弹从哪里发出来、射向哪里
	
	check(SourcePawn);
	AController* SourcePawnController = SourcePawn->GetController(); 
	UEqZeroWeaponStateComponent* WeaponStateComponent = (SourcePawnController != nullptr) ? SourcePawnController->FindComponentByClass<UEqZeroWeaponStateComponent>() : nullptr;

	// 如果模式是自定义的，调用者应在不调用此（函数 / 方法）的情况下确定转换！
	check(Source != EEqZeroAbilityTargetingSource::Custom);

	const FVector ActorLoc = SourcePawn->GetActorLocation();
	FQuat AimQuat = SourcePawn->GetActorQuat();
	AController* Controller = SourcePawn->GetController();
	FVector SourceLoc;

	double FocalDistance = 1024.0f;
	FVector FocalLoc;

	FVector CamLoc;
	FRotator CamRot;
	bool bFoundFocus = false;


	if ((Controller != nullptr) &&
		((Source == EEqZeroAbilityTargetingSource::CameraTowardsFocus) || // 从摄像机朝向焦点
			(Source == EEqZeroAbilityTargetingSource::PawnTowardsFocus) || // 从角色朝向焦点
			(Source == EEqZeroAbilityTargetingSource::WeaponTowardsFocus))) // 从武器朝向焦点
	{
		bFoundFocus = true;

		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC != nullptr)
		{
			PC->GetPlayerViewPoint(CamLoc, CamRot); // 摄像机位置和朝向
		}
		else
		{
			SourceLoc = GetWeaponTargetingSourceLocation(); // 武器位置，目前没有逻辑就是角色位置
			CamLoc = SourceLoc;
			CamRot = Controller->GetControlRotation();
		}

		FVector AimDir = CamRot.Vector().GetSafeNormal();
		FocalLoc = CamLoc + (AimDir * FocalDistance); // 假设玩家盯着 1024 厘米远的地方看

		// 区分了玩家和AI控制器，玩家从摄像机位置发射，AI从角色位置发射
		if (PC)
		{
			// 武器位置，目前没有逻辑就是角色位置
			const FVector WeaponLoc = GetWeaponTargetingSourceLocation();

			// 把枪的位置投影到视线轴上，拿到这个点
            // (WeaponLoc - FocalLoc) | AimDir 注意方向，这里应该是一个负数。*AimDir，相当于把FacalLoc向摄像机的位置推
			CamLoc = FocalLoc + (((WeaponLoc - FocalLoc) | AimDir) * AimDir);

			// 重新计算焦点，保持方向一致但基于新的起点（这步略显多余，主要是确保一致性）
			FocalLoc = CamLoc + (AimDir * FocalDistance);
		}
		else if (AAIController* AIController = Cast<AAIController>(Controller))
		{
			CamLoc = SourcePawn->GetActorLocation() + FVector(0, 0, SourcePawn->BaseEyeHeight);
		}

		if (Source == EEqZeroAbilityTargetingSource::CameraTowardsFocus)
		{
			return FTransform(CamRot, CamLoc);
		}
	}

	if ((Source == EEqZeroAbilityTargetingSource::WeaponForward) || (Source == EEqZeroAbilityTargetingSource::WeaponTowardsFocus))
	{
		SourceLoc = GetWeaponTargetingSourceLocation();
	}
	else
	{
		// 要么我们想要角色的位置，要么我们没找到摄像机
		SourceLoc = ActorLoc;
	}

	if (bFoundFocus && ((Source == EEqZeroAbilityTargetingSource::PawnTowardsFocus) || (Source == EEqZeroAbilityTargetingSource::WeaponTowardsFocus)))
	{
		// 返回一个从源点指向焦点的旋转器
		return FTransform((FocalLoc - SourceLoc).Rotation(), SourceLoc);
	}

	// 如果我们到了这里，要么是没有相机，要么是不想用相机，不管哪种情况，都继续往前走。
	return FTransform(AimQuat, SourceLoc);
}

FHitResult UEqZeroGameplayAbility_RangedWeapon::DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHits) const
{
#if ENABLE_DRAW_DEBUG
	if (EqZeroConsoleVariables::DrawBulletTracesDuration > 0.0f)
	{
		static float DebugThickness = 1.0f;
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Red, false, EqZeroConsoleVariables::DrawBulletTracesDuration, 0, DebugThickness);
	}
#endif // ENABLE_DRAW_DEBUG

	/*
	 * 来自AI的比喻。。。
	 *
	 *	先试着用针扎（Fine Trace）。

		扎到了人？-> 好，这一枪中了！结算！
		扎到了墙？-> 记录下来打中了墙，然后继续看下一步。
		啥都没扎到？-> 继续看下一步。
		如果没扎到人，试着用棍子捅（Sweep Trace）。

		棍子捅到了人？-> 等一下，先别急着结算！
		最终审核（Validation）。

		虽然棍子捅到了人，但刚才那根针是不是扎在了一堵墙上？
		如果针扎在墙上，而且这堵墙就在你要捅的人前面 -> 不论棍子有没有蹭到人，都算打在墙上（防止穿墙/隔山打牛）。
		如果针没扎到墙，或者是空气，或者墙在人后面 -> 判定宽容命中生效，算你打中了！
	 */

	FHitResult Impact;

	// 如果有物体被命中，则追踪并处理即时命中
	// 首先不使用扫描半径进行追踪
	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE)
	{
		// 发射一条 SweepRadius 0 的线
		Impact = WeaponTrace(StartTrace, EndTrace, 0.0f, bIsSimulated,  OutHits);
	}

	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE)
	{
		// 没打中，提升手感，发射一个圆柱的检测
		if (SweepRadius > 0.0f)
		{
			TArray<FHitResult> SweepHits;
			Impact = WeaponTrace(StartTrace, EndTrace, SweepRadius, bIsSimulated, SweepHits);

			// 如果启用了扫描半径的轨迹命中了一个 pawn，检查是否应该使用其命中结果
			const int32 FirstPawnIdx = FindFirstPawnHitResult(SweepHits);
			if (SweepHits.IsValidIndex(FirstPawnIdx))
			{
				// 官方注释：
				// 如果我们的线追踪中存在阻塞命中，且该命中发生在扫描命中（SweepHits）中、
				// 在我们命中 pawn 之前，那么我们应该直接使用初始的命中结果，因为 pawn 的命中应该会被阻塞。

				// 检查在“打中这个人”之前，细线有没有打中墙？
				// 注意：这里的 OutHits 其实是第一步 LineTrace 的结果
				bool bUseSweepHits = true;
				for (int32 Idx = 0; Idx < FirstPawnIdx; ++Idx)
				{
					const FHitResult& CurHitResult = SweepHits[Idx];

					auto Pred = [&CurHitResult](const FHitResult& Other)
					{
						return Other.HitObjectHandle == CurHitResult.HitObjectHandle;
					};
					if (CurHitResult.bBlockingHit && OutHits.ContainsByPredicate(Pred))
					{
						bUseSweepHits = false;
						break;
					}
				}

				if (bUseSweepHits)
				{
					OutHits = SweepHits;
				}
			}
		}
	}

	return Impact;
}

void UEqZeroGameplayAbility_RangedWeapon::PerformLocalTargeting(OUT TArray<FHitResult>& OutHits)
{
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	UEqZeroRangedWeaponInstance* WeaponData = GetWeaponInstance();
	if (AvatarPawn && AvatarPawn->IsLocallyControlled() && WeaponData)
	{
		// 首先要封装一个开火输入结构
		FRangedWeaponFiringInput InputData;
		InputData.WeaponData = WeaponData;
		InputData.bCanPlayBulletFX = (AvatarPawn->GetNetMode() != NM_DedicatedServer); // 客户端需要播放子弹特效，服务器不需要

		// 当玩家靠近墙壁时，这里应该执行更复杂的逻辑(官方注释)
		const FTransform TargetTransform = GetTargetingTransform(AvatarPawn, EEqZeroAbilityTargetingSource::CameraTowardsFocus);
		InputData.AimDir = TargetTransform.GetUnitAxis(EAxis::X);
		InputData.StartTrace = TargetTransform.GetTranslation();

		InputData.EndAim = InputData.StartTrace + InputData.AimDir * WeaponData->GetMaxDamageRange();

#if ENABLE_DRAW_DEBUG
		if (EqZeroConsoleVariables::DrawBulletTracesDuration > 0.0f)
		{
			static float DebugThickness = 2.0f;
			DrawDebugLine(GetWorld(), InputData.StartTrace, InputData.StartTrace + (InputData.AimDir * 100.0f), FColor::Yellow, false, EqZeroConsoleVariables::DrawBulletTracesDuration, 0, DebugThickness);
		}
#endif

		// 弹道模拟
		TraceBulletsInCartridge(InputData, OutHits);
	}
}

void UEqZeroGameplayAbility_RangedWeapon::TraceBulletsInCartridge(const FRangedWeaponFiringInput& InputData, OUT TArray<FHitResult>& OutHits)
{
	UEqZeroRangedWeaponInstance* WeaponData = InputData.WeaponData;
	check(WeaponData);

	// 遍历每一发子弹，进行弹道模拟。通常是1，特殊情况是散弹枪
	const int32 BulletsPerCartridge = WeaponData->GetBulletsPerCartridge();
	for (int32 BulletIndex = 0; BulletIndex < BulletsPerCartridge; ++BulletIndex)
	{
		// 扩散角度，单位是度，他收到热度的影响, 例如持续开火会增加热度，增加热度会增加扩散角度
		const float BaseSpreadAngle = WeaponData->GetCalculatedSpreadAngle();

		// 扩散系数，例如蹲下，静止不动，瞄准，会拥有一个<1的系数。乘到角度上就是射的更准了，在空中更不准。
		const float SpreadAngleMultiplier = WeaponData->GetCalculatedSpreadAngleMultiplier();

		// 两个相乘得到实际的扩散角度
		const float ActualSpreadAngle = BaseSpreadAngle * SpreadAngleMultiplier;

		const float HalfSpreadAngleInRadians = FMath::DegreesToRadians(ActualSpreadAngle * 0.5f);
		const FVector BulletDir = VRandConeNormalDistribution(InputData.AimDir, HalfSpreadAngleInRadians, WeaponData->GetSpreadExponent());

		const FVector EndTrace = InputData.StartTrace + (BulletDir * WeaponData->GetMaxDamageRange());
		FVector HitLocation = EndTrace;

		TArray<FHitResult> AllImpacts;

		// 对这样的射线做一次检测
		FHitResult Impact = DoSingleBulletTrace(InputData.StartTrace, EndTrace, WeaponData->GetBulletTraceSweepRadius(), false, AllImpacts);
		const AActor* HitActor = Impact.GetActor();

		if (HitActor)
		{
#if ENABLE_DRAW_DEBUG
			if (EqZeroConsoleVariables::DrawBulletHitDuration > 0.0f)
			{
				DrawDebugPoint(GetWorld(), Impact.ImpactPoint, EqZeroConsoleVariables::DrawBulletHitRadius, FColor::Red, false, EqZeroConsoleVariables::DrawBulletHitRadius);
			}
#endif

			// 合并所有的命中结果，这里可能是 霰弹枪中的一发射线的检测
			if (AllImpacts.Num() > 0)
			{
				OutHits.Append(AllImpacts);
			}
			HitLocation = Impact.ImpactPoint;
		}

		// 确保 OutHits 中始终有一个条目，这样方向就可以用于追踪器等。
		if (OutHits.Num() == 0)
		{
			if (!Impact.bBlockingHit)
			{
				// 在轨迹末尾找到伪造的 “影响”
				Impact.Location = EndTrace;
				Impact.ImpactPoint = EndTrace;
			}

			OutHits.Add(Impact);
		}
	}
}

void UEqZeroGameplayAbility_RangedWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Bind target data callback
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// 更新武器的开火时间
	UEqZeroRangedWeaponInstance* WeaponData = GetWeaponInstance();
	check(WeaponData);
	WeaponData->UpdateFiringTime();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UEqZeroGameplayAbility_RangedWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// 能力结束时，消耗目标数据并移除委托
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

void UEqZeroGameplayAbility_RangedWeapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		// 开启预测窗口，用于网络同步时的平滑处理
		FScopedPredictionWindow	ScopedPrediction(MyAbilityComponent);

		// 移动语义获取数据，避免拷贝。获取目标数据的所有权，以确保不会有游戏代码的回调在我们不知情的情况下使数据失效。
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		// 如果是客户端，必须调用这个函数
		if (const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority())
		{
			MyAbilityComponent->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, MyAbilityComponent->ScopedPredictionKey);
		}

		const bool bIsTargetDataValid = true;
		bool bProjectileWeapon = false;

#if WITH_SERVER_CODE
		// 服务器的命中确认

		if (!bProjectileWeapon)
		{
			if (AController* Controller = GetControllerFromActorInfo())
			{
				if (Controller->GetLocalRole() == ROLE_Authority)
				{
					if (UEqZeroWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<UEqZeroWeaponStateComponent>())
					{
						TArray<uint8> HitReplaces;
						for (uint8 i = 0; (i < LocalTargetDataHandle.Num()) && (i < 255); ++i)
						{
							if (const FEqZeroGameplayAbilityTargetData_SingleTargetHit* SingleTargetHit = static_cast<const FEqZeroGameplayAbilityTargetData_SingleTargetHit*>(LocalTargetDataHandle.Get(i)))
							{
								if (SingleTargetHit->bHitReplaced)
								{
									HitReplaces.Add(i);
								}
							}
						}

						WeaponStateComponent->ClientConfirmTargetData(LocalTargetDataHandle.UniqueId, bIsTargetDataValid, HitReplaces);
					}
				}
			}
		}
#endif //WITH_SERVER_CODE


		// 检查是否还有弹药或满足其他消耗条件
		if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			// 增加武器散布（后坐力影响）
			UEqZeroRangedWeaponInstance* WeaponData = GetWeaponInstance();
			check(WeaponData);
			WeaponData->AddSpread();

			// 蓝图挂钩：触发伤害、播放特效等
			OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
		}
		else
		{
			// 如果无法提交（例如没子弹了），则打印警告并结束技能
			UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("Weapon ability %s failed to commit (bIsTargetDataValid=%d)"), *GetPathName(), bIsTargetDataValid ? 1 : 0);
			K2_EndAbility();
		}
	}

	// 标记数据已被处理，防止重复使用
	MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UEqZeroGameplayAbility_RangedWeapon::StartRangedWeaponTargeting()
{
	check(CurrentActorInfo);

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	check(AvatarActor);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	AController* Controller = GetControllerFromActorInfo();
	check(Controller);
	UEqZeroWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<UEqZeroWeaponStateComponent>();

	// 预测窗口
	// 它告诉系统：接下来的操作（如造成伤害、消耗子弹）是我客户端先“猜”的，请在服务器确认前先暂时这么显示。
	// 如果没有客户端的 CallServerSetReplicatedTargetData 会有问题
	FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, CurrentActivationInfo.GetActivationPredictionKey());

	// 执行本地命中检测
	// 读取摄像机位置、计算散布角度、发射射线（LineTrace/Sweep），并最终填充 FoundHits 数组。这是实际算出“我打中了谁”的一步
	TArray<FHitResult> FoundHits;
	PerformLocalTargeting(FoundHits);

	// 构建 TargetData
	FGameplayAbilityTargetDataHandle TargetData;
	TargetData.UniqueId = WeaponStateComponent ? WeaponStateComponent->GetUnconfirmedServerSideHitMarkerCount() : 0;

	if (FoundHits.Num() > 0)
	{
		const int32 CartridgeID = FMath::Rand();

		for (const FHitResult& FoundHit : FoundHits)
		{
			// 创建 GAS 标准的单点命中数据结构
			FEqZeroGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FEqZeroGameplayAbilityTargetData_SingleTargetHit();
			NewTargetData->HitResult = FoundHit;
			NewTargetData->CartridgeID = CartridgeID;
			
			TargetData.Add(NewTargetData);
		}
	}

	// 在本地先记录这次命中，以便在UI上立即显示（比如先画个白色的X）
	// 虽然还没经服务器确认，但为了手感需要即时反馈
	if (WeaponStateComponent != nullptr)
	{
		WeaponStateComponent->AddUnconfirmedServerSideHitMarkers(TargetData, FoundHits);
	}
	

	// 立即处理这组数据
	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}
