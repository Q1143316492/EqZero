// Copyright Epic Games, Inc. All Rights Reserved.
// TODO 录像 不过暂时没打算做，其他OK

#include "EqZeroUserFacingExperienceDefinition.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "CommonSessionSubsystem.h"

// TODO: Uncomment when EqZeroReplaySubsystem is available
// #include "Replays/EqZeroReplaySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroUserFacingExperienceDefinition)

UCommonSession_HostSessionRequest* UEqZeroUserFacingExperienceDefinition::CreateHostingRequest(const UObject* WorldContextObject) const
{
	const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
	const FString UserFacingExperienceName = GetPrimaryAssetId().PrimaryAssetName.ToString();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UCommonSession_HostSessionRequest* Result = nullptr;


	if (UCommonSessionSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UCommonSessionSubsystem>() : nullptr)
	{
		Result = Subsystem->CreateOnlineHostSessionRequest();
	}

	if (!Result)
	{
		// Couldn't use the subsystem so create one
		Result = NewObject<UCommonSession_HostSessionRequest>();
		Result->OnlineMode = ECommonSessionOnlineMode::Online;
		Result->bUseLobbies = true;
		Result->bUseLobbiesVoiceChat = false;
		// We always enable presence on this session because it is the primary session used for matchmaking. 
		// For online systems that care about presence, only the primary session should have presence enabled
		Result->bUsePresence = !IsRunningDedicatedServer();
	}
	
	Result->MapID = MapID;
	Result->ModeNameForAdvertisement = UserFacingExperienceName;
	Result->ExtraArgs = ExtraArgs;
	Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);
	Result->MaxPlayerCount = MaxPlayerCount;

	// TODO: Enable replay recording when EqZeroReplaySubsystem is available
	/*
	if (UEqZeroReplaySubsystem::DoesPlatformSupportReplays())
	{
		if (bRecordReplay)
		{
			Result->ExtraArgs.Add(TEXT("DemoRec"), FString());
		}
	}
	*/

	return Result;
}
