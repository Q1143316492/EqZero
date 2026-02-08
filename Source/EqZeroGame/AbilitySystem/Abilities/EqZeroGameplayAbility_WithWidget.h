// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EqZeroGameplayAbility.h"
#include "UIExtensionSystem.h"

#include "EqZeroGameplayAbility_WithWidget.generated.h"

class UUserWidget;

/**
 * UEqZeroGameplayAbility_WithWidget
 *
 *	Gameplay ability that adds widgets to the UI extension subsystem when the ability is given.
 */
UCLASS(Abstract)
class UEqZeroGameplayAbility_WithWidget : public UEqZeroGameplayAbility
{
	GENERATED_BODY()

public:

	UEqZeroGameplayAbility_WithWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta=(AllowAbstract = "false"))
	TArray<TSubclassOf<UUserWidget>> WidgetClasses;

	UPROPERTY(EditDefaultsOnly, Category = "UI", meta=(Categories="UI.ExtensionPoint"))
	TArray<FGameplayTag> WidgetExtensionPointTags;

	UPROPERTY(Transient)
	TArray<FUIExtensionHandle> WidgetExtensionHandles;
};
