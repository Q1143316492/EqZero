// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"

#include "EqZeroWeaponInstance.h"
#include "AbilitySystem/EqZeroAbilitySourceInterface.h"

#include "EqZeroRangedWeaponInstance.generated.h"

class UPhysicalMaterial;

/**
 * UEqZeroRangedWeaponInstance
 *
 * 远程武器实例，继承自UEqZeroWeaponInstance，增加了远程武器特有的属性和逻辑，例如弹药、扩散等。
 */
UCLASS()
class UEqZeroRangedWeaponInstance : public UEqZeroWeaponInstance, public IEqZeroAbilitySourceInterface
{
	GENERATED_BODY()

public:
	UEqZeroRangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	void UpdateDebugVisualization();
#endif

	int32 GetBulletsPerCartridge() const
	{
		return BulletsPerCartridge;
	}
	
	float GetCalculatedSpreadAngle() const
	{
		// 返回当前的扩散角度（以度为单位，直径方向）
		return CurrentSpreadAngle;
	}

	float GetCalculatedSpreadAngleMultiplier() const
	{
		return bHasFirstShotAccuracy ? 0.0f : CurrentSpreadAngleMultiplier;
	}

	bool HasFirstShotAccuracy() const
	{
		return bHasFirstShotAccuracy;
	}

	float GetSpreadExponent() const
	{
		return SpreadExponent;
	}

	float GetMaxDamageRange() const
	{
		return MaxDamageRange;
	}

	float GetBulletTraceSweepRadius() const
	{
		return BulletTraceSweepRadius;
	}

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MinHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MaxHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread|Fire Params", meta=(ForceUnits=deg))
	float Debug_MinSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread|Fire Params", meta=(ForceUnits=deg))
	float Debug_MaxSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread Debugging")
	float Debug_CurrentHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread Debugging", meta = (ForceUnits=deg))
	float Debug_CurrentSpreadAngle = 0.0f;

	// The current *combined* spread angle multiplier
	UPROPERTY(VisibleAnywhere, Category = "Spread Debugging", meta=(ForceUnits=x))
	float Debug_CurrentSpreadAngleMultiplier = 1.0f;

#endif

	/*
	* 扩散指数会影响射击在中心线周围的聚集紧密程度
	* 当武器存在扩散（并非完美精度）时。数值越高，射击就越靠近中心（默认值为 1.0，意味着在扩散范围内均匀分布）
	* 落实到计算上，就是 pow(random(), SpreadExponent)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=0.1), Category="Spread|Fire Params")
	float SpreadExponent = 1.0f;

	/*
	 * 一条将热量映射到散布角度的曲线
	 * 该曲线的 X 范围通常设定武器的最小 / 最大热量范围
	 * 该曲线的 Y 范围用于定义最小和最大散布角度
	 *
	 * 2.5 ~ 12
	 * 
	 */
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
	FRuntimeFloatCurve HeatToSpreadCurve;

	/*
	 * 一条将当前热量映射到单次射击会进一步 “加热” 的量的曲线
	 * 这通常是一条具有单个数据点的平坦曲线，该数据点表明一次射击会增加多少热量，
	 * 但也可以是其他形状，以实现诸如通过逐渐增加更多热量来惩罚过热等功能。
	 */
	UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;
	
	/*
	* 一条将当前热量映射到每秒热量冷却速率的曲线
	* 这通常是一条具有单个数据点的平直曲线，该数据点表明热量消散的速度，但也可以是其他形状，比如通过在高热量时减缓恢复速度来惩罚过热行为。
	*/
	UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	// 开火后到扩散冷却恢复开始前的时间（以秒为单位）例如手枪配置了0.2
	UPROPERTY(EditAnywhere, Category="Spread|Fire Params", meta=(ForceUnits=s))
	float SpreadRecoveryCooldownDelay = 0.0f;

	// 开关。当玩家和武器的散布都处于最小值时，武器应该拥有完美的精度吗
	UPROPERTY(EditAnywhere, Category="Spread|Fire Params")
	bool bAllowFirstShotAccuracy = false;

	// 处于瞄准相机模式时的倍率，手枪是0.65
	UPROPERTY(EditAnywhere, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Aiming = 1.0f;

	
	/*
	 * 计算静止状态对于准星散布的影响的时候
	 * 
	 * 把当前速度 Pawn->GetVelocity().Size() 用 MapRangeClamped
	 * [StandingStillSpeedThreshold, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange] -> [SpreadAngleMultiplier_StandingStill, 1.0f]
	 * 对于 手枪的数值是 速度大小 MapRangeClamped [80, 100] -> [0.9, 1.0]
	 *
	 * 然后 StandingStillMultiplier 按照 TransitionRate_StandingStill 的速率平滑过渡到这个目标值
	 *
	 * 最终对计算的贡献主要是 StandingStillMultiplier
	 */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_StandingStill = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_StandingStill = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillSpeedThreshold = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillToMovingSpeedRange = 20.0f;

	/*
	 * 计算 蹲伏状态 对于准星散布的影响的时候
	 * - 如果角色正在蹲伏，目标值是 SpreadAngleMultiplier_Crouching (手枪数值0.65)
	 * - CrouchingMultiplier 会平滑过渡到这个目标值，过渡速率由 TransitionRate_Crouching 默认5.f
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Crouching = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_Crouching = 5.0f;

	/* 
	 * 计算 跳跃/下落状态 对于准星散布的影响的时候
	 * - 如果角色正在跳跃或下落，目标值是 SpreadAngleMultiplier_JumpingOrFalling (手枪数值1.25)
	 * - JumpFallMultiplier 会平滑过渡到这个目标值，过渡速率由 TransitionRate_JumpingOrFalling 默认5.f
	 */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Player Params")
	float TransitionRate_JumpingOrFalling = 5.0f;

	// ==================== 分割线 ========================
	
	// 每次设计多少发子弹，一般是1。但对于霰弹枪等武器可能会更多。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Config")
	int32 BulletsPerCartridge = 1;

	// 武器的最大伤害范围（以厘米为单位）。超过这个范围，武器将不再造成伤害。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Config", meta=(ForceUnits=cm))
	float MaxDamageRange = 25000.0f;

	/*
	 * 射线检测可能太细，玩家打不准
	 * 允许一个 BulletTraceSweepRadius 半径的圆柱做检测 例如手枪是6cm
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Config", meta=(ForceUnits=cm))
	float BulletTraceSweepRadius = 0.0f;

	/*
	 * 伤害的距离曲线，太远就没伤害了。
	 *
	 * 手枪 2000cm 外就没伤害了
	 * 一条曲线，用于将距离（以厘米为单位）映射到相关游戏玩法效果基础伤害的乘数上
	 * 如果这条曲线中没有数据，那么该武器被视为没有随距离产生的衰减
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Config")
	FRuntimeFloatCurve DistanceDamageFalloff;

	/*
	* 影响伤害计算方式的特殊标签列表
	* 这些标签将与被击中物体的物理材质中的标签进行比较
	* 如果存在多个标签，乘数将以相乘的方式组合
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Config")
	TMap<FGameplayTag, float> MaterialDamageMultiplier;

private:
	/*
	 * 该武器上次开火距今的时间（相对于世界时间）
	 * - 接口返回的是 GetTimeSeconds() - LastFireTime 这里默认偏移是0，直接是timeSeconds
	 */
	double LastFireTime = 0.0;

	/*
	 * 热度的概念
	 * - 开火会影响子弹扩散(还有很多因素)，所以维护了一个热度的概念用于计算。
	 */
	float CurrentHeat = 0.0f;

	// 当前的扩散角度（以度为单位，直径方向）。这个值是基于当前热度通过HeatToSpreadCurve计算得出的，并且会随着开火和冷却而变化。
	float CurrentSpreadAngle = 0.0f;

	/**
	 *是否具有第一发子弹的准确性（如果bAllowFirstShotAccuracy为true）。
	 * - 如果为true，则当前的扩散角度乘数将被视为0，提供完美的准确性。
	 * - 这个值在开火时设置，并且在玩家移动、跳跃、蹲下或瞄准时会被重置，以确保第一发子弹的准确性只适用于静止射击。
	 */
	bool bHasFirstShotAccuracy = false;

	/*
	 * 当前的组合扩散角乘数。
	 * - 这个乘数是基于玩家当前的状态（例如：站立不动、跳跃/下落、蹲下）计算得出的，并且会随着玩家状态的变化而平滑过渡。
	 * - 缓存一下直接给外部读
	 */
	float CurrentSpreadAngleMultiplier = 1.0f;

	/*
	 * 当前站立不动的乘数
	 * 当前跳跃/下落的乘数
	 * 当前蹲下的乘数
	 */
	float StandingStillMultiplier = 1.0f;
	float JumpFallMultiplier = 1.0f;
	float CrouchingMultiplier = 1.0f;

public:
	void Tick(float DeltaSeconds);

	//~UEqZeroEquipmentInstance interface
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;
	//~End of UEqZeroEquipmentInstance interface

	void AddSpread();

	//~IEqZeroAbilitySourceInterface interface
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	//~End of IEqZeroAbilitySourceInterface interface

private:
	void ComputeSpreadRange(float& MinSpread, float& MaxSpread);
	void ComputeHeatRange(float& MinHeat, float& MaxHeat);

	inline float ClampHeat(float NewHeat)
	{
		float MinHeat;
		float MaxHeat;
		ComputeHeatRange(MinHeat, MaxHeat);

		return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
	}

	// Updates the spread and returns true if the spread is at minimum
	bool UpdateSpread(float DeltaSeconds);

	// Updates the multipliers and returns true if they are at minimum
	bool UpdateMultipliers(float DeltaSeconds);
};
