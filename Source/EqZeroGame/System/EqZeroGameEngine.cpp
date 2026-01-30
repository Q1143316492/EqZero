// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameEngine)

class IEngineLoop;


UEqZeroGameEngine::UEqZeroGameEngine(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{
}

void UEqZeroGameEngine::Init(IEngineLoop* InEngineLoop)
{
        Super::Init(InEngineLoop);
}
