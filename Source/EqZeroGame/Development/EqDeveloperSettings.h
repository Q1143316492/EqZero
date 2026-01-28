// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/SoftObjectPath.h"
#include "EqDeveloperSettings.generated.h"

struct FPropertyChangedEvent;
class UEqZeroExperienceDefinition;

UENUM()
enum class ECheatExecutionTime
{
	OnCheatManagerCreated,
	OnPlayerPawnPossession
};

USTRUCT()
struct FEqCheatToRun
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	ECheatExecutionTime Phase = ECheatExecutionTime::OnPlayerPawnPossession;

	UPROPERTY(EditAnywhere)
	FString Cheat;
};

/**
 * Developer settings / editor cheats
 * 这只是一个编辑器存配置面板，没什么逻辑，可以设置一些值业务拿去读一下用于调试。
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UEqDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UEqDeveloperSettings();

	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

public:
	// 填了能覆盖PIE的体验
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZero, meta=(AllowedTypes="EqZeroExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	// 是否覆盖AI数量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZeroBots, meta=(InlineEditConditionToggle))
	bool bOverrideBotCount = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZeroBots, meta=(EditCondition=bOverrideBotCount))
	int32 OverrideNumPlayerBotsToSpawn = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZeroBots)
	bool bAllowPlayerBotsToAttack = true;

	// 在编辑器中玩游戏时，是否执行完整的游戏流程，还是跳过“等待玩家”等游戏阶段？
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZero)
	bool bTestFullGameFlowInPIE = false;

	/**
	* Should force feedback effects be played, even if the last input device was not a gamepad?
	* The default behavior in EqZero is to only play force feedback if the most recent input device was a gamepad.
	*/
	// UPROPERTY(config, EditAnywhere, Category = EqZero, meta = (ConsoleVariable = "EqZeroPC.ShouldAlwaysPlayForceFeedback"))
	// bool bShouldAlwaysPlayForceFeedback = false;

	// Should game logic load cosmetic backgrounds in the editor or skip them for iteration speed?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=EqZero)
	bool bSkipLoadingCosmeticBackgroundsInPIE = false;

	// List of cheats to auto-run during 'play in editor'
	UPROPERTY(config, EditAnywhere, Category=EqZero)
	TArray<FEqCheatToRun> CheatsToRun;

	// Should messages broadcast through the gameplay message subsystem be logged?
	UPROPERTY(config, EditAnywhere, Category=GameplayMessages, meta=(ConsoleVariable="GameplayMessageSubsystem.LogMessages"))
	bool LogGameplayMessages = false;

#if WITH_EDITORONLY_DATA
	/** A list of common maps that will be accessible via the editor toolbar */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif

#if WITH_EDITOR
public:
	// Called by the editor engine to let us pop reminder notifications when cheats are active
	EQZEROGAME_API void OnPlayInEditorStarted() const;

private:
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
};
