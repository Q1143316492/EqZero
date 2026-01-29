// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "GameplayEffectTypes.h"

#include "EqZeroGameplayEffectContext.generated.h"

class AActor;
class FArchive;
class IEqZeroAbilitySourceInterface;
class UObject;
class UPhysicalMaterial;

USTRUCT()
struct FEqZeroGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FEqZeroGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FEqZeroGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}

	static EQZEROGAME_API FEqZeroGameplayEffectContext* ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

	/** Sets the object used as the ability source */
	void SetAbilitySource(const IEqZeroAbilitySourceInterface* InObject, float InSourceLevel);

	const IEqZeroAbilitySourceInterface* GetAbilitySource() const;

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FEqZeroGameplayEffectContext* NewContext = new FEqZeroGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FEqZeroGameplayEffectContext::StaticStruct();
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

	const UPhysicalMaterial* GetPhysicalMaterial() const;

public:
	/** ID to allow the identification of multiple bullets that were part of the same cartridge */
	UPROPERTY()
	int32 CartridgeID = -1;

protected:
	/** Ability Source object (should implement IEqZeroAbilitySourceInterface). NOT replicated currently */
	UPROPERTY()
	TWeakObjectPtr<const UObject> AbilitySourceObject;
};

template<>
struct TStructOpsTypeTraits<FEqZeroGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FEqZeroGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
