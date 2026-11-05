"""Repacks a fitted Tripo model's UV atlas so its islands fill more of the square, then re-cuts its maps from the GLB's
own 4096 delivery onto the new layout (the art audit's plan row 5; numbers, what each model gained and why most gained
little: Docs/Work/2026-11-04-Art/REPACK.md). Tripo's own atlases are already tightly packed (36-66% of the square under
triangles: irregular islands and their gaps); the space to win is where the game mesh dropped faces from the atlas
(cuts, faces moved to other slots), and the islands' uneven density.

repack(obj, maps, name, size, polys) -> maps. Hooks: looter_tripo_wardens.build's `repack=True` spec flag (after the
rework, before any paint and write_maps); looter_tripo_m23/_m47's `repack_after=True` (after_fit(): after their fit,
the faces left on MI_Tripo<name>, the set written again); looter_tripo_buildings.swap(repack=True) (building(): after
the retexture, the slot-0 faces, write_set and the grade again). own_set(): faces moved onto a texture set of their own
(the build's `sets` spec key: the barge's walls).
1. pack_uv(): Tripo's islands (its seams kept) placed again by looter_tripo_repack_pack.pack, a raster packer: turned,
   scaled to one texel density by their 3D area but never below the density they had (the floor), PAD px each at the
   map's size (the gap 2 x PAD: the 2026-11-04 check measured no more mip bleed than Tripo's own layouts). It must hold
   MIN_GAIN times Tripo's UV area and no less on the big surfaces, or Tripo's layout stays (and the maps as they were).
   Cached in Intermediate/TripoRepack by a hash of what it packs.
2. transfer(): an exact UV-to-UV copy, not a ray cast: the mesh is the same, so each new texel lies in one triangle at
   known barycentrics; its place in the old layout is the same barycentrics on the old UVs, where the 4096 source is
   box-filtered over the texel's footprint (k x k point taps, k = 2 x the source px it spans; on Tripo's own layout it
   gives back the old 2:1 shrink within 0.5/255). Colour and ORM copy as they are; the tangent-space normal is decoded
   in the old UVs' MikkTSpace frame and encoded in the new one's (frames from a scratch mesh of the loop triangles, so
   n-gons are fine; Unreal runs MikkTSpace on the triangulated mesh), so a turned island lights the same. A source whose
   padding is dark round its islands is first spread DILATE px (Tripo's own already are, 1-6 px out).
3. pad(): each island's border spread PAD px outward (its own texels), the rest of the square filled smooth (push-pull).
4. obj keeps its UV layer, rewritten; the returned maps are images at size px named like the source's (an 'ORM...' map
   keeps its occlusion), ready for write_maps / write_set as the GLB's were.

Switch: LOOTER_TRIPO_REPACK unset or 1 = on wherever a spec or call asks; 0 = off (Tripo's layout, for before/after);
Name,Name = only those. Checks (survey, packers, before/after renders): Tools/Blender/tripo_repack_check.py.
"""
import math
import os
import time

import bpy
import numpy as np

import looter_tripo_repack_pack as rpk
from looter_tripo_repack_bake import _image, _pixels_u8, _ring, bleed, pad, transfer   # noqa: F401
from looter_tripo_repack_pack import (_tri_area2, _tri_area3, _uv_layer, loop_positions,   # noqa: F401
                                      loop_triangles, loop_uvs, set_loop_uvs)

ENV = 'LOOTER_TRIPO_REPACK'
SOURCE = 4096          # px: every Tripo delivery's maps (2026-11-04 survey: 64 GLBs, all 4096)
PAD = {4096: 8, 2048: 4, 1024: 2, 512: 1}    # px each island owns at the map's size (half the gap): the 2026-11-04
                                             # check, no more mip bleed than Tripo's own layouts (REPACK.md)
OWN_PAD = {2048: 2, 1024: 1}                 # an own set's (the barge's walls): 2508 m2 of walls fill 0.668 of a 2K square at
                                             # 2 px (33.4 px/m, mip-3 bleed 3.9%), 0.60 at 4 px (31.7) (2026-11-04)
PACK_VERSION = 3       # bump when the packer changes (the cache's key)
CACHE = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'Intermediate',
                     'TripoRepack')


def log(message):
    print(f'LOOTER: repack: {message}', flush=True)


def enabled(name):
    value = os.environ.get(ENV, '1').strip()
    if value in ('', '1'):
        return True
    if value == '0':
        return False
    return name in [v.strip() for v in value.split(',')]


# --- Measuring (the audit's metric) ---

def lost(obj, old, new, polys):
    """How the texel density moved, new layout against old (linear, per face, area-weighted): (p10, p50, the share of
    the surface that lost more than 2%)."""
    me = obj.data
    L, P = loop_triangles(me)
    sel = np.isin(P, polys)
    L = L[sel]
    a3 = _tri_area3(loop_positions(me)[L])
    ao, an = _tri_area2(old[L]), _tri_area2(new[L])
    ok = (ao > 1e-14) & (a3 > 1e-14)
    r, w = np.sqrt(an[ok] / ao[ok]), a3[ok]
    o = np.argsort(r)
    c = np.cumsum(w[o]) / w.sum()
    at = lambda q: round(float(r[o][min(len(r) - 1, np.searchsorted(c, q))]), 2)
    return at(0.1), at(0.5), round(float(w[r < 0.98].sum() / w.sum()), 3)


def measure(obj, size, layer=None, polys=None, raster_size=1024, uv=None):
    """The atlas as the game samples it: use (the triangles' summed UV area: the audit's share of the square), cover
    (texels covered once or more at raster_size; overlap: covered twice), islands, texels/cm (the audit's: size x
    sqrt(UV area / surface cm2)), big texels/cm (area-weighted median over the faces of the biggest islands making up
    half the surface: decks, walls, the robe), spread (area-weighted std of log2 texel density: 0 = even)."""
    me = obj.data
    L, P = loop_triangles(me)
    if polys is not None:
        sel = np.isin(P, polys)
        L, P = L[sel], P[sel]
    uv = loop_uvs(me, layer) if uv is None else uv
    U = uv[L]
    X = loop_positions(me)[L] @ np.asarray(obj.matrix_world.to_3x3()).T
    a2 = _tri_area2(U)
    a3 = _tri_area3(X) * 1e4    # cm2
    use = float(a2.sum())
    texels = size * math.sqrt(use / max(a3.sum(), 1e-9))
    isl = rpk.islands(me, uv, None if polys is None else polys)
    ti = isl[P]
    isl_area = np.bincount(ti, weights=a3)
    order = np.argsort(-isl_area)
    cum = np.cumsum(isl_area[order]) / isl_area.sum()
    big = set(order[:int(np.searchsorted(cum, 0.5)) + 1].tolist())
    dens = size * np.sqrt(a2 / np.maximum(a3, 1e-12))
    good = (a3 > 1e-6) & (a2 > 0)

    def wmedian(v, w):
        o = np.argsort(v)
        c = np.cumsum(w[o])
        return float(v[o][np.searchsorted(c, c[-1] * 0.5)]) if len(v) else 0.0
    bigmask = np.isin(ti, list(big)) & good
    lg = np.log2(np.maximum(dens[good], 1e-9))
    wmean = np.average(lg, weights=a3[good])
    spread = float(np.sqrt(np.average((lg - wmean) ** 2, weights=a3[good])))
    cov, over = rpk.coverage(U, raster_size)
    return dict(use=round(use, 3), cover=round(float(cov.mean()), 3), overlap=round(over / raster_size ** 2, 4),
                islands=int(isl.max() + 1) if len(isl) else 0, big_islands=len(big),
                texels_cm=round(texels, 3), big_texels_cm=round(wmedian(dens[bigmask], a3[bigmask]), 3),
                spread=round(spread, 2), area_m2=round(float(a3.sum()) / 1e4, 2), tris=int(len(L)))


# --- 1. Pack ---

def pack_uv(obj, uv, polys, size, pad=None, min_gain=1.0, start_use=None, floor=1.0, log_steps=False):
    """New per-loop UVs: polys' islands re-placed by looter_tripo_repack_pack.pack (Tripo's shapes and seams kept,
    turned, scaled to one density, packed), the other loops as they are; (None, scale) when min_gain times Tripo's UV
    area doesn't fit. start_use: the search starts at that share of the square instead (an own set: its islands came
    from a corner of Tripo's square). Cached in Intermediate/TripoRepack by a hash of what it packs (the packer is
    deterministic: the cache only saves its minutes on the next export)."""
    import hashlib
    me = obj.data
    L, P = loop_triangles(me)
    sel = np.isin(P, polys)
    L, P = L[sel], P[sel]
    isl = rpk.islands(me, uv, polys)[P]
    X = loop_positions(me)[L] @ np.asarray(obj.matrix_world.to_3x3()).T
    a3 = _tri_area3(X)
    pad = PAD.get(size, max(1, size // 512)) if pad is None else pad
    key = hashlib.sha1()
    for part in (np.round(uv[L], 6), np.round(a3, 9), isl,
                 np.array([size, pad, min_gain, start_use or 0.0, floor, PACK_VERSION], float)):
        key.update(np.ascontiguousarray(part).tobytes())
    cache = os.path.join(CACHE, key.hexdigest() + '.npz')
    out = uv.copy()
    if os.path.exists(cache):
        data = np.load(cache)
        if bool(data['fits']):
            out[data['loops']] = data['uv']
            return out, float(data['scale'])
        return None, float(data['scale'])
    n = int(isl.max()) + 1 if len(isl) else 0
    order = np.argsort(isl, kind='stable')
    bounds = np.searchsorted(isl[order], np.arange(n + 1))
    members = [order[bounds[k]:bounds[k + 1]] for k in range(n)]
    groups = [(uv[L[m]], float(a3[m].sum())) for m in members]
    use = start_use if start_use else float(_tri_area2(uv[L]).sum())
    start = size * math.sqrt(use / max(float(a3.sum()), 1e-12))
    placed, scale = rpk.pack(groups, size, pad, start=start, min_gain=min_gain, floor=floor,
                             log=log if log_steps else None)
    os.makedirs(CACHE, exist_ok=True)
    if placed is None:
        np.savez(cache, fits=False, scale=scale, loops=np.zeros(0, np.int64), uv=np.zeros((0, 2)))
        return None, scale
    for m, tris in zip(members, placed):
        out[L[m].ravel()] = tris.reshape(-1, 2)
    loops = np.unique(L.ravel())
    np.savez(cache, fits=True, scale=scale, loops=loops, uv=out[loops])
    return out, scale


# --- The step ---

RESULTS = {}           # name -> the last repack's numbers (before, after, mip bleed): tripo_repack_check reads them
MIN_GAIN = 1.05        # the new layout must hold this many times Tripo's UV area (and no less on the big surfaces)


def layout_bleed(obj, uv, polys, size, steps=None):
    """bleed() of a layout at size px (Tripo's own as the game's map draws it, or the repacked one)."""
    me = obj.data
    L, P = loop_triangles(me)
    sel = np.isin(P, polys)
    isl = rpk.islands(me, uv, polys)[P[sel]]
    t, r, c, _ = rpk.raster(uv[L[sel]], size)
    ids = np.full((size, size), -1, np.int64)
    ids[r, c] = isl[t]
    return bleed(ids, ids >= 0, PAD.get(size, max(1, size // 512)) if steps is None else steps)


def repack(obj, maps, name, size=2048, polys=None, layer=None, report=None):
    """obj's islands (polys: polygon indices, default all; the others keep their UVs and must not use this atlas)
    repacked and the GLB's maps (load()'s, 4096) re-cut onto them at size px. obj keeps its UV layer, rewritten.
    Returns the new maps, {'BC', 'RM', 'N'} images, or maps unchanged when the switch is off or the new layout does
    not beat Tripo's (MIN_GAIN)."""
    if not enabled(name):
        log(f"{name}: off ({ENV}), Tripo's layout kept")
        return maps
    t0 = time.time()
    me = obj.data
    old = loop_uvs(me, layer)
    pid = np.arange(len(me.polygons)) if polys is None else np.asarray(sorted(set(int(p) for p in polys)), np.int64)
    before = measure(obj, size, layer, pid, uv=old)
    new, scale = pack_uv(obj, old, pid, size, min_gain=MIN_GAIN)
    after = measure(obj, size, layer, pid, uv=new) if new is not None else dict(before, use=0.0, big_texels_cm=0.0)
    steps = PAD.get(size, max(1, size // 512))
    mip_old = layout_bleed(obj, old, pid, size, steps)
    entry = dict(before=before, after=after, mip_old=mip_old, size=size)
    RESULTS[name] = entry
    if new is None or after['use'] < before['use'] * MIN_GAIN or after['big_texels_cm'] < before['big_texels_cm']:
        log(f"{name}: the repack does not beat Tripo's layout (use {before['use']} -> {after['use']}, big texels/cm "
            f'{before["big_texels_cm"]} -> {after["big_texels_cm"]}): kept as it was')
        entry['kept'] = True
        if report is not None:
            report['repack'] = f"kept Tripo's layout (use {before['use']} -> {after['use']})"
        return maps
    entry['density'] = lost(obj, old, new, pid)
    data, known, tri, info = transfer(obj, old, new, maps, size, pid, layer)
    L, P = loop_triangles(me)
    isl = rpk.islands(me, old, pid)
    sel = np.isin(P, pid)
    tri_isl = np.where(tri >= 0, isl[P[sel]][np.maximum(tri, 0)], -1)
    mip = bleed(tri_isl, known, steps)
    out = {}
    for key, arr in data.items():
        filled = pad(arr, known, steps)
        if key == 'N':
            v = filled / 127.5 - 1.0
            v /= np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-6)
            filled = (v * 0.5 + 0.5) * 255.0
        src = maps[key]
        prefix = 'ORM' if (key == 'RM' and src.name.upper().startswith('ORM')) else key
        out[key] = _image(f'{prefix}_Repack_{name}', filled, key == 'BC')
    for key in ('BC', 'RM', 'N'):
        out.setdefault(key, maps.get(key))
    set_loop_uvs(me, new, layer)
    secs = round(time.time() - t0, 1)
    gain = after['texels_cm'] / max(before['texels_cm'], 1e-9)
    log(f'{name}: {size} px, use {before["use"]} -> {after["use"]}, texels/cm {before["texels_cm"]} -> '
        f'{after["texels_cm"]} (x{gain:.2f}), big {before["big_texels_cm"]} -> {after["big_texels_cm"]}, spread '
        f'{before["spread"]} -> {after["spread"]}, {before["islands"]} islands, taps {info["taps"]}, '
        f'source px per texel {info["scale"]}, density p10 / p50 / share that lost {entry["density"]}, '
        f'mip bleed {mip_old} -> {mip}, {secs} s')
    entry.update(mip=mip, taps=info['taps'], scale=info['scale'], secs=secs)
    if report is not None:
        report['repack'] = f'use {before["use"]} -> {after["use"]}, texels/cm x{gain:.2f}'
    return out


def _source(path):
    """The GLB's own maps (looter_tripo.load's), its mesh dropped."""
    import looter_tripo as tripo
    obj, maps = tripo.load(path)
    me = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(me)
    return maps


def _slot_polys(obj, material_name):
    names = [m.name if m else '' for m in obj.data.materials]
    if material_name not in names:
        return np.zeros(0, np.int64)
    slot = names.index(material_name)
    idx = np.empty(len(obj.data.polygons), np.int64)
    obj.data.polygons.foreach_get('material_index', idx)
    return np.nonzero(idx == slot)[0]


def after_fit(obj, name, spec, report=None):
    """For models whose fit runs after looter_tripo_wardens.build (looter_tripo_m23, _m47: cuts, other slots, colour
    picks on Tripo's UVs): the faces still on MI_Tripo<name> repacked, the set written again by write_maps."""
    import looter_tripo_wardens as tw
    if not enabled(name):
        log(f"{name}: off ({ENV}), Tripo's layout kept")
        return
    polys = _slot_polys(obj, 'Tripo' + name)
    maps = _source(os.path.join(tw.TRIPO_DIR, name, name + '.glb'))
    new = repack(obj, maps, name, spec['maps'], polys=polys, report=report)
    if new is not maps:
        tw.write_maps(new, name, spec['master'], spec['maps'], spec['kind'], tint=spec.get('tint'))
    _free(maps, new)
    _free({}, maps)


def building(shell, maps, name, set_name, path, regrade=None):
    """looter_tripo_buildings.swap's step, after the retexture: the shell's faces on its Tripo slot (0) repacked, the
    set Tripo<name> written again (write_set skips fresh files: they are removed first) and graded again."""
    import looter_textures as lt
    import looter_tripo as tripo
    polys = _slot_polys(shell, set_name)
    new = repack(shell, maps, name, 2048, polys=polys)
    if new is maps:
        return
    for kind in ('BC', 'ORM', 'N'):
        p = lt.texture_path(set_name, kind)
        if os.path.exists(p):
            os.remove(p)
    tripo.write_set(new, set_name, set_name, path, master='World', orm_size=2048, max_size=2048)
    if regrade:
        regrade()
    _free({}, new)


def _free(maps, new):
    for key, image in new.items():
        if image is not None and image is not maps.get(key) and image.name in bpy.data.images:
            bpy.data.images.remove(image)


def own_set(obj, maps, name, size, polys, work=SOURCE, report=None):
    """polys (bool per face, or indices) moved onto a texture square of their own, the set Tripo<name> (the barge's
    house and casing walls, TripoDredgeBargeWalls): their islands packed as for size px (OWN_PAD[size] a side), their maps
    cut from maps (the GLB's 4096 delivery, on the faces' UVs as they are) at work px, so a paint step working at 4096
    (looter_tripo_barge_paint) writes texel for texel and write_maps shrinks the set to size. UV0 of those faces is
    rewritten. Returns {'BC', 'RM', 'N'} images, or None when the switch is off."""
    if not enabled(name):
        log(f"{name}: off ({ENV}): no own set")
        return None
    t0 = time.time()
    me = obj.data
    polys = np.asarray(polys)
    pid = np.flatnonzero(polys) if polys.dtype == bool else np.asarray(sorted(set(polys.tolist())), np.int64)
    old = loop_uvs(me)
    before = measure(obj, size, None, pid, uv=old)
    new = None
    for share in (0.6, 0.45, 0.3, 0.15):
        new, scale = pack_uv(obj, old, pid, size, pad=OWN_PAD.get(size, PAD.get(size, 1)), min_gain=1.0,
                             start_use=share, floor=0.0)
        if new is not None:
            break
    if new is None:
        raise RuntimeError(f'{name}: its islands do not fit a square of their own')
    after = measure(obj, size, None, pid, uv=new)
    steps = max(1, OWN_PAD.get(size, PAD.get(size, 1)) * work // size)
    data, known, tri, info = transfer(obj, old, new, maps, work, pid)
    L, P = loop_triangles(me)
    isl = rpk.islands(me, old, pid)
    sel = np.isin(P, pid)
    mip = bleed(np.where(tri >= 0, isl[P[sel]][np.maximum(tri, 0)], -1), known, steps)
    out = {}
    for key, arr in list(data.items()):
        filled = pad(arr, known, steps)
        del data[key]
        if key == 'N':
            v = filled / 127.5 - 1.0
            v /= np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-6)
            filled = (v * 0.5 + 0.5) * 255.0
        prefix = 'ORM' if (key == 'RM' and maps[key].name.upper().startswith('ORM')) else key
        out[key] = _image(f'{prefix}_Repack_{name}', filled, key == 'BC')
        del filled
    for key in ('BC', 'RM', 'N'):
        out.setdefault(key, None)
    set_loop_uvs(me, new)
    px_m = lambda m: m['texels_cm'] * 100.0
    secs = round(time.time() - t0, 1)
    entry = dict(before=before, after=after, mip=mip, taps=info['taps'], scale=info['scale'], secs=secs, size=size,
                 work=work)
    RESULTS[name] = entry
    log(f'{name}: {len(pid)} faces on a set of their own ({size} px, worked at {work}): use {before["use"]} of Tripo\'s '
        f'square -> {after["use"]} of theirs, {px_m(before):.1f} -> {px_m(after):.1f} px/m (big surfaces '
        f'{before["big_texels_cm"] * 100:.1f} -> {after["big_texels_cm"] * 100:.1f}), mip bleed {mip}, {secs} s')
    if report is not None:
        report['own set ' + name] = f'{px_m(before):.1f} -> {px_m(after):.1f} px/m'
    return out
