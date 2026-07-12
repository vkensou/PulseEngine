# PulseEngine — AI Agent 指南

## 项目概览

PulseEngine 是一个模块化的基于 ECS（flecs）的游戏引擎，核心设计理念是**最小核心、可插拔、模块化**。所有功能模块以 pulse_* 插件的形式通过统一的 C API 注册到 App。

当前 milestone 为 **v0.1**：基于 pulse 插件体系重新实现贪吃蛇游戏，与 `examples/dascript` 同等能力。目前仓库里还混杂了老版本的代码，未来将会删除。

---

## 项目结构

```
PulseEngine/
├── xmake.lua                 ← 根构建文件
│
├── src/
│   ├── pulse_app/            ← 核心：App 生命周期 + 插件注册 + ECS 世界
│   ├── pulse_window/         ← SDL3 窗口管理插件
│   ├── pulse_asset/          ← 异步资源加载插件
│   ├── pulse_graphics/       ← GPU 资源管理（CGPU + rendergraph）
│   ├── rendergraph/          ← 渲染图调度执行库（纯 C API）
│   └── rgframework/          ← 老版代码
|
├── cgpu/                     ← 跨平台图形 API 封装库
├── dascript/                 ← dascript脚本语言
|
├── tests/
│   ├── app/                  ← pulse_app 测试
│   ├── window/               ← pulse_window 测试
│   ├── asset/                ← pulse_asset 测试
│   └── graphics/             ← pulse_graphics 测试
│
├── examples/
│   ├── dascript/             ← 老版 DaScript 集成示例（待迁移目标）
│   ├── rendersystem/         ← 老版渲染示例
│   ├── snake/                ← 老版贪吃蛇示例
|   └── assets/               ← 示例依赖的各种资源，包括dascript的标准库
│
├── tools/idl/                ← IDL 代码生成工具链
│
├── docs/
│   ├── 愿景.md               ← 产品愿景
│   ├── 架构目标.md            ← 架构设计目标
│   └── v0.1/
│       ├── 版本任务.md        ← 完整 checklist
│       └── 插件设计.md        ← 各插件职责、ECS 组件、API 设计
│
└── AGENTS.md                 ← 本文件
```

---

## 构建

使用 xmake 构建系统。

```bash
# 首次克隆后更新子模块
git submodule update --init --recursive

# 构建所有 target
xmake build -P "E:\myroom\projects\PulseEngine"

# 构建单个 target
xmake build -P "E:\myroom\projects\PulseEngine" test-app

# 运行 target
xmake run -P "E:\myroom\projects\PulseEngine" test-app
```

> **注意：** 在 worktree 中执行 xmake 命令必须添加 `-P <path>` 参数。`-P` 放在命令和 target_name 之间。

可用 target：
| Target | 类型 | 说明 |
|--------|------|------|
| pulse_app / pulse_window / pulse_asset / pulse_graphics | 静态库 | 核心插件模块 |
| test-app / test-window / test-asset / test-graphics | 二进制 | 各模块测试 |
| snake / rendersystem / das-example | 二进制 | 老版示例（基于 rgframework） |

---

## 插件架构

所有 pulse_* 模块遵循统一插件协议，通过 `PulsePluginDesc` 注册到 `PulseAppId`，拥有 build / post_build / shutdown 生命周期。

```
pulse_app ───── 核心：App 生命周期 + 插件注册 + ECS 世界（flecs）
  ├─ pulse_window  ── SDL3 窗口管理
  ├─ pulse_asset   ── 异步资源加载
  ├─ pulse_graphics ─ GPU 资源管理 + RenderGraph
  ├─ pulse_math    ── header-only 数学库（HandmadeMath）
  ├─ pulse_transform ─ ECS 变换 + 层级系统（v0.1 待实现）
  ├─ pulse_renderer  ─ 渲染管线框架（v0.1 待实现）
  ├─ pulse_imgui     ─ ImGui 集成（v0.1 待实现）
  ├─ pulse_script_dascript  ─ DaScript 运行时（v0.1 待实现）
  └─ pulse_dascript_binding ─ DaScript 绑定层（v0.1 待实现）
```

---

## IDL 代码生成

所有 pulse_* 模块的 C API 头文件通过 IDL 自动生成：

```
<module>.idl  →  temp.<module>.h  →  include/<module>.h（最终头文件）
```

工具链在 `tools/idl/`。IDL 语法参考 `.reasonix/skills/idl-guide.md`。

---

## 文档地图

| 文档 | 读者 | 内容 |
|------|------|------|
| `docs/架构目标.md` | 所有人 | 架构设计目标，每章标注 ✅🚧🔮 |
| `docs/v0.1/插件设计.md` | 开发者 | 每个插件的职责、ECS 组件、API 概览 |
| `docs/v0.1/版本任务.md` | 开发者 | 当前 milestone 的完整 checklist |
| `docs/IDL.md` | 开发者 | IDL 语法与代码生成流程 |
| `.reasonix/skills/idl-guide.md` | AI Agent | IDL 编写快速参考（AI 专用版） |

---

## 工作流约定

1. **了解背景后再修改** — 先读相关文档、头文件和已有实现，不猜测
2. **增量修改，保持最小** — 不改无关文件，不改超过需求的代码
3. **每次修改先验证编译** — 改完 run xmake build 确认不破坏已有 target
4. **提交前 review** — 审查 diff 确保没有遗漏或错误
5. **任何修改需要审批才能提交**

---

## v0.1 当前状态

- [x] pulse_app / pulse_window / pulse_asset / pulse_graphics — 已完成
- [x] pulse_math — header-only 数学库入口
- [ ] pulse_input — 统一输入抽象层
- [ ] pulse_transform — 变换组件 + 层级系统
- [ ] pulse_renderer — 渲染管线框架
- [ ] pulse_imgui — ImGui 集成
- [ ] pulse_script_dascript — DaScript 运行时
- [ ] pulse_dascript_binding — DaScript 绑定层
- [ ] 贪吃蛇游戏集成示例
