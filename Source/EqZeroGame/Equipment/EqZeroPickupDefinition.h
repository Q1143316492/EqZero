// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "EqZeroPickupDefinition.generated.h"

class UEqZeroInventoryItemDefinition;
class UNiagaraSystem;
class UObject;
class USoundBase;
class UStaticMesh;

/**
 * UEqZeroPickupDefinition
 *
 * 拾取物定义数据资产，配置地图上可拾取物体的外观、音效、特效等。
 * 在编辑器中创建该DataAsset并设置需要的参数，
 * 然后被拾取物Actor引用来初始化的外观和拾取后的行为。
 */
UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "EqZero Pickup Data", ShortTooltip = "用于配置拾取物的数据资产"))
class UEqZeroPickupDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	// 拾取后添加到背包的道具定义
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup|Equipment")
	TSubclassOf<UEqZeroInventoryItemDefinition> InventoryItemDefinition;

	// 拾取物的静态网格体外观
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;

	// 拾取后的冷却时间（秒），冷却结束后重新生成
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup")
	int32 SpawnCoolDownSeconds;

	// 被拾取时播放的音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup")
	TObjectPtr<USoundBase> PickedUpSound;

	// 重新生成时播放的音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup")
	TObjectPtr<USoundBase> RespawnedSound;

	// 被拾取时播放的粒子特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup")
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	// 重新生成时播放的粒子特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup")
	TObjectPtr<UNiagaraSystem> RespawnedEffect;
};


/**
 * UEqZeroWeaponPickupDefinition
 *
 * 武器拾取物定义，继承自通用拾取物定义，
 * 额外添加了武器模型的偏移和缩放控制。
 */
UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "EqZero Weapon Pickup Data", ShortTooltip = "用于配置武器拾取物的数据资产"))
class UEqZeroWeaponPickupDefinition : public UEqZeroPickupDefinition
{
	GENERATED_BODY()

public:

	// 武器模型相对于生成器的位置偏移
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup|Mesh")
	FVector WeaponMeshOffset;

	// 武器模型的缩放
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);
};
