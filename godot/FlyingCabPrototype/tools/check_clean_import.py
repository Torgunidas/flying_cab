#!/usr/bin/env python3
"""Import an isolated source copy, without .godot, and reject logged script errors.

Godot may return exit code 0 even when its editor scan reports parse errors.
The copy and log remain under ignored build/ for diagnosis.
"""
from pathlib import Path
import argparse
import re
import shutil
import subprocess
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, help="Godot executable")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    executable = shutil.which(args.engine)
    engine = Path(executable or args.engine).resolve()
    if not engine.is_file():
        parser.error("Godot executable not found")
    (root / "build").mkdir(exist_ok=True)
    copy = Path(tempfile.mkdtemp(prefix="clean-import-", dir=root / "build"))
    for folder in ("scripts", "scenes", "resources", "assets", "shaders", "tests"):
        shutil.copytree(root / folder, copy / folder)
    shutil.copy2(root / "project.godot", copy / "project.godot")
    (copy / "build").mkdir()
    log = copy / "import.log"
    output = copy / "output.log"
    with output.open("w") as stream:
        try:
            result = subprocess.run(
                [str(engine), "--headless", "--editor", "--import", "--path",
                 str(copy), "--log-file", str(log)],
                stdout=stream, stderr=subprocess.STDOUT, timeout=120, check=False,
            )
        except subprocess.TimeoutExpired:
            print(f"CLEAN_IMPORT: FAIL timeout; log={output.relative_to(root)}")
            return 1
    text = output.read_text(errors="replace")
    errors = [line for line in text.splitlines()
              if re.search(r"SCRIPT ERROR:|Parse Error:|Compile Error:|^ERROR:", line)]
    passed = result.returncode == 0 and not errors
    print(f"CLEAN_IMPORT: {'PASS' if passed else 'FAIL'} "
          f"exit={result.returncode} errors={len(errors)} log={output.relative_to(root)}")
    for error in errors:
        print(error)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
