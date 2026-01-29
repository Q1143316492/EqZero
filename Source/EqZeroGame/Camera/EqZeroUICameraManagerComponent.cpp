// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroUICameraManagerComponent)

UEqZeroUICameraManagerComponent::UEqZeroUICameraManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroUICameraManagerComponent::InitializeFor(AEqZeroPlayerCameraManager* InCameraManager)
{
	CameraManager = InCameraManager;
}

bool UEqZeroUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UEqZeroUICameraManagerComponent::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
}

void UEqZeroUICameraManagerComponent::OnShowDebugInfo(UCanvas* Canvas)
{
}
