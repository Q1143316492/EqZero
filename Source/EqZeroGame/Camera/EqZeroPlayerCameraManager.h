// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"

#include "EqZeroPlayerCameraManager.generated.h"

class FDebugDisplayInfo;
class UCanvas;
class UObject;
class UEqZeroUICameraManagerComponent;

#define EQZERO_CAMERA_DEFAULT_FOV		(80.0f)
#define EQZERO_CAMERA_DEFAULT_PITCH_MIN	(-89.0f)
#define EQZERO_CAMERA_DEFAULT_PITCH_MAX	(89.0f)

/**
 * AEqZeroPlayerCameraManager
 *
 */
UCLASS(notplaceable, MinimalAPI)
class AEqZeroPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	AEqZeroPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	UEqZeroUICameraManagerComponent* GetUICameraComponent() const;

protected:

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

private:
	/** The UI camera component, used for specific UI camera adjustments */
	UPROPERTY(Transient)
	TObjectPtr<class UEqZeroUICameraManagerComponent> UICamera;
};
