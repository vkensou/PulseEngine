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
Copy-Item (Join-Path $Templates "game_module.das") "$GameDir/${Name}_module.das"
Copy-Item (Join-Path $Templates "package.json") "$GameDir/package.json"
Copy-Item (Join-Path $Templates "schema/game_config.schema") "$GameDir/schema/${Name}_config.schema"
Copy-Item (Join-Path $Templates "assets/*") "$GameDir/assets/"
Rename-Item "$GameDir/assets/game_config.datatable" "${Name}_config.datatable"

$TextExtensions = '.das', '.json', '.prefab', '.material', '.shader', '.datatable', '.schema', '.slang'
Get-ChildItem $GameDir -Recurse -File | Where-Object { $TextExtensions -contains $_.Extension } | ForEach-Object {
    $path = $_.FullName
    $text = [System.IO.File]::ReadAllText($path)
    $text = $text.Replace('<game>', $Name).Replace('game_config', "${Name}_config")
    [System.IO.File]::WriteAllText($path, $text, $Utf8NoBom)
}

Write-Output "已生成 examples/$Name （包名 $Name / 前缀 $Pascal / 标题显示名 $DisplayName）"
Write-Output "daslang 包是纯脚本包，xmake.lua 里不需要 target，脚本未改动它"
Write-Output ""
Write-Output "剩余三步（按顺序做完才能跑起来）："
Write-Output ""
Write-Output "1) 把包加进 src/launcher/launcher.manifest.json（宿主 launcher 只加载清单里列出的包）："
Write-Output "   - packages 数组末尾追加：  { `"name`": `"$Name`" }"
Write-Output "   - 开发期一次只跑一个游戏：把其它游戏包条目（snake / snake_daslang 等）删掉，pulse_* 插件条目保持不动"
Write-Output "   - 把 pulse_window 条目的 config.primary_window.title 改成 `"$DisplayName -`"（daslang 侧没有改窗口标题的绑定，标题由清单决定）"
Write-Output ""
Write-Output "2) 写游戏逻辑：examples/$Name/$Name`_module.das 是空模板，需要自己填组件/资源/系统/状态机与 [export] importModule"
Write-Output "   - 现成参考实现：examples/snake_daslang/snake_module.das"
Write-Output "   - 要用数据表就 require ${Name}_tables public（由第 3 步生成）"
Write-Output ""
Write-Output "3) 生成数据表绑定（schema 名 = 文件名 $Name`_config，数据文件名必须同名）："
Write-Output "   xmake build tablegen"
Write-Output "   build\windows\x64\debug\tablegen.exe --out-das examples\$Name\${Name}_tables.das examples\$Name\schema\${Name}_config.schema"
Write-Output ""
Write-Output "构建与运行（daslang 包没有 xmake target，不用 build 游戏本身）："
Write-Output "   xmake build -r launcher          # 改了 launcher.manifest.json 必须 -r，copy_manifest 是 after_build"
Write-Output "   xmake run launcher"
