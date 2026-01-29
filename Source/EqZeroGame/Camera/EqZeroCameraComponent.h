// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/CameraComponent.h"
#include "EqZeroCameraComponent.generated.h"

class UCanvas;

UCLASS(Config=Game)
class UEqZeroCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:

	UEqZeroCameraComponent(const FObjectInitializer& ObjectInitializer);

	// Returns the camera component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "EqZero|Camera")
	static UEqZeroCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UEqZeroCameraComponent>() : nullptr); }

	virtual void DrawDebug(UCanvas* Canvas) const;

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

protected:

	virtual void OnRegister() override;
 
};
