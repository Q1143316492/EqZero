// Copyright Epic Games, Inc. All Rights Reserved.
// TODO 队伍和设置

#pragma once

#include "CommonLocalPlayer.h"

#include "EqZeroLocalPlayer.generated.h"

/**
 * UEqZeroLocalPlayer
 */
UCLASS(MinimalAPI)
class UEqZeroLocalPlayer : public UCommonLocalPlayer
{
	GENERATED_BODY()

public:

	UEqZeroLocalPlayer();

	//~UObject interface
	virtual void PostInitProperties() override;
	//~End of UObject interface

	//~UPlayer interface
	virtual void SwitchController(class APlayerController* PC) override;
	//~End of UPlayer interface

	//~ULocalPlayer interface
	virtual bool SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld) override;

	public:
	UFUNCTION()
	UEqZeroSettingsShared* GetSharedSettings() const;

protected:
	void OnSharedSettingsChanged(UEqZeroSettingsShared* Settings);

	UPROPERTY(Transient)
	TObjectPtr<UEqZeroSettingsShared> SharedSettings;
    virtual void InitOnlineSession() override;
    //~End of ULocalPlayer interface
};
