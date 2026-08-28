# Single-purpose PowerShell script: capture the client area of a process window.

. $env:PULSE_CAPTURE_NATIVE

[void][WindowTestNative]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type -AssemblyName System.Drawing

$pidValue   = [int]$env:PULSE_CAPTURE_PID
$outputPath = $env:PULSE_CAPTURE_SHOT

$proc = Get-Process -Id $pidValue -ErrorAction Stop
$proc.Refresh()
$handle = $proc.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) {
    throw "进程没有可截取的顶层窗口。"
}

$windowRect = New-Object WindowTestNative+RECT
if (-not [WindowTestNative]::GetWindowRect($handle, [ref]$windowRect)) {
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
    if (-not [WindowTestNative]::PrintWindow($handle, $hdc, 2)) {
        throw "窗口截取失败。"
    }
} finally {
    $graphics.ReleaseHdc($hdc)
}

try {
    # Crop to the client area so title bar / window border are not compared.
    $clientRect = New-Object WindowTestNative+RECT
    if (-not [WindowTestNative]::GetClientRect($handle, [ref]$clientRect)) {
        throw "无法读取客户区尺寸。"
    }
    $clientPoint = New-Object WindowTestNative+POINT
    if (-not [WindowTestNative]::ClientToScreen($handle, [ref]$clientPoint)) {
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