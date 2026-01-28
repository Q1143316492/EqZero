// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * 录像先不写
 * ServerFPS 只有属性
 * 技能和player state 关联逻辑还没有
 * 先完成体验部分
 */

#pragma once

#include "ModularGameState.h"

#include "EqZeroGameState.generated.h"

class APlayerState;
class UAbilitySystemComponent;
class UEqZeroAbilitySystemComponent;
class UEqZeroExperienceManagerComponent;
class UObject;
struct FFrame;

/**
 * AEqZeroGameState
 *
 *      The base game state class used by this project.
 */
UCLASS(Config = Game)
class AEqZeroGameState : public AModularGameStateBase
{
	GENERATED_BODY()

public:

	AEqZeroGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	//~End of AGameStateBase interface

	// 获取体验管理器组件
	UFUNCTION(BlueprintCallable, Category = "EqZero|GameState")
	UEqZeroExperienceManagerComponent* GetExperienceManagerComponent() const { return ExperienceManagerComponent; }

private:
	// 体验管理器组件，负责加载和管理游戏体验
	UPROPERTY()
	TObjectPtr<UEqZeroExperienceManagerComponent> ExperienceManagerComponent;

protected:
	UPROPERTY(Replicated)
	float ServerFPS;
};
