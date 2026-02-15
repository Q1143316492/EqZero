// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"

#include "UObject/ObjectPtr.h"
#include "IPickupable.generated.h"

template <typename InterfaceType> class TScriptInterface;

class AActor;
class UEqZeroInventoryItemDefinition;
class UEqZeroInventoryItemInstance;
class UEqZeroInventoryManagerComponent;
class UObject;
struct FFrame;

USTRUCT(BlueprintType)
struct FPickupTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 StackCount = 1;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UEqZeroInventoryItemDefinition> ItemDef;
};

USTRUCT(BlueprintType)
struct FPickupInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UEqZeroInventoryItemInstance> Item = nullptr;
};

/*
 * 可拾取的Actor身上挂一个，通过配置实例或者定义的方式提供转化后的物品信息
 */
USTRUCT(BlueprintType)
struct FInventoryPickup
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPickupInstance> Instances;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPickupTemplate> Templates;
};

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UPickupable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 实现可拾取接口的可以被拾取
 *   通过接口获取Actor拾取后物品的信息，可以配置实例或者是定义的方式转换为物品。
 *   但是实例目前似乎没用，从配置来靠谱一点
 */
class IPickupable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual FInventoryPickup GetPickupInventory() const = 0;
};

/**
 *
 *  
 */
UCLASS()
class UEqZeroPickupableStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UEqZeroPickupableStatics();

public:
	UFUNCTION(BlueprintPure)
	static TScriptInterface<IPickupable> GetFirstPickupableFromActor(AActor* Actor);

	// 把一个Pickupable的物品添加到库存组件中，可能是定义，也可能是实例
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (WorldContext = "Ability"))
	static void AddPickupToInventory(UEqZeroInventoryManagerComponent* InventoryComponent, TScriptInterface<IPickupable> Pickup);
};
