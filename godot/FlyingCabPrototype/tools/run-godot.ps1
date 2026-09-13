# Shared Windows entry point; installation paths remain local.
$ErrorActionPreference = 'Stop'
$fcProject = Split-Path $PSScriptRoot -Parent
$fcEngine = $env:FC_GODOT_BIN
$fcLocalPath = Join-Path $fcProject 'build/godot-path.txt'
if (-not $fcEngine -and (Test-Path -LiteralPath $fcLocalPath)) {
    $fcEngine = (Get-Content -LiteralPath $fcLocalPath -Raw).Trim()
}
if (-not $fcEngine) {
    $fcEngine = Join-Path $fcProject 'build/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
    if (-not (Test-Path -LiteralPath $fcEngine)) {
        $fcCommand = Get-Command godot -ErrorAction SilentlyContinue
        if (-not $fcCommand) { throw 'Install Godot 4.7.2 or set FC_GODOT_BIN to its executable.' }
        $fcEngine = $fcCommand.Source
    }
}
New-Item -ItemType Directory -Path (Join-Path $fcProject 'build') -Force | Out-Null
New-Item -ItemType File -Path (Join-Path $fcProject 'build/.gdignore') -Force | Out-Null
& $fcEngine --path $fcProject @args
exit $LASTEXITCODE
