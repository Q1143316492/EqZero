// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModularGameState.h"

#include "EqZeroGameState.generated.h"

class APlayerState;
class UAbilitySystemComponent;
class UEqZeroAbilitySystemComponent;
class UObject;
struct FFrame;

/**
 * AEqZeroGameState
 *
 *      The base game state class used by this project.
 */
UCLASS(Config = Game)
class AEqZeroGameState : public AModularGameStateBase
{
	GENERATED_BODY()

public:

	AEqZeroGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	//~End of AGameStateBase interface
};
