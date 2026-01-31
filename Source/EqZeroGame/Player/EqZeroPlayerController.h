// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/EqZeroCameraAssistInterface.h"
#include "CommonPlayerController.h"
#include "Teams/EqZeroTeamAgentInterface.h"

#include "EqZeroPlayerController.generated.h"

#define UE_API EQZEROGAME_API

struct FGenericTeamId;

class AEqZeroHUD;
class AEqZeroPlayerState;
class APawn;
class APlayerState;
class FPrimitiveComponentId;
class IInputInterface;
class UEqZeroAbilitySystemComponent;
class UEqZeroSettingsShared;
class UObject;
class UPlayer;
struct FFrame;

/**
 * AEqZeroPlayerController
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class AEqZeroPlayerController : public ACommonPlayerController, public IEqZeroCameraAssistInterface, public IEqZeroTeamAgentInterface
{
	GENERATED_BODY()

public:

	UE_API AEqZeroPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Getter
	 */

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	UE_API AEqZeroPlayerState* GetEqZeroPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	UE_API UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerController")
	UE_API AEqZeroHUD* GetEqZeroHUD() const;

	/**
	 * 指令
	 * - 父类的方法可以直接调用，但是抽出来应该是想加一些检查，但是目前没有
	 */

	UFUNCTION(Reliable, Server, WithValidation)
	UE_API void ServerCheat(const FString& Msg);

	UFUNCTION(Reliable, Server, WithValidation)
	UE_API void ServerCheatAll(const FString& Msg);

	//~AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~AController interface
	UE_API virtual void OnPossess(APawn* InPawn) override;
	UE_API virtual void OnUnPossess() override;
	UE_API virtual void InitPlayerState() override;
	UE_API virtual void CleanupPlayerState() override;
	UE_API virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	UE_API virtual void ReceivedPlayer() override;
	UE_API virtual void PlayerTick(float DeltaTime) override;
	UE_API virtual void SetPlayer(UPlayer* InPlayer) override;
	UE_API virtual void AddCheats(bool bForce) override;
	UE_API virtual void UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId) override;
	UE_API virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents) override;
	UE_API virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	UE_API virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	//~IEqZeroCameraAssistInterface interface
	UE_API virtual void OnCameraPenetratingTarget() override;
	//~End of IEqZeroCameraAssistInterface interface

	UFUNCTION(BlueprintCallable, Category = "EqZero|Character")
	UE_API void SetIsAutoRunning(const bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "EqZero|Character")
	UE_API bool GetIsAutoRunning() const;

private:
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	//~IEqZeroTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnEqZeroTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IEqZeroTeamAgentInterface interface

	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* Producer, int32 OldTeamID, int32 NewTeamID);

protected:
	void OnSettingsChanged(UEqZeroSettingsShared* Settings);

	UE_API virtual void OnPlayerStateChanged();

private:
	FOnEqZeroTeamIndexChangedDelegate OnTeamChangedDelegate;
	void BroadcastOnPlayerStateChanged();

protected:

	// UE_API void OnSettingsChanged(UEqZeroSettingsShared* Settings);

	UE_API void OnStartAutoRun();
	UE_API void OnEndAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnStartAutoRun"))
	UE_API void K2_OnStartAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnEndAutoRun"))
	UE_API void K2_OnEndAutoRun();

	bool bHideViewTargetPawnNextFrame = false;
};

#undef UE_API
