# Single-purpose PowerShell script: start the target executable and print its PID.

$ErrorActionPreference = 'Stop'

$exe      = $env:PULSE_PROC_EXE
$curdir   = $env:PULSE_PROC_CURDIR
$argsJson = $env:PULSE_PROC_ARGS_JSON

# Optional extra DLL search dirs (e.g. shared deps' targetdirs collected by
# xmake's runenvs), prepended so the exe can find pulse_*.dll even when
# package_output moves them into build/.../packages/<name>/.
$extraPath = $env:PULSE_PROC_PATH
if ($extraPath) {
    $env:PATH = $extraPath + ';' + $env:PATH
}

$exeArgs = @()
if ($argsJson) {
    $decodedArgs = $argsJson | ConvertFrom-Json
    if ($decodedArgs -ne $null) {
        $exeArgs = @($decodedArgs)
    }
}

# -NoNewWindow: a console subsystem exe would otherwise spawn a new console window
# owned by its own PID, and Process.MainWindowHandle may pick that console instead
# of the SDL window, corrupting capture and close.
if ($exeArgs.Count -gt 0) {
    $proc = Start-Process -FilePath $exe -ArgumentList $exeArgs -WorkingDirectory $curdir -PassThru -NoNewWindow
} else {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $curdir -PassThru -NoNewWindow
}

Write-Output ("PID=" + $proc.Id)