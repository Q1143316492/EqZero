// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "GameFramework/CheatManager.h"
#include "EqZeroCheatManager.generated.h"

class UEqZeroAbilitySystemComponent;


#ifndef USING_CHEAT_MANAGER
#define USING_CHEAT_MANAGER (1 && !UE_BUILD_SHIPPING)
#endif // #ifndef USING_CHEAT_MANAGER

DECLARE_LOG_CATEGORY_EXTERN(LogEqZeroCheat, Log, All);


/**
 * UEqZeroCheatManager
 */
UCLASS(config = Game, Within = PlayerController, MinimalAPI)
class UEqZeroCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:

	UEqZeroCheatManager();

	/*
	 * 通用 
	 */
	
	virtual void InitCheatManager() override;

	static void CheatOutputText(const FString& TextToOutput);

	UFUNCTION(exec)
	void Cheat(const FString& Msg);

	UFUNCTION(exec)
	void CheatAll(const FString& Msg);

	UFUNCTION(Exec)
	void FindAnimationsWithCurve(const FString& CurveName);

	/*
	 * 玩法指令 
	 */
	
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void PlayNextGame();

	/*
	 * 摄像机 
	 */
	
	UFUNCTION(Exec)
	virtual void ToggleFixedCamera();

	UFUNCTION(Exec)
	virtual void CycleDebugCameras();

	/*
	 * 技能 
	 */
	
	UFUNCTION(Exec)
	virtual void CycleAbilitySystemDebug();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void CancelActivatedAbilities();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void AddTagToSelf(FString TagName);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void RemoveTagFromSelf(FString TagName);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void DamageSelf(float DamageAmount);

	virtual void DamageTarget(float DamageAmount) override;

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void HealSelf(float HealAmount);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void HealTarget(float HealAmount);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void DamageSelfDestruct();

	virtual void God() override;

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void UnlimitedHealth(int32 Enabled = -1);

protected:

	virtual void EnableDebugCamera() override;
	virtual void DisableDebugCamera() override;
	bool InDebugCamera() const;

	virtual void EnableFixedCamera();
	virtual void DisableFixedCamera();
	bool InFixedCamera() const;

	void ApplySetByCallerDamage(UEqZeroAbilitySystemComponent* EqZeroASC, float DamageAmount);
	void ApplySetByCallerHeal(UEqZeroAbilitySystemComponent* EqZeroASC, float HealAmount);

	UEqZeroAbilitySystemComponent* GetPlayerAbilitySystemComponent() const;
};
