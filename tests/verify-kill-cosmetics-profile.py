#!/usr/bin/env python3
import argparse
import importlib.util
import mmap
from pathlib import Path
import re
import struct
import sys
sys.dont_write_bytecode=True

parser=argparse.ArgumentParser(description='Verify the Lite kill-cosmetic profile against its supplied game ELF')
parser.add_argument('library',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('trigger_profile',root/'tests/verify-trigger-profile.py')
trigger=importlib.util.module_from_spec(spec)
spec.loader.exec_module(trigger)
with args.library.open('rb') as stream, mmap.mmap(stream.fileno(),0,access=mmap.ACCESS_READ) as data:
    trigger.verify(data,root/'src/main/jni/Trigger')
    phoff=struct.unpack_from('<Q',data,32)[0]
    size,count=struct.unpack_from('<HH',data,54)
    segments=[struct.unpack_from('<IIQQQQQQ',data,phoff+i*size) for i in range(count)]
    source=(root/'src/main/jni/KillCosmetics/Profile.inc').read_text()
    checks=re.findall(r'\{"([^"]+)",(0x[0-9a-fA-F]+),\{([^}]+)\}\}',source)
    assert len(checks)==22 and len({rva for _,rva,_ in checks})==22
    for name,rva_text,literals in checks:
        rva=int(rva_text,16);expected=trigger.byte_literals(literals)
        assert len(expected)==16
        for kind,flags,offset,address,_,length,_,_ in segments:
            if kind==1 and flags&5==5 and address<=rva and rva+16<=address+length:
                assert data[offset+rva-address:offset+rva-address+16]==expected,name
                break
        else:raise AssertionError(name+' is not mapped executable code')
print('Kill cosmetics: 22 hook/helper/layout fingerprints match; no native offsets inferred from screenshot IDs')
