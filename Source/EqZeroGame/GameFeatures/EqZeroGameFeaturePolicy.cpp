// Copyright Epic Games, Inc. All Rights Reserved.
// TODO game cue

#include "GameFeatures/EqZeroGameFeaturePolicy.h"

// #include "AbilitySystem/EqZeroGameplayCueManager.h"
#include "GameFeatureData.h"
#include "GameplayCueSet.h"

UEqZeroGameFeaturePolicy::UEqZeroGameFeaturePolicy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UEqZeroGameFeaturePolicy& UEqZeroGameFeaturePolicy::Get()
{
	return UGameFeaturesSubsystem::Get().GetPolicy<UEqZeroGameFeaturePolicy>();
}

void UEqZeroGameFeaturePolicy::InitGameFeatureManager()
{
	Observers.Add(NewObject<UEqZeroGameFeature_HotfixManager>());
	Observers.Add(NewObject<UEqZeroGameFeature_AddGameplayCuePaths>());

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.AddObserver(Observer);
	}

	Super::InitGameFeatureManager();
}

void UEqZeroGameFeaturePolicy::ShutdownGameFeatureManager()
{
	Super::ShutdownGameFeatureManager();

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.RemoveObserver(Observer);
	}
	Observers.Empty();
}

TArray<FPrimaryAssetId> UEqZeroGameFeaturePolicy::GetPreloadAssetListForGameFeature(const UGameFeatureData* GameFeatureToLoad, bool bIncludeLoadedAssets) const
{
	return Super::GetPreloadAssetListForGameFeature(GameFeatureToLoad, bIncludeLoadedAssets);
}

const TArray<FName> UEqZeroGameFeaturePolicy::GetPreloadBundleStateForGameFeature() const
{
	return Super::GetPreloadBundleStateForGameFeature();
}

void UEqZeroGameFeaturePolicy::GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const
{
	// Editor will load both, this can cause hitching as the bundles are set to not preload in editor
	bLoadClientData = !IsRunningDedicatedServer();
	bLoadServerData = !IsRunningClientOnly();
}

bool UEqZeroGameFeaturePolicy::IsPluginAllowed(const FString& PluginURL, FString* OutReason) const
{
	return Super::IsPluginAllowed(PluginURL, OutReason);
}

//////////////////////////////////////////////////////////////////////
//

#include "Hotfix/EqZeroHotfixManager.h"

void UEqZeroGameFeature_HotfixManager::OnGameFeatureLoading(const UGameFeatureData* GameFeatureData, const FString& PluginURL)
{
	if (UEqZeroHotfixManager* HotfixManager = Cast<UEqZeroHotfixManager>(UOnlineHotfixManager::Get(nullptr)))
	{
		HotfixManager->RequestPatchAssetsFromIniFiles();
	}
}

//////////////////////////////////////////////////////////////////////
//

#include "AbilitySystemGlobals.h"
#include "GameFeatureAction_AddGameplayCuePath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameFeaturePolicy)

class FName;
struct FPrimaryAssetId;

void UEqZeroGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UEqZeroGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering);
	
	const FString PluginRootPath = TEXT("/") + PluginName;
	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		if (const UGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UGameFeatureAction_AddGameplayCuePath>(Action))
		{
			const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->GetDirectoryPathsToAdd();
			
			// need cue
			// if (UEqZeroGameplayCueManager* GCM = UEqZeroGameplayCueManager::Get())
			// {
			// 	UGameplayCueSet* RuntimeGameplayCueSet = GCM->GetRuntimeCueSet();
			// 	const int32 PreInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;

			// 	for (const FDirectoryPath& Directory : DirsToAdd)
			// 	{
			// 		FString MutablePath = Directory.Path;
			// 		UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
			// 		GCM->AddGameplayCueNotifyPath(MutablePath, /** bShouldRescanCueAssets = */ false);	
			// 	}
				
			// 	// Rebuild the runtime library with these new paths
			// 	if (!DirsToAdd.IsEmpty())
			// 	{
			// 		GCM->InitializeRuntimeObjectLibrary();	
			// 	}

			// 	const int32 PostInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;
			// 	if (PreInitializeNumCues != PostInitializeNumCues)
			// 	{
			// 		GCM->RefreshGameplayCuePrimaryAsset();
			// 	}
			// }
		}
	}
}

void UEqZeroGameFeature_AddGameplayCuePaths::OnGameFeatureUnregistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL)
{
	const FString PluginRootPath = TEXT("/") + PluginName;
	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		if (const UGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UGameFeatureAction_AddGameplayCuePath>(Action))
		{
			const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->GetDirectoryPathsToAdd();
			
			// TODO need cue
			// if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
			// {
			// 	int32 NumRemoved = 0;
			// 	for (const FDirectoryPath& Directory : DirsToAdd)
			// 	{
			// 		FString MutablePath = Directory.Path;
			// 		UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
			// 		NumRemoved += GCM->RemoveGameplayCueNotifyPath(MutablePath, /** bShouldRescanCueAssets = */ false);
			// 	}

			// 	ensure(NumRemoved == DirsToAdd.Num());
				
			// 	// Rebuild the runtime library only if there is a need to
			// 	if (NumRemoved > 0)
			// 	{
			// 		GCM->InitializeRuntimeObjectLibrary();	
			// 	}			
			// }
		}
	}
}
