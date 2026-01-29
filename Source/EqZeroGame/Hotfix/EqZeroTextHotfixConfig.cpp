// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroTextHotfixConfig.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/TextLocalizationManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroTextHotfixConfig)

UEqZeroTextHotfixConfig::UEqZeroTextHotfixConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroTextHotfixConfig::ApplyTextReplacements() const
{
	// Register polyglot text data with the text localization manager
	// This will override any existing text with matching namespace/key
	FTextLocalizationManager::Get().RegisterPolyglotTextData(TextReplacements);
}

void UEqZeroTextHotfixConfig::PostInitProperties()
{
	Super::PostInitProperties();
	ApplyTextReplacements();
}

void UEqZeroTextHotfixConfig::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	ApplyTextReplacements();
}

#if WITH_EDITOR
void UEqZeroTextHotfixConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyTextReplacements();
}
#endif
