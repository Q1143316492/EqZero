// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"

#include "EqZeroEquipmentDefinition.generated.h"

class AActor;
class UEqZeroAbilitySet;
class UEqZeroEquipmentInstance;

/**
 * 装备Actor生成信息
 * 描述装备时需要生成的Actor以及挂载规则（插槽、变换）
 */
USTRUCT()
struct FEqZeroEquipmentActorToSpawn
{
	GENERATED_BODY()

	FEqZeroEquipmentActorToSpawn()
	{}

	// 要生成的Actor类
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	// 挂载到角色骨骼上的插槽名称
	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	// 挂载后的相对变换（位置/旋转/缩放）
	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};


/**
 * UEqZeroEquipmentDefinition
 *
 * 装备定义资产，描述一件可以应用到Pawn上的装备。
 * 包含：要生成的装备实例类型、授予的技能集、需要生成的挂载Actor。
 * 作为DataAsset在编辑器中配置，运行时由 EquipmentManagerComponent 读取。
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UEqZeroEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 装备实例的类，装备时会创建该类的对象
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<UEqZeroEquipmentInstance> InstanceType;

	// 装备时授予的技能集列表（AbilitySet）
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UEqZeroAbilitySet>> AbilitySetsToGrant;

	// 装备时在Pawn身上生成的Actor列表（如武器模型等）
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FEqZeroEquipmentActorToSpawn> ActorsToSpawn;
};
