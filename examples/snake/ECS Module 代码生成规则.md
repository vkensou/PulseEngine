# ECS Module 代码生成规则

## 1. 输入

- 读取 `.h` 头文件
- 解析 `PULSE_ECS_SYSTEM(PHASE=XXX)` 宏
- 提取宏后紧跟的函数签名: `返回类型 函数名(参数列表)`

## 2. system分类

system分为普通system和事件system。区别是参数带`pulse::event_reader<T>`是事件system，不带的是普通system。事件system只能有一个`pulse::event_reader<T>`。

system还可以分为实体system和管理器system。实体system是遍历所有满足条件的实体，一帧可能运行一次或多次，管理器system不遍历具体实体，一帧运行一次。

## 3. PULSE_ECS_SYSTEM宏

### 3.1 system运行时机

由PULSE_ECS_SYSTEM中的PHASE项指定。INIT表示initPipeline，UPDATE表示updatePipeline，IMGUI表示imguiPipeline。运行时机只影响普通system。但为了统一，事件system的签名也可以写PHASE，只是不起作用。
另外，system分类与运行时机无任何关系。不管是INIT还是UPDATE/IMGUI，都有可能是实体system或者管理器system。

### 3.2 immediate

当PULSE_ECS_SYSTEM中出现`IMMEDIATE`时，说明此system是immediate system，在后面注册system时需要附加`.immediate()`

## 4. system参数类型

system的参数类型有如下几种可能：

- `pulse::command_buffer`：命令缓冲
- `pulse::res<T>` 或 `pulse::res<const T>`：外部资源
- `pulse::singleton_query<T>`：单例查询
- `pulse::event_reader<T>`：事件读取器
- `pulse::event_writer<T>`：事件写入器
- `flecs::query<T...>`：附加查询
- `flecs::entity`：system主查询遍历的实体
- 其他组件：system主查询遍历的组件列表

参数获取时一律去掉`&`。system参数带任意`flecs::entity`或其他组件的都是实体system，否则是管理器system。

## 5. Wrapper的写法

对于普通system：
- 如果是管理器system，则Wrapper的签名统一是：void {SystemName}Wrapper(flecs::iter& it)
- 如果是实体system，则Wrapper的签名统一是：void {SystemName}Wrapper(flecs::iter& it, size_t i, [组件列表])

对于事件system：
- 如果是管理器system，则Wrapper的签名统一是：void increaseScoreSystemWrapper(pulse::event_reader<{EventName}> eventReader, flecs::world& world, [附加查询列表])
- 如果是实体system，则Wrapper的签名统一是：void increaseScoreSystemWrapper(pulse::event_reader<{EventName}> eventReader, flecs::world& world, [附加查询列表]，[组件列表])

说明，对于事件system，其签名可以携带附加查询列表；对于普通system，其签名不懈怠附加查询列表。

## 6. 注册器写法

对于普通system：
- 如果是管理器system：
```
	moduleContext->world.system("{SystemName}")
		.kind(moduleContext->{Phase})
		.run({SystemName}Wrapper);
```

- 如果是实体system：
```
	moduleContext->world.system<[组件列表]>("{SystemName}")
		.kind(moduleContext->{Phase})
		.each({SystemName}Wrapper);
```

对于事件system：
- 如果是管理器system：
```
	auto {EventName}Dispatcher = std::make_unique<pulse::EventRegister<{EventName}>>();
	{EventName}Dispatcher->reg(spawnAppleSystemWrapper, [附加查询:moduleContext->world.query<[附加查询组件列表]>()]);
	{EventName}Dispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move({EventName}Dispatcher));
```

- 如果是实体system：
```
	auto {EventName}Dispatcher = std::make_unique<pulse::EntityEventRegister<{EventName}, [主查询组件列表]>>();
	{EventName}Dispatcher->reg(executeSnakeMoveSystemWrapper, [附加查询:moduleContext->world.query<[附加查询组件列表]>()]);
	{EventName}Dispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move({EventName}Dispatcher));
```
## 7. 附加查询

system无论是普通system还是事件system、还是实体system或者管理器system都有可能有附加查询。

对于普通system：

在Wrapper函数之上定义`{SystemName}WrapperState`类，并在该类中声明所需附加查询，比如：
```
struct checkCollisionSystemWrapperState
{
	flecs::query<const Player, const Position> playerQuery;
	flecs::query<const FallingBlock, const Position> blockQuery;
};
```

然后在Wrapper内获取，并传给system：
```
auto systemState = it.system().get<checkCollisionSystemWrapperState>();
oriSystem(..., systemState.additionalQuery1, systemState.additionalQuery2, ...);
```

之后在注册处设置state并创建附加query：
```
auto checkCollisionSystem = moduleContext->world.system("CheckCollision")
	.kind(moduleContext->updatePipeline)
	.run(checkCollisionSystemWrapper);
checkCollisionSystem.set<checkCollisionSystemWrapperState>({ .playerQuery = moduleContext->world.query<const Player, const Position>(), .blockQuery = moduleContext->world.query<const FallingBlock, const Position>() });
```

对于事件system，上面已经描述过了，但还是再总结一下：首先是Wrapper签名要体现附加查询，其次是注册时创建查询并传给reg函数。
