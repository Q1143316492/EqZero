// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/EqZeroGameplayAbility.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "EqZeroGameplayAbility_AutoRespawn.generated.h"

class UEqZeroHealthComponent;
struct FEqZeroPlayerResetMessage;

USTRUCT(BlueprintType)
struct FEqZeroInteractionDurationMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float Duration = 0.0f;
};

/**
 * UEqZeroGameplayAbility_AutoRespawn
 *
 *	
 */
UCLASS()
class UEqZeroGameplayAbility_AutoRespawn : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:
	UEqZeroGameplayAbility_AutoRespawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "EqZero|Ability")
	AActor* GetOwningPlayerState() const;
	
	UFUNCTION(BlueprintPure, Category = "EqZero|Ability")
	bool IsAvatarDeadOrDying() const;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnPawnAvatarSet() override;
	
	void InternalActivateAutoRespawnAbility();
	
	void OnPlayerResetMessage(FGameplayTag Channel, const FEqZeroPlayerResetMessage& Payload);
	void OnPlayerReset();

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	void OnAvatarEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	void OnDeathStarted(AActor* OwningActor);
	
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	void BindDeathListener();
	
	UFUNCTION(BlueprintCallable, Category = "EqZero|Ability")
	void ClearDeathListener();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EqZero|Ability")
	float RespawnDelayDuration = 5.f;

	UPROPERTY(BlueprintReadOnly, Category = "EqZero|Ability")
	TObjectPtr<UEqZeroHealthComponent> LastBoundHealthComponent;

	UPROPERTY(BlueprintReadOnly, Category = "EqZero|Ability")
	TObjectPtr<AActor> LastBoundAvatarActor;

	UPROPERTY(BlueprintReadOnly, Category = "EqZero|Ability")
	bool bIsListeningForReset;

	UPROPERTY(BlueprintReadOnly, Category = "EqZero|Ability")
	bool bShouldFinishRestart;
	
	FGameplayMessageListenerHandle ListenerHandle;

	FTimerHandle TimerHandle_ResetRequest;
	FTimerHandle TimerHandle_Respawn;
};
