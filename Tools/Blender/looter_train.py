"""The railway's parts kit for Art/Models/Vehicles/Train.py: the locomotives, the passenger car and Tilly's hearse car
are put together from the same pieces (wheel sets, coupling rods, couplers and buffers, bogies, steps, railings,
lamps, windows, roofs) in the same shared materials, so the train reads as one railway.

    import looter_train as tk
    k = tk.Kit('HearseCar', seed=7)
    k.box((2.8, 9.0, 0.1), (0.0, 0.0, 1.2), 'trim', strip='Beams')     # parts in model space (metres, front -Y)
    tk.end_gear(k, -5.0, -1.0)                                          # buffers and the coupler at the front
    k.socket('Coupler_Front', (0.0, -5.0 - tk.REACH, tk.COUPLER_Z))
    obj = k.finish()                                                    # one mesh, hull, sockets, AO, LODs

A Kit gathers parts (looter_props objects whose vertices are already in model space), each mapped as it's made:
tileable sets (metal, paint, crepe) at world scale, trim-sheet parts along their grain, lamp glass on the glass strip
(so swapping the LanternGlow slot for HouseTrim shows dark glass while the train is cold).

THE RAIL CONTRACT (shared with Art/Models/Props/Railway.py and Depot.py): ground z = 0; rail head tops at z = 0.25;
gauge 1.435 m between the rails' inner faces; a car's origin is on the ground under its middle, front toward -Y; wheel
treads touch z = 0.25; the platform is 0.40 m high with its edge 1.65 m from the centreline, so nothing hangs out
past 1.55 m at platform height.
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.geometry import tessellate_polygon

import looter_model as lm
import looter_textures as lt
import looter_props as lp

RAIL_TOP = 0.25
GAUGE = 1.435
RAIL_HEAD = 0.07                     # rail head width: a tread rides over its middle
TREAD_X = (GAUGE + RAIL_HEAD) * 0.5  # 0.7525
BACK_X = 0.68                        # wheel back faces (1.36 m back to back): flanges run just inside the rails
TYRE_W = 0.14                        # wheel width, back face to front face
DRIVER_R = 0.52                      # tread radius of the locomotive's drivers
CARRIAGE_R = 0.42                    # tread radius of carriage (and leading/trailing) wheels
PLATFORM_Z = 0.40
PLATFORM_EDGE = 1.65
CLEAR_X = 1.55                       # the widest anything may hang at platform height
COUPLER_Z = 1.10                     # buffer and coupler centres above the ground
REACH = 0.34                         # buffer heads (the coupler plane) stand this far out of the end beam
CRANK = 0.2                          # the drivers' crank pin radius
ROD_X = BACK_X + TYRE_W + 0.085      # the coupling rods' middle plane (outside the crank bosses)

RYE = 'C:/Dev/AI_Looter_Shooter/Art/Fonts/Rye-Regular.ttf'
FALLBACK_FONT = 'C:/Windows/Fonts/georgiab.ttf'

# The shared material names (the brief's table): one Unreal instance each, for every model that uses the name.
_MATERIALS = {}


def material(key):
    if key not in _MATERIALS:
        if key == 'trim':
            mat = lp.trim_material()
        elif key == 'glow':
            mat = lm.material('LanternGlow', 0xffc56e, Glow=5.0, Variation=0.04)   # looter_buildings' lantern glass
        else:
            spec = {
                'iron': ('MetalWorn', 'IronBlack', 0x2e2c2a), 'brass': ('MetalWorn', 'BrassWorn', 0xc49c56),
                'black': ('PaintWorn', 'PaintBlack', 0x2a2622), 'cream': ('PaintWorn', 'PaintCream', 0xe6dac2),
                'oxide': ('PaintWorn', 'PaintOxide', 0x8c3e2c), 'teal': ('PaintWorn', 'PaintTeal', 0x41706a),
                'crepe': ('Polymer', 'MourningCrepe', 0x161518), 'rust': ('MetalRust', 'MetalRust', None),
                # Painted wood for large panels (the paint sets' chips are kept to small parts).
                'wteal': ('WoodPlanks', 'WoodTeal', WOOD_TINTS['WoodTeal']),
                'wcream': ('WoodPlanks', 'WoodCream', WOOD_TINTS['WoodCream']),
                'wblack': ('WoodPlanks', 'WoodBlack', WOOD_TINTS['WoodBlack']),
                'woxide': ('WoodPlanks', 'WoodOxide', WOOD_TINTS['WoodOxide']),
                # Canvas for gas-bags, sails and rope (the skiff), and plain deck planks (the jetty).
                'canvas': ('Polymer', 'Canvas', CANVAS_TINTS['Canvas']),
                'canvasdark': ('Polymer', 'CanvasDark', CANVAS_TINTS['CanvasDark']),
                'canvasdarkpatch': ('Polymer', 'CanvasDarkPatch', CANVAS_TINTS['CanvasDarkPatch']),
                'planks': ('WoodPlanks', 'WoodPlanks', None),
            }[key]
            mat = lt.material(spec[0], name=spec[1], tint=spec[2]) if spec[2] is not None else lt.material(spec[0])
        _MATERIALS[key] = mat
    return _MATERIALS[key]


# The shared painted-wood tints (this family tunes them; the others use the same values). Tuned by preview on
# 2026-10-02: a faded teal (not turquoise), black paint with the grain showing, a faded oxide red. WoodPlanks is a
# mid-brown set and a tint can only darken it, so WoodCream is the palest it allows: old cream weathered to bare wood.
WOOD_TINTS = {'WoodTeal': 0x7cbcc4, 'WoodCream': 0xfffcf6, 'WoodBlack': 0x48423c, 'WoodOxide': 0xbc7462}
WOOD = ('wteal', 'wcream', 'wblack', 'woxide', 'planks')
BOARD = 0.11                         # painted tongue-and-groove boards' width
# The shared canvas tints (this family tunes them).
CANVAS_TINTS = {'Canvas': 0xd6c9a8, 'CanvasDark': 0x3c3833, 'CanvasDarkPatch': 0x4e4438}


def tileable(key):
    """The texture set a tileable material key maps with (None for the trim sheet and the glow)."""
    return {'iron': 'MetalWorn', 'brass': 'MetalWorn', 'black': 'PaintWorn', 'cream': 'PaintWorn',
            'oxide': 'PaintWorn', 'teal': 'PaintWorn', 'crepe': 'Polymer', 'rust': 'MetalRust',
            'wteal': 'WoodPlanks', 'wcream': 'WoodPlanks', 'wblack': 'WoodPlanks', 'woxide': 'WoodPlanks',
            'planks': 'WoodPlanks', 'canvas': 'Polymer', 'canvasdark': 'Polymer', 'canvasdarkpatch': 'Polymer'}.get(key)


def loft(sections, uvs=None, caps=(True, True)):
    """A part through cross-sections (lists of points with the same count, each a closed loop), quads between
    consecutive sections and capped ends. uvs gives (u, v) per point (same shape), or None. Returns the object."""
    bm = lp.new_bmesh()
    layer = bm.loops.layers.uv.active
    rings = [[bm.verts.new(p) for p in section] for section in sections]
    n = len(sections[0])
    for a in range(len(rings) - 1):
        for j in range(n):
            k = (j + 1) % n
            quad = (rings[a][j], rings[a][k], rings[a + 1][k], rings[a + 1][j])
            try:
                face = bm.faces.new(quad)
            except ValueError:
                continue
            if uvs is not None:
                for loop, (si, pj) in zip(face.loops, ((a, j), (a, k), (a + 1, k), (a + 1, j))):
                    loop[layer].uv = uvs[si][pj]
    for end, ring in ((0, rings[0]), (1, rings[-1])):
        if caps[end] and len(set(v.co.to_tuple(4) for v in ring)) > 2:
            try:
                face = bm.faces.new(ring)
                if uvs is not None:
                    row = uvs[0] if end == 0 else uvs[-1]
                    for loop in face.loops:
                        loop[layer].uv = row[ring.index(loop.vert)]
            except ValueError:
                pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return lp.mesh_object(bm)


def board_uv(obj, width=BOARD, seed=0, upright=True):
    """Maps a part onto WoodPlanks as tongue-and-groove boards width wide (the set's planks are 20 cm, so it's squeezed
    across them), running up the part (upright) or along it: each side face along its own plane, tops and bottoms flat."""
    import random as _random
    rnd = _random.Random(seed)
    info = lt.SETS['WoodPlanks']
    scale = info['density'] / float(info['size'])           # UV units per metre along a board
    across = scale * 0.2 / width
    du, dv = rnd.uniform(0.0, 4.0), rnd.uniform(0.0, 4.0)
    mesh = obj.data
    uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
    up = Vector((0.0, 0.0, 1.0))
    for p in mesh.polygons:
        n = p.normal
        if abs(n.z) < 0.7:
            h = n.cross(up).normalized()
            along, side = (up, h) if upright else (h, up)
        else:
            along, side = Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0))
        for loop in p.loop_indices:
            co = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv[loop].uv = (co.dot(along) * scale + du, co.dot(side) * across + dv)
    return obj


# --- Small geometry helpers ---

def orient(obj, p0, p1):
    """Turns a part built along +Z from the origin so it runs from p0 toward p1 (its length is its own)."""
    d = Vector(p1) - Vector(p0)
    q = Vector((0.0, 0.0, 1.0)).rotation_difference(d.normalized())
    obj.data.transform(Matrix.LocRotScale(Vector(p0), q, None))
    obj.data.update()
    return obj


def prism(loops, depth, matrix):
    """A flat piece with holes: loops of (u, v) points (the outline counterclockwise, then holes), extruded depth
    along +w; the frame matrix maps (u, w, v) -> model space, so with the identity the piece faces -Y like a wall."""
    bm = lp.new_bmesh()
    points = [p for loop in loops for p in loop]
    tris = tessellate_polygon([[Vector((u, v, 0.0)) for u, v in loop] for loop in loops])
    front = [bm.verts.new((u, 0.0, v)) for u, v in points]
    back = [bm.verts.new((u, depth, v)) for u, v in points]
    for a, b, c in tris:
        for face in ((front[a], front[b], front[c]), (back[c], back[b], back[a])):
            try:
                bm.faces.new(face)
            except ValueError:
                pass
    start = 0
    for loop in loops:
        n = len(loop)
        for i in range(n):
            j = (i + 1) % n
            try:
                bm.faces.new((front[start + i], front[start + j], back[start + j], back[start + i]))
            except ValueError:
                pass
        start += n
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = lp.mesh_object(bm)
    obj.data.transform(matrix)
    obj.data.update()
    return obj


def frame(face, at):
    """The matrix that stands a prism()/Kit.panel piece as a wall facing face ('-y', '+y', '+x' or '-x') with its outer
    face through at. Its u runs along x on '-y' walls, along -x on '+y', along +y on '+x' and along -y on '-x' (so
    outlines stay counterclockwise seen from outside); its v is the height."""
    c1, c2 = {'-y': ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0)), '+y': ((-1.0, 0.0, 0.0), (0.0, -1.0, 0.0)),
              '+x': ((0.0, 1.0, 0.0), (-1.0, 0.0, 0.0)), '-x': ((0.0, -1.0, 0.0), (1.0, 0.0, 0.0))}[face]
    return Matrix(((c1[0], c2[0], 0.0, at[0]), (c1[1], c2[1], 0.0, at[1]), (c1[2], c2[2], 1.0, at[2]),
                   (0.0, 0.0, 0.0, 1.0)))


def expand(outline, by):
    """An outline (counterclockwise) grown outward by about by: each corner moved along its bisector."""
    n = len(outline)
    out = []
    for i in range(n):
        p = Vector(outline[i])
        a = (p - Vector(outline[i - 1])).normalized()
        b = (Vector(outline[(i + 1) % n]) - p).normalized()
        na, nb = Vector((a.y, -a.x)), Vector((b.y, -b.x))       # outward normals of a counterclockwise loop
        bis = na + nb
        bis = bis.normalized() if bis.length > 1e-6 else na
        scale = by / max(bis.dot(na), 0.3)
        out.append((p.x + bis.x * scale, p.y + bis.y * scale))
    return out


def arch_points(x0, x1, z_spring, rise, segments=4):
    """The top of a segmental arch from (x1, z_spring) over to (x0, z_spring), counterclockwise order."""
    half = (x1 - x0) * 0.5
    radius = (half * half + rise * rise) / (2.0 * rise)
    cx, cz = (x0 + x1) * 0.5, z_spring + rise - radius
    a0 = math.atan2(z_spring - cz, half)
    pts = []
    for k in range(1, segments):
        a = a0 + (math.pi - 2.0 * a0) * k / segments
        pts.append((cx + math.cos(a) * radius, cz + math.sin(a) * radius))
    return pts


def window_outline(x0, z0, w, h, arch=0.0, segments=4):
    """A window hole's outline (counterclockwise): a rectangle, or round-topped when arch (the rise) > 0."""
    if arch <= 0.0:
        return [(x0, z0), (x0 + w, z0), (x0 + w, z0 + h), (x0, z0 + h)]
    top = z0 + h - arch
    return [(x0, z0), (x0 + w, z0), (x0 + w, top)] + arch_points(x0, x0 + w, top, arch, segments) + [(x0, top)]


def text_mesh(body, size, extrude=0.004, resolution=2, decimate=0.5):
    """Lettering in Rye (the town's sign font), as a mesh lying in XY facing +Z, centred on the origin."""
    path = RYE if os.path.exists(RYE) else FALLBACK_FONT
    font = bpy.data.fonts.load(path, check_existing=True)
    curve = bpy.data.curves.new('_text', 'FONT')
    curve.body, curve.font, curve.size = body, font, size
    curve.align_x, curve.align_y = 'CENTER', 'CENTER'
    curve.extrude = extrude
    curve.resolution_u = resolution
    src = bpy.data.objects.new('_text', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(src.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    obj = bpy.data.objects.new('_part', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.clear()
    if not mesh.uv_layers:
        mesh.uv_layers.new(name='UVMap')
    else:
        mesh.uv_layers[0].name = 'UVMap'
    # Welded (the curve's faces come apart) and merged into as few faces as the letters' outlines allow, then the
    # letters' backs put on z = 0.
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=0.0005)
    bm.to_mesh(mesh)
    bm.free()
    if extrude <= 0.0 and decimate < 0.999:
        # Rye's outlines are busy (spurs, curls): the shortest edges collapse first, which roughly halves the points
        # with no visible change at sign distance.
        mod = obj.modifiers.new('Simplify', 'DECIMATE')
        mod.ratio = decimate
        with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                       selected_editable_objects=[obj]):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    zs = [v.co.z for v in mesh.vertices]
    lo = min(zs) if zs else 0.0
    for v in mesh.vertices:
        v.co.z -= lo
    return obj


# --- The kit ---

AXES = {'x': (1.0, 0.0, 0.0), 'y': (0.0, 1.0, 0.0), 'z': (0.0, 0.0, 1.0)}


class Kit:
    """One model being put together from parts, with its hulls and sockets. Materials are keys: 'iron', 'brass',
    'black', 'cream', 'oxide', 'teal', 'crepe', 'rust' (tileable, mapped at world scale), 'trim' (the house trim sheet:
    give a strip, e.g. 'Beams', 'Siding', 'Iron', 'Glass', 'TrimTeal', 'TrimRed', 'Logs', 'Tin') and 'glow'."""

    def __init__(self, name, seed=1, alias=None):
        self.name = name
        self.rnd = random.Random(seed)
        self.parts = []
        self.hulls = []
        self.sockets = []
        # Shared parts ask for 'iron'; a body can paint them its own way (the hearse car's iron is black paint), which
        # keeps its material slots down.
        self.alias = alias or {}

    def seed(self):
        return self.rnd.randint(0, 10 ** 6)

    def map(self, obj, mat, strip=None, axis=None, slot=None, faces=False):
        """Puts mat on a part (in its own frame, before it's placed) and maps it: tileables by world-scale cube
        projection, trim parts along their grain (axis; faces=True maps each face on its own, for panels), glow and
        glass stretched over the glass strip."""
        mat = self.alias.get(mat, mat)
        if mat == 'glow' or (mat == 'trim' and strip == 'Glass'):
            lt.assign(obj, material(mat))
            lt.trim_uv(obj, None, 'Glass', fit=True)
        elif mat == 'trim':
            if faces:
                lp.trim(obj, strip, seed=self.seed(), slot=slot)
            else:
                lp.grain(obj, strip, axis=axis, seed=self.seed(), slot=slot)
        elif mat in WOOD:
            # Timbers and slats: the grain along the part's longest side.
            lt.assign(obj, material(mat))
            lt.box_uv(obj, 'WoodPlanks', along='long', seed=self.seed())
        else:
            lt.assign(obj, material(mat))
            lt.box_uv(obj, tileable(mat), seed=self.seed())
        self.parts.append(obj)
        return obj

    def box(self, size, center, mat, rot=(0.0, 0.0, 0.0), bevel=0.0, strip=None, axis=None, slot=None, faces=False):
        """A box of size (x, y, z), mapped in its own frame (trim grain along its longest side unless axis), then
        turned (degrees, X then Y then Z) and moved to center."""
        obj = lp.block(size, (0.0, 0.0, 0.0), bevel=bevel)
        if mat == 'trim' and axis is None and not faces:
            axis = AXES['xyz'[max(range(3), key=lambda i: size[i])]]
        elif isinstance(axis, str):
            axis = AXES[axis]
        self.map(obj, mat, strip, axis, slot, faces)
        return lp.place(obj, center, rot)

    def beam(self, p0, p1, w, h, mat, strip=None, up=(0.0, 0.0, 1.0), slot=None):
        """A squared timber or bar from p0 to p1, w across and h deep (h along up)."""
        part = lp.sweep([p0, p1], [(-w * 0.5, -h * 0.5), (w * 0.5, -h * 0.5), (w * 0.5, h * 0.5), (-w * 0.5, h * 0.5)],
                        up=up)
        if mat == 'trim':
            lp.grain(part, strip, axis=Vector(p1) - Vector(p0), seed=self.seed(), slot=slot)
            self.parts.append(part)
            return part
        return self.map(part, mat, strip)

    def _map_round(self, obj, mat, strip, grain):
        mat = self.alias.get(mat, mat)
        if mat == 'glow' or (mat == 'trim' and strip == 'Glass'):
            lt.assign(obj, material(mat))
            lt.trim_uv(obj, None, 'Glass', fit=True)
        elif mat == 'trim':
            lt.assign(obj, material('trim'))
            lp.lathe_uv(obj, strip, grain=grain, seed=self.seed())
        else:
            lt.assign(obj, material(mat))
            # Painted iron shows its sheet's brush lines along the part (rolled plate); brass is spun, so its lines run
            # round. Flat ends are projected flat, not wound into rings.
            set_name = tileable(mat)
            lp.lathe_uv(obj, set_name, grain='up' if mat == 'iron' and grain == 'around' else grain, seed=self.seed())
            lt.box_uv(obj, set_name, faces=lambda f: abs(f.normal.z) > 0.9, seed=self.seed())

    def lathe(self, profile, mat, p0, p1=None, sides=16, closed=False, strip=None, grain='around', spin=0.0):
        """A surface of revolution: profile [(radius, distance along the axis), ...], its axis from p0 toward p1
        (default: straight up). spin turns it about its axis first (degrees), to line up its corners."""
        obj, _ = lp.lathe(profile, segments=sides, closed=closed)
        if spin:
            obj.data.transform(Matrix.Rotation(math.radians(spin), 4, 'Z'))
        self._map_round(obj, mat, strip, grain)
        p0 = Vector(p0)
        p1 = Vector(p1) if p1 is not None else p0 + Vector((0.0, 0.0, 1.0))
        orient(obj, p0, p1)
        self.parts.append(obj)
        return obj

    def cyl(self, p0, p1, r, mat, sides=12, r1=None, caps=(True, True), strip=None, grain='around', spin=0.0):
        """A cylinder (a cone with r1) from p0 to p1, capped at the ends caps asks for."""
        length = (Vector(p1) - Vector(p0)).length
        r1 = r if r1 is None else r1
        profile = ([(0.0, 0.0)] if caps[0] else []) + [(r, 0.0), (r1, length)] + ([(0.0, length)] if caps[1] else [])
        return self.lathe(profile, mat, p0, p1, sides, strip=strip, grain=grain, spin=spin)

    def tube(self, points, r, mat, sides=6, strip=None):
        """A round bar along a polyline (handrails, pipes, rods)."""
        part = lp.sweep(points, lp.ngon(r, sides, start=math.pi / sides))
        if mat == 'trim':
            lp.grain(part, strip, axis=Vector(points[-1]) - Vector(points[0]), seed=self.seed())
            self.parts.append(part)
            return part
        return self.map(part, mat, strip)

    def panel(self, loops, depth, mat, matrix, strip=None, world=True, rotate=False):
        """A flat piece with holes (see prism()), mapped in its own frame: trim strips at world scale over the face
        (cut into bands; rotate=True stands boards upright), tileables by cube projection; then placed by matrix
        (frame() gives the matrix for a wall)."""
        obj = prism(loops, depth, Matrix.Identity(4))
        mat = self.alias.get(mat, mat)
        if mat == 'trim' and strip != 'Glass':
            lt.assign(obj, material('trim'))
            lt.trim_uv(obj, None, strip, align='world' if world else 'face', cut=world, rotate=rotate)
            self.parts.append(obj)
        elif mat in WOOD:
            # Painted tongue-and-groove: boards running up the panel.
            lt.assign(obj, material(mat))
            board_uv(obj, seed=self.seed())
            self.parts.append(obj)
        else:
            self.map(obj, mat, strip)
        obj.data.transform(matrix)
        obj.data.update()
        return obj

    def text(self, body, size, center, rot, mat, extrude=0.0):
        """Rye lettering lying in its own XY plane (facing +Z), turned by rot and moved to center: flat painted letters
        (a hair proud of the board), unless extrude."""
        mat = self.alias.get(mat, mat)
        obj = text_mesh(body, size, extrude, resolution=1)
        lt.assign(obj, material(mat))
        lt.box_uv(obj, tileable(mat) or 'HouseTrim', seed=self.seed())
        self.parts.append(obj)
        return lp.place(obj, center, rot)

    def add(self, obj):
        self.parts.append(obj)
        return obj

    def section(self, label):
        """With --stats after '--', prints the triangles of the parts added since the last section."""
        import sys
        if '--stats' not in sys.argv:
            return
        start = getattr(self, '_counted', 0)
        tris = sum(lp.tri_count(p) for p in self.parts[start:])
        print(f'TRAIN:   {self.name} {label}: {tris}', flush=True)
        self._counted = len(self.parts)

    def socket(self, name, location, rotation=(0.0, 0.0, 0.0)):
        self.sockets.append((name, tuple(location), tuple(rotation)))

    def hull(self, size, center):
        self.hulls.append((tuple(size), tuple(center)))

    def hull_points(self, points):
        """A convex hull around points (a slab that follows a curve, a wedge)."""
        self.hulls.append([Vector(p) for p in points])

    def finish(self, lods='50,20,6', screens='0.5,0.22,0.07', ao=0.6, ground=True, collision=True, smooth=35.0,
               nanite=False, ao_strength=1.0):
        """Joins the parts into the model: triangulated (the same way on every export), soft-shaded with hard edges
        past smooth degrees, its ambient occlusion baked into the vertex colour alpha, Nanite off with its LODs (or
        nanite=True: Nanite on with a full fallback), its hulls (or no collision) and its sockets. Returns the object."""
        import looter_buildings as lb
        obj = lp.join(self.name, self.parts)
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        lb._triangulate(bm)
        bm.to_mesh(obj.data)
        bm.free()
        lm.smooth(obj, smooth)
        lt.bake_vertex_ao(obj, distance=ao, ground=ground, strength=ao_strength)
        if nanite:
            obj['Fallback'] = 100.0
        else:
            obj['Nanite'] = 0
            if lods:
                obj['LODs'] = lods
                obj['LODScreens'] = screens
        for hull in self.hulls:
            if isinstance(hull, list):
                lp.hull_points(obj, hull)
            else:
                lp.hull_box(obj, hull[0], hull[1])
        if not collision:
            obj['Collision'] = 'None'
        for name, location, rotation in self.sockets:
            lm.socket(obj, name, location, rotation)
        print(f"TRAIN: {self.name}: {lp.tri_count(obj)} triangles, {len(obj.data.vertices)} vertices, "
              f"{len(self.hulls)} hulls, {len(self.sockets)} sockets, materials "
              f"{', '.join(m.name for m in obj.data.materials)}", flush=True)
        return obj


# --- Running gear: wheel sets and coupling rods (separate models: the game spins and moves them) ---

def fix_normals(obj):
    """Points a closed part's faces outward (after a mirrored placement)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def sector(k, r0, r1, a0, a1, x0, x1, mat, steps=4):
    """An annular sector (a wheel's counterweight) between radii r0 < r1 and angles a0..a1 (in the YZ plane, from +Y
    toward +Z), from x0 to x1 across."""
    outer = [(math.cos(a0 + (a1 - a0) * i / steps) * r1, math.sin(a0 + (a1 - a0) * i / steps) * r1)
             for i in range(steps + 1)]
    inner = [(math.cos(a1 - (a1 - a0) * i / steps) * r0, math.sin(a1 - (a1 - a0) * i / steps) * r0)
             for i in range(steps + 1)]
    obj = prism([outer + inner], 1.0, Matrix.Identity(4))
    k.map(obj, mat)
    # prism's (u, depth, v) -> (x across, y, z): u becomes y, depth becomes x, v stays z.
    obj.data.transform(Matrix(((0.0, x1 - x0, 0.0, x0), (1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0),
                               (0.0, 0.0, 0.0, 1.0))))
    return fix_normals(obj)


def wheel(k, side, radius, spokes, centre, tyre, hub, crank=None, counterweight=False, sides=16):
    """One wheel on its axle's +X (side 1) or -X (side -1) end, around the wheel set's origin (the axle's middle): a
    tyre with its flange inside the rails and its tread over the rail head's middle, spokes, a hub, and on drivers a
    crank pin (crank: its angle in the YZ plane, from +Y toward +Z) with a counterweight opposite."""
    out = Vector((side, 0.0, 0.0))
    back = out * BACK_X
    # The tyre (its radius over the rail head's middle is the tread radius): flange, coned tread, outer face.
    k.lathe([(radius - 0.07, 0.0), (radius + 0.028, 0.0), (radius + 0.028, 0.016), (radius + 0.004, 0.036),
             (radius - 0.008, TYRE_W), (radius - 0.07, TYRE_W)], tyre, back, back + out, sides=sides, closed=True)
    # Spokes across the middle of the wheel's width, from the hub to the tyre (their buried ends left open).
    start = (crank if crank is not None else 0.0) + math.pi / spokes
    plane = out * (BACK_X + 0.068)
    w = 0.06 if radius > 0.5 else 0.05
    for i in range(spokes):
        a = start + 2.0 * math.pi * i / spokes
        d = Vector((0.0, math.cos(a), math.sin(a)))
        spoke = lp.sweep([plane + d * hub * 0.8, plane + d * (radius - 0.06)],
                         [(-w * 0.5, -0.0225), (w * 0.5, -0.0225), (w * 0.5, 0.0225), (-w * 0.5, 0.0225)],
                         up=(1.0, 0.0, 0.0))
        bm = bmesh.new()
        bm.from_mesh(spoke.data)
        bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if abs(f.normal.dot(d)) > 0.9], context='FACES_ONLY')
        bm.to_mesh(spoke.data)
        bm.free()
        k.map(spoke, centre)
    k.lathe([(0.0, -0.03), (hub, 0.0), (hub, 0.16), (0.0, 0.19)], centre, back, back + out, sides=10)
    if crank is not None:
        c = Vector((0.0, math.cos(crank), math.sin(crank))) * CRANK
        k.cyl(back + c + out * 0.09, back + c + out * 0.15, 0.08, centre, sides=10)
        k.cyl(back + c + out * 0.15, back + c + out * (ROD_X - BACK_X + 0.045), 0.042, tyre, sides=8,
              caps=(False, False))
        k.cyl(back + c + out * (ROD_X - BACK_X + 0.04), back + c + out * (ROD_X - BACK_X + 0.06), 0.055, tyre, sides=8,
              caps=(False, True))
        if counterweight:
            x0, x1 = sorted((side * (BACK_X + 0.045), side * (BACK_X + 0.095)))
            sector(k, radius - 0.24, radius - 0.065, crank + math.pi - 0.62, crank + math.pi + 0.62, x0, x1, centre)


def wheel_set(name, radius, spokes, centre, tyre, cranks=None, journal=0.0, seed=1):
    """A wheel pair on its axle, its origin at the axle's middle (where a body's SOCKET_Axle_N is). cranks gives the
    two crank angles (+X wheel, -X wheel) for drivers. journal > 0 runs the axle out to that half-length (outside
    journal boxes on bogies). No collision: the body's box hull covers the running gear."""
    k = Kit(name, seed)
    for side, crank in ((1.0, cranks[0] if cranks else None), (-1.0, cranks[1] if cranks else None)):
        wheel(k, side, radius, spokes, centre, tyre, 0.1 if radius > 0.5 else 0.085, crank,
              counterweight=cranks is not None)
    reach = max(journal, BACK_X - 0.02)
    k.cyl((-reach, 0.0, 0.0), (reach, 0.0, 0.0), 0.075, tyre, sides=8, caps=(journal > 0.0, journal > 0.0))
    return k.finish(lods='50,20', screens='0.25,0.08', ao=0.25, ground=False, collision=False)


def coupling_rod(name, spacing, seed=1):
    """The coupling rod over three drivers spacing apart, its origin at the middle crank pin. Symmetric across its own
    plane, so one mesh serves both sides (SOCKET_Rod_L and SOCKET_Rod_R)."""
    k = Kit(name, seed)
    k.box((0.055, 2.0 * spacing, 0.085), (0.0, 0.0, 0.0), 'iron', bevel=0.012)
    for y in (-spacing, 0.0, spacing):
        k.cyl((-0.034, y, 0.0), (0.034, y, 0.0), 0.085, 'iron', sides=10)
    return k.finish(lods='50,20', screens='0.2,0.06', ao=0.15, ground=False, collision=False)


# --- Ends: buffers and the coupler (the same on every body) ---

def end_gear(k, y_face, direction, body='iron', head='iron', buffers=True):
    """Two spring buffers and a link-and-pin drawhead on an end beam whose outer face is at y_face; direction is -1 for
    the front end, 1 for the back. The buffer heads end REACH out: the coupler plane, where coupled bodies meet."""
    d = direction
    for x in ((-0.62, 0.62) if buffers else ()):
        k.box((0.26, 0.035, 0.26), (x, y_face + d * 0.0175, COUPLER_Z), body)
        k.cyl((x, y_face + d * 0.035, COUPLER_Z), (x, y_face + d * 0.21, COUPLER_Z), 0.085, body, sides=8,
              caps=(False, True), spin=22.5)
        k.cyl((x, y_face + d * 0.2, COUPLER_Z), (x, y_face + d * (REACH - 0.035), COUPLER_Z), 0.05, head, sides=6,
              caps=(False, False))
        k.lathe([(0.0, 0.0), (0.16, 0.0), (0.16, 0.026), (0.0, 0.04)], head,
                (x, y_face + d * (REACH - 0.04), COUPLER_Z), (x, y_face + d * REACH, COUPLER_Z), sides=10)
    k.box((0.24, 0.3, 0.2), (0.0, y_face + d * 0.15, COUPLER_Z), body, bevel=0.025)
    k.box((0.14, 0.02, 0.09), (0.0, y_face + d * 0.301, COUPLER_Z), 'iron')
    # The link: an oval loop resting in the drawhead's mouth, drooping a little.
    link = k.lathe([(0.075 + 0.018 * math.cos(a), 0.018 * math.sin(a)) for a in (2.0 * math.pi * i / 3 for i in range(3))],
                   body, (0.0, 0.0, 0.0), sides=8, closed=True, spin=22.5)
    link.data.transform(Matrix.Diagonal((1.0, 1.75, 1.0, 1.0)))
    lp.place(link, (0.0, y_face + d * 0.36, COUPLER_Z - 0.03), (d * 12.0, 0.0, 0.0))
    k.cyl((0.0, y_face + d * 0.22, COUPLER_Z + 0.08), (0.0, y_face + d * 0.22, COUPLER_Z + 0.19), 0.025, body, sides=6)


# --- Bogies (part of the bodies) ---

def bogie(k, y, wheelbase=1.7, mat='iron', boxes='iron'):
    """An arch-bar truck centred at y, axles at y +- wheelbase / 2 (carriage wheel sets, journals outside the wheels):
    iron side frames, journal boxes and spring nests, the bolster. Returns the two axle positions."""
    zc = RAIL_TOP + CARRIAGE_R
    half = wheelbase * 0.5
    for x in (-0.99, 0.99):
        k.beam((x, y - half - 0.16, zc + 0.17), (x, y + half + 0.16, zc + 0.17), 0.06, 0.07, mat)
        k.tube([(x, y - half - 0.05, zc + 0.12), (x, y - 0.42, zc - 0.17), (x, y + 0.42, zc - 0.17),
                (x, y + half + 0.05, zc + 0.12)], 0.032, mat, sides=4)
        for s in (-1.0, 1.0):
            k.box((0.075, 0.07, 0.42), (x, y + s * 0.3, zc), mat)
            k.box((0.2, 0.26, 0.26), (x + math.copysign(0.02, x), y + s * half, zc), boxes, bevel=0.02)
            k.box((0.05, 0.2, 0.18), (x + math.copysign(0.12, x), y + s * half, zc), boxes)
        k.box((0.16, 0.46, 0.2), (x, y, zc - 0.07), boxes)
    k.box((2.1, 0.3, 0.16), (0.0, y, zc + 0.2), mat)
    for s in (-1.0, 1.0):
        # Brake beams inside the wheels, with shoes on the treads.
        k.box((1.3, 0.08, 0.08), (0.0, y + s * (half - 0.47), zc - 0.02), mat)
        for x in (-0.75, 0.75):
            k.box((0.09, 0.06, 0.26), (x, y + s * (half - 0.445), zc), mat)
    return [(0.0, y - half, zc), (0.0, y + half, zc)]


# --- Steps, railings, lamps ---

def corner_steps(k, x_out, y_mid, top, levels=(0.98, 0.7), width=0.5):
    """Steps hanging under a floor edge at a car's side: iron hangers and wooden treads, stepping out toward x_out's
    side (its sign), the bottom tread's outer edge at x_out."""
    s = math.copysign(1.0, x_out)
    n = len(levels)
    for i, z in enumerate(levels):
        inset = 0.12 * (n - 1 - i)
        k.box((0.26, width, 0.045), (x_out - s * (0.13 + inset), y_mid, z - 0.0225), 'trim', strip='Beams', axis='y')
    bottom = levels[-1] - 0.05
    for dy in (-width * 0.5 - 0.02, width * 0.5 + 0.02):
        k.beam((x_out - s * (0.12 * n + 0.1), y_mid + dy, top), (x_out - s * 0.03, y_mid + dy, bottom), 0.025, 0.05,
               'iron', up=(0.0, 1.0, 0.0))


def railing(k, points, height, mat='iron', mid=True, r=0.02, posts=True):
    """A railing of square bar along points (on its floor): a top rail at height, a middle rail, posts at the points."""
    k.tube([(p[0], p[1], p[2] + height) for p in points], r, mat, sides=4)
    if mid:
        k.tube([(p[0], p[1], p[2] + height * 0.5) for p in points], r * 0.8, mat, sides=4)
    if posts:
        for p in points:
            k.tube([p, (p[0], p[1], p[2] + height + r)], r * 1.1, mat, sides=4)


def band(k, outer, inner, mat, matrix, strip=None):
    """A flat, one-sided band between two outlines with the same number of corners (a window's etched border), facing
    -Y in its frame (u, v), placed by matrix."""
    bm = lp.new_bmesh()
    o = [bm.verts.new((u, 0.0, v)) for u, v in outer]
    i = [bm.verts.new((u, 0.0, v)) for u, v in inner]
    n = len(outer)
    for a in range(n):
        b = (a + 1) % n
        face = bm.faces.new((o[a], i[a], i[b], o[b]))
        face.normal_update()
        if face.normal.y > 0.0:
            face.normal_flip()
    obj = lp.mesh_object(bm)
    k.map(obj, mat, strip, faces=(mat == 'trim' and strip != 'Glass'))
    obj.data.transform(matrix)
    return obj


def flat(k, points, mat, matrix, strip=None):
    """A flat, one-sided polygon (u, v) facing -Y in its frame, placed by matrix (an etched fan, a plate)."""
    bm = lp.new_bmesh()
    face = bm.faces.new([bm.verts.new((u, 0.0, v)) for u, v in points])
    face.normal_update()
    if face.normal.y > 0.0:
        face.normal_flip()
    obj = lp.mesh_object(bm)
    k.map(obj, mat, strip, faces=(mat == 'trim' and strip != 'Glass'))
    obj.data.transform(matrix)
    return obj


def quad(k, corners, mat, strip=None):
    """A single face through four corners (counterclockwise seen from the side it faces): glass panes, crepe."""
    bm = lp.new_bmesh()
    bm.faces.new([bm.verts.new(c) for c in corners])
    obj = lp.mesh_object(bm)
    return k.map(obj, mat, strip, faces=True) if mat == 'trim' and strip != 'Glass' else k.map(obj, mat, strip)


def slab(k, outline, z0, z1, mat, strip=None, axis=None):
    """A flat piece lying down: outline [(x, y), ...] counterclockwise from above, from z0 up to z1."""
    bm = lp.new_bmesh()
    low = [bm.verts.new((x, y, z0)) for x, y in outline]
    high = [bm.verts.new((x, y, z1)) for x, y in outline]
    bm.faces.new(list(reversed(low)))
    bm.faces.new(high)
    n = len(outline)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((low[i], low[j], high[j], high[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return k.map(lp.mesh_object(bm), mat, strip, axis=axis)


def window(k, face, at, outline, depth, frame_mat='oxide', frame_w=0.07, frame_t=0.03, glass=True, sill=True,
           glass_at=0.5, frame_strip=None):
    """The fittings of a window whose hole (outline, counterclockwise from outside) is cut in a wall depth thick:
    dark glass set glass_at of the way into the wall, a frame standing frame_t proud around it, and a sill under it."""
    m = frame(face, at)
    if glass:
        k.panel([outline], 0.01, 'trim', m @ Matrix.Translation((0.0, depth * glass_at, 0.0)), strip='Glass')
    if frame_w > 0.0:
        k.panel([expand(outline, frame_w), list(outline)], frame_t, frame_mat, m @ Matrix.Translation((0.0, -frame_t, 0.0)),
                strip=frame_strip, world=False)
    if sill:
        us = [p[0] for p in outline]
        z0 = min(p[1] for p in outline)
        width = max(us) - min(us) + 2.0 * frame_w + 0.06
        sill_part = lp.block((width, 0.07, 0.045), ((min(us) + max(us)) * 0.5, -0.035 - frame_t * 0.5, z0 - frame_w - 0.0225))
        k.map(sill_part, frame_mat, frame_strip, axis=(1.0, 0.0, 0.0))
        sill_part.data.transform(m)


def grab_rail(k, x, y, z0, z1, mat='brass', stand=0.06):
    """A vertical grab handle standing off a wall at x (outward by stand, toward x's sign), from z0 to z1."""
    s = math.copysign(1.0, x)
    k.tube([(x - s * 0.01, y, z0), (x + s * stand, y, z0 + 0.04), (x + s * stand, y, z1 - 0.04), (x - s * 0.01, y, z1)],
           0.016, mat, sides=5)


def oil_lamp(k, at, facing=(0.0, -1.0, 0.0), size=0.2, body='brass', socket=None, chimney=True, glass='glow',
             strip=None):
    """A small square oil lamp standing at at (its base): a brass body (or body on a trim strip), lit glass (glass=
    ('trim', 'Glass') for an unlit one), a cap and chimney, and a lens boss on the side it faces. SOCKET_<socket> at
    the glass, facing the same way."""
    x, y, z = at
    s = size
    k.box((s * 1.1, s * 1.1, s * 0.16), (x, y, z + s * 0.08), body, strip=strip, axis='x')
    if isinstance(glass, tuple):
        k.box((s * 0.82, s * 0.82, s * 0.95), (x, y, z + s * 0.635), glass[0], strip=glass[1])
    else:
        k.box((s * 0.82, s * 0.82, s * 0.95), (x, y, z + s * 0.635), glass)
    for dx in (-0.45, 0.45):
        for dy in (-0.45, 0.45):
            k.box((s * 0.1, s * 0.1, s), (x + dx * s, y + dy * s, z + s * 0.66), body, strip=strip)
    k.cyl((x, y, z + s * 1.12), (x, y, z + s * 1.42), s * 0.62, body, r1=s * 0.2, sides=8, spin=22.5, caps=(True, False),
          strip=strip)
    if chimney:
        k.cyl((x, y, z + s * 1.4), (x, y, z + s * 1.75), s * 0.12, body, sides=6, caps=(False, True), strip=strip)
    f = Vector(facing).normalized()
    k.lathe([(0.0, 0.0), (s * 0.32, 0.0), (s * 0.32, s * 0.08), (0.0, s * 0.1)], body,
            (x + f.x * s * 0.42, y + f.y * s * 0.42, z + s * 0.62), (x + f.x * s, y + f.y * s, z + s * 0.62), sides=8,
            strip=strip)
    if socket:
        k.socket(socket, (x, y, z + s * 0.64), (0.0, 0.0, math.degrees(math.atan2(f.x, -f.y))))


# --- Previews (only with --preview; everything they add is removed again, so nothing reaches an export) ---

PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Train')


def _plain(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 0.9
    return mat


def preview_track(y0, y1, name='PV_Track', platform=None):
    """Plain track for previews only: two rails (heads at z 0.25, 1.435 m apart inside) on sleepers, and, if platform
    is given (a y range), a 0.40 m platform block with its edge 1.65 m out on the +X side."""
    k = Kit(name, 99)
    length, mid = y1 - y0, (y0 + y1) * 0.5
    for s in (-1.0, 1.0):
        x = s * TREAD_X
        k.box((RAIL_HEAD, length, 0.05), (x, mid, RAIL_TOP - 0.025), 'iron')
        k.box((0.024, length, 0.07), (x, mid, RAIL_TOP - 0.085), 'iron')
        k.box((0.14, length, 0.022), (x, mid, RAIL_TOP - 0.131), 'iron')
    y = y0 + 0.3
    while y < y1 - 0.2:
        k.box((2.5, 0.22, 0.14), (0.0, y, 0.05), 'trim', strip='Beams', axis='x')
        y += 0.62
    parts = k.parts
    if platform is not None:
        slab = lp.block((3.0, platform[1] - platform[0], PLATFORM_Z), (PLATFORM_EDGE + 1.5, sum(platform) * 0.5,
                                                                       PLATFORM_Z * 0.5))
        lt.assign(slab, _plain('PV_Platform', 0x8a7a66))
        parts.append(slab)
    obj = lp.join(name, parts)
    lm.smooth(obj, 30.0)
    return obj


def attach(body, models):
    """Copies of the separate models (wheel sets, rods, the door) placed at the body's sockets, for the render:
    models maps a socket name (without SOCKET_) to a model object. Returns the copies."""
    bpy.context.view_layer.update()
    copies = []
    for child in body.children:
        if not child.name.startswith('SOCKET_'):
            continue
        key = child.name[len('SOCKET_'):].split('.')[0]
        model = models.get(key)
        if model is None:
            continue
        copy = bpy.data.objects.new('PV_' + model.name, model.data)
        bpy.context.scene.collection.objects.link(copy)
        copy.matrix_world = child.matrix_world.copy()
        copies.append(copy)
    return copies


def render(objects, name, view=(-1.0, -1.6, 0.6), lens=40.0, fit=1.05, resolution=(1600, 900), folder=None,
           ground_at=0.0):
    """lt.preview into Saved/ArtPreviews/RansomsRest/Train/<name>.png (or folder) with the ground at ground_at."""
    out = os.path.join(folder or PREVIEW_DIR, name + '.png')
    return lt.preview(objects, out, resolution=resolution, view=view, lens=lens, fit=fit, ground_at=ground_at)


def closeup(objects, name, target, direction, distance, lens=50.0, resolution=(1600, 900), folder=None):
    """A close view of objects (others hidden) from target + direction * distance, lit like lt.preview: the same sky, a
    warm sun from the camera's upper left, a ground plane at z = 0."""
    scene = bpy.context.scene
    out = os.path.join(folder or PREVIEW_DIR, name + '.png')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    shown = lt._preview_objects(objects)
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        if o not in shown:
            o.hide_render = True
    d = Vector(direction).normalized()
    cam = bpy.data.objects.new('_CloseCam', bpy.data.cameras.new('_CloseCam'))
    cam.data.lens = lens
    cam.location = Vector(target) + d * distance
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(cam)
    turn = math.atan2(d.y, d.x) - math.atan2(-1.6, -1.0)
    base = Vector((-0.55, -0.7, 0.0))
    toward = Vector((base.x * math.cos(turn) - base.y * math.sin(turn), base.x * math.sin(turn) + base.y * math.cos(turn), 0.0))
    toward = (toward.normalized() * math.cos(math.radians(45.0)) + Vector((0.0, 0.0, math.sin(math.radians(45.0))))).normalized()
    sun = bpy.data.objects.new('_CloseSun', bpy.data.lights.new('_CloseSun', 'SUN'))
    sun.data.energy, sun.data.color, sun.data.angle = 4.0, (1.0, 0.9, 0.78), math.radians(2.0)
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    mesh = bpy.data.meshes.new('_CloseGround')
    mesh.from_pydata([(-500, -500, 0), (500, -500, 0), (500, 500, 0), (-500, 500, 0)], [], [(0, 1, 2, 3)])
    mesh.materials.append(_plain('_PreviewGround2', 0x77766c))
    ground = bpy.data.objects.new('_CloseGround', mesh)
    scene.collection.objects.link(ground)
    world = lt._preview_world(scene)
    saved = (scene.camera, scene.world, scene.render.engine, scene.render.filepath)
    try:
        scene.camera, scene.world = cam, world
        scene.render.resolution_x, scene.render.resolution_y = resolution
        scene.render.resolution_percentage = 100
        scene.render.image_settings.file_format = 'PNG'
        scene.render.filepath = out
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = 32
        bpy.ops.render.render(write_still=True)
    finally:
        scene.camera, scene.world, scene.render.engine, scene.render.filepath = saved
        bpy.data.worlds.remove(world)
        for o in (cam, sun, ground):
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
            elif isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    print(f'TRAIN: closeup {out}', flush=True)
    return out


def label(body, size, at, rot=(90.0, 0.0, 0.0)):
    """A preview caption (a mesh in Rye, dark), standing at at facing -Y."""
    obj = text_mesh(body, size, extrude=0.01)
    obj.name = 'PV_Label'
    obj.data.materials.append(_plain('PV_Label', 0x2b2622))
    lp.place(obj, at, rot)
    return obj


def decimated(obj, ratio, name):
    """A copy of obj reduced to about ratio of its triangles, roughly as the importer's LODs (Unreal's quadric
    reduction) make them: at strong reductions the smallest loose parts collapse away first, then edges collapse."""
    copy = bpy.data.objects.new(name, obj.data.copy())
    bpy.context.scene.collection.objects.link(copy)
    if ratio >= 0.999:
        return copy
    target = lp.tri_count(obj) * ratio
    if ratio < 0.4:
        bm = bmesh.new()
        bm.from_mesh(copy.data)
        islands = []
        seen = set()
        for f in bm.faces:
            if f in seen:
                continue
            stack, island = [f], []
            seen.add(f)
            while stack:
                g = stack.pop()
                island.append(g)
                for e in g.edges:
                    for h in e.link_faces:
                        if h not in seen:
                            seen.add(h)
                            stack.append(h)
            pts = [v.co for g in island for v in g.verts]
            size = (Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))) -
                    Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))).length
            islands.append((size, island))
        islands.sort(key=lambda item: item[0])
        total = sum(len(i) for _, i in islands)
        gone = []
        for size, island in islands:
            if total - len(island) < target * 1.6 or size > 1.2:
                break
            gone += island
            total -= len(island)
        bmesh.ops.delete(bm, geom=gone, context='FACES')
        bm.to_mesh(copy.data)
        bm.free()
    current = lp.tri_count(copy)
    if current > target:
        mod = copy.modifiers.new('LOD', 'DECIMATE')
        mod.ratio = max(target / current, 0.01)
        mod.use_collapse_triangulate = True
        with bpy.context.temp_override(object=copy, active_object=copy, selected_objects=[copy],
                                       selected_editable_objects=[copy]):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    return copy


def lod_sheet(body, attached, name, shares=(1.0, 0.5, 0.25, 0.1), part_shares=(1.0, 0.5, 0.2, 0.2), gap=2.5):
    """Renders the body's LODs side by side (each turned to show its side, attached models reduced along with it),
    captioned with their triangle counts, into <name>.png. Returns the counts."""
    shown, counts = [], []
    x = 0.0
    lengths = [v.co.y for v in body.data.vertices]
    length = max(lengths) - min(lengths)
    for i, (share, part_share) in enumerate(zip(shares, part_shares)):
        copy = decimated(body, share, f'PV_LOD{i}')
        tris = lp.tri_count(copy)
        counts.append(tris)
        group = [copy]
        for model, matrix in attached:
            part = decimated(model, part_share, f'PV_LOD{i}_{model.name}')
            part.matrix_world = matrix
            part.parent = copy
            part.matrix_parent_inverse = copy.matrix_world.inverted()
            group.append(part)
        copy.rotation_euler = (0.0, 0.0, math.radians(90.0))
        copy.location = (x + length * 0.5, 0.0, 0.0)
        shown += group
        shown.append(label(f'LOD{i}  {tris:,} tris', 0.6, (x + length * 0.5, 0.0, 4.75), (90.0, 0.0, 0.0)))
        x += length + gap
    bpy.context.view_layer.update()
    path = render(shown, name, view=(0.0, -1.0, 0.2), lens=50.0, fit=0.36, resolution=(1920, 640))
    for o in shown:
        data = o.data
        bpy.data.objects.remove(o)
        if data is not None and data.users == 0:
            bpy.data.meshes.remove(data)
    return counts, path


def remove(objects):
    for o in objects:
        data = o.data
        bpy.data.objects.remove(o)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)
