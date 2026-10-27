"""The heroes' melee tools (CONTRACT3 note 5; the user's picks of 2026-10-27 from Art/Concepts/MeleeTools/): five
models in one script, each its own asset (Art/README.md), swung in the right hand (the Piston Fist rides the forearm).

  Melee_GraveSpade   Ellis's: Sexton's spare spade, 95 cm. Black iron blade with its edge worn bright, a cyan ember
                     crack through it, an iron socket and tread, ash shaft and D-grip. The fist on the shaft just over
                     the D-grip's ferrule. Swats with the blade's flat (+Z).
  Melee_RazorFan     Odessa's: five steel-edged trick cards (7 x 9.4 cm) fanned on a brass pivot rivet, gold-rimmed;
                     faces (suit pips, the ace of spades on top) on the back-of-hand side (-Y), oxblood backs with a
                     gold spade on the palm side. The fan leans toward +Z, its leading edge.
  Melee_FerryGaff    Crane's: a ferryman's boat hook, 120 cm. Weathered grey ash pole, black iron butt cap, band,
                     socket, spike and a hook curling back toward him (+Z), a tarnished fare coin nailed to the pole.
  Melee_PistonFist   Gauge's: no grip. Clamped round his forearm (two rings, a saddle, a split lug under it), a brass
                     cylinder on top drives a steel rod and a yoke onto a riveted iron knuckle plate over the fist, three
                     firebox-hot knuckle caps glowing orange; a little brass steam vent on the cylinder's side.
  Melee_RockPick     Pike's: a surveyor's rock pick, 35 cm. Steel head (square hammer face -Z, curved pick +Z, the
                     strike side), oval hickory handle bound in brass wire at the butt and over the fist, a teal band.

Space (Source/AI_Looter_Shooter/Player/PlayerMeleeToolTypes.h, FHeroMeleeTool): the origin is the grip point (the
middle of the fist on the handle), +X (Blender -Y) runs up the handle to the head, +Z is the side that strikes, +Y
(Blender -X) the way the right palm faces onto the handle.
  SOCKET_Grip     the grip point (the origin), turned as the mesh: the hand's frame.
  SOCKET_Tip      the striking end (blade tip, middle card's top, spike tip, knuckle caps' front, pick tip).
  SOCKET_Hook     the gaff's hook point (the yank).
The Piston Fist's origin is where it clamps on: SOCKET_Forearm on the forearm's axis at the front clamp ring, +X along
the forearm toward the knuckles, +Z out of the back of the hand. The rings fit Gauge's first-person casings (radius
3.9-4.45 cm; Tools/Blender/fp_arms_gauge.py): bore 4.7 cm. The knuckle plate's back face is 20 cm ahead of the clamp
(his fist's front about 19 cm with the clamp 6-7 cm behind the wrist). SOCKET_Vent: the vent's mouth, its front (+X)
out to the side (-Y), for the steam puff.

Units in this script: centimeters in tool space, (u, v, w) = (+X, +Z, -Y) as Unreal reads them. No collision, no
Nanite (small, held close), no LODs. Materials, tinted texture sets on the World master (wood is ToolWood, the pale
set made for these handles: Art/Textures/ToolWood/make_tool_wood.py):
  MeleeIronBlack (MetalWorn)  MeleeSteelBright (MetalWorn)  MeleeSteelHead (MetalWorn)  BrassWorn (MetalWorn, shared)
  MeleeGold (MetalWorn)  MeleeCoin (MetalWorn)  MeleeAsh / MeleeAshGrey / MeleeHickory (ToolWood)
  MeleeCardFace / MeleeCardBack / MeleeCardInk / MeleeCardRed (Polymer)  MeleeTealPaint (PaintWorn)
  MeleeEmberCyan, MeleeEmberOrange: opaque emissive surfaces (Glow; Nanite can't draw the additive kind anyway).

    Tools\\artrun.ps1 -Script Art\\Models\\Weapons\\Melee\\MeleeTools.py -Preview
renders each tool to Saved/TOOLS3/<Model>_A.png and _B.png.
"""
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

CM = 0.01
DENSITY = 1024.0          # px per meter, as the guns: seen up close
PREVIEW_DIR = os.path.join(lt.REPO, 'Saved', 'TOOLS3')


def at(u, v=0.0, w=0.0):
    """Tool space (cm) to Blender (m): u along the tool (Blender -Y), v the strike side (Z), w = Blender +X."""
    return Vector((w * CM, -u * CM, v * CM))


# --- Materials ---

def textured(set_name, name, tint):
    return lt.material(set_name, name=name, tint=tint)


def glow(name, color, strength):
    """An opaque emissive surface (the stylized Surface material's Glow). The emission is set in Blender too, only so
    the previews show it; the exporter reads the base color and the Glow property."""
    mat = lm.material(name, color, Glow=strength, Variation=0.0)
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Emission Color'].default_value = lm.hex_color(color)
    bsdf.inputs['Emission Strength'].default_value = strength
    return mat


IRON = textured('MetalWorn', 'MeleeIronBlack', 0x34353a)
STEEL_BRIGHT = textured('MetalWorn', 'MeleeSteelBright', 0xeef0f2)
STEEL_HEAD = textured('MetalWorn', 'MeleeSteelHead', 0x8e9296)
BRASS = textured('MetalWorn', 'BrassWorn', 0xc49c56)          # the clutter props' brass, shared
GOLD = textured('MetalWorn', 'MeleeGold', 0xf2c463)
COIN = textured('MetalWorn', 'MeleeCoin', 0xa8874f)
ASH = textured('ToolWood', 'MeleeAsh', 0xecdcc0)
ASH_GREY = textured('ToolWood', 'MeleeAshGrey', 0x9a9c9c)
HICKORY = textured('ToolWood', 'MeleeHickory', 0xcaa682)
CARD_FACE = textured('Polymer', 'MeleeCardFace', 0xf4ecd8)
CARD_BACK = textured('Polymer', 'MeleeCardBack', 0x6a1a22)
CARD_INK = textured('Polymer', 'MeleeCardInk', 0x1e1c1d)
CARD_RED = textured('Polymer', 'MeleeCardRed', 0xb4232c)
TEAL = textured('PaintWorn', 'MeleeTealPaint', 0x2b9a8c)
EMBER_CYAN = glow('MeleeEmberCyan', 0x56dcff, 5.0)              # Ellis's ember (as the guns' GunCyanGlow)
EMBER_ORANGE = glow('MeleeEmberOrange', 0xff7a24, 4.0)          # Gauge's firebox (as GunEmberGlow)


def mapped(obj, mat=None, along=None):
    """Puts mat on every face (None: keeps the faces' own) and maps UV0 at 1024 px/m. Every set used here is 1024 px,
    so one projection serves them all; wood takes along='long' so its grain runs along handles and poles."""
    if mat is not None:
        lt.assign(obj, mat)
    lt.box_uv(obj, 'Polymer', texel_density=DENSITY, along=along)
    return obj


def wood(obj, mat):
    return mapped(obj, mat, along='long')


# --- Shapes (tool space, cm) ---

def _lathe(profile, segments, spin=0.0, closed=False):
    """A surface of revolution round Z from [(z, r), ...] (cm); open ends capped at the axis unless closed (a ring)."""
    pts = [(r * CM, z * CM) for z, r in profile]
    if not closed:
        if pts[0][0] > 1e-6:
            pts.insert(0, (0.0, pts[0][1]))
        if pts[-1][0] > 1e-6:
            pts.append((0.0, pts[-1][1]))
    obj = lp.lathe(pts, segments=segments, closed=closed)[0]
    if spin:
        obj.data.transform(Matrix.Rotation(math.radians(spin), 4, 'Z'))
    return obj


Z_TO_U = Matrix.Rotation(math.radians(90.0), 4, 'X')    # Blender Z onto -Y: the lathe's axis along u
Z_TO_W = Matrix.Rotation(math.radians(90.0), 4, 'Y')    # Blender Z onto +X: along w


def turned(profile, u0, v=0.0, w=0.0, segments=14, spin=0.0):
    """Turned round an axis along u through (v, w): profile [(du, r), ...] from u0."""
    obj = _lathe(profile, segments, spin)
    obj.data.transform(Matrix.Translation(at(u0, v, w)) @ Z_TO_U)
    return obj


def ring(u0, u1, r_in, r_out, v=0.0, w=0.0, segments=20, chamfer=0.15):
    """A hollow ring round an axis along u (a clamp, a band), its outer edges chamfered."""
    c = chamfer
    loop = [(0.0, r_in), (0.0, r_out - c), (c, r_out), (u1 - u0 - c, r_out), (u1 - u0, r_out - c), (u1 - u0, r_in)]
    obj = _lathe(loop, segments, closed=True)
    obj.data.transform(Matrix.Translation(at(u0, v, w)) @ Z_TO_U)
    return obj


def across(profile, u, v, w0, segments=12):
    """Turned round an axis along w (a pin, a bolt, a vent pipe): profile [(dw, r), ...] from w0."""
    obj = _lathe(profile, segments)
    obj.data.transform(Matrix.Translation(at(u, v, w0)) @ Z_TO_W)
    return obj


def dome(center, axis, r, h, segments=8):
    """A rivet head of radius r, h high, on a surface at center (tool space), bulging along axis ('u', 'v', '-v')."""
    obj = _lathe([(0.0, r), (h * 0.45, r * 0.85), (h, 0.0)], segments)
    turn = {'v': Matrix(), '-v': Matrix.Rotation(math.pi, 4, 'Y'), 'u': Z_TO_U, 'w': Z_TO_W}[axis]
    obj.data.transform(Matrix.Translation(at(*center)) @ turn)
    return obj


def fillet(outline, radius, steps=3):
    """Rounds every corner of an outline by about radius."""
    if radius <= 0.0:
        return outline
    pts = [Vector(p) for p in outline]
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


def _prism(front, back, bevel, segments=2, sharp=28.0):
    """Joins two matching loops of points (Blender) into a closed prism, sharp edges bevelled by bevel cm."""
    bm = lp.new_bmesh()
    a = [bm.verts.new(p) for p in front]
    b = [bm.verts.new(p) for p in back]
    n = len(a)
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
    return lp.mesh_object(bm)


def slab(outline, width, w=0.0, bevel=0.2, round=0.0, segments=2):
    """A side outline [(u, v), ...] extruded across the tool (along w), width cm centered on w."""
    outline = fillet(outline, round)
    return _prism([at(u, v, w - width * 0.5) for u, v in outline], [at(u, v, w + width * 0.5) for u, v in outline],
                  bevel, segments)


def face(outline, u0, u1, bevel=0.2, round=0.0, segments=2):
    """A plate seen from the front: outline [(w, v), ...] extruded along u from u0 to u1."""
    outline = fillet(outline, round)
    return _prism([at(u0, v, w) for w, v in outline], [at(u1, v, w) for w, v in outline], bevel, segments)


def flat(outline, thick, v=0.0, bevel=0.15, round=0.0, segments=1):
    """A plate lying flat in the u-w plane: outline [(u, w), ...], thick cm along v, centered on v."""
    outline = fillet(outline, round)
    return _prism([at(u, v - thick * 0.5, w) for u, w in outline], [at(u, v + thick * 0.5, w) for u, w in outline],
                  bevel, segments)


def sweep(points, radius, sides=6, scales=None, squash=1.0, up=(1.0, 0.0, 0.0), start=0.0):
    """A bar swept along tool-space points [(u, v, w), ...]: an n-sided section radius cm, scaled per point."""
    return lp.sweep([at(*p) for p in points], lp.ngon(radius * CM, sides, squash=squash, start=start), up=up,
                    scales=scales)


def finish(name, parts, sockets):
    """Joins the parts into the model, finishes it (shading, baked occlusion, no Nanite, no collision) and adds its
    sockets: [(name, (u, v, w), rotation degrees), ...]."""
    model = lp.join(name, parts)
    lm.smooth(model, 30.0)
    lt.bake_vertex_ao(model, samples=16, distance=0.04, ground=False)
    model['Nanite'] = 0
    model['Collision'] = 'None'
    for socket_name, uvw, rotation in sockets:
        lm.socket(model, socket_name, at(*uvw), rotation)
    lt._log(f'{name}: {lp.tri_count(model)} triangles; materials {", ".join(m.name for m in model.data.materials)}')
    return model


# --- Shared 2D helpers ---

def _area(poly):
    return 0.5 * sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(poly, poly[1:] + poly[:1]))


def inset(poly, d):
    """A convex outline moved inward by d (each corner along its bisector)."""
    sign = 1.0 if _area(poly) > 0.0 else -1.0
    out = []
    n = len(poly)
    for i in range(n):
        p0, p1, p2 = Vector(poly[i - 1]), Vector(poly[i]), Vector(poly[(i + 1) % n])
        e0, e1 = (p1 - p0).normalized(), (p2 - p1).normalized()
        n0, n1 = Vector((-e0.y, e0.x)) * sign, Vector((-e1.y, e1.x)) * sign
        k = 1.0 + n0.dot(n1)
        out.append(tuple(p1 + (n0 + n1) * (d / max(k, 0.2))))
    return out


def rounded_rect(x0, x1, y0, y1, r, steps=3):
    """A rectangle's outline with round corners, counterclockwise; always 4 * (steps + 1) points."""
    r = max(r, 0.02)
    out = []
    for cx, cy, a0 in ((x1 - r, y1 - r, 0.0), (x0 + r, y1 - r, 90.0), (x0 + r, y0 + r, 180.0), (x1 - r, y0 + r, 270.0)):
        for k in range(steps + 1):
            a = math.radians(a0 + 90.0 * k / steps)
            out.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    return out


# =====================================================================================================================
# Ellis: the Grave Spade
# =====================================================================================================================

SPADE_TIP = 76.0              # the blade's tip (u); the D-grip's foot is at -19: 95 cm
BLADE_TOP = 48.8              # the blade's shoulders
BLADE_THICK = 0.22            # half the iron's thickness in the middle
EDGE_THICK = 0.06             # half its thickness at the worn edge
EDGE_BAND = 1.3               # how far in the bright edge reaches
DISH = 0.012                  # the blade's dish: v drops DISH * w^2 toward its sides (1.1 cm at the edge)


def dish(w):
    return -DISH * w * w


def blade_outline():
    side = [(BLADE_TOP + 0.2, 8.6), (BLADE_TOP + 0.8, 9.4), (51.0, 9.6), (57.0, 9.55), (62.5, 9.2), (66.8, 8.4),
            (70.2, 7.0), (72.9, 5.2), (74.8, 3.1), (75.8, 1.2)]
    top = [(BLADE_TOP + 0.1, -4.4), (BLADE_TOP, 0.0), (BLADE_TOP + 0.1, 4.4)]
    return top + side + [(SPADE_TIP, 0.0)] + [(u, -w) for u, w in reversed(side)]


def spade_blade():
    """The blade: a dished iron plate whose last EDGE_BAND cm thins to a bright worn edge, as one closed mesh."""
    outline = blade_outline()
    rings = [(outline, EDGE_THICK), (inset(outline, EDGE_BAND), BLADE_THICK), (inset(outline, 3.6), BLADE_THICK),
             (inset(outline, 6.2), BLADE_THICK)]
    cu = sum(p[0] for p in rings[-1][0]) / len(rings[-1][0])
    cw = sum(p[1] for p in rings[-1][0]) / len(rings[-1][0])
    bm = lp.new_bmesh()
    loops = {}
    for side in (1.0, -1.0):
        for k, (poly, t) in enumerate(rings):
            loops[side, k] = [bm.verts.new(at(u, side * t + dish(w), w)) for u, w in poly]
        centre = bm.verts.new(at(cu, side * BLADE_THICK + dish(cw), cw))
        for k in range(1, len(rings)):
            a, b = loops[side, k - 1], loops[side, k]
            for i in range(len(a)):
                j = (i + 1) % len(a)
                f = bm.faces.new((a[i], a[j], b[j], b[i]))
                f.material_index = 1 if k == 1 else 0
        last = loops[side, len(rings) - 1]
        for i in range(len(last)):
            bm.faces.new((last[i], last[(i + 1) % len(last)], centre)).material_index = 0
    a, b = loops[1.0, 0], loops[-1.0, 0]
    for i in range(len(a)):
        j = (i + 1) % len(a)
        bm.faces.new((a[i], a[j], b[j], b[i])).material_index = 1
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = lp.mesh_object(bm)
    obj.data.materials.append(IRON)
    obj.data.materials.append(STEEL_BRIGHT)
    return mapped(obj)


CRACK = [(52.9, 1.2), (54.4, 2.6), (55.3, 2.2), (56.9, 3.4), (58.0, 1.9), (59.6, 2.3), (61.0, 0.4), (62.3, 0.9),
         (63.6, -0.9), (65.4, -1.3), (66.7, -2.9), (68.9, -3.6)]
CRACK_BRANCH = [(58.0, 1.9), (59.4, 4.0), (60.1, 4.3), (61.6, 5.6)]


def crack(points, width, side):
    """The ember crack on one face of the blade: a thin glowing strip lying on the dished iron."""
    pts = []
    for (u0, w0), (u1, w1) in zip(points, points[1:]):
        for k in range(2):
            s = k / 2.0
            pts.append((u0 + (u1 - u0) * s, w0 + (w1 - w0) * s))
    pts.append(points[-1])
    half = width * 0.5
    lift = side * (BLADE_THICK + 0.035)
    # Widest a third of the way along, thinning to a hairline at its far end.
    n = len(pts) - 1
    scales = [0.5 + 0.7 * min(i / n / 0.3, 1.0) - 0.9 * max(0.0, i / n - 0.3) / 0.7 for i in range(n + 1)]
    obj = lp.sweep([at(u, lift + dish(w), w) for u, w in pts], [(-half * CM, -0.06 * CM), (half * CM, -0.06 * CM),
                                                               (half * CM, 0.06 * CM), (-half * CM, 0.06 * CM)],
                   up=(0.0, 0.0, 1.0), scales=scales)
    return mapped(obj, EMBER_CYAN)


def spade_d_grip():
    """The D-grip: two ash cheeks split from the shaft, out round to an ash crossbar, iron-pinned at its ends."""
    parts = []
    for s in (1.0, -1.0):
        path = [(-5.0, 0.0, 0.35 * s), (-6.6, 0.0, 1.2 * s), (-8.4, 0.0, 2.8 * s), (-10.6, 0.0, 4.3 * s),
                (-13.0, 0.0, 5.3 * s), (-15.4, 0.0, 5.75 * s), (-17.6, 0.0, 5.85 * s)]
        cheek = sweep(path, 0.85, sides=8, squash=1.45, up=(0.0, 0.0, 1.0),
                      scales=[0.9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.05])
        parts.append(wood(cheek, ASH))
        pin_head = dome((-17.6, 0.0, 6.2), 'w', 0.55, 0.3)
        parts.append(mapped(pin_head if s > 0 else _flip_w(pin_head), IRON))
    bar = across([(0.0, 1.35), (0.6, 1.5), (11.8, 1.5), (12.4, 1.35)], -17.6, 0.0, -6.2, segments=12)
    parts.append(wood(bar, ASH))
    ferrule = turned([(0.0, 1.95), (0.3, 2.05), (3.0, 2.05), (3.3, 1.9)], -7.6, segments=14)
    parts.append(mapped(ferrule, IRON))
    return parts


def _flip_w(obj):
    obj.data.transform(Matrix.Scale(-1.0, 4, (1.0, 0.0, 0.0)))
    bmesh_flip(obj)
    return obj


def bmesh_flip(obj):
    """Mirroring turns faces inside out: turn them back."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()


def grave_spade():
    parts = [spade_blade()]
    parts.append(crack(CRACK, 0.32, 1.0))
    parts.append(crack(CRACK_BRANCH, 0.22, 1.0))
    parts.append(crack(CRACK, 0.32, -1.0))
    parts.append(crack(CRACK_BRANCH, 0.22, -1.0))
    # The tread: the blade's top rolled over for the boot, following the dish.
    tread_w = [-9.4 + 18.8 * k / 8.0 for k in range(9)]
    tread = lp.sweep([at(BLADE_TOP + 0.55, dish(w), w) for w in tread_w],
                     [(-0.85 * CM, -0.42 * CM), (0.85 * CM, -0.42 * CM), (0.85 * CM, 0.42 * CM), (-0.85 * CM, 0.42 * CM)],
                     up=(0.0, 0.0, 1.0))
    parts.append(mapped(tread, IRON))
    # The socket the shaft sits in, flaring into the frog that runs down both faces.
    parts.append(mapped(turned([(0.0, 1.9), (0.6, 2.05), (8.5, 2.3), (11.8, 2.55), (12.6, 2.0)], 37.0, segments=14),
                        IRON))
    for s in (1.0, -1.0):
        frog = flat([(BLADE_TOP - 0.5, -2.5), (BLADE_TOP - 0.5, 2.5), (58.0, 0.6), (58.0, -0.6)], 0.5,
                    v=s * (BLADE_THICK + 0.2), bevel=0.12)
        parts.append(mapped(frog, IRON))
        parts.append(mapped(dome((43.5, s * 2.25, 0.0), 'v' if s > 0 else '-v', 0.42, 0.25), IRON))
    parts.append(wood(turned([(0.0, 1.55), (6.0, 1.6), (40.0, 1.78), (46.0, 1.95)], -6.0, segments=14), ASH))
    parts.extend(spade_d_grip())
    return finish('Melee_GraveSpade', parts, [('Grip', (0.0, 0.0, 0.0), (0, 0, 0)),
                                              ('Tip', (SPADE_TIP, 0.0, 0.0), (0, 0, 0))])


# =====================================================================================================================
# Odessa: the Razor Fan
# =====================================================================================================================

CARD_HALF = 3.5               # half the card's width (cm)
CARD_BOTTOM, CARD_TOP = -1.3, 8.1    # about the pivot: 9.4 cm tall
CARD_CORNER = 0.6
FAN_PIVOT = 2.6               # the pivot rivet's place along u (the cards' feet in the fist's index side)
FAN_ANGLES = (-26.0, -10.0, 6.0, 22.0, 38.0)   # bottom card to top, toward +Z: the fan leads with its top card
FAN_STEP = 0.17               # the stack's spacing along w (the top card nearest the back of the hand, +w)
# (inset, half thickness): the honed steel edge, the card's margin, the gold rim, the card's middle
CARD_RINGS = ((0.0, 0.025), (0.3, 0.075), (0.55, 0.075), (0.78, 0.075))
SUITS = ('spade', 'diamond', 'club', 'heart', 'spade')


def card_point(a, b, t, theta, w0):
    c, s = math.cos(math.radians(theta)), math.sin(math.radians(theta))
    return at(FAN_PIVOT + b * c - a * s, b * s + a * c, w0 + t)


def card(theta, w0, face_side):
    """One card: a thin slab whose edge is honed steel, with a gold rim inset on both faces."""
    slots = [STEEL_BRIGHT, CARD_FACE, CARD_BACK, GOLD]
    bm = lp.new_bmesh()
    loops = {}
    for side in (1.0, -1.0):
        middle = 1 if side == face_side else 2
        mats = [0, middle, 3]
        for k, (d, t) in enumerate(CARD_RINGS):
            # Corners kept round inside too (a radius near 0 left slivers without area: tangent trouble in Unreal).
            poly = rounded_rect(-CARD_HALF + d, CARD_HALF - d, CARD_BOTTOM + d, CARD_TOP - d,
                                max(CARD_CORNER - d * 0.6, 0.15))
            loops[side, k] = [bm.verts.new(card_point(a, b, side * t, theta, w0)) for a, b in poly]
        for k in range(1, len(CARD_RINGS)):
            a, b = loops[side, k - 1], loops[side, k]
            for i in range(len(a)):
                j = (i + 1) % len(a)
                bm.faces.new((a[i], a[j], b[j], b[i])).material_index = mats[k - 1]
        bm.faces.new(loops[side, len(CARD_RINGS) - 1]).material_index = middle
    a, b = loops[1.0, 0], loops[-1.0, 0]
    for i in range(len(a)):
        j = (i + 1) % len(a)
        bm.faces.new((a[i], a[j], b[j], b[i])).material_index = 0
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    obj = lp.mesh_object(bm)
    for m in slots:
        obj.data.materials.append(m)
    return mapped(obj)


def suit_outline(suit):
    """A suit's outline, about 1 across, centered, its point or lobe up (b)."""
    if suit == 'diamond':
        return [(0.0, -0.62), (0.42, 0.0), (0.0, 0.62), (-0.42, 0.0)]
    if suit == 'heart':
        pts = []
        for k in range(36):
            t = math.pi * 2.0 * k / 36.0
            x = 16.0 * math.sin(t) ** 3
            y = 13.0 * math.cos(t) - 5.0 * math.cos(2 * t) - 2.0 * math.cos(3 * t) - math.cos(4 * t)
            pts.append((x / 34.0, (y + 2.5) / 34.0))
        return pts
    stem = [(-0.05, -0.18), (-0.2, -0.52), (0.2, -0.52), (0.05, -0.18)]
    if suit == 'spade':
        pts = []
        for k in range(1, 36):
            t = math.pi * 2.0 * k / 36.0
            x = 16.0 * math.sin(t) ** 3
            y = -(13.0 * math.cos(t) - 5.0 * math.cos(2 * t) - 2.0 * math.cos(3 * t) - math.cos(4 * t))
            pts.append((x / 34.0, (y + 2.5) / 34.0 + 0.05))
        # The heart upside down, the stem dropped from its notch.
        return pts + stem
    # club: three lobes (up, lower left, lower right), the stem from the notch between the lower two
    pts = []
    for k in range(24):
        phi = math.pi * 2.0 * k / 24.0 + math.pi * 1.5 + 0.13
        r = 0.31 * (1.0 + 0.62 * math.cos(3.0 * (phi - math.pi * 0.5)))
        pts.append((math.cos(phi) * r, math.sin(phi) * r + 0.06))
    return pts + stem


def pip(suit, a0, b0, size, theta, w0, side, mat, turn=0.0):
    """A suit printed on a card: a flat outline a hair proud of the card's face."""
    t = side * (CARD_RINGS[-1][1] + 0.01)
    c, s = math.cos(math.radians(turn)), math.sin(math.radians(turn))
    bm = lp.new_bmesh()
    verts = []
    for x, y in suit_outline(suit):
        a, b = a0 + (x * c - y * s) * size, b0 + (x * s + y * c) * size
        verts.append(bm.verts.new(card_point(a, b, t, theta, w0)))
    f = bm.faces.new(verts)
    f.normal_update()
    if f.normal.x * side < 0.0:          # facing out of the card (w is Blender X)
        bmesh.ops.reverse_faces(bm, faces=[f])
    bmesh.ops.triangulate(bm, faces=[f], ngon_method='BEAUTY')
    return mapped(lp.mesh_object(bm), mat)


def razor_fan():
    parts = []
    face_side = 1.0           # the faces toward +w (Unreal -Y): the back of the hand's side, the way the player looks
    n = len(FAN_ANGLES)
    for k, theta in enumerate(FAN_ANGLES):
        w0 = (k - (n - 1) * 0.5) * FAN_STEP
        parts.append(card(theta, w0, face_side))
        suit = SUITS[k]
        ink = CARD_RED if suit in ('diamond', 'heart') else CARD_INK
        # The corner pip on the top corner the next card up leaves showing (its -Z side).
        parts.append(pip(suit, -CARD_HALF + 1.25, CARD_TOP - 1.35, 0.95, theta, w0, face_side, ink))
        if k == n - 1:
            parts.append(pip('spade', 0.0, 3.6, 3.6, theta, w0, face_side, CARD_INK))           # the ace
            parts.append(pip('spade', CARD_HALF - 1.25, CARD_BOTTOM + 1.35, 0.95, theta, w0, face_side, CARD_INK,
                             turn=180.0))
        if k == 0:
            parts.append(pip('spade', 0.0, 3.4, 2.2, theta, w0, -face_side, GOLD))            # the deck's back
    span = (n - 1) * 0.5 * FAN_STEP + 0.075
    parts.append(mapped(across([(0.0, 0.3), (2.0 * span + 0.2, 0.3)], FAN_PIVOT, 0.0, -span - 0.1, segments=8),
                        BRASS))
    for s in (1.0, -1.0):
        rivet = dome((FAN_PIVOT, 0.0, s * (span + 0.05)), 'w', 0.5, 0.26)
        if s < 0.0:
            rivet = _flip_w(rivet)
        parts.append(mapped(rivet, BRASS))
    tip_theta = math.radians(FAN_ANGLES[n // 2])
    tip = (FAN_PIVOT + CARD_TOP * math.cos(tip_theta), CARD_TOP * math.sin(tip_theta), 0.0)
    return finish('Melee_RazorFan', parts, [('Grip', (0.0, 0.0, 0.0), (0, 0, 0)), ('Tip', tip, (0, 0, 0))])


# =====================================================================================================================
# Crane: the Ferry Gaff
# =====================================================================================================================

GAFF_BUTT, GAFF_TIP = -14.0, 106.0      # 120 cm
POLE_R = 1.75
HOOK = [(89.5 + (u - 89.5) * 1.25, 0.6 + (v - 0.6) * 1.25) for u, v in
        ((89.5, 0.6), (91.2, 2.6), (93.0, 4.5), (94.8, 6.0), (96.3, 7.6), (97.2, 9.2), (97.0, 10.6), (96.0, 11.5),
         (94.6, 11.5), (93.6, 10.7), (93.1, 9.5))]


def fare_coin(u_c, phi_c):
    """The fare coin, bent round the pole and nailed through: a disc with a raised rim, its nail's head proud."""
    def wrap(obj):
        me = obj.data
        for vtx in me.vertices:
            x, y, z = vtx.co.x / CM, vtx.co.y / CM, vtx.co.z / CM
            phi = phi_c - y / POLE_R          # this way round the wrap is no mirror: faces stay outward
            radial = POLE_R - 0.04 + z
            vtx.co = at(u_c + x, radial * math.sin(phi), radial * math.cos(phi))
        me.update()
        return obj
    coin = wrap(_lathe([(0.0, 0.0), (0.0, 1.5), (0.2, 1.52), (0.3, 1.4), (0.3, 1.18), (0.22, 1.1), (0.22, 0.0)], 16))
    nail = wrap(_lathe([(0.2, 0.34), (0.32, 0.3), (0.42, 0.0)], 8))
    return [mapped(coin, COIN), mapped(nail, IRON)]


def ferry_gaff():
    parts = [wood(turned([(0.0, POLE_R - 0.03), (GAFF_TIP - 18.0 - GAFF_BUTT, POLE_R + 0.03)], GAFF_BUTT + 0.4,
                         segments=14), ASH_GREY)]
    # The butt cap, a band over the fist, and the band under the head's socket.
    parts.append(mapped(turned([(0.0, 1.55), (0.4, 1.9), (3.2, 1.9), (3.5, 1.82)], GAFF_BUTT, segments=14), IRON))
    parts.append(mapped(ring(5.8, 7.2, POLE_R - 0.1, POLE_R + 0.2, segments=14), IRON))
    parts.append(mapped(ring(80.0, 81.0, POLE_R - 0.1, POLE_R + 0.18, segments=14), IRON))
    # The head: a tapered iron socket, a square spike and the hook forged off its side.
    parts.append(mapped(turned([(0.0, 2.0), (1.0, 2.1), (2.0, 1.98), (9.0, 1.3), (10.0, 1.15)], 86.0, segments=12),
                        IRON))
    parts.append(mapped(turned([(0.0, 1.05), (1.5, 0.95), (GAFF_TIP - 95.5, 0.0)], 95.5, segments=4, spin=45.0), IRON))
    hook = sweep([(u, v, 0.0) for u, v in HOOK], 1.0, sides=6, squash=0.8,
                 scales=[1.0, 0.95, 0.9, 0.84, 0.78, 0.7, 0.62, 0.53, 0.43, 0.32, 0.12])
    parts.append(mapped(hook, IRON))
    for s in (1.0, -1.0):
        rivet = dome((88.6, 0.0, s * 1.98), 'w', 0.35, 0.2)
        parts.append(mapped(rivet if s > 0 else _flip_w(rivet), IRON))
    parts.extend(fare_coin(46.0, 0.0))
    return finish('Melee_FerryGaff', parts, [('Grip', (0.0, 0.0, 0.0), (0, 0, 0)),
                                             ('Tip', (GAFF_TIP, 0.0, 0.0), (0, 0, 0)),
                                             ('Hook', (HOOK[-1][0], HOOK[-1][1], 0.0), (0, 0, 0))])


# =====================================================================================================================
# Gauge: the Piston Fist
# =====================================================================================================================

ARM_BORE = 4.7                # the clamp rings' inside (his casings are 3.9-4.45 cm)
CYL_V = 7.9                   # the cylinder's axis over the forearm's
PLATE_U = 20.0                # the knuckle plate's back face
KNUCKLES = (-2.75, 0.0, 2.75) # the three knuckle caps across the plate (w)
KNUCKLE_V = 0.9


def piston_fist():
    parts = []
    # The clamp: a front ring with its split lug underneath, a brass rear ring, the saddle joining them on top.
    parts.append(mapped(ring(-2.0, 2.4, ARM_BORE, ARM_BORE + 0.75, segments=20), IRON))
    parts.append(mapped(ring(-10.4, -8.8, ARM_BORE + 0.2, ARM_BORE + 0.75, segments=20), BRASS))
    parts.append(mapped(slab([(-1.6, -ARM_BORE - 0.4), (2.0, -ARM_BORE - 0.4), (2.0, -6.7), (-1.6, -6.7)], 2.6,
                             bevel=0.2), IRON))
    parts.append(mapped(across([(0.0, 0.42), (3.6, 0.42)], 0.2, -5.9, -1.8, segments=8), STEEL_HEAD))
    for s in (1.0, -1.0):
        nut = across([(0.0, 0.72), (0.5, 0.72)], 0.2, -5.9, 1.3 if s > 0 else -1.8, segments=6)
        parts.append(mapped(nut, IRON))
    parts.append(mapped(slab([(-10.6, ARM_BORE + 0.3), (3.0, ARM_BORE + 0.3), (3.0, 6.5), (-10.6, 6.5)], 3.6,
                             bevel=0.25), IRON))
    # The cylinder, its end flanges and gland, the rod and the yoke onto the knuckle plate.
    parts.append(mapped(turned([(0.0, 2.0), (17.0, 2.0)], -9.0, v=CYL_V, segments=16), BRASS))
    parts.append(mapped(turned([(0.0, 2.2), (0.2, 2.45), (1.0, 2.45), (1.2, 2.2)], -9.8, v=CYL_V, segments=16), IRON))
    parts.append(mapped(turned([(0.0, 2.2), (0.2, 2.45), (1.0, 2.45), (1.2, 2.2), (1.2, 1.3), (2.0, 1.25)], 7.6,
                               v=CYL_V, segments=16), IRON))
    for u in (-3.5, 2.0):
        parts.append(mapped(ring(u, u + 0.7, 1.95, 2.18, v=CYL_V, segments=16, chamfer=0.08), IRON))
    parts.append(mapped(turned([(0.0, 0.85), (6.4, 0.85)], 9.4, v=CYL_V, segments=10), STEEL_BRIGHT))
    parts.append(mapped(slab([(15.0, 9.4), (17.4, 9.4), (PLATE_U + 1.2, 4.6), (PLATE_U + 1.2, 3.0), (PLATE_U - 0.4, 3.0),
                              (15.0, 6.6)], 3.0, bevel=0.25), IRON))
    # The knuckle plate, its rivets, and three knuckle caps glowing from the firebox.
    plate = face([(w, v) for w, v in rounded_rect(-4.3, 4.3, -3.9, 3.6, 1.2)], PLATE_U, PLATE_U + 1.4, bevel=0.25)
    parts.append(mapped(plate, IRON))
    for w in (-3.55, 3.55):
        for v in (2.85, -3.15):
            parts.append(mapped(dome((PLATE_U + 1.4, v, w), 'u', 0.36, 0.22), BRASS))
    for w in KNUCKLES:
        parts.append(mapped(turned([(0.0, 1.45), (0.8, 1.45)], PLATE_U + 1.3, v=KNUCKLE_V, w=w, segments=12), IRON))
        parts.append(mapped(turned([(0.0, 1.3), (0.35, 1.25), (0.7, 1.0), (0.95, 0.0)], PLATE_U + 2.1, v=KNUCKLE_V, w=w,
                                   segments=12), EMBER_ORANGE))
    # The steam vent: a short brass pipe out of the cylinder's side, flared at its mouth.
    vent_w0, vent_len = 1.6, 2.1
    parts.append(mapped(across([(0.0, 0.42), (1.6, 0.42), (1.9, 0.62), (vent_len, 0.62)], 5.5, CYL_V, vent_w0,
                               segments=10), BRASS))
    tip = (PLATE_U + 3.05, KNUCKLE_V, 0.0)
    return finish('Melee_PistonFist', parts, [('Forearm', (0.0, 0.0, 0.0), (0, 0, 0)), ('Tip', tip, (0, 0, 0)),
                                              ('Vent', (5.5, CYL_V, vent_w0 + vent_len), (0, 0, 90))])


# =====================================================================================================================
# Pike: the Rock Pick
# =====================================================================================================================

PICK_BUTT = -10.0
HEAD_U = 24.0                 # the head's middle along the handle (the eye)
OVAL = 0.8                    # the handle's oval: its width across (w) to its depth (v)
PICK = [(HEAD_U, 1.6), (HEAD_U + 0.25, 4.6), (HEAD_U - 0.1, 7.6), (HEAD_U - 1.0, 10.4), (HEAD_U - 2.4, 12.7),
        (HEAD_U - 3.9, 14.3)]


def oval(obj):
    obj.data.transform(Matrix.Diagonal((OVAL, 1.0, 1.0, 1.0)))
    return obj


def handle_radius(du):
    profile = [(0.0, 1.25), (0.5, 1.5), (1.6, 1.58), (3.5, 1.45), (10.0, 1.38), (22.0, 1.2), (33.0, 1.3), (35.6, 1.3)]
    for (a, ra), (b, rb) in zip(profile, profile[1:]):
        if a <= du <= b:
            return ra + (rb - ra) * (du - a) / (b - a)
    return profile[-1][1]


def rock_pick():
    profile = [(0.0, 1.25), (0.5, 1.5), (1.6, 1.58), (3.5, 1.45), (10.0, 1.38), (22.0, 1.2), (33.0, 1.3), (35.6, 1.3)]
    parts = [wood(oval(turned(profile, PICK_BUTT, segments=12)), HICKORY)]
    # Brass wire binding at the butt and over the fist, the teal band under the head.
    for u0, u1 in ((-8.9, -8.1), (-7.5, -6.7), (4.8, 5.6)):
        r = handle_radius((u0 + u1) * 0.5 - PICK_BUTT)
        parts.append(mapped(oval(ring(u0, u1, r - 0.05, r + 0.14, segments=12, chamfer=0.1)), BRASS))
    r = handle_radius(18.4 - PICK_BUTT)
    parts.append(mapped(oval(ring(17.4, 19.4, r - 0.05, r + 0.05, segments=12, chamfer=0.03)), TEAL))
    # The head: the eye, the square hammer poll (-Z), the pick curving down toward the hand (+Z), a wedge on top.
    parts.append(mapped(slab([(HEAD_U - 1.8, -2.0), (HEAD_U + 1.8, -2.0), (HEAD_U + 1.8, 2.0), (HEAD_U - 1.8, 2.0)],
                             3.2, bevel=0.3), STEEL_HEAD))
    parts.append(mapped(slab([(HEAD_U - 1.35, -1.5), (HEAD_U + 1.35, -1.5), (HEAD_U + 1.45, -5.6), (HEAD_U + 1.55, -6.6),
                              (HEAD_U - 1.55, -6.6), (HEAD_U - 1.45, -5.6)], 2.7, bevel=0.3), STEEL_HEAD))
    pick = sweep([(u, v, 0.0) for u, v in PICK], 1.1 * math.sqrt(2.0), sides=4, start=math.pi * 0.25,
                 scales=[1.0, 0.78, 0.6, 0.42, 0.24, 0.04], up=(1.0, 0.0, 0.0))
    parts.append(mapped(pick, STEEL_HEAD))
    parts.append(mapped(slab([(HEAD_U + 1.75, -0.9), (HEAD_U + 2.1, -0.9), (HEAD_U + 2.1, 0.9), (HEAD_U + 1.75, 0.9)],
                             0.45, bevel=0.05), BRASS))
    for s in (1.0, -1.0):
        rivet = dome((HEAD_U, 0.0, s * 1.6), 'w', 0.38, 0.2)
        parts.append(mapped(rivet if s > 0 else _flip_w(rivet), STEEL_BRIGHT))
    return finish('Melee_RockPick', parts, [('Grip', (0.0, 0.0, 0.0), (0, 0, 0)),
                                            ('Tip', (PICK[-1][0], PICK[-1][1], 0.0), (0, 0, 0))])


# =====================================================================================================================

for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
MODELS = [grave_spade(), razor_fan(), ferry_gaff(), piston_fist(), rock_pick()]

if lt.want_preview():
    bpy.context.view_layer.update()
    views = {'Melee_GraveSpade': ((0.25, -0.35, 1.0), (1.0, -0.2, 0.35)),
             'Melee_RazorFan': ((1.0, -0.3, 0.35), (-1.0, -0.3, 0.35)),
             'Melee_FerryGaff': ((1.0, -0.25, 0.3), (0.7, -0.9, 0.5)),
             'Melee_PistonFist': ((0.9, -0.9, 0.6), (0.8, 0.5, 0.7)),
             'Melee_RockPick': ((1.0, -0.25, 0.3), (0.6, -0.8, -0.5))}
    for model in MODELS:
        for tag, view in zip('AB', views[model.name]):
            lt.preview([model], os.path.join(PREVIEW_DIR, f'{model.name}_{tag}.png'), view=view, ground=False,
                       resolution=(1280, 720), fit=1.05)
