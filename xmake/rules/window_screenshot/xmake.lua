--!A custom xmake rule for window/rendering screenshot tests.
--
-- This rule encapsulates:
--   1. Launch the target executable
--   2. Wait a few seconds for the window to appear and render
--   3. Capture the client area (no title bar / window border)
--   4. Close the program
--   5. Compare the captured image with a baseline image
--   6. Pass only when similarity >= threshold
--
-- The rule is implemented in Lua. The low-level Windows-specific window
-- capture/close operations use PowerShell + Win32, so no Python is required.

rule("pulse.window_screenshot_test")

    on_test(function (target, opt)

        -- This screenshot flow is currently implemented for Windows only.
        if os.host() ~= "windows" then
            opt.errors = "当前非windows，不支持"
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        import("core.base.json")
        import("lib.detect.find_tool")

        local rulename = "pulse.window_screenshot_test"
        local wait = tonumber(target:extraconf("rules", rulename, "wait") or 5)
        local threshold = tonumber(target:extraconf("rules", rulename, "threshold") or 0.95)
        local tolerance = tonumber(target:extraconf("rules", rulename, "tolerance") or 8)

        -- Baseline screenshot. The default location is tests/<module>/baseline.png.
        local baseline = target:extraconf("rules", rulename, "baseline")
        if not baseline then
            local module = target:name():gsub("^test%-", "")
            baseline = path.join(os.projectdir(), "tests", module, "baseline.png")
        end
        baseline = path.absolute(baseline, os.projectdir())

        -- Actual screenshot is emitted below build/tests so failures are easy to inspect.
        local shotdir = target:extraconf("rules", rulename, "outputdir")
        if not shotdir then
            shotdir = path.join(os.projectdir(), "build", "tests", "window_screenshot")
        end
        shotdir = path.absolute(shotdir, os.projectdir())
        local shotname = opt.name:gsub("[/\\]", "_") .. ".png"
        local actualshot = path.join(shotdir, shotname)

        -- Check baseline first: never launch the program when the reference is missing.
        if not os.isfile(baseline) then
            local hint = string.format(
                "缺少基准截图: %s\n请先用 .agents/skills/test-tool-for-program-with-window 生成客户区截图，"
                .. "并保存到该路径后重试。例如:\n"
                .. "python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait_screenshot.py"
                .. " $(projectdir) %s %s %s --no-frame",
                baseline,
                path.absolute(target:targetfile(), os.projectdir()),
                wait,
                baseline)
            opt.errors = hint
            cprint("${color.failure}%s${clear}", hint)
            return false
        end

        local targetfile = path.absolute(target:targetfile(), os.projectdir())
        local rundir = opt.rundir or target:rundir()
        local runargs = table.wrap(opt.runargs or target:get("runargs"))

        local powershell = assert(find_tool("powershell"), "powershell not found!")

        -- Keep all values out of the -Command string to avoid quoting problems.
        local envs = {
            PULSE_WTEST_EXE       = targetfile,
            PULSE_WTEST_CURDIR    = rundir,
            PULSE_WTEST_ARGS_JSON = (#runargs > 0) and json.encode(runargs) or "[]",
            PULSE_WTEST_WAIT      = tostring(wait),
            PULSE_WTEST_SHOT      = actualshot,
            PULSE_WTEST_BASELINE  = baseline,
            PULSE_WTEST_THRESHOLD = tostring(threshold),
            PULSE_WTEST_TOLERANCE = tostring(tolerance)
        }

        local script = [[
$ErrorActionPreference = 'Stop'
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
    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
'@
Add-Type -TypeDefinition $nativeCode
[void][WindowCaptureNative]::SetProcessDpiAwarenessContext([IntPtr](-4))

function Save-ClientScreenshot([System.Diagnostics.Process]$proc, [string]$outputPath) {
    $proc.Refresh()
    $handle = $proc.MainWindowHandle
    if ($handle -eq [IntPtr]::Zero) {
        throw "进程没有可截取的顶层窗口。"
    }

    $windowRect = New-Object WindowCaptureNative+RECT
    if (-not [WindowCaptureNative]::GetWindowRect($handle, [ref]$windowRect)) {
        throw "无法读取窗口尺寸。"
    }
    $width = $windowRect.Right - $windowRect.Left
    $height = $windowRect.Bottom - $windowRect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "窗口尺寸无效。"
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $hdc = $graphics.GetHdc()
    try {
        if (-not [WindowCaptureNative]::PrintWindow($handle, $hdc, 2)) {
            throw "窗口截取失败。"
        }
    } finally {
        $graphics.ReleaseHdc($hdc)
    }

    try {
        # Crop to the client area so title bar / window border are not compared.
        $clientRect = New-Object WindowCaptureNative+RECT
        if (-not [WindowCaptureNative]::GetClientRect($handle, [ref]$clientRect)) {
            throw "无法读取客户区尺寸。"
        }
        $clientPoint = New-Object WindowCaptureNative+POINT
        if (-not [WindowCaptureNative]::ClientToScreen($handle, [ref]$clientPoint)) {
            throw "无法获取客户区位置。"
        }
        $offsetX = $clientPoint.X - $windowRect.Left
        $offsetY = $clientPoint.Y - $windowRect.Top
        $clientWidth = $clientRect.Right - $clientRect.Left
        $clientHeight = $clientRect.Bottom - $clientRect.Top
        if ($clientWidth -le 0 -or $clientHeight -le 0) {
            throw "客户区尺寸无效。"
        }

        $clientBitmap = $bitmap.Clone(
            [System.Drawing.Rectangle]::new($offsetX, $offsetY, $clientWidth, $clientHeight),
            $bitmap.PixelFormat)
        $bitmap.Dispose()
        $bitmap = $clientBitmap
        $bitmap.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Get-Similarity([string]$baselinePath, [string]$actualPath, [int]$tolerance) {
    $bmpA = New-Object System.Drawing.Bitmap($baselinePath)
    try {
        $bmpB = New-Object System.Drawing.Bitmap($actualPath)
        try {
            if ($bmpA.Width -ne $bmpB.Width -or $bmpA.Height -ne $bmpB.Height) {
                throw "尺寸不一致: $($bmpA.Width)x$($bmpA.Height) vs $($bmpB.Width)x$($bmpB.Height)"
            }
            [long]$total = [long]$bmpA.Width * [long]$bmpA.Height
            [long]$diff = 0
            for ($y = 0; $y -lt $bmpA.Height; $y++) {
                for ($x = 0; $x -lt $bmpA.Width; $x++) {
                    $pA = $bmpA.GetPixel($x, $y)
                    $pB = $bmpB.GetPixel($x, $y)
                    $dr = [math]::Abs([int]$pA.R - [int]$pB.R)
                    $dg = [math]::Abs([int]$pA.G - [int]$pB.G)
                    $db = [math]::Abs([int]$pA.B - [int]$pB.B)
                    if ($dr -gt $tolerance -or $dg -gt $tolerance -or $db -gt $tolerance) {
                        $diff++
                    }
                }
            }
            return 1.0 - ([double]$diff / [double]$total)
        } finally {
            $bmpB.Dispose()
        }
    } finally {
        $bmpA.Dispose()
    }
}

$exe       = $env:PULSE_WTEST_EXE
$curdir    = $env:PULSE_WTEST_CURDIR
$argsJson  = $env:PULSE_WTEST_ARGS_JSON
$wait      = [double]$env:PULSE_WTEST_WAIT
$shot      = $env:PULSE_WTEST_SHOT
$baseline  = $env:PULSE_WTEST_BASELINE
$threshold = [double]$env:PULSE_WTEST_THRESHOLD
$tolerance = [int]$env:PULSE_WTEST_TOLERANCE

$exeArgs = @()
if ($argsJson) {
    $decodedArgs = $argsJson | ConvertFrom-Json
    if ($decodedArgs -ne $null) {
        $exeArgs = @($decodedArgs)
    }
}

$shotDir = Split-Path -Parent $shot
if ($shotDir -and -not (Test-Path -LiteralPath $shotDir)) {
    New-Item -ItemType Directory -Path $shotDir -Force | Out-Null
}

$proc = $null
$exitCode = 0
$resultLine = "RESULT=PASS"
$similarity = -1.0

try {
    if ($exeArgs.Count -gt 0) {
        $proc = Start-Process -FilePath $exe -ArgumentList $exeArgs -WorkingDirectory $curdir -PassThru
    } else {
        $proc = Start-Process -FilePath $exe -WorkingDirectory $curdir -PassThru
    }
    Start-Sleep -Seconds $wait
    Save-ClientScreenshot $proc $shot
    $similarity = Get-Similarity $baseline $shot $tolerance
    Write-Output ("SIMILARITY={0}" -f $similarity.ToString("F6"))
    if ($similarity -ge $threshold) {
        $resultLine = "RESULT=PASS"
    } else {
        $resultLine = "RESULT=FAIL"
        $exitCode = 1
    }
} catch {
    $resultLine = "RESULT=ERROR"
    Write-Output ("ERROR=" + $_.Exception.Message)
    $exitCode = 2
} finally {
    if ($proc -and -not $proc.HasExited) {
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle
        if ($hwnd -ne [IntPtr]::Zero) {
            [void][WindowCaptureNative]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
            if (-not $proc.WaitForExit(8000)) {
                Write-Warning "进程未响应 WM_CLOSE，强制结束。"
                & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
            }
        } else {
            Write-Warning "找不到主窗口，强制结束。"
            & taskkill /PID $proc.Id /T /F 2>$null | Out-Null
        }
        $proc.WaitForExit()
    }
}

Write-Output $resultLine
exit $exitCode
]]

        os.mkdir(shotdir)
        local outfile = path.join(shotdir, "run_" .. shotname .. ".out")
        local errfile = path.join(shotdir, "run_" .. shotname .. ".err")
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

        local similarity = outdata:match("SIMILARITY=([%d%.]+)")
        local result = outdata:match("RESULT=(%a+)")

        if ok == 0 and result == "PASS" then
            cprint("${color.success}screenshot test passed, similarity: %s${clear}", similarity or "?")
            return true
        end

        if result == "FAIL" then
            opt.errors = string.format("截图相似度不足: %s (阈值: %s)\n实际截图: %s\n基准截图: %s",
                similarity or "?", tostring(threshold), actualshot, baseline)
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        local errmsg = outdata:match("ERROR=(.+)") or errdata or syserrors or ""
        opt.errors = string.format("窗口截图测试失败: %s\n实际截图: %s", errmsg, actualshot)
        cprint("${color.failure}%s${clear}", opt.errors)
        return false
    end)