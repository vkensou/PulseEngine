# Single-purpose PowerShell script: close a process by PID, force-kill on timeout.

. $env:PULSE_CLOSE_NATIVE

$ErrorActionPreference = 'Stop'

$pidValue = [int]$env:PULSE_CLOSE_PID
$closeMs  = [int]$env:PULSE_CLOSE_MS

$proc = Get-Process -Id $pidValue -ErrorAction SilentlyContinue
if (-not $proc) {
    exit 0
}

$proc.Refresh()
$hwnd = $proc.MainWindowHandle
if ($hwnd -ne [IntPtr]::Zero) {
    [void][WindowTestNative]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    if (-not $proc.WaitForExit($closeMs)) {
        Write-Warning "进程未响应 WM_CLOSE，强制结束。"
        & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
        $proc.WaitForExit()
        Write-Output "FORCE_KILLED=1"
    }
} else {
    Write-Warning "找不到主窗口，强制结束。"
    & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
    $proc.WaitForExit()
    Write-Output "FORCE_KILLED=1"
}