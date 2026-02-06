// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/CameraComponent.h"
#include "EqZeroCameraComponent.generated.h"

class UCanvas;
class UEqZeroCameraMode;
class UEqZeroCameraModeStack;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FMinimalViewInfo;
template <class TClass> class TSubclassOf;

DECLARE_DELEGATE_RetVal(TSubclassOf<UEqZeroCameraMode>, FEqZeroCameraModeDelegate);

UCLASS(Config=Game)
class UEqZeroCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:

	UEqZeroCameraComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "EqZero|Camera")
	static UEqZeroCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UEqZeroCameraComponent>() : nullptr); }

	virtual AActor* GetTargetActor() const { return GetOwner(); }
	
	/**
	 * 
	 */
	void AddFieldOfViewOffset(float FovOffset) { FieldOfViewOffset += FovOffset; }
	
	virtual void DrawDebug(UCanvas* Canvas) const;

	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;
protected:

	virtual void OnRegister() override;
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	virtual void UpdateCameraModes();
public:
	// 查询当前最佳摄像机模式的代理，他在UEqZeroHeroComponent中被绑定
	FEqZeroCameraModeDelegate DetermineCameraModeDelegate;
protected:
	UPROPERTY()
	TObjectPtr<UEqZeroCameraModeStack> CameraModeStack;

	float FieldOfViewOffset;
};
