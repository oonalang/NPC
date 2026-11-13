#!/usr/bin/env python3
import argparse
import io
from pathlib import Path
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument('image',type=Path)
parser.add_argument('--output',type=Path,default=Path('src/main/jni/System/UI/StartupArtData.h'))
args=parser.parse_args()
image=Image.open(args.image).convert('RGB').resize((768,256),Image.Resampling.LANCZOS)
encoded=io.BytesIO()
image.save(encoded,format='JPEG',quality=88,optimize=True)
data=encoded.getvalue()
lines=['#pragma once','namespace startup_ui { namespace media {','inline constexpr unsigned char artwork[]{']
lines += ['    '+','.join('0x%02x'%value for value in data[i:i+20])+',' for i in range(0,len(data),20)]
lines += ['};','inline constexpr int artworkWidth=768;','inline constexpr int artworkHeight=256;','}}','']
args.output.write_text('\n'.join(lines))
print(f'Embedded {len(data)} JPEG bytes in {args.output}')
