// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"

#include "EqZeroActivatableWidget.generated.h"

struct FUIInputConfig;

UENUM(BlueprintType)
enum class EEqZeroWidgetInputMode : uint8
{
	Default,
	GameAndMenu,
	Game,
	Menu
};

// 基本上所有 UI 都要继承这个类了
// 项目的层级EEqZeroWidgetInputMode的输入模式，到CommonUI的输入模式做一个映射
UCLASS(Abstract, Blueprintable)
class UEqZeroActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UEqZeroActivatableWidget(const FObjectInitializer& ObjectInitializer);

public:
	//~UCommonActivatableWidget interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~End of UCommonActivatableWidget interface

#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EEqZeroWidgetInputMode InputConfig = EEqZeroWidgetInputMode::Default;

	UPROPERTY(EditDefaultsOnly, Category = Input)
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
