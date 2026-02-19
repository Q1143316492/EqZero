// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Inventory/EqZeroInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "InventoryFragment_EquippableItem.generated.h"

class UEqZeroEquipmentDefinition;
class UObject;

UCLASS()
class UInventoryFragment_EquippableItem : public UEqZeroInventoryItemFragment
{
	GENERATED_BODY()

public:
	// 该道具对应的装备定义，穿戴时由 QuickBarComponent 读取
	UPROPERTY(EditAnywhere, Category=EqZero)
	TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition;
};
