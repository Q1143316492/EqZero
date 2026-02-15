// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Inventory/EqZeroInventoryItemDefinition.h"

#include "InventoryFragment_SetStats.generated.h"

class UEqZeroInventoryItemInstance;
class UObject;
struct FGameplayTag;

UCLASS()
class UInventoryFragment_SetStats : public UEqZeroInventoryItemFragment
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;

public:
	virtual void OnInstanceCreated(UEqZeroInventoryItemInstance* Instance) const override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
};
