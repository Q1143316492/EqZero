# GAS 体系深度解析 — 从设计原理到工程落地

> 本文基于 Unreal Engine 5.6 Lyra 架构复刻项目（EqZero），系统性剖析 Gameplay Ability System 的设计哲学、核心机制与工程实践。
> 作为 GAS 落地的设计指南。

---

## 一、30 秒介绍 GAS

如果时需要一段话概括 GAS：

> GAS 是 UE 内置的一套**数据驱动的技能框架**。它的核心思路是将游戏中的"能力"（Ability）、"效果"（Effect）、"属性"（Attribute）三者解耦：Ability 描述"发生了什么"，Effect 描述"改变了什么"，Attribute 描述"状态是什么"。再通过 GameplayTag 作为全局标签系统将它们串联起来，实现技能激活的条件门控、效果的标签查询和表现层的事件路由。GAS 原生支持网络预测，客户端可以在服务器确认前先行执行效果，确认后自动去重/回滚。整套系统加上 GameplayCue（表现层解耦）和 AbilityTask（异步任务），构成了一个可以覆盖从 MOBA 到 Looter Shooter 的通用框架。

---

## 二、五大核心组件关系图

```
┌──────────────────────────────────────────────────┐
│            AbilitySystemComponent (ASC)           │
│  ─ 技能系统的运行时核心，挂在 Actor 上             │
│  ─ 管理 GA 的授予/激活/结束                       │
│  ─ 管理 GE 的创建/应用/移除                       │
│  ─ 持有 AttributeSet                              │
│  ─ 维护 GameplayTag 容器                          │
│  ─ 网络预测的中枢 (PredictionKey / TargetData)    │
├──────────────────────────────────────────────────┤
│                                                    │
│  ┌─────────────┐  应用 GE   ┌──────────────────┐ │
│  │ GameplayAbility │ ──────► │ GameplayEffect   │ │
│  │ (GA)         │           │ (GE)              │ │
│  │ ─ 技能逻辑  │           │ ─ Modifier/Exec  │ │
│  │ ─ AbilityTask│           │ ─ Duration/Tag   │ │
│  └──────┬──────┘           └────────┬─────────┘ │
│         │ 播放                      │ 修改        │
│         ▼                          ▼            │
│  ┌─────────────┐          ┌──────────────────┐  │
│  │ GameplayCue │          │ AttributeSet     │  │
│  │ ─ 纯表现层  │          │ (AS)             │  │
│  │ ─ 特效/音效 │          │ ─ Health/Mana    │  │
│  └─────────────┘          │ ─ 支持预测       │  │
│                            └──────────────────┘  │
│                                                    │
│  ┌────────────────────────────────────────────┐   │
│  │          GameplayTag — 贯穿一切的标签系统    │   │
│  │  ─ 状态标记 (Status.Death)                  │   │
│  │  ─ 条件门控 (ActivationRequired/Blocked)     │   │
│  │  ─ 事件路由 (GameplayEvent.Death)            │   │
│  │  ─ 输入绑定 (InputTag.Ability.Fire)          │   │
│  └────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
```

**一句话理清关系**：ASC 是容器，GA 是行为，GE 是数据变更，AS 是被变更的属性，Tag 是黏合剂，Cue 是表现。

---

## 二（补）、ASC 内部数据结构全景 — 它到底持有什么

ASC 是 GAS 的运行时中枢。理解它的全部成员变量，就等于理解 GAS 在运行时管理了哪些状态、如何复制到客户端。

### ⓪ 总览：5 个 FastArray + 2 个自定义 NetSerialize + 1 个 OnRep 结构

ASC 的核心数据全部围绕网络复制设计。下面是所有重要的**复制容器**一览：

| 属性名 | 类型 | 复制方式 | 复制条件 | 管理什么 |
|--------|------|---------|---------|---------|
| `ActiveGameplayEffects` | `FActiveGameplayEffectsContainer` | **FFastArraySerializer** | `COND_Dynamic` / `COND_None` | 所有活跃 GE |
| `ActivatableAbilities` | `FGameplayAbilitySpecContainer` | **FFastArraySerializer** | `COND_ReplayOrOwner` | 所有已授予的 GA |
| `ActiveGameplayCues` | `FActiveGameplayCueContainer` | **FFastArraySerializer** | `COND_None` | 独立 Cue（非 GE 附带的） |
| `MinimalReplicationGameplayCues` | `FActiveGameplayCueContainer` | **FFastArraySerializer** | `COND_SkipOwner` | Minimal 模式下 GE 附带的 Cue |
| `ReplicatedPredictionKeyMap` | `FReplicatedPredictionKeyMap` | **FFastArraySerializer** | `COND_OwnerOnly` | 服务器确认的预测 Key |
| `MinimalReplicationTags` | `FMinimalReplicationTagCountMap` | **自定义 NetSerialize** | `COND_SkipOwner` | Minimal 模式下 GE 赋予的 Tag |
| `ReplicatedLooseTags` | `FMinimalReplicationTagCountMap` | **自定义 NetSerialize** | `COND_None` | 手动添加的复制 Loose Tag |
| `RepAnimMontageInfo` | `FGameplayAbilityRepAnimMontage` | **OnRep + 自定义 NetSerialize** | `COND_None` | Montage 同步状态 |

> **FastArray vs 自定义 NetSerialize**：FastArray（`FFastArraySerializer`）基于增量复制——只发送变化的条目，有 `PreReplicatedRemove/PostReplicatedAdd/PostReplicatedChange` 回调。自定义 NetSerialize 则手动序列化整个 Map。两者都避免了全量复制的开销。

---

### ① ActiveGameplayEffects — 效果容器（最复杂的核心）

**类型**：`FActiveGameplayEffectsContainer : FFastArraySerializer`

存储所有正在生效的 GameplayEffect 实例。每个条目是 `FActiveGameplayEffect`：

```
FActiveGameplayEffect (FFastArraySerializerItem)
 ├─ FGameplayEffectSpec Spec          // GE 完整规格（Modifier、Level、Source/Target Tags…）
 ├─ FPredictionKey PredictionKey      // 客户端预测 Key（用于 Undo/Redo 匹配）
 ├─ FActiveGameplayEffectHandle Handle // 唯一句柄（本地生成，不复制）
 ├─ float StartServerWorldTime        // 服务器端起始时间（Duration GE 的倒计时基准）
 ├─ TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles // 此 GE 授予的技能句柄
 └─ bool bIsInhibited                 // 是否被抑制（不复制，本地计算）
```

**容器内部的关键子结构**：

| 成员 | 类型 | 作用 |
|------|------|------|
| `GameplayEffects_Internal` | `TArray<FActiveGameplayEffect>` | 实际的 FastArray 条目数组 |
| `AttributeAggregatorMap` | `TMap<FGameplayAttribute, FAggregatorRef>` | 每个属性的聚合器——汇总所有 Modifier 计算最终值 |
| `AttributeValueChangeDelegates` | `TMap<FGameplayAttribute, FOnGameplayAttributeValueChange>` | 属性变化回调 |
| `SourceStackingMap` | `TMap<TWeakObjectPtr<UGameplayEffect>, ...>` | GE 堆叠追踪 |
| `PendingGameplayEffectHead` | 链表头指针 | 正在 ScopeLock 中排队的待添加 GE |

**为什么是最复杂的**：这个容器不只是"存 GE"，它还：
1. 持有 `AttributeAggregatorMap`——属性的最终值就是从这里算出来的
2. 管理 GE 的堆叠（Stacking）策略
3. 处理 Duration/Periodic 的 Tick（时间到了自动移除/执行）
4. 在 `PostReplicatedAdd` 回调中触发 GameplayCue、属性变化通知
5. 预测版 GE 和服务器版 GE 的匹配/替换也在这里发生

---

### ② ActivatableAbilities — 技能容器

**类型**：`FGameplayAbilitySpecContainer : FFastArraySerializer`

存储所有已授予（Granted）的技能规格。每个条目是 `FGameplayAbilitySpec`：

```
FGameplayAbilitySpec (FFastArraySerializerItem)
 ├─ FGameplayAbilitySpecHandle Handle       // 唯一句柄（复制）
 ├─ UGameplayAbility* Ability               // CDO 指针（复制）
 ├─ int32 Level                             // 技能等级（复制）
 ├─ int32 InputID                           // 输入绑定 ID（复制，Lyra 不用）
 ├─ TWeakObjectPtr<UObject> SourceObject    // 授予来源，如 EquipmentInstance（复制）
 ├─ FGameplayTagContainer DynamicAbilityTags // 动态标签，如 InputTag（复制）
 ├─ TArray<UGameplayAbility*> ReplicatedInstances   // 实例化的 GA 对象（复制）
 ├─ TArray<UGameplayAbility*> NonReplicatedInstances // 本地实例（不复制）
 ├─ uint8 ActiveCount                       // 当前激活计数（不复制）
 ├─ uint8 InputPressed:1                    // 输入状态（不复制）
 └─ FActiveGameplayEffectHandle GameplayEffectHandle // 授予此技能的 GE（不复制，服务器用）
```

**设计要点**：
- `COND_ReplayOrOwner`：只复制给 Owner 和 Replay。旁观者看不到你有什么技能。
- `SourceObject` 就是 Lyra 用来做 GA→Equipment 反查的关键字段。
- `DynamicAbilityTags` = Lyra 用来匹配 InputTag 的载体。
- `ReplicatedInstances` vs `NonReplicatedInstances`：取决于 GA 的 `ReplicationPolicy`。

---

### ③ 标签系统 — 三层标签管理

ASC 的 Tag 状态由多个源头汇聚：

```
GameplayTagCountContainer (本地权威)
 ├─ 来源 1: ActiveGameplayEffects 中 GE 赋予的 Tag
 ├─ 来源 2: ReplicatedLooseTags (手动 AddReplicatedLooseGameplayTag)
 ├─ 来源 3: MinimalReplicationTags (Minimal 模式下 GE 的 Tag)
 └─ 来源 4: 本地 Loose Tag (AddLooseGameplayTag, 不复制)
```

| 属性 | 复制 | 用途 |
|------|------|------|
| `GameplayTagCountContainer` | 不复制 | 本地聚合的完整 Tag 表，所有 `HasMatchingGameplayTag` 查询都走这里 |
| `ReplicatedLooseTags` | `COND_None` | 代码手动添加的需要复制的 Tag（如状态标记） |
| `MinimalReplicationTags` | `COND_SkipOwner` | Minimal 模式专用——GE 不复制，但 Tag 要复制给非 Owner |
| `BlockedAbilityTags` | 不复制 | `ApplyAbilityBlockAndCancelTags` 写入的阻塞标签 |

**Minimal Replication 的逻辑**：
- Full 模式：GE 本身复制 → 客户端从 GE 推导出 Tag → 不需要单独复制 Tag
- Minimal 模式：GE 不复制给非 Owner → 非 Owner 客户端看不到 GE → 所以要把 Tag 单独复制出来

---

### ④ GameplayCue — 表现层容器

两个 `FActiveGameplayCueContainer`（都是 FastArray）：

| 容器 | 复制条件 | 用途 |
|------|---------|------|
| `ActiveGameplayCues` | `COND_None` | 手动调用 `AddGameplayCue` 的独立 Cue |
| `MinimalReplicationGameplayCues` | `COND_SkipOwner` | Minimal 模式下 GE 附带的 Cue |

每个 `FActiveGameplayCue` 条目：
- `GameplayCueTag`：Cue 标签
- `PredictionKey`：用于去重
- `Parameters`：Cue 参数（Location、HitResult、Instigator 等）

> **为什么 Cue 有独立容器**：GE 上挂的 Cue 和手动调的 Cue 走不同流程。GE 的 Cue 随 GE 生命周期自动管理（Added/Removed），手动的 Cue 需要独立跟踪。

---

### ⑤ RepAnimMontageInfo — Montage 同步

```
FGameplayAbilityRepAnimMontage (自定义 NetSerialize)
 ├─ UAnimSequenceBase* Animation      // 动画资产
 ├─ FName SlotName                     // 动态 Montage 插槽
 ├─ float PlayRate, BlendTime          // 播放参数
 ├─ uint8 NextSectionID               // 当前 Section 跳转目标
 ├─ uint8 PlayInstanceId              // 播放实例 ID（区分同一 Montage 的多次播放）
 ├─ uint8 IsStopped:1                 // 是否已停止
 ├─ uint8 bRepPosition:1              // 是否复制精确位置（vs SectionId）
 ├─ FPredictionKey PredictionKey      // 预测去重
 └─ uint8 SectionIdToPlay             // Section-based 同步时使用
```

配套的本地状态：

```
FGameplayAbilityLocalAnimMontage (不复制)
 ├─ UAnimMontage* AnimMontage         // 本地播放的 Montage
 ├─ uint8 PlayInstanceId
 ├─ FPredictionKey PredictionKey
 └─ TWeakObjectPtr<UGameplayAbility> AnimatingAbility  // 哪个 GA 在播
```

**同步流程**：服务器的 `LocalAnimMontageInfo` 变化 → 写入 `RepAnimMontageInfo` → 复制到客户端 → `OnRep_ReplicatedAnimMontage` → SimulatedProxy 播放 Montage。AutonomousProxy 已通过预测在播了，`PlayInstanceId` 匹配则跳过。

---

### ⑥ ReplicatedPredictionKeyMap — 预测确认

**类型**：`FReplicatedPredictionKeyMap : FFastArraySerializer`

服务器消费完预测操作后，将确认的 `PredictionKey` 写入此 FastArray，复制给 Owner 客户端（`COND_OwnerOnly`）。客户端收到后：
1. 匹配本地的待确认预测副作用
2. 已确认的 GE/Cue/Montage 执行 Redo（去重）
3. 被拒绝的 Key（服务器未确认且超时）执行 Undo（回滚）

> **必须是最后一个复制属性**：引擎要求此属性在 `GetLifetimeReplicatedProps` 中排在最后，确保 OnRep 触发时其他复制数据（GE、Abilities）已经到位。

---

### ⑦ Actor 引用与 AbilityActorInfo

| 属性 | 复制 | 作用 |
|------|------|------|
| `OwnerActor` | 复制 (OnRep) | 逻辑拥有者（PlayerState） |
| `AvatarActor` | 复制 (OnRep) | 物理表示（Pawn） |
| `AbilityActorInfo` | 不复制 | 缓存所有 Actor 引用的结构体 |

`FGameplayAbilityActorInfo` 缓存了 GA 运行时需要的一切引用：
```
FGameplayAbilityActorInfo
 ├─ OwnerActor, AvatarActor
 ├─ PlayerController
 ├─ AbilitySystemComponent
 ├─ SkeletalMeshComponent
 ├─ AnimInstance
 └─ MovementComponent
```

每次 `InitAbilityActorInfo` 调用时重新填充。本地缓存，GA 内通过 `GetAbilitySystemComponentFromActorInfo()` 等快速访问。

---

### ⑧ ReplicationMode — 三种复制策略

```cpp
enum class EGameplayEffectReplicationMode : uint8
{
    Full,    // GE 完整复制给所有连接（默认）
    Mixed,   // GE 只复制给 Owner，非 Owner 走 MinimalReplication
    Minimal  // GE 不复制，全走 MinimalReplication（Tag+Cue 单独复制）
};
```

| 模式 | GE 复制给 Owner | GE 复制给非 Owner | Tag 复制 | Cue 复制 | 适用场景 |
|------|----------------|------------------|---------|---------|---------|
| Full | ✅ | ✅ | 随 GE | 随 GE | 小规模（< 10 个 ASC） |
| Mixed | ✅ | ❌ | Minimal | Minimal | 中规模（玩家用 Full，AI 用 Minimal） |
| Minimal | ❌ | ❌ | Minimal | Minimal | 大规模（MOBA 小兵等） |

**Lyra 实践**：玩家角色用 Mixed——自己能看到完整 GE 信息（UI 显示 buff 倒计时），别人只看到 Tag 和 Cue 就够了。

---

### ⑨ 其他重要属性

| 属性 | 类型 | 复制 | 用途 |
|------|------|------|------|
| `SpawnedAttributes` | `TArray<UAttributeSet*>` | `OnRep` | 所有 AttributeSet 子对象 |
| `BlockedAbilityBindings` | `TArray<uint8>` | `COND_OwnerOnly` | 按 InputID 的阻塞计数器 |
| `bSuppressGrantAbility` | `bool` | 不复制 | 抑制 GE 授予技能 |
| `bSuppressGameplayCues` | `bool` | 不复制 | 抑制所有 GameplayCue |
| `UserAbilityActivationInhibited` | `bool` | 不复制 | UI/输入层面的激活抑制 |
| `AbilityScopeLockCount` | `int32` | 不复制 | ScopeLock 嵌套计数（保护遍历安全） |
| `ScopedPredictionKey` | `FPredictionKey` | 不复制 | 当前预测窗口的 Key |

---

### ⑩ 设计启示

**一个 ASC 其实是 5 个 FastArray + 2 个自定义序列化结构的组合体**，分别管理 GE、GA、Cue、PredictionKey、Tag。这种设计的核心思路是：

1. **增量复制**：FastArray 只发变化的条目，不像 TArray 的 `UPROPERTY(Replicated)` 每次全量
2. **按受众分发**：`COND_OwnerOnly`（PredictionKey）、`COND_SkipOwner`（MinimalReplication）、`COND_None`（全员）——不同数据给不同的客户端
3. **预测友好**：几乎所有 FastArray 条目都带 `PredictionKey` 字段，支持确认/回滚匹配
4. **本地加速**：`GameplayTagCountContainer`、`AbilityActorInfo` 等不复制的本地缓存，避免每次查询都遍历复制容器

---

## 三、GAS 的三层设计哲学

### 第一层：逻辑与表现解耦

GAS 把"属性变了"（GE 修改属性）和"看到什么"（GameplayCue）分开。

- GE 负责改数值：`Health -= 50`
- Cue 负责"看到"：`GC_Impact` 播放血雾粒子

好处：GE 可以在服务器独立运行（Dedicated Server 无渲染上下文），Cue 只在有渲染的客户端运行。优化 DS 性能，也能让策划独立调整表现而不碰逻辑。

### 第二层：数据驱动而非硬编码

传统做法：代码里写 `if (HasEnoughMana) { Mana -= 10; StartCooldown(5.0f); }`

GAS 做法：
- Cost GE 定义消耗量（数据资产，策划可调）
- Cooldown GE 定义冷却时间（数据资产，策划可调）
- `CommitAbility()` 一行代码同时检查并应用两者

这意味着策划可以在编辑器里创建新技能并配置消耗/冷却，不需要程序员写 C++。

### 第三层：网络预测内建

一个技能只需写一套逻辑（"做什么"），GAS 自动处理：
- 客户端先行执行（预测）
- 服务器确认后去重（Redo）
- 服务器拒绝后回滚（Undo）

无需在技能里写 `if (HasAuthority()) ... else ...`。

---

## 四、Lyra 对 GAS 的架构扩展 — 我的项目所做的事

原版 GAS 是一个灵活但"裸"的框架。Lyra（以及我的复刻项目）在其上建立了一整套工程化架构：

### 4.1 ASC 放在 PlayerState 上

```
OwnerActor  = PlayerState  (生命周期 = 整局游戏)
AvatarActor = Pawn         (生命周期 = 单次生命)
```

**为什么**：射击游戏有死亡-重生循环。Pawn 死亡后销毁，如果 ASC 在 Pawn 上，所有 buff、冷却、弹药状态全部丢失。放在 PlayerState 上，状态跨越重生保留。

**代价**：需要在 Pawn 切换时手动调用 `InitAbilityActorInfo` 重新关联 AvatarActor，包括 AnimInstance 重绑、已有 GA 的 `OnPawnAvatarSet` 通知等。

### 4.2 数据驱动的三级技能授予

```
PawnData (角色基础)       → AbilitySet → GA: 跳跃/蹲伏/死亡
EquipmentDefinition (装备) → AbilitySet → GA: 开火/换弹/ADS
GameFeatureAction (模式)   → AbilitySet → GA: 模式特有能力
```

`AbilitySet` 是一个 DataAsset，打包了 GA + GE + AttributeSet。三个维度独立授予/收回，通过 `GrantedHandles` 管理句柄，卸装时一键收回。

**设计优势**：换武器 = 收回旧 AbilitySet + 授予新 AbilitySet，不需要任何硬编码逻辑。

### 4.3 Tag-Based Input Binding

```
Enhanced Input Action       →[InputConfig]→  InputTag
InputTag                    →[AbilitySet]→   GA.DynamicSpecSourceTags
```

抛弃了 GAS 原生的 `InputID`（int32，无语义）。用 Tag 做输入绑定：
- 灵活：同一个 InputTag 可以在不同装备下绑到不同 GA
- 语义化：`InputTag.Ability.Fire` 比 `InputID=3` 有意义
- 支持 GameFeature 模块化注入

### 4.4 Tag Relationship Mapping — 集中式技能关系管理

原生 GAS：每个 GA 上独立配 Block/Cancel Tags → 新增技能要改 N 个已有 GA。

Lyra 方案：一张 DataAsset 表集中管理所有规则：

```
AbilityTag=Ability.Type.Action.Dash
  → Block: Ability.Type.Action.Melee    (冲刺时阻止近战)
  → Cancel: Ability.Type.Action.Reload  (冲刺取消换弹)
```

新增技能只需在表里加一行，不碰已有 GA 代码。ASC 在 `ApplyAbilityBlockAndCancelTags` 和 `DoesAbilitySatisfyTagRequirements` 中注入这些规则。

### 4.5 Experience 系统 — 数据驱动的游戏模式

```
ExperienceDefinition
 ├─ DefaultPawnData (角色配置)
 ├─ GameFeaturesToEnable[] (启用的功能模块)
 ├─ Actions[] (GameFeatureAction: 注入能力/输入/UI)
 └─ ActionSets[] (可复用的 Action 集合)
```

切换游戏模式 = 切换 ExperienceDefinition。不同模式可以有完全不同的角色、技能、输入、UI，全部数据驱动。

### 4.6 InitState 状态机 — 解决网络初始化时序

```
Spawned → DataAvailable → DataInitialized → GameplayReady
```

网络游戏中，Replicated 属性到达时机不确定。PawnExtensionComponent 作为协调者，确保各组件（HeroComponent、ASC、Camera）只在前置数据就绪后才初始化。避免了"ASC 试图读 PawnData 但 PawnData 还没复制到"这类竞态条件。

---

## 五、属性系统设计 — 双 AttributeSet 与 Meta Attribute

### 5.1 为什么分成 HealthSet 和 CombatSet

| Set | 归属 | 属性 |
|-----|------|------|
| HealthSet | 受击方 | Health, MaxHealth, Healing（Meta）, Damage（Meta） |
| CombatSet | 攻击方 | BaseDamage, BaseHeal |

分离的核心原因：**伤害计算需要从 Source 捕获属性，输出到 Target 属性**。DamageExecution 从攻击方的 `CombatSet.BaseDamage` 读值，经过衰减计算，写入被击方的 `HealthSet.Damage`。

如果混在一个 Set 里，AttributeCapture 的 Source/Target 语义会混乱。

### 5.2 Meta Attribute 模式

`Damage` 和 `Healing` 不代表持久状态——它们是**一次性的中转值**：

```
Execution 写入 Damage = 50
    ↓
PostGameplayEffectExecute:
    Health = Clamp(Health - Damage, 0, MaxHealth)
    Damage = 0  // 用完即清
```

**好处**：
1. `PreGameplayEffectExecute` 可以检查/修改 Damage 值（实现最终免伤、护盾抵扣等）
2. Damage 的值可以被 GE 的 Modifier 额外修改（如全局伤害系数 buff）
3. 伤害来源信息（HitResult、WeaponInstance）通过 EffectContext 传递，在 Post 回调中完整可用

### 5.3 属性复制与预测的协作

```cpp
// 必须 REPNOTIFY_Always — 即使值没变也触发 OnRep
DOREPLIFETIME_CONDITION_NOTIFY(UEqZeroHealthSet, Health, COND_None, REPNOTIFY_Always);

void UEqZeroHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    // 关键宏：用服务器推来的值作为 BaseValue，重新聚合 FinalValue
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEqZeroHealthSet, Health, OldValue);
}
```

这确保了当客户端有预测性 Modifier 存在时，服务器复制的值能正确作为 BaseValue 参与聚合，而非覆盖 FinalValue。

---

## 六、伤害计算管线 — 从扣扳机到扣血

完整链路：

```
① 玩家按下开火 → GA_RangedWeapon.ActivateAbility
② 蓝图调 StartRangedWeaponTargeting()
    ├─ 打开 FScopedPredictionWindow (预测窗口)
    ├─ PerformLocalTargeting: 摄像机→焦点的射线检测
    │   ├─ 计算散布(热度→角度→正态分布随机偏移)
    │   ├─ 线检测 + 容错球检测 (SweepRadius)
    │   └─ 去重(同一个Actor只命中一次)
    └─ 构建 FGameplayAbilityTargetDataHandle
③ OnTargetDataReadyCallback
    ├─ 客户端 → CallServerSetReplicatedTargetData (RPC)
    ├─ 服务器 → 验证 + ClientConfirmTargetData (Client RPC)
    ├─ CommitAbility (预测性扣弹药/CD)
    └─ OnRangedWeaponTargetDataReady (蓝图)
④ 蓝图中应用伤害 GE (配有 DamageExecution)
    ├─ EffectContext: HitResult + CartridgeID + AbilitySource(武器实例)
    └─ ApplyGameplayEffectSpecToTarget
⑤ DamageExecution.Execute (仅服务器 WITH_SERVER_CODE)
    ├─ 捕获 Source.CombatSet.BaseDamage
    ├─ × 距离衰减 (武器曲线)
    ├─ × 物理材质衰减 (爆头/肢体)
    └─ → 输出 Target.HealthSet.Damage
⑥ HealthSet.PostGameplayEffectExecute
    ├─ Health -= Damage
    ├─ Damage = 0
    └─ 广播 OnHealthChanged
⑦ HealthComponent 监听
    ├─ Health ≤ 0 → 发送 GameplayEvent.Death
    └─ → GA_Death 被触发激活
```

### 自定义 EffectContext 是关键胶水

```cpp
struct FEqZeroGameplayEffectContext : public FGameplayEffectContext
{
    int32 CartridgeID = -1;                        // 散弹枪去重
    TWeakObjectPtr<const UObject> AbilitySourceObject; // 武器引用
};
```

- `CartridgeID`：散弹枪一次射击多颗弹丸，Execution 可用此判断是否同一发
- `AbilitySourceObject`：DamageExecution 通过 `IEqZeroAbilitySourceInterface` 从武器读取衰减曲线

接入方式：`UEqZeroAbilitySystemGlobals::AllocGameplayEffectContext()` 返回自定义类型 + `DefaultGame.ini` 配置。

---

## 七、自定义 AbilityCost — 可插拔的消耗系统

GAS 原生 CostGE 只能扣属性（Mana 等）。项目需要消耗弹药（TagStack 计数器）、道具等非属性资源。

```cpp
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class UEqZeroAbilityCost : public UObject
{
    virtual bool CheckCost(const UEqZeroGameplayAbility* Ability, ...) const;
    virtual void ApplyCost(const UEqZeroGameplayAbility* Ability, ...) const;
    bool bOnlyApplyCostOnHit = false;  // 只有命中才扣
};
```

GA 基类持有 `TArray<UEqZeroAbilityCost*> AdditionalCosts`。`CheckCost/ApplyCost` 中遍历执行。

三种实现：

| 类 | 消耗什么 | 用例 |
|----|---------|------|
| `AbilityCost_ItemTagStack` | 装备上的 TagStack | 射击扣弹药 |
| `AbilityCost_PlayerTagStack` | Controller 上的 TagStack | 消耗角色资源 |
| `AbilityCost_InventoryItem` | 物品栏数量 | 消耗消耗品 |

`bOnlyApplyCostOnHit` 的实现：`ApplyCost` 里仅服务器（`IsNetAuthority`）检查 TargetData 是否命中。客户端不做命中才扣的判断——因为客户端不知道服务器会不会认可命中结果。

---

## 八、装备系统与 GAS 的交汇

### 8.1 管线总览

```
InventoryItemDefinition (配置)
    └─ Fragment: EquippableItem → EquipmentDefinition
                                    ├─ InstanceType: UEqZeroRangedWeaponInstance
                                    ├─ AbilitySetsToGrant[]: Fire, Reload, ADS, Melee
                                    └─ ActorsToSpawn[]: 武器模型

EquipmentManagerComponent (运行时, FFastArraySerializer)
    ├─ EquipItem()
    │   ├─ 创建 EquipmentInstance (武器实例)
    │   ├─ GiveAbilitySet → ASC 获得 GA/GE/AS
    │   └─ SpawnEquipmentActors → 武器模型挂载
    └─ UnequipItem()
        ├─ GrantedHandles.TakeFromAbilitySystem → 一键收回所有
        ├─ DestroyEquipmentActors
        └─ DestroyInstance
```

### 8.2 GA_FromEquipment — 技能如何反查武器

```cpp
// AbilitySet 授予 GA 时，SourceObject 设为 EquipmentInstance
AbilitySpec.SourceObject = EquipmentInstance;

// GA 运行时反查：
UEqZeroEquipmentInstance* GetAssociatedEquipment() const {
    return Cast<UEqZeroEquipmentInstance>(GetCurrentAbilitySpec()->SourceObject);
}
```

射击技能通过此路径拿到 `UEqZeroRangedWeaponInstance`，读取散布参数、衰减曲线、弹药状态等。

### 8.3 武器实例的运行时数据

`UEqZeroRangedWeaponInstance` 维护的状态直接影响射击手感：

| 数据 | 作用 | 更新时机 |
|------|------|---------|
| `CurrentHeat` | 武器热度 | 开火增加，Tick 冷却 |
| `CurrentSpreadAngle` | 当前散布角 | 从热度曲线映射 |
| `CurrentSpreadAngleMultiplier` | 组合散布倍率 | 蹲伏/静止/跳跃/ADS 各倍率相乘 |
| `bHasFirstShotAccuracy` | 首发精准 | 全部倍率和散布都在最小值时为 true |
| `LastFireTime` | 上次开火时间 | 决定散布恢复延迟 |

`WeaponStateComponent`（挂在 Controller 上）负责在 Tick 中驱动 `WeaponInstance.Tick()`，更新上述所有状态。

---

## 九、网络预测 — 原理、实践与陷阱

### 9.1 预测的核心机制

```
客户端激活 GA → 引擎创建 PredictionKey (int16 递增计数器)
    → FScopedPredictionWindow 将 Key 设到 ASC.ScopedPredictionKey
    → 此窗口内所有 GE/Cue/Montage 自动关联此 Key
    → 窗口结束(ActivateAbility返回) → Key 不再有效
```

### 9.2 确认/拒绝的实际行为

| 预测对象 | 确认时（Redo） | 拒绝时（Undo） |
|---------|--------------|---------------|
| **Duration GE** | 服务器版到达 → 跳过 OnApplied/Cue → 删预测版(跳过 OnRemoved) | 删预测版 → 正常执行 OnRemoved |
| **Instant GE 改属性** | 伪造的无限Modifier被删 + 服务器Base到达 → 最终值一致 | 伪造Modifier被删 → 属性回归服务器值 |
| **Cooldown GE** | 同 Duration GE | 删除 → 技能解除冷却(可再次使用) |
| **GameplayCue (Execute)** | Multicast 到达时跳过（不重复播） | 不回滚（已播完，fire-and-forget） |
| **GameplayCue (OnAdded)** | 跳过 OnAdded | OnRemoved 正常执行 |
| **Montage** | Multicast 到达时跳过（已在播） | Task 清理 → StopMontage（动画中断） |
| **GE 赋予的 Tag** | 不重复添加 | 跟随GE移除 |
| **Loose Tag** | 不参与预测 | 不参与预测 |
| **Execution** | — | **不支持预测** |
| **GE 移除** | — | **不支持预测** |

### 9.3 预测窗口的使用场景

**不需要手动开窗口**：`ActivateAbility` 的同步调用栈内，引擎已自动开了预测窗口。所以：
```cpp
void ActivateAbility(...) {
    CommitAbility();       // ✅ 自动在预测窗口内
    PlayMontageAndWait();  // ✅ Task 继承窗口
    ApplyGE();             // ✅ 在窗口内
}
```

**需要手动开窗口**：`ActivateAbility` 返回后（异步回调、Timer、WaitEvent 后）还想做预测操作：
```cpp
void OnSomeAsyncCallback() {
    // ActivateAbility 早就返回了，窗口已关闭
    FScopedPredictionWindow Window(ASC, PredictionKey); // 手动开
    // + 必须有配套的 Server RPC 让服务器也开窗口
    CommitAbility();  // 现在可以预测了
}
```

项目中 `GA_RangedWeapon` 的 `StartRangedWeaponTargeting` 就是这个模式——蓝图在后续帧调用，已不在激活窗口内，所以手动开窗口 + `CallServerSetReplicatedTargetData` 作为配套 RPC。

### 9.4 TargetData 是独立于预测的自定义确认流程

TargetData 和 GAS 预测是**两条独立管线**：
- **GAS 预测**：PredictionKey + 自动 Undo/Redo → 解决 GE/属性/Cue/Montage 的预测
- **TargetData**：客户端算命中 → RPC 发服务器 → 服务器验证 → Client RPC 回确认 → UI 表现

两者的交汇点：
1. TargetData 在预测窗口内发送（共享 PredictionKey）
2. TargetData 到达服务器后触发 `AbilityTargetDataSetDelegate`，在服务器侧执行同样的回调
3. 回调里的 CommitAbility/ApplyGE 等操作，因为在预测窗口内，所以自动参与预测系统

### 9.5 开发中的实践规则

**安全预测**（回滚代价小）：
- 弹药消耗 — CommitAbility 中的 CostGE
- 冷却触发 — CommitAbility 中的 CooldownGE
- 开火动画 — PlayMontageAndWait
- 枪口特效 — Execute 类型 GameplayCue（播了就播了）
- 自身 buff — Duration GE

**不要预测**（回滚代价大或不支持）：
- 他人血量变化 — Execution 不支持预测，且血条回滚很丑
- 击杀判定 — 不可逆的游戏事件
- GE 移除 — 引擎不支持
- 周期性效果 — DoT tick 不支持
- 掉落/拾取 — 状态变更不应由客户端预测

**灰色地带**：
- 自身受击扣血 — 可以预测但回滚时血条波动
- 移速 buff — 预测加速后被否决会有"拉扯感"
- 护盾消耗 — 预测破盾后回滚视觉很奇怪

---

## 十、Global Ability System — WorldSubsystem 管理全场效果

```cpp
class UEqZeroGlobalAbilitySystem : public UWorldSubsystem
{
    void ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect);
    void RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect);
    void RegisterASC(UEqZeroAbilitySystemComponent* ASC);
};
```

- 所有 ASC 在 `InitAbilityActorInfo` 时注册到此 Subsystem
- `ApplyEffectToAll` 遍历所有已注册 ASC 应用 GE
- **新 ASC 注册时自动补上**当前活跃的全局 GE
- 用途：暖场无敌、全场减速、禁用某类技能等

---

## 十一、GameplayTagStack — 弹药系统的基石

基于 `FFastArraySerializer` 的网络增量复制容器：

```cpp
FGameplayTagStackContainer
    AddStack(Tag, Count)     // 增加
    RemoveStack(Tag, Count)  // 减少
    GetStackCount(Tag)       // 查询
```

项目中用于弹药系统：
- `Lyra.Weapon.MagazineAmmo` — 当前弹匣子弹数
- `Lyra.Weapon.MagazineSize` — 弹匣容量
- `Lyra.Weapon.SpareAmmo` — 备用弹药

放在 `InventoryItemInstance` 上。射击的 AbilityCost 检查/扣减此容器，换弹逻辑在服务器端修改此容器。FFastArraySerializer 保证只同步变化的条目。

---

## 十二、关键设计模式速查

| 模式 | 在哪里 | 解决什么问题 |
|------|--------|-------------|
| **数据驱动 AbilitySet** | PawnData / Equipment / GF | 三级授予，零硬编码 |
| **Tag-Based Input** | ASC + InputComponent + InputConfig | 灵活的输入→技能映射 |
| **Meta Attribute** | HealthSet.Damage/Healing | 中转值支持预处理/后处理 |
| **双 AttributeSet** | HealthSet + CombatSet | Source/Target 属性分离 |
| **可插拔 Cost** | UEqZeroAbilityCost 子类 | 非属性资源的消耗 |
| **Tag Relationship** | TagRelationshipMapping DataAsset | 集中式技能互斥管理 |
| **Definition-Instance** | Equipment / Inventory | 配置与运行时分离 |
| **CartridgeID 去重** | CustomEffectContext | 散弹枪多弹丸去重 |
| **AbilitySource 接口** | WeaponInstance | 武器特定衰减参数 |
| **Global Effect** | WorldSubsystem | 全场效果一键施加/收回 |
| **InitState 状态机** | PawnExtension + Hero | 网络安全的协作初始化 |
| **FFastArraySerializer** | Equipment / TagStack | 增量网络复制 |
| **ActivationGroup** | GA 基类 | 死亡阻塞其他技能 |

---

## 十三、应答参考

### Q: 一句话说 GAS 是什么？

> GAS 是 UE 内置的数据驱动技能框架，将技能（Ability）、效果（Effect）、属性（Attribute）三者解耦，用 GameplayTag 串联，原生支持网络预测。

### Q: 你项目中 ASC 放在哪？为什么？

> 放在 PlayerState 上。射击游戏有死亡-重生循环，Pawn 会被销毁重建。ASC 在 PlayerState 上可以跨 Pawn 生命周期保留技能状态、冷却、buff。代价是 Pawn 切换时需要手动重新关联 AvatarActor 和 AnimInstance。

### Q: 描述一次射击从按下扳机到敌人扣血的完整流程

> 按下开火 → ASC 创建 PredictionKey 激活 GA_RangedWeapon → 蓝图调 StartRangedWeaponTargeting → 预测窗口内做摄像机→焦点射线检测（含散布计算和容错球检测）→ 构建 TargetData → 本地缓存命中标记供 UI 显示 → 调 OnTargetDataReadyCallback → 客户端 RPC 发 TargetData 给服务器 → CommitAbility 预测性扣弹药 → 蓝图应用伤害 GE → 服务器执行 DamageExecution = BaseDamage × 距离衰减 × 材质衰减 → 输出到 HealthSet.Damage → PostGameplayEffectExecute 将 Damage 转为 Health 扣减 → Health 复制到客户端 → OnRep_Health + GAMEPLAYATTRIBUTE_REPNOTIFY 做 reconcile。

### Q: GAS 预测是怎么工作的？有什么限制？

> 客户端激活技能时创建 PredictionKey，在预测窗口内的所有 GE/Cue/Montage 关联此 Key。确认时通过 Key 匹配去重（不重复播 Cue、不重复加 GE），拒绝时通过 Key 找到所有预测副作用并回滚。限制：Execution 不支持预测（伤害计算只在服务器），GE 移除不可预测，周期性效果不可预测，预测窗口只在 ActivateAbility 的同步调用栈内有效——跨帧需手动开 FScopedPredictionWindow + 配套 Server RPC。

### Q: Lyra 相比传统 GAS 项目有什么架构创新？

> 七个关键创新：Experience 数据驱动游戏模式、AbilitySet 三级技能授予、Tag Relationship Mapping 集中管理技能互斥、Tag-Based Input 替代 InputID、ASC on PlayerState 跨重生保状态、InitState 状态机解决网络初始化时序、Global Ability System 全场效果管理。

### Q: 如何设计一个灵活的伤害计算管线？

> 用 GameplayEffectExecutionCalculation 做核心计算。关键设计：1) 双 AttributeSet 分离攻守属性，Execution 从 Source 捕获攻击力 → 输出到 Target 的 Meta Attribute；2) 自定义 EffectContext 携带武器引用和命中信息；3) 通过 AbilitySource 接口让武器实例提供距离/材质衰减曲线；4) Meta Attribute（Damage）经 PostGameplayEffectExecute 做最终的 Health 转换，支持预处理（免伤、护盾）。

### Q: 如何管理弹药系统？

> 用 GameplayTagStack（基于 FFastArraySerializer 的网络增量复制容器），Tag 标识弹药类型，Stack 数量就是弹药数。射击 GA 的 AbilityCost 检查并扣减 TagStack，换弹 GA 在服务器端从 SpareAmmo 转移到 MagazineAmmo。AbilityCost 是可插拔的 UObject 子类，支持"命中才扣费"的策略。

### Q: 如何处理技能之间的互斥关系？

> Lyra 的方案是 Tag Relationship Mapping DataAsset。每条规则描述：某个 AbilityTag 激活时 Block/Cancel 哪些标签的技能、添加哪些额外的 Required/Blocked 条件。ASC 在 ApplyAbilityBlockAndCancelTags 和 DoesAbilitySatisfyTagRequirements 中查询此表注入额外规则。好处：新增技能只需在表中加规则，不改已有 GA。

---

## 附录：核心类继承关系

```
UAbilitySystemComponent
  └─ UEqZeroAbilitySystemComponent
       ├─ Tag-based Input (ProcessAbilityInput)
       ├─ ActivationGroup 管理
       ├─ TagRelationshipMapping 注入
       └─ GlobalAbilitySystem 注册

UGameplayAbility
  └─ UEqZeroGameplayAbility
       ├─ ActivationPolicy / ActivationGroup
       ├─ AdditionalCosts[]
       ├─ CameraMode / FailureTag Mapping
       ├─ GA_Death, GA_Jump, GA_Dash, GA_Melee
       └─ GA_FromEquipment
            ├─ GA_RangedWeapon (射击)
            ├─ GA_ReloadMagazine (换弹)
            └─ GA_ADS (瞄准)

UAttributeSet
  └─ UEqZeroAttributeSet
       ├─ UEqZeroHealthSet  (Health, MaxHealth, Healing*, Damage*)
       └─ UEqZeroCombatSet  (BaseDamage, BaseHeal)

UObject
  └─ UEqZeroAbilityCost (Abstract, EditInlineNew)
       ├─ AbilityCost_ItemTagStack
       ├─ AbilityCost_PlayerTagStack
       └─ AbilityCost_InventoryItem

FGameplayEffectContext
  └─ FEqZeroGameplayEffectContext (+CartridgeID, +AbilitySourceObject)

UEquipmentInstance
  └─ UEqZeroWeaponInstance
       └─ UEqZeroRangedWeaponInstance (+ IEqZeroAbilitySourceInterface)

UPrimaryDataAsset: AbilitySet, PawnData, ExperienceDefinition
UDataAsset: TagRelationshipMapping, InputConfig
UWorldSubsystem: GlobalAbilitySystem
FFastArraySerializer: EquipmentList, GameplayTagStackContainer
```

---

*基于 Unreal Engine 5.6 + Lyra 架构复刻项目 EqZero*
*文档生成：2026-02-23*
