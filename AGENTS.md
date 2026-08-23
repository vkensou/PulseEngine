## 项目简介

PulseEngine是一个模块化的基于ECS（flecs）的游戏引擎，核心设计理念是**最小核心、可插拔、模块化**。所有功能模块以插件的形式构建，对外暴露capi。语言使用c++，采用xmake构建。

使用SDL（窗口、文件、输入等） + cgpu（自研的渲染RHI）。

当前状态为 **v0.2**：目标是完成包管理和虚拟文件系统。

## 目录结构

- `docs` 是文档目录
- `src/pulse_*` 是各个插件目录
- `tests/*` 是各插件的测试用例
- `cgpu` 是项目使用的RHI
- `src/rendergraph` 是项目使用的rendergraph代码
- `dascript` 是dascript目录
- `tools` 是项目使用的工具，目前有： `tools/idl` 用于生成插件头文件
- `khr` 是为ktx贴图格式引入的代码，还在用，未来会独立出去

## 规则

- 各个模块的对外公共头文件均使用idl生成，禁止直接修改模块公共头文件：`src/pulse_*/include/pulse_*.h`，必须通过idl工具生成。[IDL文档](docs/IDL.md)
- 各个模块的包注册文件均使用idl生成，禁止直接修改：`src/pulse_*/src/package_register.cpp`，必须通过生成工具生成。[文档](tools/gen_package_register/README.md)
- 当需要运行、测试带窗口的target，比如但不限于：test-grpahics、test-renderer、example-snake时，需先build，确认生成exe后，使用skill：[test-tool-for-program-with-window](.agents/skills/test-tool-for-program-with-window/SKILL.md)。
- 接口用法拿不准的时候先去 `tests` 目录查找参考，不要一上来就去翻源码。

## 注意
- xmake 的命令不支持同时构建多个目标，比如`xmake build pulse-window pulse-asset`，只能一个一个构建。
- 不要用`xmake build` 构建测试项目，直接用 `xmake test`。
- tests目录下的测试用例可以通过`xmake test`测试。
