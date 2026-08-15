#!/usr/bin/env python3
"""Run an executable in a specified working directory for a limited time."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from window_close import stop_process_tree


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="以指定路径为当前目录运行 exe，并在指定秒数后关闭。"
    )
    parser.add_argument("working_directory", type=Path, help="运行 exe 时使用的当前目录")
    parser.add_argument("executable", help="要运行的 exe 路径或名称")
    parser.add_argument("seconds", type=float, help="运行时长（秒）")
    parser.add_argument(
        "executable_args",
        nargs=argparse.REMAINDER,
        help="传递给 exe 的参数（如有以 -- 开头的参数，可先写 --）",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    working_directory = args.working_directory.expanduser().resolve()

    if not working_directory.is_dir():
        print(f"错误：当前目录不存在或不是目录：{working_directory}", file=sys.stderr)
        return 2
    if args.seconds < 0:
        print("错误：运行秒数不能为负数。", file=sys.stderr)
        return 2

    command = [args.executable, *args.executable_args]
    creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
    process = subprocess.Popen(
        command,
        cwd=working_directory,
        creationflags=creationflags,
        start_new_session=os.name != "nt",
    )

    try:
        return process.wait(timeout=args.seconds)
    except subprocess.TimeoutExpired:
        stop_process_tree(process)
        process.wait()
        return 0
    except KeyboardInterrupt:
        stop_process_tree(process)
        process.wait()
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
