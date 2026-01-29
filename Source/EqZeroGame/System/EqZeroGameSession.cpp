// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#include "EqZeroGameSession.h"

#include "EqZeroLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameSession)

AEqZeroGameSession::AEqZeroGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool AEqZeroGameSession::ProcessAutoLogin()
{
	// This is actually handled in GameMode::TryDedicatedServerLogin
	return true;
}

void AEqZeroGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

}

void AEqZeroGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}
