@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0.."

if not defined ANDROID_SDK (
    echo ERROR: ANDROID_SDK not set
    exit /b 1
)
if not defined ANDROID_NDK (
    echo ERROR: ANDROID_NDK not set
    exit /b 1
)
if not defined ABI (
    echo ERROR: ABI not set
    exit /b 1
)
if not defined BUILD_TOOLS (
    echo ERROR: BUILD_TOOLS not set
    exit /b 1
)
if not defined ANDROID_PLATFORM (
    echo ERROR: ANDROID_PLATFORM not set
    exit /b 1
)
if not defined KEYSTORE (
    echo ERROR: KEYSTORE not set
    exit /b 1
)
if not defined KEYSTORE_PASS (
    echo ERROR: KEYSTORE_PASS not set
    exit /b 1
)
if not defined KEYSTORE_ALIAS (
    echo ERROR: KEYSTORE_ALIAS not set
    exit /b 1
)
if not defined OUT_APK (
    echo ERROR: OUT_APK not set
    exit /b 1
)
if not defined JAVA_HOME (
    echo ERROR: JAVA_HOME not set
    exit /b 1
)

set "TOOLS=%ANDROID_SDK%\build-tools\%BUILD_TOOLS%"
set "AAPT=%TOOLS%\aapt.exe"
set "ZIPALIGN=%TOOLS%\zipalign.exe"
set "APKSIGNER=%TOOLS%\apksigner.bat"
set "D8=%TOOLS%\d8.bat"
set "ANDROID_JAR=%ANDROID_SDK%\platforms\%ANDROID_PLATFORM%\android.jar"

if not exist "%AAPT%" (
    echo ERROR: %AAPT% not found
    exit /b 1
)
if not exist "%ZIPALIGN%" (
    echo ERROR: %ZIPALIGN% not found
    exit /b 1
)
if not exist "%APKSIGNER%" (
    echo ERROR: %APKSIGNER% not found
    exit /b 1
)
if not exist "%D8%" (
    echo ERROR: %D8% not found
    exit /b 1
)
if not exist "%ANDROID_JAR%" (
    echo ERROR: %ANDROID_JAR% not found
    exit /b 1
)

set "JAR_TOOL=%JAVA_HOME%\bin\jar.exe"
if not exist "%JAR_TOOL%" (
    echo ERROR: %JAR_TOOL% not found
    exit /b 1
)
if not exist "%KEYSTORE%" (
    echo ERROR: %KEYSTORE% not found
    exit /b 1
)

if "%~1"=="-build" (
    call android\build_sdl_jar.bat
    if errorlevel 1 exit /b 1
    call install.bat
    if errorlevel 1 exit /b 1
)

set "SDL_JAR=build\android\SDL.jar"
if not exist "%SDL_JAR%" (
    echo ERROR: %SDL_JAR% not found, run build_sdl_jar.bat first
    exit /b 1
)

set "RELEASE_DIR=release"
if not exist "%RELEASE_DIR%\launcher.manifest.json" (
    echo ERROR: %RELEASE_DIR%\launcher.manifest.json not found, run install.bat first
    exit /b 1
)

set "STAGE=build\android\apk"
set "LIBDIR=%STAGE%\lib\%ABI%"
set "ASSETS=%STAGE%\assets"
set "DEX=%STAGE%\dex"
if exist "%STAGE%" rd /s /q "%STAGE%"
md "%LIBDIR%"
md "%ASSETS%"
md "%DEX%"

if not defined APK_MANIFEST (
    echo ERROR: APK_MANIFEST not set
    exit /b 1
)
if not exist "%APK_MANIFEST%" (
    echo ERROR: APK_MANIFEST not found: %APK_MANIFEST%
    exit /b 1
)
copy /y "%APK_MANIFEST%" "%STAGE%\AndroidManifest.xml" >nul
if defined APK_PACKAGE if not "%APK_PACKAGE%"=="" (
    powershell -NoProfile -Command "$q=[char]34;$p='%STAGE%\AndroidManifest.xml';$t=Get-Content -Raw $p;$r='(?is)(<manifest\b[^>]*\bpackage=)[^ >]+';$t2=$t -replace $r,('$1'+$q+'%APK_PACKAGE%'+$q);if($t2 -eq $t){Write-Output 'ERROR: APK_PACKAGE set but manifest has no package attribute';exit 1};[IO.File]::WriteAllText($p,$t2,(New-Object Text.UTF8Encoding $false))"
    if errorlevel 1 exit /b 1
)

md "%STAGE%\res"
if not defined APK_RES (
    echo ERROR: APK_RES not set
    exit /b 1
)
if not exist "%APK_RES%" (
    echo ERROR: APK_RES not found: %APK_RES%
    exit /b 1
)
robocopy "%APK_RES%" "%STAGE%\res" /E >nul
if errorlevel 8 exit /b 1
if defined APK_ICON if not "%APK_ICON%"=="" (
    if not exist "%APK_ICON%" (
        echo ERROR: APK_ICON not found: %APK_ICON%
        exit /b 1
    )
    md "%STAGE%\res\mipmap" 2>nul
    for %%m in (mdpi hdpi xhdpi xxhdpi xxxhdpi) do md "%STAGE%\res\mipmap-%%m" 2>nul
    copy /y "%APK_ICON%" "%STAGE%\res\mipmap\icon.png" >nul
    for %%x in (mdpi hdpi xhdpi xxhdpi xxxhdpi) do (
        copy /y "%APK_ICON%" "%STAGE%\res\mipmap-%%x\ic_launcher.png" >nul
        copy /y "%APK_ICON%" "%STAGE%\res\mipmap-%%x\icon.png" >nul
    )
)

robocopy "%RELEASE_DIR%" "%ASSETS%" /E /XF *.so *.dll *.exe *.pdb /XD bin lib >nul
if errorlevel 8 exit /b 1

for /r "%RELEASE_DIR%" %%f in (*.so) do (
    if not exist "%LIBDIR%\%%~nxf" copy /y "%%f" "%LIBDIR%\" >nul
)
if exist "%LIBDIR%\liblauncher.so" (
    if exist "%LIBDIR%\libmain.so" del /f /q "%LIBDIR%\libmain.so"
    ren "%LIBDIR%\liblauncher.so" libmain.so
)
if not exist "%LIBDIR%\libmain.so" (
    echo ERROR: libmain.so not found under %RELEASE_DIR%
    exit /b 1
)
if not exist "%LIBDIR%\libSDL3.so" (
    echo ERROR: libSDL3.so not found under %RELEASE_DIR%
    exit /b 1
)
if not exist "%LIBDIR%\libc++_shared.so" (
    set "CPPARCH="
    if "%ABI%"=="arm64-v8a" set "CPPARCH=aarch64-linux-android"
    if "%ABI%"=="armeabi-v7a" set "CPPARCH=arm-linux-androideabi"
    if "%ABI%"=="x86" set "CPPARCH=i686-linux-android"
    if "%ABI%"=="x86_64" set "CPPARCH=x86_64-linux-android"
    if not defined CPPARCH (
        echo ERROR: unsupported ABI: %ABI%
        exit /b 1
    )
    set "CPP_SHARED=%ANDROID_NDK%\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\!CPPARCH!\libc++_shared.so"
    if not exist "!CPP_SHARED!" (
        echo ERROR: !CPP_SHARED! not found
        exit /b 1
    )
    copy /y "!CPP_SHARED!" "%LIBDIR%\" >nul
)

echo packing resources...
"%AAPT%" package -f -M "%STAGE%\AndroidManifest.xml" -I "%ANDROID_JAR%" -F "%STAGE%\res.zip" -S "%STAGE%\res" -A "%ASSETS%"
if errorlevel 1 exit /b 1

echo compiling java...
call "%D8%" --lib "%ANDROID_JAR%" --output "%DEX%" "%SDL_JAR%"
if errorlevel 1 exit /b 1
if not exist "%DEX%\classes.dex" (
    echo ERROR: d8 produced no classes.dex
    exit /b 1
)

echo archiving libs and dex...
pushd "%STAGE%"
"%JAR_TOOL%" uf res.zip lib
if errorlevel 1 (
    popd
    exit /b 1
)
"%JAR_TOOL%" uf res.zip -C dex classes.dex
if errorlevel 1 (
    popd
    exit /b 1
)
popd

echo aligning apk...
"%ZIPALIGN%" -f 4 "%STAGE%\res.zip" "%STAGE%\aligned.apk"
if errorlevel 1 exit /b 1

md "build\android" 2>nul
echo signing apk...
call "%APKSIGNER%" sign --ks "%KEYSTORE%" --ks-pass "pass:%KEYSTORE_PASS%" --ks-key-alias "%KEYSTORE_ALIAS%" --out "%OUT_APK%" --in "%STAGE%\aligned.apk"
if errorlevel 1 exit /b 1

call "%APKSIGNER%" verify --print-certs "%OUT_APK%"
if errorlevel 1 exit /b 1

echo built %OUT_APK%
