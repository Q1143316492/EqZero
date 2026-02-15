// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/EqZeroInventoryItemDefinition.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroInventoryItemDefinition)

//////////////////////////////////////////////////////////////////////
// UEqZeroInventoryItemDefinition

UEqZeroInventoryItemDefinition::UEqZeroInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UEqZeroInventoryItemFragment* UEqZeroInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UEqZeroInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// UEqZeroInventoryItemDefinition

const UEqZeroInventoryItemFragment* UEqZeroInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UEqZeroInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}
