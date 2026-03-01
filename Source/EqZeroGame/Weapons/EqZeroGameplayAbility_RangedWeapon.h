// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Equipment/EqZeroGameplayAbility_FromEquipment.h"

#include "EqZeroGameplayAbility_RangedWeapon.generated.h"

enum ECollisionChannel : int;

class APawn;
class UEqZeroRangedWeaponInstance;
class UObject;
struct FCollisionQueryParams;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;
struct FGameplayTag;
struct FGameplayTagContainer;

/** 定义能力从何处开始追踪以及应该面向何处 */
UENUM(BlueprintType)
enum class EEqZeroAbilityTargetingSource : uint8
{
	// 从玩家的相机朝向相机焦点
	CameraTowardsFocus,

	// 从角色的中心出发，沿角色的朝向
	PawnForward,

	// 从角色的中心出发，朝向相机焦点
	PawnTowardsFocus,

	// 从武器的枪口或位置出发，沿角色的朝向
	WeaponForward,

	// 从武器的枪口或位置出发，朝向相机焦点
	WeaponTowardsFocus,

	// 自定义蓝图指定的源位置
	Custom
};

/**
 * UEqZeroGameplayAbility_RangedWeapon
 *
 * 远程武器开火技能
 */
UCLASS()
class UEqZeroGameplayAbility_RangedWeapon : public UEqZeroGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="EqZero|Ability")
	UEqZeroRangedWeaponInstance* GetWeaponInstance() const;

	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

protected:
	struct FRangedWeaponFiringInput
	{
		// 武器开火的输入，起点，终点，方向
		
		FVector StartTrace;
		FVector EndAim;
		FVector AimDir;

		UEqZeroRangedWeaponInstance* WeaponData = nullptr;
		bool bCanPlayBulletFX = false;

		FRangedWeaponFiringInput()
			: StartTrace(ForceInitToZero)
			, EndAim(ForceInitToZero)
			, AimDir(ForceInitToZero)
		{
		}
	};

protected:
	/*
	 * 一次 DoSingleBulletTrace 可能有多个命中，这个是在一排命中结果中，找到第一个命中 Pawn和Pawn附着物的索引的
	 */
	static int32 FindFirstPawnHitResult(const TArray<FHitResult>& HitResults);

	/*
	 * 武器射线检测
	 * SweepRadius == 0 就是线检测，否则就是圆柱体检测来容错
	 */
	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHitResults) const;

	/*
	 * 每一颗子弹都会做一次这个 Single Trace
	 */
	FHitResult DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHits) const;

	void TraceBulletsInCartridge(const FRangedWeaponFiringInput& InputData, OUT TArray<FHitResult>& OutHits);

	/*
	 * 弹道命中检测 - 数据准备和辅助接口
	 */
	virtual void AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const;

	// 辅助接口 获取碰撞Channel
	virtual ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const;

	void PerformLocalTargeting(OUT TArray<FHitResult>& OutHits);

	FVector GetWeaponTargetingSourceLocation() const;
	
	FTransform GetTargetingTransform(APawn* SourcePawn, EEqZeroAbilityTargetingSource Source) const;

	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	UFUNCTION(BlueprintCallable)
	void StartRangedWeaponTargeting();

	// target data 准备好的时候，处理伤害特效等
	UFUNCTION(BlueprintImplementableEvent)
	void OnRangedWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClassCppUse;
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};
