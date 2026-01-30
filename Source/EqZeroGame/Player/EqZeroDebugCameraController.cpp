// Copyright Epic Games, Inc. All Rights Reserved.
// OK

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
	// 镜像 LyraPlayerController 的 AddCheats（）以避免播放器卡在调试相机中 ？
#if USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else
	Super::AddCheats(bForce);
#endif
}
