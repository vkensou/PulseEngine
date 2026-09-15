---
name: pulseengine-game-module-daslang
description: 在 PulseEngine 仓库（E:\myroom\projects\PulseEngine）里用 daslang 写游戏模块（pulse 游戏包）时使用。自包含模板：用 scripts/new_game.ps1 一键生成骨架 → 填游戏逻辑 → tablegen 生成数据表绑定 → generate_module 生成模块/插件代码 → launcher.manifest.json 接入 → 跑 launcher 验证窗口标题。
---

# PulseEngine daslang 游戏模块开发

PulseEngine是模块化的。各个功能包括游戏逻辑都是独立的包。游戏模块只需要包括游戏自身逻辑和资源。由宿主 **launcher** 在运行时按清单动态加载和启动。

本 skill 自带完整模板，**不需要阅读仓库里现有的 snake_daslang 源码**——模块骨架、prefab/数据表接入、实体创建、状态机、UI 系统全部内嵌在 `templates/` 里。

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
pwsh -File .agents/skills/pulseengine-game-module-daslang/scripts/new_game.ps1 -Name bounce -DisplayName Bounce
```
脚本会将模板文件拷贝到指定目录，并替换占位符。

### 第 1 步：设计游戏

根据要做的游戏自行设计游戏逻辑

### 第 2 步：

引擎是基于ECS（flecs）的，所以实现游戏逻辑时需要合理划分组件、系统。同时引擎参考了bevy，所以也提供了一些bevy的概念。下面举例详述：

ECS组件：
```
struct SnakeMove {
    interval : float
    lastTime : float
}
```

ECS标签（tag）：
```
struct IsApple {}
```

ECS事件：
```
struct AppleEatenEvent {
    apple : Entity
}
```

```
[system(phase="update", state="UnInitialized|Loading")]
def loadSnakeResourcesSystem(app : PulseAppId, assets : Res<SnakeAssets>, state : SystemStateMachine<SnakeGameState>, cmd : CommandBuffer, primaryWindowQuery : Query<PulseWindow const, PulsePrimaryWindow const>) {
}
```