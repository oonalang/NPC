import struct, zlib, sys, math, json
from pathlib import Path
import numpy as np
from PIL import Image

def load(path):
    raw = Path(path).read_bytes()
    assert raw[:8] == b'WIZSKN01'
    data = zlib.decompress(raw[12:])
    ver, nv, ni, nb, nf, fps, nmat = struct.unpack_from('<7I', data, 0)
    dur = struct.unpack_from('<f', data, 28)[0]
    off = 32
    vdt = np.dtype([('p','<3f4'),('n','<3f4'),('uv','<2f4'),('j','4u1'),('w','4u2'),('bl','<3f4'),('sm','<3f4'),('sp','<3f4')])
    verts = np.frombuffer(data, dtype=vdt, count=nv, offset=off); off += nv*80
    idx = np.frombuffer(data, dtype='<u4', count=ni, offset=off).copy(); off += ni*4
    mats = []
    for i in range(nmat):
        first, count, png = struct.unpack_from('<3I', data, off); off += 12
        blob = data[off:off+png]; off += png
        mats.append({'first':first,'count':count,'png':blob})
    stride = nb*16+56
    anim = np.frombuffer(data, dtype='<f4', count=stride*nf, offset=off).reshape(nf, stride)
    return dict(verts=verts, idx=idx, mats=mats, anim=anim, nb=nb, nf=nf, fps=fps, stride=stride, dur=dur, nv=nv, ni=ni, nmat=nmat, raw=data, header=raw[:32])

def palette(anim, f, nb):
    flat = anim[f, :nb*16].reshape(nb, 4, 4)
    return np.transpose(flat, (0,2,1))

def skin(verts, pal, nb):
    p = np.concatenate([verts['p'], np.ones((len(verts),1))], 1)
    w = verts['w'].astype(np.float64)/65535.0
    j = verts['j'].astype(np.int64)
    out = np.zeros((len(p),3))
    for k in range(4):
        m = pal[j[:,k]]
        out += w[:,k:k+1] * np.einsum('nij,nj->ni', m, p)[:, :3]
    return out

def norm(v):
    n = np.linalg.norm(v)
    return v/n if n else v

def camera(aspect):
    f = 1.0/math.tan(16.0*math.pi/180.0)
    d = max(4.6, 0.75*f/aspect)
    eye = np.array([0.0,-d,1.3]); target = np.array([0.0,0.04,1.12])
    fwd = (target-eye)/np.linalg.norm(target-eye)
    side = norm(np.cross(fwd, np.array([0.0,0.0,1.0])))
    up = np.cross(side, fwd)
    return {'eye':eye,'fwd':fwd,'side':side,'up':up,'f':f,'aspect':aspect}

def project(cam, points, W, H):
    rel = points - cam['eye']
    x = rel @ cam['side']; y = rel @ cam['up']; z = -(rel @ cam['fwd'])
    w = -z
    ndc_x = (cam['f']/cam['aspect']) * x / w
    ndc_y = cam['f'] * y / w
    ndc_z = ((20.1 + 19.9)/2.0 - 2.0/(w)) if False else (0.5*(20.1+19.9) - (20.0*19.9)/w)/1.0
    sx = (0.5 + 0.5*ndc_x)*W
    sy = (0.5 - 0.5*ndc_y)*H
    return sx, sy, ndc_z, w

def render_frame(asset, f, W=320, H=512):
    verts, idx, anim, nb = asset['verts'], asset['idx'], asset['anim'], asset['nb']
    pal = palette(anim, f, nb)
    pos = skin(verts, pal, nb)
    cam = camera(W/H)
    sx, sy, sz, w = project(cam, pos, W, H)
    color = np.zeros((H,W,4)); depth = np.full((H,W), np.inf)
    images = []
    for m in asset['mats']:
        im = np.asarray(Image.open(__import__('io').BytesIO(m['png'])).convert('RGBA')).astype(np.float32)/255.0
        images.append(im)
    tex = np.zeros((len(color), W, 4))
    order = np.argsort(-sz.mean() if False else 0)
    for mi, m in enumerate(asset['mats']):
        first, count = m['first'], m['count']
        tri = idx[first:first+count].reshape(-1,3)
        im = images[mi]; th, tw = im.shape[:2]
        uvs = verts['uv']
        for a,b,c in tri:
            x0,y0 = sx[a],sy[a]; x1,y1 = sx[b],sy[b]; x2,y2 = sx[c],sy[c]
            z0,z1,z2 = sz[a],sz[b],sz[c]
            if w[a] <= 0 or w[b] <= 0 or w[c] <= 0: continue
            minx = int(max(0, math.floor(min(x0,x1,x2)))); maxx = int(min(W-1, math.ceil(max(x0,x1,x2))))
            miny = int(max(0, math.floor(min(y0,y1,y2)))); maxy = int(min(H-1, math.ceil(max(y0,y1,y2))))
            if minx > maxx or miny > maxy: continue
            area = (x1-x0)*(y2-y0)-(x2-x0)*(y1-y0)
            if abs(area) < 1e-9: continue
            px = np.arange(minx, maxx+1)+0.5; py = np.arange(miny, maxy+1)+0.5
            PX, PY = np.meshgrid(px, py)
            l0 = ((x1-PX)*(y2-PY)-(x2-PX)*(y1-PY))/area
            l1 = ((x2-PX)*(y0-PY)-(x0-PX)*(y2-PY))/area
            l2 = 1.0-l0-l1
            inside = (l0>=0)&(l1>=0)&(l2>=0)
            if not inside.any(): continue
            z = l0*z0+l1*z1+l2*z2
            sub = depth[miny:maxy+1, minx:maxx+1]
            mask = inside & (z < sub)
            if not mask.any(): continue
            u = l0*uvs[a,0]+l1*uvs[b,0]+l2*uvs[c,0]
            v = l0*uvs[a,1]+l1*uvs[b,1]+l2*uvs[c,1]
            ti = np.clip((v*th).astype(int), 0, th-1); tj = np.clip((u*tw).astype(int), 0, tw-1)
            texel = im[ti, tj]
            keep = mask & (texel[...,3] > 0.5)
            if not keep.any(): continue
            sub[keep] = z[keep]
            region = color[miny:maxy+1, minx:maxx+1]
            region[keep] = texel[keep]
    return (np.clip(color,0,1)*255).astype(np.uint8), pos

if __name__ == '__main__':
    asset = load(sys.argv[1] if len(sys.argv)>1 else 'src/main/jni/Assets/wiz-model.bin')
    times = [float(x) for x in sys.argv[2].split(',')] if len(sys.argv)>2 else [0,3.6,4.4,4.8,5.2,8.0,8.3,8.6]
    out = Path(sys.argv[3] if len(sys.argv)>3 else 'artifacts/wiz-frames.png')
    frames = []
    for t in times:
        f = int(round(min(asset['nf']-1, max(0, t*asset['fps']))))
        img, pos = render_frame(asset, f)
        frames.append(img)
        print('frame', f, 'z range', pos[:,2].min().round(3), pos[:,2].max().round(3))
    cols = min(4, len(frames)); rows = (len(frames)+cols-1)//cols
    sheet = Image.new('RGB', (cols*320, rows*512), (8,8,12))
    for i, fr in enumerate(frames):
        sheet.paste(Image.fromarray(fr[:,:,:3]), ((i%cols)*320, (i//cols)*512))
    sheet.save(out); print('wrote', out)
