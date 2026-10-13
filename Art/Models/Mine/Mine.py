"""The Providence silver mine (Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence", section 4 "Back Pay"):
the mine's rooms as a kit of rock pieces carved from one solid (Tools/Blender/looter_mine.py), the timber, the
barricade, the fall and the lamps, and Mine.layout.json, which places every piece in the mine's own frame.

THE FRAME ("mouth-local"): metres, x east, y north (into Larkspur High), z up; the origin is the adit's mouth on its
floor, at the portal (design (74, 230), +58 m). The mine's floor heights: the adit, gallery, office and manway's top
z = 0 (+58 m); the lower drift, side stopes and the stope z = -8 (+50 m); the decline drops 8 m between them. Its
plan reaches x -32.4..14.2, y -1..104.6, z -8..+5.2 (the gallery's crown). In Unreal the frame is the portal's: X =
y * 100 (north), Y = x * 100 (east), Z = z * 100 cm; every piece is built in Blender in the frame's own axes (x east,
y north), so placed in Unreal it takes yaw 180 in that frame (Blender's -Y is Unreal's +X). Mine.layout.json lists
both, as DenDressing.placement.json does.

The rooms (the design's order: adit -> gallery (+ office) -> decline -> drift -> stope; the manway stope -> gallery):
  RR_Mine_Adit      30 m from the mouth, 4 m wide, 3.0 m walls arching to 3.5 m; 12 timber sets every 2.5 m.
  RR_Mine_Gallery   the upper gallery, x -8..8, y 30..54 (16 x 24 m), its walls 4.3 m rising to a 5.2 m crown; six
                    timber pillars; the chalk on its west wall; the decline's head (and its winch) in the west wall,
                    the office door in the east wall, the manway's level passage in the north wall. Fight: MineUpper.
  RR_Mine_Office    the Foreman's office off the gallery's east side: a 5 x 6 m pocket (x 9..14, y 39..45) lined with
                    boards, 2.5 m to its board ceiling: desk (SOCKET_PayrollBook on its top: the payroll book is a prop
                    of its own, SM_PayrollBook in Art/Models/Props/MissionProps.py), chair, cot, notices, a shelf of
                    ledgers, the mine's plan on the wall, a lamp; the company safe's place in its north-east corner.
  RR_Mine_Decline   from the gallery's west wall 11 m west and down 4 m (20 deg), a landing (the dog-leg) with a sump
                    where the runaway car lies, 11.5 m north and down 4 m more to the drift; 8 pitched timber sets.
  RR_Mine_Drift     the lower drift, x -24..-18, y 48..84 (6 x 36 m), 3.6 m walls to a 4.1 m crown, with two side stopes
                    (8 x 6 m: west at y 56..62, east at y 68..74); 7 timber sets; the barricade across its north end.
                    Fight: MineLower.
  RR_Mine_Stope     the Foreman's arena, x -30..-4, y 84..104 (26 x 20 m), walls 5.4 m to a 7 m crown; six props; the
                    fall (RR_Mine_Fall) over its north 9 m. Fight: the Foreman.
  RR_Mine_Manway    the way back up: a gate passage from the stope's east wall (the barred gate), a timber stair of 40
                    risers climbing 8 m south (30 deg), and a level passage 19 m south into the gallery's north wall.
The kit:
  RR_Mine_TimberSet, RR_Mine_TimberSetWide   a set for the 4 m tunnels (posts, cap, lagging 2.5 m along) and for the
                    6 m drift (lagging 3 m along); pivot on the floor under the cap's middle, the tunnel along Y.
  RR_Mine_Pillar    the gallery's timber pillar (post, corbels, a 2.6 m cap along Y at 4.25-4.6 m).
  RR_Mine_StopeProp, RR_Mine_StopePropBroken   the stope's props (a 6.3 m round prop and head board) and what the
                    Foreman leaves of one: a splintered stub, the head board with its top hanging, the middle on the floor.
  RR_Mine_Barricade, RR_Mine_BarricadeTags   the eleven's barricade (chalked PAY US AND WE COME UP; a way through at its
                    east end, 1.25 x 2.3 m) and their eleven brass tags on a wire (taken at Back Pay's step 5).
  RR_Mine_ManwayGate  the barred gate's leaf, pivot on its hinge's foot, the leaf along +X when shut.
  RR_Mine_Winch     the decline's hand winch at its head, its cable snapped.
  RR_Mine_Fall      the rubble slope where the roof came down, with the broken timbers in it.
  RR_Mine_Lantern, RR_Mine_LanternDark   a miner's lantern, lit and out (the Foreman's "dark"), pivot at its glass, its
                    bracket reaching back (+Y) 0.25 m to the post or rock it's spiked into.
  RR_Mine_Shell     a blocky closed hull round the whole mine, facing out, open only at the adit: the sun's shadow when
                    the mountain outside is hidden (never seen; no collision).
Every SOCKET_Light_<n> on the rock pieces is a lamp's glass: the code lights it, and stands RR_Mine_Lantern there.

Run: --preview renders the plan, the in-tunnel views and the kit into Saved/ArtPreviews/RansomsRest/Mountain/Mine/;
--only rock|kit builds a part (no layout file then); --no-ao skips the shading bake.
"""
import json
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_mine as mn
import looter_model as lm
import looter_mountain as mtn
import looter_outcrops as lo
import looter_textures as lt
import looter_town as town
from looter_buildings import Space, Tile, Trim, place

ARGS = mtn.args()
ONLY = ARGS[ARGS.index('--only') + 1] if '--only' in ARGS else None
NO_AO = '--no-ao' in ARGS
HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else os.path.join(lt.REPO, 'Art', 'Models', 'Mine')
LAYOUT_PATH = os.path.join(lt.REPO, 'Art', 'Models', 'Mine', 'Mine.layout.json')
PREVIEWS = os.path.join(mtn.PREVIEW_DIR, 'Mine')

# --- The rooms ---

W_TUNNEL, HW_TUNNEL, H_TUNNEL = 4.0, 3.0, 3.5
SLOPE_A = math.degrees(math.atan2(4.0, 11.0))         # the decline's first run: 11 m west, 4 m down
SLOPE_B = math.degrees(math.atan2(4.0, 11.5))         # its second: 11.5 m north, 4 m down
STAIR_FOOT, STAIR_TOP = (1.5, 86.5, -8.0), (1.5, 72.5, 0.0)

ROOMS = [
    mn.Corridor('adit', (0.0, -4.0), (0.0, 31.0), [(0.0, 0.0)], W_TUNNEL, HW_TUNNEL, H_TUNNEL, amp=0.42),
    mn.Room('gallery', -8.0, 8.0, 30.0, 54.0, 0.0, 4.3, 5.2, ramp=4.0, rc=1.6, amp=0.5),
    mn.Corridor('office_door', (7.0, 42.0), (9.6, 42.0), [(0.0, 0.0)], 1.4, 2.25, 2.3, amp=0.0, cap=0.1),
    mn.Room('office', 9.0, 14.0, 39.0, 45.0, 0.0, 2.8, 2.85, ramp=1.0, rc=0.3, amp=0.0),
    mn.Corridor('decline_a', (-7.0, 34.5), (-20.0, 34.5), [(1.0, 0.0), (12.0, -4.0)], W_TUNNEL, HW_TUNNEL, H_TUNNEL,
                amp=0.42),
    mn.Room('landing', -23.2, -18.8, 32.3, 36.7, -4.0, 3.0, 3.6, ramp=1.5, rc=1.2, amp=0.45),
    mn.Room('sump', -23.15, -21.75, 32.8, 36.2, -4.5, 0.55, 0.6, ramp=0.5, rc=0.4, amp=0.05),
    mn.Corridor('decline_b', (-21.0, 35.5), (-21.0, 49.0), [(1.0, -4.0), (12.5, -8.0)], W_TUNNEL, HW_TUNNEL, H_TUNNEL,
                amp=0.42),
    mn.Room('drift', -24.0, -18.0, 48.0, 85.0, -8.0, 3.6, 4.1, ramp=2.0, rc=0.6, amp=0.5),
    mn.Room('stope_w', -32.0, -23.0, 56.0, 62.0, -8.0, 2.8, 3.4, ramp=2.0, rc=1.5, amp=0.7),
    mn.Room('stope_e', -19.0, -10.0, 68.0, 74.0, -8.0, 2.8, 3.4, ramp=2.0, rc=1.5, amp=0.7),
    mn.Room('stope', -30.0, -4.0, 84.0, 104.0, -8.0, 5.4, 7.0, ramp=5.0, rc=3.0, amp=0.62),
    mn.Corridor('manway_gate', (-5.0, 87.0), (2.7, 87.0), [(0.0, -8.0)], 2.4, 2.3, 2.6, amp=0.32, cap=0.15),
    mn.Corridor('manway_stair', (1.5, 89.0), (1.5, 71.5), [(2.5, -8.0), (16.5, 0.0)], 2.4, 2.9, 3.1, amp=0.32,
                cap=0.15),
    mn.Corridor('manway_top', (1.5, 73.0), (1.5, 53.0), [(0.0, 0.0)], 2.4, 2.4, 2.7, amp=0.32),
]
FLOORS = {'Adit': 0.0, 'Gallery': 0.0, 'Office': 0.0, 'Drift': -8.0, 'Stope': -8.0}
PIVOTS = {'Adit': (0.0, 0.0, 0.0), 'Gallery': (0.0, 42.0, 0.0), 'Office': (11.5, 42.0, 0.0),
          'Decline': (-21.0, 34.5, -4.0), 'Drift': (-21.0, 66.0, -8.0), 'Stope': (-17.0, 94.0, -8.0),
          'Manway': STAIR_FOOT}
PIECES = ['Adit', 'Gallery', 'Office', 'Decline', 'Drift', 'Stope', 'Manway']
TARGETS = {'Adit': 7000, 'Gallery': 13000, 'Office': 1200, 'Decline': 7500, 'Drift': 15000, 'Stope': 17000,
           'Manway': 6500}


def region(c):
    """Which piece a bit of rock belongs to, by where it lies (None: outside the mouth, cut away)."""
    x, y = c[0], c[1]
    if y < -1.0:
        return None
    if y < 29.0:
        return 'Adit'
    if y <= 55.0:
        if x > 8.4:
            return 'Office'
        if x >= -9.0:
            return 'Gallery'
        return 'Decline' if y < 47.0 else 'Drift'
    if x > -3.0:
        return 'Manway'
    return 'Drift' if y < 83.0 else 'Stope'


# --- The timber's places (the rock is cut back from them) ---

SET4 = dict(post=0.25, post_x=1.825, cap_z=2.95, cap_h=0.28, cap_len=4.3, lag=2.5, lag_t=0.05, lag_w=4.1)
SETW = dict(post=0.3, post_x=2.8, cap_z=3.25, cap_h=0.32, cap_len=6.5, lag=3.0, lag_t=0.05, lag_w=6.2)
PILLAR_TOP = 4.65                     # the pillar's cap top (wedges) above the floor
PROP_TOP = 6.3                        # the stope prop's head board top above the floor
PILLARS = [(x, y) for y in (36.0, 42.0, 48.0) for x in (-4.0, 4.0)]
PROPS = [(-25.0, 88.5), (-17.5, 88.5), (-10.0, 88.5), (-21.5, 93.0), (-14.0, 93.0), (-7.5, 93.0)]
BARRICADE = (-21.0, 82.5, -8.0)
GATE_HINGE = (-3.3, 86.25, -8.0)      # the manway gate's hinge foot (the leaf runs +Y from it when shut)
FALL = (-17.0, 95.5, -8.0)            # the fall's foot line, middle
WINCH = (-5.3, 34.5, 0.0)


def sets():
    """Every timber set: (kind, pivot (x, y, z), yaw, pitch)."""
    out = []
    for k in range(12):
        out.append(('Set', (0.0, 1.25 + 2.5 * k, 0.0), 0.0, 0.0))
    ta, tb = math.radians(SLOPE_A), math.radians(SLOPE_B)
    for k in range(4):
        s = 1.25 + 2.5 * k
        out.append(('Set', (-8.0 - s * math.cos(ta), 34.5, -s * math.sin(ta)), 90.0, -SLOPE_A))
    for k in range(4):
        s = 1.25 + 2.5 * k
        out.append(('Set', (-21.0, 36.5 + s * math.cos(tb), -4.0 - s * math.sin(tb)), 0.0, -SLOPE_B))
    for y in (49.5, 52.5, 55.5, 64.5, 67.5, 76.5, 79.5):
        out.append(('SetWide', (-21.0, y, -8.0), 0.0, 0.0))
    return out


def clears():
    """The boxes the rock is cut back from (oriented: center, axes, half sizes), and the caps the roof comes down on."""
    boxes, caps = [], []
    for kind, (x, y, z), yaw, pitch in sets():
        p = SET4 if kind == 'Set' else SETW
        axes = mn.frame(yaw, pitch)
        c = np.array([x, y, z])
        top = p['cap_z'] + p['cap_h'] + p['lag_t']
        for side in (-1.0, 1.0):     # the posts (from just over the floor: the floor itself is never dug)
            boxes.append((c + axes @ np.array([side * p['post_x'], 0.0, (top + 0.05) * 0.5]), axes,
                          (p['post'] * 0.5 + 0.03, p['post'] * 0.5 + 0.03, (top - 0.05) * 0.5)))
        boxes.append((c + axes @ np.array([0.0, 0.0, p['cap_z'] + (top - p['cap_z']) * 0.5 + 0.03]), axes,
                      (p['cap_len'] * 0.5 + 0.02, p['lag'] * 0.5 + 0.01, (top - p['cap_z']) * 0.5 + 0.06)))
    I = np.eye(3)
    for x, y in PILLARS:
        boxes.append((np.array([x, y, (PILLAR_TOP + 0.05) * 0.5]), I, (0.24, 0.24, (PILLAR_TOP - 0.05) * 0.5)))
        boxes.append((np.array([x, y, 4.42]), I, (0.24, 1.33, 0.25)))
        caps.append((x, y, PILLAR_TOP + 0.02, 0.9, 1.9))
    for x, y in PROPS:
        boxes.append((np.array([x, y, -8.0 + (PROP_TOP + 0.05) * 0.5]), I, (0.29, 0.29, (PROP_TOP - 0.05) * 0.5)))
        boxes.append((np.array([x, y, -8.0 + 6.0]), I, (0.85, 0.24, 0.32)))
        caps.append((x, y, -8.0 + PROP_TOP + 0.02, 0.7, 1.8))
    bx, by, bz = BARRICADE
    boxes.append((np.array([bx, by, bz + 2.2]), I, (3.08, 0.36, 2.15)))
    gx, gy, gz = GATE_HINGE
    boxes.append((np.array([gx, gy + 0.75, gz + 1.27]), I, (0.14, 1.0, 1.22)))
    return boxes, caps


CHALK = ((-7.72, 40.5, 1.75), (1.0, 0.0, 0.0), (1.45, 0.75), 0.5)   # the gallery's west wall, smoothed for the chalk


def build_field():
    boxes, caps = clears()
    return mn.Mine(ROOMS, clears=boxes, caps=caps, flats=[CHALK])


def floor_at(field):
    return lambda C: field.cave(np.asarray(C, float))[1]


def rock(field, dressing=None):
    """The rock pieces, shaded across their seams, with each piece's dressing joined in before the shading bake.
    dressing: {piece: [objects in the mine's frame]}. Returns {piece: object}, still in the mine's frame."""
    V, Q = mn.mesh(field)
    dealt = mn.deal(V, Q, region)
    parts = []
    for name in PIECES:
        obj = mn.piece_object('_Raw_' + name, V, Q, dealt[name])
        raw = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        moved = mn.reduce_locked(obj, TARGETS[name])
        mn.log(f'{name}: {raw} -> {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles, '
               f'{len(mn.border(obj))} border vertices, {moved} moved')
        parts.append(obj)
    whole = mn.weld(parts, '_Mine')
    mn.log(f'welded mine: {mn.open_edges(whole)} open edges')
    for length, centre, count in mn.holes(whole):
        mn.log(f'  an open loop besides the mouth: {count} edges, {length:.3f} m round, at {centre}')
    for obj in parts:
        mesh = obj.data
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)
    mn.shade(whole, floor_at(field))
    if dressing:
        joined = []
        for k, name in enumerate(PIECES):
            for obj in dressing.get(name, []):
                attr = obj.data.attributes.new('piece', 'INT', 'FACE')
                attr.data.foreach_set('value', [k] * len(obj.data.polygons))
                joined.append(obj)
        if joined:
            with bpy.context.temp_override(active_object=whole, object=whole, selected_objects=[whole] + joined,
                                           selected_editable_objects=[whole] + joined):
                bpy.ops.object.join()
    whole.data.set_sharp_from_angle(angle=math.radians(55.0))
    mn.keep_normals(whole)
    if not NO_AO:
        lt.bake_vertex_ao(whole, samples=24, distance=2.2, ground=False)
    names = ['RR_Mine_' + name for name in PIECES]
    pieces = mn.split(whole, len(PIECES), names)
    bpy.data.objects.remove(whole)
    return dict(zip(PIECES, pieces))


# --- The kit ---

import looter_ruins as lr


def kit_finish(m, **options):
    return m.finish(Nanite=0, LODs='50,25', **options)


def timber_set(name, p, seed):
    """A timber set: two hewn posts on the floor, a cap across them (wedged), lagging boards along the tunnel over the
    cap (p['lag'] long, so sets every p['lag'] metres close up), a few blocks over the lagging. Pivot on the floor under
    the cap's middle; the tunnel runs along Y."""
    m = mtn.Model(name, seed=seed)
    rng = m.rng
    px, s, cz, ch = p['post_x'], p['post'], p['cap_z'], p['cap_h']
    lag, lag_t, lw = p['lag'], p['lag_t'], p['lag_w']
    for side in (-1.0, 1.0):
        x = side * px
        lean = rng.uniform(-0.012, 0.012)
        mtn.beam(m, (x, 0.0, -0.12), (x + lean, 0.0, cz - 0.03), s, s)
        mtn.wedge(m, (x - side * 0.03, 0.0, cz - 0.015), size=(0.2, s + 0.03, 0.05))
        m.hull((s, s, cz + 0.12), at=(x, 0.0, (cz - 0.12) * 0.5))
    mtn.beam(m, (-p['cap_len'] * 0.5, 0.0, cz + ch * 0.5), (p['cap_len'] * 0.5, 0.0, cz + ch * 0.5), ch, s + 0.02)
    top = cz + ch
    count = int(round(lw / 0.205))
    pitch = lw / count
    for k in range(count):
        if 0 < k < count - 1 and rng.random() < 0.07:
            continue
        x = -lw * 0.5 + pitch * (k + 0.5) + rng.uniform(-0.012, 0.012)
        y0, y1 = -lag * 0.5 - 0.03 + rng.uniform(-0.02, 0.03), lag * 0.5 + 0.03 + rng.uniform(-0.03, 0.02)
        mtn.plank(m, (x, y0, top + lag_t * 0.5), (x, y1, top + lag_t * 0.5), pitch - 0.016, lag_t, face=(0.0, 0.0, 1.0),
                  look='A')
    for k in range(3):
        mtn.wedge(m, (rng.uniform(-0.3, 0.3) * lw, rng.uniform(-0.3, 0.3) * lag, top + lag_t + 0.045),
                  size=(rng.uniform(0.25, 0.4), 0.16, 0.08), turn=rng.uniform(-25.0, 25.0))
    m.hull((p['cap_len'], s + 0.02, ch + lag_t), at=(0.0, 0.0, cz + (ch + lag_t) * 0.5))
    return kit_finish(m)


def pillar():
    """The gallery's pillar: a 38 cm hewn post on a sill block, knee braces to a corbel and a 2.6 m cap along Y,
    wedged to the roof at PILLAR_TOP."""
    m = mtn.Model('RR_Mine_Pillar', seed=5821)
    rng = m.rng
    s = 0.38
    mtn.beam(m, (0.0, 0.0, -0.12), (0.0, 0.0, 4.0), s, s)
    town.box(m, (0.66, 0.66, 0.2), at=(0.0, 0.0, 0.04), look='C')
    mtn.beam(m, (0.0, -0.62, 4.125), (0.0, 0.62, 4.125), 0.25, s, face=(1.0, 0.0, 0.0))
    mtn.beam(m, (0.0, -1.3, 4.425), (0.0, 1.3, 4.425), 0.35, s + 0.02, face=(1.0, 0.0, 0.0))
    for sy in (-1.0, 1.0):
        mtn.beam(m, (0.0, sy * 0.2, 3.25), (0.0, sy * 0.95, 4.0), 0.16, 0.16, face=(1.0, 0.0, 0.0))
    for y in (-0.95, -0.1, 0.8):
        mtn.wedge(m, (rng.uniform(-0.05, 0.05), y, PILLAR_TOP - 0.025), size=(0.3, 0.24, 0.05), turn=rng.uniform(-8, 8))
    m.hull((s, s, 4.12), at=(0.0, 0.0, 1.94))
    m.hull((s + 0.02, 2.6, 0.6), at=(0.0, 0.0, 4.35))
    return kit_finish(m)


def jagged(m, base, axis, r, length, seed, sides=10, look='planks'):
    """A splintered end on a round timber: from the ring of radius r at base, splinters of raw wood reaching up to
    `length` along axis (a crown of points and a broken middle)."""
    rnd = random.Random(seed)
    a = Vector(axis).normalized()
    e1 = a.cross(Vector((0.0, 0.0, 1.0))) if abs(a.z) < 0.9 else a.cross(Vector((1.0, 0.0, 0.0)))
    e1.normalize()
    e2 = a.cross(e1)
    tb = kit._new_bmesh()
    ring = [tb.verts.new(Vector(base) + (e1 * math.cos(2 * math.pi * k / sides) + e2 * math.sin(2 * math.pi * k / sides)) * r)
            for k in range(sides)]
    tips = []
    for k in range(sides):
        t = 2 * math.pi * (k + 0.5) / sides
        reach = length * rnd.uniform(0.15, 1.0)
        inward = rnd.uniform(0.25, 0.75)
        tips.append(tb.verts.new(Vector(base) + a * reach + (e1 * math.cos(t) + e2 * math.sin(t)) * r * inward))
    center = tb.verts.new(Vector(base) + a * length * rnd.uniform(0.05, 0.3))
    for k in range(sides):
        k1 = (k + 1) % sides
        for face in ((ring[k], ring[k1], tips[k]), (ring[k1], center, tips[k]), (ring[k], tips[k], center)):
            try:
                f = tb.faces.new(face)
            except ValueError:
                continue
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    spec, key = town.finish(look)
    m.emit(tb, spec, key)


def stope_prop(broken=False):
    """The stope's prop: a round timber on a mudsill, a head board across its top and wedges to the roof at PROP_TOP.
    broken: what the Foreman's strike leaves: a splintered stub, the head board with a splintered stump hanging from
    it, and the middle lying on the floor."""
    name = 'RR_Mine_StopePropBroken' if broken else 'RR_Mine_StopeProp'
    m = mtn.Model(name, seed=5841 if broken else 5831)
    rng = m.rng
    town.box(m, (0.72, 0.56, 0.18), at=(0.0, 0.0, 0.03), look='C')
    mtn.beam(m, (-0.8, 0.0, 6.0), (0.8, 0.0, 6.0), 0.2, 0.42)
    for x in (-0.45, 0.4):
        mtn.wedge(m, (x, 0.0, 6.15), size=(0.32, 0.3, 0.1), turn=rng.uniform(-10, 10))
        mtn.wedge(m, (x + 0.06, 0.02, 6.25), size=(0.26, 0.26, 0.1), turn=rng.uniform(-10, 10))
    m.hull((1.6, 0.42, 0.4), at=(0.0, 0.0, 6.1))
    if not broken:
        mtn.log(m, (0.0, 0.0, -0.15), (0.0, 0.0, 5.9), r=0.24, r1=0.22, sides=10)
        m.hull((0.48, 0.48, 6.05), at=(0.0, 0.0, 2.875))
        return kit_finish(m)
    stub = 1.25 + rng.uniform(-0.1, 0.1)
    mtn.log(m, (0.0, 0.0, -0.15), (0.0, 0.0, stub), r=0.24, r1=0.235, sides=10, caps=(True, False))
    jagged(m, (0.0, 0.0, stub), (0.0, 0.0, 1.0), 0.235, 0.45, 5842)
    hang = 5.2
    mtn.log(m, (0.0, 0.0, hang), (0.0, 0.0, 5.9), r=0.22, sides=10, caps=(False, True))
    jagged(m, (0.0, 0.0, hang), (0.0, 0.0, -1.0), 0.22, 0.4, 5843)
    m.hull((0.44, 0.44, stub + 0.35), at=(0.0, 0.0, (stub + 0.2) * 0.5))
    # The middle on the floor, splintered at both ends, a few shards by it.
    a, b = Vector((0.45, -0.35, 0.2)), Vector((3.7, 1.05, 0.19))
    d = (b - a).normalized()
    mtn.log(m, tuple(a + d * 0.3), tuple(b - d * 0.3), r=0.2, sides=10, caps=(False, False))
    jagged(m, tuple(a + d * 0.3), tuple(-d), 0.2, 0.35, 5844)
    jagged(m, tuple(b - d * 0.3), tuple(d), 0.2, 0.4, 5845)
    side = Vector((-d.y, d.x, 0.0)) * 0.21
    m.hull_points([p + s * side + Vector((0.0, 0.0, z)) for p in (a, b) for s in (-1.0, 1.0) for z in (0.0, 0.41)])
    for k in range(4):
        c = Vector((rng.uniform(0.2, 1.4), rng.uniform(-0.9, 0.6), 0.02))
        town.box(m, (rng.uniform(0.25, 0.5), 0.05, 0.03), at=tuple(c), rot=(0.0, 0.0, rng.uniform(0, 180)), look='planks')
    return kit_finish(m)


BARRICADE_HALF, BARRICADE_TOP, GAP_X, GAP_H = 3.15, 4.15, 1.85, 2.3
TAGS_AT = (-0.4, -0.085, 1.42)         # the nail the eleven's tags hang on (the barricade's frame)


def barricade():
    """The eleven's barricade across the lower drift: props stood upright on its face and courses of timber stacked
    behind them (logs, hewn caps, lagging) from the floor to the roof, a way through at its east end (GAP_X..wall,
    2.3 m high, the lintel over it), chalked across its middle courses PAY US AND WE COME UP. Pivot on the floor at
    its middle, its face toward -Y (the drift), the stope behind it (+Y)."""
    m = mtn.Model('RR_Mine_Barricade', seed=5851)
    rng = m.rng
    H, TOP = BARRICADE_HALF, BARRICADE_TOP
    for x in (-2.55, -0.95, 0.6, GAP_X - 0.13):
        mtn.log(m, (x, -0.21, -0.1), (x + rng.uniform(-0.05, 0.05), -0.2 + rng.uniform(-0.02, 0.02), TOP + 0.1), r=0.12,
                sides=8)
    z = 0.0
    while z < TOP - 0.08:
        h = min(rng.uniform(0.2, 0.3), TOP - z)
        x1 = GAP_X + 0.3 if z + h <= GAP_H + 0.01 else H
        x0 = -H
        mid = z + h * 0.5
        kind = 'beam' if 1.45 < mid < 2.55 else rng.choice(('log', 'log', 'beam', 'plank'))
        if z < GAP_H < z + h:          # the course across the gap's top is the lintel: a whole log
            kind = 'log'
            x1 = H
        tilt = rng.uniform(-0.03, 0.03)
        if kind == 'log':
            mtn.log(m, (x0, 0.05, mid - tilt), (x1, 0.05, mid + tilt), r=h * 0.5, sides=8)
        elif kind == 'beam':
            mtn.beam(m, (x0, 0.05, mid - tilt * 0.3), (x1, 0.05, mid + tilt * 0.3), h - 0.008, 0.24)
        else:
            for dz in (-h * 0.25, h * 0.25):
                mtn.plank(m, (x0, -0.03, mid + dz - tilt), (x1, -0.03, mid + dz + tilt), h * 0.5 - 0.01, 0.05,
                          face=(0.0, -1.0, 0.0), look='A')
            mtn.beam(m, (x0, 0.12, mid), (x1, 0.12, mid), h - 0.01, 0.18)
        z += h
    # Chalk on the hewn courses' faces (their faces at y = 0.05 - 0.12).
    mtn.chalk(m, 'PAY US AND', (-2.9, -0.07, 2.12), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), 0.32, seed=5852)
    mtn.chalk(m, 'WE COME UP', (-2.7, -0.07, 1.62), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), 0.32, seed=5853)
    mtn.tally(m, (-2.85, -0.07, 1.48), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), 11, height=0.14, seed=5854)
    # The nail the tags hang on.
    mtn.rod(m, (TAGS_AT[0], TAGS_AT[1] + 0.06, TAGS_AT[2]), (TAGS_AT[0], TAGS_AT[1] - 0.02, TAGS_AT[2] + 0.01), r=0.006,
            sides=5)
    m.hull((GAP_X + 0.3 + H, 0.62, TOP), at=((GAP_X + 0.3 - H) * 0.5, -0.04, TOP * 0.5))
    m.hull((H - GAP_X, 0.62, TOP - GAP_H), at=((H + GAP_X) * 0.5, -0.04, (TOP + GAP_H) * 0.5))
    m.socket('Barricade', (TAGS_AT[0], -0.3, TAGS_AT[2] - 0.05))
    m.socket('Way', ((GAP_X + H) * 0.5, -0.4, 0.0))
    return kit_finish(m)


def barricade_tags():
    """The eleven's brass tags on a loop of wire, hung on the barricade's nail; placed with the barricade (the same
    pivot), and taken away at Back Pay's step 5."""
    m = mtn.Model('RR_Mine_BarricadeTags', seed=5861)
    rng = m.rng
    x, y, z = TAGS_AT
    mtn.ring(m, [(0.045, -0.003), (0.05, -0.003), (0.05, 0.003), (0.045, 0.003), (0.045, -0.003)],
             (x, y - 0.012, z - 0.05), (0.0, 1.0, 0.0), sides=12, look='brass')
    for k in range(11):
        t = math.radians(-70.0 + 140.0 * k / 10.0)
        cx, cz = x + 0.06 * math.sin(t), z - 0.05 - 0.06 * math.cos(t)
        tb = kit._new_bmesh()
        bmesh.ops.create_cone(tb, cap_ends=True, cap_tris=False, segments=10, radius1=0.021, radius2=0.021, depth=0.003)
        m.emit(tb, Tile('MetalWorn'), 'brass',
               Matrix.Translation((cx, y - 0.02 - 0.003 * (k % 3), cz)) @ Matrix.Rotation(math.radians(90.0 + rng.uniform(-12, 12)), 4, 'X')
               @ Matrix.Rotation(math.radians(rng.uniform(-20, 20)), 4, 'Y'))
    return m.finish(Nanite=0, Collision='None')


def manway_gate():
    """The manway's barred gate: an iron frame and bars, two hinge eyes on its hinge side, a hasp and an open padlock
    hanging. Pivot on the hinge's foot; the leaf runs along +X (1.5 m), 2.1 m tall; turn it about Z to open."""
    m = mtn.Model('RR_Mine_ManwayGate', seed=5871)
    W, H = 1.5, 2.1
    for x in (0.04, W - 0.04):
        mtn.iron_box(m, (0.05, 0.03, H - 0.02), at=(x, 0.0, H * 0.5))
    for z in (0.12, 1.05, H - 0.08):
        mtn.iron_box(m, (W - 0.04, 0.025, 0.06), at=(W * 0.5, 0.0, z))
    for k in range(1, 9):
        x = 0.04 + (W - 0.08) * k / 9.0
        mtn.rod(m, (x, 0.0, 0.08), (x, 0.0, H - 0.05), r=0.013, sides=6)
    for z in (0.32, 1.78):
        mtn.rod(m, (0.0, 0.0, z - 0.07), (0.0, 0.0, z + 0.07), r=0.03, sides=8)
        mtn.iron_box(m, (0.06, 0.025, 0.05), at=(0.03, 0.0, z))
    mtn.iron_box(m, (0.14, 0.03, 0.05), at=(W - 0.02, -0.03, 1.05))
    mtn.ring(m, [(0.025, -0.006), (0.03, -0.006), (0.03, 0.006), (0.025, 0.006), (0.025, -0.006)], (W + 0.02, -0.05, 0.98),
             (1.0, 0.0, 0.0), sides=8)
    mtn.iron_box(m, (0.06, 0.03, 0.07), at=(W + 0.02, -0.05, 0.92))
    m.hull((W, 0.07, H), at=(W * 0.5, 0.0, H * 0.5))
    return kit_finish(m)


def winch():
    """The decline's hand winch: two timber A-frames on sills, a timber drum between them on an iron axle with its
    flanges, a gear wheel and a pinion on a crank shaft with handles both sides, a brake lever, and the haul cable
    wound on the drum and running out the front (-Y, down the decline), snapped. Pivot on the floor at its middle."""
    m = mtn.Model('RR_Mine_Winch', seed=5881)
    for x in (-0.68, 0.68):
        mtn.beam(m, (x, -0.85, 0.1), (x, 0.85, 0.1), 0.2, 0.22, face=(x, 0.0, 0.0))
        for y in (-0.6, 0.6):
            mtn.beam(m, (x, y, 0.15), (x, 0.0, 0.88), 0.16, 0.16, face=(x, 0.0, 0.0))
        town.box(m, (0.22, 0.32, 0.16), at=(x, 0.0, 0.93), look='C')
        mtn.iron_box(m, (0.24, 0.16, 0.06), at=(x, 0.0, 1.03))
    for y in (-0.8, 0.8):
        mtn.beam(m, (-0.78, y, 0.24), (0.78, y, 0.24), 0.14, 0.14, face=(0.0, y, 0.0))
    mtn.rod(m, (-0.85, 0.0, 0.9), (0.95, 0.0, 0.9), r=0.035, sides=8)
    town.cylinder(m, (-0.52, 0.0, 0.9), (0.52, 0.0, 0.9), 0.26, sides=12, look='C', face=(0.0, 0.0, 1.0))
    for x in (-0.52, 0.52):
        mtn.sheave(m, (x, 0.0, 0.9), (1.0, 0.0, 0.0), 0.34, width=0.03, spokes=0, groove=False, hub=0.05)
    mtn.sheave(m, (0.86, 0.0, 0.9), (1.0, 0.0, 0.0), 0.42, width=0.06, spokes=6, groove=False)
    mtn.rod(m, (-0.95, 0.42, 1.2), (1.0, 0.42, 1.2), r=0.03, sides=8)
    mtn.sheave(m, (0.86, 0.42, 1.2), (1.0, 0.0, 0.0), 0.11, width=0.06, spokes=0, groove=False, hub=0.04)
    for x, sign in ((-0.97, -1.0), (1.02, 1.0)):
        mtn.iron_box(m, (0.04, 0.06, 0.36), at=(x, 0.42, 1.04), rot=(18.0, 0.0, 0.0))
        mtn.rod(m, (x, 0.47, 0.88), (x + sign * 0.24, 0.47, 0.88), r=0.022, sides=6)
    mtn.beam(m, (-0.8, 0.3, 0.2), (-0.8, 0.5, 1.55), 0.08, 0.08, face=(-1.0, 0.0, 0.0))
    mtn.iron_box(m, (0.03, 0.4, 0.03), at=(-0.7, 0.22, 0.62), rot=(-60.0, 0.0, 0.0))
    for k in range(5):
        x = -0.4 + 0.18 * k
        mtn.ring(m, [(0.262, -0.016), (0.285, -0.016), (0.285, 0.016), (0.262, 0.016), (0.262, -0.016)], (x, 0.0, 0.9),
                 (1.0, 0.0, 0.0), sides=12)
    pts = [(0.3, -0.27, 0.92), (0.32, -0.9, 0.35), (0.35, -1.5, 0.04), (0.5, -2.6, 0.03), (0.42, -3.3, 0.04)]
    for p0, p1 in zip(pts, pts[1:]):
        mtn.rod(m, p0, p1, r=0.016, sides=6, caps=(False, False))
    m.hull((1.7, 1.75, 1.25), at=(0.0, 0.0, 0.62))
    return kit_finish(m)


def mine_lantern(lit):
    """A miner's lantern: a square glass between iron plates, a chimney cap, corner wires, a bail, and a flat iron
    bracket from the bail back along +Y 0.28 m to a spike (into a post or the rock). Pivot at the glass's middle."""
    name = 'RR_Mine_Lantern' if lit else 'RR_Mine_LanternDark'
    m = mtn.Model(name, seed=5891)
    m.box((0.12, 0.12, 0.17), uv=Trim('H4', fit=True), mat='glow' if lit else 'trim')
    mtn.iron_box(m, (0.17, 0.17, 0.035), at=(0.0, 0.0, -0.1), bevel=0.006)
    mtn.iron_box(m, (0.16, 0.16, 0.03), at=(0.0, 0.0, 0.1), bevel=0.006)
    town.cylinder(m, (0.0, 0.0, 0.115), (0.0, 0.0, 0.2), 0.055, 0.03, sides=8, look='iron', face=(0.0, -1.0, 0.0))
    for dx in (-0.065, 0.065):
        for dy in (-0.065, 0.065):
            mtn.rod(m, (dx, dy, -0.085), (dx, dy, 0.09), r=0.007, sides=4)
    for p0, p1 in (((-0.075, 0.0, 0.11), (-0.04, 0.0, 0.25)), ((-0.04, 0.0, 0.25), (0.04, 0.0, 0.25)),
                   ((0.04, 0.0, 0.25), (0.075, 0.0, 0.11))):
        mtn.rod(m, p0, p1, r=0.006, sides=4)
    mtn.iron_box(m, (0.022, 0.3, 0.01), at=(0.0, 0.14, 0.275))
    mtn.iron_box(m, (0.02, 0.012, 0.07), at=(0.0, 0.285, 0.25))
    mtn.iron_box(m, (0.016, 0.06, 0.016), at=(0.0, 0.3, 0.215))
    return m.finish(Nanite=0, Collision='None')


def fall():
    """The fall: the roof come down over the stope's north end, a slope of broken rock from the floor to the roof (beds
    of rubble rising in steps, boulders tumbled over them and out onto the floor), with props snapped in it, lagging and
    a cap sticking out. Pivot on the stope's floor at the middle of its foot; it runs 9.6 m north (+Y) into the north
    wall and 13.9 m either side into the walls."""
    rnd = random.Random(9521)
    rockf = lo.Rock(seed=9521, sink=0.3, lumps=(0.2, 2.0), detail=(0.07, 0.8), grain=(0.02, 0.3), rain=0.0,
                    warp=(0.35, 3.0), fold=0.12)
    rows = [(0.5, 0.35), (1.9, 1.2), (3.3, 2.2), (4.7, 3.3), (6.1, 4.5), (7.5, 5.8), (8.9, 7.3)]
    for y, top in rows:
        rockf.add(lo.Block(lo.rect(14.0, 0.95), -0.3, top, center=(rnd.uniform(-0.3, 0.3), y), yaw=rnd.uniform(-1.5, 1.5),
                           r_side=0.5, r_top=0.65, r_bottom=0.1, blend=0.95))

    def surface(y):
        return float(np.interp(y, [r[0] - 0.8 for r in rows], [r[1] for r in rows], left=0.0))
    for k in range(46):
        x, y = rnd.uniform(-12.6, 12.6), rnd.uniform(-1.2, 8.4)
        size = rnd.uniform(0.4, 1.5) * (0.7 if y < 0.5 else 1.0)
        poly = lo.chamfer(lo.rect(size * 0.5, size * rnd.uniform(0.35, 0.5)), rnd, 0.8, (0.05, size * 0.2))
        h = size * rnd.uniform(0.5, 0.75)
        z0 = max(surface(y), 0.0) - h * 0.45
        rockf.add(lo.Block(poly, z0, z0 + h, center=(x, y), yaw=rnd.uniform(0, 360),
                           tilt=(rnd.uniform(-25, 25), rnd.uniform(-25, 25)), r_side=size * 0.12, r_top=size * 0.16,
                           r_bottom=size * 0.08, blend=0.12))
    rockf.calm_box((-14.5, -2.0, -0.3), (14.5, -0.6, 0.15), 0.1)
    obj = lo.mesh_rock('RR_Mine_Fall', rockf, cell=0.1, source=9000)
    lm.smooth(obj, 55.0)
    lt.assign(obj, mtn.EXTRA['rockden']())
    mn.rock_uv(obj, list(range(len(obj.data.polygons))), mn.ROCK_DENSITY, 95)
    # The broken timber in it.
    m = mtn.Model('_FallTimber', seed=9522)
    for (x, y, lean, turn, length) in ((-8.5, 2.6, 38.0, 20.0, 3.4), (3.5, 4.0, 55.0, -30.0, 2.8), (9.6, 1.5, 70.0, 160.0, 2.4)):
        a = Vector((x, y, surface(y) - 0.4))
        d = Matrix.Rotation(math.radians(turn), 3, 'Z') @ Vector((0.0, -math.sin(math.radians(lean)), math.cos(math.radians(lean))))
        mtn.log(m, tuple(a), tuple(a + d * length), r=0.19, sides=10, caps=(True, False))
        jagged(m, tuple(a + d * length), tuple(d), 0.19, 0.42, int(x * 10) + 9530)
    for k in range(7):
        x, y = rnd.uniform(-11.0, 11.0), rnd.uniform(1.0, 7.0)
        a = Vector((x, y, surface(y) - 0.05))
        d = Matrix.Rotation(math.radians(rnd.uniform(0, 360)), 3, 'Z') @ Vector((0.0, -0.9, rnd.uniform(0.2, 0.6)))
        mtn.plank(m, tuple(a), tuple(a + d.normalized() * rnd.uniform(1.2, 2.2)), 0.19, 0.05, face=(0.0, 0.0, 1.0), look='A')
    mtn.beam(m, (-3.0, 5.6, surface(5.6) - 0.1), (-0.5, 4.6, surface(4.6) + 0.35), 0.28, 0.26, face=(0.0, -1.0, 0.4))
    timber = m.finish(ao=False)
    homes_reports_pop('_FallTimber')
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=[obj, timber],
                                   selected_editable_objects=[obj, timber]):
        bpy.ops.object.join()
    if not NO_AO:
        lt.bake_vertex_ao(obj, samples=24, distance=1.6, ground=True)
    obj['Fallback'] = 100.0
    return obj


def homes_reports_pop(name):
    mtn.homes.REPORTS.pop(name, None)


def shell():
    """The shell: the union of every room's box (with room to spare) as one closed blocky hull facing out, open only
    where the adit leaves it (the adit's box starts 2 m in from the mouth). It casts the sun's shadow over the mine
    when the mountain outside is hidden; it is never seen, and has no collision."""
    boxes = []
    for r in ROOMS:
        lo_, hi_ = np.array(r.box[0], float) - 0.4, np.array(r.box[1], float) + 0.4
        if r.name == 'adit':
            lo_[1] = 2.0
        boxes.append((lo_, hi_))

    class Boxes:
        def field(self, P):
            d = None
            for lo_, hi_ in boxes:
                c, h = (lo_ + hi_) * 0.5, (hi_ - lo_) * 0.5
                q = np.abs(P - c) - h
                e = np.linalg.norm(np.maximum(q, 0.0), axis=1) + np.minimum(q.max(axis=1), 0.0)
                d = e if d is None else np.minimum(d, e)
            return d

    solid = Boxes()
    cell = 0.5
    lo_ = np.min([b[0] for b in boxes], axis=0) - 1.0
    hi_ = np.max([b[1] for b in boxes], axis=0) + 1.0
    shape = tuple(int(math.ceil((h - l) / cell)) + 1 for l, h in zip(lo_, hi_))
    xs, ys, zs = (lo_[i] + cell * np.arange(shape[i]) for i in range(3))
    X, Y, Z = np.meshgrid(xs, ys, zs, indexing='ij')
    F = solid.field(np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)).reshape(shape).astype(np.float32)
    F[0], F[-1], F[:, 0], F[:, -1], F[:, :, 0], F[:, :, -1] = 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    V, Qd = lo.surface_nets(F, lo_, cell)
    V = lo.project(V, solid, cell, iterations=3)
    obj = lo.mesh_object('RR_Mine_Shell', V, Qd)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(3.0), verts=bm.verts[:], edges=bm.edges[:])
    mouth = [f for f in bm.faces if f.calc_center_median().y < 2.4 and abs(f.calc_center_median().x) < 4.6
             and -2.0 < f.calc_center_median().z < 5.6]
    bmesh.ops.delete(bm, geom=mouth, context='FACES')
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(obj.data)
    bm.free()
    lt.assign(obj, mtn.EXTRA['rockden']())
    lt.box_uv(obj, 'RockCliff', texel_density=64, seed=3)
    obj['Collision'] = 'None'
    obj['Fallback'] = 100.0
    return obj


# --- The rooms' dressing (built in the mine's frame, joined into their pieces before the shading bake) ---

def office_dressing():
    """The Foreman's office: board floor on sleepers, plank walls on posts, a board ceiling on two beams, the door frame
    with its plank door swung in against the wall; the desk against the east wall (the payroll book's place), a chair, a
    cot of canvas along the south wall, a shelf of ledgers, the company's notices and the mine's plan pinned up."""
    m = mtn.Model('_Office', seed=5901)
    rng = m.rng
    X0, X1, Y0, Y1 = 9.15, 13.85, 39.15, 44.85
    FL, CEIL = 0.06, 2.5
    # Floor: boards along X on sleepers.
    for y in (39.5, 41.0, 42.5, 44.0):
        town.box(m, (X1 - X0, 0.12, 0.05), at=((X0 + X1) * 0.5, y, 0.01), look='C', drop=('-z',))
    count = int((Y1 - Y0) / 0.2)
    for k in range(count):
        y = Y0 + (Y1 - Y0) * (k + 0.5) / count
        mtn.plank(m, (X0 - 0.75 if 41.4 < y < 42.6 else X0, y, FL - 0.015), (X1, y, FL - 0.015), (Y1 - Y0) / count - 0.008,
                  0.03, face=(0.0, 0.0, 1.0), look='planks')
    # Walls: upright planks on the inside of four corner posts and a plate, the west wall round the door.
    door_y0, door_y1, door_h = 41.45, 42.55, 2.05
    for (x, y) in ((X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1), (X0, door_y0 - 0.06), (X0, door_y1 + 0.06)):
        mtn.post(m, x, y, 0.0, CEIL, size=0.14)
    walls = [((X0, Y0), (X1, Y0), (0.0, 1.0, 0.0)), ((X1, Y0), (X1, Y1), (-1.0, 0.0, 0.0)),
             ((X1, Y1), (X0, Y1), (0.0, -1.0, 0.0)), ((X0, Y1), (X0, Y0), (1.0, 0.0, 0.0))]
    for (ax, ay), (bx, by), inward in walls:
        length = math.hypot(bx - ax, by - ay)
        n = int(length / 0.22)
        for k in range(n):
            t = (k + 0.5) / n
            px, py = ax + (bx - ax) * t, ay + (by - ay) * t
            if inward[0] > 0 and door_y0 - 0.1 < py < door_y1 + 0.1:
                top = CEIL - 0.02
                mtn.plank(m, (px, py, door_h + 0.08), (px, py, top), length / n - 0.012, 0.03, face=inward, look='planks',
                          lift=-0.08)
                continue
            mtn.plank(m, (px, py, FL), (px, py, CEIL - 0.02), length / n - 0.012, 0.03, face=inward, look='planks',
                      lift=-0.08)
    mtn.beam(m, (X0 + 0.02, door_y0 - 0.05, door_h + 0.06), (X0 + 0.02, door_y1 + 0.05, door_h + 0.06), 0.12, 0.14,
             face=(-1.0, 0.0, 0.0))
    # The door, swung in against the wall north of the doorway.
    hinge = Vector((X0 + 0.1, door_y1 + 0.02, FL))
    for k in range(5):
        y = hinge.y + 0.11 + 0.22 * k
        mtn.plank(m, (hinge.x + 0.06, y, FL + 0.02), (hinge.x + 0.06, y, door_h - 0.02), 0.21, 0.04, face=(1.0, 0.0, 0.0),
                  look='A')
    for z in (0.35, door_h - 0.4):
        mtn.plank(m, (hinge.x + 0.11, hinge.y + 0.05, z), (hinge.x + 0.11, hinge.y + 1.05, z), 0.14, 0.03,
                  face=(1.0, 0.0, 0.0), look='A')
    # Ceiling: two beams along Y and boards across them.
    for x in (10.7, 12.3):
        mtn.beam(m, (x, Y0, CEIL - 0.11), (x, Y1, CEIL - 0.11), 0.2, 0.16, face=(0.0, 0.0, -1.0))
    n = int((X1 - X0) / 0.2)
    for k in range(n):
        x = X0 + (X1 - X0) * (k + 0.5) / n
        mtn.plank(m, (x, Y0, CEIL + 0.015), (x, Y1, CEIL + 0.015), (X1 - X0) / n - 0.01, 0.03, face=(0.0, 0.0, -1.0),
                  look='planks')
    # The desk against the east wall, the chair before it, the payroll book's place on it, an inkwell and papers.
    dx0, dx1, dy0, dy1, dz = 12.85, 13.78, 40.35, 42.05, 0.78
    town.box(m, (dx1 - dx0, dy1 - dy0, 0.04), at=((dx0 + dx1) * 0.5, (dy0 + dy1) * 0.5, dz - 0.02), look='C')
    for y in (dy0 + 0.22, dy1 - 0.22):
        town.box(m, (dx1 - dx0 - 0.06, 0.4, dz - 0.04 - FL), at=((dx0 + dx1) * 0.5, y, FL + (dz - 0.04 - FL) * 0.5),
                 look='C')
    town.box(m, (0.02, dy1 - dy0 - 0.9, 0.5), at=(dx0 + 0.04, (dy0 + dy1) * 0.5, dz - 0.3), look='C')
    # The payroll book is its own prop now (SM_PayrollBook) on SOCKET_PayrollBook. Its old book is left unbuilt: built into
    # a stand-in that draws on the same random stream and material slots, then thrown away, so the rest comes out the same.
    sink = mtn.Model('_Unbuilt', seed=0)
    sink.rng, sink.slots = m.rng, m.slots
    mtn.book(sink, (13.3, 40.75, dz), size=(0.26, 0.34, 0.04), turn=8.0, cover='black', open_=False)
    sink.bm.free()
    bpy.data.objects.remove(sink._obj)
    bpy.data.meshes.remove(sink._mesh)
    town.cylinder(m, (13.55, 41.55, dz), (13.55, 41.55, dz + 0.06), 0.03, sides=6, look='black', face=(0.0, -1.0, 0.0))
    for k in range(3):
        mtn.paper(m, (13.0 + rng.uniform(-0.05, 0.1), 41.2 + 0.18 * k, dz + 0.003 + 0.002 * k), (1.0, 0.0, 0.0),
                  (0.0, 1.0, 0.0), 0.2, 0.28)
    cx, cy = 12.4, 41.2
    town.box(m, (0.42, 0.42, 0.04), at=(cx, cy, 0.47), look='C')
    town.box(m, (0.03, 0.42, 0.45), at=(cx - 0.2, cy, 0.72), look='C')
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            town.board(m, (cx + sx * 0.18, cy + sy * 0.18, FL), (cx + sx * 0.18, cy + sy * 0.18, 0.45), 0.035, 0.035,
                       look='C', drop=('-x', '+x'))
    # The cot along the south wall: a frame of poles, canvas slung on it, a blanket.
    for y in (39.38, 40.12):
        mtn.log(m, (9.75, y, 0.42), (11.85, y, 0.42), r=0.03, sides=6)
    for x in (9.85, 11.75):
        for y in (39.38, 40.12):
            mtn.log(m, (x, y, FL), (x, y + 0.0, 0.45), r=0.025, sides=6)
    mtn.homes.sheet(m, [[(x, y, 0.41 - 0.06 * math.sin(math.pi * (x - 9.75) / 2.1) * (1.0 - abs(y - 39.75) / 0.4))
                         for y in (39.4, 39.75, 40.1)] for x in (9.75, 10.45, 11.15, 11.85)],
                    Tile('Polymer'), mat='canvas', facing=(0.0, 0.0, 1.0))
    town.box(m, (0.9, 0.7, 0.05), at=(10.4, 39.75, 0.42), rot=(0.0, 0.0, 6.0), look='woodoxide')
    # The shelf on the north wall, ledgers on it; the notices and the plan.
    town.box(m, (1.4, 0.26, 0.04), at=(10.2, Y1 - 0.2, 1.5), look='C')
    for k in range(6):
        w = rng.uniform(0.04, 0.07)
        town.box(m, (w, 0.22, rng.uniform(0.26, 0.32)), at=(9.6 + 0.09 * k, Y1 - 0.2, 1.52 + 0.14),
                 rot=(0.0, rng.uniform(-6, 6) if k == 5 else 0.0, 0.0), look=rng.choice(('black', 'oxide', 'black')))
    for k, (x, z, w, h) in enumerate(((11.3, 1.55, 0.45, 0.6), (11.85, 1.6, 0.42, 0.55), (12.4, 1.45, 0.4, 0.52))):
        mtn.paper(m, (x, Y1 - 0.11, z), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), w, h)
        for j in range(5):
            town.box(m, (w * rng.uniform(0.5, 0.8), 0.002, 0.012), at=(x + w * 0.5, Y1 - 0.112, z + h - 0.12 - 0.08 * j),
                     look='black')
    town.text(m, 'PROVIDENCE MINING CO.', (11.85, Y1 - 0.115, 2.2), height=0.09, width=1.5,
              space=None, look='black', lift=0.0)
    # The mine's plan on the south wall, read from the room (facing south): it runs west.
    mtn.paper(m, (10.9, Y0 + 0.11, 1.2), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0), 0.9, 0.65)
    mtn.chalk(m, 'ADIT\nSTOPE', (10.75, Y0 + 0.112, 1.68), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0), 0.07, look='black',
              seed=5902)
    pts = [(10.8, 1.3), (10.5, 1.32), (10.45, 1.55), (10.15, 1.55), (10.15, 1.42), (10.05, 1.32)]
    for (ax, az), (bx, bz) in zip(pts, pts[1:]):
        town.board(m, (ax, Y0 + 0.113, az), (bx, Y0 + 0.113, bz), 0.012, 0.002, look='black', face=(0.0, 1.0, 0.0))
    return m.finish(ao=False)


def gallery_dressing():
    """The gallery's chalk (DAY 6. TEAGUE WILL DIG US OUT., and the days counted), a bench along the east wall where the
    shift ate, picks against a pillar, a stack of lagging boards on cribbing blocks (cover in the fight)."""
    m = mtn.Model('_Gallery', seed=5911)
    rng = m.rng
    face = CHALK[0][0] + 0.004       # on the smoothed west wall, read from the room (facing east): it runs north
    mtn.chalk(m, 'DAY 6.', (face, 39.45, 2.05), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), 0.2, seed=5912)
    mtn.chalk(m, 'TEAGUE WILL\nDIG US OUT.', (face, 39.35, 1.72), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), 0.17, seed=5913)
    mtn.tally(m, (face, 39.4, 1.06), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), 6, height=0.2, seed=5914)
    # The bench: two planks on three log stumps along the east wall.
    for y in (46.6, 47.8, 49.0):
        mtn.log(m, (6.95, y, -0.05), (6.95, y, 0.4), r=0.17, sides=8)
    for x in (6.82, 7.08):
        mtn.plank(m, (x, 46.3, 0.43), (x, 49.3, 0.43), 0.25, 0.05, face=(0.0, 0.0, 1.0), look='A')
    m.hull((0.55, 3.1, 0.46), at=(6.95, 47.8, 0.23))
    # Picks leaning on the north-east pillar.
    for k, lean in enumerate((12.0, -9.0)):
        base = Vector((4.25 + 0.08 * k, 48.0 + 0.32 + 0.2 * k, 0.0))
        top = base + Vector((math.sin(math.radians(lean)) * 0.2, 0.18, 0.86))
        mtn.log(m, tuple(base), tuple(top), r=0.022, sides=6)
        d = (top - base).normalized()
        side = Vector((1.0, 0.0, 0.0)).cross(d).normalized()
        mtn.iron_box(m, (0.05, 0.05, 0.05), at=tuple(top))
        for s in (-1.0, 1.0):
            tip = top + side * s * 0.28 - d * 0.05 * (1.0 if s > 0 else 0.0)
            mtn.rod(m, tuple(top), tuple(tip), r=0.016, sides=5)
    # Lagging boards stacked on cribbing in the south-east corner.
    for k, (x, y) in enumerate(((5.0, 32.4), (6.6, 32.4))):
        for j in range(3):
            town.box(m, (0.22, 1.3, 0.18), at=(x + (0.35 if j % 2 else 0.0), y, 0.09 + 0.18 * j), rot=(0, 0, 90 * (j % 2)),
                     look='C')
    for j in range(7):
        town.board(m, (4.55, 31.95 + 0.06 * j - 0.18, 0.62 + 0.05 * j), (7.4, 32.0 + 0.04 * j - 0.18, 0.62 + 0.05 * j),
                   0.2, 0.045, look='A', face=(0.0, 0.0, 1.0))
    m.hull((3.0, 1.4, 0.98), at=(5.95, 32.25, 0.49))
    return m.finish(ao=False)


def manway_dressing():
    """The manway: the gate's frame across the gate passage (the leaf is RR_Mine_ManwayGate), the timber stair of 40
    risers from the stope's level to the gallery's, a pole handrail on its west side, stulls across the level passage."""
    m = mtn.Model('_Manway', seed=5921)
    gx, gy, gz = GATE_HINGE
    for y in (gy - 0.1, gy + 1.6):
        mtn.beam(m, (gx, y, gz - 0.1), (gx, y, gz + 2.32), 0.2, 0.2, face=(-1.0, 0.0, 0.0))
    mtn.beam(m, (gx, gy - 0.35, gz + 2.42), (gx, gy + 1.85, gz + 2.42), 0.2, 0.22, face=(-1.0, 0.0, 0.0))
    town.box(m, (0.26, 1.9, 0.06), at=(gx, gy + 0.75, gz + 0.0), look='C')
    rise, going = lr.flight(m, Vector(STAIR_FOOT) + Vector((0.0, 0.0, 0.0)), Vector(STAIR_TOP), (1.0, 0.0, 0.0), 1.5, 40,
                            seed=5922)
    # The handrail on the west side: posts on the stringer, a pole along the nosing line a metre up.
    foot, head = Vector(STAIR_FOOT), Vector(STAIR_TOP)
    run = Vector((0.0, head.y - foot.y, 0.0))
    for k in range(6):
        t = (k + 0.3) / 6.0
        p = foot.lerp(head, t) + Vector((-0.86, 0.0, rise * 0.5))
        mtn.log(m, tuple(p - Vector((0.0, 0.0, 0.2))), tuple(p + Vector((0.0, 0.0, 1.0))), r=0.04, sides=6)
    a = foot + Vector((-0.86, 0.0, 1.0 + rise * 0.5))
    b = head + Vector((-0.86, 0.0, 1.0))
    mtn.log(m, tuple(a), tuple(b), r=0.035, sides=6)
    # Stulls: timbers wedged across the level passage near its roof.
    for y in (58.0, 63.5, 69.0):
        mtn.log(m, (0.15, y, 2.2), (2.85, y + 0.05, 2.25), r=0.12, sides=8)
    return m.finish(ao=False)


def drift_dressing():
    """The drift's tools: picks and a shovel left against the walls."""
    m = mtn.Model('_Drift', seed=5931)
    for base, lean in (((-23.55, 61.0, -8.0), (0.12, 0.1)), ((-18.45, 75.5, -8.0), (-0.12, 0.08)),
                       ((-23.6, 70.2, -8.0), (0.14, -0.06))):
        b = Vector(base)
        t = b + Vector((lean[0], lean[1], 0.88))
        mtn.log(m, tuple(b), tuple(t), r=0.022, sides=6)
        d = (t - b).normalized()
        side = Vector((0.0, 1.0, 0.0)).cross(d).normalized()
        mtn.iron_box(m, (0.05, 0.05, 0.05), at=tuple(t))
        for s in (-1.0, 1.0):
            mtn.rod(m, tuple(t), tuple(t + side * s * 0.27), r=0.016, sides=5)
    b, t = Vector((-18.5, 58.0, -8.0)), Vector((-18.62, 58.3, -6.75))
    mtn.log(m, tuple(b + (t - b) * 0.25), tuple(t), r=0.02, sides=6)
    town.box(m, (0.24, 0.02, 0.3), at=tuple(b + (t - b) * 0.12), rot=(-14.0, 0.0, 0.0), look='iron')
    return m.finish(ao=False)


def decline_dressing():
    """The decline's torn rails at the landing, where the runaway car jumped the track."""
    m = mtn.Model('_Decline', seed=5941)
    for a, b in (((-18.3, 34.15, -3.62), (-20.4, 33.6, -3.92)), ((-18.25, 34.85, -3.6), (-19.7, 35.6, -3.8))):
        mtn.iron_box(m, (math.dist(a, b), 0.04, 0.065), at=tuple((Vector(a) + Vector(b)) * 0.5),
                     rot=(0.0, math.degrees(math.atan2(b[2] - a[2], math.hypot(b[0] - a[0], b[1] - a[1]))),
                          math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))))
    for k in range(3):
        town.box(m, (1.1, 0.13, 0.09), at=(-19.0 - 0.6 * k, 34.5 + 0.3 * (k - 1), -3.7 - 0.12 * k + 0.045),
                 rot=(0.0, 0.0, 70.0 + 15 * k), look='woodblack')
    return m.finish(ao=False)


# --- Sockets and the layout ---

def yaw_facing(front):
    """The yaw (degrees about Z) that turns a socket's front (-Y) toward the horizontal direction front."""
    return math.degrees(math.atan2(front[0], -front[1]))


def lamp_on(kind_pivot_yaw_pitch, side, z=2.15):
    """A lamp on a timber set's post (side -1 or 1, along the set's X), its glass 0.28 m in from the post's inside
    face: (location, yaw)."""
    kind, (x, y, zz), yaw, pitch = kind_pivot_yaw_pitch
    p = SET4 if kind == 'Set' else SETW
    axes = mn.frame(yaw, pitch)
    inner = side * (p['post_x'] - p['post'] * 0.5 - 0.28)
    point = np.array([x, y, zz]) + axes @ np.array([inner, 0.0, z])
    across = axes @ np.array([side, 0.0, 0.0])
    return tuple(point), yaw_facing((-across[0], -across[1]))


def wall_lamp(piece, origin, direction, z_over=0.0):
    """A lamp on a rock wall: the wall found from origin along direction on the piece's mesh, the glass 0.28 m off it
    (the bracket's reach), facing away from the wall."""
    hit = mn.raycast(piece, origin, direction)
    if hit is None:
        raise RuntimeError(f'{piece.name}: no wall from {origin} along {direction}')
    d = Vector(direction).normalized()
    p = hit[0] - d * 0.28
    return (p.x, p.y, p.z + z_over), yaw_facing((-d.x, -d.y))


def sockets(rocks, fall_obj):
    """Every socket of the rock pieces, in the mine's frame: {piece: [(name, location, yaw)]}."""
    all_sets = sets()
    out = {name: [] for name in PIECES}
    adit = [s for s in all_sets if s[0] == 'Set' and s[2] == 0.0 and s[1][2] == 0.0]
    for n, (k, side) in enumerate(((1, 1.0), (4, -1.0), (7, 1.0), (10, -1.0)), start=1):
        loc, yaw = lamp_on(adit[k], side)
        out['Adit'].append((f'Light_{n}', loc, yaw))
    for n, (x, y) in enumerate(PILLARS, start=1):
        side = 1.0 if x > 0 else -1.0
        out['Gallery'].append((f'Light_{n}', (x - side * (0.19 + 0.28), y, 2.4), yaw_facing((-side, 0.0))))
    out['Gallery'] += [('FightCenter', (0.0, 42.0, 0.0), 0.0),
                       ('ChalkTally', (CHALK[0][0] + 0.15, 40.6, 1.6), yaw_facing((1.0, 0.0))),
                       ('LunchPail_1', (6.95, 47.2, 0.455), yaw_facing((-1.0, 0.0))),
                       ('LunchPail_2', (6.95, 48.45, 0.455), yaw_facing((-1.0, 0.0))),
                       ('Winch', WINCH, yaw_facing((-1.0, 0.0)))]
    out['Office'] += [('Light_1', (11.5, 44.57, 1.95), 0.0),
                      ('PayrollBook', (13.3, 40.75, 0.78), yaw_facing((-1.0, 0.0)) + 8.0),   # on the desk's top
                      ('CompanySafe', (13.15, 44.2, 0.06), yaw_facing((-1.0, -0.4))),
                      ('Notices', (11.85, 44.7, 1.6), 0.0)]
    dec_a = [s for s in all_sets if s[2] == 90.0]
    dec_b = [s for s in all_sets if s[0] == 'Set' and s[2] == 0.0 and s[3] != 0.0]
    loc, yaw = lamp_on(dec_a[2], 1.0)
    out['Decline'].append(('Light_1', loc, yaw))
    loc, yaw = lamp_on(dec_b[2], 1.0)
    out['Decline'].append(('Light_2', loc, yaw))
    out['Decline'].append(('Sump', (-22.45, 34.5, -4.5), 0.0))
    wide = [s for s in all_sets if s[0] == 'SetWide']
    for n, (y, side) in enumerate(((52.5, -1.0), (64.5, 1.0), (76.5, -1.0), (79.5, 1.0)), start=1):
        loc, yaw = lamp_on(next(s for s in wide if abs(s[1][1] - y) < 1e-6), side, z=2.3)
        out['Drift'].append((f'Light_{n}', loc, yaw))
    out['Drift'] += [('FightCenter', (-21.0, 66.0, -8.0), 0.0),
                     ('LunchPail_3', (-23.35, 58.6, -8.0), yaw_facing((1.0, 0.0))),
                     ('LunchPail_4', (-18.65, 71.2, -8.0), yaw_facing((-1.0, 0.0))),
                     ('LunchPail_5', (-29.0, 60.4, -8.0), yaw_facing((1.0, 0.0)))]
    stope = rocks['Stope']
    for n, (o, d) in enumerate((((-11.0, 87.0, -5.4), (0.0, -1.0, 0.0)), ((-26.0, 87.5, -5.4), (-1.0, 0.0, 0.0)),
                                ((-26.0, 93.0, -5.4), (-1.0, 0.0, 0.0)), ((-8.0, 91.0, -5.4), (1.0, 0.0, 0.0)),
                                ((-8.0, 95.6, -5.4), (1.0, 0.0, 0.0)), ((-15.5, 87.0, -5.4), (0.0, -1.0, 0.0))),
                               start=1):
        loc, yaw = wall_lamp(stope, o, d)
        out['Stope'].append((f'Light_{n}', loc, yaw))
    out['Stope'] += [('FightCenter', (-17.0, 90.0, -8.0), 0.0),
                     ('PayrollChest', (-7.6, 94.2, -8.0), 0.0),
                     ('Foreman', (-15.0, 95.0, -8.0), yaw_facing((0.0, 1.0))),
                     ('LunchPail_6', (-28.4, 89.6, -8.0), yaw_facing((1.0, 0.0)))]
    fx, fy, fz = FALL
    for n, (x, y) in enumerate(((-22.0, 96.7), (-12.5, 96.9), (-17.5, 96.3)), start=1):
        hit = mn.raycast(fall_obj, (x - fx, y - fy, 6.0), (0.0, 0.0, -1.0), 12.0)
        z = (hit[0].z if hit else 0.4) + fz
        out['Stope'].append((f'Rise_{n}', (x, y, z), 0.0))
    for n, (x, y) in enumerate(PROPS, start=1):
        out['Stope'].append((f'Prop_{n}', (x, y, -8.0), 0.0))
    manway = rocks['Manway']
    for n, (o, d) in enumerate((((-1.2, 87.0, -6.3), (0.0, 1.0, 0.0)), ((1.5, 63.0, 1.95), (-1.0, 0.0, 0.0)),
                                ((1.5, 77.5, -2.4), (1.0, 0.0, 0.0))), start=1):
        loc, yaw = wall_lamp(manway, o, d)
        out['Manway'].append((f'Light_{n}', loc, yaw))
    out['Manway'].append(('ManwayGate', GATE_HINGE, 90.0))
    return out


M_TO_UNREAL = np.array([[0.0, 1.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])    # the mine's frame to Unreal's (X north)
C_MESH = np.array([[0.0, -1.0, 0.0], [-1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])        # Blender mesh axes to Unreal's


def rotation_b(yaw=0.0, pitch=0.0, roll=0.0):
    """A placement's rotation in the mine's frame: roll about the piece's Y, then pitch about its X, then yaw about Z."""
    return np.array(Matrix.Rotation(math.radians(yaw), 3, 'Z') @ Matrix.Rotation(math.radians(pitch), 3, 'X') @
                    Matrix.Rotation(math.radians(roll), 3, 'Y'))


def unreal(location, yaw=0.0, pitch=0.0, roll=0.0):
    """The placement in Unreal's terms, in the portal's frame (X north, Y east, Z up, cm): location and an FRotator
    (pitch, yaw, roll in degrees) for the actor."""
    R = M_TO_UNREAL @ rotation_b(yaw, pitch, roll) @ C_MESH
    X, Y, Z = R[:, 0], R[:, 1], R[:, 2]
    rot = (math.degrees(math.asin(max(-1.0, min(1.0, X[2])))), math.degrees(math.atan2(X[1], X[0])),
           math.degrees(math.atan2(-Y[2], Z[2])))
    loc = M_TO_UNREAL @ np.array(location) * 100.0
    return [round(float(v), 2) for v in loc], [round(v + 0.0, 3) for v in rot]


def placement(asset, location, yaw=0.0, pitch=0.0, roll=0.0, note=''):
    loc_u, rot_u = unreal(location, yaw, pitch, roll)
    return {'asset': 'SM_' + asset, 'mine_m': [round(float(v), 4) for v in location],
            'yaw_pitch_roll_mine_deg': [round(yaw, 3), round(pitch, 3), round(roll, 3)],
            'unreal_cm': loc_u, 'unreal_rotator_pitch_yaw_roll': rot_u, 'note': note}


def tracks():
    """The tramway pieces through the mine: (asset, location, yaw, pitch, note). A track piece runs along its own -Y,
    so yaw 180 runs it north, -90 west; pitch tips its run down a decline."""
    out = []
    for k in range(11):
        out.append(('RR_TramTrack_4m', (0.0, 4.0 * k, 0.0), 180.0, 0.0, 'adit and the gallery spur'))
    out.append(('RR_TramTrack_End', (0.0, 44.0, 0.0), 180.0, 0.0, "the gallery spur's end"))
    out.append(('RR_TramTrack_2m', (-6.0, 34.5, 0.0), -90.0, 0.0, "from the winch to the decline's head"))
    ta, tb = math.radians(SLOPE_A), math.radians(SLOPE_B)
    s = 0.0
    for length in (4, 4, 2):
        out.append((f'RR_TramTrack_{length}m', (-8.0 - s * math.cos(ta), 34.5, -s * math.sin(ta)), -90.0, SLOPE_A,
                    "the decline's first run (torn up 1.7 m short of the landing)"))
        s += length
    s = 0.0
    for length in (4, 4, 4):
        out.append((f'RR_TramTrack_{length}m', (-21.0, 36.5 + s * math.cos(tb), -4.0 - s * math.sin(tb)), 180.0, SLOPE_B,
                    "the decline's second run"))
        s += length
    for k in range(8):
        out.append(('RR_TramTrack_4m', (-21.0, 48.0 + 4.0 * k, -8.0), 180.0, 0.0, 'the drift'))
    out.append(('RR_TramTrack_2m', (-21.0, 80.0, -8.0), 180.0, 0.0, 'the drift, to the barricade'))
    out.append(('RR_TramTrack_4m', (-21.0, 83.2, -8.0), 180.0, 0.0, "the stope's stub"))
    out.append(('RR_TramTrack_2m', (-21.0, 87.2, -8.0), 180.0, 0.0, "the stope's stub"))
    out.append(('RR_TramTrack_End', (-21.0, 89.2, -8.0), 180.0, 0.0, "the stope's stub's end"))
    return out


def layout(socket_table, rock_objs, kit_objs):
    """Mine.layout.json: every piece's placement in the mine's frame and in Unreal's, the rooms, the sockets."""
    pieces = []
    for name in PIECES:
        pieces.append(placement('RR_Mine_' + name, PIVOTS[name], note='rock piece (mesh collision)'))
    pieces.append(placement('RR_Mine_Shell', (0.0, 0.0, 0.0), note='closed hull for the sun\'s shadow; no collision'))
    pieces.append(placement('RR_Mine_Fall', FALL, note="the stope's north end"))
    for kind, loc, yaw, pitch in sets():
        pieces.append(placement('RR_Mine_TimberSet' if kind == 'Set' else 'RR_Mine_TimberSetWide', loc, yaw, pitch))
    for x, y in PILLARS:
        pieces.append(placement('RR_Mine_Pillar', (x, y, 0.0)))
    for n, (x, y) in enumerate(PROPS, start=1):
        pieces.append(placement('RR_Mine_StopeProp', (x, y, -8.0), note=f'SOCKET_Prop_{n}; swap for RR_Mine_StopePropBroken'))
    pieces.append(placement('RR_Mine_Barricade', BARRICADE))
    pieces.append(placement('RR_Mine_BarricadeTags', BARRICADE, note='taken at Back Pay step 5'))
    pieces.append(placement('RR_Mine_ManwayGate', GATE_HINGE, 90.0, note='shut; open by turning about Z (hinge)'))
    pieces.append(placement('RR_Mine_Winch', WINCH, yaw_facing((-1.0, 0.0)), note="at the decline's head"))
    for asset, loc, yaw, pitch, note in tracks():
        pieces.append(placement(asset, loc, yaw, pitch, note=note))
    cars = [((0.0, 37.0, 0.0), 180.0, 0.0, 0.0, 'on the gallery spur'),
            ((5.6, 51.4, 0.5), 32.0, 0.0, 90.0, "on its side in the gallery's north-east corner"),
            ((-22.45, 34.5, -4.0), 0.0, 0.0, 90.0, "the runaway, on its side in the landing's sump"),
            ((-21.0, 86.4, -8.0), 180.0, 0.0, 0.0, "on the stope's stub"),
            ((-11.6, 93.4, -7.5), 75.0, 0.0, 90.0, 'on its side by the fall')]
    for loc, yaw, pitch, roll, note in cars:
        pieces.append(placement('RR_OreCar', loc, yaw, pitch, roll, note=note))
    lamps = []
    for piece, entries in socket_table.items():
        for name, loc, yaw in entries:
            if name.startswith('Light_'):
                lamps.append(placement('RR_Mine_Lantern', loc, yaw, note=f'{piece} SOCKET_{name}'))
    rooms = []
    for r in ROOMS:
        rooms.append({'name': r.name, 'box_min_m': [round(float(v), 2) for v in r.box[0]],
                      'box_max_m': [round(float(v), 2) for v in r.box[1]]})
    data = {
        'frame': "the mine's frame: metres, x east, y north (into Larkspur High), z up; origin the adit's mouth on its "
                 "floor at the portal (design (74, 230), +58 m). Unreal: the portal's frame, X = y * 100 (north), "
                 "Y = x * 100 (east), Z = z * 100 cm; every mine piece is built in the frame's own axes, so a piece "
                 "with yaw 0 here is yaw 180 in Unreal (unreal_rotator_* gives each one exactly).",
        'world_origin_design_m': [74.0, 230.0, 58.0],
        'floors_m': {'adit, gallery, office, manway top': 0.0, 'drift, side stopes, stope, manway foot': -8.0,
                     'decline': '0 at the gallery wall (x -8) to -4 at the landing (x -19..-23) to -8 at the drift (y 48)'},
        'mouth': {'opening_m': 'the rock ends at y = -1.0 in a ring about 4.0 wide and 3.0-3.5 high; SM_RR_MinePortal '
                               '(Diggings.py) covers y -3.6..0 with its lagged entry (clear 3.6 x 2.95 m), its wings and '
                               'brow reaching x -4.85..4.85 and up to z 5.3; its doors open out to y -5.2',
                  'portal_asset': 'SM_RR_MinePortal at the mine origin, same transform as the pieces (yaw 180 in Unreal)',
                  'terrain': 'the cliff band notched round the portal (about 10 m wide, 5.5 m high, its face near y -3.4); '
                             'the ground over the whole footprint (x -34..16, y 2..106) above z +8 (+66 m), the mine '
                             'shell (SM_RR_Mine_Shell) reaching z +6.6'},
        'rooms': rooms,
        'pieces': pieces,
        'lanterns': lamps,
        'sockets': {piece: [{'name': 'SOCKET_' + n, 'mine_m': [round(float(v), 4) for v in loc], 'yaw_deg': round(yaw, 2)}
                            for n, loc, yaw in entries] for piece, entries in socket_table.items()},
    }
    with open(LAYOUT_PATH, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent=1)
    lt._log(f'mine: layout {LAYOUT_PATH} ({len(pieces)} pieces, {len(lamps)} lanterns)')
    return data


def attach_sockets(obj, pivot, entries):
    for name, loc, yaw in entries:
        lm.socket(obj, name, tuple(Vector(loc) - Vector(pivot)), (0.0, 0.0, yaw))


# --- Previews ---

LABELS = [('ADIT  z 0', (0.0, 14.0)), ('GALLERY  z 0', (0.0, 42.0)), ('OFFICE', (11.5, 42.0)),
          ('DECLINE  0 to -8', (-15.0, 38.5)), ('LOWER DRIFT  z -8', (-21.0, 66.0)), ('STOPE  z -8', (-17.0, 89.5)),
          ('THE FALL', (-17.0, 99.5)), ('MANWAY  -8 to 0', (1.5, 64.0)), ('BARRICADE', (-21.0, 81.0)),
          ('MOUTH', (0.0, -2.5))]


def instances(data, library, skip=()):
    """Copies of the library's models where the layout puts them (sharing their meshes), for the renders."""
    out = []
    for p in data['pieces'] + data['lanterns']:
        name = p['asset'][3:]
        if name in skip or name not in library:
            continue
        src = library[name]
        inst = bpy.data.objects.new('_I_' + name, src.data)
        bpy.context.scene.collection.objects.link(inst)
        yaw, pitch, roll = p['yaw_pitch_roll_mine_deg']
        inst.matrix_world = Matrix.Translation(p['mine_m']) @ Matrix(rotation_b(yaw, pitch, roll).tolist()).to_4x4()
        if name.startswith('RR_Mine_Lantern'):
            inst.visible_shadow = False      # its glass would shut its own lamp in (set the same in the game)
        out.append(inst)
    return out


def cut_above_floor(obj, field, height=2.4, offset=(0.0, 0.0, 0.0)):
    """A copy of a rock piece without the faces higher than `height` over its room's floor: a plan view's cut."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = '_Cut_' + obj.name
    for child in list(copy.children):
        child.parent = None
    bpy.context.scene.collection.objects.link(copy)
    mesh = copy.data
    n = len(mesh.polygons)
    C = np.empty(3 * n)
    mesh.polygons.foreach_get('center', C)
    C = C.reshape(-1, 3) + np.array(offset)
    floor = field.cave(C)[1]
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.faces.ensure_lookup_table()
    gone = [bm.faces[i] for i in np.nonzero(C[:, 2] > floor + height)[0]]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    bm.to_mesh(mesh)
    bm.free()
    copy.matrix_world = Matrix.Translation(offset)
    copy.hide_render = False
    return copy


def label(text, at, size=1.4, color=0xf2a64a, z=12.0):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    try:
        curve.font = bpy.data.fonts.load(mtn.homes.FONT, check_existing=True)
    except Exception:
        pass
    curve.size = size
    curve.align_x, curve.align_y = 'CENTER', 'CENTER'
    mat = bpy.data.materials.new('_LabelMat')
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(color)
    emit.inputs['Strength'].default_value = 1.6
    mat.node_tree.links.new(emit.outputs['Emission'], out.inputs['Surface'])
    curve.materials.append(mat)
    obj = bpy.data.objects.new('_Label', curve)
    obj.location = (at[0], at[1], z)
    obj.rotation_euler = (0.0, 0.0, math.radians(90.0))
    bpy.context.scene.collection.objects.link(obj)
    return obj


def render(scene, path, eye, target, lens=22.0, size=(1600, 900), exposure=0.0, ortho=None, up=None):
    cam = bpy.data.objects.get('_MineCam') or bpy.data.objects.new('_MineCam', bpy.data.cameras.new('_MineCam'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    if up is None:
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    else:
        forward = (Vector(target) - Vector(eye)).normalized()
        right = forward.cross(Vector(up)).normalized()
        cam_up = right.cross(forward)
        cam.rotation_euler = Matrix((right, cam_up, -forward)).transposed().to_euler()
    cam.data.type = 'ORTHO' if ortho else 'PERSP'
    if ortho:
        cam.data.ortho_scale = ortho
    cam.data.lens = lens
    cam.data.clip_start, cam.data.clip_end = 0.05, 500.0
    cam.data.sensor_fit = 'HORIZONTAL'
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    lt._log(f'mine: rendered {path}')
    return path


def previews(rocks, kits, data, field):
    """The plan (cut at 2.4 m over each floor, north to the right), the rooms lit only by their lamps, and the kit."""
    import runpy
    saved = sys.argv[:]
    sys.argv = [a for a in sys.argv if a != '--preview']
    before = set(bpy.data.objects)
    runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'Tramway.py'), run_name='__tramway__')
    sys.argv = saved
    library = {o.name: o for o in bpy.data.objects if o not in before and o.parent is None and o.type == 'MESH'}
    library.update({o.name: o for o in kits})
    for o in library.values():
        o.hide_render = True
    for o in rocks.values():
        o.hide_render = True
    scene, sun = mtn.stage(samples=48)
    for o in scene.objects:
        if o.name in ('_Ground',):
            o.hide_render = True
    shots = {}

    # The plan: the rock cut over each floor, the floor's things in place, lit from above.
    cuts = [cut_above_floor(obj, field, 2.4, PIVOTS[name]) for name, obj in rocks.items()]
    floor_kit = instances(data, library, skip=('RR_Mine_TimberSet', 'RR_Mine_TimberSetWide', 'RR_Mine_Pillar',
                                               'RR_Mine_StopeProp', 'RR_Mine_Lantern', 'RR_Mine_Shell'))
    for inst in floor_kit:
        if inst.name.startswith('_I_RR_Mine_Barricade'):
            inst.hide_render = True
    props = []
    for x, y in PILLARS + PROPS:
        z = 0.0 if y < 60 else -8.0
        dot = bpy.data.objects.new('_I_Post', library['RR_Mine_StopeProp'].data)
        scene.collection.objects.link(dot)
        dot.matrix_world = Matrix.Translation((x, y, z)) @ Matrix.Diagonal((1.0, 1.0, 0.3, 1.0))
        props.append(dot)
    labels = [label(text, at) for text, at in LABELS]
    world = mtn.dark_world(0.6)
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.32, 0.34, 0.38, 1.0)
    scene.world = world
    sun.data.energy, sun.data.angle = 3.0, math.radians(3.0)
    sun.rotation_euler = (math.radians(18.0), math.radians(-12.0), 0.0)
    shots['plan'] = render(scene, os.path.join(PREVIEWS, 'Mine_Plan.png'), (-9.0, 51.0, 80.0), (-9.0, 51.0, -8.0),
                           size=(2400, 1250), ortho=116.0, up=(-1.0, 0.0, 0.0))
    for o in cuts + props + labels:
        bpy.data.objects.remove(o)
    for o in floor_kit:
        bpy.data.objects.remove(o)

    # Inside: everything in place, the sun off, a faint grey fill (the game's sky light underground), the lamps lit.
    full = instances(data, library, skip=('RR_Mine_Shell', 'RR_Mine_BarricadeTags'))
    tags = instances({'pieces': [p for p in data['pieces'] if p['asset'] == 'SM_RR_Mine_BarricadeTags'], 'lanterns': []},
                     library)
    rock_inst = []
    for name, obj in rocks.items():
        inst = bpy.data.objects.new('_I_' + obj.name, obj.data)
        scene.collection.objects.link(inst)
        inst.matrix_world = Matrix.Translation(PIVOTS[name])
        rock_inst.append(inst)
    sun.hide_render = True
    sun.data.energy = 0.0
    world = mtn.dark_world(0.35)
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.36, 0.38, 0.42, 1.0)
    scene.world = world
    lamps = [mtn.point_light('_Lamp', p['mine_m'], energy=22.0) for p in data['lanterns']]
    views = {
        'Mine_Adit': ((0.7, 3.0, 1.5), (-0.2, 30.0, 1.4), 22.0),
        'Mine_Gallery': ((-1.2, 30.6, 1.7), (2.5, 50.0, 1.6), 18.0),
        'Mine_Office': ((9.7, 39.7, 1.65), (13.5, 43.8, 0.85), 16.0),
        'Mine_Decline': ((-7.6, 33.4, 1.5), (-20.0, 35.4, -3.0), 20.0),
        'Mine_Drift': ((-20.6, 50.0, -6.4), (-21.0, 82.5, -6.8), 22.0),
        'Mine_Barricade': ((-22.3, 78.6, -6.35), (-21.0, 82.6, -6.25), 24.0),
        'Mine_Stope': ((-5.4, 85.0, -6.0), (-20.0, 98.5, -5.6), 16.0),
        'Mine_Manway': ((1.6, 87.6, -6.7), (1.5, 72.5, 1.1), 20.0),
    }
    for name, (eye, target, lens) in views.items():
        shots[name] = render(scene, os.path.join(PREVIEWS, name + '.png'), eye, target, lens=lens, size=(1600, 900),
                             exposure=0.6)
    for o in full + tags + rock_inst + lamps:
        bpy.data.objects.remove(o)

    # The kit in a row, in daylight, with a 1.8 m figure.
    homes = mtn.homes
    sun.hide_render = False
    row = ['RR_Mine_TimberSet', 'RR_Mine_TimberSetWide', 'RR_Mine_Pillar', 'RR_Mine_StopeProp', 'RR_Mine_StopePropBroken',
           'RR_Mine_Barricade', 'RR_Mine_ManwayGate', 'RR_Mine_Winch', 'RR_Mine_Lantern', 'RR_OreCar']
    x = 0.0
    shown = []
    for name in row:
        src = library[name]
        inst = bpy.data.objects.new('_K_' + name, src.data)
        scene.collection.objects.link(inst)
        lo_ = min(v.co.x for v in src.data.vertices)
        hi_ = max(v.co.x for v in src.data.vertices)
        inst.location = (x - lo_, 0.0, 1.5 if name == 'RR_Mine_Lantern' else 0.0)
        x += hi_ - lo_ + 0.9
        shown.append(inst)
    tags_k = bpy.data.objects.new('_K_Tags', library['RR_Mine_BarricadeTags'].data)
    scene.collection.objects.link(tags_k)
    tags_k.location = next(o for o in shown if o.name.startswith('_K_RR_Mine_Barricade')).location
    human = homes.figure()
    human.location = (x + 0.3, 0.5, 0.0)
    for o in scene.objects:
        if o.name == '_Ground':
            o.hide_render = False
    bpy.context.view_layer.update()
    shots['kit'] = mtn.shot(scene, sun, shown + [tags_k], 'Mine_Kit', view=(-0.25, -1.0, 0.32), lens=40.0, figure=human,
                            size=(2400, 900), folder=PREVIEWS)
    fall_k = bpy.data.objects.new('_K_Fall', library['RR_Mine_Fall'].data)
    scene.collection.objects.link(fall_k)
    for o in shown + [tags_k]:
        o.hide_render = True
    human.location = (5.0, -2.0, 0.0)
    shots['fall'] = mtn.shot(scene, sun, [fall_k], 'Mine_Fall', view=(-0.3, -1.0, 0.45), lens=35.0, figure=human,
                             folder=PREVIEWS)
    return shots


# --- Build ---

field = build_field()
rocks, fall_obj, kit_objs = {}, None, []
if ONLY in (None, 'kit'):
    kit_objs = [timber_set('RR_Mine_TimberSet', SET4, 5801), timber_set('RR_Mine_TimberSetWide', SETW, 5811), pillar(),
                stope_prop(False), stope_prop(True), barricade(), barricade_tags(), manway_gate(), winch(),
                mine_lantern(True), mine_lantern(False)]
    fall_obj = fall()
    shell_obj = shell()
    kit_objs += [fall_obj, shell_obj]
if ONLY in (None, 'rock'):
    dressing = {'Office': [office_dressing()], 'Gallery': [gallery_dressing()], 'Manway': [manway_dressing()],
                'Drift': [drift_dressing()], 'Decline': [decline_dressing()]}
    for name in ('_Office', '_Gallery', '_Manway', '_Drift', '_Decline'):
        homes_reports_pop(name)
    for objs in dressing.values():
        for obj in objs:
            for child in list(obj.children):
                bpy.data.objects.remove(child)
    rocks = rock(field, dressing)
    table = sockets(rocks, fall_obj) if fall_obj is not None else None
    for name, obj in rocks.items():
        obj.data.transform(Matrix.Translation(-Vector(PIVOTS[name])))
        obj.data.update()
        obj['Fallback'] = 100.0
        if table:
            attach_sockets(obj, PIVOTS[name], table[name])
    if table and ONLY is None:
        data = layout(table, rocks, kit_objs)
        if lt.want_preview():
            previews(rocks, kit_objs, data, field)
for obj in list(rocks.values()) + kit_objs:
    mtn.describe(obj)
mtn.save_reports('Mine')
