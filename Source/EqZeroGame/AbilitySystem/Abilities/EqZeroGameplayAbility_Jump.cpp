// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_Jump.h"

#include "EqZeroGameplayAbility.h"
#include "Character/EqZeroCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Jump)

struct FGameplayTagContainer;


UEqZeroGameplayAbility_Jump::UEqZeroGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UEqZeroGameplayAbility_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	const AEqZeroCharacter* EqZeroCharacter = Cast<AEqZeroCharacter>(ActorInfo->AvatarActor.Get());
	if (!EqZeroCharacter || !EqZeroCharacter->CanJump())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UEqZeroGameplayAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CharacterJumpStop();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UEqZeroGameplayAbility_Jump::CharacterJumpStart()
{
	if (AEqZeroCharacter* EqZeroCharacter = GetEqZeroCharacterFromActorInfo())
	{
		if (EqZeroCharacter->IsLocallyControlled() && !EqZeroCharacter->bPressedJump)
		{
			EqZeroCharacter->UnCrouch();
			EqZeroCharacter->Jump();
		}
	}
}

void UEqZeroGameplayAbility_Jump::CharacterJumpStop()
{
	if (AEqZeroCharacter* EqZeroCharacter = GetEqZeroCharacterFromActorInfo())
	{
		if (EqZeroCharacter->IsLocallyControlled() && EqZeroCharacter->bPressedJump)
		{
			EqZeroCharacter->StopJumping();
		}
	}
}
