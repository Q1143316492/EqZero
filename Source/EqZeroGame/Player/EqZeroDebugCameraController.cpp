// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroDebugCameraController.h"
#include "EqZeroCheatManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroDebugCameraController)

AEqZeroDebugCameraController::AEqZeroDebugCameraController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheatClass = UEqZeroCheatManager::StaticClass();
}

void AEqZeroDebugCameraController::AddCheats(bool bForce)
{
	Super::AddCheats(bForce);
}
