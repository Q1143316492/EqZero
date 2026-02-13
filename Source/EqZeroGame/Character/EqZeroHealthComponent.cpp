// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/EqZeroHealthComponent.h"

#include "AbilitySystem/Attributes/EqZeroAttributeSet.h"
#include "EqZeroLogChannels.h"
#include "System/EqZeroAssetManager.h"
#include "System/EqZeroGameData.h"
#include "EqZeroGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/EqZeroHealthSet.h"
#include "Messages/EqZeroVerbMessage.h"
#include "Messages/EqZeroVerbMessageHelpers.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHealthComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EqZero_Elimination_Message, "EqZero.Elimination.Message");
// UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_DamageSelfDestruct, "Gameplay.DamageSelfDestruct");
// UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_FellOutOfWorld, "Gameplay.FellOutOfWorld");


UEqZeroHealthComponent::UEqZeroHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = EEqZeroDeathState::NotDead;
}

void UEqZeroHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEqZeroHealthComponent, DeathState);
}

void UEqZeroHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();

	Super::OnUnregister();
}

void UEqZeroHealthComponent::InitializeWithAbilitySystem(UEqZeroAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UEqZeroHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	// 设置属性为默认值，都是最终应该是配表
	AbilitySystemComponent->SetNumericAttributeBase(UEqZeroHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());

	ClearGameplayTags();

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void UEqZeroHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UEqZeroHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Death_Dead, 0);
	}
}

float UEqZeroHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float UEqZeroHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float UEqZeroHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();

		return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
	}

	return 0.0f;
}

void UEqZeroHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	UE_LOG(LogEqZero, Log, TEXT("EqZeroHealthComponent: Health changed for owner [%s] from [%f] to [%f] (instigator: [%s], causer: [%s], magnitude: [%f])."), *GetNameSafe(GetOwner()), OldValue, NewValue, *GetNameSafe(DamageInstigator), *GetNameSafe(DamageCauser), DamageMagnitude);
	
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UEqZeroHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UEqZeroHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		// 发送 "GameplayEvent.Death" 这个事件会触发死亡技能
		{
			FGameplayEventData Payload;
			Payload.EventTag = EqZeroGameplayTags::GameplayEvent_Death;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec->Def;
			Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		{
			/*
			 * 官方写的待办
			 * 填写上下文标签，以及任何非能力系统的来源 / 发起者标签
			 * 判断这是敌方击杀、自我失误击杀、队友击杀等等……
			 */
			FEqZeroVerbMessage Message;
			Message.Verb = TAG_EqZero_Elimination_Message;
			Message.Instigator = DamageInstigator;
			Message.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Message.Target = UEqZeroVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
			Message.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}
	}

#endif // #if WITH_SERVER_CODE
}

void UEqZeroHealthComponent::OnRep_DeathState(EEqZeroDeathState OldDeathState)
{
	const EEqZeroDeathState NewDeathState = DeathState;

	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		// 服务器正试图让我们倒退，但我们已经预测到了服务器状态之后的情况。
		UE_LOG(LogEqZero, Warning, TEXT("EqZeroHealthComponent: Predicted past server death state [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EEqZeroDeathState::NotDead)
	{
		if (NewDeathState == EEqZeroDeathState::DeathStarted)
		{
			StartDeath();
		}
		else if (NewDeathState == EEqZeroDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}
	else if (OldDeathState == EEqZeroDeathState::DeathStarted)
	{
		if (NewDeathState == EEqZeroDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf((DeathState == NewDeathState), TEXT("EqZeroHealthComponent: Death transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
}

void UEqZeroHealthComponent::StartDeath()
{
	if (DeathState != EEqZeroDeathState::NotDead)
	{
		return;
	}

	DeathState = EEqZeroDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UEqZeroHealthComponent::FinishDeath()
{
	if (DeathState != EEqZeroDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EEqZeroDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UEqZeroHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	if ((DeathState == EEqZeroDeathState::NotDead) && AbilitySystemComponent)
	{
		// TODO 这里GameData还没有配置
		const TSubclassOf<UGameplayEffect> DamageGE = UEqZeroAssetManager::GetSubclass(UEqZeroGameData::Get().DamageGameplayEffect_SetByCaller);
		if (!DamageGE)
		{
			UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to find gameplay effect [%s]."), *GetNameSafe(GetOwner()), *UEqZeroGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		if (!Spec)
		{
			UE_LOG(LogEqZero, Error, TEXT("EqZeroHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to make outgoing spec for [%s]."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGE));
			return;
		}

		Spec->AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);

		if (bFellOutOfWorld)
		{
			Spec->AddDynamicAssetTag(TAG_Gameplay_FellOutOfWorld);
		}

		const float DamageAmount = GetMaxHealth();

		Spec->SetSetByCallerMagnitude(EqZeroGameplayTags::SetByCaller_Damage, DamageAmount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}
