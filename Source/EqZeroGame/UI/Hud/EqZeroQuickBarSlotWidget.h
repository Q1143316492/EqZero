// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/EqZeroTaggedWidget.h"
#include "Animation/WidgetAnimation.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "EqZeroQuickBarSlotWidget.generated.h"

class UImage;
class UCommonTextBlock;
class UCommonNumericTextBlock;
class UEqZeroQuickBarComponent;
class UEqZeroInventoryItemInstance;

/**
 * W_QuickBarSlot 的 C++ 父类
 * 快捷栏单个格子 Widget
 */
UCLASS(Abstract, Blueprintable)
class EQZEROGAME_API UEqZeroQuickBarSlotWidget : public UEqZeroTaggedWidget
{
	GENERATED_BODY()

public:
	UEqZeroQuickBarSlotWidget(const FObjectInitializer& ObjectInitializer);

    
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category=QuickBar)
	bool IsThisTheActiveQuickBarSlot() const;

	UFUNCTION(BlueprintCallable, Category=QuickBar)
	void UpdateComponentRef(UEqZeroQuickBarComponent* InQuickBarComponent);

	void SetSlotIndex(int32 InSlotIndex) { SlotIndex = InSlotIndex; }
protected:
	void UpdateSlot(const UEqZeroInventoryItemInstance* ItemInstance);
	void UpdateEmptyState(bool bInIsEmpty);
	void UpdateIsSelected();
	void UpdateTotalAmmoLeft();
	
public:
	UPROPERTY(BlueprintReadOnly, Category=QuickBar)
	bool bIsEmpty = false;
	
	UPROPERTY(BlueprintReadOnly, Category=QuickBar)
	bool bIsSelected = false;
	
	UPROPERTY(BlueprintReadOnly, Category=QuickBar)
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category=QuickBar)
	TObjectPtr<UEqZeroQuickBarComponent> CachedQuickBarComponent;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> WeaponCard;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> ItemGlow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> ItemGlow_Boost;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> AmmoIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> EmptyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonNumericTextBlock> WeaponAmmoCount;

	// UPROPERTY(Transient, meta = (BindWidgetAnim))
	// TObjectPtr<UWidgetAnimation> InactiveToActive;

	// UPROPERTY(Transient, meta = (BindWidgetAnim))
	// TObjectPtr<UWidgetAnimation> ActiveToInactive;

private:
	FGameplayMessageListenerHandle ActiveIndexChangedHandle;
	FGameplayMessageListenerHandle SlotsChangedHandle;
};
