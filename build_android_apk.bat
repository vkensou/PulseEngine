@echo off
setlocal
cd /d "%~dp0"
set "ANDROID_SDK=D:\pgtools\android\sdk"
set "ANDROID_NDK=D:\pgtools\android\sdk\ndk\27.1.12297006"
set "ABI=arm64-v8a"
set "BUILD_TOOLS=36.0.0"
set "ANDROID_PLATFORM=android-36"
set "APK_PACKAGE=com.pulse.snake"
set "APK_MANIFEST=android\AndroidManifest.xml"
set "APK_RES=android\res"
set "APK_ICON=tests\renderer\baseline.png"
set "KEYSTORE=android\xmake-debug.jks"
set "KEYSTORE_PASS=123456"
set "KEYSTORE_ALIAS=xmake"
set "OUT_APK=build\android\PulseEngine.apk"
set "JAVA_HOME=D:\pgtools\openjdk\openjdk17"
set "SDL_JAR=android\SDL.jar"
set "EXTRA_LIB_DIRS=android-binaries-1.4.304.1\lib"
call android\build_apk.bat %*
exit /b %ERRORLEVEL%
