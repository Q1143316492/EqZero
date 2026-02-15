// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/EqZeroInventoryManagerComponent.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/EqZeroInventoryItemDefinition.h"
#include "Inventory/EqZeroInventoryItemInstance.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroInventoryManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EqZero_Inventory_Message_StackChanged, "EqZero.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FEqZeroInventoryEntry

FString FEqZeroInventoryEntry::GetDebugString() const
{
	TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FEqZeroInventoryList

void FEqZeroInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FEqZeroInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, Stack.StackCount, 0);
		Stack.LastObservedCount = 0;
	}
}

void FEqZeroInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FEqZeroInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, 0, Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FEqZeroInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FEqZeroInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, Stack.LastObservedCount,Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FEqZeroInventoryList::BroadcastChangeMessage(FEqZeroInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	FEqZeroInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_EqZero_Inventory_Message_StackChanged, Message);
}

UEqZeroInventoryItemInstance* FEqZeroInventoryList::AddEntry(const TSubclassOf<UEqZeroInventoryItemDefinition> &ItemDef, int32 StackCount)
{
	UEqZeroInventoryItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
 	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());


	FEqZeroInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	/*
	 * Using the actor instead of component as the outer due to UE-127172
	 * 这个物品Instance是属于Component的，所以创建的时候，应该把 OwnerComponent作为Outer，
	 *
	 * UE-127172:
	 * 这个 Bug 指出，当一个支持网络复制的子对象（Subobject，即这里的 ItemInstance）
	 * 它的 Outer 是一个 组件 (ActorComponent) 而不是 Actor 时，网路复制（Replication）可能会出现问题，或者导致数据无法正确同步给客户端。
	 * 
	 */
	NewEntry.Instance = NewObject<UEqZeroInventoryItemInstance>(OwnerComponent->GetOwner());
	NewEntry.Instance->SetItemDef(ItemDef);
	
	for (UEqZeroInventoryItemFragment* Fragment : GetDefault<UEqZeroInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	//const UEqZeroInventoryItemDefinition* ItemCDO = GetDefault<UEqZeroInventoryItemDefinition>(ItemDef);
	MarkItemDirty(NewEntry);

	return Result;
}

void FEqZeroInventoryList::AddEntry(UEqZeroInventoryItemInstance* Instance)
{
	unimplemented();
}

void FEqZeroInventoryList::RemoveEntry(UEqZeroInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FEqZeroInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

TArray<UEqZeroInventoryItemInstance*> FEqZeroInventoryList::GetAllItems() const
{
	TArray<UEqZeroInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FEqZeroInventoryEntry& Entry : Entries)
	{
		// 更希望不在这里处理这件事，而是把它进一步隐藏起来 (官方的TODO)
		if (Entry.Instance != nullptr)
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

//////////////////////////////////////////////////////////////////////
// UEqZeroInventoryManagerComponent

UEqZeroInventoryManagerComponent::UEqZeroInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void UEqZeroInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool UEqZeroInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 StackCount)
{
	// 这里应该添加一些检查，比如这个物品是否有堆叠限制，或者是否唯一等等
	return true;
}

UEqZeroInventoryItemInstance* UEqZeroInventoryManagerComponent::AddItemDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 StackCount)
{
	UEqZeroInventoryItemInstance* Result = nullptr;
	if (ItemDef != nullptr)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);
		
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}
	return Result;
}

void UEqZeroInventoryManagerComponent::AddItemInstance(UEqZeroInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void UEqZeroInventoryManagerComponent::RemoveItemInstance(UEqZeroInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

TArray<UEqZeroInventoryItemInstance*> UEqZeroInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

UEqZeroInventoryItemInstance* UEqZeroInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef) const
{
	for (const FEqZeroInventoryEntry& Entry : InventoryList.Entries)
	{
		UEqZeroInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 UEqZeroInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FEqZeroInventoryEntry& Entry : InventoryList.Entries)
	{
		UEqZeroInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool UEqZeroInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	// 官方说这里有个 O(n2) 的复杂度，可以优化
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (UEqZeroInventoryItemInstance* Instance = UEqZeroInventoryManagerComponent::FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

void UEqZeroInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing UEqZeroInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FEqZeroInventoryEntry& Entry : InventoryList.Entries)
		{
			UEqZeroInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool UEqZeroInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FEqZeroInventoryEntry& Entry : InventoryList.Entries)
	{
		UEqZeroInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}
