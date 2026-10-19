# SPDX-License-Identifier: AGPL-3.0-or-later
import hashlib
import io
from pathlib import Path
import sys
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from publish_release import ASSETS, GitHub, NoRedirect, safe_path, unpack_artifact, validate_run


class ReleaseGuards(unittest.TestCase):
    def archive(self, names):
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, 'w') as archive:
            for name in names:
                archive.writestr(name, b'reviewed file')
        data = buffer.getvalue()
        return data, 'sha256:' + hashlib.sha256(data).hexdigest()

    def test_exact_payload(self):
        data, digest = self.archive(sorted(ASSETS))
        self.assertEqual(set(unpack_artifact(data, digest)), ASSETS)

    def test_wrong_digest_is_rejected(self):
        data, digest = self.archive(sorted(ASSETS))
        with self.assertRaises(ValueError):
            unpack_artifact(data, 'sha256:' + '0' * 64)

    def test_missing_extra_and_traversal_files_are_rejected(self):
        for names in [list(ASSETS)[:1], list(ASSETS) + ['extra.txt'], list(ASSETS) + ['../escape']]:
            with self.subTest(names=names):
                data, digest = self.archive(names)
                with self.assertRaises(ValueError):
                    unpack_artifact(data, digest)

    def test_unsafe_source_paths(self):
        for name in ['/absolute', '../escape', 'sid-lab/.git/config', 'sid-lab/../escape', 'folder\\file']:
            with self.subTest(name=name), self.assertRaises(ValueError):
                safe_path(name)

    def test_only_successful_trusted_main_build_is_accepted(self):
        valid = dict(status='completed', conclusion='success', event='push', head_branch='main',
                     head_sha='reviewed', head_repository={'full_name': 'project/instrument'}, path='.github/workflows/build.yml')
        validate_run(valid, 'project/instrument', 'reviewed')
        for key, value in [('conclusion', 'failure'), ('event', 'pull_request'), ('head_branch', 'other'),
                           ('head_sha', 'different'), ('head_repository', {'full_name': 'other/project'}), ('path', 'other.yml')]:
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_run(dict(valid, **{key: value}), 'project/instrument', 'reviewed')

    def test_authenticated_redirects_are_not_forwarded(self):
        import urllib.request
        request = urllib.request.Request('https://api.github.com/example', headers={'Authorization': 'Bearer placeholder'})
        self.assertIsNone(NoRedirect().redirect_request(request, None, 302, '', {}, 'https://storage.example/file'))


if __name__ == '__main__':
    unittest.main()
