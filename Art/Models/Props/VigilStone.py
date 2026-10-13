"""A vigil stone (World/BossShrine: ABossShrine; the user's call, 2026-10-12: "a pedestal with a menu that spawns next to the
boss arenas after you kill the boss the first time so you can respawn the boss"): in the Reaches the keepers sit the night
with their dead by candlelight, and at a vigil stone the candles are lit and the iron bell rung to call the laid-to-rest
back. It rises beside each boss arena after the first defeat. A scripted model (Art/README.md) in the ruins family's look
(looter_ruins: the brief's shared granite, black iron and brass), its stone the graves kit's.

  VigilStone        a weathered granite stele with a round arched top (0.64 m wide, 1.34 m high over the ground) on two
                    rough-hewn steps (1.2 x 0.8 m, then 0.96 x 0.6 m), a dark rune-stone panel let into its face with the
                    keepers' mark in brass on it (a ring round a candle flame), seven grave candles of wax on the steps'
                    fronts, burnt down to different heights in their own spilled wax, their wicks black; and on the upper
                    step's back right corner a forged black iron crook (a square bar post, its arm reaching forward over the
                    candles with a brace under it and a hook at its end) where the bell hangs.
  VigilStoneBell    the small iron hand bell (15 cm across the lip, 20 cm high), its hanger loop and its clapper under the lip:
                    a model of its own so the game can swing it when it's rung. Its pivot is the top of its loop, the bell
                    hanging down -Z; it sits on the stone's SOCKET_Bell.
  VigilStoneFlames  the seven candle flames as one glowing surface (CandleFlame: an emissive surface; Nanite can't draw the
                    additive glow), built in the stone's own space so it shares the stone's pivot: the game shows it while
                    the candles burn (the boss can be called back) and hides it when they go out.

The pivot is on the ground under the middle of the lower step; the front (where the player kneels) faces -Y. 1.2 x 0.8 m,
1.62 m high to the crook's arm.

Sockets (a socket's front is its empty's -Y, its top the empty's +Z; Art/README.md):
  Interact   the front, centered, 0.95 m up, facing out (the actor's forward in Unreal): where the player looks to kneel.
  Bell       under the crook's hook at (0.45, -0.14, 1.535): the bell's pivot (its loop's top) goes here.
  Light      over the candles on the upper step (0, -0.20, 0.42): the candles' small warm light.
  Mark       the keepers' mark on the panel (0, 0.04, 0.88), facing out.
The C++ stands its plain shapes in for these with the same numbers (BossShrine.cpp, StandInSocket): keep them in step.

Collision: four UCX_ boxes on the stone (the two steps, the stele, the iron post); the candles and the arm have none. The
bell and the flames have none. The stone keeps Nanite with every triangle in its fallback; the bell and the flames are small
and close (Nanite off, LODs).

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Props/VigilStone.py -- --preview        previews into Saved/ArtPreviews/Buildings/
    Tools\\models.ps1 -Only VigilStone -NoImport -Out Intermediate\\ArtExport_VigilStone      export test
"""
import math
import random

import bmesh
import bpy
from mathutils import Vector

import looter_buildings as kit
import looter_model as lm
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

# The brief's shared names (MI_<name> in Unreal): the graves' granite, the Reliquary's dark rune stone, black iron, brass.
RUNE = lt.material('RockGranite', name='ChestRuneStone', tint=0x4a423b)
FLAME = lm.material('CandleFlame', 0xffb35a, Glow=6.0, Variation=0.0)

# The steps: (size x, y, z), centre.
LOWER = ((1.2, 0.8, 0.16), (0.0, 0.0, 0.08))
UPPER = ((0.96, 0.6, 0.12), (0.0, 0.04, 0.22))
STEP_TOP = 0.28
# The stele: width, its front and back faces (y), its shoulder and its arch's radius.
STELE_W, STELE_FRONT, STELE_BACK = 0.64, 0.05, 0.23
STELE_SHOULDER = 1.02
ARCH_R = STELE_W * 0.5
# The panel and the mark on it.
PANEL = (0.40, 0.46)
PANEL_Z = 0.85
MARK_Z = 0.88
# The crook: its post on the upper step's back right corner, its arm forward to the hook.
POST_X, POST_Y, POST_TOP = 0.45, 0.24, 1.6
ARM_Z = 1.58
HOOK = (0.45, -0.14, 1.555)
BELL_AT = (0.45, -0.14, 1.535)
# The candles: (x, y, the step's top they stand on, height, radius), burnt down to different heights.
CANDLES = [(-0.36, -0.17, STEP_TOP, 0.14, 0.030), (-0.20, -0.20, STEP_TOP, 0.20, 0.034), (-0.05, -0.18, STEP_TOP, 0.10, 0.028),
           (0.12, -0.21, STEP_TOP, 0.17, 0.032), (0.28, -0.17, STEP_TOP, 0.08, 0.026), (-0.50, -0.31, 0.16, 0.12, 0.030),
           (0.42, -0.30, 0.16, 0.06, 0.027)]
SEED = 2026


def prism_xz(outline, y0, y1, bevel=0.0):
    """A slab whose face is the convex outline [(x, z), ...] (seen from the front), from y0 to y1, its rim chamfered."""
    bm = lp.new_bmesh()
    front = [bm.verts.new((x, y0, z)) for x, z in outline]
    back = [bm.verts.new((x, y1, z)) for x, z in outline]
    bm.faces.new(front)
    bm.faces.new(list(reversed(back)))
    count = len(outline)
    for i in range(count):
        j = (i + 1) % count
        bm.faces.new((front[i], front[j], back[j], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bevel > 0.0:
        rim = [e for e in bm.edges if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > math.radians(35.0)]
        bmesh.ops.bevel(bm, geom=rim, offset=bevel, segments=1, affect='EDGES', clamp_overlap=True)
    return lp.mesh_object(bm)


def granite(part, seed, rough=0.0):
    """A stone part on the shared granite, at world scale, a little uneven when rough."""
    lt.assign(part, lr.material('granite'))
    if rough > 0.0:
        lp.rough(part, rough, 3.0, seed)
    lt.box_uv(part, 'RockGranite', seed=seed)
    return part


def arch_outline(width, base, shoulder, radius, steps=12):
    """A headstone's outline: straight sides from base to shoulder, a round arch over them (counter-clockwise from the
    bottom left, seen from the front)."""
    half = width * 0.5
    points = [(-half, base), (half, base), (half, shoulder)]
    for k in range(1, steps):
        a = math.pi * k / steps
        points.append((radius * math.cos(a), shoulder + radius * math.sin(a)))
    points.append((-half, shoulder))
    return points


# ---------------------------------------------------------------------------
# The stone
# ---------------------------------------------------------------------------

def steps(m):
    """Two rough-hewn granite steps, the upper set back toward the stele."""
    for k, (size, center) in enumerate((LOWER, UPPER)):
        part = lp.block(size, center, bevel=0.02 if k == 0 else 0.015)
        # Rows of vertices along them, so the hewing and the baked shade have somewhere to live.
        lp.slice_at(part, (1.0, 0.0, 0.0), [-size[0] * 0.5 + size[0] * i / 6 for i in range(1, 6)])
        lp.slice_at(part, (0.0, 1.0, 0.0), [center[1] - size[1] * 0.25, center[1] + size[1] * 0.25])
        m.add(granite(part, SEED + k, rough=0.006))
        m.hull(size, at=center)


def stele(m):
    """The arched stele, standing on the upper step toward its back."""
    outline = arch_outline(STELE_W, STEP_TOP - 0.01, STELE_SHOULDER, ARCH_R)
    part = prism_xz(outline, STELE_FRONT, STELE_BACK, bevel=0.012)
    lp.slice_at(part, (0.0, 0.0, 1.0), [0.45, 0.62, 0.8, 0.95])
    m.add(granite(part, SEED + 7, rough=0.004))
    m.hull((STELE_W, STELE_BACK - STELE_FRONT, STELE_SHOULDER + ARCH_R - STEP_TOP),
           at=(0.0, (STELE_FRONT + STELE_BACK) * 0.5, (STEP_TOP + STELE_SHOULDER + ARCH_R) * 0.5))


def panel_and_mark(m):
    """The dark rune-stone panel let into the stele's face, and the keepers' mark in brass on it: a ring round a flame."""
    w, h = PANEL
    panel = lp.block((w, 0.014, h), (0.0, STELE_FRONT - 0.004, PANEL_Z), bevel=0.004)
    lt.assign(panel, RUNE)
    lt.box_uv(panel, 'RockGranite', seed=SEED + 11)
    m.add(panel)
    face = STELE_FRONT - 0.012
    ring = [(0.12 * math.cos(a), face, MARK_Z + 0.12 * math.sin(a)) for a in (2.0 * math.pi * k / 24 for k in range(25))]
    m.add(lr.tube(ring, 0.008, sides=5, key='brass', set_name='MetalWorn', caps=(False, False), seed=SEED + 12), smooth=True)
    # The flame: a teardrop standing on a little candle stub, in relief.
    flame = [(0.0, MARK_Z - 0.055), (0.03, MARK_Z - 0.035), (0.037, MARK_Z - 0.005), (0.026, MARK_Z + 0.035),
             (0.008, MARK_Z + 0.07), (0.0, MARK_Z + 0.085), (-0.008, MARK_Z + 0.07), (-0.026, MARK_Z + 0.035),
             (-0.037, MARK_Z - 0.005), (-0.03, MARK_Z - 0.035)]
    part = prism_xz(list(reversed(flame)), face - 0.006, face + 0.004, bevel=0.002)
    lt.assign(part, lr.material('brass'))
    lt.box_uv(part, 'MetalWorn', seed=SEED + 13)
    m.add(part)
    stub = lp.block((0.03, 0.01, 0.035), (0.0, face - 0.001, MARK_Z - 0.075), bevel=0.003)
    lt.assign(stub, lr.material('brass'))
    lt.box_uv(stub, 'MetalWorn', seed=SEED + 14)
    m.add(stub)


def candles(m):
    """The grave candles, each in its own spilled wax, its top melted into a cup, its wick black."""
    rnd = random.Random(SEED + 20)
    for k, (x, y, base, height, radius) in enumerate(CANDLES):
        wax, _ = lr.lathe([(0.0, 0.0), (radius, 0.0), (radius * 1.02, height * 0.45), (radius * 0.99, height - 0.012),
                           (radius * 0.82, height), (radius * 0.5, height - 0.006), (0.0, height - 0.008)], 10, 'cream',
                          set_name='PaintWorn', seed=SEED + 21 + k)
        # A drip or two run down the side, and the pool it stands in.
        drip, _ = lr.lathe([(0.0, 0.0), (radius * 1.7, 0.0), (radius * 1.45, 0.007), (radius * 1.08, 0.014), (0.0, 0.014)], 10,
                           'cream', set_name='PaintWorn', seed=SEED + 31 + k)
        lp.place(wax, (x, y, base + 0.006), (0.0, 0.0, rnd.uniform(0.0, 36.0)))
        lp.place(drip, (x, y, base), (0.0, 0.0, rnd.uniform(0.0, 36.0)))
        m.add(wax, smooth=True)
        m.add(drip, smooth=True)
        side = rnd.uniform(0.0, 2.0 * math.pi)
        run = [(x + math.cos(side) * radius * 1.01, y + math.sin(side) * radius * 1.01, base + 0.006 + height - 0.01),
               (x + math.cos(side) * radius * 1.05, y + math.sin(side) * radius * 1.05, base + 0.006 + height * 0.55)]
        m.add(lr.tube(run, 0.005, sides=5, key='cream', set_name='PaintWorn', radii=[0.004, 0.006], seed=SEED + 41 + k),
              smooth=True)
        m.cylinder((x, y, base + 0.006 + height - 0.01), (x, y, base + 0.006 + height + 0.012), 0.0025, sides=4,
                   uv=kit.Tile('BarkOak'), mat='charcoal')


def crook(m):
    """The forged black iron crook: a square bar post with a foot plate, its arm forward over the candles with a brace
    under it, a scroll at its back and a hook at its end."""
    iron = dict(key='ironblack', set_name='MetalWorn')
    m.box((0.09, 0.09, 0.018), at=(POST_X, POST_Y, STEP_TOP + 0.009), uv=kit.Tile('MetalWorn'), mat='ironblack', bevel=0.004)
    m.add(lr.tube([(POST_X, POST_Y, STEP_TOP), (POST_X, POST_Y, POST_TOP)], 0.017, sides=4, seed=SEED + 50, **iron))
    m.add(lr.tube([(POST_X, POST_Y + 0.02, ARM_Z), (POST_X, HOOK[1] + 0.02, ARM_Z)], 0.014, sides=4, seed=SEED + 51, **iron))
    m.add(lr.tube([(POST_X, POST_Y, ARM_Z - 0.32), (POST_X, POST_Y - 0.2, ARM_Z - 0.01)], 0.009, sides=4, seed=SEED + 52, **iron))
    # A scroll curled off the post's top behind the arm (the smith's flourish).
    scroll = [(POST_X, POST_Y + 0.02 + 0.045 * math.sin(a), ARM_Z + 0.045 - 0.045 * math.cos(a) * (1.0 - a / 9.0))
              for a in (k * 0.35 for k in range(16))]
    m.add(lr.tube(scroll, 0.008, sides=4, seed=SEED + 53, caps=(True, True), **iron), smooth=True)
    # The hook the bell's loop hangs on.
    hook = [(HOOK[0], HOOK[1] + 0.012 * math.sin(a), HOOK[2] + 0.012 * math.cos(a)) for a in (k * math.pi / 6 for k in range(10))]
    hook = [(HOOK[0], HOOK[1] + 0.02, ARM_Z)] + hook
    m.add(lr.tube(hook, 0.005, sides=4, seed=SEED + 54, caps=(True, True), **iron), smooth=True)
    m.hull((0.06, 0.06, POST_TOP - STEP_TOP), at=(POST_X, POST_Y, (POST_TOP + STEP_TOP) * 0.5))


def vigil_stone():
    m = lr.Model('VigilStone', seed=SEED)
    steps(m)
    stele(m)
    panel_and_mark(m)
    candles(m)
    crook(m)
    m.socket('Interact', (0.0, -0.42, 0.95))
    m.socket('Bell', BELL_AT)
    m.socket('Light', (0.0, -0.20, 0.42))
    m.socket('Mark', (0.0, STELE_FRONT - 0.012, MARK_Z))
    return m.finish(view=(-1.2, -1.6, 0.6), fit=0.9, fallback=100.0, ao_distance=0.5, out=lt.preview_path('Buildings', 'VigilStone'))


# ---------------------------------------------------------------------------
# The bell, and the flames
# ---------------------------------------------------------------------------

def vigil_stone_bell():
    """The iron hand bell hanging from its loop's top (the pivot), its clapper under the lip."""
    m = lr.Model('VigilStoneBell', seed=SEED + 60)
    body, _ = lr.lathe([(0.0, -0.026), (0.028, -0.028), (0.043, -0.048), (0.047, -0.09), (0.054, -0.135), (0.068, -0.172),
                        (0.075, -0.19), (0.072, -0.198), (0.06, -0.196), (0.0, -0.185)], 14, 'ironblack', set_name='MetalWorn',
                       seed=SEED + 61)
    m.add(body, smooth=True)
    loop = [(0.012 * math.sin(a), 0.0, -0.014 + 0.012 * math.cos(a)) for a in (2.0 * math.pi * k / 12 for k in range(13))]
    m.add(lr.tube(loop, 0.0045, sides=5, key='ironblack', set_name='MetalWorn', caps=(False, False), seed=SEED + 62), smooth=True)
    m.add(lr.tube([(0.0, 0.0, -0.18), (0.0, 0.0, -0.2)], 0.003, sides=4, key='ironblack', set_name='MetalWorn', seed=SEED + 63))
    clapper, _ = lr.lathe([(0.0, -0.198), (0.009, -0.202), (0.012, -0.21), (0.009, -0.218), (0.0, -0.221)], 8, 'ironblack',
                          set_name='MetalWorn', seed=SEED + 64)
    m.add(clapper, smooth=True)
    # It hangs below its pivot: no ground under it to shade it.
    return m.finish(ground=False, view=(-0.6, -0.8, -0.05), fit=1.2, fallback=None, ao_distance=0.08,
                    out=lt.preview_path('Buildings', 'VigilStoneBell'), Nanite=0, LODs='50,25', Collision='None')


def vigil_stone_flames():
    """The seven flames as one glowing surface, each a teardrop over its wick, in the stone's own space."""
    m = lr.Model('VigilStoneFlames', seed=SEED + 70)
    rnd = random.Random(SEED + 71)
    for k, (x, y, base, height, radius) in enumerate(CANDLES):
        s = 0.85 + 0.3 * rnd.random()
        part, _ = lp.lathe([(0.0, 0.0), (0.006 * s, 0.006 * s), (0.0105 * s, 0.018 * s), (0.008 * s, 0.032 * s),
                            (0.003 * s, 0.045 * s), (0.0, 0.052 * s)], segments=8)
        lt.assign(part, FLAME)
        lp.lathe_uv(part, 'PaintWorn', seed=SEED + 72 + k)
        lr.split_poles(part, 'around')
        lp.place(part, (x, y, base + 0.006 + height + 0.004), (rnd.uniform(-4.0, 4.0), rnd.uniform(-4.0, 4.0), 0.0))
        m.add(part, smooth=True)
    return m.finish(ao=False, view=(-0.8, -1.2, 0.4), fit=1.2, fallback=None, out=lt.preview_path('Buildings', 'VigilStoneFlames'),
                    Nanite=0, Collision='None')


ONLY = next((a.split('=', 1)[1].split(',') for a in kit._args() if a.startswith('--only=')), None)
BUILDERS = [('VigilStone', vigil_stone), ('VigilStoneBell', vigil_stone_bell), ('VigilStoneFlames', vigil_stone_flames)]
models = [build() for name, build in BUILDERS if ONLY is None or name in ONLY]
