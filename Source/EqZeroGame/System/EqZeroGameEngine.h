// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/GameEngine.h"

#include "EqZeroGameEngine.generated.h"

class IEngineLoop;
class UObject;


UCLASS()
class UEqZeroGameEngine : public UGameEngine
{
        GENERATED_BODY()

public:

        UEqZeroGameEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

        virtual void Init(IEngineLoop* InEngineLoop) override;
};
