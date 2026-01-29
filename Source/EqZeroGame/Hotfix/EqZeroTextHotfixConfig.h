// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "EqZeroTextHotfixConfig.generated.h"

struct FPolyglotTextData;
struct FPropertyChangedEvent;

/**
 * UEqZeroTextHotfixConfig
 * 
 * Developer settings class that allows hotfixing individual FText values throughout the game.
 * Useful for correcting typos, updating translations, or changing displayed text without code changes.
 * 
 * How It Works:
 * - Uses FPolyglotTextData to define text replacements
 * - FPolyglotTextData supports multiple languages (polyglot = "many languages")
 * - Registered with FTextLocalizationManager to override text lookups
 * - Changes are applied immediately via PostReloadConfig/PostEditChange
 * - Can be configured via Config/DefaultGame.ini
 * 
 * Configuration Example in DefaultGame.ini:
 * ```ini
 * [/Script/EqZeroGame.EqZeroTextHotfixConfig]
 * +TextReplacements=(Namespace="Game",Key="WelcomeMessage",NativeString="Welcome to EqZero!",LocalizedStrings=(...))
 * +TextReplacements=(Namespace="UI",Key="ButtonStart",NativeString="Start Game",LocalizedStrings=(...))
 * ```
 * 
 * Common Use Cases:
 * - Fix typos in shipped text
 * - Update promotional messages
 * - Correct translation errors
 * - Change UI text for events
 * - A/B testing different text variations
 * 
 * Note: This uses Unreal's text localization system, so text must be properly
 * set up with LOCTEXT/NSLOCTEXT macros to be hotfixable.
 */
UCLASS(config=Game, defaultconfig)
class UEqZeroTextHotfixConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEqZeroTextHotfixConfig(const FObjectInitializer& ObjectInitializer);

	// UObject interface
	virtual void PostInitProperties() override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

private:
	/** Apply all configured text replacements to the localization system */
	void ApplyTextReplacements() const;

private:
	/**
	 * List of FText values to hotfix.
	 * Each entry can specify different text per language.
	 * 
	 * FPolyglotTextData contains:
	 * - Namespace: Text namespace (e.g., "Game", "UI")
	 * - Key: Text key (unique identifier)
	 * - NativeString: Default text in native language
	 * - LocalizedStrings: Map of language code -> translated text
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Text Hotfix")
	TArray<FPolyglotTextData> TextReplacements;
};
