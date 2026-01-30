// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Abilities/EqZeroGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

#include "EqZeroAbilitySystemComponent.generated.h"

#define UE_API EQZEROGAME_API

class AActor;
class UGameplayAbility;
class UEqZeroAbilityTagRelationshipMapping;
class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;

UE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

/**
 * UEqZeroAbilitySystemComponent
 *
 *      Base ability system component class used by this project.
 */
UCLASS(MinimalAPI)
class UEqZeroAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UE_API UEqZeroAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	UE_API virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/**
	 * 取消技能，暖场阶段或者指令触发取消所有技能
	 */
	typedef TFunctionRef<bool(const UEqZeroGameplayAbility* EqZeroAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	UE_API void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

	UE_API void CancelInputActivatedAbilities(bool bReplicateCancelAbility);

	/**
	 * 技能输入
	 */
	UE_API void AbilityInputTagPressed(const FGameplayTag& InputTag);
	UE_API void AbilityInputTagReleased(const FGameplayTag& InputTag);

	UE_API void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	UE_API void ClearAbilityInput();

	/**
	 * 技能组
	 */
	UE_API bool IsActivationGroupBlocked(EEqZeroAbilityActivationGroup Group) const;
	UE_API void AddAbilityToActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility);
	UE_API void RemoveAbilityFromActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility);
	UE_API void CancelActivationGroupAbilities(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* IgnoreEqZeroAbility, bool bReplicateCancelAbility);

	/*
	 *  GameData 配置了一下动态标签玩法效果
	 */
	
	UE_API void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);
	UE_API void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/** Gets the ability target data associated with the given ability handle and activation info */
	UE_API void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);

	UE_API void SetTagRelationshipMapping(UEqZeroAbilityTagRelationshipMapping* NewMapping);

	/** Looks at ability tags and gathers additional required and blocking tags */
	UE_API void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;
	UE_API void TryActivateAbilitiesOnSpawn();

protected:

	UE_API virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	UE_API virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	UE_API virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	UE_API virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;
	UE_API virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	UE_API virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	UE_API virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) override;

	UFUNCTION(Client, Unreliable)
	UE_API void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	UE_API void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
protected:
	UPROPERTY()
	TObjectPtr<UEqZeroAbilityTagRelationshipMapping> TagRelationshipMapping;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	int32 ActivationGroupCounts[(uint8)EEqZeroAbilityActivationGroup::MAX];
};

#undef UE_API