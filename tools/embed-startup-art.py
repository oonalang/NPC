#!/usr/bin/env python3
import argparse
import bisect
import io
import re
from pathlib import Path
from PIL import Image, ImageOps

parser=argparse.ArgumentParser()
parser.add_argument('portrait',type=Path)
parser.add_argument('animation',type=Path)
parser.add_argument('--output',type=Path,default=Path('src/main/jni/System/UI/StartupArtData.h'))
args=parser.parse_args()
portrait=Image.open(args.portrait).convert('RGB').resize((160,160),Image.Resampling.LANCZOS)
gif=Image.open(args.animation)
frames=[]
ends=[]
elapsed=0
for index in range(gif.n_frames):
    gif.seek(index)
    frames.append(gif.convert('RGB').copy())
    elapsed+=max(20,int(gif.info.get('duration',100)))
    ends.append(elapsed)
count=min(24,len(frames))
width,height,gutter,columns=320,100,2,4
rows=(count+columns-1)//columns
atlas=Image.new('RGB',(columns*(width+gutter*2),rows*(height+gutter*2)))
for index in range(count):
    source=frames[min(len(frames)-1,bisect.bisect_right(ends,index*elapsed/count))]
    frame=ImageOps.fit(source,(width,height),Image.Resampling.LANCZOS,centering=(.5,.80))
    x=(index%columns)*(width+gutter*2)+gutter
    y=(index//columns)*(height+gutter*2)+gutter
    atlas.paste(frame,(x,y))
    atlas.paste(frame.crop((0,0,width,1)).resize((width,gutter)),(x,y-gutter))
    atlas.paste(frame.crop((0,height-1,width,height)).resize((width,gutter)),(x,y+height))
    atlas.paste(frame.crop((0,0,1,height)).resize((gutter,height)),(x-gutter,y))
    atlas.paste(frame.crop((width-1,0,width,height)).resize((gutter,height)),(x+width,y))
    for dx,dy,sx,sy in [(-gutter,-gutter,0,0),(width,-gutter,width-1,0),(-gutter,height,0,height-1),(width,height,width-1,height-1)]:
        atlas.paste(frame.getpixel((sx,sy)),(x+dx,y+dy,x+dx+gutter,y+dy+gutter))

def encode(image,quality):
    result=io.BytesIO()
    image.save(result,format='JPEG',quality=quality,optimize=True)
    return result.getvalue()

portrait_data=encode(portrait,90)
banner_data=encode(atlas,84)
lines=['#pragma once','namespace startup_ui { namespace media {',
       'inline constexpr int artworkWidth=160,artworkHeight=160;',
       f'inline constexpr int bannerWidth={atlas.width},bannerHeight={atlas.height};',
       f'inline constexpr int bannerFrameWidth={width},bannerFrameHeight={height},bannerGutter={gutter};',
       f'inline constexpr int bannerColumns={columns},bannerFrameCount={count},bannerDurationMs={elapsed};',
       '#ifndef CLAY_CPU_PREVIEW']
parts_dir=args.output.parent/'StartupMediaData'
parts_dir.mkdir(parents=True,exist_ok=True)
written=set()
for name,data in [('artwork',portrait_data),('bannerArtwork',banner_data)]:
    lines.append(f'inline constexpr unsigned char {name}[]'+'{')
    for part,start in enumerate(range(0,len(data),32000)):
        filename=f'{name}-{part:02d}.inc'
        chunk=data[start:start+32000]
        content=['    '+','.join('0x%02x'%value for value in chunk[i:i+20])+',' for i in range(0,len(chunk),20)]
        (parts_dir/filename).write_text('\n'.join(content)+'\n')
        written.add(filename)
        lines.append(f'#include "StartupMediaData/{filename}"')
    lines.append('};')
lines.extend(['#endif','}}',''])
args.output.write_text('\n'.join(lines))
for path in parts_dir.iterdir():
    if re.fullmatch(r'(artwork|bannerArtwork)-[0-9]+\.inc',path.name) and path.name not in written:
        path.unlink()
print(f'Portrait {len(portrait_data)} bytes; {count} banner frames, {elapsed}ms loop, atlas {atlas.size}, {len(banner_data)} bytes')
