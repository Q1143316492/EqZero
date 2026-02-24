# GameplayCue Notify 类型决策指南

> 基于 UE 5.6 源码分析，系统性总结 GameplayCue Notify 的各种类型、事件回调、决策流程。

---

## 目录

1. [GameplayCue 是什么](#1-gameplaycue-是什么)
2. [触发 Cue 的两种方式：Executed vs Added](#2-触发-cue-的两种方式executed-vs-added)
3. [四个核心事件回调](#3-四个核心事件回调)
4. [六种 Notify 类型总览](#4-六种-notify-类型总览)
5. [继承关系与核心区别：实例化 vs 非实例化](#5-继承关系与核心区别实例化-vs-非实例化)
6. [各类型详解](#6-各类型详解)
7. [BurstLatent 的 OnExecute vs OnBurst 详解](#7-burstlatent-的-onexecute-vs-onburst-详解)
8. [Looping 的四阶段事件详解](#8-looping-的四阶段事件详解)
9. [决策流程图](#9-决策流程图)
10. [实战选型速查表](#10-实战选型速查表)
11. [性能与网络考量](#11-性能与网络考量)
12. [常见陷阱](#12-常见陷阱)

---

## 1. GameplayCue 是什么

GameplayCue 是 GAS 中**专门用于表现层（VFX/SFX/UI）**的机制，核心设计哲学是：

- **逻辑与表现分离**：GameplayAbility/GameplayEffect 负责逻辑，GameplayCue 负责效果表现
- **网络友好**：Cue 只在客户端执行，Dedicated Server 默认不执行表现逻辑
- **Tag 驱动**：通过 `GameplayCue.XXX` 格式的 GameplayTag 来触发对应的 Notify

触发方式示例：
```cpp
// 方式1：直接通过 ASC 触发（一次性）
ASC->ExecuteGameplayCue(Tag, Parameters);

// 方式2：通过 GE 附带触发（持续性，随 GE 生命周期）
// 在 GameplayEffect 的 GameplayCues 数组中配置 Tag

// 方式3：手动 Add/Remove（持续性，手动控制）
ASC->AddGameplayCue(Tag, Parameters);
ASC->RemoveGameplayCue(Tag);
```

---

## 2. 触发 Cue 的两种方式：Executed vs Added

这是理解所有 Cue 类型的基础。GAS 底层只有两种触发模式：

| 模式 | 触发的事件 | 典型来源 | 适用场景 |
|------|-----------|---------|---------|
| **Executed** | `OnExecute` | `ExecuteGameplayCue()`、Instant GE、Periodic GE 的每次 Tick | 一次性效果（爆发、命中） |
| **Added/Removed** | `OnActive` → `WhileActive` → `OnRemove` | `AddGameplayCue()`、Duration/Infinite GE | 持续性效果（Buff、护盾） |

> **关键理解**：一个 Duration GE 会先触发 `OnActive`/`WhileActive`，其 Period Tick 会触发 `OnExecute`，移除时触发 `OnRemove`。这就是为什么 Looping 类型有 4 个阶段回调。

---

## 3. 四个核心事件回调

所有 Cue Notify 的基类都定义了这 4 个虚函数（源自 `UGameplayCueNotify_Static` 和 `AGameplayCueNotify_Actor`）：

| 回调 | Actor 中的显示名 | 触发时机 | 说明 |
|------|----------------|---------|------|
| `OnExecute` | On Execute | `EGameplayCueEvent::Executed` 时 | 即时效果或周期性 Tick |
| `OnActive` | **On Burst** | `EGameplayCueEvent::OnActive` 时 | Cue 首次激活，客户端见证了激活时刻 |
| `WhileActive` | **On Become Relevant** | `EGameplayCueEvent::WhileActive` 时 | Cue 处于激活状态时（包括中途加入） |
| `OnRemove` | **On Cease Relevant** | `EGameplayCueEvent::Removed` 时 | Cue 被移除时 |

> ⚠️ **命名陷阱**：在 `AGameplayCueNotify_Actor` 中，`OnActive` 的显示名是 **"On Burst"**，`WhileActive` 是 **"On Become Relevant"**，`OnRemove` 是 **"On Cease Relevant"**。这与 `UGameplayCueNotify_Burst` 的 `OnBurst` 方法是**完全不同的概念**！

### OnActive vs WhileActive 的区别

```
场景：玩家 A 给自己上了一个 Buff（Duration GE），此时玩家 B 中途加入游戏

玩家 A 视角：
  GE Applied → OnActive ✓ → WhileActive ✓ → ... → GE Removed → OnRemove ✓

玩家 B 视角（中途加入）：
  GE Applied → OnActive ✗（没有见证到激活时刻）→ WhileActive ✓ → ... → GE Removed → OnRemove ✓
```

- **OnActive**：仅在客户端**亲眼见证**激活那一刻才调用（适合播放一次性的 "开始" 特效）
- **WhileActive**：只要 Cue 处于激活状态就会调用，即使是 Join-in-Progress（适合设置持续性的循环特效）

---

## 4. 六种 Notify 类型总览

| 类型 | 基类 | 实例化 | 生命周期 | 推荐用途 |
|------|------|--------|---------|---------|
| **GameplayCueNotify_Static** | `UObject` | ❌ 非实例化 | 无状态，调用即结束 | 一次性轻量效果 |
| **GameplayCueNotify_Burst** | Static | ❌ 非实例化 | 无状态 | 一次性爆发效果（推荐） |
| **GameplayCueNotify_HitImpact** | Static | ❌ 非实例化 | 无状态 | ⛔ **已废弃**，用 Burst 替代 |
| **GameplayCueNotify_Actor** | `AActor` | ✅ 实例化 | 有状态，可持续存在 | 需要完全自定义的持续效果 |
| **GameplayCueNotify_BurstLatent** | Actor | ✅ 实例化 | 短暂存在（默认 5s） | 一次性但需要延迟/时间轴 |
| **GameplayCueNotify_Looping** | Actor | ✅ 实例化 | 随 Cue 生命周期 | 持续循环效果（推荐） |

---

## 5. 继承关系与核心区别：实例化 vs 非实例化

```
UObject
  └── UGameplayCueNotify_Static          ← 非实例化（CDO 上直接调用）
        ├── UGameplayCueNotify_Burst       ← 非实例化
        └── UGameplayCueNotify_HitImpact   ← 非实例化（已废弃）

AActor
  └── AGameplayCueNotify_Actor           ← 实例化（Spawn Actor）
        ├── AGameplayCueNotify_BurstLatent ← 实例化
        └── AGameplayCueNotify_Looping     ← 实例化
```

### 非实例化（Static 系列）

- 不会 Spawn 到世界中，直接在 CDO（Class Default Object）上调用函数
- **不能使用** Timeline、Delay、任何 Latent Action
- 性能最好，零开销
- 适合简单的 "播放一个粒子 + 播放一个声音" 场景

### 实例化（Actor 系列）

- 会 Spawn 一个 Actor 到世界中
- **可以使用** Timeline、Delay、Tick 等 Latent 功能
- 支持对象池（Recycle），通过 `NumPreallocatedInstances` 预分配
- 有状态，可以追踪 Instigator/SourceObject
- 可以 Attach 到目标 Actor（`bAutoAttachToOwner`）

---

## 6. 各类型详解

### 6.1 UGameplayCueNotify_Static

最基础的基类。提供 4 个事件回调（OnExecute/OnActive/WhileActive/OnRemove），你需要自己重写想要处理的事件。

**蓝图中可用的事件：**
- `HandleGameplayCue` — 通用入口，所有事件类型都会调用
- `OnExecute` / `OnActive` / `WhileActive` / `OnRemove` — 分事件处理

**使用场景：** 需要完全自定义的非实例化 Cue（较少直接使用，通常用 Burst 子类）。

### 6.2 UGameplayCueNotify_Burst ⭐

继承自 Static，专为"一次性爆发"效果设计。

**核心流程（源码）：**
```
OnExecute_Implementation() 被调用
  ├── 检查 DefaultSpawnCondition 是否满足
  ├── BurstEffects.ExecuteEffects()  ← 自动 Spawn 配置好的粒子/声音
  └── OnBurst()  ← 调用蓝图可实现事件，传入 SpawnResults
```

**蓝图中的关键接口：**
- **`Event On Burst`** — 最核心的蓝图事件。在这里做 BurstEffects 之外的额外逻辑
- **`BurstEffects`** — 在编辑器属性面板配置粒子/声音列表，自动 Spawn
- **`DefaultSpawnCondition`** — 配置 Spawn 条件
- **`DefaultPlacementInfo`** — 配置位置/旋转/附着规则

**适合场景：** 命中火花、技能释放闪光、爆炸烟雾、音效播放等一次性效果。

### 6.3 UGameplayCueNotify_HitImpact ⛔

**已废弃（Deprecated）**。只是简单地在命中位置 Spawn 一个 ParticleSystem + 播放一个 Sound。请使用 Burst 替代。

### 6.4 AGameplayCueNotify_Actor

实例化 Cue 的基类。功能最完整但也最自由，你需要自己管理一切。

**相比 Static 多出的能力：**
- Timeline / Delay / Tick
- 生命周期管理：`bAutoDestroyOnRemove`、`AutoDestroyDelay`
- 实例控制：`bUniqueInstancePerInstigator`、`bUniqueInstancePerSourceObject`
- 事件重入：`bAllowMultipleOnActiveEvents`、`bAllowMultipleWhileActiveEvents`
- 对象池回收：`Recycle()` / `ReuseAfterRecycle()`
- Attach：`bAutoAttachToOwner`

**使用场景：** 需要完全自定义生命周期的复杂效果（较少直接使用，通常用 BurstLatent 或 Looping 子类）。

### 6.5 AGameplayCueNotify_BurstLatent ⭐

继承自 Actor，为"需要延迟的一次性效果"设计。

**核心流程（源码）：**
```
OnExecute_Implementation() 被调用
  ├── 检查 DefaultSpawnCondition
  ├── BurstEffects.ExecuteEffects()  ← 自动 Spawn 效果
  ├── OnBurst()  ← 蓝图事件，可以在这里做 Timeline/Delay
  └── SetTimer(FinishTimerHandle, Lifetime)  ← 默认 5 秒后自动销毁/回收
        Lifetime = Max(AutoDestroyDelay, 5.0f)
```

**与 Burst 的核心区别：**

| 对比项 | Burst (Static) | BurstLatent (Actor) |
|--------|---------------|-------------------|
| 可用 Delay/Timeline | ❌ 不可以 | ✅ 可以 |
| 世界中的存在 | 不存在于世界 | Spawn 为 Actor |
| 性能 | 更轻量 | 有 Actor 开销 |
| 自动回收 | 无需回收 | 默认 5s 后回收 |
| 对象池 | 无 | 支持（默认预分配 3 个） |

**适合场景：** 延迟爆炸、分阶段展开的特效（先闪光→延迟→爆炸）、需要 Timeline 控制的效果。

### 6.6 AGameplayCueNotify_Looping ⭐

继承自 Actor，为"持续循环效果"设计，生命周期完全由 Cue 的 Add/Remove 驱动。

**四阶段效果系统：**

| 阶段 | 属性面板配置 | 蓝图事件 | 触发时机 |
|------|------------|---------|---------|
| Application | `ApplicationEffects` | `OnApplication` | OnActive（首次激活时） |
| Looping | `LoopingEffects` | `OnLoopingStart` | WhileActive（当激活状态可见时） |
| Recurring | `RecurringEffects` | `OnRecurring` | OnExecute（周期性 tick） |
| Removal | `RemovalEffects` | `OnRemoval` | OnRemove（Cue 移除时） |

**典型生命周期：**
```
GE Applied (Duration/Infinite)
  ├── OnActive → ApplicationEffects Spawn → OnApplication 蓝图
  ├── WhileActive → LoopingEffects Start → OnLoopingStart 蓝图
  │     （循环粒子开始播放）
  ├── OnExecute (Period Tick) → RecurringEffects Spawn → OnRecurring 蓝图
  ├── OnExecute (Period Tick) → RecurringEffects Spawn → OnRecurring 蓝图
  │     ...
  └── OnRemove → RemoveLoopingEffects() → RemovalEffects Spawn → OnRemoval 蓝图
        （循环粒子停止，播放结束特效）
```

**适合场景：** Buff 光环、持续燃烧、护盾环绕、环境特效（风暴/光环）、DOT 效果（每次 Tick 有额外表现）。

---

## 7. BurstLatent 的 OnExecute vs OnBurst 详解

这是最常见的困惑点。以下是精确的调用链：

```cpp
// 引擎源码 AGameplayCueNotify_BurstLatent::OnExecute_Implementation
bool AGameplayCueNotify_BurstLatent::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
    // 1. 构建 SpawnContext
    FGameplayCueNotify_SpawnContext SpawnContext(World, Target, Parameters);
    
    // 2. 检查是否应该 Spawn
    if (DefaultSpawnCondition.ShouldSpawn(SpawnContext))
    {
        // 3. 自动执行 BurstEffects 配置的效果
        BurstEffects.ExecuteEffects(SpawnContext, BurstSpawnResults);
        
        // 4. 调用蓝图事件 OnBurst
        OnBurst(Target, Parameters, BurstSpawnResults);
    }
    
    // 5. 设置自动销毁定时器（默认 5 秒）
    const float Lifetime = FMath::Max<float>(AutoDestroyDelay, 5.0f);
    World->GetTimerManager().SetTimer(..., Lifetime);
    
    return false;
}
```

### 决策要点

| 你想做的事情 | 在哪里做 |
|-------------|---------|
| 配置要 Spawn 的粒子/声音 | 属性面板的 `BurstEffects` |
| 控制 Spawn 条件（LocalPlayer? 距离？） | 属性面板的 `DefaultSpawnCondition` |
| 控制 Spawn 位置/旋转/附着 | 属性面板的 `DefaultPlacementInfo` |
| 在 BurstEffects 之后做额外逻辑（Timeline/Delay） | 蓝图的 `Event OnBurst` |
| 需要访问已 Spawn 的效果引用 | `OnBurst` 的 `SpawnResults` 参数 |
| 想完全自定义触发逻辑，不用 BurstEffects | 重写 `OnExecute` |

### 总结

- **`OnExecute`** = 引擎回调入口。默认实现会执行 BurstEffects 并调用 OnBurst。通常**不需要重写**。
- **`OnBurst`** = 业务层蓝图事件。在所有配置好的效果 Spawn 之后调用，你在这里写**额外的自定义逻辑**。

如果你只需要 Spawn 粒子和声音，只配 `BurstEffects` 属性即可，甚至不需要实现 `OnBurst`。

---

## 8. Looping 的四阶段事件详解

Looping 类型最容易让人困惑的是有 4 个蓝图事件入口。以下是完整决策：

| 蓝图事件 | 对应引擎 Override | 你应该在这里做什么 |
|----------|-----------------|-----------------|
| `OnApplication` | `OnActive` | 播放初始的一次性"开始"效果（如：护盾激活的闪光） |
| `OnLoopingStart` | `WhileActive` | 开始持续性循环效果（如：护盾环绕的粒子循环） |
| `OnRecurring` | `OnExecute` | 周期性触发的效果（如：DOT 每次 Tick 的伤害数字飘字） |
| `OnRemoval` | `OnRemove` | 结束效果（如：护盾破碎的爆裂特效） |

**关键区别**：`OnApplication` 只有亲眼见证激活的客户端才会触发，`OnLoopingStart` 对所有可见的客户端都会触发（包括中途加入）。所以：
- 循环粒子放在 `LoopingEffects`（由 `OnLoopingStart` 启动）
- 一次性的 "开始" 特效放在 `ApplicationEffects`（由 `OnApplication` 启动）

---

## 9. 决策流程图

```
需要 GameplayCue
  │
  ├── 一次性效果（Executed）？
  │     │
  │     ├── 需要 Delay / Timeline？
  │     │     ├── YES → BurstLatent
  │     │     └── NO → Burst ✅（首选）
  │     │
  │     └── 需要完全自定义？ → 重写 Static 的 OnExecute
  │
  └── 持续性效果（Added/Removed）？
        │
        ├── 有循环粒子/声音需要持续播放？
        │     └── YES → Looping ✅（首选）
        │
        ├── 需要 Tick / 完全自定义生命周期？
        │     └── YES → Actor（手动管理）
        │
        └── 只在开始/结束各播一次效果？
              └── Looping（只用 Application + Removal 阶段）
```

**简化版决策：**
1. **一次性，简单** → `Burst`
2. **一次性，要 Delay/Timeline** → `BurstLatent`
3. **持续性** → `Looping`
4. **以上都不够用** → `Actor` 手撸

---

## 10. 实战选型速查表

| 效果 | 推荐类型 | 理由 |
|------|---------|------|
| 子弹命中火花 | Burst | 简单一次性，不需要延迟 |
| 子弹命中血液效果 | Burst | 同上 |
| 技能释放闪光 | Burst | 瞬间播完 |
| 爆炸（先闪光→延迟→爆炸粒子） | BurstLatent | 需要 Delay 分阶段 |
| 手雷延迟爆炸的展开效果 | BurstLatent | 需要 Timeline 控制扩散 |
| 持续燃烧 Debuff | Looping | 有循环火焰粒子 |
| DOT 毒素（每秒掉血+飘字） | Looping | 循环 + 周期性 Recurring |
| 护盾环绕 Buff | Looping | 有循环粒子，Add/Remove 生命周期 |
| 加速 Buff 脚下光环 | Looping | Attached 循环效果 |
| 风暴/天气环境效果 | Looping | 持续循环 |
| 火焰喷射器持续喷射 | Actor（手动） | 需要 Tick 精确控制方向 |
| 光束连接两个 Actor | Actor（手动） | 需要 `bUniqueInstancePerInstigator` + Tick 更新位置 |

---

## 11. 性能与网络考量

### 性能排序（从低到高开销）

1. **Burst** — 零实例化开销，CDO 直接调用
2. **Static** — 同上
3. **BurstLatent** — 有 Actor Spawn，但支持对象池（默认预分配 3 个）
4. **Looping** — 同上
5. **Actor** — 完全手动管理

### 对象池

Actor 系列（BurstLatent、Looping、Actor）都支持对象池：
- `NumPreallocatedInstances`：预分配实例数量（默认 3）
- 用完后 `Recycle()` → 放回池中 → 下次 `ReuseAfterRecycle()` 复用
- **BurstLatent** 默认 5 秒后自动回收（`DefaultBurstLatentLifetime = 5.0f`）
- **Looping** 在 OnRemove 时设置 `bAutoDestroyOnRemove = true`

### 网络

- GameplayCue **永远不要在 Dedicated Server 上执行表现逻辑**
- 通过 GE 触发的 Cue 会自动处理网络复制（Replicated GameplayCueTags）
- 通过 `ExecuteGameplayCue()` 触发的 Cue 走 RPC
- `AddGameplayCue()` / `RemoveGameplayCue()` 走 Replicated Tag
- 客户端预测：配合 PredictionKey 避免重复播放

---

## 12. 常见陷阱

### 1. 在 Burst 中使用 Delay
```
❌ Burst 是非实例化的，不能使用 Delay/Timeline
✅ 改用 BurstLatent
```

### 2. 混淆 Actor 基类的 "On Burst" 与 Burst 类的 "OnBurst"
```
AGameplayCueNotify_Actor 的 "On Burst" = OnActive（蓝图显示名）
UGameplayCueNotify_Burst 的 "OnBurst" = BurstEffects 执行后的蓝图回调

两者完全不同！
```

### 3. Looping 循环效果放错阶段
```
❌ 把循环粒子放在 ApplicationEffects → 只播放一次，不会循环
✅ 循环粒子放在 LoopingEffects，一次性的开始特效放在 ApplicationEffects
```

### 4. BurstLatent 忘记设置 AutoDestroyDelay
```
默认 5 秒后销毁。如果你的 Timeline 超过 5 秒：
✅ 设置 AutoDestroyDelay 为你实际需要的时长
   实际 Lifetime = Max(AutoDestroyDelay, 5.0f)
```

### 5. 中途加入不显示效果
```
WhileActive/OnBecomeRelevant 是为中途加入设计的。
如果你只用了 OnActive/OnBurst 来启动循环效果：
❌ 中途加入的玩家看不到效果
✅ 循环效果应在 WhileActive 中启动（Looping 已自动处理）
```

### 6. 多个 Source 覆盖实例
```
默认情况下同一个 Cue 对同一个 Target 只有一个实例。
如果需要每个 Instigator 都有独立的效果（比如光束）：
✅ 启用 bUniqueInstancePerInstigator
```
