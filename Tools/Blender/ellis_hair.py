"""Ellis's hair as hair cards (Art/Models/Characters/Ellis.py's build_hair calls build() here; the user, 2026-10-15:
"hair looks like mud"): layered strips of a script-made strand atlas, as big studios' real-time hair is made.

  - The atlas (make_textures; Art/Textures/EllisHair/T_EllisHair_BC|N|ORM.png, 2048): the top quarter a sheet of
    hair tiling across (the underlayer round the head), below it eight strips 256 px wide, one card each: two dense
    (the main layer), three clumped (the outer), two wisps, one of fly-aways. Strands drawn one by one in numpy (no
    library art): dark brown, darker at the root, warmer and dustier at the tip, gathered into clumps toward tapered
    ends. BC's alpha is the strands' coverage (soft at their edges: the game dithers it); N the strands' round profile
    (DirectX); ORM: R occlusion (the root and the deeper strands), G roughness (each strand its own, so a highlight
    breaks along them), B NOT metal: each strand's random id, which M_CharacterHair tips its tangent by.
  - The cards (build): roots under the hat band (Tools/Blender/ellis_hat.py's band_ring), pressed within its room
    there and fuller below it, grown down over the head and bandana, kept inside the turned-up collar and the cape;
    each card arched to the head's curve, leaning round it a little, the outer ones flicked out at the tips; normals
    the hair volume's (out from the head); UVs straight onto its strip (root at the strip's top). Layers from the scalp
    out: the sheet (one strip of quads round the back from temple to temple), main, outer, wisps, fly-aways, and the
    lock across the left brow. Points the hat's brim would sweep as the game tips it back are moved in under the crown.
  - chain_joints: the back hair's swing chain for Ellis.py's swing_layout.

Run (Blender 4.4, beside the editor through Tools/artrun.ps1):
    artrun.ps1 -Script Tools/Blender/ellis_hair.py -ScriptArgs --textures        the atlas (also with Blender's
        python.exe -I: numpy only), and its views Saved/Concepts/EllisRound6/hair/atlas_*.png
    artrun.ps1 -Script Tools/Blender/ellis_hair.py -ScriptArgs --look,<tag>      Ellis built (no simulation, no bake)
        and his head rendered in Eevee, the hair textured (alpha dithered), the rest flat colours, from 6 sides at 1 m and
        3 m (the game's 1080p scale), into Saved/Concepts/EllisRound6/hair/<tag>_*.png and a contact sheet; with
        --aniso in Cycles (the strands' anisotropic highlight) at 1 m; --look,none builds and logs only
"""
import math
import os
import sys

import numpy as np

try:
    import bpy
    from mathutils import Vector
except ImportError:      # plain Python with numpy still makes the textures
    bpy = Vector = None

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import looter_textures as lt  # noqa: E402

SET = 'EllisHair'
SIZE = 2048
SHEET_ROWS = (4, 508)            # the underlayer: tiles across the whole width; root at the top row
STRIP_ROWS = (528, 2040)         # the cards' strips, root at the top
STRIP_W = 256
PAD = 10                         # px kept clear inside each strip's sides (mips don't bleed into the neighbour)
KINDS = ('dense', 'dense', 'clump', 'clump', 'clump', 'wisp', 'wisp', 'fly')
PX_M = 6000.0                    # about how many texels a metre of card has (the normal map's scale)


def strips_of(kind):
    return [i for i, k in enumerate(KINDS) if k == kind]


def strip_uv(k, s, t):
    """Blender UVs on strip k: s 0..1 across the card, t 0..1 from root to tip."""
    x = k * STRIP_W + PAD + np.asarray(s) * (STRIP_W - 2 * PAD)
    row = STRIP_ROWS[0] + np.asarray(t) * (STRIP_ROWS[1] - STRIP_ROWS[0])
    return x / SIZE, 1.0 - row / SIZE


def sheet_uv(s, t):
    x = np.asarray(s) * SIZE
    row = SHEET_ROWS[0] + np.asarray(t) * (SHEET_ROWS[1] - SHEET_ROWS[0])
    return x / SIZE, 1.0 - row / SIZE


# ------------------------------------------------------------------------------------------------------- the atlas

# sRGB: near black-brown, warmer and dustier toward the tips (the user: dark hair, a little warm in the highlights)
ROOT = np.array([0.080, 0.060, 0.047])
MID = np.array([0.160, 0.118, 0.090])
TIP = np.array([0.185, 0.138, 0.104])
DUST = np.array([0.30, 0.27, 0.24])

# per kind: clumps across a strip and sub-clumps in each (forked, uneven tips), fine strands, body strands (wide, dark,
# behind: the clump's mass), how hard strands gather toward their clump (pull1: the clump narrows) and their sub-clump
# (pull2: the tips part into points), the clumps' lengths (of the strip), strands' widths (px), how wide the strands
# spread over the strip (1 its width), a value multiplier, and how far down a solid fill reaches (sheet only)
KIND = {
    'dense': dict(clumps=(4, 5), subs=(2, 3), strands=430, body=30, pull1=0.45, pull2=0.70, clump_len=(0.80, 1.0),
                  width=(1.3, 2.3), spread=1.0, value=0.85),
    'clump': dict(clumps=(3, 4), subs=(2, 3), strands=300, body=14, pull1=0.55, pull2=0.82, clump_len=(0.60, 1.0),
                  width=(1.2, 2.2), spread=0.92, value=1.0),
    'wisp': dict(clumps=(1, 2), subs=(2, 3), strands=110, body=3, pull1=0.6, pull2=0.85, clump_len=(0.72, 1.0),
                 width=(1.1, 1.9), spread=0.55, value=1.05),
    'fly': dict(clumps=(0, 0), subs=(1, 1), strands=16, body=0, pull1=0.0, pull2=0.0, clump_len=(0.55, 1.0),
                width=(1.3, 1.9), spread=1.0, value=1.15),
    'sheet': dict(clumps=(24, 26), subs=(2, 3), strands=2600, body=150, pull1=0.4, pull2=0.6, clump_len=(0.72, 0.98),
                  width=(1.3, 2.3), spread=1.0, value=0.72, fill=0.70),
}


class _Acc:
    """Premultiplied layers of a region, strands painted back to front."""

    def __init__(self, h, w):
        self.A = np.zeros((h, w), np.float32)
        self.C = np.zeros((h, w, 3), np.float32)
        self.Hg = np.zeros((h, w), np.float32)
        self.R = np.zeros((h, w), np.float32)
        self.I = np.zeros((h, w), np.float32)
        self.Z = np.zeros((h, w), np.float32)


def _paint(acc, x, w, a, col, rough, sid, z, periodic):
    """One strand over the region: per row its centre x, width w and opacity a (rows from the region's top), colour
    (rows, 3), roughness (rows,), id and depth z. Box-filtered coverage across, a round profile for the height."""
    H, W = acc.A.shape
    R = len(x)
    if R == 0:
        return
    half = np.maximum(w * 0.5, 0.05)
    span = int(np.ceil(float(w.max()))) + 3
    lo = np.floor(x - half - 1.0).astype(np.int64)
    cols = lo[:, None] + np.arange(span)[None, :]
    left = np.maximum(cols, (x - half)[:, None])
    right = np.minimum(cols + 1.0, (x + half)[:, None])
    cov = np.clip(right - left, 0.0, 1.0)
    d = (cols + 0.5 - x[:, None]) / np.maximum(half[:, None], 0.6)
    prof = np.sqrt(np.clip(1.0 - d * d, 0.0, 1.0)) * half[:, None]
    al = (cov * a[:, None]).astype(np.float32)
    rows = np.broadcast_to(np.arange(R)[:, None], cols.shape)
    if periodic:
        cc = cols % W
        keep = al > 1e-4
    else:
        keep = (al > 1e-4) & (cols >= 0) & (cols < W)
        cc = np.clip(cols, 0, W - 1)
    r_, c_, al = rows[keep], cc[keep], al[keep]
    inv = 1.0 - al
    acc.A[r_, c_] = al + acc.A[r_, c_] * inv
    acc.C[r_, c_] = col[r_] * al[:, None] + acc.C[r_, c_] * inv[:, None]
    acc.Hg[r_, c_] = (z * 3.0 + prof[keep]) * al + acc.Hg[r_, c_] * inv
    acc.R[r_, c_] = rough[r_] * al + acc.R[r_, c_] * inv
    acc.I[r_, c_] = sid * al + acc.I[r_, c_] * inv
    acc.Z[r_, c_] = z * al + acc.Z[r_, c_] * inv


def _region(h, w, kind, seed, periodic=False):
    """Paints a region's strands; returns (colour sRGB, alpha, height px, roughness, id, depth)."""
    cfg = KIND[kind]
    rng = np.random.default_rng(seed)
    acc = _Acc(h, w)
    lo_x, hi_x = (0.0, float(w)) if periodic else (PAD, w - PAD)
    span_x = hi_x - lo_x
    t_all = np.arange(h, dtype=np.float64) / (h - 1)
    # the clumps: a centre path each, a length, a value and roughness offset
    n_cl = int(rng.integers(cfg['clumps'][0], cfg['clumps'][1] + 1)) if cfg['clumps'][1] else 0
    clumps = []
    spacing = span_x / max(n_cl, 1)
    for c in range(n_cl):
        cx0 = lo_x + (c + 0.5 + rng.uniform(-0.25, 0.25)) / n_cl * span_x
        if not periodic:
            # clumps toward the card's middle: its sides stay soft
            cx0 = lo_x + span_x * (0.5 + (cx0 - lo_x - 0.5 * span_x) / span_x * cfg['spread'] * 0.85)
        drift = rng.normal() * spacing * 0.25
        wave = (rng.uniform(2.0, 7.0), rng.uniform(0.4, 1.2), rng.uniform(0, 6.28))
        L = rng.uniform(*cfg['clump_len'])
        n_sub = int(rng.integers(cfg['subs'][0], cfg['subs'][1] + 1))
        subs = []
        for k in range(n_sub):
            off = ((k + 0.5) / n_sub - 0.5) * spacing * 0.75 + rng.normal() * spacing * 0.06
            subs.append(dict(off=off, drift=rng.normal() * spacing * 0.12, L=min(1.0, L * rng.uniform(0.78, 1.0))))
        clumps.append(dict(x0=cx0, drift=drift, wave=wave, L=L, v=math.exp(rng.normal() * 0.14),
                           r=rng.normal() * 0.04, subs=subs))

    def clump_x(c, t):
        a_, f_, p_ = c['wave']
        return c['x0'] + c['drift'] * t + a_ * np.sin(2 * math.pi * (f_ * t) + p_)

    def offset(x0, c):
        return ((x0 - c['x0'] + w / 2) % w) - w / 2 if periodic else x0 - c['x0']

    strands = []
    # body strands first (deepest): wide and dark, across each clump's width, so the card's middle reads solid
    n_body = cfg['body']
    n_fine = cfg['strands']
    for i in range(n_body + n_fine):
        body = i < n_body
        if n_cl:
            if body:
                c = clumps[i % n_cl]
                x0 = c['x0'] + rng.uniform(-0.36, 0.36) * spacing
            else:
                x0 = lo_x + rng.uniform() * span_x
                if not periodic:
                    # fewer strands toward the strip's sides
                    x0 = lo_x + span_x * (0.5 + (rng.uniform() + rng.uniform() * 0.3 - 0.65) / 1.3 * cfg['spread'])
                # the nearest clump mostly; now and then a neighbour (strands crossing between clumps)
                order = np.argsort([abs(offset(x0, cc)) for cc in clumps])
                c = clumps[order[0] if rng.random() < 0.88 or len(order) < 2 else order[1]]
            rel = offset(x0, c)
            sub = min(c['subs'], key=lambda q: abs(q['off'] - rel))
            L = min(0.995, sub['L'] * (rng.uniform(0.62, 0.85) if body else rng.uniform(0.80, 1.04)))
        else:
            # fly-aways: one strand each, curling out
            c = sub = None
            x0 = lo_x + rng.uniform(0.1, 0.9) * span_x
            L = rng.uniform(*cfg['clump_len'])
        rows = int(L * (h - 1)) + 1
        t = t_all[:rows]
        wa, wf, wp = rng.uniform(0.2, 1.4), rng.uniform(0.8, 3.0), rng.uniform(0, 6.28)
        wav = wa * np.sin(2 * math.pi * wf * t + wp)
        if c is not None:
            k = 0.8 if body else rng.uniform(0.85, 1.08)
            P1 = cfg['pull1'] * k * lt.smooth(0.0, 1.0, t / c['L'])
            P2 = cfg['pull2'] * k * lt.smooth(0.25, 1.0, t / sub['L']) ** 1.4
            centre = clump_x(c, t)
            own = centre + rel * (1.0 - P1) + wav
            subx = centre + sub['off'] * (1.0 - P1) + sub['drift'] * t
            x = own * (1.0 - P2) + (subx + (rel - sub['off']) * 0.08) * P2
        else:
            bend = rng.normal() * span_x * 0.10
            x = np.clip(x0 + bend * t ** 2.0 + wav * 1.5, lo_x, hi_x)
        if body:
            wd = rng.uniform(6.0, 13.0) * np.ones(rows)
        else:
            wd = rng.uniform(*cfg['width']) * np.ones(rows)
        tipf = lt.smooth(L - 0.16, L, t) if L > 0.2 else t / max(L, 1e-3)
        wd = wd * (1.0 - 0.65 * tipf)
        a = (1.0 - lt.smooth(L - 0.035, L, t)) * (1.0 if body else rng.uniform(0.86, 1.0))
        # softer at the strip's sides, and a strand fades out where it wanders to the strip's edge (clamped there, the
        # strands drew the card's straight side as a line across the hair)
        if not periodic:
            edge = np.minimum(x - lo_x, hi_x - x)
            a = a * (0.45 + 0.55 * np.clip(edge / (0.12 * span_x), 0.0, 1.0)) * lt.smooth(0.0, 8.0, edge)
        # colour root to tip
        v = math.exp(rng.normal() * 0.16) * (c['v'] if c is not None else 1.0) * cfg['value']
        u = rng.random()
        tint = np.ones(3)
        if u < 0.04:
            tint = np.array([1.25, 1.08, 0.9]) * 1.35     # a sun-bleached strand
        elif u < 0.11:
            tint = np.full(3, 0.6)                        # near black
        col = ROOT[None] + (MID - ROOT)[None] * lt.smooth(0.0, 0.35, t)[:, None]
        tt = lt.smooth(0.5, 1.0, t / max(L, 1e-3))[:, None]
        col = col + (TIP - col) * tt
        col = col + (DUST - col) * (0.10 * tt)
        col = col * v * tint
        if body:
            col = col * 0.62
        rough = 0.41 + rng.normal() * 0.05 + (c['r'] if c is not None else 0.0)
        rough = rough + 0.07 * lt.smooth(0.6, 1.0, t / max(L, 1e-3)) + 0.05 * (1.0 - lt.smooth(0.0, 0.2, t))
        if body:
            rough = rough + 0.08
        z = rng.uniform(0.0, 0.3) if body else rng.uniform(0.3, 1.0)
        if c is not None and not body and rng.random() < 0.06:
            z = 1.0                                       # a stray on top
        strands.append((z, x, wd, a, np.clip(col, 0, 1), np.clip(rough, 0.2, 0.8), rng.random() if not body else 0.5))
    if 'fill' in cfg:
        # the underlayer: solid down to a wavy edge, then the strands' own ends
        edge = cfg['fill'] + 0.08 * lt.noise((8, w), seed + 99, 40.0, 0.01)[4]
        fill_a = 1.0 - lt.smooth(edge[None, :] - 0.08, edge[None, :], t_all[:, None])
        fc = (ROOT + (MID - ROOT) * lt.smooth(0.0, 0.5, t_all)[:, None]) * cfg['value'] * 0.6
        acc.A[:] = fill_a
        acc.C[:] = fc[:, None, :] * fill_a[..., None]
        acc.R[:] = 0.5 * fill_a
        acc.I[:] = 0.5 * fill_a
    strands.sort(key=lambda s: s[0])
    for z, x, wd, a, col, rough, sid in strands:
        _paint(acc, x, wd, a, col, rough, sid, z, periodic)
    A = acc.A
    safe = np.maximum(A, 1e-4)
    colour = acc.C / safe[..., None]
    rough = np.where(A > 1e-3, acc.R / safe, 0.5)
    sid = np.where(A > 1e-3, acc.I / safe, 0.5)
    height = np.where(A > 1e-3, acc.Hg / safe, 0.0)
    depth = np.where(A > 1e-3, acc.Z / safe, 0.0)
    return colour, A, height, rough, sid, depth, t_all


def make_textures(look=True):
    """Writes Art/Textures/EllisHair/T_EllisHair_BC (RGBA), _N and _ORM (B: the strands' ids)."""
    colour = np.zeros((SIZE, SIZE, 3), np.float32)
    alpha = np.zeros((SIZE, SIZE), np.float32)
    height = np.zeros((SIZE, SIZE), np.float32)
    rough = np.full((SIZE, SIZE), 0.5, np.float32)
    sid = np.full((SIZE, SIZE), 0.5, np.float32)
    ao = np.ones((SIZE, SIZE), np.float32)
    regions = [('sheet', SHEET_ROWS, 0, SIZE, 11, True, 0.72)]
    for k, kind in enumerate(KINDS):
        regions.append((kind, STRIP_ROWS, k * STRIP_W, STRIP_W, 101 + 17 * k, False, 0.92 if kind == 'dense' else 1.0))
    for kind, (r0, r1), x0, w, seed, periodic, shade in regions:
        h = r1 - r0
        c, a, hg, r, i, dpt, t = _region(h, w, kind, seed, periodic)
        colour[r0:r1, x0:x0 + w] = c
        alpha[r0:r1, x0:x0 + w] = a
        height[r0:r1, x0:x0 + w] = hg
        rough[r0:r1, x0:x0 + w] = r
        sid[r0:r1, x0:x0 + w] = i
        # occlusion: the root under the band, and the deeper strands
        root = 0.55 + 0.45 * lt.smooth(0.0, 0.4, t)[:, None]
        ao[r0:r1, x0:x0 + w] = np.clip(root * (0.72 + 0.28 * dpt) * shade, 0.0, 1.0)
        print(f'HAIR texture: {kind} at ({x0}, {r0}): coverage {a.mean():.2f}', flush=True)
    normal = lt.normal_map(height / PX_M, 1.0 / PX_M * 1.0, strength=0.7)
    flat = alpha < 0.02
    normal[flat] = (128, 128, 255)
    lt.save_set(SET, colour, height / PX_M, 1.0 / PX_M, rough, metal=sid, ao=ao, alpha=alpha, normal=normal, look=look)
    if look:
        atlas_views(colour, alpha, normal, rough, ao)
    return {k: lt.texture_path(SET, k) for k in ('BC', 'N', 'ORM')}


def atlas_views(colour, alpha, normal, rough, ao):
    """Saved/Concepts/EllisRound6/hair/atlas_*.png: the whole atlas over a pale ground, and crops at full size of the
    sheet, a dense strip and a clumped one, flat and lit (a light from the upper left, a highlight band)."""
    out = os.path.join(REPO, 'Saved', 'Concepts', 'EllisRound6', 'hair')
    ground = np.array([0.78, 0.76, 0.72])
    over = colour * alpha[..., None] + ground * (1.0 - alpha[..., None])
    lt.write_png(os.path.join(out, 'atlas_bc.png'), lt.to8(over[::2, ::2]))
    n = normal.astype(np.float32) / 127.5 - 1.0
    lin = colour ** 2.2
    diff = np.clip(n[..., 0] * -0.4 + -n[..., 1] * 0.5 + n[..., 2] * 0.77, 0.0, 1.0)
    spec = np.clip(n[..., 0] * 0.0 + -n[..., 1] * 0.2 + n[..., 2] * 0.98, 0.0, 1.0) ** (2.0 / np.maximum(rough, 0.05) ** 2)
    lit = (lin * (0.35 + 1.4 * diff[..., None]) * ao[..., None] + 0.04 * spec[..., None] * ao[..., None]) ** (1 / 2.2)
    lit = lit * alpha[..., None] + ground * (1.0 - alpha[..., None])
    crops = [over[0:512, 0:768], lit[0:512, 0:768], over[528:1300, 0:512], lit[528:1300, 0:512],
             over[1300:2048, 512:1024], lit[1300:2048, 1280:1792]]
    for i, cpx in enumerate(crops):
        lt.write_png(os.path.join(out, f'atlas_crop{i}.png'), lt.to8(cpx))


def have_textures():
    return all(os.path.exists(lt.texture_path(SET, k)) for k in ('BC', 'N', 'ORM'))


# ---------------------------------------------------------------------------------------------- Blender material

def material(name='Ellis_Hair'):
    """The atlas on a Principled BSDF for previews (Eevee: alpha dithered, as the game dithers it; Cycles: an
    anisotropic highlight across the strands). B of the ORM is the strands' id, not metal."""
    if not have_textures():
        make_textures(look=False)
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    b = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(b.outputs[0], out.inputs['Surface'])
    uv = nt.nodes.new('ShaderNodeUVMap')
    uv.uv_map = 'UVMap'

    def img(kind, cs):
        n = nt.nodes.new('ShaderNodeTexImage')
        n.image = bpy.data.images.load(lt.texture_path(SET, kind), check_existing=True)
        n.image.colorspace_settings.name = cs
        if kind == 'BC':
            n.image.alpha_mode = 'STRAIGHT'
        nt.links.new(uv.outputs['UV'], n.inputs['Vector'])
        return n
    bc, nm, orm = img('BC', 'sRGB'), img('N', 'Non-Color'), img('ORM', 'Non-Color')
    sep = nt.nodes.new('ShaderNodeSeparateColor')
    nt.links.new(orm.outputs['Color'], sep.inputs[0])
    mul = nt.nodes.new('ShaderNodeMix')
    mul.data_type = 'RGBA'
    mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    nt.links.new(bc.outputs['Color'], mul.inputs['A'])
    nt.links.new(sep.outputs['Red'], mul.inputs['B'])
    nt.links.new(mul.outputs['Result'], b.inputs['Base Color'])
    nt.links.new(bc.outputs['Alpha'], b.inputs['Alpha'])
    nt.links.new(sep.outputs['Green'], b.inputs['Roughness'])
    # DirectX normal (green down) to Blender's OpenGL
    sn = nt.nodes.new('ShaderNodeSeparateColor')
    nt.links.new(nm.outputs['Color'], sn.inputs[0])
    fl = nt.nodes.new('ShaderNodeMath')
    fl.operation = 'SUBTRACT'
    fl.inputs[0].default_value = 1.0
    nt.links.new(sn.outputs['Green'], fl.inputs[1])
    cn = nt.nodes.new('ShaderNodeCombineColor')
    nt.links.new(sn.outputs['Red'], cn.inputs['Red'])
    nt.links.new(fl.outputs['Value'], cn.inputs['Green'])
    nt.links.new(sn.outputs['Blue'], cn.inputs['Blue'])
    nmap = nt.nodes.new('ShaderNodeNormalMap')
    nmap.uv_map = 'UVMap'
    nt.links.new(cn.outputs['Color'], nmap.inputs['Color'])
    nt.links.new(nmap.outputs['Normal'], b.inputs['Normal'])
    b.inputs['Metallic'].default_value = 0.0
    for key, value in (('Specular IOR Level', 0.55), ('Anisotropic', 0.75), ('Coat Weight', 0.0)):
        if key in b.inputs:
            b.inputs[key].default_value = value
    tan = nt.nodes.new('ShaderNodeTangent')
    tan.direction_type = 'UV_MAP'
    tan.uv_map = 'UVMap'
    if 'Tangent' in b.inputs:
        nt.links.new(tan.outputs['Tangent'], b.inputs['Tangent'])
    mat.use_backface_culling = False
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    if hasattr(mat, 'blend_method'):
        try:
            mat.blend_method = 'HASHED'
        except Exception:
            pass
    return mat


# ------------------------------------------------------------------------------------------------------- the cards

# The cards, layer by layer from the scalp out. span: degrees round from the face (0) past the back (180), the
# forehead left bare. A layer: (name, cards, width m, lift at the root m (under the band), lift below it m, length
# range (of the drop from the root to the tips' height), strip kind, rows, quads across, twist degrees, wander, the
# tips' flick out range, sweep: how far a card leans round the head either way). Shorter outer layers (a layered, shaggy
# cut: their clumps' ends show over the longer ones); leaning cards cross into clumps (all plumb read as a curtain).
# The sheet: columns, rows, lift root/below, its length's share.
CARDS = dict(
    seed=7,
    span=(46.0, 314.0),
    under=0.012,                       # roots this far above the band's lower edge, under it
    sheet=dict(cols=41, rows=7, lift=(0.0015, 0.0035), length=0.95),
    layers=(
        ('main', 30, 0.040, 0.0025, 0.0060, (0.92, 1.04), 'dense', 8, 2, 8.0, 0.05, (0.0, 0.25), 0.14),
        ('outer', 36, 0.029, 0.0035, 0.0090, (0.55, 0.95), 'clump', 8, 2, 14.0, 0.08, (0.2, 0.7), 0.26),
        ('wisp', 24, 0.016, 0.0040, 0.0115, (0.70, 1.05), 'wisp', 6, 1, 20.0, 0.12, (0.2, 0.9), 0.35),
        ('fly', 12, 0.011, 0.0040, 0.0120, (0.55, 0.92), 'fly', 6, 1, 25.0, 0.10, (0.15, 0.5), 0.25),
    ),
    # the tips under the eyes: by the temples (the jaw), and how much further down round the back (into the
    # turned-up collar: collar length); at least below the collar's rim where it stands round the neck
    # floor: nothing lower than this under the eyes (154.5 cm: the collar's inner lining has its floor at 153.7)
    tips=dict(temple=0.062, back=0.068, under_rim=0.022, floor=0.135),
    brow=dict(cards=3, width=0.019, rows=6, degs=(6.0, 22.0)),
)


def round_angle(p, centre):
    """Angles round a vertical axis at centre (0 the face, + toward the figure's left), as Ellis.py's."""
    rel = np.atleast_2d(p)[:, :2] - np.asarray(centre)[:2]
    return np.arctan2(rel[:, 0], -rel[:, 1])


def direction(a):
    return np.array([math.sin(a), -math.cos(a), 0.0])


def band_ring(fig, head_c, skin_tree, log):
    """The hat band's lower edge round the head: f(angle) -> (its height, its room off the scalp, m). From
    Tools/Blender/ellis_hat.py's band_ring when the HAT build has it, else measured off the band in the scene."""
    pts = None
    try:
        import ellis_hat
        got = ellis_hat.band_ring(fig)
        got = got.get('points', got.get('ring')) if isinstance(got, dict) else got
        pts = np.asarray(got, dtype=np.float64).reshape(-1, 3)
        log(f'hair: the band line from ellis_hat.band_ring ({len(pts)} points)')
    except Exception:
        bands = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('HatBand')
                 and not o.name.endswith('_low')]
        if bands:
            co = np.concatenate([np.array([v.co[:] for v in o.data.vertices]) @ np.array(o.matrix_world)[:3, :3].T
                                 + np.array(o.matrix_world)[:3, 3] for o in bands])
            ang = round_angle(co, head_c)
            bins = np.round((ang + math.pi) / (2 * math.pi) * 72).astype(int) % 72
            low = []
            for b in range(72):
                sel = co[bins == b]
                if len(sel):
                    k = np.argmin(sel[:, 2])
                    low.append(sel[k])
            pts = np.array(low)
    if pts is None or len(pts) < 8:
        z = float(fig.head_point(np.array([0.0, fig.hf['eye_y'] + 0.47, 0.0]))[2])
        log('hair: no hat band found, the roots at the portrait\'s band height')
        return lambda a: (z, 0.006)
    ang = round_angle(pts, head_c)
    order = np.argsort(ang)
    ang, pts = ang[order], pts[order]
    room = []
    for p in pts:
        hit = skin_tree.find_nearest(Vector(tuple(p)), 0.05)
        room.append((Vector(tuple(p)) - hit[0]).length if hit[0] is not None else 0.006)
    room = np.array(room)
    A = np.concatenate([ang - 2 * math.pi, ang, ang + 2 * math.pi])
    Z = np.tile(pts[:, 2], 3)
    Rm = np.tile(room, 3)
    log(f'hair: band edge {pts[:, 2].min():.3f}..{pts[:, 2].max():.3f} m, its room off the scalp '
        f'{room.min() * 1000:.1f}..{room.max() * 1000:.1f} mm')
    return lambda a: (float(np.interp(a, A, Z)), float(np.interp(a, A, Rm)))


def grow(tree, root, length, rows, clear, out_bias, rng, wob, down=(0.0, 0.0, -1.0), first=None, flare=0.0,
         axis=None, keep_in=None):
    """A card's centre line from root: falls along down (the first step along first if given), slides over the
    surfaces in tree, kept clear(t) off them (pressed thin under the band, fuller below), wandering a little; toward
    its end it flicks out from the head's axis (axis: a point on it) by flare; keep_in(point) keeps it inside the
    collar."""
    pts = [np.array(root, dtype=np.float64)]
    p = Vector(tuple(root))
    step = length / (rows - 1)
    w = Vector(tuple(rng.normal(size=3))) * wob
    for k in range(rows - 1):
        t = (k + 1) / (rows - 1)
        d = Vector(down) + w * t
        if first is not None and k == 0:
            d = Vector(tuple(first))
        if flare and axis is not None:
            r = Vector((p.x - axis[0], p.y - axis[1], 0.0))
            if r.length > 1e-6:
                d = d + r.normalized() * (flare * t ** 1.6)
        d.normalize()
        hit = tree.find_nearest(p, 0.2)
        if hit[0] is not None and (p - hit[0]).length < clear(t) * 3.0:
            n = hit[1]
            if d.dot(n) < 0.0:
                d = d - n * d.dot(n)
            d = (d + n * out_bias * 0.25).normalized()
        p = p + d * step
        hit = tree.find_nearest(p, 0.2)
        if hit[0] is not None and (p - hit[0]).dot(hit[1]) < clear(t):
            p = hit[0] + hit[1] * clear(t)
        if keep_in is not None:
            p = Vector(tuple(keep_in(np.array(p[:]))))
        pts.append(np.array(p[:]))
    return np.array(pts)


def frames(st, head_c, twist):
    """Along a card's centre line: tangent, across (tangent to the head, carried along so it never flips) and out."""
    tang = np.gradient(st, axis=0)
    tang /= np.maximum(np.linalg.norm(tang, axis=1, keepdims=True), 1e-9)
    out0 = st - head_c
    out0 /= np.maximum(np.linalg.norm(out0, axis=1, keepdims=True), 1e-9)
    side = np.cross(tang, out0)
    for i in range(len(st)):
        if np.linalg.norm(side[i]) < 0.3 and i > 0:
            side[i] = side[i - 1]
        side[i] /= max(np.linalg.norm(side[i]), 1e-9)
        if i > 0 and side[i] @ side[i - 1] < 0.0:
            side[i] = -side[i]
    out = np.cross(side, tang)
    out = np.where(((out * out0).sum(1) < 0.0)[:, None], -out, out)
    t = np.linspace(0.0, 1.0, len(st))[:, None]
    th = np.radians(twist) * t
    side2 = side * np.cos(th) + out * np.sin(th)
    out2 = out * np.cos(th) - side * np.sin(th)
    return tang, side2, out2


class _Cards:
    def __init__(self):
        self.verts, self.faces, self.uv, self.normals = [], [], [], []

    def add_grid(self, P, U, V, N):
        """A (rows, cols) grid of points P with per-point UVs (U, V) and normals N, faced along N."""
        rows, cols = P.shape[:2]
        base = len(self.verts)
        self.verts += [tuple(p) for p in P.reshape(-1, 3)]
        self.normals += [tuple(n) for n in N.reshape(-1, 3)]
        probe = np.cross(P[0, 1] - P[0, 0], P[1, 0] - P[0, 0]) if cols > 1 else np.zeros(3)
        flip = probe @ N[0, 0] > 0.0
        for i in range(rows - 1):
            for j in range(cols - 1):
                a, b = base + i * cols + j, base + i * cols + j + 1
                c, d = base + (i + 1) * cols + j + 1, base + (i + 1) * cols + j
                quad = [a, b, c, d] if flip else [a, d, c, b]
                self.faces.append(quad)
                for q in quad:
                    r, cc = divmod(q - base, cols)
                    self.uv.append((float(U[r, cc]), float(V[r, cc])))


def card_grid(st, side, out, width, cols, cup_r):
    """Points across a card: cols quads, the edges curled toward the head as its surface curves (radius cup_r)."""
    rows = len(st)
    s = np.linspace(-0.5, 0.5, cols + 1)
    P = np.zeros((rows, cols + 1, 3))
    for j, sj in enumerate(s):
        off = sj * width
        curl = (off ** 2) / (2.0 * np.maximum(cup_r, 0.03))
        P[:, j] = st + side * off[:, None] - out * curl[:, None]
    return P, (s + 0.5)


def build(fig, under_obs, name, mat, log, **params):
    """The hair cards as one object (UVs on the EllisHair atlas, custom normals out of the hair's volume)."""
    import looter_garments as gm
    P = dict(CARDS, **params)
    rng = np.random.default_rng(P['seed'])
    # grown over the head and the bandana; the turned-up collar and the cape are walls the hair stays inside (as
    # colliders, one-sided sheets, they pushed the cards out through the collar)
    walls = [o for o in under_obs if o.name.startswith(('Collar', 'Cape'))]
    tree = gm.bvh_objects([o for o in under_obs if o not in walls])
    wall_tree = gm.bvh_objects(walls) if walls else None
    neck = np.asarray(fig.neck, dtype=np.float64)

    def keep_in(q, margin=0.005):
        """q pulled in toward the neck's axis to margin inside the collar (or the cape) where they stand round it."""
        if wall_tree is None:
            return q
        d = np.array([q[0] - neck[0], q[1] - neck[1], 0.0])
        r = float(np.linalg.norm(d))
        if r < 1e-4:
            return q
        d /= r
        o = Vector((float(neck[0]), float(neck[1]), float(q[2])))
        hit = wall_tree.ray_cast(o, Vector(tuple(d)), r + margin)
        if hit[0] is None:
            return q
        r_in = max(hit[3] - margin, 0.6 * hit[3])
        return np.array([neck[0] + d[0] * r_in, neck[1] + d[1] * r_in, q[2]])
    skin_tree = gm.bvh_objects(under_obs[:1])
    hf = fig.hf
    hv = hf['verts']
    band_y = hf['eye_y'] + 0.47
    cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5 - 0.1
    head_c = fig.head_point(np.array([0.0, band_y - 0.3, cz]))
    ring = band_ring(fig, head_c, skin_tree, log)
    eye_z = fig.eye_z
    cards = _Cards()
    span0, span1 = P['span']
    counts = {}

    def back_of(deg):
        return min(1.0, (180.0 - abs(180.0 - deg)) / 150.0)

    def root_at(deg, lift):
        a = math.radians(deg)
        z, room = ring(a)
        z += P['under']
        dvec = direction(a)
        o = head_c + dvec * 0.3
        o[2] = z
        hit = skin_tree.ray_cast(Vector(tuple(o)), Vector(tuple(-dvec)), 0.5)
        if hit[0] is None:
            hit = skin_tree.find_nearest(Vector(tuple(o)))
        # inside the band's room off the scalp (band_ring is the sweatband's inner face), a millimetre spare
        lift = min(lift, max(0.0006, room - 0.001))
        return np.array(hit[0]) + np.array(hit[1]) * lift, z, lift

    # collar length: the tips at the jaw by the temples, round the back into the turned-up collar, below its rim
    tips = P['tips']
    rim = None
    collar = next((o for o in under_obs if o.name.startswith('Collar')), None)
    if collar is not None:
        m = np.array(collar.matrix_world)
        cco = np.array([v.co[:] for v in collar.data.vertices]) @ m[:3, :3].T + m[:3, 3]
        ca = round_angle(cco, head_c)
        bins = np.floor((ca + math.pi) / (2 * math.pi) * 72).astype(int) % 72
        top = np.array([cco[bins == b_, 2].max() if (bins == b_).any() else np.nan for b_ in range(72)])
        good = ~np.isnan(top)
        if good.sum() > 8:
            centres = -math.pi + (np.arange(72) + 0.5) * 2 * math.pi / 72
            rim = (centres[good], top[good])

    def tip_z(deg):
        z = eye_z - tips['temple'] - tips['back'] * back_of(deg) ** 0.6
        if rim is not None and back_of(deg) > 0.4:
            a = math.radians(deg)
            a = a - 2 * math.pi if a > math.pi else a
            z = min(z, float(np.interp(a, rim[0], rim[1], period=2 * math.pi)) - tips['under_rim'])
        return z
    floor_z = eye_z - tips['floor']
    log(f'hair: tips at {tip_z(span0):.3f} m by the temples, {tip_z(90.0):.3f} at the sides, {tip_z(180.0):.3f} at '
        f'the back')

    def normals_for(Pg, out):
        rad = Pg - head_c
        rad /= np.maximum(np.linalg.norm(rad, axis=-1, keepdims=True), 1e-9)
        n = 0.7 * rad + 0.3 * out[:, None, :]
        return n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-9)

    # the sheet: one strip round the back from temple to temple, close to the head, the underlayer
    sh = P['sheet']
    cols = []
    for c in range(sh['cols']):
        deg = span0 + (span1 - span0) * c / (sh['cols'] - 1)
        lo_lift, hi_lift = sh['lift']
        root, zr, lo_lift = root_at(deg, lo_lift)
        L = max(0.03, min((zr - tip_z(deg)) * 1.04 * sh['length'] * (1.0 + 0.25 * back_of(deg)), zr - floor_z - 0.004))
        clear = (lambda lo, hi: (lambda t: lo + (hi - lo) * float(lt.smooth(0.05, 0.35, t))))(lo_lift, hi_lift)
        cols.append(grow(tree, root, L, sh['rows'], clear, 0.25, rng, 0.0, keep_in=keep_in))
    G = np.stack(cols, axis=1)                                     # (rows, cols, 3)
    out_all = G - head_c
    out_all /= np.linalg.norm(out_all, axis=-1, keepdims=True)
    t = np.linspace(0.0, 1.0, G.shape[0])
    U, V = sheet_uv(np.linspace(0.0, 1.0, G.shape[1])[None, :].repeat(G.shape[0], 0), t[:, None].repeat(G.shape[1], 1))
    cards.add_grid(G, U, V, out_all)
    counts['sheet'] = sh['cols']

    # the cards, layer by layer
    for (layer, n, width, lift0, lift1, lscale, kind, rows, across, twist, wob, flick, sweep) in P['layers']:
        strips = strips_of(kind)
        step = (span1 - span0) / n
        shift = 0.5 if layer == 'outer' else 0.0          # the outer layer's cards over the main's gaps
        for c in range(n):
            deg = span0 + (step * (c + 0.5 + shift + rng.uniform(-0.3, 0.3))) % (span1 - span0)
            if layer in ('wisp', 'fly'):
                deg = span0 + (span1 - span0) * rng.uniform(0.02, 0.98)
            root, zr, l0 = root_at(deg, lift0)
            L = max(0.03, min((zr - tip_z(deg)) * 1.04 * rng.uniform(*lscale), zr - floor_z - 0.004))
            # pressed thin under the band and just below it (under the brim's swept path), fuller further down
            clear = (lambda lo, hi: (lambda t: lo + (hi - lo) * float(lt.smooth(0.08, 0.45, t))))(l0, lift1)
            a = math.radians(deg)
            down = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-sweep, sweep) + np.array([0.0, 0.0, -1.0])
            st = grow(tree, root, L, rows, clear, 0.3 + 0.3 * rng.random(), rng, wob, down=tuple(down),
                      flare=rng.uniform(*flick) * (0.4 + 0.6 * back_of(deg)), axis=head_c, keep_in=keep_in)
            tw = rng.uniform(-twist, twist)
            tang, side, out = frames(st, head_c, tw)
            w = width * rng.uniform(0.8, 1.2) * (1.0 - 0.25 * np.linspace(0.0, 1.0, rows))
            cup = np.linalg.norm((st - head_c)[:, :2], axis=1)
            Pg, sfrac = card_grid(st, side, out, w, across, cup)
            Pg = np.array([[keep_in(q) for q in row] for row in Pg])
            k = int(rng.choice(strips))
            sf = sfrac[::-1] if rng.random() < 0.5 else sfrac
            tt = np.linspace(0.0, 1.0, rows)
            U, V = strip_uv(k, sf[None, :].repeat(rows, 0), tt[:, None].repeat(across + 1, 1))
            cards.add_grid(Pg, U, V, normals_for(Pg, out))
        counts[layer] = n

    # the lock out from under the band across the left brow, its ends over the eye's outer corner (clear of the slit)
    br = P['brow']
    eye_l = max(fig.eyes, key=lambda e: e[0])
    for k in range(br['cards']):
        f = k / max(br['cards'] - 1, 1)
        deg = br['degs'][0] + (br['degs'][1] - br['degs'][0]) * f
        root, zr, _ = root_at(deg, 0.003)
        tip = eye_l + np.array([0.012 + 0.012 * f, -0.004, 0.006 - 0.004 * f])
        d = tip - root
        L = float(np.linalg.norm(d)) * 1.05
        st = grow(skin_tree, root, L, br["rows"], lambda t: 0.0012 + 0.003 * t, 0.2, rng, 0.03,
                  down=tuple(d / np.linalg.norm(d)))
        tang, side, out = frames(st, head_c, rng.uniform(-10, 10))
        w = br['width'] * (1.0 - 0.3 * np.linspace(0.0, 1.0, br['rows']))
        Pg, sfrac = card_grid(st, side, out, w, 2, np.full(br['rows'], 0.09))
        kk = int(rng.choice(strips_of('dense') if k < 2 else strips_of('clump')))
        tt = np.linspace(0.0, 1.0, br['rows'])
        U, V = strip_uv(kk, sfrac[None, :].repeat(br['rows'], 0), tt[:, None].repeat(3, 1))
        cards.add_grid(Pg, U, V, normals_for(Pg, out))
    counts['brow'] = br['cards']

    # under the band (its foot to 3.5 cm up the sweatband) the hair lies pressed within the band's room off the scalp
    # (a card's sides stood off the curved head there, the brow lock off the forehead's 2 mm)
    verts = np.array(cards.verts)
    pressed = 0
    for i, q in enumerate(verts):
        z0, room = ring(float(round_angle(q, head_c)[0]))
        if z0 - 0.002 <= q[2] <= z0 + 0.035:
            loc, nrm, _i, _d = skin_tree.find_nearest(Vector(tuple(q)), 0.05)
            if loc is None:
                continue
            allowed = max(0.0006, room - 0.0008)            # (band_ring: the sweatband's inner face)
            d = (Vector(tuple(q)) - loc).dot(nrm)
            if d > allowed:
                verts[i] = q - np.array(nrm) * (d - allowed)
                pressed += 1
    log(f'hair: {pressed} card points pressed in under the band')
    # the hat tipped back while aiming must not sweep its brim through the hair
    verts, moved = under_brim(verts, P.get('pivot'), head_c, skin_tree, log)
    log(f"hair: {moved} card points moved in under the crown, out of the tipped brim's way")
    # a last guard: no card point (a curled edge, a flick) under the floor
    low = verts[:, 2] < floor_z
    verts[low, 2] = floor_z
    log(f"hair: {int(low.sum())} card points lifted to the floor at {floor_z:.3f} m (the collar lining's floor under it)")
    cards.verts = [tuple(q) for q in verts]
    # under the band (its foot to 3.5 cm up the sweatband) the hair must lie within the band's room off the scalp
    over, worst, where = 0, 0.0, []
    for q in verts:
        z0, room = ring(float(round_angle(q, head_c)[0]))
        if z0 <= q[2] <= z0 + 0.035:
            loc, nrm, _i, _d = skin_tree.find_nearest(Vector(tuple(q)), 0.05)
            if loc is not None:
                gap = (Vector(tuple(q)) - loc).dot(nrm) - (room - 0.0005)
                if gap > 0.0:
                    over, worst = over + 1, max(worst, gap)
                    where.append((int(round(math.degrees(float(round_angle(q, head_c)[0])) / 10.0)) * 10,
                                  round((q[2] - z0) * 100, 1), round(gap * 1000, 1)))
    log(f'hair: under the band {over} card points past its room (the most {worst * 1000:.1f} mm) {sorted(where)[:12]}')
    me = bpy.data.meshes.new(name)
    me.from_pydata(cards.verts, [], cards.faces)
    layer_uv = me.uv_layers.new(name='UVMap')
    layer_uv.data.foreach_set('uv', np.asarray(cards.uv, dtype=np.float32).ravel())
    me.validate(clean_customdata=False)
    me.update()
    me.shade_smooth()
    me.normals_split_custom_set_from_vertices([tuple(n) for n in cards.normals])
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(mat)
    tris = sum(len(f) - 2 for f in cards.faces)
    co = np.array(cards.verts)
    back = co[np.abs(np.abs(round_angle(co, head_c)) - math.pi) < 0.35]
    log(f'{name}: hair cards ' + ', '.join(f'{k} {v}' for k, v in counts.items()) + f'; {tris} triangles; '
        f'the back from {back[:, 2].max():.3f} to {back[:, 2].min():.3f} m' if len(back) else '')
    return ob


def under_brim(verts, pivot, head_c, skin_tree, log, degs=(5.0, 10.0, 15.0, 20.0), margin=0.006):
    """Card points the hat's brim would sweep through as the game tips it back on its bone (pivot: head, tail; up to
    Ellis.py's HAT_TIP 20 degrees, front up: the brim's back comes down) moved in under the crown, or down if the
    skull is in the way. margin: the brim's half thickness (5.5 mm game brim) and a little air."""
    from mathutils.bvhtree import BVHTree
    brims = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('HatBrim')
             and not o.name.endswith('_low')]
    if not brims or pivot is None:
        return verts, 0
    co, faces = [], []
    for o in brims:
        m = np.array(o.matrix_world)
        c = np.array([v.co[:] for v in o.data.vertices]) @ m[:3, :3].T + m[:3, 3]
        faces += [[i + len(co) for i in p.vertices] for p in o.data.polygons]
        co += [tuple(q) for q in c]
    co = np.array(co)
    h, t = (np.asarray(x, dtype=np.float64) for x in pivot)
    axis = np.cross((t - h) / np.linalg.norm(t - h), (0.0, 0.0, 1.0))
    axis /= np.linalg.norm(axis)
    rest = BVHTree.FromPolygons([tuple(q) for q in co], faces)
    tipped = []
    for d in degs:
        a = math.radians(d)
        K = np.array([[0, -axis[2], axis[1]], [axis[2], 0, -axis[0]], [-axis[1], axis[0], 0]])
        R = np.eye(3) + math.sin(a) * K + (1 - math.cos(a)) * K @ K
        tipped.append(BVHTree.FromPolygons([tuple(q) for q in (co - h) @ R.T + h], faces))
    up, down = Vector((0.0, 0.0, 1.0)), Vector((0.0, 0.0, -1.0))

    def hit(q):
        v = Vector(tuple(q))
        under_rest = rest.ray_cast(v, up, 0.3)[0] is not None
        for tr in tipped:
            if tr.ray_cast(v, up, margin)[0] is not None or tr.ray_cast(v, down, margin)[0] is not None:
                return True
            if under_rest and tr.ray_cast(v, up, 0.3)[0] is None:
                return True                                # the brim came down past it
        return False
    out = verts.copy()
    moved = 0
    zmin = co[:, 2].min() - 0.05
    for i, q in enumerate(verts):
        if q[2] < zmin or not hit(q):
            continue
        r = np.array([q[0] - head_c[0], q[1] - head_c[1], 0.0])
        r /= max(np.linalg.norm(r), 1e-9)
        new = None
        for k in range(1, 16):
            c = q - r * 0.002 * k
            loc, nrm, _i, _d = skin_tree.find_nearest(Vector(tuple(c)), 0.05)
            if loc is not None and (Vector(tuple(c)) - loc).dot(nrm) < 0.0015:
                break
            if not hit(c):
                new = c
                break
        if new is None:
            for k in range(1, 21):
                c = q - np.array([0.0, 0.0, 0.002 * k])
                if not hit(c):
                    new = c
                    break
        if new is not None:
            out[i] = new
            moved += 1
    return out, moved


def chain_joints(co, centre, top, back=math.pi, width=math.radians(25)):
    """The back hair's swing chain (Ellis.py's swing_layout: hair_01's head, hair_02's, then the end): down the middle
    of the back at the cards' median depth at each height (the outermost card, a flicked one or a fly-away, bent the
    chain 10 cm out), from top down to where most of the back hair ends (lying on the nape, inside the collar)."""
    ang = round_angle(co, centre)
    sel = co[np.abs((ang - back + math.pi) % (2 * math.pi) - math.pi) < width]
    r_all = np.hypot(sel[:, 0] - centre[0], sel[:, 1] - centre[1])
    end_z = float(np.percentile(sel[:, 2], 8))
    d = direction(back)

    def at(z):
        near = np.abs(sel[:, 2] - z) < 0.012
        r = float(np.median(r_all[near])) if near.any() else float(np.median(r_all))
        return np.array([centre[0] + d[0] * r, centre[1] + d[1] * r, z])
    return [at(top), at((top + end_z) * 0.5), at(end_z)]


# ------------------------------------------------------------------------------------------------- the look renders

FLAT = {   # the other parts in flat colours near their looks (linear)
    'Skin': (0.42, 0.36, 0.33), 'Scalp': (0.04, 0.032, 0.026), 'Felt': (0.16, 0.12, 0.09), 'HatBand': (0.09, 0.05, 0.03),
    'Bandana': (0.45, 0.06, 0.04), 'Duster': (0.20, 0.14, 0.09), 'Vest': (0.03, 0.03, 0.035), 'Shirt': (0.30, 0.31, 0.33),
    'EyeGlow': (0.3, 0.9, 1.0),
}
VIEWS = (('back', 180.0), ('bq', 135.0), ('side', 90.0), ('q3', 35.0), ('front', 0.0), ('rside', -90.0))
OUT = os.path.join(REPO, 'Saved', 'Concepts', 'EllisRound6', 'hair')


def _flat_material(key, colour):
    name = f'_Flat_{key}'
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    b = mat.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*colour, 1.0)
    b.inputs['Roughness'].default_value = 0.75
    if key == 'EyeGlow':
        b.inputs['Emission Color'].default_value = (*colour, 1.0)
        b.inputs['Emission Strength'].default_value = 4.0
    return mat


def look(tag, head, cycles=False):
    """Renders the built figure's head (the scene as Ellis.py left it, hair textured, the rest flat) from VIEWS at
    about 1 m and 3 m (the 3 m ones at the game's 1080p, 90 degree scale), and a contact sheet."""
    sc = bpy.context.scene
    for ob in sc.objects:
        if ob.type != 'MESH':
            continue
        if ob.name.startswith('_'):
            ob.hide_render = True
            continue
        if ob.name.startswith('Hair'):
            continue
        for i, s in enumerate(ob.material_slots):
            key = s.material['LookKey'] if s.material and 'LookKey' in s.material else ''
            colour = FLAT.get(key, FLAT['Duster'] if key in ('Duster', 'Collar', 'Cape') else (0.18, 0.16, 0.14))
            ob.material_slots[i].material = _flat_material(key or 'Other', colour)
    sc.render.engine = 'CYCLES' if cycles else 'BLENDER_EEVEE_NEXT'
    if cycles:
        # Cycles: the anisotropic highlight across the strands (Eevee draws none), as the game's Hair shading has
        sc.cycles.device = 'CPU'
        sc.cycles.samples = 64
        sc.cycles.use_denoising = True
        sc.cycles.transparent_max_bounces = 24
    ee = sc.eevee
    for attr, value in (('taa_render_samples', 96), ('use_shadows', True), ('use_raytracing', True),
                        ('shadow_ray_count', 2), ('shadow_step_count', 8)):
        if hasattr(ee, attr):
            setattr(ee, attr, value)
    try:
        sc.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    world = bpy.data.worlds.get('HairLook') or bpy.data.worlds.new('HairLook')
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (0.42, 0.50, 0.62, 1.0)
    bg.inputs['Strength'].default_value = 0.9
    sc.world = world
    sun = bpy.data.objects.get('HairSun')
    if sun is None:
        ld = bpy.data.lights.new('HairSun', 'SUN')
        ld.energy = 4.0
        ld.angle = math.radians(2.0)
        ld.color = (1.0, 0.93, 0.82)
        sun = bpy.data.objects.new('HairSun', ld)
        sc.collection.objects.link(sun)
    # about 40 degrees up, from his front left (the game's afternoon sun)
    sun.rotation_euler = (math.radians(50.0), 0.0, math.radians(-150.0))
    cd = bpy.data.cameras.get('HairCam') or bpy.data.cameras.new('HairCam')
    cam = bpy.data.objects.get('HairCam') or bpy.data.objects.new('HairCam', cd)
    if cam.name not in sc.collection.objects:
        sc.collection.objects.link(cam)
    sc.camera = cam
    os.makedirs(OUT, exist_ok=True)
    shots = []
    # (label, distance, lens, size, height above the head's aim)
    for label, dist, lens, size, up in (('1m', 1.0, 75.0, 900, 0.02), ('3m', 3.0, 54.0, 640, 0.30))[:1 if cycles else 2]:
        cd.lens = lens
        sc.render.resolution_x = sc.render.resolution_y = size
        for view, yaw in (VIEWS[:4] if cycles else VIEWS):
            a = math.radians(yaw)
            target = Vector((float(head[0]), float(head[1]), float(head[2]) - (0.0 if label == '1m' else 0.25)))
            cam.location = target + Vector((math.sin(a) * dist, -math.cos(a) * dist, up))
            cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
            sc.render.filepath = os.path.join(OUT, f'{tag}_{label}_{view}.png')
            bpy.ops.render.render(write_still=True)
            shots.append(sc.render.filepath)
            print(f'HAIR look: {sc.render.filepath}', flush=True)
    contact_sheet(shots, os.path.join(OUT, f'{tag}_sheet.png'), cols=4 if cycles else len(VIEWS))


def contact_sheet(paths, out, cell=420, cols=None):
    """The renders side by side (rows of len(VIEWS)), each scaled to cell px."""
    imgs = []
    for p in paths:
        im = bpy.data.images.load(p, check_existing=False)
        w, h = im.size
        px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1, :, :3]
        bpy.data.images.remove(im)
        ys = (np.arange(cell) * h / cell).astype(int)
        xs = (np.arange(cell) * w / cell).astype(int)
        imgs.append(px[ys][:, xs])
    cols = cols or len(VIEWS)
    rows = (len(imgs) + cols - 1) // cols
    sheet = np.zeros((rows * cell, cols * cell, 3), np.float32)
    for i, im in enumerate(imgs):
        r, c = divmod(i, cols)
        sheet[r * cell:(r + 1) * cell, c * cell:(c + 1) * cell] = im
    # the renders are already display-referred (AgX, sRGB): written as the bytes they are
    lt.write_png(out, lt.to8(sheet))
    print(f'HAIR look: {out}', flush=True)


def run_look(tag, cycles=False):
    """Builds Ellis (Ellis.py's main in its --shape mode, no simulation, its previews skipped) and renders."""
    sys.argv = [sys.argv[0], '--', '--shape', '--no-sim']
    sys.path.insert(0, os.path.join(REPO, 'Art', 'Models', 'Characters'))
    import Ellis as E
    E.preview = lambda *a, **k: None
    E.main()
    fig_head = None
    for ob in bpy.context.scene.objects:
        if ob.name.startswith('Eye_'):
            p = ob.matrix_world.translation
            fig_head = (0.0, p.y + 0.03, p.z + 0.02) if fig_head is None else fig_head
    if tag != 'none':                      # --look none: the build and its numbers only
        look(tag, fig_head, cycles)


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    args = [a for arg in args for a in arg.split(',') if a]
    if '--textures' in args:
        print(make_textures())
    if '--look' in args:
        run_look(args[args.index('--look') + 1] if args.index('--look') + 1 < len(args) else 'look', '--aniso' in args)
