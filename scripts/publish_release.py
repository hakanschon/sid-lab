#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Publish verified packages from a successful same-repository CI run as a preview."""
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import plistlib
import re
import stat
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile

from check_public import sensitive_findings
from verify_dependencies import verify

ASSETS = {'SIDLab-AU-arm64.zip', 'SIDLab-Standalone-arm64.zip', 'SIDLab-source.tar.gz'}


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        return None


def safe_path(name):
    path = PurePosixPath(name)
    if not path.parts or path.is_absolute() or '..' in path.parts or '\\' in name or '.git' in path.parts:
        raise ValueError('Unsafe archive member path')
    return path


def unpack_artifact(data, digest):
    if digest != 'sha256:' + hashlib.sha256(data).hexdigest():
        raise ValueError('CI artifact digest mismatch')
    payload = {}
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for entry in archive.infolist():
            name = safe_path(entry.filename).as_posix()
            if (name not in ASSETS or name in payload or entry.is_dir()
                    or stat.S_ISLNK(entry.external_attr >> 16) or entry.file_size > 150_000_000):
                raise ValueError('Unexpected CI artifact member')
            payload[name] = archive.read(entry)
    if set(payload) != ASSETS:
        raise ValueError('CI artifact does not contain exactly the three release files')
    return payload


def validate_run(run, repo, commit):
    if (run.get('status') != 'completed' or run.get('conclusion') != 'success'
            or run.get('event') != 'push' or run.get('head_branch') != 'main'
            or run.get('head_sha') != commit
            or run.get('head_repository', {}).get('full_name') != repo
            or run.get('path') != '.github/workflows/build.yml'):
        raise ValueError('Release requires a successful trusted main-branch build of the exact tag commit')


class GitHub:
    def __init__(self, repo, token):
        self.repo, self.token = repo, token
        self.base = 'https://api.github.com/repos/' + repo
        self.opener = urllib.request.build_opener(NoRedirect())

    def request(self, path, method='GET', body=None):
        data = None if body is None else json.dumps(body).encode()
        request = urllib.request.Request(self.base + path, data=data, method=method, headers={
            'Authorization': 'Bearer ' + self.token, 'Accept': 'application/vnd.github+json',
            'X-GitHub-Api-Version': '2022-11-28', 'User-Agent': 'SID-Lab-release',
            'Content-Type': 'application/json'})
        with self.opener.open(request, timeout=60) as response:
            return json.load(response)

    def download_artifact(self, artifact_id):
        request = urllib.request.Request(self.base + '/actions/artifacts/' + str(artifact_id) + '/zip',
            headers={'Authorization': 'Bearer ' + self.token, 'User-Agent': 'SID-Lab-release'})
        try:
            with self.opener.open(request, timeout=60) as response:
                return response.read()
        except urllib.error.HTTPError as error:
            if error.code != 302:
                raise
            location = error.headers['Location']
            if urllib.parse.urlparse(location).scheme != 'https':
                raise ValueError('Unsafe artifact redirect')
            # Signed storage redirects never receive the GitHub Authorization header.
            anonymous = urllib.request.Request(location, headers={'User-Agent': 'SID-Lab-release'})
            with self.opener.open(anonymous, timeout=120) as response:
                return response.read()

    def upload(self, release_id, name, data):
        url = ('https://uploads.github.com/repos/' + self.repo + '/releases/' + str(release_id)
               + '/assets?name=' + urllib.parse.quote(name, safe=''))
        request = urllib.request.Request(url, data=data, method='POST', headers={
            'Authorization': 'Bearer ' + self.token, 'User-Agent': 'SID-Lab-release',
            'Content-Type': 'text/plain' if name.endswith('.txt') else 'application/octet-stream'})
        with self.opener.open(request, timeout=120) as response:
            return json.load(response)


def verify_packages(payload, tree, version, directory):
    source = directory / 'source'
    source.mkdir()
    modes = {}
    with tarfile.open(fileobj=io.BytesIO(payload['SIDLab-source.tar.gz']), mode='r:gz') as archive:
        seen = set()
        total = 0
        for entry in archive:
            relative = safe_path(entry.name)
            if relative.parts[0] != 'sid-lab' or relative.as_posix() in seen:
                raise ValueError('Unsafe/duplicate complete source entry')
            seen.add(relative.as_posix())
            if entry.isdir():
                continue
            total += entry.size
            if not entry.isfile() or total > 500_000_000 or entry.uid or entry.gid or entry.uname or entry.gname:
                raise ValueError('Unsafe source entry or filesystem owner metadata')
            target = source / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.extractfile(entry).read())
            modes[relative.as_posix()] = entry.mode
    source = source / 'sid-lab'
    expected = {'DEPENDENCY_SOURCES.json'}
    manifest = json.loads((source / 'DEPENDENCY_SOURCES.json').read_text())
    for item in tree['tree']:
        if item['type'] == 'blob':
            expected.add(item['path'])
            data = (source / item['path']).read_bytes()
            actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
            if actual != item['sha'] or sensitive_findings(item['path'], data):
                raise ValueError('Complete source does not match the reviewed tag or contains a sensitive pattern')
            if bool(modes['sid-lab/' + item['path']] & 0o111) != (item['mode'] == '100755'):
                raise ValueError('Complete source executable mode differs from the reviewed tag')
        elif item['type'] == 'commit':
            dependency = manifest['dependencies'][PurePosixPath(item['path']).name]
            if dependency['revision'] != item['sha']:
                raise ValueError('Dependency source does not match the pinned Git link')
            expected.update(item['path'] + '/' + safe_path(name).as_posix() for name in dependency['files'])
    if {p.relative_to(source).as_posix() for p in source.rglob('*') if p.is_file()} != expected:
        raise ValueError('Unexpected files in the complete corresponding source')
    verify(source)  # Trusted verifier reads data only; extracted scripts are never executed.
    for name, extension in [('SIDLab-AU-arm64.zip', 'component'), ('SIDLab-Standalone-arm64.zip', 'app')]:
        bundle_name = 'SID Lab.' + extension
        destination = directory / extension
        destination.mkdir()
        with zipfile.ZipFile(io.BytesIO(payload[name])) as archive:
            seen = set()
            total = 0
            for entry in archive.infolist():
                relative = safe_path(entry.filename)
                total += entry.file_size
                if (relative.parts[0] != bundle_name or relative.as_posix() in seen or total > 200_000_000
                        or stat.S_ISLNK(entry.external_attr >> 16)):
                    raise ValueError('Unsafe/duplicate binary archive member')
                seen.add(relative.as_posix())
                if not entry.is_dir() and sensitive_findings(relative, archive.read(entry)):
                    raise ValueError('Sensitive pattern in binary release')
            archive.extractall(destination)
            for entry in archive.infolist():
                if not entry.is_dir():
                    (destination / entry.filename).chmod((entry.external_attr >> 16) & 0o777 or 0o644)
        bundle = destination / bundle_name
        info = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
        if info['CFBundleIdentifier'] != 'org.sidlab.instrument' or info['CFBundleShortVersionString'] != version:
            raise ValueError('Binary version or neutral bundle identity mismatch')
        subprocess.run(['/usr/bin/codesign', '--verify', '--strict', str(bundle)], check=True)
        details = subprocess.run(['/usr/bin/codesign', '-dv', str(bundle)], capture_output=True, text=True, check=True)
        if 'Signature=adhoc' not in details.stderr:
            raise ValueError('Preview signing status differs from the release notes')
        if subprocess.check_output(['/usr/bin/lipo', '-archs', str(bundle / 'Contents/MacOS/SID Lab')]).strip() != b'arm64':
            raise ValueError('Unexpected binary architecture')
        legal = bundle / 'Contents/Resources/Legal'
        for path in legal.rglob('*'):
            if path.is_file() and path.read_bytes() != (source / path.relative_to(legal)).read_bytes():
                raise ValueError('Bundled legal notice differs from corresponding source')
        for required in ['LICENSE', 'LICENSES.md', 'CREDITS.md', 'THIRD_PARTY_NOTICES.md']:
            if not (legal / required).is_file():
                raise ValueError('Required legal resource missing')


def main():
    repo = os.environ['GITHUB_REPOSITORY']
    tag, run_id = os.environ['RELEASE_TAG'], os.environ['BUILD_RUN_ID']
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo) or not re.fullmatch(r'v\d+\.\d+\.\d+', tag) or not run_id.isdigit():
        raise ValueError('Invalid release inputs')
    github = GitHub(repo, os.environ['GH_TOKEN'])
    ref = github.request('/git/ref/tags/' + tag)
    if ref['object']['type'] != 'commit':
        raise ValueError('Release requires an existing lightweight tag pinned to a commit')
    commit = ref['object']['sha']
    run = github.request('/actions/runs/' + run_id)
    validate_run(run, repo, commit)
    artifacts = github.request('/actions/runs/' + run_id + '/artifacts?per_page=100')['artifacts']
    matching = [a for a in artifacts if a['name'] == 'SIDLab-macos-arm64-' + commit and not a['expired']]
    if len(matching) != 1:
        raise ValueError('Expected exactly one matching, unexpired CI artifact')
    artifact = matching[0]
    payload = unpack_artifact(github.download_artifact(artifact['id']), artifact['digest'])
    commit_metadata = github.request('/git/commits/' + commit)
    tree = github.request('/git/trees/' + commit_metadata['tree']['sha'] + '?recursive=1')
    if tree['truncated']:
        raise ValueError('Incomplete source tree response')
    with tempfile.TemporaryDirectory(prefix='sidlab-release-') as temporary:
        verify_packages(payload, tree, tag[1:], Path(temporary))
    checksums = ''.join(hashlib.sha256(data).hexdigest() + '  ' + name + '\n' for name, data in sorted(payload.items()))
    payload['SHA256SUMS.txt'] = checksums.encode()
    notes = Path('docs/releases/' + tag + '.md').read_text()
    if commit not in notes or 'not Developer ID signed or notarized by Apple' not in notes:
        raise ValueError('Release notes must state the exact source commit and preview signing status')
    releases = github.request('/releases?per_page=100')
    candidates = [r for r in releases if r['tag_name'] == tag]
    if len(candidates) > 1:
        raise ValueError('Ambiguous release drafts')
    release = candidates[0] if candidates else github.request('/releases', 'POST', {
        'tag_name': tag, 'target_commitish': commit, 'name': 'SID Lab ' + tag[1:] + ' — Apple Silicon Preview',
        'body': notes, 'draft': True, 'prerelease': True, 'make_latest': 'false'})
    assets = {a['name']: a for a in github.request('/releases/' + str(release['id']) + '/assets?per_page=100')}
    if set(assets) - set(payload):
        raise ValueError('Unexpected existing release asset; no assets were changed')
    if not release['draft']:
        if set(assets) != set(payload):
            raise ValueError('Published release inventory differs; no changes made')
    for name, data in sorted(payload.items()):
        asset = assets.get(name)
        if asset is None:
            asset = github.upload(release['id'], name, data)
        if (asset.get('state') != 'uploaded' or asset.get('size') != len(data)
                or asset.get('digest') != 'sha256:' + hashlib.sha256(data).hexdigest()):
            raise ValueError('Release asset checksum mismatch; release remains unpublished')
    if release['draft']:
        release = github.request('/releases/' + str(release['id']), 'PATCH', {
            'name': 'SID Lab ' + tag[1:] + ' — Apple Silicon Preview', 'body': notes,
            'draft': False, 'prerelease': True, 'make_latest': 'false'})
    print('Verified public preview release: ' + release['html_url'])
    print(checksums)


if __name__ == '__main__':
    try:
        main()
    except urllib.error.HTTPError as error:
        sys.exit('GitHub release request failed with HTTP ' + str(error.code) + '; credentials and redirect URLs are withheld.')
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        sys.exit('Release verification failed: ' + str(error))
