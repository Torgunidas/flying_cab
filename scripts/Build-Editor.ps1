[CmdletBinding()]
param([string]$EngineRoot = $env:UE_ROOT)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $EngineRoot) { throw 'Pass -EngineRoot pointing to UE 5.8, or set UE_ROOT.' }
$projectPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject'
$version = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
$project = Get-Content -LiteralPath $projectPath -Raw | ConvertFrom-Json
if ("$($version.MajorVersion).$($version.MinorVersion)" -ne $project.EngineAssociation) {
    throw "Engine version differs from project requirement: $($project.EngineAssociation)."
}
$buildScript = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
if (-not (Test-Path -LiteralPath $buildScript -PathType Leaf)) { throw "Missing build script: $buildScript" }
& $buildScript FlyingCabFlightLabEditor Win64 Development "-Project=$projectPath" -WaitMutex
exit $LASTEXITCODE
