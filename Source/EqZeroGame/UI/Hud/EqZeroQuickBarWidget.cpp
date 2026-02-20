// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Hud/EqZeroQuickBarWidget.h"

#include "EqZeroQuickBarSlotWidget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroQuickBarWidget)

UEqZeroQuickBarWidget::UEqZeroQuickBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroQuickBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (QuickBarSlot1)
	{
		QuickBarSlot1->SetSlotIndex(0);
	}
	if (QuickBarSlot2)
	{
		QuickBarSlot2->SetSlotIndex(1);
	}
	if (QuickBarSlot3)
	{
		QuickBarSlot3->SetSlotIndex(2);
	}
}