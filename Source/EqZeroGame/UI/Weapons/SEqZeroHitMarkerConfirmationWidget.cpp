// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Weapons/SEqZeroHitMarkerConfirmationWidget.h"
#include "Weapons/EqZeroWeaponStateComponent.h"
#include "Styling/SlateBrush.h"

class FPaintArgs;
class FSlateRect;

SEqZeroHitMarkerConfirmationWidget::SEqZeroHitMarkerConfirmationWidget()
{
}

void SEqZeroHitMarkerConfirmationWidget::Construct(const FArguments& InArgs, const FLocalPlayerContext& InContext, const TMap<FGameplayTag, FSlateBrush>& ZoneOverrideImages)
{
	PerHitMarkerImage = InArgs._PerHitMarkerImage;
	PerHitMarkerZoneOverrideImages = ZoneOverrideImages;
	AnyHitsMarkerImage = InArgs._AnyHitsMarkerImage;
	bColorAndOpacitySet = InArgs._ColorAndOpacity.IsSet();
	ColorAndOpacity = InArgs._ColorAndOpacity;

	MyContext = InContext;
}

int32 SEqZeroHitMarkerConfirmationWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 是否启用（父控件禁用则自身也禁用）
	const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
	// 计算控件本地空间的中心点
	// GetLocalPositionAtCoordinates(0.5, 0.5) = 控件矩形的正中心
	const FVector2D LocalCenter = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));

	// 根据当前的击中通知不透明度决定是否绘制标记
	const bool bDrawMarkers = (HitNotifyOpacity > KINDA_SMALL_NUMBER);
	
	if (bDrawMarkers)
	{
		
		TArray<FEqZeroScreenSpaceHitLocation> LastWeaponDamageScreenLocations;
		if (APlayerController* PC = MyContext.IsInitialized() ? MyContext.GetPlayerController() : nullptr)
		{
			if (UEqZeroWeaponStateComponent* WeaponStateComponent = PC->FindComponentByClass<UEqZeroWeaponStateComponent>())
			{
				WeaponStateComponent->GetLastWeaponDamageScreenLocations(LastWeaponDamageScreenLocations);
			}
		}

		if ((LastWeaponDamageScreenLocations.Num() > 0) && (PerHitMarkerImage != nullptr))
		{
			const FVector2D HalfAbsoluteSize = AllottedGeometry.GetAbsoluteSize() * 0.5f;

			for (const FEqZeroScreenSpaceHitLocation& Hit : LastWeaponDamageScreenLocations)
			{
				// 1. 查是否有这个 HitZone 的专属图片（如爆头），没有则用默认图
				const FSlateBrush* LocationMarkerImage = PerHitMarkerZoneOverrideImages.Find(Hit.HitZone);
				if (LocationMarkerImage == nullptr)
				{
					LocationMarkerImage = PerHitMarkerImage;
				}

				// 2. 颜色 × HitNotifyOpacity（实现淡出效果）
				FLinearColor MarkerColor = bColorAndOpacitySet ?
					ColorAndOpacity.Get().GetColor(InWidgetStyle) :
					(InWidgetStyle.GetColorAndOpacityTint() * LocationMarkerImage->GetTint(InWidgetStyle));
				MarkerColor.A *= HitNotifyOpacity;

				// 3. 关键坐标转换：窗口屏幕坐标 → 控件本地坐标
				//    Hit.Location 是视口坐标，加上 MyCullingRect.TopLeft 转为窗口坐标
				//    再用 AbsoluteToLocal 转为控件内部坐标
				const FVector2D WindowSSLocation = Hit.Location + MyCullingRect.GetTopLeft(); // 在非全屏模式下考虑窗口装饰
				const FSlateRenderTransform DrawPos(AllottedGeometry.AbsoluteToLocal(WindowSSLocation));

				// 图片中心对齐到命中点
				const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(LocationMarkerImage->ImageSize, FSlateLayoutTransform(-(LocationMarkerImage->ImageSize * 0.5f)), DrawPos));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, LocationMarkerImage, DrawEffects, MarkerColor);
			}
		}
		
		if (AnyHitsMarkerImage != nullptr)
		{
			FLinearColor MarkerColor = bColorAndOpacitySet ?
				ColorAndOpacity.Get().GetColor(InWidgetStyle) :
				(InWidgetStyle.GetColorAndOpacityTint() * AnyHitsMarkerImage->GetTint(InWidgetStyle));
			MarkerColor.A *= HitNotifyOpacity;

			// 否则在十字准线的中心显示命中通知
			const FPaintGeometry Geometry(
				AllottedGeometry.ToPaintGeometry(
					AnyHitsMarkerImage->ImageSize,
					FSlateLayoutTransform(LocalCenter - (AnyHitsMarkerImage->ImageSize * 0.5f))));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, AnyHitsMarkerImage, DrawEffects, MarkerColor);
		}
	}

	return LayerId;
}

FVector2D SEqZeroHitMarkerConfirmationWidget::ComputeDesiredSize(float) const
{
	return FVector2D(100.0f, 100.0f);
}

void SEqZeroHitMarkerConfirmationWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	HitNotifyOpacity = 0.0f;

	if (APlayerController* PC = MyContext.IsInitialized() ? MyContext.GetPlayerController() : nullptr)
	{
		if (UEqZeroWeaponStateComponent* DamageMarkerComponent = PC->FindComponentByClass<UEqZeroWeaponStateComponent>())
		{
			const double TimeSinceLastHitNotification = DamageMarkerComponent->GetTimeSinceLastHitNotification();
			if (TimeSinceLastHitNotification < HitNotifyDuration)
			{
				HitNotifyOpacity = FMath::Clamp(1.0f - (float)(TimeSinceLastHitNotification / HitNotifyDuration), 0.0f, 1.0f);
			}
		}
	}
}
