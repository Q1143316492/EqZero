// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EqZeroPenetrationAvoidanceFeeler.generated.h"

/**
 * 摄像机和目标物体之间有一堵墙。需要通过射线检测把摄像机挪到合适的位置
 * 这个就是计算过程中用到的那个结构体
 */
USTRUCT()
struct FEqZeroPenetrationAvoidanceFeeler
{
	GENERATED_BODY()

    /**
     * 单射线的检测可能不准确
     * 所以用了多条，有不同的旋转角度和影响权重，就是下面的这几个变量
     */

	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	FRotator AdjustmentRot;

	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float WorldWeight;

	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float PawnWeight;

	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float Extent;

	/**
     * 只要角色不动，不需要每一帧都检测。
     * 如果这一帧没检测到，那后面大概率也不会有，所以间隔一会再检测。
     * 相关描述变量
     */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	int32 TraceInterval;

	UPROPERTY(transient)
	int32 FramesUntilNextTrace;


	FEqZeroPenetrationAvoidanceFeeler()
		: AdjustmentRot(ForceInit)
		, WorldWeight(0)
		, PawnWeight(0)
		, Extent(0)
		, TraceInterval(0)
		, FramesUntilNextTrace(0)
	{
	}

	FEqZeroPenetrationAvoidanceFeeler(const FRotator& InAdjustmentRot,
									const float& InWorldWeight, 
									const float& InPawnWeight, 
									const float& InExtent, 
									const int32& InTraceInterval = 0, 
									const int32& InFramesUntilNextTrace = 0)
		: AdjustmentRot(InAdjustmentRot)
		, WorldWeight(InWorldWeight)
		, PawnWeight(InPawnWeight)
		, Extent(InExtent)
		, TraceInterval(InTraceInterval)
		, FramesUntilNextTrace(InFramesUntilNextTrace)
	{
	}
};
