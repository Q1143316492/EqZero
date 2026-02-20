// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/EqZeroTaggedWidget.h"

#include "EqZeroWeaponAmmoAndNameWidget.generated.h"

class UImage;
class UVerticalBox;
class UCommonTextBlock;
class UEqZeroInventoryItemInstance;

/**
 * W_WeaponAmmoAndName 的 C++ 父类
 * 显示当前武器弹药数与名称的 Widget
 * 
 * 代办：
 * - 这里有一个点击换弹，但是我们做的是PC
 */
UCLASS(Abstract, Blueprintable)
class EQZEROGAME_API UEqZeroWeaponAmmoAndNameWidget : public UEqZeroTaggedWidget
{
	GENERATED_BODY()

public:
	UEqZeroWeaponAmmoAndNameWidget(const FObjectInitializer& ObjectInitializer);
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:

	UPROPERTY(BlueprintReadOnly, Category=Weapon)
	TObjectPtr<UEqZeroInventoryItemInstance> InventoryItem;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> AmmoIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBoxForHiding;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> AmmoLeftInMagazineWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TotalCountWidget;

};
