#!/usr/bin/env python3
"""Run an executable, wait, capture its main window, then close it."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from window_close import stop_process_tree


def parse_args() -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description="运行 exe，等待后截取其窗口并关闭。")
    parser.add_argument("working_directory", type=Path, help="exe 的当前目录")
    parser.add_argument("executable", help="exe 路径或名称")
    parser.add_argument("seconds", type=float, help="截屏前等待秒数")
    parser.add_argument("screenshot_path", type=Path, help="PNG/JPG/BMP 截图路径")
    parser.add_argument(
        "--frame",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="截图是否包含标题栏和窗口边框（默认包含）；用 --no-frame 只截客户区。",
    )
    # 用 parse_known_args 而非 REMAINDER：剩余参数（含 -/-- 开头的 exe 参数）
    # 全部原样交给 exe，而本脚本自己的选项可以出现在任意位置。
    args, extra = parser.parse_known_args()
    return args, extra


def take_screenshot(process_id: int, output_path: Path, include_frame: bool) -> None:
    if os.name != "nt":
        raise RuntimeError("截屏功能目前仅支持 Windows。")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    formats = {".png": "Png", ".jpg": "Jpeg", ".jpeg": "Jpeg", ".bmp": "Bmp"}
    image_format = formats.get(output_path.suffix.lower())
    if image_format is None:
        raise ValueError("截图路径必须使用 .png、.jpg、.jpeg 或 .bmp 扩展名。")

    # Values are passed through environment variables because powershell.exe
    # treats arguments following -Command as additional PowerShell source text.
    script = r"""
$ProcessId = [int]$env:RUN_EXE_SCREENSHOT_PID
$OutputPath = $env:RUN_EXE_SCREENSHOT_PATH
$ImageFormat = $env:RUN_EXE_SCREENSHOT_FORMAT
$IncludeFrame = $env:RUN_EXE_SCREENSHOT_FRAME -eq "1"
Add-Type -AssemblyName System.Drawing
$nativeCode = @'
using System;
using System.Runtime.InteropServices;
public static class WindowCaptureNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")]
    public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint flags);
}
'@
Add-Type -TypeDefinition $nativeCode
# DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (-4) prevents GetWindowRect from
# returning DPI-virtualized dimensions on displays using 125%/150% scaling.
[void][WindowCaptureNative]::SetProcessDpiAwarenessContext([IntPtr](-4))
$target = Get-Process -Id $ProcessId -ErrorAction Stop
$target.Refresh()
$handle = $target.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) { throw "进程没有可截取的顶层窗口。" }
$windowRect = New-Object WindowCaptureNative+RECT
if (-not [WindowCaptureNative]::GetWindowRect($handle, [ref]$windowRect)) { throw "无法读取窗口尺寸。" }
$width = $windowRect.Right - $windowRect.Left
$height = $windowRect.Bottom - $windowRect.Top
if ($width -le 0 -or $height -le 0) { throw "窗口尺寸无效。" }
$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$hdc = $graphics.GetHdc()
try {
    if (-not [WindowCaptureNative]::PrintWindow($handle, $hdc, 2)) { throw "窗口截取失败。" }
} finally {
    $graphics.ReleaseHdc($hdc)
}
try {
    if (-not $IncludeFrame) {
        # 客户区在窗口内的偏移 = 客户区左上角的屏幕坐标 - 窗口左上角的屏幕坐标，
        # 据此从整窗截图中裁剪出不含标题栏和边框的客户区。
        $clientRect = New-Object WindowCaptureNative+RECT
        if (-not [WindowCaptureNative]::GetClientRect($handle, [ref]$clientRect)) { throw "无法读取客户区尺寸。" }
        $clientPoint = New-Object WindowCaptureNative+POINT
        if (-not [WindowCaptureNative]::ClientToScreen($handle, [ref]$clientPoint)) { throw "无法获取客户区位置。" }
        $offsetX = $clientPoint.X - $windowRect.Left
        $offsetY = $clientPoint.Y - $windowRect.Top
        $clientWidth = $clientRect.Right - $clientRect.Left
        $clientHeight = $clientRect.Bottom - $clientRect.Top
        if ($clientWidth -le 0 -or $clientHeight -le 0) { throw "客户区尺寸无效。" }
        $clientBitmap = $bitmap.Clone(
            [System.Drawing.Rectangle]::new($offsetX, $offsetY, $clientWidth, $clientHeight),
            $bitmap.PixelFormat
        )
        $bitmap.Dispose()
        $bitmap = $clientBitmap
    }
    $format = [System.Drawing.Imaging.ImageFormat]::$ImageFormat
    $bitmap.Save($OutputPath, $format)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}
"""
    environment = os.environ.copy()
    environment["RUN_EXE_SCREENSHOT_PID"] = str(process_id)
    environment["RUN_EXE_SCREENSHOT_PATH"] = str(output_path)
    environment["RUN_EXE_SCREENSHOT_FORMAT"] = image_format
    environment["RUN_EXE_SCREENSHOT_FRAME"] = "1" if include_frame else "0"
    subprocess.run(
        ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", script],
        env=environment,
        check=True,
    )


def main() -> int:
    args, executable_args = parse_args()
    working_directory = args.working_directory.expanduser().resolve()
    screenshot_path = args.screenshot_path.expanduser().resolve()
    if not working_directory.is_dir():
        print(f"错误：当前目录不存在或不是目录：{working_directory}", file=sys.stderr)
        return 2
    if args.seconds < 0:
        print("错误：等待秒数不能为负数。", file=sys.stderr)
        return 2

    process = subprocess.Popen(
        [args.executable, *executable_args],
        cwd=working_directory,
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0,
        start_new_session=os.name != "nt",
    )
    try:
        time.sleep(args.seconds)
        take_screenshot(process.pid, screenshot_path, args.frame)
        print(f"截图已保存：{screenshot_path}")
        return 0
    except KeyboardInterrupt:
        return 130
    except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
        print(f"错误：{error}", file=sys.stderr)
        return 1
    finally:
        stop_process_tree(process)
        process.wait()


if __name__ == "__main__":
    raise SystemExit(main())
