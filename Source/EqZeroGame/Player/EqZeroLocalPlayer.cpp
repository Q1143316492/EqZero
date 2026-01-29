// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroLocalPlayer.h"
#include "Settings/EqZeroSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroLocalPlayer)

UEqZeroLocalPlayer::UEqZeroLocalPlayer()
{
}

void UEqZeroLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (UEqZeroSettingsShared* SharedSettingsVar = NewObject<UEqZeroSettingsShared>(this))
	{
		SharedSettings = SharedSettingsVar;
	}
}

UEqZeroSettingsShared* UEqZeroLocalPlayer::GetSharedSettings() const
{
	return SharedSettings;
}

void UEqZeroLocalPlayer::OnSharedSettingsChanged(UEqZeroSettingsShared* Settings)
{
}

void UEqZeroLocalPlayer::SwitchController(class APlayerController* PC)
{
	Super::SwitchController(PC);
}

bool UEqZeroLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	const bool bResult = Super::SpawnPlayActor(URL, OutError, InWorld);

	return bResult;
}

void UEqZeroLocalPlayer::InitOnlineSession()
{
	Super::InitOnlineSession();
}
