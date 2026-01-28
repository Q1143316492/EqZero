// Copyright Epic Games, Inc. All Rights Reserved.
// OK 这个类是记录一下 game feature action 的数量，到0的时候执行一下回收

#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "EqZeroExperienceManager.generated.h"

/**
 * 体验管理器 - 主要用于管理多个 PIE 会话之间的协调
 * Manager for experiences - primarily for arbitration between multiple PIE sessions
 */
UCLASS(MinimalAPI)
class UEqZeroExperienceManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	EQZEROGAME_API void OnPlayInEditorBegun();

	static EQZEROGAME_API void NotifyOfPluginActivation(const FString PluginURL);
	static EQZEROGAME_API bool RequestToDeactivatePlugin(const FString PluginURL);
#else
	static void NotifyOfPluginActivation(const FString PluginURL) {}
	static bool RequestToDeactivatePlugin(const FString PluginURL) { return true; }
#endif

private:
	// The map of requests to active count for a given game feature plugin
	// (to allow first in, last out activation management during PIE)
	// 请求映射到给定游戏特性插件的活跃计数
	// (允许在 PIE 期间进行先进后出的激活管理)
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
};
