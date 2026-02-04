// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Debug/EqZeroAnimDebugWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAnimDebugWidget)

UEqZeroAnimDebugWidget::UEqZeroAnimDebugWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroAnimDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MessageChannel.IsValid())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		ListenerHandle = MessageSubsystem.RegisterListener(MessageChannel, this, &ThisClass::OnDebugMessageReceived);
	}
}

void UEqZeroAnimDebugWidget::NativeDestruct()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.UnregisterListener(ListenerHandle);

	Super::NativeDestruct();
}

void UEqZeroAnimDebugWidget::OnDebugMessageReceived(FGameplayTag Channel, const FEqZeroAnimDebugMessage& Payload)
{
	if (VBox_List)
	{
		const int32 IncomingCount = Payload.DebugLines.Num();
		int32 CurrentCount = VBox_List->GetChildrenCount();

		// Add needed widgets
		for (int32 i = CurrentCount; i < IncomingCount; ++i)
		{
			UTextBlock* NewText = NewObject<UTextBlock>(VBox_List);
			VBox_List->AddChildToVerticalBox(NewText);
		}

		// Update text and visibility
		// We re-query ChildrenCount because we might have added some
		CurrentCount = VBox_List->GetChildrenCount();
		
		for (int32 i = 0; i < CurrentCount; ++i)
		{
			UTextBlock* TextBlock = Cast<UTextBlock>(VBox_List->GetChildAt(i));
			if (TextBlock)
			{
				if (i < IncomingCount)
				{
					TextBlock->SetText(FText::FromString(Payload.DebugLines[i]));
					if (!TextBlock->IsVisible())
					{
						TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
				}
				else
				{
					if (TextBlock->IsVisible())
					{
						TextBlock->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
			}
		}
	}
}
