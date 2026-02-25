// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerBotController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/EqZeroGameMode.h"
#include "EqZeroLogChannels.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerBotController)

AEqZeroPlayerBotController::AEqZeroPlayerBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsPlayerState = true; // 需要PlayerState来存储Bot的PawnData等信息
	bStopAILogicOnUnposses = false; // 失去pawn控制权时不停止AI逻辑，继续执行重启流程
}

ETeamAttitude::Type AEqZeroPlayerBotController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other)) 
    {
        return ETeamAttitude::Hostile; // 默认全是敌人
	}

	return ETeamAttitude::Neutral;
}

void AEqZeroPlayerBotController::OnPlayerStateChanged()
{
	// 留给子类的
}

void AEqZeroPlayerBotController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();
	LastSeenPlayerState = PlayerState;
}

void AEqZeroPlayerBotController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AEqZeroPlayerBotController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AEqZeroPlayerBotController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AEqZeroPlayerBotController::ServerRestartController()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	ensure((GetPawn() == nullptr) && IsInState(NAME_Inactive));

	if (IsInState(NAME_Inactive) || (IsInState(NAME_Spectating)))
	{
		AEqZeroGameMode* const GameMode = GetWorld()->GetAuthGameMode<AEqZeroGameMode>();

		if ((GameMode == nullptr) || !GameMode->ControllerCanRestart(this))
		{
			return;
		}

		// If we're still attached to a Pawn, leave it
		if (GetPawn() != nullptr)
		{
			UnPossess();
		}

		// Re-enable input, similar to code in ClientRestart
		ResetIgnoreInputFlags();

		GameMode->RestartPlayer(this);
	}
}

void AEqZeroPlayerBotController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}
