// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroExperienceManager.h"
#include "Engine/Engine.h"
#include "Subsystems/SubsystemCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroExperienceManager)

#if WITH_EDITOR

void UEqZeroExperienceManager::OnPlayInEditorBegun()
{
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}

void UEqZeroExperienceManager::NotifyOfPluginActivation(const FString PluginURL)
{
	if (GIsEditor)
	{
		UEqZeroExperienceManager* ExperienceManagerSubsystem = GEngine->GetEngineSubsystem<UEqZeroExperienceManager>();
		check(ExperienceManagerSubsystem);

		// Track the number of requesters who activate this plugin. Multiple load/activation requests are always allowed because concurrent requests are handled.
		// 跟踪激活此插件的请求者数量。多个加载/激活请求始终被允许，因为并发请求会被处理。
		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		++Count;
	}
}

bool UEqZeroExperienceManager::RequestToDeactivatePlugin(const FString PluginURL)
{
	if (GIsEditor)
	{
		UEqZeroExperienceManager* ExperienceManagerSubsystem = GEngine->GetEngineSubsystem<UEqZeroExperienceManager>();
		check(ExperienceManagerSubsystem);

		// Only let the last requester to get this far deactivate the plugin
		// 只让最后一个请求者停用插件
		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		--Count;

		if (Count == 0)
		{
			ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		return false;
	}

	return true;
}

#endif
