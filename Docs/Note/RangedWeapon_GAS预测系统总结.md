# UEqZeroGameplayAbility_RangedWeapon — GAS 预测射击系统总结

> 基于 `UEqZeroGameplayAbility_RangedWeapon` 及关联组件的源码分析，以 **GAS Prediction（预测）** 为核心视角，系统性梳理远程武器开火技能的客户端/服务器协作逻辑。

---

## 一、整体架构概览

```
继承链:
UGameplayAbility
  └─ UEqZeroGameplayAbility (项目基类)
       └─ UEqZeroGameplayAbility_FromEquipment (装备技能基类, SourceObject → 装备实例)
            └─ UEqZeroGameplayAbility_RangedWeapon (远程武器开火技能)

关联类:
├── UEqZeroRangedWeaponInstance      — 远程武器数据实例 (扩散/热度/衰减)
├── UEqZeroWeaponInstance            — 武器基类 (动画层/设备属性/使用时间)
├── UEqZeroWeaponStateComponent      — Controller 上的武器状态组件 (命中确认/UI反馈)
├── FEqZeroGameplayAbilityTargetData_SingleTargetHit — 扩展 TargetData (CartridgeID)
└── UAbilitySystemComponent (ASC)    — GAS 核心，管理预测 Key、TargetData 复制
```

---

## 二、GAS 预测的核心概念

### 2.1 什么是 GAS 预测？

在多人游戏中，客户端到服务器有网络延迟。GAS 预测机制允许：

1. **客户端先行执行**：客户端不等服务器确认就先执行技能效果（播特效、扣弹药等）
2. **服务器权威验证**：服务器收到数据后做最终判定
3. **回滚/确认**：如果服务器否决，客户端回滚预测的效果；如果确认，则保持

### 2.2 关键 GAS 基础设施

| 概念 | 说明 |
|------|------|
| **Prediction Key** | 每次技能激活分配唯一 Key，用于匹配客户端预测与服务器确认 |
| **FScopedPredictionWindow** | RAII 作用域对象，标记一段代码在预测窗口内运行 |
| **CallServerSetReplicatedTargetData** | 客户端→服务器 RPC，将本地命中数据发送给服务器 |
| **AbilityTargetDataSetDelegate** | ASC 上基于 (SpecHandle, PredictionKey) 注册的委托，TargetData 就绪时触发 |
| **ConsumeClientReplicatedTargetData** | 消耗已处理的 TargetData，防止重复使用 |

---

## 三、完整开火流程（时序图）

```
客户端                                              服务器
  │                                                   │
  │  1. ActivateAbility()                              │
  │     ├─ 注册 AbilityTargetDataSetDelegate           │
  │     ├─ 更新 WeaponData->UpdateFiringTime()         │
  │     └─ 调用蓝图 (Super 触发蓝图逻辑)               │
  │                                                   │
  │  2. 蓝图调用 StartRangedWeaponTargeting()          │
  │     ├─ FScopedPredictionWindow 开启预测窗口         │
  │     ├─ PerformLocalTargeting() → 本地射线检测       │
  │     │   ├─ GetTargetingTransform (摄像机→焦点)      │
  │     │   └─ TraceBulletsInCartridge                  │
  │     │       └─ DoSingleBulletTrace × N颗子弹       │
  │     │           ├─ 线检测 (SweepRadius=0)           │
  │     │           └─ 容错检测 (SweepRadius>0)         │
  │     ├─ 构建 FGameplayAbilityTargetDataHandle       │
  │     ├─ WeaponStateComponent                         │
  │     │   ->AddUnconfirmedServerSideHitMarkers()     │
  │     │   (本地暂存命中, UI 先表现)                    │
  │     └─ 调用 OnTargetDataReadyCallback()             │
  │                                                   │
  │  3. OnTargetDataReadyCallback() [客户端]            │
  │     ├─ FScopedPredictionWindow                      │
  │     ├─ CallServerSetReplicatedTargetData ─────────►│ 4. 服务器收到 TargetData
  │     │   (RPC: 发送命中数据到服务器)                  │    ├─ AbilityTargetDataSetDelegate 触发
  │     ├─ CommitAbility (预测性扣弹药/CD)              │    │   OnTargetDataReadyCallback() [服务器]
  │     ├─ WeaponData->AddSpread()                     │    ├─ 服务器验证命中有效性
  │     └─ OnRangedWeaponTargetDataReady               │    ├─ WeaponStateComponent
  │         (蓝图: 播特效/应用GE伤害)                   │    │   ->ClientConfirmTargetData ──────────►
  │                                                   │    │   (Client RPC: 通知客户端确认结果)
  │  6. ClientConfirmTargetData_Implementation         │    ├─ CommitAbility (权威扣弹药/CD)
  │     ├─ 查找 UniqueId 匹配的未确认批次               │    ├─ WeaponData->AddSpread()
  │     ├─ 根据 HitReplaces 过滤无效命中               │    └─ OnRangedWeaponTargetDataReady
  │     ├─ 有效命中 → ActuallyUpdateDamageInstigatedTime│       (蓝图: 权威应用GE伤害)
  │     ├─ 加入 LastWeaponDamageScreenLocations        │
  │     └─ UI 显示确认后的命中标记 (如爆头变色)          │
  │                                                   │
  │  7. EndAbility()                                   │
  │     ├─ 移除 AbilityTargetDataSetDelegate           │
  │     └─ ConsumeClientReplicatedTargetData           │
```

---

## 四、客户端关键代码路径

### 4.1 ActivateAbility — 入口

```cpp
void ActivateAbility(...)
{
    // 1. 注册 TargetData 回调 (Key = PredictionKey)
    OnTargetDataReadyCallbackDelegateHandle = 
        MyAbilityComponent->AbilityTargetDataSetDelegate(
            CurrentSpecHandle, 
            CurrentActivationInfo.GetActivationPredictionKey()
        ).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

    // 2. 更新武器开火时间 (影响扩散恢复)
    WeaponData->UpdateFiringTime();

    // 3. Super → 蓝图 ActivateAbility
    Super::ActivateAbility(...);
}
```

**要点**：注册回调时使用的 `GetActivationPredictionKey()` 就是 GAS 预测的核心——客户端和服务器通过同一个 Key 来匹配同一次开火事件。

### 4.2 StartRangedWeaponTargeting — 蓝图调用的核心函数

```cpp
void StartRangedWeaponTargeting()
{
    // ★ 开启预测窗口
    FScopedPredictionWindow ScopedPrediction(
        MyAbilityComponent, 
        CurrentActivationInfo.GetActivationPredictionKey()
    );

    // 本地射线检测
    TArray<FHitResult> FoundHits;
    PerformLocalTargeting(FoundHits);

    // 构建 GAS TargetData
    FGameplayAbilityTargetDataHandle TargetData;
    TargetData.UniqueId = WeaponStateComponent->GetUnconfirmedServerSideHitMarkerCount();
    for (const FHitResult& Hit : FoundHits)
    {
        auto* NewData = new FEqZeroGameplayAbilityTargetData_SingleTargetHit();
        NewData->HitResult = Hit;
        NewData->CartridgeID = FMath::Rand(); // 同一发弹药共享
        TargetData.Add(NewData);
    }

    // 本地预存命中 (UI 即时反馈)
    WeaponStateComponent->AddUnconfirmedServerSideHitMarkers(TargetData, FoundHits);

    // 立即触发回调 (客户端先行处理)
    OnTargetDataReadyCallback(TargetData, FGameplayTag());
}
```

### 4.3 OnTargetDataReadyCallback — 客户端 + 服务器共用

```cpp
void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
    // ★ 再次开启预测窗口
    FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);

    // ★ 客户端专有：将 TargetData RPC 到服务器
    if (IsLocallyControlled() && !IsNetAuthority())
    {
        MyAbilityComponent->CallServerSetReplicatedTargetData(
            CurrentSpecHandle,
            CurrentActivationInfo.GetActivationPredictionKey(),
            LocalTargetDataHandle,
            ApplicationTag,
            MyAbilityComponent->ScopedPredictionKey
        );
    }

    // ★ 服务器专有：验证命中并通知客户端
    #if WITH_SERVER_CODE
    if (Controller->GetLocalRole() == ROLE_Authority)
    {
        // 分析 HitReplaces (被替换的无效命中)
        WeaponStateComponent->ClientConfirmTargetData(
            UniqueId, bIsTargetDataValid, HitReplaces);
    }
    #endif

    // 客户端和服务器都执行：
    if (CommitAbility(...))  // 预测性消耗 (弹药/CD)
    {
        WeaponData->AddSpread();  // 增加武器热度/扩散
        OnRangedWeaponTargetDataReady(LocalTargetDataHandle); // 蓝图处理
    }
}
```

---

## 五、服务器关键代码路径

### 5.1 TargetData 到达服务器

当客户端调用 `CallServerSetReplicatedTargetData` 后，服务器的 ASC 会：
1. 将数据写入 `AbilityTargetDataMap`
2. 触发 `AbilityTargetDataSetDelegate`——即调用服务器侧的 `OnTargetDataReadyCallback`

### 5.2 服务器命中验证

```cpp
// WITH_SERVER_CODE 块
if (Controller->GetLocalRole() == ROLE_Authority)
{
    TArray<uint8> HitReplaces;
    for (uint8 i = 0; i < LocalTargetDataHandle.Num(); ++i)
    {
        if (SingleTargetHit->bHitReplaced)
        {
            HitReplaces.Add(i); // 标记被服务器否决的命中
        }
    }
    // Client RPC → 通知客户端
    WeaponStateComponent->ClientConfirmTargetData(UniqueId, bIsTargetDataValid, HitReplaces);
}
```

> **当前实现说明**：目前服务器直接信任客户端数据（`bIsTargetDataValid = true`），`bHitReplaced` 是预留的扩展点。在生产环境中，服务器应做反作弊验证（如检查射线起点合理性、距离校验等）。

### 5.3 CommitAbility 的双端执行

`CommitAbility` 在客户端和服务器都会执行：

- **客户端**：预测性扣除弹药/触发冷却，如果后续服务器否决，GAS 会自动回滚
- **服务器**：权威性扣除弹药/触发冷却，这是最终生效的版本

---

## 六、WeaponStateComponent 维护数据

`UEqZeroWeaponStateComponent` 挂在 `AController` 上，负责命中标记的生命周期管理。

### 6.1 核心数据结构

```cpp
// 单个命中标记 (屏幕空间)
struct FEqZeroScreenSpaceHitLocation {
    FVector2D Location;           // 屏幕坐标
    FGameplayTag HitZone;         // 受击部位 Tag (Gameplay.Zone.Head 等)
    bool bShowAsSuccess = false;  // 是否判定为有效命中 (命中敌人)
};

// 一次射击的命中批次
struct FEqZeroServerSideHitMarkerBatch {
    TArray<FEqZeroScreenSpaceHitLocation> Markers;  // 本次射击所有命中点
    uint8 UniqueId = 0;                              // 与 TargetData.UniqueId 匹配
};
```

### 6.2 维护的状态数据

| 字段 | 类型 | 说明 |
|------|------|------|
| `UnconfirmedServerSideHitMarkers` | `TArray<FEqZeroServerSideHitMarkerBatch>` | **等待服务器确认的命中批次队列**。客户端射击后立即加入，服务器确认后移除 |
| `LastWeaponDamageScreenLocations` | `TArray<FEqZeroScreenSpaceHitLocation>` | **已确认的命中位置列表**。UI Tick 中读取此列表绘制命中反馈。超过 0.1 秒后清空 |
| `LastWeaponDamageInstigatedTime` | `double` | **上次造成伤害的世界时间戳**。用于判断命中反馈是否过期 (>0.1s 则清空) |

### 6.3 状态流转

```
射击 → AddUnconfirmedServerSideHitMarkers()
         ↓ 命中数据暂存到 UnconfirmedServerSideHitMarkers
         ↓ (屏幕坐标 + 命中部位 + 是否有效命中)
         │
服务器确认 → ClientConfirmTargetData_Implementation()
         ↓ 按 UniqueId 查找匹配批次
         ↓ 排除 HitReplaces 中的无效命中
         ↓ 有效命中 → 写入 LastWeaponDamageScreenLocations
         ↓         → 更新 LastWeaponDamageInstigatedTime
         ↓ 移除该批次
         │
UI Tick → GetLastWeaponDamageScreenLocations()
         ↓ 读取已确认命中列表进行渲染
         ↓ 0.1s 后自动清空
```

### 6.4 Tick 逻辑

```cpp
void TickComponent(float DeltaTime, ...)
{
    // 找到当前装备的远程武器实例, 调用其 Tick
    // 武器 Tick 更新: 热度冷却、扩散恢复、各状态乘数平滑过渡
    CurrentWeapon->Tick(DeltaTime);
}
```

---

## 七、RangedWeaponInstance 武器状态数据

`UEqZeroRangedWeaponInstance` 维护的运行时状态直接影响射击精度：

| 字段 | 说明 |
|------|------|
| `CurrentHeat` | 当前武器热度。开火增加（`AddSpread()`），冷却减少（`UpdateSpread()`） |
| `CurrentSpreadAngle` | 当前扩散角（度）。由 `HeatToSpreadCurve` 映射自 `CurrentHeat` |
| `CurrentSpreadAngleMultiplier` | 组合倍率 = 瞄准 × 静止 × 蹲伏 × 跳跃 |
| `bHasFirstShotAccuracy` | 首发精准。当所有倍率和扩散都在最小值时为 true（扩散倍率视为 0） |
| `StandingStillMultiplier` | 静止状态倍率。速度 < 阈值时趋向最小值（手枪 0.9） |
| `CrouchingMultiplier` | 蹲下倍率（手枪 0.65） |
| `JumpFallMultiplier` | 跳跃/下落倍率（手枪 1.25，更不准） |
| `LastFireTime` | 上次开火时间。影响扩散恢复延迟判断 |

---

## 八、实现基于预测射击的关键步骤清单

如果你要在此基础上实现或扩展预测逻辑，以下是必须理解和实现的关键点：

### 8.1 客户端侧

1. **ActivateAbility 时注册 TargetData 委托**
   ```cpp
   ASC->AbilityTargetDataSetDelegate(SpecHandle, PredictionKey).AddUObject(...)
   ```

2. **在预测窗口内执行本地检测**
   ```cpp
   FScopedPredictionWindow ScopedPrediction(ASC, PredictionKey);
   // ... 射线检测 ...
   ```

3. **构建 TargetData 并本地预缓存**
   - `FGameplayAbilityTargetDataHandle` 包装命中结果
   - `WeaponStateComponent->AddUnconfirmedServerSideHitMarkers()` 预存 UI 数据

4. **将 TargetData 发送到服务器**
   ```cpp
   ASC->CallServerSetReplicatedTargetData(SpecHandle, PredictionKey, TargetData, Tag, ScopedKey);
   ```

5. **本地预测执行效果**
   - `CommitAbility()` 预测性消耗
   - `OnRangedWeaponTargetDataReady()` 蓝图播放特效

6. **EndAbility 时清理**
   - 移除委托 + `ConsumeClientReplicatedTargetData()`

### 8.2 服务器侧

1. **ASC 自动调度 TargetData 到委托**
   - 服务器收到 `CallServerSetReplicatedTargetData` 后，ASC 内部触发相同的 `OnTargetDataReadyCallback`

2. **验证命中合法性**（当前实现信任客户端，生产环境需加强）
   - 检查射线起点合理性
   - 验证命中距离
   - 反作弊过滤

3. **通过 Client RPC 通知客户端结果**
   ```cpp
   WeaponStateComponent->ClientConfirmTargetData(UniqueId, bSuccess, HitReplaces);
   ```

4. **权威执行 CommitAbility 和 GE 应用**

### 8.3 对齐机制

| 机制 | 客户端 | 服务器 | 对齐方式 |
|------|--------|--------|----------|
| 技能激活 | Prediction Key | 同一 Prediction Key | GAS 自动匹配 |
| TargetData | 本地生成 | 通过 RPC 接收 | `CallServerSetReplicatedTargetData` |
| 弹药消耗 | 预测性扣除 | 权威扣除 | `CommitAbility` + GAS 回滚机制 |
| GE 应用 | 预测应用 | 权威应用 | Prediction Key 匹配 + 回滚 |
| UI 命中标记 | 先画未确认 | 发 RPC 确认 | `UniqueId` 匹配 |

---

## 九、重要注意事项

### 9.1 FScopedPredictionWindow 出现了两次

- **第一次**：`StartRangedWeaponTargeting()` 中，用于包裹射线检测和 TargetData 构建
- **第二次**：`OnTargetDataReadyCallback()` 中，用于包裹 RPC 发送和效果执行

两次都使用同一个 Prediction Key，确保整个射击流程在同一个预测上下文中。

### 9.2 bHitReplaced 扩展点

`FEqZeroGameplayAbilityTargetData_SingleTargetHit` 继承自引擎的 `FGameplayAbilityTargetData_SingleTargetHit`，其中 `bHitReplaced` 是引擎提供的字段。当前代码中服务器直接遍历检查此字段，但未主动设置——这是**服务器端验证/回放（Server-Side Rewind）**的预留接口。

### 9.3 CartridgeID 的用途

同一次扣扳机（一个弹药壳）发射的所有子弹共享同一个 `CartridgeID`（通过 `FMath::Rand()` 生成）。这在伤害计算的 `GameplayEffectExecutionCalculation` 中用于：
- 避免散弹枪的多颗弹丸重复计算某些效果
- 在 `FEqZeroGameplayEffectContext` 中传递给下游系统

### 9.4 不是所有射线检测都在服务器重做

当前实现是**客户端权威命中检测**模式——服务器信任客户端射线检测结果。这是 Lyra 项目的默认设计：
- 优点：延迟体验好，玩家感觉"打得准" 
- 缺点：容易被外挂利用
- 如需服务器验证，需在 `OnTargetDataReadyCallback` 的 `WITH_SERVER_CODE` 块中添加回放逻辑

---

## 十、流程总结（一句话版）

> 客户端在预测窗口内执行射线检测 → 构建 TargetData → 本地预缓存 UI 命中标记 → 预测性执行效果 → RPC 发送到服务器 → 服务器验证并权威执行 → Client RPC 确认命中 → 客户端更新最终 UI 反馈。

---

---

## 附录：三个关键疑问深入解析

### Q1：Prediction Key 在哪里生成？

**答案：客户端在技能激活时由 ASC 自动生成，使用一个全局递增的 `int16` 计数器。**

引擎源码路径：`GameplayPrediction.cpp`

```cpp
// 生成新key的核心——一个 static 局部递增计数器
void FPredictionKey::GenerateNewPredictionKey()
{
    static KeyType GKey = 1;    // KeyType = int16
    Current = GKey++;
    if (GKey <= 0) { GKey = 1; }  // 溢出处理
}

// 只在客户端生成（服务器返回无效key）
FPredictionKey FPredictionKey::CreateNewPredictionKey(const UAbilitySystemComponent* OwningComponent)
{
    FPredictionKey NewKey;
    if (OwningComponent->GetOwnerRole() != ROLE_Authority)
    {
        NewKey.GenerateNewPredictionKey();
    }
    return NewKey;
}
```

**生成时机：** `InternalTryActivateAbility` 中，对于 `LocalPredicted` 策略的技能，客户端会：

```cpp
// AbilitySystemComponent_Abilities.cpp
else if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)
{
    // ★ 这里的 true 参数表示 "可以生成新Key"
    FScopedPredictionWindow ScopedPredictionWindow(this, true);

    // 将这个Key设为当前技能激活的预测Key
    ActivationInfo.SetPredicting(ScopedPredictionKey);

    // 立即发RPC到服务器，携带这个Key
    CallServerTryActivateAbility(Handle, Spec->InputPressed, ScopedPredictionKey);

    // 本地先激活（预测执行）
    AbilitySource->CallActivateAbility(Handle, ActorInfo, ActivationInfo, ...);
}
```

所以流程是：
1. 客户端 `TryActivateAbility` → `InternalTryActivateAbility`
2. `FScopedPredictionWindow(ASC, true)` 在构造时调用 `GenerateDependentPredictionKey()` → `GenerateNewPredictionKey()`
3. Key 被存入 `ASC->ScopedPredictionKey`，同时设入 `ActivationInfo`
4. 通过 `CallServerTryActivateAbility` RPC 将 Key 发送给服务器
5. 服务器收到后，用**同一个 Key** 设入自己的 `ActivationInfo`

之后我们的代码里用 `CurrentActivationInfo.GetActivationPredictionKey()` 获取的就是这个 Key。

---

### Q2：FScopedPredictionWindow 具体做了什么？

**答案：它是一个 RAII 作用域守卫，在构造时设置当前预测Key到ASC上，在析构时恢复旧Key并（服务器侧）确认该Key。**

它有两个构造函数，分别用于客户端和服务器：

#### 客户端构造函数 `FScopedPredictionWindow(ASC, bCanGenerateNewKey=true)`

```cpp
// 保存旧Key
RestoreKey = ASC->ScopedPredictionKey;

// 生成新的依赖Key（Base 指向旧Key，Current 是新值）
ASC->ScopedPredictionKey.GenerateDependentPredictionKey();
```

**作用：** 在这个作用域内，所有预测操作（如 `ApplyGameplayEffect`、`CallServerSetReplicatedTargetData`）都会使用新生成的 `ScopedPredictionKey`，让 GAS 知道"这些操作属于同一个预测上下文"。

#### 服务器构造函数 `FScopedPredictionWindow(ASC, InPredictionKey)`

```cpp
// 保存旧Key
RestoreKey = ASC->ScopedPredictionKey;
// 设置客户端传来的Key
ASC->ScopedPredictionKey = InPredictionKey;
```

#### 析构函数（关键！）

```cpp
~FScopedPredictionWindow()
{
    // ★ 服务器侧：确认这个PredictionKey，通知客户端"我处理了这个预测"
    if (SetReplicatedPredictionKey && ScopedPredictionKey.IsValidKey())
    {
        ASC->ReplicatedPredictionKeyMap.ReplicatePredictionKey(ScopedPredictionKey);
        // 这会通过 FastArray 复制到客户端，触发 OnRep
        // 客户端收到后知道"服务器已确认这个Key对应的操作"，从而保留预测结果
    }

    // 恢复之前的Key
    ASC->ScopedPredictionKey = RestoreKey;
}
```

**总结 FScopedPredictionWindow 的三个职责：**

| 职责 | 构造时 | 析构时 |
|------|--------|--------|
| 管理 Key 作用域 | 保存旧 Key，设置新 Key 到 ASC | 恢复旧 Key |
| 客户端：生成预测 Key | `GenerateDependentPredictionKey()` | - |
| 服务器：确认预测 Key | - | `ReplicatePredictionKey()` 告知客户端 |

**在我们代码里出现了两次：**

```cpp
// StartRangedWeaponTargeting() 里——客户端入口
FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, 
    CurrentActivationInfo.GetActivationPredictionKey());
// ↑ 这里用的是 "服务器版构造函数"（传入已有Key），不生成新Key
//   目的是把激活时的PredictionKey设为当前作用域的Key

// OnTargetDataReadyCallback() 里——客户端和服务器都走
FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);
// ↑ 客户端：bCanGenerateNewKey 默认 true，会生成一个依赖Key
//   服务器：IsNetSimulating()==false，会设置并在析构时确认Key
```

---

### Q3：UniqueId 为什么用 `GetUnconfirmedServerSideHitMarkerCount()`？不会被清掉后冲突吗？

**答案：严格来说确实存在极端情况下的碰撞风险，但在实际游戏节奏中基本不会出问题。这是 Lyra 的设计取舍。**

先看 `UniqueId` 到底是什么——它是 `FGameplayAbilityTargetDataHandle` 上的一个 `uint8` 字段：

```cpp
// 引擎源码 GameplayAbilityTargetTypes.h
struct FGameplayAbilityTargetDataHandle
{
    uint8 UniqueId = 0;  // ★ 引擎不使用这个字段，纯粹给游戏层自定义用
    TArray<TSharedPtr<FGameplayAbilityTargetData>> Data;
};
```

**引擎自身从不读取或校验这个值。** 它完全是给游戏代码用的"便签"。

#### 赋值逻辑追踪

```cpp
// StartRangedWeaponTargeting()
TargetData.UniqueId = WeaponStateComponent->GetUnconfirmedServerSideHitMarkerCount();
//                     ↑ 返回 UnconfirmedServerSideHitMarkers.Num()

// AddUnconfirmedServerSideHitMarkers()
UnconfirmedServerSideHitMarkers.Emplace_GetRef(InTargetData.UniqueId);  // 存入数组

// 服务器侧 OnTargetDataReadyCallback()
WeaponStateComponent->ClientConfirmTargetData(LocalTargetDataHandle.UniqueId, ...);
//                                            ↑ 透传给客户端

// ClientConfirmTargetData_Implementation()
if (Batch.UniqueId == UniqueId) { ... UnconfirmedServerSideHitMarkers.RemoveAt(i); }
//                                    ↑ 确认后删除
```

#### 关键追踪（模拟连续射击）

```
时间线:
Shot 1: Count=0 → UniqueId=0, 存入 → Array=[{id:0}]
Shot 2: Count=1 → UniqueId=1, 存入 → Array=[{id:0},{id:1}]
Server确认Shot1(id=0) → Array=[{id:1}]
Shot 3: Count=1 → UniqueId=1, 存入 → Array=[{id:1},{id:1}]  ← ★ 潜在碰撞！
```

**你的直觉是对的——如果确认速度和射击速度交错，UniqueId 会重复。**

#### 为什么实际上"基本没问题"

1. **搜索用 `break`**：`ClientConfirmTargetData_Implementation` 里 `for` 循环找到第一个匹配就 `break` 并 `RemoveAt`，所以即使有重复 id，也只处理最早的那个（FIFO 顺序），这恰好是正确的

2. **`uint8` 范围 0-255**：在正常开火频率下，未确认队列长度极小（通常 0-3 个），碰撞概率几乎为零

3. **这只影响 UI 命中标记**：即使碰撞了，最坏结果是某一帧的命中标记显示/不显示不对，不影响伤害计算等核心逻辑（那些走的是 GAS Prediction Key 路径）

4. **低延迟网络下**：确认 RPC 通常在下一帧或几帧内返回，队列几乎不会积累

#### 如果想更健壮

可以改用自递增 ID 替代 Count：

```cpp
// 示例改法
static uint8 NextHitMarkerBatchId = 0;
TargetData.UniqueId = NextHitMarkerBatchId++;
```

这样即使清掉旧批次，ID 也不会回退。但 Lyra 没这么做，说明 Epic 认为当前设计的 trade-off 是可接受的。

*文档生成时间：2026-02-22*  
*基于源码：EqZeroGameplayAbility_RangedWeapon.h/cpp, EqZeroWeaponStateComponent.h/cpp, EqZeroRangedWeaponInstance.h/cpp*  
*引擎源码：GameplayPrediction.h/cpp, AbilitySystemComponent_Abilities.cpp, GameplayAbilityTargetTypes.h*
