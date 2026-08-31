"""Graceful process shutdown shared by the window test scripts.

Windows: send WM_CLOSE to the process's main window and wait for it to exit
on its own; fall back to force-killing the process tree on timeout.
POSIX: send SIGTERM, then SIGKILL on timeout.
"""

from __future__ import annotations

import ctypes
import os
import signal
import subprocess
import sys
from ctypes import wintypes

WM_CLOSE = 0x0010
# 发送 WM_CLOSE 后等待进程自行退出的秒数，超时则强制结束进程树。
DEFAULT_GRACE_SECONDS = 8.0

if os.name == "nt":
    _user32 = ctypes.WinDLL("user32", use_last_error=True)

    _EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    _user32.EnumWindows.argtypes = [_EnumWindowsProc, wintypes.LPARAM]
    _user32.EnumWindows.restype = wintypes.BOOL
    _user32.GetWindowThreadProcessId.argtypes = [
        wintypes.HWND,
        ctypes.POINTER(wintypes.DWORD),
    ]
    _user32.GetWindowThreadProcessId.restype = wintypes.DWORD
    _user32.IsWindowVisible.argtypes = [wintypes.HWND]
    _user32.IsWindowVisible.restype = wintypes.BOOL
    _user32.PostMessageW.argtypes = [
        wintypes.HWND,
        wintypes.UINT,
        wintypes.WPARAM,
        wintypes.LPARAM,
    ]
    _user32.PostMessageW.restype = wintypes.BOOL


def _find_main_window(pid: int) -> int | None:
    """Return the first visible top-level window owned by pid, or None."""
    found: list[int] = []

    @_EnumWindowsProc
    def callback(hwnd, lparam):
        window_pid = wintypes.DWORD()
        _user32.GetWindowThreadProcessId(hwnd, ctypes.byref(window_pid))
        if window_pid.value == pid and _user32.IsWindowVisible(hwnd):
            found.append(hwnd)
            return False  # stop enumerating
        return True

    _user32.EnumWindows(callback, 0)
    return found[0] if found else None


def _send_wm_close(pid: int) -> bool:
    hwnd = _find_main_window(pid)
    if hwnd is None:
        return False
    return bool(_user32.PostMessageW(hwnd, WM_CLOSE, 0, 0))


def stop_process_tree(
    process: subprocess.Popen,
    grace_seconds: float = DEFAULT_GRACE_SECONDS,
) -> None:
    """Close a process, preferring graceful shutdown over killing.

    On Windows the main window is sent WM_CLOSE and the process gets
    grace_seconds to exit on its own; if it has no window or does not exit in
    time, the whole process tree is force-killed (taskkill /T /F) so nothing
    is left behind. On POSIX the process group receives SIGTERM, then SIGKILL
    after 3 seconds.
    """
    if process.poll() is not None:
        return

    if os.name == "nt":
        if _send_wm_close(process.pid):
            try:
                process.wait(timeout=grace_seconds)
                return
            except subprocess.TimeoutExpired:
                print(
                    f"警告：进程 {process.pid} 未响应 WM_CLOSE，强制结束。",
                    file=sys.stderr,
                )
        else:
            print(
                f"警告：找不到进程 {process.pid} 的主窗口，强制结束。",
                file=sys.stderr,
            )
        subprocess.run(
            ["taskkill", "/PID", str(process.pid), "/T", "/F"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    else:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
