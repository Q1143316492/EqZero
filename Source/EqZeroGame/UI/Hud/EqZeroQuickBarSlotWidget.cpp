// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Hud/EqZeroQuickBarSlotWidget.h"
#include "Inventory/InventoryFragment_QuickBarIcon.h"
#include "Inventory/EqZeroInventoryItemInstance.h"
#include "Components/Image.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CommonTextBlock.h"
#include "CommonNumericTextBlock.h"
#include "Equipment/EqZeroQuickBarComponent.h"
#include "EqZeroGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroQuickBarSlotWidget)

UEqZeroQuickBarSlotWidget::UEqZeroQuickBarSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroQuickBarSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UpdateSlot(nullptr);

	UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(this);

	// Then 0: 监听激活槽位变更 → UpdateIsSelected
	ActiveIndexChangedHandle = MsgSys.RegisterListener<FEqZeroQuickBarActiveIndexChangedMessage>(
		FGameplayTag::RequestGameplayTag(TEXT("EqZero.QuickBar.Message.ActiveIndexChanged")),
		[this](FGameplayTag, const FEqZeroQuickBarActiveIndexChangedMessage& Message)
		{
			if (Message.Owner == GetOwningPlayer())
			{
				UpdateIsSelected();
			}
		});

	// Then 1: 监听槽位内容变更 → UpdateSlot
	SlotsChangedHandle = MsgSys.RegisterListener<FEqZeroQuickBarSlotsChangedMessage>(
		FGameplayTag::RequestGameplayTag(TEXT("EqZero.QuickBar.Message.SlotsChanged")),
		[this](FGameplayTag, const FEqZeroQuickBarSlotsChangedMessage& Message)
		{
			if (Message.Owner == GetOwningPlayer())
			{
				if (Message.Slots.IsValidIndex(SlotIndex))
				{
					UpdateSlot(Message.Slots[SlotIndex]);
				}
				else
				{
					UpdateSlot(nullptr);
				}
			}
		});
}

void UEqZeroQuickBarSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateComponentRef(GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UEqZeroQuickBarComponent>() : nullptr);
	UpdateTotalAmmoLeft();
}

void UEqZeroQuickBarSlotWidget::NativeDestruct()
{
	ActiveIndexChangedHandle.Unregister();
	SlotsChangedHandle.Unregister();

	Super::NativeDestruct();
}

bool UEqZeroQuickBarSlotWidget::IsThisTheActiveQuickBarSlot() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UEqZeroQuickBarComponent* QuickBar = PC->FindComponentByClass<UEqZeroQuickBarComponent>())
		{
			return SlotIndex == QuickBar->GetActiveSlotIndex();
		}
	}
	return false;
}

void UEqZeroQuickBarSlotWidget::UpdateComponentRef(UEqZeroQuickBarComponent* InQuickBarComponent)
{
	if (CachedQuickBarComponent == InQuickBarComponent)
	{
		return ;
	}
	
	CachedQuickBarComponent = InQuickBarComponent;
	if (!CachedQuickBarComponent)
	{
		return;
	}

	if (CachedQuickBarComponent->GetSlots().IsValidIndex(SlotIndex))
	{
		UpdateSlot(CachedQuickBarComponent->GetSlots()[SlotIndex]);
	}
	else
	{
		UpdateSlot(nullptr);
	}
}

void UEqZeroQuickBarSlotWidget::UpdateSlot(const UEqZeroInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		UpdateEmptyState(true);
		return;
	}

	if (const UInventoryFragment_QuickBarIcon* IconInfo = ItemInstance->FindFragmentByClass<UInventoryFragment_QuickBarIcon>())
	{
		// Brush → WeaponCard dynamic material "IconTexture"
		if (UTexture* CardTexture = Cast<UTexture>(IconInfo->Brush.GetResourceObject()))
		{
			if (UMaterialInstanceDynamic* DynMat = WeaponCard->GetDynamicMaterial())
			{
				DynMat->SetTextureParameterValue(TEXT("IconTexture"), CardTexture);
			}
		}

		// AmmoBrush → AmmoIcon dynamic material "TextureMask"
		if (UTexture* AmmoTexture = Cast<UTexture>(IconInfo->AmmoBrush.GetResourceObject()))
		{
			if (UMaterialInstanceDynamic* DynMat = AmmoIcon->GetDynamicMaterial())
			{
				DynMat->SetTextureParameterValue(TEXT("TextureMask"), AmmoTexture);
			}
		}

		UpdateEmptyState(false);
	}
	else
	{
		UpdateEmptyState(true);
	}
}

void UEqZeroQuickBarSlotWidget::UpdateEmptyState(bool bInIsEmpty)
{
	// 这里可以加动画
	
	if (bInIsEmpty)
	{
		EmptyText->SetVisibility(ESlateVisibility::Visible);
		WeaponCard->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		EmptyText->SetVisibility(ESlateVisibility::Hidden);
		WeaponCard->SetVisibility(ESlateVisibility::Visible);
	}
	bIsEmpty = bInIsEmpty;
}

void UEqZeroQuickBarSlotWidget::UpdateIsSelected()
{
	if (bIsEmpty)
	{
		return;
	}

	if (IsThisTheActiveQuickBarSlot())
	{
		// 激活
		if (!bIsSelected)
		{
			bIsSelected = true;
			// PlayAnimation(InactiveToActive, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		}
	}
	else
	{
		// 取消激活
		if (bIsSelected)
		{
			// if (IsAnimationPlaying(InactiveToActive))
			{
				// StopAnimation(InactiveToActive);
			}
			bIsSelected = false;
			// PlayAnimation(ActiveToInactive, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		}
	}
}

void UEqZeroQuickBarSlotWidget::UpdateTotalAmmoLeft()
{
	if (!CachedQuickBarComponent)
	{
		return;
	}

	const TArray<UEqZeroInventoryItemInstance*> Slots = CachedQuickBarComponent->GetSlots();
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return;
	}

	const UEqZeroInventoryItemInstance* Item = Slots[SlotIndex];
	if (!Item || bIsEmpty)
	{
		return;
	}

	const int32 SpareAmmo = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo);
	const int32 MagAmmo   = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo);
	WeaponAmmoCount->SetCurrentValue(static_cast<float>(SpareAmmo + MagAmmo));
}