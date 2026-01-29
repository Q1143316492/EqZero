// Copyright Epic Games, Inc. All Rights Reserved.
// TODO recorder

#include "EqZeroGameState.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Async/TaskGraphInterfaces.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/EqZeroExperienceManagerComponent.h"
#include "Messages/EqZeroVerbMessage.h"
#include "Player/EqZeroPlayerState.h"
#include "EqZeroLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameState)

class APlayerState;
class FLifetimeProperty;

extern ENGINE_API float GAverageFPS;

AEqZeroGameState::AEqZeroGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ExperienceManagerComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroExperienceManagerComponent>(this, TEXT("ExperienceManagerComponent"));
	ServerFPS = 0.0f;
}

void AEqZeroGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(/*Owner=*/ this, /*Avatar=*/ this);
}

UAbilitySystemComponent* AEqZeroGameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEqZeroGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEqZeroGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
}

void AEqZeroGameState::RemovePlayerState(APlayerState* PlayerState)
{
	//@TODO: This isn't getting called right now (only the 'rich' AGameMode uses it, not AGameModeBase)
	// Need to at least comment the engine code, and possibly move things around
	Super::RemovePlayerState(PlayerState);
}

void AEqZeroGameState::SeamlessTravelTransitionCheckpoint(bool bToTransitionMap)
{
	// Remove inactive and bots
	for (int32 i = PlayerArray.Num() - 1; i >= 0; i--)
	{
		APlayerState* PlayerState = PlayerArray[i];
		if (PlayerState && (PlayerState->IsABot() || PlayerState->IsInactive()))
		{
			RemovePlayerState(PlayerState);
		}
	}
}

void AEqZeroGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEqZeroGameState, ServerFPS);
}

void AEqZeroGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() == ROLE_Authority)
	{
		ServerFPS = GAverageFPS;
	}
}

void AEqZeroGameState::MulticastMessageToClients_Implementation(const FEqZeroVerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void AEqZeroGameState::MulticastReliableMessageToClients_Implementation(const FEqZeroVerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}

float AEqZeroGameState::GetServerFPS() const
{
	return ServerFPS;
}
