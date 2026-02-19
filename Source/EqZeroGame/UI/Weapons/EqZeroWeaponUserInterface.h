// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"

#include "EqZeroWeaponUserInterface.generated.h"

class UEqZeroWeaponInstance;
class UObject;
struct FGeometry;

UCLASS()
class UEqZeroWeaponUserInterface : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UEqZeroWeaponUserInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponChanged(UEqZeroWeaponInstance* OldWeapon, UEqZeroWeaponInstance* NewWeapon);

private:
	void RebuildWidgetFromWeapon();

private:
	UPROPERTY(Transient)
	TObjectPtr<UEqZeroWeaponInstance> CurrentInstance;
};
