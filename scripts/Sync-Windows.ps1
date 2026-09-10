# Synchronizes this clone with its upstream and rebuilds the editor modules.
# A plain `git pull` is not enough: the editor loads the existing Binaries/Win64 module
# without any prompt (it only compares the BuildId with the engine), so after pulling
# C++ changes the game silently keeps running the old code. See docs/WORKING_ON_MAC_AND_PC.md.
[CmdletBinding()]
param([string]$EngineRoot = $env:UE_ROOT)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $EngineRoot) { throw 'Pass -EngineRoot pointing to UE 5.8, or set UE_ROOT.' }
$repoRoot = Split-Path -Parent $PSScriptRoot
$projectDir = Join-Path $repoRoot 'unreal/FlyingCabFlightLab'

if (Get-Process -Name 'UnrealEditor', 'UnrealEditor-Cmd' -ErrorAction SilentlyContinue) {
    throw 'Close Unreal Editor before synchronizing; a running editor keeps the old module loaded.'
}

Push-Location $repoRoot
try {
    $branch = (& git rev-parse --abbrev-ref HEAD).Trim()
    if ($branch -ne 'main') { Write-Warning "Current branch is '$branch', not main. Syncing this branch with its upstream." }
    if (& git status --porcelain) { Write-Warning 'Working tree has local changes. They are kept; commit them before switching computers.' }

    $before = (& git rev-parse HEAD).Trim()
    & git pull --ff-only
    if ($LASTEXITCODE -ne 0) { throw 'git pull --ff-only failed. Compare the histories manually; never force-push or reset.' }
    $after = (& git rev-parse HEAD).Trim()
    if ($before -ne $after) {
        Write-Output "Updated $($before.Substring(0, 7)) -> $($after.Substring(0, 7))."
        $changed = & git diff --name-only $before $after -- (Join-Path $projectDir 'Source') (Join-Path $projectDir 'FlyingCabFlightLab.uproject')
        if ($changed) {
            Write-Output 'C++ sources or the project descriptor changed:'
            $changed | Write-Output
        }
    }
    else { Write-Output "No new commits; HEAD stays at $($after.Substring(0, 7))." }

    # Always build. UnrealBuildTool finishes in seconds when nothing changed, and a stale
    # module would otherwise load without warning.
    & (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw "Editor build failed with exit code $LASTEXITCODE." }
    $head = (& git rev-parse --short HEAD).Trim()
    Write-Output "Editor modules are current for $head. Open $projectDir\FlyingCabFlightLab.uproject."
}
finally { Pop-Location }
