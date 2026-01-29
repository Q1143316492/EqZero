// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * TODO recorder
 */

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularGameState.h"

#include "EqZeroGameState.generated.h"

#define UE_API EQZEROGAME_API

struct FEqZeroVerbMessage;

class APlayerState;
class UAbilitySystemComponent;
class UEqZeroAbilitySystemComponent;
class UEqZeroExperienceManagerComponent;
class UObject;
struct FFrame;


/**
 * AEqZeroGameState
 *
 *      The base game state class used by this project.
 */
UCLASS(Config = Game)
class AEqZeroGameState : public AModularGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	AEqZeroGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void PostInitializeComponents() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface
	UE_API virtual void AddPlayerState(APlayerState* PlayerState) override;
	UE_API virtual void RemovePlayerState(APlayerState* PlayerState) override;
	UE_API virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	/**
	 *  技能组件 
	 * */

	//~IAbilitySystemInterface
	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	// Gets the ability system component used for game wide things
	UFUNCTION(BlueprintCallable, Category = "EqZero|GameState")
	UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const { return AbilitySystemComponent; }

	/**
	 * 消息
	 */

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "EqZero|GameState")
	UE_API void MulticastMessageToClients(const FEqZeroVerbMessage Message);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "EqZero|GameState")
	UE_API void MulticastReliableMessageToClients(const FEqZeroVerbMessage Message);

	/**
	 * FPS
	 */
	UE_API float GetServerFPS() const;

private:
	UPROPERTY()
	TObjectPtr<UEqZeroExperienceManagerComponent> ExperienceManagerComponent;

	UPROPERTY(VisibleAnywhere, Category = "EqZero|GameState")
	TObjectPtr<UEqZeroAbilitySystemComponent> AbilitySystemComponent;

protected:
	UPROPERTY(Replicated)
	float ServerFPS;
};

#undef UE_API