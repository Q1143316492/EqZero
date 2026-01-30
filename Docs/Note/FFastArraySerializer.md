# FFastArraySerializer

是 Unreal Engine 中用于优化 **由结构体组成的数组（TArray）** 网络复制的一套机制。它主要用于解决普通 TArray 在网络同步时的性能瓶颈和逻辑痛点（特别是 Gameplay 框架中广泛使用的 GameplayEffects, GameplayTags 等）。

## 1. 为什么这么设计？ (Design Rationale)

普通的 TArray 网络复制（Generic Array Replication）非常简单粗暴：它通常依赖于索引（Index）。

- 痛点 1：插入/删除导致的错位。如果你删除了第 0 个元素，后面所有元素的索引都会变。服务端发给客户端说“更新第 5 个元素”，客户端因为索引整体前移，可能更新到了原本的第 6 个元素上。为了解决这个问题，普通复制通常需要发送全量数据或大量冗余数据。

- 痛点 2：回调缺失。普通数组也就是数值同步，客户端收到数据变了就是变了，很难知道具体是“哪个元素被添加了”或“哪个元素被移除了”，从而无法触发如“播放特效”、“移除Buff”等游戏逻辑。

- 痛点 3：带宽浪费。对于大数组，如果只改了一个元素的属性，普通复制可能需扫描整个数组。

FFastArraySerializer 的设计核心是：

1. 基于 ID（ReplicationID）而非索引：每个元素都有一个唯一的 ID。即使数组顺序变了，只要 ID 没变，系统就知道它是同一个对象。

2. 增量更新（Delta Serialization）：只发送变化的元素。

3. 事件驱动：能够精确通知客户端 Add, Remove, Change 事件。

## 2. 工作流程

Engine\Source\Runtime\Net\Core\Classes\Net\Serialization\FastArraySerializer.h
这个文件开头有教你怎么用

```cpp
/** Step 1: Make your struct inherit from FFastArraySerializerItem */
USTRUCT()
struct FExampleItemEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	// Your data:
	UPROPERTY()
	int32		ExampleIntProperty;	

	UPROPERTY()
	float		ExampleFloatProperty;

	/** 
	 * Optional functions you can implement for client side notification of changes to items; 
	 * Parameter type can match the type passed as the 2nd template parameter in associated call to FastArrayDeltaSerialize
	 * 
	 * NOTE: It is not safe to modify the contents of the array serializer within these functions, nor to rely on the contents of the array 
	 * being entirely up-to-date as these functions are called on items individually as they are updated, and so may be called in the middle of a mass update.
     * 可以用于接收修改通知，但不要修改容器的内容，而且内容可能不完全是最新的
	 */
	void PreReplicatedRemove(const struct FExampleArray& InArraySerializer);
	void PostReplicatedAdd(const struct FExampleArray& InArraySerializer);
	void PostReplicatedChange(const struct FExampleArray& InArraySerializer);

	// Optional: debug string used with LogNetFastTArray logging
	FString GetDebugString();

};
```

```cpp
/** Step 2: You MUST wrap your TArray in another struct that inherits from FFastArraySerializer */
USTRUCT()
struct FExampleArray: public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FExampleItemEntry>	Items;	/** Step 3: You MUST have a TArray named Items of the struct you made in step 1. */

	/** Step 4: Copy this, replace example with your names */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
	   return FFastArraySerializer::FastArrayDeltaSerialize<FExampleItemEntry, FExampleArray>( Items, DeltaParms, *this );
	}
};
```
你需要完成一个有Array的结构, 并实现 NetDeltaSerialize 函数。
写法是固定的

```cpp
/** Step 5: Copy and paste this struct trait, replacing FExampleArray with your Step 2 struct. */
template<>
struct TStructOpsTypeTraits< FExampleArray > : public TStructOpsTypeTraitsBase2< FExampleArray >
{
       enum 
       {
			WithNetDeltaSerializer = true,
       };
};
```

```cpp
/** 第 6 步及后续步骤：
- 声明一个 FExampleArray（第 2 步）类型的 UPROPERTY。
- 当你更改数组中的某个元素时，必须对 FExampleArray 调用 MarkItemDirty 函数。你需要传入你弄脏的元素的引用。
参见 FFastArraySerializer::MarkItemDirty。
- 如果你从数组中移除某些内容，必须对 FExampleArray 调用 MarkArrayDirty 函数。
- 在你的类的 GetLifetimeReplicatedProps 函数中，使用 DOREPLIFETIME (你的类名，你的数组结构体属性名);
你可以在你的结构体（第 1 步）中提供以下函数，以在添加 / 删除 / 移除之前获得通知：
-void PreReplicatedRemove(const FFastArraySerializer& Serializer)
-void PostReplicatedAdd(const FFastArraySerializer& Serializer)
-void PostReplicatedChange(const FFastArraySerializer& Serializer)
-void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters)
就是这样！
 */ 
```



## 3. 网络序列化原理与 Fast TArray 实现细节

*(这部分内容翻译自 Engine 源码 `FastArraySerializer.h` 中的注释，解释了 Unreal 网络序列化的底层机制)*

### 网络序列化概览 (An Overview of Net Serialization)

一切始于 `UNetDriver::ServerReplicateActors`。
系统选择要复制的 Actor，创建 Actor Channel，然后调用 `UActorChannel::ReplicateActor`。
`ReplicateActor` 最终负责决定哪些属性发生了变化，并构建要发送给客户端的 `FOutBunch`。

`UActorChannel` 有两种方式来决定哪些属性需要发送：

1.  **传统方式（基于扁平缓冲区）**：
    即 `UActorChannel::Recent`，这是一个 `TArray<uint8>` 缓冲区，代表了 Actor 属性的一块扁平内存。如果你知道 FProperty 的偏移量，这块内存字面上可以直接被转换为 `AActor*` 并读取属性值。
    `Recent` 缓冲区代表了使用此 Actor Channel 的客户端**当前所拥有**的值。我们将 `Recent` 与当前值进行比较，从而决定发送什么。
    *   这就很适合“原子”属性：ints, floats, object* 等。
    *   这完全不适合“动态”属性，比如 `TArrays`。`TArray` 存储了 Num/Max，但数据是指向堆内存的指针。数组数据没法塞进扁平的 `Recent` 缓冲区里。（对于这些属性，“动态”可能不是个好名字）。

2.  **动态状态映射**：
    为了解决这个问题，`UActorChannel` 还有一个用于“动态”状态的 TMap：`UActorChannel::RecentDynamicState`。这个 Map 允许我们根据属性的 RepIndex 查找其“基础状态（Base State）”。

### NetSerialize 与 NetDeltaSerialize

1.  **NetSerialize**:
    那些能塞进扁平 `Recent` 缓冲区的属性完全可以使用 `NetSerialize` 进行序列化。`NetSerialize` 只是向 `FArchive` 进行读写。
    因为复制系统可以直接查看 `Recent[]` 缓冲区并进行直接内存比较，它能知道哪些属性脏了（Dirty）。`NetSerialize` 只需要负责读写数据。

2.  **NetDeltaSerialize**:
    动态属性只能通过 `NetDeltaSerialize` 进行序列化。`NetDeltaSerialize` 是基于给定的基础状态（Base State）进行的序列化，它会产生：
    *   一个“增量（Delta）”状态（发送给客户端）。
    *   一个“完整（Full）”状态（保存下来作为未来增量序列化的基础状态）。
    `NetDeltaSerialize` 本质上同时执行了**差异比对（Diffing）**和序列化。它必须执行差异比对，这样才能知道属性的哪些部分需要发送。

### 基础状态与动态属性复制 (Base States)

*   对于复制系统 / `UActorChannel` 而言，基础状态可以是任何东西。它只认 `INetDeltaBaseState*` 接口。
*   `UActorChannel::ReplicateActor` 会最终决定是调用 `FProperty::NetSerializeItem` 还是 `FProperty::NetDeltaSerializeItem`。
*   如上所述，`NetDeltaSerialize` 接收一个额外的基础状态，并产生一个差异状态和一个完整状态。产生的完整状态将用作下次增量序列化的基础状态。
*   `INetDeltaBaseState` 是在 `NetDeltaSerialize` 函数内部创建的。复制系统 / `UActorChannel` 不需要知道细节。

目前其实有两种形式的增量序列化：**通用复制 (Generic Replication)** 和 **快速数组复制 (Fast Array Replication)**。

### 通用增量复制 (Generic Delta Replication)

通用增量复制由 `FStructProperty::NetDeltaSerializeItem`, `FArrayProperty::NetDeltaSerializeItem` 等实现。
它的工作原理是：首先 `NetSerialize` 对象当前的状态（即“完整”状态），然后使用 `memcmp` 将其与之前的基础状态进行比较。
`FStructProperty` 和 `FArrayProperty` 通过迭代它们的字段或数组元素并调用 `FProperty` 函数来工作，同时嵌入元数据。

例如 `FArrayProperty` 基本上是这样写的：
> “数组现在有 X 个元素” -> “这是元素 Y” -> FProperty::NetDeltaSerialize 的输出 -> “这是元素 Z” -> 等等

通用数据复制是处理 `FArrayProperty` 和 `FStructProperty` 序列化的“默认”方式。这适用于任何包含子属性的数组或结构体，只要这些属性能够进行 `NetSerialize`。

### 自定义网络增量序列化 (Custom Net Delta Serialization)

自定义网络增量序列化通过结构体特征（Trait）系统工作。如果一个结构体拥有 `WithNetDeltaSerializer` 特征，或者是 `FStructProperty::NetDeltaSerializeItem`，则会调用其原本的 `NetDeltaSerialize` 函数，而不是走通用增量复制的代码路径。

### 快速 TArray 复制 (Fast TArray Replication)

Fast TArray Replication 是通过自定义网络增量序列化实现的。它不再使用扁平的 `TArray` 缓冲区来表示状态，而是只关心一个 **ID 和 ReplicationKeys** 的 Map。
ID 映射到数组中的项，所有项都在 `FFastArraySerializerItem` 中定义了一个 `ReplicationID` 字段。
`FFastArraySerializerItem` 还有一个 `ReplicationKey` 字段。当使用 `MarkItemDirty` 将项标记为脏时，它们会被赋予一个新的 `ReplicationKey`，如果还没有 `ReplicationID`，则会分配一个。

#### FastArrayDeltaSerialize (具体流程)

**服务端序列化（写入）期间**：
我们比较旧的基础状态（例如旧的 ID<->Key Map）与数组的当前状态。
*   如果 ID 丢失了，我们将其作为**删除**写入数据包。
*   如果是新的或已更改（Key 变了），我们将其作为**更改**写入，并附带其状态（通过 `NetSerialize` 调用序列化）。

例如，实际写入的内容可能如下所示：
> “数组有 X 个更改的元素，Y 个删除的元素” -> “元素 A 更改了” -> 结构体项其余部分的 NetSerialize 输出 -> “元素 B 被删除了” -> 等等

注意：`ReplicationID` 是被复制且在客户端和服务器之间同步的。**数组索引（Index）则不是**。

**客户端序列化（读取）期间**：
客户端读取更改的元素数量和删除的元素数量。它还会构建一个 `ReplicationID` -> `当前数组本地索引` 的映射。
当它反序列化 ID 时，它会查找该元素，然后执行所需操作（必要时创建、序列化当前状态或删除）。

内部结构体的增量序列化现在默认是启用的。这意味着当 `ReplicationKey` 更改时，我们会将结构体的当前状态与上次发送的状态进行比较，跟踪更改列表，并仅发送像标准复制路径那样更改的属性。
如果这导致特定 FastArray 类型出现问题，可以通过在构造函数中调用 `FFastArraySerializer::SetDeltaSerializationEnabled(false)` 来禁用它。

`ReplicationID` 和 `ReplicationKeys` 由 `FFastArraySerializer` 上的 `MarkItemDirty` 函数设置。它们只是随着事物变化而按顺序分配的 `int32`。除了唯一性之外，它们没有什么特别之处。