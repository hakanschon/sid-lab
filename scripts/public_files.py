#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""The reviewed public file set; local evidence and development history stay local."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def public_files(root=ROOT):
    manifest = json.loads((root / '.public-files.json').read_text())
    paths = [root / p for p in manifest['files']]
    for directory in manifest['directories']:
        paths.extend(p for p in (root / directory).rglob('*') if p.is_file())
    result = []
    for p in sorted(set(paths)):
        relative = p.relative_to(root)
        if '__pycache__' in relative.parts or p.suffix in ('.pyc', '.pyo') or p.name == '.DS_Store':
            continue
        if not p.is_file() or p.is_symlink():
            raise ValueError('Missing file or unsupported symlink: ' + str(relative))
        result.append(relative)
    return result


def dependencies(root=ROOT):
    return [dict(zip(('name', 'version', 'revision', 'url'), line.split()))
            for line in (root / 'dependencies.lock').read_text().splitlines() if line.strip()]
