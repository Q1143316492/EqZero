// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroRangedWeaponInstance.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/EqZeroCameraComponent.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Weapons/EqZeroWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroRangedWeaponInstance)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EqZero_Weapon_SteadyAimingCamera, "EqZero.Weapon.SteadyAimingCamera");

UEqZeroRangedWeaponInstance::UEqZeroRangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 2.0f);
}

void UEqZeroRangedWeaponInstance::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

#if WITH_EDITOR
void UEqZeroRangedWeaponInstance::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateDebugVisualization();
}

void UEqZeroRangedWeaponInstance::UpdateDebugVisualization()
{
	ComputeHeatRange(Debug_MinHeat, Debug_MaxHeat);
	ComputeSpreadRange(Debug_MinSpreadAngle, Debug_MaxSpreadAngle);
	Debug_CurrentHeat = CurrentHeat;
	Debug_CurrentSpreadAngle = CurrentSpreadAngle;
	Debug_CurrentSpreadAngleMultiplier = CurrentSpreadAngleMultiplier;

	UE_LOG(LogTemp, Log, TEXT("Heat: %.2f  Spread: %.2f  Multiplier: %.2f"), CurrentHeat, CurrentSpreadAngle, CurrentSpreadAngleMultiplier);
}
#endif

void UEqZeroRangedWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// 计算热度的最大最小范围，然后取中值为当前热度
	float MinHeatRange;
	float MaxHeatRange;
	ComputeHeatRange(MinHeatRange, MaxHeatRange);
	CurrentHeat = (MinHeatRange + MaxHeatRange) * 0.5f; // 对于手枪，这个范围是 0, 8，所以初始热度是 4

	//然后根据这条曲线，曲线类似 y=kx正比例，但是有弧度，结果取了个中点。对于手枪这个值是 7.25
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

	// 这些倍数目前都是 1.0
	CurrentSpreadAngleMultiplier = 1.0f;
	StandingStillMultiplier = 1.0f;
	JumpFallMultiplier = 1.0f;
	CrouchingMultiplier = 1.0f;
}

void UEqZeroRangedWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
}

void UEqZeroRangedWeaponInstance::Tick(float DeltaSeconds)
{
	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);
	
	const bool bMinSpread = UpdateSpread(DeltaSeconds);
	const bool bMinMultipliers = UpdateMultipliers(DeltaSeconds);

	// 不处于任何扩散状态，且配置允许首发精准度。那么下一次射击，就没有扩散，超级准的一枪
	bHasFirstShotAccuracy = bAllowFirstShotAccuracy && bMinMultipliers && bMinSpread;

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

void UEqZeroRangedWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
{
	float Min1;
	float Max1;
	HeatToHeatPerShotCurve.GetRichCurveConst()->GetTimeRange(Min1, Max1); // 0, 0

	float Min2;
	float Max2;
	HeatToCoolDownPerSecondCurve.GetRichCurveConst()->GetTimeRange(Min2, Max2); // 0, 0

	float Min3;
	float Max3;
	HeatToSpreadCurve.GetRichCurveConst()->GetTimeRange(Min3, Max3); // 0, 8

	// 对于手枪pistol 最后结果是 0, 8
	MinHeat = FMath::Min(FMath::Min(Min1, Min2), Min3);
	MaxHeat = FMath::Max(FMath::Max(Max1, Max2), Max3);
}

void UEqZeroRangedWeaponInstance::ComputeSpreadRange(float& MinSpread, float& MaxSpread)
{
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(MinSpread, MaxSpread);
}

void UEqZeroRangedWeaponInstance::AddSpread()
{
	// 根据配置，手枪每次增加热度每次12
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);

	// 同时也更新扩散角度
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

float UEqZeroRangedWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	// 伤害计算那边会获取武器的距离衰减曲线。技能的 Execution 那里调用，超过距离就是0
	const FRichCurve* Curve = DistanceDamageFalloff.GetRichCurveConst();
	return Curve->HasAnyData() ? Curve->Eval(Distance) : 1.0f;
}

float UEqZeroRangedWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	float CombinedMultiplier = 1.0f;
	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
			{
				CombinedMultiplier *= *pTagMultiplier;
			}
		}
	}

	return CombinedMultiplier;
}

bool UEqZeroRangedWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	const float TimeSinceFired = GetWorld()->TimeSince(LastFireTime);

	// 开火后，超过这个时间，热度下降
	if (TimeSinceFired > SpreadRecoveryCooldownDelay)
	{
		// 根据当前热量决定冷却速度（例如：热量越高，冷却越快）。
		// 但是 HeatToCoolDownPerSecondCurve 曲线目前只有一个 (0, 10)的点
		const float CooldownRate = HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat);

		// 减少热量，然后修改当前扩散角度
		CurrentHeat = ClampHeat(CurrentHeat - (CooldownRate * DeltaSeconds));
		CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
	}
	
	float MinSpread;
	float MaxSpread;
	ComputeSpreadRange(MinSpread, MaxSpread);

	// 是否已经恢复到了最小扩散状态（用于判断首发精准度）, 最小扩散点是曲线的中点，上面几行
	return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
}

bool UEqZeroRangedWeaponInstance::UpdateMultipliers(float DeltaSeconds)
{
	const float MultiplierNearlyEqualThreshold = 0.05f;

	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);
	UCharacterMovementComponent* CharMovementComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent());

	/*
	 * 计算静止 对于准星 散射的影响
	 * - 把速度PawnSpeed MapRangeClamped 对于手枪数值是 [80, 80+20] -> [0.9, 1.0]，也就是说当速度在0-80时，乘数是0.9，超过100时乘数是1.0，在80-100之间平滑过渡
	 * - StandingStillMultiplier 会平滑过渡到这个目标值，过渡速率由 TransitionRate_StandingStill 默认5.f
	 * - bStandingStillMultiplierAtMin 用于判断当前乘数是否已经接近最小值（也就是玩家是否基本上处于静止状态）
	 */
	const float PawnSpeed = Pawn->GetVelocity().Size();
	const float MovementTargetValue = FMath::GetMappedRangeValueClamped(
		FVector2D(StandingStillSpeedThreshold, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange),
		FVector2D(SpreadAngleMultiplier_StandingStill, 1.0f),
		PawnSpeed);
	StandingStillMultiplier = FMath::FInterpTo(StandingStillMultiplier, MovementTargetValue, DeltaSeconds, TransitionRate_StandingStill);
	const bool bStandingStillMultiplierAtMin = FMath::IsNearlyEqual(StandingStillMultiplier, SpreadAngleMultiplier_StandingStill, SpreadAngleMultiplier_StandingStill*0.1f);

	/*
	 * 蹲伏时的乘数
	 * - 如果角色正在蹲伏，目标值是 SpreadAngleMultiplier_Crouching (手枪数值0.65)
	 * - CrouchingMultiplier 会平滑过渡到这个目标值，过渡速率由 TransitionRate_Crouching 默认5.f
	 */
	const bool bIsCrouching = (CharMovementComp != nullptr) && CharMovementComp->IsCrouching();
	const float CrouchingTargetValue = bIsCrouching ? SpreadAngleMultiplier_Crouching : 1.0f;
	CrouchingMultiplier = FMath::FInterpTo(CrouchingMultiplier, CrouchingTargetValue, DeltaSeconds, TransitionRate_Crouching);
	const bool bCrouchingMultiplierAtTarget = FMath::IsNearlyEqual(CrouchingMultiplier, CrouchingTargetValue, MultiplierNearlyEqualThreshold);

	/*
	 * 跳跃/下落时的乘数
	 * - 如果角色正在跳跃或下落，目标值是 SpreadAngleMultiplier_JumpingOrFalling (手枪数值1.25)
	 * - JumpFallMultiplier 会平滑过渡到这个目标值，过渡速率由 TransitionRate_JumpingOrFalling 默认5.f
	 */
	const bool bIsJumpingOrFalling = (CharMovementComp != nullptr) && CharMovementComp->IsFalling();
	const float JumpFallTargetValue = bIsJumpingOrFalling ? SpreadAngleMultiplier_JumpingOrFalling : 1.0f;
	JumpFallMultiplier = FMath::FInterpTo(JumpFallMultiplier, JumpFallTargetValue, DeltaSeconds, TransitionRate_JumpingOrFalling);
	const bool bJumpFallMultiplerIs1 = FMath::IsNearlyEqual(JumpFallMultiplier, 1.0f, MultiplierNearlyEqualThreshold);

	/*
	 * 判断是否在瞄准 然后计算贡献
	 * - 
	 */
	float AimingAlpha = 0.0f;
	if (const UEqZeroCameraComponent* CameraComponent = UEqZeroCameraComponent::FindCameraComponent(Pawn))
	{
		float TopCameraWeight;
		FGameplayTag TopCameraTag;
		CameraComponent->GetBlendInfo(TopCameraWeight, TopCameraTag);

		AimingAlpha = (TopCameraTag == TAG_EqZero_Weapon_SteadyAimingCamera) ? TopCameraWeight : 0.0f;
	}
	const float AimingMultiplier = FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, 1.0f),
		FVector2D(1.0f, SpreadAngleMultiplier_Aiming),
		AimingAlpha);
	const bool bAimingMultiplierAtTarget = FMath::IsNearlyEqual(AimingMultiplier, SpreadAngleMultiplier_Aiming, KINDA_SMALL_NUMBER);

	/*
	 * 把上面的几个乘数综合起来，得到最终的扩散角乘数 CurrentSpreadAngleMultiplier
	 */
	const float CombinedMultiplier = AimingMultiplier * StandingStillMultiplier * CrouchingMultiplier * JumpFallMultiplier;
	CurrentSpreadAngleMultiplier = CombinedMultiplier;

	// 需要处理这些点差乘数，它们表明我们并未处于最小点差状态。
	return bStandingStillMultiplierAtMin && bCrouchingMultiplierAtTarget && bJumpFallMultiplerIs1 && bAimingMultiplierAtTarget;
}
