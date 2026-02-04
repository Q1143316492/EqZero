// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroTSCheats.h"
#include "GameFramework/CheatManager.h"
#include "EqZeroLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroTSCheats)

UEqZeroTSCheats::UEqZeroTSCheats()
{
#if USING_CHEAT_MANAGER
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda(
			[](UCheatManager* CheatManager)
			{
				// 尝试从配置加载蓝图子类
				const UEqZeroTSCheats* CDO = GetDefault<UEqZeroTSCheats>();
				UClass* ClassToSpawn = UEqZeroTSCheats::StaticClass();
				
				if (CDO->TSCheatsClass.IsValid())
				{
					if (UClass* LoadedClass = CDO->TSCheatsClass.TryLoadClass<UEqZeroTSCheats>())
					{
						ClassToSpawn = LoadedClass;
						UE_LOG(LogEqZero, Log, TEXT("[TSCheats] Using blueprint class: %s"), *CDO->TSCheatsClass.ToString());
					}
				}
				
				CheatManager->AddCheatManagerExtension(NewObject<UEqZeroTSCheats>(CheatManager, ClassToSpawn));
			}));
	}
#endif
}

void UEqZeroTSCheats::TSTest_Implementation()
{
	UE_LOG(LogEqZero, Log, TEXT("[TSCheats] TSTest called (C++ default implementation)"));
}

void UEqZeroTSCheats::TSCommand_Implementation(const FString& Command)
{
	UE_LOG(LogEqZero, Log, TEXT("[TSCheats] TSCommand: %s (C++ default implementation)"), *Command);
}

void UEqZeroTSCheats::TSDebug_Implementation(const FString& Message)
{
	UE_LOG(LogEqZero, Log, TEXT("[TSCheats] TSDebug: %s (C++ default implementation)"), *Message);
}

void UEqZeroTSCheats::TSCppExample(const FString& Info)
{
    UE_LOG(LogEqZero, Log, TEXT("[TSCheats] TSCppExample called with Info: %s"), *Info);
}
