// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroTaggedActor.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroTaggedActor)

AEqZeroTaggedActor::AEqZeroTaggedActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AEqZeroTaggedActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(StaticGameplayTags);
}

#if WITH_EDITOR
bool AEqZeroTaggedActor::CanEditChange(const FProperty* InProperty) const
{
	// Prevent editing of the other tags property
    // 避免改到 actor 里面的那个 tag 了, 只能改这个 StaticGameplayTags
	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AActor, Tags))
	{
		return false;
	}

	return Super::CanEditChange(InProperty);
}
#endif
