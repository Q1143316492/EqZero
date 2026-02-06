// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"
#include "GameplayTagContainer.h"

#include "EqZeroCameraMode.generated.h"

#define UE_API EQZEROGAME_API

class AActor;
class UCanvas;
class UEqZeroCameraComponent;

/**
 * EEqZeroCameraModeBlendFunction
 *
 *      Blend function used for transitioning between camera modes.
 */
UENUM(BlueprintType)
enum class EEqZeroCameraModeBlendFunction : uint8
{
	// Does a simple linear interpolation.
	Linear,

	// Immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by the exponent.
	EaseIn,

	// Smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by the exponent.
	EaseOut,

	// Smoothly accelerates and decelerates.  Ease amount controlled by the exponent.
	EaseInOut,

	COUNT   UMETA(Hidden)
};


/**
 * FEqZeroCameraModeView
 *
 *      描述摄像机的数据
 */
struct FEqZeroCameraModeView
{
public:

	FEqZeroCameraModeView();

	void Blend(const FEqZeroCameraModeView& Other, float OtherWeight);

public:

	FVector Location;
	FRotator Rotation;
	FRotator ControlRotation;
	float FieldOfView;
};


/**
 * UEqZeroCameraMode
 *
 *      Base class for all camera modes.
 */
UCLASS(MinimalAPI, Abstract, NotBlueprintable)
class UEqZeroCameraMode : public UObject
{
	GENERATED_BODY()

public:

	UE_API UEqZeroCameraMode();

	/**
	 * Getter
	 */
	UE_API UEqZeroCameraComponent* GetEqZeroCameraComponent() const;

	UE_API virtual UWorld* GetWorld() const override;

	UE_API AActor* GetTargetActor() const;

	const FEqZeroCameraModeView& GetCameraModeView() const { return View; }

	FGameplayTag GetCameraTypeTag() const
	{
		return CameraTypeTag;
	}

	/**
	 * 子类重写的激活和取消激活函数
	 */
	virtual void OnActivation() {};
	virtual void OnDeactivation() {};

	UE_API void UpdateCameraMode(float DeltaTime);

	/**
	 * 混合
	 */
	float GetBlendTime() const { return BlendTime; }
	float GetBlendWeight() const { return BlendWeight; }
	UE_API void SetBlendWeight(float Weight);

	/**
	 * Debug
	 */
	UE_API virtual void DrawDebug(UCanvas* Canvas) const;

protected:

	UE_API virtual FVector GetPivotLocation() const;
	UE_API virtual FRotator GetPivotRotation() const;

	UE_API virtual void UpdateView(float DeltaTime);
	UE_API virtual void UpdateBlending(float DeltaTime);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	FGameplayTag CameraTypeTag;

	FEqZeroCameraModeView View;

	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView;

	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin;

	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax;

	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendTime;

	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	EEqZeroCameraModeBlendFunction BlendFunction;

	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendExponent;

	float BlendAlpha;

	float BlendWeight;

protected:
	// 跳过插值，直接设置到目标，下一帧就改为false
	UPROPERTY(transient)
	uint32 bResetInterpolation:1;
};


/**
 * UEqZeroCameraModeStack
 *
 *      Stack used for blending camera modes.
 */
UCLASS()
class UEqZeroCameraModeStack : public UObject
{
	GENERATED_BODY()

public:

	UEqZeroCameraModeStack();

	void ActivateStack();

	void DeactivateStack();

	bool IsStackActivate() const { return bIsActive; }

	void PushCameraMode(TSubclassOf<UEqZeroCameraMode> CameraModeClass);

	bool EvaluateStack(float DeltaTime, FEqZeroCameraModeView& OutCameraModeView);

	void DrawDebug(UCanvas* Canvas) const;

	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;

protected:

	UEqZeroCameraMode* GetCameraModeInstance(TSubclassOf<UEqZeroCameraMode> CameraModeClass);

	void UpdateStack(float DeltaTime);
	void BlendStack(FEqZeroCameraModeView& OutCameraModeView) const;

protected:

	bool bIsActive;

	UPROPERTY()
	TArray<TObjectPtr<UEqZeroCameraMode>> CameraModeInstances; // 对象池，不删除

	UPROPERTY()
	TArray<TObjectPtr<UEqZeroCameraMode>> CameraModeStack; // 实际参与混合的栈
};

#undef UE_API