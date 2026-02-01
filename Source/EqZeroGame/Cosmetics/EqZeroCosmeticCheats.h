// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/CheatManager.h"

#include "EqZeroCosmeticCheats.generated.h"

class UEqZeroControllerComponent_CharacterParts;
class UObject;
struct FFrame;

UCLASS(NotBlueprintable)
class UEqZeroCosmeticCheats final : public UCheatManagerExtension
{
	GENERATED_BODY()

public:
	UEqZeroCosmeticCheats();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void AddCharacterPart(const FString& AssetName, bool bSuppressNaturalParts = true);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void ReplaceCharacterPart(const FString& AssetName, bool bSuppressNaturalParts = true);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void ClearCharacterPartOverrides();

private:
	UEqZeroControllerComponent_CharacterParts* GetCosmeticComponent() const;
};
