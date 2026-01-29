// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DebugCameraController.h"
#include "EqZeroDebugCameraController.generated.h"

UCLASS()
class AEqZeroDebugCameraController : public ADebugCameraController
{
	GENERATED_BODY()

public:
	AEqZeroDebugCameraController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void AddCheats(bool bForce) override;
};
