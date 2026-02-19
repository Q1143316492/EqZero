// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/LocalPlayer.h"
#include "GameplayTagContainer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Accessibility/SlateWidgetAccessibleTypes.h"
#include "Widgets/SLeafWidget.h"

class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;
struct FGameplayTag;
struct FGeometry;
struct FSlateBrush;

class SEqZeroHitMarkerConfirmationWidget : public SLeafWidget
{
	SLATE_BEGIN_ARGS(SEqZeroHitMarkerConfirmationWidget)
		: _PerHitMarkerImage(FCoreStyle::Get().GetBrush("Throbber.CircleChunk"))
		, _AnyHitsMarkerImage(nullptr)
		, _HitNotifyDuration(0.4f)
	{
	}
		// 用于单个命中标记的要绘制的标记图像，一个中间的红色菱形
		SLATE_ARGUMENT(const FSlateBrush*, PerHitMarkerImage)
		
		// 命中都会显示的图案，大红叉
		SLATE_ARGUMENT(const FSlateBrush*, AnyHitsMarkerImage)
		
		// 显示击中通知的持续时间（以秒为单位）（它们会在这段时间内逐渐变为透明）
		SLATE_ATTRIBUTE(float, HitNotifyDuration)
		
		// 标记笔的颜色和不透明度
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const FLocalPlayerContext& InContext, const TMap<FGameplayTag, FSlateBrush>& ZoneOverrideImages);

	SEqZeroHitMarkerConfirmationWidget();

	//~SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override { return true; }
	//~End of SWidget interface

private:
	// 用于绘制单个命中标记的标记图像
	const FSlateBrush* PerHitMarkerImage = nullptr;

	// 这是一个 TAG=>图片。例如爆头识别到这个TAG，就显示这个图片盖上去
	TMap<FGameplayTag, FSlateBrush> PerHitMarkerZoneOverrideImages;

	// 如果存在任何命中结果时要绘制的标记图像
	const FSlateBrush* AnyHitsMarkerImage = nullptr;

	// 命中标记的不透明度
	float HitNotifyOpacity = 0.0f;

	// 显示击中通知的持续时间（以秒为单位）（它们会在这段时间内逐渐变为透明）
	float HitNotifyDuration = 0.4f;

	// 标记笔的颜色和不透明度
	TAttribute<FSlateColor> ColorAndOpacity;
	
	bool bColorAndOpacitySet;

	// 拥有 HUD 的玩家上下文
	FLocalPlayerContext MyContext;
};
