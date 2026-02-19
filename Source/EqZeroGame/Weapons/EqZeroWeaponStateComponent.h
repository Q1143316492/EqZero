// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "GameplayTagContainer.h"

#include "EqZeroWeaponStateComponent.generated.h"

class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;
struct FGameplayEffectContextHandle;
struct FHitResult;

// 在准星中会显示远程武器命中的命中标记
// 当对敌人造成伤害时，会显示一个 “成功” 的命中标记。
struct FEqZeroScreenSpaceHitLocation
{
	/** 视口屏幕空间中的命中位置 */
	FVector2D Location;	
	FGameplayTag HitZone;
	bool bShowAsSuccess = false;
};

struct FEqZeroServerSideHitMarkerBatch
{
	FEqZeroServerSideHitMarkerBatch() { }

	FEqZeroServerSideHitMarkerBatch(uint8 InUniqueId) :
		UniqueId(InUniqueId)
	{ }

	TArray<FEqZeroScreenSpaceHitLocation> Markers;

	uint8 UniqueId = 0;
};

// 追踪武器状态和近期确认的命中标记，以便在屏幕上显示
// 通过GameFeatureAction 加到 Controller上
UCLASS()
class UEqZeroWeaponStateComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	UEqZeroWeaponStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*
	 * 当服务器确认命中数据时调用 ClientConfirmTargetData 来通知客户端结果。
	 * 客户端会根据结果来更新命中标记的显示，例如显示成功
	 */
	UFUNCTION(Client, Reliable)
	void ClientConfirmTargetData(uint16 UniqueId, bool bSuccess, const TArray<uint8>& HitReplaces);

	/*
	 * 客户端本地计算完命中后，会调用 AddUnconfirmedServerSideHitMarkers 将命中数据暂时存起来。
	 * 这样可以在等待服务器确认前就能立即在 UI 上做一些预表现（如果是需要极快响应的设计）。
	 */
	void AddUnconfirmedServerSideHitMarkers(const FGameplayAbilityTargetDataHandle& InTargetData, const TArray<FHitResult>& FoundHits);

	/**
	 * 更新该玩家最后一次主动造成伤害的时间（没有调用？）
	 * - 官方留了注释，我们只更新射击命中的效果，可能没有GE触发命中效果的流程，所以预留，没有使用
	 */
	void UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext);

	/**
	 * 获取该玩家发起伤害的最新位置数组，位于屏幕空间中
	 * - UI调用获取
	 * - 0.1s前的会被清空
	 */
	void GetLastWeaponDamageScreenLocations(TArray<FEqZeroScreenSpaceHitLocation>& WeaponDamageScreenLocations)
	{
		WeaponDamageScreenLocations = LastWeaponDamageScreenLocations;
	}

	/**
	 * 返回自上次（ outgoing ）伤害命中通知发生以来经过的时间
	 */
	double GetTimeSinceLastHitNotification() const;

	int32 GetUnconfirmedServerSideHitMarkerCount() const
	{
		return UnconfirmedServerSideHitMarkers.Num();
	}

protected:
	virtual bool ShouldShowHitAsSuccess(const FHitResult& Hit) const;

	virtual bool ShouldUpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext) const;

	void ActuallyUpdateDamageInstigatedTime();

private:
	/**
	 * 上一次伤害标记的时间，如果是网络延迟 0.1s 以前的标记也不需要表现了
	 */
	double LastWeaponDamageInstigatedTime = 0.0;

	/**
	 * 我们最近发起的武器伤害（已确认命中）的屏幕空间位置
	 */
	TArray<FEqZeroScreenSpaceHitLocation> LastWeaponDamageScreenLocations;

	/**
	 * 未经证实的命中
	 */
	TArray<FEqZeroServerSideHitMarkerBatch> UnconfirmedServerSideHitMarkers;
};
