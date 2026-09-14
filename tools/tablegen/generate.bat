@echo off
rem Regenerate every checked-in tablegen output: tests fixture + both snake examples.
setlocal enabledelayedexpansion
set ROOT=%~dp0..\..
set TABLEGEN=%ROOT%\build\windows\x64\debug\tablegen.exe
if not exist "%TABLEGEN%" (
    echo tablegen.exe not found, run: xmake build tablegen
    exit /b 1
)

set FILES=
for %%f in ("%ROOT%\tests\datatable\schema\*.schema") do set FILES=!FILES! "%%f"
if not exist "%ROOT%\tests\datatable\das" mkdir "%ROOT%\tests\datatable\das"
"%TABLEGEN%" --out-h "%ROOT%\tests\datatable\schema\tables_generated.h" --out-cpp "%ROOT%\tests\datatable\schema\tables_generated.cpp" --out-das "%ROOT%\tests\datatable\das\das_tables.das" !FILES!
if errorlevel 1 exit /b 1

"%TABLEGEN%" --out-h "%ROOT%\examples\snake\schema\tables_generated.h" --out-cpp "%ROOT%\examples\snake\schema\tables_generated.cpp" "%ROOT%\examples\snake\schema\snake_config.schema"
if errorlevel 1 exit /b 1

"%TABLEGEN%" --out-das "%ROOT%\examples\snake_daslang\snake_tables.das" "%ROOT%\examples\snake_daslang\schema\snake_config.schema"
if errorlevel 1 exit /b 1

echo Done.
