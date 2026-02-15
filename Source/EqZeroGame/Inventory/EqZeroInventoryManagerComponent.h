// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "EqZeroInventoryManagerComponent.generated.h"

#define UE_API EQZEROGAME_API

class UEqZeroInventoryItemDefinition;
class UEqZeroInventoryItemInstance;
class UEqZeroInventoryManagerComponent;
class UObject;
struct FFrame;
struct FEqZeroInventoryList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/** 物品添加到库存时的消息 */
USTRUCT(BlueprintType)
struct FEqZeroInventoryChangeMessage
{
	GENERATED_BODY()

	// 官方说，最好是基于 names+owning actors 来表示库存，而不是直接暴露组件
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UEqZeroInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

USTRUCT(BlueprintType)
struct FEqZeroInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FEqZeroInventoryEntry() {}

	FString GetDebugString() const;

private:
	friend FEqZeroInventoryList;
	friend UEqZeroInventoryManagerComponent;

	UPROPERTY()
	TObjectPtr<UEqZeroInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FEqZeroInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FEqZeroInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FEqZeroInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UEqZeroInventoryItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEqZeroInventoryEntry, FEqZeroInventoryList>(Entries, DeltaParms, *this);
	}

	UEqZeroInventoryItemInstance* AddEntry(const TSubclassOf<UEqZeroInventoryItemDefinition> &ItemClass, int32 StackCount);
	void AddEntry(UEqZeroInventoryItemInstance* Instance);
	void RemoveEntry(UEqZeroInventoryItemInstance* Instance);

private:
	void BroadcastChangeMessage(FEqZeroInventoryEntry& Entry, int32 OldCount, int32 NewCount);

private:
	friend UEqZeroInventoryManagerComponent;

private:
	UPROPERTY()
	TArray<FEqZeroInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FEqZeroInventoryList> : public TStructOpsTypeTraitsBase2<FEqZeroInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};


/**
 * 库存组件
 */
UCLASS(MinimalAPI, BlueprintType)
class UEqZeroInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UE_API UEqZeroInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API bool CanAddItemDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API UEqZeroInventoryItemInstance* AddItemDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API void AddItemInstance(UEqZeroInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UE_API void RemoveItemInstance(UEqZeroInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	UE_API TArray<UEqZeroInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UE_API UEqZeroInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef) const;

	UE_API int32 GetTotalItemCountByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef) const;
	UE_API bool ConsumeItemsByDefinition(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, int32 NumToConsume);

	//~UObject interface
	UE_API virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	UE_API virtual void ReadyForReplication() override;
	//~End of UObject interface

private:
	UPROPERTY(Replicated)
	FEqZeroInventoryList InventoryList;
};

#undef UE_API
