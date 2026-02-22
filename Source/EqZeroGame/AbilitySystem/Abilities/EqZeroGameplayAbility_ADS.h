// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility.h"

#include "EqZeroGameplayAbility_ADS.generated.h"

class UInputMappingContext;
class USoundBase;
class APlayerController;
class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;


/**
 * FEqZeroADSUIMessage
 *
 *	通过 GameplayMessageSubsystem 广播的 ADS 状态消息。
 */
USTRUCT(BlueprintType)
struct FEqZeroADSUIMessage
{
	GENERATED_BODY()

	// 是否开镜
	UPROPERTY(BlueprintReadWrite)
	bool bON = false;

	// 触发此消息的玩家控制器
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> Controller = nullptr;
};


/**
 * UEqZeroGameplayAbility_ADS
 *
 *	Gameplay ability used for ADS (Aim Down Sights / 开镜瞄准).
 */
UCLASS(Abstract)
class EQZEROGAME_API UEqZeroGameplayAbility_ADS : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_ADS(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ADS 摄像机模式
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	TSubclassOf<UEqZeroCameraMode> DefaultCameraMode;

	// 开镜时的移速倍率（相对于默认移速）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	float ADSMultiplier = 0.5f;

	// 开镜时叠加的输入映射上下文
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	TObjectPtr<UInputMappingContext> ADSInputMappingContext;

	// 输入映射上下文优先级
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	int32 ADSMappingContextPriority = 11;

	// 开镜音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	TObjectPtr<USoundBase> ZoomInSound;

	// 关镜音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EqZero|ADS")
	TObjectPtr<USoundBase> ZoomOutSound;

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	// 通过 GameplayMessageSubsystem 广播 ADS 状态给 UI
	void BroadcastToUI(bool bON);

	// WaitInputRelease 回调
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	// 运行时缓存的默认最大移速
	float MaxWalkSpeedDefault = 0.0f;
};
