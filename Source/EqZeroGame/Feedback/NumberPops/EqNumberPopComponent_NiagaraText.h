// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqNumberPopComponent.h"

#include "EqNumberPopComponent_NiagaraText.generated.h"

class UEqDamagePopStyleNiagara;
class UNiagaraComponent;
class UObject;

UCLASS(Blueprintable)
class UEqNumberPopComponent_NiagaraText : public UEqNumberPopComponent
{
	GENERATED_BODY()

public:

	UEqNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UEqNumberPopComponent interface
	virtual void AddNumberPop(const FEqNumberPopRequest& NewRequest) override;
	//~End of UEqNumberPopComponent interface

protected:

	TArray<int32> DamageNumberArray;

	/** Style patterns to attempt to apply to the incoming number pops */
	UPROPERTY(EditDefaultsOnly, Category = "Number Pop|Style")
	TObjectPtr<UEqDamagePopStyleNiagara> Style;

	// Niagara Component used to display the damage
	UPROPERTY(EditDefaultsOnly, Category = "Number Pop|Style")
	TObjectPtr<UNiagaraComponent> NiagaraComp;
};
