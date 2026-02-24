// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "EqDamagePopStyle.generated.h"

class UObject;
class UStaticMesh;

UCLASS()
class UEqDamagePopStyle : public UDataAsset
{
	GENERATED_BODY()

public:

	UEqDamagePopStyle();

	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	FString DisplayText;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	FGameplayTagQuery MatchPattern;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop", meta=(EditCondition=bOverrideColor))
	FLinearColor Color;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop", meta=(EditCondition=bOverrideColor))
	FLinearColor CriticalColor;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop", meta=(EditCondition=bOverrideMesh))
	TObjectPtr<UStaticMesh> TextMesh;

	UPROPERTY()
	bool bOverrideColor = false;

	UPROPERTY()
	bool bOverrideMesh = false;
};
