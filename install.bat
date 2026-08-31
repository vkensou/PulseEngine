xmake build

set "dest_file=release"

rd /s /q "%dest_file%"
md "%dest_file%"

xmake install -o "%dest_file%" launcher

copy /y "%dest_file%\bin\*.*" "%dest_file%\"
rd /s /q "%dest_file%\bin"

copy /y "%dest_file%\lib\*.*" "%dest_file%\"
rd /s /q "%dest_file%\lib"

copy "src\launcher\launcher.manifest.json" "%dest_file%"

xmake install -o "%dest_file%/packages/pulse_window" pulse_window
xmake install -o "%dest_file%/packages/pulse_input" pulse_input
xmake install -o "%dest_file%/packages/pulse_asset" pulse_asset
xmake install -o "%dest_file%/packages/pulse_graphics" pulse_graphics
xmake install -o "%dest_file%/packages/pulse_transform" pulse_transform
xmake install -o "%dest_file%/packages/pulse_renderer" pulse_renderer
xmake install -o "%dest_file%/packages/pulse_imgui" pulse_imgui
xmake install -o "%dest_file%/packages/pulse_daslang" pulse_daslang
xmake install -o "%dest_file%/packages/snake" example-snake
xcopy "examples\snake_daslang\*.*" "%dest_file%\packages\snake_daslang" /e /i /y

