// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Weapons/EqZeroHitMarkerConfirmationWidget.h"

#include "Blueprint/UserWidget.h"
#include "UI/Weapons/SEqZeroHitMarkerConfirmationWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHitMarkerConfirmationWidget)

class SWidget;

UEqZeroHitMarkerConfirmationWidget::UEqZeroHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
	AnyHitsMarkerImage.DrawAs = ESlateBrushDrawType::NoDrawType;
}

void UEqZeroHitMarkerConfirmationWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMarkerWidget.Reset();
}

TSharedRef<SWidget> UEqZeroHitMarkerConfirmationWidget::RebuildWidget()
{
	UUserWidget* OuterUserWidget = GetTypedOuter<UUserWidget>();
	FLocalPlayerContext DummyContext;
	const FLocalPlayerContext& PlayerContextRef = (OuterUserWidget != nullptr) ? OuterUserWidget->GetPlayerContext() : DummyContext;

	MyMarkerWidget = SNew(SEqZeroHitMarkerConfirmationWidget, PlayerContextRef, PerHitMarkerZoneOverrideImages)
		.PerHitMarkerImage(&(this->PerHitMarkerImage))
		.AnyHitsMarkerImage(&(this->AnyHitsMarkerImage))
		.HitNotifyDuration(this->HitNotifyDuration);

	return MyMarkerWidget.ToSharedRef();
}
