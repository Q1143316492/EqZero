// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncAction_ExperienceReady.generated.h"

class AGameStateBase;
class UEqZeroExperienceDefinition;
class UWorld;
struct FFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FExperienceReadyAsyncDelegate);

/**
 * 在蓝图中等待体验加载完成再继续执行的蓝图异步节点
 */
UCLASS()
class UAsyncAction_ExperienceReady : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly="true"))
	static UAsyncAction_ExperienceReady* WaitForExperienceReady(UObject* WorldContextObject);

	virtual void Activate() override;

public:

	// Called when the experience has been determined and is ready/loaded
	UPROPERTY(BlueprintAssignable)
	FExperienceReadyAsyncDelegate OnReady;

private:
	void Step1_HandleGameStateSet(AGameStateBase* GameState);
	void Step2_ListenToExperienceLoading(AGameStateBase* GameState);
	void Step3_HandleExperienceLoaded(const UEqZeroExperienceDefinition* CurrentExperience);
	void Step4_BroadcastReady();

	TWeakObjectPtr<UWorld> WorldPtr;
};
