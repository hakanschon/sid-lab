#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Audit public files, reachable Git history and decompressed release archives."""
import argparse
import hashlib
import json
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path, PurePosixPath

from check_public import sensitive_findings
from public_files import ROOT, public_files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--history', action='store_true', help='Scan every object reachable from local Git refs')
    parser.add_argument('--forbid-text', action='append', default=[], help='Additional private marker to reject (not printed)')
    parser.add_argument('archives', nargs='*', type=Path)
    args = parser.parse_args()
    forbidden = [value.casefold() for value in args.forbid_text]
    exceptions = json.loads((ROOT / 'scripts/upstream_audit_exceptions.json').read_text())
    errors = set()
    reviewed_exceptions = set()
    counts = {'public files': 0, 'history objects': 0, 'archive files': 0}

    def inspect(label, name, data):
        normalized = PurePosixPath(name).as_posix().removeprefix('sid-lab/')
        exception = exceptions.get(normalized, {})
        for finding in sensitive_findings(name, data):
            if (finding in exception.get('findings', [])
                    and hashlib.sha256(data).hexdigest() == exception.get('sha256')):
                reviewed_exceptions.add(normalized)
            else:
                errors.add(label + ': ' + finding)
        # UTF-8 and UTF-16 cover source, common image metadata and bundle strings.
        texts = [data.decode('utf-8', errors='ignore').casefold(),
                 data.decode('utf-16-le', errors='ignore').casefold()]
        if any(marker in text for marker in forbidden for text in texts):
            errors.add(label + ': forbidden private marker')

    for relative in public_files():
        inspect(str(relative), relative, (ROOT / relative).read_bytes())
        counts['public files'] += 1

    if args.history:
        def git(*command):
            return subprocess.check_output(['git', '-C', str(ROOT), *command])
        for entry in git('rev-list', '--objects', '--all').decode().splitlines():
            oid, _, name = entry.partition(' ')
            kind = git('cat-file', '-t', oid).decode().strip()
            data = git('cat-file', kind, oid)
            inspect('Git ' + kind + ' ' + oid[:12], name or oid, data)
            counts['history objects'] += 1

    def archive_name(name):
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or '\\' in name:
            raise ValueError('Unsafe archive member path')
        return path

    for archive in args.archives:
        if zipfile.is_zipfile(archive):
            with zipfile.ZipFile(archive) as source:
                for info in source.infolist():
                    path = archive_name(info.filename)
                    if info.is_dir():
                        continue
                    if (info.external_attr >> 16) & 0o170000 == 0o120000:
                        raise ValueError('Symlink in binary archive')
                    inspect(archive.name + '/' + str(path), path, source.read(info))
                    counts['archive files'] += 1
        else:
            with tarfile.open(archive, 'r:*') as source:
                for info in source:
                    path = archive_name(info.name)
                    if info.isdir():
                        continue
                    if not info.isfile():
                        raise ValueError('Non-regular file in source archive')
                    if info.uname or info.gname or info.uid or info.gid:
                        errors.add(archive.name + ': filesystem owner metadata')
                    metadata = repr(info.pax_headers).encode()
                    inspect(archive.name + '/' + str(path), path, source.extractfile(info).read())
                    inspect(archive.name + '/metadata/' + str(path), path, metadata)
                    counts['archive files'] += 1
    if errors:
        sys.exit('\n'.join(sorted(errors)))
    print('Audit passed: ' + ', '.join(str(count) + ' ' + name for name, count in counts.items()) + '.')
    print('No unreviewed credential patterns, sensitive filenames, local home paths or supplied private markers found.')
    if reviewed_exceptions:
        print('Reviewed upstream example files (exact SHA-256 only): ' + str(len(reviewed_exceptions)))
    print('Pattern checks cannot guarantee that every possible secret is absent.')


if __name__ == '__main__':
    main()
