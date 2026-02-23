# GameplayTag 架构设计指南

## 一、当前结构诊断

项目当前有 **208 个 tags，22 个根命名空间**。存在以下典型问题：

| 问题 | 示例 |
|------|------|
| **命名空间前缀混杂** | `Lyra.*`、`ShooterGame.*`、`EqZero.*` 三套前缀共存，语义边界模糊 |
| **"伤害"散落四处** | `Gameplay.Damage`、`EqZero.Damage`、`Lyra.Damage`、`GameplayEffect.DamageType` 都涉及伤害 |
| **消息通道不统一** | `Ability.*.Message`、`Gameplay.Message.*`、`Lyra.*.Message`、`ShooterGame.*.Message` |
| **Event 二义性** | `Event.Movement.*`（动画事件）vs `GameplayEvent.*`（GAS 事件），概念不同但命名相近 |
| **遗留标记** | `TODO.GameModeDamageImmunity` 残留在生产 tag 树中 |

---

## 二、推荐的根命名空间设计（10-12 个即可）

项目初期应把根命名空间控制在 **10-12 个以内**，每个有明确职责：

```
[根命名空间]           [职责]                              [定义位置]
─────────────────────────────────────────────────────────────────────
Ability.*              技能系统元数据（类型/行为/失败原因）    C++ Native
State.*                实体状态标签（替代旧 Status）          C++ Native
Input.*                输入映射标签                          C++ Native
Event.*                GAS GameplayEvent 事件               C++ Native + INI
Effect.*               GE 分类（伤害类型/治疗类型/Trait）     INI
Cue.*                  GameplayCue 视觉/音效                INI
Message.*              GameplayMessageSubsystem 通道        INI
Movement.*             移动模式                             C++ Native
Phase.*                游戏流程/阶段                         INI（GameFeature）
UI.*                   UI 层级/HUD 插槽                     INI
Platform.*             平台能力检测                          INI
Cosmetic.*             外观/表现                            INI
SetByCaller.*          GE 数值传递                          C++ Native
Cheat.*                调试作弊                             C++ Native
```

---

## 三、核心设计原则

### 1. "一个概念一个家"原则

所有跟"伤害"相关的标签，应该在同一个子树里一眼就能看清全貌：

```
Effect.Damage.Type.Rifle
Effect.Damage.Type.Shotgun
Effect.Damage.Type.Melee
Effect.Damage.Type.Grenade
Effect.Damage.Trait.Instant
Effect.Damage.Trait.Periodic
Effect.Damage.Source.FellOutOfWorld
Effect.Damage.Source.SelfDestruct
```

而不是散布在 `Gameplay.Damage`、`GameplayEffect.DamageType`、`EqZero.Damage` 等多处。

### 2. "动词.名词" vs "名词.动词" 保持统一

推荐 **名词前置**（与 UE 引擎惯例一致）：

```
✅  Ability.Type.Action.Dash     （名词链：技能 → 类型 → 动作 → 冲刺）
✅  Input.Weapon.Fire             （名词链：输入 → 武器 → 开火）
❌  Event.Movement.WeaponFire    （动词 Event 开头，但 Movement 又是名词中间层）
```

### 3. 层级深度控制在 3-4 层

```
✅  Ability.ActivateFail.Cost          (3 层)
✅  Effect.Damage.Type.Rifle           (4 层)
❌  Lyra.ShooterGame.Accolade.EliminationStreak.20  (5 层, 过深)
```

超过 4 层时考虑：要么你的分类粒度太细，要么根节点选择有问题。

### 4. 消除"项目名"前缀

`Lyra.*` 和 `EqZero.*` 这种项目名前缀应该在早期就清除。它不提供语义信息，只说明"这是从哪抄来的"：

```
❌  Lyra.ShooterGame.Weapon.MagazineAmmo
✅  Weapon.Ammo.Magazine

❌  EqZero.Damage.Message
✅  Message.Damage.Taken

❌  Lyra.HUD.PlayerHUD
✅  UI.HUD.Player
```

### 5. 消息通道统一到 `Message.*`

当前消息 tag 散落在 5 个不同的父节点下。统一后：

```
Message.Ability.ActivateFail         // 替代 Ability.UserFacingSimpleActivateFail.Message
Message.Ability.PlayMontageOnFail    // 替代 Ability.PlayMontageOnActivateFail.Message
Message.Ability.Respawn.Duration     // 替代 Ability.Respawn.Duration.Message
Message.Ability.Respawn.Completed    // 替代 Ability.Respawn.Completed.Message
Message.Ability.Dash.Duration        // 替代 Ability.Dash.Duration.Message
Message.Damage.Taken                 // 替代 Lyra.Damage.Taken.Message & EqZero.Damage.Message
Message.Nameplate.Add                // 替代 Gameplay.Message.Nameplate.Add
Message.Nameplate.Remove
Message.Nameplate.Discover
Message.Notification.KillFeed        // 替代 Lyra.AddNotification.KillFeed
Message.ControlPoint.Captured        // 替代 ShooterGame.ControlPoint.Captured.Message
```

---

## 四、按功能模块的推荐架构（完整版）

```
Ability
├── ActivateFail
│   ├── ActivationGroup
│   ├── Cooldown
│   ├── Cost
│   ├── IsDead
│   ├── MagazineFull
│   ├── Networking
│   ├── NoSpareAmmo
│   ├── TagsBlocked
│   └── TagsMissing
├── Behavior
│   └── SurvivesDeath
└── Type
    ├── Action              // Dash, Jump, Melee, Grenade, Reload, WeaponFire, ADS, Drop, Emote
    ├── Info                // ShowLeaderboard
    ├── Passive             // AutoReload, AutoRespawn, ChangeQuickbarSlot
    └── StatusChange        // Death, Spawning

State                       // 实体当前状态（用于 tag 查询/阻断）
├── Crouching
├── AutoRunning
├── SpawningIn
├── Death
│   ├── Dying
│   └── Dead
└── Init                    // 替代旧 InitState
    ├── Spawned
    ├── DataAvailable
    ├── DataInitialized
    └── GameplayReady

Input                       // 全部输入映射
├── Move
├── Look.Mouse / Look.Stick
├── Jump
├── Crouch
├── AutoRun
├── Ability                 // Dash, Emote, Heal, Interact, Melee, ShowLeaderboard, Toggle*
├── Weapon                  // Fire, FireAuto, ADS, Reload, Grenade
└── Quickslot               // CycleForward, CycleBackward, SelectSlot, Drop

Event                       // SendGameplayEventToActor 使用
├── Death
├── Reset
├── RequestReset
├── MeleeHit
├── ReloadDone
└── Movement                // 动画层事件: ADS, Dash, Melee, Reload, WeaponFire

Effect                      // GE 元标签
├── Damage
│   ├── Type                // Basic, Rifle, Shotgun, Pistol, Melee, Grenade
│   ├── Trait               // Instant, Periodic
│   └── Source              // FellOutOfWorld, SelfDestruct
├── Heal
│   └── Trait               // Instant, Periodic
├── Immunity
│   └── Damage
└── SetByCaller             // Damage, Heal

GameplayCue                 // 引擎硬编码前缀，不可改名
├── Character               // DamageTaken, Dash, Death, Heal, Melee, Spawn
├── Weapon                  // Rifle.Fire, Pistol.Fire, Shotgun.Fire, Grenade.Detonate, Melee.*
└── World                   // Launcher, Teleporter

Message                     // GameplayMessageSubsystem 通道
├── Ability.*               // 各种 Duration/Fail 消息
├── Damage.Taken
├── Nameplate.*
├── Notification.KillFeed
├── UserMessage.*           // MatchDecided, WaitingForPlayers
└── ControlPoint.Captured

Movement
└── Mode                    // Walking, Falling, Flying, Swimming, NavWalking, Custom

Phase                       // 游戏流程
├── Warmup
├── MatchBeginCountdown
├── Playing
└── PostGame

UI
├── Layer                   // Game, GameMenu, Menu, Modal
├── HUD                     // Player, TempTop
└── Slot                    // Equipment, Reticle, TeamScore, EliminationFeed, etc.

Platform
└── Trait                   // Input.*, SupportsWindowedMode, CanExitApplication, etc.

Cosmetic
├── AnimationStyle          // Feminine, Masculine
└── BodyStyle               // Medium

Gameplay                    // 杂项运行时标签
├── AbilityInputBlocked
├── MovementStopped
├── Zone.WeakSpot
└── Score                   // Eliminations, Deaths, Assists, ControlPointCapture

Cheat                       // GodMode, UnlimitedHealth

Weapon                      // 武器资源标签
└── Ammo
    ├── Magazine
    ├── MagazineSize
    └── Spare
```

> **注意**: `GameplayCue.*` 是引擎硬编码前缀 (`UGameplayCueManager` 会按前缀查找)，**不能改名**。

---

## 五、定义位置的最佳实践

| 类型 | 推荐定义位置 | 理由 |
|------|-------------|------|
| C++ 代码中硬查询的 tag | `UE_DEFINE_GAMEPLAY_TAG` in `.cpp` | 编译时检查，不会拼错 |
| 由蓝图/配置驱动的 tag | `DefaultGameplayTags.ini` | 设计师可自行添加 |
| GameFeature 专有的 tag | `Plugins/GameFeatures/*/Config/Tags/*.ini` | 随 Feature 启用/卸载 |
| **永远不要做的事** | 同一个 tag 在 INI 和 C++ 中都定义 | 会导致不清楚"谁拥有这个 tag" |

---

## 六、检查清单（每次添加新 tag 前过一遍）

1. **这个 tag 已经存在了吗？** → 搜索树，避免 `Gameplay.Message.ADS` 和 `Event.Movement.ADS` 这种语义重叠
2. **它属于哪个根命名空间？** → 不确定就对照上面的"推荐架构"表
3. **层级超过 4 层了吗？** → 如果超过，审视中间层是否真有价值
4. **它需要在 C++ 中硬编码查询吗？** → 是则用 Native Tag，否则用 INI
5. **DevComment 写了吗？** → 没注释的 tag 3 个月后没人记得它干什么

---

## 七、对当前项目的重构优先级

如果要渐进式重构（不一次性全改），按这个顺序：

| 优先级 | 动作 | 影响范围 |
|--------|------|---------|
| **P0** | 删除 `TODO.GameModeDamageImmunity` | 无影响 |
| **P1** | `Lyra.*` → 各归各位（`Message.*`, `UI.*`, `Weapon.*`, `Gameplay.Score.*`） | 19 tags |
| **P2** | 统一所有 `*.Message` → `Message.*` | ~12 tags |
| **P3** | `ShooterGame.*` → `Phase.*` + `Gameplay.Score.*` | 11 tags |
| **P4** | `GameplayEffect.*` → `Effect.*` | 10 tags, 仅 INI 定义 |
| **P5** | `Status.*` + `InitState.*` → `State.*` | 10 tags, 需改 C++ |

每一步都可以独立完成、独立测试，不需要一次全改。

---

## 附录：Tag Dump 工具

运行以下命令可随时重新生成完整的 tag 树：

```powershell
.\Tools\DumpGameplayTags.ps1
```

输出文件：`Tools\GameplayTagDump.txt`
