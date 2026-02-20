// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility_FromEquipment.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "EqZeroGameplayAbility_ReloadMagazine.generated.h"

/**
 * UEqZeroGameplayAbility_ReloadMagazine
 *
 * 装弹技能基类，继承自 UEqZeroGameplayAbility_FromEquipment。
 */
UCLASS()
class EQZEROGAME_API UEqZeroGameplayAbility_ReloadMagazine : public UEqZeroGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_ReloadMagazine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	void DebugLog();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Reload)
	TObjectPtr<UAnimMontage> ReloadMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Reload)
	float PlayRate = 1.0f;

protected:
	bool DidBlockFiring = false;

	// 服务器端实际执行换弹逻辑（将备弹填入弹匣）
	UFUNCTION()
	void ReloadAmmoIntoMagazine(FGameplayEventData Payload);

	// 蒙太奇回调
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();
    
	UFUNCTION()
	void OnMontageCancelled();

	// 换弹完成事件回调
	UFUNCTION()
	void OnReloadEventReceived(FGameplayEventData Payload);
};
