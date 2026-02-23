# ASC 技能触发流程总结

> UEqZeroAbilitySystemComponent 的技能触发设计，从增强输入到技能激活的完整链路、OnSpawn 触发机制、ActivationGroup 互斥管理，以及 Tag-Based 输入相比原生 InputID 的优势。

---

## 一、技能的三种激活策略（ActivationPolicy）

```cpp
enum class EEqZeroAbilityActivationPolicy : uint8
{
    OnInputTriggered,   // 按下输入时激活（离散，如开火/跳跃）
    WhileInputActive,   // 输入持续期间反复尝试激活（持续，如蓄力/冲刺）
    OnSpawn             // Pawn Avatar 就绪时自动激活（如死亡监听/被动 buff）
};
```

这三种策略覆盖了所有技能的触发时机，由 GA 基类 `UEqZeroGameplayAbility` 的 `ActivationPolicy` 属性配置，**策划在编辑器设置，代码零修改**。

---

## 二、增强输入到技能激活的完整链路

### 2.1 数据层：InputConfig 把 InputAction 和 InputTag 绑定

```
UEqZeroInputConfig (DataAsset)
 ├─ NativeInputActions[]   → Move, Look, Crouch 等固定操作
 └─ AbilityInputActions[]  → 每条是 { UInputAction, FGameplayTag }
                              例如: { IA_Fire, InputTag.Ability.Fire }
                              例如: { IA_Jump, InputTag.Ability.Jump }
```

**关键**：InputConfig 不直接引用任何 GA 类——它只定义"这个按键对应哪个 InputTag"。

### 2.2 绑定层：HeroComponent 在 InitState 数据就绪后绑定输入

```
HandleChangeInitState (DataAvailable → DataInitialized)
 └─ InitializePlayerInput(InputComponent)
     ├─ AddMappingContext (IMC → EnhancedInput 子系统)
     ├─ BindNativeAction (Move/Look/Crouch → 固定函数)
     └─ BindAbilityActions (所有 AbilityInputActions)
         → 对每个 { InputAction, InputTag }:
             Triggered → Input_AbilityInputTagPressed(InputTag)
             Completed → Input_AbilityInputTagReleased(InputTag)
```

`UEqZeroInputComponent::BindAbilityActions` 遍历 InputConfig 的 `AbilityInputActions`，为每个 InputAction 绑定 Triggered/Completed 事件，回调参数是 **InputTag**（不是 GA 类）。

### 2.3 路由层：HeroComponent → ASC 的 Tag 缓冲

```cpp
// HeroComponent 收到输入回调
void Input_AbilityInputTagPressed(FGameplayTag InputTag) {
    EqZeroASC->AbilityInputTagPressed(InputTag);  // 转发给 ASC
}
```

ASC 收到 InputTag 后，**不立即激活技能**，而是写入三个缓冲数组：

```cpp
void AbilityInputTagPressed(const FGameplayTag& InputTag) {
    // 遍历所有已授予的 GA，找 DynamicSpecSourceTags 匹配此 InputTag 的
    for (AbilitySpec : ActivatableAbilities.Items) {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) {
            InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);  // 按下队列
            InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);     // 持续队列
        }
    }
}

void AbilityInputTagReleased(const FGameplayTag& InputTag) {
    for (AbilitySpec : ActivatableAbilities.Items) {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) {
            InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle); // 释放队列
            InputHeldSpecHandles.Remove(AbilitySpec.Handle);        // 移出持续队列
        }
    }
}
```

**InputTag → GA 的匹配机制**：AbilitySet 在 `GiveAbility` 时，将 InputTag 写入 `AbilitySpec.DynamicAbilityTags`。ASC 通过 `GetDynamicSpecSourceTags().HasTagExact(InputTag)` 查找匹配的 GA。

### 2.4 处理层：PostProcessInput 统一消费

```
PlayerController::PostProcessInput (每帧 Tick)
 └─ ASC::ProcessAbilityInput(DeltaTime, bGamePaused)
```

`ProcessAbilityInput` 是整个输入→技能的核心调度，一帧内按严格顺序处理：

```
① 检查全局阻塞 — HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked)
   → 如果阻塞，ClearAbilityInput() 丢弃所有输入，直接返回

② 处理 WhileInputActive（持续按住的技能）
   → 遍历 InputHeldSpecHandles
   → 技能未激活 && ActivationPolicy == WhileInputActive → 加入待激活列表

③ 处理 OnInputTriggered（按下触发的技能）
   → 遍历 InputPressedSpecHandles
   → 技能已激活 → AbilitySpecInputPressed (传递输入事件给运行中的技能)
   → 技能未激活 && ActivationPolicy == OnInputTriggered → 加入待激活列表

④ 统一激活
   → 遍历待激活列表，TryActivateAbility(Handle)
   → 之所以统一激活，是避免 WhileInputActive 激活后又被 OnInputTriggered 发送事件

⑤ 处理 InputReleased（松开的技能）
   → 遍历 InputReleasedSpecHandles
   → 技能已激活 → AbilitySpecInputReleased (传递释放事件给运行中的技能)
   → InputPressed = false

⑥ 清空 Pressed/Released 缓冲（Held 保留到下一帧）
```

### 2.5 完整时序图

```
[帧 N - 输入采集阶段]
  EnhancedInput 检测 IA_Fire Triggered
    → UEqZeroInputComponent 回调
      → HeroComponent::Input_AbilityInputTagPressed(InputTag.Ability.Fire)
        → ASC::AbilityInputTagPressed(InputTag.Ability.Fire)
          → InputPressedSpecHandles += GA_RangedWeapon.Handle
          → InputHeldSpecHandles   += GA_RangedWeapon.Handle

[帧 N - Tick 阶段]
  PlayerController::PostProcessInput
    → ASC::ProcessAbilityInput
      → WhileInputActive: GA_RangedWeapon 不是此策略，跳过
      → OnInputTriggered: GA_RangedWeapon 匹配，加入待激活
      → TryActivateAbility(GA_RangedWeapon.Handle)
        → 引擎创建 PredictionKey → ActivateAbility → 预测管线启动

[帧 N+K - 输入采集阶段]
  EnhancedInput 检测 IA_Fire Completed
    → HeroComponent::Input_AbilityInputTagReleased(InputTag.Ability.Fire)
      → ASC::AbilityInputTagReleased(InputTag.Ability.Fire)
        → InputReleasedSpecHandles += GA_RangedWeapon.Handle
        → InputHeldSpecHandles     -= GA_RangedWeapon.Handle

[帧 N+K - Tick 阶段]
  PlayerController::PostProcessInput
    → ASC::ProcessAbilityInput
      → InputReleased: GA_RangedWeapon 激活中 → AbilitySpecInputReleased
        → InvokeReplicatedEvent(InputReleased, ...) → WaitInputRelease Task 可响应
```

---

## 三、OnSpawn 自动激活

### 3.1 两个触发时机

OnSpawn 策略的 GA 在两个时机尝试激活：

```
① OnGiveAbility — GA 被授予时
   UEqZeroGameplayAbility::OnGiveAbility
     → TryActivateAbilityOnSpawn(ActorInfo, Spec)

② InitAbilityActorInfo — Avatar 切换时（重生）
   UEqZeroAbilitySystemComponent::InitAbilityActorInfo
     → bHasNewPawnAvatar == true
       → TryActivateAbilitiesOnSpawn()  // 遍历所有已授予 GA
         → 每个 GA 的 TryActivateAbilityOnSpawn
```

**为什么需要两个时机**：
- **时机①**：正常流程——先有 Avatar，后授予技能（Equipment 装备时、GameFeature 注入时）
- **时机②**：ASC 在 PlayerState 上 + 重生流程——技能早就授予了（上一条命），但 Avatar 是新的 Pawn

### 3.2 激活条件

```cpp
void TryActivateAbilityOnSpawn(ActorInfo, Spec) const {
    if (ActivationPolicy != OnSpawn) return;
    if (Spec.IsActive()) return;           // 已经在运行了
    if (AvatarActor->GetTearOff()) return; // 正在销毁
    if (AvatarActor->GetLifeSpan() > 0) return; // 即将销毁

    // 根据 NetExecutionPolicy 决定谁来激活
    bool bLocalShouldActivate  = IsLocallyControlled() && (LocalPredicted || LocalOnly);
    bool bServerShouldActivate = IsNetAuthority()       && (ServerOnly || ServerInitiated);

    if (bLocalShouldActivate || bServerShouldActivate)
        ASC->TryActivateAbility(Spec.Handle);
}
```

**要点**：
- `Spec.IsActive()` 防止重复激活（重生时已有 GA 可能还在运行）
- `GetTearOff()` / `GetLifeSpan()` 过滤正在销毁的 Avatar
- 网络角色检查确保只在正确端激活

### 3.3 典型用例

| GA | 用途 | NetExecutionPolicy |
|----|------|-------------------|
| GA_Death | 监听 Health ≤ 0 → 触发死亡 | ServerOnly |
| GA_PassiveBuff | 常驻被动效果 | ServerOnly |
| GA_ListenForDamage | 受击反馈 UI | LocalOnly |

---

## 四、ActivationGroup — 技能互斥管理

### 4.1 三种组

```cpp
enum class EEqZeroAbilityActivationGroup : uint8 {
    Independent,           // 独立，不受任何约束（默认，绝大多数技能）
    Exclusive_Replaceable, // 排他可替换，会被新的排他技能取消
    Exclusive_Blocking     // 排他阻塞，阻止其他排他技能激活
};
```

### 4.2 运作流程

```
技能激活 → NotifyAbilityActivated
  → AddAbilityToActivationGroup(Group, Ability)
    ├─ Independent: 无操作
    └─ Exclusive_Replaceable / Exclusive_Blocking:
        → CancelActivationGroupAbilities(Exclusive_Replaceable, 排除自身)
        → 也就是：新的排他技能进来，取消所有"可替换"的排他技能

技能结束 → NotifyAbilityEnded
  → RemoveAbilityFromActivationGroup(Group, Ability)
    → ActivationGroupCounts[Group]--
```

**ActivationGroupCounts** 是一个计数器数组，`IsActivationGroupBlocked` 通过检查 Exclusive_Blocking 计数 > 0 来阻塞：

```cpp
bool IsActivationGroupBlocked(Group) const {
    if (Group == Independent) return false;  // 独立永远不被阻塞
    // Replaceable 和 Blocking 都在有 Blocking 技能时被阻塞
    return ActivationGroupCounts[Exclusive_Blocking] > 0;
}
```

### 4.3 项目中的实际用法

> 项目中绝大多数技能是 **Independent**（开火、换弹、跳跃、冲刺等）。
> 唯一使用 **Exclusive_Blocking** 的是 **GA_Death**：角色死亡时独占，阻止并取消所有 Exclusive_Replaceable 技能。
> 这意味着：死亡时不能开火、不能换弹，但 Independent 技能（如 UI 监听）不受影响。

---

## 五、AbilitySpecInputPressed/Released — 运行中技能的输入转发

对于**已经激活**的技能，输入事件不是重新激活，而是透传：

```cpp
void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) {
    Super::AbilitySpecInputPressed(Spec);
    // 不使用 bReplicateInputDirectly（原生方案），改用 ReplicatedEvent
    InvokeReplicatedEvent(InputPressed, Spec.Handle, PredictionKey);
}
```

**为什么用 ReplicatedEvent 而不是 bReplicateInputDirectly**：
- `bReplicateInputDirectly` 每帧复制 InputPressed 状态 → 带宽浪费
- `InvokeReplicatedEvent` 是事件驱动 → 只在按下/释放时发一次
- AbilityTask（如 `WaitInputPress`、`WaitInputRelease`）监听的就是这个事件

---

## 六、TAG_Gameplay_AbilityInputBlocked — 全局输入阻塞

```cpp
void ProcessAbilityInput(float DeltaTime, bool bGamePaused) {
    if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked)) {
        ClearAbilityInput();
        return;
    }
    // ... 正常处理
}
```

任何时候给 ASC 加上 `Gameplay.AbilityInputBlocked` 标签，所有输入驱动的技能激活都被抑制。清除标签即恢复。

**用途**：暖场倒计时、过场动画、菜单打开时禁止操作。

---

## 七、设计优势总结

### 7.1 为什么用 InputTag 而不是原生 InputID

| 维度 | 原生 InputID (int32) | EqZero InputTag |
|------|---------------------|-----------------|
| **语义** | `InputID=3` → 必须查代码才知道是什么 | `InputTag.Ability.Fire` → 自描述 |
| **灵活性** | 编译期绑死 GA 和 InputID | 运行时通过 DynamicAbilityTags 匹配 |
| **装备切换** | 换武器要重新绑 InputID | 换武器 = 收回旧 AbilitySet + 授予新 AbilitySet，InputTag 自动匹配新 GA |
| **多 GA 同 Input** | 一个 InputID 只绑一个 GA | 一个 InputTag 可匹配多个 GA（虽然通常一对一） |
| **GameFeature** | 难以模块化注入 | GF 可以通过 InputConfig 注入新的 InputTag→GA 绑定 |
| **数据驱动** | 硬编码 | InputConfig DataAsset 配置，策划可调 |

### 7.2 为什么 ProcessAbilityInput 放在 PostProcessInput

```
PlayerController Tick:
  PreProcessInput  → 引擎处理输入前的钩子
  ProcessPlayerInput → EnhancedInput 处理输入 → 触发 Triggered/Completed 回调
                       → AbilityInputTagPressed/Released 写入缓冲
  PostProcessInput → ASC::ProcessAbilityInput 统一消费缓冲
```

所有输入在同一帧内 **先收集、后处理**：
1. 避免输入处理顺序影响结果（先收到 Fire 还是先收到 Reload 不影响）
2. WhileInputActive 和 OnInputTriggered 统一排序后激活，防止冲突
3. 释放事件在激活之后处理，确保运行中的技能先收到输入事件

### 7.3 为什么 OnSpawn 有两个触发点

| 场景 | 先发生什么 | 触发点 |
|------|-----------|--------|
| 首次进入游戏 | ASC 初始化 → 装备加载 → GiveAbility | `OnGiveAbility` |
| 重生 | GA 已在 ASC 上（上一条命授予的） → 新 Pawn 创建 | `InitAbilityActorInfo` |
| 切换装备 | 收回旧 AbilitySet → 授予新 AbilitySet | `OnGiveAbility` |

两个入口确保**无论技能和 Avatar 谁先就绪**，OnSpawn 技能都能及时激活。

### 7.4 整体架构的解耦层次

```
Layer 1: 硬件输入 → Enhanced Input System (UE 原生)
Layer 2: InputAction → InputTag (EqZeroInputConfig, DataAsset)
Layer 3: InputTag → ASC 缓冲 (HeroComponent 转发, ASC 缓存)
Layer 4: 缓冲 → TryActivateAbility (ProcessAbilityInput, 每帧统一调度)
Layer 5: GA 内部 → ActivationPolicy 决定响应方式
```

每层只依赖相邻层的数据/接口，互不感知内部实现：
- InputConfig 不知道有哪些 GA
- HeroComponent 不知道输入会触发什么技能
- ASC 不知道输入来自键盘还是手柄
- GA 不知道自己是被输入还是被 OnSpawn 激活的

---

## 八、ProcessAbilityInput 的设计优势深入分析

### 8.1 缓冲-批处理模式的好处

传统做法是在 InputAction 回调里**直接**调 `TryActivateAbility`。ProcessAbilityInput 改为**缓冲→批处理**，带来以下优势：

| 优势 | 说明 |
|------|------|
| **消除输入顺序依赖** | 同一帧内 Fire 和 Reload 同时按下，不管谁先触发 EnhancedInput 回调，都进入缓冲。ProcessAbilityInput 统一决策，不会因为回调顺序不同导致不同结果 |
| **防止同帧重复激活** | WhileInputActive 写了 Held 缓冲，OnInputTriggered 写了 Pressed 缓冲。两者合并去重后统一调 TryActivateAbility，避免一帧内同一技能被 Held+Pressed 激活两次 |
| **输入事件传递的正确时序** | 先执行激活（③④步），再处理 Released（⑤步）。如果 Released 先处理了的，技能可能在收到 Released 后结束，后续 Pressed 又激活，导致一帧闪烁 |
| **全局阻塞一处检查** | 只需在 ProcessAbilityInput 入口检查 `TAG_Gameplay_AbilityInputBlocked`，不必在每个输入回调里都判断 |
| **纯本地计算，无网络开销** | 三个缓冲数组不复制。输入只在本地消费，TryActivateAbility 内部才走 GAS 的预测/RPC 流程 |
| **支持帧内一致性** | 同一帧按下 Fire + 松开 Dash，ProcessAbilityInput 保证 Fire 激活和 Dash 释放事件都在同一帧内完成。如果分散在回调中处理，两者可能跨帧 |

### 8.2 与 Enhanced Input 的 Triggered 事件配合

Enhanced Input 的 `ETriggerEvent::Triggered` 在按住期间**每帧**都会触发（不是只触发一次）。这意味着：

```
[帧 1] 按下开火键 → EnhancedInput: Triggered → AbilityInputTagPressed
[帧 2] 继续按住   → EnhancedInput: Triggered → AbilityInputTagPressed (再次)
[帧 3] 继续按住   → EnhancedInput: Triggered → AbilityInputTagPressed (再次)
[帧 N] 松开       → EnhancedInput: Completed  → AbilityInputTagReleased
```

每帧 Triggered 都把 Handle 加入 `InputPressedSpecHandles`（AddUnique 防重复）。

ProcessAbilityInput 每帧检查：
- 技能**未激活** → TryActivateAbility（新的一发射击）
- 技能**已激活** → AbilitySpecInputPressed（事件传递给运行中的技能）
- 帧末清空 `InputPressedSpecHandles`

这形成了**"每帧都有一次激活机会"**的节奏——只要上一发结束了，下一帧立刻再激活。

---

## 九、持续射击的技能生命周期

### 9.1 GA_RangedWeapon 的一发一激活模式

GA_RangedWeapon 使用 `OnInputTriggered` 策略，**每次激活 = 一发射击**。持续射击不是一次激活内循环，而是**快速重复的 激活→结束→再激活**。

### 9.2 单发射击的完整生命周期

```
TryActivateAbility(Handle)
  → ActivateAbility
    ├─ 绑定 TargetData 回调
    ├─ 更新武器 FiringTime
    └─ Super::ActivateAbility → 蓝图接管

蓝图:
  → StartRangedWeaponTargeting()
    ├─ PerformLocalTargeting → 射线检测
    └─ 构建 TargetData → 触发 OnTargetDataReadyCallback

OnTargetDataReadyCallback:
  ├─ 客户端发 TargetData RPC
  ├─ 服务器确认并回传
  ├─ CommitAbility (扣弹药/CD)
  │   ├─ 成功 → OnRangedWeaponTargetDataReady (蓝图实现，应用伤害 GE)
  │   └─ 失败 → K2_EndAbility()  ← 弹药耗尽，技能结束
  └─ ConsumeClientReplicatedTargetData

蓝图处理完后:
  → K2_EndAbility()  ← 正常结束一发
```

### 9.3 持续射击的帧级时序

```
[帧 1] Triggered → Pressed缓冲 → ProcessAbilityInput → TryActivateAbility → 射击开始
[帧 1] 射击流程执行 → TargetData → CommitAbility → 蓝图处理 → EndAbility（一发结束）

[帧 2] Triggered(还在按) → Pressed缓冲 → ProcessAbilityInput → 技能未激活 → TryActivateAbility → 第二发
[帧 2] ... 同上 → EndAbility

[帧 3] Triggered → ... → TryActivateAbility → 第三发

[帧 N] CommitAbility 失败(没子弹了) → K2_EndAbility
[帧 N+1] Triggered → Pressed缓冲 → ProcessAbilityInput → TryActivateAbility
   → CanActivateAbility → CheckCost → false (无弹药) → 激活失败 → 不再射击
```

### 9.4 技能的五种结束时机

| 时机 | 触发条件 | 结束方式 |
|------|---------|---------|
| **正常完成** | 一发射击流程跑完 | 蓝图在 OnRangedWeaponTargetDataReady 处理完后调 `EndAbility` |
| **弹药耗尽** | CommitAbility 中 CheckCost 失败 | OnTargetDataReadyCallback 内调 `K2_EndAbility()` |
| **外部取消** | ActivationGroup 互斥（如死亡） | `CancelAbility` → EndAbility(bWasCancelled=true) |
| **TAG 阻塞** | 下一帧输入被 `AbilityInputBlocked` 拦截 | 技能自然结束后，不再被重新激活 |
| **输入释放** | 玩家松开开火键 | EnhancedInput 不再触发 Triggered → Pressed 缓冲为空 → 技能结束后不被重新激活 |

### 9.5 为什么"一发一激活"而不是"一次激活内循环"

| 维度 | 一发一激活 | 一次激活内循环 |
|------|-----------|--------------|
| **预测** | 每发都有独立 PredictionKey → 独立的 Undo/Redo | 只有一个 PredictionKey → 回滚一发就回滚整个连射 |
| **弹药检查** | 每发 CommitAbility → 精确到每一发的消耗检查 | 循环内手动检查 → 容易遗漏 |
| **中断响应** | 帧间有"间隙"可以插入取消/切换 | 循环内需要手动检查取消标记 |
| **射速控制** | 依赖 Enhanced Input 的 Triggered 频率 + CanActivateAbility | 需要自己维护 Timer/Cooldown |
| **网络安全** | 每发通过 TryActivateAbility 走完整授权流程 | 循环内的后续射击可能绕过服务器验证 |
| **代码简洁** | GA 只管"一发"，循环由输入系统驱动 | GA 内部要管循环、退出条件、边界情况 |

**本质上，ProcessAbilityInput 的缓冲-批处理模式 + Enhanced Input 的每帧 Triggered，天然构成了一个"无需手写循环"的连射机制。**

射速控制通过 `CanActivateAbility` 里的冷却 GE 或武器的 FireRate 检查实现——如果上一发的冷却还没结束，TryActivateAbility 自然失败，跳过这一帧，下一帧再试。

---

*基于 UEqZeroAbilitySystemComponent / UEqZeroHeroComponent / UEqZeroInputComponent / UEqZeroGameplayAbility_RangedWeapon 源码分析*
