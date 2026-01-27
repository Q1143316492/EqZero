// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"

#include "EqZeroGameInstance.generated.h"

namespace puerts { class FJsEnv; }

class AEqZeroPlayerController;
class UObject;

UCLASS(MinimalAPI, Config = Game)
class UEqZeroGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:

	UEqZeroGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	EQZEROGAME_API AEqZeroPlayerController* GetPrimaryPlayerController() const;

	virtual bool CanJoinRequestedSession() const override;

protected:

	virtual void Init() override;
	virtual void Shutdown() override;

private:
	TSharedPtr<puerts::FJsEnv> GameScript;
};
