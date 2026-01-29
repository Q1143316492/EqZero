// Copyright Epic Games, Inc. All Rights Reserved.
// TODO setting local

#include "EqZeroInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Player/EqZeroLocalPlayer.h"
// #include "Settings/EqZeroSettingsLocal.h" // TODO: Implement this class

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroInputComponent)

class UEqZeroInputConfig;

UEqZeroInputComponent::UEqZeroInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UEqZeroInputComponent::AddInputMappings(const UEqZeroInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);
}

void UEqZeroInputComponent::RemoveInputMappings(const UEqZeroInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);
}

void UEqZeroInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}