// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCharacter.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EqZeroGameplayTags.h"
#include "EqZeroLogChannels.h"
#include "EqZeroCharacterMovementComponent.h"
#include "EqZeroPawnExtensionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/EqZeroPlayerController.h"
#include "Player/EqZeroPlayerState.h"
#include "TimerManager.h"

// TODO: Migrate these classes
// #include "Camera/EqZeroCameraComponent.h"
#include "Character/EqZeroHealthComponent.h"
#include "Character/EqZeroPawnExtensionComponent.h"
#include "EqZeroCharacterMovementComponent.h"
// #include "System/EqZeroSignificanceManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCharacter)

class AActor;
class FLifetimeProperty;
class IRepChangedPropertyTracker;
class UInputComponent;

static FName NAME_EqZeroCharacterCollisionProfile_Capsule(TEXT("EqZeroPawnCapsule"));
static FName NAME_EqZeroCharacterCollisionProfile_Mesh(TEXT("EqZeroPawnMesh"));

AEqZeroCharacter::AEqZeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UEqZeroCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetNetCullDistanceSquared(900000000.0f);

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetCollisionProfileName(NAME_EqZeroCharacterCollisionProfile_Capsule);

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.
	MeshComp->SetCollisionProfileName(NAME_EqZeroCharacterCollisionProfile_Mesh);

	UEqZeroCharacterMovementComponent* EqZeroMoveComp = CastChecked<UEqZeroCharacterMovementComponent>(GetCharacterMovement());
	EqZeroMoveComp->GravityScale = 1.0f;
	EqZeroMoveComp->MaxAcceleration = 2400.0f;
	EqZeroMoveComp->BrakingFrictionFactor = 1.0f;
	EqZeroMoveComp->BrakingFriction = 6.0f;
	EqZeroMoveComp->GroundFriction = 8.0f;
	EqZeroMoveComp->BrakingDecelerationWalking = 1400.0f;
	EqZeroMoveComp->bUseControllerDesiredRotation = false;
	EqZeroMoveComp->bOrientRotationToMovement = false;
	EqZeroMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	EqZeroMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	EqZeroMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	EqZeroMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	EqZeroMoveComp->SetCrouchedHalfHeight(65.0f);

	PawnExtComponent = CreateDefaultSubobject<UEqZeroPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	HealthComponent = CreateDefaultSubobject<UEqZeroHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	// TODO: Create CameraComponent when UEqZeroCameraComponent is available
	// CameraComponent = CreateDefaultSubobject<UEqZeroCameraComponent>(TEXT("CameraComponent"));
	// CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	BaseEyeHeight = 80.0f;
	CrouchedEyeHeight = 50.0f;
}

void AEqZeroCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AEqZeroCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AEqZeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AEqZeroCharacter::Reset()
{
	DisableMovementAndCollision();

	K2_OnReset();

	UninitAndDestroy();
}

void AEqZeroCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
}

void AEqZeroCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians   = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ           = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}
}

void AEqZeroCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
}

AEqZeroPlayerController* AEqZeroCharacter::GetEqZeroPlayerController() const
{
	return CastChecked<AEqZeroPlayerController>(GetController(), ECastCheckedType::NullAllowed);
}

AEqZeroPlayerState* AEqZeroCharacter::GetEqZeroPlayerState() const
{
	return CastChecked<AEqZeroPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UEqZeroAbilitySystemComponent* AEqZeroCharacter::GetEqZeroAbilitySystemComponent() const
{
	return Cast<UEqZeroAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* AEqZeroCharacter::GetAbilitySystemComponent() const
{
	if (PawnExtComponent == nullptr)
	{
		return nullptr;
	}
	return PawnExtComponent->GetEqZeroAbilitySystemComponent();
}

void AEqZeroCharacter::OnAbilitySystemInitialized()
{
	UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent();
	check(EqZeroASC);
	HealthComponent->InitializeWithAbilitySystem(EqZeroASC);

	InitializeGameplayTags();
}

void AEqZeroCharacter::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AEqZeroCharacter::PossessedBy(AController* NewController)
{

	Super::PossessedBy(NewController);
	PawnExtComponent->HandleControllerChanged();
}

void AEqZeroCharacter::UnPossessed()
{
	Super::UnPossessed();
}

void AEqZeroCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	PawnExtComponent->HandleControllerChanged();
}

void AEqZeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	PawnExtComponent->HandlePlayerStateReplicated();
}

void AEqZeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AEqZeroCharacter::InitializeGameplayTags()
{
	// Clear tags that may be lingering on the ability system from the previous pawn.
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		for (const TPair<uint8, FGameplayTag>& TagMapping : EqZeroGameplayTags::MovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				EqZeroASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		for (const TPair<uint8, FGameplayTag>& TagMapping : EqZeroGameplayTags::CustomMovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				EqZeroASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		UEqZeroCharacterMovementComponent* EqZeroMoveComp = CastChecked<UEqZeroCharacterMovementComponent>(GetCharacterMovement());
		SetMovementModeTag(EqZeroMoveComp->MovementMode, EqZeroMoveComp->CustomMovementMode, true);
	}
}

void AEqZeroCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool AEqZeroCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		return EqZeroASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool AEqZeroCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		return EqZeroASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool AEqZeroCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		return EqZeroASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

// 掉出边界就死了
void AEqZeroCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
}

void AEqZeroCharacter::OnDeathStarted(AActor*)
{
	DisableMovementAndCollision();
}

void AEqZeroCharacter::OnDeathFinished(AActor*)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}


void AEqZeroCharacter::DisableMovementAndCollision()
{
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (UEqZeroCharacterMovementComponent* MoveComp = Cast<UEqZeroCharacterMovementComponent>(GetCharacterMovement()))
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

void AEqZeroCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}


void AEqZeroCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		if (EqZeroASC->GetAvatarActor() == this)
		{
			PawnExtComponent->UninitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}

void AEqZeroCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (UEqZeroCharacterMovementComponent* MoveComp = Cast<UEqZeroCharacterMovementComponent>(GetCharacterMovement()))
	{
		SetMovementModeTag(PrevMovementMode, PreviousCustomMode, false);
		SetMovementModeTag(MoveComp->MovementMode, MoveComp->CustomMovementMode, true);
	}
}

void AEqZeroCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		const FGameplayTag* MovementModeTag = nullptr;
		if (MovementMode == MOVE_Custom)
		{
			MovementModeTag = EqZeroGameplayTags::CustomMovementModeTagMap.Find(CustomMovementMode);
		}
		else
		{
			MovementModeTag = EqZeroGameplayTags::MovementModeTagMap.Find(MovementMode);
		}

		if (MovementModeTag && MovementModeTag->IsValid())
		{
			EqZeroASC->SetLooseGameplayTagCount(*MovementModeTag, (bTagEnabled ? 1 : 0));
		}
	}
}

void AEqZeroCharacter::ToggleCrouch()
{
	const UEqZeroCharacterMovementComponent* MoveComp = Cast<UEqZeroCharacterMovementComponent>(GetCharacterMovement());
	
	if (IsCrouched() || MoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (MoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void AEqZeroCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Crouching, 1);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AEqZeroCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UEqZeroAbilitySystemComponent* EqZeroASC = GetEqZeroAbilitySystemComponent())
	{
		EqZeroASC->SetLooseGameplayTagCount(EqZeroGameplayTags::Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool AEqZeroCharacter::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}

void AEqZeroCharacter::OnRep_ReplicatedAcceleration()
{
	if (UEqZeroCharacterMovementComponent* MovementComponent = Cast<UEqZeroCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel         = MovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians   = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]

		MovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void AEqZeroCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// TODO: Implement team assignment logic
}

FGenericTeamId AEqZeroCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnEqZeroTeamIndexChangedDelegate* AEqZeroCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AEqZeroCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	// TODO: Implement team replication response
}

bool AEqZeroCharacter::UpdateSharedReplication()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		FSharedRepMovement SharedMovement;
		if (SharedMovement.FillForCharacter(this))
		{
			// Only call FastSharedReplication if data has changed since the last frame.
			// Skipping this call will cause replication to reuse the same bunch that we previously
			// produced, but not send it to clients that already received. (But a new client who has not received
			// it, will get it this frame)
			if (!SharedMovement.Equals(LastSharedReplication, this))
			{
				LastSharedReplication = SharedMovement;
				SetReplicatedMovementMode(SharedMovement.RepMovementMode);

				FastSharedReplication(SharedMovement);
			}
			return true;
		}
	}

	// We cannot fastrep right now. Don't send anything.
	return false;
}

void AEqZeroCharacter::FastSharedReplication_Implementation(const FSharedRepMovement& SharedRepMovement)
{
	if (GetWorld()->IsPlayingReplay())
	{
		return;
	}

	// Timestamp is checked to reject old moves.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		// Timestamp
		SetReplicatedServerLastTransformUpdateTimeStamp(SharedRepMovement.RepTimeStamp);

		// Movement mode
		if (GetReplicatedMovementMode() != SharedRepMovement.RepMovementMode)
		{
			SetReplicatedMovementMode(SharedRepMovement.RepMovementMode);
			GetCharacterMovement()->bNetworkMovementModeChanged = true;
			GetCharacterMovement()->bNetworkUpdateReceived = true;
		}

		// Location, Rotation, Velocity, etc.
		FRepMovement& MutableRepMovement = GetReplicatedMovement_Mutable();
		MutableRepMovement = SharedRepMovement.RepMovement;

		// This also sets LastRepMovement
		OnRep_ReplicatedMovement();

		// Jump force
		SetProxyIsJumpForceApplied(SharedRepMovement.bProxyIsJumpForceApplied);

		// Crouch
		if (IsCrouched() != SharedRepMovement.bIsCrouched)
		{
			SetIsCrouched(SharedRepMovement.bIsCrouched);
			OnRep_IsCrouched();
		}
	}
}

FSharedRepMovement::FSharedRepMovement()
{
	RepMovement.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
}

bool FSharedRepMovement::FillForCharacter(ACharacter* Character)
{
	if (USceneComponent* PawnRootComponent = Character->GetRootComponent())
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();

		RepMovement.Location = FRepMovement::RebaseOntoZeroOrigin(PawnRootComponent->GetComponentLocation(), Character);
		RepMovement.Rotation = PawnRootComponent->GetComponentRotation();
		RepMovement.LinearVelocity = CharacterMovement->Velocity;
		RepMovementMode = CharacterMovement->PackNetworkMovementMode();
		bProxyIsJumpForceApplied = Character->GetProxyIsJumpForceApplied() || (Character->JumpForceTimeRemaining > 0.0f);
		bIsCrouched = Character->IsCrouched();

		// Timestamp is sent as zero if unused
		if ((CharacterMovement->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || CharacterMovement->bNetworkAlwaysReplicateTransformUpdateTimestamp)
		{
			RepTimeStamp = CharacterMovement->GetServerLastTransformUpdateTimeStamp();
		}
		else
		{
			RepTimeStamp = 0.f;
		}

		return true;
	}
	return false;
}

bool FSharedRepMovement::Equals(const FSharedRepMovement& Other, ACharacter* Character) const
{
	if (RepMovement.Location != Other.RepMovement.Location)
	{
		return false;
	}

	if (RepMovement.Rotation != Other.RepMovement.Rotation)
	{
		return false;
	}

	if (RepMovement.LinearVelocity != Other.RepMovement.LinearVelocity)
	{
		return false;
	}

	if (RepMovementMode != Other.RepMovementMode)
	{
		return false;
	}

	if (bProxyIsJumpForceApplied != Other.bProxyIsJumpForceApplied)
	{
		return false;
	}

	if (bIsCrouched != Other.bIsCrouched)
	{
		return false;
	}

	return true;
}

bool FSharedRepMovement::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	RepMovement.NetSerialize(Ar, Map, bOutSuccess);
	Ar << RepMovementMode;
	Ar << bProxyIsJumpForceApplied;
	Ar << bIsCrouched;

	// Timestamp, if non-zero.
	uint8 bHasTimeStamp = (RepTimeStamp != 0.f);
	Ar.SerializeBits(&bHasTimeStamp, 1);
	if (bHasTimeStamp)
	{
		Ar << RepTimeStamp;
	}
	else
	{
		RepTimeStamp = 0.f;
	}

	return true;
}
