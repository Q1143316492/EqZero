// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroEquipmentManagerComponent.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/ActorChannel.h"
#include "EqZeroEquipmentDefinition.h"
#include "EqZeroEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroEquipmentManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

//////////////////////////////////////////////////////////////////////
// FEqZeroAppliedEquipmentEntry

FString FEqZeroAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(EquipmentDefinition.Get()));
}

//////////////////////////////////////////////////////////////////////
// FEqZeroEquipmentList

void FEqZeroEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// 客户端收到条目移除通知时，触发装备的 OnUnequipped 回调
	for (int32 Index : RemovedIndices)
	{
		const FEqZeroAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FEqZeroEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// 客户端收到条目新增通知时，触发装备的 OnEquipped 回调
	for (int32 Index : AddedIndices)
	{
		const FEqZeroAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FEqZeroEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// 条目变更时的回调，目前无额外处理
}

UEqZeroAbilitySystemComponent* FEqZeroEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UEqZeroAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UEqZeroEquipmentInstance* FEqZeroEquipmentList::AddEntry(TSubclassOf<UEqZeroEquipmentDefinition> EquipmentDefinition)
{
	UEqZeroEquipmentInstance* Result = nullptr;

	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	// 从装备定义的CDO获取配置信息
	const UEqZeroEquipmentDefinition* EquipmentCDO = GetDefault<UEqZeroEquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<UEqZeroEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UEqZeroEquipmentInstance::StaticClass();
	}

	// 创建新条目并生成装备实例对象
	FEqZeroAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	// 注意：Outer 使用 Actor 而不是 Component，这是因为 UE 的子对象复制机制要求（UE-127172）
	NewEntry.Instance = NewObject<UEqZeroEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	Result = NewEntry.Instance;

	// 授予装备定义中配置的技能集
	if (UEqZeroAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TObjectPtr<const UEqZeroAbilitySet>& AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, Result);
		}
	}
	else
	{
		// TODO: 如果获取不到ASC，考虑添加警告日志
	}

	// 生成装备相关的Actor（武器模型等）
	Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);

	MarkItemDirty(NewEntry);

	return Result;
}

void FEqZeroEquipmentList::RemoveEntry(UEqZeroEquipmentInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FEqZeroAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			// 撤销已授予的技能
			if (UEqZeroAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}

			// 销毁装备相关的Actor
			Instance->DestroyEquipmentActors();

			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// UEqZeroEquipmentManagerComponent

UEqZeroEquipmentManagerComponent::UEqZeroEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UEqZeroEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
}

UEqZeroEquipmentInstance* UEqZeroEquipmentManagerComponent::EquipItem(TSubclassOf<UEqZeroEquipmentDefinition> EquipmentClass)
{
	UEqZeroEquipmentInstance* Result = nullptr;
	if (EquipmentClass != nullptr)
	{
		Result = EquipmentList.AddEntry(EquipmentClass);
		if (Result != nullptr)
		{
			Result->OnEquipped();

			// 注册为复制子对象，使装备实例可以通过网络同步
			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void UEqZeroEquipmentManagerComponent::UnequipItem(UEqZeroEquipmentInstance* ItemInstance)
{
	if (ItemInstance != nullptr)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}

		ItemInstance->OnUnequipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

bool UEqZeroEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// 手动复制所有装备实例子对象
	for (FEqZeroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		UEqZeroEquipmentInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UEqZeroEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UEqZeroEquipmentManagerComponent::UninitializeComponent()
{
	TArray<UEqZeroEquipmentInstance*> AllEquipmentInstances;

	for (const FEqZeroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}

	for (UEqZeroEquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		UnequipItem(EquipInstance);
	}

	Super::UninitializeComponent();
}

void UEqZeroEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// 网络复制就绪后，将所有已存在的装备实例注册为复制子对象
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FEqZeroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			UEqZeroEquipmentInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

UEqZeroEquipmentInstance* UEqZeroEquipmentManagerComponent::GetFirstInstanceOfType(TSubclassOf<UEqZeroEquipmentInstance> InstanceType)
{
	for (FEqZeroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UEqZeroEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

TArray<UEqZeroEquipmentInstance*> UEqZeroEquipmentManagerComponent::GetEquipmentInstancesOfType(TSubclassOf<UEqZeroEquipmentInstance> InstanceType) const
{
	TArray<UEqZeroEquipmentInstance*> Results;
	for (const FEqZeroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UEqZeroEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Results.Add(Instance);
			}
		}
	}
	return Results;
}
