// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "EqZeroInventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class UEqZeroInventoryItemInstance;
struct FFrame;

//////////////////////////////////////////////////////////////////////

// Fragment
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UEqZeroInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UEqZeroInventoryItemInstance* Instance) const {}
};

//////////////////////////////////////////////////////////////////////

/**
 * UEqZeroInventoryItemDefinition
 */
UCLASS(Blueprintable, Const, Abstract)
class UEqZeroInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UEqZeroInventoryItemFragment>> Fragments;

public:
	const UEqZeroInventoryItemFragment* FindFragmentByClass(TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass) const;
};

UCLASS()
class UEqZeroInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UEqZeroInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef, TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass);
};
