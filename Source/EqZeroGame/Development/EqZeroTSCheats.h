// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/CheatManager.h"
#include "EqZeroTSCheats.generated.h"

#ifndef USING_CHEAT_MANAGER
#define USING_CHEAT_MANAGER (1 && !UE_BUILD_SHIPPING)
#endif // #ifndef USING_CHEAT_MANAGER

/**
 * UEqZeroTSCheats
 * 
 * CheatManager 扩展，用于执行 TypeScript 指令
 * 可以通过蓝图子类添加自定义函数，然后通过 Puerts Mixin 混入 TypeScript 实现
 */
UCLASS(Blueprintable, BlueprintType, Config=Game, MinimalAPI)
class UEqZeroTSCheats : public UCheatManagerExtension
{
	GENERATED_BODY()

public:
	UEqZeroTSCheats();

	/** 蓝图子类路径，用于自动注册 */
	UPROPERTY(Config)
	FSoftClassPath TSCheatsClass;

	/**
	 * 示例：执行 TypeScript 测试指令
	 * 在蓝图子类中可以添加更多 Exec 函数，通过 Puerts Mixin 实现具体逻辑
	 */
	UFUNCTION(Exec, BlueprintNativeEvent, Category = "TypeScript Cheats")
	void TSTest();

	/**
	 * 示例：带参数的 TypeScript 指令
	 */
	UFUNCTION(Exec, BlueprintNativeEvent, Category = "TypeScript Cheats")
	void TSCommand(const FString& Command);

	/**
	 * 示例：打印调试信息
	 */
	UFUNCTION(Exec, BlueprintNativeEvent, Category = "TypeScript Cheats")
	void TSDebug(const FString& Message);

    /**
     * 示例：C++ 实现的指令函数
     */
    UFUNCTION(Exec, BlueprintCallable, Category = "TypeScript Cheats")
    void TSCppExample(const FString& Info);
};
