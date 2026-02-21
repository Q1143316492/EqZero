// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroWeaponSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/EqZeroPickupDefinition.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryFragment_SetStats.h"
#include "Kismet/GameplayStatics.h"
#include "EqZeroLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Equipment/EqZeroQuickBarComponent.h"
#include "Inventory/EqZeroInventoryManagerComponent.h"
#include "Inventory/EqZeroInventoryItemInstance.h"
#include "EqZeroGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroWeaponSpawner)

class FLifetimeProperty;
class UNiagaraSystem;
class USoundBase;
struct FHitResult;

// 构造函数：设置默认值
AEqZeroWeaponSpawner::AEqZeroWeaponSpawner()
{
	// 开启每帧 Tick，如不需要可关闭以提升性能
	PrimaryActorTick.bCanEverTick = true;

	// 创建碰撞体积并设为根组件，注册重叠回调
	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &AEqZeroWeaponSpawner::OnOverlapBegin);

	// 创建底座网格体并挂载到根
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);

	// 创建武器展示网格体并挂载到根
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	// 初始化默认参数
	WeaponMeshRotationSpeed = 40.0f;
	CoolDownTime = 30.0f;
	CheckExistingOverlapDelay = 0.25f;
	bIsWeaponAvailable = true;
	bReplicates = true;
}

// 游戏开始或 Actor 生成时调用
void AEqZeroWeaponSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 从武器定义中读取冷却时间
	if (WeaponDefinition && WeaponDefinition->InventoryItemDefinition)
	{
		CoolDownTime = WeaponDefinition->SpawnCoolDownSeconds;
	}
	else if (const UWorld* World = GetWorld())
	{
		// 非回放模式下，缺少有效武器定义时打印错误
		if (!World->IsPlayingReplay())
		{
			UE_LOG(LogEqZero, Error, TEXT("'%s' 没有设置有效的武器定义！请在实例上设置 WeaponDefinition 数据。"), *GetNameSafe(this));
		}
	}
}

// Actor 结束时清理所有定时器
void AEqZeroWeaponSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
		World->GetTimerManager().ClearTimer(CheckOverlapsDelayTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// 每帧更新冷却进度和武器旋转
void AEqZeroWeaponSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 更新冷却百分比，驱动 UI 倒计时指示器
	UWorld* World = GetWorld();
	if (World->GetTimerManager().IsTimerActive(CoolDownTimerHandle))
	{
		CoolDownPercentage = 1.0f - World->GetTimerManager().GetTimerRemaining(CoolDownTimerHandle) / CoolDownTime;
	}

	// 让武器网格体持续旋转
	WeaponMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * WeaponMeshRotationSpeed, 0.0f));
}

// 构造完成时根据武器定义设置展示网格体
void AEqZeroWeaponSpawner::OnConstruction(const FTransform& Transform)
{
	if (WeaponDefinition != nullptr && WeaponDefinition->DisplayMesh != nullptr)
	{
		WeaponMesh->SetStaticMesh(WeaponDefinition->DisplayMesh);
		WeaponMesh->SetRelativeLocation(WeaponDefinition->WeaponMeshOffset);
		WeaponMesh->SetRelativeScale3D(WeaponDefinition->WeaponMeshScale);
	}
}

// 有 Actor 进入碰撞体积时触发，仅在服务端且武器可用时尝试拾取
void AEqZeroWeaponSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (GetLocalRole() == ROLE_Authority && bIsWeaponAvailable && OverlappingPawn != nullptr)
	{
		AttemptPickUpWeapon(OverlappingPawn);
	}
}

// 检查武器生成时是否已有 Pawn 站在生成器上
void AEqZeroWeaponSpawner::CheckForExistingOverlaps()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		AttemptPickUpWeapon(Cast<APawn>(OverlappingActor));
	}
}

// 尝试让 Pawn 拾取武器：验证权限、可用性及技能组件后调用 GiveWeapon
void AEqZeroWeaponSpawner::AttemptPickUpWeapon_Implementation(APawn* Pawn)
{
	if (GetLocalRole() == ROLE_Authority && bIsWeaponAvailable && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		TSubclassOf<UEqZeroInventoryItemDefinition> WeaponItemDefinition = WeaponDefinition ? WeaponDefinition->InventoryItemDefinition : nullptr;
		if (WeaponItemDefinition != nullptr)
		{
			// 尝试将武器给予 Pawn
			if (GiveWeapon(WeaponItemDefinition, Pawn))
			{
				// 武器成功拾取：隐藏武器、播放特效、开始冷却
				bIsWeaponAvailable = false;
				SetWeaponPickupVisibility(false);
				PlayPickupEffects();
				StartCoolDown();
			}
		}
	}
}

bool AEqZeroWeaponSpawner::GiveWeapon_Implementation(TSubclassOf<UEqZeroInventoryItemDefinition> WeaponItemClass, APawn* ReceivingPawn)
{
	auto Controller = ReceivingPawn->GetController();
	if (!Controller)
	{
		return false;
	}
	
	auto QuickBarComponent = Controller->FindComponentByClass<UEqZeroQuickBarComponent>();
	auto InventoryManager = Controller->FindComponentByClass<UEqZeroInventoryManagerComponent>();
	if (!QuickBarComponent || !InventoryManager)
	{
		return false;
	}

	auto ExistingWeaponInstance = InventoryManager->FindFirstItemStackByDefinition(WeaponItemClass);
	if (ExistingWeaponInstance && bGiveAmmoForDuplicatedWeapon)
	{
		// 计算默认最大备用弹药量与当前剩余量的差值，补充到上限
		const int32 DefaultSpareAmmo = GetDefaultStatFromItemDef(WeaponItemClass, EqZeroGameplayTags::Lyra_Weapon_SpareAmmo);
		const int32 CurrentSpareAmmo = ExistingWeaponInstance->GetStatTagStackCount(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo);
		const int32 AmmoToAdd = DefaultSpareAmmo - CurrentSpareAmmo;
		if (AmmoToAdd > 0)
		{
			ExistingWeaponInstance->AddStatTagStack(EqZeroGameplayTags::Lyra_Weapon_SpareAmmo, AmmoToAdd);
		}
		return true;
	}

	auto NextSlot = QuickBarComponent->GetNextFreeItemSlot();
	if (NextSlot >= 0)
	{
		// 还有空位，直接添加新武器实例
		
		// TODO gameplay cue

		if (auto Ins = InventoryManager->AddItemDefinition(WeaponItemClass, 1))
		{
			QuickBarComponent->AddItemToSlot(NextSlot, Ins);

			// 源项目有一个捡起是否直接切换的逻辑，不打算做
			
			return true;
		}
		
	}
	
	return false;
}

// 启动冷却定时器
void AEqZeroWeaponSpawner::StartCoolDown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoolDownTimerHandle, this, &AEqZeroWeaponSpawner::OnCoolDownTimerComplete, CoolDownTime);
	}
}

// 重置冷却：清除定时器，在服务端恢复武器可见性并检查重叠
void AEqZeroWeaponSpawner::ResetCoolDown()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		bIsWeaponAvailable = true;
		PlayRespawnEffects();
		SetWeaponPickupVisibility(true);

		// 延迟后检查重叠，给客户端 OnRep 足够时间先行触发
		if (World)
		{
			World->GetTimerManager().SetTimer(CheckOverlapsDelayTimerHandle, this, &AEqZeroWeaponSpawner::CheckForExistingOverlaps, CheckExistingOverlapDelay);
		}
	}

	CoolDownPercentage = 0.0f;
}

// 冷却定时器到期回调
void AEqZeroWeaponSpawner::OnCoolDownTimerComplete()
{
	ResetCoolDown();
}

// 设置武器展示网格体的可见性（含子组件）
void AEqZeroWeaponSpawner::SetWeaponPickupVisibility(bool bShouldBeVisible)
{
	WeaponMesh->SetVisibility(bShouldBeVisible, true);
}

// 播放捡起武器时的音效和粒子特效
void AEqZeroWeaponSpawner::PlayPickupEffects_Implementation()
{
	if (WeaponDefinition != nullptr)
	{
		USoundBase* PickupSound = WeaponDefinition->PickedUpSound;
		if (PickupSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}

		UNiagaraSystem* PickupEffect = WeaponDefinition->PickedUpEffect;
		if (PickupEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, WeaponMesh->GetComponentLocation());
		}
	}
}

// 播放武器重新生成时的音效和粒子特效
void AEqZeroWeaponSpawner::PlayRespawnEffects_Implementation()
{
	if (WeaponDefinition != nullptr)
	{
		USoundBase* RespawnSound = WeaponDefinition->RespawnedSound;
		if (RespawnSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}

		UNiagaraSystem* RespawnEffect = WeaponDefinition->RespawnedEffect;
		if (RespawnEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RespawnEffect, WeaponMesh->GetComponentLocation());
		}
	}
}

// 客户端收到 bIsWeaponAvailable 复制变化时调用，同步表现层
void AEqZeroWeaponSpawner::OnRep_WeaponAvailability()
{
	if (bIsWeaponAvailable)
	{
		// 武器重新可用：播放重生特效并显示网格体
		PlayRespawnEffects();
		SetWeaponPickupVisibility(true);
	}
	else
	{
		// 武器已被拾取：隐藏网格体、开始冷却并播放拾取特效
		SetWeaponPickupVisibility(false);
		StartCoolDown();
		PlayPickupEffects();
	}
}

void AEqZeroWeaponSpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 同步武器可用状态到所有客户端
	DOREPLIFETIME(AEqZeroWeaponSpawner, bIsWeaponAvailable);
}

// 从道具定义 CDO 中查找指定 GameplayTag 对应的属性数值
int32 AEqZeroWeaponSpawner::GetDefaultStatFromItemDef(const TSubclassOf<UEqZeroInventoryItemDefinition> WeaponItemClass, FGameplayTag StatTag)
{
	if (WeaponItemClass != nullptr)
	{
		if (UEqZeroInventoryItemDefinition* WeaponItemCDO = WeaponItemClass->GetDefaultObject<UEqZeroInventoryItemDefinition>())
		{
			if (const UInventoryFragment_SetStats* ItemStatsFragment = Cast<UInventoryFragment_SetStats>(
				WeaponItemCDO->FindFragmentByClass(UInventoryFragment_SetStats::StaticClass())))
			{
				return ItemStatsFragment->GetItemStatByTag(StatTag);
			}
		}
	}

	return 0;
}
