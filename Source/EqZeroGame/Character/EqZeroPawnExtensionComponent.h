// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"

#include "EqZeroPawnExtensionComponent.generated.h"

#define UE_API EQZEROGAME_API

namespace EEndPlayReason { enum Type : int; }

class UGameFrameworkComponentManager;
class UEqZeroAbilitySystemComponent;
class UEqZeroPawnData;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;

/**
 * 非常重要的组件
 * - 分离角色技能初始化状态的逻辑用的
 */
UCLASS(MinimalAPI)
class UEqZeroPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	UE_API UEqZeroPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	static UE_API const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	UE_API virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	UE_API virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	UE_API virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	UE_API virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	UFUNCTION(BlueprintPure, Category = "EqZero|Pawn")
	static UEqZeroPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UEqZeroPawnExtensionComponent>() : nullptr); }

    /**
     * PawnData
     */

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	UE_API void SetPawnData(const UEqZeroPawnData* InPawnData);

    /**
     * 技能系统组件
     */

	UFUNCTION(BlueprintPure, Category = "EqZero|Pawn")
	UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const { return AbilitySystemComponent; }

    // 技能组件生命周期
	UE_API void InitializeAbilitySystem(UEqZeroAbilitySystemComponent* InASC, AActor* InOwnerActor);
	UE_API void UninitializeAbilitySystem();

    // 控制器和玩家状态变化处理
	UE_API void HandleControllerChanged();
	UE_API void HandlePlayerStateReplicated();
	UE_API void SetupPlayerInputComponent();

    // 技能组件初始化和反初始化委托注册
	UE_API void OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);
	UE_API void OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

protected:

	UE_API virtual void OnRegister() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	UE_API void OnRep_PawnData();

    /**
     * 技能系统初始化委托
     * Pawn
     * ASC
     */

    FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;

	UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "EqZero|Pawn")
	TObjectPtr<const UEqZeroPawnData> PawnData;

	UPROPERTY(Transient)
	TObjectPtr<UEqZeroAbilitySystemComponent> AbilitySystemComponent;
};

#undef UE_API