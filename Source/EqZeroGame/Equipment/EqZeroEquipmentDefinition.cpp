// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroEquipmentDefinition.h"
#include "EqZeroEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroEquipmentDefinition)

UEqZeroEquipmentDefinition::UEqZeroEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 默认使用基础装备实例类型
	InstanceType = UEqZeroEquipmentInstance::StaticClass();
}
