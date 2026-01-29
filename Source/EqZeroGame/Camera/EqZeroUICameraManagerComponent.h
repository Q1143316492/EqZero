// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "EqZeroUICameraManagerComponent.generated.h"

class AEqZeroPlayerCameraManager;

UCLASS(Transient)
class UEqZeroUICameraManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEqZeroUICameraManagerComponent(const FObjectInitializer& ObjectInitializer);

	void InitializeFor(AEqZeroPlayerCameraManager* InCameraManager);

	bool NeedsToUpdateViewTarget() const;
	void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime);

	void OnShowDebugInfo(UCanvas* Canvas);

private:
	UPROPERTY(Transient)
	TObjectPtr<AEqZeroPlayerCameraManager> CameraManager;
};
