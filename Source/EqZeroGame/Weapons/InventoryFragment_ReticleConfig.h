// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Inventory/EqZeroInventoryItemDefinition.h"

#include "InventoryFragment_ReticleConfig.generated.h"

class UEqZeroReticleWidgetBase;
class UObject;

UCLASS()
class UInventoryFragment_ReticleConfig : public UEqZeroInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Reticle)
	TArray<TSubclassOf<UEqZeroReticleWidgetBase>> ReticleWidgets;
};
