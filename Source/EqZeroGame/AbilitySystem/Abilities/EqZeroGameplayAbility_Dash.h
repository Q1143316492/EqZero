// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility_WithWidget.h"

#include "EqZeroGameplayAbility_Dash.generated.h"

class UAnimMontage;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilityActivationInfo;
struct FGameplayEventData;

/**
 * UEqZeroGameplayAbility_Dash
 *
 *	Gameplay ability for character dash movement.
 */
UCLASS(Abstract)
class UEqZeroGameplayAbility_Dash : public UEqZeroGameplayAbility_WithWidget
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void GetDirection(FVector& OutFacing, FVector& OutLastMovementInput, FVector& OutMovement, bool &OutPositive);

	UFUNCTION(BlueprintCallable, Category = "Dash")
	void SelectDirectionalMontage(const FVector& Facing, const FVector& MovementDirection, UAnimMontage*& DirectionalMontage, bool& BiasForwardMovement);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnRootMotionFinished();

	UFUNCTION(Server, Reliable)
	void ServerSendInfo(const FVector& InDirection, UAnimMontage* InMontage);

	void ExecuteDash(const FVector& InDirection, UAnimMontage* InMontage);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float Strength = 1850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float RootMotionDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float AbilityDuration = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	FVector Direction = FVector::ZeroVector;

    /**
     * 配置
     */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Montages")
	TObjectPtr<UAnimMontage> Dash_Fwd_Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Montages")
	TObjectPtr<UAnimMontage> Dash_Bwd_Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Montages")
	TObjectPtr<UAnimMontage> Dash_Left_Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Montages")
	TObjectPtr<UAnimMontage> Dash_Right_Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float Cooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Montages")
	TObjectPtr<UAnimMontage> Montage = nullptr;

private:
	// Timer handle for delayed ability end
	FTimerHandle EndAbilityTimerHandle;
};
