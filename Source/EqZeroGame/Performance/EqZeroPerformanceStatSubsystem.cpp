// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPerformanceStatSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/EqZeroGameState.h"
#include "EqZeroPerformanceStatTypes.h"
#include "Performance/LatencyMarkerModule.h"
#include "ProfilingDebugging/CsvProfiler.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPerformanceStatSubsystem)

CSV_DEFINE_CATEGORY(EqZeroPerformance, /*bIsEnabledByDefault=*/false);

class FSubsystemCollectionBase;

//////////////////////////////////////////////////////////////////////
// FEqZeroPerformanceStatCache

void FEqZeroPerformanceStatCache::StartCharting()
{
}

void FEqZeroPerformanceStatCache::ProcessFrame(const FFrameData& FrameData)
{
	// Record stats about the frame data
	{
		RecordStat(
			EEqZeroDisplayablePerformanceStat::ClientFPS,
			(FrameData.TrueDeltaSeconds != 0.0) ?
			1.0 / FrameData.TrueDeltaSeconds :
			0.0);

		RecordStat(EEqZeroDisplayablePerformanceStat::IdleTime, FrameData.IdleSeconds);
		RecordStat(EEqZeroDisplayablePerformanceStat::FrameTime, FrameData.TrueDeltaSeconds);
		RecordStat(EEqZeroDisplayablePerformanceStat::FrameTime_GameThread, FrameData.GameThreadTimeSeconds);
		RecordStat(EEqZeroDisplayablePerformanceStat::FrameTime_RenderThread, FrameData.RenderThreadTimeSeconds);
		RecordStat(EEqZeroDisplayablePerformanceStat::FrameTime_RHIThread, FrameData.RHIThreadTimeSeconds);
		RecordStat(EEqZeroDisplayablePerformanceStat::FrameTime_GPU, FrameData.GPUTimeSeconds);
	}

	if (UWorld* World = MySubsystem->GetGameInstance()->GetWorld())
	{

		if (const AEqZeroGameState* GameState = World->GetGameState<AEqZeroGameState>())
		{
			RecordStat(EEqZeroDisplayablePerformanceStat::ServerFPS, GameState->GetServerFPS());
		}

		if (APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(World))
		{
			if (APlayerState* PS = LocalPC->GetPlayerState<APlayerState>())
			{
				RecordStat(EEqZeroDisplayablePerformanceStat::Ping, PS->GetPingInMilliseconds());
			}

			if (UNetConnection* NetConnection = LocalPC->GetNetConnection())
			{
				const UNetConnection::FNetConnectionPacketLoss& InLoss = NetConnection->GetInLossPercentage();
				RecordStat(EEqZeroDisplayablePerformanceStat::PacketLoss_Incoming, InLoss.GetAvgLossPercentage());

				const UNetConnection::FNetConnectionPacketLoss& OutLoss = NetConnection->GetOutLossPercentage();
				RecordStat(EEqZeroDisplayablePerformanceStat::PacketLoss_Outgoing, OutLoss.GetAvgLossPercentage());

				RecordStat(EEqZeroDisplayablePerformanceStat::PacketRate_Incoming, NetConnection->InPacketsPerSecond);
				RecordStat(EEqZeroDisplayablePerformanceStat::PacketRate_Outgoing, NetConnection->OutPacketsPerSecond);

				RecordStat(EEqZeroDisplayablePerformanceStat::PacketSize_Incoming, (NetConnection->InPacketsPerSecond != 0) ? (NetConnection->InBytesPerSecond / (float)NetConnection->InPacketsPerSecond) : 0.0f);
				RecordStat(EEqZeroDisplayablePerformanceStat::PacketSize_Outgoing, (NetConnection->OutPacketsPerSecond != 0) ? (NetConnection->OutBytesPerSecond / (float)NetConnection->OutPacketsPerSecond) : 0.0f);
			}

			// Finally, record some input latency related stats if they are enabled
			TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get().GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
			for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
			{
				if (LatencyMarkerModule->GetEnabled())
				{
					const float TotalLatencyMs = LatencyMarkerModule->GetTotalLatencyInMs();
					if (TotalLatencyMs > 0.0f)
					{
						// Record some stats about the latency of the game
						RecordStat(EEqZeroDisplayablePerformanceStat::Latency_Total, TotalLatencyMs);
						RecordStat(EEqZeroDisplayablePerformanceStat::Latency_Game, LatencyMarkerModule->GetGameLatencyInMs());
						RecordStat(EEqZeroDisplayablePerformanceStat::Latency_Render, LatencyMarkerModule->GetRenderLatencyInMs());

						// Record some CSV profile stats.
						// You can see these by using the following commands
						// Start and stop the profile:
						//      CsvProfile Start
						//      CsvProfile Stop
						//
						// Or, you can profile for a certain number of frames:
						// CsvProfile Frames=10
						//
						// And this will output a .csv file to the Saved\Profiling\CSV folder
#if CSV_PROFILER
						if (FCsvProfiler* Profiler = FCsvProfiler::Get())
						{
							static const FName TotalLatencyStatName = TEXT("EqZero_Latency_Total");
							Profiler->RecordCustomStat(TotalLatencyStatName, CSV_CATEGORY_INDEX(EqZeroPerformance), TotalLatencyMs, ECsvCustomStatOp::Set);

							static const FName GameLatencyStatName = TEXT("EqZero_Latency_Game");
							Profiler->RecordCustomStat(GameLatencyStatName, CSV_CATEGORY_INDEX(EqZeroPerformance), LatencyMarkerModule->GetGameLatencyInMs(), ECsvCustomStatOp::Set);

							static const FName RenderLatencyStatName = TEXT("EqZero_Latency_Render");
							Profiler->RecordCustomStat(RenderLatencyStatName, CSV_CATEGORY_INDEX(EqZeroPerformance), LatencyMarkerModule->GetRenderLatencyInMs(), ECsvCustomStatOp::Set);
						}
#endif

						// Some more fine grain latency numbers can be found on the marker module if desired
						//LatencyMarkerModule->GetRenderLatencyInMs()));
						//LatencyMarkerModule->GetDriverLatencyInMs()));
						//LatencyMarkerModule->GetOSRenderQueueLatencyInMs()));
						//LatencyMarkerModule->GetGPURenderLatencyInMs()));
						break;
					}
				}
			}
		}
	}
}

void FEqZeroPerformanceStatCache::StopCharting()
{
}

void FEqZeroPerformanceStatCache::RecordStat(const EEqZeroDisplayablePerformanceStat Stat, const double Value)
{
	PerfStateCache.FindOrAdd(Stat).RecordSample(Value);
}

double FEqZeroPerformanceStatCache::GetCachedStat(EEqZeroDisplayablePerformanceStat Stat) const
{
	static_assert((int32)EEqZeroDisplayablePerformanceStat::Count == 18, "Need to update this function to deal with new performance stats");
	if (const FEqZeroSampledStatCache* Cache = GetCachedStatData(Stat))
	{
		return Cache->GetLastCachedStat();
	}

	return 0.0;
}

const FEqZeroSampledStatCache* FEqZeroPerformanceStatCache::GetCachedStatData(const EEqZeroDisplayablePerformanceStat Stat) const
{
	static_assert((int32)EEqZeroDisplayablePerformanceStat::Count == 18, "Need to update this function to deal with new performance stats");
	return PerfStateCache.Find(Stat);
}

//////////////////////////////////////////////////////////////////////
// UEqZeroPerformanceStatSubsystem

void UEqZeroPerformanceStatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Tracker = MakeShared<FEqZeroPerformanceStatCache>(this);
	GEngine->AddPerformanceDataConsumer(Tracker);
}

void UEqZeroPerformanceStatSubsystem::Deinitialize()
{
	GEngine->RemovePerformanceDataConsumer(Tracker);
	Tracker.Reset();
}

double UEqZeroPerformanceStatSubsystem::GetCachedStat(EEqZeroDisplayablePerformanceStat Stat) const
{
	return Tracker->GetCachedStat(Stat);
}

const FEqZeroSampledStatCache* UEqZeroPerformanceStatSubsystem::GetCachedStatData(const EEqZeroDisplayablePerformanceStat Stat) const
{
	return Tracker->GetCachedStatData(Stat);
}
