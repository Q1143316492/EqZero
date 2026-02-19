// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroWeaponStateComponent.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Equipment/EqZeroEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "Physics/PhysicalMaterialWithTags.h"
// #include "Teams/EqZeroTeamSubsystem.h" // Pending migration
#include "EqZeroLogChannels.h"
#include "Weapons/EqZeroRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWeaponStateComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Zone, "Gameplay.Zone");

UEqZeroWeaponStateComponent::UEqZeroWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UEqZeroWeaponStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/*
	 * 调用角色身上所有远程武器实例的 Tick 方法，确保它们能持续更新状态（例如热量、扩散等），以便在 UI 上做出相应反馈。
	 */
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (UEqZeroEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UEqZeroEquipmentManagerComponent>())
		{
			if (UEqZeroRangedWeaponInstance* CurrentWeapon = Cast<UEqZeroRangedWeaponInstance>(EquipmentManager->GetFirstInstanceOfType(UEqZeroRangedWeaponInstance::StaticClass())))
			{
				CurrentWeapon->Tick(DeltaTime);
			}
		}
	}
}

bool UEqZeroWeaponStateComponent::ShouldShowHitAsSuccess(const FHitResult& Hit) const
{
	AActor* HitActor = Hit.GetActor();

	// 暂时没有队伍系统，默认所有人都是敌人

	return true;
}

bool UEqZeroWeaponStateComponent::ShouldUpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext) const
{
	// 官方写的: 就这个组件而言，我们真正只关心由武器造成的伤害
	// 或者从武器发射的投射物造成的伤害，并且应该筛选出这类伤害
	// （或者或许可以看看造成伤害的物体是否也是我们当前准星配置的来源）
	return EffectContext.GetEffectCauser() != nullptr;
}

void UEqZeroWeaponStateComponent::ClientConfirmTargetData_Implementation(uint16 UniqueId, bool bSuccess, const TArray<uint8>& HitReplaces)
{
	/*
	 * 通过 ClientConfirmTargetData (RPC)，服务器通知客户端：“刚才那次射击有效”。客户端收到后，会将对应的命中标记状态更新为“成功”（bShowAsSuccess），
	 * 这通常会触发 UI 层播放更明显的反馈音效或变色（例如爆头变红）。
	 *
	 * 注意这里是 Client RPC，服务器调用，客户端执行
	 */
	for (int i = 0; i < UnconfirmedServerSideHitMarkers.Num(); i++)
	{
		FEqZeroServerSideHitMarkerBatch& Batch = UnconfirmedServerSideHitMarkers[i];
		if (Batch.UniqueId == UniqueId) // 找到对应的 Batch 做确认
		{
			if (bSuccess && (HitReplaces.Num() != Batch.Markers.Num()))
			{
				// 服务器确认这次射击有效 (bSuccess=true)
				// 且被“替换”掉的命中数量不等于总命中数（说明至少有一个有效命中没被改写）
				UWorld* World = GetWorld();
				bool bFoundShowAsSuccessHit = false;

				// 如果当前命中的索引不在被替换/否决的列表中
				// 并且客户端当初判断这是一个值得显示的命中 (bShowAsSuccess=true)
				int32 HitLocationIndex = 0;
				for (const FEqZeroScreenSpaceHitLocation& Entry : Batch.Markers)
				{
					if (!HitReplaces.Contains(HitLocationIndex) && Entry.bShowAsSuccess)
					{
						// 只需执行一次：更新“造成伤害的时间”
						if (!bFoundShowAsSuccessHit)
						{
							ActuallyUpdateDamageInstigatedTime();
						}

						bFoundShowAsSuccessHit = true;

						// 将这个确认为成功的命中加入到 LastWeaponDamageScreenLocations
						// UI 层会在 Tick 中读取这个列表来绘制命中反馈
						LastWeaponDamageScreenLocations.Add(Entry);
					}
					++HitLocationIndex;
				}
			}

			UnconfirmedServerSideHitMarkers.RemoveAt(i);
			break;
		}
	}
}

void UEqZeroWeaponStateComponent::AddUnconfirmedServerSideHitMarkers(const FGameplayAbilityTargetDataHandle& InTargetData, const TArray<FHitResult>& FoundHits)
{
	/*
	 * 客户端本地计算完命中后，会调用 AddUnconfirmedServerSideHitMarkers 将命中数据暂时存起来。
	 * 这样可以在等待服务器确认前就能立即在 UI 上做一些预表现（如果是需要极快响应的设计）。
	 */
	
	APlayerController* OwnerPC = GetController<APlayerController>();
	if (!OwnerPC)
	{
		UE_LOG(LogEqZeroAbilitySystem, Warning, TEXT("UEqZeroWeaponStateComponent::AddUnconfirmedServerSideHitMarkers called but no owning player controller found"));
		return ;
	}

	FEqZeroServerSideHitMarkerBatch& NewUnconfirmedHitMarker = UnconfirmedServerSideHitMarkers.Emplace_GetRef(InTargetData.UniqueId);
	
	for (const FHitResult& Hit : FoundHits)
	{
		FVector2D HitScreenLocation;
		// 世界3D坐标转换为屏幕2D坐标，bPlayerViewportRelative=false表示转换后的坐标是相对于整个屏幕的，而不是当前玩家视口的左上角(分屏时会有区别)
		if (UGameplayStatics::ProjectWorldToScreen(OwnerPC, Hit.Location, HitScreenLocation, false))
		{
			// 记录命中情况，根据队伍接口(还没有)，判断是否需要显示命中成功的效果
			FEqZeroScreenSpaceHitLocation& Entry = NewUnconfirmedHitMarker.Markers.AddDefaulted_GetRef();
			Entry.Location = HitScreenLocation;
			Entry.bShowAsSuccess = ShouldShowHitAsSuccess(Hit);

			// 解析受击部位 (Hit Zone)
			// 通过读取物理材质上的 Tag，系统可以知道这次击中了什么部位（头、胸、腿等）。用于UI显示
			FGameplayTag HitZone;
			if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(Hit.PhysMaterial.Get()))
			{
				for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
				{
					if (MaterialTag.MatchesTag(TAG_Gameplay_Zone))
					{
						Entry.HitZone = MaterialTag;
						break;
					}
				}
			}
		}
	}
}

void UEqZeroWeaponStateComponent::UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext)
{
	if (ShouldUpdateDamageInstigatedTime(EffectContext))
	{
		ActuallyUpdateDamageInstigatedTime();
	}
}

void UEqZeroWeaponStateComponent::ActuallyUpdateDamageInstigatedTime()
{
	// 如果我们的 LastWeaponDamageInstigatedTime 不是最近的，就清空我们的 LastWeaponDamageScreenLocations 数组。
	UWorld* World = GetWorld();
	if (World->GetTimeSeconds() - LastWeaponDamageInstigatedTime > 0.1)
	{
		LastWeaponDamageScreenLocations.Reset();
	}
	LastWeaponDamageInstigatedTime = World->GetTimeSeconds();
}

double UEqZeroWeaponStateComponent::GetTimeSinceLastHitNotification() const
{
	UWorld* World = GetWorld();
	return World->TimeSince(LastWeaponDamageInstigatedTime);
}
