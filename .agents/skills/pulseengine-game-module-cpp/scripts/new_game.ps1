param(
    [Parameter(Mandatory = $true, Position = 0)][string]$Name,
    [string]$DisplayName = "",
    [string]$Root = ""
)

$ErrorActionPreference = "Stop"

if (-not $Root) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot "../../../..")).Path
}
$Root = (Resolve-Path $Root).Path

if ($Name -notmatch '^[a-z][a-z0-9_]*$') {
    throw "游戏名 '$Name' 非法：只允许小写字母、数字、下划线，且以字母开头"
}

$Pascal = ($Name -split '_' | Where-Object { $_ } | ForEach-Object { $_.Substring(0, 1).ToUpper() + $_.Substring(1) }) -join ''
if (-not $DisplayName) {
    $DisplayName = $Pascal
}

$Templates = Join-Path (Split-Path $PSScriptRoot -Parent) "templates"
$GameDir = Join-Path $Root "examples/$Name"

if (Test-Path $GameDir) {
    throw "examples/$Name 已存在，先删除或换一个游戏名"
}

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

New-Item -ItemType Directory -Path $GameDir, "$GameDir/assets", "$GameDir/schema" | Out-Null
Copy-Item (Join-Path $Templates "game.h") "$GameDir/$Name.h"
Copy-Item (Join-Path $Templates "game.cpp") "$GameDir/$Name.cpp"
Copy-Item (Join-Path $Templates "package.json") "$GameDir/package.json"
Copy-Item (Join-Path $Templates "schema/game_config.schema") "$GameDir/schema/${Name}_config.schema"
Copy-Item (Join-Path $Templates "assets/*") "$GameDir/assets/"
Rename-Item "$GameDir/assets/game_config.datatable" "${Name}_config.datatable"

foreach ($asset in 'Quad.obj', 'color.slang', 'color.vert.spv', 'color.frag.spv') {
    $source = Join-Path $Root "examples/snake/assets/$asset"
    if (-not (Test-Path $source)) {
        throw "缺少可复用资产 examples/snake/assets/$asset"
    }
    Copy-Item $source "$GameDir/assets/$asset"
}

$TextExtensions = '.h', '.cpp', '.json', '.prefab', '.material', '.shader', '.datatable', '.schema', '.slang'
Get-ChildItem $GameDir -Recurse -File | Where-Object { $TextExtensions -contains $_.Extension } | ForEach-Object {
    $path = $_.FullName
    $text = [System.IO.File]::ReadAllText($path)
    $text = $text.Replace('<game>', $Name).Replace('<Game>', $Pascal).Replace('<GAME_NAME>', $DisplayName).Replace('game_config', "${Name}_config").Replace('GameConfig', "${Pascal}Config")
    [System.IO.File]::WriteAllText($path, $text, $Utf8NoBom)
}

$xmakePath = Join-Path $Root "xmake.lua"
$xmakeText = [System.IO.File]::ReadAllText($xmakePath)
$targetMarker = "target(`"example-$Name`")"
if (-not $xmakeText.Contains($targetMarker)) {
    $block = [System.IO.File]::ReadAllText((Join-Path $Templates "xmake_target.lua")).Replace('<game>', $Name)
    if (-not $xmakeText.EndsWith("`n")) {
        $xmakeText += "`n"
    }
    $xmakeText += "`n" + $block.TrimEnd() + "`n"
    [System.IO.File]::WriteAllText($xmakePath, $xmakeText, $Utf8NoBom)
    $xmakeState = "已把 target(`"example-$Name`") 追加到 xmake.lua 末尾"
} else {
    $xmakeState = "xmake.lua 里已有 target(`"example-$Name`")，未改动"
}

Write-Output "已生成 examples/$Name （模块名 $Name / 前缀 $Pascal / 标题显示名 $DisplayName）"
Write-Output $xmakeState
Write-Output ""
Write-Output "剩余三步（按顺序做完才能跑起来）："
Write-Output ""
Write-Output "1) 把包加进 src/launcher/launcher.manifest.json（宿主 launcher 只加载清单里列出的包）："
Write-Output "   - packages 数组末尾追加：  { `"name`": `"$Name`" }"
Write-Output "   - 开发期一次只跑一个游戏：把其它游戏包条目（snake / snake_daslang 等）删掉，pulse_* 插件条目保持不动"
Write-Output "   - 窗口初始标题在 pulse_window 条目的 config.primary_window.title，按需改；游戏 UI 每帧会覆盖它"
Write-Output ""
Write-Output "2) 生成数据表绑定（schema 名 = 文件名 $Name`_config，数据文件名必须同名）："
Write-Output "   xmake build tablegen"
Write-Output "   build\windows\x64\debug\tablegen.exe --out-h examples\$Name\schema\tables_generated.h --out-cpp examples\$Name\schema\tables_generated.cpp examples\$Name\schema\${Name}_config.schema"
Write-Output ""
Write-Output "3) 生成模块与插件代码（产物 $Name`_module.h / $Name`_module.cpp / $Name`_plugin.cpp，禁止手改）："
Write-Output "   tools\idl\lua54.exe tools\generate_module\generate_module.lua generate examples/$Name/$Name.h examples/$Name/package.json"
Write-Output ""
Write-Output "构建与运行："
Write-Output "   xmake build example-$Name"
Write-Output "   xmake build -r launcher          # 改了 launcher.manifest.json 必须 -r，copy_manifest 是 after_build"
Write-Output "   xmake run launcher"
