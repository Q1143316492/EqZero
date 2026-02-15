// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/IPickupable.h"

#include "GameFramework/Actor.h"
#include "Inventory/EqZeroInventoryManagerComponent.h"
#include "UObject/ScriptInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IPickupable)

class UActorComponent;

UEqZeroPickupableStatics::UEqZeroPickupableStatics()
	: Super(FObjectInitializer::Get())
{
}

TScriptInterface<IPickupable> UEqZeroPickupableStatics::GetFirstPickupableFromActor(AActor* Actor)
{
	// 可能Actor自己就是可拾取的，直接返回
	TScriptInterface<IPickupable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	// 如果Actor本身不可拾取，可能会有组件实现了可拾取接口
	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		return TScriptInterface<IPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IPickupable>();
}

void UEqZeroPickupableStatics::AddPickupToInventory(UEqZeroInventoryManagerComponent* InventoryComponent, TScriptInterface<IPickupable> Pickup)
{
	if (InventoryComponent && Pickup)
	{
		const FInventoryPickup& PickupInventory = Pickup->GetPickupInventory();

		for (const FPickupTemplate& Template : PickupInventory.Templates)
		{
			InventoryComponent->AddItemDefinition(Template.ItemDef, Template.StackCount);
		}

		for (const FPickupInstance& Instance : PickupInventory.Instances)
		{
			InventoryComponent->AddItemInstance(Instance.Item);
		}
	}
}
