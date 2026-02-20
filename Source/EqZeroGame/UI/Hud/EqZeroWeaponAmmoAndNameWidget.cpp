// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Hud/EqZeroWeaponAmmoAndNameWidget.h"

#include "CommonTextBlock.h"
#include "Equipment/EqZeroQuickBarComponent.h"
#include "Inventory/InventoryFragment_QuickBarIcon.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EqZeroGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWeaponAmmoAndNameWidget)

UEqZeroWeaponAmmoAndNameWidget::UEqZeroWeaponAmmoAndNameWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroWeaponAmmoAndNameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UEqZeroQuickBarComponent* QuickBar = GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UEqZeroQuickBarComponent>() : nullptr;
	if (!QuickBar)
	{
		InventoryItem = nullptr;
		return;
	}

	InventoryItem = QuickBar->GetActiveSlotItem();
	if (!InventoryItem)
	{
		VerticalBoxForHiding->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	 if (const UInventoryFragment_QuickBarIcon* IconInfo = InventoryItem->FindFragmentByClass<UInventoryFragment_QuickBarIcon>())
	 {
	 	if (UTexture* AmmoTexture = Cast<UTexture>(IconInfo->AmmoBrush.GetResourceObject()))
	 	{
	 		if (UMaterialInstanceDynamic* DynMat = AmmoIcon->GetDynamicMaterial())
	 		{
	 			DynMat->SetTextureParameterValue(TEXT("TextureMask"), AmmoTexture);
	 		}
	 	}
	 }

	VerticalBoxForHiding->SetVisibility(ESlateVisibility::Visible);

	AmmoLeftInMagazineWidget->SetText(FText::AsNumber(InventoryItem->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo)));
	TotalCountWidget->SetText(FText::AsNumber(InventoryItem->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo)));
		
}