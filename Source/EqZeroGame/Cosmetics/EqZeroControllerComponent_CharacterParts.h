// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "EqZeroCharacterPartTypes.h"

#include "EqZeroControllerComponent_CharacterParts.generated.h"

class APawn;
class UEqZeroPawnComponent_CharacterParts;
class UObject;
struct FFrame;

// 添加装扮的来源或者原因
enum class ECharacterPartSource : uint8
{
	Natural,

	NaturalSuppressedViaCheat,

	AppliedViaDeveloperSettingsCheat,

	AppliedViaCheatManager
};

//////////////////////////////////////////////////////////////////////


// 添加 Character Part 请求的 Request 结构体
USTRUCT()
struct FEqZeroControllerCharacterPartEntry
{
	GENERATED_BODY()

	FEqZeroControllerCharacterPartEntry()
	{}

public:
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FEqZeroCharacterPart Part;
	
	FEqZeroCharacterPartHandle Handle;

	ECharacterPartSource Source = ECharacterPartSource::Natural;
};

//////////////////////////////////////////////////////////////////////

/*
 * 这是一个用GameFeatureAction_AddComponent添加到 controller 上的组件
 * 他提供了一个接口，用于添加角色的装扮部分
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class UEqZeroControllerComponent_CharacterParts : public UControllerComponent
{
	GENERATED_BODY()

public:
	UEqZeroControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

    /**
     * 添加和删除接口
     */

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void AddCharacterPart(const FEqZeroCharacterPart& NewPart);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveCharacterPart(const FEqZeroCharacterPart& PartToRemove);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveAllCharacterParts();

	void ApplyDeveloperSettings();

protected:
	UPROPERTY(EditAnywhere, Category=Cosmetics)
	TArray<FEqZeroControllerCharacterPartEntry> CharacterParts;

private:
	UEqZeroPawnComponent_CharacterParts* GetPawnCustomizer() const;

	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void AddCharacterPartInternal(const FEqZeroCharacterPart& NewPart, ECharacterPartSource Source);

	void AddCheatPart(const FEqZeroCharacterPart& NewPart, bool bSuppressNaturalParts);
	void ClearCheatParts();

	void SetSuppressionOnNaturalParts(bool bSuppressed);

	friend class UEqZeroCosmeticCheats;
};
