// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/EqZeroTaggedWidget.h"

#include "EqZeroQuickBarWidget.generated.h"


class UEqZeroQuickBarSlotWidget;
/**
 * W_QuickBar 的 C++ 父类
 * 快捷栏整体 Widget
 */
UCLASS(Abstract, Blueprintable)
class EQZEROGAME_API UEqZeroQuickBarWidget : public UEqZeroTaggedWidget
{
	GENERATED_BODY()

public:
	UEqZeroQuickBarWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;

public:
	// W_QuickBarSlot W_QuickBarSlot_1

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEqZeroQuickBarSlotWidget> QuickBarSlot1;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEqZeroQuickBarSlotWidget> QuickBarSlot2;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEqZeroQuickBarSlotWidget> QuickBarSlot3;
};
