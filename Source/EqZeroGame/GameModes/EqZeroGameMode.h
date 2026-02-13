// Copyright Epic Games, Inc. All Rights Reserved.
// 总结一下就是体验加载和重生相关的代码
// OK

#pragma once

#include "ModularGameMode.h"

#include "EqZeroGameMode.generated.h"

class AActor;
class AController;
class AGameModeBase;
class APawn;
class APlayerController;
class UClass;
class UCommonUserInfo;
class UEqZeroExperienceDefinition;
class UEqZeroPawnData;
class UObject;
struct FFrame;
struct FPrimaryAssetId;

enum class ECommonSessionOnlineMode : uint8;
enum class ECommonUserPrivilege : uint8;
enum class ECommonUserOnlineContext : uint8;

/**
 * Post login event, triggered when a player or bot joins the game as well as after seamless and non seamless travel
 *
 * This is called after the player has finished initialization
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEqZeroGameModePlayerInitialized, AGameModeBase* /*GameMode*/, AController* /*NewPlayer*/);

/**
 * AEqZeroGameMode
 *
 *      The base game mode class used by this project.
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base game mode class used by this project."))
class AEqZeroGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:

	AEqZeroGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "EqZero|Pawn")
	const UEqZeroPawnData* GetPawnDataForController(const AController* InController) const;

	//~AGameModeBase interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual void InitGameState() override;
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;

	virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	virtual void FailedToRestartPlayer(AController* NewPlayer) override;
	//~End of AGameModeBase interface

	/*
	 * 和重生相关接口
	 */
	UFUNCTION(BlueprintCallable)
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);
	
	virtual bool ControllerCanRestart(AController* Controller);

	FOnEqZeroGameModePlayerInitialized OnGameModePlayerInitialized;

protected:
	void OnExperienceLoaded(const UEqZeroExperienceDefinition* CurrentExperience);
	bool IsExperienceLoaded() const;

	void OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource);

	void HandleMatchAssignmentIfNotExpectingOne();

	bool TryDedicatedServerLogin();

	void HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode);

	UFUNCTION()
	void OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);
};
