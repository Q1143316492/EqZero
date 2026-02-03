// Copyright Epic Games, Inc. All Rights Reserved.
// TODO setting local

#include "EqZeroInputComponent.h"
#include "System/EqZeroGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Player/EqZeroLocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
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

void UEqZeroInputComponent::OnNativeInputAction(FGameplayTag InputTag, const FInputActionValue& InputActionValue)
{
	// 通过 InputComponent 的 Owner (通常是 Pawn) 找到 PlayerController -> LocalPlayer -> GameInstance
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}
	
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return;
	}
	
	UEqZeroLocalPlayer* LocalPlayer = Cast<UEqZeroLocalPlayer>(PC->GetLocalPlayer());
	if (!LocalPlayer)
	{
		return;
	}
	
	UEqZeroGameInstance* GameInstance = Cast<UEqZeroGameInstance>(LocalPlayer->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnNativeInputAction(InputTag, InputActionValue);
	}
}