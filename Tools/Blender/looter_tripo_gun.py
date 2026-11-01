"""Tripo models in place of a scripted gun's parts, for any looter_guns.Gun (the Drover's first, 2026-11-01; the Bullpup
keeps its own copy of this in Bullpup.py). A gun script builds every part from code, then calls override(): each spec
found in its Tripo folder's parts_set<N>.py files replaces that part's model, keeping its name, key and sockets, so the
game can't tell the two apart (saved guns keep their parts by key).

What a spec does (a dict; the folder's parts_set<N>.py hold PARTS = {'<Slot>_<Key>': spec}):
  glb       the model file in the folder (<Gun><Slot>_<Key>.glb)
  axes      looter_tripo.fit's forward / left / up (default muzzle -Y, left +X: Tripo's align_image output, front view =
            from the muzzle)
  keep      rule(piece) -> bool: scripted pieces kept on top of the Tripo mesh (piece.mat, piece.lo, piece.hi in gun cm;
            piece.obj): crisp glow strips, lenses and reticles, moving bits Tripo wasn't shown (the cylinder's rod and
            crane). The part's box is the scripted part's bounds (box_from='gone': the pieces not kept), unless
  box       ((u0, u1), (v0, v1), (w0, w1) or w): where the mesh's bounds go (gun cm)
  cuts      fn(ctx) -> [cutter objects]: subtracted from the fitted mesh (a frame's window, a sight's notch or bore)
  glows     {glow material name: colour test}: faces painted that colour become that glow (looter_tripo.is_<name> or a
            function of (r, g, b)); Gun*Glow and GunReticle take the element's colour in the game
  paint     dict(slot='GunPolymerSand', test=..., tint=0xe6d8b6, mode='rgb'|'luma', by='pieces'|'faces'): the pieces
            (or faces) mostly that colour become the paintable slot <slot>_<set> (the gun's paint of <slot>, the maker's
            shell colour, recolours them); test: a looter_tripo is_<name> or a function of (r, g, b) that also runs on
            arrays (&, <, no `and`)
  sockets   {name: ((u, v), w) or ((u, v), w, (rx, ry, rz))}: sockets replacing the scripted part's of that name
  spots     {name: ((u, v), w, fingers, palm) or Trigger: (u, v)}: hand markers (looter_guns Gun.hand_mark)
  prepare(ctx), finish(ctx): hooks before the faces are sorted (move or true the mesh, measure; may set ctx.sockets,
            ctx.glow_faces) and after the pieces are added (more pieces: ctx.add(obj)). ctx: key, obj, maps, mat, box,
            kept (the scripted pieces kept), tripo, lg, lt, glow(name), log(text), sockets, glow_faces, add(*objs).
"""
import glob
import math
import os
import re
import runpy
from types import SimpleNamespace

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import looter_guns as lg
import looter_textures as lt
import looter_tripo as tripo

LODS = '50,20'              # LOD1, LOD2 (percent of triangles): as the Bullpup's Tripo parts
LOD_SCREENS = (0.3, 0.12)   # a whole gun's switch sizes; a part's scale with its size against the gun's length


def specs(folder, env):
    """{key: (spec, file)} from folder's parts_set<N>.py; the environment variable env: '0' none, '1,3' only those sets."""
    wanted = os.environ.get(env, '')
    if wanted == '0':
        return {}
    sets = {int(s) for s in wanted.split(',') if s.strip().isdigit()}
    found = {}
    for path in sorted(glob.glob(os.path.join(lt.REPO, folder, 'parts_set*.py'))):
        number = int(re.search(r'parts_set(\d+)', os.path.basename(path)).group(1))
        if sets and number not in sets:
            continue
        for key, spec in runpy.run_path(path)['PARTS'].items():
            if key in found:
                raise ValueError(f'{key} is in {os.path.basename(path)} and {found[key][1]}')
            found[key] = (spec, os.path.basename(path))
    return found


def uvw_bounds(objs):
    """(lo, hi) of the objects' vertices in gun space, cm (u, v, w); the objects as Gun.add holds them (world = gun)."""
    pts = []
    for o in objs:
        M = o.matrix_world
        pts += [tripo.gun_uvw(M @ v.co) for v in o.data.vertices]
    lo = tuple(min(p[i] for p in pts) for i in range(3))
    hi = tuple(max(p[i] for p in pts) for i in range(3))
    return lo, hi


def piece(obj):
    lo, hi = uvw_bounds([obj])
    mat = obj.data.materials[0].name if obj.data.materials and obj.data.materials[0] else ''
    return SimpleNamespace(obj=obj, mat=mat, lo=lo, hi=hi)


def glow(name):
    mat = bpy.data.materials.get(name)
    if mat is None:
        raise KeyError(f'no glow material {name} (looter_guns or the gun script makes them)')
    return mat


def _test(test):
    return test if callable(test) else getattr(tripo, 'is_' + test)


def add_split(g, key, obj, mat, maps, glows, paint):
    """Adds obj to model key: glows {material: faces} as their own slots, the paint's pieces (or faces) as the paintable
    slot, the rest textured with Tripo's maps (as Bullpup.py's tripo_add)."""
    taken, unique = set(), {}
    for k, faces in glows.items():   # a face in two groups goes to the first
        faces = [f for f in dict.fromkeys(faces) if f not in taken]
        if faces:
            unique[k] = faces
            taken.update(faces)
    keep = [mat]
    if paint:
        test = _test(paint.get('test', 'sand'))
        by_faces = paint.get('by') == 'faces'
        hit = set(tripo.faces_where(obj, maps['BC'], test, share=0.5 if by_faces else 0.6))
        shell = []
        if by_faces:
            shell = [f for f in tripo.majority(obj, hit) if f not in taken]
        else:
            area = [p.area for p in obj.data.polygons]
            for isl in tripo.islands(obj):
                if sum(area[f] for f in isl.faces if f in hit) > 0.5 * sum(area[f] for f in isl.faces):
                    shell += [f for f in isl.faces if f not in taken]
        if shell:
            painted = tripo.paint_set(maps, mat.name[len('Gun'):], paint.get('slot', lg.POLY_SAND.name),
                                      paint.get('tint', 0xe6d8b6), obj['TripoFile'],
                                      test=None if paint.get('test', 'sand') == 'sand' else test,
                                      mode=paint.get('mode', 'rgb'))
            unique[painted] = shell
            keep.append(painted)
            taken.update(shell)
        lt._log(f'Tripo {key}: {len(shell)} of {len(obj.data.polygons)} faces in the paintable shell '
                f'({paint.get("slot", lg.POLY_SAND.name)})')
    rest = [i for i in range(len(obj.data.polygons)) if i not in taken]
    g.add(key, *tripo.split(obj, {mat: rest, **unique}, keep_uv=tuple(keep)))


def build(g, key, spec, folder, prefix):
    """Replaces g's scripted model key with the Tripo model spec describes (see the module's notes)."""
    old = list(g.parts.get(key, []))
    rule = spec.get('keep')
    infos = [piece(o) for o in old]
    kept = [p.obj for p in infos if rule and rule(p)]
    gone = [o for o in old if o not in kept]
    if spec.get('box'):
        box = spec['box']
    else:
        # The bounds of what Tripo was shown: the whole scripted part (kept pieces too: they were in its views), or
        # with box_from='gone' only the pieces it replaces (views that left some out, as the cylinder's rod and crane).
        lo, hi = uvw_bounds(gone if spec.get('box_from') == 'gone' else old)
        box = ((lo[0], hi[0]), (lo[1], hi[1]), (lo[2], hi[2]))
    path = os.path.join(folder, spec['glb'])
    set_name = f'Tripo{prefix}' + key.replace('_', '')
    obj, maps = tripo.load(path)
    mat = tripo.write_set(maps, set_name, 'Gun' + set_name, path)
    tripo.fit(obj, *box, **spec.get('axes', dict(forward='-Y', left='+X')))
    for o in gone:
        bpy.data.objects.remove(o)
    g.parts[key] = kept
    log = lambda text: lt._log(f'Tripo {key}: {text}')
    ctx = SimpleNamespace(key=key, obj=obj, maps=maps, mat=mat, box=box, kept=kept, tripo=tripo, lg=lg, lt=lt,
                          glow=glow, log=log, sockets=dict(spec.get('sockets', {})), glow_faces={},
                          add=lambda *objs: g.add(key, *objs))
    log(f'{len(obj.data.polygons)} faces fitted to u {box[0][0]:.2f}..{box[0][1]:.2f}, v {box[1][0]:.2f}..'
        f'{box[1][1]:.2f}; {len(kept)} scripted pieces kept')
    if spec.get('prepare'):
        spec['prepare'](ctx)
    if spec.get('cuts'):
        source = obj['TripoFile']
        before = len(obj.data.polygons)
        obj = lg.cut(obj, spec['cuts'](ctx))
        obj['TripoFile'] = source
        ctx.obj = obj
        log(f'cut: {before} -> {len(obj.data.polygons)} faces')
    glows = {}
    for name, faces in ctx.glow_faces.items():
        glows.setdefault(glow(name), []).extend(faces)
    for name, test in spec.get('glows', {}).items():
        test = _test(test)
        faces = tripo.faces_where(obj, maps['BC'], test) or tripo.faces_where(obj, maps['BC'], test, corners=False)
        glows.setdefault(glow(name), []).extend(faces)
        log(f'{len(faces)} faces painted for {name}')
    add_split(g, key, obj, mat, maps, glows, spec.get('paint'))
    if spec.get('finish'):
        spec['finish'](ctx)
    g.sockets = [s for s in g.sockets if not (s[0] == key and s[1] in ctx.sockets)]
    for name, value in ctx.sockets.items():
        uv, w, rotation = tripo.socket_spec(value)
        g.socket(key, name, uv, w=w, rotation=rotation)
    for name, spot in spec.get('spots', {}).items():
        g.sockets = [s for s in g.sockets if not (s[0] == key and s[1] == name)]
        if name == 'Trigger':
            g.socket(key, name, spot)
            continue
        role, side, pose = name.split('_')
        uv, w, fingers, palm = spot
        g.hand_mark(key, role, side, pose, uv, w, fingers, palm)


def override(g, found, folder, prefix):
    """Every spec in found (specs()) replacing its scripted part; call before g.build()."""
    for key, (spec, file) in list(found.items()):
        if key not in g.parts:
            raise KeyError(f'{file}: {key} is no {g.name} part')
        if not os.path.exists(os.path.join(lt.REPO, folder, spec['glb'])):
            # A set being made part by part: the scripted part stays until its model is in.
            lt._log(f'WARNING Tripo {key}: no {folder}/{spec["glb"]}; the scripted part stays')
            del found[key]
            continue
        build(g, key, spec, os.path.join(lt.REPO, folder), prefix)


def lods(built, found, gun_length):
    """LODs on the Tripo parts (thousands of triangles against a scripted part's hundreds): first person keeps LOD0; a
    part's switch sizes scale with its size against the whole gun's, so a gun's parts switch together."""
    for key in found:
        share = max(built[key].dimensions) / gun_length
        built[key]['LODs'] = LODS
        built[key]['LODScreens'] = ','.join(f'{s * share:.4f}' for s in LOD_SCREENS)


# --- Helpers for the specs ---

def rotate_about_u(obj, degrees, v=0.0, w=0.0):
    """Turns the mesh about the axis along u through (v, w) (gun cm); positive carries the top to the gun's left."""
    pivot = lg.at(0.0, v, w)
    # About Blender +Y (the gun's back), a positive angle carries +Z (up) over to +X (the gun's left).
    obj.data.transform(Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(degrees), 4, 'Y') @
                       Matrix.Translation(-pivot))
    obj.data.update()


def is_round(c):
    """A loaded round's nose or rim as Tripo paints it (copper or brass: warm, saturated, red over blue)."""
    r, g, b = c
    return (r > 0.3) & (g > 0.3 * r) & (g < 0.9 * r) & (b < 0.6 * r)


def chamber_phase(obj, u_face, v_axis, ring, chambers, image=None, test=is_round, step=1.0):
    """Where the chambers are round the drum (degrees from the top, toward the gun's left), seen from the front: rays
    along -u from ahead of u_face at radius ring round the axis. With image (the base colour), the rounds' colour on
    what the rays hit marks the chambers (Tripo paints the copper noses in the mouths even where it models them flush);
    else the shallow webs between the mouths do (a chamber is half a pitch from a web). Returns (phase, share): share is
    the colour's share of the ring (or the depth contrast, cm); phase None when nothing was found."""
    from mathutils.bvhtree import BVHTree
    me = obj.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
    angles = np.arange(0.0, 360.0, step)
    pitch = 360.0 / chambers
    k = 2.0 * math.pi * chambers / 360.0
    fold = lambda sel: math.degrees(math.atan2(np.sin(angles[sel] * k).sum(), np.cos(angles[sel] * k).sum())) / chambers
    depth, faces = [], []
    for a in angles:
        r = math.radians(a)
        v, w = v_axis + ring * math.cos(r), ring * math.sin(r)
        hit, _, index, _ = tree.ray_cast(lg.at(u_face + 3.0, v, w), Vector((0.0, 1.0, 0.0)), 8.0 * lg.CM)
        depth.append(u_face + 3.0 - (tripo.gun_uvw(hit)[0] if hit is not None else u_face - 5.0))
        faces.append(index)
    if image is not None:
        px = tripo.pixels(image, min(1024, image.size[0]))[..., :3]
        h, wd = px.shape[:2]
        uv = me.uv_layers.active.data
        warm = []
        for index in faces:
            if index is None:
                warm.append(False)
                continue
            pts = [uv[li].uv for li in me.polygons[index].loop_indices]
            cu, cv = sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts)
            c = px[min(h - 1, max(0, int((1.0 - cv % 1.0) * h))), min(wd - 1, max(0, int((cu % 1.0) * wd)))]
            warm.append(bool(test(c)))
        warm = np.array(warm)
        share = float(warm.mean())
        if 0.05 < share < 0.95:
            return fold(warm) % pitch, share
    depth = np.array(depth)
    contrast = float(np.percentile(depth, 90) - np.percentile(depth, 10))
    if contrast < 0.05:
        return None, contrast
    webs = depth < np.percentile(depth, 10)
    return (fold(webs) + pitch * 0.5) % pitch, contrast


def true_round(obj, u0, u1, v_axis, radius, band=0.25):
    """Vertices of the drum's outer wall (between u0 and u1, within band of radius from the axis) pushed onto the circle:
    Tripo's turned parts come out a little polygonal and off-round."""
    moved = 0
    for vert in obj.data.vertices:
        u, v, w = tripo.gun_uvw(vert.co)
        if not (u0 <= u <= u1):
            continue
        dv, dw = v - v_axis, w
        r = math.hypot(dv, dw)
        if r > 1e-6 and abs(r - radius) < band:
            s = radius / r
            vert.co = lg.at(u, v_axis + dv * s, dw * s)
            moved += 1
    obj.data.update()
    return moved
