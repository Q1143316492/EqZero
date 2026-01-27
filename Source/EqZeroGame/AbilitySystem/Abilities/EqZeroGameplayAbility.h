// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"

#include "EqZeroGameplayAbility.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;

class AActor;
class AController;
class APlayerController;
class FText;
// class IEqZeroAbilitySourceInterface; // TODO
class UAnimMontage;
// class UEqZeroAbilityCost; // TODO
class UEqZeroAbilitySystemComponent;
// class UEqZeroCameraMode; // TODO
// class UEqZeroHeroComponent; // TODO
// class AEqZeroPlayerController; // TODO
// class AEqZeroCharacter; // TODO
class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;

/**
 * EEqZeroAbilityActivationPolicy
 *
 *      定义技能激活的时机
 *      - OnInputTriggered: 当输入触发时尝试激活技能
 *      - WhileInputActive: 当输入持续激活时持续尝试激活技能
 *      - OnSpawn: 当Avatar被分配时尝试激活技能
 *
 */
UENUM(BlueprintType)
enum class EEqZeroAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive,
	OnSpawn
};


/**
 * EEqZeroAbilityActivationGroup
 *
 *      定义技能激活时与其他技能的关系。
 *    - Independent: 独立技能，不受其他技能影响
 *    - Exclusive_Replaceable: 排他可替换技能，会被其他排他技能取消
 *    - Exclusive_Blocking: 排他阻塞技能，会阻止其他排他技能
 */
UENUM(BlueprintType)
enum class EEqZeroAbilityActivationGroup : uint8
{
	Independent,
	Exclusive_Replaceable,
	Exclusive_Blocking,
	MAX     UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FEqZeroAbilityMontageFailureMessage
{
	GENERATED_BODY()

public:
	// 如果ASC是player拥有的，失败激活技能的Player controller
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> AvatarActor = nullptr;

	// 技能失败的原因
	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer FailureTags;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAnimMontage> FailureMontage = nullptr;
};

/**
 * UEqZeroGameplayAbility
 *      项目中使用的基础游戏玩法能力类。
 */
UCLASS(MinimalAPI, Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class UEqZeroGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	friend class UEqZeroAbilitySystemComponent;

public:

	EQZEROGAME_API UEqZeroGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponentFromActorInfo() const;

	// TODO: Uncomment when EqZeroPlayerController is created
	/*
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API AEqZeroPlayerController* GetEqZeroPlayerControllerFromActorInfo() const;
	*/

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API AController* GetControllerFromActorInfo() const;

	// TODO: Uncomment when EqZeroCharacter is created
	/*
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API AEqZeroCharacter* GetEqZeroCharacterFromActorInfo() const;
	*/

	// TODO: Uncomment when EqZeroHeroComponent is created
	/*
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API UEqZeroHeroComponent* GetHeroComponentFromActorInfo() const;
	*/

	EEqZeroAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	EEqZeroAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

	EQZEROGAME_API void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "EqZero|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	EQZEROGAME_API bool CanChangeActivationGroup(EEqZeroAbilityActivationGroup NewGroup) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "EqZero|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	EQZEROGAME_API bool ChangeActivationGroup(EEqZeroAbilityActivationGroup NewGroup);

	// 设置技能的摄像机模式。
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	// TODO: Replace UObject with UEqZeroCameraMode when created
	// EQZEROGAME_API void SetCameraMode(TSubclassOf<UEqZeroCameraMode> CameraMode);
	EQZEROGAME_API void SetCameraMode(TSubclassOf<UObject> CameraMode);

	// 清除技能的摄像机模式。如果需要，在技能结束时自动调用。
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	EQZEROGAME_API void ClearCameraMode();

	void OnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
	{
		NativeOnAbilityFailedToActivate(FailedReason);
		ScriptOnAbilityFailedToActivate(FailedReason);
	}

protected:
	EQZEROGAME_API virtual void NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	UFUNCTION(BlueprintImplementableEvent)
	EQZEROGAME_API void ScriptOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	//~UGameplayAbility interface
	EQZEROGAME_API virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	EQZEROGAME_API virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
	EQZEROGAME_API virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	EQZEROGAME_API virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	EQZEROGAME_API virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	EQZEROGAME_API virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	EQZEROGAME_API virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	EQZEROGAME_API virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	EQZEROGAME_API virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	EQZEROGAME_API virtual void ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec& Spec, FGameplayAbilitySpec* AbilitySpec) const override;
	EQZEROGAME_API virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	//~End of UGameplayAbility interface

	EQZEROGAME_API virtual void OnPawnAvatarSet();

	// TODO: Replace IEqZeroAbilitySourceInterface with correct interface when created
	/*
	EQZEROGAME_API virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel, const IEqZeroAbilitySourceInterface*& OutAbilitySource, AActor*& OutEffectCauser) const;
	*/
	EQZEROGAME_API virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel, const UObject*& OutAbilitySource, AActor*& OutEffectCauser) const;

	/** Called when this ability is granted to the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	EQZEROGAME_API void K2_OnAbilityAdded();

	/** Called when this ability is removed from the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	EQZEROGAME_API void K2_OnAbilityRemoved();

	/** Called when the ability system is initialized with a pawn avatar. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
	EQZEROGAME_API void K2_OnPawnAvatarSet();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Ability Activation")
	EEqZeroAbilityActivationPolicy ActivationPolicy;

	// 定义此技能激活与其他技能激活之间的关系。例如死亡技能阻塞其他所有技能
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Ability Activation")
	EEqZeroAbilityActivationGroup ActivationGroup;

	// Additional costs that must be paid to activate this ability
	UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
	// TODO: Replace UObject with UEqZeroAbilityCost
	// TArray<TObjectPtr<UEqZeroAbilityCost>> AdditionalCosts;
	TArray<TObjectPtr<UObject>> AdditionalCosts;

	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	TMap<FGameplayTag, FText> FailureTagToUserFacingMessages;

	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FailureTagToAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	bool bLogCancelation;

	// Current camera mode set by the ability.
	// TODO: Replace UObject with UEqZeroCameraMode
	// TSubclassOf<UEqZeroCameraMode> ActiveCameraMode;
	TSubclassOf<UObject> ActiveCameraMode;
};
