#!/usr/bin/env python3
import base64
import hashlib
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='shai-release-test-') as folder:
    temp=Path(folder);private=temp/'private.pem';der=temp/'private.der';library=temp/'library.so';manifest=temp/'manifest'
    subprocess.run(['openssl','genpkey','-algorithm','RSA','-pkeyopt','rsa_keygen_bits:3072','-out',str(private)],check=True,stderr=subprocess.DEVNULL)
    subprocess.run(['openssl','pkcs8','-topk8','-nocrypt','-in',str(private),'-outform','DER','-out',str(der)],check=True)
    public=subprocess.check_output(['openssl','pkey','-in',str(private),'-pubout','-outform','DER'])
    fixture=temp/'LicenseRoot.java'
    fixture.write_text('package com.android.support; final class LicenseRoot {static final String PUBLIC_KEY="'+base64.b64encode(public).decode()+'";static final String KEY_ID="'+hashlib.sha256(public).hexdigest()+'";}')
    library.write_bytes(b'ELF-test-fixture'+bytes(range(256))*64)
    subprocess.run(['python3',str(root/'tools/sign-native-release.py'),str(library),'--private-key',str(private),'--out',str(manifest)],check=True,stdout=subprocess.DEVNULL)
    classes=temp/'classes';classes.mkdir()
    sources=[fixture,root/'src/main/java/com/android/support/VerifiedLibrary.java',root/'tests/java/android/util/Base64.java',root/'tests/java/com/android/support/VerifiedLibraryTest.java']
    subprocess.run(['java','--module','jdk.compiler/com.sun.tools.javac.Main','-source','8','-target','8','-d',str(classes),*[str(p) for p in sources]],check=True)
    subprocess.run(['java','-cp',str(classes),'com.android.support.VerifiedLibraryTest',str(manifest),str(library),str(der)],check=True)
