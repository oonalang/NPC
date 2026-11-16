#!/usr/bin/env python3
import argparse
import base64
import hashlib
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import tempfile
import zipfile


p = argparse.ArgumentParser(description='Update a private full panel archive and include its matching signed native release')
p.add_argument('--source-panel', type=Path, required=True)
p.add_argument('--private-key', type=Path, required=True)
p.add_argument('--library', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
output = a.out.resolve()
if output == root or root in output.parents:
    p.error('Keep the private deployment archive outside the checkout')
if output in (a.source_panel.resolve(), a.private_key.resolve(), a.library.resolve()):
    p.error('Output must not replace an input file')
expected = re.search(r'kKeyId\[\] = "([a-f0-9]{64})"', (root / 'src/main/jni/Security/LicenseRoot.h').read_text()).group(1)
der = subprocess.check_output(['openssl', 'pkey', '-in', str(a.private_key), '-pubout', '-outform', 'DER'])
if hashlib.sha256(der).hexdigest() != expected:
    p.error('Signing key does not match the native client trust root')
pem = subprocess.check_output(['openssl', 'pkey', '-in', str(a.private_key), '-pubout'])
binary = a.library.read_bytes()
if expected.encode() not in binary or pem not in binary:
    p.error('Native library must be rebuilt with the matching public root')
java = (root / 'src/main/java/com/android/support/LicenseRoot.java').read_text()
java_public = re.search(r'PUBLIC_KEY = "([^"]+)"', java).group(1)
if base64.b64decode(java_public, validate=True) != der or expected not in java:
    p.error('Java and native trust roots differ')
subprocess.run(['python3', str(root / 'tools/verify-native-hardening.py'), str(a.library)], check=True)
files = {}
with zipfile.ZipFile(a.source_panel) as source:
    if len(source.infolist()) > 512 or sum(item.file_size for item in source.infolist()) > 32 * 1024 * 1024:
        p.error('Source panel archive exceeds bounded size')
    for item in source.infolist():
        name = item.filename
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or '\\' in name or '\x00' in name or path.as_posix() != name.rstrip('/'):
            p.error('Source panel has an unsafe archive path')
        if stat.S_ISLNK(item.external_attr >> 16):
            p.error('Source panel must not contain symlinks')
        if item.is_dir():
            continue
        if name in files:
            p.error('Source panel has duplicate archive entries')
        if path.parts[0] == 'data' and name != 'data/.htaccess':
            p.error('Keep live license/giveaway data outside the deployment ZIP')
        files[name] = source.read(item)
required = {'config.php', 'admin.php', 'get-key.php', 'index.php', 'lib.php', 'giveaway_lib.php', '.htaccess', 'private/.htaccess', 'data/.htaccess'}
if not required.issubset(files):
    p.error('Expected full panel files at the root of the archive')
for name in ('admin.php', 'get-key.php'):
    marker = b'?>\n<!doctype html>'
    old_parts = files[name].split(marker, 1)
    new_parts = (root / 'server/license' / name).read_bytes().split(marker, 1)
    if len(old_parts) != 2 or len(new_parts) != 2 or old_parts[1] != new_parts[1]:
        p.error('Panel UI differs from the audited upload: ' + name)
if files['index.php'] != (root / 'server/license/index.php').read_bytes():
    p.error('Panel index differs from the audited upload')
config = files['config.php'].decode('utf-8')
if not re.search(r"['\"]build['\"]\s*=>\s*1\s*[,\]]", config):
    p.error('Panel build must match this build-1 client')
with tempfile.TemporaryDirectory(prefix='shai-full-panel-') as folder:
    temp = Path(folder)
    public = temp / 'public.pem'
    public.write_bytes(pem)
    modulus = subprocess.check_output(['openssl', 'rsa', '-pubin', '-in', str(public), '-modulus', '-noout']).decode().strip().split('=', 1)[1]
    pattern = r"(['\"]app_modulus['\"]\s*=>\s*)(['\"])[a-fA-F0-9]*\2"
    config, count = re.subn(pattern, lambda m: m.group(1) + "'" + modulus + "'", config)
    if count != 1:
        p.error('Expected exactly one literal app_modulus setting')
    files['config.php'] = config.encode('utf-8')
    for name in ('connect.php', 'lib.php', 'SessionProtocol.php', 'admin.php', 'get-key.php', 'giveaway_lib.php', 'index.php', '.htaccess', 'UPDATE_README.md', 'FREE_PREMIUM_GUIDE.txt', 'GIVEAWAY_SETUP.txt'):
        files[name] = (root / 'server/license' / name).read_bytes()
    files['private/licence_private.pem'] = a.private_key.read_bytes()
    manifest = temp / 'libZANEV1.manifest'
    subprocess.run(['python3', str(root / 'tools/sign-native-release.py'), str(a.library), '--private-key', str(a.private_key), '--out', str(manifest)], check=True)
    payload, signature = manifest.read_bytes().splitlines()
    (temp / 'signature.bin').write_bytes(base64.b64decode(signature, validate=True))
    subprocess.run(['openssl', 'dgst', '-sha256', '-verify', str(public), '-signature', str(temp / 'signature.bin')], input=payload, stdout=subprocess.DEVNULL, check=True)
    files['native/libZANEV1.so'] = binary
    files['native/libZANEV1.manifest'] = manifest.read_bytes()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('wb') as target:
        output.chmod(0o600)
        with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
            for directory in ('private/', 'data/', 'native/'):
                info = zipfile.ZipInfo(directory)
                info.external_attr = (0o40700 if directory != 'native/' else 0o40755) << 16
                archive.writestr(info, b'')
            for name, data in sorted(files.items()):
                info = zipfile.ZipInfo(name)
                private = name == 'config.php' or name.startswith('private/') or name.startswith('data/')
                info.external_attr = (0o100600 if private else 0o100644) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, data)
print('Private complete panel:', output)
print('Native SHA-256:', hashlib.sha256(binary).hexdigest())
print('Preserved config and UI; no live data JSON or test-only vendor code included.')
