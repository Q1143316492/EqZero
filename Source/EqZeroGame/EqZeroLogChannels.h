// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

EQZEROGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogEqZero, Log, All);
EQZEROGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogEqZeroExperience, Log, All);
EQZEROGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogEqZeroAbilitySystem, Log, All);
EQZEROGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogEqZeroTeams, Log, All);

EQZEROGAME_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
