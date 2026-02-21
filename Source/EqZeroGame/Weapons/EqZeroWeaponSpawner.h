// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "EqZeroWeaponSpawner.generated.h"

#define UE_API EQZEROGAME_API

namespace EEndPlayReason { enum Type : int; }

class APawn;
class UCapsuleComponent;
class UEqZeroInventoryItemDefinition;
class UEqZeroWeaponPickupDefinition;
class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FFrame;
struct FGameplayTag;
struct FHitResult;

/**
 * AEqZeroWeaponSpawner
 *
 * 武器生成器 Actor，负责在地图中生成可拾取的武器。
 * 玩家走入碰撞体积时会触发拾取逻辑，拾取后进入冷却，
 * 冷却结束后武器重新出现并可再次被拾取。
 */
UCLASS(MinimalAPI, Blueprintable, BlueprintType)
class AEqZeroWeaponSpawner : public AActor
{
	GENERATED_BODY()

public:
	UE_API AEqZeroWeaponSpawner();

protected:
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UE_API virtual void Tick(float DeltaTime) override;

	UE_API void OnConstruction(const FTransform& Transform) override;

protected:
	// 武器生成器的数据资产配置，定义武器外观、音效、特效等
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	TObjectPtr<UEqZeroWeaponPickupDefinition> WeaponDefinition;

	// 武器当前是否可被拾取，通过 OnRep 同步到客户端
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponAvailability, Category = "EqZero|WeaponPickup")
	bool bIsWeaponAvailable;

	// 武器被拾取后到重新生成的冷却时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	float CoolDownTime;

	// 武器变为可用后延迟多久检查是否有 Pawn 站在生成器上，
	// 给 bIsWeaponAvailable 的 OnRep 足够时间触发并播放特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	float CheckExistingOverlapDelay;

	// 武器重新生成进度（0~1），用于驱动 UI 倒计时指示器
	UPROPERTY(BlueprintReadOnly, Transient, Category = "EqZero|WeaponPickup")
	float CoolDownPercentage;

	// 是否在玩家已拥有该武器时仍给予弹药，默认为 true
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	bool bGiveAmmoForDuplicatedWeapon = true;
public:
	// 根碰撞体积，用于检测玩家走入/离开
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;

	// 生成器底座静态网格体
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	// 悬浮展示武器的静态网格体
	UPROPERTY(BlueprintReadOnly, Category = "EqZero|WeaponPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 武器网格体每秒旋转速度（度/秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EqZero|WeaponPickup")
	float WeaponMeshRotationSpeed;

	// 武器冷却定时器句柄
	FTimerHandle CoolDownTimerHandle;

	// 延迟检查重叠定时器句柄
	FTimerHandle CheckOverlapsDelayTimerHandle;

	// 碰撞体积重叠事件回调，有 Pawn 进入时触发拾取逻辑
	UFUNCTION()
	UE_API void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	// 武器生成时检查是否已有 Pawn 站在生成器上
	UE_API void CheckForExistingOverlaps();

	// 尝试让指定 Pawn 拾取武器（可在蓝图中重写）
	UFUNCTION(BlueprintNativeEvent)
	UE_API void AttemptPickUpWeapon(APawn* Pawn);

	// 将武器给予指定 Pawn 的背包（蓝图实现事件）
	UFUNCTION(BlueprintNativeEvent, Category = "EqZero|WeaponPickup")
	UE_API bool GiveWeapon(TSubclassOf<UEqZeroInventoryItemDefinition> WeaponItemClass, APawn* ReceivingPawn);

	// 开始冷却计时
	UE_API void StartCoolDown();

	// 重置冷却，重新生成武器（可在蓝图中调用）
	UFUNCTION(BlueprintCallable, Category = "EqZero|WeaponPickup")
	UE_API void ResetCoolDown();

	// 冷却定时器到期时的回调
	UFUNCTION()
	UE_API void OnCoolDownTimerComplete();

	// 设置武器网格体的可见性
	UE_API void SetWeaponPickupVisibility(bool bShouldBeVisible);

	// 播放武器被拾取时的特效和音效（可在蓝图中重写）
	UFUNCTION(BlueprintNativeEvent, Category = "EqZero|WeaponPickup")
	UE_API void PlayPickupEffects();

	// 播放武器重新生成时的特效和音效（可在蓝图中重写）
	UFUNCTION(BlueprintNativeEvent, Category = "EqZero|WeaponPickup")
	UE_API void PlayRespawnEffects();

	// 武器可用状态同步回调（客户端收到 bIsWeaponAvailable 变化时触发）
	UFUNCTION()
	UE_API void OnRep_WeaponAvailability();

	/**
	 * 从道具定义中查找指定标签的默认属性值，找不到返回 0
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EqZero|WeaponPickup")
	static UE_API int32 GetDefaultStatFromItemDef(const TSubclassOf<UEqZeroInventoryItemDefinition> WeaponItemClass, FGameplayTag StatTag);
};

#undef UE_API
