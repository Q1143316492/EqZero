// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "EqZeroAnimInstance.generated.h"

class UAbilitySystemComponent;


/**
 * UEqZeroAnimInstance
 *
 *      The base game animation instance class used by this project.
 */
UCLASS(Config = Game)
class UEqZeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UEqZeroAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float GroundDistance = -1.0f;
};
