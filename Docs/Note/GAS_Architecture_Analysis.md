# EqZero GAS Architecture — Comprehensive Analysis Report

> Based on complete reading of ~90+ source files in the EqZero project (Lyra-derived, UE 5.6).  
> Research-only document — no code changes proposed.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Lyra Design Patterns](#2-lyra-design-patterns)
3. [AbilitySet Data-Driven Approach](#3-abilityset-data-driven-approach)
4. [Equipment → Ability → Weapon Pipeline](#4-equipment--ability--weapon-pipeline)
5. [Custom GameplayEffectContext](#5-custom-gameplayeffectcontext)
6. [Attribute Design](#6-attribute-design)
7. [Custom AbilityCost System](#7-custom-abilitycost-system)
8. [Input Binding to Abilities](#8-input-binding-to-abilities)
9. [Tag Management Patterns](#9-tag-management-patterns)
10. [Custom Abilities](#10-custom-abilities)
11. [Existing Documentation Notes](#11-existing-documentation-notes)

---

## 1. Architecture Overview

### 1.1 ASC Ownership Model

The `UEqZeroAbilitySystemComponent` (ASC) **lives on `AEqZeroPlayerState`**, not on the Pawn. The Pawn is the *Avatar Actor* — it changes on respawn while the ASC persists across deaths. This is the standard Lyra pattern for multiplayer shooters.

```
AEqZeroPlayerState
  └── UEqZeroAbilitySystemComponent  (Owner + persistence)

AEqZeroCharacter (Avatar)
  ├── UEqZeroPawnExtensionComponent  (holds ASC ref, manages lifecycle)
  ├── UEqZeroHealthComponent         (bridges ASC health events to actor)
  ├── UEqZeroHeroComponent           (input binding, camera mode)
  └── UEqZeroCameraComponent
```

**Lifecycle flow:**
1. `ExperienceManagerComponent` loads Experience on GameState
2. Experience sets PawnData via GameMode → PlayerState → PawnExtensionComponent
3. PawnExtensionComponent drives init state machine: `Spawned → DataAvailable → DataInitialized → GameplayReady`
4. On `DataInitialized`, HeroComponent calls `PawnExtComp->InitializeAbilitySystem(PS->GetASC(), PS)` which calls `ASC->InitAbilityActorInfo(PS, Pawn)`
5. Character's `OnAbilitySystemInitialized` → HealthComponent binds to ASC
6. HeroComponent binds input, sets camera delegate

### 1.2 System Layers

| Layer | Components | Responsibility |
|-------|-----------|----------------|
| **Experience** | ExperienceManagerComponent, ExperienceDefinition, ActionSets | Loads GameFeature plugins, executes GameFeatureActions |
| **GameFeature Actions** | AddAbilities, AddInputBinding | Grants abilities/attributes/sets, binds input configs |
| **Character** | PawnExtensionComponent, HeroComponent, HealthComponent | Init state machine, ASC lifecycle, input, health bridge |
| **AbilitySystem** | ASC, AbilitySet, TagRelationshipMapping, GlobalAbilitySystem | Core GAS management, activation groups, tag relationships |
| **Equipment** | EquipmentDefinition, EquipmentInstance, EquipmentManagerComponent | Grants AbilitySets, spawns actors, manages equipment list |
| **Inventory** | InventoryManagerComponent, ItemDefinition, ItemInstance, Fragments | Item data, tag stacks (ammo), fragment system |
| **Weapons** | RangedWeaponInstance, WeaponStateComponent, RangedWeapon ability | Spread/heat, targeting, hit confirmation |

### 1.3 Network Replication Strategy

- **FFastArraySerializer** used for: Equipment list, Inventory list, GameplayTagStack
- ASC on PlayerState — persists across respawns, only needs re-avataring
- `FSharedRepMovement` with compressed acceleration (polar coordinates) for bandwidth optimization
- Equipment instances replicated as subobjects (`ReplicateSubobjects`)
- Inventory items replicated as registered subobjects (`AddReplicatedSubObject`)
- `GameplayMessageSubsystem` for local (non-replicated) event broadcasting

---

## 2. Lyra Design Patterns

### 2.1 Experience System

The Experience system is the **top-level orchestrator** that defines what a game mode looks like:

**`UEqZeroExperienceDefinition`** (UPrimaryDataAsset):
- `DefaultPawnData` — which PawnData (and thus which abilities, input, camera) to use
- `GameFeaturesToEnable` — list of plugin names to load/activate
- `Actions` — array of instanced `UGameFeatureAction` (AddAbilities, AddInputBinding, etc.)
- `ActionSets` — reusable groups of actions (`UEqZeroExperienceActionSet`)

**`UEqZeroExperienceManagerComponent`** (on GameState):
- State machine: `Unloaded → Loading → LoadingGameFeatures → ExecutingActions → Loaded`
- Async loads bundles (client/server-aware), then loads GameFeature plugins
- Executes all actions (`OnGameFeatureRegistering → OnGameFeatureLoading → OnGameFeatureActivating`)
- Three priority-level delegates: `OnExperienceLoaded_HighPriority/Normal/LowPriority`
- `CallOrRegister_OnExperienceLoaded` pattern — call immediately if loaded, else queue
- Replicates `CurrentExperience` to clients via `OnRep_CurrentExperience`
- On `EndPlay`, deactivates all plugins and actions (FILO deactivation noted as TODO)

### 2.2 Init State System

Uses `IGameFrameworkInitStateInterface` with a 4-stage state chain:

```
InitState_Spawned → InitState_DataAvailable → InitState_DataInitialized → InitState_GameplayReady
```

**PawnExtensionComponent** (coordinator):
- `Spawned`: Pawn exists
- `DataAvailable`: PawnData set + Controller present (authority/local) 
- `DataInitialized`: ALL registered features reached DataAvailable
- `GameplayReady`: Always passes

**HeroComponent** (participant):
- `Spawned`: Pawn exists
- `DataAvailable`: PlayerState exists + Controller paired + InputComponent ready (local human)
- `DataInitialized`: PawnExtension reached DataInitialized + PlayerState valid
- `GameplayReady`: Always passes

Both components watch each other via `BindOnActorInitStateChanged` and `CheckDefaultInitializationForImplementers`, creating a cooperative multi-component initialization system.

### 2.3 Tag Relationship Mapping

**`UEqZeroAbilityTagRelationshipMapping`** (UDataAsset, configured in PawnData):

Each entry maps an `AbilityTag` to:
- `AbilityTagsToBlock` — prevent activation of abilities with these tags
- `AbilityTagsToCancel` — cancel running abilities with these tags  
- `ActivationRequiredTags` — extra required tags for abilities with this tag
- `ActivationBlockedTags` — extra blocked tags for abilities with this tag

The ASC overrides `ApplyAbilityBlockAndCancelTags` and injects additional block/cancel tags. It also overrides `GetAdditionalActivationTagRequirements` to add required/blocked tags. This enables **data-driven ability relationships** without hardcoding in ability classes.

### 2.4 Global Ability System

**`UEqZeroGlobalAbilitySystem`** (UWorldSubsystem):
- Tracks all registered ASCs (registered on avatar actor set)
- `ApplyAbilityToAll` / `RemoveAbilityFromAll` — system-wide ability grants
- `ApplyEffectToAll` / `RemoveEffectFromAll` — system-wide effect applications
- New ASCs auto-receive currently active global effects on registration
- Use case: warmup invincibility, global modifiers, match-wide effects

### 2.5 Modular Character (AModularCharacter)

The base character inherits from `AModularCharacter` plus implements:
- `IAbilitySystemInterface` — standard GAS interface, delegates to PawnExtComponent→ASC
- `IGameplayCueInterface` — for direct cue handling
- `IGameplayTagAssetInterface` — tag queries delegate to ASC
- `IEqZeroTeamAgentInterface` — team system (TODO: not yet fully implemented)

---

## 3. AbilitySet Data-Driven Approach

### 3.1 Structure

**`UEqZeroAbilitySet`** (UPrimaryDataAsset) contains three arrays:

```cpp
TArray<FEqZeroAbilitySet_GameplayAbility> GrantedGameplayAbilities;  // GA class + level + InputTag
TArray<FEqZeroAbilitySet_GameplayEffect>  GrantedGameplayEffects;    // GE class + level
TArray<FEqZeroAbilitySet_AttributeSet>    GrantedAttributes;         // AS class
```

### 3.2 Grant Flow

`GiveToAbilitySystem(ASC, OutGrantedHandles)`:
1. **Attribute Sets first** — `NewObject<UAttributeSet>` with Pawn as outer, then `ASC->AddAttributeSetSubobject`
2. **Abilities second** — `ASC->GiveAbility` with `FGameplayAbilitySpec` that:
   - Sets `SourceObject = AbilitySet` for tracking
   - Adds `InputTag` via `Spec.GetDynamicSpecSourceTags().AddTag(InputTag)` — this is how the ASC later matches abilities to input
3. **Effects last** — `ASC->ApplyGameplayEffectToSelf`

### 3.3 Cleanup

`FEqZeroAbilitySet_GrantedHandles::TakeFromAbilitySystem(ASC)`:
- Removes all granted abilities, effects, and attribute sets in reverse
- Authority-only operations throughout

### 3.4 Where AbilitySets Are Granted

1. **PawnData.AbilitySets** → granted by `GameFeatureAction_AddAbilities` on PlayerState ability ready
2. **EquipmentDefinition.AbilitySetsToGrant** → granted when equipment is equipped
3. **GameFeatureAction_AddAbilities.AbilitySets** → granted by experience/game feature system

---

## 4. Equipment → Ability → Weapon Pipeline

### 4.1 Equipment Definitions

**`UEqZeroEquipmentDefinition`** (UObject):
- `InstanceType` — class of `UEqZeroEquipmentInstance` to create (or weapon subclass)
- `AbilitySetsToGrant` — array of AbilitySets to grant when equipped
- `ActorsToSpawn` — array of `{ActorClass, AttachSocket, AttachTransform}`

### 4.2 Equipment Manager

**`UEqZeroEquipmentManagerComponent`** (PawnComponent):
- Uses `FEqZeroEquipmentList` (FFastArraySerializer) for replicated equipment list
- **EquipItem(EquipmentDefinition)**:
  1. Creates `UEqZeroEquipmentInstance` (outer = Pawn for replication)
  2. Grants all AbilitySets → stored in `FEqZeroAppliedEquipmentEntry.GrantedHandles`
  3. Spawns all actors → stored in `EquipmentInstance.SpawnedActors` (replicated)
  4. Calls `EquipmentInstance->OnEquipped()`
  5. Marks item dirty for replication
- **UnequipItem(EquipmentInstance)**:
  1. Calls `OnUnequipped()`
  2. Removes granted ability sets
  3. Destroys spawned actors
  4. Removes from list, marks array dirty

### 4.3 Weapon Instances

**`UEqZeroWeaponInstance`** (extends EquipmentInstance):
- Animation layer class selection
- Device properties (PS5 adaptive triggers) 
- Time-since-last-fire tracking

**`UEqZeroRangedWeaponInstance`** (extends WeaponInstance):
- Implements `IEqZeroAbilitySourceInterface` for attenuation queries
- **Spread system (heat-based)**:
  - `HeatToSpreadCurve` — maps heat (0-1) to spread angle (degrees)
  - `HeatToHeatPerShotCurve` — how much heat each shot adds (varies with current heat)
  - `HeatToCoolDownPerSecondCurve` — cool-down rate varies with heat
  - Multipliers: standing still, crouching, jumping/falling, aiming (ADS)
  - `bAllowFirstShotAccuracy` — zero spread if heat = 0 and conditions met
- `BulletsPerCartridge` — shotgun multi-bullet support
- `BulletTraceSweepRadius` — sweep trace radius for aim assist
- `DistanceDamageFalloff` — curve for distance attenuation
- `MaterialDamageMultiplier` — physical material → damage multiplier map
- `GetDistanceAttenuation(Distance)` → evaluates DistanceDamageFalloff curve
- `GetPhysicalMaterialAttenuation(PhysMat, Tags)` → evaluates MaterialDamageMultiplier map

### 4.4 Ability_FromEquipment Base Class

**`UEqZeroGameplayAbility_FromEquipment`**:
- `GetAssociatedEquipment()` — retrieves EquipmentInstance from `CurrentSpecHandle.SourceObject`
- `GetAssociatedItem()` — retrieves InventoryItemInstance from `Equipment->Instigator`
- Data validation enforces instanced-per-actor policy
- Base class for weapon abilities (RangedWeapon, ReloadMagazine) and equipment-bound abilities

### 4.5 Complete Pipeline

```
Experience loads → GameFeatureAction_AddAbilities → grants AbilitySets to PlayerState
                                                      ↓
InventoryManager.AddItem(WeaponDef) → creates ItemInstance with tags (ammo stats via Fragment_SetStats)
                                                      ↓
EquipmentManager.EquipItem(EquipmentDef) → creates WeaponInstance (Instigator = ItemInstance)
                                          → grants WeaponAbilitySet (fire, reload, ADS)
                                          → spawns weapon actor (mesh) attached to socket
                                                      ↓
Player presses fire input → ASC matches InputTag to RangedWeapon ability
                          → ability gets WeaponInstance via FromEquipment.GetAssociatedEquipment()
                          → queries spread, bullets per cartridge, attenuation
                          → fires traces, applies damage via DamageExecution
                          → ItemTagStack cost (ammo) deducted
```

---

## 5. Custom GameplayEffectContext

### 5.1 FEqZeroGameplayEffectContext

Extends `FGameplayEffectContext` with:

| Field | Type | Purpose |
|-------|------|---------|
| `CartridgeID` | `int32` | Unique ID per cartridge (shot). For shotguns: all pellets in one shot share the same CartridgeID, allowing de-duplication of damage/effects |
| `AbilitySourceObject` | `TWeakObjectPtr<const UObject>` | Weak pointer to `IEqZeroAbilitySourceInterface` implementor (typically `UEqZeroRangedWeaponInstance`), used by DamageExecution for attenuation queries |

**Key behaviors:**
- `GetPhysicalMaterial()` — extracts from HitResult
- `NetSerialize` — extends base serialization (CartridgeID notably NOT replicated — only used post-activation)
- Custom `Duplicate()` for proper deep copy

### 5.2 Allocation Override

**`UEqZeroAbilitySystemGlobals`** overrides `AllocGameplayEffectContext()` to return `FEqZeroGameplayEffectContext`. This ensures ALL gameplay effect contexts in the system are the custom type.

### 5.3 Target Data Extension

**`FEqZeroGameplayAbilityTargetData_SingleTargetHit`** extends engine target data with `CartridgeID`. The `AddTargetDataToContext()` method populates the CartridgeID into the custom effect context, linking individual hit results to their cartridge for damage execution.

---

## 6. Attribute Design

### 6.1 Two-Set Separation

| Set | Attributes | Role | Owner |
|-----|-----------|------|-------|
| **EqZeroHealthSet** | Health, MaxHealth, Healing (meta), Damage (meta) | Target-side: receives damage/healing | Avatar (Pawn) |
| **EqZeroCombatSet** | BaseDamage, BaseHeal | Source-side: provides base values | Instigator |

This separation means:
- **Source** has CombatSet attributes that define how much damage they deal
- **Target** has HealthSet attributes that define how much health they have
- DamageExecution reads from source CombatSet, writes to target HealthSet

### 6.2 HealthSet — Meta Attribute Pattern

`Healing` and `Damage` are **meta attributes** — they are not directly replicated but act as transient calculation channels:

**PreGameplayEffectExecute:**
- Checks `DamageImmunity` tag → zeroes Damage
- Checks `GodMode` cheat → zeroes Damage

**PostGameplayEffectExecute:**
- Applies `Damage` → subtracts from Health
- Applies `Healing` → adds to Health (clamped to MaxHealth)
- Zeroes meta attributes after processing
- Broadcasts `FEqZeroVerbMessage` (Damage verb) via GameplayMessageSubsystem
- Fires `OnOutOfHealth` delegate when Health reaches 0
- Fires `OnHealthChanged` delegate on any Health change

**PreAttributeBaseChange / PreAttributeChange:**
- Clamps Health to [0, MaxHealth]

**PostAttributeChange:**
- When MaxHealth decreases, adjusts Health if it exceeds new MaxHealth

### 6.3 CombatSet

Simple source-side storage. `BaseDamage` and `BaseHeal` are replicated. Used as inputs to DamageExecution.

### 6.4 Damage Execution

**`UEqZeroDamageExecution`** (GameplayEffectExecutionCalculation):
1. Captures `BaseDamage` from source CombatSet
2. Gets `IEqZeroAbilitySourceInterface` from effect context's AbilitySourceObject
3. Calculates distance from effect context origin (or EffectCauser location) to target
4. Queries `GetDistanceAttenuation(distance)` → multiplier
5. Queries `GetPhysicalMaterialAttenuation(physMat, tags)` → multiplier
6. Final damage = BaseDamage × distanceAttenuation × materialAttenuation
7. Outputs to HealthSet `Damage` attribute
8. Wrapped in `#if WITH_SERVER_CODE` — server-only calculation

---

## 7. Custom AbilityCost System

### 7.1 Architecture

**`UEqZeroAbilityCost`** (abstract UObject, DefaultToInstanced, EditInlineNew):
- `CheckCost(Ability, Spec)` → returns bool
- `ApplyCost(Ability, Spec, Handle)` → applies the cost
- `ShouldOnlyApplyCostOnHit()` → deferred cost application (e.g., ammo only consumed on hit)

The base `UEqZeroGameplayAbility` holds:
```cpp
TArray<TObjectPtr<UEqZeroAbilityCost>> AdditionalCosts;
```

**CheckCost flow** (overridden `CanActivateAbility`):
1. Base GAS cost check (GE-based)
2. For each AdditionalCost: `Cost->CheckCost()` — all must pass

**ApplyCost flow** (overridden `ApplyCost`):
1. Base GAS cost apply
2. For each AdditionalCost: if `!ShouldOnlyApplyCostOnHit()`, apply immediately

For hit-only costs, the weapon ability applies them manually after confirming target data.

### 7.2 Implementations

**`UEqZeroAbilityCost_ItemTagStack`**:
- Checks/applies cost against the associated item's GameplayTagStack
- Gets item via `FromEquipment->GetAssociatedItem()->GetStatTagStackCount(Tag)`
- `Quantity` is `FScalableFloat` (can scale with ability level)
- Adds configurable `FailureTag` on check failure (shown as user message)
- **Primary use: ammunition consumption** (magazine ammo per shot)

**`UEqZeroAbilityCost_PlayerTagStack`**:
- Same pattern but checks PlayerState's tag stack count
- Useful for player-level resources (grenades, special charges)

**`UEqZeroAbilityCost_InventoryItem`**:
- Placeholder (TODO) for consuming inventory items as costs

---

## 8. Input Binding to Abilities

### 8.1 Input Configuration

**`UEqZeroInputConfig`** (UDataAsset) contains two arrays:

```cpp
TArray<FEqZeroInputAction> NativeInputActions;    // InputAction + InputTag → hard-coded handlers
TArray<FEqZeroInputAction> AbilityInputActions;   // InputAction + InputTag → ability activation
```

### 8.2 Binding Flow

1. **HeroComponent::InitializePlayerInput** is called during `DataInitialized` transition
2. Clears all mappings, adds DefaultInputMappings (IMCs with priorities)
3. Creates `UEqZeroInputComponent` casts and calls:
   - `BindNativeAction(InputConfig, Tag, TriggerEvent, Handler)` — for Move, Look, Crouch, AutoRun
   - `BindAbilityActions(InputConfig, PressedFunc, ReleasedFunc)` — for all ability inputs
4. Each ability input action is bound to:
   - `Triggered` → `Input_AbilityInputTagPressed(InputTag)` → `ASC->AbilityInputTagPressed(Tag)`
   - `Completed` → `Input_AbilityInputTagReleased(InputTag)` → `ASC->AbilityInputTagReleased(Tag)`

### 8.3 ASC Input Processing

The ASC does NOT use the engine's built-in `InputID` system. Instead:

**`AbilityInputTagPressed(InputTag)`**:
- Finds all ability specs matching InputTag via `DynamicSpecSourceTags`  
- For `OnInputTriggered` policy: adds to `InputPressedSpecHandles` (processed this frame)
- For `WhileInputActive` policy: adds to `InputHeldSpecHandles` (persistent)

**`AbilityInputTagReleased(InputTag)`**:
- Finds matching specs
- Adds to `InputReleasedSpecHandles`
- Removes from `InputHeldSpecHandles`

**`ProcessAbilityInput()`** (called each frame):
1. Process `InputPressedSpecHandles` → `TryActivateAbility` for each
2. Process `InputHeldSpecHandles` → `TryActivateAbility` for each (keeps held abilities active)
3. Process `InputReleasedSpecHandles` → `ASC->InputReleased` for each (for WaitInputRelease tasks)
4. Clears pressed/released arrays

### 8.4 GameFeature Input Binding

**`UGameFeatureAction_AddInputBinding`**:
- Registers as extension handler on `APawn::StaticClass()`
- On `BindInputsNow` event (from HeroComponent), calls `HeroComponent->AddAdditionalInputConfig()`
- Allows experiences to inject additional ability input bindings at runtime

---

## 9. Tag Management Patterns

### 9.1 Central Tag Registry

**`EqZeroGameplayTags`** namespace declares all project tags as `FGameplayTag` statics:

| Category | Examples | Purpose |
|----------|---------|---------|
| `Ability_ActivateFail.*` | Cooldown, Cost, IsDead, TagsBlocked, TagsMissing, Networking | Failure reason tags shown to player |
| `Ability_Behavior.*` | SurvivesDeath | Tags that modify ability lifecycle |
| `InputTag.*` | Move, Look_Mouse, Look_Stick, Crouch, AutoRun, Dash, ADS | Input-to-ability mapping keys |
| `InitState.*` | Spawned, DataAvailable, DataInitialized, GameplayReady | Component initialization states |
| `GameplayEvent.*` | Death, Reset, RequestReset, ReloadDone | Gameplay event triggers |
| `SetByCaller.*` | Damage, Healing | GE magnitude channels |
| `Status.*` | Crouching, Death_Dying, Death_Dead, ADS, Sprinting | State tags on ASC |
| `Movement_Mode.*` | Walking, NavWalking, Falling, Swimming, Flying, Custom | Movement mode tracking |
| `Cheat.*` | GodMode, UnlimitedHealth | Debug tags |
| `Lyra.Weapon.*` | MagazineAmmo, MagazineSize, SpareAmmo | Ammo tag stacks |
| `Ability_Respawn.*` | Duration | Respawn system |
| `Event_Movement.*` | Dash | Movement event tags |

### 9.2 Movement Mode Tags

```cpp
TMap<uint8, FGameplayTag> MovementModeTagMap;      // EMovementMode → Tag
TMap<uint8, FGameplayTag> CustomMovementModeTagMap; // Custom mode → Tag
```

Character's `OnMovementModeChanged` → removes old tag, adds new tag via `SetLooseGameplayTagCount`. This enables abilities to react to movement mode changes through tag requirements.

### 9.3 GameplayTagStack

**`FGameplayTagStackContainer`** (FFastArraySerializer):
- Replicated tag-to-count map using `TArray<FGameplayTagStack>` (with `FFastArraySerializerItem`)
- `AddStack(Tag, Count)` — increments existing or creates new entry
- `RemoveStack(Tag, Count)` — decrements, removes entry if count reaches 0
- `GetStackCount(Tag)` — query current count
- Used by: `UEqZeroInventoryItemInstance` (ammo tracking), `AEqZeroPlayerState` (player resources)

### 9.4 Dynamic Tag GE System

The ASC provides:
- `AddDynamicTagGameplayEffect(Tag)` → applies a minimal GE that grants the tag
- `RemoveDynamicTagGameplayEffect(Tag)` → removes by handle
- The GE class comes from `GameData` config (UEqZeroGameData::DynamicTagGameplayEffect)

### 9.5 Failure Tag Messaging

The base GameplayAbility has:
```cpp
TMap<FGameplayTag, FEqZeroAbilityFailureMessage> FailureTagToUserFacingMessages;
TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FailureTagToAnimMontage;
```

When ability activation fails, the ASC calls `NotifyAbilityFailed` which triggers user-facing messages and/or failure animations based on the specific failure tag.

---

## 10. Custom Abilities

### 10.1 Base GameplayAbility (UEqZeroGameplayAbility)

**Key defaults (differing from engine):**
- `ReplicationPolicy`: DoNotReplicate
- `InstancingPolicy`: InstancedPerActor
- `NetExecutionPolicy`: LocalPredicted  
- `NetSecurityPolicy`: ClientOrServer

**ActivationPolicy enum:**
- `OnInputTriggered` — standard press-to-activate
- `WhileInputActive` — activate on press, stays active while held
- `OnSpawn` — auto-activate when ability is granted

**ActivationGroup enum (exclusive ability system):**
- `Independent` — unlimited concurrent instances
- `Exclusive_Replaceable` — only one at a time, new one cancels old
- `Exclusive_Blocking` — only one at a time, blocks new activations

The ASC tracks `ActivationGroupCounts[3]` and checks compatibility in `IsActivationGroupBlocked()`.

**Additional features:**
- `AdditionalCosts` array — plugin-style cost system (see §7)
- `ActiveCameraMode` — set/clear via HeroComponent
- `ShouldOnlyApplyCostOnHit` support — deferred cost application
- `DoesAbilitySatisfyTagRequirements` override — integrates TagRelationshipMapping
- `MakeEffectContext` override — populates AbilitySource
- `ApplyAbilityTagsToGameplayEffectSpec` — adds `PhysicalMaterialWithTags` to captured tags

### 10.2 Death Ability (GA_Death)

- **Trigger**: `GameplayEvent.Death` (sent by HealthComponent when health reaches 0)
- **NetExecutionPolicy**: ServerInitiated
- **Behavior**:
  1. Cancels all abilities except `SurvivesDeath`-tagged
  2. Changes to `Exclusive_Blocking` group (prevents all other exclusive abilities)
  3. Calls `HealthComponent->StartDeath()` (sets death state)
  4. Sets DeathCameraMode on HeroComponent
  5. Fires GameplayCue with rich parameters (instigator, damage info via VerbMessage→CueParams conversion)
  6. Waits `DeathDuration` seconds, then calls `HealthComponent->FinishDeath()` (destruction)

### 10.3 Jump Ability

- **NetExecutionPolicy**: LocalPredicted
- Checks `CanJump()`
- UnCrouches before jumping if crouched
- Calls `Character->Jump()`, ends on release → `StopJumping()`

### 10.4 Reset Ability

- **Trigger**: `GameplayEvent.RequestReset`
- **NetExecutionPolicy**: ServerInitiated
- Cancels all abilities, resets ASC, broadcasts reset message

### 10.5 Dash Ability

- Extends `WithWidget` (for cooldown UI)
- **NetExecutionPolicy**: LocalPredicted
- Directional: 4 montages (Forward/Backward/Left/Right) selected by yaw delta between movement input and character facing
- `SelectDirectionalMontage()` — calculates angle between velocity/acceleration direction and actor rotation
- Uses `AbilityTask_ApplyRootMotionConstantForce` for the dash movement
- Server RPC for dedicated server authority
- Timer-based delayed end after root motion completes

### 10.6 Melee Ability

- Extends `WithWidget`
- **Authority-only hit detection**: capsule trace → validates with line trace (prevents through-wall hits)
- Root motion toward target (locks on)
- Applies DamageEffectClass GE with hit result in effect context

### 10.7 ADS (Aim Down Sights)

- **NetExecutionPolicy**: LocalPredicted
- Sets camera mode (ADS camera)
- Modifies `MaxWalkSpeed` (movement speed reduction)
- Adds ADS `InputMappingContext` at higher priority (sensitivity change)
- Plays zoom sounds (in/out)
- Broadcasts UI update via `GameplayMessageSubsystem`
- Uses `WaitInputRelease` task → ends on release

### 10.8 AutoRespawn

- **ActivationPolicy**: OnSpawn (auto-activates)
- **NetExecutionPolicy**: ServerOnly
- `bRetriggerInstancedAbility = true`
- Listens for death message (`GameplayMessageSubsystem`)
- On death:
  1. Broadcasts respawn duration message
  2. Waits configured delay
  3. Requests player restart via `GameMode->RequestPlayerRestartNextFrame()`
- Handles avatar EndPlay (clears death listener, re-binds)
- Tears down death listener on ability end

### 10.9 WithWidget (UI Extension Base)

- On `OnGiveAbility`: registers widget classes to UI extension points via `UUIExtensionSubsystem`
- On `OnRemoveAbility`: unregisters widget handles
- Uses `LocalPlayer` context for widget registration
- Subclassed by Dash, Melee (for cooldown/charge UI)

### 10.10 RangedWeapon Ability

- Most complex ability in the project (~671 lines)
- **Targeting system** with `EEqZeroAbilityTargetingSource` enum:
  - `CameraTowardsFocus` — camera forward with spreading
  - `PawnForward` / `PawnTowardsFocus` — pawn-based alternatives
  - `WeaponForward` / `WeaponTowardsFocus` — weapon socket-based
- **Trace logic**:
  - `TraceBulletsInCartridge()` — loops `BulletsPerCartridge` times
  - `DoSingleBulletTrace()`:
    1. Fine line trace first
    2. If no hit + SweepRadius > 0: sweep trace for aim assist
    3. Through-wall prevention: validates hit is reachable from weapon muzzle
  - `VRandConeNormalDistribution` — normal distribution spread within cone
- **Server confirmation flow**:
  - Client gathers target data → sends to server
  - `OnTargetDataReadyCallback` → validates → applies damage GE per hit
  - Applies deferred costs (ammo) only after data ready
  - `UEqZeroWeaponStateComponent` manages hit markers and server-side confirmation

### 10.11 ReloadMagazine Ability

- Extends `FromEquipment`
- **CanActivate checks**: magazine not full AND spare ammo > 0
- Adds firing block tag if magazine is empty
- Plays reload montage + waits for `GameplayEvent.ReloadDone` notify
- Authority-only ammo transfer: calculates transfer amount (min of needed/available), modifies SpareAmmo and MagazineAmmo tag stacks

---

## 11. Existing Documentation Notes

### 11.1 GameplayPrediction.md (394 lines, Chinese-English bilingual)

Comprehensive translation of GAS prediction system from engine source comments:

**Core Concepts:**
- **PredictionKey**: Unique client-generated ID, replicates client→server, server→originating-client-only
- **Prediction Window**: Valid during `ActivateAbility` callstack only (no cross-frame prediction)
- **Ability Activation Flow**: `TryActivateAbility → ServerTryActivateAbility → ClientActivateAbility(Failed/Succeed)`
  - On reject: immediate rollback via `FPredictionKeyDelegates`
  - On accept: wait for `ReplicatedPredictionKey` to catch up, then remove predictive side effects

**GameplayEffect Prediction:**
- GEs are side effects, not independently accepted/rejected
- Predicted only with valid prediction key
- Duplicate handling: skip "on applied" logic if matching predicted effect exists

**Attribute Prediction:**
- Delta prediction, not absolute (predict -10 mana, not 90 mana)
- Instant GEs treated as infinite-duration during prediction
- `REPNOTIFY_Always` + `GAMEPLAYATTRIBUTE_REPNOTIFY` macro for proper reaggregation

**Advanced Topics:**
- Dependency chains (X→Y→Z) via Base PredictionKey
- Additional prediction windows via `FScopedPredictionWindow`
- Limitations: meta attributes (Damage/Healing) can't be predicted, % multiplicative GE inaccuracy, rollback of chained activations not supported

### 11.2 UGameplayCueManager详解.md (682 lines, Chinese)

Detailed analysis covering:
- Core data structures (`FGameplayCuePendingExecute`, ObjectLibrary, PredictionKey)
- Initialization and asset loading flow (RuntimeObjectLibrary, UGameplayCueSet acceleration maps)
- Event types (Duration: OnActive/WhileActive/Removed vs Instant: Executed)
- Server-side flow for both one-shot and duration cues
- `FlushPendingCues` — batched RPC sending
- Client prediction flow
- Replication via `FActiveGameplayCueContainer`
- Dedicated server suppression
- Actor lifecycle and pooling
- Async loading and deferred execution  
- Tag translation system
- Non-replicated GameplayCues

### 11.3 FFastArraySerializer.md (192 lines, Chinese)

Covers:
- **Design rationale**: index-based problems (insertion/deletion misalignment), missing callbacks, bandwidth waste
- **Core design**: ID-based (ReplicationID) not index-based, delta serialization, event-driven (Add/Remove/Change callbacks)
- **Implementation steps**: Inherit FFastArraySerializerItem, wrap in FFastArraySerializer-derived struct, implement NetDeltaSerialize, add struct traits, use MarkItemDirty/MarkArrayDirty
- **Deep dive into UE net serialization**: Traditional flat buffer vs dynamic state map
- **NetSerialize vs NetDeltaSerialize**: Flat-buffer properties vs delta-state properties
- **Generic Delta Replication** vs **Fast TArray Replication**: ID+Key map approach, server write/client read flow, inner struct delta serialization

---

## Summary of Key Architectural Decisions

1. **ASC on PlayerState** — survives death, only avatar changes on respawn
2. **Tag-based input** — no engine InputID, abilities matched via DynamicSpecSourceTags
3. **Data-driven everything** — PawnData, AbilitySets, TagRelationshipMapping, InputConfig all configured as data assets
4. **Experience-driven modularity** — GameFeature plugins loaded per-experience, actions grant abilities/input dynamically
5. **Multi-component init state machine** — cooperative initialization prevents race conditions
6. **Separated source/target attributes** — CombatSet (damage source) vs HealthSet (damage target)
7. **Custom effect context** — CartridgeID for shotgun de-duplication, AbilitySource for attenuation queries
8. **Plugin-style cost system** — extensible per-ability costs via UEqZeroAbilityCost array
9. **Heat-based weapon spread** — realistic spread mechanics with multiple condition multipliers
10. **FFastArraySerializer everywhere** — Equipment, Inventory, TagStacks all use efficient delta replication
