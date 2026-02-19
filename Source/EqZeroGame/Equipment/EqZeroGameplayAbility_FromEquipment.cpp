// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_FromEquipment.h"
#include "EqZeroEquipmentInstance.h"
#include "Inventory/EqZeroInventoryItemInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_FromEquipment)

UEqZeroGameplayAbility_FromEquipment::UEqZeroGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UEqZeroEquipmentInstance* UEqZeroGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	// AbilitySet 授予技能时会将装备实例设为 SourceObject
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UEqZeroEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UEqZeroInventoryItemInstance* UEqZeroGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	// 装备实例的 Instigator 通常就是背包道具实例
	if (UEqZeroEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UEqZeroInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UEqZeroGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 装备技能必须使用实例化策略，否则无法通过 SourceObject 获取装备引用
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	{
		Context.AddError(NSLOCTEXT("EqZero", "EquipmentAbilityMustBeInstanced", "装备技能必须使用实例化策略(Instanced)"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
