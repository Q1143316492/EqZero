// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "EqZeroWorldSettings.generated.h"

class UObject;
class UEqZeroExperienceDefinition;

/**
 * 默认的世界设置对象，主要用于设置在该地图上游玩时所使用的默认游戏体验
 */
UCLASS(MinimalAPI)
class AEqZeroWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:

	AEqZeroWorldSettings(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif

public:
	FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category=GameMode)
	TSoftClassPtr<UEqZeroExperienceDefinition> DefaultGameplayExperience;

public:

#if WITH_EDITORONLY_DATA
    // 有些地图是单机模式，如果设置了在 PIE 里面会改成单机
    UPROPERTY(EditDefaultsOnly, Category=PIE)
	bool ForceStandaloneNetMode = false;
#endif
};
