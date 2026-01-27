// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroEditorEngine.h"

// #include "Development/EqZeroDeveloperSettings.h"
// #include "Development/EqZeroPlatformEmulationSettings.h"
#include "Engine/GameInstance.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameModes/EqZeroWorldSettings.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroEditorEngine)

class IEngineLoop;

#define LOCTEXT_NAMESPACE "EqZeroEditor"

UEqZeroEditorEngine::UEqZeroEditorEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroEditorEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
}

void UEqZeroEditorEngine::Start()
{
	Super::Start();
}

void UEqZeroEditorEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);
	
	FirstTickSetup();
}

void UEqZeroEditorEngine::FirstTickSetup()
{
	if (bFirstTickSetup)
	{
		return;
	}

	bFirstTickSetup = true;

	// 在编辑器里面默认显示插件的文件夹
	GetMutableDefault<UContentBrowserSettings>()->SetDisplayPluginFolders(true);

}

FGameInstancePIEResult UEqZeroEditorEngine::PreCreatePIEInstances(const bool bAnyBlueprintErrors, const bool bStartInSpectatorMode, const float PIEStartTime, const bool bSupportsOnlinePIE, int32& InNumOnlinePIEInstances)
{
	if (const AEqZeroWorldSettings* EqZeroWorldSettings = Cast<AEqZeroWorldSettings>(EditorWorld->GetWorldSettings()))
	{
		// 它尝试获取当前编辑的关卡的世界设置，并确认该设置里是否勾选了 ForceStandaloneNetMode（强制单机模式）
		// 这个是给炸弹人那个纯单机关卡用的
		if (EqZeroWorldSettings->ForceStandaloneNetMode)
		{
			EPlayNetMode OutPlayNetMode;
			PlaySessionRequest->EditorPlaySettings->GetPlayNetMode(OutPlayNetMode);
			if (OutPlayNetMode != PIE_Standalone)
			{
				PlaySessionRequest->EditorPlaySettings->SetPlayNetMode(PIE_Standalone);

				FNotificationInfo Info(LOCTEXT("ForcingStandaloneForFrontend", "Forcing NetMode: Standalone for the Frontend"));
				Info.ExpireDuration = 2.0f;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		}
	}

	// TODO cwl 这两个设置还没有 Should add delegates that a *non-editor* module could bind to for PIE start/stop instead of poking directly
	// GetDefault<UEqZeroDeveloperSettings>()->OnPlayInEditorStarted();
	// GetDefault<UEqZeroPlatformEmulationSettings>()->OnPlayInEditorStarted();

	//
	FGameInstancePIEResult Result = Super::PreCreatePIEServerInstance(bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime, bSupportsOnlinePIE, InNumOnlinePIEInstances);

	return Result;
}

#undef LOCTEXT_NAMESPACE
