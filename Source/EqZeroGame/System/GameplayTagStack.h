// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "GameplayTagStack.generated.h"

struct FGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

USTRUCT(BlueprintType)
struct FGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FGameplayTagStack()
	{}

	FGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		, StackCount(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FGameplayTagStackContainer;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

USTRUCT(BlueprintType)
struct FGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FGameplayTagStackContainer()
	//	: Owner(nullptr)
	{
	}

public:
	void AddStack(FGameplayTag Tag, int32 StackCount);
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	// 客户端调用会更新客户端的这个 map
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagStack, FGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	// 感觉一个是拿来查的，一个是同步用的，两个是一样的东西

	UPROPERTY()
	TArray<FGameplayTagStack> Stacks;

	TMap<FGameplayTag, int32> TagToCountMap;
};

template<>
struct TStructOpsTypeTraits<FGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
