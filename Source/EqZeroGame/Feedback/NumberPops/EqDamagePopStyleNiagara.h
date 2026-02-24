// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "EqDamagePopStyleNiagara.generated.h"

class UNiagaraSystem;

/** PopStyle is used to define what Niagara asset should be used for the Damage System representation */
UCLASS()
class UEqDamagePopStyleNiagara : public UDataAsset
{
	GENERATED_BODY()

public:

	// Name of the Niagara Array to set the Damage information
	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	FName NiagaraArrayName;

	// Niagara System used to display the damages
	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	TObjectPtr<UNiagaraSystem> TextNiagara;
};
