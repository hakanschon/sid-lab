#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify pinned Git checkouts, or every dependency file in a complete source archive."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from public_files import ROOT, dependencies


def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()


def verify(root=ROOT):
    archive_file = root / 'DEPENDENCY_SOURCES.json'
    archive = json.loads(archive_file.read_text()) if archive_file.is_file() else None
    for dep in dependencies(root):
        base = root / 'third_party' / dep['name']
        if (base / '.git').exists():
            if git('-C', str(base), 'rev-parse', 'HEAD') != dep['revision']:
                raise ValueError(dep['name'] + ': wrong revision; initialize the pinned submodules')
            if git('-C', str(base), 'status', '--porcelain', '--untracked-files=normal'):
                raise ValueError(dep['name'] + ': source has local changes')
        elif archive:
            entry = archive['dependencies'][dep['name']]
            if entry['revision'] != dep['revision'] or not entry['files']:
                raise ValueError(dep['name'] + ': source manifest revision mismatch')
            actual = {str(p.relative_to(base)) for p in base.rglob('*') if p.is_file()}
            if actual != set(entry['files']):
                raise ValueError(dep['name'] + ': source archive file inventory mismatch')
            for name, expected in entry['files'].items():
                relative = Path(name)
                if relative.is_absolute() or '..' in relative.parts:
                    raise ValueError('Unsafe source-manifest path')
                p = base / relative
                if p.is_symlink() or hashlib.sha256(p.read_bytes()).hexdigest() != expected:
                    raise ValueError(dep['name'] + ': source archive checksum mismatch: ' + name)
        else:
            raise ValueError('Missing ' + dep['name'] + '. Clone recursively or use the complete SID Lab source archive.')


if __name__ == '__main__':
    try:
        verify()
        print('Pinned dependency sources verified.')
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
