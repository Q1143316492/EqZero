// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroAnimInstance.h"
#include "AbilitySystemGlobals.h"
#include "Character/EqZeroCharacter.h"
#include "Character/EqZeroCharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroAnimInstance)


struct FEqZeroCharacterGroundInfo;
class UEqZeroCharacterMovementComponent;
class AEqZeroCharacter;

UEqZeroAnimInstance::UEqZeroAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	GameplayTagPropertyMap.Initialize(this, ASC);
}

#if WITH_EDITOR
EDataValidationResult UEqZeroAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	GameplayTagPropertyMap.IsDataValid(this, Context);

	return ((Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif // WITH_EDITOR

void UEqZeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}

	Character = Cast<AEqZeroCharacter>(TryGetPawnOwner());
}

void UEqZeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character.IsValid())
	{
		return;
	}
	
	UEqZeroCharacterMovementComponent* CharMoveComp = CastChecked<UEqZeroCharacterMovementComponent>(Character->GetCharacterMovement());
	const FEqZeroCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	GroundDistance = GroundInfo.GroundDistance;

	UpdateData(DeltaSeconds);
}

void UEqZeroAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}

void UEqZeroAnimInstance::UpdateData(float DeltaSeconds)
{
	if (DeltaSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData();
	UpdateAccelerationData();
	UpdateWallDetectionHeuristic();
	UpdateCharacterStateData(DeltaSeconds);
	UpdateBlendWeightData(DeltaSeconds);
	UpdateRootYawOffset(DeltaSeconds);
	UpdateAimingData();
	UpdateJumpFallData();

	IsFirstUpdate = false;
}

void UEqZeroAnimInstance::UpdateLocationData(float DeltaSeconds)
{
	// 距离上一次更新 位移 和 速度
	DisplacementSinceLastUpdate = (Character->GetActorLocation() - WorldLocation).Size();
	WorldLocation = Character->GetActorLocation();
	DisplacementSpeed = DisplacementSinceLastUpdate / DeltaSeconds;
	if (IsFirstUpdate)
	{
		DisplacementSinceLastUpdate = DisplacementSpeed = 0.f;
	}
}

void UEqZeroAnimInstance::UpdateRotationData()
{
	// Yaw 变化量 和 速度
	FRotator NewWorldRotation = Character->GetActorRotation();
	YawDeltaSinceLastUpdate = UKismetMathLibrary::NormalizedDeltaRotator(NewWorldRotation, WorldRotation).Yaw;
	YawDeltaSpeed = YawDeltaSinceLastUpdate / GetWorld()->GetDeltaSeconds();
	WorldRotation = NewWorldRotation;

	AdditiveLeanAngle = YawDeltaSpeed * 0.0375; // TODO tmp value

	if (IsFirstUpdate)
	{
		YawDeltaSinceLastUpdate = AdditiveLeanAngle = 0.f;
		// YawDeltaSpeed 不需要忽略，我们需要至少两次update来计算delta
	}
}

void UEqZeroAnimInstance::UpdateVelocityData()
{
	bool WasMovingLastUpdate = LocalVelocity2D.SizeSquared() > KINDA_SMALL_NUMBER;
	
	WorldVelocity = Character->GetVelocity();
	FVector WorldVelocity2D = WorldVelocity * FVector(1.0f, 1.0f, 0.0f);

	// Local
	LocalVelocity2D = WorldRotation.UnrotateVector(WorldVelocity2D);
	LocalVelocityDirectionAngle = UKismetAnimationLibrary::CalculateDirection(WorldVelocity2D, WorldRotation);
	LocalVelocityDirectionAngleWithOffset = LocalVelocityDirectionAngle - RootYawOffset;

	LocalVelocityDirection = SelectCardinalDirectionFromAngle(
		LocalVelocityDirectionAngleWithOffset,
		CardinalDirectionDeadZone,
		LocalVelocityDirection,
		WasMovingLastUpdate);

	LocalVelocityDirectionNoOffset = SelectCardinalDirectionFromAngle(
		LocalVelocityDirectionAngle,
		CardinalDirectionDeadZone,
		LocalVelocityDirectionNoOffset,
		WasMovingLastUpdate);

	HasVelocity = WorldVelocity2D.SizeSquared() > KINDA_SMALL_NUMBER;

}

EEqZeroCardinalDirection UEqZeroAnimInstance::SelectCardinalDirectionFromAngle(
	float Angle,
	float DeadZone,
	EEqZeroCardinalDirection CurrentDirection,
	bool UseCurrentDirection)
{
	float AbsAngle = FMath::Abs(Angle);
	float FwdDeadZone = DeadZone;
	float BwdDeadZone = DeadZone;

	// 是否扩大当前方向的死区
	if (UseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case EEqZeroCardinalDirection::Forward:
			FwdDeadZone = FwdDeadZone * 2;
			break;
		case EEqZeroCardinalDirection::Backward:
			BwdDeadZone = BwdDeadZone * 2;
			break;
		default:
			break;
		}
	}

	// 方向
	if (AbsAngle <= 45 + FwdDeadZone)
	{
		return EEqZeroCardinalDirection::Forward;
	}
	else if (AbsAngle >= 135 - BwdDeadZone)
	{
		return EEqZeroCardinalDirection::Backward;
	}
	else
	{
		if (Angle > 0)
		{
			return EEqZeroCardinalDirection::Right;
		}
		else
		{
			return EEqZeroCardinalDirection::Left;
		}
	}
}

void UEqZeroAnimInstance::UpdateAccelerationData()
{
	FVector WorldAcceleration2D = Character->GetCharacterMovement()->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	LocalAcceleration2D = WorldRotation.UnrotateVector(WorldAcceleration2D);
	HasAcceleration = WorldAcceleration2D.SizeSquared() > KINDA_SMALL_NUMBER;

	// 根据加速度计算出一个基准方向，用于确定转身方向。加速度比速度更能准确地传达角色的意图。
	if (HasAcceleration)
	{
		PivotDirection2D = FMath::Lerp(PivotDirection2D, WorldAcceleration2D.GetSafeNormal(), 0.5f).GetSafeNormal();
		float Direction = UKismetAnimationLibrary::CalculateDirection(PivotDirection2D, WorldRotation);

		EEqZeroCardinalDirection TempDirection = SelectCardinalDirectionFromAngle(Direction, CardinalDirectionDeadZone, EEqZeroCardinalDirection::Forward, false);
		CardinalDirectionFromAcceleration = GetOppositeCardinalDirection(TempDirection);
	}
}

EEqZeroCardinalDirection UEqZeroAnimInstance::GetOppositeCardinalDirection(EEqZeroCardinalDirection Direction) const
{
	switch (Direction)
	{
	case EEqZeroCardinalDirection::Forward:
		return EEqZeroCardinalDirection::Backward;
	case EEqZeroCardinalDirection::Backward:
		return EEqZeroCardinalDirection::Forward;
	case EEqZeroCardinalDirection::Left:
		return EEqZeroCardinalDirection::Right;
	case EEqZeroCardinalDirection::Right:
		return EEqZeroCardinalDirection::Left;
	default:
		return EEqZeroCardinalDirection::Forward;
	}
}

void UEqZeroAnimInstance::UpdateWallDetectionHeuristic()
{
	// 该逻辑通过检查速度和加速度之间是否存在大角度（即角色正朝着墙壁推进，但实际上却在向侧面滑动）以及角色是否在尝试加速但速度相对较低，来判断角色是否正在撞到墙上。

	IsRunningIntoWall = false;
	if (LocalAcceleration2D.Size() > 0.1f && LocalVelocity2D.Size() < 200.0f)
	{
		float Dot = FVector::DotProduct(LocalAcceleration2D.GetSafeNormal(), LocalVelocity2D.GetSafeNormal());
		// Check if angle is in [-0.6, 0.6] range (approx 53 to 127 degrees)
		// implying pushing into wall but sliding
		if (Dot >= -0.6f && Dot <= 0.6f)
		{
			IsRunningIntoWall = true;
		}
	}
}

void UEqZeroAnimInstance::UpdateCharacterStateData(float DeltaSeconds)
{
	// movement states
	bool WasCrouchingLastUpdate = IsCrouching;
	if (auto CharMoveComp = CastChecked<UEqZeroCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		IsOnGround = CharMoveComp->IsMovingOnGround();
		IsCrouching = CharMoveComp->IsCrouching();
		CrouchStateChange = (IsCrouching != WasCrouchingLastUpdate);

		IsJumping = IsFalling = false;
		if (CharMoveComp->MovementMode == MOVE_Falling && WorldVelocity.Z > 0.f)
		{
			IsFalling = IsJumping = true;
		}
	}

	// ADS
	ADSStateChanged = (GameplayTag_IsADS != WasADSLastUpdate);
	WasADSLastUpdate = GameplayTag_IsADS;

	// Weapon fired state
	if (GameplayTag_IsFiring)
	{
		TimeSinceFiredWeapon = 0.f;
	}
	else
	{
		TimeSinceFiredWeapon += DeltaSeconds;
	}
}

void UEqZeroAnimInstance::UpdateBlendWeightData(float DeltaSeconds)
{
	// 在播放蒙太奇且在地面时，应用上半身动态加成权重。否则，平滑地将其插值回0。
	const bool bShouldApply = IsAnyMontagePlaying() && IsOnGround;

	if (bShouldApply)
	{
		UpperbodyDynamicAdditiveWeight = 1.0f;
	}
	else
	{
		UpperbodyDynamicAdditiveWeight = FMath::FInterpTo(UpperbodyDynamicAdditiveWeight, 0.0f, DeltaSeconds, 6.0f);
	}
}

void UEqZeroAnimInstance::UpdateRootYawOffset(float DeltaSeconds)
{
	// 当脚部不移动时（例如在空闲期间），将根节点沿与 Pawn 所有者旋转方向相反的方向偏移，以防止网格随 Pawn 一起旋转。
	if (RootYawOffsetMode == EEqZeroRootYawOffsetMode::Accumulate)
	{
		SetRootYawOffset(RootYawOffset - YawDeltaSinceLastUpdate);
	}
	
	// 当角色在移动时，平滑地将偏移量插值回0。
	if (GameplayTag_IsDashing || RootYawOffsetMode == EEqZeroRootYawOffsetMode::BlendOut)
	{
		// 弹簧插值，平滑归位到0
		const float InterpolatedRootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			RootYawOffset, 
			0.0f, 
			RootYawOffsetSpringState, 
			80.0f, // 刚度，越大归位越快
			1.0f, // 无回弹
			DeltaSeconds, 
			1.0f, 
			0.5f
		);

		SetRootYawOffset(InterpolatedRootYawOffset);
	}

	// 重置为消除偏航偏移。每次更新时，状态都需要请求累积或保持偏移量。否则，偏移量将被消除。
	// 这主要是因为大多数状态都希望消除偏移量，因此这样可以避免为每个状态单独标记。
	RootYawOffsetMode = EEqZeroRootYawOffsetMode::BlendOut;
}

void UEqZeroAnimInstance::SetRootYawOffset(float InRootYawOffset)
{
	if (!bEnableRootYawOffset)
	{
		RootYawOffset = 0.f;
		return;
	}
	
	/*
	* 我们对偏移量进行限制，是因为偏移量过大时，角色需要向后瞄准过远，导致脊柱过度扭转。原地转身动画通常能够跟上偏移量的变化，
	* 但如果用户快速旋转视角，这种限制会导致角色的脚部滑动。
	* 如有需要，可以通过增加瞄准动画的旋转角度（最大可达 180 度）或更频繁地触发原地转身动画来替代这种限制。
	 */

	const float NormalizedAngle = UKismetMathLibrary::NormalizeAxis(InRootYawOffset); // 标准化到 [-180, 180]
	const FVector2D ClampRange = IsCrouching ? RootYawOffsetAngleClampCrouched : RootYawOffsetAngleClamp;

	// 如果 Min == Max，则不进行 Clamp
	if (FMath::IsNearlyEqual(ClampRange.X, ClampRange.Y))
	{
		RootYawOffset = NormalizedAngle;
	}
	else
	{
		// 这东西是配置，就是不想等的，好像也不会有相等的情况，一直是走这里
		RootYawOffset = UKismetMathLibrary::ClampAngle(NormalizedAngle, ClampRange.X, ClampRange.Y);
	}
	AimYaw = RootYawOffset;
}

void UEqZeroAnimInstance::UpdateAimingData()
{
	AimPitch = UKismetMathLibrary::NormalizeAxis(Character->GetBaseAimRotation().Pitch);
}

void UEqZeroAnimInstance::UpdateJumpFallData()
{
	if (IsJumping)
	{
		TimeToJumpApex = (0 - WorldVelocity.Z) / Character->GetCharacterMovement()->GetGravityZ();
	}
	else
	{
		TimeToJumpApex = 0.f;
	}
}
