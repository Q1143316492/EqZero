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
		// 这是配置，默认True
		// 如果标记要贴在圆周外侧，那么半径需要加上标记图片尺寸的一半，才能让标记的边缘贴在圆周上。
		// 形象的理解就是准星向外推，圆和准星正方形的交接不是准星中心点，而是准星边缘。
		XRadius += MarkerBrush->ImageSize.X * 0.5f;
		YRadius += MarkerBrush->ImageSize.X * 0.5f;
	}

	// 步骤 2：角度转弧度

	// 图片自身配置的旋转
	const float LocalRotationRadians = FMath::DegreesToRadians(Marker.ImageRotationAngle);

	/*
	 * 图片 极坐标 下的旋转
	 *          0
	 *
	 * -90              90
	 *
	 *         180
	 */
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

			// ====== 6.5 测试绘制 ======
			// 可调参数 —— 改这些值来观察绘制变化
			// const float TestPositionAngle  = 30.0f;    // 圆周位置角度（度）：0=正上方, 90=右, 180=下, 270=左
			// const float TestRotationAngle  = 45.0f;    // 图片自身旋转角度（度）：正值=顺时针
			// const float TestOffsetX        = 32.0f;    // 额外 X 偏移（像素）：正值=向右
			// const float TestOffsetY        = 64.0f;    // 额外 Y 偏移（像素）：正值=向下
			// const float TestScale          = 1.0f;    // 图片缩放倍率

			// 测试图片是 立着的 长方形
			// --- A: 在控件正中心画一个不旋转、不偏移的 Marker（基准参照） ---
			// {
			// 	const FVector2D ScaledSize = MarkerBrush->ImageSize * TestScale;
			// 	// 图片左上角 = 中心 - 半尺寸，使图片中心对齐控件中心
			// 	const FVector2D TopLeft = LocalCenter - ScaledSize * 0.5f;

			// 	const FPaintGeometry CenterGeo = AllottedGeometry.ToPaintGeometry(
			// 		ScaledSize,                          // 绘制尺寸
			// 		FSlateLayoutTransform(TopLeft)        // 布局偏移（左上角位置）
			// 	);
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, CenterGeo, MarkerBrush, DrawEffects,
			// 		FLinearColor(0.0f, 0.5f, 1.0f, 0.8f)); // 半透明蓝色，作为参照
			// }

			// --- B: 只做平移，不旋转 —— 观察 TestOffsetX / TestOffsetY 的效果 ---
			// {
			// 	const FVector2D ScaledSize = MarkerBrush->ImageSize * TestScale;
			// 	const FVector2D TopLeft = FVector2D(TestOffsetX, TestOffsetY);

			// 	const FPaintGeometry OffsetGeo = AllottedGeometry.ToPaintGeometry(
			// 		ScaledSize,
			// 		FSlateLayoutTransform(TopLeft)
			// 	);
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, OffsetGeo, MarkerBrush, DrawEffects,
			// 		FLinearColor(0.0f, 1.0f, 0.0f, 0.6f)); // 半透明绿色
			// }

			// --- C: 放到圆周上 —— 观察 TestPositionAngle + BaseRadius 的效果 ---
			// {
			// 	const float PosRad = FMath::DegreesToRadians(TestPositionAngle);
			// 	float TestRadius = 64.f; // BaseRadius;
			// 	// const FVector2D CircleOffset(
			// 	// 	TestRadius * FMath::Sin(PosRad) * ApplicationScale,
			// 	// 	-TestRadius * FMath::Cos(PosRad) * ApplicationScale
			// 	// );

			// 	// 数学课学的极坐标试试, 也是OK的，理解不同，位置不同，配置不同，但都是对的
			// 	const FVector2D CircleOffset(
			// 		TestRadius * FMath::Cos(PosRad) * ApplicationScale,
			// 		-TestRadius * FMath::Sin(PosRad) * ApplicationScale
			// 	);

			// 	const FVector2D ScaledSize = MarkerBrush->ImageSize * TestScale;
			// 	const FVector2D TopLeft = CircleOffset; //LocalCenter - ScaledSize * 0.5f + CircleOffset;

			// 	const FPaintGeometry CircleGeo = AllottedGeometry.ToPaintGeometry(
			// 		ScaledSize,
			// 		FSlateLayoutTransform(TopLeft)
			// 	);
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, CircleGeo, MarkerBrush, DrawEffects,
			// 		FLinearColor(1.0f, 1.0f, 0.0f, 0.8f)); // 半透明黄色
			// }

			// --- D: 纯自身旋转演示 —— 只关注旋转，不做圆周偏移 ---
			//
			// ═══════════════════════════════════════════════════════
			// 核心问题：Slate 的 FSlateRenderTransform 旋转基准点是 (0,0) 即图片左上角
			//          如果直接旋转，图片会绕左上角转，效果很奇怪
			//
			// 解决办法：用 Concatenate 把三步合成一个变换矩阵
			//   步骤①  T(-W/2, -H/2)    把图片中心移到原点
			//   步骤②  R(θ)              绕原点旋转
			//   步骤③  T(+W/2, +H/2)    移回去
			//
			// ═══════════════════════════════════════════════════════
			//
			// 【相关接口解析】
			//
			// FSlateRenderTransform:
			//   Slate 的 2D 渲染变换，本质是 2x2矩阵 + 平移向量
			//   支持旋转、缩放、平移、错切
			//
			// FQuat2D(弧度):
			//   2D 旋转"四元数"（其实就是 2x2 旋转矩阵的封装）
			//   正值 = 顺时针（因为 Slate Y轴朝下）
			//
			// Concatenate(A, B, C...):
			//   把多个变换按顺序串联：先执行 A，再执行 B，再执行 C
			//   FVector2D 在这里被当作"平移变换"
			//   FQuat2D 被当作"旋转变换"
			//   所以 Concatenate(平移, 旋转, 平移) 就是三步合一
			//
			// ToPaintGeometry(Size, LayoutTransform, RenderTransform, Pivot):
			//   4参数版本：
			//   - Size:            绘制尺寸（像素）
			//   - LayoutTransform: 布局偏移（确定图片左上角在控件中的位置）
			//   - RenderTransform: 渲染变换（在布局之后额外施加的变换）
			//   - Pivot:           RenderTransform 的基准点（归一化 0~1）
			//                      (0,0)=左上角  (0.5,0.5)=中心
			//
			// ═══════════════════════════════════════════════════════

			// --- D2: 正确做法 —— 手动 Concatenate 绕中心旋转 ---
			// {
			// 	const float RotRad = FMath::DegreesToRadians(-45.f); // 负值 = 逆时针旋转作为对比
			// 	const FVector2D HalfSize = MarkerBrush->ImageSize * 0.5f;

			// 	//   Concatenate 执行顺序：从左到右
			// 	//   ① FVector2D(-HalfSize)  →  平移：把图片中心挪到左上角原点
			// 	//   ② FQuat2D(RotRad)        →  旋转：绕原点（此时就是图片中心）旋转
			// 	//   ③ FVector2D(+HalfSize)   →  平移：挪回去
			// 	FSlateRenderTransform RotateAroundCenter(Concatenate(
			// 		FVector2D(-HalfSize.X, -HalfSize.Y),  // ① 移到原点
			// 		FQuat2D(RotRad),                        // ② 旋转
			// 		FVector2D(HalfSize.X, HalfSize.Y)      // ③ 移回来
			// 	));

			// 	const FPaintGeometry CorrectGeo(AllottedGeometry.ToPaintGeometry(
			// 		MarkerBrush->ImageSize,
			// 		FSlateLayoutTransform(LocalCenter - HalfSize),  // 布局：图片中心对齐控件中心
			// 		RotateAroundCenter,                              // 渲染变换：绕自身中心旋转
			// 		FVector2D(0.0f, 0.0f)                            // 必须是 (0,0)，Concatenate 要求，如果有pivot和 FSlateRenderTransform 第一步第三步重复了。
			// 	));
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, CorrectGeo, MarkerBrush, DrawEffects,
			// 		FLinearColor(1.0f, 0.2f, 0.2f, 1.0f)); // 不透明红色 —— 绕中心旋转，位置稳定
			// }

			// --- D3: 更简单的方式 —— 用 Pivot 参数代替手动 Concatenate ---
			// {
			// 	const float RotRad = FMath::DegreesToRadians(TestRotationAngle);

			// 	// 直接用 FQuat2D，但把 Pivot 设成 (0.5, 0.5) = 图片中心
			// 	// 引擎会自动帮你做"移到中心→旋转→移回"
			// 	FSlateRenderTransform SimpleRotate{FQuat2D(RotRad)};

			// 	const FVector2D HalfSize = MarkerBrush->ImageSize * 0.5f;
			// 	const FVector2D TopLeft = LocalCenter - HalfSize;
			// 	const FPaintGeometry SimpleGeo(AllottedGeometry.ToPaintGeometry(
			// 		MarkerBrush->ImageSize,
			// 		FSlateLayoutTransform(TopLeft),
			// 		SimpleRotate,
			// 		FVector2D(0.5f, 0.5f)  // ← Pivot = 图片中心！引擎自动绕这个点旋转
			// 	));
			// 	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, SimpleGeo, MarkerBrush, DrawEffects,
			// 		FLinearColor(0.2f, 1.0f, 0.2f, 0.6f)); // 半透明绿色 —— 效果和 D2 红色完全重合
			// }
			// ====== 测试绘制结束 ======

			// 7. 遍历每一个标记，逐个绘制
			for (const FEqZeroCircumferenceMarkerEntry& Marker : MarkerList)
			{
				// 8. 计算该标记的变换矩阵
				//    包含：图片自旋 + 平移到圆周上的位置
				const FSlateRenderTransform MarkerTransform = GetMarkerRenderTransform(Marker, BaseRadius, ApplicationScale);

				// 9. 构建绘制几何体
				//    - 大小 = MarkerBrush->ImageSize（图片像素尺寸）
				//    - 布局偏移 = LocalCenter - ImageSize*0.5  → 把图片中心对齐到控件中心
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
