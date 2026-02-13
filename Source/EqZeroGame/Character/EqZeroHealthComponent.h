// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "EqZeroHealthComponent.generated.h"

#define UE_API EQZEROGAME_API

class UEqZeroHealthComponent;

class UEqZeroAbilitySystemComponent;
class UEqZeroHealthSet;
class UObject;
struct FFrame;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEqZeroHealth_DeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FEqZeroHealth_AttributeChanged, UEqZeroHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/**
 * EEqZeroDeathState
 *
 *      Defines current state of death.
 */
UENUM(BlueprintType)
enum class EEqZeroDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};


/**
 * UEqZeroHealthComponent
 */
UCLASS(MinimalAPI, Blueprintable, Meta=(BlueprintSpawnableComponent))
class UEqZeroHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	UE_API UEqZeroHealthComponent(const FObjectInitializer& ObjectInitializer);

	/*
	 * Getter
	 */
	UFUNCTION(BlueprintPure, Category = "EqZero|Health")
	static UEqZeroHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UEqZeroHealthComponent>() : nullptr); }

	/*
	 * 技能组件
	 */
	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	UE_API void InitializeWithAbilitySystem(UEqZeroAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	UE_API void UninitializeFromAbilitySystem();

	/*
	 * Get Attribute
	 */
	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	UE_API float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	UE_API float GetMaxHealth() const;

	// 返回当前生命值在 [0.0, 1.0] 范围内的比例。
	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	UE_API float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|Health")
	EEqZeroDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "EqZero|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return (DeathState > EEqZeroDeathState::NotDead); }

	/*
	 * 死亡流程
	 */
	UE_API virtual void StartDeath();

	UE_API virtual void FinishDeath();

	UE_API virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

public:

	/*
	 * 生命变化和死亡事件
	 */
	UPROPERTY(BlueprintAssignable)
	FEqZeroHealth_AttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FEqZeroHealth_AttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FEqZeroHealth_DeathEvent OnDeathStarted;

	UPROPERTY(BlueprintAssignable)
	FEqZeroHealth_DeathEvent OnDeathFinished;

protected:

	UE_API virtual void OnUnregister() override;

	UE_API void ClearGameplayTags();

	UE_API virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	UE_API virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	UE_API virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
	UE_API virtual void OnRep_DeathState(EEqZeroDeathState OldDeathState);

protected:
	UPROPERTY()
	TObjectPtr<UEqZeroAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UEqZeroHealthSet> HealthSet;

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EEqZeroDeathState DeathState;
};

#undef UE_API
