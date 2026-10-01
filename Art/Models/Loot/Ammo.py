"""The ammo pickups (AAmmoPickup): each ammo type's icon modeled in 3D. The user's calls on 2026-10-01: instead of
ammo boxes, "the icon of the actual ammo should be modeled 3D" (like Fortnite's ammo); with the icons' dark ink line;
and clustered, since ammo never drops in amounts under 10, so one bundle per type serves every amount.

Each model, SM_Ammo<Type>, is a bundle of its type's round, turned from the very outline its icon is drawn from
(Art/Icons/InkedIcons.py, ROUNDS), so the pickup on the ground matches the icon in the HUD and the inventory. The rounds
stand on end round a middle one, the outer ones leaning out a little:
  AmmoAssaultRifle  seven bottlenecked rounds with pointed bullets
  AmmoShotgun       five shotgun shells: brass heads, oxblood paper hulls, star crimps
  AmmoPistol        five fat straight rounds with round-nosed bullets
  AmmoSMG           seven small short rounds with flat-nosed bullets
  AmmoSniper        four tall bottlenecked rounds
Each round wears the icons' ink line: an inverted hull, its faces turned inward, so a one-sided material shows only the
rim behind the round. It joins the model after the ambient occlusion bake (baked with it, it would shade the round
black).

They're oversized to read in the grass: about 20 cm across and 13-26 cm tall. The origin is the middle of the
bundle's footprint on the ground, and AAmmoPickup spins the bundle about it. Materials: tinted texture sets the game
already loads (MetalWorn brass and copper, Polymer for the hulls and the ink), so no new textures. No collision (the
pickup has its own sphere), no Nanite, no LODs (a reduced inverted hull could cut into its round).

    blender -b --factory-startup --python Art/Models/Loot/Ammo.py -- --preview
"""
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp

sys.path.append(os.path.join(lt.REPO, 'Art', 'Icons'))
import InkedIcons   # noqa: E402  (the icons' outlines)

TYPES = ('AssaultRifle', 'Shotgun', 'Pistol', 'SMG', 'Sniper')
UNIT = 0.0055        # metres per icon unit, before each bundle's own scale
INK_WIDTH = 0.006    # the ink line's thickness
SEGMENTS = 10        # round each round's axis (a shell: 12, for its six-fold crimp); the ink shell matches

# Each bundle: how many rounds stand round the middle one, the rounds' scale, how far the outer ones lean (degrees).
BUNDLES = {
    'AssaultRifle': (6, 0.85, 6.0),
    'Shotgun': (4, 0.62, 7.0),
    'Pistol': (4, 0.8, 8.0),
    'SMG': (6, 0.72, 6.0),
    'Sniper': (3, 0.8, 5.0),
}

BRASS = lt.material('MetalWorn', name='AmmoCaseBrass', tint=0xf4c870)
COPPER = lt.material('MetalWorn', name='AmmoBulletCopper', tint=0xd0835a)
HULL = lt.material('Polymer', name='AmmoShellHull', tint=0x9c4434)            # an old oxblood paper hull
INK_LINE = lt.material('Polymer', name='AmmoInkLine', tint=0x0a1218)
INK_LINE.use_backface_culling = True          # as the game's one-sided material: only the rim behind the round shows
INK_LINE.use_backface_culling_shadow = True   # and it casts no shadow over the round inside it


def dedupe(points):
    out = []
    for p in points:
        if not out or abs(p[0] - out[-1][0]) > 1e-9 or abs(p[1] - out[-1][1]) > 1e-9:
            out.append(p)
    return out


# --- One round's profiles, from its icon outline (x right and y down in icon units, to radius and height in m) ---

def cartridge_profiles(args, scale):
    """The case (brass, closed over its mouth), the bullet (copper, seated a little into the case) and the outline
    for the ink line, as lathe profiles [(radius, height)]."""
    cx, base, w, case_top, shoulder_top, neck_w, neck_top, tip, nose = args
    outline, _ = InkedIcons.cartridge(*args)
    right = dedupe(outline[:len(outline) // 2])   # the right half, from the rim up to the tip
    mouth = next(i for i, (x, y) in enumerate(right) if abs(y - neck_top) < 1e-9)   # where the case ends
    case_pts = right[:mouth + 1]
    nose_pts = right[mouth + 1:]
    nose_pts = nose_pts[:1] + nose_pts[2::2]   # every other step of the bullet's curve is plenty at this size
    k = UNIT * scale
    m = lambda p: ((p[0] - cx) * k, (base - p[1]) * k)
    case = [(0.0, 0.0)] + [m(p) for p in case_pts] + [(0.0, m(case_pts[-1])[1])]
    nose_m = [m(p) for p in nose_pts]
    bullet = [(nose_m[0][0], nose_m[0][1] - 0.6 * k)] + nose_m
    closed = [(0.0, nose_m[-1][1])] if nose_m[-1][0] > 1e-9 else []   # a flat nose's top
    groove = w / 2 - 0.9
    rim_and_case = [m(p) for p in case_pts if abs((p[0] - cx) - groove) > 1e-6]   # the ink skips the groove
    return [(case, BRASS), (bullet + closed, COPPER)], [(0.0, 0.0)] + rim_and_case + nose_m + closed, None


def shell_profiles(args, scale):
    """A shotgun shell: brass head with its rim, the hull a hair narrower above it, a top folded into a star crimp."""
    cx, base, w, top = args
    hw, head, h = w / 2, 10.0, base - top
    rim = hw - 1.2
    brass = [(0.0, 0.0), (hw + 0.6, 0.0), (hw + 0.6, 1.8), (hw, 2.4), (hw, head), (0.0, head)]
    hull = [(hw * 0.985, head - 0.4), (hw * 0.985, h - 2.5), (rim, h - 0.4), (rim * 0.6, h - 0.15), (2.0, h - 0.45),
            (0.0, h - 0.9)]
    whole = [(0.0, 0.0), (hw + 0.6, 0.0), (hw + 0.6, 1.8), (hw, 2.4), (hw, h - 2.5), (rim, h - 0.4), (0.0, h - 0.4)]
    k = UNIT * scale
    sc = lambda prof: [(r * k, z * k) for r, z in prof]
    return [(sc(brass), BRASS), (sc(hull), HULL)], sc(whole), (rim * k, (h - 0.4) * k, 1.6 * k)


def crimp(obj, top, radius, depth):
    """Folds a shell's top into six creases meeting at the middle, as a star crimp does."""
    for v in obj.data.vertices:
        r = math.hypot(v.co.x, v.co.y)
        if v.co.z > top - depth * 0.4 and r < radius:
            fold = 0.5 - 0.5 * math.cos(6.0 * math.atan2(v.co.y, v.co.x))
            v.co.z -= depth * fold * (1.0 - r / radius) ** 0.7
    obj.data.update()


def ink_shell(whole, segments):
    """The ink line: the round's outline pushed out by INK_WIDTH, turned, its faces turned inward."""
    out = []
    n = len(whole)
    for i, (r, z) in enumerate(whole):
        if r <= 1e-9:   # on the axis: straight down at the base, straight up at the tip
            out.append((0.0, z - INK_WIDTH if i == 0 else z + INK_WIDTH * 1.2))
            continue
        a, b = Vector(whole[max(i - 1, 0)]), Vector(whole[min(i + 1, n - 1)])
        d = (b - a).normalized()
        out.append((r + d.y * INK_WIDTH, z - d.x * INK_WIDTH))
    obj = lp.lathe(out, segments=segments)[0]
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    lt.assign(obj, INK_LINE)
    return obj


def round_parts(kind, scale):
    """One round standing at the origin: its parts, its ink shell and its radius."""
    what, args = InkedIcons.ROUNDS[kind][0]
    pieces, whole, crimp_at = (cartridge_profiles if what == 'cartridge' else shell_profiles)(args, scale)
    segments = SEGMENTS if what == 'cartridge' else 12
    parts = []
    for profile, mat in pieces:
        obj = lp.lathe(profile, segments=segments)[0]
        lt.assign(obj, mat)
        lp.lathe_uv(obj, mat['TextureSet'])
        if crimp_at and mat is HULL:
            crimp(obj, crimp_at[1], crimp_at[0], crimp_at[2])
        parts.append(obj)
    return parts, ink_shell(whole, segments), max(r for r, _ in pieces[0][0])


def build(kind):
    """The bundle: a round in the middle, the others round it touching, leaning out; then the ink."""
    ring, scale, lean = BUNDLES[kind]
    rng = random.Random(TYPES.index(kind) * 7 + 3)
    parts, shells = [], []
    radius = None
    for i in range(ring + 1):
        pieces, shell, r = round_parts(kind, scale)
        radius = radius or r
        if i == 0:
            spot, tilt = Vector((0.0, 0.0, 0.0)), rng.uniform(-1.5, 1.5)
            axis = Vector((1.0, 0.0, 0.0))
        else:
            a = 2.0 * math.pi * (i - 1) / ring + rng.uniform(-0.06, 0.06)
            spot = Vector((math.cos(a), math.sin(a), 0.0)) * radius * 2.06
            axis = Vector((-math.sin(a), math.cos(a), 0.0))   # leaning about this tips the round's top outward
            tilt = lean + rng.uniform(-1.5, 1.5)
        m = Matrix.Translation(spot) @ Matrix.Rotation(math.radians(tilt), 4, axis) @ Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z')
        for p in pieces + [shell]:
            p.data.transform(m)
            p.data.update()
        parts += pieces
        shells.append(shell)
    model = lp.join(f'Ammo{kind}', parts)
    lp.finish(model, ao=0.05, nanite=False, smooth=40.0)
    # The ink joins after the ambient occlusion bake: baked with it, it would shade the rounds inside it black.
    model = lp.join(model.name, [model] + shells)
    lm.smooth(model, 40.0)
    model['Nanite'] = 0
    model['Collision'] = 'None'
    return model


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
models = [build(kind) for kind in TYPES]
for k, model in enumerate(models):
    model.location.x = (k - 2) * 0.35   # side by side in the scene (the export ignores it)
    lt._log(f'{model.name}: {sum(len(p.vertices) - 2 for p in model.data.polygons)} triangles, '
            f'materials {", ".join(m.name for m in model.data.materials)}')

if lt.want_preview():
    bpy.context.view_layer.update()
    lt.preview(models, lt.preview_path('Loot', 'Ammo'), view=(-0.35, -1.6, 0.55))
