// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCameraMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/Character.h"
#include "EqZeroCameraComponent.h"
#include "EqZeroPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCameraMode)


//////////////////////////////////////////////////////////////////////////
// FEqZeroCameraModeView
//////////////////////////////////////////////////////////////////////////
FEqZeroCameraModeView::FEqZeroCameraModeView()
	: Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(EQZERO_CAMERA_DEFAULT_FOV)
{
}

void FEqZeroCameraModeView::Blend(const FEqZeroCameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.0f)
	{
		return;
	}
	else if (OtherWeight >= 1.0f)
	{
		*this = Other;
		return;
	}

	Location = FMath::Lerp(Location, Other.Location, OtherWeight);

	const FRotator DeltaRotation = (Other.Rotation - Rotation).GetNormalized();
	Rotation = Rotation + (OtherWeight * DeltaRotation);

	const FRotator DeltaControlRotation = (Other.ControlRotation - ControlRotation).GetNormalized();
	ControlRotation = ControlRotation + (OtherWeight * DeltaControlRotation);

	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
}


//////////////////////////////////////////////////////////////////////////
// UEqZeroCameraMode
//////////////////////////////////////////////////////////////////////////
UEqZeroCameraMode::UEqZeroCameraMode()
{
	FieldOfView = EQZERO_CAMERA_DEFAULT_FOV;
	ViewPitchMin = EQZERO_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = EQZERO_CAMERA_DEFAULT_PITCH_MAX;

	BlendTime = 0.5f;
	BlendFunction = EEqZeroCameraModeBlendFunction::EaseOut;
	BlendExponent = 4.0f;
	BlendAlpha = 1.0f;
	BlendWeight = 1.0f;
}

UEqZeroCameraComponent* UEqZeroCameraMode::GetEqZeroCameraComponent() const
{
	return CastChecked<UEqZeroCameraComponent>(GetOuter());
}

UWorld* UEqZeroCameraMode::GetWorld() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? nullptr : GetOuter()->GetWorld();
}

AActor* UEqZeroCameraMode::GetTargetActor() const
{
	const UEqZeroCameraComponent* EqZeroCameraComponent = GetEqZeroCameraComponent();
	return EqZeroCameraComponent->GetTargetActor();
}

FVector UEqZeroCameraMode::GetPivotLocation() const
{
	// 获取摄像机目标轴心的位置，主要是考虑角色蹲伏，所以有一点特殊

	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetPawn))
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			check(TargetCharacterCDO);

			const UCapsuleComponent* CapsuleComp = TargetCharacter->GetCapsuleComponent();
			check(CapsuleComp);

			const UCapsuleComponent* CapsuleCompCDO = TargetCharacterCDO->GetCapsuleComponent();
			check(CapsuleCompCDO);

			// 角色蹲下的时候要根据胶囊体的情况得到眼睛的高度。来确定摄像机轴心位置
			const float DefaultHalfHeight = CapsuleCompCDO->GetUnscaledCapsuleHalfHeight();
			const float ActualHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();

			// 蹲伏 ActualHalfHeight 从 DefaultHalfHeight 变小，所以 HeightAdjustment 是正数。调整回来
			const float HeightAdjustment = (DefaultHalfHeight - ActualHalfHeight) + TargetCharacterCDO->BaseEyeHeight;

			return TargetCharacter->GetActorLocation() + (FVector::UpVector * HeightAdjustment);
		}

		return TargetPawn->GetPawnViewLocation();
	}

	return TargetActor->GetActorLocation();
}

FRotator UEqZeroCameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		return TargetPawn->GetViewRotation();
	}

	return TargetActor->GetActorRotation();
}

void UEqZeroCameraMode::UpdateCameraMode(float DeltaTime)
{
	// 先根据当前的轴心，初始化一份 view 的数据
	UpdateView(DeltaTime);

	// 累加权重值
	UpdateBlending(DeltaTime);
}

void UEqZeroCameraMode::UpdateView(float DeltaTime)
{
	FVector PivotLocation = GetPivotLocation();
	FRotator PivotRotation = GetPivotRotation();

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

void UEqZeroCameraMode::SetBlendWeight(float Weight)
{
	// 直接设置到某个混合权重

	BlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);

	// BlendExponent是配置吧，由于我们直接设置了混合权重，需要计算混合alpha以适应混合函数。
	const float InvExponent = (BlendExponent > 0.0f) ? (1.0f / BlendExponent) : 1.0f;

	// 影响的只是 BlendAlpha 的计算公式，前期我们直接按线性混合Linear理解
	switch (BlendFunction)
	{
	case EEqZeroCameraModeBlendFunction::Linear:
		BlendAlpha = BlendWeight;
		break;

	case EEqZeroCameraModeBlendFunction::EaseIn:
		BlendAlpha = FMath::InterpEaseIn(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	case EEqZeroCameraModeBlendFunction::EaseOut:
		BlendAlpha = FMath::InterpEaseOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	case EEqZeroCameraModeBlendFunction::EaseInOut:
		BlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	default:
		checkf(false, TEXT("SetBlendWeight: Invalid BlendFunction [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

void UEqZeroCameraMode::UpdateBlending(float DeltaTime)
{
	/**
	 * BlendTime时间内要混合完成，所以每次要混合 (DeltaTime / BlendTime) 的权重
	 * BlendAlpha 是描述混合了多少，0是完全没有混合，1是完全混合完成。
	 * 由于混合可能非线性
	 * 所以 BlendWeight 是根据 BlendAlpha 和 BlendFunction 计算出来的，描述当前混合的权重
	 */
	if (BlendTime > 0.0f)
	{
		BlendAlpha += (DeltaTime / BlendTime);
		BlendAlpha = FMath::Min(BlendAlpha, 1.0f);
	}
	else
	{
		BlendAlpha = 1.0f;
	}

	const float Exponent = (BlendExponent > 0.0f) ? BlendExponent : 1.0f;

	switch (BlendFunction)
	{
	case EEqZeroCameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha;
		break;

	case EEqZeroCameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EEqZeroCameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EEqZeroCameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	default:
		checkf(false, TEXT("UpdateBlending: Invalid BlendFunction [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

void UEqZeroCameraMode::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetDrawColor(FColor::White);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("      EqZeroCameraMode: %s (%f)"), *GetName(), BlendWeight));
}


//////////////////////////////////////////////////////////////////////////
// UEqZeroCameraModeStack
//////////////////////////////////////////////////////////////////////////
UEqZeroCameraModeStack::UEqZeroCameraModeStack()
{
	bIsActive = true;
}

void UEqZeroCameraModeStack::ActivateStack()
{
	if (!bIsActive)
	{
		bIsActive = true;

		for (UEqZeroCameraMode* CameraMode : CameraModeStack)
		{
			check(CameraMode);
			CameraMode->OnActivation();
		}
	}
}

void UEqZeroCameraModeStack::DeactivateStack()
{
	if (bIsActive)
	{
		bIsActive = false;

		for (UEqZeroCameraMode* CameraMode : CameraModeStack)
		{
			check(CameraMode);
			CameraMode->OnDeactivation();
		}
	}
}

void UEqZeroCameraModeStack::PushCameraMode(TSubclassOf<UEqZeroCameraMode> CameraModeClass)
{
	// 压入一个摄像机模式到进来

	if (!CameraModeClass)
	{
		return;
	}

	// 从CameraMode实例池拿一个，如果没有就创建
	UEqZeroCameraMode* CameraMode = GetCameraModeInstance(CameraModeClass);
	check(CameraMode);

	int32 StackSize = CameraModeStack.Num();

	if ((StackSize > 0) && (CameraModeStack[0] == CameraMode))
	{
		// 栈顶就是他，重复了，这里跳过
		return;
	}

	// 如果这个CameraMode在栈里面，就要删除它，并把他重新在栈顶 (Array[0]是顶) 添加。
	// 例如 第三人称模式，开镜模式
	// 例如 第三人称->开镜->取消开镜回到第三人称 这时候就是删除第三人称，在栈顶重新添加，并且要算贡献来决定混合权重
	int32 ExistingStackIndex = INDEX_NONE;
	float ExistingStackContribution = 1.0f;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		if (CameraModeStack[StackIndex] == CameraMode)
		{
			ExistingStackIndex = StackIndex;
			ExistingStackContribution *= CameraMode->GetBlendWeight();
			break;
		}
		else
		{
			// 距离，一开始有个权重1的第三人称。开镜那么贡献就是 (1 - 1) = 0。结合Insert0。权重数组为[0][1]
			ExistingStackContribution *= (1.0f - CameraModeStack[StackIndex]->GetBlendWeight());
		}
	}

	if (ExistingStackIndex != INDEX_NONE)
	{
		// 找到了删除，贡献也算出来了
		CameraModeStack.RemoveAt(ExistingStackIndex);
		StackSize--;
	}
	else
	{
		// 没找到，贡献0
		ExistingStackContribution = 0.0f;
	}

	// 决定初始权重
	// 如果容器是空的，那么初始权重就是1
	// 否则，由于BlendTime是配置必须合法，那么权重就是 ExistingStackContribution
	// 如果是新的，那就是从0开始，否则就是上面计算的贡献值
	const bool bShouldBlend = ((CameraMode->GetBlendTime() > 0.0f) && (StackSize > 0));
	const float BlendWeight = (bShouldBlend ? ExistingStackContribution : 1.0f);
	CameraMode->SetBlendWeight(BlendWeight);

	// 插入到栈顶，并且包装最后一个权重是1
	CameraModeStack.Insert(CameraMode, 0);
	CameraModeStack.Last()->SetBlendWeight(1.0f);

	// 如果这是一个新的CameraMode，那么就激活它
	if (ExistingStackIndex == INDEX_NONE)
	{
		CameraMode->OnActivation();
	}
}

bool UEqZeroCameraModeStack::EvaluateStack(float DeltaTime, FEqZeroCameraModeView& OutCameraModeView)
{
	if (!bIsActive)
	{
		return false;
	}

	UpdateStack(DeltaTime);
	BlendStack(OutCameraModeView);

	return true;
}

UEqZeroCameraMode* UEqZeroCameraModeStack::GetCameraModeInstance(TSubclassOf<UEqZeroCameraMode> CameraModeClass)
{
	check(CameraModeClass);

	// CameraModeInstances 相当于示例池子，看起来也不删除
	for (UEqZeroCameraMode* CameraMode : CameraModeInstances)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}

	// 没找到就创建一个
	UEqZeroCameraMode* NewCameraMode = NewObject<UEqZeroCameraMode>(GetOuter(), CameraModeClass, NAME_None, RF_NoFlags);
	check(NewCameraMode);

	CameraModeInstances.Add(NewCameraMode);

	return NewCameraMode;
}

void UEqZeroCameraModeStack::UpdateStack(float DeltaTime)
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	int32 RemoveCount = 0;
	int32 RemoveIndex = INDEX_NONE;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		UEqZeroCameraMode* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		// 这里面会根据Pivot重新计算数据，然后累加权重
		CameraMode->UpdateCameraMode(DeltaTime);

		// 如果权重已经满了，后面的就不用更新了，但是会留一个权重是1的在栈里面
		if (CameraMode->GetBlendWeight() >= 1.0f)
		{
			// 此模式以下的所有内容现在都无关紧要，可以删除。
			// 如果只有一个第三人称，RemoveIndex = (0 + 1), RemoveCount = (1 - 1) = 0
			// 最终会留一个混合完成的Mode在栈里面
			RemoveIndex = (StackIndex + 1);
			RemoveCount = (StackSize - RemoveIndex);
			break;
		}
	}

	if (RemoveCount > 0)
	{
		// 非激活事件，然后删除
		for (int32 StackIndex = RemoveIndex; StackIndex < StackSize; ++StackIndex)
		{
			UEqZeroCameraMode* CameraMode = CameraModeStack[StackIndex];
			check(CameraMode);

			CameraMode->OnDeactivation();
		}

		CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
	}
}

void UEqZeroCameraModeStack::BlendStack(FEqZeroCameraModeView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	// 拿到最后一个
	const UEqZeroCameraMode* CameraMode = CameraModeStack[StackSize - 1];
	check(CameraMode);

	OutCameraModeView = CameraMode->GetCameraModeView();

	// 从倒数第二个向前遍历，一路混合过去
	// 例如第三人称+开镜 
	//    - 栈权重是 [开镜:0] [第三人称:1]，返回最后一个，但是倒着遍历混合前面的权重，
	//    如果随着累加，权重到了 [开镜:0.2] [第三人称:1]
	//    - 那么返回 第三人称，但是要混合 0.2 的开镜的数据。
	for (int32 StackIndex = (StackSize - 2); StackIndex >= 0; --StackIndex)
	{
		CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		OutCameraModeView.Blend(CameraMode->GetCameraModeView(), CameraMode->GetBlendWeight());
	}
}

void UEqZeroCameraModeStack::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetDrawColor(FColor::Green);
	DisplayDebugManager.DrawString(FString(TEXT("   --- Camera Modes (Begin) ---")));

	for (const UEqZeroCameraMode* CameraMode : CameraModeStack)
	{
		check(CameraMode);
		CameraMode->DrawDebug(Canvas);
	}

	DisplayDebugManager.SetDrawColor(FColor::Green);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   --- Camera Modes (End) ---")));
}

void UEqZeroCameraModeStack::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	if (CameraModeStack.Num() == 0)
	{
		OutWeightOfTopLayer = 1.0f;
		OutTagOfTopLayer = FGameplayTag();
		return;
	}
	else
	{
		UEqZeroCameraMode* TopEntry = CameraModeStack.Last();
		check(TopEntry);
		OutWeightOfTopLayer = TopEntry->GetBlendWeight();
		OutTagOfTopLayer = TopEntry->GetCameraTypeTag();
	}
}