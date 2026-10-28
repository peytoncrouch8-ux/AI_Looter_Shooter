# Cuts the five chests' Tripo views out of the sheet render (render_sheet.py) and writes Tier<N>/upload, upload_white,
# open_ref.png. Per-tier crop boxes, back paint-over boxes and edge trims are in cfg.json (pixels of the 6144 render).
#   blender -b --factory-startup --python Art/Concepts/Chests/make/build_views.py [-- 1 3]
import sys, os, json, numpy as np
D=os.path.dirname(os.path.abspath(__file__))
TMP=os.path.abspath(os.path.join(D,"..","..","..","..","Saved","ChestViews"))
OUT=os.path.abspath(os.path.join(D,".."))
sys.path.insert(0, D)
from imgio import *
cfg = json.load(open(os.path.join(D,"cfg.json")))
only = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
sheet = load(os.path.join(TMP,"sheet_front.png"))
PAD=20
def crop(box):
    x0,x1,y0,y1 = box
    return sheet[max(0,y0-PAD):y1+PAD, max(0,x0-PAD):x1+PAD].copy()

def boxblur(a, r, axis):
    if r<1: return a
    c = np.cumsum(np.pad(a, [(r+1,r) if i==axis else (0,0) for i in range(a.ndim)], mode='edge'), axis=axis)
    n=a.shape[axis]
    hi = np.take(c, np.arange(2*r+1, 2*r+1+n), axis=axis); lo = np.take(c, np.arange(0, n), axis=axis)
    return (hi-lo)/(2*r+1)
def blur(a, r):
    for _ in range(3):
        a = boxblur(boxblur(a, r, 0), r, 1)
    return a
def dilate_colors(rgb, known, iters):
    rgb = rgb*known[...,None]; k = known.astype(np.float32)
    for _ in range(iters):
        s = blur1(rgb*k[...,None]); w = blur1(k)
        new = (w>1e-4) & (k<0.5)
        rgb[new] = s[new]/w[new][:,None]; k = np.maximum(k, new.astype(np.float32))
        if not new.any(): break
    return rgb
def blur1(a):
    p = np.pad(a, [(1,1),(1,1)]+[(0,0)]*(a.ndim-2))
    return sum(p[dy:dy+a.shape[0], dx:dx+a.shape[1]] for dy in range(3) for dx in range(3))

def clean(img, hole_max):
    a = img[...,3]
    m = a>0.5
    lab,n = label(m)
    if n>1:
        sizes = np.bincount(lab.ravel()); sizes[0]=0
        dropped = [int(s) for i,s in enumerate(sizes) if i and i!=sizes.argmax() and s>0]
        if dropped: print("  dropped pieces (px):", sorted(dropped, reverse=True)[:6])
        m = lab==sizes.argmax()
    # holes
    bl,bn = label(~m)
    h,w = m.shape
    border = set(np.unique(np.concatenate([bl[0],bl[-1],bl[:,0],bl[:,-1]])))
    sizes = np.bincount(bl.ravel())
    for i in range(1,bn+1):
        if i in border: continue
        if sizes[i] <= hole_max: m[bl==i]=True
        else: print("  kept see-through gap of", sizes[i], "px")
    known = (a>0.5) & m
    rgb = dilate_colors(img[...,:3].copy(), known, 60)
    # smooth ragged edge, then pull it in a little so stretched edge texels go
    ms = blur(m.astype(np.float32), 3) > 0.5
    ms = blur(ms.astype(np.float32), 2) > 0.8
    lab,n = label(ms)
    if n>1:
        sizes=np.bincount(lab.ravel()); sizes[0]=0; ms = lab==sizes.argmax()
    alpha = blur(ms.astype(np.float32), 1)
    return np.dstack([rgb, alpha])

def fill(img, x0,x1,y0,y1, wl, wr, mode="reflect", xf=None, ef=14):
    """Paint over [x0,x1)x[y0,y1) with reflected, stretched copies of the wl columns left of it and the wr columns right of it."""
    src = img.copy(); mid=(x0+x1)/2.0
    rows = slice(y0,y1); out = img[rows, x0:x1, :3].copy()
    xs = np.arange(x0,x1)+0.5
    def sample(sx):
        i0 = np.clip(np.floor(sx-0.5).astype(int),0,img.shape[1]-1); f=(sx-0.5)-np.floor(sx-0.5)
        i1 = np.clip(i0+1,0,img.shape[1]-1)
        return src[rows,i0,:3]*(1-f)[None,:,None] + src[rows,i1,:3]*f[None,:,None]
    if mode=="shift": L = sample(xs-(mid-x0)); R = sample(xs+(x1-mid))
    else: L = sample(x0 - (xs-x0)*wl/(mid-x0)); R = sample(x1 + (x1-xs)*wr/(x1-mid))
    xf = xf or 0.6*(x1-x0); t = np.clip((xs-mid)/xf+0.5, 0, 1); t = (t*t*(3-2*t))[None,:,None]
    out = L*(1-t)+R*t
    # feather top/bottom into the original
    ys = np.arange(y0,y1)
    e = np.minimum(np.clip((ys-y0)/ef,0,1), np.clip((y1-1-ys)/ef,0,1))[:,None,None]
    img[rows, x0:x1, :3] = out*e + img[rows, x0:x1, :3]*(1-e)

def resample_matrix(n_in, n_out, scale):
    # area filter: output pixel j covers input [j/scale, (j+1)/scale)
    M = np.zeros((n_out, n_in), np.float32)
    for j in range(n_out):
        a=j/scale; b=(j+1)/scale
        i0=int(np.floor(a)); i1=int(np.ceil(b))
        for i in range(max(i0,0), min(i1,n_in)):
            M[j,i]=max(0.0, min(b,i+1)-max(a,i))
        s=M[j].sum()
        if s>0: M[j]/=s
    return M
def resize(img, scale):
    h,w = img.shape[:2]; oh=max(1,int(round(h*scale))); ow=max(1,int(round(w*scale)))
    my=resample_matrix(h,oh,scale*oh/(h*scale)); mx=resample_matrix(w,ow,scale*ow/(w*scale))
    p = img.copy(); p[...,:3]*=p[...,3:4]
    r = np.einsum('ij,jkc->ikc', my, p); r = np.einsum('ij,kjc->kic', mx, r)
    a = r[...,3:4]; r[...,:3] = np.where(a>1e-5, r[...,:3]/np.maximum(a,1e-5), 0)
    return r
def bbox(img):
    ys,xs=np.nonzero(img[...,3]>0.02); return xs.min(),xs.max()+1,ys.min(),ys.max()+1
def place(img, N=1024):
    x0,x1,y0,y1=bbox(img); img=img[y0:y1,x0:x1]
    c=np.zeros((N,N,4),np.float32); h,w=img.shape[:2]
    oy=(N-h)//2; ox=(N-w)//2
    c[oy:oy+h, ox:ox+w]=img; return c
def white(img):
    o=img.copy(); a=o[...,3:4]; o[...,:3]=o[...,:3]*a+(1-a); o[...,3]=1; return o

for tier, t in cfg.items():
    if only and tier not in only: continue
    print("TIER", tier)
    front = crop(t['front']); side = crop(t['side'])
    for img,key in ((front,'trim_front'),(side,'trim_side')):
        if key in t:
            l,r = t[key]; xs_=np.nonzero((img[...,3]>0.5).any(0))[0]
            img[:, :xs_[0]+l, 3]=0; img[:, xs_[-1]+1-r:, 3]=0
    print(" front"); front = clean(front, t.get('hole_max',6000))
    print(" side"); side = clean(side, t.get('hole_max',6000))
    back = front.copy()
    for op in t['back_fill']: fill(back, *op)
    back = back[:, ::-1].copy()
    fx0,fx1,fy0,fy1 = bbox(front); sx0,sx1,sy0,sy1 = bbox(side)
    k = (fy1-fy0)/(sy1-sy0)
    fw,fh = fx1-fx0, fy1-fy0; sw = (sx1-sx0)*k
    s = 0.80*1024/max(fw,fh,sw)
    print("  front %dx%d side %dx%d k=%.3f s=%.3f" % (fw,fh,sx1-sx0,sy1-sy0,k,s))
    views = {'front':resize(front,s), 'back':resize(back,s), 'right':resize(side,s*k)}
    views['left'] = views['right'][:, ::-1].copy()
    d = os.path.join(OUT,'Tier%s'%tier)
    for sub in ('upload','upload_white'): os.makedirs(os.path.join(d,sub), exist_ok=True)
    for name,v in views.items():
        p = place(v)
        save(p, os.path.join(d,'upload',name+'.png')); save(white(p), os.path.join(d,'upload_white',name+'.png'))
    # open reference
    o = crop(t['open']); oa=o[...,3]
    lab_,n_ = label(oa>0.5); sz=np.bincount(lab_.ravel()); keep=np.isin(lab_, [i for i in range(1,n_+1) if sz[i]>=2000])
    keep = blur(keep.astype(np.float32),2)>0.02
    oa[:] = np.where(keep & (oa>0.1), oa, 0)
    o[...,:3] = dilate_colors(o[...,:3].copy(), oa>0.5, 4)
    so = 0.9*1024/max(o.shape[:2]); save(place(resize(o, so)), os.path.join(d,'open_ref.png'))
    # inspection: back at half size on grey
    g=resize(back,0.5); g[...,:3]=g[...,:3]*g[...,3:4]+0.5*(1-g[...,3:4]); g[...,3]=1; save(g, os.path.join(TMP,"back_t%s.png"%tier))
