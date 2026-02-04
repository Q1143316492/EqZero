// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/EqZeroTaggedWidget.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "EqZeroAnimDebugWidget.generated.h"

class UScrollBox;
class UVerticalBox;
class UTextBlock;

USTRUCT(BlueprintType)
struct FEqZeroAnimDebugMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> DebugLines;
};

/**
 * Debug widget to display messages from AnimInstance
 */
UCLASS()
class EQZEROGAME_API UEqZeroAnimDebugWidget : public UEqZeroTaggedWidget
{
	GENERATED_BODY()

public:
	UEqZeroAnimDebugWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void OnDebugMessageReceived(FGameplayTag Channel, const FEqZeroAnimDebugMessage& Payload);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	FGameplayTag MessageChannel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> SBox_Anim;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_List;

	// Optional: Limit the number of messages
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	int32 MaxMessageCount = 50;

private:
	FGameplayMessageListenerHandle ListenerHandle;
};
