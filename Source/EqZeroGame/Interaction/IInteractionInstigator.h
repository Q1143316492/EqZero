// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "InteractionOption.h"
#include "IInteractionInstigator.generated.h"

struct FInteractionQuery;

/**  */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractionInstigator : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implementing this interface allows you to add an arbitrator to the interaction process.  For example,
 * some games present the user with a menu to pick which interaction they want to perform.  This will allow you
 * to take the multiple matches (Assuming your ULyraGameplayAbility_Interact subclass generates more than one option).
 * 实现这个接口能让你在交互过程中添加一个仲裁者。
 * 例如，有些游戏会向用户展示一个菜单，供其选择想要执行的交互操作。
 * 这将允许你处理多个匹配项（假设你的 ULyraGameplayAbility_Interact 子类生成了不止一个选项）。
 */
class IInteractionInstigator
{
	GENERATED_BODY()

public:
	/** 如果有不止一个需要决定的交互选项，将会调用此函数 */
	virtual FInteractionOption ChooseBestInteractionOption(const FInteractionQuery& InteractQuery, const TArray<FInteractionOption>& InteractOptions) = 0;
};
