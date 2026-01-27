// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameMode.h"

#include "EqZeroGameState.h"
#include "EqZeroLogChannels.h"
#include "System/EqZeroGameInstance.h"
#include "Player/EqZeroPlayerController.h"
#include "Character/EqZeroPawnData.h"
#include "UI/EqZeroHUD.h"
#include "Player/EqZeroPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameMode)

AEqZeroGameMode::AEqZeroGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AEqZeroGameState::StaticClass();
	PlayerControllerClass = AEqZeroPlayerController::StaticClass();
	HUDClass = AEqZeroHUD::StaticClass();
	PlayerStateClass = AEqZeroPlayerState::StaticClass();
	DefaultPawnClass = nullptr; // Uses PawnData
	SpectatorClass = nullptr;
}

const UEqZeroPawnData* AEqZeroGameMode::GetPawnDataForController(const AController* InController) const
{
	// See if the pawn data is already set on the player state
	if (InController)
	{
		if (const AEqZeroPlayerState* EqZeroPS = InController->GetPlayerState<AEqZeroPlayerState>())
		{
			if (const UEqZeroPawnData* PawnData = EqZeroPS->GetPawnData<UEqZeroPawnData>())
			{
				return PawnData;
			}
		}
	}

	return nullptr;
}

void AEqZeroGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	
	// Wait for the next tick to process match assignment
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

UClass* AEqZeroGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UEqZeroPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* AEqZeroGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UEqZeroPawnData* PawnData = const_cast<UEqZeroPawnData*>(GetPawnDataForController(NewPlayer)))
			{
				// Keep a pointer to the pawn data
				// PawnData->ApplyToPawn(SpawnedPawn); 
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
	}

	return nullptr;
}

bool AEqZeroGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn transform
	return false;
}

void AEqZeroGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Check to see if we have already assigned a match to this player
	/*
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
	*/
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

AActor* AEqZeroGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AEqZeroGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (const UEqZeroPawnData* PawnData = GetPawnDataForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = NewPlayer->GetPawn())
		{
			// Optional: Apply abilities from pawn data
		}
	}
	
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AEqZeroGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

void AEqZeroGameMode::InitGameState()
{
	Super::InitGameState();

	// Listen for experience load events?
}

bool AEqZeroGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void AEqZeroGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AEqZeroGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);
}

void AEqZeroGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && Controller)
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
}

bool AEqZeroGameMode::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}

	// But we can check if experience is loaded
	// if (!IsExperienceLoaded()) { return false; }

	return true;
}

void AEqZeroGameMode::OnExperienceLoaded(const UEqZeroExperienceDefinition* CurrentExperience)
{
	// Spawn players, etc.
}

bool AEqZeroGameMode::IsExperienceLoaded() const
{
	// return (ExperienceManagerComponent && ExperienceManagerComponent->IsExperienceLoaded());
	return true; // Placeholder
}

void AEqZeroGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	// Assign experience
}

void AEqZeroGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	// Default match assignment logic
}
