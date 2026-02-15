// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetActor_Trace.h"

#include "GameplayAbilityTargetActor_Interact.generated.h"

class AActor;
class UObject;


/**
 * 所有交互目标角色的中间基类？
 *	这个类在Lyra项目中也没有直接用到
 *	但是 PerformTrace的逻辑和 UAbilityTask_WaitForInteractableTargets_SingleLineTrace 是类似的
 */
UCLASS(Blueprintable)
class AGameplayAbilityTargetActor_Interact : public AGameplayAbilityTargetActor_Trace
{
	GENERATED_BODY()

public:
	AGameplayAbilityTargetActor_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual FHitResult PerformTrace(AActor* InSourceActor) override;

protected:
};
