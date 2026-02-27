// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroAbilitySystemComponent.h"

#include "Abilities/EqZeroGameplayAbility.h"
#include "AbilitySystem/EqZeroAbilityTagRelationshipMapping.h"
#include "Animation/EqZeroAnimInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "EqZeroGlobalAbilitySystem.h"
#include "EqZeroLogChannels.h"
#include "System/EqZeroAssetManager.h"
#include "System/EqZeroGameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

UEqZeroAbilitySystemComponent::UEqZeroAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UEqZeroAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEqZeroGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UEqZeroGlobalAbilitySystem>(GetWorld()))
	{
		GlobalAbilitySystem->UnregisterASC(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UEqZeroAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	// 挂在 PlayerState 上，但是由于时序，可能pawn ext 和 player state 都有可能调用过来

	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	// 如果 avatar 变了，重新初始化
	// 我们的ASC是在PlayerState，重生的时候Avatar会变。或者是业务上的操控角色变化
	if (bHasNewPawnAvatar)
	{
		// Notify all abilities that a new pawn avatar has been set
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ensureMsgf(AbilitySpec.Ability && AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("InitAbilityActorInfo: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				UEqZeroGameplayAbility* EqZeroAbilityInstance = Cast<UEqZeroGameplayAbility>(AbilityInstance);
				if (EqZeroAbilityInstance)
				{
					// Ability instances may be missing for replays
					EqZeroAbilityInstance->OnPawnAvatarSet();
				}
			}
		}

		// 全局注册一下ASC，这样就能一下子让全部角色播放技能了。这是一个WorldSubSystem
		if (UEqZeroGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UEqZeroGlobalAbilitySystem>(GetWorld()))
		{
			GlobalAbilitySystem->RegisterASC(this);
		}

		// 重新关联动画和ASC
		if (UEqZeroAnimInstance* EqZeroAnimInst = Cast<UEqZeroAnimInstance>(ActorInfo->GetAnimInstance()))
		{
			EqZeroAnimInst->InitializeWithAbilitySystem(this);
		}

		// 激活OnSpawn的时候就要激活的技能
		TryActivateAbilitiesOnSpawn();
	}
}

void UEqZeroAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	// 激活OnSpawn的时候就要激活的技能
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const UEqZeroGameplayAbility* EqZeroAbilityCDO = Cast<UEqZeroGameplayAbility>(AbilitySpec.Ability))
		{
			EqZeroAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void UEqZeroAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility)
{
	// 简单概况就是 通过 ShouldCancelFunc 检查需要取激活的技能，取消调用
	
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		UEqZeroGameplayAbility* EqZeroAbilityCDO = Cast<UEqZeroGameplayAbility>(AbilitySpec.Ability);
		if (!EqZeroAbilityCDO)
		{
			UE_LOG(LogEqZeroAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Non-EqZeroGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
			continue;
		}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// Cancel all the spawned instances.
		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			UEqZeroGameplayAbility* EqZeroAbilityInstance = CastChecked<UEqZeroGameplayAbility>(AbilityInstance);

			if (ShouldCancelFunc(EqZeroAbilityInstance, AbilitySpec.Handle))
			{
				if (EqZeroAbilityInstance->CanBeCanceled())
				{
					EqZeroAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), EqZeroAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
				}
				else
				{
					UE_LOG(LogEqZeroAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *EqZeroAbilityInstance->GetName());
				}
			}
		}
	}
}

void UEqZeroAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
	// 从GM指令发起的取消激活技能
	auto ShouldCancelFunc = [this](const UEqZeroGameplayAbility* EqZeroAbility, FGameplayAbilitySpecHandle Handle)
	{
		const EEqZeroAbilityActivationPolicy ActivationPolicy = EqZeroAbility->GetActivationPolicy();
		return ((ActivationPolicy == EEqZeroAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy == EEqZeroAbilityActivationPolicy::WhileInputActive));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UEqZeroAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// Invoke the InputPressed event. This is not replicated here.
		// If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void UEqZeroAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void UEqZeroAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void UEqZeroAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void UEqZeroAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
	{
		ClearAbilityInput();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// 官方留了一个待办，想用 FScopedServerAbilityRPCBatcher ScopedRPCBatcher 优化一下下面这堆循环

	//
	// 处理所有按住触发的技能
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			// 维护输入还在按住的状态
			// 虽然InputPressedSpecHandles也有，但是 Pressed里面每帧清空，跨帧的IA会有问题。
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UEqZeroGameplayAbility* EqZeroAbilityCDO = Cast<UEqZeroGameplayAbility>(AbilitySpec->Ability);
				if (EqZeroAbilityCDO && EqZeroAbilityCDO->GetActivationPolicy() == EEqZeroAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// 按下触发的技能
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UEqZeroGameplayAbility* EqZeroAbilityCDO = Cast<UEqZeroGameplayAbility>(AbilitySpec->Ability);

					if (EqZeroAbilityCDO && EqZeroAbilityCDO->GetActivationPolicy() == EEqZeroAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	// 尝试激活所有通过按压和长按触发的能力。
	// 我们一次性完成所有操作，这样长按输入就不会激活该能力 We do it all at once so that held inputs don't activate the ability
	// 然后也不会因为按压而向该能力发送输入事件。 and then also send one input event to the ability because of the press.
	// 解释：如果不批处理，WhileInputActive 先激活了技能，紧接着 Pressed 循环又看到它 IsActive 就给它发 InputPressed 事件 —— 技能会收到一个多余的输入事件
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// 处理所有松开触发的技能
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UEqZeroAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UEqZeroAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	if (UEqZeroGameplayAbility* EqZeroAbility = Cast<UEqZeroGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(EqZeroAbility->GetActivationGroup(), EqZeroAbility);
	}
}

void UEqZeroAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
	{
		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
		{
			ClientNotifyAbilityFailed(Ability, FailureReason);
			return;
		}
	}

	HandleAbilityFailed(Ability, FailureReason);
}

void UEqZeroAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	if (UEqZeroGameplayAbility* EqZeroAbility = Cast<UEqZeroGameplayAbility>(Ability))
	{
		RemoveAbilityFromActivationGroup(EqZeroAbility->GetActivationGroup(), EqZeroAbility);
	}
}

void UEqZeroAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	FGameplayTagContainer ModifiedBlockTags = BlockTags;
	FGameplayTagContainer ModifiedCancelTags = CancelTags;

	/*
	 * 技能激活 → GAS 调用 ApplyAbilityBlockAndCancelTags
	 * → TagRelationshipMapping->GetAbilityTagsToBlockAndCancel() 追加 Block/Cancel tag
	 * → Super::ApplyAbilityBlockAndCancelTags 执行实际的阻塞/取消
	 */
	
	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
	}

	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags, bExecuteCancelTags, ModifiedCancelTags);

	// 这里可以添加一些额外的逻辑，例如阻塞输入和移动
}

void UEqZeroAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	// 应用任何特殊逻辑，如阻止输入或移动
}

void UEqZeroAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
{
	/*
	* 技能尝试激活 → GAS 调用 CanActivateAbility
	* → GetAdditionalActivationTagRequirements
	* → TagRelationshipMapping->GetRequiredAndBlockedActivationTags() 追加条件
	* → GAS 用这些 tag 做最终的标签门控检查
	*/

	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
	}
}

void UEqZeroAbilitySystemComponent::SetTagRelationshipMapping(UEqZeroAbilityTagRelationshipMapping* NewMapping)
{
	TagRelationshipMapping = NewMapping;
}

void UEqZeroAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	HandleAbilityFailed(Ability, FailureReason);
}

void UEqZeroAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	//UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

	if (const UEqZeroGameplayAbility* EqZeroAbility = Cast<const UEqZeroGameplayAbility>(Ability))
	{
		EqZeroAbility->OnAbilityFailedToActivate(FailureReason);
	}
}

bool UEqZeroAbilitySystemComponent::IsActivationGroupBlocked(EEqZeroAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case EEqZeroAbilityActivationGroup::Independent:
		// Independent abilities are never blocked.
		bBlocked = false;
		break;

	case EEqZeroAbilityActivationGroup::Exclusive_Replaceable:
	case EEqZeroAbilityActivationGroup::Exclusive_Blocking:
		// Exclusive abilities can activate if nothing is blocking.
		bBlocked = (ActivationGroupCounts[(uint8)EEqZeroAbilityActivationGroup::Exclusive_Blocking] > 0);
		break;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	return bBlocked;
}

void UEqZeroAbilitySystemComponent::AddAbilityToActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility)
{
	check(EqZeroAbility);
	check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);

	ActivationGroupCounts[(uint8)Group]++;

	const bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case EEqZeroAbilityActivationGroup::Independent:
		// Independent abilities 什么也不需要
		break;

	case EEqZeroAbilityActivationGroup::Exclusive_Replaceable:
	case EEqZeroAbilityActivationGroup::Exclusive_Blocking:
		// 进来一个 可替代其他技能的技能，或者是阻塞其他技能的技能，就取消其他可被替代的技能Exclusive_Replaceable除了自己
		// 项目实际上所有技能都是独立的(没找到)，只有死亡技能会强改一个 Exclusive_Blocking 写进来，然后尝试取消其他技能。
		// 都是其他技能开火啥的都是独立的，也没有需要取消的。
		CancelActivationGroupAbilities(EEqZeroAbilityActivationGroup::Exclusive_Replaceable, EqZeroAbility, bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	// 跑进去应该是逻辑错误了，才会有多个独占技能在运行
	const int32 ExclusiveCount = ActivationGroupCounts[(uint8)EEqZeroAbilityActivationGroup::Exclusive_Replaceable] + ActivationGroupCounts[(uint8)EEqZeroAbilityActivationGroup::Exclusive_Blocking];
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogEqZeroAbilitySystem, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UEqZeroAbilitySystemComponent::RemoveAbilityFromActivationGroup(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* EqZeroAbility)
{
	check(EqZeroAbility);
	check(ActivationGroupCounts[(uint8)Group] > 0);

	ActivationGroupCounts[(uint8)Group]--;
}

void UEqZeroAbilitySystemComponent::CancelActivationGroupAbilities(EEqZeroAbilityActivationGroup Group, UEqZeroGameplayAbility* IgnoreEqZeroAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this, Group, IgnoreEqZeroAbility](const UEqZeroGameplayAbility* EqZeroAbility, FGameplayAbilitySpecHandle Handle)
	{
		return ((EqZeroAbility->GetActivationGroup() == Group) && (EqZeroAbility != IgnoreEqZeroAbility));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UEqZeroAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UEqZeroAssetManager::GetSubclass(UEqZeroGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UEqZeroGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
		return;
	}

	Spec->DynamicGrantedTags.AddTag(Tag);
	ApplyGameplayEffectSpecToSelf(*Spec);
}

void UEqZeroAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UEqZeroAssetManager::GetSubclass(UEqZeroGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *UEqZeroGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	Query.EffectDefinition = DynamicTagGE;

	RemoveActiveEffects(Query);
}

void UEqZeroAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
	if (ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
	}
}
