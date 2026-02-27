// Copyright Epic Games, Inc. All Rights Reserved.
// TODO ability debug

#include "EqZeroHUD.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Async/TaskGraphInterfaces.h"
#include "Components/GameFrameworkComponentManager.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHUD)

class AActor;
class UWorld;

AEqZeroHUD::AEqZeroHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AEqZeroHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AEqZeroHUD::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();

	// UE_LOG(LogTemp, Log, TEXT("EqZeroHUD: %s, IsLocalController=%d, NetMode=%d LocalRole=%d"),
	// 	*GetNameSafe(this),
	// 	GetOwningPlayerController() ? GetOwningPlayerController()->IsLocalController() : -1,
	// 	(int32)GetWorld()->GetNetMode(), (int32)GetLocalRole()); // 1, 3 NM_Client, 3 ROLE_Authority
}

void AEqZeroHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AEqZeroHUD::GetDebugActorList(TArray<AActor*>& InOutList)
{
	Super::GetDebugActorList(InOutList);

	// TODO ability debug
}
