# Read-only drift check. Never updates the approved baseline or game files.
[CmdletBinding()]
param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$ManifestPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'docs/INPUT_CANONICAL_BASELINE_2026-09-04.json')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectDirectory = [IO.Path]::GetFullPath($ProjectRoot)
$manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or @($manifest.files).Count -eq 0) {
    throw 'Unsupported or empty input baseline manifest.'
}
$differences = @()
foreach ($entry in $manifest.files) {
    $filePath = [IO.Path]::GetFullPath((Join-Path $projectDirectory $entry.path))
    $projectPrefix = $projectDirectory.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $filePath.StartsWith($projectPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Manifest path is outside the project: $($entry.path)"
    }
    if (-not (Test-Path -LiteralPath $filePath -PathType Leaf)) {
        $differences += "MISSING $($entry.path)"
        continue
    }
    if ($entry.format -eq 'utf8-lf') {
        $sourceText = [IO.File]::ReadAllText($filePath).Replace("`r`n", "`n").Replace("`r", "`n")
        $hasher = [Security.Cryptography.SHA256]::Create()
        try {
            $digest = $hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($sourceText))
            $actualHash = ([BitConverter]::ToString($digest)).Replace('-', '')
        }
        finally { $hasher.Dispose() }
    }
    elseif ($entry.format -eq 'binary') {
        $actualHash = (Get-FileHash -LiteralPath $filePath -Algorithm SHA256).Hash
    }
    else { throw "Unknown hash format: $($entry.format)" }
    if ($actualHash -ne $entry.sha256) {
        $differences += "CHANGED $($entry.path)"
    }
}
if ($differences.Count -gt 0) {
    Write-Output "INPUT BASELINE DIFFERS: $($manifest.id)"
    $differences | Write-Output
    Write-Output 'Review the differences. Do not overwrite user changes or regenerate the approved manifest.'
    exit 1
}
Write-Output "INPUT BASELINE MATCH: $($manifest.id) ($(@($manifest.files).Count) files)."
Write-Output 'This verifies listed source/assets only, not behavior, build binaries or local overrides.'
exit 0
