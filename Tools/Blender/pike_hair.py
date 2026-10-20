"""Pike's hair as hair cards (Art/Models/Characters/Pike.py's build calls build() here), as Ellis's (ellis_hair.py: the
user found Ellis's earlier hair "mud"; cards on a script-made strand atlas are how real-time hair is made):

  - The atlas (make_textures; Art/Textures/PikeHair/T_PikeHair_BC|N|ORM.png, 2048): ellis_hair's layout exactly (the
    top quarter a sheet tiling across, eight strips below: two dense, three clumped, two wisps, one fly-aways), so its
    strip_uv and sheet_uv place Pike's cards too; the strands grey salt and pepper (the portrait's: most white-grey,
    about a third dark), wiry, yellowed a little toward the tips (pipe smoke). Drawn one by one in numpy.
  - The whiskers (whiskers): mutton chops from the temples down over the cheeks to the jaw, joined round the mouth's
    corners to a big mustache drooping over the lip (the portrait's regions: Pike.py beard_mask), each an underlayer
    sheet close to the skin and layered cards over it, grown along the portrait's directions; the chin bare.
  - The brows (brows): short cards along each brow ridge, bushy, flicking up and out at the ends.
  - The hair under the bowler (ellis_hair.build with Pike's cards: short round the back from ear to ear, the roots
    under the hat band).
"""
import math
import os

import numpy as np

try:
    import bpy
    from mathutils import Vector
except ImportError:      # plain Python with numpy still makes the textures
    bpy = Vector = None

import ellis_hair as eh
import looter_textures as lt

SET = 'PikeHair'
SIZE, SHEET_ROWS, STRIP_ROWS, STRIP_W, PAD, KINDS, PX_M = eh.SIZE, eh.SHEET_ROWS, eh.STRIP_ROWS, eh.STRIP_W, eh.PAD, \
    eh.KINDS, eh.PX_M

# sRGB: the light strands white-grey, a little warm; the dark ones grey-brown; tips yellowed (smoke) and dusty
LIGHT = (np.array([0.50, 0.48, 0.45]), np.array([0.72, 0.70, 0.66]), np.array([0.80, 0.77, 0.70]))
DARK = (np.array([0.13, 0.115, 0.10]), np.array([0.24, 0.215, 0.19]), np.array([0.32, 0.29, 0.25]))
DARK_SHARE = 0.32
DUST = np.array([0.55, 0.50, 0.42])
KIND = {k: dict(v) for k, v in eh.KIND.items()}
for _k in KIND.values():
    _k['width'] = (_k['width'][0] * 1.1, _k['width'][1] * 1.15)        # coarser, wiry whiskers
# Soft card edges (2026-10-20: up close in game the chops' edge stepped like stairs, each card's top and sides a hard
# line): the strands' roots fade in from a start of their own, up to this share of the card down (the broad dark under-
# strands, the rest), over this share; toward a strip's sides they thin to SIDE_FLOOR over SIDE_SHARE of its width.
ROOT_FADE = (0.05, 0.16, 0.07)
SIDE_FLOOR, SIDE_SHARE = 0.2, 0.2
CHOP_EDGE = 26           # narrow cards each side along the chops' edge up the cheek (whiskers)


def _region(h, w, kind, seed, periodic=False):
    """ellis_hair._region's strands, Pike's colours: each strand light or dark (salt and pepper), wavier."""
    cfg = KIND[kind]
    rng = np.random.default_rng(seed)
    acc = eh._Acc(h, w)
    lo_x, hi_x = (0.0, float(w)) if periodic else (PAD, w - PAD)
    span_x = hi_x - lo_x
    t_all = np.arange(h, dtype=np.float64) / (h - 1)
    n_cl = int(rng.integers(cfg['clumps'][0], cfg['clumps'][1] + 1)) if cfg['clumps'][1] else 0
    clumps = []
    spacing = span_x / max(n_cl, 1)
    for c in range(n_cl):
        cx0 = lo_x + (c + 0.5 + rng.uniform(-0.25, 0.25)) / n_cl * span_x
        if not periodic:
            cx0 = lo_x + span_x * (0.5 + (cx0 - lo_x - 0.5 * span_x) / span_x * cfg['spread'] * 0.85)
        wave = (rng.uniform(2.0, 8.0), rng.uniform(0.5, 1.5), rng.uniform(0, 6.28))
        L = rng.uniform(*cfg['clump_len'])
        n_sub = int(rng.integers(cfg['subs'][0], cfg['subs'][1] + 1))
        subs = []
        for k in range(n_sub):
            off = ((k + 0.5) / n_sub - 0.5) * spacing * 0.75 + rng.normal() * spacing * 0.06
            subs.append(dict(off=off, drift=rng.normal() * spacing * 0.12, L=min(1.0, L * rng.uniform(0.78, 1.0))))
        clumps.append(dict(x0=cx0, drift=rng.normal() * spacing * 0.25, wave=wave, L=L, v=math.exp(rng.normal() * 0.12),
                           r=rng.normal() * 0.04, subs=subs, dark=rng.random() < DARK_SHARE * 0.6))

    def clump_x(c, t):
        a_, f_, p_ = c['wave']
        return c['x0'] + c['drift'] * t + a_ * np.sin(2 * math.pi * (f_ * t) + p_)

    def offset(x0, c):
        return ((x0 - c['x0'] + w / 2) % w) - w / 2 if periodic else x0 - c['x0']

    strands = []
    for i in range(cfg['body'] + cfg['strands']):
        body = i < cfg['body']
        if n_cl:
            if body:
                c = clumps[i % n_cl]
                x0 = c['x0'] + rng.uniform(-0.36, 0.36) * spacing
            else:
                x0 = lo_x + rng.uniform() * span_x
                if not periodic:
                    x0 = lo_x + span_x * (0.5 + (rng.uniform() + rng.uniform() * 0.3 - 0.65) / 1.3 * cfg['spread'])
                order = np.argsort([abs(offset(x0, cc)) for cc in clumps])
                c = clumps[order[0] if rng.random() < 0.85 or len(order) < 2 else order[1]]
            rel = offset(x0, c)
            sub = min(c['subs'], key=lambda q: abs(q['off'] - rel))
            L = min(0.995, sub['L'] * (rng.uniform(0.62, 0.85) if body else rng.uniform(0.78, 1.04)))
        else:
            c = sub = None
            x0 = lo_x + rng.uniform(0.1, 0.9) * span_x
            L = rng.uniform(*cfg['clump_len'])
        rows = int(L * (h - 1)) + 1
        t = t_all[:rows]
        wa, wf, wp = rng.uniform(0.4, 2.2), rng.uniform(1.0, 4.0), rng.uniform(0, 6.28)      # wiry: wavier
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
            bend = rng.normal() * span_x * 0.12
            x = np.clip(x0 + bend * t ** 2.0 + wav * 1.5, lo_x, hi_x)
        wd = (rng.uniform(6.0, 13.0) if body else rng.uniform(*cfg['width'])) * np.ones(rows)
        tipf = lt.smooth(L - 0.16, L, t) if L > 0.2 else t / max(L, 1e-3)
        wd = wd * (1.0 - 0.65 * tipf)
        a = (1.0 - lt.smooth(L - 0.035, L, t)) * (1.0 if body else rng.uniform(0.86, 1.0))
        # each strand fading in from its own start a little way down: a card's top lying over the skin (the chops'
        # edge up the cheek, the brows) has no hard line across it (in game the mask clipped it into steps)
        t0 = rng.uniform(0.0, ROOT_FADE[0] if body else ROOT_FADE[1])
        a = a * lt.smooth(t0, t0 + ROOT_FADE[2], t)
        if not periodic:
            edge = np.minimum(x - lo_x, hi_x - x)
            a = a * (SIDE_FLOOR + (1.0 - SIDE_FLOOR) * np.clip(edge / (SIDE_SHARE * span_x), 0.0, 1.0)) * \
                lt.smooth(0.0, 8.0, edge)
        dark = (rng.random() < DARK_SHARE) or (c is not None and c['dark'] and rng.random() < 0.5)
        root, mid, tip = DARK if dark else LIGHT
        v = math.exp(rng.normal() * 0.12) * (c['v'] if c is not None else 1.0) * cfg['value']
        col = root[None] + (mid - root)[None] * lt.smooth(0.0, 0.35, t)[:, None]
        tt = lt.smooth(0.5, 1.0, t / max(L, 1e-3))[:, None]
        col = col + (tip - col) * tt
        col = col + (DUST - col) * (0.10 * tt)
        col = col * v
        if body:
            col = col * 0.62
        rough = 0.44 + rng.normal() * 0.05 + (c['r'] if c is not None else 0.0)
        rough = rough + 0.07 * lt.smooth(0.6, 1.0, t / max(L, 1e-3)) + 0.05 * (1.0 - lt.smooth(0.0, 0.2, t))
        if body:
            rough = rough + 0.08
        z = rng.uniform(0.0, 0.3) if body else rng.uniform(0.3, 1.0)
        strands.append((z, x, wd, a, np.clip(col, 0, 1), np.clip(rough, 0.2, 0.8), rng.random() if not body else 0.5))
    if 'fill' in cfg:
        edge = cfg['fill'] + 0.08 * lt.noise((8, w), seed + 99, 40.0, 0.01)[4]
        fill_a = 1.0 - lt.smooth(edge[None, :] - 0.08, edge[None, :], t_all[:, None])
        fill_a = fill_a * lt.smooth(0.0, ROOT_FADE[2] + 0.03, t_all)[:, None]       # the sheet's roots fade in too
        mixc = LIGHT[0] * (1 - DARK_SHARE) + DARK[0] * DARK_SHARE
        mixm = LIGHT[1] * (1 - DARK_SHARE) + DARK[1] * DARK_SHARE
        fc = (mixc + (mixm - mixc) * lt.smooth(0.0, 0.5, t_all)[:, None]) * cfg['value'] * 0.62
        acc.A[:] = fill_a
        acc.C[:] = fc[:, None, :] * fill_a[..., None]
        acc.R[:] = 0.5 * fill_a
        acc.I[:] = 0.5 * fill_a
    strands.sort(key=lambda s: s[0])
    for z, x, wd, a, col, rough, sid in strands:
        eh._paint(acc, x, wd, a, col, rough, sid, z, periodic)
    A = acc.A
    safe = np.maximum(A, 1e-4)
    colour = acc.C / safe[..., None]
    rough = np.where(A > 1e-3, acc.R / safe, 0.5)
    sid = np.where(A > 1e-3, acc.I / safe, 0.5)
    height = np.where(A > 1e-3, acc.Hg / safe, 0.0)
    depth = np.where(A > 1e-3, acc.Z / safe, 0.0)
    return colour, A, height, rough, sid, depth, t_all


def make_textures(look=False):
    """Writes Art/Textures/PikeHair/T_PikeHair_BC (RGBA), _N and _ORM (B: the strands' ids, not metal)."""
    colour = np.zeros((SIZE, SIZE, 3), np.float32)
    alpha = np.zeros((SIZE, SIZE), np.float32)
    height = np.zeros((SIZE, SIZE), np.float32)
    rough = np.full((SIZE, SIZE), 0.5, np.float32)
    sid = np.full((SIZE, SIZE), 0.5, np.float32)
    ao = np.ones((SIZE, SIZE), np.float32)
    regions = [('sheet', SHEET_ROWS, 0, SIZE, 13, True, 0.72)]
    for k, kind in enumerate(KINDS):
        regions.append((kind, STRIP_ROWS, k * STRIP_W, STRIP_W, 211 + 17 * k, False, 0.92 if kind == 'dense' else 1.0))
    for kind, (r0, r1), x0, w, seed, periodic, shade in regions:
        h = r1 - r0
        c, a, hg, r, i, dpt, t = _region(h, w, kind, seed, periodic)
        colour[r0:r1, x0:x0 + w] = c
        alpha[r0:r1, x0:x0 + w] = a
        height[r0:r1, x0:x0 + w] = hg
        rough[r0:r1, x0:x0 + w] = r
        sid[r0:r1, x0:x0 + w] = i
        root = 0.55 + 0.45 * lt.smooth(0.0, 0.4, t)[:, None]
        ao[r0:r1, x0:x0 + w] = np.clip(root * (0.72 + 0.28 * dpt) * shade, 0.0, 1.0)
        print(f'PIKE HAIR texture: {kind} at ({x0}, {r0}): coverage {a.mean():.2f}', flush=True)
    normal = lt.normal_map(height / PX_M, 1.0 / PX_M * 1.0, strength=0.7)
    normal[alpha < 0.02] = (128, 128, 255)
    lt.save_set(SET, colour, height / PX_M, 1.0 / PX_M, rough, metal=sid, ao=ao, alpha=alpha, normal=normal, look=look)
    return {k: lt.texture_path(SET, k) for k in ('BC', 'N', 'ORM')}


def have_textures():
    return all(os.path.exists(lt.texture_path(SET, k)) for k in ('BC', 'N', 'ORM'))


def material(name='Pike_Hair'):
    """The atlas on a Principled BSDF for previews (alpha hashed, two-sided; Cycles: an anisotropic highlight along
    the strands), as ellis_hair.material's."""
    if not have_textures():
        make_textures()
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
    for key, value in (('Specular IOR Level', 0.5), ('Anisotropic', 0.6), ('Coat Weight', 0.0)):
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


# ------------------------------------------------------------------------------------------------------- the whiskers

def regions(fig):
    """Per MakeHuman vertex: 0 none, 1 the chops, 2 the corners by the mouth, 3 the mustache (Pike.py's beard_mask,
    split)."""
    v = fig.b.h['verts']
    ey = fig.hf['eye_y']
    ax = np.abs(v[:, 0])
    ang = np.degrees(np.arctan2(ax, v[:, 2] - 0.45))
    top = ey - 0.64 + 0.82 * np.clip((ang - 35) / 65, 0, 1)
    chops = (ax > 0.42) & (ang > 30) & (ang < 104) & (v[:, 1] > ey - np.where(ang < 60, 0.8, 1.2)) & (v[:, 1] < top)
    corner = (ax > 0.30) & (ax < 0.55) & (v[:, 1] > ey - 0.72) & (v[:, 1] < ey - 0.52) & (v[:, 2] > 0.9)
    mous = (ax < 0.42) & (v[:, 1] > ey - 0.63) & (v[:, 1] < ey - 0.47) & (v[:, 2] > 1.1)
    head = fig.w_head > 0.6
    out = np.zeros(len(v), dtype=np.int64)
    out[chops & head] = 1
    out[corner & head] = 2
    out[mous & head] = 3
    return out


def grow_bent(tree, root, length, rows, clear, out_bias, rng, wob, down, bend):
    """ellis_hair.grow's walk over the surfaces in tree, its direction turning toward bend along the card (a mutton
    chop's ends curling forward round the jaw)."""
    pts = [np.array(root, dtype=np.float64)]
    p = Vector(tuple(root))
    step = length / (rows - 1)
    w = Vector(tuple(rng.normal(size=3))) * wob
    for k in range(rows - 1):
        t = (k + 1) / (rows - 1)
        d = (Vector(tuple(down)) + w * t + Vector(tuple(bend)) * t ** 1.5).normalized()
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
        pts.append(np.array(p[:]))
    return np.array(pts)


def _card(cards, tree, head_c, rng, root, direction, length, width, kind, rows=5, lift=(0.0015, 0.004), out_bias=0.3,
          wob=0.04, twist=12.0, across=2, cup=0.06, flare=0.0, normal_bias=None, bend=None, top=None):
    """One card from root along direction for length, kept lift(t) off the surfaces in tree, curled to cup, UVs on a
    strip of kind; bend turns it along its length; top(point), the height of the edge it grows under there."""
    clear = lambda t: lift[0] + (lift[1] - lift[0]) * float(lt.smooth(0.0, 0.6, t))
    if bend is not None:
        st = grow_bent(tree, root, length, rows, clear, out_bias, rng, wob, direction, bend)
    else:
        st = eh.grow(tree, root, length, rows, clear, out_bias, rng, wob, down=tuple(direction), flare=flare, axis=head_c)
    tang, side, out = eh.frames(st, head_c, rng.uniform(-twist, twist))
    w = width * rng.uniform(0.85, 1.15) * (1.0 - 0.35 * np.linspace(0.0, 1.0, rows))
    Pg, sfrac = eh.card_grid(st, side, out, w, across, np.full(rows, cup))
    if top is not None:
        # each column's root end slid down the card to under the edge, easing out toward the tip: a card's level top
        # across a rising edge (the chops' up the cheek) stood out of it as a step (2026-10-20)
        ease = 1.0 - np.linspace(0.0, 1.0, rows)
        for j in range(Pg.shape[1]):
            over = Pg[0, j, 2] - top(Pg[0, j])
            if over > 0.0:
                Pg[:, j] += tang * (over / max(-tang[0, 2], 0.3) * ease)[:, None]
    k = int(rng.choice(eh.strips_of(kind)))
    sf = sfrac[::-1] if rng.random() < 0.5 else sfrac
    tt = np.linspace(0.0, 1.0, rows)
    U, V = eh.strip_uv(k, sf[None, :].repeat(rows, 0), tt[:, None].repeat(across + 1, 1))
    rad = Pg - head_c
    rad /= np.maximum(np.linalg.norm(rad, axis=-1, keepdims=True), 1e-9)
    nb = normal_bias if normal_bias is not None else rad
    n = 0.6 * nb + 0.4 * out[:, None, :]
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-9)
    cards.add_grid(Pg, U, V, n)


def _sheet(cards, tree, head_c, rng, roots, dirs, lengths, rows=5, lift=(0.0012, 0.0028)):
    """An underlayer: columns from roots (in order across) along dirs, joined into one grid on the atlas's sheet."""
    cols = []
    for r, d, L in zip(roots, dirs, lengths):
        clear = (lambda lo, hi: (lambda t: lo + (hi - lo) * float(lt.smooth(0.0, 0.5, t))))(*lift)
        cols.append(eh.grow(tree, r, L, rows, clear, 0.25, rng, 0.0, down=tuple(d)))
    G = np.stack(cols, axis=1)
    out = G - head_c
    out /= np.linalg.norm(out, axis=-1, keepdims=True)
    t = np.linspace(0.0, 1.0, G.shape[0])
    U, V = eh.sheet_uv(np.linspace(0.0, 0.5, G.shape[1])[None, :].repeat(G.shape[0], 0) + rng.uniform(0, 0.5),
                       t[:, None].repeat(G.shape[1], 1))
    cards.add_grid(G, U, V, out)


def _object(name, cards, mat):
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
    return ob


def whiskers(M, fig, skin, mat):
    """The mutton chops, the corners and the mustache, each built like shingles: a sheet close to the skin, then rows
    of wide cards from roots in a grid over the region, the higher roots' cards lying over the lower ones (hair from
    higher up falls over the roots below it), and a few wisps over all."""
    import looter_garments as gm
    tree = gm.bvh_objects([skin])
    reg = regions(fig)
    W = fig.wv
    rng = np.random.default_rng(41)
    fwd = np.array([0.0, -1.0, 0.0])
    down = np.array([0.0, 0.0, -1.0])
    head_c = fig.sk['head'][0] + np.array([0.0, 0.02, 0.02])
    cards = eh._Cards()
    counts = {}

    def snap(p, lift=0.0015):
        loc, nrm, _i, _d = tree.find_nearest(Vector(tuple(p)), 0.05)
        return (np.array(loc) + np.array(nrm) * lift, np.array(nrm)) if loc is not None else (np.asarray(p), None)

    def on_skin(a_deg, z, sx):
        """The skin's point at angle a_deg (from the front round the head's axis, toward side sx) and height z."""
        a = math.radians(a_deg) * sx
        d = np.array([math.sin(a), -math.cos(a), 0.0])
        o = np.array([head_c[0], head_c[1], z]) + d * 0.3
        hit = tree.ray_cast(Vector(tuple(o)), Vector(tuple(-d)), 0.4)
        return None if hit[0] is None else np.array(hit[0])

    def add(key, n=1):
        counts[key] = counts.get(key, 0) + n

    for sx in (1.0, -1.0):
        side = np.array([sx, 0.0, 0.0])
        # ---- the chops: the region by angle round the head, its top and bottom at each angle
        idx = np.nonzero(((reg == 1) | (reg == 2)) & (W[:, 0] * sx > 0.0))[0]
        P = W[idx]
        ang = np.degrees(np.abs(round_angle(P, head_c)))
        cols = np.linspace(ang.min() + 2.0, ang.max() - 1.0, 12)
        prof = []
        for c in cols:
            sel = np.abs(ang - c) < 5.0
            if sel.any():
                # no lower than the jaw, and higher toward the ear (turned, the head brought them down on the collar)
                prof.append((c, P[sel, 2].max(), max(P[sel, 2].min(), fig.chin_z + 0.012 + 0.016 * float(np.clip((c - 55.0) / 30.0, 0, 1)))))
        prof = np.array(prof)
        roots = []
        for c, t, b in prof:
            q = on_skin(c, t - 0.003, sx)
            roots.append(snap(q if q is not None else P[np.argmin(np.abs(ang - c))])[0])
        dirs = [gm.unit(down + 0.12 * fwd + 0.18 * side) for _ in prof]
        lengths = [max(0.02, (t - b) * 1.04) for c, t, b in prof]
        _sheet(cards, tree, head_c, rng, roots, dirs, lengths, rows=6, lift=(0.0012, 0.0026))
        add('chops sheet')
        # four levels of cards, the higher roots' over the lower ones; the cards curl forward toward the mouth lower
        # down (more at the front of the cheek), the outer levels clumped (their tapered ends break the outline up)
        a_front, a_back = prof[0, 0], prof[-1, 0]
        edge_at = lambda q: float(np.interp(np.degrees(np.abs(round_angle(q, head_c)[0])), prof[:, 0], prof[:, 1])) - 0.003
        for level, (f, lift, kind, width) in enumerate(((0.72, (0.0028, 0.0065), 'dense', 0.022),
                                                       (0.50, (0.0048, 0.0098), 'dense', 0.022),
                                                       (0.27, (0.0068, 0.0130), 'clump', 0.021),
                                                       (0.04, (0.0090, 0.0165), 'clump', 0.020))):
            stagger = 0.5 * (level % 2)
            for k in range(len(prof) - (1 if stagger else 0)):
                c = prof[k, 0] + stagger * (prof[min(k + 1, len(prof) - 1), 0] - prof[k, 0])
                t = np.interp(c, prof[:, 0], prof[:, 1])
                b = np.interp(c, prof[:, 0], prof[:, 2])
                z = t - f * (t - b) - 0.003
                p = on_skin(c + rng.uniform(-1.5, 1.5), z, sx)
                if p is None:
                    continue
                r, _n = snap(p, lift[0])
                L = (z - b) * 0.98 + 0.008 + rng.uniform(-0.003, 0.004)
                front = float(np.clip((a_back - c) / max(a_back - a_front, 1.0), 0.0, 1.0))
                d = gm.unit(down + 0.10 * fwd + 0.20 * side + rng.normal(size=3) * 0.05)
                bend = 0.25 * fwd * (0.4 + 0.8 * front) - 0.10 * side
                # the lower levels lie under the rest: four rows, the lowest flat (their triangles pay for the edge's
                # cards)
                rows_, across_ = ((4, 1), (4, 2), (5, 2), (5, 2))[level]
                _card(cards, tree, head_c, rng, r, d, max(L, 0.016), width, kind, rows=rows_, lift=lift, out_bias=0.25,
                      wob=0.05, twist=14.0, across=across_, cup=0.06, bend=bend, top=edge_at)
                add(f'chops {kind}')
        # the edge up the cheek: the chops' top rises from the mouth's corner to the temple, and the levels' cards, each
        # with a level top, stepped up it like stairs; narrow cards rooted close together along it, over the levels,
        # their roots fading in (the atlas), break it up as a whisker's edge
        for k in range(CHOP_EDGE):
            c = a_front + (a_back - a_front) * (k + rng.uniform(0.2, 0.8)) / CHOP_EDGE
            t = np.interp(c, prof[:, 0], prof[:, 1])
            b = np.interp(c, prof[:, 0], prof[:, 2])
            z = t - rng.uniform(0.002, 0.008)
            p = on_skin(c, z, sx)
            if p is None:
                continue
            r, _n = snap(p, 0.0095)
            front = float(np.clip((a_back - c) / max(a_back - a_front, 1.0), 0.0, 1.0))
            L = min(0.6 * (z - b), 0.034) * rng.uniform(0.75, 1.0) + 0.008
            d = gm.unit(down + 0.10 * fwd + 0.20 * side + rng.normal(size=3) * 0.08)
            kind = 'wisp' if k % 3 == 2 else 'clump'
            _card(cards, tree, head_c, rng, r, d, max(L, 0.014), rng.uniform(0.009, 0.012), kind, rows=4,
                  lift=(0.0095, 0.0170), out_bias=0.25, wob=0.05, twist=16.0, across=1, cup=0.0,
                  bend=0.25 * fwd * (0.4 + 0.8 * front) - 0.10 * side, top=edge_at)
            add('chops edge')
        for _ in range(6):
            i = idx[rng.integers(len(idx))]
            r, _n = snap(W[i], 0.006)
            d = gm.unit(down + 0.15 * fwd + 0.3 * side + rng.normal(size=3) * 0.12)
            _card(cards, tree, head_c, rng, r, d, rng.uniform(0.022, 0.034), 0.011, 'wisp', rows=4,
                  lift=(0.0065, 0.010), out_bias=0.3, flare=0.08)
            add('chops wisp')
        # ---- the mustache: roots in rows under the nose across the upper lip, fanning out and drooping
        midx = np.nonzero((reg == 3) & (W[:, 0] * sx > -0.003))[0]
        Q = W[midx]
        xs = np.linspace(0.003, 0.040, 8)
        lines = []
        for x in xs:
            sel = np.abs(Q[:, 0] * sx - x) < 0.006
            if sel.any():
                q = Q[sel]
                lines.append((x, q[:, 2].max(), q[:, 2].min(), q[np.argmax(q[:, 2]), 1]))
        lines = np.array(lines)
        nose_z = Q[:, 2].max()
        roots, dirs, lengths = [], [], []
        for x, t, b, y in lines:
            k = min(x / 0.040, 1.0)
            roots.append(snap(np.array([sx * x, y - 0.01, t - 0.002]), 0.0018)[0])
            dirs.append(gm.unit(np.array([sx * (0.25 + 1.4 * k), -0.5, -1.0])))
            lengths.append(0.024 + 0.016 * k)
        _sheet(cards, tree, head_c, rng, roots, dirs, lengths, rows=6, lift=(0.0018, 0.0042))
        add('mustache sheet')
        for level, (f, lift, kind, width, reach) in enumerate(((0.75, (0.0028, 0.0055), 'dense', 0.020, 1.0),
                                                              (0.40, (0.0045, 0.0075), 'dense', 0.020, 1.1),
                                                              (0.05, (0.0065, 0.0100), 'clump', 0.019, 1.2))):
            stagger = 0.5 * (level % 2)
            for j in range(len(lines)):
                x = lines[j, 0] + stagger * 0.005
                t = np.interp(x, lines[:, 0], lines[:, 1])
                b = np.interp(x, lines[:, 0], lines[:, 2])
                y = np.interp(x, lines[:, 0], lines[:, 3])
                z = t - f * (t - b)
                r, _n = snap(np.array([sx * x, y - 0.01, z]), lift[0])
                k = min(x / 0.040, 1.0)
                L = (0.028 + 0.020 * k) * reach * rng.uniform(0.9, 1.1)
                d = gm.unit(np.array([sx * (0.35 + 1.5 * k), -0.55, -1.0 - 0.4 * k]) + rng.normal(size=3) * 0.05)
                _card(cards, tree, head_c, rng, r, d, L, width, kind, rows=5, lift=lift, out_bias=0.35, wob=0.02,
                      twist=6.0, cup=0.05)
                add(f'mustache {kind}')
        for _ in range(6):
            i = midx[rng.integers(len(midx))]
            r, _n = snap(W[i], 0.007)
            k = min(abs(W[i][0]) / 0.040, 1.0)
            d = gm.unit(np.array([sx * (0.5 + 1.6 * k), -0.5, -1.0]) + rng.normal(size=3) * 0.1)
            _card(cards, tree, head_c, rng, r, d, rng.uniform(0.03, 0.045), 0.012, 'wisp', rows=5,
                  lift=(0.007, 0.011), out_bias=0.35)
            add('mustache wisp')
        # ---- the corners: from the mustache's ends down past the mouth's corners into the chops
        cidx = np.nonzero((reg == 2) & (W[:, 0] * sx > 0.0))[0]
        for _ in range(5):
            i = cidx[rng.integers(len(cidx))]
            r, _n = snap(W[i], 0.004)
            d = gm.unit(down + 0.25 * side + 0.1 * fwd + rng.normal(size=3) * 0.08)
            _card(cards, tree, head_c, rng, r, d, rng.uniform(0.025, 0.035), 0.018, 'dense', rows=5,
                  lift=(0.004, 0.0075), out_bias=0.3)
            add('corners')
        M.log(f'whiskers {"left" if sx > 0 else "right"}: chops over {prof[0, 0]:.0f}..{prof[-1, 0]:.0f} degrees, '
              f'the mustache from {nose_z:.3f} m')
    ob = _object('Whiskers', cards, mat)
    M.log('whiskers: ' + ', '.join(f'{k} {v}' for k, v in counts.items()) + f'; {sum(len(f) - 2 for f in cards.faces)} '
          f'triangles')
    return ob


def brows(M, fig, skin, mat):
    """Bushy grey brows: two layers of short cards along each brow ridge from the inner end out, lying close, flicking
    up and out."""
    import looter_garments as gm
    tree = gm.bvh_objects([skin])
    rng = np.random.default_rng(43)
    head_c = fig.sk['head'][0] + np.array([0.0, 0.02, 0.02])
    cards = eh._Cards()
    hf = fig.hf
    n = 0
    for e in hf['eyes']:
        sx = 1.0 if e[0] > 0 else -1.0
        for layer, (count, lift, width) in enumerate(((8, (0.0008, 0.0022), 0.012), (6, (0.0022, 0.0040), 0.010))):
            for t in np.linspace(0.0, 1.0, count):
                # the portrait's brow line (MakeHuman decimetres, head space): over the eye, low at the inner end
                x = e[0] + sx * (-0.20 + 0.46 * t)
                y = e[1] + 0.17 + 0.05 * math.sin(t * math.pi * 0.8) - 0.03 * (1 - t) - 0.02 * layer
                p = fig.head_point(np.array([x, y, hf['front'] + 0.3]))
                q = fig.head_point(np.array([x, y, hf['front'] - 1.0]))
                hit = tree.ray_cast(Vector(tuple(p)), Vector(tuple(gm.unit(q - p))), float(np.linalg.norm(q - p)))
                if hit[0] is None:
                    continue
                root = np.array(hit[0]) + np.array(hit[1]) * lift[0]
                d = gm.unit(np.array([sx * (0.9 + 0.5 * t), -0.1, 0.25 + 0.55 * t]) + rng.normal(size=3) * 0.08)
                _card(cards, tree, head_c, rng, root, d, (0.015 + 0.005 * t) * rng.uniform(0.85, 1.15), width,
                      'dense' if layer == 0 else 'clump', rows=4, lift=lift, out_bias=0.15, wob=0.04, cup=0.05)
                n += 1
    ob = _object('Brows', cards, mat)
    M.log(f'brows: {n} cards, {sum(len(f) - 2 for f in cards.faces)} triangles')
    return ob


def round_angle(p, centre):
    rel = np.atleast_2d(p)[:, :2] - np.asarray(centre)[:2]
    return np.arctan2(rel[:, 0], -rel[:, 1])


# The hair under the bowler: ellis_hair.build's cards with Pike's layers: short (the nape above the neckerchief), round
# the back from behind one ear to the other (the chops take the temples), no lock over the brow.
BACK_HAIR = dict(
    seed=9,
    span=(98.0, 262.0),
    under=0.010,
    sheet=dict(cols=30, rows=5, lift=(0.0015, 0.0030), length=0.95),
    layers=(
        ('main', 22, 0.034, 0.0025, 0.0045, (0.92, 1.04), 'dense', 6, 2, 8.0, 0.04, (0.0, 0.08), 0.12),
        ('outer', 24, 0.026, 0.0030, 0.0065, (0.60, 0.95), 'clump', 6, 2, 12.0, 0.05, (0.0, 0.15), 0.18),
        ('wisp', 8, 0.014, 0.0035, 0.0075, (0.70, 1.0), 'wisp', 5, 1, 16.0, 0.06, (0.0, 0.2), 0.24),
    ),
    tips=dict(temple=0.0, back=0.040, under_rim=0.02, floor=0.090),
    brow=dict(cards=0, width=0.019, rows=6, degs=(6.0, 22.0)),
)


def build(M, fig, skin, under):
    """Every hair object: the back hair under the bowler, the whiskers, the brows."""
    mat = M.slot('Hair')
    hair = eh.build(fig, [skin] + list(under), 'Hair', mat, M.log, pivot=M.HAT.get('pivot'), **BACK_HAIR)
    return [hair, whiskers(M, fig, skin, mat), brows(M, fig, skin, mat)]
