// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerState.h"

#include "AbilitySystem/Attributes/EqZeroCombatSet.h"
#include "AbilitySystem/Attributes/EqZeroHealthSet.h"
#include "AbilitySystem/EqZeroAbilitySet.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Character/EqZeroPawnData.h"
// TODO: Implement pawn extension component when it's ported
// #include "Character/EqZeroPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/EqZeroExperienceManagerComponent.h"
#include "GameModes/EqZeroGameMode.h"
#include "EqZeroLogChannels.h"
#include "EqZeroPlayerController.h"
#include "Messages/EqZeroVerbMessage.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerState)

class AController;
class APlayerState;
class FLifetimeProperty;

const FName AEqZeroPlayerState::NAME_EqZeroAbilityReady("EqZeroAbilitiesReady");

AEqZeroPlayerState::AEqZeroPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MyPlayerConnectionType(EEqZeroPlayerConnectionType::Player)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UEqZeroAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HealthSet = CreateDefaultSubobject<UEqZeroHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UEqZeroCombatSet>(TEXT("CombatSet"));

	SetNetUpdateFrequency(100.0f); // 技能需要高频一点
}

void AEqZeroPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroPlayerState::Reset()
{
	Super::Reset();
}

void AEqZeroPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	// TODO: Implement when pawn extension component is ported
	// if (UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	// {
	//     PawnExtComp->CheckDefaultInitialization();
	// }
}

void AEqZeroPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

void AEqZeroPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
		case EEqZeroPlayerConnectionType::Player:
		case EEqZeroPlayerConnectionType::InactivePlayer:
			//@TODO: Ask the experience if we should destroy disconnecting players immediately or leave them around
			// (e.g., for long running servers where they might build up if lots of players cycle through)
			bDestroyDeactivatedPlayerState = true;
			break;
		default:
			bDestroyDeactivatedPlayerState = true;
			break;
	}

	SetPlayerConnectionType(EEqZeroPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void AEqZeroPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == EEqZeroPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(EEqZeroPlayerConnectionType::Player);
	}
}

void AEqZeroPlayerState::OnExperienceLoaded(const UEqZeroExperienceDefinition* /*CurrentExperience*/)
{
    if (AEqZeroGameMode* EqZeroGameMode = GetWorld()->GetAuthGameMode<AEqZeroGameMode>())
    {
        if (const UEqZeroPawnData* NewPawnData = EqZeroGameMode->GetPawnDataForController(GetOwningController()))
        {
            SetPawnData(NewPawnData);
        }
        else
        {
            UE_LOG(LogEqZero, Error, TEXT("AEqZeroPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
        }
    }
}

void AEqZeroPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)

	SharedParams.Condition = ELifetimeCondition::COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, SharedParams);

	DOREPLIFETIME(ThisClass, StatTags);
}

FRotator AEqZeroPlayerState::GetReplicatedViewRotation() const
{
	// Could replace this with custom replication
	return ReplicatedViewRotation;
}

void AEqZeroPlayerState::SetReplicatedViewRotation(const FRotator& NewRotation)
{
	if (NewRotation != ReplicatedViewRotation)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this);
		ReplicatedViewRotation = NewRotation;
	}
}

AEqZeroPlayerController* AEqZeroPlayerState::GetEqZeroPlayerController() const
{
	return Cast<AEqZeroPlayerController>(GetOwner());
}

UAbilitySystemComponent* AEqZeroPlayerState::GetAbilitySystemComponent() const
{
	return GetEqZeroAbilitySystemComponent();
}

void AEqZeroPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
	    AGameStateBase* GameState = GetWorld()->GetGameState();
	    check(GameState);
	    UEqZeroExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UEqZeroExperienceManagerComponent>();
	    check(ExperienceComponent);
	    ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnEqZeroExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
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
		UE_LOG(LogEqZero, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	for (const UEqZeroAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_EqZeroAbilityReady);

	ForceNetUpdate();
}

void AEqZeroPlayerState::OnRep_PawnData()
{
}

void AEqZeroPlayerState::SetPlayerConnectionType(EEqZeroPlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void AEqZeroPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AEqZeroPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AEqZeroPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AEqZeroPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void AEqZeroPlayerState::ClientBroadcastMessage_Implementation(const FEqZeroVerbMessage Message)
{
	// This check is needed to prevent running the action when in standalone mode
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}
