// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonPlayerController.h"

#include "EqZeroPlayerController.generated.h"

class AEqZeroHUD;
class AEqZeroPlayerState;
class UEqZeroAbilitySystemComponent;
class APawn;
class APlayerState;
class UObject;

/**
 * AEqZeroPlayerController
 *
 *      The base player controller class used by this project.
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class AEqZeroPlayerController : public ACommonPlayerController
{
	GENERATED_BODY()

public:

	AEqZeroPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	AEqZeroPlayerState* GetEqZeroPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	AEqZeroHUD* GetEqZeroHUD() const;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~End of APlayerController interface

protected:

	virtual void OnPlayerStateChanged();
};
