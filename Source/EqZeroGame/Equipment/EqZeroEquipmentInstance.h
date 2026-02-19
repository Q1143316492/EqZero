// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"

#include "EqZeroEquipmentInstance.generated.h"

class AActor;
class APawn;
struct FFrame;
struct FEqZeroEquipmentActorToSpawn;

/**
 * UEqZeroEquipmentInstance
 *
 * 装备实例，代表已经生成并应用到Pawn上的一件装备。
 * 该对象的Outer是拥有它的Pawn（用于网络复制），
 * 通过 Instigator 记录触发装备的源头（通常是背包道具实例）。
 *
 * 生命周期：
 *   EquipmentManagerComponent::EquipItem() 创建
 *     -> OnEquipped() / K2_OnEquipped()
 *     -> ... 使用中 ...
 *     -> OnUnequipped() / K2_OnUnequipped()
 *   EquipmentManagerComponent::UnequipItem() 销毁
 */
UCLASS(BlueprintType, Blueprintable)
class UEqZeroEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UEqZeroEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	//~End of UObject interface

	/*
	 * Getter
	 */
	UFUNCTION(BlueprintPure, Category=Equipment)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	/*
	 * 挂载和销毁装备相关的Actor
	 */
	virtual void SpawnEquipmentActors(const TArray<FEqZeroEquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();

	/*
	 * 
	 */
	virtual void OnEquipped();
	virtual void OnUnequipped();

protected:
#if UE_WITH_IRIS
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

private:
	UFUNCTION()
	void OnRep_Instigator();

private:
	// 触发装备的源头（如背包物品实例），网络复制
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	// 装备生成的Actor列表，网络复制
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;
};
