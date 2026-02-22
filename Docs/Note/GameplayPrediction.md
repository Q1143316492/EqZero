# Gameplay Ability Prediction — 中英双译

> 原文来自 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/GameplayPrediction.h`

---

## Overview of Gameplay Ability Prediction
## Gameplay Ability 预测系统概述

---

### High Level Goals / 高层目标

**EN:**
At the GameplayAbility level (implementing an ability) prediction is transparent. An ability says "Do X→Y→Z", and we will automatically predict the parts of that that we can.
We wish to avoid having logic such as "If Authority: Do X. Else: Do predictive version of X" in the ability itself.

**CN:**
在 GameplayAbility 层面（实现某个 Ability 时），预测是透明的。一个 Ability 只需描述"执行 X→Y→Z"，系统会自动对其中可以预测的部分进行预测。
我们希望避免在 Ability 内部出现"如果是权威端：执行 X；否则：执行 X 的预测版本"这样的逻辑。

---

**EN:**
At this point, not all cases are solved, but we have a very solid framework for working with client side prediction.

**CN:**
目前并非所有情况都已解决，但我们已经拥有一个非常扎实的客户端预测框架。

---

**EN:**
When we say "client side prediction" we really mean client predicting game simulation state. Things can still be 'completely client side' without having to work within a prediction system.
For example, footsteps are completely client side and never interact with this system. But clients predicting their mana going from 100 to 90 when they cast a spell is 'client side prediction'.

**CN:**
我们所说的"客户端预测"，真正的含义是客户端对游戏仿真状态进行预测。有些东西仍然可以"完全在客户端"运行，而无需进入预测系统。
例如，脚步声完全是客户端本地的，从不与该系统交互。但客户端预测施法时蓝量从 100 降到 90，才是"客户端预测"。

---

### What do we currently predict? / 当前预测哪些内容？

**EN:**
- Initial GameplayAbility activation (and chained activation with caveats)
- Triggered Events
- GameplayEffect application:
  - Attribute modification (EXCEPTIONS: Executions do not currently predict, only attribute modifiers)
  - GameplayTag modification
- Gameplay Cue events (both from within predictive gameplay effect and on their own)
- Montages
- Movement (built into UE UCharacterMovement)

**CN:**
- 初始 GameplayAbility 激活（以及有条件的链式激活）
- 触发事件（Triggered Events）
- GameplayEffect 应用：
  - 属性修改（例外：Executions 目前不支持预测，仅属性修改器支持）
  - GameplayTag 修改
- Gameplay Cue 事件（包括在预测 GameplayEffect 内部触发的，以及独立触发的）
- 蒙太奇（Montages）
- 移动（内置于 UE 的 UCharacterMovement）

---

### What we don't predict / 当前不预测的内容

**EN:**
- GameplayEffect removal
- GameplayEffect periodic effects (dots ticking)

**CN:**
- GameplayEffect 的移除
- GameplayEffect 的周期性效果（持续伤害跳点等）

---

### Problems we attempt to solve / 我们试图解决的问题

**EN:**
1. **"Can I do this?"** — Basic protocol for prediction.
2. **"Undo"** — How to undo side effects when a prediction fails.
3. **"Redo"** — How to avoid replaying side effects that we predicted locally but that also get replicated from the server.
4. **"Completeness"** — How to be sure we *really* predicted all side effects.
5. **"Dependencies"** — How to manage dependent prediction and chains of predicted events.
6. **"Override"** — How to override state predictively that is otherwise replicated/owned by the server.

**CN:**
1. **"我能做这件事吗？"** — 预测的基本协议。
2. **"撤销（Undo）"** — 预测失败时，如何回滚副作用。
3. **"重做（Redo）"** — 如何避免重复执行本地已预测、且服务器也复制下来的副作用。
4. **"完整性（Completeness）"** — 如何确保我们真正预测了所有副作用。
5. **"依赖（Dependencies）"** — 如何管理依赖性预测和链式预测事件。
6. **"覆盖（Override）"** — 如何以预测方式覆盖原本由服务器复制/持有的状态。

---

## Implementation Details / 实现细节

---

### *** PredictionKey ***

**EN:**
A fundamental concept in this system is the Prediction Key (`FPredictionKey`). A prediction key on its own is simply a unique ID that is generated in a central place on the client. The client will send its prediction key to the server,
and associate predictive actions and side effects with this key. The server may respond with an accept/reject for the prediction key, and will also associate the server-side created side effects with this prediction key.

**CN:**
该系统的核心概念是预测键（`FPredictionKey`）。预测键本身只是一个在客户端某个中心位置生成的唯一 ID。客户端将预测键发送给服务器，并将预测动作和副作用与该键关联。服务器可以对预测键做出接受/拒绝的响应，同时也会将服务器端产生的副作用与该预测键关联。

---

**EN:**
**(IMPORTANT)** `FPredictionKey` always replicates client → server, but when replicating server → clients they *only* replicate to the client that sent the prediction key to the server in the first place.
This happens in `FPredictionKey::NetSerialize`. All other clients will receive an invalid (0) prediction key when a prediction key sent from a client is replicated back down through a replicated property.

**CN:**
**（重要）** `FPredictionKey` 始终从客户端复制到服务器，但在服务器向客户端复制时，它*只*复制给最初发送该预测键的那个客户端。
这在 `FPredictionKey::NetSerialize` 中实现。所有其他客户端在通过复制属性接收到来自客户端的预测键时，将收到一个无效（0）的预测键。

---

### *** Ability Activation / Ability 激活 ***

**EN:**
Ability Activation is a first class predictive action — it generates an initial prediction key. Whenever a client predictively activates an ability, it explicitly asks the server and the server explicitly responds. Once an ability has been
predictively activated (but the request has not yet been sent), the client has a valid 'prediction window' where predictive side effects can happen which are not explicitly 'asked about'. (E.g., we do not explicitly ask 'Can I decrement mana, Can I
put this ability on cooldown. Those actions are considered logically atomic with activating an ability). You can think of this prediction window as being the initial callstack of `ActivateAbility`. Once `ActivateAbility` ends, your
prediction window (and therefore your prediction key) is no longer valid. This is important, because many things can invalidate your prediction window such as any timers or latent nodes in your Blueprint; we do not predict over multiple frames.

**CN:**
Ability 激活是一种一等（first-class）预测动作——它会生成一个初始预测键。每当客户端预测性地激活某个 Ability 时，它会显式地向服务器发起请求，服务器也会显式地做出响应。一旦 Ability 被预测性激活（但请求尚未发送），客户端就进入一个有效的"预测窗口"，在此窗口内发生的预测副作用无需被显式询问。（例如，我们不会显式询问"我能扣蓝吗？我能让这个 Ability 进入冷却吗？"这些动作被视为与激活 Ability 在逻辑上是原子性的。）你可以将这个预测窗口理解为 `ActivateAbility` 的初始调用栈。一旦 `ActivateAbility` 返回，你的预测窗口（以及预测键）就不再有效。这一点非常重要，因为蓝图中的定时器或延迟节点等许多因素都可能使预测窗口失效；我们不跨帧进行预测。

---

**EN:**
`AbilitySystemComponent` provides a set of functions for communicating ability activation between clients and server:
`TryActivateAbility` → `ServerTryActivateAbility` → `ClientActivateAbility(Failed/Succeed)`.

1. Client calls `TryActivateAbility` which generates a new `FPredictionKey` and calls `ServerTryActivateAbility`.
2. Client continues (before hearing back from server) and calls `ActivateAbility` with the generated PredictionKey associated with the Ability's `ActivationInfo`.
3. Any side effects that happen *before the call to ActivateAbility finish* have the generated `FPredictionKey` associated with them.
4. Server decides if the ability really happened in `ServerTryActivateAbility`, calls `ClientActivateAbility(Failed/Succeed)` and sets `UAbilitySystemComponent::ReplicatedPredictionKey` to the generated key that was sent with the request by the client.
5. If client receives `ClientAbilityFailed`, it immediately kills the ability and rolls back side effects that were associated with the prediction key.
   - 5a. 'Rolling back' logic is registered via `FPredictionKeyDelegates` and `FPredictionKey::NewRejectedDelegate/NewCaughtUpDelegate/NewRejectOrCaughtUpDelegate`.
   - 5b. `ClientAbilityFailed` is really the only case where we 'reject' prediction keys and thus all of our current predictions rely on if an ability activates or not.
6. If `ServerTryActivateAbility` succeeds, client must wait until property replication catches up (the Succeed RPC will be sent immediately, property replication will happen on its own). Once the `ReplicatedPredictionKey` catches up to the key used in previous steps, the client can undo its predictive side effects.
   See `FReplicatedPredictionKeyItem::OnRep` for the CatchUpTo logic. See `UAbilitySystemComponent::ReplicatedPredictionKeyMap` for how the keys actually get replicated. See `~FScopedPredictionWindow` where the server acknowledges keys.

**CN:**
`AbilitySystemComponent` 提供了一组函数，用于在客户端和服务器之间传递 Ability 激活信息：
`TryActivateAbility` → `ServerTryActivateAbility` → `ClientActivateAbility(Failed/Succeed)`。

1. 客户端调用 `TryActivateAbility`，生成新的 `FPredictionKey` 并调用 `ServerTryActivateAbility`。
2. 客户端不等待服务器响应，继续执行，以生成的 PredictionKey 调用 `ActivateAbility`，该键与 Ability 的 `ActivationInfo` 关联。
3. 在 `ActivateAbility` 调用完成之前发生的所有副作用都与生成的 `FPredictionKey` 关联。
4. 服务器在 `ServerTryActivateAbility` 中决定 Ability 是否真正发生，并调用 `ClientActivateAbility(Failed/Succeed)`，同时将 `UAbilitySystemComponent::ReplicatedPredictionKey` 设置为客户端请求中携带的预测键。
5. 若客户端收到 `ClientAbilityFailed`，立即终止 Ability 并回滚与该预测键关联的副作用。
   - 5a. "回滚"逻辑通过 `FPredictionKeyDelegates` 和 `FPredictionKey::NewRejectedDelegate/NewCaughtUpDelegate/NewRejectOrCaughtUpDelegate` 注册。
   - 5b. `ClientAbilityFailed` 实际上是唯一会"拒绝"预测键的情况，因此当前所有预测都依赖于 Ability 是否成功激活。
6. 若 `ServerTryActivateAbility` 成功，客户端必须等待属性复制追上来（Succeed RPC 会立即发送，属性复制会自行完成）。一旦 `ReplicatedPredictionKey` 追上前述步骤中使用的键，客户端就可以撤销其预测的副作用。
   参见 `FReplicatedPredictionKeyItem::OnRep` 了解 CatchUpTo 逻辑，参见 `UAbilitySystemComponent::ReplicatedPredictionKeyMap` 了解键的实际复制方式，参见 `~FScopedPredictionWindow` 了解服务器如何确认键。

---

### *** GameplayEffect Prediction / GameplayEffect 预测 ***

**EN:**
GameplayEffects are considered side effects of ability activation and are not separately accepted/rejected.

1. GameplayEffects are only applied on clients if there is a valid prediction key.
2. Attributes, GameplayCues, and GameplayTags are all predicted if the GameplayEffect is predicted.
3. When the `FActiveGameplayEffect` is created, it stores the prediction key (`FActiveGameplayEffect::PredictionKey`). Instant effects are explained below in "Attribute Prediction".
4. On the server, the same prediction key is also set on the server's `FActiveGameplayEffect` that will be replicated down.
5. As a client, if you get a replicated `FActiveGameplayEffect` with a valid prediction key on it, you check to see if you have an `ActiveGameplayEffect` with that same key; if there is a match, we do not apply the 'on applied' type of logic (e.g., GameplayCues). This solves the "Redo" problem. However we will have 2 of the 'same' GameplayEffects in our `ActiveGameplayEffects` container, temporarily.
6. At the same time, `FReplicatedPredictionKeyItem::OnRep` will catch up and the predictive effects will be removed. When they are removed in this case, we again check PredictionKey and decide if we should not do the 'On Remove' logic / GameplayCue.

**CN:**
GameplayEffect 被视为 Ability 激活的副作用，不会被单独接受/拒绝。

1. 只有存在有效预测键时，GameplayEffect 才会在客户端被应用。
2. 若 GameplayEffect 被预测，属性、GameplayCue 和 GameplayTag 均被一并预测。
3. 创建 `FActiveGameplayEffect` 时，会存储预测键（`FActiveGameplayEffect::PredictionKey`）。即时效果详见下文"属性预测"。
4. 在服务器端，同样的预测键也被设置在将要向下复制的服务器 `FActiveGameplayEffect` 上。
5. 客户端收到带有有效预测键的复制 `FActiveGameplayEffect` 时，会检查是否已有具有相同键的 `ActiveGameplayEffect`；若匹配，则不执行"应用时"类型的逻辑（例如 GameplayCue），以此解决"Redo"问题。但在 `ActiveGameplayEffects` 容器中会临时存在两个"相同"的 GameplayEffect。
6. 与此同时，`FReplicatedPredictionKeyItem::OnRep` 会追上来，预测性效果将被移除。在此情况下移除时，我们再次检查 PredictionKey，并决定是否跳过"移除时"逻辑 / GameplayCue。

---

### *** Attribute Prediction / 属性预测 ***

**EN:**
Since attributes are replicated as standard uproperties, predicting modification to them can be tricky ("Override" problem). Instantaneous modification can be even harder since these are non-stateful by nature.
The basic plan of attack is to treat attribute prediction as delta prediction rather than absolute value prediction. We do not predict that we have 90 mana, we predict that we have -10 mana from the server value, until the server confirms our prediction key. Basically, treat instant modifications as *infinite duration modifications* to attributes while they are done predictively. This solves "Undo" and "Redo".

For the "override" problem, we handle this in the properties `OnRep` by treating the replicated (server) value as the 'base value' instead of 'final value' of the attribute, and to reaggregate our 'final value' after a replication happens.

1. We treat predictive instant gameplay effects as infinite duration gameplay effects. See `UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf`.
2. We have to *always* receive RepNotify calls on our attributes (not just when there is a change from last local value). Done with `REPNOTIFY_Always`.
3. In the attribute RepNotify, we call into `AbilitySystemComponent::ActiveGameplayEffects` to update our 'final value' given the new 'base value'. The `GAMEPLAYATTRIBUTE_REPNOTIFY` macro can do this.
4. Everything else will work like above (GameplayEffect prediction): when the prediction key is caught up, the predictive GameplayEffect is removed and we will return to the server given value.

**CN:**
由于属性是作为标准 uproperty 复制的，预测对其的修改可能很棘手（"Override"问题）。即时修改更难，因为其本质上是无状态的。

基本思路是将属性预测视为增量预测而非绝对值预测。我们不预测自己有 90 点蓝，而是预测相对于服务器值有 -10 点蓝，直到服务器确认预测键。本质上，在预测性执行期间，将即时修改视为*无限持续时间的属性修改*。这解决了"Undo"和"Redo"问题。

对于"Override"问题，在属性的 `OnRep` 中通过将复制的（服务器）值视为属性的"基础值"而非"最终值"来处理，并在复制发生后重新聚合"最终值"。

1. 将预测性即时 GameplayEffect 视为无限持续时间的 GameplayEffect。参见 `UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf`。
2. 必须*始终*接收属性的 RepNotify 回调（不仅仅是在与上次本地值不同时）。通过 `REPNOTIFY_Always` 实现。
3. 在属性 RepNotify 中，调用 `AbilitySystemComponent::ActiveGameplayEffects` 以根据新"基础值"更新"最终值"，`GAMEPLAYATTRIBUTE_REPNOTIFY` 宏可完成此操作。
4. 其余逻辑与上文 GameplayEffect 预测相同：预测键被追上时，预测性 GameplayEffect 被移除，属性回归服务器给定值。

---

### *** Gameplay Cue Events / Gameplay Cue 事件 ***

**EN:**
Outside of GameplayEffects, Gameplay Cues can be activated on their own. These functions (`UAbilitySystemComponent::ExecuteGameplayCue` etc.) take network role and prediction keys into account.

1. In `UAbilitySystemComponent::ExecuteGameplayCue`, if authority then do the multicast event (with replication key). If non-authority but with a valid prediction key, predict the GameplayCue.
2. On the receiving end (`NetMulticast_InvokeGameplayCueExecuted` etc.), if there is a replication key, then don't do the event (assume you predicted it).

Remember that `FPredictionKeys` only replicate to the originating owner.

**CN:**
在 GameplayEffect 之外，Gameplay Cue 也可以独立激活。相关函数（如 `UAbilitySystemComponent::ExecuteGameplayCue`）会考虑网络角色和预测键。

1. 在 `UAbilitySystemComponent::ExecuteGameplayCue` 中，若为权威端则执行多播事件（携带复制键）；若为非权威端但具有有效预测键，则预测该 GameplayCue。
2. 在接收端（如 `NetMulticast_InvokeGameplayCueExecuted`），若存在复制键，则不执行事件（假设已经预测过了）。

注意：`FPredictionKey` 只复制给发起方 Owner。

---

### *** Triggered Data Prediction / 触发数据预测 ***

**EN:**
Triggered Data is currently used to activate abilities. Essentially this all goes through the same code path as `ActivateAbility`. Rather than the ability being activated from input press, it is activated from another game code driven event. Clients are able to predictively execute these events which predictively activate abilities.

There are some nuances however, since the server will also run the code that triggers events. The server won't just wait to hear from the client. The server will keep a list of triggered abilities that have been activated from a predictive ability. When receiving a `TryActivate` from a triggered ability, the server will look to see if *it* has already run this ability, and respond with that information.

The issue is we do not properly rollback these operations. There is work left to do on Triggered Events and replication.

**CN:**
触发数据目前用于激活 Ability，其代码路径与 `ActivateAbility` 基本相同。区别在于 Ability 不是由输入触发，而是由游戏代码驱动的事件触发。客户端可以预测性地执行这些事件，从而预测性地激活 Ability。

但有一些细节：服务器也会独立运行触发事件的代码，而不是等待客户端。服务器会维护一个由预测性 Ability 激活的触发 Ability 列表。当收到某个触发 Ability 的 `TryActivate` 时，服务器会查看自己是否已经运行过该 Ability，并以此作为响应。

问题在于，我们目前无法正确回滚这些操作。触发事件和复制方面仍有工作待完成。

---

## Advanced Topic: Dependencies / 高级主题：依赖链

**EN:**
We can have situations such as "Ability X activates and immediately triggers an event which activates Ability Y which triggers another Ability Z". The dependency chain is X→Y→Z.
Each of those abilities could be rejected by the server. If Y is rejected, then Z also never happened, but the server never tries to run Z, so the server doesn't explicitly decide 'no Z can't run'.

To handle this, we have a concept of a Base PredictionKey, which is a member of `FPredictionKey`. When calling `TryActivateAbility`, we pass in the current PredictionKey (if applicable). That prediction key is used as the base for any new prediction keys generated. We build a chain of keys this way, and can then invalidate Z if Y is rejected.

The prediction key of X is considered the Base key for Y and Z. The dependency from Y to Z is kept completely client side, done by `FPredictionKeyDelegates::AddDependancy`. We add delegates to reject/catchup Z if Y is rejected/confirmed.

This dependency system allows us to have multiple predictive actions that are not logically atomic within a single prediction window/scope.

There is a problem though: because the dependencies are kept client side, the server does not actually know if it had previously rejected a dependent action. You can design around this issue by using activation tags in your gameplay abilities. For instance, when predicting dependents GA_Combo1 → GA_Combo2, you could make GA_Combo2 only activate if it has a GameplayTag given by GA_Combo1. Thus a rejection of GA_Combo1 would also cause the server to reject the activation of GA_Combo2.

**CN:**
我们可能遇到这样的情况："Ability X 激活后立即触发事件激活 Ability Y，Y 又触发 Ability Z"，依赖链为 X→Y→Z。
每个 Ability 都可能被服务器拒绝。若 Y 被拒绝，Z 也从未发生，但服务器从未尝试运行 Z，因此服务器不会显式地决定"Z 不能运行"。

为解决此问题，引入了基础预测键（Base PredictionKey）的概念，它是 `FPredictionKey` 的一个成员。调用 `TryActivateAbility` 时，传入当前 PredictionKey（如果有），该键将作为新生成预测键的基础键。以此方式构建键链，当 Y 被拒绝时可以连带使 Z 无效。

X 的预测键被视为 Y 和 Z 的基础键。Y 到 Z 的依赖完全保存在客户端，通过 `FPredictionKeyDelegates::AddDependancy` 实现。若 Y 被拒绝/确认，注册的委托将同步拒绝/确认 Z。

该依赖系统使我们能够在单个预测窗口/作用域内拥有多个逻辑上不原子的预测动作。

但存在一个问题：因为依赖关系保存在客户端，服务器实际上不知道它之前是否拒绝了某个依赖动作。可以通过在 Gameplay Ability 中使用激活标签（activation tags）来规避此问题。例如，在预测连招 GA_Combo1 → GA_Combo2 时，可以让 GA_Combo2 只在拥有由 GA_Combo1 给予的 GameplayTag 时才激活，从而使 GA_Combo1 的拒绝也导致服务器拒绝 GA_Combo2 的激活。

---

## Additional Prediction Windows / 额外的预测窗口

**EN:**
As stated, a prediction key is only usable during a single logical scope. Once `ActivateAbility` returns, we are essentially done with that key. If the ability is waiting on an external event or timer, it's possible we will have already received a confirm/reject from the server by the time we're ready to continue execution.

This isn't that bad, except that abilities will sometimes want to react to player input. For example, 'a hold down and charge' ability wants to instantly predict some stuff when the button is released. It is possible to create a new prediction window within an ability with `FScopedPredictionWindow`.

`FScopedPredictionWindow` provides a way to send the server a new prediction key and have the server pick up and use that key within the same logical scope.

`UAbilityTask_WaitInputRelease::OnReleaseCallback` is a good example:
1. Client enters `OnReleaseCallback` and starts a new `FScopedPredictionWindow`, creating a new prediction key (`FScopedPredictionWindow::ScopedPredictionKey`).
2. Client calls `AbilitySystemComponent->ServerInputRelease` passing `ScopedPrediction.ScopedPredictionKey` as a parameter.
3. Server runs `ServerInputRelease_Implementation`, takes the passed-in PredictionKey and sets it as `UAbilitySystemComponent::ScopedPredictionKey` via an `FScopedPredictionWindow`.
4. Server runs `UAbilityTask_WaitInputRelease::OnReleaseCallback` *within the same scope*.
5. When the server hits the `FScopedPredictionWindow` in `::OnReleaseCallback`, it gets the prediction key from `UAbilitySystemComponent::ScopedPredictionKey`, which is then used for all side effects within this logical scope.
6. Once the server ends this scoped prediction window, the prediction key used is finished and set to `ReplicatedPredictionKey`.
7. All side effects created in this scope now share a key between client and server.

The key to this working is that `::OnReleaseCallback` calls `::ServerInputRelease` which calls `::OnReleaseCallback` on the server. There is no room for anything else to happen and use the given prediction key.

While there is no "Try/Failed/Succeed" calls in this example, all side effects are procedurally grouped/atomic. This solves the "Undo" and "Redo" problems for any arbitrary function calls that run on both server and client.

**CN:**
如前所述，预测键只能在单个逻辑作用域内使用。一旦 `ActivateAbility` 返回，该键基本就废弃了。若 Ability 在等待外部事件或定时器，当准备继续执行时，可能已经收到了服务器的确认/拒绝。

这本身不是大问题，但某些 Ability 需要响应玩家输入。例如，"长按蓄力"类 Ability 希望在按键松开时立即预测某些内容。可以通过 `FScopedPredictionWindow` 在 Ability 内部创建新的预测窗口。

`FScopedPredictionWindow` 提供了一种向服务器发送新预测键、并让服务器在同一逻辑作用域内使用该键的方式。

以 `UAbilityTask_WaitInputRelease::OnReleaseCallback` 为例：
1. 客户端进入 `OnReleaseCallback`，创建新的 `FScopedPredictionWindow`，生成新预测键（`FScopedPredictionWindow::ScopedPredictionKey`）。
2. 客户端调用 `AbilitySystemComponent->ServerInputRelease`，传入 `ScopedPrediction.ScopedPredictionKey`。
3. 服务器执行 `ServerInputRelease_Implementation`，取出传入的 PredictionKey，通过 `FScopedPredictionWindow` 将其设为 `UAbilitySystemComponent::ScopedPredictionKey`。
4. 服务器在*同一作用域内*运行 `UAbilityTask_WaitInputRelease::OnReleaseCallback`。
5. 当服务器执行到 `::OnReleaseCallback` 中的 `FScopedPredictionWindow` 时，从 `UAbilitySystemComponent::ScopedPredictionKey` 获取预测键，该键将用于此逻辑作用域内所有副作用。
6. 服务器结束该作用域预测窗口后，所用预测键完成使命并被设为 `ReplicatedPredictionKey`。
7. 此作用域内创建的所有副作用现在在客户端和服务器间共享同一个键。

此机制的关键在于：`::OnReleaseCallback` 调用 `::ServerInputRelease`，后者在服务器上调用 `::OnReleaseCallback`，不给其他代码使用该预测键留有机会。

虽然本例中没有"Try/Failed/Succeed"调用，但所有副作用在流程上是分组/原子性的，从而为在服务器和客户端上运行的任意函数调用解决了"Undo"和"Redo"问题。

---

## Unsupported / Issues / Todo / 不支持的功能 / 已知问题 / 待办事项

**EN:**
Triggered events do not explicitly replicate. E.g., if a triggered event only runs on the server, the client will never hear about it. This also prevents us from doing cross player/AI etc events. Support for this should eventually be added following the same pattern as GameplayEffect and GameplayCues (predict triggered event with a prediction key, ignore the RPC event if it has a prediction key).

Big caveat with this whole system: Rollback of any chained activations (including triggered events) is currently not possible out of the box. For example, if GA_Mispredict is predicted and immediately activates GA_Predict1, and then GA_Mispredict is rejected by the server, GA_Predict1 will still be executing on both client and server because we have no delegates that reject dependent abilities, and the server isn't even aware there are dependencies. You can design around this by using the tag system to ensure GA_Mispredict succeeded.

**CN:**
触发事件目前不会显式复制。例如，若触发事件只在服务器运行，客户端永远无法得知。这也阻止了跨玩家/AI 等事件的实现。该功能最终应被添加，遵循与 GameplayEffect 和 GameplayCue 相同的模式（用预测键预测触发事件，若 RPC 事件携带预测键则忽略）。

整个系统的一个重大限制：链式激活（包括触发事件）的回滚目前无法开箱即用。例如，若 GA_Mispredict 被预测激活且立即激活了 GA_Predict1，随后 GA_Mispredict 被服务器拒绝，GA_Predict1 仍会在客户端和服务器上继续执行，因为没有委托来拒绝依赖的 Ability，服务器也不知道存在依赖关系。可以通过标签系统设计规避，确保 GA_Mispredict 成功后 GA_Predict1 才能激活。

---

### *** Predicting "Meta" Attributes (Damage/Healing) vs "Real" Attributes (Health) / 预测"元"属性（伤害/治疗）vs "真实"属性（生命值）***

**EN:**
We are unable to apply meta attributes predictively. Meta attributes only work on instant effects, in the back end of GameplayEffect (`Pre/Post Modify Attribute` on the `UAttributeSet`). These events are not called when applying duration-based gameplay effects.

In order to support this, we would probably add some limited support for duration based meta attributes, and move the transform of the instant gameplay effect from the front end (`UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf`) to the backend (`UAttributeSet::PostModifyAttribute`).

**CN:**
我们无法预测性地应用元属性（Meta Attributes）。元属性只在即时效果的后端（`UAttributeSet` 的 `Pre/Post Modify Attribute`）工作，在应用基于持续时间的 GameplayEffect 时不会调用这些事件。

为支持此功能，可能需要对基于持续时间的元属性添加有限支持，并将即时 GameplayEffect 的转换逻辑从前端（`UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf`）移至后端（`UAttributeSet::PostModifyAttribute`）。

---

### *** Predicting Ongoing Multiplicative GameplayEffects / 预测持续性乘法 GameplayEffect ***

**EN:**
There are also limitations when predicting % based gameplay effects. Since the server replicates down the 'final value' of an attribute, but not the entire aggregator chain of what is modifying it, we may run into cases where the client cannot accurately predict new gameplay effects.

For example:
- Client has a perm +10% movement speed buff with base movement speed of 500 → 550 is the final movement speed.
- Client has an ability which grants an additional 10% movement speed buff. It is expected to *sum* the % based multipliers for a final 20% bonus to 500 → 600.
- However on the client, we just apply a 10% buff to 550 → 605.

This will need to be fixed by replicating down the aggregator chain for attributes. We already replicate some of this data, but not the full modifier list.

**CN:**
预测基于百分比的 GameplayEffect 时也存在限制。由于服务器复制下来的是属性的"最终值"，而非完整的聚合器修改链，客户端可能无法准确预测新 GameplayEffect 的效果。

例如：
- 客户端有一个永久 +10% 移速 Buff，基础移速 500 → 最终移速 550。
- 客户端有一个 Ability 额外给予 10% 移速 Buff，预期是将两个百分比相加，最终移速 500 × 1.20 = 600。
- 但在客户端，实际是对 550 应用 10% → 605，产生偏差。

此问题需要通过向下复制属性聚合器链来修复。目前已复制了部分数据，但尚未复制完整的修改器列表。

---

### *** "Weak Prediction" / "弱预测"***

**EN:**
We will probably still have cases that do not fit well into this system. For example, an ability where any player that collides with/touches another receives a GameplayEffect that slows them and turns their material blue. Since we can't send Server RPCs every time this happens, there is no way to correlate the gameplay effect side effects between client and server.

One approach may be to think about a weaker form of prediction — one where there is not a fresh prediction key used and instead the server assumes the client will predict all side effects from an entire ability. This would at least solve the "redo" problem but would not solve the "completeness" problem. If the client side prediction could be made as minimal as possible — for example only predicting an initial particle effect rather than predicting state and attribute changes — then the problems get less severe.

One can envision a weak prediction mode which is what certain abilities fall back to when there is no fresh prediction key that can accurately correlate side effects. When in weak prediction mode, perhaps only certain actions can be predicted — for example `GameplayCue` execute events, but not `OnAdded/OnRemove` events.

**CN:**
仍然可能存在一些不适合该系统的场景。例如，某个 Ability 使所有与玩家碰撞/接触的玩家都受到减速并将材质变蓝的 GameplayEffect。由于无法每次都发送 Server RPC，客户端和服务器之间无法关联 GameplayEffect 副作用。

一种思路是引入"弱预测"模式——不使用新鲜的预测键，而是让服务器假设客户端会预测整个 Ability 的所有副作用。这至少能解决"Redo"问题，但不能解决"完整性（Completeness）"问题。如果客户端预测尽可能精简——例如只预测初始粒子效果而不预测状态和属性变化——问题就不那么严重了。

可以设想一种弱预测模式，作为当没有可用的可准确关联副作用的新鲜预测键时某些 Ability 的回退方案。在弱预测模式下，也许只有某些动作可以被预测——例如 `GameplayCue` 的执行事件，而非 `OnAdded/OnRemove` 事件。



