// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Hud/EqZeroHealthBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CommonNumericTextBlock.h"
#include "Character/EqZeroHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHealthBarWidget)

UEqZeroHealthBarWidget::UEqZeroHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetDynamicMaterials();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);
		OnPossessedPawnChanged(nullptr, PC->GetPawn());
	}

	InitializeBarVisuals();
}

void UEqZeroHealthBarWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPossessedPawnChanged);
	}

	Super::NativeDestruct();
}

void UEqZeroHealthBarWidget::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// 1: 解绑旧 Pawn 的血量监听，并重置动画
	if (UEqZeroHealthComponent* OldHealth = UEqZeroHealthComponent::FindHealthComponent(OldPawn))
	{
		OldHealth->OnHealthChanged.RemoveDynamic(this, &ThisClass::OnHealthChange);
	}

	UnbindAllFromAnimationFinished(OnEliminated);
	StopAnimation(OnEliminated);
	PlayAnimation(OnSpawned, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);

	// 2: 绑定新 Pawn 的血量监听并刷新显示
	if (UEqZeroHealthComponent* NewHealth = UEqZeroHealthComponent::FindHealthComponent(NewPawn))
	{
		NewHealth->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChange);
		NormalizedHealth = NewHealth->GetHealthNormalized();
	}

	InitializeBarVisuals();
	UpdateHealthbar();
}

void UEqZeroHealthBarWidget::OnHealthChange(UEqZeroHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	const float OldNormalized = OldValue / 100.0f;
	const float NewNormalized = NewValue / 100.0f;
	NormalizedHealth = NewNormalized;

	// 1. 更新材质参数：Health_Current = 旧值, Health_Updated = 新值
	for (UMaterialInstanceDynamic* MID : TArray<UMaterialInstanceDynamic*>{ BarBorderMID, BarFillMID, BarGlowMID })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Health_Current"), OldNormalized);
			MID->SetScalarParameterValue(TEXT("Health_Updated"), NewNormalized);
		}
	}

	// 2. 重置动画状态
	ResetAnimatedState();

	// 3. 数字已在目标值且是治疗/不变时跳过动画播放
	const bool bAlreadyAtTarget = HealthNumber && FMath::IsNearlyEqual(HealthNumber->GetTargetValue(), NewValue);
	const bool bIsHealingOrSame = NewValue >= OldValue;
	if (bAlreadyAtTarget && bIsHealingOrSame)
	{
		return;
	}

	// 4. 设置 DamageOrHealing 参数并播放对应动画
	const float DamageOrHealingValue = bIsHealingOrSame ? 1.0f : 0.0f;
	for (UMaterialInstanceDynamic* MID : TArray<UMaterialInstanceDynamic*>{ BarBorderMID, BarFillMID, BarGlowMID })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("DamageOrHealing"), DamageOrHealingValue);
		}
	}

	UWidgetAnimation* AnimToPlay = bIsHealingOrSame ? OnHealed : OnDamaged;
	PlayAnimation(AnimToPlay, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);

	// 5. 数字先跳到旧值，再插值到新值
	if (HealthNumber)
	{
		HealthNumber->SetCurrentValue(OldValue);
		HealthNumber->InterpolateToValue(NewValue, 1.0f, 4.0f, 0.0f);
	}

	// 6. 否为小数点比较2 = 0，绑定 OnDamaged 动画结束时触发 EventOnEliminated
	if (FMath::IsNearlyZero(NewValue))
	{
		FWidgetAnimationDynamicEvent EliminatedDelegate;
		EliminatedDelegate.BindDynamic(this, &ThisClass::EventOnEliminated);
		BindToAnimationFinished(OnDamaged, EliminatedDelegate);
	}
}

void UEqZeroHealthBarWidget::EventOnEliminated()
{
	PlayAnimation(OnEliminated, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
}

void UEqZeroHealthBarWidget::UpdateHealthbar()
{
    
}

void UEqZeroHealthBarWidget::SetDynamicMaterials()
{
	if (BarBorder)
	{
		BarBorderMID = BarBorder->GetDynamicMaterial();
	}

	if (BarFill)
	{
		BarFillMID = BarFill->GetDynamicMaterial();
	}

	if (BarGlow)
	{
		BarGlowMID = BarGlow->GetDynamicMaterial();
	}
}

void UEqZeroHealthBarWidget::ResetAnimatedState()
{
	for (UMaterialInstanceDynamic* MID : TArray<UMaterialInstanceDynamic*>{ BarBorderMID, BarFillMID, BarGlowMID })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Animate_Damage"),     0.0f);
			MID->SetScalarParameterValue(TEXT("Animate_DamageFade"), 0.0f);
		}
	}

	for (UMaterialInstanceDynamic* MID : TArray<UMaterialInstanceDynamic*>{ BarBorderMID, BarGlowMID })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Animate_Glow_AlphaChange"), 0.0f);
			MID->SetScalarParameterValue(TEXT("Animate_Glow_ColorChange"), 0.0f);
		}
	}
}

void UEqZeroHealthBarWidget::InitializeBarVisuals()
{
	NormalizedHealth = 1.0f;

	for (UMaterialInstanceDynamic* MID : TArray<UMaterialInstanceDynamic*>{ BarBorderMID, BarFillMID, BarGlowMID })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Health_Current"), NormalizedHealth);
			MID->SetScalarParameterValue(TEXT("Health_Updated"), NormalizedHealth);
		}
	}

	if (HealthNumber)
	{
		HealthNumber->InterpolateToValue(NormalizedHealth * 100.0f, 1.0f, 4.0f, 0.0f);
	}

	ResetAnimatedState();
}
