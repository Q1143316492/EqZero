// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "GameFramework/GameSession.h"

#include "EqZeroGameSession.generated.h"

class APlayerController;
class FName;
class FUniqueNetId;

/**
 * AEqZeroGameSession
 * 
 * 游戏会话类，负责管理在线会话的创建、加入和管理
 * Manages online session creation, joining, and management
 */
UCLASS(Config = Game)
class EQZEROGAME_API AEqZeroGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	AEqZeroGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool ProcessAutoLogin() override;

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
};
