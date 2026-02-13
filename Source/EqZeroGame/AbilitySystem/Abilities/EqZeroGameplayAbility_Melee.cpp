// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_Melee.h"
#include "Physics/EqZeroCollisionChannels.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Character/EqZeroCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Physics/EqZeroCollisionChannels.h"
#include "AbilitySystem/EqZeroAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroGameplayAbility_Melee)

UEqZeroGameplayAbility_Melee::UEqZeroGameplayAbility_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Distance = 350.0f;
	Radius = 96.0f;
	HalfHeight = 48.0f;
	Strength = 2200.0f;
	Duration = 0.05f;
}

void UEqZeroGameplayAbility_Melee::PlayMeleeMontage()
{
	// TODO 近战蒙太奇根据武器的，但是武器还没有，先走配置
	// /Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_Pistol_Melee.AM_MM_Pistol_Melee
	
	if (Montage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			FName("PlayDashMontage"),
			Montage,
			1.0f,
			NAME_None,
			true
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &UEqZeroGameplayAbility_Melee::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UEqZeroGameplayAbility_Melee::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &UEqZeroGameplayAbility_Melee::OnMontageCancelled);
			MontageTask->OnBlendOut.AddDynamic(this, &UEqZeroGameplayAbility_Melee::OnMontageBlendOut);
			MontageTask->ReadyForActivation();
		}
	}
}

void UEqZeroGameplayAbility_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CommitAbility(Handle, ActorInfo, ActivationInfo);

	PlayMeleeMontage();

	if (HasAuthority(&ActivationInfo))
	{
		MeleeCapsuleTrace();
	}
}

void UEqZeroGameplayAbility_Melee::MeleeCapsuleTrace()
{
	auto Character = GetEqZeroCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	FVector Start = Character->GetMesh()->GetSocketLocation(FName("hand_r"));
	FVector End = Character->GetActorForwardVector() * Distance + Start;
	TArray<AActor*> IgnoreActors = { Character };

	FHitResult HitResult;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    /**
     * 
     *  Line Trace (射线检测):最常用，一条没有厚度的线。适合射击、视线检测。
     *  LineTraceSingle... / LineTraceMulti...
     *        
     *  Sphere Trace (球形扫描): 发射一个球体。适合判定稍微宽一点的投射物。
     *  SphereTraceSingle... / SphereTraceMulti...
     *        
     *  Box Trace (盒体扫描): 发射一个盒子。适合大范围的挥砍或车辆撞击。
     *  BoxTraceSingle... / BoxTraceMulti...
     *   
     *  Capsule Trace (胶囊体扫描): (你当前使用的) 发射一个胶囊体。适合角色移动检测或形状类似人的攻击判定。
     *  CapsuleTraceSingle... / CapsuleTraceMulti...
     */
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingleForObjects(
		this,
		Start,
		End,
		Radius,
		HalfHeight,
		ObjectTypes,
		false,
		IgnoreActors,
		DebugType,
		HitResult,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f
	);

	if (bHit)
	{
		if (auto HitActor = HitResult.GetActor())
		{
			if (auto HitASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
			{
				// 避免路径中有其他东西挡住了攻击，这样不算的
				FHitResult BlockHitResult;
				IgnoreActors.Add(HitActor);
                bHit = UKismetSystemLibrary::LineTraceSingle(
					this,
					Start,
					HitResult.ImpactPoint,
					UEngineTypes::ConvertToTraceType(EqZero_TraceChannel_Weapon),
					false,
					IgnoreActors,
					DebugType,
					BlockHitResult,
					true
				);

				if (!bHit)
				{
					MeleeHitTargetSuccess(HitASC, HitResult);
				}
			}
		}
	}
}

void UEqZeroGameplayAbility_Melee::MeleeHitTargetSuccess(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult)
{
	ApplyMeleeRootMotion(HitResult);
	ApplyMeleeGameplayEffectAndCue(TargetASC, HitResult);
}

void UEqZeroGameplayAbility_Melee::ApplyMeleeRootMotion(const FHitResult& HitResult)
{
	auto Character = GetEqZeroCharacterFromActorInfo();
	if (!Character)	{
		return;
	}

	UAbilityTask_ApplyRootMotionConstantForce* RootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		FName("None"),
		Character->GetActorForwardVector(),
		UKismetMathLibrary::NormalizeToRange(HitResult.Distance, 0.0f, Distance) * Strength,
		Duration,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
		FVector::ZeroVector,
		1000.0f,
		true
	);

	if (RootMotionTask)
	{
		RootMotionTask->ReadyForActivation();
	}
}

void UEqZeroGameplayAbility_Melee::ApplyMeleeGameplayEffectAndCue(UAbilitySystemComponent* TargetASC, const FHitResult& HitResult)
{
	auto SourceASC = GetEqZeroAbilitySystemComponentFromActorInfo();
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddHitResult(HitResult);
	
	if (DamageEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

void UEqZeroGameplayAbility_Melee::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEqZeroGameplayAbility_Melee::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEqZeroGameplayAbility_Melee::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEqZeroGameplayAbility_Melee::OnMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}