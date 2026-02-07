// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * ECC_GameTraceChannel1 比较抽象, 做了一个定义
 * 别忘了 DefaultEngine.ini [/Script/Engine.CollisionProfile] 要对应
 **/

// 用于 Actors/Components 的交互的射线
#define EqZero_TraceChannel_Interaction					ECC_GameTraceChannel1

// 武器射线检测, 会命中 physics assets 而不是 capsules
#define EqZero_TraceChannel_Weapon						ECC_GameTraceChannel2

// 武器用的射线检测, 会击中 pawn capsules 而不是 physics assets
#define EqZero_TraceChannel_Weapon_Capsule				ECC_GameTraceChannel3

// 武器用的射线检测, 会穿过多个Pawn，而不是在第一次击中时停下
#define EqZero_TraceChannel_Weapon_Multi					ECC_GameTraceChannel4

// Allocated to aim assist by the ShooterCore game feature 预留配给瞄准辅助
// ECC_GameTraceChannel5
