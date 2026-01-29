// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroHeroComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Logging/MessageLog.h"
#include "EqZeroLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "Player/EqZeroPlayerController.h"
#include "Player/EqZeroPlayerState.h"
#include "Player/EqZeroLocalPlayer.h"
#include "EqZeroPawnExtensionComponent.h"
#include "EqZeroPawnData.h"
#include "EqZeroCharacter.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Input/EqZeroInputConfig.h"
#include "Input/EqZeroInputComponent.h"
// #include "Camera/EqZeroCameraComponent.h" // TODO: Implement this class
#include "EqZeroGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "PlayerMappableInputConfig.h"
#include "Camera/EqZeroCameraMode.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace EqZeroHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UEqZeroHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UEqZeroHeroComponent::NAME_ActorFeatureName("Hero");

UEqZeroHeroComponent::UEqZeroHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;
}

void UEqZeroHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogEqZero, Error, TEXT("[UEqZeroHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));
#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("EqZeroHeroComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("EqZeroHeroComponent");

			FMessageLog(HeroMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));

			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool UEqZeroHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == EqZeroGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == EqZeroGameplayTags::InitState_Spawned && DesiredState == EqZeroGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<AEqZeroPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AEqZeroPlayerController* EqZeroPC = GetController<AEqZeroPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !EqZeroPC || !EqZeroPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == EqZeroGameplayTags::InitState_DataAvailable && DesiredState == EqZeroGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AEqZeroPlayerState* EqZeroPS = GetPlayerState<AEqZeroPlayerState>();

		return EqZeroPS && Manager->HasFeatureReachedInitState(Pawn, UEqZeroPawnExtensionComponent::NAME_ActorFeatureName, EqZeroGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == EqZeroGameplayTags::InitState_DataInitialized && DesiredState == EqZeroGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UEqZeroHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == EqZeroGameplayTags::InitState_DataAvailable && DesiredState == EqZeroGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AEqZeroPlayerState* EqZeroPS = GetPlayerState<AEqZeroPlayerState>();
		if (!ensure(Pawn && EqZeroPS))
		{
			return;
		}

		const UEqZeroPawnData* PawnData = nullptr;

		if (UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UEqZeroPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(EqZeroPS->GetEqZeroAbilitySystemComponent(), EqZeroPS);
		}

		if (AEqZeroPlayerController* EqZeroPC = GetController<AEqZeroPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		// Hook up the delegate for all pawns, in case we spectate later
		if (PawnData)
		{
            // TODO: EqZeroCameraComponent missing
			/*
			if (UEqZeroCameraComponent* CameraComponent = UEqZeroCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
            */
		}
	}
}

void UEqZeroHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UEqZeroPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == EqZeroGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UEqZeroHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { EqZeroGameplayTags::InitState_Spawned, EqZeroGameplayTags::InitState_DataAvailable, EqZeroGameplayTags::InitState_DataInitialized, EqZeroGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UEqZeroHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UEqZeroPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(EqZeroGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UEqZeroHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UEqZeroHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UEqZeroLocalPlayer* LP = Cast<UEqZeroLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UEqZeroPawnData* PawnData = PawnExtComp->GetPawnData<UEqZeroPawnData>())
		{
			if (const UEqZeroInputConfig* InputConfig = Cast<UEqZeroInputConfig>(PawnData->InputConfig))
			{
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);
							}

							FModifyContextOptions Options = {};
							Options.bIgnoreAllPressedKeysUntilRelease = false;
							// Actually add the config to the local player
							Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
						}
					}
				}

				UEqZeroInputComponent* EqZeroIC = Cast<UEqZeroInputComponent>(PlayerInputComponent);
				if (ensureMsgf(EqZeroIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UEqZeroInputComponent or a subclass of it.")))
				{
                    // mapping
					EqZeroIC->AddInputMappings(InputConfig, Subsystem);

                    // 技能输入
					TArray<uint32> BindHandles;
					EqZeroIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

                    // 运动输入
					EqZeroIC->BindNativeAction(InputConfig, EqZeroGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					EqZeroIC->BindNativeAction(InputConfig, EqZeroGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					EqZeroIC->BindNativeAction(InputConfig, EqZeroGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
					EqZeroIC->BindNativeAction(InputConfig, EqZeroGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
					EqZeroIC->BindNativeAction(InputConfig, EqZeroGameplayTags::InputTag_AutoRun, ETriggerEvent::Triggered, this, &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UEqZeroHeroComponent::AddAdditionalInputConfig(const UEqZeroInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UEqZeroInputComponent* EqZeroIC = Pawn->FindComponentByClass<UEqZeroInputComponent>();
		if (ensureMsgf(EqZeroIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UEqZeroInputComponent or a subclass of it.")))
		{
			EqZeroIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UEqZeroHeroComponent::RemoveAdditionalInputConfig(const UEqZeroInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UEqZeroHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UEqZeroHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>() )
	{
		if (const UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UEqZeroAbilitySystemComponent* EqZeroASC = PawnExtComp->GetEqZeroAbilitySystemComponent())
			{
				EqZeroASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UEqZeroHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UEqZeroAbilitySystemComponent* EqZeroASC = PawnExtComp->GetEqZeroAbilitySystemComponent())
		{
			EqZeroASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UEqZeroHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	if (AEqZeroPlayerController* EqZeroController = Cast<AEqZeroPlayerController>(Controller))
	{
		EqZeroController->SetIsAutoRunning(false);
	}

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UEqZeroHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UEqZeroHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * EqZeroHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * EqZeroHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UEqZeroHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (AEqZeroCharacter* Character = GetPawn<AEqZeroCharacter>())
	{
		Character->ToggleCrouch();
	}
}

void UEqZeroHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (AEqZeroPlayerController* Controller = Cast<AEqZeroPlayerController>(Pawn->GetController()))
		{
			Controller->SetIsAutoRunning(!Controller->GetIsAutoRunning());
		}
	}
}

TSubclassOf<UEqZeroCameraMode> UEqZeroHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (UEqZeroPawnExtensionComponent* PawnExtComp = UEqZeroPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UEqZeroPawnData* PawnData = PawnExtComp->GetPawnData<UEqZeroPawnData>())
		{
			return Cast<UClass>(PawnData->DefaultCameraMode);
		}
	}

	return nullptr;
}

void UEqZeroHeroComponent::SetAbilityCameraMode(TSubclassOf<UEqZeroCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void UEqZeroHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}