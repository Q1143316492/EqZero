// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroSettingsLocal.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Rendering/SlateRenderer.h"
#include "GenericPlatform/GenericPlatformFramePacer.h"
#include "CommonInputSubsystem.h"
#include "Performance/LatencyMarkerModule.h"
#include "Performance/EqZeroPerformanceStatTypes.h"
#include "Performance/EqZeroPerformanceSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroSettingsLocal)

FEqZeroScalabilitySnapshot::FEqZeroScalabilitySnapshot()
{
	Qualities = Scalability::GetQualityLevels();
}

UEqZeroSettingsLocal::UEqZeroSettingsLocal()
{
}

UEqZeroSettingsLocal* UEqZeroSettingsLocal::Get()
{
	return GEngine ? CastChecked<UEqZeroSettingsLocal>(GEngine->GetGameUserSettings()) : nullptr;
}

void UEqZeroSettingsLocal::BeginDestroy()
{
	Super::BeginDestroy();
}

void UEqZeroSettingsLocal::SetToDefaults()
{
	Super::SetToDefaults();
}

void UEqZeroSettingsLocal::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
}

void UEqZeroSettingsLocal::ConfirmVideoMode()
{
	Super::ConfirmVideoMode();
}

float UEqZeroSettingsLocal::GetEffectiveFrameRateLimit()
{
	return Super::GetEffectiveFrameRateLimit();
}

void UEqZeroSettingsLocal::ResetToCurrentSettings()
{
	Super::ResetToCurrentSettings();
}

void UEqZeroSettingsLocal::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	
	// TODO: [Migration] Apply Sound Settings
	// TODO: [Migration] Apply CommonInput settings
}

int32 UEqZeroSettingsLocal::GetOverallScalabilityLevel() const
{
	return Super::GetOverallScalabilityLevel();
}

void UEqZeroSettingsLocal::SetOverallScalabilityLevel(int32 Value)
{
	Super::SetOverallScalabilityLevel(Value);
}

void UEqZeroSettingsLocal::OnExperienceLoaded()
{
	// TODO: [Migration] Update settings based on experience
}

void UEqZeroSettingsLocal::OnHotfixDeviceProfileApplied()
{
	// TODO: [Migration] Re-apply profile
}

void UEqZeroSettingsLocal::SetShouldUseFrontendPerformanceSettings(bool bInFrontEnd)
{
	bInFrontEndForPerformancePurposes = bInFrontEnd;
}

bool UEqZeroSettingsLocal::ShouldUseFrontendPerformanceSettings() const
{
	return bInFrontEndForPerformancePurposes;
}

EEqZeroStatDisplayMode UEqZeroSettingsLocal::GetPerfStatDisplayState(EEqZeroDisplayablePerformanceStat Stat) const
{
	if (const EEqZeroStatDisplayMode* Mode = DisplayStatList.Find(Stat))
	{
		return *Mode;
	}
	return EEqZeroStatDisplayMode::Hidden;
}

void UEqZeroSettingsLocal::SetPerfStatDisplayState(EEqZeroDisplayablePerformanceStat Stat, EEqZeroStatDisplayMode DisplayMode)
{
	if (DisplayMode == EEqZeroStatDisplayMode::Hidden)
	{
		DisplayStatList.Remove(Stat);
	}
	else
	{
		DisplayStatList.Add(Stat, DisplayMode);
	}
	PerfStatSettingsChangedEvent.Broadcast();
}

bool UEqZeroSettingsLocal::DoesPlatformSupportLatencyMarkers()
{
	return false;
}

void UEqZeroSettingsLocal::SetEnableLatencyFlashIndicators(const bool bNewVal)
{
	bEnableLatencyFlashIndicators = bNewVal;
	LatencyFlashInidicatorSettingsChangedEvent.Broadcast();
}

bool UEqZeroSettingsLocal::DoesPlatformSupportLatencyTrackingStats()
{
	return false;
}

void UEqZeroSettingsLocal::SetEnableLatencyTrackingStats(const bool bNewVal)
{
	bEnableLatencyTrackingStats = bNewVal;
	LatencyStatIndicatorSettingsChangedEvent.Broadcast();
	ApplyLatencyTrackingStatSetting();
}

void UEqZeroSettingsLocal::ApplyLatencyTrackingStatSetting()
{
	// TODO: [Migration] Apply logic
}

float UEqZeroSettingsLocal::GetDisplayGamma() const
{
	return DisplayGamma;
}

void UEqZeroSettingsLocal::SetDisplayGamma(float InGamma)
{
	DisplayGamma = InGamma;
	ApplyDisplayGamma();
}

void UEqZeroSettingsLocal::ApplyDisplayGamma()
{
	if (GEngine)
	{
		GEngine->DisplayGamma = DisplayGamma;
	}
}

float UEqZeroSettingsLocal::GetFrameRateLimit_OnBattery() const
{
	return FrameRateLimit_OnBattery;
}

void UEqZeroSettingsLocal::SetFrameRateLimit_OnBattery(float NewLimitFPS)
{
	FrameRateLimit_OnBattery = NewLimitFPS;
	UpdateEffectiveFrameRateLimit();
}

float UEqZeroSettingsLocal::GetFrameRateLimit_InMenu() const
{
	return FrameRateLimit_InMenu;
}

void UEqZeroSettingsLocal::SetFrameRateLimit_InMenu(float NewLimitFPS)
{
	FrameRateLimit_InMenu = NewLimitFPS;
	UpdateEffectiveFrameRateLimit();
}

float UEqZeroSettingsLocal::GetFrameRateLimit_WhenBackgrounded() const
{
	return FrameRateLimit_WhenBackgrounded;
}

void UEqZeroSettingsLocal::SetFrameRateLimit_WhenBackgrounded(float NewLimitFPS)
{
	FrameRateLimit_WhenBackgrounded = NewLimitFPS;
	UpdateEffectiveFrameRateLimit();
}

float UEqZeroSettingsLocal::GetFrameRateLimit_Always() const
{
	return GetFrameRateLimit();
}

void UEqZeroSettingsLocal::SetFrameRateLimit_Always(float NewLimitFPS)
{
	SetFrameRateLimit(NewLimitFPS);
	UpdateEffectiveFrameRateLimit();
}

void UEqZeroSettingsLocal::UpdateEffectiveFrameRateLimit()
{
	// TODO: Logic to choose between battery, menu, etc.
}

int32 UEqZeroSettingsLocal::GetDefaultMobileFrameRate()
{
	return 30; // Default
}

int32 UEqZeroSettingsLocal::GetMaxMobileFrameRate()
{
	return 60; // Default
}

bool UEqZeroSettingsLocal::IsSupportedMobileFramePace(int32 TestFPS)
{
	return true;
}

int32 UEqZeroSettingsLocal::GetFirstFrameRateWithQualityLimit() const
{
	return 30;
}

int32 UEqZeroSettingsLocal::GetLowestQualityWithFrameRateLimit() const
{
	return 0;
}

void UEqZeroSettingsLocal::ResetToMobileDeviceDefaults()
{
	// TODO
}

int32 UEqZeroSettingsLocal::GetMaxSupportedOverallQualityLevel() const
{
	return 3;
}

void UEqZeroSettingsLocal::SetMobileFPSMode(int32 NewLimitFPS)
{
	// TODO
}

void UEqZeroSettingsLocal::ClampMobileResolutionQuality(int32 TargetFPS)
{
	// TODO
}

void UEqZeroSettingsLocal::RemapMobileResolutionQuality(int32 FromFPS, int32 ToFPS)
{
	// TODO
}

void UEqZeroSettingsLocal::ClampMobileFPSQualityLevels(bool bWriteBack)
{
	// TODO
}

void UEqZeroSettingsLocal::ClampMobileQuality()
{
	// TODO
}

int32 UEqZeroSettingsLocal::GetHighestLevelOfAnyScalabilityChannel() const
{
	return 3;
}

void UEqZeroSettingsLocal::OverrideQualityLevelsToScalabilityMode(const FEqZeroScalabilitySnapshot& InMode, Scalability::FQualityLevels& InOutLevels)
{
	// TODO
}

void UEqZeroSettingsLocal::ClampQualityLevelsToDeviceProfile(const Scalability::FQualityLevels& ClampLevels, Scalability::FQualityLevels& InOutLevels)
{
	// TODO
}

void UEqZeroSettingsLocal::SetDesiredMobileFrameRateLimit(int32 NewLimitFPS)
{
	DesiredMobileFrameRateLimit = NewLimitFPS;
}

FString UEqZeroSettingsLocal::GetDesiredDeviceProfileQualitySuffix() const
{
	return DesiredUserChosenDeviceProfileSuffix;
}

void UEqZeroSettingsLocal::SetDesiredDeviceProfileQualitySuffix(const FString& InDesiredSuffix)
{
	DesiredUserChosenDeviceProfileSuffix = InDesiredSuffix;
}

void UEqZeroSettingsLocal::UpdateGameModeDeviceProfileAndFps()
{
	// TODO
}

void UEqZeroSettingsLocal::UpdateConsoleFramePacing()
{
	// TODO
}

void UEqZeroSettingsLocal::UpdateDesktopFramePacing()
{
	// TODO
}

void UEqZeroSettingsLocal::UpdateMobileFramePacing()
{
	// TODO
}

void UEqZeroSettingsLocal::UpdateDynamicResFrameTime(float TargetFPS)
{
	// TODO
}

bool UEqZeroSettingsLocal::IsHeadphoneModeEnabled() const
{
	return bUseHeadphoneMode;
}

void UEqZeroSettingsLocal::SetHeadphoneModeEnabled(bool bEnabled)
{
	bUseHeadphoneMode = bEnabled;
}

bool UEqZeroSettingsLocal::CanModifyHeadphoneModeEnabled() const
{
	return true;
}

bool UEqZeroSettingsLocal::IsHDRAudioModeEnabled() const
{
	return bUseHDRAudioMode;
}

void UEqZeroSettingsLocal::SetHDRAudioModeEnabled(bool bEnabled)
{
	bUseHDRAudioMode = bEnabled;
	// TODO: Apply
}

bool UEqZeroSettingsLocal::CanRunAutoBenchmark() const
{
	return true;
}

bool UEqZeroSettingsLocal::ShouldRunAutoBenchmarkAtStartup() const
{
	return false;
}

void UEqZeroSettingsLocal::RunAutoBenchmark(bool bSaveImmediately)
{
	RunHardwareBenchmark();
	ApplyScalabilitySettings();
	if (bSaveImmediately)
	{
		SaveSettings();
	}
}

void UEqZeroSettingsLocal::ApplyScalabilitySettings()
{
	Scalability::SetQualityLevels(ScalabilityQuality);
}

float UEqZeroSettingsLocal::GetOverallVolume() const
{
	return OverallVolume;
}

void UEqZeroSettingsLocal::SetOverallVolume(float InVolume)
{
	OverallVolume = InVolume;
	// TODO: Apply to bus
}

float UEqZeroSettingsLocal::GetMusicVolume() const
{
	return MusicVolume;
}

void UEqZeroSettingsLocal::SetMusicVolume(float InVolume)
{
	MusicVolume = InVolume;
}

float UEqZeroSettingsLocal::GetSoundFXVolume() const
{
	return SoundFXVolume;
}

void UEqZeroSettingsLocal::SetSoundFXVolume(float InVolume)
{
	SoundFXVolume = InVolume;
}

float UEqZeroSettingsLocal::GetDialogueVolume() const
{
	return DialogueVolume;
}

void UEqZeroSettingsLocal::SetDialogueVolume(float InVolume)
{
	DialogueVolume = InVolume;
}

float UEqZeroSettingsLocal::GetVoiceChatVolume() const
{
	return VoiceChatVolume;
}

void UEqZeroSettingsLocal::SetVoiceChatVolume(float InVolume)
{
	VoiceChatVolume = InVolume;
}

void UEqZeroSettingsLocal::SetAudioOutputDeviceId(const FString& InAudioOutputDeviceId)
{
	AudioOutputDeviceId = InAudioOutputDeviceId;
	OnAudioOutputDeviceChanged.Broadcast(InAudioOutputDeviceId);
}

void UEqZeroSettingsLocal::SetVolumeForSoundClass(FName ChannelName, float InVolume)
{
	// TODO
}

void UEqZeroSettingsLocal::ApplySafeZoneScale()
{
	// TODO
	// SSafeZone::SetGlobalSafeZoneScale(GetSafeZone());
}

void UEqZeroSettingsLocal::SetVolumeForControlBus(USoundControlBus* InSoundControlBus, float InVolume)
{
	// TODO
}

void UEqZeroSettingsLocal::SetControllerPlatform(const FName InControllerPlatform)
{
	ControllerPlatform = InControllerPlatform;
	// TODO: Apply to CommonInput
}

FName UEqZeroSettingsLocal::GetControllerPlatform() const
{
	return ControllerPlatform;
}

void UEqZeroSettingsLocal::LoadUserControlBusMix()
{
	// TODO
}

void UEqZeroSettingsLocal::OnAppActivationStateChanged(bool bIsActive)
{
}

void UEqZeroSettingsLocal::ReapplyThingsDueToPossibleDeviceProfileChange()
{
}
