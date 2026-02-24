# NumberPops 跳字系统架构总结

> 源码路径: `Source/EqZeroGame/Feedback/NumberPops/`

---

## 一、概述

NumberPops（跳字系统）用于在世界空间中显示浮动数字，最常见的用途是**伤害数字弹出**（Damage Pop）。当角色受到伤害时，在受击位置附近弹出一个带动画的数字，让玩家直观看到造成的伤害值。

系统支持两种独立的渲染方案，可根据项目需求选择其一：
1. **MeshText 方案** — 基于 StaticMesh + 材质动态实例（MID），用 WPO 材质动画驱动数字显示
2. **NiagaraText 方案** — 基于 Niagara 粒子系统，用 Niagara 数据接口传递伤害数据

---

## 二、文件清单

| 文件 | 职责 |
|------|------|
| `EqNumberPopComponent.h/cpp` | 抽象基类 + 请求结构体 `FEqNumberPopRequest` |
| `EqDamagePopStyle.h/cpp` | MeshText 方案的样式数据资产（颜色 / Mesh / Tag匹配） |
| `EqDamagePopStyleNiagara.h` | Niagara 方案的样式数据资产（NiagaraSystem 引用） |
| `EqNumberPopComponent_MeshText.h/cpp` | MeshText 方案的具体实现（含对象池） |
| `EqNumberPopComponent_NiagaraText.h/cpp` | Niagara 方案的具体实现 |

---

## 三、类继承关系

```
UControllerComponent
  └── UEqNumberPopComponent              (Abstract 抽象基类)
        ├── UEqNumberPopComponent_MeshText      (Blueprintable, 基于StaticMesh)
        └── UEqNumberPopComponent_NiagaraText   (Blueprintable, 基于Niagara)

UDataAsset
  ├── UEqDamagePopStyle            (MeshText方案的样式)
  └── UEqDamagePopStyleNiagara     (Niagara方案的样式)
```

---

## 四、核心数据结构

### 4.1 FEqNumberPopRequest（弹出请求）

这是整个系统的入口参数，描述了"在哪里弹什么数字"。

```cpp
USTRUCT(BlueprintType)
struct FEqNumberPopRequest
{
    FVector WorldLocation;           // 弹出的世界位置
    FGameplayTagContainer SourceTags; // 来源标签（用于样式匹配）
    FGameplayTagContainer TargetTags; // 目标标签（用于样式匹配）
    int32 NumberToDisplay = 0;        // 要显示的数字
    bool bIsCriticalDamage = false;   // 是否暴击
};
```

### 4.2 UEqDamagePopStyle（MeshText 样式资产）

每个 Style 是一个 DataAsset，可在编辑器里创建多个并配置不同条件：

| 属性 | 说明 |
|------|------|
| `DisplayText` | 显示文本（预留） |
| `MatchPattern` | `FGameplayTagQuery`，当 Request 的 TargetTags 匹配时此 Style 生效 |
| `Color` / `CriticalColor` | 普通/暴击颜色（`bOverrideColor = true` 时生效） |
| `TextMesh` | 要使用的 StaticMesh（`bOverrideMesh = true` 时生效） |

### 4.3 UEqDamagePopStyleNiagara（Niagara 样式资产）

| 属性 | 说明 |
|------|------|
| `NiagaraArrayName` | Niagara 系统中接收伤害数据的数组参数名 |
| `TextNiagara` | 用于显示数字的 NiagaraSystem 资产引用 |

---

## 五、MeshText 方案详解

### 5.1 核心流程 (`AddNumberPop`)

```
调用 AddNumberPop(Request)
   │
   ├─ 1. 本地检查: 只在 LocalController 上执行，避免 ListenServer 重复弹出
   │
   ├─ 2. 数字拆分: 将 NumberToDisplay 拆成逐位数组
   │     例: 1234 → [0, 1, 2, 3, 4]  (index 0 预留给 +/- 符号)
   │
   ├─ 3. 对象池获取/创建 StaticMeshComponent
   │     ├─ 按 Mesh 类型分池 (PooledComponentMap)
   │     ├─ 池中有组件 → Pop 复用
   │     └─ 池中无组件 → NewObject 创建，配置:
   │           • 无碰撞 (NoCollision)
   │           • 自定义深度 (Stencil=123, 用于后处理排除)
   │           • 放大包围盒 (BoundsScale=2000, 因WPO动画位移大)
   │           • 创建动态材质实例 (MID)
   │
   ├─ 4. 注册组件 → 加入 LiveComponents 列表 → 启动定时释放
   │
   ├─ 5. 确定位置: 获取相机变换 + 随机偏移(±5单位)
   │
   └─ 6. 设置材质参数 (SetMaterialParameters)
         ├─ 符号位 (+Or-)
         ├─ 颜色 (根据 Style 的 TagQuery 匹配)
         ├─ 每位数字的: 位置(a参数) / 缩放旋转(b参数) / 时长(c参数)
         ├─ 距离自适应缩放 (离摄像机越远字越大)
         ├─ 暴击额外放大 (CriticalHitSizeMultiplier = 1.7x)
         └─ MoveToCamera 深度偏移 (防止被遮挡，观战时关闭)
```

### 5.2 对象池 & 生命周期管理

```
      创建/复用                    到期回收
    ┌──────────┐               ┌──────────────┐
    │PooledMap │──Pop()──→使用──→│LiveComponents│
    │(按Mesh分)│←──Push()──回收──│  (按时间序)  │
    └──────────┘               └──────────────┘
                                      │
                          ReleaseNextComponents()
                          由 Timer 驱动 (间隔 = ComponentLifespan)
```

- **`PooledComponentMap`**: `TMap<UStaticMesh*, FPooledNumberPopComponentList>` — 按 Mesh 类型分池
- **`LiveComponents`**: `TArray<FLiveNumberPopEntry>` — 当前正在播放的组件，按时间排序
- **`ReleaseTimerHandle`**: 定时器，在 `ComponentLifespan`（默认1秒）后触发 `ReleaseNextComponents()`
- 回收时 **UnregisterComponent** 然后 Push 回池，不销毁对象

### 5.3 材质参数命名约定

每位数字对应 3 组材质参数（最多支持 9 位数字 = index 0~8）：

| 参数组 | 命名模式 | 数据内容 (RGBA) |
|--------|----------|-----------------|
| 位置 | `0a, 1a, 2a, ...` | RGB = 相机空间方向, A = 累积偏移 |
| 缩放/旋转 | `0b, 1b, 2b, ...` | R = FontX缩放, G = FontY缩放, B = 数字值, A = 旋转角度 |
| 时长 | `0c, 1c, 2c, ...` | R = 到期时间, G = 随机种子 |

全局参数：

| 参数名 | 说明 |
|--------|------|
| `+Or-` | 符号位 (0.0=正, 0.5=负) |
| `Color` | 数字颜色 |
| `Animation Lifespan` | 动画时长 |
| `isCriticalHit?` | 是否暴击 (0/1) |
| `MoveToCamera` | 深度偏移开关 (0/1) |

### 5.4 可配置属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `ComponentLifespan` | 1.0s | 数字存活时间 |
| `DistanceFromCameraBeforeDoublingSize` | 1024 | 超过此距离开始放大数字 |
| `CriticalHitSizeMultiplier` | 1.7 | 暴击数字放大倍率 |
| `FontXSize` | 10.92 | 字体X尺寸 |
| `FontYSize` | 21.0 | 字体Y尺寸 |
| `SpacingPercentageForOnes` | 0.8 | 数字"1"的间距系数(比其他数字窄) |
| `NumberOfNumberRotations` | 1.0 | 旋转圈数 |

---

## 六、Niagara 方案详解

### 6.1 核心流程 (`AddNumberPop`)

```
调用 AddNumberPop(Request)
   │
   ├─ 1. 处理暴击: 暴击时将 damage 取负值 (用正负区分普通/暴击)
   │
   ├─ 2. 懒创建 NiagaraComponent (只创建一次，后续复用)
   │     └─ SetAsset(Style->TextNiagara), bAutoActivate=false
   │
   ├─ 3. Activate + 设置世界位置
   │
   └─ 4. 通过 NiagaraDataInterfaceArrayFunctionLibrary
         将伤害数据追加到 Niagara Array:
         FVector4(X, Y, Z, Damage) — XYZ=位置, W=伤害值
```

### 6.2 特点

- **更简洁**: 不需要对象池，所有渲染逻辑交给 Niagara System
- **数据驱动**: 伤害数据通过 `NiagaraArrayName` 指定的数组参数传入
- **暴击用负数区分**: 正数=普通伤害，负数=暴击伤害，Niagara 内部自行判断表现

---

## 七、使用方法

### 7.1 基本接入步骤

#### Step 1: 创建蓝图子类

在编辑器中创建一个蓝图，继承 `UEqNumberPopComponent_MeshText` 或 `UEqNumberPopComponent_NiagaraText`。

- **MeshText 方案**: 在蓝图 Details 面板中配置 `Styles` 数组（添加 `UEqDamagePopStyle` 数据资产）
- **Niagara 方案**: 在蓝图 Details 面板中配置 `Style`（指定 `UEqDamagePopStyleNiagara` 数据资产）

#### Step 2: 创建样式数据资产

**MeshText**:
1. Content Browser → 右键 → Miscellaneous → Data Asset → 选择 `EqDamagePopStyle`
2. 配置 `MatchPattern`（GameplayTagQuery，匹配目标标签）
3. 勾选 `bOverrideColor` 并设置颜色
4. 勾选 `bOverrideMesh` 并指定数字 StaticMesh

**Niagara**:
1. Content Browser → 右键 → Miscellaneous → Data Asset → 选择 `EqDamagePopStyleNiagara`
2. 指定 `TextNiagara`（Niagara System 资产）
3. 设置 `NiagaraArrayName`（Niagara 中接收数据的数组名）

#### Step 3: 挂载到 PlayerController

`UEqNumberPopComponent` 继承自 `UControllerComponent`，需要作为组件添加到 PlayerController 上。

```cpp
// 方式一: 在 PlayerController 蓝图中直接添加组件

// 方式二: C++ 中动态添加
if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
{
    UEqNumberPopComponent* PopComp = NewObject<UEqNumberPopComponent_MeshText>(PC, PopComponentBPClass);
    PopComp->RegisterComponent();
}
```

#### Step 4: 发送弹出请求

```cpp
// 在伤害处理逻辑中（例如 GameplayEffect 回调、伤害事件等）调用:
if (APlayerController* PC = /* 获取造成伤害的玩家控制器 */)
{
    if (UEqNumberPopComponent* PopComp = PC->FindComponentByClass<UEqNumberPopComponent>())
    {
        FEqNumberPopRequest Request;
        Request.WorldLocation = HitResult.ImpactPoint;  // 受击位置
        Request.NumberToDisplay = DamageAmount;           // 伤害值
        Request.bIsCriticalDamage = bWasCritical;         // 是否暴击
        Request.SourceTags = SourceAbilityTags;           // 来源标签（可选）
        Request.TargetTags = TargetActorTags;             // 目标标签（用于样式匹配）

        PopComp->AddNumberPop(Request);
    }
}
```

### 7.2 蓝图调用

`AddNumberPop` 标记了 `BlueprintCallable`，可以直接在蓝图中调用：

```
Get Player Controller
  → Get Component by Class (EqNumberPopComponent)
    → Add Number Pop
        ├─ World Location: Hit Location
        ├─ Number To Display: Damage
        ├─ Is Critical Damage: bool
        ├─ Source Tags: (可选)
        └─ Target Tags: (可选)
```

### 7.3 多样式配置示例

MeshText 方案支持 `Styles` 数组，按顺序匹配（第一个匹配的生效）：

```
Styles[0]:  MatchPattern = "Status.Fire"     → Color = 红橙色,   Mesh = FireNumberMesh
Styles[1]:  MatchPattern = "Status.Ice"      → Color = 冰蓝色,   Mesh = IceNumberMesh
Styles[2]:  MatchPattern = "Status.Healing"  → Color = 绿色,     Mesh = HealNumberMesh
Styles[3]:  MatchPattern = (默认匹配所有)    → Color = 白色,     Mesh = DefaultNumberMesh
```

当传入 `Request.TargetTags` 包含 `Status.Fire` 时，数字会以红橙色和火焰数字Mesh显示。

---

## 八、两种方案对比

| 维度 | MeshText | NiagaraText |
|------|----------|-------------|
| **渲染方式** | StaticMesh + 材质WPO动画 | Niagara粒子系统 |
| **样式系统** | 支持多Style数组 + TagQuery匹配 | 单一Style |
| **对象池** | 有（按Mesh类型分池，自动回收复用） | 无（单NiagaraComponent复用） |
| **暴击区分** | 材质参数 `isCriticalHit?` | 正负数区分 |
| **数字拆分** | C++逐位拆分 → 材质参数 | 整数直接传入Niagara |
| **扩展性** | 需要配套材质 | 需要配套Niagara System |
| **性能特点** | 对象池减少GC，材质参数更新开销 | Niagara GPU加速 |
| **适用场景** | 需要精细控制每位数字表现 | 需要粒子特效风格的数字 |

---

## 九、注意事项

1. **仅本地执行**: MeshText 方案会检查 `IsLocalController()`，避免 ListenServer 上多次弹出
2. **最大位数限制**: MeshText 默认支持最多 9 位数字（0~8），超出时会被截断为 999999999
3. **材质配套**: MeshText 方案需要一个配套材质，包含上述所有参数名（0a~8a, 0b~8b, 0c~8c 等）
4. **包围盒放大**: StaticMesh 的 BoundsScale 被设为 2000，因为 WPO 动画会使数字远离原始位置
5. **Custom Depth**: Stencil 值设为 123，可在后处理中用来排除数字不受 DOF/Bloom 等影响
6. **模块依赖**: 需要 `Niagara` 模块（已在 `EqZeroGame.Build.cs` 的 PublicDependencyModuleNames 中）

---

## 十、目录结构

```
Source/EqZeroGame/Feedback/NumberPops/
├── EqNumberPopComponent.h          // 抽象基类 + FEqNumberPopRequest
├── EqNumberPopComponent.cpp
├── EqDamagePopStyle.h              // MeshText 样式 DataAsset
├── EqDamagePopStyle.cpp
├── EqDamagePopStyleNiagara.h       // Niagara 样式 DataAsset (仅头文件)
├── EqNumberPopComponent_MeshText.h     // MeshText 实现
├── EqNumberPopComponent_MeshText.cpp
├── EqNumberPopComponent_NiagaraText.h  // Niagara 实现
└── EqNumberPopComponent_NiagaraText.cpp
```
