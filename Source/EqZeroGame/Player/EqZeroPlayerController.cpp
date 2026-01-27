// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerController.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "EqZeroPlayerState.h"
#include "UI/EqZeroHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerController)

AEqZeroPlayerController::AEqZeroPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AEqZeroPlayerState* AEqZeroPlayerController::GetEqZeroPlayerState() const
{
	return CastChecked<AEqZeroPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UEqZeroAbilitySystemComponent* AEqZeroPlayerController::GetEqZeroAbilitySystemComponent() const
{
	const AEqZeroPlayerState* EqZeroPS = GetEqZeroPlayerState();
	return (EqZeroPS ? EqZeroPS->GetEqZeroAbilitySystemComponent() : nullptr);
}

AEqZeroHUD* AEqZeroPlayerController::GetEqZeroHUD() const
{
	return CastChecked<AEqZeroHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}

void AEqZeroPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(false);
}

void AEqZeroPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEqZeroPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AEqZeroPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AEqZeroPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
}

void AEqZeroPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
}

void AEqZeroPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnPlayerStateChanged();
}

void AEqZeroPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AEqZeroPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void AEqZeroPlayerController::OnPlayerStateChanged()
{
	// Empty for now
}
