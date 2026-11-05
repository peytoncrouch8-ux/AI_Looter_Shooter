"""Tripo models repainted in our looks zone by zone (the REBAKE lane, 2026-11-04; looter_rebake's method on Tripo's own
paint layout): each texel of a model's atlas goes to the zone its Tripo colour is nearest (a zone table per model,
MODELS), and takes that zone's look from our sets or our character looks, so where each colour lies stays Tripo's
and the paint itself becomes ours: crisp anti-aliased borders, edge wear and cavity grime from the shape, normals from
our sets. Helpers: looter_tripo_zones_look.

    rebake(obj, model, maps, faces=None, own=False, grade=None)
obj: the model's mesh (its active UV layer is the atlas maps use); maps: {'BC', 'RM', 'N'} images (read Tripo's paint
there, written back), or {'colour': (S, S, 3) linear array} for a colour-only atlas (the Unpaid's paint: returned
repainted, nothing written). faces: a bool per face (None: all); own: those faces have a UV square of their own in maps
(the bakes see a copy of only them). grade: write_maps' (saturation, contrast) to undo before the images are written.
Hooks: looter_tripo_barge_paint (the barge's walls), hook('Lamplighter') (SPECS['Lamplighter']['paint'] in
looter_tripo_wardens), parts_unpaid_paint (the Unpaid's paint). LOOTER_REBAKE unset: a table's 'on' (default True); =1 every
model, =0 none, =Name,Name only those.

The steps (numbers in DEFAULTS, a model's own in its 'params'):
1. Bakes at work px on the atlas (Cycles): which texels the faces cover, each texel's place and normal (model space,
   metres), pointiness, and which lie on faces whose UVs stretch over stretch.
2. Zones: Tripo's colour (smoothed zone_smooth px inside the faces) to the nearest of the table's prototypes in OKLab
   (chroma counted chroma x); 'keep' prototypes and whatever lies farther than keep_dist from every zone keep Tripo's
   texel (glass, lamps, glow). Stretched texels take the zone most good texels within vote_cell m of them have (no
   streaks). Each zone's share is rounded (zone_round) and sharpened (zone_edge): borders about a texel wide, no specks.
3. Looks: a 'set' zone samples one of our tiled sets in model space (tri-planar at its density, a mip coarser than the
   footprint asks), its light and dark at pattern, times the zone's colour (a shared instance's tint, a colour, or
   'tripo': the zone's own mean); a 'look' zone bakes one of looter_looks' character looks (Cycles, on the atlas; they
   carry their own wear and grime). Tripo's own light and dark inside a zone kept at value_keep (smoothed value_smooth px),
   and its fine painted detail (seams, buttons, a weave: its light and dark over detail_px) at the zone's 'detail'.
4. Wear on the set zones (rb.wear's rule), the colour's relief and the sets' normals in each texel's tangent frame.
5. Written into the maps at the faces' texels and a margin round their islands, undoing write_maps' grade, or returned.
A model's table: 'zones' [dict(name, protos (sRGB hex), set + tint/colour + hue, or look=(function, kwargs); chips: the
detail layer's chips show there (alpha='chips' writes that weight as the colour map's alpha mask); up=(lo, hi): only
texels whose normal's z lies there; faces=[(attribute, channel, lo, hi)]: only faces whose value lies there], 'keep' (prototypes
kept as Tripo painted them), 'params' (DEFAULTS overrides).
"""
import os
import time

import bpy
import numpy as np

import looter_rebake as rb
import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_zones_look as zl

DEFAULTS = dict(
    work=4096,              # px the texels are worked out at (write_maps shrinks a 4096 to the spec's 2048)
    out_px=2048,            # the map's written size (write_maps' unsharp radius is per it)
    margin=8,               # px round the faces' islands written too
    stretch=3.0,            # UV stretch (singular values' ratio) past which a face's zone is voted in 3D
    zone_smooth=1.2, chroma=2.0, keep_dist=0.16, zone_round=1.2, zone_edge=0.25, vote_cell=0.15,
    density=320.0, tile_m=3.2, pattern=0.5, mip_bias=1.0,
    value_smooth=10.0, value_keep=0.35, value_clamp=(0.55, 1.6),
    detail=0.0, detail_px=2.0,   # Tripo's fine light and dark kept (a zone's 'detail'), and over how many px it's measured
    match_value=False,           # each zone's mean brightness brought back to Tripo's there (a zone's 'match_value')
    edge=0.6, grime=0.35, set_normal=1.0, tripo_normal=0.4, relief=0.6,
    look_samples=4,         # Cycles samples a character look is baked with (they average its fine pattern in a texel)
)
UNSHARP = (1.2, 0.85)       # write_maps' unsharp mask: sigma 1.2 px per 1024 px of the written map, amount 0.85

# --- The zone tables ---
# The barge's house and casing walls (round 1, measured 2026-11-04: OKLab clusters of Tripo's wall paint, sRGB means and
# shares): the shared instances' sets and tints as BargeKit.py makes them (MI_<name>), Tripo's own slate and rusts.
BARGE = dict(
    zones=[
        dict(name='PaintCream', set='PaintWorn', tint=0xe6dac2, chips=True,         # cream a19a89 2.5%, dirty 867761 2.2%
             protos=(0xa19a89, 0x867761, 0xc8c0ac, 0xddd6c6)),
        dict(name='PaintOxide', set='PaintWorn', tint=0x8c3e2c, chips=True,         # oxide red 803b25 7.6%
             protos=(0x803b25, 0x9c4a32, 0xa84a2c, 0xb4583c)),
        dict(name='PaintSlate', set='PaintWorn', colour=0x4a4640, chips=True,       # the casing 4a4640 19.9%, 3a322a 19.6%
             protos=(0x4a4640, 0x3a322a, 0x5a5650)),
        dict(name='Rust', set='MetalRust', colour=0x56321e, hue=0.3,                # rust 56321e 15.2%, tan 6b5541 3.0%
             protos=(0x56321e, 0x6b5541, 0x6e3e22)),
        dict(name='RustDark', set='MetalRust', colour=0x2e2015, hue=0.3,            # the rims round the rust 2e2015 14.5%
             protos=(0x2e2015,)),
        dict(name='PaintBlack', set='PaintWorn', tint=0x2a2622, chips=True,         # bands and frames 191009 11.2%
             protos=(0x191009, 0x1e1610)),
    ],
    keep=(0x050201,),       # the window glass (4.3%); lamps and the green trim lie past keep_dist
    params={})
MODELS = {'DredgeBarge': BARGE}


def log(message):
    print(f'LOOTER: rebake: {message}', flush=True)


def enabled(model):
    """LOOTER_REBAKE unset: each model's table says ('on', default True); 1 every model; 0 none; Name,Name those."""
    value = os.environ.get('LOOTER_REBAKE', '').strip()
    if value == '':
        return table(model).get('on', True)
    return value == '1' or model in [v.strip() for v in value.split(',')]


def table(model):
    """MODELS[model] with its params filled from DEFAULTS (the tables live in looter_tripo_zones_tables)."""
    import looter_tripo_zones_tables as tables
    spec = dict(MODELS.get(model) or tables.MODELS[model])
    params = dict(DEFAULTS)
    params.update(spec.get('params', {}))
    spec['params'] = params
    return spec


# --- 2. Zones ---

def zones(spec, bc, core, stretched, pos, nrm, face_vals):
    """Each texel's zone weights ((texels, K + 1), rows of 1; the last: Tripo's paint kept) and the dominant zone as an
    int8 image (-1 off the faces)."""
    p, table_ = spec['params'], spec['zones']
    keep = len(table_)
    idx = np.nonzero(core)
    lab = zl.oklab(zl.masked_blur(bc, core, p['zone_smooth'])[idx])
    w = np.array([1.0, p['chroma'], p['chroma']], np.float32)
    best = np.full(len(lab), np.inf, np.float32)
    zc = np.full(len(lab), keep, np.int8)
    nz = nrm[idx][:, 2] / np.maximum(np.linalg.norm(nrm[idx], axis=1), 1e-9)
    for k, z in enumerate(table_):
        off = ~((nz > z['up'][0]) & (nz < z['up'][1])) if 'up' in z else None
        for attr, ch, lo, hi in z.get('faces', ()):
            v = face_vals[(attr, ch)]
            out = ~((v > lo) & (v < hi))
            off = out if off is None else (off | out)
        for proto in z['protos']:
            d = np.sqrt((((lab - zl.oklab(zl.hex_rgb(proto))) * w) ** 2).sum(-1))
            if off is not None:
                d[off] = np.inf                  # the zone's faces only (a deck's up faces, a garment's)
            better = d < best
            best[better] = d[better]
            zc[better] = k
    zc[best > p['keep_dist']] = keep
    for proto in spec.get('keep', ()):
        zc[np.sqrt((((lab - zl.oklab(zl.hex_rgb(proto))) * w) ** 2).sum(-1)) < best] = keep
    del lab, best
    good, bad = ~stretched[idx], stretched[idx]
    if bad.any() and good.any():
        zc = zl.vote_3d(zc, pos[idx], good, bad, keep + 1, p['vote_cell'])
        log(f'{bad.sum()} stretched texels voted in 3D')
    W = np.empty((len(zc), keep + 1), np.float32)
    one = np.zeros(core.shape, np.float32)
    for k in range(keep + 1):
        one[idx] = zc == k
        W[:, k] = zl.masked_blur(one, core, p['zone_round'])[idx]
    W = np.exp((W - W.max(1, keepdims=True)) / p['zone_edge'])
    W /= W.sum(1, keepdims=True)
    W[W < 0.02] = 0.0
    W /= W.sum(1, keepdims=True)
    zone = np.full(core.shape, -1, np.int8)
    zone[idx] = W.argmax(1)
    return zone, W


# --- 3. Looks ---

def looks(spec, bake, W, zone, idx, frame, bc_t, orm_t, n_t, want_maps):
    """Colour (linear), ORM and tangent-space normal per texel, each zone's look weighted by W; the kept zone Tripo's."""
    p, table_ = spec['params'], spec['zones']
    keep = len(table_)
    P, N, T, B, foot = frame
    n = len(P)
    size = zone.shape[0]
    col, orm, nts = np.zeros((n, 3), np.float32), np.zeros((n, 3), np.float32), np.zeros((n, 3), np.float32)
    tc = zl.lin(bc_t[idx])
    to = orm_t[idx][:, :3].copy() if orm_t is not None else np.tile(np.array([[1.0, 0.6, 0.0]], np.float32), (n, 1))
    to[:, 0] = to[:, 0] * 0.85 + 0.15            # Tripo's occlusion as write_maps lifts it
    tn = (n_t[idx][:, :3] * 2.0 - 1.0) if n_t is not None else np.tile(np.array([[0.0, 0.0, 1.0]], np.float32), (n, 1))
    luma_t = zl.lin(bc_t) @ zl.LUMA
    sets = {}
    worn = np.zeros(n, np.float32)               # how much of a texel takes the set zones' wear
    for k in range(keep + 1):
        sel = W[:, k] > 0.0
        if not sel.any():
            continue
        wk = W[sel, k][:, None]
        if k == keep:
            col[sel] += wk * tc[sel]
            orm[sel] += wk * to[sel]
            nts[sel] += wk * tn[sel]
            continue
        z = table_[k]
        m = (zone == k).astype(np.float32)
        den = lt.blur(m, p['value_smooth'])[idx][sel]
        num = lt.blur(luma_t * m, p['value_smooth'])[idx][sel]
        v = np.where(den > 0.05, num / np.maximum(den, 1e-6), np.nan)
        med = float(np.nanmedian(v)) if np.isfinite(v).any() else 1.0
        value = np.clip(np.nan_to_num(v / max(med, 1e-4), nan=1.0), *p['value_clamp']) ** p['value_keep']
        keep_detail = z.get('detail', p['detail'])
        if keep_detail > 0.0:
            # Tripo's painted detail (seams, buttons, a weave, a rope's twist, plank streaks): its fine light and dark
            # over its own neighbourhood in the zone (detail_px), kept at keep_detail on top of our look.
            fd = lt.blur(m, p['detail_px'])[idx][sel]
            fn_ = lt.blur(luma_t * m, p['detail_px'])[idx][sel]
            fine = np.where(fd > 0.05, fn_ / np.maximum(fd, 1e-6), luma_t[idx][sel])
            value = value * np.clip(luma_t[idx][sel] / np.maximum(fine, 1e-4), 0.4, 2.5) ** keep_detail
        colour = z.get('colour', 'tripo' if 'look' in z else None)
        if colour == 'tripo':
            mine = W[:, k] > 0.5
            colour = zl.srgb(tc[mine].mean(0)) if mine.any() else zl.hex_rgb(z['protos'][0])
        if 'look' in z:
            fn, kwargs = z['look']
            kwargs = dict(kwargs)
            for key, val in kwargs.items():
                if isinstance(val, str) and val.startswith('tripo'):
                    # 'tripo': the zone's own mean colour (sRGB); 'tripo*0.6' that times 0.6 (a look's dark end)
                    f = float(val.split('*')[1]) if '*' in val else 1.0
                    kwargs[key] = tuple(float(min(1.0, c * f)) for c in colour)
            maps_k = bake_look(bake, fn, kwargs, size, p['look_samples'], want_maps)
            lc = maps_k['COLOR'][idx][sel]
            # The look's own light and dark over its mean in the zone, at the zone's pattern (1: the look as baked), on
            # the zone's colour: a look adds its grain and wear without overruling Tripo's paint.
            mine_k = W[sel, k] > 0.5
            lmean = lc[mine_k].mean(0) if mine_k.any() else lc.mean(0)
            ratio = lc / np.maximum(lmean, 1e-4)
            luma = (ratio @ zl.LUMA)[:, None]
            ratio = 1.0 + (luma + (ratio - luma) * z.get('hue', 1.0) - 1.0) * z.get('pattern', 1.0)
            col[sel] += wk * zl.lin(np.asarray(colour, np.float32))[None] * ratio * value[:, None]
            if want_maps:
                o = np.stack([to[sel, 0], maps_k['ROUGH'][idx][sel], maps_k['METAL'][idx][sel]], 1)
                orm[sel] += wk * o
                nk = maps_k['NORMAL'][idx][sel] * 2.0 - 1.0
                nk[:, :2] += tn[sel, :2] * p['tripo_normal']
                nts[sel] += wk * nk
            del maps_k
            continue
        s = sets.get(z['set']) or sets.setdefault(z['set'], zl.Set(z['set']))
        density = z.get('density') or lt.SETS.get(z['set'], {}).get('density') or p['density']
        fm = foot[sel] * 2.0 ** p['mip_bias']
        ratio = zl.triplanar(s, 'BC', P[sel], N[sel], fm, density, z.get('axes')) / np.maximum(s.mean, 1e-4)
        luma = (ratio @ zl.LUMA)[:, None]
        ratio = 1.0 + (luma + (ratio - luma) * z.get('hue', 1.0) - 1.0) * z.get('pattern', p['pattern'])
        if 'tint' in z:
            base = s.mean * zl.lin(zl.hex_rgb(z['tint']))
        elif isinstance(colour, (int, np.integer)):
            base = zl.lin(zl.hex_rgb(colour))
        else:
            base = zl.lin(np.asarray(colour, np.float32))
        if 'saturate' in z:
            # A mean over Tripo's streaked paint comes out greyer than its paint reads: give its colour back.
            base = float(base @ zl.LUMA) + (base - float(base @ zl.LUMA)) * z['saturate']
        col[sel] += wk * np.clip(base, 0.0, 1.0)[None] * ratio * value[:, None]
        o = zl.triplanar(s, 'ORM', P[sel], N[sel], fm, density, z.get('axes'))
        o[:, 0] *= to[sel, 0]                    # Tripo's occlusion (its shape's) under the set's own
        orm[sel] += wk * o
        no = zl.triplanar(s, 'N', P[sel], N[sel], fm, density, z.get('axes'))
        ts = np.stack([(no * T[sel]).sum(1), (no * B[sel]).sum(1), (no * N[sel]).sum(1)], 1)
        ts[:, :2] = ts[:, :2] * p['set_normal'] + tn[sel, :2] * p['tripo_normal']
        nts[sel] += wk * ts
        worn[sel] += wk[:, 0]
    return col, orm, nts, worn


# --- A character look baked on the atlas ---

_WHITE = '_looter_white'


def bake_look(obj, fn, kwargs, size, samples, want_maps):
    """One of looter_looks' character looks (fn its function's name, kwargs its colours) baked on obj's atlas: COLOR
    (linear), and with want_maps ROUGH, METAL (floats) and NORMAL (tangent space, OpenGL, 0..1); rows top first. The
    looks read object space in metres (one pattern across islands), pointiness (their wear and grime) and a colour
    attribute or two (a white one stands in)."""
    import looter_looks as ll
    me = obj.data
    if _WHITE not in me.color_attributes:
        white = me.color_attributes.new(_WHITE, 'FLOAT_COLOR', 'CORNER')
        white.data.foreach_set('color', np.ones(len(me.loops) * 4, np.float32))
    kw = dict(kwargs)
    if fn == 'skin':
        kw.setdefault('zone_attr', _WHITE)
    mat = getattr(ll, fn)('_look_' + fn, **kw)
    kept = [s.material for s in obj.material_slots]
    if not kept:
        me.materials.append(mat)
    for s in obj.material_slots:
        s.material = mat
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    nodes = mat.node_tree.nodes
    out_node = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
    surface = out_node.inputs['Surface'].links[0].from_socket if out_node.inputs['Surface'].links else None
    tex = nodes.new('ShaderNodeTexImage')
    nodes.active = tex
    result = {}
    passes = [('COLOR', 'OUT_COLOR'), ('ROUGH', 'OUT_ROUGH'), ('METAL', 'OUT_METAL'), ('NORMAL', None)] if want_maps \
        else [('COLOR', 'OUT_COLOR')]
    for key, reroute in passes:
        img = rb._image('_look_' + key, size)
        rb._fill(img, (0.0, 0.0, 0.0, 0.0) if key != 'NORMAL' else (0.5, 0.5, 1.0, 1.0))
        tex.image = img
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        if reroute is None:
            if surface is not None:
                mat.node_tree.links.new(surface, out_node.inputs['Surface'])
            sc.cycles.samples = 1
            bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', margin=4, margin_type='EXTEND', use_clear=False,
                                target='IMAGE_TEXTURES')
        else:
            em = nodes.get('_look_emit') or nodes.new('ShaderNodeEmission')
            em.name = '_look_emit'
            for ln in list(em.inputs['Color'].links):
                mat.node_tree.links.remove(ln)
            mat.node_tree.links.new(nodes[reroute].outputs[0], em.inputs['Color'])
            mat.node_tree.links.new(em.outputs[0], out_node.inputs['Surface'])
            sc.cycles.samples = samples
            bpy.ops.object.bake(type='EMIT', margin=4, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
        a = rb._read(img)
        result[key] = a[..., :3] if key in ('COLOR', 'NORMAL') else a[..., 0]
        bpy.data.images.remove(img)
    if kept:
        for s, m in zip(obj.material_slots, kept):
            s.material = m
    else:
        me.materials.clear()
    bpy.data.materials.remove(mat)
    return result


# --- The pipeline ---

def rebake(obj, model, maps, faces=None, own=False, grade=None, report=None, alpha=None):
    """See the module's docstring. Returns the new colour array (array mode) or None; LAST holds the zone map."""
    if not enabled(model):
        log(f'{model}: skipped (LOOTER_REBAKE)')
        return maps.get('colour')
    spec = table(model)
    p = spec['params']
    t_start = time.time()
    keep = len(spec['zones'])
    picked_all = np.ones(len(obj.data.polygons), bool) if faces is None else np.asarray(faces, bool)
    if not picked_all.any():
        log(f'{model}: no faces to paint')
        return maps.get('colour')
    bake = zl.faces_only(obj, picked_all) if own else obj
    picked = np.ones(len(bake.data.polygons), bool) if own else picked_all
    array = 'colour' in maps
    size = p['work'] if not array else maps['colour'].shape[0] * max(1, p['work'] // maps['colour'].shape[0])
    t = time.time()
    flags = np.zeros((len(picked), 3), np.float32)
    flags[picked, 0] = 1.0
    flags[picked & (zl.stretch(bake.data) > p['stretch']), 1] = 1.0
    raw = zl.bake_faces(bake, flags, size, 0)
    core, stretched = raw[..., 0] > 0.5, raw[..., 1] > 0.5
    del raw
    cover = zl.bake_faces(bake, np.ones((len(picked), 3), np.float32), size, 0)[..., 0] > 0.5
    pos, nrm, point = zl.bake_own(bake, size)
    log(f'{model}: bakes at {size} px {time.time() - t:.0f} s; the faces cover {core.mean() * 100:.2f}% '
        f'({stretched.sum() / max(core.sum(), 1) * 100:.1f}% of them stretched)')
    t = time.time()
    if array:
        bc_t = zl.srgb(zl.upsample(maps['colour'], size))
        orm_t = n_t = None
    else:
        bc_t = np.ascontiguousarray(tripo.pixels(maps['BC'], size)[..., :3], dtype=np.float32)
        orm_t = tripo.pixels(maps['RM'], size) if maps.get('RM') is not None else None
        n_t = tripo.pixels(maps['N'], size) if maps.get('N') is not None else None
    face_vals = {}
    for z in spec['zones']:
        for attr, ch, _lo, _hi in z.get('faces', ()):
            if (attr, ch) not in face_vals:
                vals = zl.face_values(bake, attr, ch)
                img = zl.bake_faces(bake, np.repeat(vals[:, None], 3, 1), size, 0)[..., 0]
                face_vals[(attr, ch)] = img[np.nonzero(core)]
    zone, W = zones(spec, bc_t, core, stretched, pos, nrm, face_vals)
    share = W.sum(0) / max(len(W), 1) * 100.0
    log(f'{model}: zones ' + ', '.join(f'{z["name"]} {s:.1f}%' for z, s in zip(spec['zones'], share)) +
        f', Tripo kept {share[keep]:.1f}% ({time.time() - t:.0f} s)')
    idx = np.nonzero(core)
    t = time.time()
    frame = zl.frames(pos, nrm, core, idx)
    typical = frame[5]
    del pos, nrm
    px_m = 1.0 / typical * p['out_px'] / size
    log(f'{model}: {len(frame[0])} texels, footprint median {typical * 100:.2f} cm ({px_m:.0f} px/m at {p["out_px"]})')
    col, orm, nts, worn = looks(spec, bake, W, zone, idx, frame[:5], bc_t, orm_t, n_t, not array)
    del orm_t, n_t, frame
    col, orm = zl.wear(col, orm, worn, lt.blur(point, 1.5)[idx], p['edge'], p['grime'])
    del point
    # Each zone's brightness against Tripo's there (logged); match_value brings a zone's mean back to Tripo's, so our
    # looks add their texture without moving the model's light and dark (a shared tint is meant to move it: off then).
    tc = zl.lin(bc_t[idx])
    del bc_t
    shifts = []
    for k, z in enumerate(spec['zones']):
        mine = W[:, k] > 0.5
        if not mine.any():
            continue
        ratio = float((tc[mine] @ zl.LUMA).mean() / max((col[mine] @ zl.LUMA).mean(), 1e-5))
        if z.get('match_value', p['match_value']):
            sel = W[:, k] > 0.0
            col[sel] *= (1.0 + (np.clip(ratio, 0.6, 1.8) - 1.0) * W[sel, k])[:, None]
        shifts.append(f'{z["name"]} {1.0 / ratio:.2f}')
    del tc
    log(f"{model}: zone brightness over Tripo's (before any match): " + ', '.join(shifts))
    if own:
        mesh = bake.data
        bpy.data.objects.remove(bake)
        bpy.data.meshes.remove(mesh)
    ours = W[:, keep] < 0.99
    chips = W[:, [k for k, z in enumerate(spec['zones']) if z.get('chips')]].sum(1)
    LAST.update(zone=zone, core=core, own=bool(own), model=model)
    log(f'{model}: looks, wear {time.time() - t:.0f} s')
    t = time.time()
    if array:
        out = maps['colour'].copy()
        mine = np.zeros(core.shape + (3,), np.float32)
        weight = np.zeros(core.shape, np.float32)
        mine[idx] = col
        weight[idx] = 1.0 - W[:, keep]
        f = size // out.shape[0]
        if f > 1:
            mine = mine.reshape(out.shape[0], f, out.shape[1], f, 3).mean((1, 3))
            weight = weight.reshape(out.shape[0], f, out.shape[1], f).mean((1, 3))
            cov = core.reshape(out.shape[0], f, out.shape[1], f).mean((1, 3))
            mine = np.where(cov[..., None] > 0, mine / np.maximum(cov, 1e-6)[..., None], 0.0)
            weight = np.where(cov > 0, weight / np.maximum(cov, 1e-6), 0.0)
        out = out * (1.0 - weight[..., None]) + mine * weight[..., None]
        log(f'{model}: repainted ({time.time() - t:.0f} s); all {time.time() - t_start:.0f} s')
        if report is not None:
            report['rebake'] = {'zones %': [round(float(v), 1) for v in share], 'px/m': round(px_m), 's': round(time.time() - t_start)}
        return out
    nts = zl.relief(nts, col, 1.0 - W[:, keep], core, idx, p['relief'])
    free = ~cover
    seed = np.full((size, size), -1, np.int64)
    seed[idx[0][ours], idx[1][ours]] = np.nonzero(ours)[0]
    src = zl.grow(seed, free, p['margin'])
    widx = np.nonzero(src >= 0)
    s_of = src[widx]
    if grade is not None:
        zl.write_bc(maps['BC'], widx, col[s_of], size, grade, UNSHARP)
    else:
        zl.write_raw(maps['BC'], widx, zl.srgb(col[s_of]), size)
    if maps.get('RM') is not None:
        o = orm[s_of].copy()
        o[:, 0] = np.clip((o[:, 0] - 0.15) / 0.85, 0.0, 1.0)       # write_maps lifts occlusion: x * 0.85 + 0.15
        zl.write_raw(maps['RM'], widx, o, size)
    if maps.get('N') is not None:
        zl.write_raw(maps['N'], widx, nts[s_of] * 0.5 + 0.5, size)
    if alpha == 'chips':
        seed[:] = -1
        seed[idx] = np.arange(len(W))
        src = zl.grow(seed, free, p['margin'])
        aidx = np.nonzero(src >= 0)
        zl.attach_alpha(maps['BC'], aidx, chips[src[aidx]], size)
    if report is not None:
        report['rebake'] = {'faces': int(picked.sum()), 'texels': int(len(W)), 'written': int(len(s_of)),
                            'zones %': [round(float(v), 1) for v in share], 'px/m': round(px_m, 1),
                            'chips %': round(float(chips.mean()) * 100.0, 1), 's': round(time.time() - t_start)}
    log(f'{model}: written {len(s_of)} texels ({time.time() - t:.0f} s); all {time.time() - t_start:.0f} s')
    return None


LAST = {}                   # the last rebake's zone map, for the previews (zone_preview)


def hook(model, kind='ship', faces=None):
    """A looter_tripo_wardens paint hook (obj, report, maps) for a whole Tripo model (or faces(obj) of it)."""
    def paint(obj, report, maps):
        import looter_tripo_wardens as tw
        rebake(obj, model, maps, faces=faces(obj) if faces else None, grade=tw.GRADE[kind], report=report)
    return paint
