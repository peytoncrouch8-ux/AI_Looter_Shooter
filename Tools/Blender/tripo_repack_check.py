"""Checks for the Tripo repack (Tools/Blender/looter_tripo_repack.py; Docs/Work/2026-11-04-Art/REPACK.md), beside the editor:

  Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_repack_check.py -ScriptArgs survey[,<glob>...]
      every Tripo GLB's atlas as it came: use, islands, density spread, Tripo's own padding, and the share of its colour
      map's variance lost by keeping it at 2048 / 1024 / 512 px (what a game map that size can't show).
  ... -ScriptArgs packers,<Name>[,<Name>...]
      Tripo's packing against Blender's packer (concave, any turn) and ours (looter_tripo_repack_pack), on the GLB as
      loaded, at the model's map size.
  ... -ScriptArgs barge
      the barge's walls on their own set against the walls in the main atlas: REBAKE's close views, before and after,
      into Saved/ArtPreviews/Repack/BargeWalls/.
  ... -ScriptArgs model,<kind>:<Name>[,...]
      the model built by its own pipeline twice (LOOTER_TRIPO_REPACK=0, then on): its numbers before and after, renders at
      1, 3 and 10 m from its biggest surface (1920 x 1080, 90 degrees across: the audit's view) and a sheet of 1:1 crops,
      into Saved/ArtPreviews/Repack/; sharpness = the mean squared Laplacian of the render's luma on the model.
      Kinds: wardens (looter_tripo_wardens.SPECS), m23, m47, building (Farmhouse, Barn or Cottage: the script
      Art/Models/Buildings/<X>.py, model <X>_Ransom).
Results merge into Saved/ArtPreviews/Repack/results.json. The builds write the models' maps into Art/Textures as an
export does (the last build, repack on, wins).
"""
import glob
import json
import math
import os
import runpy
import sys
import time

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo_repack as trp
import looter_tripo_repack_pack as rpk

REPO = lt.REPO
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Repack')
DISTANCES = (1.0, 3.0, 10.0)
RES = (1920, 1080)
CROP = (800, 450)


def log(message):
    print(f'REPACK CHECK: {message}', flush=True)


def _clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


# --- Survey ---

def detail(luma, mask):
    """The share of the colour map's variance (luma, used texels only) lost by keeping it at each size: {px: share}."""
    out = {}
    total = float(luma[mask].var()) or 1.0
    size = luma.shape[0]
    level = size
    while level > 256:
        level //= 2
        f = size // level
        small = luma.reshape(level, f, level, f).mean((1, 3))
        up = np.repeat(np.repeat(small, f, 0), f, 1)
        out[level] = round(float(((luma - up)[mask]).var()) / total, 4)
    return out


def survey(paths):
    import looter_tripo as tripo
    for path in paths:
        t0 = time.time()
        _clear()
        obj, maps = tripo.load(path)
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        m = trp.measure(obj, trp.SOURCE)
        bc = trp._pixels_u8(maps['BC'])
        L, _ = trp.loop_triangles(obj.data)
        cov, _ = rpk.coverage(trp.loop_uvs(obj.data)[L], trp.SOURCE)
        luma = (bc[..., :3].astype(np.float32) @ np.array([0.299, 0.587, 0.114], np.float32)) / 255.0
        inside = float(luma[cov].mean())
        near = trp._ring(cov, 2)
        d = detail(luma, cov)
        row = dict(name=os.path.basename(path)[:-4], **m, pad12=round(float(luma[near].mean()) / max(inside, 1e-3), 2),
                   lost2048=d.get(2048), lost1024=d.get(1024), lost512=d.get(512), secs=round(time.time() - t0, 1))
        print('SURVEY ' + ' '.join(f'{k}={v}' for k, v in row.items()), flush=True)


# --- Packers ---

def _blender_pack(obj, size, pad):
    """Blender's own packer (average islands scale, concave shapes, any turn, the same gap) on a copy: per-loop UVs."""
    me = obj.data.copy()
    work = bpy.data.objects.new('_PackWork', me)
    bpy.context.scene.collection.objects.link(work)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    work.select_set(True)
    bpy.context.view_layer.objects.active = work
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.average_islands_scale()
    bpy.ops.uv.pack_islands(rotate=True, rotate_method='ANY', scale=True, margin_method='FRACTION',
                            margin=2.0 * pad / size, shape_method='CONCAVE')
    bpy.ops.object.mode_set(mode='OBJECT')
    uv = trp.loop_uvs(me)
    bpy.data.objects.remove(work)
    bpy.data.meshes.remove(me)
    return uv


def packers(names):
    import looter_tripo as tripo
    import looter_tripo_wardens as tw
    for name in names:
        _clear()
        obj, maps = tripo.load(os.path.join(tw.TRIPO_DIR, name, name + '.glb'))
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        size = _map_size(name)
        pad = trp.PAD[size]
        pid = np.arange(len(obj.data.polygons))
        old = trp.loop_uvs(obj.data)
        rows = {'tripo': (old, 0.0)}
        t0 = time.time()
        rows['blender'] = (_blender_pack(obj, size, pad), time.time() - t0)
        for p in (pad, pad // 2):
            t0 = time.time()
            uv, _ = trp.pack_uv(obj, old, pid, size, pad=p, log_steps=True)
            if uv is None:
                log(f'PACKER {name} {size} ours pad {p}: misses at Tripo\'s own area')
                continue
            rows[f'ours pad {p}'] = (uv, time.time() - t0)
        for key, (uv, secs) in rows.items():
            m = trp.measure(obj, size, None, pid, uv=uv)
            mip = trp.layout_bleed(obj, uv, pid, size, pad)
            log(f'PACKER {name} {size} {key}: use {m["use"]} cover {m["cover"]} overlap {m["overlap"]} texels/cm '
                f'{m["texels_cm"]} big {m["big_texels_cm"]} spread {m["spread"]} mip bleed {mip} ({secs:.1f} s)')


def _map_size(name):
    import looter_tripo_m23 as m23
    import looter_tripo_m47 as m47
    import looter_tripo_wardens as tw
    for table in (tw.SPECS, m23.BASE, m47.BASE):
        if name in table:
            return table[name]['maps']
    return 2048


# --- Models ---

def _build(kind, name):
    """The model built by its pipeline in the open scene: its object."""
    if kind == 'wardens':
        import looter_tripo_wardens as tw
        obj, _ = tw.build(name)
        return obj
    if kind == 'm23':
        import looter_tripo_m23 as m23
        return m23.build(name)[0]
    if kind == 'm47':
        import looter_tripo_m47 as m47
        return m47.build(name)[0]
    if kind == 'building':
        os.environ['LOOTER_BUILDINGS_TRIPO'] = name + 'Ransom'
        runpy.run_path(os.path.join(REPO, 'Art', 'Models', 'Buildings', name + '.py'), run_name='__main__')
        return bpy.data.objects[name + '_Ransom']
    raise ValueError(kind)


def _target(obj, set_name):
    """Where the renders look: the middle of the biggest island on the Tripo slot that a camera 1 m out along its
    normal sees unblocked (point, normal)."""
    me = obj.data
    uv = trp.loop_uvs(me)
    polys = trp._slot_polys(obj, set_name)
    if not len(polys):
        polys = np.arange(len(me.polygons))
    isl = rpk.islands(me, uv, polys)
    C = np.empty(len(me.polygons) * 3)
    N = np.empty(len(me.polygons) * 3)
    A = np.empty(len(me.polygons))
    me.polygons.foreach_get('center', C)
    me.polygons.foreach_get('normal', N)
    me.polygons.foreach_get('area', A)
    C, N = C.reshape(-1, 3), N.reshape(-1, 3)
    M = obj.matrix_world
    bvh = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    area = np.bincount(isl[polys], weights=A[polys])
    # Sides first (walls, hulls, a prop's flank: what the player faces), then anything not facing down.
    for k, steep in [(k, True) for k in np.argsort(-area)[:60]] + [(k, False) for k in np.argsort(-area)[:40]]:
        f = polys[isl[polys] == k]
        n = (N[f] * A[f, None]).sum(0)
        if np.linalg.norm(n) < 1e-9:
            continue
        n /= np.linalg.norm(n)
        if n[2] < -0.5 or (steep and abs(n[2]) > 0.35):
            continue
        # the island's face nearest its area-weighted middle
        mid = (C[f] * A[f, None]).sum(0) / A[f].sum()
        p = C[f][np.argmin(np.linalg.norm(C[f] - mid, axis=1))]
        P, Nw = M @ Vector(p), (M.to_3x3() @ Vector(n)).normalized()
        eye = P + Nw * 1.0
        hit = bvh.ray_cast(eye, (P - eye).normalized(), 1.05)
        if hit[0] is not None and (hit[0] - P).length < 0.02:
            return P, Nw
    return M @ Vector(C[np.argmax(A)]), Vector((0.0, -1.0, 0.2)).normalized()


def _scene_for_render():
    sc = bpy.context.scene
    for engine in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = engine
            break
        except TypeError:
            continue
    sc.render.resolution_x, sc.render.resolution_y = RES
    sc.render.resolution_percentage = 100
    sc.render.film_transparent = True
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGBA'
    sc.view_settings.view_transform = 'Standard'
    if not sc.world:
        sc.world = bpy.data.worlds.new('_RepackWorld')
    sc.world.use_nodes = True
    sc.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.32, 0.34, 0.38, 1.0)
    sc.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 1.0
    if '_RepackSun' not in bpy.data.objects:
        sun = bpy.data.lights.new('_RepackSun', 'SUN')
        sun.energy = 3.0
        so = bpy.data.objects.new('_RepackSun', sun)
        sc.collection.objects.link(so)
        so.rotation_euler = (math.radians(50), 0.0, math.radians(35))
    return sc


def _render(obj, point, normal, distance, path):
    sc = _scene_for_render()
    cam_data = bpy.data.cameras.get('_RepackCam') or bpy.data.cameras.new('_RepackCam')
    cam_data.sensor_fit = 'HORIZONTAL'
    cam_data.angle = math.radians(90.0)
    cam_data.clip_start = 0.02
    cam = bpy.data.objects.get('_RepackCam') or bpy.data.objects.new('_RepackCam', cam_data)
    if cam.name not in sc.collection.objects:
        sc.collection.objects.link(cam)
    d = (normal + Vector((0.0, 0.0, 0.25))).normalized()
    cam.location = point + d * distance
    cam.rotation_euler = (point - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.camera = cam
    for o in sc.objects:
        if o.type == 'MESH':
            o.hide_render = not (o is obj or o.parent is obj) or o.name.startswith(('UCX_', '_'))
    for im in bpy.data.images:
        if im.filepath and im.source == 'FILE':
            im.reload()
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    buf = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(buf)
    bpy.data.images.remove(img)
    return buf.reshape(h, w, 4)[::-1]


def sharpness(rgba):
    """The mean squared Laplacian of the render's luma (sRGB 0..1) where the model covers a pixel and its four neighbours."""
    luma = rgba[..., :3] @ np.array([0.299, 0.587, 0.114], np.float32)
    a = rgba[..., 3] > 0.99
    lap = 4 * luma[1:-1, 1:-1] - luma[:-2, 1:-1] - luma[2:, 1:-1] - luma[1:-1, :-2] - luma[1:-1, 2:]
    m = a[1:-1, 1:-1] & a[:-2, 1:-1] & a[2:, 1:-1] & a[1:-1, :-2] & a[1:-1, 2:]
    return round(float((lap[m] ** 2).mean()) * 1e4, 3) if m.any() else 0.0, round(float(m.mean()), 3)


def _crop(rgba):
    h, w = rgba.shape[:2]
    cw, ch = CROP
    y0, x0 = (h - ch) // 2, (w - cw) // 2
    out = rgba[y0:y0 + ch, x0:x0 + cw, :3].copy()
    a = rgba[y0:y0 + ch, x0:x0 + cw, 3:4]
    return out * a + np.array([0.32, 0.34, 0.38], np.float32) * (1 - a)


def model(kind, name):
    os.makedirs(OUT, exist_ok=True)
    set_name = 'Tripo' + (name + 'Ransom' if kind == 'building' else name)
    key = name + 'Ransom' if kind == 'building' else name
    res = {}
    view = None
    crops = {}
    for state in ('before', 'after'):
        _clear()
        os.environ[trp.ENV] = '0' if state == 'before' else '1'
        t0 = time.time()
        obj = _build(kind, name)
        res[f'{state}_build_s'] = round(time.time() - t0, 1)
        if view is None:
            view = _target(obj, set_name)
        for d in DISTANCES:
            path = os.path.join(OUT, f'{key}_{int(d)}m_{state}.png')
            rgba = _render(obj, view[0], view[1], d, path)
            res[f'sharp_{int(d)}m_{state}'], res[f'cover_{int(d)}m'] = sharpness(rgba)
            crops[(d, state)] = _crop(rgba)
    os.environ[trp.ENV] = '1'
    entry = trp.RESULTS.get(key, {})
    res.update({k: v for k, v in entry.items() if k in ('before', 'after', 'mip_old', 'mip', 'taps', 'scale', 'secs',
                                                        'kept', 'size', 'density')})
    res['view'] = [round(v, 3) for v in view[0]] + [round(v, 3) for v in view[1]]
    rows = [np.concatenate([crops[(d, 'before')], np.ones((CROP[1], 8, 3), np.float32), crops[(d, 'after')]], 1)
            for d in DISTANCES]
    gap = np.ones((8, CROP[0] * 2 + 8, 3), np.float32)
    sheet = np.concatenate(sum([[r, gap] for r in rows], [])[:-1], 0)
    lt.write_png(os.path.join(OUT, f'{key}_sheet.png'), lt.to8(np.clip(sheet, 0, 1)))
    path = os.path.join(OUT, 'results.json')
    allres = json.load(open(path)) if os.path.exists(path) else {}
    allres[key] = res
    json.dump(allres, open(path, 'w'), indent=1)
    log(f'MODEL {key}: ' + json.dumps({k: v for k, v in res.items() if k not in ('before', 'after')}))
    if 'before' in res:
        log(f'MODEL {key} before {json.dumps(res["before"])}')
        log(f'MODEL {key} after {json.dumps(res["after"])}')


def barge():
    """The barge's walls on their own set (SPECS['DredgeBarge']['sets']) against the walls painted in the main atlas:
    the build with LOOTER_TRIPO_REPACK=0, then on, each through the REBAKE lane's close views (looter_tripo_barge_paint
    .preview_views: the house at 1 and 3 m, the casing at 3 m, the walks), into Saved/ArtPreviews/Repack/BargeWalls/."""
    import looter_tripo_barge_paint as bpaint
    import looter_tripo_wardens as tw
    folder = os.path.join(OUT, 'BargeWalls')
    os.makedirs(folder, exist_ok=True)
    details = (('', bpaint.DETAIL_TODAY), ('_paintdetail', bpaint.DETAIL_PROPOSED))
    for state in ('before', 'after'):
        _clear()
        os.environ[trp.ENV] = '0' if state == 'before' else '1'
        obj, report = tw.build('DredgeBarge')
        for img in bpy.data.images:
            if img.filepath and 'TripoDredgeBarge' in img.filepath:
                img.reload()            # lt.material reuses a loaded image by its path
        bpaint.preview_views(obj, folder, state, details)
        log(f"BARGE {state}: {report.get('own set DredgeBargeWalls')} paint {report.get('paint')}")
    os.environ[trp.ENV] = '1'
    entry = trp.RESULTS.get('DredgeBargeWalls', {})
    log('BARGE walls set: ' + json.dumps({k: entry.get(k) for k in ('before', 'after', 'mip', 'taps', 'scale', 'secs')}))


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if not args:
        raise SystemExit('give survey, packers or model')
    what, rest = args[0], args[1:]
    if what == 'survey':
        pats = rest or ['Art/Tripo/*/*.glb', 'Art/Models/Creatures/Tripo/*.glb']
        survey(sorted(p for pat in pats for p in glob.glob(os.path.join(REPO, pat))))
    elif what == 'packers':
        packers(rest)
    elif what == 'barge':
        barge()
    elif what == 'model':
        import traceback
        for item in rest:
            kind, name = item.split(':')
            try:
                model(kind, name)
            except Exception:
                log(f'MODEL {name} FAILED:\n{traceback.format_exc()}')
    else:
        raise SystemExit(f'unknown check {what}')
