// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Weapons/SEqZeroCircumferenceMarkerWidget.h"

#include "Engine/UserInterfaceSettings.h"
#include "Styling/SlateBrush.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SEqZeroCircumferenceMarkerWidget)

class FPaintArgs;
class FSlateRect;

SEqZeroCircumferenceMarkerWidget::SEqZeroCircumferenceMarkerWidget()
{
}

void SEqZeroCircumferenceMarkerWidget::Construct(const FArguments& InArgs)
{
	MarkerBrush = InArgs._MarkerBrush;
	MarkerList = InArgs._MarkerList;
	Radius = InArgs._Radius;
	bColorAndOpacitySet = InArgs._ColorAndOpacity.IsSet();
	ColorAndOpacity = InArgs._ColorAndOpacity;
}

FSlateRenderTransform SEqZeroCircumferenceMarkerWidget::GetMarkerRenderTransform(const FEqZeroCircumferenceMarkerEntry& Marker, const float BaseRadius, const float HUDScale) const
{
	// 步骤 1：确定实际半径
	float XRadius = BaseRadius;
	float YRadius = BaseRadius;
	if (bReticleCornerOutsideSpreadRadius)
	{
		// 图片中心在圆周外侧半个图片宽度处
		// 即图片的内边缘刚好贴住半径圆
		XRadius += MarkerBrush->ImageSize.X * 0.5f;
		YRadius += MarkerBrush->ImageSize.X * 0.5f;
	}

	// 步骤 2：角度转弧度
	const float LocalRotationRadians = FMath::DegreesToRadians(Marker.ImageRotationAngle);
	const float PositionAngleRadians = FMath::DegreesToRadians(Marker.PositionAngle);

	// 步骤 3：绕图片自身中心旋转
	// Slate 的旋转基准点默认是左上角，要绕中心旋转需要用"先移-旋-移回"技巧：
	//   T(-W/2, -H/2)  →  把图片中心平移到原点
	//   R(θ)           →  绕原点旋转
	//   T(+W/2, +H/2)  →  恢复位置
	FSlateRenderTransform RotateAboutOrigin(
		Concatenate(
			FVector2D(
				-MarkerBrush->ImageSize.X * 0.5f, -MarkerBrush->ImageSize.Y * 0.5f),
				FQuat2D(LocalRotationRadians),
				FVector2D(MarkerBrush->ImageSize.X * 0.5f, MarkerBrush->ImageSize.Y * 0.5f)
		)
	);

	// 步骤 4：平移到圆周上的目标位置
	// 极坐标 → 笛卡尔坐标（Slate Y轴朝下，所以 Y 取负才是"上方"）
	//   x = R · sin(θ)   → 0°时 sin=0，无水平偏移
	//   y = -R · cos(θ)  → 0°时 cos=1，-R 即向上
	return TransformCast<FSlateRenderTransform>(
		Concatenate(
			RotateAboutOrigin,
			FVector2D(
				XRadius * FMath::Sin(PositionAngleRadians) * HUDScale, // X 偏移
				-YRadius * FMath::Cos(PositionAngleRadians) * HUDScale // Y 偏移（负号！）
			)
		)
	);
}

int32 SEqZeroCircumferenceMarkerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 1. 是否启用（父控件禁用则自身也禁用）
	const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect; // 禁用时灰显

	// 2. 计算控件本地空间的中心点
	// GetLocalPositionAtCoordinates(0.5, 0.5) = 控件矩形的正中心
	const FVector2D LocalCenter = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f)); 

	// 3. 前置检查：MarkerList 非空 且 笔刷非空
	const bool bDrawMarkers = (MarkerList.Num() > 0) && (MarkerBrush != nullptr);

	if (bDrawMarkers == true)
	{
		// 4. 确定颜色
		// 优先使用用户手动设置的 ColorAndOpacity
		// 否则用父控件颜色调色 * 笔刷自身颜色（颜色叠加）
		const FLinearColor MarkerColor = bColorAndOpacitySet ?
			ColorAndOpacity.Get().GetColor(InWidgetStyle) :
			(InWidgetStyle.GetColorAndOpacityTint() * MarkerBrush->GetTint(InWidgetStyle));

		// 5. Alpha 为 0 时跳过绘制（性能优化）
		if (MarkerColor.A > KINDA_SMALL_NUMBER)
		{
			const float BaseRadius = Radius.Get();

			// 6. 获取 HUD 缩放系数（适配不同分辨率）
			const float ApplicationScale = GetDefault<UUserInterfaceSettings>()->ApplicationScale;

			// 7. 遍历每一个标记，逐个绘制
			for (const FEqZeroCircumferenceMarkerEntry& Marker : MarkerList)
			{
				// 8. 计算该标记的变换矩阵
				//    包含：图片自旋 + 平移到圆周上的位置
				const FSlateRenderTransform MarkerTransform = GetMarkerRenderTransform(Marker, BaseRadius, ApplicationScale);

				// 9. 构建绘制几何体
				//    - 大小 = MarkerBrush->ImageSize（图片像素尺寸）
				//    - 布局偏移 = LocalCenter - ImageSize*0.5  → 先把图片左上角对齐到控件中心
				//    - 渲染变换 = MarkerTransform             → 再做旋转+平移到圆周
				//    - Pivot = (0, 0)                         → 变换基准点是图片左上角
				const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(MarkerBrush->ImageSize,
					FSlateLayoutTransform(LocalCenter - (MarkerBrush->ImageSize * 0.5f)),
					MarkerTransform, FVector2D(0.0f, 0.0f)));

				// 10. 提交一个 Box 绘制命令（即绘制一张贴图）
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, MarkerBrush, DrawEffects, MarkerColor);
			}
		}
	}

	return LayerId; // 没有新增层级，直接返回原 LayerId
}

FVector2D SEqZeroCircumferenceMarkerWidget::ComputeDesiredSize(float) const
{
	check(MarkerBrush);
	const float SampledRadius = Radius.Get();
	return FVector2D((MarkerBrush->ImageSize.X + SampledRadius) * 2.0f, (MarkerBrush->ImageSize.Y + SampledRadius) * 2.0f);
}

void SEqZeroCircumferenceMarkerWidget::SetRadius(float NewRadius)
{
	if (Radius.IsBound() || (Radius.Get() != NewRadius))
	{
		Radius = NewRadius;
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SEqZeroCircumferenceMarkerWidget::SetMarkerList(TArray<FEqZeroCircumferenceMarkerEntry>& NewMarkerList)
{
	MarkerList = NewMarkerList;
}
