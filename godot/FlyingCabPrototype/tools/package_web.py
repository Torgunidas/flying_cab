#!/usr/bin/env python3
"""Package only runtime files, leaving unrelated local exports/archives untouched."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
web = root / "build/web"
required = {"index.html", "index.js", "index.wasm", "index.pck"}
optional = {
    "index.png", "index.icon.png", "index.apple-touch-icon.png",
    "index.audio.worklet.js", "index.audio.position.worklet.js",
}
missing = [name for name in required if not (web / name).is_file() or (web / name).stat().st_size == 0]
if missing:
    raise SystemExit("Incomplete web export: " + ", ".join(sorted(missing)))
files = sorted(web / name for name in required | optional if (web / name).is_file())
info = json.loads((root / "resources/build_info.json").read_text())
output = root / "build/FlyingCabFlight01-itch.zip"
with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
    for file in files:
        archive.write(file, file.name)
versioned = root / "build" / (info["build_id"] + "-itch.zip")
shutil.copy2(output, versioned)
manifest = {
    "build_id": info["build_id"], "source_sha256": info["source_sha256"],
    "files": {file.name: hashlib.sha256(file.read_bytes()).hexdigest() for file in files},
}
(root / "build/web-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(output)
print(versioned)
