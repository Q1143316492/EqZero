// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/EqZeroGameplayAbility.h"

#include "EqZeroGameplayAbility_FromEquipment.generated.h"

class UEqZeroEquipmentInstance;
class UEqZeroInventoryItemInstance;

/**
 * UEqZeroGameplayAbility_FromEquipment
 *
 * 由装备授予并关联的技能基类。
 * 当装备通过 AbilitySet 授予技能时，SourceObject 会被设为装备实例，
 * 因此可以通过 GetAssociatedEquipment() 反查到是哪件装备触发了该技能，
 * 进而通过 GetAssociatedItem() 追溯到背包中的道具实例。
 *
 * 注意：此技能必须使用实例化策略（Instanced），不能使用 NonInstanced，
 *       编辑器中的数据校验会强制检查。
 */
UCLASS()
class UEqZeroGameplayAbility_FromEquipment : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 获取关联的装备实例（通过 AbilitySpec 的 SourceObject）
	UFUNCTION(BlueprintCallable, Category="EqZero|Ability")
	UEqZeroEquipmentInstance* GetAssociatedEquipment() const;

	// 获取关联的背包道具实例（通过装备实例的 Instigator）
	UFUNCTION(BlueprintCallable, Category="EqZero|Ability")
	UEqZeroInventoryItemInstance* GetAssociatedItem() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

};
