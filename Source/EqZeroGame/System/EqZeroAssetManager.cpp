// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAssetManager.h"
#include "EqZeroLogChannels.h"
#include "EqZeroGameplayTags.h"
#include "EqZeroGameData.h"
#include "AbilitySystemGlobals.h"
#include "Character/EqZeroPawnData.h"
#include "Misc/App.h"
#include "Stats/StatsMisc.h"
#include "Engine/Engine.h"
#include "Misc/ScopedSlowTask.h"
#include "EqZeroAssetManagerStartupJob.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAssetManager)

const FName FEqZeroBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("EqZero.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(UEqZeroAssetManager::DumpLoadedAssets)
);

//////////////////////////////////////////////////////////////////////

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FEqZeroAssetManagerStartupJob(#JobFunc, [this](const FEqZeroAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

//////////////////////////////////////////////////////////////////////

UEqZeroAssetManager::UEqZeroAssetManager()
{
	DefaultPawnData = nullptr;
}

UEqZeroAssetManager& UEqZeroAssetManager::Get()
{
	check(GEngine);

	if (UEqZeroAssetManager* Singleton = Cast<UEqZeroAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogEqZero, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to EqZeroAssetManager!"));

	// Fatal error above prevents this from being called.
	return *NewObject<UEqZeroAssetManager>();
}

UObject* UEqZeroAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool UEqZeroAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UEqZeroAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void UEqZeroAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogEqZero, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : Get().LoadedAssets)
	{
		UE_LOG(LogEqZero, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogEqZero, Log, TEXT("... %d assets in loaded pool"), Get().LoadedAssets.Num());
	UE_LOG(LogEqZero, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

void UEqZeroAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UEqZeroAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

    // TODO cwl 目前先不预加载

	// STARTUP_JOB(InitializeGameplayCueManager());

	{
		// Load base game data asset
		STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	}

	// Run all the queued up startup jobs
	DoAllStartupJobs();
}

void UEqZeroAssetManager::InitializeGameplayCueManager()
{
	SCOPED_BOOT_TIMING("UEqZeroAssetManager::InitializeGameplayCueManager");

	// TODO: Initialize custom GameplayCueManager if needed
	// For now, we'll use the default GAS GameplayCueManager
    // 这里有个提取加载需要加载的 Cues 的机会，现在还没写 Cue Manager
	UAbilitySystemGlobals::Get().GetGameplayCueManager();
}


const UEqZeroGameData& UEqZeroAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<UEqZeroGameData>(EqZeroGameDataPath);
}

const UEqZeroPawnData* UEqZeroAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}

UPrimaryDataAsset* UEqZeroAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("EqZeroEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogEqZero, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				// This should always work
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		// It is not acceptable to fail to load any GameData asset. It will result in soft failures that are hard to diagnose.
		UE_LOG(LogEqZero, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}


void UEqZeroAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("UEqZeroAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		// No need for periodic progress updates, just run the jobs
		for (const FEqZeroAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			float TotalJobValue = 0.0f;
			for (const FEqZeroAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}

			float AccumulatedJobValue = 0.0f;
			for (FEqZeroAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				const float JobValue = StartupJob.JobWeight;
				StartupJob.SubstepProgressDelegate.BindLambda([This = this, AccumulatedJobValue, JobValue, TotalJobValue](float NewProgress)
				{
					const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.0f, 1.0f) * JobValue;
					const float OverallPercentWithSubstep = (AccumulatedJobValue + SubstepAdjustment) / TotalJobValue;

					This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
				});

				StartupJob.DoJob();

				StartupJob.SubstepProgressDelegate.Unbind();

				AccumulatedJobValue += JobValue;

				UpdateInitialGameContentLoadPercent(AccumulatedJobValue / TotalJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.0f);
		}
	}

	StartupJobs.Empty();

	UE_LOG(LogEqZero, Display, TEXT("All startup jobs took %.2f seconds to complete"), FPlatformTime::Seconds() - AllStartupJobsStartTime);
}

void UEqZeroAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// Could route this to the early startup loading screen
}

#if WITH_EDITOR
void UEqZeroAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	// {
	// 	FScopedSlowTask SlowTask(0, NSLOCTEXT("EqZeroEditor", "BeginLoadingPIEData", "Loading PIE Data"));
	// 	const bool bShowCancelButton = false;
	// 	const bool bAllowInPIE = true;
	// 	SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

	// 	const UEqZeroGameData& LocalGameDataCommon = GetGameData();

	// 	// Intentionally after GetGameData to avoid counting GameData time in this timer
	// 	SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

	// 	// You could add preloading of anything else needed for the experience we'll be using here
	// 	// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	// }
}
#endif
