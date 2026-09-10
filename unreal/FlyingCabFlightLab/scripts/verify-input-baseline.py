#!/usr/bin/env python3
"""Read-only drift check of the canonical input baseline, equivalent to Verify-InputBaseline.ps1.

Same algorithm: utf8-lf entries are decoded as UTF-8 (BOM stripped), line endings normalized
to LF and hashed with SHA-256; binary entries are hashed byte for byte. Never updates the
approved manifest or game files. Exit code 0 on match, 1 on any difference.
"""
import argparse
import hashlib
import json
import os
import sys


def main() -> int:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--project-root', default=os.path.dirname(script_dir),
                        help='Unreal project folder (default: parent of this scripts folder)')
    parser.add_argument('--manifest', default=None,
                        help='Manifest path (default: docs/INPUT_CANONICAL_BASELINE_2026-09-04.json in the project)')
    args = parser.parse_args()

    project = os.path.realpath(args.project_root)
    manifest_path = args.manifest or os.path.join(project, 'docs', 'INPUT_CANONICAL_BASELINE_2026-09-04.json')
    with open(manifest_path, encoding='utf-8-sig') as handle:
        manifest = json.load(handle)
    files = manifest.get('files') or []
    if manifest.get('schemaVersion') != 1 or not files:
        print('Unsupported or empty input baseline manifest.', file=sys.stderr)
        return 2

    differences = []
    for entry in files:
        relative = entry['path']
        path = os.path.realpath(os.path.join(project, relative))
        if os.path.commonpath([project, path]) != project:
            print(f'Manifest path is outside the project: {relative}', file=sys.stderr)
            return 2
        if not os.path.isfile(path):
            differences.append(f'MISSING {relative}')
            continue
        with open(path, 'rb') as handle:
            data = handle.read()
        fmt = entry['format']
        if fmt == 'utf8-lf':
            text = data.decode('utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')
            actual = hashlib.sha256(text.encode('utf-8')).hexdigest().upper()
        elif fmt == 'binary':
            actual = hashlib.sha256(data).hexdigest().upper()
        else:
            print(f'Unknown hash format: {fmt}', file=sys.stderr)
            return 2
        if actual != entry['sha256'].upper():
            differences.append(f'CHANGED {relative}')

    if differences:
        print(f"INPUT BASELINE DIFFERS: {manifest['id']}")
        for line in differences:
            print(line)
        print('Review the differences. Do not overwrite user changes or regenerate the approved manifest.')
        return 1
    print(f"INPUT BASELINE MATCH: {manifest['id']} ({len(files)} files).")
    print('This verifies listed source/assets only, not behavior, build binaries or local overrides.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
