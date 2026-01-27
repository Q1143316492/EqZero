// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"

#include "EqZeroPlayerState.generated.h"

class AController;
class AEqZeroPlayerController;
class APlayerState;
class FName;
class UAbilitySystemComponent;
class UEqZeroAbilitySystemComponent;
class UEqZeroPawnData;
class UObject;
struct FFrame;

/**
 * AEqZeroPlayerState
 *
 *      Base player state class used by this project.
 */
UCLASS(MinimalAPI, Config = Game)
class AEqZeroPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEqZeroPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerState")
	AEqZeroPlayerController* GetEqZeroPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerState")
	UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const { return AbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const UEqZeroPawnData* InPawnData);

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

protected:
	UFUNCTION()
	void OnRep_PawnData();

protected:

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UEqZeroPawnData> PawnData;

private:

	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "EqZero|PlayerState")
	TObjectPtr<UEqZeroAbilitySystemComponent> AbilitySystemComponent;
};
