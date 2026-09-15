# verify_game_title.ps1 — PulseEngine 游戏冒烟验证
#
# 启动宿主 launcher（游戏是它动态加载的包），等待数秒后读取窗口标题。
# 标题由游戏 UI 系统按 <GAME_NAME> - <score> 写入，出现预期文本即证明：
# 包加载成功、prefab/数据表异步加载完成、状态机流转到游戏态、UI 系统全链路正常。
# 不要用 stdout 日志验证（管道下 printf 全缓冲，超时杀进程会丢日志）。
#
# 用法（工作目录必须是构建输出目录，launcher.manifest.json 就在那里）:
#   pwsh -File verify_game_title.ps1 `
#       -Exe build/windows/x64/debug/launcher.exe `
#       -WorkingDir build/windows/x64/debug `
#       -ExeArgs E:/myroom/projects/PulseEngine/src,E:/myroom/projects/PulseEngine/examples `
#       -Expect "Bounce -"
#
# xmake run launcher 等价于：工作目录=构建输出目录、exe=launcher.exe、ExeArgs=绝对路径的 src 与 examples。
#
# 退出码: 0=通过  1=进程提前退出  2=标题不匹配

param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$WorkingDir = "",
    [string]$Expect = "",
    [string[]]$ExeArgs = @(),
    [int]$Seconds = 15
)

# pwsh -File 会把 "a,b" 当成一个整体参数传进来，这里按逗号/空白拆开再交给被拉起的进程。
$ExeArgList = @($ExeArgs | ForEach-Object { $_ -split '[,\s]+' } | Where-Object { $_ })

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class PulseTitleCheck {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
}
"@

$proc = Start-Process -FilePath $Exe -ArgumentList $ExeArgList -WorkingDirectory $WorkingDir -PassThru
Start-Sleep -Seconds $Seconds

$script:titles = @()
$script:mainWindow = [IntPtr]::Zero
$cb = [PulseTitleCheck+EnumWindowsProc]{ param($hWnd, $lParam)
    $owner = 0
    [PulseTitleCheck]::GetWindowThreadProcessId($hWnd, [ref]$owner) | Out-Null
    if ($owner -eq $proc.Id) {
        $sb = New-Object System.Text.StringBuilder 256
        [PulseTitleCheck]::GetWindowText($hWnd, $sb, 256) | Out-Null
        if ($sb.Length -gt 0) { $script:titles += $sb.ToString() }
        if ($script:mainWindow -eq [IntPtr]::Zero -and [PulseTitleCheck]::IsWindowVisible($hWnd)) {
            $script:mainWindow = $hWnd
        }
    }
    return $true
}
[PulseTitleCheck]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

function Close-GameProcess {
    if (-not $proc.HasExited -and $script:mainWindow -ne [IntPtr]::Zero) {
        [PulseTitleCheck]::PostMessageW($script:mainWindow, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        for ($i = 0; $i -lt 80; $i++) {
            Start-Sleep -Milliseconds 100
            if ($proc.HasExited) { break }
        }
    }
    if (-not $proc.HasExited) {
        & taskkill /PID $proc.Id /T /F 2>&1 | Out-Null
    }
}

$title = $script:titles -join " | "
Write-Output ("WINDOW_TITLES: " + $title)

$exitedEarly = $proc.HasExited
$exitCode = if ($exitedEarly) { $proc.ExitCode } else { 0 }
Close-GameProcess

if ($exitedEarly) {
    Write-Output ("FAIL: process exited early, code=" + $exitCode)
    exit 1
}
if ($Expect -ne "" -and $title -notmatch [regex]::Escape($Expect)) {
    Write-Output ("FAIL: expected title contains '" + $Expect + "'")
    exit 2
}

Write-Output "OK: game is running with expected UI title"
exit 0
