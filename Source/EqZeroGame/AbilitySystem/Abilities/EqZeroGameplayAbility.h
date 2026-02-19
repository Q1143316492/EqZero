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
class IEqZeroAbilitySourceInterface;
class UAnimMontage;
class UEqZeroAbilityCost;
class UEqZeroAbilitySystemComponent;
class UEqZeroCameraMode;
class UEqZeroHeroComponent;
class AEqZeroPlayerController;
class AEqZeroCharacter;
class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;

#define UE_API EQZEROGAME_API

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

	UE_API UEqZeroGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*
	 * Getter
	 */
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API AEqZeroPlayerController* GetEqZeroPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API AController* GetControllerFromActorInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API AEqZeroCharacter* GetEqZeroCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API UEqZeroHeroComponent* GetHeroComponentFromActorInfo() const;

	EEqZeroAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	
	EEqZeroAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

	/**
	 * 在Spawn时尝试激活技能
	 */
	UE_API void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	/**
	 * EndAbility 在基类中是 protected，此方法作为 public bridge 供外部调用。
	 * 等价于 Blueprint 的 "End Ability" 节点（bWasCancelled=false）。
	 */
	UE_API void ForceEndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

	/*
	 * ActivationGroup 改变激活组
	 * EEqZeroAbilityActivationGroup
	 *	- Independent: 独立技能，不受其他技能影响
	 *	- Exclusive_Replaceable: 排他可替换技能，会被其他排
	 *	- Exclusive_Blocking: 排他阻塞技能，会阻止其他排他技能
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "EqZero|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	UE_API bool CanChangeActivationGroup(EEqZeroAbilityActivationGroup NewGroup) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "EqZero|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	UE_API bool ChangeActivationGroup(EEqZeroAbilityActivationGroup NewGroup);

	/*
	 * 摄像机模式
	 */
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API void SetCameraMode(TSubclassOf<UEqZeroCameraMode> CameraMode);

	// 清除技能的摄像机模式。如果需要，在技能结束时自动调用。
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	UE_API void ClearCameraMode();

	/*
	 * 技能激活失败
	 */
	void OnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
	{
		NativeOnAbilityFailedToActivate(FailedReason);
		ScriptOnAbilityFailedToActivate(FailedReason);
	}

protected:
	UE_API virtual void NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	UFUNCTION(BlueprintImplementableEvent)
	UE_API void ScriptOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	//~UGameplayAbility interface
	UE_API virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	UE_API virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
	UE_API virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	UE_API virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	UE_API virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UE_API virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UE_API virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	UE_API virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	UE_API virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	UE_API virtual void ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec& Spec, FGameplayAbilitySpec* AbilitySpec) const override;
	UE_API virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	//~End of UGameplayAbility interface

	/*
	 * 技能的Avatar被设置时调用, 通常 InitAbilityActorInfo 中变更Avatar时调用
	 */
	UE_API virtual void OnPawnAvatarSet();
	
	UE_API virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel, const IEqZeroAbilitySourceInterface*& OutAbilitySource, AActor*& OutEffectCauser) const;

	/*
	 * 蓝图接口
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	UE_API void K2_OnAbilityAdded();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	UE_API void K2_OnAbilityRemoved();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
	UE_API void K2_OnPawnAvatarSet();

protected:
	/*
	 * 激活策略
	 * - OnInputTriggered: 当输入触发时尝试激活技能
	 * - WhileInputActive: 当输入持续激活时持续尝试激活技能
	 * - OnSpawn: 当Avatar被分配时尝试激活技能
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Ability Activation")
	EEqZeroAbilityActivationPolicy ActivationPolicy;

	/*
	 * 定义此技能激活与其他技能激活之间的关系。例如死亡技能阻塞其他所有技能
	 * - Independent: 独立技能，不受其他技能影响
	 * - Exclusive_Replaceable: 排他可替换技能，会被其他排他技能取消
	 * - Exclusive_Blocking: 排他阻塞技能，会阻止其他排他技能
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|Ability Activation")
	EEqZeroAbilityActivationGroup ActivationGroup;

	/*
	 * 当前技能的花费, 我们自己定义的结构,并在CanActivateAbility中进行检查, 在ActivateAbility中进行扣除
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
	TArray<TObjectPtr<UEqZeroAbilityCost>> AdditionalCosts;

	/*
	 * 进失败的时候UI显示的失败原因和动画等
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	TMap<FGameplayTag, FText> FailureTagToUserFacingMessages;

	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FailureTagToAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	bool bLogCancelation;

	/*
	 * HeroComp会来拿这个CameraMode来切换摄像机, 例如ADS开镜技能就有这个配置
	 */
	TSubclassOf<UEqZeroCameraMode> ActiveCameraMode;
};

#undef UE_API