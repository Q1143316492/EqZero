// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class EqZeroEditorTarget : TargetRules
{
	public EqZeroEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange(new string[] { "EqZeroGame", "EqZeroEditor" });

		// if (!bBuildAllModules)
		// {
		// 	NativePointerMemberBehaviorOverride = PointerMemberBehavior.Disallow;
		// }

		EqZeroGameTarget.ApplySharedEqZeroTargetSettings(this);

		// Upgraded to avoid engine rebuilds as per error message
		// DefaultBuildSettings = BuildSettingsVersion.V6;
		// BuildEnvironment = TargetBuildEnvironment.Unique;
		bOverrideBuildEnvironment = true;

		// This is used for touch screen development along with the "Unreal Remote 2" app
		EnablePlugins.Add("RemoteSession");
	}
}
