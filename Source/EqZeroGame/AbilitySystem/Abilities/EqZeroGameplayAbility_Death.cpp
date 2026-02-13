// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_Death.h"

#include "AbilitySystem/Abilities/EqZeroGameplayAbility.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/EqZeroHealthComponent.h"
#include "EqZeroGameplayTags.h"
#include "EqZeroLogChannels.h"
#include "Camera/EqZeroCameraMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Death)

UEqZeroGameplayAbility_Death::UEqZeroGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated; // 死亡应该由服务器控制

	bAutoStartDeath = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// 为 CDO 添加默认触发标签的功能。
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = EqZeroGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UEqZeroGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);
	
	UEqZeroAbilitySystemComponent* EqZeroASC = CastChecked<UEqZeroAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	// 取消所有能力并阻止其他能力的启动。注意这里ignore是重生技能，不能把重生技能给干掉了
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(EqZeroGameplayTags::Ability_Behavior_SurvivesDeath);
	EqZeroASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	// 死亡技能不能被取消
	SetCanBeCanceled(false);

	// 这个会在ASC写一个阻塞，然后其他技能都不能触发
	if (!ChangeActivationGroup(EEqZeroAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogEqZeroAbilitySystem, Error, TEXT("UEqZeroGameplayAbility_Death::ActivateAbility: Ability [%s] failed to change activation group to blocking."), *GetName());
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	if (DeathCameraModeClass)
	{
		SetCameraMode(DeathCameraModeClass);
	}

	if (TriggerEventData && DeathGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.NormalizedMagnitude = TriggerEventData->EventMagnitude;
		CueParams.RawMagnitude = TriggerEventData->EventMagnitude;
		CueParams.EffectContext = TriggerEventData->ContextHandle;
		CueParams.MatchedTagName = TriggerEventData->EventTag;
		CueParams.OriginalTag = TriggerEventData->EventTag;
		
		CueParams.AggregatedSourceTags = TriggerEventData->InstigatorTags;
		CueParams.AggregatedTargetTags = TriggerEventData->TargetTags;

		// Instigator: 也就是凶手
		CueParams.Instigator = const_cast<AActor*>(TriggerEventData->Instigator.Get());
		
		// EffectCauser: 通常指直接造成死亡的物体（比如子弹、投掷物），尝试从 OptionalObject 获取
		// 这里必须先去 const，再 Cast 成 Actor，因为 OptionalObject 是 UObject 类型
		CueParams.EffectCauser = Cast<AActor>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get()));

		// SourceObject: 可以放凶手或者凶器，或者其他上下文对象
		CueParams.SourceObject = const_cast<UObject*>(TriggerEventData->OptionalObject.Get());

		CueParams.GameplayEffectLevel = 1;
		CueParams.AbilityLevel = 1;
		
		K2_ExecuteGameplayCueWithParams(DeathGameplayCueTag, CueParams);
	}

	// 角色在濒死（布娃娃）状态停留多长时间，才调用 HealthComponent 上的 DeathFinished 并清理该角色。
	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, DeathDuration);
	Task->OnFinish.AddDynamic(this, &ThisClass::OnDeathWaitFinished);
	Task->ReadyForActivation();
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UEqZeroGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);

	// 当技能结束时，务必尝试完成击杀，以防技能无法做到这一点。
	// 如果死亡尚未开始，这不会有任何作用。
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UEqZeroGameplayAbility_Death::StartDeath()
{
	// ServerInitiated 服务端执行，然后客户端复制，所以双端执行
	if (UEqZeroHealthComponent* HealthComponent = UEqZeroHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EEqZeroDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void UEqZeroGameplayAbility_Death::FinishDeath()
{
	if (UEqZeroHealthComponent* HealthComponent = UEqZeroHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EEqZeroDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}

void UEqZeroGameplayAbility_Death::OnDeathWaitFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

