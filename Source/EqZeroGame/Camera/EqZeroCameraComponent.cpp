// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCameraComponent)

UEqZeroCameraComponent::UEqZeroCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroCameraComponent::OnRegister()
{
	Super::OnRegister();
}

void UEqZeroCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	Super::GetCameraView(DeltaTime, DesiredView);
}

void UEqZeroCameraComponent::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("EqZeroCameraComponent: %s"), *GetNameSafe(this)));
}
