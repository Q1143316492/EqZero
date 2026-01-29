// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/RuntimeOptionsBase.h"

#include "EqZeroRuntimeOptions.generated.h"

class UObject;
struct FFrame;

/**
 * UEqZeroRuntimeOptions
 * 
 * Runtime configuration system that allows changing game behavior without code changes.
 * Extends URuntimeOptionsBase to provide hotfixable game options via console variables.
 * 
 * How It Works:
 * - Add UPROPERTY variables to this class (e.g., bDisableFeatureX, MaxPlayerCount, etc.)
 * - Properties default to their normal/desired state
 * - Variables are automatically registered as console commands under 'ro' namespace
 * - Can be modified at runtime via console: `ro.VariableName Value`
 * - Can be overridden at startup via command line: `-ro.VariableName=Value`
 * - Can be hotfixed via cloud configuration files
 * 
 * Example Usage:
 * ```cpp
 * // In this class:
 * UPROPERTY(config=RuntimeOptions)
 * bool bEnableNewFeature = false;
 * 
 * // In game code:
 * if (UEqZeroRuntimeOptions::Get().bEnableNewFeature)
 * {
 *     // New feature code
 * }
 * 
 * // In console:
 * ro.bEnableNewFeature true
 * 
 * // Command line:
 * Game.exe -ro.bEnableNewFeature=true
 * ```
 * 
 * Common Use Cases:
 * - Feature flags (enable/disable features remotely)
 * - Balance tuning (adjust gameplay parameters)
 * - Debug/test toggles
 * - A/B testing configurations
 * - Emergency kill switches for problematic features
 * 
 * Note: Only available in non-shipping builds for command-line overrides.
 * Hotfixing via cloud config works in all builds.
 */
UCLASS(MinimalAPI, config = RuntimeOptions, BlueprintType)
class UEqZeroRuntimeOptions : public URuntimeOptionsBase
{
	GENERATED_BODY()

public:
	/** Get the singleton instance of runtime options (const version) */
	static EQZEROGAME_API const UEqZeroRuntimeOptions& Get();

	UEqZeroRuntimeOptions();

	/** Get the singleton instance of runtime options (mutable version for Blueprint) */
	UFUNCTION(BlueprintPure, Category = Options)
	static EQZEROGAME_API UEqZeroRuntimeOptions* GetRuntimeOptions();

	// Add your runtime options here as UPROPERTY(config=RuntimeOptions)
	// Example:
	// UPROPERTY(config=RuntimeOptions, EditAnywhere, Category = "Features")
	// bool bEnableExperimentalFeature = false;
};
