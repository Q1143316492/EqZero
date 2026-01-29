// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "GameplayAbilitySpecHandle.h"
#include "EqZeroHeroComponent.generated.h"

#define UE_API EQZEROGAME_API

namespace EEndPlayReason { enum Type : int; }
struct FLoadedMappableConfigPair;
struct FMappableConfigPair;

class UGameFrameworkComponentManager;
class UInputComponent;
class UEqZeroCameraMode;
class UEqZeroInputConfig;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;
struct FInputActionValue;

/**
 * 输入，摄像机
 */
UCLASS(MinimalAPI, Blueprintable, Meta=(BlueprintSpawnableComponent))
class UEqZeroHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	UE_API UEqZeroHeroComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "EqZero|Hero")
	static UEqZeroHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UEqZeroHeroComponent>() : nullptr); }

	UE_API void SetAbilityCameraMode(TSubclassOf<UEqZeroCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle);

    /**
     * 技能
     */
    UE_API void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

    /**
     * 输入
     */
	UE_API void AddAdditionalInputConfig(const UEqZeroInputConfig* InputConfig);
	UE_API void RemoveAdditionalInputConfig(const UEqZeroInputConfig* InputConfig);

	UE_API bool IsReadyToBindInputs() const;

	static UE_API const FName NAME_BindInputsNow;
	static UE_API const FName NAME_ActorFeatureName;

    /**
     * 状态接口
     */

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	UE_API virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	UE_API virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	UE_API virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	UE_API virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

protected:

	UE_API virtual void OnRegister() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * 输入
     */

	UE_API virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	UE_API void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	UE_API void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	UE_API void Input_Move(const FInputActionValue& InputActionValue);
	UE_API void Input_LookMouse(const FInputActionValue& InputActionValue);
	UE_API void Input_LookStick(const FInputActionValue& InputActionValue);
	UE_API void Input_Crouch(const FInputActionValue& InputActionValue);
	UE_API void Input_AutoRun(const FInputActionValue& InputActionValue);

    /**
     * 摄像机
     */
	UE_API TSubclassOf<UEqZeroCameraMode> DetermineCameraMode() const;

protected:

	UPROPERTY(EditAnywhere)
	TArray<FInputMappingContextAndPriority> DefaultInputMappings;

	UPROPERTY()
	TSubclassOf<UEqZeroCameraMode> AbilityCameraMode;

	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	bool bReadyToBindInputs;
};

#undef UE_API