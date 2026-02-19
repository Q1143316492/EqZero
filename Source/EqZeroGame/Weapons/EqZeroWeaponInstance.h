// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Cosmetics/EqZeroCosmeticAnimationTypes.h"
#include "Equipment/EqZeroEquipmentInstance.h"
#include "GameFramework/InputDevicePropertyHandle.h"

#include "EqZeroWeaponInstance.generated.h"

class UAnimInstance;
class UObject;
struct FFrame;
struct FGameplayTagContainer;
class UInputDeviceProperty;

/**
 * UEqZeroWeaponInstance
 *
 *  武器实例
 *  - 设备属性：例如PS5手柄的自适应扳机配置
 *  - 动画配置：不同的武器可能会链接不同的动画层
 *  - 武器使用时间逻辑：例如可以根据上次使用时间来判断是否触发第一枪精准等
 */
UCLASS()
class UEqZeroWeaponInstance : public UEqZeroEquipmentInstance
{
	GENERATED_BODY()

public:
	UEqZeroWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UEqZeroEquipmentInstance interface
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;
	//~End of UEqZeroEquipmentInstance interface

	/*
	 * 武器使用时间逻辑
	 */
	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	UFUNCTION(BlueprintPure)
	float GetTimeSinceLastInteractedWith() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FEqZeroAnimLayerSelectionSet EquippedAnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FEqZeroAnimLayerSelectionSet UneuippedAnimSet;

	/**
	 * Device properties that should be applied while this weapon is equipped.
	 * These properties will be played in with the "Looping" flag enabled, so they will
	 * play continuously until this weapon is unequipped!
	 * Device Properties 的意思是例如PS5手柄的自适应扳机配置
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Input Devices")
	TArray<TObjectPtr<UInputDeviceProperty>> ApplicableDeviceProperties;
	
	// 通过武器选择链接动画层的接口
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Animation)
	TSubclassOf<UAnimInstance> PickBestAnimLayer(bool bEquipped, const FGameplayTagContainer& CosmeticTags) const;

	UFUNCTION(BlueprintCallable)
	const FPlatformUserId GetOwningUserId() const;

	UFUNCTION()
	void OnDeathStarted(AActor* OwningActor);

	void ApplyDeviceProperties();
	void RemoveDeviceProperties();

private:
	UPROPERTY(Transient)
	TSet<FInputDevicePropertyHandle> DevicePropertyHandles;

	double TimeLastEquipped = 0.0;
	double TimeLastFired = 0.0;
};
