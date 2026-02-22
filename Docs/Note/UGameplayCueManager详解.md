# UGameplayCueManager 详解

> 基于 UE 5.6 源码分析，涵盖客户端/服务器流程、预测（Prediction）机制、资源加载、Actor 回收等核心内容。

---

## 目录

1. [概述](#1-概述)
2. [核心数据结构](#2-核心数据结构)
3. [初始化与资源加载流程](#3-初始化与资源加载流程)
4. [GameplayCue 事件类型](#4-gameplaycue-事件类型)
5. [服务器端流程（Authority）](#5-服务器端流程authority)
6. [客户端预测流程（Prediction）](#6-客户端预测流程prediction)
7. [网络复制（Replication）流程](#7-网络复制replication流程)
8. [GameplayCue 路由与执行](#8-gameplaycue-路由与执行)
9. [Dedicated Server 上的抑制机制](#9-dedicated-server-上的抑制机制)
10. [CueNotify Actor 生命周期与回收](#10-cuenotify-actor-生命周期与回收)
11. [异步加载与延迟执行](#11-异步加载与延迟执行)
12. [Tag 翻译系统](#12-tag-翻译系统)
13. [非复制 GameplayCue](#13-非复制-gameplaycue)
14. [关键 CVar 配置](#14-关键-cvar-配置)
15. [完整流程图](#15-完整流程图)

---

## 1. 概述

`UGameplayCueManager` 是 GAS（Gameplay Ability System）中负责管理和分发 **GameplayCue** 事件的**全局单例管理器**（继承自 `UDataAsset`）。它的核心职责包括：

- **Cue 事件的调度（Dispatch）**：接收来自 `UAbilitySystemComponent` 的 Cue 请求，决定如何路由到具体的 Notify 类或接口
- **网络复制协调**：管理 Cue 的 RPC 发送（`FlushPendingCues`），配合 PredictionKey 实现客户端预测
- **资源管理**：通过 ObjectLibrary 发现、加载、缓存 GameplayCueNotify 蓝图类
- **Actor 池化/回收**：管理 `AGameplayCueNotify_Actor` 的预分配和回收
- **Tag 翻译**：通过 `FGameplayCueTranslationManager` 在运行时将 Cue Tag 映射到不同变体

获取方式：
```cpp
UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager();
```

---

## 2. 核心数据结构

### 2.1 FGameplayCuePendingExecute

表示一个**待发送的 Cue 执行事件**，在 `FlushPendingCues()` 时被统一处理。

| 字段 | 类型 | 说明 |
|------|------|------|
| `GameplayCueTags` | `TArray<FGameplayTag, TInlineAllocator<1>>` | 要触发的 Cue Tag 列表（通常为 1 个） |
| `PredictionKey` | `FPredictionKey` | 关联的预测 Key |
| `PayloadType` | `EGameplayCuePayloadType` | 数据载荷类型 |
| `OwningComponent` | `UAbilitySystemComponent*` | 所属 ASC |
| `FromSpec` | `FGameplayEffectSpecForRPC` | PayloadType=FromSpec 时使用 |
| `CueParameters` | `FGameplayCueParameters` | PayloadType=CueParameters 时使用 |

**PayloadType 枚举**：
- **`FromSpec`**：携带 `FGameplayEffectSpecForRPC`，信息更多但带宽更大
- **`CueParameters`**：携带 `FGameplayCueParameters`，更轻量，由 CVar `AbilitySystem.AlwaysConvertGESpecToGCParams` 控制是否在服务器端转换

### 2.2 FGameplayCueObjectLibrary

封装了用于发现 GameplayCueNotify 资产的 ObjectLibrary：

- **RuntimeGameplayCueObjectLibrary**：运行时使用，扫描 "always loaded" 路径
- **EditorGameplayCueObjectLibrary**：仅编辑器使用，反映资产注册表中所有可能的 GC Notify

### 2.3 FPredictionKey

预测系统的核心标识：

| 字段 | 说明 |
|------|------|
| `Current` | 唯一 ID（int16） |
| `Base` | 依赖链中的原始 Key |
| `bIsServerInitiated` | 是否由服务器发起（不可用于客户端预测） |

关键判断方法：
- `IsLocalClientKey()`：`Current > 0 && !bIsServerInitiated` — 本客户端生成的预测 Key
- `IsServerInitiatedKey()`：`bIsServerInitiated == true`

---

## 3. 初始化与资源加载流程

### 3.1 创建时机

```
UAbilitySystemGlobals::InitGlobalData()
  └── GetGameplayCueManager()  // 首次调用时创建
        └── UGameplayCueManager::OnCreated()
              ├── 绑定 FWorldDelegates::OnPostWorldCleanup
              ├── 绑定 FNetworkReplayDelegates::OnPreScrub
              └── (编辑器) 绑定 OnFEngineLoopInitComplete
```

### 3.2 Runtime ObjectLibrary 初始化

```
InitializeRuntimeObjectLibrary()
  ├── 获取路径: GetAlwaysLoadedGameplayCuePaths()  // 来自 UAbilitySystemGlobals
  ├── 创建/清空 GlobalGameplayCueSet
  ├── 配置扫描选项:
  │     bShouldSyncScan = true    // 同步扫描资产注册表
  │     bShouldSyncLoad = false   // 不同步加载
  │     bShouldAsyncLoad = true   // 异步加载
  └── InitObjectLibrary(RuntimeGameplayCueObjectLibrary)
        ├── 创建 ActorObjectLibrary + StaticObjectLibrary
        ├── LoadBlueprintAssetDataFromPaths()  // 扫描路径下的蓝图
        ├── BuildCuesToAddToGlobalSet()        // 从 AssetData 中提取 GameplayCueTag → 路径 映射
        ├── SetToAddTo->AddCues(CuesToAdd)     // 添加到 UGameplayCueSet
        ├── StreamableManager.RequestAsyncLoad()  // 异步加载资产
        └── TranslationManager.BuildTagTranslationTable()
```

### 3.3 UGameplayCueSet 的加速映射

`UGameplayCueSet` 内部维护了 `GameplayCueDataMap`（`TMap<FGameplayTag, int32>`），将 **Tag → 数组索引** 映射。构建加速映射时会处理 Tag 层级继承：

- 如果 `GameplayCue.A.B.C` 没有处理器，但 `GameplayCue.A.B` 有，则子 Tag 的条目指向父 Tag 的处理器
- 通过 `ParentDataIdx` 链实现 Tag 层级回退（Fallback）

---

## 4. GameplayCue 事件类型

```cpp
enum class EGameplayCueEvent : uint8
{
    OnActive,      // Cue 被添加时触发（一次性）
    WhileActive,   // Cue 存在期间持续触发
    Executed,      // 一次性执行（Fire-and-forget）
    Removed        // Cue 被移除时触发
};
```

**两种 Cue 模式**：

| 模式 | 事件 | 典型用途 | 网络行为 |
|------|------|---------|---------|
| **持续型（Duration/Infinite GE）** | OnActive → WhileActive → Removed | 持续性 Buff 效果（燃烧光环） | 通过 `FActiveGameplayCueContainer` 复制 |
| **一次性（Instant GE / 直接调用）** | Executed | 即时效果（击中闪光） | 通过 Multicast RPC |

---

## 5. 服务器端流程（Authority）

### 5.1 一次性 Cue（Executed）

```
GameplayEffect 应用 / Ability 主动调用
  └── UAbilitySystemComponent::ExecuteGameplayCue()
        └── UGameplayCueManager::InvokeGameplayCueExecuted_FromSpec()
              ├── 检查 bSuppressGameplayCues
              ├── 获取 ReplicationInterface
              ├── (如果 AlwaysConvertGESpecToGCParams)
              │     创建 FGameplayCuePendingExecute(PayloadType=CueParameters)
              ├── (否则)
              │     创建 FGameplayCuePendingExecute(PayloadType=FromSpec)
              └── AddPendingCueExecuteInternal()
                    ├── ProcessPendingCueExecute()  // 子类可过滤
                    ├── 加入 PendingExecuteCues 列表
                    └── 如果 GameplayCueSendContextCount == 0:
                          FlushPendingCues()
```

### 5.2 持续型 Cue（Add / Remove）

```
GameplayEffect 激活
  └── UAbilitySystemComponent::AddGameplayCue_Internal()
        ├── 添加到 FActiveGameplayCueContainer (复制列表)
        ├── ForceReplication()
        ├── ReplicationInterface->Call_InvokeGameplayCueAddedAndWhileActive_*()
        │     └── NetMulticast_InvokeGameplayCueAddedAndWhileActive_*()
        │           └── (客户端) InvokeGameplayCueEvent(OnActive + WhileActive)
        └── (服务器本地) InvokeGameplayCueEvent(WhileActive)

GameplayEffect 移除
  └── UAbilitySystemComponent::RemoveGameplayCue_Internal()
        ├── 从 FActiveGameplayCueContainer 移除
        └── (客户端通过 OnRep 触发 Removed)
```

### 5.3 FlushPendingCues 详解

`FlushPendingCues()` 是 Cue RPC 发送的核心：

```cpp
void UGameplayCueManager::FlushPendingCues()
{
    for (每个 PendingCue)
    {
        bool bHasAuthority = OwningComponent->IsOwnerActorAuthoritative();
        bool bLocalPredictionKey = PredictionKey.IsLocalClientKey();

        if (PayloadType == CueParameters)
        {
            if (bHasAuthority)
            {
                // 服务器：发送 Multicast RPC
                RepInterface->ForceReplication();
                RepInterface->Call_InvokeGameplayCueExecuted_WithParams(Tag, PredKey, Params);
            }
            else if (bLocalPredictionKey)
            {
                // 预测客户端：本地执行（不发RPC）
                OwningComponent->InvokeGameplayCueEvent(Tag, Executed, Params);
            }
        }
        else if (PayloadType == FromSpec)
        {
            if (bHasAuthority)
            {
                RepInterface->ForceReplication();
                RepInterface->Call_InvokeGameplayCueExecuted_FromSpec(FromSpec, PredKey);
            }
            else if (bLocalPredictionKey)
            {
                OwningComponent->InvokeGameplayCueEvent(FromSpec, Executed);
            }
        }
    }
}
```

**`FScopedGameplayCueSendContext`** 用于批量处理：在作用域内递增 `GameplayCueSendContextCount`，离开作用域时递减，当计数归零时才 Flush，避免在同一帧内多次发送 RPC。

---

## 6. 客户端预测流程（Prediction）

### 6.1 预测的完整生命周期

```
┌─────────────────────────────────────────────────────────────────┐
│ 客户端（Autonomous Proxy）                                       │
│                                                                  │
│ 1. 激活技能 → 创建 FPredictionKey (IsLocalClientKey() = true)    │
│ 2. 技能触发 ExecuteGameplayCue / AddGameplayCue                  │
│ 3. FlushPendingCues:                                             │
│    !bHasAuthority && bLocalPredictionKey → 本地直接执行 Cue       │
│    (不发送 RPC，仅客户端本地播放效果)                              │
│                                                                  │
│ 4. 将 PredictionKey + 技能请求发送给服务器                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ 服务器                                                           │
│                                                                  │
│ 5. 服务器处理技能，触发相同 Cue                                   │
│ 6. FlushPendingCues:                                             │
│    bHasAuthority → 发送 NetMulticast RPC（携带 PredictionKey）    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ 所有客户端收到 Multicast RPC                                      │
│                                                                  │
│ 7. NetMulticast_Implementation 中：                              │
│    if (IsOwnerActorAuthoritative() ||                           │
│        PredictionKey.IsLocalClientKey() == false)                │
│    {                                                             │
│        InvokeGameplayCueEvent(...);  // 执行 Cue                 │
│    }                                                             │
│                                                                  │
│    • 预测发起客户端: IsLocalClientKey()==true → 跳过（已预测播放） │
│    • 其他客户端: IsLocalClientKey()==false → 正常执行              │
│    • 服务器自身: IsOwnerActorAuthoritative()==true                │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 预测失败处理

如果服务器拒绝了预测（例如技能激活失败）：

1. 服务器通过 `FPredictionKey` 的 Rejected 代理通知客户端
2. `FActiveGameplayCueContainer` 上的 `PredictiveRemove` 被调用
3. 客户端撤销已预测播放的 Cue 效果

### 6.3 持续型 Cue 的预测

```cpp
// AddGameplayCue_Internal 中（客户端预测路径）
if (ScopedPredictionKey.IsLocalClientKey())
{
    // 预测性添加到容器
    Container->PredictiveAdd(GameplayCueTag, PredictionKey);
    // 本地立即播放
    InvokeGameplayCueEvent(Tag, OnActive, Params);
    InvokeGameplayCueEvent(Tag, WhileActive, Params);
}
```

### 6.4 Mixed Replication 模式

对于使用 `MinimalReplication` 的场景（如 AI 代理）：

- 服务器在 `AddGameplayCue_Internal` 中创建 `ServerInitiatedKey`
- Autonomous Proxy 收到 Multicast 时检查 `bIsMixedReplicationFromServer`
- 如果为 true 且是本地控制的玩家，则跳过 Multicast（数据将通过 GE OnRep 到来）

---

## 7. 网络复制（Replication）流程

### 7.1 IAbilitySystemReplicationProxyInterface

此接口将 **RPC 发送决策** 与 **RPC 实际实现** 分离：

| Call_ 方法（入口） | NetMulticast_ 方法（RPC 实现） | 用途 |
|---|---|---|
| `Call_InvokeGameplayCueExecuted_FromSpec` | `NetMulticast_InvokeGameplayCueExecuted_FromSpec` | 从 GE Spec 触发一次性 Cue |
| `Call_InvokeGameplayCueExecuted_WithParams` | `NetMulticast_InvokeGameplayCueExecuted_WithParams` | 从 Tag+Params 触发一次性 Cue |
| `Call_InvokeGameplayCuesExecuted_WithParams` | `NetMulticast_InvokeGameplayCuesExecuted_WithParams` | 多 Tag 变体 |
| `Call_InvokeGameplayCueAdded_WithParams` | `NetMulticast_InvokeGameplayCueAdded_WithParams` | 持续 Cue 添加（OnActive） |
| `Call_InvokeGameplayCueAddedAndWhileActive_FromSpec` | `NetMulticast_InvokeGameplayCueAddedAndWhileActive_FromSpec` | 添加+WhileActive 合并 |

- **`Call_*`** 方法是虚函数，可被覆写以拦截/批量处理/重定向 RPC（例如基于 PlayerState 的代理复制）
- **`NetMulticast_*`** 方法是 `UFUNCTION(NetMulticast, unreliable)` 标记的 Multicast RPC
- 所有 Multicast 的 `_Implementation` 中都带有预测守卫逻辑

### 7.2 RPC 限流检测

`CheckForTooManyRPCs()` 在每次 Flush 时检查单个 NetUpdate 内的 RPC 数量是否超过 `net.MaxRPCPerNetUpdate`，超出时输出警告日志。

### 7.3 两种数据载荷

| 载荷类型 | 数据量 | 信息量 | 何时使用 |
|---------|--------|--------|---------|
| `FromSpec`（`FGameplayEffectSpecForRPC`） | 较大 | 携带 GE Spec 信息 | 默认模式 |
| `CueParameters`（`FGameplayCueParameters`） | 较小 | 仅基础参数 | `AbilitySystem.AlwaysConvertGESpecToGCParams=1` |

---

## 8. GameplayCue 路由与执行

### 8.1 HandleGameplayCue 入口

```
HandleGameplayCue(TargetActor, Tag, EventType, Parameters)
  ├── ShouldSuppressGameplayCues()  → 是否抑制
  ├── TranslateGameplayCue()         → Tag 翻译
  └── RouteGameplayCue()             → 路由到具体处理器
```

### 8.2 RouteGameplayCue 路由逻辑

```
RouteGameplayCue(TargetActor, Tag, EventType, Parameters)
  ├── 设置 Parameters.OriginalTag
  ├── 检查 IGameplayCueInterface::ShouldAcceptGameplayCue()
  ├── 广播 OnRouteGameplayCue 事件（非 Shipping 构建）
  ├── Debug 可视化绘制（如果 DisplayGameplayCues 开启）
  ├── RuntimeCueSet->HandleGameplayCue()    ← 全局 CueSet 处理
  ├── GameplayCueInterface->HandleGameplayCue()  ← 接口处理
  └── TargetActor->ForceNetUpdate()         ← 强制网络更新（用于 Replay 录制）
```

### 8.3 UGameplayCueSet::HandleGameplayCue 内部

```
HandleGameplayCue(TargetActor, Tag, EventType, Parameters)
  ├── 在 GameplayCueDataMap 中查找 Tag → 获取 DataIdx
  ├── HandleGameplayCueNotify_Internal(DataIdx)
  │     ├── 尝试 ResolveObject() 加载类
  │     ├── 如果未加载 → HandleMissingGameplayCue() (同步/异步加载)
  │     ├── UGameplayCueNotify_Static (CDO 调用):
  │     │     ├── CDO->HandleGameplayCue(TargetActor, EventType, Parameters)
  │     │     └── 如果 !IsOverride → 递归 ParentDataIdx（Tag 层级回退）
  │     └── AGameplayCueNotify_Actor (实例化):
  │           ├── CueManager->GetInstancedCueActor() → 获取/创建实例
  │           ├── 调用实例的对应事件方法
  │           └── 如果 Executed + bAutoDestroyOnRemove → 立即触发 Removed
  └── 返回
```

---

## 9. Dedicated Server 上的抑制机制

GameplayCue 在 Dedicated Server 上有 **三层抑制**：

### 第一层：全局抑制（CueManager 级）

```cpp
bool UGameplayCueManager::ShouldSuppressGameplayCues(AActor* TargetActor)
{
    if (DisableGameplayCues ||           // CVar 全局禁用
        !TargetActor ||                  // 无效目标
        (GameplayCueRunOnDedicatedServer == 0 && IsDedicatedServerForGameplayCue()))
    {
        return true;                     // 默认：DS 上不执行 Cue 逻辑
    }
    return false;
}
```

**关键行为**：Dedicated Server 上默认不执行 Cue 的本地处理（不 Spawn Actor、不播放特效），但 **RPC 发送路径仍然正常运行**（`FlushPendingCues` 是权威端逻辑），服务器只发 RPC，客户端负责播放。

### 第二层：组件级抑制

`UAbilitySystemComponent::bSuppressGameplayCues` — 每个 ASC 可单独启用抑制。检查点：
- `InvokeGameplayCueEvent` 各重载
- `InvokeGameplayCueExecuted_*` 各变体

### 第三层：调试过滤

`GameplayCue.FilterCuesByTag` 控制台命令：仅执行匹配指定 Tag 的 Cue（仅非 Shipping 构建）。

### EGameplayCueExecutionOptions

调用时可传入选项标志：
```cpp
IgnoreInterfaces   // 跳过 IGameplayCueInterface 检查
IgnoreNotifies     // 跳过 Notify Spawn
IgnoreTranslation  // 跳过 Tag 翻译
IgnoreSuppression  // 忽略抑制检查，强制执行
IgnoreDebug        // 不显示调试可视化
```

---

## 10. CueNotify Actor 生命周期与回收

### 10.1 获取实例：GetInstancedCueActor

```
GetInstancedCueActor(TargetActor, CueClass, Parameters)
  ├── FindExistingCueOnActor()          ← 已存在？复用
  │     └── 检查 bUniqueInstancePerInstigator / bUniqueInstancePerSourceObject
  ├── FindRecycledCue()                 ← 回收池中有？复用
  │     └── 从 PreallocatedInstances 列表 Pop
  │         └── ReuseAfterRecycle()
  └── SpawnActor<AGameplayCueNotify_Actor>()  ← 新建
        └── Owner = TargetActor（引用存于 TargetActor::Children）
```

### 10.2 回收流程

```
NotifyGameplayCueActorFinished(Actor)
  ├── 检查 GameplayCueActorRecycle CVar
  ├── Actor->Recycle()
  │     └── 成功：bInRecycleQueue = true，放回 PreallocatedInstances
  └── 失败：Actor->Destroy()
```

### 10.3 预分配

- `CheckForPreallocation()`：加载 Notify Class 后检查 `NumPreallocatedInstances`
- `UpdatePreallocation(World)`：每帧调用，逐个 Spawn 预分配实例
- `ResetPreallocation(World)`：世界切换时重置

---

## 11. 异步加载与延迟执行

当 Cue 被触发但对应的 Notify 类尚未加载时：

```
HandleMissingGameplayCue(OwningSet, CueData, TargetActor, EventType, Parameters)
  │
  ├── ShouldSyncLoadMissingGameplayCues() == true（默认 false）:
  │     └── StreamableManager.LoadSynchronous()  → 同步加载并立即执行
  │
  └── ShouldAsyncLoadMissingGameplayCues() == true（默认 true）:
        ├── 缓存 FAsyncLoadPendingGameplayCue (Tag, TargetActor, EventType, Parameters)
        ├── AsyncLoadPendingGameplayCues Map 中记录
        ├── StreamableManager.RequestAsyncLoad()
        └── 加载完成回调 OnMissingCueAsyncLoadComplete():
              ├── 取出所有待执行的 PendingCues
              ├── 验证 OwningSet/TargetActor/World 是否仍然有效
              └── 逐个调用 OwningSet->HandleGameplayCue()
```

---

## 12. Tag 翻译系统

`FGameplayCueTranslationManager` 支持在运行时将一个 GameplayCue Tag 翻译成另一个。

```
TranslateGameplayCue(Tag, TargetActor, Parameters)
  └── TranslationManager.TranslateTag(Tag, TargetActor, Parameters)
```

典型用例：根据角色皮肤/性别/武器类型将 `GameplayCue.Hit.Impact` 翻译为 `GameplayCue.Hit.Impact.Metal` 等变体。

翻译表在 `InitObjectLibrary()` 末尾通过 `TranslationManager.BuildTagTranslationTable()` 构建。

---

## 13. 非复制 GameplayCue

对于不需要网络复制的 Cue（如动画驱动的效果），提供静态方法：

```cpp
// 添加持续性 Cue（本地）
UGameplayCueManager::AddGameplayCue_NonReplicated(Target, Tag, Params);
  ├── ASC->AddLooseGameplayTag(Tag)
  ├── HandleGameplayCue(OnActive)
  └── HandleGameplayCue(WhileActive)

// 移除持续性 Cue（本地）
UGameplayCueManager::RemoveGameplayCue_NonReplicated(Target, Tag, Params);
  ├── ASC->RemoveLooseGameplayTag(Tag)
  └── HandleGameplayCue(Removed)

// 一次性 Cue（本地）
UGameplayCueManager::ExecuteGameplayCue_NonReplicated(Target, Tag, Params);
  └── HandleGameplayCue(Executed)
```

**设计原则**（源码注释）：
> - Abilities 始终使用 **复制的** GameplayCue（因为不在 Simulated Proxy 上执行）
> - Animations 始终使用 **非复制的** GameplayCue（因为总是在 Simulated Proxy 上执行）

---

## 14. 关键 CVar 配置

| CVar | 默认值 | 说明 |
|------|--------|------|
| `AbilitySystem.DisableGameplayCues` | 0 | 全局禁用所有 Cue |
| `AbilitySystem.DisplayGameplayCues` | 0 | 在世界中以文字显示 Cue 事件 |
| `AbilitySystem.GameplayCue.DisplayDuration` | 5.0 | Debug 文字显示时长 |
| `AbilitySystem.GameplayCue.RunOnDedicatedServer` | 0 | DS 上是否执行 Cue 本地逻辑 |
| `AbilitySystem.GameplayCue.EnableSuppressCuesOnGameplayCueManager` | true | 是否允许 CueManager 级别的组件抑制 |
| `AbilitySystem.AlwaysConvertGESpecToGCParams` | 0 | 1=服务器端将 GE Spec 转为轻量 Params |
| `AbilitySystem.GameplayCueActorRecycle` | 1 | 启用 Actor 回收 |
| `AbilitySystem.GameplayCueActorRecycleDebug` | 0 | Actor 回收调试日志 |
| `AbilitySystem.LogGameplayCueActorSpawning` | 0 | Actor Spawn 日志 |
| `AbilitySystem.GameplayCueCheckForTooManyRPCs` | 1 | 检测 RPC 限流 |

---

## 15. 完整流程图

### 15.1 一次性 Cue（Executed）端到端流程

```
╔═══════════════════════════════════════════════════════════════════╗
║                         Authority (服务器)                        ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  GameplayEffect.Instant / Ability 直接调用                        ║
║       │                                                           ║
║       ▼                                                           ║
║  ASC::ExecuteGameplayCue()                                        ║
║       │                                                           ║
║       ▼                                                           ║
║  GCM::InvokeGameplayCueExecuted_FromSpec()                        ║
║       │                                                           ║
║       ├── bSuppressGameplayCues? → return                         ║
║       ├── 获取 ReplicationInterface                               ║
║       ├── 创建 FGameplayCuePendingExecute                         ║
║       │                                                           ║
║       ▼                                                           ║
║  AddPendingCueExecuteInternal()                                   ║
║       │                                                           ║
║       ├── ProcessPendingCueExecute() (子类过滤)                   ║
║       ├── PendingExecuteCues.Add()                                ║
║       │                                                           ║
║       ▼ (SendContextCount == 0)                                   ║
║  FlushPendingCues()                                               ║
║       │                                                           ║
║       ├── bHasAuthority == true:                                  ║
║       │   ├── ForceReplication()                                  ║
║       │   └── Call_InvokeGameplayCueExecuted_WithParams()         ║
║       │         └── [Multicast RPC] ─────────────────────┐        ║
║       │                                                   │        ║
╚═══════════════════════════════════════════════════════════│════════╝
                                                            │
            ┌───────────────────────────────────────────────┘
            │
            ▼
╔═══════════════════════════════════════════════════════════════════╗
║                    所有客户端（收到 Multicast）                    ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  NetMulticast_InvokeGameplayCueExecuted_WithParams_Implementation ║
║       │                                                           ║
║       ├── if (IsOwnerActorAuthoritative() ||                     ║
║       │       PredictionKey.IsLocalClientKey() == false)          ║
║       │   {                                                       ║
║       │       InvokeGameplayCueEvent(Tag, Executed, Params)      ║
║       │   }                                                       ║
║       │                                                           ║
║       │   预测客户端: IsLocalClientKey()=true → SKIP             ║
║       │   其他客户端: IsLocalClientKey()=false → EXECUTE         ║
║       │                                                           ║
║       ▼                                                           ║
║  GCM::HandleGameplayCue()                                         ║
║       ├── ShouldSuppressGameplayCues()                            ║
║       ├── TranslateGameplayCue()                                  ║
║       └── RouteGameplayCue()                                      ║
║             ├── CueSet->HandleGameplayCue()                       ║
║             │     └── 加载并调用 Static/Actor Notify              ║
║             └── IGameplayCueInterface->HandleGameplayCue()        ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
```

### 15.2 持续型 Cue（Add → WhileActive → Remove）端到端流程

```
╔═══════════════════════════════════════════════════════════════════╗
║                         Authority (服务器)                        ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  Duration/Infinite GE 应用                                        ║
║       │                                                           ║
║       ▼                                                           ║
║  ASC::AddGameplayCue_Internal(Tag, PredictionKey)                 ║
║       │                                                           ║
║       ├── Container.AddCue(Tag) → FActiveGameplayCueContainer    ║
║       │   (Replicated Property → 客户端 OnRep)                    ║
║       │                                                           ║
║       ├── ForceReplication()                                      ║
║       │                                                           ║
║       ├── RepInterface->Call_InvokeGameplayCueAddedAndWhileActive ║
║       │     └── [Multicast RPC] → 客户端播放 OnActive+WhileActive ║
║       │                                                           ║
║       └── (DS抑制: 不本地执行 HandleGameplayCue)                  ║
║                                                                   ║
║  ··· Cue 存在期间 ···                                             ║
║                                                                   ║
║  GE 移除                                                          ║
║       │                                                           ║
║       ▼                                                           ║
║  ASC::RemoveGameplayCue_Internal(Tag)                             ║
║       │                                                           ║
║       └── Container.RemoveCue(Tag)                                ║
║             └── (Replicated → 客户端 OnRep → Removed 事件)        ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝

╔═══════════════════════════════════════════════════════════════════╗
║                     客户端预测路径（如果预测）                     ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  Ability 激活（预测）                                             ║
║       │                                                           ║
║       ▼                                                           ║
║  ASC::AddGameplayCue_Internal + ScopedPredictionKey.IsLocalClient ║
║       │                                                           ║
║       ├── Container.PredictiveAdd(Tag, PredKey)                   ║
║       ├── InvokeGameplayCueEvent(OnActive)   ← 立即本地播放       ║
║       └── InvokeGameplayCueEvent(WhileActive)                     ║
║                                                                   ║
║  ··· 等待服务器确认 ···                                           ║
║                                                                   ║
║  收到 Multicast: IsLocalClientKey()=true → SKIP（不重复播放）     ║
║  收到 OnRep: 数据同步确认                                         ║
║                                                                   ║
║  如果预测失败:                                                     ║
║       PredictionKey.Rejected → PredictiveRemove → Removed 事件    ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
```

---

## 附录：源码文件索引

| 文件 | 路径 |
|------|------|
| GameplayCueManager.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| GameplayCueManager.cpp | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/` |
| GameplayCue_Types.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| GameplayCueSet.h / .cpp | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public(Private)/` |
| GameplayCueNotify_Actor.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| GameplayCueNotify_Static.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| AbilitySystemComponent.h / .cpp | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public(Private)/` |
| AbilitySystemReplicationProxyInterface.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| GameplayCueTranslator.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
| GameplayPrediction.h | `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/` |
