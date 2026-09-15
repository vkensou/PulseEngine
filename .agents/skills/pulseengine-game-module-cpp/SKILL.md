---
name: pulseengine-game-module-cpp
description: 在 PulseEngine 仓库（E:\myroom\projects\PulseEngine）里用 C++ 写游戏模块（pulse 游戏包）时使用。用户说"写个游戏"、"再做/做一个游戏"、"写个游戏模块"、"测试游戏引擎/引擎好不好用"，或提到 PULSE_ECS_* 宏、generate_module、launcher.manifest.json、pulse_prefab、pulse_datatable 时触发。只覆盖 C++ 模块（shared 库 + launcher 动态加载）；纯 daslang 脚本内容包（examples/snake_daslang 那条路径）不在本 skill 范围内。自包含模板：用 scripts/new_game.ps1 一键生成骨架 → 填游戏逻辑 → tablegen 生成数据表绑定 → generate_module 生成模块/插件代码 → launcher.manifest.json 接入 → xmake 构建 → 跑 launcher 验证窗口标题。
---

# PulseEngine C++ 游戏模块开发

游戏 = 一个 **pulse 包**：C++ 模块（声明 + 实现 + 生成器产物）+ `package.json` + 资产（prefab/material/shader/data table）+ 根 `xmake.lua` 里的一个 `shared` target。它编译成 `example_<game>.dll`，由通用宿主 **launcher** 在运行时按清单动态加载。

本 skill 自带完整模板，**不需要阅读仓库里现有的 snake 源码**——模块骨架、prefab/数据表接入、实体创建、状态机、UI 系统全部内嵌在 `templates/` 里。

**边界**：只讲 C++ 游戏模块。仓库里还有一条"纯内容包"路径（`examples/snake_daslang`：只有 `.das` + 资源，没有 C++、没有 xmake target，靠 `pulse_daslang` 的脚本运行时加载），不在本 skill 范围内，别把两者混在一起。

## 心智模型

```
launcher.exe                    宿主：只链 pulse_app / pulse_package_loader / pulse_vfs
  └─ 读 launcher.manifest.json  → packages 列表（名字 + 各自的 config）
  └─ 按 search roots 找 <root>/<name>/package.json
  └─ 按 dependencies 拓扑排序后 dlopen(<library>.dll) → 调 pulse_package_register
       └─ 游戏包：pulse_add_<game>_plugin → build 钩子 → importModule(&moduleContext)
            └─ 注册组件反射 / 资源 / 系统 / 状态机
```

- 游戏**没有自己的 `main.cpp`**，也不再被引擎"零感知地"静态链接进某个 exe（`examples/snake/main.cpp_bak` 是历史遗留，不参与构建）。
- 每个游戏是**独立 DLL**，所以模块符号冲突不再是问题（早先"多模块共存必须 static"的坑随之消失）。
- `assets: true` 的包，其目录会**永久挂载在 VFS `/`**，所以 prefab/material 里的 `"assets/xxx"` 才解析得到。

## 模板文件

| 模板 | 落地路径（`<game>` 为游戏名） | 说明 |
|---|---|---|
| `templates/game.h` | `examples/<game>/<game>.h` | 模块声明：状态机 / 组件 / 单例 / 事件 / 资源 / 系统 |
| `templates/game.cpp` | `examples/<game>/<game>.cpp` | 系统实现：prefab+数据表异步加载、实体创建、移动/碰撞示例、事件链、重开、UI |
| `templates/package.json` | `examples/<game>/package.json` | 包元数据（library / entry / dependencies / paths） |
| `templates/xmake_target.lua` | 追加到根 `xmake.lua` | `shared` target + `pulse.package_install` |
| `templates/assets/*.prefab` | `examples/<game>/assets/` | 两个实体模板（board / ball），带 tag 的写法可自行加 `tags` 段 |
| `templates/assets/*.material` | 同上 | 材质（只设 `albedo` 颜色） |
| `templates/assets/color.shader` | 同上 | 复用 `color.vert.spv` / `color.frag.spv` |
| `templates/assets/game_config.datatable` | `examples/<game>/assets/<game>_config.datatable` | 数据表本体 |
| `templates/schema/game_config.schema` | `examples/<game>/schema/<game>_config.schema` | 数据表 schema（tablegen 输入） |

`Quad.obj` / `color.slang` / `color.vert.spv` / `color.frag.spv` 从 `examples/snake/assets/` 复用，脚手架脚本会自动拷过来；要别的颜色只写新的 `.material` 即可。

## 工作流

### 第 0 步：一键生成骨架

```sh
pwsh -File .agents/skills/pulseengine-game-module-cpp/scripts/new_game.ps1 -Name bounce -DisplayName Bounce
```

脚本做四件事：拷贝模板到 `examples/bounce/`（`bounce.h` / `bounce.cpp` / `package.json` / `assets/` / `schema/bounce_config.schema`）、改名 `<game>_config.*`、替换全部占位符、把 `target("example-bounce")` 追加到根 `xmake.lua`。游戏名只允许小写字母/数字/下划线。

占位符（手写或修改模板时同样适用，替换顺序：先 `<game>`，再 `<Game>`，最后 `<GAME_NAME>`）：

| 占位符 | 含义 | Bounce 示例 |
|---|---|---|
| `<game>` | 小写名（文件/目录/target/数据表前缀） | `bounce` |
| `<Game>` | CamelCase 前缀（C++ 类型与函数名） | `Bounce` |
| `<GAME_NAME>` | 窗口标题里的显示名 | `Bounce` |
| `game_config` / `GameConfig` | 配置表名（文件与生成类型） | `bounce_config` / `BounceConfig` |

### 第 1 步：设计游戏，改 `<game>.h`

按设计增删声明，宏注解格式别动：

- **状态机**：默认 5 态 `UnInitialized → Loading → LoadFailed / Playing ⇄ GameOver`（异步资源加载 + 游戏 + 重开），一般不需要改。状态枚举用 `PULSE_ECS_STATE_MACHINE(INIT=UnInitialized)` 标记，系统/事件用 `STATE=<Game>GameState::Playing` 门控（生成器会注册进状态机并按 flecs disable 机制批量开关）。
- **组件** `PULSE_ECS_COMPONENT`：普通 struct（HMM_Vec3、int、float、枚举、`flecs::entity`、`std::vector` 等）。
- **单例** `PULSE_ECS_SINGLETON_COMPONENT`：全局配置/计分/prefab 句柄，`command_buffer.set_singleton<T>` 写入，系统参数 `pulse::singleton_query<const T>&` 或直接当组件参数读（见下方速查表）。
- **标签** `PULSE_ECS_TAG`：无成员 struct（不能有成员，生成器会报错）。
- **事件** `PULSE_ECS_EVENT`：命名以 Event 结尾。
- **资源** `PULSE_ECS_RESOURCE`：模块内异步加载请求的容器（生成器会 `registerResource` 成稀疏单例），系统参数用 `pulse::res<<Game>Assets>` 拿引用。
- 声明顺序：状态机枚举在最前，其余随意。

组件/单例/标签/事件的 **flecs 反射由生成器自动产出**（枚举、结构体成员按类型映射，HMM 数学类型与引擎组件同形态），不需要手写注册。

### 第 2 步：写系统实现（`<game>.cpp`）

- 模板里已给出可编译的完整骨架：`instantiatePrefab`/`createEntities` 辅助函数、`load<Game>ResourcesSystem`（prefab + 数据表并行加载 → 建实体/相机 → Playing）、`move<Game>BallSystem`（计时移动 + 边界反弹 + 广播事件）、`on<Game>ScoredSystem`/`on<Game>GameOverSystem`/`restart<Game>System`（事件链）、`<game>UISystem`（写窗口标题、GameOver 显示重启按钮）、`<game>FpsUISystem`。**按游戏逻辑改这些系统，别改骨架的三段（加载/UI/实体创建）结构**。
- 系统函数实现**不要加 static**（生成的 wrapper 要跨翻译单元调用）；模块内部辅助函数放**匿名 namespace**（等价于 static，且更整洁）。
- 实体创建两条路：**要渲染的实体**用 `<Game>Prefabs` 里的句柄 `instantiatePrefab(app, handle, pos, scale)`（prefab 自带 `PulseRenderable`，实例上只改运行期才知道的变换），**纯逻辑实体**用 `command_buffer.entity()`；往实体上 `set<T>` 在系统 stage 里都排队到 merge 生效。
- 事件广播：`event_writer<T>::broadcast(payload)`（全局）/ `send<C...>(entity, payload)`（指定实体）。
- 遵守 AGENTS.md：不写注释、同一条语句不换行、不做无谓兼容。

### 第 3 步：生成数据表绑定（tablegen）

```sh
xmake build tablegen
build\windows\x64\debug\tablegen.exe --out-h examples\bounce\schema\tables_generated.h --out-cpp examples\bounce\schema\tables_generated.cpp examples\bounce\schema\bounce_config.schema
```

- schema 文件名 = 表类型名（`bounce_config` → 行结构体 `pulse_tables::PulseBounceConfigRow`、查表类 `PulseBounceConfigRowTable`）。
- 数据文件名**必须与类型名同名**：`assets/bounce_config.datatable`，内容首行 `schema: "bounce_config"`。
- 多个互相引用的 schema（struct 列 / enum / `ref` 列）必须一次性全部传给 tablegen。
- 数据表是异步资产：系统里先 `pulse_tables::RegisterSchemas(pulse_get_data_table_system(app))`，再 `Pulse<Game>ConfigRowTable::Load(app, "assets/<game>_config.datatable")`，之后每帧轮询 `IsReady` / 用 `GetError` 检失败，就绪后 `GetRow(app, "default")->字段`。表只加载一次，重开时从单例配置取参数、不要重读文件。
- 列类型：`int/float/bool/string/enum schema/struct schema/ref`；`min`/`max` 仅 int/float；主键有且只有一个且必须是 string 或 int。

### 第 4 步：生成模块与插件代码

```sh
tools\idl\lua54.exe tools\generate_module\generate_module.lua generate examples/bounce/bounce.h examples/bounce/package.json
```

- 产物（同目录，**禁止手改**）：`bounce_module.h` / `bounce_module.cpp`（反射注册 + 状态机注册 + 全部系统 wrapper + `importModule`）、`bounce_plugin.cpp`（`pulse_package_register` + `pulse_add_bounce_plugin`，依赖列表来自 `package.json` 的 `dependencies`）。
- 生成器会校验并在写错时报错退出：PHASE 非法、STATE 成员不存在或没写 `枚举名::成员`、一个系统多个 `event_reader`、`PULSE_ECS_TAG` 带成员、重复标记、组件成员形态不支持（位域/函数指针/默认成员初始化器等）。
- 想先看解析结果：把 `generate` 换成 `dump`。生成后读一遍产物，确认系统都在、依赖列表对。
- 生成器行为有疑问时读 `tools/generate_module/ECS Module 代码生成规则.md`（244 行，权威且够用）。**不要逐段读 `generate_module.lua`（87KB）或 flecs 源码**——历次会话在这上面浪费了几十次调用。

### 第 5 步：接进 launcher（宿主）

开发期宿主就是仓库里的 `launcher`，它只加载 `src/launcher/launcher.manifest.json` 里列出的包：

1. `packages` 数组末尾加 `{ "name": "bounce" }`（放在所有 `pulse_*` 之后）。
2. **一次只跑一个游戏**：把其它游戏包条目（`snake` / `snake_daslang`）删掉；`pulse_*` 插件条目一个都别动。
3. 窗口初始标题/尺寸在 `pulse_window` 条目的 `config.primary_window`（游戏 UI 每帧会覆盖标题，配置只决定第一帧）。
4. 改完 manifest 跑一次 `xmake build launcher`（`pulse.copy_manifest` 是 after_build，实测目标已最新时也会执行拷贝）；若构建目录里的 `launcher.manifest.json` 没更新，再用 `xmake build -r launcher`。

### 第 6 步：构建

```sh
xmake build example-bounce
xmake build launcher
```

- 产物在 `build/windows/x64/debug/`：`example_bounce.dll` + `launcher.exe` + `pulse_*.dll` + `launcher.manifest.json`。
- `xmake` 一次只能构建一个 target（`xmake build a b` 不合法）。
- 不要用 `xmake build` 构建测试项目，测试用 `xmake test`。
- 只改游戏代码时不需要重编 launcher；改过 `launcher.manifest.json` 才需要再 `xmake build launcher`（见第 5 步第 4 条）。

### 第 7 步：运行与验证

```sh
xmake run launcher
```

`launcher` target 的 `set_runargs` 已经传了绝对路径的 `src` 与 `examples`，运行目录是构建输出目录，直接就能跑。手动等价写法：

```sh
cd build\windows\x64\debug
launcher.exe E:\myroom\projects\PulseEngine\src E:\myroom\projects\PulseEngine\examples
```

冒烟验证（窗口标题由游戏 UI 系统写成 `<GAME_NAME> - <score>`，出现即证明：包加载成功、prefab 与数据表异步加载完成、状态机流转到 Playing、UI 全链路正常）：

```sh
pwsh -File .agents/skills/pulseengine-game-module-cpp/scripts/verify_game_title.ps1 `
    -Exe build/windows/x64/debug/launcher.exe `
    -WorkingDir build/windows/x64/debug `
    -ExeArgs E:/myroom/projects/PulseEngine/src,E:/myroom/projects/PulseEngine/examples `
    -Expect "Bounce -"
```

要看画面用 skill `test-tool-for-program-with-window`（注意它 SKILL.md 里的 `example-snake.exe` 已过时，现在要传 `launcher.exe` + `src examples` 两个参数，工作目录用构建输出目录）。模板的场地是内缩的固定矩形，截图里应当能同时看到浅灰边框和黄色球——两者缺一就是在踩坑 5/6。

本 skill 的模板已按上述流程端到端验证过：`new_game.ps1 -Name bounce` → tablegen → generate_module → manifest → `xmake build example-bounce` → 标题 `Bounce - 4`、截图可见边框与球。

## 参数类型速查表

| 参数类型 | 生成行为 |
|---|---|
| `pulse::command_buffer&` | 实体/单例的 deferred 操作（`entity()` / `set_singleton<T>` / `destruct` / `defer_suspend`） |
| `pulse::res<T>` / `pulse::res<const T>` | 读外部单例（world.get/get_mut），如 `PulseTimer`、`PulseKeyboardInput`、自己的 `PULSE_ECS_RESOURCE` |
| `pulse::singleton_query<const T>&` | 单例查询（SingleHolder 上的组件），`get()` 读 / `get_mut()` 写 |
| `pulse::event_reader<T>` | 事件系统标记（**必须第一个参数，每系统只能 1 个**），`read()` 取负载 |
| `pulse::event_writer<T>` | 发事件：`broadcast(payload)` / `send<C...>(entity, payload)` |
| `flecs::query<T...>&` | 附加查询（查询别的实体；普通系统注册时存 WrapperState，事件系统直接进 wrapper 参数） |
| `flecs::entity` | 主查询实体（wrapper 内 `it.entity(i)` 注入） |
| `PulseAppId` | `pulse_get_app_from_world` 反查的 app 句柄，不参与主查询 |
| `pulse::system_state_machine<T>` | 状态机访问器：`is(...)` / `to(...)` / `current()` |
| 其他组件类型（`const T&` / `T&`） | 主查询组件，按声明顺序遍历 |

判定规则：带 `flecs::entity` 或任意组件参数 = **实体系统**（`.each`）；否则 = **管理器系统**（`.run`，每帧一次）。带 `event_reader` = 事件系统。管理器和事件系统的 wrapper 不接收附加查询参数（附加查询走系统的 state）。

**单例组件当组件参数**时，遍历的是 `SingleHolder` 那一个实体（所以“跑一次”），例如模板的 `<game>UISystem(const <Game>Score& score, ...)`、`on<Game>ScoredSystem(..., <Game>Score& score)`；想读别的单例就在参数里加 `pulse::singleton_query<const T>&`。

## 踩坑清单

1. **三个名字必须一致**：`package.json` 的 `name` = 生成的插件名 = `launcher.manifest.json` 里的条目名；`package.json` 的 `library` = `set_basename` = `<library>.dll`。名字对不上会在依赖/去重/找库阶段失败。
2. **`"assets": true` 是承重的**：loader 只保留 `assets: true` 的包目录挂在 VFS `/`，prefab/material/shader 里的资源路径才解析得到；漏了它资源全 404，且报错信息不会直说是这个原因。
3. **资源路径是 VFS 路径（相对包根）**，不是相对 `.prefab` 文件所在目录：`"assets/Quad.obj"`、`"assets/ball.material"`。prefab 里的网格/材质句柄由 prefab loader 自己按字符串预加载后再建实体。
4. **prefab 支持层级与继承**：一个 `.prefab` 可以是单个 prefab，也可以是命名库；段落有 `name` / `components` / `tags` / `pairs` / `children` / `extends`。要 tag 就在 `tags : ["Xxx"]` 里写（tag 必须是纯 tag，组件只能写在 `components`，pair 只能写在 `pairs`）。参考 `examples/snake/assets/*.prefab` 与 `tests/prefab/data/`。
5. **要渲染的实体必须从 prefab 实例化**：`PulseRenderable`（网格 + 材质）写在 prefab 模板里，只有 `pulse_prefab_instantiate` 出来的实例才带它。用 `command_buffer.entity()` 新建的实体**没有渲染组件**，逻辑跑得通（会移动、会碰撞）但画面上什么都没有——模板里球是 `instantiatePrefab(app, prefabs.ball, ...)` 后再 `set<<Game>Ball>`，就是这个原因。写新实体时先问：它要不要画出来？
6. **场地别按相机视锥算**：模板把场地做成内缩的固定矩形（±6/±4），截图能同时看到边框和球。若按视锥算（`halfHeight = -cameraZ * tan(fov/2)`），边框正好落在屏幕边缘、场地填满整屏，截图里几乎全黑，看不出对错——早期 snake 的边框就是这个行为。
7. **系统运行期间不要 `defer_suspend` 后直接改结构**：实体创建走正常 deferred（`command_buffer.entity()`）；唯一例外是事件系统里重建实体（模板 `restart<Game>System` 就是这么写的）。
8. **窗口标题不要裸调 `SDL_SetWindowTitle`**（会被 `PulseWindow` 同步系统覆盖），用 `pulse_window_set_title(app, windowEntity, title)`；entity 从 `flecs::query<PulseWindow, PulsePrimaryWindow>&` 取 `first()`。重复设置相同标题是 no-op，不会每帧分配。
9. **改了 `launcher.manifest.json` 要重跑 `xmake build launcher`**（见第 5 步）；且开发期清单里同时留两个游戏包会让两套系统/UI 一起跑。
10. **stdout 重定向是全缓冲**，超时杀进程会丢日志：判断游戏状态用窗口标题（脚本）或截图，不要 grep stdout。
11. **系统函数不能 static**（wrapper 跨翻译单元调用）；模块内部辅助函数放匿名 namespace。
12. **不再需要 UTF-8 BOM**：根 `xmake.lua` 已全局强制 `/utf-8`（cl/clang-cl）与 `-finput-charset/-fexec-charset=utf-8`，任何编码的源文件都能编。
13. **不需要手动绑定 C 组件**：C 组件 id ↔ C++ 类型的绑定由各插件内部 `register_components` 完成（`pulse_cpp_gameplay.h` 的注释写着这一点），早期 main.cpp 里那 6 行绑定段已随 main.cpp 一起消失。
14. **生成产物禁止手改**：`<game>_module.h/.cpp`、`<game>_plugin.cpp`、`schema/tables_generated.*` 都是工具产物，改了就重跑生成器。同理模块公共头 `src/pulse_*/include/*.h` 由 IDL 生成。
15. **Git Bash 用正斜杠路径**：`E:/myroom/projects/PulseEngine/...`，反斜杠结尾的 `\"` 会转义引号把命令拼坏。

## 交付检查单

- [ ] `examples/<game>/` 下：`<game>.h`、`<game>.cpp`、`package.json`、`assets/`、`schema/`
- [ ] 占位符全部替换（`grep -rn "<game>\|<Game>\|<GAME_NAME>\|game_config" examples/<game>` 应为空）
- [ ] `schema/<game>_config.schema` 与 `assets/<game>_config.datatable` 同名，`.datatable` 首行 `schema: "<game>_config"`
- [ ] `<game>_module.h/.cpp`、`<game>_plugin.cpp` 已生成并读过一遍
- [ ] 根 `xmake.lua` 有 `target("example-<game>")`（shared + `pulse.package_install` + schema 源文件）
- [ ] `launcher.manifest.json` 里加了 `{ "name": "<game>" }`，且没有别的游戏包同时在列
- [ ] `xmake build example-<game>` 与 `xmake build -r launcher` 通过（无 LNK/无 `pulse.package_install` 报错）
- [ ] `verify_game_title.ps1` 确认窗口标题出现 `<GAME_NAME> -`
- [ ] 遵守 AGENTS.md：不写注释、同一条语句不换行、改动面向长期而无历史包袱

## 参考资料

- 生成规则（权威，短）：`tools/generate_module/ECS Module 代码生成规则.md`
- 数据表：`tools/tablegen/README.md`、`docs/v0.3/数据表方案.md`、示例 `examples/snake/schema/*`
- datalist（prefab/data table 共用语法）：`docs/datalist.md`
- 包/宿主/规则：`src/launcher/main.cpp`、`src/pulse_package_loader/src/package_loader.cpp`、`xmake/rules/package_output/xmake.lua`
- 现成参考实现：`examples/snake/`（C++ 包，prefab + 数据表 + 状态机全套）
