// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/EqZeroPlayerStart.h" // explicit path

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerStart)

AEqZeroPlayerStart::AEqZeroPlayerStart(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

EEqZeroPlayerStartLocationOccupancy AEqZeroPlayerStart::GetLocationOccupancy(AController* const ControllerPawnToFit) const
{
	UWorld* const World = GetWorld();
	if (HasAuthority() && World)
	{
		if (AGameModeBase* AuthGameMode = World->GetAuthGameMode())
		{
			TSubclassOf<APawn> PawnClass = AuthGameMode->GetDefaultPawnClassForController(ControllerPawnToFit);
			const APawn* const PawnToFit = PawnClass ? GetDefault<APawn>(PawnClass) : nullptr;

			FVector ActorLocation = GetActorLocation();
			const FRotator ActorRotation = GetActorRotation();

			if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation, nullptr))
			{
                // 1. 完美空闲：如果把 Pawn 放在这，完全没有碰到任何物理碰撞（墙、地板、其他人）
				return EEqZeroPlayerStartLocationOccupancy::Empty;
			}
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
                // 2. 稍微有点挤：中心点可能被挡住了，但引擎的 FindTeleportSpot 能在附近稍微挪一挪找到站脚的地方
				return EEqZeroPlayerStartLocationOccupancy::Partial;
			}
		}
	}

    // 3. 彻底堵死：完全没法生成
	return EEqZeroPlayerStartLocationOccupancy::Full;
}

bool AEqZeroPlayerStart::IsClaimed() const
{
	return ClaimingController != nullptr;
}

bool AEqZeroPlayerStart::TryClaim(AController* OccupyingController)
{
    // 占位机制，避免段时间多个角色生成在同一个地方
	if (OccupyingController != nullptr && !IsClaimed())
	{
		ClaimingController = OccupyingController;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ExpirationTimerHandle, FTimerDelegate::CreateUObject(this, &AEqZeroPlayerStart::CheckUnclaimed), ExpirationCheckInterval, true);
		}
		return true;
	}
	return false;
}

void AEqZeroPlayerStart::CheckUnclaimed()
{
	if (ClaimingController != nullptr && ClaimingController->GetPawn() != nullptr && GetLocationOccupancy(ClaimingController) == EEqZeroPlayerStartLocationOccupancy::Empty)
	{
		ClaimingController = nullptr;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
		}
	}
}
