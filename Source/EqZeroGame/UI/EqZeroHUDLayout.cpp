// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroHUDLayout.h"

#include "CommonUIExtensions.h"
#include "CommonUISettings.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputSettings.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Input/CommonUIInputTypes.h"
#include "ICommonUIModule.h"
#include "NativeGameplayTags.h"
#include "EqZeroLogChannels.h"
// TODO: Include EqZeroControllerDisconnectedScreen
//#include "UI/Foundation/EqZeroControllerDisconnectedScreen.h" 
#include "UI/EqZeroActivatableWidget.h"

#if WITH_EDITOR
#include "CommonUIVisibilitySubsystem.h"
#endif  // WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHUDLayout)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_Input_PrimarlyController, "Platform.Trait.Input.PrimarlyController");

UEqZeroHUDLayout::UEqZeroHUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SpawnedControllerDisconnectScreen(nullptr)
{
	// 默认情况下，只有主要的控制器平台才需要断开连接屏幕。
	PlatformRequiresControllerDisconnectScreen.AddTag(TAG_Platform_Trait_Input_PrimarlyController);
}

void UEqZeroHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 注册 ESC 事件
	// Plugins Common UI Input Settings 里面配置一下Tag和触发按钮，lyra是ESC和手柄按下遥感
	RegisterUIActionBinding(FBindUIActionArgs(
		FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE),
		false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));

	// 是否显示控制器断连界面
	if (ShouldPlatformDisplayControllerDisconnectScreen())
	{
		// Bind to when input device connections change
		IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
		DeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleInputDeviceConnectionChanged);
		DeviceMapper.GetOnInputDevicePairingChange().AddUObject(this, &ThisClass::HandleInputDevicePairingChanged);
	}
}

void UEqZeroHUDLayout::NativeDestruct()
{
	Super::NativeDestruct();

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().RemoveAll(this);
	DeviceMapper.GetOnInputDevicePairingChange().RemoveAll(this);

	if (RequestProcessControllerStateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RequestProcessControllerStateHandle);
		RequestProcessControllerStateHandle.Reset();
	}
}

void UEqZeroHUDLayout::HandleEscapeAction()
{
	// 把 ESC 界面推出来
	if (ensure(!EscapeMenuClass.IsNull()))
	{
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, EscapeMenuClass);
	}
}

void UEqZeroHUDLayout::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();
	ensure(OwningLocalPlayerId.IsValid());

	// 这种设备连接变化发生在另一个玩家身上，我们忽略它就行。
	if (PlatformUserId != OwningLocalPlayerId)
	{
		return;
	}

	NotifyControllerStateChangeForDisconnectScreen();
}

void UEqZeroHUDLayout::HandleInputDevicePairingChanged(FInputDeviceId InputDeviceId, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId)
{
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();
	ensure(OwningLocalPlayerId.IsValid());

	// 如果这种配对变化与我们的本地玩家有关，请通知变化情况。
	if (NewUserPlatformId == OwningLocalPlayerId || OldUserPlatformId == OwningLocalPlayerId)
	{
		NotifyControllerStateChangeForDisconnectScreen();
	}
}

bool UEqZeroHUDLayout::ShouldPlatformDisplayControllerDisconnectScreen() const
{
	// 我们只希望这个菜单出现在主要的控制器平台上，反正就是一TAG检查
	bool bHasAllRequiredTags = ICommonUIModule::GetSettings().GetPlatformTraits().HasAll(PlatformRequiresControllerDisconnectScreen);

	// 检查一下我们在编辑器中也可能在模拟的标签
#if WITH_EDITOR
	const FGameplayTagContainer& PlatformEmulationTags = UCommonUIVisibilitySubsystem::Get(GetOwningLocalPlayer())->GetVisibilityTags();
	bHasAllRequiredTags |= PlatformEmulationTags.HasAll(PlatformRequiresControllerDisconnectScreen);
#endif  // WITH_EDITOR

	return bHasAllRequiredTags;
}

void UEqZeroHUDLayout::NotifyControllerStateChangeForDisconnectScreen()
{
	ensure(ShouldPlatformDisplayControllerDisconnectScreen());

	if (!RequestProcessControllerStateHandle.IsValid())
	{
		RequestProcessControllerStateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
			RequestProcessControllerStateHandle.Reset();
			ProcessControllerDevicesHavingChangedForDisconnectScreen();
			return false;
		}));
	}
}

void UEqZeroHUDLayout::ProcessControllerDevicesHavingChangedForDisconnectScreen()
{
	ensure(ShouldPlatformDisplayControllerDisconnectScreen());
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();

	ensure(OwningLocalPlayerId.IsValid());

	// 获取所有映射到我们玩家的输入设备
	const IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> MappedInputDevices;
	const int32 NumDevicesMappedToUser = DeviceMapper.GetAllInputDevicesForUser(OwningLocalPlayerId, OUT MappedInputDevices);

	// 检查是否有其他已连接的游戏手柄设备映射到该平台用户
	bool bHasConnectedController = false;

	for (const FInputDeviceId MappedDevice : MappedInputDevices)
	{
		if (DeviceMapper.GetInputDeviceConnectionState(MappedDevice) == EInputDeviceConnectionState::Connected)
		{
			const FHardwareDeviceIdentifier HardwareInfo = UInputDeviceSubsystem::Get()->GetInputDeviceHardwareIdentifier(MappedDevice);
			if (HardwareInfo.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad)
			{
				bHasConnectedController = true;
			}
		}
	}

	// 显示和隐藏控制器断开事件
	if (!bHasConnectedController)
	{
		DisplayControllerDisconnectedMenu();
	}
	else if (SpawnedControllerDisconnectScreen)
	{
		HideControllerDisconnectedMenu();
	}
}

void UEqZeroHUDLayout::DisplayControllerDisconnectedMenu_Implementation()
{
	UE_LOG(LogEqZero, Log, TEXT("[%hs] Display controller disconnected menu!"), __func__);

	// if (ControllerDisconnectedScreen)
	// {
	// 	// Push the "controller disconnected" widget to the menu layer
	// 	SpawnedControllerDisconnectScreen = UCommonUIExtensions::PushContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, ControllerDisconnectedScreen);
	// }
}

void UEqZeroHUDLayout::HideControllerDisconnectedMenu_Implementation()
{
	UE_LOG(LogEqZero, Log, TEXT("[%hs] Hide controller disconnected menu!"), __func__);

	// UCommonUIExtensions::PopContentFromLayer(SpawnedControllerDisconnectScreen);
	// SpawnedControllerDisconnectScreen = nullptr;
}
