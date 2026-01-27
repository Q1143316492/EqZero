// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class EqZeroServerTarget : TargetRules
{
	public EqZeroServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;

		ExtraModuleNames.AddRange(new string[] { "EqZeroGame" });

		EqZeroGameTarget.ApplySharedEqZeroTargetSettings(this);

		bUseChecksInShipping = true;
	}
}
