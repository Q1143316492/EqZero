// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_WithWidget.h"
#include "Blueprint/UserWidget.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

UEqZeroGameplayAbility_WithWidget::UEqZeroGameplayAbility_WithWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEqZeroGameplayAbility_WithWidget::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	UWorld* World = GetWorld();
	UUIExtensionSubsystem* ExtensionSubsystem = World ? World->GetSubsystem<UUIExtensionSubsystem>() : nullptr;
	
	if (!ExtensionSubsystem || !ActorInfo)
	{
		return;
	}

	// Context must match what UUIExtensionPointWidget registers: LocalPlayer
	ULocalPlayer* LocalPlayer = nullptr;
	if (APlayerController* PC = ActorInfo->PlayerController.Get())
	{
		LocalPlayer = PC->GetLocalPlayer();
	}

	if (!LocalPlayer)
	{
		return;
	}

	for (int32 i = 0; i < WidgetClasses.Num(); ++i)
	{
		if (WidgetClasses.IsValidIndex(i) && WidgetExtensionPointTags.IsValidIndex(i))
		{
			if (TSubclassOf<UUserWidget> WidgetClass = WidgetClasses[i])
			{
				FGameplayTag ExtensionPoint = WidgetExtensionPointTags[i];
				FUIExtensionHandle Handle = ExtensionSubsystem->RegisterExtensionAsWidgetForContext(ExtensionPoint, LocalPlayer, WidgetClass, -1);
				WidgetExtensionHandles.Add(Handle);
			}
		}
	}
}

void UEqZeroGameplayAbility_WithWidget::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	for (FUIExtensionHandle& Handle : WidgetExtensionHandles)
	{
		Handle.Unregister();
	}
	WidgetExtensionHandles.Empty();

	Super::OnRemoveAbility(ActorInfo, Spec);
}
