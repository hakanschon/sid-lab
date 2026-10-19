#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Create an independent, single-commit public Git repository; never rewrite history."""
import argparse
from datetime import datetime, timezone
import os
import shutil
import subprocess
from pathlib import Path
from public_files import ROOT, public_files, dependencies
from check_public import main as check_public


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--author-name', required=True)
    parser.add_argument('--author-email', required=True)
    parser.add_argument('--origin', help='Optional future public repository URL; nothing is pushed')
    args = parser.parse_args()
    destination = args.destination.resolve()
    if destination.exists():
        parser.error('Destination already exists; choose a new folder.')
    check_public()
    files = public_files()
    destination.mkdir(parents=True)
    for relative in files:
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / relative, target)
        target.chmod((ROOT / relative).stat().st_mode & 0o777)
    def git(*command):
        subprocess.run(['git', '-C', str(destination), *command], check=True, stdout=subprocess.DEVNULL)
    git('init', '--template=', '--initial-branch=main')
    git('config', 'user.name', args.author_name)
    git('config', 'user.email', args.author_email)
    git('add', '--all')
    for dep in dependencies():
        (destination / 'third_party' / dep['name']).mkdir(parents=True, exist_ok=True)
        git('update-index', '--add', '--cacheinfo', '160000,' + dep['revision'] + ',third_party/' + dep['name'])
    timestamp = datetime.now(timezone.utc).isoformat()
    identity = dict(os.environ, GIT_AUTHOR_NAME=args.author_name,
                    GIT_AUTHOR_EMAIL=args.author_email, GIT_COMMITTER_NAME=args.author_name,
                    GIT_COMMITTER_EMAIL=args.author_email,
                    GIT_AUTHOR_DATE=timestamp, GIT_COMMITTER_DATE=timestamp)
    subprocess.run(['git', '-C', str(destination), '-c', 'commit.gpgsign=false',
                    'commit', '-m', 'Initial public release of SID Lab'],
                   env=identity, check=True, stdout=subprocess.DEVNULL)
    if args.origin:
        git('remote', 'add', 'origin', args.origin)
    print('Public repository prepared at ' + str(destination))
    print('Dependency gitlinks retained. Initialize submodules before building. Nothing was pushed.')


if __name__ == '__main__':
    main()
