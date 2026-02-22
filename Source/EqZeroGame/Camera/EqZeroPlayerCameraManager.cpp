// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EqZeroCameraComponent.h"
#include "EqZeroUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AEqZeroPlayerCameraManager::AEqZeroPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = EQZERO_CAMERA_DEFAULT_FOV;
	ViewPitchMin = EQZERO_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = EQZERO_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UEqZeroUICameraManagerComponent>(TEXT("UICamera"));
}

UEqZeroUICameraManagerComponent* AEqZeroPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AEqZeroPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AEqZeroPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("EqZeroPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UEqZeroCameraComponent* CameraComponent = UEqZeroCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}
