D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 shell logcat -c
timeout /t 1 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 shell am start -n com.pulse.snake/org.libsdl.app.SDLActivity
timeout /t 10 /nobreak
D:\pgtools\android\sdk\platform-tools\adb.exe -s 127.0.0.1:5555 logcat -d > log.txt