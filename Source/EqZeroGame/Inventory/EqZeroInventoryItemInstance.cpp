// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/EqZeroInventoryItemInstance.h"

#include "Inventory/EqZeroInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroInventoryItemInstance)

class FLifetimeProperty;

UEqZeroInventoryItemInstance::UEqZeroInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

#if UE_WITH_IRIS
void UEqZeroInventoryItemInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// Build descriptors and allocate PropertyReplicationFragments for this object
	// 为该对象构建描述符并分配属性复制片段
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS

void UEqZeroInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UEqZeroInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 UEqZeroInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UEqZeroInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void UEqZeroInventoryItemInstance::SetItemDef(TSubclassOf<UEqZeroInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}

const UEqZeroInventoryItemFragment* UEqZeroInventoryItemInstance::FindFragmentByClass(TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UEqZeroInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}
