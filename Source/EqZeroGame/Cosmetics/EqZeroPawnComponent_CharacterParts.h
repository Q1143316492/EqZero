// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/PawnComponent.h"
#include "Cosmetics/EqZeroCosmeticAnimationTypes.h"
#include "EqZeroCharacterPartTypes.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "EqZeroPawnComponent_CharacterParts.generated.h"

class UEqZeroPawnComponent_CharacterParts;
namespace EEndPlayReason { enum Type : int; }
struct FGameplayTag;
struct FEqZeroCharacterPartList;

class AActor;
class UChildActorComponent;
class UObject;
class USceneComponent;
class USkeletalMeshComponent;
struct FFrame;
struct FNetDeltaSerializeInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEqZeroSpawnedCharacterPartsChanged, UEqZeroPawnComponent_CharacterParts*, ComponentWithChangedParts);

//////////////////////////////////////////////////////////////////////

// A single applied character part
USTRUCT()
struct FEqZeroAppliedCharacterPartEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FEqZeroAppliedCharacterPartEntry()
	{}

	FString GetDebugString() const;

private:
	friend FEqZeroCharacterPartList;
	friend UEqZeroPawnComponent_CharacterParts;

private:
	// The character part being represented
	UPROPERTY()
	FEqZeroCharacterPart Part;

	// Handle index we returned to the user (server only)
	UPROPERTY(NotReplicated)
	int32 PartHandle = INDEX_NONE;

	// The spawned actor instance (client only)
	UPROPERTY(NotReplicated)
	TObjectPtr<UChildActorComponent> SpawnedComponent = nullptr;
};

//////////////////////////////////////////////////////////////////////

// Replicated list of applied character parts
USTRUCT(BlueprintType)
struct FEqZeroCharacterPartList : public FFastArraySerializer
{
	GENERATED_BODY()

	FEqZeroCharacterPartList()
		: OwnerComponent(nullptr)
	{
	}

public:
	// 这三在客户端执行
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEqZeroAppliedCharacterPartEntry, FEqZeroCharacterPartList>(Entries, DeltaParms, *this);
	}

	FEqZeroCharacterPartHandle AddEntry(FEqZeroCharacterPart NewPart);
	void RemoveEntry(FEqZeroCharacterPartHandle Handle);
	void ClearAllEntries(bool bBroadcastChangeDelegate);

	FGameplayTagContainer CollectCombinedTags() const;

	void SetOwnerComponent(UEqZeroPawnComponent_CharacterParts* InOwnerComponent)
	{
		OwnerComponent = InOwnerComponent;
	}

private:
	friend UEqZeroPawnComponent_CharacterParts;

	bool SpawnActorForEntry(FEqZeroAppliedCharacterPartEntry& Entry);
	bool DestroyActorForEntry(FEqZeroAppliedCharacterPartEntry& Entry);

private:
	UPROPERTY()
	TArray<FEqZeroAppliedCharacterPartEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UEqZeroPawnComponent_CharacterParts> OwnerComponent;

	int32 PartHandleCounter = 0;
};

template<>
struct TStructOpsTypeTraits<FEqZeroCharacterPartList> : public TStructOpsTypeTraitsBase2<FEqZeroCharacterPartList>
{
	enum { WithNetDeltaSerializer = true };
};

//////////////////////////////////////////////////////////////////////

// A component that handles spawning cosmetic actors attached to the owner pawn on all clients
UCLASS(meta=(BlueprintSpawnableComponent))
class UEqZeroPawnComponent_CharacterParts : public UPawnComponent
{
	GENERATED_BODY()

public:
	UEqZeroPawnComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
	//~End of UActorComponent interface

	/*
	 * 添加删除装扮
	 */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	FEqZeroCharacterPartHandle AddCharacterPart(const FEqZeroCharacterPart& NewPart);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveCharacterPart(FEqZeroCharacterPartHandle Handle);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveAllCharacterParts();

	/*
	 * Getters
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, BlueprintCosmetic, Category=Cosmetics)
	TArray<AActor*> GetCharacterPartActors() const;
	
	USkeletalMeshComponent* GetParentMeshComponent() const;
	
	USceneComponent* GetSceneComponentToAttachTo() const;

	/*
	 * Tags
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, BlueprintCosmetic, Category=Cosmetics)
	FGameplayTagContainer GetCombinedTags(FGameplayTag RequiredPrefix) const;

	/*
	 * 事件
	 */
	void BroadcastChanged();

	UPROPERTY(BlueprintAssignable, Category=Cosmetics, BlueprintCallable)
	FEqZeroSpawnedCharacterPartsChanged OnCharacterPartsChanged;

private:
	UPROPERTY(Replicated, Transient)
	FEqZeroCharacterPartList CharacterPartList;

	/**
	 * 这是一个要蓝图中配置的东西
	 */
	UPROPERTY(EditAnywhere, Category=Cosmetics)
	FEqZeroAnimBodyStyleSelectionSet BodyMeshes;
};
