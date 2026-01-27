// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "GameplayAbilitySpecHandle.h"
#include "EqZeroAbilitySet.generated.h"

class UAttributeSet;
class UGameplayEffect;
class UEqZeroAbilitySystemComponent;
class UEqZeroGameplayAbility;
class UObject;


/**
 * FEqZeroAbilitySet_GameplayAbility
 *
 *      描述一个技能
 */
USTRUCT(BlueprintType)
struct FEqZeroAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UEqZeroGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


/**
 * FEqZeroAbilitySet_GameplayEffect
 *
 *      描述一个技能效果
 */
USTRUCT(BlueprintType)
struct FEqZeroAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};

/**
 * FEqZeroAbilitySet_AttributeSet
 *
 *      描述一个属性集
 */
USTRUCT(BlueprintType)
struct FEqZeroAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet;

};

/**
 * FEqZeroAbilitySet_GrantedHandles
 *
 *      Data used to store handles to what has been granted by the ability set.
 */
USTRUCT(BlueprintType)
struct FEqZeroAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void AddAttributeSet(UAttributeSet* Set);

	void TakeFromAbilitySystem(UEqZeroAbilitySystemComponent* EqZeroASC);

protected:

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};


/**
 * UEqZeroAbilitySet
 *
 *      Non-mutable data asset
 */
UCLASS(BlueprintType, Const)
class UEqZeroAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UEqZeroAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	void GiveToAbilitySystem(UEqZeroAbilitySystemComponent* EqZeroASC, FEqZeroAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:

	// GA
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FEqZeroAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// GE
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FEqZeroAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// AS
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
	TArray<FEqZeroAbilitySet_AttributeSet> GrantedAttributes;
};
