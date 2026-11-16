#!/usr/bin/env python3
import argparse
from pathlib import Path
import re
import struct

p=argparse.ArgumentParser(description='Check release ELF protections and the JNI export surface')
p.add_argument('library',type=Path)
a=p.parse_args()
data=a.library.read_bytes()
if data[:6]!=b'\x7fELF\x02\x01':raise SystemExit('Expected little-endian ELF64')
if struct.unpack_from('<HH',data,16)!=(3,183):raise SystemExit('Expected an ARM64 shared library')
phoff,shoff=struct.unpack_from('<QQ',data,32)
phsize,phcount,shsize,shcount=struct.unpack_from('<HHHH',data,54)
segments=[struct.unpack_from('<IIQQQQQQ',data,phoff+i*phsize) for i in range(phcount)]
stack=[s for s in segments if s[0]==0x6474e551]
assert len(stack)==1 and not stack[0][1]&1,'Executable or unspecified stack'
assert any(s[0]==0x6474e552 and s[6]>0 for s in segments),'Missing GNU RELRO'
dynamic=next(s for s in segments if s[0]==2)
tags=dict(struct.unpack_from('<QQ',data,offset) for offset in range(dynamic[2],dynamic[2]+dynamic[5],16))
assert 24 in tags or tags.get(30,0)&8 or tags.get(0x6ffffffb,0)&1,'Lazy binding remains enabled'
sections=[struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*shsize) for i in range(shcount)]
exports=[];undefined=[]
for section in sections:
    if section[1]!=11:continue
    strings=sections[section[6]]
    for offset in range(section[4],section[4]+section[5],section[9]):
        name,info,other,index,_,_=struct.unpack_from('<IBBHQQ',data,offset)
        start=strings[4]+name;end=data.index(b'\0',start)
        text=data[start:end].decode('ascii')
        if not index:undefined.append(text)
        elif info>>4 in (1,2) and other&3 in (0,3):exports.append(text)
assert 'JNI_OnLoad' in exports,'JNI_OnLoad missing'
assert all(name in ('JNI_OnLoad','JNI_OnUnload') or name.startswith('Java_') for name in exports),exports
assert '__stack_chk_fail' in undefined,'No stack-protector reference'
assert not re.search(rb'-----BEGIN (?:RSA )?PRIVATE KEY-----\s*[A-Za-z0-9+/=]{64,}',data),'Private-key material found'
print('ARM64 ELF: GNU RELRO, immediate binding, non-executable stack, stack protector and JNI-only exports verified')
print('Exports:',', '.join(exports))
