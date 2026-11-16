#!/usr/bin/env python3
import argparse
import base64
import hashlib
from pathlib import Path
import os
import subprocess

p = argparse.ArgumentParser(description='Create a project signing key outside git and update public trust roots')
p.add_argument('--private-out', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
key = a.private_out.resolve()
if key == root or root in key.parents:
    p.error('Keep the private key outside the source checkout')
if key.exists():
    p.error('Private key already exists; refusing to replace it')
key.parent.mkdir(parents=True, exist_ok=True)
os.umask(0o077)
subprocess.run(['openssl', 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:3072', '-out', str(key)], check=True, stderr=subprocess.DEVNULL)
pem = subprocess.check_output(['openssl','pkey','-in',str(key),'-pubout']).decode()
der = subprocess.check_output(['openssl','pkey','-in',str(key),'-pubout','-outform','DER'])
fingerprint = hashlib.sha256(der).hexdigest()
header = '#pragma once\nnamespace PanelLicense {\ninline constexpr char kPublicKey[] =\n'
for line in pem.splitlines():
    header += '    "' + line + '\\n"\n'
header += ';\ninline constexpr char kKeyId[] = "' + fingerprint + '";\n}\n'
(root/'src/main/jni/Security/LicenseRoot.h').write_text(header)
java = 'package com.android.support;\n\nfinal class LicenseRoot {\n    static final String PUBLIC_KEY = "' + base64.b64encode(der).decode() + '";\n    static final String KEY_ID = "' + fingerprint + '";\n    private LicenseRoot() {}\n}\n'
(root/'src/main/java/com/android/support/LicenseRoot.java').write_text(java)
print('Public root updated. Key ID:', fingerprint)
print('Private key saved outside git:', key)
