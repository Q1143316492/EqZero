// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "EqZeroAttributeSet.h"
#include "NativeGameplayTags.h"

#include "EqZeroHealthSet.generated.h"

#define UE_API EQZEROGAME_API

class UObject;
struct FFrame;

EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Damage);
EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageImmunity);
EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageSelfDestruct);
EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_FellOutOfWorld);
EQZEROGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EqZero_Damage_Message);

struct FGameplayEffectModCallbackData;


/**
 * UEqZeroHealthSet
 *
 *	定义了用于承受伤害的属性的类。
 *	属性示例包括：生命值、最大生命值、治疗和伤害。
 */
UCLASS(MinimalAPI, BlueprintType)
class UEqZeroHealthSet : public UEqZeroAttributeSet
{
	GENERATED_BODY()

public:

	UE_API UEqZeroHealthSet();

	ATTRIBUTE_ACCESSORS(UEqZeroHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UEqZeroHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UEqZeroHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UEqZeroHealthSet, Damage);

	mutable FEqZeroAttributeEvent OnHealthChanged; // 客户端可能丢失
	mutable FEqZeroAttributeEvent OnMaxHealthChanged;
	mutable FEqZeroAttributeEvent OnOutOfHealth;

protected:
    /**
     * 属性同步回调
     */

	UFUNCTION()
	UE_API void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	UE_API void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    /**
     * 效果执行前后
     */

	UE_API virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	UE_API virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    /**
     * 属性变化
     */
	UE_API virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override; 
	UE_API virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	UE_API virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	UE_API void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
    /**
     * 生命值相关
     */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "EqZero|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "EqZero|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	bool bOutOfHealth;

	// 缓存一下修改前的数值，用于做是否变化的逻辑
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

    /**
     * 伤害和治疗
     */

	UPROPERTY(BlueprintReadOnly, Category="EqZero|Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Healing;

	UPROPERTY(BlueprintReadOnly, Category="EqZero|Health", Meta=(HideFromModifiers, AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
};

#undef UE_API
