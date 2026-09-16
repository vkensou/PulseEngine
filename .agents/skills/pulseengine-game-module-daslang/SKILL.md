---
name: pulseengine-game-module-daslang
description: 在当前仓库里用 daslang 写游戏模块（pulse 游戏包）时使用。自包含完整模板。
---

# PulseEngine daslang 游戏模块开发

本文提到的命令、文档的目录均是相对于仓库根目录，而非当前skill目录。其他文件如`templates`等则是在当前skill目录。

PulseEngine是模块化的。各个功能包括游戏逻辑都是独立的包。游戏模块只需要包括游戏自身逻辑和资源。由宿主 **launcher** 在运行时按清单动态加载和启动。

本 skill 自带完整模板————模块骨架、prefab/数据表接入、实体创建、状态机、UI 系统全部内嵌在 `templates/` 里。**不需要阅读仓库里现有的 snake_daslang 源码**，有问题直接问用户。

## 游戏运行模型

```
launcher.exe                    宿主链接核心包 pulse_app / pulse_package_loader / pulse_vfs等
  └─ 读 launcher.manifest.json  → packages 列表（名字 + 各自的 config）
  └─ 按 search roots 找 <root>/<name>/package.json
  └─ 按 dependencies 拓扑排序后 dlopen(<library>.dll) → 调 pulse_package_register
       └─ 游戏包：pulse_add_<game>_plugin → build 钩子 → importModule(&moduleContext)
            └─ 注册组件反射 / 资源 / 系统 / 状态机
```

## 模板文件

| 模板 | 落地路径（`<game>` 为游戏名） | 说明 |
|---|---|---|
| `templates/game_module.das` | `examples/<game>/<game>_module.das` | 游戏逻辑模板脚本 |
| `templates/package.json` | `examples/<game>/package.json` | 包元数据（library / entry / dependencies / paths） |
| `templates/assets/rect.prefab` | `examples/<game>/assets/` | prefab示例，一个纯色方块prefab |
| `templates/assets/rect.material` | 同上 | 材质示例，一个纯色材质 |
| `templates/assets/color.shader` | 同上 | 纯色shader |
| `templates/assets/game_config.datatable` | `examples/<game>/assets/<game>_config.datatable` | 数据表本体 |
| `templates/schema/game_config.schema` | `examples/<game>/schema/<game>_config.schema` | 数据表 schema（tablegen 输入） |

`Quad.obj` / `color.slang` / `color.vert.spv` / `color.frag.spv` 等原始资源可以直接复用，也可以修改后使用。

## 工作流

### 第 0 步：一键生成骨架

```sh
pwsh -File .agents/skills/pulseengine-game-module-daslang/scripts/new_game.ps1 -Name <game>
```
脚本会将模板文件拷贝到指定目录，并替换占位符。

### 第 1 步：设计游戏

根据要做的游戏自行设计游戏逻辑

### 第 2 步：系统实现（`<game>_module.das`）

引擎是基于ECS（flecs）的，所以实现游戏逻辑时需要合理划分组件、系统。同时引擎参考了bevy，所以也提供了一些类似bevy的基础设施。详见： [ECS基础设施-daslang](docs/ECS基础设施-daslang.md)
暂时不要设置PulseWindow组件的title。

### 第 3 步：数据表

数据表和数据schema是分开的，详细文档在 [数据表文档](docs/v0.3/数据表方案.md)。引擎提供工具（tablegen）生成数据绑定代码，方便使用。工具使用方法如下：

```sh
xmake build tablegen
build\windows\x64\debug\tablegen.exe --out-das examples\<game>\<game>_tables.h examples\<game>\schema\<game>_config.schema
```

### 第 4 步：接进launcher（宿主）

开发期宿主就是仓库里的 `launcher`，它只加载 `src/launcher/launcher.manifest.json` 里列出的包：

1. `packages` 数组末尾加 `{ "name": "<game>" }`。
2. **一次只跑一个游戏**：把其它游戏包条目（`snake` / `snake_daslang`）删掉；你依赖的插件条目都要列出来。
3. 窗口初始标题/尺寸在 `pulse_window` 条目的 `config.primary_window`。

### 第 5 步：构建

```sh
xmake build
```

daslang包本身不需要构建，但launcher、native包需要构建。launcher对各个可选包没有直接依赖关系，所以最好一次性构建整个工程。

### 第 6 步：运行与验证

正常来说运行下面命令即可

```sh
xmake run launcher
```

但AI操作带窗口的程序不方便，使用skill `test-tool-for-program-with-window`。
