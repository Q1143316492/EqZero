// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "Inventory/EqZeroInventoryItemInstance.h"

#include "EqZeroQuickBarComponent.generated.h"

class AActor;
class UEqZeroEquipmentInstance;
class UEqZeroEquipmentManagerComponent;
class UObject;
struct FFrame;

/**
 * UEqZeroQuickBarComponent
 *
 * 快捷栏组件，挂载在 Controller 上，管理玩家的快捷装备槽位。
 * 每个槽位持有一个背包道具实例的引用。切换激活槽位时，
 * 自动卸下当前装备并穿上新选中的装备。
 *
 * 网络同步：
 *   - Slots 数组和 ActiveSlotIndex 通过 DOREPLIFETIME 复制
 *   - 切换槽位通过 Server RPC 保证 Authority 端执行
 *   - OnRep 回调中通过 GameplayMessage 广播变更通知UI更新
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class UEqZeroQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UEqZeroQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="EqZero")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category="EqZero")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="EqZero")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UEqZeroInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UEqZeroInventoryItemInstance* GetActiveSlotItem() const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, UEqZeroInventoryItemInstance* Item);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UEqZeroInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

	virtual void BeginPlay() override;

private:
	void UnEquipItemInSlot();
	void EquipItemInSlot();

	UEqZeroEquipmentManagerComponent* FindEquipmentManager() const;

protected:
	UPROPERTY()
	int32 NumSlots = 3;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<TObjectPtr<UEqZeroInventoryItemInstance>> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	TObjectPtr<UEqZeroEquipmentInstance> EquippedItem;
};


/**
 * 快捷栏槽位变更消息，通过 GameplayMessage 广播给UI层
 */
USTRUCT(BlueprintType)
struct FEqZeroQuickBarSlotsChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<UEqZeroInventoryItemInstance>> Slots;
};


/**
 * 快捷栏激活索引变更消息，通过 GameplayMessage 广播给UI层
 */
USTRUCT(BlueprintType)
struct FEqZeroQuickBarActiveIndexChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 ActiveIndex = 0;
};
