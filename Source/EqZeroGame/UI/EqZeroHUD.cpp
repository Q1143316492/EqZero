// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHUD)

AEqZeroHUD::AEqZeroHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AEqZeroHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AEqZeroHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEqZeroHUD::GetDebugActorList(TArray<AActor*>& InOutList)
{
	Super::GetDebugActorList(InOutList);
}
