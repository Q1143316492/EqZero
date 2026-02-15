// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityTask_GrantNearbyInteraction.h"

#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/InteractionStatics.h"
#include "Physics/EqZeroCollisionChannels.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_GrantNearbyInteraction)

static int32 DebugInteractionScan = 0;
FAutoConsoleVariableRef CVarDebugInteractionScan(
	TEXT("EqZero.Interaction.DebugScan"),
	DebugInteractionScan,
	TEXT("Visualize interaction scan range."),
	ECVF_Default
);

UAbilityTask_GrantNearbyInteraction::UAbilityTask_GrantNearbyInteraction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilityTask_GrantNearbyInteraction* UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(UGameplayAbility* OwningAbility, float InteractionScanRange, float InteractionScanRate)
{
	UAbilityTask_GrantNearbyInteraction* MyObj = NewAbilityTask<UAbilityTask_GrantNearbyInteraction>(OwningAbility);
	MyObj->InteractionScanRange = InteractionScanRange;
	MyObj->InteractionScanRate = InteractionScanRate;
	return MyObj;
}

void UAbilityTask_GrantNearbyInteraction::Activate()
{
	SetWaitingOnAvatar();

	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(QueryTimerHandle, this, &ThisClass::QueryInteractables, InteractionScanRate, true);
}

void UAbilityTask_GrantNearbyInteraction::OnDestroy(bool AbilityEnded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueryTimerHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_GrantNearbyInteraction::QueryInteractables()
{
	UWorld* World = GetWorld();
	AActor* ActorOwner = GetAvatarActor();
	
	if (World && ActorOwner)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(UAbilityTask_GrantNearbyInteraction), false);

#if ENABLE_DRAW_DEBUG
		if (DebugInteractionScan > 0)
		{
			DrawDebugSphere(World, ActorOwner->GetActorLocation(), InteractionScanRange, 16, FColor::Red, false, 0.1f);
		}
#endif

		// 球形碰撞检测
		TArray<FOverlapResult> OverlapResults;
		World->OverlapMultiByChannel(OUT OverlapResults, ActorOwner->GetActorLocation(),
			FQuat::Identity, EqZero_TraceChannel_Interaction, FCollisionShape::MakeSphere(InteractionScanRange), Params);

		if (OverlapResults.Num() > 0)
		{
			// 从重叠结果中获取所有的可交互目标
			TArray<TScriptInterface<IInteractableTarget>> InteractableTargets;
			UInteractionStatics::AppendInteractableTargetsFromOverlapResults(OverlapResults, OUT InteractableTargets);
			
			FInteractionQuery InteractionQuery;
			InteractionQuery.RequestingAvatar = ActorOwner;
			InteractionQuery.RequestingController = Cast<AController>(ActorOwner->GetOwner());

			// 遍历可交互的目标
			TArray<FInteractionOption> Options;
			for (TScriptInterface<IInteractableTarget>& InteractiveTarget : InteractableTargets)
			{
				// InteractiveTarget 就是场景中实现了可交互接口的 Actor，从中收集 Options
				FInteractionOptionBuilder InteractionBuilder(InteractiveTarget, Options);
				InteractiveTarget->GatherInteractionOptions(InteractionQuery, InteractionBuilder);
			}

			// 根据收集的Options检查是否有交互限制
			for (FInteractionOption& Option : Options)
			{
				if (Option.InteractionAbilityToGrant)
				{
					// 赋予玩家这个对应的交互技能
					FObjectKey ObjectKey(Option.InteractionAbilityToGrant);
					if (!InteractionAbilityCache.Find(ObjectKey))
					{
						FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1, INDEX_NONE, this);
						FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
						InteractionAbilityCache.Add(ObjectKey, Handle);
					}
				}
			}
		}
	}
}

