// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "EnhancedInputComponent.h"
#include "EqZeroInputConfig.h"
#include "InputActionValue.h"

#include "EqZeroInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;

/**
 * UEqZeroInputComponent
 */
UCLASS(Config = Input)
class UEqZeroInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:

	UEqZeroInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const UEqZeroInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UEqZeroInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

    void OnNativeInputAction(FGameplayTag InputTag, const FInputActionValue& InputActionValue);

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UEqZeroInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UEqZeroInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};


template<class UserClass, typename FuncType>
void UEqZeroInputComponent::BindNativeAction(const UEqZeroInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindActionValueLambda(IA, TriggerEvent, [this, Object, Func, InputTag](const FInputActionValue& InputActionValue)
		{
			(Object->*Func)(InputActionValue);
            OnNativeInputAction(InputTag, InputActionValue);
		});
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UEqZeroInputComponent::BindAbilityActions(const UEqZeroInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FEqZeroInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}
}