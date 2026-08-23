--!A custom xmake rule for window-title smoke tests.
--
-- This rule is used by test-window: it launches the target executable,
-- waits for a top-level window with the expected title to appear, sends
-- WM_CLOSE to that window, and force-kills the process tree if it does not
-- exit in time. It is intentionally separate from pulse.window_screenshot_test
-- because this test only needs to verify the window title, not a screenshot.

rule("pulse.window_title_test")

    on_test(function (target, opt)

        -- This title-test flow is currently implemented for Windows only.
        if os.host() ~= "windows" then
            opt.errors = "当前非windows，不支持"
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        import("core.base.json")
        import("lib.detect.find_tool")

        local rulename = "pulse.window_title_test"
        local title = target:extraconf("rules", rulename, "title") or "test-window retitled"
        local wait = tonumber(target:extraconf("rules", rulename, "wait") or 10)
        local close_ms = tonumber(target:extraconf("rules", rulename, "close_timeout_ms") or 8000)

        local targetfile = path.absolute(target:targetfile(), os.projectdir())
        local rundir = opt.rundir or target:rundir()
        local runargs = table.wrap(opt.runargs or target:get("runargs"))
        local powershell = assert(find_tool("powershell"), "powershell not found!")

        local envs = {
            PULSE_WT_EXE       = targetfile,
            PULSE_WT_CURDIR    = rundir,
            PULSE_WT_ARGS_JSON = (#runargs > 0) and json.encode(runargs) or "[]",
            PULSE_WT_TITLE     = title,
            PULSE_WT_WAIT      = tostring(wait),
            PULSE_WT_CLOSE_MS  = tostring(close_ms),
        }

        local script = [[
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class WindowTitleTestNative {
    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
'@

$exe      = $env:PULSE_WT_EXE
$curdir   = $env:PULSE_WT_CURDIR
$argsJson = $env:PULSE_WT_ARGS_JSON
$title    = $env:PULSE_WT_TITLE
$wait     = [double]$env:PULSE_WT_WAIT
$closeMs  = [int]$env:PULSE_WT_CLOSE_MS

$exeArgs = @()
if ($argsJson) {
    $decoded = $argsJson | ConvertFrom-Json
    if ($decoded -ne $null) {
        $exeArgs = @($decoded)
    }
}

$proc = $null
$result = "RESULT=FAIL"
$found = $false
$forceKilled = $false

try {
    if ($exeArgs.Count -gt 0) {
        $proc = Start-Process -FilePath $exe -ArgumentList $exeArgs -WorkingDirectory $curdir -PassThru
    } else {
        $proc = Start-Process -FilePath $exe -WorkingDirectory $curdir -PassThru
    }

    $deadline = (Get-Date).AddSeconds($wait)
    while (-not $proc.HasExited -and (Get-Date) -lt $deadline) {
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle
        if ($hwnd -ne [IntPtr]::Zero) {
            $proc.Refresh()
            if ($proc.MainWindowTitle -ceq $title) {
                $found = $true
                Write-Output ("FOUND_TITLE=" + $title)
                [void][WindowTitleTestNative]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
                if (-not $proc.WaitForExit($closeMs)) {
                    Write-Warning "进程未响应 WM_CLOSE，强制结束。"
                    $forceKilled = $true
                    & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
                    $proc.WaitForExit()
                }
                break
            }
        }
        Start-Sleep -Milliseconds 100
    }

    if ($found) {
        $result = "RESULT=PASS"
        if ($forceKilled) {
            Write-Output "NOTE=FORCE_KILLED"
        }
    } else {
        if ($proc.HasExited) {
            Write-Output ("ERROR=进程提前退出，未找到标题为 '" + $title + "' 的窗口，exit=" + $proc.ExitCode)
        } else {
            Write-Output ("ERROR=超时未找到标题为 '" + $title + "' 的窗口")
            & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
            $proc.WaitForExit()
        }
        $result = "RESULT=FAIL"
    }
} catch {
    Write-Output ("ERROR=" + $_.Exception.Message)
    $result = "RESULT=ERROR"
    if ($proc -and -not $proc.HasExited) {
        & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
        $proc.WaitForExit()
    }
}

Write-Output $result
if ($result -eq "RESULT=PASS") { exit 0 } else { exit 1 }
]]

        local outdir = path.join(os.projectdir(), "build", "tests", "window_title")
        os.mkdir(outdir)
        local logname = opt.name:gsub("[/\\]", "_")
        local outfile = path.join(outdir, logname .. ".out")
        local errfile = path.join(outdir, logname .. ".err")
        os.tryrm(outfile)
        os.tryrm(errfile)

        local ok, syserrors = os.execv(powershell.program,
            {"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", script},
            {try = true, curdir = rundir, envs = envs, stdout = outfile, stderr = errfile})

        local outdata = (os.isfile(outfile) and io.readfile(outfile)) or ""
        local errdata = (os.isfile(errfile) and io.readfile(errfile)) or ""
        os.tryrm(outfile)
        os.tryrm(errfile)

        opt.stdout = outdata
        opt.stderr = errdata

        local result = outdata:match("RESULT=(%a+)")
        if ok == 0 and result == "PASS" then
            cprint("${color.success}window title test passed: %s${clear}", outdata:match("FOUND_TITLE=(.+)") or "ok")
            return true
        end

        local errmsg = outdata:match("ERROR=(.+)") or errdata or syserrors or "未知错误"
        opt.errors = string.format("窗口标题测试失败: %s", errmsg)
        cprint("${color.failure}%s${clear}", opt.errors)
        return false
    end)
