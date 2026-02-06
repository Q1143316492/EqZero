// Copyright Epic Games, Inc. All Rights Reserved.
// OK

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "EqZeroCameraAssistInterface.generated.h"

/** */
UINTERFACE(BlueprintType)
class UEqZeroCameraAssistInterface : public UInterface
{
	GENERATED_BODY()
};

class IEqZeroCameraAssistInterface
{
	GENERATED_BODY()

public:
	/**
	 * 获取允许相机穿透的Actor列表，在第三人称相机中非常实用。
	 * 比如第三人称跟拍相机需要忽略视角目标、玩家Pawn、载具等对象时，可将其加入该列表，避免相机被这些自身相关对象阻挡。
	 */
	virtual void GetIgnoredActorsForCameraPenetration(TArray<const AActor*>& OutActorsAllowPenetration) const { }

	/**
	 * 相机需要防止穿透的目标Actor。
	 * 正常情况下，该目标几乎都是相机的视角目标（ViewTarget），若不实现此接口，将沿用这一默认逻辑。
	 * 但部分场景中，相机的视角目标和需要始终保持在画面内的根Actor并非同一个对象，此时可通过该接口自定义目标。
	 */
	virtual TOptional<AActor*> GetCameraPreventPenetrationTarget() const
	{
		return TOptional<AActor*>();
	}

	/**
	 * 当相机穿透焦点目标时触发的回调。
	 * 若需要在相机与目标重叠时隐藏该目标Actor（比如相机穿模角色时隐藏角色模型），可在该接口中实现相关逻辑。
	 */
	virtual void OnCameraPenetratingTarget() { }
};
