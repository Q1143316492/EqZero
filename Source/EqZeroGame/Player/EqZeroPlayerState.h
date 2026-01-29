// Copyright Epic Games, Inc. All Rights Reserved.
// TODO team

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "System/GameplayTagStack.h"

#include "EqZeroPlayerState.generated.h"

#define UE_API EQZEROGAME_API

struct FEqZeroVerbMessage;
class AController;
class AEqZeroPlayerController;
class APlayerState;
class FName;
class UAbilitySystemComponent;
class UEqZeroAbilitySystemComponent;
class UEqZeroExperienceDefinition;
class UEqZeroPawnData;
class UObject;
struct FFrame;
struct FGameplayTag;

// 客户端连接的类型。
UENUM()
enum class EEqZeroPlayerConnectionType : uint8
{
	// 一个活跃的玩家
	Player = 0, 

	// 连接到正在运行游戏的观众
	LiveSpectator,

	// 离线观看演示录制的观众
	ReplaySpectator,

	// 一个已停用的玩家（已断开连接）
	InactivePlayer
};

/**
 * AEqZeroPlayerState
 */
UCLASS(MinimalAPI, Config = Game)
class AEqZeroPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UE_API AEqZeroPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerState")
	UE_API AEqZeroPlayerController* GetEqZeroPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "EqZero|PlayerState")
	UEqZeroAbilitySystemComponent* GetEqZeroAbilitySystemComponent() const { return AbilitySystemComponent; }
	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	UE_API void SetPawnData(const UEqZeroPawnData* InPawnData);

	//~AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	UE_API virtual void Reset() override;
	UE_API virtual void ClientInitialize(AController* C) override;
	UE_API virtual void CopyProperties(APlayerState* PlayerState) override;
	UE_API virtual void OnDeactivated() override;
	UE_API virtual void OnReactivated() override;
	//~End of APlayerState interface

	static UE_API const FName NAME_EqZeroAbilityReady;

	UE_API void SetPlayerConnectionType(EEqZeroPlayerConnectionType NewType);
	EEqZeroPlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }

	/**
	 * Tag Stack 相关
	 * 		就是一个比较好用的 tag => 数字 的映射容器
	 */

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	UE_API void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	UE_API void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, Category=Teams)
	UE_API int32 GetStatTagStackCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category=Teams)
	UE_API bool HasStatTag(FGameplayTag Tag) const;

	/**
	 * 消息
	 *  */
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "EqZero|PlayerState")
	UE_API void ClientBroadcastMessage(const FEqZeroVerbMessage Message);

	/**
	 * 视角旋转 观战用
	 */
	UE_API FRotator GetReplicatedViewRotation() const;
	UE_API void SetReplicatedViewRotation(const FRotator& NewRotation);

private:
	void OnExperienceLoaded(const UEqZeroExperienceDefinition* CurrentExperience);

protected:
	UFUNCTION()
	UE_API void OnRep_PawnData();

protected:

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UEqZeroPawnData> PawnData;

private:
	UPROPERTY(VisibleAnywhere, Category = "EqZero|PlayerState")
	TObjectPtr<UEqZeroAbilitySystemComponent> AbilitySystemComponent;

	// 生命值、最大生命值、治疗和伤害
	UPROPERTY()
	TObjectPtr<const class UEqZeroHealthSet> HealthSet;

	// 基础伤害，基础治疗
	UPROPERTY()
	TObjectPtr<const class UEqZeroCombatSet> CombatSet;

	UPROPERTY(Replicated)
	EEqZeroPlayerConnectionType MyPlayerConnectionType;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation;
};

#undef UE_API