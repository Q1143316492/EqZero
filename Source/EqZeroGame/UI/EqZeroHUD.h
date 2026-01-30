// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "GameFramework/HUD.h"

#include "EqZeroHUD.generated.h"

namespace EEndPlayReason { enum Type : int; }

class AActor;
class UObject;

/**
 * AEqZeroHUD
 * 这个项目中 HUD 主要用来调试
 * UI 的逻辑你应该从 game future 上处理
 */
UCLASS(Config = Game)
class AEqZeroHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEqZeroHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UObject interface
	virtual void PreInitializeComponents() override;
	//~End of UObject interface

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AHUD interface
	virtual void GetDebugActorList(TArray<AActor*>& InOutList) override;
	//~End of AHUD interface
};
