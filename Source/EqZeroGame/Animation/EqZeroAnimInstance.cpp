// Copyright Epic Games, Inc. All Rights Reserved.
// TODO 需要 move comp

#include "EqZeroAnimInstance.h"
#include "AbilitySystemGlobals.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAnimInstance)


UEqZeroAnimInstance::UEqZeroAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	GameplayTagPropertyMap.Initialize(this, ASC);
}

#if WITH_EDITOR
EDataValidationResult UEqZeroAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	GameplayTagPropertyMap.IsDataValid(this, Context);

	return ((Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif // WITH_EDITOR

void UEqZeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void UEqZeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// TODO: Update character-specific data when EqZeroCharacter and EqZeroCharacterMovementComponent are available
	// const AEqZeroCharacter* Character = Cast<AEqZeroCharacter>(GetOwningActor());
	// if (!Character)
	// {
	// 	return;
	// }
	//
	// UEqZeroCharacterMovementComponent* CharMoveComp = CastChecked<UEqZeroCharacterMovementComponent>(Character->GetCharacterMovement());
	// const FEqZeroCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	// GroundDistance = GroundInfo.GroundDistance;
}
