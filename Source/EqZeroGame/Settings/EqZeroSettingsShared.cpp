// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroSettingsShared.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/LocalPlayer.h"

// TODO: [Migration] Include CommonUser module headers if using ULocalPlayerSaveGame
// #include "CommonLocalPlayer.h" 

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroSettingsShared)

static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");

UEqZeroSettingsShared::UEqZeroSettingsShared()
{
	FInternationalization::Get().OnCultureChanged().AddUObject(this, &ThisClass::OnCultureChanged);
	
	// TODO: [Migration] Initialize deadzones from CVars if needed
	// GamepadMoveStickDeadZone = ...
	// GamepadLookStickDeadZone = ...
}

int32 UEqZeroSettingsShared::GetLatestDataVersion() const
{
	return 1;
}

UEqZeroSettingsShared* UEqZeroSettingsShared::CreateTemporarySettings(const ULocalPlayer* LocalPlayer)
{
	// TODO: [Migration] Implement CreateNewSaveGameForLocalPlayer
	// This usually requires ULocalPlayerSaveGame logic.
	return nullptr;
}

UEqZeroSettingsShared* UEqZeroSettingsShared::LoadOrCreateSettings(const ULocalPlayer* LocalPlayer)
{
	// TODO: [Migration] Implement LoadOrCreateSaveGameForLocalPlayer
	return nullptr;
}

bool UEqZeroSettingsShared::AsyncLoadOrCreateSettings(const ULocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate)
{
	// TODO: [Migration] Implement AsyncLoadOrCreateSaveGameForLocalPlayer
	return false;
}

void UEqZeroSettingsShared::SaveSettings()
{
	// TODO: [Migration] Implement Save
	// AsyncSaveGameToSlotForLocalPlayer();
}

void UEqZeroSettingsShared::ApplySettings()
{
	ApplySubtitleOptions();
	ApplyBackgroundAudioSettings();
	ApplyCultureSettings();
	// TODO: [Migration] Apply EnhancedInput settings
}

void UEqZeroSettingsShared::SetColorBlindStrength(int32 InColorBlindStrength)
{
	InColorBlindStrength = FMath::Clamp(InColorBlindStrength, 0, 10);
	if (ColorBlindStrength != InColorBlindStrength)
	{
		ColorBlindStrength = InColorBlindStrength;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

int32 UEqZeroSettingsShared::GetColorBlindStrength() const
{
	return ColorBlindStrength;
}

void UEqZeroSettingsShared::SetColorBlindMode(EEqZeroColorBlindMode InMode)
{
	if (ColorBlindMode != InMode)
	{
		ColorBlindMode = InMode;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

EEqZeroColorBlindMode UEqZeroSettingsShared::GetColorBlindMode() const
{
	return ColorBlindMode;
}

void UEqZeroSettingsShared::ApplySubtitleOptions()
{
	// TODO: [Migration] Apply subtitle options using SubtitleDisplaySubsystem
}

void UEqZeroSettingsShared::SetAllowAudioInBackgroundSetting(EEqZeroAllowBackgroundAudioSetting NewValue)
{
	if (ChangeValueAndDirty(AllowAudioInBackground, NewValue))
	{
		ApplyBackgroundAudioSettings();
	}
}

void UEqZeroSettingsShared::ApplyBackgroundAudioSettings()
{
	// TODO: [Migration] Check OwningPlayer
	// if (OwningPlayer && OwningPlayer->IsPrimaryPlayer())
	{
		FApp::SetUnfocusedVolumeMultiplier((AllowAudioInBackground != EEqZeroAllowBackgroundAudioSetting::Off) ? 1.0f : 0.0f);
	}
}

void UEqZeroSettingsShared::ApplyCultureSettings()
{
	// Logic copied from Lyra
	if (bResetToDefaultCulture)
	{
		const FCulturePtr SystemDefaultCulture = FInternationalization::Get().GetDefaultCulture();
		check(SystemDefaultCulture.IsValid());

		const FString CultureToApply = SystemDefaultCulture->GetName();
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		bResetToDefaultCulture = false;
	}
	else if (!PendingCulture.IsEmpty())
	{
		const FString CultureToApply = PendingCulture;
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *CultureToApply, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ClearPendingCulture();
	}
}

void UEqZeroSettingsShared::ResetCultureToCurrentSettings()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

const FString& UEqZeroSettingsShared::GetPendingCulture() const
{
	return PendingCulture;
}

void UEqZeroSettingsShared::SetPendingCulture(const FString& NewCulture)
{
	PendingCulture = NewCulture;
	bResetToDefaultCulture = false;
	bIsDirty = true;
}

void UEqZeroSettingsShared::OnCultureChanged()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

void UEqZeroSettingsShared::ClearPendingCulture()
{
	PendingCulture.Reset();
}

bool UEqZeroSettingsShared::IsUsingDefaultCulture() const
{
	FString Culture;
	GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Culture, GGameUserSettingsIni);

	return Culture.IsEmpty();
}

void UEqZeroSettingsShared::ResetToDefaultCulture()
{
	ClearPendingCulture();
	bResetToDefaultCulture = true;
	bIsDirty = true;
}

void UEqZeroSettingsShared::ApplyInputSensitivity()
{
	// TODO: [Migration] Apply input sensitivity
}
