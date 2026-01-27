// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class EqZeroClientTarget : TargetRules
{
	public EqZeroClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;

		ExtraModuleNames.AddRange(new string[] { "EqZeroGame" });

		EqZeroGameTarget.ApplySharedEqZeroTargetSettings(this);
	}
}
