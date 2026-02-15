// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "InteractionQuery.generated.h"


/**
 * 
 */
USTRUCT(BlueprintType)
struct FInteractionQuery
{
	GENERATED_BODY()

public:
	/** 请求的 pawn. */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> RequestingAvatar;

	/** 允许我们指定一个控制器 —— 它不需要与发出请求的虚拟形象的所有者相匹配. */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AController> RequestingController;

	/** 一个通用的 UObject，用于存储交互所需的额外数据 */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<UObject> OptionalObjectData;
};
