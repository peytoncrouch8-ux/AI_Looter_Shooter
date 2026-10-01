"""Building blocks for scripted guns, in the gun's own space and in centimeters, so a gun reads like its blueprint:

    u   along the gun, 0 at the back of the receiver, + toward the muzzle (Blender -Y, Unreal +X)
    v   up, 0 on the bore axis
    w   across, + to the gun's left (Blender +X; the right side, with the ejection port, faces Blender -X)

Shapes are side outlines extruded across the gun with rounded edges (slab), parts turned round the bore or another
axis along u (turned, tube), pins and drums across it (pin), swept strips and pipes (strip, pipe), Picatinny rails
(rail), and boolean cuts for ports, slots and holes (cut with cutters from cbox, cslab, cpin). Every part is mapped
at the gun's texel density (1024 px/m) on its material's texture set. A Gun collects parts into its models (Body,
Magazine, Sight ...) and finishes them: shading, baked occlusion, sockets, no Nanite.

    import looter_guns as lg
    gun = lg.Gun('Regulator')
    gun.add('Body', lg.slab([(0, -1.6), (27, -1.6), (27, 2.2), (0, 2.2)], 2.9, mat=lg.BLACK, bevel=0.25))
    gun.add('Magazine', ...)
    gun.socket('Body', 'Muzzle', (78, 0))
    gun.build(origins={'Magazine': (17, -3)})
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp

CM = 0.01
DENSITY = 1024.0   # px per meter: guns are seen up close


def at(u, v=0.0, w=0.0):
    return Vector((w * CM, -u * CM, v * CM))


# --- Materials: tinted texture sets, so one worn-metal, one polymer and one walnut texture make every gun ---

SET_OF = {}


def material(name, set_name, tint=None, uv_scale=1.0):
    # The Gun master: the World master's look plus per-gun wear (each gun rolls how battered it is).
    mat = lt.material(set_name, name=name, tint=tint, uv_scale=uv_scale, master='Gun')
    SET_OF[mat.name] = set_name
    return mat


def glow(name, color, strength=3.0):
    """An opaque emissive surface (sight reticles, accent strips)."""
    return lm.material(name, color, Glow=strength, Variation=0.0)


BLUED = material('GunSteelBlued', 'MetalWorn', 0x3b4048)
BLACK = material('GunBlackMetal', 'MetalWorn', 0x2c2d30)
STEEL = material('GunSteelBare', 'MetalWorn', 0x75777a)
BRASS = material('GunBrass', 'MetalWorn', 0xf0bd5c)
WALNUT = material('GunWalnut', 'GunWood')
POLY_BLACK = material('GunPolymerBlack', 'Polymer', 0x38393b)
POLY_GREY = material('GunPolymerGrey', 'Polymer', 0x6a6d70)
POLY_TAN = material('GunPolymerTan', 'Polymer', 0xc9ad84)
POLY_SAND = material('GunPolymerSand', 'Polymer', 0xe6d8b6)
POLY_OLIVE = material('GunPolymerOlive', 'Polymer', 0x6f7550)
TAPE = material('GunTape', 'Polymer', 0x8a765a)
RUBBER = material('GunRubber', 'Polymer', 0x262626)
SALVAGE = material('GunSalvage', 'MetalRust', 0x93aaa6, uv_scale=2.0)
def glass(name, tint, opacity=0.15):
    """See-through glass (lenses, sight windows) on the Glass master, so a sight can be aimed through."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    rgba = lm.hex_color(tint)
    bsdf = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = rgba
    bsdf.inputs['Alpha'].default_value = opacity
    mat.diffuse_color = rgba[:3] + [opacity]
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Glass'
    mat['Tint'] = f'#{tint:06x}'
    mat['Opacity'] = opacity
    return mat


LENS = glass('GunLens', 0x3d5868)


def mapped(obj, mat):
    """Puts mat on every face and maps it at the gun's texel density."""
    if mat is None:
        return obj
    lt.assign(obj, mat)
    set_name = SET_OF.get(mat.name)
    if set_name:
        lt.box_uv(obj, set_name, texel_density=DENSITY)
    return obj


# --- Shapes ---

def fillet(outline, radius, steps=3):
    """Rounds every corner of an outline into a curve about radius cm across (less where its edges are short), so
    stocks, grips and shells read molded or carved rather than cut from a board."""
    if radius <= 0.0:
        return outline
    pts = [Vector((u, v)) for u, v in outline]
    out = []
    for i, p1 in enumerate(pts):
        p0, p2 = pts[i - 1], pts[(i + 1) % len(pts)]
        d0, d1 = p0 - p1, p2 - p1
        l0, l1 = d0.length, d1.length
        if l0 < 1e-6 or l1 < 1e-6:
            out.append(p1)
            continue
        d0, d1 = d0 / l0, d1 / l1
        angle = math.acos(max(-1.0, min(1.0, d0.dot(d1))))
        if angle > math.radians(172.0):
            out.append(p1)
            continue
        t = min(radius / math.tan(angle * 0.5), l0 * 0.45, l1 * 0.45)
        a, b = p1 + d0 * t, p1 + d1 * t
        for k in range(steps + 1):
            s = k / steps
            out.append((1 - s) ** 2 * a + 2 * (1 - s) * s * p1 + s * s * b)
    return [(p.x, p.y) for p in out]


def slab(outline, width, w=0.0, bevel=0.3, segments=2, mat=None, sharp=28.0, round=0.0):
    """A side outline [(u, v), ...] (cm, counterclockwise or not) extruded across the gun, width cm centered on w; edges
    sharper than `sharp` degrees are rounded by bevel cm, and the outline's corners by round cm (fillet)."""
    outline = fillet(outline, round)
    bm = lp.new_bmesh()
    w0, w1 = w - width * 0.5, w + width * 0.5
    a = [bm.verts.new(at(u, v, w0)) for u, v in outline]
    b = [bm.verts.new(at(u, v, w1)) for u, v in outline]
    n = len(outline)
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[i], a[j], b[j], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bevel > 0.0:
        edges = [e for e in bm.edges if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > math.radians(sharp)]
        bmesh.ops.bevel(bm, geom=edges, offset=bevel * CM, segments=segments, affect='EDGES', clamp_overlap=True,
                        profile=0.5)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    return mapped(lp.mesh_object(bm), mat)


def box(u0, u1, v0, v1, width, w=0.0, bevel=0.2, mat=None, segments=2):
    return slab([(u0, v0), (u1, v0), (u1, v1), (u0, v1)], width, w, bevel, segments, mat)


def _lathe(profile, segments, spin):
    """A closed surface of revolution round Z from [(z cm, r cm), ...], ends capped."""
    pts = [(r * CM, z * CM) for z, r in profile]
    if pts[0][0] > 1e-6:
        pts.insert(0, (0.0, pts[0][1]))
    if pts[-1][0] > 1e-6:
        pts.append((0.0, pts[-1][1]))
    obj = lp.lathe(pts, segments=segments)[0]
    if spin:
        obj.data.transform(Matrix.Rotation(math.radians(spin), 4, 'Z'))
    return obj


def turned(profile, u0, v=0.0, w=0.0, segments=16, mat=None, spin=0.0):
    """A part turned round an axis along u: profile [(du, r), ...] in cm from u0 (a muzzle device, a scope tube, an
    octagonal handguard with segments=8 and spin=22.5)."""
    obj = _lathe(profile, segments, spin)
    obj.data.transform(Matrix.Translation(at(u0, v, w)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return mapped(obj, mat)


def tube(u0, u1, r, v=0.0, w=0.0, segments=16, mat=None, r1=None, spin=0.0):
    return turned([(0.0, r), (u1 - u0, r if r1 is None else r1)], u0, v, w, segments, mat, spin)


def pin(u, v, w0, w1, r, mat=None, segments=12, profile=None):
    """A cylinder across the gun (a pin, a drum, a knob), from w0 to w1; profile [(dw, r), ...] overrides r."""
    obj = _lathe(profile or [(0.0, r), (w1 - w0, r)], segments, 0.0)
    obj.data.transform(Matrix.Translation(at(u, v, w0)) @ Matrix.Rotation(math.radians(90.0), 4, 'Y'))
    return mapped(obj, mat)


def upright(u, v0, v1, r, w=0.0, mat=None, segments=12, profile=None):
    """A cylinder standing along v (a vertical grip, a turret knob) from v0 to v1."""
    obj = _lathe(profile or [(0.0, r), (v1 - v0, r)], segments, 0.0)
    obj.data.transform(Matrix.Translation(at(u, v0, w)))
    return mapped(obj, mat)


def strip(points, across, thick, w=0.0, mat=None):
    """A flat strip swept along side-view points [(u, v), ...]: across cm wide across the gun, thick cm (a trigger
    guard, a sling loop)."""
    a, t = across * CM * 0.5, thick * CM * 0.5
    obj = lp.sweep([at(u, v, w) for u, v in points], [(-t, -a), (t, -a), (t, a), (-t, a)], up=(1.0, 0.0, 0.0))
    return mapped(obj, mat)


def pipe(points, r, w=0.0, mat=None, sides=8):
    """A round bar swept along side-view points (a bent handle, a skeleton stock)."""
    obj = lp.sweep([at(*p) if len(p) == 3 else at(p[0], p[1], w) for p in points], lp.ngon(r * CM, sides),
                   up=(1.0, 0.0, 0.0))
    return mapped(obj, mat)


def rail(u0, u1, v, width=2.1, mat=None, pitch=1.0, down=False):
    """A Picatinny rail on a surface at height v: a base and its cross-slotted teeth (down=True: under a surface,
    teeth pointing down)."""
    s = -1.0 if down else 1.0
    parts = [box(u0, u1, min(v, v + s * 0.45), max(v, v + s * 0.45), width, bevel=0.08, mat=mat, segments=1)]
    u = u0 + 0.35
    while u + 0.55 <= u1 - 0.2:
        a, b = v + s * 0.4, v + s * 0.95
        parts.append(box(u, u + 0.53, min(a, b), max(a, b), width, bevel=0.07, mat=mat, segments=1))
        u += pitch
    return lp.join('_rail', parts)


def rivet(u, v, w, side=1.0, r=0.22, mat=None):
    """A small dome on a side face (side +1: the gun's left, -1: its right)."""
    obj = _lathe([(0.0, r), (r * 0.35, r * 0.8), (r * 0.6, 0.0)], 8, 0.0)
    rot = Matrix.Rotation(math.radians(90.0 * side), 4, 'Y')
    obj.data.transform(Matrix.Translation(at(u, v, w)) @ rot)
    return mapped(obj, mat)


def curved_outline(front, rear, steps=10):
    """An outline between two edge curves, each a quadratic Bezier [(u, v) start, control, end] (start at the top):
    the front edge down, then the rear edge back up (magazines, grips)."""
    def bez(p, t):
        (a, b), (c, d), (e, f) = p
        return ((1 - t) ** 2 * a + 2 * (1 - t) * t * c + t * t * e, (1 - t) ** 2 * b + 2 * (1 - t) * t * d + t * t * f)
    fr = [bez(front, i / steps) for i in range(steps + 1)]
    rr = [bez(rear, i / steps) for i in range(steps + 1)]
    return fr + list(reversed(rr))


# --- Cutting ---

def cbox(u0, u1, v0, v1, w0, w1, rotate_u=0.0):
    """A box cutter; rotate_u turns it about the bore axis (degrees)."""
    obj = lp.block(((w1 - w0) * CM, (u1 - u0) * CM, (v1 - v0) * CM), at((u0 + u1) * 0.5, (v0 + v1) * 0.5, (w0 + w1) * 0.5))
    if rotate_u:
        obj.data.transform(Matrix.Rotation(math.radians(rotate_u), 4, 'Y'))
    return obj


def cslab(outline, w0, w1, round=0.0):
    return slab(outline, w1 - w0, (w0 + w1) * 0.5, bevel=0.0, round=round)


def cpin(u, v, w0, w1, r, segments=12):
    return pin(u, v, w0, w1, r, segments=segments)


def cut(obj, cutters):
    """Boolean-subtracts the cutters from obj (they're removed), then maps obj again. The exact solver now and then
    returns nothing for a heavily beveled part; then the fast solver is tried, and if that fails too the part stays
    uncut (with a warning) rather than vanishing."""
    original = obj.data.copy()
    count = len(original.vertices)
    for solver in ('EXACT', 'FAST'):
        mods = []
        for c in cutters:
            m = obj.modifiers.new('cut', 'BOOLEAN')
            m.operation, m.solver, m.object = 'DIFFERENCE', solver, c
            c.hide_render = c.hide_viewport = True
            mods.append(m)
        with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                       selected_editable_objects=[obj]):
            for m in mods:
                bpy.ops.object.modifier_apply(modifier=m.name)
        if len(obj.data.vertices) >= count * 0.6:
            break
        broken = obj.data
        obj.data = original.copy()
        bpy.data.meshes.remove(broken)
    else:
        lt._log(f'warning: a cut on {obj.name} failed with both solvers; the part stays uncut')
    bpy.data.meshes.remove(original)
    for c in cutters:
        mesh = c.data
        bpy.data.objects.remove(c)
        bpy.data.meshes.remove(mesh)
    mat = obj.data.materials[0] if obj.data.materials else None
    for p in obj.data.polygons:
        p.material_index = 0
    return mapped(obj, mat)


# --- Assembly ---

class Gun:
    """Parts by model name; build() joins each model, puts its origin where asked and finishes it."""

    def __init__(self, name):
        self.name = name
        self.parts = {}
        self.sockets = []

    def add(self, model, *objs):
        for o in objs:
            if o is not None:
                self.parts.setdefault(model, []).append(o)
        return objs[0] if len(objs) == 1 else objs

    def socket(self, model, name, uv, w=0.0, rotation=(0.0, 0.0, 0.0)):
        self.sockets.append((model, name, at(uv[0], uv[1], w), rotation))

    def build(self, origins=None):
        """Joins the models (named <Gun><Model>), each with its origin at origins[model] (u, v) or the gun's origin,
        placed where it belongs; returns {model: object}."""
        origins = origins or {}
        built = {}
        for model, parts in self.parts.items():
            obj = lp.join(self.name + model, parts)
            origin = at(*origins[model]) if model in origins else Vector()
            obj.data.transform(Matrix.Translation(-origin))
            obj.location = origin
            lm.smooth(obj, 30.0)
            lt.bake_vertex_ao(obj, samples=16, distance=0.04, ground=False)
            obj['Nanite'] = 0
            built[model] = obj
        for model, name, location, rotation in self.sockets:
            owner = built[model]
            lm.socket(owner, name, location - owner.location, rotation)
        return built
