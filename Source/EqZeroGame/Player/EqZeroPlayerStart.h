// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "GameplayTagContainer.h"

#include "EqZeroPlayerStart.generated.h"

class AController;
class UObject;

// 出生点的状态
enum class EEqZeroPlayerStartLocationOccupancy
{
	Empty,
	Partial,
	Full
};

/**
 * AEqZeroPlayerStart
 * 
 *     这个信息拖进场景里面那个重生点
 */
UCLASS(MinimalAPI, Config = Game)
class AEqZeroPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	AEqZeroPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const FGameplayTagContainer& GetGameplayTags() { return StartPointTags; }

	EEqZeroPlayerStartLocationOccupancy GetLocationOccupancy(AController* const ControllerPawnToFit) const;

	bool IsClaimed() const;

	bool TryClaim(AController* OccupyingController);

protected:
	void CheckUnclaimed();

	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Player Start Claiming")
	float ExpirationCheckInterval = 1.f;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer StartPointTags;

	FTimerHandle ExpirationTimerHandle;
};
