// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility_WithWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EqZeroGameplayAbility_Melee.generated.h"

namespace EDrawDebugTrace
{
	enum Type : int;
}

class UGameplayEffect;
class ACharacter;
class UAnimMontage;

/**
 * UEqZeroGameplayAbility_Melee
 *
 */
UCLASS()
class UEqZeroGameplayAbility_Melee : public UEqZeroGameplayAbility_WithWidget
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_Melee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void PlayMeleeMontage();

	void MeleeCapsuleTrace();

	void MeleeHitTargetSuccess(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult);

	void ApplyMeleeRootMotion(const FHitResult& HitResult);

	void ApplyMeleeGameplayEffectAndCue(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult);
	
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut();
	
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
    TEnumAsByte<EDrawDebugTrace::Type> DebugType = EDrawDebugTrace::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float Distance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float Radius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float HalfHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float Strength;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float Duration;
};
