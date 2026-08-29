@echo off
setlocal
cd /d "%~dp0.."

if not defined ANDROID_SDK (
    echo ERROR: ANDROID_SDK not set
    exit /b 1
)
if not defined ANDROID_PLATFORM (
    echo ERROR: ANDROID_PLATFORM not set
    exit /b 1
)
if not defined JAVA_HOME (
    echo ERROR: JAVA_HOME not set
    exit /b 1
)
if not defined SDL_SRC (
    echo ERROR: SDL_SRC not set
    exit /b 1
)
if not defined JAR_OUT (
    echo ERROR: JAR_OUT not set
    exit /b 1
)

set "ANDROID_JAR=%ANDROID_SDK%\platforms\%ANDROID_PLATFORM%\android.jar"
if not exist "%ANDROID_JAR%" (
    echo ERROR: %ANDROID_JAR% not found
    exit /b 1
)

set "JAVAC=%JAVA_HOME%\bin\javac.exe"
set "JAR_TOOL=%JAVA_HOME%\bin\jar.exe"
if not exist "%JAVAC%" (
    echo ERROR: %JAVAC% not found
    exit /b 1
)
if not exist "%JAR_TOOL%" (
    echo ERROR: %JAR_TOOL% not found
    exit /b 1
)

set "BUILD_DIR=build\android\sdl_jar"
set "CLASSES=%BUILD_DIR%\classes"

if exist "%JAR_OUT%" if not "%~1"=="-f" (
    echo SDL.jar already exists: %JAR_OUT%
    echo run "%~nx0 -f" to rebuild
    exit /b 0
)

if not exist "%SDL_SRC%" (
    echo ERROR: %SDL_SRC% not found
    exit /b 1
)

if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
md "%BUILD_DIR%"
md "%CLASSES%"
dir /s /b "%SDL_SRC%\*.java" > "%BUILD_DIR%\sources.txt"

"%JAVAC%" --release 8 --class-path "%ANDROID_JAR%" -encoding UTF-8 -d "%CLASSES%" @"%BUILD_DIR%\sources.txt"
if errorlevel 1 exit /b 1

"%JAR_TOOL%" cf "%JAR_OUT%" -C "%CLASSES%" .
if errorlevel 1 exit /b 1

echo built %JAR_OUT%