#!/usr/bin/env python3
"""Stamp the exact runtime source set; never includes local paths or user data."""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[1]
paths = [root / "project.godot"]
for folder in ("scripts", "scenes", "shaders", "resources", "assets"):
    paths.extend(p for p in (root / folder).rglob("*") if p.is_file() and p.suffix not in (".uid", ".import") and p.name != "build_info.json")
digest = hashlib.sha256()
for path in sorted(paths):
    digest.update(path.relative_to(root).as_posix().encode())
    digest.update(b"\0")
    digest.update(path.read_bytes())
build_id = "city03-" + digest.hexdigest()[:12]
(root / "resources/build_info.json").write_text(json.dumps({"build_id": build_id, "source_sha256": digest.hexdigest()}, indent=2) + "\n")
print(build_id)
