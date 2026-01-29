// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCheatManager.h"
#include "GameFramework/Pawn.h"
#include "EqZeroPlayerController.h"
#include "EqZeroDebugCameraController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Console.h"
#include "GameFramework/HUD.h"
#include "System/EqZeroAssetManager.h"
#include "System/EqZeroGameData.h"
#include "EqZeroGameplayTags.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/EqZeroHealthComponent.h"
#include "Character/EqZeroPawnExtensionComponent.h"
// #include "System/EqZeroSystemStatics.h"
#include "Development/EqZeroDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCheatManager)

DEFINE_LOG_CATEGORY(LogEqZeroCheat);

namespace EqZeroCheat
{
	static const FName NAME_Fixed = FName(TEXT("Fixed"));

	static bool bEnableDebugCameraCycling = false;
	static FAutoConsoleVariableRef CVarEnableDebugCameraCycling(
		TEXT("EqZeroCheat.EnableDebugCameraCycling"),
		bEnableDebugCameraCycling,
		TEXT("If true then you can cycle the debug camera while running the game."),
		ECVF_Cheat);

	static bool bStartInGodMode = false;
	static FAutoConsoleVariableRef CVarStartInGodMode(
		TEXT("EqZeroCheat.StartInGodMode"),
		bStartInGodMode,
		TEXT("If true then the God cheat will be applied on begin play"),
		ECVF_Cheat);
};


UEqZeroCheatManager::UEqZeroCheatManager()
{
	DebugCameraControllerClass = AEqZeroDebugCameraController::StaticClass();
}

void UEqZeroCheatManager::InitCheatManager()
{
	Super::InitCheatManager();

#if WITH_EDITOR
	if (GIsEditor)
	{
		APlayerController* PC = GetOuterAPlayerController();
		for (const FEqZeroCheatToRun& CheatRow : GetDefault<UEqZeroDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == EEqZeroCheatExecutionTime::OnCheatManagerCreated)
			{
				PC->ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif

	if (EqZeroCheat::bStartInGodMode)
	{
		God();
	}
}

void UEqZeroCheatManager::CheatOutputText(const FString& TextToOutput)
{
#if USING_CHEAT_MANAGER
	// Output to the console.
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->ViewportConsole)
	{
		GEngine->GameViewport->ViewportConsole->OutputText(TextToOutput);
	}

	// Output to log.
	UE_LOG(LogEqZeroCheat, Display, TEXT("%s"), *TextToOutput);
#endif // USING_CHEAT_MANAGER
}

void UEqZeroCheatManager::Cheat(const FString& Msg)
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		EqZeroPC->ServerCheat(Msg.Left(128));
	}
}

void UEqZeroCheatManager::CheatAll(const FString& Msg)
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		EqZeroPC->ServerCheatAll(Msg.Left(128));
	}
}

void UEqZeroCheatManager::PlayNextGame()
{
	// UEqZeroSystemStatics::PlayNextGame(this);
}

void UEqZeroCheatManager::EnableDebugCamera()
{
	Super::EnableDebugCamera();
}

void UEqZeroCheatManager::DisableDebugCamera()
{
	FVector DebugCameraLocation;
	FRotator DebugCameraRotation;

	ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* OriginalPC = nullptr;

	if (DebugCC)
	{
		OriginalPC = DebugCC->OriginalControllerRef;
		DebugCC->GetPlayerViewPoint(DebugCameraLocation, DebugCameraRotation);
	}

	Super::DisableDebugCamera();

	if (OriginalPC && OriginalPC->PlayerCameraManager && (OriginalPC->PlayerCameraManager->CameraStyle == EqZeroCheat::NAME_Fixed))
	{
		OriginalPC->SetInitialLocationAndRotation(DebugCameraLocation, DebugCameraRotation);

		OriginalPC->PlayerCameraManager->ViewTarget.POV.Location = DebugCameraLocation;
		OriginalPC->PlayerCameraManager->ViewTarget.POV.Rotation = DebugCameraRotation;
		OriginalPC->PlayerCameraManager->PendingViewTarget.POV.Location = DebugCameraLocation;
		OriginalPC->PlayerCameraManager->PendingViewTarget.POV.Rotation = DebugCameraRotation;
	}
}

bool UEqZeroCheatManager::InDebugCamera() const
{
	return (Cast<ADebugCameraController>(GetOuter()) ? true : false);
}

void UEqZeroCheatManager::EnableFixedCamera()
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		PC->SetCameraMode(EqZeroCheat::NAME_Fixed);
	}
}

void UEqZeroCheatManager::DisableFixedCamera()
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		PC->SetCameraMode(NAME_Default);
	}
}

bool UEqZeroCheatManager::InFixedCamera() const
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	const APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		return (PC->PlayerCameraManager->CameraStyle == EqZeroCheat::NAME_Fixed);
	}

	return false;
}

void UEqZeroCheatManager::ToggleFixedCamera()
{
	if (InFixedCamera())
	{
		DisableFixedCamera();
	}
	else
	{
		EnableFixedCamera();
	}
}

void UEqZeroCheatManager::CycleDebugCameras()
{
	if (!EqZeroCheat::bEnableDebugCameraCycling)
	{
		return;
	}

	if (InDebugCamera())
	{
		EnableFixedCamera();
		DisableDebugCamera();
	}
	else if (InFixedCamera())
	{
		DisableFixedCamera();
		DisableDebugCamera();
	}
	else
	{
		EnableDebugCamera();
		DisableFixedCamera();
	}
}

void UEqZeroCheatManager::CycleAbilitySystemDebug()
{
	APlayerController* PC = Cast<APlayerController>(GetOuterAPlayerController());

	if (PC && PC->MyHUD)
	{
		if (!PC->MyHUD->bShowDebugInfo || !PC->MyHUD->DebugDisplay.Contains(TEXT("AbilitySystem")))
		{
			PC->MyHUD->ShowDebug(TEXT("AbilitySystem"));
		}

		PC->ConsoleCommand(TEXT("AbilitySystem.Debug.NextCategory"));
	}
}

void UEqZeroCheatManager::CancelActivatedAbilities()
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
	{
		const bool bReplicateCancelAbility = true;
		EqZeroASC->CancelInputActivatedAbilities(bReplicateCancelAbility);
	}
}

void UEqZeroCheatManager::AddTagToSelf(FString TagName)
{
	FGameplayTag Tag = EqZeroGameplayTags::FindTagByString(TagName, true);
	if (Tag.IsValid())
	{
		if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
		{
			EqZeroASC->AddDynamicTagGameplayEffect(Tag);
		}
	}
	else
	{
		UE_LOG(LogEqZeroCheat, Display, TEXT("AddTagToSelf: Could not find any tag matching [%s]."), *TagName);
	}
}

void UEqZeroCheatManager::RemoveTagFromSelf(FString TagName)
{
	FGameplayTag Tag = EqZeroGameplayTags::FindTagByString(TagName, true);
	if (Tag.IsValid())
	{
		if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
		{
			EqZeroASC->RemoveDynamicTagGameplayEffect(Tag);
		}
	}
	else
	{
		UE_LOG(LogEqZeroCheat, Display, TEXT("RemoveTagFromSelf: Could not find any tag matching [%s]."), *TagName);
	}
}

void UEqZeroCheatManager::DamageSelf(float DamageAmount)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
	{
		ApplySetByCallerDamage(EqZeroASC, DamageAmount);
	}
}

void UEqZeroCheatManager::DamageTarget(float DamageAmount)
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		if (EqZeroPC->GetNetMode() == NM_Client)
		{
			// Automatically send cheat to server for convenience.
			EqZeroPC->ServerCheat(FString::Printf(TEXT("DamageTarget %.2f"), DamageAmount));
			return;
		}

		FHitResult TargetHitResult;
		AActor* TargetActor = GetTarget(EqZeroPC, TargetHitResult);

		if (UEqZeroAbilitySystemComponent* EqZeroTargetASC = Cast<UEqZeroAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor)))
		{
			ApplySetByCallerDamage(EqZeroTargetASC, DamageAmount);
		}
	}
}

void UEqZeroCheatManager::ApplySetByCallerDamage(UEqZeroAbilitySystemComponent* EqZeroASC, float DamageAmount)
{
	check(EqZeroASC);

	TSubclassOf<UGameplayEffect> DamageGE = UEqZeroAssetManager::GetSubclass(UEqZeroGameData::Get().DamageGameplayEffect_SetByCaller);
	FGameplayEffectSpecHandle SpecHandle = EqZeroASC->MakeOutgoingSpec(DamageGE, 1.0f, EqZeroASC->MakeEffectContext());

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(EqZeroGameplayTags::SetByCaller_Damage, DamageAmount);
		EqZeroASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UEqZeroCheatManager::HealSelf(float HealAmount)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
	{
		ApplySetByCallerHeal(EqZeroASC, HealAmount);
	}
}

void UEqZeroCheatManager::HealTarget(float HealAmount)
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		FHitResult TargetHitResult;
		AActor* TargetActor = GetTarget(EqZeroPC, TargetHitResult);

		if (UEqZeroAbilitySystemComponent* EqZeroTargetASC = Cast<UEqZeroAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor)))
		{
			ApplySetByCallerHeal(EqZeroTargetASC, HealAmount);
		}
	}
}

void UEqZeroCheatManager::ApplySetByCallerHeal(UEqZeroAbilitySystemComponent* EqZeroASC, float HealAmount)
{
	check(EqZeroASC);

	TSubclassOf<UGameplayEffect> HealGE = UEqZeroAssetManager::GetSubclass(UEqZeroGameData::Get().HealGameplayEffect_SetByCaller);
	FGameplayEffectSpecHandle SpecHandle = EqZeroASC->MakeOutgoingSpec(HealGE, 1.0f, EqZeroASC->MakeEffectContext());

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(EqZeroGameplayTags::SetByCaller_Heal, HealAmount);
		EqZeroASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

UEqZeroAbilitySystemComponent* UEqZeroCheatManager::GetPlayerAbilitySystemComponent() const
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		return EqZeroPC->GetEqZeroAbilitySystemComponent();
	}
	return nullptr;
}

void UEqZeroCheatManager::DamageSelfDestruct()
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		if (const UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(EqZeroPC->GetPawn()))
		{
			if (PawnExtComp->HasReachedInitState(EqZeroGameplayTags::InitState_GameplayReady))
			{
				if (UEqZeroHealthComponent* HealthComponent = UEqZeroHealthComponent::FindHealthComponent(EqZeroPC->GetPawn()))
				{
					HealthComponent->DamageSelfDestruct();
				}
			}
		}
	}
}

void UEqZeroCheatManager::God()
{
	if (AEqZeroPlayerController* EqZeroPC = Cast<AEqZeroPlayerController>(GetOuterAPlayerController()))
	{
		if (EqZeroPC->GetNetMode() == NM_Client)
		{
			// Automatically send cheat to server for convenience.
			EqZeroPC->ServerCheat(FString::Printf(TEXT("God")));
			return;
		}

		if (UEqZeroAbilitySystemComponent* EqZeroASC = EqZeroPC->GetEqZeroAbilitySystemComponent())
		{
			const FGameplayTag Tag = EqZeroGameplayTags::Cheat_GodMode;
			const bool bHasTag = EqZeroASC->HasMatchingGameplayTag(Tag);

			if (bHasTag)
			{
				EqZeroASC->RemoveDynamicTagGameplayEffect(Tag);
			}
			else
			{
				EqZeroASC->AddDynamicTagGameplayEffect(Tag);
			}
		}
	}
}

void UEqZeroCheatManager::UnlimitedHealth(int32 Enabled)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetPlayerAbilitySystemComponent())
	{
		const FGameplayTag Tag = EqZeroGameplayTags::Cheat_UnlimitedHealth;
		const bool bHasTag = EqZeroASC->HasMatchingGameplayTag(Tag);

		if ((Enabled == -1) || ((Enabled > 0) && !bHasTag) || ((Enabled == 0) && bHasTag))
		{
			if (bHasTag)
			{
				EqZeroASC->RemoveDynamicTagGameplayEffect(Tag);
			}
			else
			{
				EqZeroASC->AddDynamicTagGameplayEffect(Tag);
			}
		}
	}
}
