#!/usr/bin/env python3
import argparse
import hashlib
import io
from pathlib import Path
import re
from fontTools import subset
from fontTools.ttLib import TTFont

parser = argparse.ArgumentParser(description='Import the supplied Celerity font data with only the glyphs used by Lite')
parser.add_argument('source', type=Path, help='Extracted Celerity source directory')
parser.add_argument('--out', type=Path, default=Path(__file__).resolve().parents[1] / 'src/main/jni/Assets')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
icons = [0xf013, 0xf015, 0xf05b, 0xf07b, 0xf0e7, 0xf1fc, 0xf500]
fonts = [('SFProText-Semibold.hpp', 'SFProText_Semibold', 'lite-body.ttf', range(0x20,0x100)),
         ('sfpro-bold.hpp', 'embedded_font', 'lite-title.otf', range(0x20,0x100)),
         ('fontawesome_binary.hpp', 'font_awesome_binary', 'lite-icons.ttf', icons)]
for source, name, output, glyphs in fonts:
    path = args.source / 'ui/assets' / source
    text = path.read_text(encoding='utf-8-sig')
    match = re.search(r'\b' + re.escape(name) + r'\s*\[\s*\]\s*=\s*\{(.*?)\}', text, re.S)
    if not match:
        parser.error(f'Font byte array not found: {path}:{name}')
    data = bytes(int(number,16) for number in re.findall(r'0x([0-9a-fA-F]{2})\b',match.group(1)))
    font = TTFont(io.BytesIO(data), recalcTimestamp=False)
    missing = set(glyphs) - set(font.getBestCmap())
    if output == 'lite-icons.ttf' and missing:
        parser.error(f'Source icon glyphs missing: {missing}')
    options = subset.Options()
    options.name_IDs = ['*']
    options.name_languages = ['*']
    options.name_legacy = True
    options.recalc_timestamp = False
    operation = subset.Subsetter(options=options)
    operation.populate(unicodes=glyphs)
    operation.subset(font)
    target = args.out / output
    font.save(target, reorderTables=True)
    print(f'{source} SHA256={hashlib.sha256(path.read_bytes()).hexdigest()} -> {output} ({target.stat().st_size} bytes)')
