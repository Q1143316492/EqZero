// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "System/GameplayTagStack.h"
#include "Templates/SubclassOf.h"

#include "EqZeroInventoryItemInstance.generated.h"

class FLifetimeProperty;

class UEqZeroInventoryItemDefinition;
class UEqZeroInventoryItemFragment;
struct FFrame;
struct FGameplayTag;

/**
 * UEqZeroInventoryItemInstance
 */
UCLASS(BlueprintType)
class UEqZeroInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~End of UObject interface

	/*
	 * Tag => num 的维护
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	TSubclassOf<UEqZeroInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UEqZeroInventoryItemFragment* FindFragmentByClass(TSubclassOf<UEqZeroInventoryItemFragment> FragmentClass) const;

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return Cast<ResultClass>(FindFragmentByClass(ResultClass::StaticClass()));
	}

private:
#if UE_WITH_IRIS
	/** Register all replication fragments */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	void SetItemDef(TSubclassOf<UEqZeroInventoryItemDefinition> InDef);

	friend struct FEqZeroInventoryList;

private:
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef;
};
