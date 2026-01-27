// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/EqZeroGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

#include "EqZeroAbilitySystemComponent.generated.h"

class AActor;
class UGameplayAbility;
// class UEqZeroAbilityTagRelationshipMapping; // TODO
class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;

EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

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

	EQZEROGAME_API UEqZeroAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	EQZEROGAME_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	EQZEROGAME_API virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	EQZEROGAME_API bool IsOwnerActorAuthoritative() const;

	EQZEROGAME_API void RemoveSpawnedAttribute(UAttributeSet* AttributeSet);

	typedef TFunctionRef<bool(const UEqZeroGameplayAbility* EqZeroAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	EQZEROGAME_API void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

	EQZEROGAME_API void CancelInputActivatedAbilities(bool bReplicateCancelAbility);

	EQZEROGAME_API void AbilityInputTagPressed(const FGameplayTag& InputTag);
	EQZEROGAME_API void AbilityInputTagReleased(const FGameplayTag& InputTag);

	EQZEROGAME_API void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	EQZEROGAME_API void ClearAbilityInput();

	EQZEROGAME_API bool IsActivationGroupBlocked(EEqZeroAbilityActivationGroup Group) const;
	EQZEROGAME_API void AddAbilityToActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility);
	EQZEROGAME_API void RemoveAbilityFromActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility);
	EQZEROGAME_API void CancelActivationGroupAbilities(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* IgnoreEqZeroAbility, bool bReplicateCancelAbility);

	// Uses a gameplay effect to add the specified dynamic granted tag.
	EQZEROGAME_API void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	EQZEROGAME_API void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/** Gets the ability target data associated with the given ability handle and activation info */
	EQZEROGAME_API void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);

	/** Sets the current tag relationship mapping, if null it will clear it out */
	// TODO: Replace UObject with UEqZeroAbilityTagRelationshipMapping
	// EQZEROGAME_API void SetTagRelationshipMapping(UEqZeroAbilityTagRelationshipMapping* NewMapping);
	EQZEROGAME_API void SetTagRelationshipMapping(UObject* NewMapping);

	/** Looks at ability tags and gathers additional required and blocking tags */
	EQZEROGAME_API void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;
	EQZEROGAME_API void TryActivateAbilitiesOnSpawn();

protected:

	EQZEROGAME_API virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	EQZEROGAME_API virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	EQZEROGAME_API virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	EQZEROGAME_API virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;
	EQZEROGAME_API virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	EQZEROGAME_API virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	EQZEROGAME_API virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) override;

	/** Notify client that an ability failed to activate */
	UFUNCTION(Client, Unreliable)
	EQZEROGAME_API void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	EQZEROGAME_API void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
protected:

	// If set, this table is used to look up tag relationships for activate and cancel
	UPROPERTY()
	// TODO: Replace UObject with UEqZeroAbilityTagRelationshipMapping
	// TObjectPtr<UEqZeroAbilityTagRelationshipMapping> TagRelationshipMapping;
	TObjectPtr<UObject> TagRelationshipMapping;

	// Handles to abilities that had their input pressed this frame.
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// Handles to abilities that had their input released this frame.
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// Handles to abilities that have their input held.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	// Number of abilities running in each activation group.
	int32 ActivationGroupCounts[(uint8)EEqZeroAbilityActivationGroup::MAX];
};
