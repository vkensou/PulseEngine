---
name: test-tool-for-program-with-window
description: 当需要运行、测试带窗口的程序时，使用本skill。本skill提供了运行一段时间后关闭、运行一段时间后截图并关闭的功能。
---

# PulseEngine 窗口测试脚本

两个 Python 脚本用于冒烟测试窗口程序（如 `example-snake.exe`），支持"运行 + 定时关闭"和"运行 + 截屏 + 关闭"两类场景。脚本位于本 skill 的 `scripts/` 目录，直接调用即可，无需安装依赖（仅标准库 + Windows 自带 PowerShell）。

## 脚本一： run_exe_wait.py —— 定时运行后关闭

```sh
python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait.py <工作目录> <exe路径> <秒数> [exe参数...]
```

行为：以 `<工作目录>` 为当前目录启动 exe，等待 `<秒数>` 后关闭。exe 提前崩溃则返回其退出码；正常运行满秒数后退出码为 0。

示例（snake 资源是相对路径，工作目录必须指向仓库根）：

```sh
python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait.py E:/myroom/projects/PulseEngine E:/myroom/projects/PulseEngine/build/windows/x64/debug/example-snake.exe 5
```

## 脚本二： run_exe_wait_screenshot.py —— 运行后截屏再关闭

```sh
python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait_screenshot.py <工作目录> <exe路径> <等待秒数> <截图路径.png/jpg/bmp> [--no-frame] [exe参数...]
```

行为：启动 exe，等待 `<等待秒数>` 让窗口出现并渲染，截取主窗口保存到 `<截图路径>`，然后关闭。成功打印"截图已保存"，退出码 0。

- **默认截整个窗口**（含标题栏和边框）；加 `--no-frame` 只截客户区（裁掉标题栏边框）。`--no-frame` 可放在命令行任意位置。
- 扩展名决定格式：`.png` / `.jpg` / `.jpeg` / `.bmp`，其他后缀报错。
- 示例：

```sh
python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait_screenshot.py E:/myroom/projects/PulseEngine E:/myroom/projects/PulseEngine/build/windows/x64/debug/example-snake.exe 5 E:/myroom/projects/PulseEngine/build/out/snake.png --no-frame
```

## 关闭机制（两个脚本共用 scripts/window_close.py）

1. 通过 `EnumWindows` + `GetWindowThreadProcessId` 找到进程主窗口，发送 `WM_CLOSE`，等 8 秒让程序自行退出（程序会走正常资源清理流程）。
2. 超时（如程序弹了"是否保存"对话框）或找不到主窗口（如等待秒数太短、窗口还没创建）时，降级 `taskkill /T /F` 强杀进程树并打印警告——保证不残留进程。
3. POSIX 下为 SIGTERM → SIGKILL。

**判断关闭成功**：程序正常退出时 stderr 会出现引擎清理日志（如 `Snake example exited.`）；若看到"警告：...强制结束"说明走的是兜底强杀。

## 验证建议

- 截屏脚本跑完后检查图片：`file <截图路径>` 看尺寸——snake 窗口 800x600 客户区，含边框约 816x639，`--no-frame` 恰好 800x600，尺寸对不上说明窗口没起来或截图时机太早。
- 用 `tasklist //FI "IMAGENAME eq example-snake.exe"` 确认无残留进程。
- 等待秒数给足（≥5 秒）：等太短时窗口未创建，脚本只能强杀，也截不到内容。

## 踩坑清单

1. **Git Bash 下路径用正斜杠**：`E:/myroom/projects/PulseEngine/...`，不要用反斜杠。
2. **工作目录必须是存在的目录**，否则脚本报"当前目录不存在或不是目录"退出（exit 2）。exe 的资源若用相对路径（snake/breakout 的 `examples/snake/assets`），工作目录必须传仓库根。
3. **exe 参数里带 `-`/`--` 开头的选项**（如 `-fullscreen`）：本脚本自己的选项（`--no-frame`）会先被解析，其余参数原样传给 exe；若 exe 参数恰好与本脚本选项同名，用 `--` 分隔，如 `... out.png -- --no-frame`（传给 exe）。
4. **Python 版本 ≥ 3.9**（`--frame`/`--no-frame` 用 `argparse.BooleanOptionalAction`）；仓库环境 python 3.14 可用。脚本只用标准库。
5. **不要用截图尺寸反推窗口内容**：尺寸正确只说明窗口存在。
