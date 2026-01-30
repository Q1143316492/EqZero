// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPerformanceSettings.h"

#include "Engine/PlatformSettingsManager.h"
#include "Misc/EnumRange.h"
#include "EqZeroPerformanceStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPerformanceSettings)

//////////////////////////////////////////////////////////////////////

UEqZeroPlatformSpecificRenderingSettings::UEqZeroPlatformSpecificRenderingSettings()
{
	MobileFrameRateLimits.Append({ 20, 30, 45, 60, 90, 120 });
}

const UEqZeroPlatformSpecificRenderingSettings* UEqZeroPlatformSpecificRenderingSettings::Get()
{
	UEqZeroPlatformSpecificRenderingSettings* Result = UPlatformSettingsManager::Get().GetSettingsForPlatform<ThisClass>();
	check(Result);
	return Result;
}

//////////////////////////////////////////////////////////////////////

UEqZeroPerformanceSettings::UEqZeroPerformanceSettings()
{
	PerPlatformSettings.Initialize(UEqZeroPlatformSpecificRenderingSettings::StaticClass());

	CategoryName = TEXT("Game");

	DesktopFrameRateLimits.Append({ 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 });

	// Default to all stats are allowed
	FEqZeroPerformanceStatGroup& StatGroup = UserFacingPerformanceStats.AddDefaulted_GetRef();
	for (EEqZeroDisplayablePerformanceStat PerfStat : TEnumRange<EEqZeroDisplayablePerformanceStat>())
	{
		StatGroup.AllowedStats.Add(PerfStat);
	}
}
