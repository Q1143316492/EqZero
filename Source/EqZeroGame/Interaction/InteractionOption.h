// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "InteractionOption.generated.h"

class IInteractableTarget;
class UUserWidget;

/**
 *
 */
USTRUCT(BlueprintType)
struct FInteractionOption
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TScriptInterface<IInteractableTarget> InteractableTarget;

	// 交互的时候，玩家的 UI 会显示一个交互的按钮，按钮上的图标就是这个文本
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	// 交互的子文本，可能会显示在按钮下方，或者在其他地方
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SubText;

	// METHODS OF INTERACTION
	//--------------------------------------------------------------

	// 1) 给虚拟形象赋予一项能力，使其在进行互动时能够激活该能力。

	// 当虚拟形象靠近可交互物体时，授予其相应能力。
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> InteractionAbilityToGrant;

	// - OR -

	// 2) 让我们正在交互的对象拥有自己的能力系统和交互能力，这样我们就可以激活它的（能力系统和交互能力）了。

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> TargetAbilitySystem = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle TargetInteractionAbilityHandle;

	// UI
	//--------------------------------------------------------------

	// 交互物体时，显示的是什么UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	//--------------------------------------------------------------

public:
	FORCEINLINE bool operator==(const FInteractionOption& Other) const
	{
		return InteractableTarget == Other.InteractableTarget &&
			InteractionAbilityToGrant == Other.InteractionAbilityToGrant&&
			TargetAbilitySystem == Other.TargetAbilitySystem &&
			TargetInteractionAbilityHandle == Other.TargetInteractionAbilityHandle &&
			InteractionWidgetClass == Other.InteractionWidgetClass &&
			Text.IdenticalTo(Other.Text) &&
			SubText.IdenticalTo(Other.SubText);
	}

	FORCEINLINE bool operator!=(const FInteractionOption& Other) const
	{
		return !operator==(Other);
	}

	FORCEINLINE bool operator<(const FInteractionOption& Other) const
	{
		return InteractableTarget.GetInterface() < Other.InteractableTarget.GetInterface();
	}
};
