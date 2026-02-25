# Slate OnPaint 绘制系统详解

## 目录

1. [Slate 渲染管线概览](#1-slate-渲染管线概览)
2. [SWidget 核心虚函数](#2-swidget-核心虚函数)
3. [OnPaint 函数签名逐参数解析](#3-onpaint-函数签名逐参数解析)
4. [FGeometry：坐标系与空间转换](#4-fgeometry坐标系与空间转换)
5. [FSlateDrawElement：绘制指令](#5-fslatedrawelement绘制指令)
6. [FSlateBrush：图片/材质资源描述](#6-fslatebrush图片材质资源描述)
7. [LayerId 与绘制顺序](#7-layerid-与绘制顺序)
8. [一个最简单的 OnPaint 示例](#8-一个最简单的-onpaint-示例)
9. [进阶示例：带动画的自定义控件](#9-进阶示例带动画的自定义控件)
10. [项目中的 HitMarker 代码逐行解读](#10-项目中的-hitmarker-代码逐行解读)
11. [常见陷阱与最佳实践](#11-常见陷阱与最佳实践)

---

## 1. Slate 渲染管线概览

Slate 是 UE 的即时模式（Immediate Mode）UI 框架。每一帧的渲染流程：

```
SWidget::Paint()          // 引擎内部调用入口
  ├─ OnPaint()            // 你的绘制逻辑（虚函数，可重写）
  │    ├─ FSlateDrawElement::MakeBox / MakeLine / MakeText ...
  │    │    → 向 FSlateWindowElementList 中添加绘制指令
  │    └─ return LayerId  // 返回当前使用的最高层级
  └─ ArrangeChildren()    // SPanel 等容器会递归绘制子控件
```

关键点：
- **OnPaint 不直接调用 GPU**，而是生成一系列 `FSlateDrawElement` 绘制指令，由 Slate Renderer 在帧末统一提交到 GPU。
- 控件树自上而下递归调用 `Paint()`，父控件负责调用子控件的 `Paint()`。
- `SLeafWidget`（叶子控件）没有子控件，只需重写 `OnPaint` 即可。
- `SPanel`/`SCompoundWidget` 需要在 `OnPaint` 中调用 `ArrangeChildren` 或手动绘制子控件。

---

## 2. SWidget 核心虚函数

| 虚函数 | 作用 | 何时重写 |
|--------|------|----------|
| `OnPaint()` | 自定义绘制逻辑 | 需要自绘内容时（本文重点） |
| `ComputeDesiredSize()` | 返回控件期望的大小 | 自定义控件必须重写 |
| `Tick()` | 每帧逻辑更新 | 需要动画/状态更新时 |
| `OnArrangeChildren()` | 布局子控件 | SPanel 子类必须重写 |
| `ComputeVolatility()` | 是否每帧都需要重绘 | 动态内容返回 true |

继承关系选择：
```
SWidget
  ├─ SLeafWidget           ← 无子控件，纯自绘（如本项目的 HitMarker）
  ├─ SCompoundWidget        ← 有单个子控件槽位
  └─ SPanel                 ← 有多个子控件
```

---

## 3. OnPaint 函数签名逐参数解析

```cpp
virtual int32 OnPaint(
    const FPaintArgs& Args,                    // 绘制参数（父控件传递的上下文）
    const FGeometry& AllottedGeometry,         // ★ 控件在屏幕上的几何信息
    const FSlateRect& MyCullingRect,           // 裁剪矩形（超出部分不绘制）
    FSlateWindowElementList& OutDrawElements,  // ★ 输出：绘制指令列表
    int32 LayerId,                             // 当前绘制层级
    const FWidgetStyle& InWidgetStyle,         // 继承的样式（颜色、透明度）
    bool bParentEnabled                        // 父控件是否启用
) const;
```

### 各参数详解

#### `FPaintArgs& Args`
- 包含父到子传递的绘制上下文（如父控件的绘制信息）。
- 大多数情况下你不需要直接使用它，除非要绘制子控件时传递给它们。

#### `FGeometry& AllottedGeometry` ★ 最重要
- 描述了**本控件在屏幕上的位置、大小和变换**。
- 常用方法：
  ```cpp
  AllottedGeometry.GetLocalSize()                    // 控件本地尺寸 FVector2D
  AllottedGeometry.GetAbsoluteSize()                 // 屏幕上的实际像素尺寸
  AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f))  // 本地空间中心点
  AllottedGeometry.AbsoluteToLocal(AbsPos)           // 绝对坐标 → 本地坐标
  AllottedGeometry.LocalToAbsolute(LocalPos)         // 本地坐标 → 绝对坐标
  AllottedGeometry.ToPaintGeometry(Size, LayoutTransform)  // 生成绘制用几何体
  ```

#### `FSlateRect& MyCullingRect`
- 当前的可见裁剪区域（绝对坐标）。
- 超出这个矩形的内容不需要绘制，引擎会自动裁剪。
- 可用 `MyCullingRect.GetTopLeft()` 获取裁剪区的左上角位置（窗口偏移）。

#### `FSlateWindowElementList& OutDrawElements` ★ 绘制输出
- 这是一个绘制指令的收集器——你调用 `FSlateDrawElement::MakeXxx()` 时，指令就添加到这里。
- 最终由 Slate Renderer 统一渲染。

#### `int32 LayerId`
- 层级号，决定绘制先后（数值越大越靠前）。
- 如果你的控件只画一层，直接返回 `LayerId` 即可。
- 如果需要分层（如背景+前景），可以 `LayerId + 1` 区分。

#### `FWidgetStyle& InWidgetStyle`
- 从父控件继承下来的样式：
  ```cpp
  InWidgetStyle.GetColorAndOpacityTint()  // 继承的颜色/透明度
  ```

#### `bool bParentEnabled`
- 父控件是否处于启用状态。通常配合 `ShouldBeEnabled(bParentEnabled)` 使用：
  ```cpp
  const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
  const ESlateDrawEffect DrawEffects = bIsEnabled 
      ? ESlateDrawEffect::None 
      : ESlateDrawEffect::DisabledEffect;
  ```

#### 返回值 `int32`
- 返回你使用的最高 LayerId。如果只用了传入的 LayerId，就直接 `return LayerId`。

---

## 4. FGeometry：坐标系与空间转换

Slate 中有三个坐标系：

```
┌─────────────────────────────────────┐
│ 屏幕绝对坐标 (Absolute)             │  ← 整个显示器/窗口的像素坐标
│  ┌────────────────────────────┐     │
│  │ 控件本地坐标 (Local)       │     │  ← 控件自身的 (0,0) 到 (Width, Height)
│  │   ┌──────────┐            │     │
│  │   │绘制坐标   │            │     │  ← ToPaintGeometry 产生的坐标
│  │   └──────────┘            │     │
│  └────────────────────────────┘     │
└─────────────────────────────────────┘
```

核心转换：
```cpp
// 绝对 → 本地
FVector2D LocalPos = Geometry.AbsoluteToLocal(AbsolutePos);

// 本地 → 绝对
FVector2D AbsPos = Geometry.LocalToAbsolute(LocalPos);

// 获取控件中心（本地坐标）
FVector2D Center = Geometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));

// 创建绘制用几何体（在控件内某个位置画一张图）
FPaintGeometry PaintGeo = Geometry.ToPaintGeometry(
    ImageSize,              // 绘制尺寸
    FSlateLayoutTransform(Offset)  // 在本地空间内的偏移
);
```

---

## 5. FSlateDrawElement：绘制指令

这是你在 OnPaint 中实际"画东西"的 API：

| 方法 | 用途 |
|------|------|
| `MakeBox()` | 绘制矩形/图片（最常用） |
| `MakeText()` | 绘制文本 |
| `MakeLine()` / `MakeLines()` | 绘制线段 |
| `MakeSpline()` | 绘制样条曲线 |
| `MakeGradient()` | 绘制渐变 |
| `MakeCustom()` | 自定义绘制（传入 FSlateRenderTransform） |

### MakeBox 详解（最常用）

```cpp
FSlateDrawElement::MakeBox(
    OutDrawElements,       // 绘制指令列表
    LayerId,               // 层级
    PaintGeometry,         // 在哪里画，画多大
    &SlateBrush,           // 用什么图片/材质
    DrawEffects,           // 绘制效果（正常 / 禁用灰度）
    Tint                   // 颜色着色 FLinearColor
);
```

### MakeText 示例

```cpp
FSlateDrawElement::MakeText(
    OutDrawElements,
    LayerId,
    PaintGeometry,
    Text,                  // FString
    FontInfo,              // FSlateFontInfo
    DrawEffects,
    Color                  // FLinearColor
);
```

### MakeLines 示例

```cpp
TArray<FVector2F> Points = { {0,0}, {100,50}, {200,0} };
FSlateDrawElement::MakeLines(
    OutDrawElements,
    LayerId,
    PaintGeometry,
    Points,
    DrawEffects,
    Color,
    bAntialias,            // 是否抗锯齿
    Thickness              // 线宽
);
```

---

## 6. FSlateBrush：图片/材质资源描述

`FSlateBrush` 描述了一个可绘制的图像资源：

```cpp
FSlateBrush MyBrush;
MyBrush.SetResourceObject(MyTexture);        // 设置 UTexture2D
MyBrush.ImageSize = FVector2D(64.0f, 64.0f); // 原始图片尺寸
MyBrush.TintColor = FSlateColor(FLinearColor::White);
MyBrush.DrawAs = ESlateBrushDrawType::Image; // Image / Box / Border / NoDrawType
```

获取 Brush 的常用方式：
```cpp
// 从 Style 获取
FCoreStyle::Get().GetBrush("Throbber.CircleChunk")

// 从 FSlateStyleSet 获取
FAppStyle::Get().GetBrush("WhiteBrush")

// 自定义创建（代码中）
static FSlateBrush CustomBrush;
// 设置属性...
```

---

## 7. LayerId 与绘制顺序

```cpp
int32 OnPaint(..., int32 LayerId, ...) const
{
    // 第一层：绘制背景
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, ...);
    
    // 第二层：绘制前景（LayerId + 1 确保在背景之上）
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, ...);
    
    // 返回使用的最高层级
    return LayerId + 1;
}
```

规则：
- 相同 LayerId 的元素按添加顺序绘制（后添加的在上面）。
- 不同 LayerId 的元素按数值大小绘制（大的在上面）。
- 子控件通常使用父控件返回的 LayerId + 1。

---

## 8. 一个最简单的 OnPaint 示例

### 目标：画一个红色矩形 + 居中白色文字

#### .h 文件

```cpp
#pragma once

#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Fonts/SlateFontInfo.h"

/**
 * 最简单的自绘 Slate 控件示例。
 * 继承 SLeafWidget（无子控件的叶子节点）。
 */
class SMySimplePaintWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMySimplePaintWidget)
        : _Text(FText::FromString(TEXT("Hello Slate!")))
        , _BackgroundColor(FLinearColor::Red)
    {}
        /** 要显示的文本 */
        SLATE_ATTRIBUTE(FText, Text)

        /** 背景颜色 */
        SLATE_ARGUMENT(FLinearColor, BackgroundColor)
    SLATE_END_ARGS()

    /** Slate 控件的构造函数（不是 C++ 构造函数） */
    void Construct(const FArguments& InArgs);

    //~ SWidget interface
    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled
    ) const override;

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    //~ End SWidget interface

private:
    TAttribute<FText> DisplayText;
    FLinearColor BackgroundColor;
    FSlateFontInfo FontInfo;
};
```

#### .cpp 文件

```cpp
#include "SMySimplePaintWidget.h"
#include "Rendering/DrawElements.h"     // FSlateDrawElement
#include "Styling/CoreStyle.h"          // FCoreStyle

void SMySimplePaintWidget::Construct(const FArguments& InArgs)
{
    DisplayText = InArgs._Text;
    BackgroundColor = InArgs._BackgroundColor;

    // 设置字体（使用引擎默认字体，大小 16）
    FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 16);
}

int32 SMySimplePaintWidget::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    // ===== Step 1: 判断启用状态 =====
    const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
    const ESlateDrawEffect DrawEffects = bIsEnabled
        ? ESlateDrawEffect::None
        : ESlateDrawEffect::DisabledEffect;

    // ===== Step 2: 绘制背景矩形 =====
    // 使用引擎内置的白色 Brush，通过 Tint 颜色来改变显示
    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

    // ToPaintGeometry() 无参版本 = 填满整个控件区域
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),  // 铺满整个控件
        WhiteBrush,
        DrawEffects,
        BackgroundColor   // 着色为我们想要的背景色
    );

    // ===== Step 3: 绘制居中文字 =====
    const FString TextStr = DisplayText.Get().ToString();
    
    // 测量文本尺寸，用于居中对齐
    const FVector2D TextSize = FSlateApplication::Get()
        .GetRenderer()->GetFontMeasureService()
        ->Measure(TextStr, FontInfo);
    
    // 计算居中偏移
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D TextOffset = (LocalSize - TextSize) * 0.5f;

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 1,   // 文字在背景之上
        AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(TextOffset)),
        TextStr,
        FontInfo,
        DrawEffects,
        FLinearColor::White
    );

    return LayerId + 1;
}

FVector2D SMySimplePaintWidget::ComputeDesiredSize(float) const
{
    // 期望的最小尺寸（布局系统会参考这个值）
    return FVector2D(200.0f, 50.0f);
}
```

#### 使用方式（在 UMG 或其他 Slate 容器中）

```cpp
// 创建控件
TSharedRef<SMySimplePaintWidget> MyWidget = 
    SNew(SMySimplePaintWidget)
    .Text(FText::FromString(TEXT("Hit!")))
    .BackgroundColor(FLinearColor(0.8f, 0.1f, 0.1f, 1.0f));

// 添加到某个容器，例如 SOverlay
MyOverlay->AddSlot()
[
    MyWidget
];
```

---

## 9. 进阶示例：带动画的自定义控件

这个示例展示了 `Tick()` + `OnPaint()` 协作实现脉冲动画效果：

```cpp
// ===== .h =====
class SPulsingCircle : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SPulsingCircle) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        // 标记为易变——每帧都需要重绘
        // 等价于重写 ComputeVolatility() 返回 true
        SetCanTick(true);
    }

    virtual bool ComputeVolatility() const override { return true; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100, 100); }

    virtual void Tick(const FGeometry& Geo, const double InCurrentTime, const float InDeltaTime) override
    {
        // 在 Tick 中更新动画状态（不要在 OnPaint 中做逻辑更新！）
        PulseAlpha = (FMath::Sin(InCurrentTime * 4.0) + 1.0) * 0.5; // 0~1 脉冲
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
    {
        const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
        
        // 根据脉冲值改变透明度
        FLinearColor Color = FLinearColor::Green;
        Color.A = PulseAlpha;

        // 画一个在控件中心、尺寸随脉冲变化的方块
        const FVector2D Center = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));
        const float Size = 20.0f + PulseAlpha * 30.0f;  // 20~50 像素
        const FVector2D BoxSize(Size, Size);
        
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Center - BoxSize * 0.5f)),
            Brush,
            ESlateDrawEffect::None,
            Color
        );

        return LayerId;
    }

private:
    float PulseAlpha = 0.0f;
};
```

---

## 10. 项目中的 HitMarker 代码逐行解读

以 `SEqZeroHitMarkerConfirmationWidget::OnPaint` 为例，对照上面的知识点：

```
继承关系: SLeafWidget → 纯自绘控件，无子控件

绘制流程:
 1. 判断启用状态 → ShouldBeEnabled + ESlateDrawEffect
 2. 计算中心点   → GetLocalPositionAtCoordinates(0.5, 0.5)
 3. 判断是否需要绘制 → HitNotifyOpacity > KINDA_SMALL_NUMBER
 4. 获取命中数据 → 从 WeaponStateComponent 获取屏幕空间命中位置列表
 5. 逐命中点绘制:
    a. 查找 HitZone 专属图片（如爆头图标），无则用默认图
    b. 计算颜色 × HitNotifyOpacity（淡出效果）
    c. 坐标转换: 视口绝对坐标 → 控件本地坐标
    d. 生成 PaintGeometry（图片中心对齐到命中点）
    e. MakeBox 绘制
 6. 绘制中心的通用命中标记（AnyHitsMarkerImage）

状态更新 (Tick):
 - 每帧检查距上次命中的时间
 - 计算 HitNotifyOpacity = 1 - (elapsed / duration)
 - 实现自然淡出效果
```

### 关键代码段解析

```cpp
// —— 坐标转换精华 ——
// Hit.Location 是视口坐标（相对于游戏视口左上角）
// MyCullingRect.GetTopLeft() 是窗口装饰偏移（标题栏等）
// 两者相加 = 窗口绝对坐标
const FVector2D WindowSSLocation = Hit.Location + MyCullingRect.GetTopLeft();

// AbsoluteToLocal: 将窗口绝对坐标转为控件本地坐标
// 用 FSlateRenderTransform 包装，作为绘制时的偏移
const FSlateRenderTransform DrawPos(AllottedGeometry.AbsoluteToLocal(WindowSSLocation));

// ToPaintGeometry 三参数版本:
//   参数1: ImageSize → 绘制尺寸
//   参数2: FSlateLayoutTransform(-(ImageSize * 0.5)) → 图片中心对齐（向左上偏移半个图片大小）
//   参数3: DrawPos → 绘制位置（命中点的本地坐标）
const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
    LocationMarkerImage->ImageSize,
    FSlateLayoutTransform(-(LocationMarkerImage->ImageSize * 0.5f)),
    DrawPos
));
```

---

## 11. 常见陷阱与最佳实践

### ❌ 不要在 OnPaint 中修改成员变量
`OnPaint` 是 `const` 函数。状态更新放在 `Tick()` 中。
```cpp
// 错误！OnPaint 是 const
int32 OnPaint(...) const { MyValue = 42; } // 编译错误

// 正确：在 Tick 中更新
void Tick(...) { MyValue = 42; }
```

### ❌ 不要忘记 ComputeVolatility
如果你的控件每帧都在变化（如动画），必须返回 true，否则 Slate 可能缓存旧的绘制结果：
```cpp
virtual bool ComputeVolatility() const override { return true; }
```

### ❌ 不要忘记 ComputeDesiredSize
即使你的控件大小完全由父控件决定，也要给一个合理的默认值：
```cpp
virtual FVector2D ComputeDesiredSize(float) const override
{
    return FVector2D(100.0f, 100.0f);
}
```

### ✅ 善用 ToPaintGeometry 的不同重载

```cpp
// 1. 填满整个控件
AllottedGeometry.ToPaintGeometry()

// 2. 指定大小 + 偏移
AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Offset))

// 3. 指定大小 + 布局变换 + 渲染变换（用于旋转/缩放）
AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Offset), FSlateRenderTransform(RenderPos))
```

### ✅ 颜色继承链
正确处理父控件传递的颜色/透明度：
```cpp
// 获取继承的颜色 Tint
FLinearColor FinalColor = InWidgetStyle.GetColorAndOpacityTint() * MyBrush->GetTint(InWidgetStyle);

// 或者如果控件有自定义颜色属性
FLinearColor FinalColor = MyColorAttribute.Get().GetColor(InWidgetStyle);
```

### ✅ SLATE_BEGIN_ARGS 宏速查

```cpp
SLATE_BEGIN_ARGS(SMyWidget) {}
    // SLATE_ARGUMENT: 构造时传入的简单值（不支持绑定）
    SLATE_ARGUMENT(float, Size)              // ._Size

    // SLATE_ATTRIBUTE: 支持绑定的值（TAttribute<T>，可以绑委托实时获取）
    SLATE_ATTRIBUTE(FText, Label)            // ._Label  → TAttribute<FText>

    // SLATE_EVENT: 委托/事件
    SLATE_EVENT(FOnClicked, OnClicked)       // ._OnClicked

    // SLATE_STYLE_ARGUMENT: 样式引用
    SLATE_STYLE_ARGUMENT(FButtonStyle, Style)
SLATE_END_ARGS()
```

---

## 参考资料

- UE 源码: `Runtime/SlateCore/Public/Widgets/SLeafWidget.h`
- UE 源码: `Runtime/SlateCore/Public/Rendering/DrawElements.h` (FSlateDrawElement)
- UE 源码: `Runtime/SlateCore/Public/Layout/Geometry.h` (FGeometry)
- UE 文档: [Slate Architecture](https://docs.unrealengine.com/en-US/ProgrammingAndScripting/Slate/Architecture/)
