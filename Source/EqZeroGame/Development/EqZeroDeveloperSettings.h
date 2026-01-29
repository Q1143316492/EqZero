// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "EqZeroDeveloperSettings.generated.h"

class UEqZeroExperienceDefinition;

UENUM()
enum class EEqZeroCheatExecutionTime
{
	OnCheatManagerCreated,
	OnPlayerPawnPossession
};

USTRUCT()
struct FEqZeroCheatToRun
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EEqZeroCheatExecutionTime Phase = EEqZeroCheatExecutionTime::OnCheatManagerCreated;

	UPROPERTY(EditAnywhere)
	FString Cheat;
};

/**
 * Developer settings for this project
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="EqZero Developer Settings"))
class UEqZeroDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	EQZEROGAME_API UEqZeroDeveloperSettings();

	EQZEROGAME_API static const UEqZeroDeveloperSettings* Get();

	// Checks if the asset is in the loaded assets list, if so it will be loaded.
	//UFUNCTION(BlueprintCallable, Category = "EqZero|DeveloperSettings")
	//bool IsAssetLoaded(const FPrimaryAssetId& AssetId) const;

public:
	// The experience override to use for Play in Editor (initial map load)
	UPROPERTY(EditAnywhere, config, Category=Lyra, meta=(AllowedTypes="LyraExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	UPROPERTY(EditAnywhere, config, Category=Lyra)
	bool bOverrideBotCount = false;

	UPROPERTY(EditAnywhere, config, Category=Lyra, meta=(EditCondition=bOverrideBotCount))
	int32 OverrideNumPlayerBots = 0;

	UPROPERTY(EditAnywhere, config, Category=Lyra)
	bool bAllowPlayerBotsToAttack = true;

	// Cheats to run when the game starts
	UPROPERTY(EditAnywhere, config, Category=Lyra)
	TArray<FEqZeroCheatToRun> CheatsToRun;
	
	// List of assets to load when the game starts (in editor)
	//UPROPERTY(EditAnywhere, config, Category=Lyra)
	//TArray<FSoftObjectPath> AssetsToLoad;
};

#undef UE_API
