// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameInstance.h"

#include "Player/EqZeroPlayerController.h"
#include "JsEnv.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameInstance)

UEqZeroGameInstance::UEqZeroGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


AEqZeroPlayerController* UEqZeroGameInstance::GetPrimaryPlayerController() const
{
	return Cast<AEqZeroPlayerController>(Super::GetPrimaryPlayerController());
}

bool UEqZeroGameInstance::CanJoinRequestedSession() const
{
	// TODO: Checks for session join
	return Super::CanJoinRequestedSession();
}

void UEqZeroGameInstance::Init()
{
	Super::Init();
	GameScript = MakeShared<puerts::FJsEnv>();
	
	TArray<TPair<FString, UObject*>> Arguments;
	Arguments.Add(TPair<FString, UObject*>("GameInstance", this));
	GameScript->Start("Main", Arguments);
}

void UEqZeroGameInstance::Shutdown()
{
	Super::Shutdown();
	GameScript.Reset();
}
