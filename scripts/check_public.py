#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Check public files for credential patterns, unsafe links and missing notices."""
import re
import sys
from pathlib import Path
from urllib.parse import unquote
from public_files import ROOT, public_files


PATTERNS = {
    'private key': rb'-----BEGIN (?:[A-Z]+ )*PRIVATE KEY-----',
    'GitHub credential': rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})',
    'cloud access key': rb'(?:AKIA|ASIA)[0-9A-Z]{16}',
    'Google API key': rb'AIza[0-9A-Za-z_-]{35}',
    'Slack credential': rb'xox[baprs]-[A-Za-z0-9-]{20,}',
    'service credential': rb'(?:sk_live_|sk-proj-)[A-Za-z0-9_-]{20,}',
    'credential in URL': rb'https?://[^/\s:@]+:[^/\s@]+@',
    'credential assignment': rb'(?im)\b(?:api[_-]?key|access[_-]?token|client[_-]?secret|password)\s*[=:]\s*(?:[\x22\x27][A-Za-z0-9_./+=-]{16,}[\x22\x27]|[A-Za-z0-9_./+=-]{16,}\s*$)',
    'personal absolute path': rb'/Users/(?!Shared(?:/|\x00|$))[A-Za-z0-9_. -]+/',
}
FORBIDDEN_NAMES = {
    '.env', '.netrc', '.git-credentials', '.npmrc', '.pypirc',
    'credentials', 'credentials.json', 'authorized_keys', 'known_hosts',
}
FORBIDDEN_DIRS = {'.git', '.ssh', '.aws', '.gnupg'}
FORBIDDEN_SUFFIXES = {'.pem', '.p12', '.p8', '.pfx', '.key', '.keystore', '.jks', '.kdbx', '.mobileprovision'}


def sensitive_findings(relative, data):
    """Return labels only: diagnostics must never print credential contents."""
    path = Path(relative)
    name = path.name.lower()
    labels = []
    if (any(part.lower() in FORBIDDEN_DIRS for part in path.parts)
            or name in FORBIDDEN_NAMES or (name.startswith('.env.') and name != '.env.example')
            or re.match(r'id_(rsa|dsa|ecdsa|ed25519)(\.|$)', name)
            or path.suffix.lower() in FORBIDDEN_SUFFIXES):
        labels.append('credential/store filename')
    labels.extend(label for label, pattern in PATTERNS.items() if re.search(pattern, data))
    return labels


def main():
    files = public_files()
    errors = []
    for relative in files:
        path = ROOT / relative
        data = path.read_bytes()
        errors.extend(str(relative) + ': ' + label for label in sensitive_findings(relative, data))
        if path.suffix == '.md':
            for target in re.findall(r'\]\(([^)]+)\)', data.decode()):
                target = target.split('#', 1)[0]
                if not target or re.match(r'[a-zA-Z]+:', target):
                    continue
                resolved = (path.parent / unquote(target)).resolve()
                if not resolved.is_relative_to(ROOT) or not resolved.exists():
                    errors.append(str(relative) + ': missing/unsafe local link ' + target)
    for required in ('LICENSE', 'CREDITS.md', 'THIRD_PARTY_NOTICES.md', 'licenses/GPL-3.0.txt', 'licenses/JUCE.spdx.json'):
        if not (ROOT / required).is_file():
            errors.append('Required notice missing: ' + required)
    if errors:
        sys.exit('\n'.join(errors))
    print('Public file, link and notice checks passed (' + str(len(files)) + ' files).')
    print('Credential-pattern checks are bounded checks, not a guarantee that all sensitive data is absent.')


if __name__ == '__main__':
    main()
