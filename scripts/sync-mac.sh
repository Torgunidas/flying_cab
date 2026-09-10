#!/bin/bash
# Synchronizes this clone with its upstream and rebuilds the editor modules.
# A plain `git pull` is not enough: the editor loads the existing Binaries/Mac module
# without any prompt (it only compares the BuildId with the engine), so after pulling
# C++ changes the game silently keeps running the old code. See docs/WORKING_ON_MAC_AND_PC.md.
set -euo pipefail

engine_root="${1:-${UE_ROOT:-}}"
if [[ -z "$engine_root" ]]; then
    echo 'Pass the UE 5.8 installation folder as argument, or set UE_ROOT.' >&2
    exit 1
fi
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project_dir="$repo_root/unreal/FlyingCabFlightLab"

if pgrep -x UnrealEditor >/dev/null 2>&1 || pgrep -x UnrealEditor-Cmd >/dev/null 2>&1; then
    echo 'Close Unreal Editor before synchronizing; a running editor keeps the old module loaded.' >&2
    exit 1
fi

cd "$repo_root"
branch="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$branch" != "main" ]]; then
    echo "Warning: current branch is '$branch', not main. Syncing this branch with its upstream." >&2
fi
if [[ -n "$(git status --porcelain)" ]]; then
    echo 'Warning: working tree has local changes. They are kept; commit them before switching computers.' >&2
fi

before="$(git rev-parse HEAD)"
git pull --ff-only
after="$(git rev-parse HEAD)"
if [[ "$before" != "$after" ]]; then
    echo "Updated ${before:0:7} -> ${after:0:7}."
    changed="$(git diff --name-only "$before" "$after" -- "$project_dir/Source" "$project_dir/FlyingCabFlightLab.uproject")"
    if [[ -n "$changed" ]]; then
        echo 'C++ sources or the project descriptor changed:'
        echo "$changed"
    fi
else
    echo "No new commits; HEAD stays at ${after:0:7}."
fi

# Always build. UnrealBuildTool finishes in seconds when nothing changed, and a stale
# module would otherwise load without warning.
bash "$repo_root/scripts/build-editor-mac.sh" "$engine_root"
echo "Editor modules are current for $(git rev-parse --short HEAD). Open $project_dir/FlyingCabFlightLab.uproject."
