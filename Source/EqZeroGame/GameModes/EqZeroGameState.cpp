// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameState.h"

#include "EqZeroLogChannels.h"
#include "EqZeroExperienceManagerComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameState)

AEqZeroGameState::AEqZeroGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	// AbilitySystemComponent->SetIsReplicated(true);
	// AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 创建体验管理器组件
	ExperienceManagerComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroExperienceManagerComponent>(this, TEXT("ExperienceManagerComponent"));
}

void AEqZeroGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEqZeroGameState, ServerFPS);
}

void AEqZeroGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
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
	Super::RemovePlayerState(PlayerState);
}
