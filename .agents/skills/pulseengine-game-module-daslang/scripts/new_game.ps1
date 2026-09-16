param(
    [Parameter(Mandatory = $true, Position = 0)][string]$Name,
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
