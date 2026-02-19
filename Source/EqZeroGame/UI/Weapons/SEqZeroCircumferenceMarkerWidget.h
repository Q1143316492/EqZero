// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/CoreStyle.h"
#include "Widgets/Accessibility/SlateWidgetAccessibleTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

#include "SEqZeroCircumferenceMarkerWidget.generated.h"

class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;
struct FGeometry;
struct FSlateBrush;

USTRUCT(BlueprintType)
struct FEqZeroCircumferenceMarkerEntry
{
	GENERATED_BODY()

	
	/*
	 * 极坐标
	 *          0
	 *
	 * -90              90
	 *
	 *         180
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ForceUnits=deg))
	float PositionAngle = 0.0f;

	// 图片自己的旋转
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ForceUnits=deg))
	float ImageRotationAngle = 0.0f;
};

/**
 * 这个画的是准星，上下左右四个正方形，中间镂空。
 *    |
 * --   --
 *    |
 */
class SEqZeroCircumferenceMarkerWidget : public SLeafWidget
{
	SLATE_BEGIN_ARGS(SEqZeroCircumferenceMarkerWidget)
		: _MarkerBrush(FCoreStyle::Get().GetBrush("Throbber.CircleChunk"))
		, _Radius(48.0f)
	{
	}
		// 准星 上下左右四个正方形，的图片
		SLATE_ARGUMENT(const FSlateBrush*, MarkerBrush)

        // 标记这个 上下左右四个正方形的信息，极坐标位置和图片旋转角度
		SLATE_ARGUMENT(TArray<FEqZeroCircumferenceMarkerEntry>, MarkerList)
		
        // 圆心到标记中心的距离，支持动态绑定
		SLATE_ATTRIBUTE(float, Radius)
        
		// 标记颜色，支持动态绑定
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	SEqZeroCircumferenceMarkerWidget();

	//~SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override { return true; }
	//~End of SWidget interface

	void SetRadius(float NewRadius);
	void SetMarkerList(TArray<FEqZeroCircumferenceMarkerEntry>& NewMarkerList);

private:
	FSlateRenderTransform GetMarkerRenderTransform(const FEqZeroCircumferenceMarkerEntry& Marker, const float BaseRadius, const float HUDScale) const;

private:
	// 准星 上下左右四个正方形，的图片
	const FSlateBrush* MarkerBrush;

	// 标记这个 上下左右四个正方形的信息，极坐标位置和图片旋转角度
	TArray<FEqZeroCircumferenceMarkerEntry> MarkerList;

	// 圆心到标记中心的距离
	TAttribute<float> Radius;

	// 标记颜色
	TAttribute<FSlateColor> ColorAndOpacity;

	// 判断颜色是否手动设置，没设置则继承父控件的颜色调色
	bool bColorAndOpacitySet;

	// 控制标记是贴在圆周内侧还是外侧（图片尺寸的一半作为偏移）
	uint8 bReticleCornerOutsideSpreadRadius : 1;
};
