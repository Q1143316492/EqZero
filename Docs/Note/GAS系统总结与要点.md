# Gameplay Ability System (GAS) 深度总结 — 基于 Lyra 复刻项目

> 本文档系统性总结了基于 Lyra 架构复刻项目中对 GAS 的深度应用，涵盖架构设计、Lyra 独有模式、以及中的关键知识点。

---

## 目录

1. [GAS 核心架构总览](#1-gas-核心架构总览)
2. [ASC 的所有权设计 — ASC on PlayerState](#2-asc-的所有权设计--asc-on-playerstate)
3. [Lyra 的模块化角色初始化 — InitState 状态机](#3-lyra-的模块化角色初始化--initstate-状态机)
4. [AbilitySet — 数据驱动的技能授予体系](#4-abilityset--数据驱动的技能授予体系)
5. [Experience 系统 — 技能配置的顶层入口](#5-experience-系统--技能配置的顶层入口)
6. [Tag Relationship Mapping — 基于标签的技能关系管理](#6-tag-relationship-mapping--基于标签的技能关系管理)
7. [ActivationPolicy 与 ActivationGroup](#7-activationpolicy-与-activationgroup)
8. [输入系统与技能的绑定 — Tag-Based Input Binding](#8-输入系统与技能的绑定--tag-based-input-binding)
9. [自定义 GameplayEffectContext](#9-自定义-gameplayeffectcontext)
10. [Attribute 设计 — HealthSet 与 CombatSet 分离](#10-attribute-设计--healthset-与-combatset-分离)
11. [DamageExecution — 伤害计算管线](#11-damageexecution--伤害计算管线)
12. [自定义 AbilityCost 系统](#12-自定义-abilitycost-系统)
13. [Equipment → Ability → Weapon 管线](#13-equipment--ability--weapon-管线)
14. [Global Ability System — 全局技能/效果管理](#14-global-ability-system--全局技能效果管理)
15. [GameplayTagStack — 可复制的标签计数器](#15-gameplaytagstack--可复制的标签计数器)
16. [动态标签 GameplayEffect](#16-动态标签-gameplayeffect)
17. [GameplayTag 管理体系](#17-gameplaytag-管理体系)
18. [具体技能实现分析](#18-具体技能实现分析)
19. [网络同步与预测](#19-网络同步与预测)
20. [高频问题与回答要点](#20-高频问题与回答要点)

---

## 1. GAS 核心架构总览

### 系统组成

GAS 由以下核心组件构成:

| 组件 | 职责 |
|------|------|
| **AbilitySystemComponent (ASC)** | 技能系统的核心运行时，管理 GA/GE/AS 的生命周期 |
| **GameplayAbility (GA)** | 技能的逻辑实现单元 |
| **GameplayEffect (GE)** | 对属性的修改描述（Modifier / Execution） |
| **AttributeSet (AS)** | 属性集合（Health, Damage 等） |
| **GameplayTag** | 标签系统，GAS 中用于状态标记、条件门控、消息传递 |
| **GameplayCue** | 表现层（特效/音效），与逻辑解耦 |
| **AbilityTask** | 技能内的异步任务（等待输入、等待事件等） |

### 项目中的架构分层

```
Experience Definition (体验定义)
    ├── PawnData (角色数据)
    │     ├── AbilitySets[] (技能集)
    │     ├── InputConfig (输入配置)
    │     └── TagRelationshipMapping (标签关系映射)
    ├── GameFeatureActions (游戏特性动作)
    │     ├── AddAbilities (添加技能/属性)
    │     ├── AddInputBinding (添加输入绑定)
    │     └── AddWidget (添加UI)
    └── ActionSets (动作集合)

Equipment System (装备系统)
    ├── EquipmentDefinition → AbilitySets[] (装备定义)
    ├── EquipmentInstance → WeaponInstance (装备实例)
    └── EquipmentManagerComponent (装备管理器)

AbilitySystem (技能系统)
    ├── EqZeroAbilitySystemComponent (自定义 ASC)
    ├── EqZeroGameplayAbility (自定义 GA 基类)
    ├── HealthSet / CombatSet (属性集)
    ├── DamageExecution (伤害计算)
    └── AbilityCost (自定义花费)
```

---

## 2. ASC 的所有权设计 — ASC on PlayerState

### 设计决策

项目中 ASC 挂载在 **PlayerState** 上，而非 Pawn：

```
OwnerActor = PlayerState
AvatarActor = Pawn (Character)
```

### 为什么这么做

1. **角色重生后状态保留**: Pawn 死亡销毁后，PlayerState 依然存在，ASC 上的 GE（如 buff/debuff）、冷却信息不会丢失
2. **角色切换**: 如果业务需要操控不同角色（载具、附身等），只需切换 AvatarActor，ASC 上的状态依然保留
3. **Lyra 的推荐范式**: 适用于多人联机的射击游戏、需要死亡-重生循环的游戏

### 关键实现

`InitAbilityActorInfo` 中检测 AvatarActor 变化：
- 当新的 Pawn 挂载时，通知所有已授予的 Ability 执行 `OnPawnAvatarSet()`
- 重新关联 AnimInstance 与 ASC
- 注册到 GlobalAbilitySystem
- 触发 OnSpawn 策略的技能激活

```cpp
// EqZeroAbilitySystemComponent.cpp
void UEqZeroAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
    const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
    Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
    if (bHasNewPawnAvatar)
    {
        // 通知所有技能 Avatar 变化
        // 注册到 GlobalAbilitySystem
        // 重新关联 AnimInstance
        // 激活 OnSpawn 技能
    }
}
```

### 要点

> **Q: ASC 应该放在 Pawn 还是 PlayerState 上？**
>
> A: 取决于游戏类型。对于有死亡-重生机制的多人游戏，放在 PlayerState 上更合适，因为：
> 1. Pawn 销毁后状态可以保留（buff、冷却）
> 2. 支持角色切换而不丢失技能状态
> 3. 需要注意的是 AvatarActor 变化时需要重新初始化 ActorInfo，处理好 AnimInstance 的重绑定

---

## 3. Lyra 的模块化角色初始化 — InitState 状态机

### 四阶段初始化

Lyra 使用 `IGameFrameworkInitStateInterface` 定义了四个初始化阶段：

```
Spawned → DataAvailable → DataInitialized → GameplayReady
```

| 阶段 | 标签 | 含义 |
|------|------|------|
| Spawned | `InitState.Spawned` | Actor 已生成，但数据尚未就绪 |
| DataAvailable | `InitState.DataAvailable` | PawnData 等关键数据已可用 |
| DataInitialized | `InitState.DataInitialized` | 技能/输入/属性已初始化 |
| GameplayReady | `InitState.GameplayReady` | 所有初始化完成，可以开始游戏 |

### 协作式初始化

角色上有多个组件需要协作初始化：

```
EqZeroPawnExtensionComponent (协调者)
    ├── 持有 PawnData 引用
    ├── 创建/初始化 ASC
    └── 协调其他组件的初始化

EqZeroHeroComponent (英雄组件)
    ├── 等待 PawnData 就绪
    ├── 绑定输入 → 技能
    ├── 设置摄像机
    └── 授予 PawnData 中的 AbilitySets
```

### 为什么需要状态机

在网络游戏中，各个组件的数据到达时机是不确定的（Replicated 属性的到达顺序）。状态机确保：

1. 每个组件只在其前置依赖就绪后才进入下一阶段
2. 避免竞态条件（Race Condition）
3. 客户端和服务端都走相同的初始化流程

---

## 4. AbilitySet — 数据驱动的技能授予体系

### 核心设计

`UEqZeroAbilitySet` 是一个 `UPrimaryDataAsset`，将 GA、GE、AS 打包成一个可配置的数据资产：

```cpp
UCLASS(BlueprintType, Const)
class UEqZeroAbilitySet : public UPrimaryDataAsset
{
    // 要授予的技能列表（GA + Level + InputTag）
    TArray<FEqZeroAbilitySet_GameplayAbility> GrantedGameplayAbilities;
    
    // 要授予的效果列表（GE + Level）
    TArray<FEqZeroAbilitySet_GameplayEffect> GrantedGameplayEffects;
    
    // 要授予的属性集列表（AS Class）
    TArray<FEqZeroAbilitySet_AttributeSet> GrantedAttributes;
};
```

### 关键点：InputTag 绑定

每个 GA 条目都可以关联一个 `InputTag`：

```cpp
struct FEqZeroAbilitySet_GameplayAbility
{
    TSubclassOf<UEqZeroGameplayAbility> Ability;
    int32 AbilityLevel = 1;
    FGameplayTag InputTag;  // 例如 "InputTag.Ability.Fire"
};
```

授予技能时，InputTag 被设置为 `DynamicSpecSourceTags`：
```cpp
AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityEntry.InputTag);
```

这样在处理输入时，就能通过 Tag 匹配找到对应的技能。

### GrantedHandles — 授予/收回的句柄管理

```cpp
struct FEqZeroAbilitySet_GrantedHandles
{
    TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
    TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
    TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
    
    void TakeFromAbilitySystem(UEqZeroAbilitySystemComponent* ASC);  // 一键收回
};
```

### 三个授予来源

AbilitySet 从三个层级授予：

| 来源 | 时机 | 说明 |
|------|------|------|
| PawnData | 角色初始化时 | 基础技能（移动、跳跃、死亡等） |
| EquipmentDefinition | 装备时 | 装备相关技能（开火、换弹、ADS等） |
| GameFeatureAction | GF 激活时 | 模式特定技能（如特殊模式下的能力） |

---

## 5. Experience 系统 — 技能配置的顶层入口

### 什么是 Experience

Experience 是 Lyra 最具特色的设计之一。它是一个数据驱动的"游戏模式配置包"：

```cpp
UCLASS(BlueprintType, Const)
class UEqZeroExperienceDefinition : public UPrimaryDataAsset
{
    TArray<FString> GameFeaturesToEnable;          // 要启用的 GameFeature 插件
    const UEqZeroPawnData* DefaultPawnData;        // 默认角色配置
    TArray<UGameFeatureAction*> Actions;           // GameFeature Actions
    TArray<UEqZeroExperienceActionSet*> ActionSets; // Action 集合
};
```

### 体验加载流程

```
WorldSettings 指定 Experience
    → ExperienceManagerComponent 加载 Experience 资产
    → 启用 GameFeature 插件
    → 执行 GameFeatureActions
        → AddAbilities: 为角色添加技能/属性
        → AddInputBinding: 添加输入绑定
        → AddWidget: 添加 UI
    → 广播 OnExperienceLoaded
    → PawnData 就绪 → 角色初始化开始
```

### 要点

> **Q: Lyra 的 Experience 系统解决了什么问题？**
>
> A: 传统的做法是在 GameMode 中硬编码游戏规则，Experience 将其变为数据驱动。一个 Experience 包含了 PawnData（角色配置）、GameFeatureActions（模块化功能注入）、ActionSets（可复用的功能集合）。切换游戏模式只需切换 Experience 资产，无需修改代码。同时结合 GameFeature 插件，实现了真正的模块化 — 不同模式的技能、输入、UI 可以独立开发，按需组合。

---

## 6. Tag Relationship Mapping — 基于标签的技能关系管理

### 设计背景

GAS 原生的 Block/Cancel Tags 是直接写在每个 GA 上的。这会导致：
- 新增技能时需要修改所有相关技能的 Tag 配置
- 技能之间的关系分散在各个 GA 中，难以维护

### Lyra 的解决方案

引入 `UEqZeroAbilityTagRelationshipMapping` 数据资产，**集中管理所有技能之间的关系**：

```cpp
USTRUCT()
struct FEqZeroAbilityTagRelationship
{
    FGameplayTag AbilityTag;                    // 源技能标签
    FGameplayTagContainer AbilityTagsToBlock;   // 激活时阻塞这些技能
    FGameplayTagContainer AbilityTagsToCancel;  // 激活时取消这些技能
    FGameplayTagContainer ActivationRequiredTags; // 激活的附加必需标签
    FGameplayTagContainer ActivationBlockedTags;  // 激活的附加阻塞标签
};
```

### 工作原理

1. **激活时**: ASC 重写 `ApplyAbilityBlockAndCancelTags()`，在执行 Block/Cancel 前，先查询 TagRelationshipMapping 补充额外的 Block/Cancel Tags

2. **检查能否激活时**: ASC 提供 `GetAdditionalActivationTagRequirements()`，GA 的 `DoesAbilitySatisfyTagRequirements()` 中调用它获取额外的 Required/Blocked Tags

```
技能激活 → ApplyAbilityBlockAndCancelTags
    → TagRelationshipMapping.GetAbilityTagsToBlockAndCancel() // 追加
    → Super::ApplyAbilityBlockAndCancelTags()                 // 执行

技能尝试激活 → CanActivateAbility → DoesAbilitySatisfyTagRequirements
    → GetAdditionalActivationTagRequirements()
    → TagRelationshipMapping.GetRequiredAndBlockedActivationTags() // 追加条件
```

### 要点

> **Q: 如何管理大量技能之间的互斥/阻塞关系？**
>
> A: 将技能间的关系从单个 GA 中抽离出来，集中到一个 DataAsset 中管理（Tag Relationship Mapping）。每条规则描述：某个 AbilityTag 激活时应该 Block/Cancel 哪些技能、需要什么前置条件。这样新增技能只需在映射表中添加一条规则，不需要修改已有技能。ASC 在 `ApplyAbilityBlockAndCancelTags` 和 `DoesAbilitySatisfyTagRequirements` 中注入这些额外规则。

---

## 7. ActivationPolicy 与 ActivationGroup

### ActivationPolicy — 激活时机

```cpp
enum class EEqZeroAbilityActivationPolicy : uint8
{
    OnInputTriggered,   // 按下输入时激活（一次性）
    WhileInputActive,   // 输入持续按住时持续尝试激活
    OnSpawn             // Avatar 设置时自动激活
};
```

| 策略 | 用例 |
|------|------|
| OnInputTriggered | 开火、跳跃、冲刺等 |
| WhileInputActive | 持续按住才生效的技能 |
| OnSpawn | 死亡技能监听、被动效果 |

### ActivationGroup — 激活互斥组

```cpp
enum class EEqZeroAbilityActivationGroup : uint8
{
    Independent,            // 独立，不受影响
    Exclusive_Replaceable,  // 排他可替换，会被后来的排他技能取消
    Exclusive_Blocking      // 排他阻塞，阻止其他排他技能激活
};
```

**实际项目中的应用**:
- 绝大多数技能（开火、跳跃、冲刺等）都是 `Independent`
- 死亡技能运行时会动态切换为 `Exclusive_Blocking`，从而取消所有其他 `Exclusive_Replaceable` 技能并阻塞新技能激活

```cpp
// 死亡技能中动态切换 ActivationGroup
void UEqZeroGameplayAbility_Death::ActivateAbility(...)
{
    ChangeActivationGroup(EEqZeroAbilityActivationGroup::Exclusive_Blocking);
    // ...
}
```

---

## 8. 输入系统与技能的绑定 — Tag-Based Input Binding

### 架构设计

Lyra 摒弃了 GAS 原生的 `InputID` 绑定方式，改用 **基于 GameplayTag 的输入绑定**：

```
Enhanced Input Action → EqZeroInputConfig (InputTag 映射表) → EqZeroInputComponent → ASC
```

### 流程

1. **配置阶段**: `UEqZeroInputConfig` 中配置 `InputAction → InputTag` 的映射
2. **绑定阶段**: `HeroComponent` 初始化时通过 `EqZeroInputComponent` 绑定输入回调
3. **授予阶段**: AbilitySet 授予 GA 时，将 InputTag 写入 `DynamicSpecSourceTags`
4. **运行时**:
   - 输入触发 → `ASC::AbilityInputTagPressed(InputTag)`
   - 将匹配的 AbilitySpec 加入 Pressed/Held 队列
   - 每帧 `ProcessAbilityInput()` 处理队列

```cpp
void UEqZeroAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
    // 1. 检查输入阻塞标签
    if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
    {
        ClearAbilityInput();
        return;
    }
    
    // 2. Held 队列 → WhileInputActive 策略的技能
    // 3. Pressed 队列 → OnInputTriggered 策略的技能 + 已激活技能的输入事件
    // 4. 批量尝试激活
    // 5. Released 队列 → 发送 InputReleased 事件
    // 6. 清理 Pressed/Released 队列
}
```

### 为什么不用原生 InputID

- 原生 InputID 是 `int32`，缺乏语义
- 基于 Tag 的方式更灵活，可以动态绑定/解绑
- Tag 可以直接参与 GAS 的标签门控系统
- 更好地支持 GameFeature 的模块化注入

---

## 9. 自定义 GameplayEffectContext

### 为什么需要自定义

原生的 `FGameplayEffectContext` 包含基本的 Instigator、EffectCauser、HitResult 等。项目需要额外信息来支持更精确的伤害计算。

### 自定义字段

```cpp
struct FEqZeroGameplayEffectContext : public FGameplayEffectContext
{
    // 用于标识同一弹药的多个弹丸（散弹枪去重）
    int32 CartridgeID = -1;
    
    // 技能来源对象（实现了 IEqZeroAbilitySourceInterface）
    // 用于获取距离衰减曲线、物理材质衰减等
    TWeakObjectPtr<const UObject> AbilitySourceObject;
};
```

### CartridgeID 的用途

散弹枪一次射击发射多个弹丸。每个弹丸都会产生独立的伤害 GE。CartridgeID 标识它们来自同一次射击，用于：
- GameplayCue 去重（同一次射击只播一次击中反馈）
- 伤害统计（区分不同次射击）

### AbilitySourceInterface

```cpp
class IEqZeroAbilitySourceInterface
{
    float GetDistanceAttenuation(float Distance, ...) const;      // 距离衰减
    float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysMat, ...) const; // 物理材质衰减
};
```

武器实例（`UEqZeroRangedWeaponInstance`）实现此接口，根据武器配置的衰减曲线返回衰减系数。

### 如何接入

在 `UEqZeroAbilitySystemGlobals` 中重写全局 Context 分配：

```cpp
FGameplayEffectContext* UEqZeroAbilitySystemGlobals::AllocGameplayEffectContext() const
{
    return new FEqZeroGameplayEffectContext();
}
```

在 `DefaultGame.ini` 中指定：
```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
AbilitySystemGlobalsClassName=/Script/EqZeroGame.EqZeroAbilitySystemGlobals
```

---

## 10. Attribute 设计 — HealthSet 与 CombatSet 分离

### 双 AttributeSet 架构

| AttributeSet | 属性 | 归属 | 作用 |
|---|---|---|---|
| **HealthSet** | Health, MaxHealth, Healing, Damage | 受击方 | 承受伤害/治疗 |
| **CombatSet** | BaseDamage, BaseHeal | 攻击方 | 描述输出能力 |

### 为什么分离

1. **职责清晰**: HealthSet 描述"我有多少血"，CombatSet 描述"我能打多少伤害"
2. **Execution Capture**: `DamageExecution` 从 **Source 的 CombatSet** 捕获 BaseDamage，计算后写入 **Target 的 HealthSet.Damage**
3. **灵活的 buff 系统**: 可以独立修改攻击方的输出（CombatSet）和防御方的承受（HealthSet）

### Meta Attribute 模式

HealthSet 中的 `Damage` 和 `Healing` 是 **Meta Attribute（元属性）**：

- 不代表持久状态，是临时的"传递用属性"
- `DamageExecution` → 写入 `Damage` 值 → `PostGameplayEffectExecute` 中将 `Damage` 转化为 `Health` 的扣减
- 这样做的好处：可以在 `PreGameplayEffectExecute` 中做最后一道检查（如免伤、最低血量）

```cpp
void UEqZeroHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        // Damage → Health 转换
        SetHealth(FMath::Clamp(GetHealth() - GetDamage(), 0.0f, GetMaxHealth()));
        SetDamage(0.0f);  // 重置元属性
    }
    else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth() + GetHealing(), 0.0f, GetMaxHealth()));
        SetHealing(0.0f);
    }
}
```

### 属性变化回调链

```
GE 应用 → PreGameplayEffectExecute (可拦截，如免伤判断)
        → 属性修改
        → PostGameplayEffectExecute (Meta → Real 转换)
        → PreAttributeChange (钳制值)
        → PostAttributeChange (广播事件)
        → OnRep_Health (客户端同步回调)
```

---

## 11. DamageExecution — 伤害计算管线

### 完整伤害流程

```
攻击方开火 → 命中判定 (Trace)
    → 创建 GE Spec (含 DamageExecution)
    → 设置 EffectContext (HitResult, CartridgeID, AbilitySource)
    → 应用 GE 到目标 ASC
    → DamageExecution::Execute()
        → 捕获 Source.CombatSet.BaseDamage
        → 计算距离衰减 (AbilitySource.GetDistanceAttenuation)
        → 计算材质衰减 (AbilitySource.GetPhysicalMaterialAttenuation)
        → 输出到 Target.HealthSet.Damage
    → HealthSet.PostGameplayEffectExecute()
        → Damage → Health 转换
        → 触发 OnHealthChanged / OnOutOfHealth
    → HealthComponent 监听 → 触发死亡流程
```

### 衰减计算

```cpp
float DamageDone = BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * DamageInteractionAllowedMultiplier;
OutExecutionOutput.AddOutputModifier(
    FGameplayModifierEvaluatedData(UEqZeroHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, DamageDone)
);
```

### 要点

> **Q: GAS 中如何实现灵活的伤害计算？**
>
> A: 使用 `GameplayEffectExecutionCalculation`。关键步骤：
> 1. 定义 `FGameplayEffectAttributeCaptureDefinition` 声明需要捕获的属性（如 Source 的 BaseDamage）
> 2. 在 `Execute_Implementation` 中从自定义 GameplayEffectContext 获取额外信息（HitResult, AbilitySource）
> 3. 通过 `IEqZeroAbilitySourceInterface` 获取武器特定的衰减曲线
> 4. 最终输出到 Target 的 Meta Attribute（Damage），由 AttributeSet 的 PostGameplayEffectExecute 完成最终转换

---

## 12. 自定义 AbilityCost 系统

### 设计动机

GAS 原生的 Cost 只支持通过 GE 消耗属性值（如蓝量）。项目需要更灵活的消耗：
- 消耗弹药（基于 TagStack 的子弹数）
- 消耗道具（基于物品栏的物品数）
- 命中后才扣费

### 架构

```cpp
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class UEqZeroAbilityCost : public UObject
{
    virtual bool CheckCost(const UEqZeroGameplayAbility* Ability, ...) const;
    virtual void ApplyCost(const UEqZeroGameplayAbility* Ability, ...) const;
    
    // 只有命中才扣费（如稀有弹药）
    bool bOnlyApplyCostOnHit = false;
};
```

### 三种 Cost 实现

| Cost 类型 | 消耗对象 | 用途 |
|---|---|---|
| `AbilityCost_ItemTagStack` | Equipment 上的 TagStack（如弹药数） | 射击消耗子弹 |
| `AbilityCost_PlayerTagStack` | Controller 上的 TagStack | 消耗角色资源 |
| `AbilityCost_InventoryItem` | Inventory 中的道具数量 | 消耗消耗品 |

### GA 基类中的集成

```cpp
// EqZeroGameplayAbility.h
UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
TArray<TObjectPtr<UEqZeroAbilityCost>> AdditionalCosts;
```

在 `CheckCost` 和 `ApplyCost` 中遍历所有 AdditionalCosts：

```cpp
bool UEqZeroGameplayAbility::CheckCost(...)
{
    if (!Super::CheckCost(...)) return false;
    for (auto Cost : AdditionalCosts)
    {
        if (Cost && !Cost->CheckCost(this, Handle, ActorInfo, OptionalRelevantTags))
            return false;
    }
    return true;
}
```

---

## 13. Equipment → Ability → Weapon 管线

### 完整管线

```
InventoryItemDefinition (物品定义)
    └── Fragment: EquippableItem → EquipmentDefinition
                                      ├── InstanceType: WeaponInstance / RangedWeaponInstance
                                      ├── AbilitySetsToGrant[]: (Fire, Reload, ADS 等)
                                      └── ActorsToSpawn[]: (武器模型 Actor)

EquipmentManagerComponent (FFastArraySerializer 管理)
    ├── EquipItem() → 创建 EquipmentInstance → 授予 AbilitySets → 生成武器模型
    └── UnequipItem() → 收回技能 → 销毁模型 → 销毁实例
```

### 武器实例与 AbilitySource

```cpp
class UEqZeroRangedWeaponInstance : public UEqZeroWeaponInstance, public IEqZeroAbilitySourceInterface
{
    // 武器属性
    float SpreadAngle;              // 散射角度
    float SpreadAngleMultiplier;    // 运动时散射增大
    float SpreadExponent;           // 散射指数
    float HeatToSpreadCurve;        // 枪管热度→散射映射
    float HeatToHeatPerShotCurve;   // 热度→每发增热映射
    float HeatToCoolDownPerSecondCurve; // 热度→冷却速率
    
    // 来自 IEqZeroAbilitySourceInterface
    UCurveFloat* DistanceDamageFalloff;        // 距离衰减曲线
    UCurveFloat* PhysicalMaterialAttenuation;  // 材质衰减曲线
};
```

### GA_FromEquipment

```cpp
// 从装备发起的技能基类
class UEqZeroGameplayAbility_FromEquipment : public UEqZeroGameplayAbility
{
    // 自动获取授予此技能的装备实例
    UEqZeroEquipmentInstance* GetAssociatedEquipment() const;
    UEqZeroInventoryItemInstance* GetAssociatedItem() const;
};
```

射击技能 (`GA_RangedWeapon`)、换弹技能 (`GA_ReloadMagazine`)、ADS 技能 (`GA_ADS`) 都继承自此基类。

---

## 14. Global Ability System — 全局技能/效果管理

### 设计目的

需要对 **场上所有玩家** 同时施加/移除技能或效果的场景：
- 暖场阶段全场无敌
- 全场减速
- 全场禁用某类技能

### 实现

`UEqZeroGlobalAbilitySystem` 是一个 `UWorldSubsystem`：

```cpp
class UEqZeroGlobalAbilitySystem : public UWorldSubsystem
{
    void ApplyAbilityToAll(TSubclassOf<UGameplayAbility> Ability);
    void ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect);
    void RemoveAbilityFromAll(TSubclassOf<UGameplayAbility> Ability);
    void RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect);
    
    void RegisterASC(UEqZeroAbilitySystemComponent* ASC);
    void UnregisterASC(UEqZeroAbilitySystemComponent* ASC);
};
```

**关键设计**: 新加入的 ASC 注册时，会自动应用所有当前活跃的全局 Ability/Effect。

---

## 15. GameplayTagStack — 可复制的标签计数器

### 是什么

一个基于 `FFastArraySerializer` 的可网络同步的"标签-数量"容器：

```cpp
struct FGameplayTagStackContainer : public FFastArraySerializer
{
    void AddStack(FGameplayTag Tag, int32 StackCount);
    void RemoveStack(FGameplayTag Tag, int32 StackCount);
    int32 GetStackCount(FGameplayTag Tag) const;
    bool ContainsTag(FGameplayTag Tag) const;
};
```

### 应用场景

| 场景 | Tag | 说明 |
|------|-----|------|
| 弹药系统 | `Lyra.Weapon.MagazineAmmo` | 当前弹夹内子弹数 |
| 弹药系统 | `Lyra.Weapon.MagazineSize` | 弹夹容量 |
| 弹药系统 | `Lyra.Weapon.SpareAmmo` | 备用弹药数 |

### 为什么用 FFastArraySerializer

- 增量复制：只同步变化的项，不是全量同步
- 比 `TArray<UPROPERTY(Replicated)>` 的复制效率更高
- 支持插入/删除通知

---

## 16. 动态标签 GameplayEffect

### 设计

通过一个通用的 GE 模板，运行时动态设置 `DynamicGrantedTags`，实现给 ASC 动态添加/移除 GameplayTag：

```cpp
void UEqZeroAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
    const TSubclassOf<UGameplayEffect> DynamicTagGE = /* 从 GameData 获取 */;
    FGameplayEffectSpec* Spec = MakeOutgoingSpec(DynamicTagGE, ...).Data.Get();
    Spec->DynamicGrantedTags.AddTag(Tag);
    ApplyGameplayEffectSpecToSelf(*Spec);
}
```

用途：
- 运行时标记状态（如蹲伏 `Status.Crouching`）
- 移动模式标签（`Movement.Mode.Walking/Falling/...`）
- 任何需要通过 GE 管理生命周期的动态标签

---

## 17. GameplayTag 管理体系

### 分类体系

项目采用 Native Gameplay Tags（C++ 注册），分为以下几类：

| 前缀 | 用途 | 示例 |
|------|------|------|
| `Ability.ActivateFail.*` | 技能激活失败原因 | `IsDead`, `Cooldown`, `Cost`, `TagsBlocked` |
| `Ability.Behavior.*` | 技能行为标记 | `SurvivesDeath` |
| `Ability.Type.*` | 技能类型标记 | `StatusChange.Death` |
| `InputTag.*` | 输入绑定标签 | `Move`, `Look_Mouse`, `Crouch` |
| `InitState.*` | 初始化状态 | `Spawned`, `DataAvailable`, `DataInitialized` |
| `GameplayEvent.*` | 事件标签 | `Death`, `Reset`, `RequestReset` |
| `Status.*` | 状态标签 | `Crouching`, `Death.Dying`, `Death.Dead` |
| `Movement.Mode.*` | 移动模式 | `Walking`, `Falling`, `Swimming` |
| `Event.Movement.*` | 动画事件 | `ADS`, `WeaponFire`, `Reload`, `Dash` |
| `Gameplay.*` | 通用玩法标签 | `Damage`, `DamageImmunity`, `AbilityInputBlocked` |
| `SetByCaller.*` | SetByCaller 标签 | `Damage`, `Heal` |
| `Cheat.*` | 作弊标签 | `GodMode`, `UnlimitedHealth` |

### 移动模式标签映射

```cpp
const TMap<uint8, FGameplayTag> MovementModeTagMap;  // EMovementMode → Tag
const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;
```

CharacterMovementComponent 中移动模式变化时，自动通过动态标签 GE 更新对应的 GameplayTag。

### 失败标签的 UI 反馈

GA 基类中配置：

```cpp
// 失败原因 → UI 文本
TMap<FGameplayTag, FText> FailureTagToUserFacingMessages;
// 失败原因 → 动画蒙太奇
TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FailureTagToAnimMontage;
```

当技能激活失败时，ASC 通过 `NotifyAbilityFailed` → `HandleAbilityFailed` → `OnAbilityFailedToActivate` 传递失败原因标签，UI 层根据标签显示对应文案/动画。

---

## 18. 具体技能实现分析

### GA_Death — 死亡技能

- **ActivationPolicy**: OnSpawn（角色生成时即激活监听）
- **触发方式**: 通过 GameplayEvent `GameplayEvent.Death` 触发
- **关键行为**:
  1. 动态切换 ActivationGroup 为 `Exclusive_Blocking` → 取消其他所有排他技能
  2. 取消所有输入激活的技能
  3. 通过 HealthComponent 执行死亡流程
  4. 等待死亡蒙太奇结束
  5. 广播死亡完成事件
- **标签**: `Ability.Behavior.SurvivesDeath` — 有此标签的技能不会被死亡取消

### GA_Jump — 跳跃技能

- **ActivationPolicy**: OnInputTriggered
- **ActivationGroup**: Independent
- **特点**: 直接操作 CharacterMovementComponent 的 Jump/StopJumping， EndAbility 时清理

### GA_Dash — 冲刺技能

- **方向性蒙太奇**: 根据输入方向选择前/后/左/右冲刺蒙太奇
- **根运动**: 使用动画 Root Motion 驱动移动
- **实现**: 使用 AbilityTask_PlayMontageAndWait

### GA_RangedWeapon — 远程武器射击

- **完整射击流程**:
  1. 从武器实例获取射击参数（散射、热度）
  2. 执行射线检测（支持多弹丸散弹）
  3. 构建 TargetData
  4. 创建伤害 GE，设置 Context（CartridgeID, AbilitySource）
  5. 应用伤害到目标
  6. 触发 GameplayCue（枪口火焰、弹道、命中效果）
  7. 更新武器热度

### GA_ADS — 瞄准镜

- **切换摄像机**: 通过 `SetCameraMode()` 切换为瞄准镜摄像机模式
- **速度修改**: ADS 状态下降低移动速度
- **UI 联动**: 通过 GameplayMessage 通知 UI 显示/隐藏准星
- **技能状态**: 使用 WaitInputRelease 任务监听松开

### GA_ReloadMagazine — 换弹

- **成本检查**: 检查 SpareAmmo TagStack
- **蒙太奇驱动**: 播放换弹蒙太奇
- **弹药修改**: 通过 TagStack 加减弹药数

### GA_AutoRespawn — 自动重生

- **消息驱动**: 监听死亡完成消息
- **延迟重生**: 配置重生倒计时
- **流程**: 死亡 → 等待时间 → 发送 RequestReset 事件 → 重生

---

## 19. 网络同步与预测

### GAS 预测系统概述

GAS 预测的核心原则是 **对 Ability 实现透明** — 技能只需描述"做什么"，系统自动处理预测。

### 当前可预测的内容

- GA 激活（包括有条件的链式激活）
- GE 应用（属性 Modifier，Tag 修改）
- GameplayCue 事件
- 蒙太奇播放
- 角色移动

### 当前不可预测的内容

- GE 移除
- GE 周期性效果（DoT）
- Execution（如 DamageExecution）

### 预测窗口 (Prediction Window)

```
客户端：激活 GA → 创建 PredictionKey → 预测性应用 GE → 等待服务端确认
服务端：收到 RPC → 验证 → 执行 → 回复确认/拒绝
客户端：收到确认 → 保留预测结果 / 收到拒绝 → 回滚
```

### 项目中的网络考量

1. **不使用 bReplicateInputDirectly**: 改用 ReplicatedEvent，保证 `WaitInputPress/Release` AbilityTask 正常工作
2. **失败通知到客户端**: 通过 `ClientNotifyAbilityFailed` (Client RPC, Unreliable) 通知客户端显示失败 UI
3. **Equipment 使用 FFastArraySerializer**: 装备列表的增量复制
4. **TagStack 使用 FFastArraySerializer**: 弹药等计数器的增量复制
5. **TargetData 同步**: 通过 `GetAbilityTargetData` 从 `AbilityTargetDataMap` 获取复制的目标数据

---

## 20. 高频问题与回答要点

### Q1: 简述 GAS 的核心组件及其关系

> **A**: GAS 核心由五部分组成：
> - **ASC** 是运行时核心，管理技能（GA）的生命周期、效果（GE）的应用、属性（AS）的持有
> - **GA** 定义技能逻辑，通过 AbilityTask 实现异步行为
> - **GE** 描述对属性的修改，分为 Instant/Duration/Infinite 三种
> - **AS** 定义属性集合，响应属性变化
> - **GameplayTag** 贯穿所有组件，用于状态标记、条件门控、事件标识

### Q2: 描述你项目中的伤害计算流程

> **A**: 
> 1. 射击技能做射线检测，命中后创建带 DamageExecution 的 GE
> 2. 自定义 GameplayEffectContext 携带 CartridgeID（散弹去重）和 AbilitySource（武器引用）
> 3. DamageExecution 从攻击方 CombatSet 捕获 BaseDamage
> 4. 通过 AbilitySource 接口获取武器配置的距离衰减和材质衰减曲线
> 5. 最终伤害值写入目标 HealthSet 的 Damage 元属性
> 6. HealthSet 的 PostGameplayEffectExecute 将 Damage 转换为 Health 扣减
> 7. HealthComponent 监听 Health 变化，触发死亡流程

### Q3: Lyra 相比传统 GAS 项目有哪些架构创新？

> **A**: 主要创新：
> 1. **Experience 系统** — 数据驱动的游戏模式配置，结合 GameFeature 实现模块化
> 2. **Tag Relationship Mapping** — 集中管理技能间的阻塞/取消关系，避免修改散落在各个 GA 中
> 3. **AbilitySet** — 将 GA/GE/AS 打包为数据资产，支持从 PawnData、Equipment、GameFeature 三个层级授予
> 4. **Tag-Based Input** — 摒弃 InputID，用 GameplayTag 绑定输入和技能
> 5. **ASC on PlayerState** — 支持跨 Pawn 生命周期的状态保持
> 6. **InitState 状态机** — 解决网络游戏中组件初始化时序问题
> 7. **Global Ability System** — WorldSubsystem 管理全场技能/效果

### Q4: 如何处理技能的消耗？

> **A**: 项目设计了一个插件式的 Cost 系统。GA 基类持有一个 `TArray<UEqZeroAbilityCost*>` 数组，每个 Cost 是一个 UObject 子类（EditInlineNew），实现 CheckCost/ApplyCost 接口。项目实现了三种：弹药 TagStack 消耗、角色 TagStack 消耗、物品栏道具消耗。特别地，支持 `bOnlyApplyCostOnHit` 标记，只在命中时才扣费。

### Q5: GAS 的网络预测是如何工作的？

> **A**: GAS 使用 PredictionKey 机制。客户端激活技能时创建一个 PredictionKey，预测性地应用效果（属性修改、Tag 变更、Cue 播放）。服务端执行相同逻辑后回复确认。如果服务端拒绝，客户端按 PredictionKey 回滚所有对应的预测效果。注意 Execution 类型的 GE 不支持预测，只有 Modifier 支持。项目中伤害（DamageExecution）只在服务端计算。

### Q6: 解释你的装备系统如何与 GAS 交互

> **A**: 装备系统采用 Definition-Instance 分离模式：
> - `EquipmentDefinition` 是数据配置，声明装备时要授予的 AbilitySets 和要生成的 Actor
> - `EquipmentManagerComponent` 使用 FFastArraySerializer 管理装备列表，实现增量复制
> - 装备时创建 `EquipmentInstance`（如 WeaponInstance），授予 AbilitySets，生成模型
> - 卸装时通过 GrantedHandles 一次性收回所有 GA/GE/AS，销毁模型
> - `GA_FromEquipment` 基类提供快速访问关联装备实例/物品实例的能力
> - 武器实例实现 `IEqZeroAbilitySourceInterface`，为伤害计算提供衰减数据

### Q7: 如何调试 GAS 相关的问题？

> **A**: 
> 1. `showdebug abilitysystem` — 运行时查看 ASC 上所有 GA/GE/Tag 的状态
> 2. Gameplay Debugger (`) — 可视化查看标签、属性
> 3. 项目中自定义的日志通道 `LogEqZeroAbilitySystem`
> 4. `FailureTagToUserFacingMessages` — 通过失败标签追踪技能未能激活的原因
> 5. 属性变化回调链每一步都可以加断点/日志

---

## 附录：核心类继承关系

```
UAbilitySystemComponent
    └── UEqZeroAbilitySystemComponent
            ├── Tag-based Input Processing
            ├── ActivationGroup Management
            ├── TagRelationshipMapping Integration
            └── GlobalAbilitySystem Registration

UGameplayAbility
    └── UEqZeroGameplayAbility
            ├── ActivationPolicy / ActivationGroup
            ├── AdditionalCosts[]
            ├── CameraMode
            ├── FailureTag → UI/Montage Mapping
            ├── UEqZeroGameplayAbility_Death
            ├── UEqZeroGameplayAbility_Jump
            ├── UEqZeroGameplayAbility_Dash
            ├── UEqZeroGameplayAbility_Melee
            ├── UEqZeroGameplayAbility_AutoRespawn
            ├── UEqZeroGameplayAbility_WithWidget
            └── UEqZeroGameplayAbility_FromEquipment
                    ├── UEqZeroGameplayAbility_RangedWeapon
                    ├── UEqZeroGameplayAbility_ReloadMagazine
                    └── UEqZeroGameplayAbility_ADS

UAttributeSet
    └── UEqZeroAttributeSet
            ├── UEqZeroHealthSet (Health, MaxHealth, Healing, Damage)
            └── UEqZeroCombatSet (BaseDamage, BaseHeal)

UObject
    └── UEqZeroAbilityCost (Abstract)
            ├── UEqZeroAbilityCost_ItemTagStack   (弹药)
            ├── UEqZeroAbilityCost_PlayerTagStack  (角色资源)
            └── UEqZeroAbilityCost_InventoryItem   (道具)

FGameplayEffectContext
    └── FEqZeroGameplayEffectContext (+CartridgeID, +AbilitySourceObject)

UPrimaryDataAsset
    ├── UEqZeroAbilitySet (GA/GE/AS 打包)
    ├── UEqZeroPawnData (角色配置)
    └── UEqZeroExperienceDefinition (体验定义)

UDataAsset
    └── UEqZeroAbilityTagRelationshipMapping (标签关系映射)

UWorldSubsystem
    └── UEqZeroGlobalAbilitySystem (全局技能/效果管理)

FFastArraySerializer
    └── FGameplayTagStackContainer (标签计数器)
```

---

## 附录：关键设计模式速查

| 模式 | 位置 | 说明 |
|------|------|------|
| Data-Driven AbilitySet | AbilitySet | GA/GE/AS 打包为 DataAsset，三级授予 |
| Tag-Based Input | ASC + InputComponent | 用 GameplayTag 替代 InputID |
| Meta Attribute | HealthSet.Damage/Healing | 临时传递属性，Post 回调中转化 |
| Dual AttributeSet | HealthSet + CombatSet | Source/Target 分离 |
| Plugin Cost | AbilityCost 子类 | EditInlineNew 可插拔消耗 |
| Tag Relationship | TagRelationshipMapping | 集中式技能关系管理 |
| Definition-Instance | Equipment / Inventory | 数据定义与运行时实例分离 |
| CartridgeID Dedup | GameplayEffectContext | 散弹去重 |
| AbilitySource Interface | WeaponInstance | 武器特定衰减曲线 |
| Global Effect | GlobalAbilitySystem | WorldSubsystem 全场效果 |
| InitState Machine | PawnExtension + Hero | 协作式组件初始化 |
| DynamicTag GE | ASC | 运行时动态 Tag 管理 |
| ActivationGroup | GA | 死亡阻塞其他技能 |
| FFastArraySerializer | Equipment / TagStack | 增量网络复制 |
