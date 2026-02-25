// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModularAIController.h"
#include "Teams/EqZeroTeamAgentInterface.h"

#include "EqZeroPlayerBotController.generated.h"

namespace ETeamAttitude { enum Type : int; }
struct FGenericTeamId;

class APlayerState;
class UAIPerceptionComponent;
class UObject;

/**
 * AEqZeroPlayerBotController
 *
 *	The controller class used by player bots in this project.
 */
UCLASS(Blueprintable)
class AEqZeroPlayerBotController : public AModularAIController
{
	GENERATED_BODY()

public:
	AEqZeroPlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 这是AI感知的接口
    ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	// 尝试重启此控制器
	void ServerRestartController();

	virtual void OnUnPossess() override;

protected:
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:
	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

private:
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
