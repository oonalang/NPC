#!/usr/bin/env python3
import argparse
import base64
import hashlib
from pathlib import Path
import re
import stat
import subprocess
import tempfile
import zipfile


p = argparse.ArgumentParser()
p.add_argument('--source-panel', type=Path, required=True)
p.add_argument('--package', type=Path, required=True)
p.add_argument('--library', type=Path, required=True)
p.add_argument('--php', required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
assert stat.S_IMODE(a.package.stat().st_mode) == 0o600
with zipfile.ZipFile(a.source_panel) as source, zipfile.ZipFile(a.package) as package:
    assert package.testzip() is None
    names = package.namelist()
    assert len(names) == len(set(names))
    originals = {n for n in source.namelist() if not n.endswith('/')}
    assert originals.issubset(names)
    assert not any(n.startswith('data/') and n != 'data/.htaccess' and not n.endswith('/') for n in names)
    assert not any('fixture' in n or n.startswith(('tests/', '.git/')) for n in names)
    for name in ('admin.php', 'get-key.php'):
        marker = b'?>\n<!doctype html>'
        assert source.read(name).split(marker, 1)[1] == package.read(name).split(marker, 1)[1]
    for name in ('index.php', 'data/.htaccess', 'private/.htaccess'):
        assert source.read(name) == package.read(name)
    for name in ('connect.php', 'lib.php', 'SessionProtocol.php', 'admin.php', 'get-key.php', 'giveaway_lib.php', '.htaccess', 'UPDATE_README.md', 'FREE_PREMIUM_GUIDE.txt', 'GIVEAWAY_SETUP.txt'):
        assert package.read(name) == (root / 'server/license' / name).read_bytes()
    pattern = rb"(['\"]app_modulus['\"]\s*=>\s*)(['\"])[a-fA-F0-9]*\2"
    original_config = source.read('config.php')
    updated_config = package.read('config.php')
    strip_modulus = lambda value: re.sub(pattern, rb'\1\2MODULUS\2', value)
    assert strip_modulus(original_config) == strip_modulus(updated_config), 'Other config values changed'
    for name in ('config.php', 'private/licence_private.pem'):
        assert stat.S_IMODE(package.getinfo(name).external_attr >> 16) == 0o600
    binary = package.read('native/libZANEV1.so')
    assert binary == a.library.read_bytes()
    assert package.read('native/WIZ_LICENSE.md') == (root / 'src/main/jni/Assets/WIZ_LICENSE.md').read_bytes()
    payload, signature = package.read('native/libZANEV1.manifest').splitlines()
    fields = payload.decode().split('|')
    expected = re.search(r'kKeyId\[\] = "([a-f0-9]{64})"', (root / 'src/main/jni/Security/LicenseRoot.h').read_text()).group(1)
    assert fields == ['v1', 'SHAI_ETHNIR', '1', 'libZANEV1.so', str(len(binary)), hashlib.sha256(binary).hexdigest(), expected]
    with tempfile.TemporaryDirectory(prefix='shai-package-check-') as folder:
        temp = Path(folder)
        private = temp / 'private.pem'
        private.write_bytes(package.read('private/licence_private.pem'))
        private.chmod(0o600)
        pem = subprocess.check_output(['openssl', 'pkey', '-in', str(private), '-pubout'])
        der = subprocess.check_output(['openssl', 'pkey', '-in', str(private), '-pubout', '-outform', 'DER'])
        assert hashlib.sha256(der).hexdigest() == expected and pem in binary
        public = temp / 'public.pem'
        public.write_bytes(pem)
        modulus = subprocess.check_output(['openssl', 'rsa', '-pubin', '-in', str(public), '-modulus', '-noout']).strip().split(b'=', 1)[1]
        assert modulus in updated_config and len(modulus) == 768
        (temp / 'signature').write_bytes(base64.b64decode(signature, validate=True))
        subprocess.run(['openssl', 'dgst', '-sha256', '-verify', str(public), '-signature', str(temp / 'signature')], input=payload, stdout=subprocess.DEVNULL, check=True)
        for name in names:
            if name.endswith('.php'):
                subprocess.run([a.php, '-n', '-l'], input=package.read(name), stdout=subprocess.DEVNULL, check=True)
print('Full deployment ZIP: original UI/settings/guards preserved, fresh matching RSA3072 key/modulus, signed exact native library, PHP syntax, private permissions, archive integrity and absence of live databases/vendor mocks verified')
