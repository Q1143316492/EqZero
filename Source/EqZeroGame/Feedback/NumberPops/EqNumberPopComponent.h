// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "GameplayTagContainer.h"

#include "EqNumberPopComponent.generated.h"

class UObject;
struct FFrame;

USTRUCT(BlueprintType)
struct FEqNumberPopRequest
{
	GENERATED_BODY()

	// The world location to create the number pop at
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eq|Number Pops")
	FVector WorldLocation;

	// Tags related to the source/cause of the number pop (for determining a style)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eq|Number Pops")
	FGameplayTagContainer SourceTags;

	// Tags related to the target of the number pop (for determining a style)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eq|Number Pops")
	FGameplayTagContainer TargetTags;

	// The number to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eq|Number Pops")
	int32 NumberToDisplay = 0;

	// Whether the number is 'critical' or not (@TODO: move to a tag)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eq|Number Pops")
	bool bIsCriticalDamage = false;

	FEqNumberPopRequest()
		: WorldLocation(ForceInitToZero)
	{
	}
};


UCLASS(Abstract)
class UEqNumberPopComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	UEqNumberPopComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Adds a damage number to the damage number list for visualization */
	UFUNCTION(BlueprintCallable, Category = Foo)
	virtual void AddNumberPop(const FEqNumberPopRequest& NewRequest) {}
};
