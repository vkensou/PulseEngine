xmake f -p android -a arm64-v8a -m debug -y
xmake build
call install.bat
call build_android_apk.bat
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 install "build\android\PulseEngine.apk"
timeout /t 1 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 shell logcat -c
timeout /t 1 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 shell am start -n com.pulse.snake/org.libsdl.app.SDLActivity
timeout /t 10 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 logcat -d > log.txt
timeout /t 1 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 shell am force-stop com.pulse.snake