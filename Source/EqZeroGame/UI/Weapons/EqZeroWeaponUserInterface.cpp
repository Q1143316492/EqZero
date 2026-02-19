// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroWeaponUserInterface.h"

#include "Equipment/EqZeroEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "Weapons/EqZeroWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWeaponUserInterface)

struct FGeometry;

UEqZeroWeaponUserInterface::UEqZeroWeaponUserInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroWeaponUserInterface::NativeConstruct()
{
	Super::NativeConstruct();
}

void UEqZeroWeaponUserInterface::NativeDestruct()
{
	Super::NativeDestruct();
}

void UEqZeroWeaponUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UEqZeroEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UEqZeroEquipmentManagerComponent>())
		{
			if (UEqZeroWeaponInstance* NewInstance = EquipmentManager->GetFirstInstanceOfType<UEqZeroWeaponInstance>())
			{
				if (NewInstance != CurrentInstance && NewInstance->GetInstigator() != nullptr)
				{
					UEqZeroWeaponInstance* OldWeapon = CurrentInstance;
					CurrentInstance = NewInstance;
					RebuildWidgetFromWeapon();
					OnWeaponChanged(OldWeapon, CurrentInstance);
				}
			}
		}
	}
}

void UEqZeroWeaponUserInterface::RebuildWidgetFromWeapon()
{
	
}
