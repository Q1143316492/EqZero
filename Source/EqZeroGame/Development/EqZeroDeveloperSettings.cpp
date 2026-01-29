// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroDeveloperSettings.h"
#include "Misc/App.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroDeveloperSettings)

UEqZeroDeveloperSettings::UEqZeroDeveloperSettings()
{
}

const UEqZeroDeveloperSettings* UEqZeroDeveloperSettings::Get()
{
	return GetDefault<UEqZeroDeveloperSettings>();
}
