# Single-purpose PowerShell script: compare two images and print similarity.

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$baselinePath = $env:PULSE_CMP_BASELINE
$actualPath   = $env:PULSE_CMP_ACTUAL
$tolerance    = [int]$env:PULSE_CMP_TOLERANCE

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
        $similarity = 1.0 - ([double]$diff / [double]$total)
        Write-Output ("SIMILARITY={0}" -f $similarity.ToString("F6"))
    } finally {
        $bmpB.Dispose()
    }
} finally {
    $bmpA.Dispose()
}