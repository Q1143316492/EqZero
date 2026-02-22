// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_ADS.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Character/EqZeroCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EqZeroGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Player/EqZeroPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_ADS)


UEqZeroGameplayAbility_ADS::UEqZeroGameplayAbility_ADS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UEqZeroGameplayAbility_ADS::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 切换 ADS 摄像机
	SetCameraMode(DefaultCameraMode);

	// 缓存默认移速，并应用 ADS 移速倍率
	if (AEqZeroCharacter* Character = GetEqZeroCharacterFromActorInfo())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			MaxWalkSpeedDefault = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed = MaxWalkSpeedDefault * ADSMultiplier;
		}
	}

	// 添加 ADS 输入映射上下文（更高优先级使移速修改器生效）
	if (AEqZeroPlayerController* PC = GetEqZeroPlayerControllerFromActorInfo())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (ADSInputMappingContext)
			{
				FModifyContextOptions Options;
				Options.bForceImmediately = true;
				Subsystem->AddMappingContext(ADSInputMappingContext, ADSMappingContextPriority, Options);
			}
		}
	}

	// 本地端：通知 UI、播放音效
	if (IsLocallyControlled())
	{
		BroadcastToUI(true);

		if (ZoomInSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ZoomInSound);
		}
	}

	// 等待输入松开后结束技能
	UAbilityTask_WaitInputRelease* WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	WaitInputReleaseTask->OnRelease.AddDynamic(this, &UEqZeroGameplayAbility_ADS::OnInputReleased);
	WaitInputReleaseTask->ReadyForActivation();
}

void UEqZeroGameplayAbility_ADS::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 移除 ADS 摄像机
	ClearCameraMode();

	// 本地端：通知 UI 关闭 ADS 状态，并播放关镜音效
	if (IsLocallyControlled())
	{
		BroadcastToUI(false);

		if (ZoomOutSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ZoomOutSound);
		}
	}

	// 恢复默认移速
	if (AEqZeroCharacter* Character = GetEqZeroCharacterFromActorInfo())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = MaxWalkSpeedDefault;
		}
	}

	// 移除 ADS 输入映射上下文
	if (AEqZeroPlayerController* PC = GetEqZeroPlayerControllerFromActorInfo())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (ADSInputMappingContext)
			{
				Subsystem->RemoveMappingContext(ADSInputMappingContext);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UEqZeroGameplayAbility_ADS::BroadcastToUI(bool bON)
{
	FEqZeroADSUIMessage Message;
	Message.bON = bON;
	Message.Controller = GetEqZeroPlayerControllerFromActorInfo();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(EqZeroGameplayTags::Gameplay_Message_ADS, Message);
}

void UEqZeroGameplayAbility_ADS::OnInputReleased(float TimeHeld)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, false);
}
