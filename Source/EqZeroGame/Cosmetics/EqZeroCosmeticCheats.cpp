// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCosmeticCheats.h"
#include "Cosmetics/EqZeroCharacterPartTypes.h"
#include "EqZeroControllerComponent_CharacterParts.h"
#include "GameFramework/CheatManagerDefines.h"
#include "System/EqZeroDevelopmentStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCosmeticCheats)

//////////////////////////////////////////////////////////////////////
// UEqZeroCosmeticCheats

UEqZeroCosmeticCheats::UEqZeroCosmeticCheats()
{
#if UE_WITH_CHEAT_MANAGER
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda(
			[](UCheatManager* CheatManager)
			{
				CheatManager->AddCheatManagerExtension(NewObject<ThisClass>(CheatManager));
			}));
	}
#endif
}

void UEqZeroCosmeticCheats::AddCharacterPart(const FString& AssetName, bool bSuppressNaturalParts)
{
#if UE_WITH_CHEAT_MANAGER
	if (UEqZeroControllerComponent_CharacterParts* CosmeticComponent = GetCosmeticComponent())
	{
		TSubclassOf<AActor> PartClass = UEqZeroDevelopmentStatics::FindClassByShortName<AActor>(AssetName);
		if (PartClass != nullptr)
		{
			FEqZeroCharacterPart Part;
			Part.PartClass = PartClass;

			CosmeticComponent->AddCheatPart(Part, bSuppressNaturalParts);
		}
	}
#endif
}

void UEqZeroCosmeticCheats::ReplaceCharacterPart(const FString& AssetName, bool bSuppressNaturalParts)
{
	ClearCharacterPartOverrides();
	AddCharacterPart(AssetName, bSuppressNaturalParts);
}

void UEqZeroCosmeticCheats::ClearCharacterPartOverrides()
{
#if UE_WITH_CHEAT_MANAGER
	if (UEqZeroControllerComponent_CharacterParts* CosmeticComponent = GetCosmeticComponent())
	{
		CosmeticComponent->ClearCheatParts();
	}
#endif
}

UEqZeroControllerComponent_CharacterParts* UEqZeroCosmeticCheats::GetCosmeticComponent() const
{
	if (APlayerController* PC = GetPlayerController())
	{
		return PC->FindComponentByClass<UEqZeroControllerComponent_CharacterParts>();
	}

	return nullptr;
}
