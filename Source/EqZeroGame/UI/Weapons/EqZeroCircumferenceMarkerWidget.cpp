// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Weapons/EqZeroCircumferenceMarkerWidget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCircumferenceMarkerWidget)

class SWidget;

UEqZeroCircumferenceMarkerWidget::UEqZeroCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
}

void UEqZeroCircumferenceMarkerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMarkerWidget.Reset();
}

TSharedRef<SWidget> UEqZeroCircumferenceMarkerWidget::RebuildWidget()
{
	MyMarkerWidget = SNew(SEqZeroCircumferenceMarkerWidget)
		.MarkerBrush(&MarkerImage)
		.Radius(this->Radius)
		.MarkerList(this->MarkerList);

	return MyMarkerWidget.ToSharedRef();
}

void UEqZeroCircumferenceMarkerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyMarkerWidget->SetRadius(Radius);
	MyMarkerWidget->SetMarkerList(MarkerList);
}

void UEqZeroCircumferenceMarkerWidget::SetRadius(float InRadius)
{
	Radius = InRadius;
	if (MyMarkerWidget.IsValid())
	{
		MyMarkerWidget->SetRadius(InRadius);
	}
}
