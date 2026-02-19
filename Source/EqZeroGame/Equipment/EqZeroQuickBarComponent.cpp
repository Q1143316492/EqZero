// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroQuickBarComponent.h"

#include "Equipment/EqZeroEquipmentDefinition.h"
#include "Equipment/EqZeroEquipmentInstance.h"
#include "Equipment/EqZeroEquipmentManagerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroQuickBarComponent)

class FLifetimeProperty;
class UEqZeroEquipmentDefinition;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EqZero_QuickBar_Message_SlotsChanged, "EqZero.QuickBar.Message.SlotsChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EqZero_QuickBar_Message_ActiveIndexChanged, "EqZero.QuickBar.Message.ActiveIndexChanged");

UEqZeroQuickBarComponent::UEqZeroQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UEqZeroQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UEqZeroQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
	}

	Super::BeginPlay();
}

void UEqZeroQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	// 从当前位置向前查找下一个有道具的槽位
	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num()-1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

void UEqZeroQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	// 从当前位置向后查找上一个有道具的槽位
	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num()-1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

void UEqZeroQuickBarComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));
	check(EquippedItem == nullptr);

	if (UEqZeroInventoryItemInstance* SlotItem = Slots[ActiveSlotIndex])
	{
		// 从道具的可装备片段中获取装备定义
		if (const UInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UEqZeroEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				if (UEqZeroEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
				{
					EquippedItem = EquipmentManager->EquipItem(EquipDef);
					if (EquippedItem != nullptr)
					{
						// 将道具实例设为装备的 Instigator，方便技能回溯到源道具
						EquippedItem->SetInstigator(SlotItem);
					}
				}
			}
		}
	}
}

void UEqZeroQuickBarComponent::UnEquipItemInSlot()
{
	if (UEqZeroEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		if (EquippedItem != nullptr)
		{
			EquipmentManager->UnequipItem(EquippedItem);
			EquippedItem = nullptr;
		}
	}
}

UEqZeroEquipmentManagerComponent* UEqZeroQuickBarComponent::FindEquipmentManager() const
{
	// 从 Controller -> Pawn -> 查找 EquipmentManagerComponent
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UEqZeroEquipmentManagerComponent>();
		}
	}
	return nullptr;
}

void UEqZeroQuickBarComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		// 卸下当前 -> 切换索引 -> 穿上新装备
		UnEquipItemInSlot();

		ActiveSlotIndex = NewIndex;

		EquipItemInSlot();

		OnRep_ActiveSlotIndex();
	}
}

UEqZeroInventoryItemInstance* UEqZeroQuickBarComponent::GetActiveSlotItem() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

int32 UEqZeroQuickBarComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (const TObjectPtr<UEqZeroInventoryItemInstance>& ItemPtr : Slots)
	{
		if (ItemPtr == nullptr)
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}

void UEqZeroQuickBarComponent::AddItemToSlot(int32 SlotIndex, UEqZeroInventoryItemInstance* Item)
{
	if (Slots.IsValidIndex(SlotIndex) && (Item != nullptr))
	{
		if (Slots[SlotIndex] == nullptr)
		{
			Slots[SlotIndex] = Item;
			OnRep_Slots();
		}
	}
}

UEqZeroInventoryItemInstance* UEqZeroQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	UEqZeroInventoryItemInstance* Result = nullptr;

	// 如果移除的是当前激活槽位，先卸下装备
	if (ActiveSlotIndex == SlotIndex)
	{
		UnEquipItemInSlot();
		ActiveSlotIndex = -1;
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];

		if (Result != nullptr)
		{
			Slots[SlotIndex] = nullptr;
			OnRep_Slots();
		}
	}

	return Result;
}

void UEqZeroQuickBarComponent::OnRep_Slots()
{
	// 通过 GameplayMessage 广播槽位变更，UI层监听此消息来更新显示
	FEqZeroQuickBarSlotsChangedMessage Message;
	Message.Owner = GetOwner();
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_EqZero_QuickBar_Message_SlotsChanged, Message);
}

void UEqZeroQuickBarComponent::OnRep_ActiveSlotIndex()
{
	// 通过 GameplayMessage 广播激活索引变更
	FEqZeroQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ActiveSlotIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_EqZero_QuickBar_Message_ActiveIndexChanged, Message);
}
