// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroRuntimeOptions.h"

#include "UObject/Class.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroRuntimeOptions)

UEqZeroRuntimeOptions::UEqZeroRuntimeOptions()
{
	// Set the console command prefix for all runtime options
	// Variables will be accessible as: ro.VariableName
	OptionCommandPrefix = TEXT("ro");
}

UEqZeroRuntimeOptions* UEqZeroRuntimeOptions::GetRuntimeOptions()
{
	return GetMutableDefault<UEqZeroRuntimeOptions>();
}

const UEqZeroRuntimeOptions& UEqZeroRuntimeOptions::Get()
{
	const UEqZeroRuntimeOptions& RuntimeOptions = *GetDefault<UEqZeroRuntimeOptions>();
	return RuntimeOptions;
}
