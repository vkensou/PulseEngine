@echo off
setlocal
cd /d "%~dp0"
set "ANDROID_SDK=D:\pgtools\android\sdk"
set "ANDROID_PLATFORM=android-36"
set "JAVA_HOME=D:\pgtools\openjdk\openjdk17"
set "SDL_SRC=android\sdl_java"
set "JAR_OUT=android\SDL.jar"
call android\build_sdl_jar.bat %*
exit /b %ERRORLEVEL%
