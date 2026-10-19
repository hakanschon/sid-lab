#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Package complete source, including pinned dependencies and their file checksums."""
import argparse
import hashlib
import io
import json
import subprocess
import tarfile
from pathlib import Path
from public_files import ROOT, public_files, dependencies
from verify_dependencies import verify


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    verify()
    files = public_files()
    manifest = {'format': 'SIDLabDependencySources', 'dependencies': {}}
    previous = ROOT / 'DEPENDENCY_SOURCES.json'
    old = json.loads(previous.read_text()) if previous.exists() else None
    for dep in dependencies():
        base = ROOT / 'third_party' / dep['name']
        names = (subprocess.check_output(['git', '-C', str(base), 'ls-files', '-z']).decode().split('\0')[:-1]
                 if (base / '.git').exists() else list(old['dependencies'][dep['name']]['files']))
        checksums = {}
        for name in sorted(names):
            p = base / name
            if p.is_symlink() or not p.is_file():
                raise ValueError('Unsupported dependency source entry: ' + name)
            checksums[name] = hashlib.sha256(p.read_bytes()).hexdigest()
            files.append(p.relative_to(ROOT))
        manifest['dependencies'][dep['name']] = {'revision': dep['revision'], 'files': checksums}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(args.output, 'w:gz', format=tarfile.PAX_FORMAT) as archive:
        for relative in sorted(files):
            info = archive.gettarinfo(str(ROOT / relative), arcname='sid-lab/' + relative.as_posix())
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = 0
            # No filesystem-owner metadata or local absolute paths enter the archive.
            info.pax_headers = {}
            with (ROOT / relative).open('rb') as source:
                archive.addfile(info, source)
        data = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
        info = tarfile.TarInfo('sid-lab/DEPENDENCY_SOURCES.json')
        info.size = len(data)
        info.mode = 0o644
        archive.addfile(info, io.BytesIO(data))
    print('Complete corresponding source: ' + str(args.output))


if __name__ == '__main__':
    main()
