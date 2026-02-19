// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EqZeroHealthBarWidget.generated.h"

class APawn;
class APlayerController;
class UImage;
class UMaterialInstanceDynamic;
class UCommonNumericTextBlock;
class UWidgetAnimation;
class UEqZeroHealthComponent;

/**
 * Widget for displaying the player's health bar in the HUD.
 */
UCLASS(Abstract, Blueprintable)
class EQZEROGAME_API UEqZeroHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEqZeroHealthBarWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void OnHealthChange(UEqZeroHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	UFUNCTION()
	void EventOnEliminated();

	void SetDynamicMaterials();
	void InitializeBarVisuals();
	void UpdateHealthbar();

	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void ResetAnimatedState();

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonNumericTextBlock> HealthNumber;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> BarBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> BarFill;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> BarGlow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BarBorderMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BarFillMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BarGlowMID;

	UPROPERTY(BlueprintReadOnly, Category = "HealthBar")
	float NormalizedHealth = 1.f;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OnDamaged;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OnHealed;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OnEliminated;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OnSpawned;
};
