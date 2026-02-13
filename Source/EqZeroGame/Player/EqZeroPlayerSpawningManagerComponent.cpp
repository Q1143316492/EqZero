// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/EqZeroPlayerSpawningManagerComponent.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "Player/EqZeroPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPlayerSpawningManagerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogPlayerSpawning, Log, All);

UEqZeroPlayerSpawningManagerComponent::UEqZeroPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEqZeroPlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAdded);

	UWorld* World = GetWorld();
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));

	for (TActorIterator<AEqZeroPlayerStart> It(World); It; ++It)
	{
		if (AEqZeroPlayerStart* PlayerStart = *It)
		{
			CachedPlayerStarts.Add(PlayerStart);
		}
	}
}

void UEqZeroPlayerSpawningManagerComponent::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (AEqZeroPlayerStart* PlayerStart = Cast<AEqZeroPlayerStart>(Actor))
			{
				ensure(!CachedPlayerStarts.Contains(PlayerStart));
				CachedPlayerStarts.Add(PlayerStart);
			}
		}
	}
}

void UEqZeroPlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (AEqZeroPlayerStart* PlayerStart = Cast<AEqZeroPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.Add(PlayerStart);
	}
}

AActor* UEqZeroPlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (Player)
	{
#if WITH_EDITOR
		if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
		{
			return PlayerStart;
		}
#endif

        // 找到一个出生点
		TArray<AEqZeroPlayerStart*> StarterPoints;
		for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
		{
			// 这是弱指针Array，所以我们需要检查它是否仍然有效
			if (AEqZeroPlayerStart* Start = StartIt->Get())
			{
				StarterPoints.Add(Start);
			}
			else
			{
				StartIt.RemoveCurrent();
			}
		}

        // 但是现在没有观战 Skip
        // 如果玩家状态是专门的观众，就让他们从任意随机起点开始，但他们不会占据该起点
        if (APlayerState* PlayerState = Player->GetPlayerState<APlayerState>())
		{
			if (PlayerState->IsOnlyASpectator())
			{
				if (!StarterPoints.IsEmpty())
				{
					return StarterPoints[FMath::RandRange(0, StarterPoints.Num() - 1)];
				}

				return nullptr;
			}
		}

		// 子类没有覆写就是 nullptr
		AActor* PlayerStart = OnChoosePlayerStart(Player, StarterPoints);
		if (!PlayerStart)
		{
			PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StarterPoints);
		}

		if (AEqZeroPlayerStart* LyraStart = Cast<AEqZeroPlayerStart>(PlayerStart))
		{
			// 能选到就占位
			LyraStart->TryClaim(Player);
		}

		return PlayerStart;
	}

	return nullptr;
}

#if WITH_EDITOR
APlayerStart* UEqZeroPlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player)
{
	// 仅对玩家控制器启用 “从此处开始播放”，机器人等应从普通生成点生成。
	if (Player->IsA<APlayerController>())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (APlayerStart* PlayerStart = *It)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						// 在 PIE 模式下，如果我们找到第一个 “从此处开始播放” 的 PlayerStart，始终优先选择它。
						return PlayerStart;
					}
				}
			}
		}
	}

	return nullptr;
}
#endif

bool UEqZeroPlayerSpawningManagerComponent::ControllerCanRestart(AController* Player)
{
	// 暂无逻辑
	bool bCanRestart = true;
	return bCanRestart;
}

void UEqZeroPlayerSpawningManagerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}

void UEqZeroPlayerSpawningManagerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

APlayerStart* UEqZeroPlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AEqZeroPlayerStart*>& StartPoints) const
{
	/*
	 * 有的点是 Empty（完全空闲），有的点是 Partial（部分被占用，但仍然可以放置玩家）
	 * 优先空闲，其次是部分占用。所以才有这个遍历
	 */
	if (Controller)
	{
		TArray<AEqZeroPlayerStart*> UnOccupiedStartPoints;
		TArray<AEqZeroPlayerStart*> OccupiedStartPoints;

		for (AEqZeroPlayerStart* StartPoint : StartPoints)
		{
			switch (auto State = StartPoint->GetLocationOccupancy(Controller))
			{
				case EEqZeroPlayerStartLocationOccupancy::Empty:
					UnOccupiedStartPoints.Add(StartPoint);
					break;
				case EEqZeroPlayerStartLocationOccupancy::Partial:
					OccupiedStartPoints.Add(StartPoint);
					break;
				default:
					break;
			}
		}

		if (UnOccupiedStartPoints.Num() > 0)
		{
			return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}

	return nullptr;
}
