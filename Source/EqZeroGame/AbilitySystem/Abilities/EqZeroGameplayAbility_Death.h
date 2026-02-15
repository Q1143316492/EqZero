// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility.h"

#include "EqZeroGameplayAbility_Death.generated.h"

class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;


/**
 * UEqZeroGameplayAbility_Death
 *    这个技能是 GameplayEvent.Death 的 FGameplayEventData 触发的
 */
UCLASS(Abstract)
class UEqZeroGameplayAbility_Death : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "EqZeroAbility")
	void StartDeath();

	UFUNCTION(BlueprintCallable, Category = "EqZeroAbility")
	void FinishDeath();

	UFUNCTION()
	void OnDeathWaitFinished();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZeroDeath")
	TSubclassOf<UEqZeroCameraMode> DeathCameraModeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZeroDeath")
	float DeathDuration = 8.f;
	
	// 如果启用，该能力将自动调用 StartDeath。如果死亡已启动，那么当该能力结束时，FinishDeath 总会被调用。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZeroDeath")
	bool bAutoStartDeath;

	// GameplayCue.Character.Death 记得要配置一下
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZeroDeath")
	FGameplayTag DeathGameplayCueTag;
};
