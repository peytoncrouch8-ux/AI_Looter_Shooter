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


# --- Frontier-tech pieces (2026-10-21: the Rustler SMG, Thresher LMG, Longhorn sniper and Powderkeg launcher) ---
# The look the user picked: sand polymer shells (POLY_SAND, tinted per gun), gunmetal (BLUED, BLACK), brass fittings,
# ember-orange glow and cyan optics. The two glows the game drives carry UV0.x along their part (along_uv), so the game
# can light them part of the way: GunAmmoGlow (the rounds left) and GunHeatGlow (barrel heat).

RETICLE = glow('GunReticle', 0xff3b2e, 6.0)          # as every gun's: sight dots and crosses
EMBER = glow('GunEmberGlow', 0xff7a24, 4.0)          # fixed ember glow: vents, lines, a loaded rocket's tip
CYAN = glow('GunCyanGlow', 0x56dcff, 5.0)            # optics: LEDs, objective rings
AMMO_GLOW = glow('GunAmmoGlow', 0xff8a2e, 4.0)       # the rounds left: lit from UV0.x 0 up to the share left
HEAT_GLOW = glow('GunHeatGlow', 0xff5a1e, 3.0)       # barrel heat: lit by the game as the gun heats
COPPER = material('GunCopper', 'MetalWorn', 0xc9794a)   # bullet jackets (the Drover's)


def along_uv(obj, u0, u1, v=0.0, w=0.0):
    """UV0 for a part the game lights part of the way (GunAmmoGlow, GunHeatGlow): x runs from 0 at u0 to 1 at u1 (u0
    may lie past u1: a magazine counts from its feed end), y once round the axis along u through (v, w), so no face is
    without UV area. Call it on the part in gun space, before Gun.build moves it to its origin."""
    me = obj.data
    if not me.uv_layers:
        me.uv_layers.new(name='UVMap')
    uv = me.uv_layers.active.data
    span = (u1 - u0) or 1.0
    for poly in me.polygons:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            # A hair's offset by the distance from the axis gives end caps (all at one u) some UV area.
            x = min(max((-co.y / CM - u0) / span, 0.0), 1.0) + 0.001 * math.hypot(co.z / CM - v, co.x / CM - w)
            y = (math.atan2(co.z / CM - v, co.x / CM - w) / (2.0 * math.pi)) % 1.0
            uv[li].uv = (x, y)
    obj['KeepUV'] = 1   # fill_uvs leaves it be
    return obj


def fill_uvs(gun):
    """Box-projects UV0 onto the gun's parts that have none worth the name (glows, glass: materials without a texture
    set, which mapped() leaves at zero), so Unreal's tangent build doesn't warn on them. Parts along_uv mapped keep
    theirs. Call it before gun.build()."""
    for parts in gun.parts.values():
        for obj in parts:
            me = obj.data
            if obj.get('KeepUV'):
                del obj['KeepUV']   # not a property the model should carry
                continue
            if not me.materials or SET_OF.get(me.materials[0].name if me.materials[0] else ''):
                continue
            if not me.uv_layers:
                me.uv_layers.new(name='UVMap')
            uv = me.uv_layers.active.data
            for poly in me.polygons:
                n = poly.normal
                a = max(range(3), key=lambda i: abs(n[i]))
                i, j = [k for k in range(3) if k != a]
                for li in poly.loop_indices:
                    co = me.vertices[me.loops[li].vertex_index].co
                    uv[li].uv = (co[i] * 10.0, co[j] * 10.0)


def bezier(p0, p1, p2, n=8):
    """A quadratic curve from p0 to p2 pulled toward p1, as n + 1 points (any number of coordinates)."""
    return [tuple((1 - t) ** 2 * a + 2 * (1 - t) * t * b + t * t * c for a, b, c in zip(p0, p1, p2))
            for t in (k / n for k in range(n + 1))]


def helix(u0, u1, r_in, r_out, pitch, band, v=0.0, w=0.0, steps=24, phase=0.0, mat=None):
    """A coil band wound round an axis along u through (v, w), from u0 to u1: band cm wide along u, between radii r_in
    and r_out, advancing pitch cm a turn (a helical magazine's frame, a spring, a cooling coil). The gaps between its
    turns (pitch - band wide) show what lies inside. phase (degrees) turns where it starts."""
    bm = lp.new_bmesh()
    hb = band * 0.5
    turns = max((u1 - u0 - band) / pitch, 0.0)
    n = max(int(math.ceil(turns * steps)), 1)
    rings = []
    for i in range(n + 1):
        t = turns * i / n
        a = 2.0 * math.pi * t + math.radians(phase)
        c = u0 + hb + pitch * t
        ca, sa = math.cos(a), math.sin(a)
        rings.append([bm.verts.new(at(c + du, v + r * sa, w + r * ca))
                      for du, r in ((-hb, r_in), (hb, r_in), (hb, r_out), (-hb, r_out))])
    for r0, r1 in zip(rings, rings[1:]):
        for j in range(4):
            bm.faces.new((r0[j], r0[(j + 1) % 4], r1[(j + 1) % 4], r1[j]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mapped(lp.mesh_object(bm), mat)


def hollow(profile, u0, v=0.0, segments=20, mat=None, wall=0.12, w=0.0):
    """A tube turned round an axis along u (profile [(du, r), ...] as turned) but open at both ends: a thin wall that
    follows the outline inside, so the eye looks down it (scope tubes, sight rings)."""
    outer = [(r * CM, du * CM) for du, r in profile]
    inner = [((r - wall) * CM, du * CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(at(u0, v, w)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return mapped(obj, mat)


def cartridge(u, v, w=0.0, length=5.6, r=0.5, case=BRASS, tip=COPPER, segments=10):
    """A round lying along u with its base at u (a belt's rounds, a magazine's window): a necked brass case and a copper
    bullet. Returns the two pieces."""
    body = length * 0.62
    return [turned([(0.0, r * 1.05), (0.12, r * 1.05), (0.16, r * 0.9), (0.3, r), (body, r), (body + 0.35, r * 0.62),
                    (body + 0.6, r * 0.62)], u, v, w, segments=segments, mat=case),
            turned([(0.0, r * 0.6), (length * 0.2, r * 0.6), (length * 0.32, r * 0.42), (length * 0.38 - 0.02, 0.06)],
                   u + body + 0.55, v, w, segments=segments, mat=tip)]


# --- Frontier-tech sights (each family builds its own copies on its own rail) ---

def scope_glass(gun, m, u_eye, u_obj, v, r_eye, r_obj, dot=False, wall=0.12):
    """A tube sight's glass: a lens just inside each end, and a reticle just behind the objective (a dot, or a fine
    cross), tiny because it sits over the screen's crosshair."""
    gun.add(m, tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=LENS))
    gun.add(m, tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=LENS))
    u = u_obj - 0.4
    if dot:
        gun.add(m, pin(u, v, -0.06, 0.06, 0.06, mat=RETICLE, segments=10))
    else:
        gun.add(m, box(u, u + 0.02, v - 0.075, v + 0.075, 0.02, bevel=0.0, mat=RETICLE))
        gun.add(m, box(u, u + 0.02, v - 0.01, v + 0.01, 0.15, bevel=0.0, mat=RETICLE))


def frontier_sight(gun, m, kind, uc, v0, shell=POLY_SAND, metal=BLACK, trim=None, span=14.0, zoom=4.0):
    """One of the frontier-tech sights on a rail whose teeth top out at v0, centered on uc; adds its SOCKET_Aim (where
    the eye lines up: the irons' rear ring, the dot, the eyepiece). trim: a material for its rings and caps (BRASS on
    rarer ones). Kinds:
      Iron    folding irons span cm apart: a peep ring at the back, a post with a brass bead and a cyan dot in front
      Pip     a 1x tube dot with a cyan LED knob and a sand sunshade
      Reflex  an open window in a sand hood, a cyan strip under the glass
      Holo    a holographic sight: an open hood over the front of a long base, a ringed dot, a sand cap with a cyan bar
      Prism   a compact 2x prism, a cyan ring round its objective
      Scope   a tube scope (zoom 3-8 makes it longer and wider at the objective), two rings, turrets, an ember line
              along the top and a cyan objective ring"""
    trim = trim or metal
    if kind == 'Iron':
        ur, uf = uc - span * 0.5, uc + span * 0.5
        h = v0 + 2.5
        gun.add(m, box(ur - 1.4, ur + 1.4, v0, v0 + 0.6, 2.2, bevel=0.15, mat=metal))
        blade = slab([(ur - 1.0, v0 + 0.6), (ur + 1.0, v0 + 0.6), (ur + 0.7, h + 0.9), (ur - 0.7, h + 0.9)], 1.8,
                     bevel=0.15, mat=metal, round=0.3)
        gun.add(m, cut(blade, [tube(ur - 2.0, ur + 2.0, 0.42, v=h, segments=16)]))
        gun.add(m, hollow([(0.0, 0.62), (0.5, 0.62)], ur - 0.25, h, 16, trim, wall=0.2))
        gun.add(m, box(uf - 1.2, uf + 1.2, v0, v0 + 0.6, 2.0, bevel=0.15, mat=metal))
        gun.add(m, slab([(uf - 0.9, v0 + 0.6), (uf + 0.9, v0 + 0.6), (uf + 0.4, h - 0.2), (uf - 0.4, h - 0.2)], 1.6,
                        bevel=0.12, mat=metal, round=0.2))
        gun.add(m, upright(uf, h - 0.3, h - 0.05, 0.16, mat=BRASS, segments=10))
        gun.add(m, pin(uf - 0.17, h - 0.18, -0.07, 0.07, 0.08, mat=CYAN, segments=8))
        for w in (-1.05, 1.05):   # wings guarding the front post
            gun.add(m, slab([(uf - 0.7, v0 + 0.6), (uf + 0.7, v0 + 0.6), (uf + 0.4, h + 0.2), (uf - 0.4, h + 0.2)], 0.3,
                            w=w, bevel=0.06, mat=metal, round=0.2))
        gun.socket(m, 'Aim', (ur, h))
        return
    if kind == 'Pip':
        h = v0 + 2.7
        gun.add(m, box(uc - 3.6, uc + 3.6, v0, v0 + 1.2, 2.2, bevel=0.25, mat=metal))
        gun.add(m, hollow([(0, 1.4), (0.8, 1.4), (1.2, 1.2), (6.0, 1.2), (6.4, 1.4), (7.4, 1.4)], uc - 3.7, h, 20, metal))
        gun.add(m, hollow([(0, 1.5), (2.2, 1.55)], uc + 3.7, h, 20, shell))                  # sunshade
        gun.add(m, hollow([(0, 1.43), (0.3, 1.43)], uc - 3.75, h, 20, trim, wall=0.18))     # eyepiece ring
        gun.add(m, pin(uc, h, 1.0, 1.9, 0.6, mat=metal, segments=14))                       # LED knob on the left
        gun.add(m, pin(uc, h, 1.9, 2.0, 0.34, mat=CYAN, segments=12))
        scope_glass(gun, m, uc - 3.7, uc + 3.7, h, 1.4, 1.4, dot=True)
        gun.socket(m, 'Aim', (uc - 3.5, h))
        return
    if kind == 'Reflex':
        gun.add(m, box(uc - 3.6, uc + 3.4, v0, v0 + 1.0, 2.6, bevel=0.25, mat=metal))
        hood = slab([(uc - 1.8, v0 + 1.0), (uc + 3.4, v0 + 1.0), (uc + 3.4, v0 + 4.6), (uc + 2.2, v0 + 5.6),
                     (uc - 0.4, v0 + 5.6), (uc - 1.8, v0 + 4.4)], 2.6, bevel=0.25, mat=shell, round=0.4)
        gun.add(m, cut(hood, [cbox(uc - 2.8, uc + 4.4, v0 + 1.7, v0 + 4.8, -1.0, 1.0)]))
        gun.add(m, box(uc + 2.4, uc + 2.55, v0 + 1.7, v0 + 4.8, 2.0, bevel=0.0, mat=LENS))
        gun.add(m, box(uc + 1.9, uc + 2.9, v0 + 1.05, v0 + 1.25, 1.6, bevel=0.0, mat=CYAN))
        gun.add(m, box(uc - 3.4, uc - 1.6, v0 + 1.0, v0 + 1.7, 2.0, bevel=0.15, mat=trim))   # the emitter
        gun.add(m, pin(uc + 2.3, v0 + 3.25, -0.1, 0.1, 0.1, mat=RETICLE))
        gun.socket(m, 'Aim', (uc + 2.3, v0 + 3.25))
        return
    if kind == 'Holo':
        # The hood covers only the front of the base, so the eye sees a short window, not down a tunnel; behind it a low
        # control block stays under the line of sight (2026-10-22: the full-length hood had solid ends and hid the view).
        gun.add(m, box(uc - 5.4, uc + 5.4, v0, v0 + 1.3, 3.0, bevel=0.25, mat=metal))
        hood = slab([(uc - 1.0, v0 + 1.2), (uc + 5.4, v0 + 1.2), (uc + 5.4, v0 + 5.5), (uc + 4.2, v0 + 6.4),
                     (uc + 0.2, v0 + 6.4), (uc - 1.0, v0 + 5.5)], 3.4, bevel=0.35, segments=3, mat=metal, round=0.4)
        gun.add(m, cut(hood, [cbox(uc - 2.0, uc + 6.4, v0 + 1.9, v0 + 5.6, -1.25, 1.25)]))
        gun.add(m, slab([(uc - 5.2, v0 + 1.2), (uc - 1.0, v0 + 1.2), (uc - 1.0, v0 + 1.85), (uc - 4.6, v0 + 1.85)], 2.8,
                        bevel=0.2, mat=metal, round=0.2))                                     # the control block
        for u in (uc - 4.2, uc - 2.8):   # its two buttons
            gun.add(m, upright(u, v0 + 1.8, v0 + 1.95, 0.4, mat=shell, segments=12))
        gun.add(m, box(uc + 0.6, uc + 3.8, v0 + 6.3, v0 + 6.75, 1.9, bevel=0.15, mat=shell))
        gun.add(m, box(uc + 0.9, uc + 3.5, v0 + 6.7, v0 + 6.85, 0.5, bevel=0.0, mat=CYAN))
        for u in (uc - 1.15, uc + 5.15):   # frames round the window's ends, open like it
            frame = box(u, u + 0.45, v0 + 1.2, v0 + 6.4, 3.5, bevel=0.05, mat=trim)
            gun.add(m, cut(frame, [cbox(u - 0.5, u + 0.95, v0 + 1.9, v0 + 5.6, -1.25, 1.25)]))
        gun.add(m, box(uc + 3.9, uc + 4.05, v0 + 1.9, v0 + 5.6, 2.5, bevel=0.0, mat=LENS))
        gun.add(m, pin(uc + 3.8, v0 + 3.75, -0.12, 0.12, 0.12, mat=RETICLE))
        gun.add(m, hollow([(0.0, 0.62), (0.03, 0.62)], uc + 3.78, v0 + 3.75, 24, RETICLE, wall=0.06))   # its ring
        gun.socket(m, 'Aim', (uc + 3.8, v0 + 3.75))
        return
    if kind == 'Prism':
        h = v0 + 2.7
        body = slab([(uc - 5.0, v0), (uc + 5.0, v0), (uc + 5.0, v0 + 4.1), (uc + 3.5, v0 + 4.8), (uc - 3.5, v0 + 4.8),
                     (uc - 5.0, v0 + 4.1)], 3.0, bevel=0.35, segments=3, mat=metal, round=0.5)
        gun.add(m, cut(body, [tube(uc - 6.0, uc + 6.0, 1.05, v=h, segments=20)]))
        gun.add(m, slab([(uc - 3.2, v0 + 4.6), (uc + 3.2, v0 + 4.6), (uc + 2.6, v0 + 5.3), (uc - 2.6, v0 + 5.3)], 2.4,
                        bevel=0.2, mat=shell, round=0.3))
        gun.add(m, hollow([(0, 1.3), (0.35, 1.3)], uc + 4.9, h, 20, CYAN, wall=0.25))
        gun.add(m, hollow([(0, 1.3), (0.35, 1.3)], uc - 5.25, h, 20, trim, wall=0.25))
        gun.add(m, box(uc + 4.55, uc + 4.57, h - 0.075, h + 0.075, 0.02, bevel=0.0, mat=RETICLE))
        gun.add(m, box(uc + 4.55, uc + 4.57, h - 0.01, h + 0.01, 0.15, bevel=0.0, mat=RETICLE))
        for u in (uc - 4.9, uc + 5.0):
            gun.add(m, tube(u, u + 0.12, 1.1, v=h, segments=20, mat=LENS))
        gun.socket(m, 'Aim', (uc - 4.9, h))
        return
    if kind == 'Scope':
        k = (zoom - 3.0) / 5.0                        # 0 at 3x, 1 at 8x
        length = 17.0 + 9.0 * k
        r_eye, r_tube, r_obj = 1.8, 1.25 + 0.1 * k, 2.0 + 0.7 * k
        h = v0 + r_obj + 1.6
        u_eye = uc - length * 0.45
        u_obj = u_eye + length
        bell = min(5.5 + 3.0 * k, length * 0.35)
        gun.add(m, hollow([(0, r_eye), (3.6, r_eye), (5.0, r_tube), (length - bell, r_tube), (length - bell + 2.0, r_obj),
                           (length, r_obj)], u_eye, h, 24, metal))
        for u in (uc - 4.4, uc + 4.4):   # rings round the tube on their bases, open in the middle like it
            gun.add(m, hollow([(0.0, r_tube + 0.22), (1.3, r_tube + 0.22)], u - 0.65, h, 20, trim, wall=0.24))
            gun.add(m, box(u - 0.9, u + 0.9, v0, h - r_tube - 0.05, 2.1, bevel=0.2, mat=metal))
        gun.add(m, upright(uc, h + r_tube - 0.1, h + r_tube + 1.2, 0.85, mat=metal, segments=16))
        gun.add(m, upright(uc, h + r_tube + 1.2, h + r_tube + 1.45, 0.65, mat=trim, segments=16))
        gun.add(m, pin(uc, h, -r_tube - 1.2, -r_tube + 0.1, 0.85, mat=metal, segments=16))
        gun.add(m, box(u_eye + 5.4, u_obj - bell - 0.3, h + r_tube - 0.05, h + r_tube + 0.12, 0.3, bevel=0.0, mat=EMBER))
        gun.add(m, box(u_eye + 5.2, u_obj - bell - 0.1, h + r_tube - 0.2, h + r_tube + 0.05, 0.8, bevel=0.05,
                       mat=shell))
        gun.add(m, hollow([(0, r_obj + 0.06), (0.35, r_obj + 0.06)], u_obj - 0.35, h, 24, CYAN, wall=0.22))
        gun.add(m, hollow([(0, r_eye + 0.08), (0.9, r_eye + 0.08)], u_eye, h, 24, RUBBER, wall=0.25))   # eyecup
        scope_glass(gun, m, u_eye, u_obj, h, r_eye, r_obj)
        gun.socket(m, 'Aim', (u_eye + 0.2, h))
        return
    raise ValueError(f'frontier_sight: no kind {kind!r}')


# --- Element parts (2026-10-22): each element part of the seven families wears its element ---
# Ember (burn): heat-scored metal, temper colours, glowing ember vents. Soul-light (vs undead): pale gold filigree,
# saint's medals and lanterns, a soft gold-white glow band. Grave-salt (slow): pale salt crystals growing out of the part,
# cyan-white glow inlays. Each family's script builds an element part on the part it replaced (same slot, origin and
# sockets, so it fits every body), then dresses it with these.

SOUL = glow('GunSoulGlow', 0xffe4a8, 3.0)                  # Soul-light: the glow band, a medal's heart, a lantern's light
SALT_GLOW = glow('GunSaltGlow', 0xc4f2ff, 3.0)             # Grave-salt: the light in the crystals and the inlays
GILT = material('GunGilt', 'MetalWorn', 0xf6dfa0)          # Soul-light's pale gold (paler than brass)
SALT = material('GunSaltCrystal', 'Polymer', 0xe4f2f3)     # Grave-salt's crystals
SCORCHED = material('GunScorched', 'MetalWorn', 0x4a3a36)  # Ember's heat-scored metal
TEMPER = material('GunHeatTint', 'MetalWorn', 0x6b5682)    # the blue-violet temper colour where heat scored it


def gdir(du, dv, dw):
    """A direction in gun space, in Blender's axes (as at() is for points)."""
    return Vector((dw, -du, dv))


def _basis(normal, up=None):
    """Right, up and out axes (Blender's) for a face whose normal is given; up is squared to it (any, when None)."""
    n = Vector(normal).normalized()
    hint = Vector(up) if up is not None else (Vector((0.0, 0.0, 1.0)) if abs(n.z) < 0.9 else Vector((1.0, 0.0, 0.0)))
    y = hint - n * hint.dot(n)
    if y.length < 1e-6:
        y = Vector((1.0, 0.0, 0.0)) - n * n.x
    y.normalize()
    return y.cross(n).normalized(), y, n


def frame(center, normal, up=None):
    """A face's own space in cm: x right, y up, z out of the face, centered on center (u, v, w); normal and up are
    gun-space directions (du, dv, dw). Returns the matrix to Blender's space (meters)."""
    x, y, n = _basis(gdir(*normal), gdir(*up) if up is not None else None)
    rot = Matrix((x, y, n)).transposed().to_4x4()
    return Matrix.Translation(at(*center)) @ rot @ Matrix.Diagonal((CM, CM, CM, 1.0))


def _out_of(space):
    return tuple(space.to_3x3().col[2].normalized())


def plate(outline, z0, z1, space, mat=None):
    """A flat outline [(x, y), ...] (cm, in a frame()'s space) standing from z0 to z1 out of the face: inlays, stars,
    lantern panes."""
    bm = lp.new_bmesh()
    a = [bm.verts.new(space @ Vector((x, y, z0))) for x, y in outline]
    b = [bm.verts.new(space @ Vector((x, y, z1))) for x, y in outline]
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    n = len(outline)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[i], a[j], b[j], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    return mapped(lp.mesh_object(bm), mat)


def disc(r, z0, z1, space, mat=None, segments=12, profile=None):
    """A round boss standing out of a face from z0 to z1 (profile [(dz, r), ...] overrides the plain disc)."""
    obj = _lathe(profile or [(0.0, r), (z1 - z0, r)], segments, 0.0)
    obj.data.transform(space @ Matrix.Translation((0.0, 0.0, z0)) @ Matrix.Diagonal((1 / CM, 1 / CM, 1 / CM, 1.0)))
    return mapped(obj, mat)


def star(rays, r_out, r_in, rotate=0.0):
    """A rayed star's outline (x, y) for plate(), a ray pointing up."""
    pts = []
    for k in range(rays * 2):
        a = math.radians(rotate + 90.0) + math.pi * k / rays
        r = r_out if k % 2 == 0 else r_in
        pts.append((r * math.cos(a), r * math.sin(a)))
    return pts


def _loop(space, cx, cy, r, z, thick, mat, sides=8):
    ring = [space @ Vector((cx + r * math.cos(2 * math.pi * k / sides), cy + r * math.sin(2 * math.pi * k / sides), z))
            for k in range(sides + 1)]
    return mapped(lp.sweep(ring, lp.ngon(thick * CM, 4), up=_out_of(space)), mat)


def medal(center, normal, r, up=(0.0, 1.0, 0.0), mat=None, heart=None, rays=8, bail=True, segments=14):
    """A saint's medal on a face, r cm across: a gilt disc with a raised rim, a rayed star and a glowing heart, a small
    bail on top. Returns the pieces."""
    mat, heart = mat or GILT, heart or SOUL
    space = frame(center, normal, up)
    out = [disc(r, 0.0, 0.18, space, mat, segments, profile=[(0.0, r), (0.12, r), (0.18, r * 0.86), (0.18, 0.0)]),
           plate(star(rays, r * 0.74, r * 0.3), 0.14, 0.26, space, mat),
           disc(r * 0.26, 0.2, 0.32, space, heart, 10)]
    if bail:
        out.append(_loop(space, 0.0, r * 1.18, r * 0.2, 0.08, max(0.035, r * 0.07), mat))
    return out


def lantern(center, normal, h, up=(0.0, 1.0, 0.0), mat=None, light=None):
    """A lantern in relief on a face, h cm tall: a glowing pane in a gilt frame, a peaked cap, a foot and a ring on top.
    Returns the pieces."""
    mat, light = mat or GILT, light or SOUL
    space = frame(center, normal, up)
    w = h * 0.34
    out = [plate([(-w, -h * 0.32), (w, -h * 0.32), (w * 1.1, h * 0.22), (-w * 1.1, h * 0.22)], 0.0, 0.14, space, light),
           plate([(-w * 1.35, h * 0.2), (w * 1.35, h * 0.2), (w * 0.35, h * 0.46), (-w * 0.35, h * 0.46)], 0.0, 0.24,
                 space, mat),                                                              # the cap
           plate([(-w * 1.2, -h * 0.44), (w * 1.2, -h * 0.44), (w, -h * 0.3), (-w, -h * 0.3)], 0.0, 0.24, space, mat)]
    bw = h * 0.035
    for x in (-w * 1.08, 0.0, w * 1.08):                                                   # the frame's bars
        out.append(plate([(x - bw, -h * 0.31), (x + bw, -h * 0.31), (x + bw, h * 0.21), (x - bw, h * 0.21)], 0.0, 0.22,
                         space, mat))
    out.append(_loop(space, 0.0, h * 0.55, h * 0.09, 0.1, max(0.035, h * 0.03), mat))
    return out


def hanging_lantern(u, v_top, w=0.0, h=1.6, mat=None, light=None, segments=8):
    """A small lantern hung under a part, h cm tall from the ring at (u, v_top, w) down: a ring, a peaked cap, a
    glowing body between four bars, a foot. Returns the pieces."""
    mat, light = mat or GILT, light or SOUL
    r = h * 0.24
    ring_r = h * 0.09
    ring = [(u + ring_r * math.sin(2 * math.pi * k / 8), v_top - ring_r + ring_r * math.cos(2 * math.pi * k / 8), w)
            for k in range(9)]
    top = v_top - ring_r * 2.0
    cap0, body0, foot0 = top - h * 0.2, top - h * 0.68, top - h * 0.8
    out = [pipe(ring, max(0.035, h * 0.03), mat=mat, sides=4),
           upright(u, cap0, top, r, w=w, mat=mat, segments=segments, profile=[(0.0, r * 1.15), (h * 0.06, r * 1.15),
                                                                              (h * 0.2, r * 0.25)]),
           upright(u, body0, cap0, r * 0.82, w=w, mat=light, segments=segments),
           upright(u, foot0, body0, r * 1.1, w=w, mat=mat, segments=segments,
                   profile=[(0.0, r * 0.8), (h * 0.04, r * 1.1), (h * 0.12, r * 1.1)])]
    for k in range(4):
        a = math.pi * 0.25 + math.pi * 0.5 * k
        du, dw = r * 0.9 * math.cos(a), r * 0.9 * math.sin(a)
        out.append(box(u + du - h * 0.03, u + du + h * 0.03, body0, cap0, h * 0.06, w=w + dw, bevel=0.0, mat=mat))
    return out


def curl(center, normal, r0, up=(0.0, 1.0, 0.0), turns=1.6, flip=1.0, start=0.0, mat=None, thick=0.06, lift=0.03,
         steps=16):
    """A curl of filigree lying on a face (a scroll), r0 cm across at its outer end."""
    space = frame(center, normal, up)
    pts = []
    for k in range(steps):
        t = k / (steps - 1.0)
        a = math.radians(start) + flip * 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append(space @ Vector((r * math.cos(a), r * math.sin(a), lift)))
    return mapped(lp.sweep(pts, lp.ngon(thick * CM, 4), up=_out_of(space)), mat or GILT)


def vine(p0, p1, normal, amp, waves=2.0, mat=None, thick=0.06, curls=True):
    """Filigree along a face from p0 to p1 (u, v, w; normal (du, dv, dw) out of the face): a wave amp cm high with a
    small curl tucked under every crest, alternating sides. Returns the pieces."""
    a, b = at(*p0), at(*p1)
    n = gdir(*normal).normalized()
    along = b - a
    length = along.length
    along.normalize()
    side = n.cross(along).normalized()
    lift = n * 0.03 * CM
    steps = max(int(waves * 10), 8)
    pts = [a + along * length * (k / steps) + side * amp * CM * math.sin(2 * math.pi * waves * k / steps) + lift
           for k in range(steps + 1)]
    out = [mapped(lp.sweep(pts, lp.ngon(thick * CM, 4), up=tuple(n)), mat or GILT)]
    if curls:
        crests = int(round(waves * 2))
        for c in range(crests):
            s = 1.0 if c % 2 == 0 else -1.0
            mid = a + along * length * ((c + 0.5) / crests) + side * amp * CM * s * 0.45 + lift
            pts = []
            for k in range(10):
                q = k / 9.0
                ang = math.pi * 0.5 + 2.0 * math.pi * 1.1 * q
                r = amp * 0.5 * (1.0 - 0.7 * q) * CM
                pts.append(mid + side * s * r * math.sin(ang) + along * r * math.cos(ang))
            out.append(mapped(lp.sweep(pts, lp.ngon(thick * 0.8 * CM, 4), up=tuple(n)), mat or GILT))
    return out


def crystal(base, direction, length, r, mat=None, sides=6, tip=0.32, foot=0.7, sink=0.3, twist=0.0):
    """A pointed salt crystal growing from base (u, v, w) along direction (du, dv, dw): a six-sided prism r cm across,
    narrower at its foot (sunk sink cm into what it grows from), its last tip share drawn to a point."""
    bm = lp.new_bmesh()
    o = at(*base)
    x, y, d = _basis(gdir(*direction))
    angles = [math.radians(twist) + 2.0 * math.pi * k / sides for k in range(sides)]

    def ring(rr, h):
        return [bm.verts.new(o + d * h * CM + (x * math.cos(a) + y * math.sin(a)) * rr * CM) for a in angles]

    lo = ring(r * foot, -sink)
    hi = ring(r, length * (1.0 - tip))
    apex = bm.verts.new(o + d * length * CM)
    bm.faces.new(list(reversed(lo)))
    for i in range(sides):
        j = (i + 1) % sides
        bm.faces.new((lo[i], lo[j], hi[j], hi[i]))
        bm.faces.new((hi[i], hi[j], apex))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    return mapped(lp.mesh_object(bm), mat or SALT)


def salt_cluster(seed, center, normal, spread, count, length=(0.5, 1.2), r=(0.12, 0.26), lean=32.0, glow_every=3,
                 sides=6, up=None, squash=1.0):
    """count crystals growing out of a face round center (u, v, w), within spread cm of it (squash scales the spread
    along the face's up), leaning up to lean degrees off its normal (du, dv, dw); the first stands tallest in the
    middle, and every glow_every-th is lit from within (GunSaltGlow). Seeded: the same every run. Returns the pieces."""
    import random
    rnd = random.Random(seed)
    nb = gdir(*normal).normalized()
    x, y, _ = _basis(nb, gdir(*up) if up is not None else None)
    out = []
    for k in range(count):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        s = spread * math.sqrt(rnd.random()) if k else 0.0
        p = at(*center) + (x * math.cos(a) + y * math.sin(a) * squash) * s * CM
        tilt = math.radians(rnd.uniform(0.0, lean))
        b = rnd.uniform(0.0, 2.0 * math.pi)
        d = (nb * math.cos(tilt) + (x * math.cos(b) + y * math.sin(b)) * math.sin(tilt)).normalized()
        big = 1.0 - 0.5 * s / max(spread, 1e-6)
        length_k = rnd.uniform(*length) * (0.6 + 0.4 * big)
        r_k = rnd.uniform(*r) * (0.75 + 0.25 * big)
        twist = rnd.uniform(0.0, 60.0)
        mat = SALT_GLOW if glow_every and k % glow_every == glow_every - 1 else SALT
        out.append(crystal((-p.y / CM, p.z / CM, p.x / CM), (-d.y, d.z, d.x), length_k, r_k, mat=mat, sides=sides,
                           twist=twist))
    return out


def want_elements():
    """True when the script was started with --elements after '--' (the element sheet only)."""
    import sys
    return '--elements' in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])


def element_preview(name, assemble, default, slot, keys, built, fp=None, side=None, close=None, res=(960, 540)):
    """Saved/ArtPreviews/Weapons/<name>.png: a row per key of slot (the plain part first, then the element ones), the
    gun in first person (from behind, above and to its left, as the eye sees it), from its right side, and the part
    alone close up from the front left. assemble(choice) puts a gun together and returns its objects."""
    fp = fp or dict(view=(0.42, 1.0, 0.32), lens=45.0, fit=0.8)
    side = side or dict(view=(-1.0, -0.02, 0.05), lens=80.0, fit=0.9)
    close = close or dict(view=(0.8, -0.75, 0.3), lens=60.0, fit=1.0)
    tiles = []
    for key in keys:
        choice = dict(default, **{slot: key})
        for kwargs in (fp, side):
            tiles.append((lambda c=choice: assemble(c), dict(kwargs, ground=False, resolution=res)))

        def alone(c=choice):
            assemble(c)
            return [built[f'{slot}_{c[slot]}']]
        tiles.append((alone, dict(close, ground=False, resolution=res)))
    path = element_sheet(lt.preview_path('Weapons', name), tiles, cols=3)
    assemble(default)
    return path


def element_sheet(out_png, tiles, cols=2, gap=8):
    """Renders tiles [(objects or a function returning them, lt.preview keyword arguments), ...] and sets them side by
    side, cols to a row, in one PNG on dark grey (the element previews: a row per element, first person and side).
    Each tile's own PNG stays in an Elements folder beside it."""
    import os
    import numpy as np
    folder = os.path.join(os.path.dirname(out_png), 'Elements')
    stem = os.path.splitext(os.path.basename(out_png))[0]
    images = []
    for k, (objs, kwargs) in enumerate(tiles):
        objs = objs() if callable(objs) else objs
        path = lt.preview(objs, os.path.join(folder, f'{stem}_{k:02d}.png'), **kwargs)
        image = bpy.data.images.load(path, check_existing=False)
        image.colorspace_settings.name = 'Non-Color'
        w, h = image.size
        px = np.empty(w * h * 4, np.float32)
        image.pixels.foreach_get(px)
        bpy.data.images.remove(image)
        images.append(px.reshape(h, w, 4)[::-1, :, :3])
    th = max(t.shape[0] for t in images)
    tw = max(t.shape[1] for t in images)
    rows = (len(images) + cols - 1) // cols
    sheet = np.full((rows * th + (rows + 1) * gap, cols * tw + (cols + 1) * gap, 3), 0.08, np.float32)
    for i, t in enumerate(images):
        r, c = divmod(i, cols)
        y0, x0 = gap + r * (th + gap), gap + c * (tw + gap)
        sheet[y0:y0 + t.shape[0], x0:x0 + t.shape[1]] = t
    lt.write_png(out_png, lt.to8(np.clip(sheet, 0.0, 1.0)))
    lt._log(f'element sheet: {out_png}')
    return out_png
