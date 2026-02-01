// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAimSensitivityData.h"

#include "Settings/EqZeroSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAimSensitivityData)

UEqZeroAimSensitivityData::UEqZeroAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SensitivityMap =
	{
		{ EEqZeroGamepadSensitivity::Slow,			0.5f },
		{ EEqZeroGamepadSensitivity::SlowPlus,		0.75f },
		{ EEqZeroGamepadSensitivity::SlowPlusPlus,	0.9f },
		{ EEqZeroGamepadSensitivity::Normal,		1.0f },
		{ EEqZeroGamepadSensitivity::NormalPlus,	1.1f },
		{ EEqZeroGamepadSensitivity::NormalPlusPlus,1.25f },
		{ EEqZeroGamepadSensitivity::Fast,			1.5f },
		{ EEqZeroGamepadSensitivity::FastPlus,		1.75f },
		{ EEqZeroGamepadSensitivity::FastPlusPlus,	2.0f },
		{ EEqZeroGamepadSensitivity::Insane,		2.5f },
	};
}

const float UEqZeroAimSensitivityData::SensitivtyEnumToFloat(const EEqZeroGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}
