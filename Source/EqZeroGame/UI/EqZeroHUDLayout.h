// Copyright Epic Games, Inc. All Rights Reserved.
// TODO 控制器(手柄)断连界面还没写

#pragma once

#include "EqZeroActivatableWidget.h"
#include "CommonActivatableWidget.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"

#include "EqZeroHUDLayout.generated.h"

class UCommonActivatableWidget;
class UObject;
// TODO: Create UEqZeroControllerDisconnectedScreen
// class UEqZeroControllerDisconnectedScreen;

/**
 * UEqZeroHUDLayout
 *
 *      布局的那个UI类
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "EqZero HUD Layout", Category = "EqZero|HUD"))
class UEqZeroHUDLayout : public UEqZeroActivatableWidget
{
	GENERATED_BODY()

public:

	UEqZeroHUDLayout(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	void HandleEscapeAction();

	/**
	* 控制器断开连接时的回调函数。这将检查玩家现在是否没有映射到他们的输入设备，这意味着他们无法玩游戏。
	*/
	void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

	/**
	* 控制器更改其所属平台用户时的回调函数。我们将使用它来检查
	*/
	void HandleInputDevicePairingChanged(FInputDeviceId InputDeviceId, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId);

	/**
	* 开Tick检查控制器断连状态
	*/
	void NotifyControllerStateChangeForDisconnectScreen();

	/**
	 * 楼上开的那个tick的具体逻辑
	 */
	virtual void ProcessControllerDevicesHavingChangedForDisconnectScreen();

	/**
     * 是否显示控制器断连界面，根据平台特征和TAG来检查
     */
    virtual bool ShouldPlatformDisplayControllerDisconnectScreen() const;

	/**
	* Pushes the ControllerDisconnectedMenuClass to the Menu layer (UI.Layer.Menu)
	*/
	UFUNCTION(BlueprintNativeEvent, Category="Controller Disconnect Menu")
	void DisplayControllerDisconnectedMenu();

	/**
	* Hides the controller disconnected menu if it is active.
	*/
	UFUNCTION(BlueprintNativeEvent, Category="Controller Disconnect Menu")
	void HideControllerDisconnectedMenu();

	/**
	 * 游戏按ESC会出来的那个界面
	 */
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;

	/**
	* 控制器断连界面（这个界面还没写）
	*/
	UPROPERTY(EditDefaultsOnly, Category="Controller Disconnect Menu")
	TSubclassOf<UCommonActivatableWidget> ControllerDisconnectedScreen; // TODO: Use TSubclassOf<UEqZeroControllerDisconnectedScreen>

	/**
	 * 显示控制器断开需要的平台标签，如果没配就永远不会出现
	 */
	UPROPERTY(EditDefaultsOnly, Category="Controller Disconnect Menu")
	FGameplayTagContainer PlatformRequiresControllerDisconnectScreen;

	/**
	 * 控制器断连界面的实例
	 */
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> SpawnedControllerDisconnectScreen;

	/*
	 * 检查是否连上的代理
	 */
	FTSTicker::FDelegateHandle RequestProcessControllerStateHandle;
};
