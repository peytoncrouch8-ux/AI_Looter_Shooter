"""Building blocks for scripted props (Art/Models/Props/*.py): parts made from boxes, sweeps and lathes, mapped onto
the house trim sheet (or a tileable set) at world scale, merged into one model with collision hulls and baked ambient
occlusion. It sits on top of looter_model (lm) and looter_textures (lt); models.ps1 puts Tools/Blender on sys.path, so
a prop script does:

    import looter_model as lm
    import looter_textures as lt
    import looter_props as lp

    post = lp.block((0.14, 0.14, 1.4), (0.0, 0.0, 0.55), bevel=0.014)
    lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=3)      # one weathered board, grain up the post
    lp.slice_at(post, (0.0, 0.0, 1.0), (0.03,))                 # a vertex row above the ground for the AO
    model = lp.join('FencePost', [post])
    lp.hull_box(model, (0.16, 0.16, 1.22), (0.0, 0.0, 0.61))
    lp.finish(model, nanite=False)                              # shading, baked AO, export settings

Parts are plain mesh objects whose vertices are already in model space (their transforms stay identity): build them
with block/sweep/lathe, move them with place(), map them (grain, trim, lathe_uv; before place() for lathe_uv), and
join() them into the model. Distances are metres; the front faces -Y (Art/README.md).
"""
import math
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_model as lm
import looter_textures as lt


def trim_material():
    """The house trim sheet's material, shared by every prop (made on first use)."""
    mat = bpy.data.materials.get('HouseTrim')
    if mat is None or mat.get('TextureSet') != 'HouseTrim':
        mat = lt.material('HouseTrim')
    return mat


STRIP_SLOT = {'Siding': 0.2, 'Beams': 0.2, 'Logs': 0.4}   # a board, a beam face, a log
# Stretches of each trim strip without board end joints: (slot, start, end) in metres along U (the strip repeats
# every 6.4 m), measured from T_HouseTrim_BC (2026-09-30, 16:47); measure again if the sheet's joints move. Long
# parts are mapped inside one, so a post or a rail is one piece.
JOINT_FREE = {
    'Siding': [(0, 0.55, 4.25), (0, 4.36, 6.85), (1, 2.03, 3.07), (1, 4.16, 5.67), (1, 5.77, 8.33), (2, 1.41, 3.55),
               (2, 3.66, 7.71), (3, 0.59, 2.57), (3, 4.83, 6.89), (3, 3.67, 4.73)],
    'Beams': [(k, 0.0, 6.4) for k in range(4)],
    'Logs': [(k, 0.0, 6.4) for k in range(2)],
    'TrimTeal': [(0, 6.08, 8.95), (0, 2.65, 4.53), (0, 4.63, 5.98)],
    'TrimRed': [(0, 4.24, 5.85), (0, 5.96, 10.54)],
    'Iron': [(0, 0.0, 6.4)],
    'Plaster': [(0, 0.0, 6.4)],
    'Glass': [(0, 0.0, 6.4)],
}


def mesh_object(bm, name='_part'):
    """A new object from a bmesh (freed), linked to the scene."""
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def new_bmesh():
    """An empty bmesh with a UV layer (so primitives fill in UVs)."""
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')
    return bm


def place(obj, location=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0)):
    """Moves a part's mesh: turned by rotation (degrees, X then Y then Z) about the origin, then moved to location."""
    obj.data.transform(Matrix.LocRotScale(Vector(location), Euler([math.radians(a) for a in rotation]), None))
    obj.data.update()
    return obj


def block(size, center=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0), bevel=0.0):
    """A box of size (x, y, z), chamfered by bevel, turned then moved to center."""
    bm = new_bmesh()
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Diagonal(Vector(size)).to_4x4())
    if bevel > 0.0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=bevel, segments=1, affect='EDGES', clamp_overlap=True)
    return place(mesh_object(bm), center, rotation)


def sweep(points, profile, up=(0.0, 0.0, 1.0), scales=None):
    """A closed tube along the polyline points; its cross-section is the 2D profile [(across, up), ...]."""
    bm = new_bmesh()
    pts = [Vector(p) for p in points]
    rings = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        side = t.cross(Vector(up))
        if side.length < 1e-6:
            side = t.cross(Vector((1.0, 0.0, 0.0)))
        side.normalize()
        upv = side.cross(t).normalized()
        s = scales[i] if scales else 1.0
        rings.append([bm.verts.new(p + side * (a * s) + upv * (b * s)) for a, b in profile])
    n = len(profile)
    for r0, r1 in zip(rings, rings[1:]):
        for j in range(n):
            bm.faces.new((r0[j], r0[(j + 1) % n], r1[(j + 1) % n], r1[j]))
    bm.faces.new(rings[0])
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(bm)


def ngon(radius, sides, jitter=0.0, seed=0, squash=1.0, start=0.0):
    """A 2D polygon profile for sweep(), a little uneven if jitter > 0."""
    rnd = random.Random(seed)
    return [(math.cos(start + 2.0 * math.pi * i / sides) * radius * (1.0 + rnd.uniform(-jitter, jitter)),
             math.sin(start + 2.0 * math.pi * i / sides) * radius * squash * (1.0 + rnd.uniform(-jitter, jitter)))
            for i in range(sides)]


def rough(obj, amount, frequency=3.0, seed=0):
    """Hand-made unevenness: every vertex moves a little (the same amount where parts touch)."""
    offset = Vector((seed * 3.7, seed * 1.3, seed * 5.1))
    for v in obj.data.vertices:
        p = v.co * frequency + offset
        v.co += Vector((noise.noise(p), noise.noise(p + Vector((17.0, 0.0, 0.0))),
                        noise.noise(p + Vector((0.0, 29.0, 0.0))))) * amount
    obj.data.update()
    return obj


def _face_axes(n):
    """U and V of a face as lt.trim_uv sees it: U horizontal along the face, V up it."""
    if abs(n.z) < 0.95:
        up = Vector((0.0, 0.0, 1.0))
    else:
        up = Vector((0.0, 1.0, 0.0)) if n.z > 0.0 else Vector((0.0, -1.0, 0.0))
    v = (up - n * up.dot(n)).normalized()
    return v.cross(n).normalized(), v


def trim(obj, strip, faces=None, seed=0, slot=None, material=None):
    """Maps a part onto a strip of the house trim sheet at world scale, with the grain (the strip's U) along each
    face's longer side, on one board (Siding), beam face (Beams) or log (Logs) picked by seed or slot."""
    lt.assign(obj, material or trim_material(), faces)
    rnd = random.Random(seed)
    indices = list(range(len(obj.data.polygons))) if faces is None else list(faces)
    along, across, widest = [], [], 0.0
    for i in indices:
        p = obj.data.polygons[i]
        u, v = _face_axes(p.normal)
        cos = [obj.data.vertices[k].co for k in p.vertices]
        du = max(c.dot(u) for c in cos) - min(c.dot(u) for c in cos)
        dv = max(c.dot(v) for c in cos) - min(c.dot(v) for c in cos)
        if dv > du * 1.05:
            across.append(i)
            widest = max(widest, du)
        else:
            along.append(i)
            widest = max(widest, dv)
    step = STRIP_SLOT.get(strip)
    v_offset = 0.0
    if step:
        slots = max(1, int((0.8 - widest + 1e-4) // step) + 1)
        v_offset = step * ((rnd.randrange(slots) if slot is None else min(slot, slots - 1)))
    elif strip in ('Iron', 'TrimTeal', 'TrimRed', 'Glass', 'H1', 'H2', 'H3', 'H4'):
        v_offset = max(0.0, (0.2 - widest) * 0.5)
    u_offset = rnd.uniform(0.0, 6.4)
    if along:
        lt.trim_uv(obj, along, strip, u_offset=u_offset, v_offset=v_offset)
    if across:
        lt.trim_uv(obj, across, strip, u_offset=u_offset, v_offset=v_offset, rotate=True)
    return obj


def _long_axis(points):
    """The direction a set of points spreads most along (their principal axis)."""
    import numpy as np
    a = np.array([tuple(p) for p in points])
    a = a - a.mean(axis=0)
    values, vectors = np.linalg.eigh(a.T @ a)
    return Vector(vectors[:, -1]).normalized()


def grain(obj, strip, axis=None, seed=0, faces=None, material=None, slot=None):
    """Maps a long part (post, rail, plank, handle, spoke) onto one board of a trim strip at world scale: the grain
    runs along the part (axis, default its longest direction) continuously around its faces, and the part lies on a
    stretch of the board without end joints (JOINT_FREE), so it reads as one piece of wood (or iron, or paint)."""
    lt.assign(obj, material or trim_material(), faces)
    mesh = obj.data
    indices = list(range(len(mesh.polygons))) if faces is None else list(faces)
    used = sorted({k for i in indices for k in mesh.polygons[i].vertices})
    axis = Vector(axis).normalized() if axis is not None else _long_axis([mesh.vertices[k].co for k in used])
    along = [mesh.vertices[k].co.dot(axis) for k in used]
    a0, length = min(along), max(along) - min(along)
    rnd = random.Random(seed)
    if strip in lt.SETS:
        # A tileable set: V along the part (bark furrows run along V), U around it.
        info = lt.SETS[strip]
        scale = info['density'] / float(info['size'])
        offset = (rnd.uniform(0.0, 3.0), rnd.uniform(0.0, 3.0))
        uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
        for i in indices:
            p = mesh.polygons[i]
            n = p.normal
            if abs(n.dot(axis)) > 0.7:
                # An end (a log's cut face): mapped flat in its own plane, or its UVs would collapse to a line.
                ua = n.cross(Vector((0.0, 0.0, 1.0)) if abs(n.z) < 0.9 else Vector((1.0, 0.0, 0.0))).normalized()
                va = n.cross(ua)
                for loop in p.loop_indices:
                    co = mesh.vertices[mesh.loops[loop].vertex_index].co
                    uv[loop].uv = ((co.dot(ua) + offset[0]) * scale, (co.dot(va) + offset[1]) * scale)
                continue
            ua = (axis - n * n.dot(axis)).normalized()
            va = n.cross(ua)
            for loop in p.loop_indices:
                co = mesh.vertices[mesh.loops[loop].vertex_index].co
                uv[loop].uv = ((co.dot(va) + offset[0]) * scale, (co.dot(axis) - a0 + offset[1]) * scale)
        return obj
    regions = [r for r in JOINT_FREE[strip] if slot is None or r[0] == slot]
    fitting = [r for r in regions if r[2] - r[1] >= length + 0.1] or sorted(regions, key=lambda r: r[1] - r[2])[:1]
    board, start, end = rnd.choice(fitting)
    u_start = start + 0.05 + rnd.uniform(0.0, max(0.0, end - start - length - 0.1))
    key = lt.TRIM_ALIASES.get(strip, strip)
    v_lo, v_hi = lt.TRIM_STRIPS[key]
    scale = lt.TRIM_DENSITY / lt.TRIM_SIZE              # UV units per metre
    strip_height = (v_hi - v_lo) / scale
    size = STRIP_SLOT.get(strip, strip_height)
    inset = 1.0 / lt.TRIM_SIZE
    uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
    for i in indices:
        p = mesh.polygons[i]
        n = p.normal
        if abs(n.dot(axis)) > 0.7:   # an end: any direction in its plane
            ua = n.cross(Vector((0.0, 0.0, 1.0)) if abs(n.z) < 0.9 else Vector((1.0, 0.0, 0.0))).normalized()
        else:
            ua = (axis - n * n.dot(axis)).normalized()
        va = n.cross(ua)
        cos = [mesh.vertices[k].co for k in p.vertices]
        vs = [c.dot(va) for c in cos]
        v_min, width = min(vs), max(vs) - min(vs)
        base = min(board * size + max(0.0, (size - width) * 0.5), max(strip_height - width, 0.0))
        for loop in p.loop_indices:
            co = mesh.vertices[mesh.loops[loop].vertex_index].co
            u = u_start + (co.dot(axis) - a0 if abs(n.dot(axis)) <= 0.7 else co.dot(ua))
            v = min(max(base + co.dot(va) - v_min, 0.0), strip_height)
            uv[loop].uv = (u * scale, min(max(v_lo + v * scale, v_lo + inset), v_hi - inset))
    return obj


def slice_at(obj, normal, positions):
    """Adds edge loops across a part where planes cut it (normal, at these distances along it from the origin). Vertex
    ambient occlusion lives on vertices: a face whose corners are buried in another part (or under the ground) goes
    dark all over, so give it vertices in the open. Use it after the UV mapping (the new vertices interpolate it)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    n = Vector(normal).normalized()
    for d in positions:
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=n * d, plane_no=n, dist=1e-5)
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def dice(obj, step):
    """Cuts a part into a grid of about step metres (for vertex AO on large faces); after the UV mapping."""
    for axis in range(3):
        values = [v.co[axis] for v in obj.data.vertices]
        lo, hi = min(values), max(values)
        count = int((hi - lo) / step)
        if count >= 1:
            n = Vector((1.0 if axis == 0 else 0.0, 1.0 if axis == 1 else 0.0, 1.0 if axis == 2 else 0.0))
            slice_at(obj, n, [lo + (hi - lo) * (k + 1) / (count + 1) for k in range(count)])
    return obj


def join(name, parts):
    """Merges the parts into one mesh object called name (the model)."""
    parts = [p for p in parts if p is not None]
    with bpy.context.temp_override(active_object=parts[0], object=parts[0], selected_objects=parts,
                                   selected_editable_objects=parts):
        bpy.ops.object.join()
    obj = parts[0]
    obj.name = name
    obj.data.name = name
    return obj


def hull_box(root, size, center=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0)):
    hull = block(size, center, rotation)
    hull.name = 'UCX_' + root.name
    hull.parent = root
    hull.display_type = 'WIRE'
    return hull


def hull_points(root, points):
    """A convex collision hull around points."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    hull = mesh_object(bm, 'UCX_' + root.name)
    hull.parent = root
    hull.display_type = 'WIRE'
    return hull


def finish(obj, ao=0.6, nanite=True, fallback=100, smooth=35.0):
    """Shading, baked ambient occlusion and the export settings of a finished prop."""
    lm.smooth(obj, smooth)
    lt.bake_vertex_ao(obj, distance=ao)
    if not nanite:
        obj['Nanite'] = 0
    elif fallback is not None:
        obj['Fallback'] = fallback
    return obj


def lathe(profile, segments=16, closed=False):
    """A closed surface of revolution around Z: profile [(radius, z), ...] from one end to the other, a radius of 0
    closing an end (or closed=True: the profile is a loop, as for a ring). Returns the part and each face's band (the
    profile step it was made from), faces in order."""
    if closed:
        profile = list(profile) + [profile[0]]
    bm = new_bmesh()
    rings = []
    for index, (r, z) in enumerate(profile):
        if closed and index == len(profile) - 1:
            rings.append(rings[0])
        elif r <= 1e-6:
            rings.append([bm.verts.new((0.0, 0.0, z))])
        else:
            rings.append([bm.verts.new((math.cos(2.0 * math.pi * k / segments) * r,
                                        math.sin(2.0 * math.pi * k / segments) * r, z)) for k in range(segments)])
    bands = []
    for b, (r0, r1) in enumerate(zip(rings, rings[1:])):
        for k in range(segments):
            k1 = (k + 1) % segments
            if len(r0) == 1 and len(r1) == 1:
                continue
            if len(r0) == 1:
                bm.faces.new((r0[0], r1[k], r1[k1]))
            elif len(r1) == 1:
                bm.faces.new((r0[k], r0[k1], r1[0]))
            else:
                bm.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
            bands.append(b)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(bm), bands


def lathe_uv(obj, strip, faces=None, grain='around', seed=0, per_column=False):
    """Cylindrical mapping around the part's own Z axis at world scale (use it before place()). grain='around': U
    runs around (hoops, tyres, a drum's paint), V up; 'up': U runs up (staves), V around. strip is a trim strip
    (Siding, Beams, Iron, ...; per_column puts every column of faces, a stave, on its own board and stretch) or a
    tileable set (MetalRust, Hay, ...). Flat rings (steps, rims) run around too, V across the ring."""
    mesh = obj.data
    indices = list(range(len(mesh.polygons))) if faces is None else list(faces)
    rnd = random.Random(seed)
    tileable = strip in lt.SETS
    if tileable:
        info = lt.SETS[strip]
        scale = info['density'] / float(info['size'])
    else:
        v_lo, v_hi = lt.TRIM_STRIPS[lt.TRIM_ALIASES.get(strip, strip)]
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        strip_height = (v_hi - v_lo) / scale
        size = STRIP_SLOT.get(strip, strip_height)
        inset = 1.0 / lt.TRIM_SIZE
    uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
    columns = {}
    u_base = rnd.uniform(0.0, 6.4)
    for i in indices:
        p = mesh.polygons[i]
        center = p.center
        a_c = math.atan2(center.y, center.x)
        radius = max(math.hypot(center.x, center.y), 1e-4)
        cos = [mesh.vertices[mesh.loops[loop].vertex_index].co for loop in p.loop_indices]

        def arc(co):
            d = (math.atan2(co.y, co.x) - a_c + math.pi) % (2.0 * math.pi) - math.pi
            return (a_c + d) * radius
        flat = abs(p.normal.z) > 0.9
        across = [math.hypot(c.x, c.y) for c in cos] if flat else [c.z for c in cos]
        arcs = [arc(c) for c in cos]
        s, t = (arcs, across) if grain == 'around' or flat else (across, [-a for a in arcs])
        if tileable:
            coords = [(si * scale, ti * scale) for si, ti in zip(s, t)]
        else:
            key = round(a_c, 2) if per_column else 0
            if key not in columns:
                if per_column and strip in JOINT_FREE:
                    board, start, end = rnd.choice(JOINT_FREE[strip])
                    columns[key] = (board, start + rnd.uniform(0.0, max(0.0, end - start - 1.0)))
                else:
                    columns[key] = (rnd.randrange(max(1, int(strip_height / size + 1e-6))), u_base)
            board, u0 = columns[key]
            t_min, width = min(t), max(t) - min(t)
            base = min(board * size + max(0.0, (size - width) * 0.5), max(strip_height - width, 0.0))
            coords = [((u0 + si) * scale,
                       min(max(v_lo + min(max(base + ti - t_min, 0.0), strip_height) * scale, v_lo + inset), v_hi - inset))
                      for si, ti in zip(s, t)]
        for loop, co in zip(p.loop_indices, coords):
            uv[loop].uv = co
    return obj


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def nail(center, normal, size=0.024):
    """An oversized square nail head standing out of a surface (normal: the surface's outward direction)."""
    n = Vector(normal).normalized()
    head = block((size, size, 0.008), (0.0, 0.0, 0.0))
    rotation = n.to_track_quat('Z', 'Y').to_euler()
    place(head, Vector(center) + n * 0.004, [math.degrees(a) for a in rotation])
    return grain(head, 'Iron', axis=(1.0, 0.0, 0.0), seed=int(abs(center[0] * 1000 + center[2] * 100)))
