// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroGameData.h"
#include "EqZeroAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameData)

UEqZeroGameData::UEqZeroGameData()
{
}

const UEqZeroGameData& UEqZeroGameData::Get()
{
	return UEqZeroAssetManager::Get().GetGameData();
}
