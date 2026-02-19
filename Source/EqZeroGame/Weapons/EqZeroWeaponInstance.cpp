// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroWeaponInstance.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputDeviceProperties.h"
#include "Character/EqZeroHealthComponent.h"
#include "Cosmetics/EqZeroCosmeticAnimationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWeaponInstance)

class UAnimInstance;
struct FGameplayTagContainer;

UEqZeroWeaponInstance::UEqZeroWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (APawn* Pawn = GetPawn())
	{
		if (Pawn->IsPlayerControlled()) // 设备属性不针对机器人
		{
			if (UEqZeroHealthComponent* HealthComponent = UEqZeroHealthComponent::FindHealthComponent(GetPawn()))
			{
				HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
			}
		}
	}
}

void UEqZeroWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	UWorld* World = GetWorld();
	check(World);
	TimeLastEquipped = World->GetTimeSeconds();

	ApplyDeviceProperties();
}

void UEqZeroWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();

	RemoveDeviceProperties();
}

void UEqZeroWeaponInstance::UpdateFiringTime()
{
	UWorld* World = GetWorld();
	check(World);
	TimeLastFired = World->GetTimeSeconds();
}

float UEqZeroWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	UWorld* World = GetWorld();
	check(World);
	const double WorldTime = World->GetTimeSeconds();

	double Result = WorldTime - TimeLastEquipped;

	if (TimeLastFired > 0.0)
	{
		const double TimeSinceFired = WorldTime - TimeLastFired;
		Result = FMath::Min(Result, TimeSinceFired);
	}

	return Result;
}

TSubclassOf<UAnimInstance> UEqZeroWeaponInstance::PickBestAnimLayer(bool bEquipped, const FGameplayTagContainer& CosmeticTags) const
{
	const FEqZeroAnimLayerSelectionSet& SetToQuery = (bEquipped ? EquippedAnimSet : UneuippedAnimSet);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

const FPlatformUserId UEqZeroWeaponInstance::GetOwningUserId() const
{
	if (const APawn* Pawn = GetPawn())
	{
		return Pawn->GetPlatformUserId();
	}
	return PLATFORMUSERID_NONE;
}

void UEqZeroWeaponInstance::ApplyDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();

	if (UserId.IsValid())
	{
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			for (TObjectPtr<UInputDeviceProperty>& DeviceProp : ApplicableDeviceProperties)
			{
				FActivateDevicePropertyParams Params = {};
				Params.UserId = UserId;

				// By default, the device property will be played on the Platform User's Primary Input Device.
				// If you want to override this and set a specific device, then you can set the DeviceId parameter.
				//Params.DeviceId = <some specific device id>;
				
				// Don't remove this property it was evaluated. We want the properties to be applied as long as we are holding the 
				// weapon, and will remove them manually in OnUnequipped
				Params.bLooping = true;
			
				DevicePropertyHandles.Emplace(InputDeviceSubsystem->ActivateDeviceProperty(DeviceProp, Params));
			}
		}	
	}
}

void UEqZeroWeaponInstance::RemoveDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();
	
	if (UserId.IsValid() && !DevicePropertyHandles.IsEmpty())
	{
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			InputDeviceSubsystem->RemoveDevicePropertyHandles(DevicePropertyHandles);
			DevicePropertyHandles.Empty();
		}
	}
}

void UEqZeroWeaponInstance::OnDeathStarted(AActor* OwningActor)
{
	RemoveDeviceProperties();
}
