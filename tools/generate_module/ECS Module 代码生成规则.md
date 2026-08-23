# ECS Module 代码生成规则

## 0. 需要注意的点

- 不要在头文件的注释里写`PULSE_ECS_COMPONENT`/`PULSE_ECS_SINGLETON_COMPONENT`/`PULSE_ECS_EVENT`/`PULSE_ECS_SYSTEM`/`PULSE_ECS_STATE_MACHINE`/`PULSE_ECS_RESOURCE`等宏，目前生成器还无法正确处理注释

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

### 3.3 STATE（状态集）

当PULSE_ECS_SYSTEM中出现`STATE=A|B|C`时，表示该system/observer的"启用状态集"：只在枚举状态为A、B或C时运行。生成时：

- 普通system注册改为保留句柄（`auto {SystemName} = ...`），并追加`stateMachine.reg({SystemName}, { A, B, C });`
- 事件system注册改为`stateMachine.reg({EventName}Dispatcher->observe(moduleContext->world), { A, B, C });`（observe返回observer实体；同组监听器共享一个observer，状态集取第一个，不一致时向stderr告警）

STATE 的枚举类型须与`PULSE_ECS_STATE_MACHINE`标记的枚举一致，且成员必须写`枚举名::成员`形式（裸成员名无法在生成代码中直接引用，直接报错）。

### 3.4 PULSE_ECS_STATE_MACHINE（状态机）

```cpp
PULSE_ECS_STATE_MACHINE(INIT=UnInitialized)
enum class SnakeGameState { ... };
```

标记模块的状态枚举，`INIT=`指定初始状态。生成时自动产出：

- 状态机注册表单例（flecs单例组件，先`world.set`后`get_mut`）：
  ```cpp
  moduleContext->world.set<pulse::StateMachine<SnakeGameState>>(pulse::StateMachine<SnakeGameState>{});
  auto& stateMachine = moduleContext->world.get_mut<pulse::StateMachine<SnakeGameState>>();
  ```
- 状态管理系统（每帧最先跑、IMMEDIATE，注册在所有UPDATE系统之前）：
  ```cpp
  moduleContext->world.system("SnakeGameStateMachine")   // 枚举名去尾部State + StateMachine
      .kind(moduleContext->updatePipeline)
      .immediate()
      .run(snakeGameStateMachineWrapper);
  ```
- 收尾初始化：`stateMachine.init(moduleContext->world, SnakeGameState::UnInitialized);`

当前生成器只支持一个状态机（多个时报错）。

### 3.5 PULSE_ECS_RESOURCE（ECS资源单例）

```cpp
PULSE_ECS_RESOURCE
struct SnakeAssets { ... };
```

标记资源结构体。生成时在importModule开头产出：
```cpp
pulse::registerResource<SnakeAssets>(moduleContext->world, "Snake Assets", SnakeAssets{});
```
注册显示名由结构体名CamelCase转空格分隔（`SnakeAssets` -> `"Snake Assets"`）。

## 4. system参数类型

system的参数类型有如下几种可能：

- `pulse::command_buffer`：命令缓冲
- `pulse::res<T>` 或 `pulse::res<const T>`：外部资源
- `pulse::singleton_query<T>`：单例查询
- `pulse::event_reader<T>`：事件读取器
- `pulse::event_writer<T>`：事件写入器
- `flecs::query<T...>`：附加查询
- `flecs::entity`：system主查询遍历的实体
- `PulseAppId`：app句柄。Wrapper内生成`auto app = pulse_get_app_from_world(world.c_ptr());`，不参与主查询
- `pulse::system_state_machine<T>`：状态机访问器。Wrapper内生成`auto state = pulse::system_state_machine<T>(world);`，不参与主查询
- 其他组件：system主查询遍历的组件列表

参数获取时一律去掉`&`。system参数带任意`flecs::entity`或其他组件的都是实体system，否则是管理器system。

## 5. Wrapper的写法

对于普通system：
- 如果是管理器system，则Wrapper的签名统一是：void {SystemName}Wrapper(flecs::iter& it)
- 如果是实体system，则Wrapper的签名统一是：void {SystemName}Wrapper(flecs::iter& it, size_t i, [组件列表])

对于事件system：
- 如果是管理器system，则Wrapper的签名统一是：void increaseScoreSystemWrapper(pulse::event_reader<{EventName}> eventReader, flecs::world& world, [附加查询列表])
- 如果是实体system，则Wrapper的签名统一是：void increaseScoreSystemWrapper(pulse::event_reader<{EventName}> eventReader, flecs::world& world, [附加查询列表]，[组件列表])

说明，对于事件system的Wrapper，其签名会携带附加查询列表；对于普通system的Wrapper，其签名不携带附加查询列表。

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

## 8. 插件入口生成

`generate` 模式除生成 `<文件名>_module.{h,cpp}` 外，还会在同目录生成
`<文件名>_plugin.cpp`，用于把 ECS 模块包装成 launcher 可动态加载的
PulsePlugin：

- 模块名 `snake` 会生成 `PulseSnakePlugin`、`pulse_add_snake_plugin`、
  `snake_plugin_build` / `snake_plugin_shutdown`，以及
  `pulse_package_register` 导出入口。
- 插件 build 阶段固定做：`pulse::init_gameplay_base` →
  `pulse::make_module_context` → `importModule`。
- 生成器不内置默认插件依赖列表，不假设任何依赖。
- 插件依赖只来自 packageinfo 的 `dependencies` 字段。
- 可通过可选参数 `[packageinfo.json]`（例如 `examples/snake/package.json`）
  传入 manifest，生成器会读取其中的 `dependencies`。
- 不传 `packageinfo` 时，若头文件同目录存在 `package.json`，生成器也会自动读取；
  都没有时插件依赖为空。
- 生成器通过同目录下的独立 `tools/generate_module/json.lua` 解析 JSON，
  只用于读取 packageinfo 的依赖信息；`package.json` 本身保持 JSON 格式，
  不影响其他 Python/加载器工具。
- 生成的插件文件与手写版本允许存在小幅风格差异，保证可编译、可运行即可。
