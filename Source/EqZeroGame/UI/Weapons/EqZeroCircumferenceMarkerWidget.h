// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "UI/Weapons/SEqZeroCircumferenceMarkerWidget.h"

#include "EqZeroCircumferenceMarkerWidget.generated.h"

class SWidget;
class UObject;
struct FFrame;

/**
 * 这个画的是准星，上下左右四个正方形，中间镂空。
 *    |
 * --   --
 *    |
 */
UCLASS()
class UEqZeroCircumferenceMarkerWidget : public UWidget
{
	GENERATED_BODY()

public:
	UEqZeroCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer);

	/*
	 * UWidget interface
	 */
public:
	virtual void SynchronizeProperties() override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	/*
	 * UVisual interface
	 */
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
	// ==========================================================================
	
public:
	// 绘制标记的位置 / 方向列表
	// (0, 0), (90, 90), (-90, -90), (180, 180)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TArray<FEqZeroCircumferenceMarkerEntry> MarkerList;

	// 我们画的是准星，这那个圆的半径
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(ClampMin=0.0))
	float Radius = 48.0f;

	// 我们画的是准星，上下左右四个正方形。这是其中的一个正方形的图片
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush MarkerImage;

	// 标线片角部图像是否放置在扩散半径之外
	// 改为 0-1 的浮点对齐（例如，在半径内部 / 之上 / 外部）?
	UPROPERTY(EditAnywhere, Category=Corner)
	uint8 bReticleCornerOutsideSpreadRadius : 1;

public:
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetRadius(float InRadius);

private:
	TSharedPtr<SEqZeroCircumferenceMarkerWidget> MyMarkerWidget;
};
