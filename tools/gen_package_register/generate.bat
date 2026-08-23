@echo off
rem Generate pulse_package_register for every pulse module under src.
rem Batch driver: calls generate.lua once per module idl.
for /d %%d in (..\..\src\pulse_*) do (
    for %%i in ("%%d\idl\*.idl") do (
        if exist "%%i" (
            echo.
            .\lua54.exe generate.lua "%%i"
            if errorlevel 1 exit /b 1
        )
    )
)
echo.
echo Done.