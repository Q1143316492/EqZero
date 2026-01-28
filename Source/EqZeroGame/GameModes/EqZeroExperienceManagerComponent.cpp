// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroExperienceManagerComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "EqZeroExperienceDefinition.h"
#include "EqZeroExperienceActionSet.h"
#include "EqZeroExperienceManager.h"
#include "GameFeaturesSubsystem.h"
#include "System/EqZeroAssetManager.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystemSettings.h"
#include "TimerManager.h"
#include "EqZeroLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroExperienceManagerComponent)

// 这里是虚幻，自己写的一堆 to do，我们可以先不管
//@TODO: Async load the experience definition itself
//@TODO: Handle failures explicitly (go into a 'completed but failed' state rather than check()-ing)
//@TODO: Do the action phases at the appropriate times instead of all at once
//@TODO: Support deactivating an experience and do the unloading actions
//@TODO: Think about what deactivation/cleanup means for preloaded assets
//@TODO: Handle deactivating game features, right now we 'leak' them enabled
// (for a client moving from experience to experience we actually want to diff the requirements and only unload some, not unload everything for them to just be immediately reloaded)
//@TODO: Handle both built-in and URL-based plugins (search for colon?)

namespace EqZeroConsoleVariables
{
	static float ExperienceLoadRandomDelayMin = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayMin(
		TEXT("EqZero.chaos.ExperienceDelayLoad.MinSecs"),
		ExperienceLoadRandomDelayMin,
		TEXT("This value (in seconds) will be added as a delay of load completion of the experience (along with the random value EqZero.chaos.ExperienceDelayLoad.RandomSecs)"),
		ECVF_Default);

	static float ExperienceLoadRandomDelayRange = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayRange(
		TEXT("EqZero.chaos.ExperienceDelayLoad.RandomSecs"),
		ExperienceLoadRandomDelayRange,
		TEXT("A random amount of time between 0 and this value (in seconds) will be added as a delay of load completion of the experience (along with the fixed value EqZero.chaos.ExperienceDelayLoad.MinSecs)"),
		ECVF_Default);

	float GetExperienceLoadDelayDuration()
	{
		return FMath::Max(0.0f, ExperienceLoadRandomDelayMin + FMath::FRand() * ExperienceLoadRandomDelayRange);
	}
}

UEqZeroExperienceManagerComponent::UEqZeroExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UEqZeroExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId ExperienceId)
{
	UEqZeroAssetManager& AssetManager = UEqZeroAssetManager::Get();
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId);
	TSubclassOf<UEqZeroExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad());
	check(AssetClass);
	const UEqZeroExperienceDefinition* Experience = GetDefault<UEqZeroExperienceDefinition>(AssetClass);

	check(Experience != nullptr);
	check(CurrentExperience == nullptr);
	CurrentExperience = Experience;
	StartExperienceLoad();
}

void UEqZeroExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_HighPriority(FOnEqZeroExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}

void UEqZeroExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnEqZeroExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}

void UEqZeroExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_LowPriority(FOnEqZeroExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

const UEqZeroExperienceDefinition* UEqZeroExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == EEqZeroExperienceLoadState::Loaded);
	check(CurrentExperience != nullptr);
	return CurrentExperience;
}

bool UEqZeroExperienceManagerComponent::IsExperienceLoaded() const
{
	return (LoadState == EEqZeroExperienceLoadState::Loaded) && (CurrentExperience != nullptr);
}

void UEqZeroExperienceManagerComponent::OnRep_CurrentExperience()
{
	StartExperienceLoad();
}

void UEqZeroExperienceManagerComponent::StartExperienceLoad()
{
	check(CurrentExperience != nullptr);
	check(LoadState == EEqZeroExperienceLoadState::Unloaded);

	UE_LOG(LogEqZeroExperience, Log, TEXT("EXPERIENCE: StartExperienceLoad(CurrentExperience = %s, %s)"),
		*CurrentExperience->GetPrimaryAssetId().ToString(),
		*GetClientServerContextString(this));

	LoadState = EEqZeroExperienceLoadState::Loading;

	UEqZeroAssetManager& AssetManager = UEqZeroAssetManager::Get();

	TSet<FPrimaryAssetId> BundleAssetList;
	TSet<FSoftObjectPath> RawAssetList;

	BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());
	for (const TObjectPtr<UEqZeroExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}

	// Load assets associated with the experience
	// 加载与体验关联的资源

	TArray<FName> BundlesToLoad;
	BundlesToLoad.Add(FEqZeroBundles::Equipped);

	//@TODO: Centralize this client/server stuff into the EqZeroAssetManager
	const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	const bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer);
	const bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client);
	if (bLoadClient)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
	}
	if (bLoadServer)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
	}

	TSharedPtr<FStreamableHandle> BundleLoadHandle = nullptr;
	if (BundleAssetList.Num() > 0)
	{
		BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(BundleAssetList.Array(), BundlesToLoad, {}, false, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}

	TSharedPtr<FStreamableHandle> RawLoadHandle = nullptr;
	if (RawAssetList.Num() > 0)
	{
		RawLoadHandle = AssetManager.LoadAssetList(RawAssetList.Array(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, TEXT("StartExperienceLoad()"));
	}

	// If both async loads are running, combine them
	// 如果两个异步加载都在运行，将它们合并
	TSharedPtr<FStreamableHandle> Handle = nullptr;
	if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
	{
		Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({ BundleLoadHandle, RawLoadHandle });
	}
	else
	{
		Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
	}

	FStreamableDelegate OnAssetsLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// Assets were already loaded, call the delegate now
		// 资源已经加载完成，立即调用委托
		FStreamableHandle::ExecuteDelegate(OnAssetsLoadedDelegate);
	}
	else
	{
		Handle->BindCompleteDelegate(OnAssetsLoadedDelegate);

		Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda([OnAssetsLoadedDelegate]()
			{
				OnAssetsLoadedDelegate.ExecuteIfBound();
			}));
	}

	// This set of assets gets preloaded, but we don't block the start of the experience based on it
	// 这组资源会被预加载，但我们不会因为它而阻塞体验的开始
	TSet<FPrimaryAssetId> PreloadAssetList;
	//@TODO: Determine assets to preload (but not blocking-ly)
	if (PreloadAssetList.Num() > 0)
	{
		AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
	}
}

void UEqZeroExperienceManagerComponent::OnExperienceLoadComplete()
{
	check(LoadState == EEqZeroExperienceLoadState::Loading);
	check(CurrentExperience != nullptr);

	UE_LOG(LogEqZeroExperience, Log, TEXT("EXPERIENCE: OnExperienceLoadComplete(CurrentExperience = %s, %s)"),
		*CurrentExperience->GetPrimaryAssetId().ToString(),
		*GetClientServerContextString(this));

	// find the URLs for our GameFeaturePlugins - filtering out dupes and ones that don't have a valid mapping
	// 查找我们的 GameFeaturePlugins 的 URL - 过滤掉重复项和没有有效映射的项
	GameFeaturePluginURLs.Reset();

	auto CollectGameFeaturePluginURLs = [This=this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
	{
		for (const FString& PluginName : FeaturePluginList)
		{
			FString PluginURL;
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, /*out*/ PluginURL))
			{
				This->GameFeaturePluginURLs.AddUnique(PluginURL);
			}
			else
			{
				ensureMsgf(false, TEXT("OnExperienceLoadComplete failed to find plugin URL from PluginName %s for experience %s - fix data, ignoring for this run"), *PluginName, *Context->GetPrimaryAssetId().ToString());
			}
		}
	};

	CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);
	for (const TObjectPtr<UEqZeroExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
		}
	}

	// Load and activate the features
	// 加载并激活特性
	NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();
	if (NumGameFeaturePluginsLoading > 0)
	{
		LoadState = EEqZeroExperienceLoadState::LoadingGameFeatures;
		for (const FString& PluginURL : GameFeaturePluginURLs)
		{
			UEqZeroExperienceManager::NotifyOfPluginActivation(PluginURL);
			UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
		}
	}
	else
	{
		OnExperienceFullLoadCompleted();
	}
}

void UEqZeroExperienceManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{
	// decrement the number of plugins that are loading
	// 减少正在加载的插件数量
	NumGameFeaturePluginsLoading--;

	if (NumGameFeaturePluginsLoading == 0)
	{
		OnExperienceFullLoadCompleted();
	}
}

void UEqZeroExperienceManagerComponent::OnExperienceFullLoadCompleted()
{
	check(LoadState != EEqZeroExperienceLoadState::Loaded);

	// Insert a random delay for testing (if configured)
	// 插入随机延迟用于测试（如果配置了的话）
	if (LoadState != EEqZeroExperienceLoadState::LoadingChaosTestingDelay)
	{
		const float DelaySecs = EqZeroConsoleVariables::GetExperienceLoadDelayDuration();
		if (DelaySecs > 0.0f)
		{
			FTimerHandle DummyHandle;

			LoadState = EEqZeroExperienceLoadState::LoadingChaosTestingDelay;
			GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnExperienceFullLoadCompleted, DelaySecs, /*bLooping=*/ false);

			return;
		}
	}

	LoadState = EEqZeroExperienceLoadState::ExecutingActions;

	// Execute the actions
	// 执行 Actions
	FGameFeatureActivatingContext Context;

	// Only apply to our specific world context if set
	// 如果设置了，只应用于我们特定的世界上下文
	const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	if (ExistingWorldContext)
	{
		Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}

	auto ActivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
	{
		for (UGameFeatureAction* Action : ActionList)
		{
			if (Action != nullptr)
			{
				//@TODO: The fact that these don't take a world are potentially problematic in client-server PIE
				// The current behavior matches systems like gameplay tags where loading and registering apply to the entire process,
				// but actually applying the results to actors is restricted to a specific world
				Action->OnGameFeatureRegistering();
				Action->OnGameFeatureLoading();
				Action->OnGameFeatureActivating(Context);
			}
		}
	};

	ActivateListOfActions(CurrentExperience->Actions);
	for (const TObjectPtr<UEqZeroExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			ActivateListOfActions(ActionSet->Actions);
		}
	}

	LoadState = EEqZeroExperienceLoadState::Loaded;

	OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_HighPriority.Clear();

	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();

	OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_LowPriority.Clear();
}

void UEqZeroExperienceManagerComponent::OnActionDeactivationCompleted()
{
	check(IsInGameThread());
	++NumObservedPausers;

	if (NumObservedPausers == NumExpectedPausers)
	{
		OnAllActionsDeactivated();
	}
}

void UEqZeroExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentExperience);
}

void UEqZeroExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// deactivate any features this experience loaded
	// 停用此体验加载的任何特性
	//@TODO: This should be handled FILO as well
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		if (UEqZeroExperienceManager::RequestToDeactivatePlugin(PluginURL))
		{
			UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
		}
	}

	//@TODO: Ensure proper handling of a partially-loaded state too
	if (LoadState == EEqZeroExperienceLoadState::Loaded)
	{
		LoadState = EEqZeroExperienceLoadState::Deactivating;

		// Make sure we won't complete the transition prematurely if someone registers as a pauser but fires immediately
		// 确保如果有人注册为暂停器但立即触发，我们不会过早完成转换
		NumExpectedPausers = INDEX_NONE;
		NumObservedPausers = 0;

		// Deactivate and unload the actions
		// 停用并卸载 Actions
		FGameFeatureDeactivatingContext Context(TEXT(""), [this](FStringView) { this->OnActionDeactivationCompleted(); });

		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}

		auto DeactivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
		{
			for (UGameFeatureAction* Action : ActionList)
			{
				if (Action)
				{
					Action->OnGameFeatureDeactivating(Context);
					Action->OnGameFeatureUnregistering();
				}
			}
		};

		DeactivateListOfActions(CurrentExperience->Actions);
		for (const TObjectPtr<UEqZeroExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			if (ActionSet != nullptr)
			{
				DeactivateListOfActions(ActionSet->Actions);
			}
		}

		NumExpectedPausers = Context.GetNumPausers();

		if (NumExpectedPausers > 0)
		{
			UE_LOG(LogEqZeroExperience, Error, TEXT("Actions that have asynchronous deactivation aren't fully supported yet in EqZero experiences"));
		}

		if (NumExpectedPausers == NumObservedPausers)
		{
			OnAllActionsDeactivated();
		}
	}
}

bool UEqZeroExperienceManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (LoadState != EEqZeroExperienceLoadState::Loaded)
	{
		OutReason = TEXT("Experience still loading");
		return true;
	}
	else
	{
		return false;
	}
}

void UEqZeroExperienceManagerComponent::OnAllActionsDeactivated()
{
	//@TODO: We actually only deactivated and didn't fully unload...
	LoadState = EEqZeroExperienceLoadState::Unloaded;
	CurrentExperience = nullptr;
	//@TODO:	GEngine->ForceGarbageCollection(true);
}
