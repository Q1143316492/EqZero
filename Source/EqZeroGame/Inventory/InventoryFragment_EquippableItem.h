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
	// TODO 装备系统还没有
	// UPROPERTY(EditAnywhere, Category=EqZero)
	// TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition;
};
