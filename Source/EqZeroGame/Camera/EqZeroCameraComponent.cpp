// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroCameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EqZeroCameraMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroCameraComponent)

UEqZeroCameraComponent::UEqZeroCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraModeStack = nullptr;
	FieldOfViewOffset = 0.0f;
}

void UEqZeroCameraComponent::OnRegister()
{
	Super::OnRegister();

	if (!CameraModeStack)
	{
		CameraModeStack = NewObject<UEqZeroCameraModeStack>(this);
		check(CameraModeStack);
	}
}

void UEqZeroCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	Super::GetCameraView(DeltaTime, DesiredView);
	
	// 通过代理查询一下当前最合适的摄像机模式是什么，可能是技能组件的影响。如果是重复的摄像模式栈里面会处理。
	UpdateCameraModes();

	// 遍历更新栈里面CameraMode的权重，达到1权重的删除，但是会留一个权重1的在最后
	// 然后拿到最后一个CameraMode作为输出，输出前，要混合前面CameraMode的权重，赋值给这里的 CameraModeView
	FEqZeroCameraModeView CameraModeView;
	CameraModeStack->EvaluateStack(DeltaTime, CameraModeView);

	// 保持玩家控制器与最新视图同步
	/*
	 * 为什么要 SetControlRotation？
	 * DesiredView 是本帧渲染的输入，传给引擎决定这帧怎么画画面，只活在本地这一帧，下帧就丢弃了
	 * SetControlRotation 是写入 PlayerController 的持久状态。ControlRotation 这个值会被 UE 自动复制给服务端，服务端用它做权威判定（射线从哪个方向打、技能目标方向等）。
	 *
	 * 如果去掉 画面视觉上没问题（DesiredView 正常）
	 * 但 ControlRotation 就不会随 CameraMode Stack 计算的结果更新了，服务端拿到的还是旧的或者默认的朝向
	 * 开枪时服务端校验的射线方向就和客户端实际看到的方向对不上，Replay 回放里角色朝向也会错
	 */
	if (APawn* TargetPawn = Cast<APawn>(GetTargetActor()))
	{
		if (APlayerController* PC = TargetPawn->GetController<APlayerController>())
		{
			PC->SetControlRotation(CameraModeView.ControlRotation);
		}
	}

	// 应用任何已添加到视野中的偏移量（这个在项目中没用到）
	CameraModeView.FieldOfView += FieldOfViewOffset;
	FieldOfViewOffset = 0.0f;

	// 保持相机组件与最新视图同步
	SetWorldLocationAndRotation(CameraModeView.Location, CameraModeView.Rotation);
	FieldOfView = CameraModeView.FieldOfView;

	// 填充数值，主要就是 Super::GetCameraView(DeltaTime, DesiredView); 没调用，要自己填一下
	DesiredView.Location = CameraModeView.Location;
	DesiredView.Rotation = CameraModeView.Rotation;
	DesiredView.FOV = CameraModeView.FieldOfView;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;

	// 查看摄像机是否想要覆盖所使用的后期处理设置。
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}

	// if (IsXRHeadTrackedCamera())
	// {
	// 	Super::GetCameraView(DeltaTime, DesiredView);
	// }
}

void UEqZeroCameraComponent::UpdateCameraModes()
{
	check(CameraModeStack);

	if (CameraModeStack->IsStackActivate())
	{
		if (DetermineCameraModeDelegate.IsBound())
		{
			if (const TSubclassOf<UEqZeroCameraMode> CameraMode = DetermineCameraModeDelegate.Execute())
			{
				CameraModeStack->PushCameraMode(CameraMode);
			}
		}
	}
}

void UEqZeroCameraComponent::DrawDebug(UCanvas* Canvas) const
{
	// showdebug camera

	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("EqZeroCameraComponent: %s"), *GetNameSafe(GetTargetActor())));

	DisplayDebugManager.SetDrawColor(FColor::White);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   Location: %s"), *GetComponentLocation().ToCompactString()));
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   Rotation: %s"), *GetComponentRotation().ToCompactString()));
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   FOV: %f"), FieldOfView));

	check(CameraModeStack);
	CameraModeStack->DrawDebug(Canvas);
}

void UEqZeroCameraComponent::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	check(CameraModeStack);
	CameraModeStack->GetBlendInfo(OutWeightOfTopLayer, OutTagOfTopLayer);
}

