#!/bin/bash
set -euo pipefail

engine_root="${1:-${UE_ROOT:-}}"
if [[ -z "$engine_root" ]]; then
    echo 'Pass the UE 5.8 installation folder as argument, or set UE_ROOT.' >&2
    exit 1
fi
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project="$repo_root/unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject"
version_file="$engine_root/Engine/Build/Build.version"
major=$(/usr/bin/plutil -extract MajorVersion raw -o - "$version_file")
minor=$(/usr/bin/plutil -extract MinorVersion raw -o - "$version_file")
required=$(/usr/bin/plutil -extract EngineAssociation raw -o - "$project")
if [[ "$major.$minor" != "$required" ]]; then
    echo "Engine version differs from project requirement: $required." >&2
    exit 1
fi
build_script="$engine_root/Engine/Build/BatchFiles/Mac/Build.sh"
if [[ ! -f "$build_script" ]]; then
    echo "Missing build script: $build_script" >&2
    exit 1
fi
exec /bin/bash "$build_script" FlyingCabFlightLabEditor Mac Development "-Project=$project" -WaitMutex
