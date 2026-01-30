# 1_项目初始化

[TOC]

=== 初始化项目结构 ===
除了 EqZeroEditor.Target.cs 之外，创建
EqZeroClienrt.Target.cs, EqZeroServer.Target.cs, EqZeroGame.Target.cs 三个 Target 文件
创建 EqZeroEditor 和 EqZeroGame 两个文件夹里面只有最基础的 build.cs文件 和几个必要的模块文件，其他全空着

=== 蓝图资源和 Plugins插件 ===
把所有的插件除了 game future 文件夹全部拷贝过来
直接把Content的内容复制过来，虽然有很多蓝图坏掉了，但是这样比较快。

需要注意的是这个配置，由于我们用了 Lyra 里面的插件，如果不配置这个，直接在加载 local player 的时候就炸掉了，无法运行
```ini
[/Script/EqZeroGame.EqZeroUIManagerSubsystem]
DefaultUIPolicyClass=/Game/UI/B_EqZeroUIPolicy.B_EqZeroUIPolicy_C
```

准备Puerts插件，主流程看TypeScript文件夹Main.ts
在GameInstance C++ 里面创建虚拟机
到这里我们能正常进入一个空场景，TS 脚本也能正常运行

=== 体验 ===

接下来我们要把体验的加载流程搞出来
但是由于一些关联类还没有，先做一些准备工作

- UEqZeroDeveloperSettings 只是一个配置面板，能够覆盖一些参数，这里只是数值，使用要逻辑自己写

```cpp
	/**
	* Should force feedback effects be played, even if the last input device was not a gamepad?
	* The default behavior in EqZero is to only play force feedback if the most recent input device was a gamepad.
	*/
	// UPROPERTY(config, EditAnywhere, Category = EqZero, meta = (ConsoleVariable = "EqZeroPC.ShouldAlwaysPlayForceFeedback"))
	// bool bShouldAlwaysPlayForceFeedback = false;
```

注意 ConsoleVariable 如果没有这个定义会crash我这里选择先注释掉

他需要有这种完全的写法，意思是控制台变量可以修改这个值
```cpp
namespace Lyra
{
	namespace Input
	{
		static int32 ShouldAlwaysPlayForceFeedback = 0;
		static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("LyraPC.ShouldAlwaysPlayForceFeedback"),
			ShouldAlwaysPlayForceFeedback,
			TEXT("Should force feedback effects be played, even if the last input device was not a gamepad?"));
	}
}
```

- 迁移 AssetManager 注意这个在ini要配置一下
EqZeroAssetManager, EqZeroAssetManagerStartupJob, EqZeroGameData 完成
注意ini里面要配置一下 asset manager, 另外game data创建一下吧

但是逻辑有一些crash，StartInitialLoading PreBeginPIE 先注释，等后续类完成后再考虑，而且预加载也不影响逻辑。
加上只是避免后面忘记了

- 顺手把 gameplay tag .ini copy 过来，反正打算直接用lyra的

- EqZeroUserFacingExperienceDefinition 地上切关卡的时候，有一个交互物，有一些UI的效果配置

# 2_体验的加载流程

我们首先要检查体验的配置情况是否迁移完全

UEqZeroExperienceDefinition
	- UEqZeroPawnData 没全完成
	- UEqZeroExperienceActionSet 技能集合，一组GameFeatureAction
	- 一组 UGameFeatureAction

然后是体验加载的流程

从 AEqZeroGameMode::InitGame 开始的一串流程，完成

然后创建game feature 并且在里面创建体验，注意项目设置和game feature的asset manager都要配置好

才能够完成体验的加载，否则check一下就炸了

# 3_gameinstance 完成

这里有一个状态机的初始化

还有一些加密的流程

# 4_把gameplay的核心几个了类搞出来

把很多关联类搞了出来，做一个分类是玩家，系统，UI

- AEqZeroCharacter

	- AEqZeroCharacter

	- UEqZeroHealthComponent

	- UEqZeroHeroComponent

	- UEqZeroPawnExtensionComponent

- AEqZeroPlayerController

- AEqZeroPlayerState

- UEqZeroLocalPlayer


动画，摄像机，技能先放一放

===

系统

- AEqZeroGameMode

- AEqZeroGameState

	- UEqZeroExperienceManagerComponent

===

UI，设置归一类，剩下的非重要模块，比如队伍。

然后能过编译

# 5_两个设置类

还有两个 settings local(需要ini配置，本质是GameUserSettings子类) 和 shared settings

目前基本是空类。。。

TODO ini 还没配置

# 6_登录与界面加载

Lyra_Plugins_CommonLoadingScreen
里面可以配置一个loading screen 还有一些原因更新显示的流程(skip先不做吧)

登录，从game state comp 里面到推送登入界面上来

Lyra UI登录流程 这篇文章。
这一步多了
UEqZeroActivatableWidget
UEqZeroButtonBase
UEqZeroFrontendStateComponent

DS模式下为什么主界面出不来