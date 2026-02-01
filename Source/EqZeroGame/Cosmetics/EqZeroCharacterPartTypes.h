// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "EqZeroCharacterPartTypes.generated.h"

class UChildActorComponent;
class UEqZeroPawnComponent_CharacterParts;
struct FEqZeroCharacterPartList;

//////////////////////////////////////////////////////////////////////

// 应该如何在生成的部件角色上配置碰撞？
UENUM()
enum class ECharacterCustomizationCollisionMode : uint8
{
	NoCollision, // 不要碰撞
	UseCollisionFromCharacterPart // 不要改动角色部件的碰撞设置
};

//////////////////////////////////////////////////////////////////////

// 添加角色部件条目时创建的句柄，可用于稍后移除它
USTRUCT(BlueprintType)
struct FEqZeroCharacterPartHandle
{
	GENERATED_BODY()

	void Reset()
	{
		PartHandle = INDEX_NONE;
	}

	bool IsValid() const
	{
		return PartHandle != INDEX_NONE;
	}

private:
	UPROPERTY()
	int32 PartHandle = INDEX_NONE;

	friend FEqZeroCharacterPartList;
};

//////////////////////////////////////////////////////////////////////
// A character part request

USTRUCT(BlueprintType)
struct FEqZeroCharacterPart
{
	GENERATED_BODY()

	// 要生成的部件，比如直接把角色作为一个子 actor 附加上去
	// 这里的类型是 AEqZeroTaggedActor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> PartClass;

	// 要附加部件的插槽（如果有）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	// 如何处理部件中原始组件的碰撞
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECharacterCustomizationCollisionMode CollisionMode = ECharacterCustomizationCollisionMode::NoCollision;

	// 比较两个部件，忽略碰撞模式
	static bool AreEquivalentParts(const FEqZeroCharacterPart& A, const FEqZeroCharacterPart& B)
	{
		return (A.PartClass == B.PartClass) && (A.SocketName == B.SocketName);
	}
};
