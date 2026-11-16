#!/usr/bin/env python3
import argparse
import base64
import hashlib
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description='Sign a native release without putting a private key in the app')
p.add_argument('library',type=Path)
p.add_argument('--private-key',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args()
size=a.library.stat().st_size
if not 0<size<=64*1024*1024: p.error('Library must be between 1 byte and 64 MiB')
der=subprocess.check_output(['openssl','pkey','-in',str(a.private_key),'-pubout','-outform','DER'])
kid=hashlib.sha256(der).hexdigest()
digest=hashlib.sha256(a.library.read_bytes()).hexdigest()
payload=f'v1|SHAI_ETHNIR|1|libZANEV1.so|{size}|{digest}|{kid}'.encode('ascii')
signature=subprocess.run(['openssl','dgst','-sha256','-sign',str(a.private_key)],input=payload,stdout=subprocess.PIPE,check=True).stdout
a.out.parent.mkdir(parents=True,exist_ok=True)
a.out.write_bytes(payload+b'\n'+base64.b64encode(signature)+b'\n')
print('Signed native release:',digest)
