// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/EqZeroAbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "EqZeroEquipmentManagerComponent.generated.h"

#define UE_API EQZEROGAME_API

class UActorComponent;
class UEqZeroAbilitySystemComponent;
class UEqZeroEquipmentDefinition;
class UEqZeroEquipmentInstance;
class UEqZeroEquipmentManagerComponent;
class UObject;
struct FFrame;
struct FEqZeroEquipmentList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/**
 * 已应用的装备条目（FastArraySerializer元素）
 * 每个条目对应一件已装备的装备，包含装备定义、实例引用和已授予的技能句柄
 */
USTRUCT(BlueprintType)
struct FEqZeroAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FEqZeroAppliedEquipmentEntry()
	{}

	FString GetDebugString() const;

private:
	friend FEqZeroEquipmentList;
	friend UEqZeroEquipmentManagerComponent;

	// 装备定义类（描述装备的配置数据）
	UPROPERTY()
	TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition;

	// 装备实例（运行时创建的对象）
	UPROPERTY()
	TObjectPtr<UEqZeroEquipmentInstance> Instance = nullptr;

	// 已授予的技能句柄，仅在Authority端有效，用于卸下时撤销技能
	UPROPERTY(NotReplicated)
	FEqZeroAbilitySet_GrantedHandles GrantedHandles;
};

/**
 * 装备列表（FastArraySerializer）
 * 管理所有已装备的条目，负责增删以及网络同步回调（增/删/改）。
 * 增加装备时会从装备定义中读取技能集并授予ASC，
 * 删除装备时会撤销已授予的技能并销毁相关Actor。
 */
USTRUCT(BlueprintType)
struct FEqZeroEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FEqZeroEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FEqZeroEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract —— 网络复制回调
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEqZeroAppliedEquipmentEntry, FEqZeroEquipmentList>(Entries, DeltaParms, *this);
	}

	/*
	 * 添加和删除条目接口，供 EquipmentManagerComponent 调用
	 */
	UEqZeroEquipmentInstance* AddEntry(TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(UEqZeroEquipmentInstance* Instance);

private:
	UEqZeroAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UEqZeroEquipmentManagerComponent;

private:
	// 已装备条目数组（网络复制）
	UPROPERTY()
	TArray<FEqZeroAppliedEquipmentEntry> Entries;

	// 所属组件引用（不复制）
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FEqZeroEquipmentList> : public TStructOpsTypeTraitsBase2<FEqZeroEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};


/**
 * UEqZeroEquipmentManagerComponent
 *
 * 装备管理组件，挂载在Pawn上，负责装备的穿戴/卸下。
 * 核心流程：
 *   EquipItem(定义类) -> 创建实例 -> 授予技能 -> 生成Actor -> 触发OnEquipped
 *   UnequipItem(实例) -> 触发OnUnequipped -> 撤销技能 -> 销毁Actor -> 移除条目
 *
 * 使用 FastArraySerializer 实现装备列表的增量网络复制，
 * 并通过 SubObject 机制复制装备实例对象本身。
 */
UCLASS(BlueprintType, Const)
class UEqZeroEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UE_API UEqZeroEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*
	 * 核心接口：穿戴和卸下装备
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UE_API UEqZeroEquipmentInstance* EquipItem(TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UE_API void UnequipItem(UEqZeroEquipmentInstance* ItemInstance);

	//~UObject interface
	UE_API virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	//~End of UObject interface

	//~UActorComponent interface
	UE_API virtual void InitializeComponent() override;
	UE_API virtual void UninitializeComponent() override;
	UE_API virtual void ReadyForReplication() override;
	//~End of UActorComponent interface

	/** 获取指定类型的第一个装备实例，没有则返回nullptr */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UE_API UEqZeroEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UEqZeroEquipmentInstance> InstanceType);

	/** 获取指定类型的所有装备实例 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UE_API TArray<UEqZeroEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UEqZeroEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

private:
	UPROPERTY(Replicated)
	FEqZeroEquipmentList EquipmentList;
};

#undef UE_API
