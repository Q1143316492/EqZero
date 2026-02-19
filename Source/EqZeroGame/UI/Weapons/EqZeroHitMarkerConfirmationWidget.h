// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "GameplayTagContainer.h"

#include "EqZeroHitMarkerConfirmationWidget.generated.h"

class SEqZeroHitMarkerConfirmationWidget;
class SWidget;
class UObject;
struct FGameplayTag;

/*
 * 这个画的是击中效果那个红色的叉
 * 
 */
UCLASS()
class UEqZeroHitMarkerConfirmationWidget : public UWidget
{
	GENERATED_BODY()

public:
	UEqZeroHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer);

	/*
	 * UWidget interface
	 */
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	/*
	 * UVisual interface
	 */
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// ==========================================================================
	
public:
	// 显示击中通知的持续时间（以秒为单位）（它们会在这段时间内逐渐变为透明）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(ClampMin=0.0, ForceUnits=s))
	float HitNotifyDuration = 0.4f;

	// 用于绘制单个命中标记的标记图像
	// 建议看蓝图，一个红色的菱形
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush PerHitMarkerImage;

	// 这是一个 TAG=>图片。例如爆头识别到这个TAG，就显示这个图片盖上去
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TMap<FGameplayTag, FSlateBrush> PerHitMarkerZoneOverrideImages;

	// 如果存在任何命中结果时要绘制的标记图像。
	// 常规的命中红叉
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FSlateBrush AnyHitsMarkerImage;

private:
	TSharedPtr<SEqZeroHitMarkerConfirmationWidget> MyMarkerWidget;
};
