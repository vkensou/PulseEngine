# Single-purpose PowerShell script: wait for a process window with the expected title.

$ErrorActionPreference = 'Stop'

$pidValue = [int]$env:PULSE_TITLE_PID
$title    = $env:PULSE_TITLE
$wait     = [double]$env:PULSE_TITLE_WAIT

$proc = Get-Process -Id $pidValue -ErrorAction Stop
$deadline = (Get-Date).AddSeconds($wait)
$found = $false

while (-not $proc.HasExited -and (Get-Date) -lt $deadline) {
    $proc.Refresh()
    $hwnd = $proc.MainWindowHandle
    if ($hwnd -ne [IntPtr]::Zero) {
        $proc.Refresh()
        if ($proc.MainWindowTitle -ceq $title) {
            $found = $true
            Write-Output ("FOUND_TITLE=" + $title)
            break
        }
    }
    Start-Sleep -Milliseconds 100
}

if (-not $found) {
    if ($proc.HasExited) {
        Write-Output ("ERROR=进程提前退出，未找到标题为 '" + $title + "' 的窗口，exit=" + $proc.ExitCode)
    } else {
        Write-Output ("ERROR=超时未找到标题为 '" + $title + "' 的窗口")
    }
    exit 1
}

exit 0