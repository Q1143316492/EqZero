// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPlayerController.h"
#include "CommonInputTypeEnum.h"
#include "Components/PrimitiveComponent.h"
#include "EqZeroLogChannels.h"
#include "EqZeroCheatManager.h"
#include "EqZeroPlayerState.h"
#include "Camera/EqZeroPlayerCameraManager.h"
#include "UI/EqZeroHUD.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "EngineUtils.h"
#include "EqZeroGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "AbilitySystemGlobals.h"
#include "CommonInputSubsystem.h"
#include "EqZeroLocalPlayer.h"
#include "Settings/EqZeroSettingsShared.h"
#include "Teams/EqZeroTeamAgentInterface.h"
#include "../Development/EqZeroDeveloperSettings.h"
#include "GameMapsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerController)

namespace EqZero
{
	namespace Input
	{
		static int32 ShouldAlwaysPlayForceFeedback = 0;
		static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("EqZeroPC.ShouldAlwaysPlayForceFeedback"),
			ShouldAlwaysPlayForceFeedback,
			TEXT("Should force feedback effects be played, even if the last input device was not a gamepad?"));
	}
}

AEqZeroPlayerController::AEqZeroPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// PlayerCameraManagerClass = AEqZeroPlayerCameraManager::StaticClass(); // TODO

#if USING_CHEAT_MANAGER
	// CheatClass = UEqZeroCheatManager::StaticClass(); // TODO
#endif // #if USING_CHEAT_MANAGER
}

void AEqZeroPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// TODO 这里有个HTTP接口接收外部请求执行GM指令

	SetActorHiddenInGame(false);
}

void AEqZeroPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEqZeroPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 禁用复制 PC 目标视图，因为它在回放或客户端侧 spectating 时效果不佳。
	// 引擎的 TargetViewRotation 仅在 APlayerController::TickActor 中设置，前提是服务器提前知道
	// 正在 spectate 某个特定的 pawn，并且它仅针对 COND_OwnerOnly 进行向下复制。
	// 在客户端保存的回放中，COND_OwnerOnly 永远不会为 true，并且在录制时并不总是知道目标 pawn。
	// 为了支持客户端保存的回放，此复制已移至 ReplicatedViewRotation 并在 PlayerTick 中更新。
	DISABLE_REPLICATED_PROPERTY(APlayerController, TargetViewRotation);
}

void AEqZeroPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AEqZeroPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// If we are auto running then add some player input
	if (GetIsAutoRunning())
	{
		if (APawn* CurrentPawn = GetPawn())
		{
			const FRotator MovementRotation(0.0f, GetControlRotation().Yaw, 0.0f);
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			CurrentPawn->AddMovementInput(MovementDirection, 1.0f);
		}
	}

	AEqZeroPlayerState* EqZeroPlayerState = GetEqZeroPlayerState();

	if (PlayerCameraManager && EqZeroPlayerState)
	{
		APawn* TargetPawn = PlayerCameraManager->GetViewTargetPawn();

		if (TargetPawn)
		{
			// Update view rotation on the server so it replicates
			if (HasAuthority() || TargetPawn->IsLocallyControlled())
			{
				EqZeroPlayerState->SetReplicatedViewRotation(TargetPawn->GetViewRotation());
			}

			// Update the target view rotation if the pawn isn't locally controlled
			if (!TargetPawn->IsLocallyControlled())
			{
				EqZeroPlayerState = TargetPawn->GetPlayerState<AEqZeroPlayerState>();
				if (EqZeroPlayerState)
				{
					// Get it from the spectated pawn's player state, which may not be the same as the PC's playerstate
					TargetViewRotation = EqZeroPlayerState->GetReplicatedViewRotation();
				}
			}
		}
	}
}

AEqZeroPlayerState* AEqZeroPlayerController::GetEqZeroPlayerState() const
{
	return CastChecked<AEqZeroPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UEqZeroAbilitySystemComponent* AEqZeroPlayerController::GetEqZeroAbilitySystemComponent() const
{
	const AEqZeroPlayerState* EqZeroPS = GetEqZeroPlayerState();
	return (EqZeroPS ? EqZeroPS->GetEqZeroAbilitySystemComponent() : nullptr);
}

AEqZeroHUD* AEqZeroPlayerController::GetEqZeroHUD() const
{
	return CastChecked<AEqZeroHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}

void AEqZeroPlayerController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void AEqZeroPlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IEqZeroTeamAgentInterface* PlayerStateTeamInterface = Cast<IEqZeroTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IEqZeroTeamAgentInterface* PlayerStateTeamInterface = Cast<IEqZeroTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void AEqZeroPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AEqZeroPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AEqZeroPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();

	// When we're a client connected to a remote server, the player controller may replicate later than the PlayerState and AbilitySystemComponent.
	// However, TryActivateAbilitiesOnSpawn depends on the player controller being replicated in order to check whether on-spawn abilities should
	// execute locally. Therefore once the PlayerController exists and has resolved the PlayerState, try once again to activate on-spawn abilities.
	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (AEqZeroPlayerState* EqZeroPS = GetPlayerState<AEqZeroPlayerState>())
		{
			if (UEqZeroAbilitySystemComponent* EqZeroASC = EqZeroPS->GetEqZeroAbilitySystemComponent())
			{
				EqZeroASC->RefreshAbilityActorInfo();
				EqZeroASC->TryActivateAbilitiesOnSpawn();
			}
		}
	}
}

void AEqZeroPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);

	if (const UEqZeroLocalPlayer* EqZeroLocalPlayer = Cast<UEqZeroLocalPlayer>(InPlayer))
	{
		UEqZeroSettingsShared* UserSettings = EqZeroLocalPlayer->GetSharedSettings();
		UserSettings->OnSettingChanged.AddUObject(this, &ThisClass::OnSettingsChanged);

		OnSettingsChanged(UserSettings);
	}
}

// TODO
// void AEqZeroPlayerController::OnSettingsChanged(UEqZeroSettingsShared* InSettings)
// {
// 	bForceFeedbackEnabled = InSettings->GetForceFeedbackEnabled();
// }

void AEqZeroPlayerController::AddCheats(bool bForce)
{
#if USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else //#if USING_CHEAT_MANAGER
	Super::AddCheats(bForce);
#endif // #else //#if USING_CHEAT_MANAGER
}

void AEqZeroPlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogEqZero, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AEqZeroPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AEqZeroPlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogEqZero, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<AEqZeroPlayerController> It(GetWorld()); It; ++It)
		{
			AEqZeroPlayerController* EqZeroPC = (*It);
			if (EqZeroPC)
			{
				EqZeroPC->ClientMessage(EqZeroPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AEqZeroPlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void AEqZeroPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void AEqZeroPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AEqZeroPlayerController::OnCameraPenetratingTarget()
{
	bHideViewTargetPawnNextFrame = true;
}

void AEqZeroPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

#if WITH_SERVER_CODE && WITH_EDITOR
	if (GIsEditor && (InPawn != nullptr) && (GetPawn() == InPawn))
	{
		for (const FEqZeroCheatToRun& CheatRow : GetDefault<UEqZeroDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == EEqZeroCheatExecutionTime::OnPlayerPawnPossession)
			{
				ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif

	SetIsAutoRunning(false);
}

void AEqZeroPlayerController::SetIsAutoRunning(const bool bEnabled)
{
	const bool bIsAutoRunning = GetIsAutoRunning();
	if (bEnabled != bIsAutoRunning)
	{
		if (!bEnabled)
		{
			OnEndAutoRun();
		}
		else
		{
			OnStartAutoRun();
		}
	}
}

bool AEqZeroPlayerController::GetIsAutoRunning() const
{
	bool bIsAutoRunning = false;
	if (const UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		bIsAutoRunning = EqZeroASC->GetTagCount(EqZeroGameplayTags::Status_AutoRunning) > 0;
	}
	return bIsAutoRunning;
}

void AEqZeroPlayerController::OnStartAutoRun()
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_AutoRunning, 1);
		K2_OnStartAutoRun();
	}
}

void AEqZeroPlayerController::OnEndAutoRun()
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_AutoRunning, 0);
		K2_OnEndAutoRun();
	}
}

void AEqZeroPlayerController::UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId)
{
	if (bForceFeedbackEnabled)
	{
		if (const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
		{
			const ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
			if (EqZero::Input::ShouldAlwaysPlayForceFeedback || CurrentInputType == ECommonInputType::Gamepad || CurrentInputType == ECommonInputType::Touch)
			{
				InputInterface->SetForceFeedbackChannelValues(ControllerId, ForceFeedbackValues);
				return;
			}
		}
	}

	InputInterface->SetForceFeedbackChannelValues(ControllerId, FForceFeedbackValues());
}

void AEqZeroPlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, OutHiddenComponents);

	if (bHideViewTargetPawnNextFrame)
	{
		AActor* const ViewTargetPawn = PlayerCameraManager ? Cast<AActor>(PlayerCameraManager->GetViewTarget()) : nullptr;
		if (ViewTargetPawn)
		{
			// internal helper func to hide all the components
			auto AddToHiddenComponents = [&OutHiddenComponents](const TInlineComponentArray<UPrimitiveComponent*>& InComponents)
			{
				// add every component and all attached children
				for (UPrimitiveComponent* Comp : InComponents)
				{
					if (Comp->IsRegistered())
					{
						OutHiddenComponents.Add(Comp->GetPrimitiveSceneId());

						for (USceneComponent* AttachedChild : Comp->GetAttachChildren())
						{
							static FName NAME_NoParentAutoHide(TEXT("NoParentAutoHide"));
							UPrimitiveComponent* AttachChildPC = Cast<UPrimitiveComponent>(AttachedChild);
							if (AttachChildPC && AttachChildPC->IsRegistered() && !AttachChildPC->ComponentTags.Contains(NAME_NoParentAutoHide))
							{
								OutHiddenComponents.Add(AttachChildPC->GetPrimitiveSceneId());
							}
						}
					}
				}
			};

			// hide pawn's components
			TInlineComponentArray<UPrimitiveComponent*> PawnComponents;
			ViewTargetPawn->GetComponents(PawnComponents);
			AddToHiddenComponents(PawnComponents);
		}

		// we consumed it, reset for next frame
		bHideViewTargetPawnNextFrame = false;
	}
}

void AEqZeroPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

void AEqZeroPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogEqZero, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AEqZeroPlayerController::GetGenericTeamId() const
{
	if (const IEqZeroTeamAgentInterface* PSWithTeamInterface = Cast<IEqZeroTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnEqZeroTeamIndexChangedDelegate& AEqZeroPlayerController::GetTeamChangedDelegateChecked()
{
	return OnTeamChangedDelegate;
}

void AEqZeroPlayerController::OnPlayerStateChangedTeam(UObject* Producer, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
}

void AEqZeroPlayerController::OnSettingsChanged(UEqZeroSettingsShared* Settings)
{
	if (IsLocalPlayerController())
	{
		// Update settings
	}
}

void AEqZeroPlayerController::ConditionalBroadcastTeamChanged(TScriptInterface<IEqZeroTeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		OnTeamChangedDelegate.Broadcast(This.GetObject(), OldTeamID, NewTeamID);
	}
}
