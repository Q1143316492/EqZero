// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerState.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Character/EqZeroPawnData.h"
#include "EqZeroPlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerState)

AEqZeroPlayerState::AEqZeroPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AbilitySystemComponent 需要高频率
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AEqZeroPlayerState::GetAbilitySystemComponent() const
{
	return GetEqZeroAbilitySystemComponent();
}

AEqZeroPlayerController* AEqZeroPlayerState::GetEqZeroPlayerController() const
{
	return Cast<AEqZeroPlayerController>(GetOwner());
}

void AEqZeroPlayerState::SetPawnData(const UEqZeroPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	ForceNetUpdate();
}

void AEqZeroPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void AEqZeroPlayerState::Reset()
{
	Super::Reset();
}

void AEqZeroPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->InitAbilityActorInfo(this, GetPawn());
	}
}

void AEqZeroPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AEqZeroPlayerState* EqZeroPlayerState = Cast<AEqZeroPlayerState>(PlayerState);
	if (EqZeroPlayerState)
	{
		EqZeroPlayerState->PawnData = PawnData;
	}
}

void AEqZeroPlayerState::OnDeactivated()
{
	Super::OnDeactivated();
}

void AEqZeroPlayerState::OnReactivated()
{
	Super::OnReactivated();
}

void AEqZeroPlayerState::OnRep_PawnData()
{
}

void AEqZeroPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEqZeroPlayerState, PawnData);
}
