// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Components/GameStateComponent.h"
#include "LoadingProcessInterface.h"

#include "EqZeroExperienceManagerComponent.generated.h"

namespace UE::GameFeatures { struct FResult; }

class UEqZeroExperienceDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEqZeroExperienceLoaded, const UEqZeroExperienceDefinition* /*Experience*/);

/**
 * 体验加载状态枚举
 * Enum for experience load state
 */
enum class EEqZeroExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating
};

/**
 * 体验管理器组件 - 负责加载和管理游戏体验
 * Experience Manager Component - responsible for loading and managing game experiences
 * 
 * 这个组件会附加到 GameState 上，负责：
 * 1. 加载体验定义及其关联的资源包
 * 2. 加载和激活 Game Feature 插件
 * 3. 执行 GameFeatureAction
 * 4. 在体验加载完成后广播事件
 */
UCLASS(MinimalAPI)
class UEqZeroExperienceManagerComponent final : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:

	EQZEROGAME_API UEqZeroExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	EQZEROGAME_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	EQZEROGAME_API virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

	// Tries to set the current experience, either a UI or gameplay one
	// 尝试设置当前体验，可以是 UI 体验或游戏体验
	EQZEROGAME_API void SetCurrentExperience(FPrimaryAssetId ExperienceId);

    // 在完成后调用的委托分为三个优先级
	EQZEROGAME_API void CallOrRegister_OnExperienceLoaded_HighPriority(FOnEqZeroExperienceLoaded::FDelegate&& Delegate);
	EQZEROGAME_API void CallOrRegister_OnExperienceLoaded(FOnEqZeroExperienceLoaded::FDelegate&& Delegate);
	EQZEROGAME_API void CallOrRegister_OnExperienceLoaded_LowPriority(FOnEqZeroExperienceLoaded::FDelegate&& Delegate);

	// This returns the current experience if it is fully loaded, asserting otherwise
	// (i.e., if you called it too soon)
	// 返回当前体验（如果已完全加载），否则触发断言
	// (例如，如果调用太早)
	EQZEROGAME_API const UEqZeroExperienceDefinition* GetCurrentExperienceChecked() const;

	EQZEROGAME_API bool IsExperienceLoaded() const;

private:
	UFUNCTION()
	void OnRep_CurrentExperience();

	void StartExperienceLoad();
	void OnExperienceLoadComplete();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnExperienceFullLoadCompleted();

	void OnActionDeactivationCompleted();
	void OnAllActionsDeactivated();

private:
	UPROPERTY(ReplicatedUsing=OnRep_CurrentExperience)
	TObjectPtr<const UEqZeroExperienceDefinition> CurrentExperience;

	EEqZeroExperienceLoadState LoadState = EEqZeroExperienceLoadState::Unloaded;

	int32 NumGameFeaturePluginsLoading = 0;
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Delegate called when the experience has finished loading just before others
	 * (e.g., subsystems that set up for regular gameplay)
	 * 在体验加载完成时调用的委托，在其他委托之前调用
	 * (例如，为常规游戏设置的子系统)
     * 比如想要调用某个接口，但是这个时候体验没有加载完成，就会绑定到这个代理上，等到体验加载完成后再回调
	 */
	FOnEqZeroExperienceLoaded OnExperienceLoaded_HighPriority;
	FOnEqZeroExperienceLoaded OnExperienceLoaded;
	FOnEqZeroExperienceLoaded OnExperienceLoaded_LowPriority;
};
