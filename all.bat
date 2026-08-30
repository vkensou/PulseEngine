xmake f -p android -a arm64-v8a -m debug -y
xmake build
call install.bat
call build_android_apk.bat
D:\pgtools\android\sdk\platform-tools\adb.exe -s 9708aa53 install "build\android\PulseEngine.apk"
timeout /t 1 /nobreak
call clearlog.bat
timeout /t 1 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 9708aa53 shell am start -n com.pulse.snake/org.libsdl.app.SDLActivity
timeout /t 10 /nobreak
call getlog.bat