// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroWorldCollectable.h"

#include "Async/TaskGraphInterfaces.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWorldCollectable)

struct FInteractionQuery;

AEqZeroWorldCollectable::AEqZeroWorldCollectable()
{
}

void AEqZeroWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

FInventoryPickup AEqZeroWorldCollectable::GetPickupInventory() const
{
	return StaticInventory;
}
