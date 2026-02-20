// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGameplayAbility_ReloadMagazine.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "EqZeroGameplayTags.h"
#include "Inventory/EqZeroInventoryItemInstance.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked,       "Ability.Weapon.NoFiring");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayEvent_ReloadDone, "GameplayEvent.ReloadDone");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ActivateFail_MagazineFull, "Ability.ActivateFail.MagazineFull");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ActivateFail_NoSpareAmmo,  "Ability.ActivateFail.NoSpareAmmo");

UEqZeroGameplayAbility_ReloadMagazine::UEqZeroGameplayAbility_ReloadMagazine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UEqZeroGameplayAbility_ReloadMagazine::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UEqZeroInventoryItemInstance* Item = GetAssociatedItem();
	if (!Item)
	{
		return false;
	}

	const int32 MagAmmo  = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo);
	const int32 MagSize  = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineSize);

	// 弹匣已满，无需装填
	if (MagAmmo >= MagSize)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(TAG_ActivateFail_MagazineFull);
		}
		return false;
	}

	const int32 SpareAmmo = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo);

	// 备用弹药为空，无法装填
	if (SpareAmmo <= 0)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(TAG_ActivateFail_NoSpareAmmo);
		}
		return false;
	}

	return true;
}

void UEqZeroGameplayAbility_ReloadMagazine::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (auto InventoryItemInstance = GetAssociatedItem())
	{
		DidBlockFiring = InventoryItemInstance->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo) == 0;

		if (DidBlockFiring)
		{
			if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
			{
				ASC->AddLooseGameplayTag(TAG_WeaponFireBlocked);
			}
		}
	}

	// 蒙太奇任务：播放换弹动画
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		FName("ReloadMontage"),
		ReloadMontage,
		PlayRate,
		NAME_None,
		false
	);

	if (MontageTask)
	{
		// 动画在客户端播放完成时，不要结束服务器上的技能，让其自行结束
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}

	// 事件任务：等待动画 Notify 发出 GameplayEvent.ReloadDone
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		TAG_GameplayEvent_ReloadDone,
		nullptr,
		true,
		true
	);

	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &ThisClass::OnReloadEventReceived);
		EventTask->ReadyForActivation();
	}
}

void UEqZeroGameplayAbility_ReloadMagazine::OnReloadEventReceived(FGameplayEventData Payload)
{
	// 仅在服务器端执行实际换弹逻辑
	if (HasAuthority(&CurrentActivationInfo))
	{
		ReloadAmmoIntoMagazine(Payload);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEqZeroGameplayAbility_ReloadMagazine::ReloadAmmoIntoMagazine(FGameplayEventData Payload)
{
	UEqZeroInventoryItemInstance* Item = GetAssociatedItem();
	if (!Item)
	{
		return;
	}

	const int32 MagSize   = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineSize); // 弹匣容量
	const int32 MagAmmo   = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo); // 当前弹匣中的子弹数量
	const int32 SpareAmmo = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo); // 当前备用弹药数量
	
	// 计算本次需要装填的子弹数量，不超过备用弹药
	const int32 Needed    = FMath::Min(MagSize - MagAmmo, SpareAmmo);

	if (Needed > 0)
	{
		Item->AddStatTagStack(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo, Needed);
		Item->RemoveStatTagStack(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo, Needed);
		DebugLog();
	}
}

void UEqZeroGameplayAbility_ReloadMagazine::DebugLog()
{
	if (auto Item = GetAssociatedItem())
	{
		const int32 MagSize   = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineSize);
		const int32 MagAmmo   = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_MagazineAmmo);
		const int32 SpareAmmo = Item->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo);

		UE_LOG(LogTemp, Log, TEXT("DebugLog: MagSize=%d, MagAmmo=%d, SpareAmmo=%d"), MagSize, MagAmmo, SpareAmmo);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DebugLog: No associated item found."));
	}
}

void UEqZeroGameplayAbility_ReloadMagazine::OnMontageCompleted()
{
	// 动画在客户端播放完毕时仅本地结束技能，不通知服务器（让服务器自行结束）
	// 相当于 K2_EndAbilityLocally
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UEqZeroGameplayAbility_ReloadMagazine::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UEqZeroGameplayAbility_ReloadMagazine::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UEqZeroGameplayAbility_ReloadMagazine::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (DidBlockFiring)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->RemoveLooseGameplayTag(TAG_WeaponFireBlocked);
		}
	}
}
