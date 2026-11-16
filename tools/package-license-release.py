#!/usr/bin/env python3
import argparse
import base64
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

p=argparse.ArgumentParser(description='Prepare separate public and private deployment archives outside git')
p.add_argument('--library',type=Path,required=True)
p.add_argument('--private-key',type=Path,required=True)
p.add_argument('--output-dir',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1];output=a.output_dir.resolve()
if output==root or root in output.parents:p.error('Use an output directory outside the checkout')
expected=re.search(r'kKeyId\[\] = "([a-f0-9]{64})"',(root/'src/main/jni/Security/LicenseRoot.h').read_text()).group(1)
der=subprocess.check_output(['openssl','pkey','-in',str(a.private_key),'-pubout','-outform','DER'])
if hashlib.sha256(der).hexdigest()!=expected:p.error('Signing key does not match the client public root')
pem=subprocess.check_output(['openssl','pkey','-in',str(a.private_key),'-pubout'])
binary=a.library.read_bytes()
if expected.encode() not in binary or pem not in binary:p.error('Rebuild the library after updating the client public root')
java=(root/'src/main/java/com/android/support/LicenseRoot.java').read_text()
javaPublic=re.search(r'PUBLIC_KEY = "([^"]+)"',java).group(1)
if base64.b64decode(javaPublic,validate=True)!=der or expected not in java:p.error('Java and native trust roots differ')
subprocess.run(['python3',str(root/'tools/verify-native-hardening.py'),str(a.library)],check=True)
output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='shai-release-') as folder:
    manifest=Path(folder)/'libZANEV1.manifest'
    subprocess.run(['python3',str(root/'tools/sign-native-release.py'),str(a.library),'--private-key',str(a.private_key),'--out',str(manifest)],check=True)
    with zipfile.ZipFile(output/'shai-public-update.zip','w',zipfile.ZIP_DEFLATED) as archive:
        def public_file(name,data):
            info=zipfile.ZipInfo(name);info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
            archive.writestr(info,data)
        for name in ('connect.php','SessionProtocol.php','lib.php'):
            public_file(name,(root/'server/license'/name).read_bytes())
        directory=zipfile.ZipInfo('native/');directory.external_attr=0o40755<<16;archive.writestr(directory,b'')
        public_file('native/libZANEV1.so',a.library.read_bytes())
        public_file('native/libZANEV1.manifest',manifest.read_bytes())
        public_file('native/WIZ_LICENSE.md',(root/'src/main/jni/Assets/WIZ_LICENSE.md').read_bytes())
    manifestBytes=manifest.read_bytes()
    (output/'libZANEV1.manifest').write_bytes(manifestBytes)
    with zipfile.ZipFile(output/'shai-private-key.zip','w',zipfile.ZIP_DEFLATED) as archive:
        archive.write(a.private_key,'private/licence_private.pem')
        rules='<IfModule mod_authz_core.c>\nRequire all denied\n</IfModule>\n<IfModule !mod_authz_core.c>\nOrder allow,deny\nDeny from all\n</IfModule>\n'
        info=zipfile.ZipInfo('private/.htaccess');info.external_attr=0o100600<<16;archive.writestr(info,rules)
    (output/'shai-private-key.zip').chmod(0o600)
print('Public deployment:',output/'shai-public-update.zip')
print('Private server key:',output/'shai-private-key.zip')
print('Private key is not included in the public deployment or git.')
