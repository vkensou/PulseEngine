## 项目简介

PulseEngine是一个模块化的基于ECS（flecs）的游戏引擎，核心设计理念是**最小核心、可插拔、模块化**。所有功能模块以插件的形式构建，对外暴露capi。语言使用c++，采用xmake构建。

使用SDL（窗口、文件、输入等） + cgpu（自研的渲染RHI）。

当前状态为 **v0.1**：目标是基于 pulse 插件体系重新实现贪吃蛇游戏，达到 `legacy/examples/dascript` 同等能力。目前仓库里还混杂了老版本的代码，未来将会删除。

## 目录结构

- `docs` 是文档目录
- `src/pulse_*` 是各个插件目录
- `tests/*` 是各插件的测试工程
- `cgpu` 是项目使用的RHI
- `src/rendergraph` 是项目使用的rendergraph代码
- `dascript` 是dascript目录
- `tools` 是项目使用的工具，目前有： `tools/idl` 用于生成插件头文件
- `khr` 是为ktx贴图格式引入的代码，还在用，未来会独立出去
- `legacy` 是之前的老代码，仅作参考，未来删除

## 规则

- 各个模块的对外公共头文件均使用idl生成，禁止直接修改模块公共头文件：`src/pulse_*/include/pulse_*.h`，必须通过idl工具生成。[IDL文档](docs/IDL.md)
- 当需要运行、测试带窗口的target，比如但不限于：test-grpahics、test-renderer、example-snake时，需先build，确认生成exe后，使用skill：[test-tool-for-program-with-window](.agents/skills/test-tool-for-program-with-window/SKILL.md)。
- 接口用法拿不准的时候先去 `tests` 目录查找参考，不要一上来就去翻源码。
