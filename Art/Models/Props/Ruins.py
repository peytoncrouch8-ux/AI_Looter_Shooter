"""Ruins and obstacle dressing for Ransom's Rest (Docs/Areas/RansomsRest.md: "What breaks up the open ground"). A
scripted model file (Art/README.md) built with looter_buildings and looter_props through Tools/Blender/looter_ruins.py.
Every pivot is on the ground at the middle of the footprint and the front faces -Y, unless a model says otherwise.

  Ruin_Chimney        the Burnt Homestead (-72, 49): a 6.6 m fieldstone chimney standing at the gable end of a burnt
                      farmhouse's foundation (7.4 x 5.4 m), its firebox charred, an iron crane and pot still in it, a
                      charred lintel, the hearth stone, charred sill timbers, two charred corner posts and a fallen
                      beam leaning on the stack
  Ruin_LanternHouse   the old lantern house (-102, 35): a roofless fieldstone hut (4.1 x 5.0 m) with a door gap in the
                      front gable, a window gap, lantern niches in the back gable, a stone bench and a lantern shelf
                      inside, its ridge beam fallen across it
  Ruin_Springhouse    over Mill Creek's spring (84, -16): a roofless stone springhouse (3.4 x 3.8 m, walls 1.0-1.45 m),
                      its floor a flat basin held in by the walls and a stone curb across the doorway; the spring
                      comes in by a spout in the back wall and leaves by a lip in the front wall. The level adds the
                      water at SOCKET_Water
  Ruin_LiveryFooting  the burnt livery (15, -31): the stone footing of an 8 x 12 m stable, its charred posts burnt to
                      stumps and stubs (none over 2.8 m), stall partitions, fallen beams and roof tin
  Ruin_Derrick        the Old Quarry's fallen derrick (20, 95): the mast lying broken in two on its foot, the boom
                      across it with its sheave block and hook, a stiff leg still propped across the mast, the bull
                      wheel fallen flat, the hand winch and a coil of rope, guy ropes trailing (16 x 11 m with them)
  DressedBlock_A, _B  squared quarry blocks (1.2 m and 1.7 m), for the quarry floor and the Spoil Bank
  Wagon_BurntA, _B    burnt freight wagons by the town gate (-17, -19): A stands slumped on a burnt-out wheel, B lies
                      tipped on its side with its cargo spilled
  StoneWall_Broken    a 3 m dry-stone wall segment breached in the middle (the old pound wall)
  StoneWall_Fallen    a 3 m stretch of fallen wall: a low remnant and its spill (the fallen glebe wall)
  StoneWall_Corner    a square corner with 1.5 m arms (the sheep fold)
  StoneWall_Half      a 1.5 m length of standing wall with a through stone (the sheep fold's gate side)
  Cairn_A, _B, _C     knee-high keeper's cairns (the west road to the Keeper's Gate); C carries a lantern hook
  HangingRope         the hanging tree's frayed rope and noose (20, 46). Pivot at the top, the branch's middle;
                      no collision

The wall pieces chain with Fences.py's StoneWall (3 m, along +X from its pivot) and use its own profile, surface and
texture mapping (looter_ruins.fences_wall()), so their full-height ends match SM_StoneWall and SM_StoneWallEnd without a
seam:
  - StoneWall_Broken and StoneWall_Corner have full ends at both joints: put them anywhere in a standing run.
    StoneWall_Corner turns left (toward +Y): its outgoing end is at (1.5, 1.5), facing +Y, where the next segment
    starts turned 90 degrees, so a corner plus whole walls make sides of 3 m, 6 m, 9 m...
  - StoneWall_Fallen has low remnant ends: it chains with itself (and its spill continues), not with standing wall.
  - StoneWall_Half has full ends at both joints (x = 0 and x = 1.5): it fits anywhere in a standing run, so runs of
    corners and walls step by 1.5 m instead of 3 m (a corner's arm, a half and a StoneWallEnd each side of a 1.2 m
    gateway make a 9 m gate side).

Big pieces keep Nanite (their whole mesh is the fallback, within 1-4k triangles); the walls keep Nanite like
SM_StoneWall; small ones (blocks, cairns, the rope) have no Nanite and LODs of 50% and 25%.
"""
import math
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Opening, Tile, Trim
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

W = lr.fences_wall()
SMALL = dict(Nanite=0, LODs='50,25')


# --- Shared pieces ---

def charred(m, p0, p1, width, thick, seed, face=(0.0, 0.0, 1.0), burn=(0.0, 0.0), jag=0.08, mat='charcoal',
            uv=None, cuts=None, splinters=None):
    """A charred timber from p0 to p1 (its middle line), width across and thick deep: alligatored char (Charcoal on
    the bark texture). An end with burn > 0 (metres at p0, metres at p1) is eaten in over that length, left blunt and
    ragged (cut at a slant, jag metres uneven) with a few splinters standing out of it."""
    rnd = random.Random(seed)
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    cuts = max(1, int(length / 0.45)) if cuts is None else cuts
    tb = kit._box((length, thick, width), 0.0, cuts)
    phase = Vector((rnd.uniform(0, 40), rnd.uniform(0, 40), rnd.uniform(0, 40)))
    ends = [rnd.uniform(0.5, 0.68), rnd.uniform(0.5, 0.68)]          # how thick each burnt end is left
    slant = [Vector((0.0, rnd.uniform(-1, 1), rnd.uniform(-1, 1))) for _ in range(2)]

    def shape(co):
        t = min(max(co.x / length + 0.5, 0.0), 1.0)
        k = 1.0
        if burn[0] > 0.0 and t * length < burn[0]:
            k = min(k, ends[0] + (1.0 - ends[0]) * (t * length / burn[0]) ** 0.8)
        if burn[1] > 0.0 and (1.0 - t) * length < burn[1]:
            k = min(k, ends[1] + (1.0 - ends[1]) * ((1.0 - t) * length / burn[1]) ** 0.8)
        n = noise.noise(co * 4.0 + phase)
        x = co.x
        for end, sign, test in ((0, -1.0, t < 0.02), (1, 1.0, t > 0.98)):
            if test and burn[end] > 0.0:
                # A slanted, uneven end: char doesn't burn square.
                tilt = slant[end].y * co.y / max(thick, 1e-3) + slant[end].z * co.z / max(width, 1e-3)
                x -= sign * (jag * (0.6 + 0.4 * tilt) + jag * 0.5 * noise.noise(co * 23.0 + phase))
        return Vector((x, co.y * k * (1.0 + 0.1 * n), co.z * k * (1.0 + 0.1 * n)))
    frame = kit.toward(p0, p1, face)
    m.emit(tb, uv or lr.Grain('BarkOak'), mat, frame @ Matrix.Translation((length * 0.5, 0.0, 0.0)), shape=shape)
    # Splinters out of the burnt ends.
    for end in (0, 1):
        if burn[end] <= 0.0:
            continue
        count = (rnd.randint(1, 3) if min(width, thick) > 0.09 else 1) if splinters is None else splinters
        for _ in range(count):
            lng = rnd.uniform(0.07, 0.2) * min(1.0, max(width, thick) / 0.15)
            s = rnd.uniform(0.022, 0.04)
            oy = rnd.uniform(-0.3, 0.3) * thick * ends[end]
            oz = rnd.uniform(-0.3, 0.3) * width * ends[end]
            x = (length + lng * 0.5 - jag) if end else (-lng * 0.5 + jag)
            tip = Euler((0.0, math.radians(rnd.uniform(-10, 10)), math.radians(rnd.uniform(-10, 10)))).to_matrix()
            sp = kit._box((lng, s, s * 1.3), 0.0, 0)
            taper = (lambda co, lng=lng, end=end: Vector((co.x, co.y * (1.0 - 0.6 * ((co.x / lng + 0.5) if end else
                                                         (0.5 - co.x / lng))), co.z * (1.0 - 0.6 * ((co.x / lng + 0.5)
                                                         if end else (0.5 - co.x / lng))))))
            m.emit(sp, lr.Grain('BarkOak'), mat, frame @ Matrix.Translation((x, oy, oz)) @ tip.to_4x4(), shape=taper)


def rubble(m, center, size, seed, key='trim', strip='Stone', set_name=None, tilt=18.0):
    """A loose stone lying on the ground (its bottom near center's z)."""
    rnd = random.Random(seed)
    sx, sy, sz = size
    c = Vector(center) + Vector((0.0, 0.0, sz * 0.32))
    # Small stones are plain lumpy boxes (12 triangles); bigger ones get their edges knocked off.
    part = lr.stone_block(size, c, (rnd.uniform(-tilt, tilt), rnd.uniform(-tilt, tilt), rnd.uniform(0.0, 180.0)),
                          seed=seed, rough=min(size) * 0.09, bevel=min(size) * 0.18 if max(size) > 0.33 else 0.0,
                          key=key, strip=strip, set_name=set_name)
    m.add(part)


def ragged(rng, length, base, rough, lo_at=None, drop=0.0, steps=9):
    """A wall's top from right to left (as an outline continues): base height, stepped roughness, and a breach of
    depth drop centered at lo_at (0..1 along the wall). (LookoutTower.py's ruined walls.)"""
    points = []
    for k in range(steps, -1, -1):
        t = k / steps
        z = base + rng.uniform(-rough, rough)
        if lo_at is not None:
            z -= drop * max(0.0, 1.0 - abs(t - lo_at) / 0.35)
        x = length * t
        points.append((x, z))
        if 0 < k:
            points.append((x - length / steps * rng.uniform(0.25, 0.6), z + rng.uniform(-rough, rough) * 0.8))
    return points


def gable_top(rng, length, eave, peak, rough, fall=None, steps=10):
    """A gable wall's top from right to left: up from the eave to the peak and down again, ragged; fall = (t0, t1,
    height) knocks the part between t0 and t1 (0..1 from the left) down to about that height."""
    points = []
    for k in range(steps, -1, -1):
        t = k / steps
        z = eave + (peak - eave) * (1.0 - abs(t - 0.5) * 2.0) + rng.uniform(-rough, rough)
        if fall is not None and fall[0] <= t <= fall[1]:
            z = min(z, fall[2] + rng.uniform(-rough, rough) * 1.5)
        points.append((length * t, z))
        if 0 < k:
            x = length * (t - rng.uniform(0.3, 0.6) / steps)
            tt = x / length
            zz = eave + (peak - eave) * (1.0 - abs(tt - 0.5) * 2.0) + rng.uniform(-rough, rough)
            if fall is not None and fall[0] <= tt <= fall[1]:
                zz = min(zz, fall[2] + rng.uniform(-rough, rough) * 1.5)
            points.append((x, zz))
    return points


def top_at(top, x):
    """The height of a top outline (from right to left) at x."""
    pts = sorted(top)
    for (x0, z0), (x1, z1) in zip(pts, pts[1:]):
        if x0 <= x <= x1:
            return z0 + (z1 - z0) * (x - x0) / max(x1 - x0, 1e-6)
    return pts[0][1] if x < pts[0][0] else pts[-1][1]


def stone_wall(m, p0, p1, thick, top, openings=(), inset=0.0, uv=None, hull=True, slices=1.0):
    """A ruined fieldstone wall from p0 to p1 (outside on the right, as kit.wall_space): its top outline (right to
    left) and openings; hull boxes follow the wall strip by strip under its lowest top (open where the openings are).
    Returns the wall's space (shifted by inset)."""
    space = kit.wall_space(p0, p1, 0.0)
    length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
    local = kit.Space(space.matrix @ Matrix.Translation((inset, 0.0, 0.0)))
    outline = kit.wall_outline(length, top[0][1], [o for o in openings if o.z <= 1e-4], None)[:-2] + top
    m.panel(outline, kit.holes([o for o in openings if o.kind != 'niche']), thick, uv or Trim('D', world=True, v=0.13),
            space=local, around=list(openings), slices=slices)
    if hull:
        edges = {0.0, length}
        for o in openings:
            if o.kind != 'niche':
                edges |= {o.x, o.x + o.w}
        x = 1.2
        while x < length - 0.3:
            edges.add(x)
            x += 1.2
        edges = sorted(e for e in edges if -1e-6 <= e <= length + 1e-6)
        for a, b in zip(edges, edges[1:]):
            if b - a < 0.05:
                continue
            mid = (a + b) * 0.5
            za, zb = top_at(top, a + 0.02) - 0.06, top_at(top, b - 0.02) - 0.06
            blocked = [o for o in openings if o.kind != 'niche' and o.x - 1e-4 <= mid <= o.x + o.w + 1e-4]
            spans = [(0.0, None)]
            for o in blocked:
                spans = [(0.0, o.z), (o.z + o.h, None)] if o.z > 0.05 else [(o.z + o.h, None)]
            for z0, z1 in spans:
                lo = z0
                if z1 is not None:
                    hi_a = hi_b = z1
                else:
                    hi_a, hi_b = za, zb
                if min(hi_a, hi_b) - lo < 0.08:
                    continue
                m.hull_points([(a, 0.0, lo), (b, 0.0, lo), (a, thick, lo), (b, thick, lo),
                               (a, 0.0, hi_a), (b, 0.0, hi_b), (a, thick, hi_a), (b, thick, hi_b)], space=local)
    return local


# --- Ruin_Chimney: the Burnt Homestead ---

def ruin_chimney(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    HX, HY = 3.7, 2.7       # half the foundation's outer size
    FT = 0.42               # foundation thickness
    # The chimney straddles the left gable wall: firebox inside the wall line, the stack outside.
    CX0, CX1 = -HX - 0.78, -HX + 0.44     # its base, along X (the house side is +X)
    CY = 0.9                               # half its width
    BASE_TOP, SHOULDER, TOP = 2.1, 2.65, 6.4
    FB_W, FB_H, FB_D = 0.96, 0.86, 0.5    # the firebox opening (width along Y, height) and depth

    # The stack: the firebox section (built round the opening), the shoulders weathering in, and the flue.
    stone = Trim('D', world=True, v=0.21)
    back = CX1 - FB_D
    m.box((back - CX0, 2 * CY, BASE_TOP), at=((CX0 + back) * 0.5, 0.0, BASE_TOP * 0.5), uv=stone, cuts=1)
    for side in (-1.0, 1.0):
        w = CY - FB_W * 0.5
        m.box((FB_D, w, BASE_TOP), at=((back + CX1) * 0.5, side * (FB_W * 0.5 + w * 0.5), BASE_TOP * 0.5), uv=stone)
    m.box((FB_D, FB_W, BASE_TOP - FB_H), at=((back + CX1) * 0.5, 0.0, (BASE_TOP + FB_H) * 0.5), uv=stone)
    flue_x = (CX0 + CX1) * 0.5 - 0.12      # the flue rises over the back of the firebox
    fx, fy = 0.74, 0.92
    def shoulder(co):
        """Weathering in from the firebox section to the flue, and over toward the flue's side."""
        t = co.z / (SHOULDER - BASE_TOP) + 0.5
        return Vector((co.x * (1.0 - (1.0 - fx / (CX1 - CX0)) * t) + (flue_x - (CX0 + CX1) * 0.5) * t,
                       co.y * (1.0 - (1.0 - fy / (2 * CY)) * t), co.z))
    m.emit(kit._box((CX1 - CX0, 2 * CY, SHOULDER - BASE_TOP)), stone, 'trim',
           kit.place(((CX0 + CX1) * 0.5, 0.0, (BASE_TOP + SHOULDER) * 0.5)), shape=shoulder)
    kit.stone_stack(m, (flue_x, 0.0, SHOULDER), (fx, fy), TOP - SHOULDER, taper=0.035)
    # A corbelled cap course with a stone knocked out of it, and the flue's dark throat.
    m.box((fx + 0.16, fy + 0.16, 0.12), at=(flue_x, 0.0, TOP + 0.02), uv=stone, bevel=0.02)
    for k, (dx, dy, w, d) in enumerate(((-0.2, -0.22, 0.32, 0.4), (0.18, 0.2, 0.36, 0.46), (-0.2, 0.25, 0.3, 0.36))):
        m.add(lr.stone_block((w, d, 0.13), (flue_x + dx, dy, TOP + 0.14), (0.0, 0.0, rng.uniform(-8, 8)),
                             seed=seed + 50 + k, rough=0.008, bevel=0.02))
    m.box((fx * 0.5, fy * 0.42, 0.02), at=(flue_x, 0.0, TOP + 0.09), uv=lr.Grain('BarkOak'), mat='charcoal')
    m.section('chimney')

    # Soot lining the firebox, a charred timber lintel and the hearth stone in front.
    lin = 0.012
    m.box((lin, FB_W - 0.02, FB_H - 0.01), at=(back + lin * 0.5 + 0.003, 0.0, FB_H * 0.5), uv=lr.Grain('BarkOak'),
          mat='charcoal')
    for side in (-1.0, 1.0):
        m.box((FB_D - 0.01, lin, FB_H - 0.01),
              at=(back + FB_D * 0.5, side * (FB_W * 0.5 - lin * 0.5 - 0.003), FB_H * 0.5), uv=lr.Grain('BarkOak'),
              mat='charcoal')
    m.box((FB_D - 0.01, FB_W - 0.02, lin), at=(back + FB_D * 0.5, 0.0, FB_H - lin * 0.5 - 0.003),
          uv=lr.Grain('BarkOak'), mat='charcoal')
    charred(m, (CX1 + 0.02, -FB_W * 0.5 - 0.22, FB_H + 0.11), (CX1 + 0.02, FB_W * 0.5 + 0.22, FB_H + 0.11), 0.22, 0.2,
            seed + 1, face=(1.0, 0.0, 0.0), burn=(0.0, 0.0))
    m.add(lr.stone_block((0.62, 1.55, 0.12), (CX1 + 0.36, 0.0, 0.03), (0.0, 1.5, 0.0), seed=seed + 2, rough=0.006,
                         bevel=0.02))
    # The iron crane on the left jamb: a pintle bar, an arm with its brace, a hook and a pot.
    iron = Trim('H3', fit=True)
    px, py = CX1 - 0.07, -FB_W * 0.5 + 0.06
    m.cylinder((px, py, 0.08), (px, py, 0.8), 0.016, sides=6, uv=iron)
    m.board((px, py, 0.72), (px + 0.03, py + 0.62, 0.72), 0.035, 0.018, face=(1.0, 0.0, 0.0), uv=iron)
    m.board((px, py, 0.3), (px + 0.02, py + 0.42, 0.7), 0.025, 0.014, face=(1.0, 0.0, 0.0), uv=iron)
    hook = Vector((px + 0.025, py + 0.48, 0.72))
    m.cylinder(hook, hook - Vector((0.0, 0.0, 0.16)), 0.008, sides=4, uv=iron)
    pot, _ = lr.lathe([(0.0, 0.0), (0.11, 0.005), (0.16, 0.06), (0.17, 0.13), (0.15, 0.2), (0.155, 0.215),
                       (0.13, 0.215), (0.0, 0.17)], 12, 'trim', strip='Iron', seed=seed + 3)
    lp.place(pot, (hook.x, hook.y, 0.32))
    m.add(pot, smooth=True)
    # Its bail, up to the hook.
    m.add(lr.tube([(hook.x - 0.155 * math.cos(a), hook.y, 0.51 + 0.05 * math.sin(a))
                   for a in [math.pi * k / 6 for k in range(7)]], 0.006, sides=4, strip='Iron', caps=(False, False)))
    m.section('firebox')

    # The foundation: low fieldstone walls round the footprint, broken in places, a step at the door gap.
    runs = [
        # start, end, set in, height, gaps (from, to along the wall, remaining height)
        ((-HX, -HY), (HX, -HY), 0.0, 0.46, [(4.3, 5.35, 0.0), (1.2, 2.0, 0.17)]),
        ((HX, -HY), (HX, HY), FT, 0.42, [(3.2, 4.2, 0.2)]),
        ((HX, HY), (-HX, HY), 0.0, 0.44, [(1.5, 2.7, 0.12)]),
        ((-HX, HY), (-HX, -HY), FT, 0.4, [(HY - FT - CY, HY - FT + CY, None)]),   # the chimney stands in this one
    ]
    for p0, p1, inset, height, gaps in runs:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        cuts = [0.0, length]
        for a, b, _ in gaps:
            cuts += [a, b]
        cuts = sorted(set(min(max(c, 0.0), length) for c in cuts))
        for a, b in zip(cuts, cuts[1:]):
            gap = next((g for g in gaps if g[0] <= (a + b) * 0.5 <= g[1]), None)
            h = height + rng.uniform(-0.05, 0.04) if gap is None else gap[2]
            if h is None or b - a < 0.05:
                continue
            m.box((b - a, FT, h + 0.15), at=(inset + (a + b) * 0.5, FT * 0.5, (h - 0.15) * 0.5),
                  uv=Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space=space, cuts=max(0, int((b - a) / 1.0)))
            if h > 0.25:
                m.hull((b - a, FT, h), at=(inset + (a + b) * 0.5, FT * 0.5, h * 0.5), space=space)
        for a, b, rest in gaps:
            if rest is not None and rest < 0.3:
                # The stones that fell out of the gap lie in front of it and inside.
                for k in range(3):
                    x = inset + rng.uniform(a, b)
                    y = rng.choice((-0.35, FT + 0.35)) + rng.uniform(-0.15, 0.15)
                    world = space.world((x, y, 0.0))
                    rubble(m, (world.x, world.y, 0.0), (rng.uniform(0.22, 0.36), rng.uniform(0.18, 0.28),
                                                        rng.uniform(0.12, 0.2)), rng.randrange(1 << 20))
    m.add(lr.stone_block((1.1, 0.42, 0.16), (-HX + 4.82, -HY - 0.26, 0.04), (0.0, 0.0, 2.0), seed=seed + 4,
                         rough=0.008, bevel=0.025))      # the door step, in front of the gap
    m.section('foundation')

    # Charred sills on the foundation, burnt through in places; floor joist stubs; two corner posts and a fallen beam.
    sills = [((-HX + 0.2, -HY + 0.21, 0.56), (-0.6, -HY + 0.21, 0.56), (0.0, 0.6)),
             ((2.2, -HY + 0.21, 0.55), (HX - 0.2, -HY + 0.21, 0.56), (0.5, 0.0)),
             ((HX - 0.21, -HY + 0.3, 0.52), (HX - 0.21, 0.4, 0.53), (0.0, 0.7)),
             ((HX - 0.2, HY - 0.21, 0.55), (0.6, HY - 0.21, 0.55), (0.0, 0.45)),
             ((-1.1, HY - 0.21, 0.53), (-HX + 0.45, HY - 0.21, 0.53), (0.6, 0.0))]
    for k, (p0, p1, burn) in enumerate(sills):
        charred(m, p0, p1, 0.2, 0.2, seed + 10 + k, burn=burn)
    for k, (p0, p1) in enumerate((((1.0, -HY + 0.3, 0.42), (1.3, 0.2, 0.18)), ((2.6, -HY + 0.3, 0.46), (2.4, 0.9, 0.3)),
                                  ((-1.6, HY - 0.3, 0.44), (-1.2, 0.3, 0.12)))):
        charred(m, p0, p1, 0.18, 0.08, seed + 20 + k, face=(0.0, 0.0, 1.0), burn=(0.0, 0.9))
    for k, (x, y, h) in enumerate(((HX - 0.2, -HY + 0.2, 1.95), (-HX + 0.45, HY - 0.22, 1.35),
                                   (HX - 0.2, HY - 0.2, 0.75))):
        charred(m, (x, y, 0.4), (x, y, h), 0.2, 0.2, seed + 30 + k, face=(0.0, -1.0, 0.0), burn=(0.0, 0.55), jag=0.12)
        m.hull((0.22, 0.22, h - 0.4), at=(x, y, (h + 0.4) * 0.5))
    lean0, lean1 = Vector((-0.4, 0.75, 0.0)), Vector((CX1 + 0.03, 0.45, 1.85))
    charred(m, lean0, lean1, 0.22, 0.18, seed + 40, face=(0.0, -1.0, 0.3), burn=(0.4, 0.15))
    # The roof came down inside: a heap of charred rafters and boards in the back right corner.
    heap = Vector((2.3, 1.15, 0.0))
    for k in range(7):
        a = math.radians(rng.uniform(0.0, 180.0))
        d = Vector((math.cos(a), math.sin(a), 0.0))
        lng = rng.uniform(1.0, 2.0)
        c = heap + Vector((rng.uniform(-0.45, 0.45), rng.uniform(-0.35, 0.35), 0.06 + 0.08 * (k % 3)))
        charred(m, c - d * lng * 0.5, c + d * lng * 0.5 + Vector((0.0, 0.0, rng.uniform(-0.04, 0.14))),
                rng.uniform(0.12, 0.2), rng.uniform(0.05, 0.11), seed + 60 + k, face=(0.0, 0.0, 1.0),
                burn=(rng.uniform(0.0, 0.3), rng.uniform(0.15, 0.4)), splinters=1)
    m.hull((1.6, 1.3, 0.3), at=(heap.x, heap.y, 0.15), rot=(0.0, 0.0, 20.0))
    # The cast-iron stove the fire couldn't take, its pipe fallen beside it.
    iron = Trim('H3', fit=True)
    sv = Vector((0.5, 1.75, 0.0))
    m.box((0.62, 0.46, 0.44), at=(sv.x, sv.y, 0.36), uv=iron, bevel=0.02)
    m.box((0.68, 0.52, 0.04), at=(sv.x, sv.y, 0.6), uv=iron)
    for dx in (-0.26, 0.26):
        for dy in (-0.18, 0.18):
            m.box((0.05, 0.05, 0.16), at=(sv.x + dx, sv.y + dy, 0.08), uv=iron)
    m.box((0.3, 0.02, 0.2), at=(sv.x - 0.08, sv.y - 0.235, 0.36), uv=Trim('H4', fit=True))     # the dark fire door
    m.cylinder((sv.x + 0.15, sv.y + 0.05, 0.62), (sv.x + 0.15, sv.y + 0.05, 0.86), 0.07, sides=8, uv=iron)
    m.cylinder((sv.x + 0.4, sv.y - 0.45, 0.07), (sv.x + 1.3, sv.y - 0.7, 0.07), 0.07, sides=8, uv=iron)
    m.hull((0.7, 0.55, 0.64), at=(sv.x, sv.y, 0.32))
    m.section('timbers')

    # Rubble inside: stones from the wall gaps, a heap of fallen flue stones by the stack.
    for k in range(9):
        x, y = rng.uniform(-HX + 0.7, HX - 0.6), rng.uniform(-HY + 0.6, HY - 0.6)
        rubble(m, (x, y, 0.0), (rng.uniform(0.2, 0.34), rng.uniform(0.16, 0.26), rng.uniform(0.1, 0.18)),
               rng.randrange(1 << 20))
    for k in range(5):
        a = rng.uniform(-1.2, 1.2)
        rubble(m, (CX0 - rng.uniform(0.25, 0.8), a, 0.0), (rng.uniform(0.24, 0.38), rng.uniform(0.18, 0.28),
                                                            rng.uniform(0.12, 0.2)), rng.randrange(1 << 20))
    m.section('rubble')

    # Collision: the stack (its firebox too small to enter), the hearth and the leaning beam.
    m.hull((CX1 - CX0, 2 * CY, BASE_TOP), at=((CX0 + CX1) * 0.5, 0.0, BASE_TOP * 0.5))
    m.hull((fx + 0.1, fy + 0.1, TOP + 0.15 - BASE_TOP), at=(flue_x, 0.0, (TOP + 0.15 + BASE_TOP) * 0.5))
    d = (lean1 - lean0).normalized()
    side = d.cross(Vector((0.0, 0.0, 1.0))).normalized() * 0.12
    m.hull_points([lean0 + side, lean0 - side, lean1 + side, lean1 - side,
                   lean0 + side + Vector((0.0, 0.0, 0.2)), lean1 - side + Vector((0.0, 0.0, 0.2))])
    return m.finish(view=(1.0, -1.4, 0.5), fit=0.85, fallback=100.0, ao_distance=0.8, out=lr.preview_path(name))


# --- Ruin_LanternHouse ---

def ruin_lantern_house(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    HX, HY = 2.05, 2.5
    T = 0.5
    EAVE, PEAK = 2.35, 3.55
    stone = Trim('D', world=True, v=0.31)
    door = Opening('door', 1.55, 0.0, 1.02, 2.0)
    window = Opening('window', 1.6, 1.05, 0.58, 0.66)
    niches = [Opening('niche', x, 1.25, 0.34, 0.44) for x in (0.85, 1.85, 2.85)]
    # Front gable: its right end (seen from outside) has fallen down to about 1.8 m, past the door.
    front_top = gable_top(rng, 2 * HX, EAVE, PEAK, 0.07, fall=(0.76, 1.0, 1.8), steps=7)
    stone_wall(m, (-HX, -HY), (HX, -HY), T, front_top, [door], uv=stone, slices=1.4)
    # Back gable: whole but for its ragged edge; a vent slot near the peak; three lantern niches inside.
    back_top = gable_top(rng, 2 * HX, EAVE - 0.05, PEAK - 0.05, 0.06, steps=6)
    vent = Opening('window', HX - 0.09, PEAK - 0.95, 0.18, 0.45)
    outer_t, inner_t = 0.3, T - 0.3
    space = stone_wall(m, (HX, HY), (-HX, HY), outer_t, back_top, [vent], uv=stone, slices=1.4)
    inner = kit.Space(space.matrix @ Matrix.Translation((0.0, outer_t, 0.0)))
    outline = kit.wall_outline(2 * HX, back_top[0][1], [], None)[:-2] + back_top
    m.panel(outline, kit.holes([vent] + niches), inner_t, stone, space=inner, around=niches + [vent], slices=1.4)
    for o in niches:
        # A stone sill and a lintel slab for each niche, and soot over it.
        m.box((o.w + 0.1, 0.12, 0.05), at=(o.x + o.w * 0.5, outer_t + inner_t + 0.03, o.z - 0.02),
              uv=Trim('D', v=0.1), space=space)
        m.box((o.w + 0.16, inner_t + 0.04, 0.08), at=(o.x + o.w * 0.5, outer_t + inner_t * 0.5, o.z + o.h + 0.04),
              uv=Trim('C', lane=2), space=space)
    m.hull_points([(0.0, outer_t, 0.0), (2 * HX, outer_t, 0.0), (0.0, T, 0.0), (2 * HX, T, 0.0),
                   (0.0, outer_t, EAVE - 0.2), (2 * HX, outer_t, EAVE - 0.2), (0.0, T, EAVE - 0.2),
                   (2 * HX, T, EAVE - 0.2)], space=space)
    # Side walls fit between the gables: the left one breached, the right one with the window gap.
    left_top = ragged(rng, 2 * HY - 2 * T, EAVE - 0.05, 0.12, lo_at=0.42, drop=0.75, steps=6)
    stone_wall(m, (-HX, HY), (-HX, -HY), T, left_top, [], inset=T, uv=stone, slices=1.4)
    right_top = ragged(rng, 2 * HY - 2 * T, EAVE, 0.1, lo_at=0.85, drop=0.35, steps=6)
    rs = stone_wall(m, (HX, -HY), (HX, HY), T, right_top, [window], inset=T, uv=stone, slices=1.4)
    m.section('walls')

    # The door's timber lintel (whole, its right end on the fallen gable's rubble), a worn threshold; the window's sill
    # and lintel.
    front = kit.wall_space((-HX, -HY), (HX, -HY), 0.0)
    m.board((door.x - 0.32, T * 0.5, door.h + 0.12), (door.x + door.w + 0.28, T * 0.5, door.h + 0.09), 0.24, T + 0.04,
            uv=Trim('C', lane='each'), space=front)
    m.box((door.w + 0.1, T + 0.1, 0.1), at=(door.x + door.w * 0.5, T * 0.5, 0.02), uv=Trim('D', v=0.05), space=front)
    m.box((window.w + 0.24, T + 0.14, 0.07), at=(window.x + window.w * 0.5, T * 0.5 - 0.02, window.z - 0.035),
          uv=Trim('D', v=0.2), space=rs)
    m.board((window.x - 0.2, T * 0.5, window.z + window.h + 0.09), (window.x + window.w + 0.2, T * 0.5,
            window.z + window.h + 0.09), 0.18, T + 0.02, uv=Trim('C', lane='each'), space=rs)
    m.section('openings')

    # Inside: a stone bench on the left, a lantern shelf on iron brackets on the right with hooks, one old lantern
    # still hanging (its glass dark, on the trim's window glass).
    bx = -HX + T + 0.22
    m.add(lr.stone_block((0.44, 1.7, 0.1), (bx, 0.4, 0.46), (0.0, 0.0, 0.0), seed=seed + 1, rough=0.006, bevel=0.02))
    for y in (-0.25, 1.05):
        m.add(lr.stone_block((0.36, 0.3, 0.42), (bx, y, 0.2), (0.0, 0.0, rng.uniform(-6, 6)),
                             seed=seed + 2 + int(y * 10), rough=0.01, bevel=0.03))
    m.hull((0.46, 1.72, 0.51), at=(bx, 0.4, 0.255))
    sx = HX - T - 0.12
    m.board((sx, -1.3, 1.62), (sx, 0.6, 1.62), 0.24, 0.04, face=(0.0, 0.0, 1.0), uv='A')
    iron = Trim('H3', fit=True)
    for y in (-1.1, 0.4):
        m.board((sx + 0.1, y, 1.56), (sx - 0.1, y, 1.56), 0.03, 0.012, face=(0.0, 1.0, 0.0), uv=iron)
        m.board((sx + 0.11, y, 1.36), (sx - 0.09, y, 1.56), 0.025, 0.012, face=(0.0, 1.0, 0.0), uv=iron)
    for k, y in enumerate((-1.0, -0.6, -0.2, 0.2)):
        m.board((sx + 0.02, y, 1.6), (sx - 0.06, y, 1.6), 0.012, 0.012, face=(0.0, 1.0, 0.0), uv=iron)
        m.board((sx - 0.06, y, 1.6), (sx - 0.07, y, 1.5), 0.012, 0.012, face=(1.0, 0.0, 0.0), uv=iron)
    old = Vector((sx - 0.07, -0.6, 1.3))
    m.box((0.16, 0.16, 0.2), at=(old.x, old.y, old.z), uv=Trim('H4', fit=True))
    for dx in (-0.08, 0.08):
        for dy in (-0.08, 0.08):
            m.box((0.02, 0.02, 0.23), at=(old.x + dx, old.y + dy, old.z), uv=iron)
    for z in (old.z - 0.11, old.z + 0.11):
        m.box((0.2, 0.2, 0.025), at=(old.x, old.y, z), uv=iron)
    m.cylinder((old.x, old.y, old.z + 0.12), (old.x, old.y, old.z + 0.19), 0.1, 0.03, sides=4, uv=iron, phase=45.0)
    m.cylinder((old.x, old.y, old.z + 0.19), (old.x, old.y, 1.5), 0.006, sides=4, uv=iron)
    m.section('inside')

    # The ridge beam, fallen from the back gable's peak to the floor; a purlin and two rafters on the floor; flagstones
    # and rubble.
    m.board((0.1, HY - 0.32, PEAK - 0.32), (0.55, -1.3, 0.12), 0.24, 0.2, face=(1.0, 0.0, 0.25),
            uv=Trim('C', lane='each'))
    m.board((-1.1, -1.6, 0.09), (0.9, 1.1, 0.1), 0.18, 0.16, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    m.board((-0.9, 1.6, 0.06), (-0.2, -0.4, 0.07), 0.12, 0.1, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    m.board((HX - 0.3, -1.95, 0.05), (HX + 0.6, -0.4, 0.06), 0.12, 0.1, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    for k in range(5):
        x, y = rng.uniform(-1.2, 1.2), rng.uniform(-1.9, 1.7)
        m.add(lr.stone_block((rng.uniform(0.45, 0.7), rng.uniform(0.4, 0.6), 0.06), (x, y, 0.0),
                             (0.0, 0.0, rng.uniform(0, 180)), seed=rng.randrange(1 << 20), rough=0.004, bevel=0.0))
    for k in range(10):
        corner = rng.choice(((1.2, -1.85), (-1.2, -1.7), (1.3, 1.5), (HX + 0.7, -HY - 0.3), (HX + 0.4, -HY - 0.9)))
        rubble(m, (corner[0] + rng.uniform(-0.45, 0.45), corner[1] + rng.uniform(-0.35, 0.35), 0.0),
               (rng.uniform(0.2, 0.36), rng.uniform(0.16, 0.28), rng.uniform(0.1, 0.18)), rng.randrange(1 << 20))
    m.section('timbers and rubble')
    return m.finish(view=(-1.0, -1.5, 0.75), fit=0.85, fallback=100.0, ao_distance=0.8, out=lr.preview_path(name))


# --- Ruin_Springhouse ---

def ruin_springhouse(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    HX, HY = 1.7, 1.9
    T = 0.45
    WATER = 0.24
    stone = Trim('D', world=True, v=0.4)
    DOOR = 0.45            # half the doorway: the low walls are simply open there
    lip_x = HX - 0.7       # where the spring leaves, through the front wall right of the door
    stone_wall(m, (-HX, -HY), (-DOOR, -HY), T, ragged(rng, HX - DOOR, 1.3, 0.09, lo_at=0.3, drop=0.25, steps=5), [],
               uv=stone)
    stone_wall(m, (DOOR, -HY), (HX, -HY), T, ragged(rng, HX - DOOR, 1.36, 0.08, steps=5),
               [Opening('window', lip_x - DOOR - 0.13, 0.19, 0.26, 0.17)], uv=stone)
    stone_wall(m, (HX, HY), (-HX, HY), T, ragged(rng, 2 * HX, 1.4, 0.06), [], uv=stone)
    stone_wall(m, (-HX, HY), (-HX, -HY), T, ragged(rng, 2 * HY - 2 * T, 1.28, 0.1, lo_at=0.65, drop=0.3), [], inset=T,
               uv=stone)
    stone_wall(m, (HX, -HY), (HX, HY), T, ragged(rng, 2 * HY - 2 * T, 1.36, 0.08), [], inset=T, uv=stone)
    m.section('walls')

    # The basin: a flagstone floor, a stone curb across the doorway that holds the water in, the spout the spring runs
    # in by, and the lip it leaves by (through the front wall, right of the door).
    IX, IY = HX - T, HY - T
    m.box((2 * IX + 0.02, 2 * IY + 0.02, 0.06), at=(0.0, 0.0, -0.01), uv=Trim('D', world=True, v=0.6), cuts=2)
    m.hull((2 * IX, 2 * IY, 0.04), at=(0.0, 0.0, 0.0))
    m.add(lr.stone_block((2 * DOOR + 0.1, T + 0.04, 0.32), (0.0, -HY + T * 0.5, 0.14), seed=seed + 1, rough=0.006,
                         bevel=0.03))
    m.hull((2 * DOOR + 0.1, T, 0.31), at=(0.0, -HY + T * 0.5, 0.155))
    spout = lr.stone_block((0.26, 0.42, 0.12), (0.35, IY - 0.08, 0.56), (0.0, 0.0, 0.0), seed=seed + 2, rough=0.006,
                           bevel=0.02)
    m.add(spout)
    m.box((0.1, 0.38, 0.03), at=(0.35, IY - 0.1, 0.625), uv=Trim('H4', fit=True))     # its wet groove
    m.add(lr.stone_block((0.32, T + 0.5, 0.1), (lip_x, -HY + T * 0.5 - 0.22, WATER - 0.07), seed=seed + 3, rough=0.006,
                         bevel=0.02))
    m.box((0.12, T + 0.46, 0.02), at=(lip_x, -HY + T * 0.5 - 0.22, WATER - 0.012), uv=Trim('H4', fit=True))
    # A ledge for the crocks along the left wall, two crocks on it, one fallen in.
    m.add(lr.stone_block((0.34, 2 * IY - 0.3, 0.08), (-IX + 0.17, 0.1, 0.48), seed=seed + 4, rough=0.004, bevel=0.015))
    for k, (y, h, r) in enumerate(((0.55, 0.3, 0.12), (-0.25, 0.24, 0.1))):
        crock, _ = lr.lathe([(0.0, 0.0), (r * 0.85, 0.0), (r, h * 0.2), (r * 1.02, h * 0.7), (r * 0.78, h * 0.92),
                             (r * 0.62, h), (r * 0.66, h * 1.04), (r * 0.5, h * 1.04), (0.0, h * 0.95)], 12, 'trim',
                            strip='Plaster', seed=seed + 5 + k)
        lp.place(crock, (-IX + 0.17, y, 0.52))
        m.add(crock, smooth=True)
    crock, _ = lr.lathe([(0.0, 0.0), (0.09, 0.0), (0.11, 0.06), (0.11, 0.2), (0.08, 0.25), (0.0, 0.24)], 12, 'trim',
                        strip='Plaster', seed=seed + 8)
    lp.place(crock, (0.0, 0.0, 0.0), (82.0, 0.0, 30.0))
    lp.place(crock, (-0.4, -0.7, 0.12))
    m.add(crock, smooth=True)
    m.section('basin')

    # A rafter that fell across the walls, and stones fallen out of the low places.
    m.board((-HX - 0.3, -0.6, 1.24), (HX + 0.25, 0.5, 1.3), 0.14, 0.12, face=(0.0, -0.2, 1.0),
            uv=Trim('C', lane='each'))
    for k in range(8):
        side = rng.choice((-1.0, 1.0))
        x = side * (HX + rng.uniform(0.25, 0.7)) if k % 2 else rng.uniform(-HX, HX)
        y = rng.uniform(-HY, HY) if k % 2 else side * (HY + rng.uniform(0.3, 0.7))
        if y < -HY and abs(x) < 1.0:
            continue
        rubble(m, (x, y, 0.0), (rng.uniform(0.2, 0.34), rng.uniform(0.16, 0.26), rng.uniform(0.1, 0.18)),
               rng.randrange(1 << 20))
    m.section('rubble')
    m.socket('Water', (0.0, 0.0, WATER))
    obj = m.finish(view=(-1.0, -1.45, 0.95), fit=0.9, fallback=100.0, ao_distance=0.7, preview=False)
    # Damp: the walls darken toward the water inside the basin.
    lr.darken(obj, lambda p, n: (1.0 if abs(p.x) < IX + 0.01 and abs(p.y) < IY + 0.01 else 0.0) *
              max(0.0, 1.0 - max(p.z - 0.05, 0.0) / 0.35), strength=0.35)
    lr.preview([obj], name, view=(-1.0, -1.45, 0.95), fit=0.9)
    return obj


# --- Ruin_LiveryFooting ---

def ruin_livery(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    HX, HY = 4.0, 6.0
    FT, FH = 0.45, 0.42
    runs = [
        ((-HX, -HY), (HX, -HY), 0.0, [(2.4, 5.6, None)]),             # the big doors' gap
        ((HX, -HY), (HX, HY), FT, [(4.0, 5.1, None), (8.6, 9.6, 0.15)]),
        ((HX, HY), (-HX, HY), 0.0, [(2.8, 4.1, 0.2)]),
        ((-HX, HY), (-HX, -HY), FT, [(6.0, 7.4, 0.1)]),
    ]
    for p0, p1, inset, gaps in runs:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        cuts = sorted({0.0, length} | {min(max(c, 0.0), length) for g in gaps for c in g[:2]})
        for a, b in zip(cuts, cuts[1:]):
            gap = next((g for g in gaps if g[0] <= (a + b) * 0.5 <= g[1]), None)
            h = FH + rng.uniform(-0.05, 0.05) if gap is None else gap[2]
            if h is None or b - a < 0.05:
                continue
            m.box((b - a, FT, h + 0.15), at=(inset + (a + b) * 0.5, FT * 0.5, (h - 0.15) * 0.5),
                  uv=Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space=space, cuts=max(0, int((b - a) / 1.0)))
            if h > 0.25:
                m.hull((b - a, FT, h), at=(inset + (a + b) * 0.5, FT * 0.5, h * 0.5), space=space)
    m.section('footing')

    # Charred sills on the footing, mostly burnt away.
    for k, (p0, p1, burn) in enumerate((((-HX + 0.22, -HY + 0.4, FH + 0.1), (-HX + 0.22, -1.0, FH + 0.1), (0.3, 0.8)),
                                        ((-HX + 0.22, 1.4, FH + 0.1), (-HX + 0.22, HY - 0.3, FH + 0.1), (0.6, 0.0)),
                                        ((HX - 0.22, -2.4, FH + 0.1), (HX - 0.22, 2.0, FH + 0.1), (0.7, 0.5)),
                                        ((-2.6, HY - 0.22, FH + 0.1), (1.2, HY - 0.22, FH + 0.1), (0.4, 0.9)))):
        charred(m, p0, p1, 0.2, 0.2, seed + 10 + k, burn=burn)
    # Posts along the sides and the door posts, burnt to all heights (none over 2.8 m: the lookout sees over it).
    posts = []
    for side in (-1.0, 1.0):
        for y in (-HY + 0.25, -3.6, -1.2, 1.2, 3.6, HY - 0.25):
            posts.append((side * (HX - 0.22), y))
    posts += [(-1.65, -HY + 0.22), (1.65, -HY + 0.22), (0.0, -1.2), (0.0, 3.6)]
    heights = [2.75, 0.6, 1.8, 0.0, 1.2, 2.2, 1.5, 0.45, 2.5, 0.9, 0.0, 1.9, 2.8, 2.35, 0.7, 1.1]
    for k, ((x, y), h) in enumerate(zip(posts, heights)):
        if h <= 0.0:
            continue
        z0 = FH if abs(abs(x) - (HX - 0.22)) < 0.01 or abs(y + HY - 0.22) < 0.01 else 0.0
        top = z0 + h * rng.uniform(0.9, 1.0)
        charred(m, (x, y, z0 - 0.05), (x, y, min(top, 2.8)), 0.22, 0.22, seed + 30 + k, face=(0.0, -1.0, 0.0),
                burn=(0.0, min(0.6, h * 0.5)), jag=0.14)
        if top - z0 > 0.6:
            m.hull((0.24, 0.24, min(top, 2.8) - z0), at=(x, y, (min(top, 2.8) + z0) * 0.5))
    m.section('posts')

    # The stalls: low partitions of charred planks between posts, partly burnt away.
    for k, y in enumerate((-2.4, 0.0, 2.4)):
        x0, x1 = -HX + 0.3, -HX + 2.6
        for j, z in enumerate((0.3, 0.55, 0.8)):
            reach = x1 - rng.uniform(0.0, 1.6) * (j / 2.0 + 0.3)
            charred(m, (x0, y, z), (reach, y, z + rng.uniform(-0.03, 0.03)), 0.22, 0.05, seed + 60 + k * 3 + j,
                    face=(0.0, -1.0, 0.0), burn=(0.0, 0.35))
        charred(m, (x1, y, 0.0), (x1, y, rng.uniform(0.7, 1.15)), 0.14, 0.14, seed + 70 + k, face=(0.0, -1.0, 0.0),
                burn=(0.0, 0.3))
        m.hull((x1 - x0, 0.1, 0.8), at=((x0 + x1) * 0.5, y, 0.4))
    # Fallen beams and rafters, one leaning on a post; crumpled roof tin; an iron tyre ring against the footing.
    for k, (p0, p1) in enumerate((((-1.8, -3.9, 0.11), (2.4, -0.4, 0.13)), ((0.6, 1.2, 0.1), (3.2, 4.4, 0.3)),
                                  ((-3.2, 4.4, 0.08), (-0.4, 2.0, 0.09)), ((1.3, -3.4, 0.0), (3.6, -1.25, 1.55)))):
        charred(m, p0, p1, 0.22, 0.2, seed + 80 + k, face=(0.0, 0.0, 1.0), burn=(0.35, 0.5))
    # Roof tin that came down with the fire: sheets buckled over the debris, in three bent lengths each.
    for k, (x, y, yaw, bends) in enumerate(((1.2, 2.4, 20.0, (8.0, -14.0, 10.0)),
                                            (-1.6, -4.3, -35.0, (-5.0, 12.0, -9.0)),
                                            (2.5, -2.0, 70.0, (12.0, -6.0, 16.0)))):
        frame = kit.place((x, y, 0.06), (0.0, 0.0, yaw))
        at, angle = Vector((-0.95, 0.0, 0.0)), 0.0
        u0 = rng.randrange(8) * 0.8
        for j, bend in enumerate(bends):
            angle += bend
            d = Vector((math.cos(math.radians(angle)), 0.0, math.sin(math.radians(angle))))
            seg = 0.66
            mid = at + d * seg * 0.5
            m.box((seg, 0.8, 0.012), at=(mid.x, mid.y, max(mid.z, 0.0) + 0.006), rot=(0.0, -angle, 0.0),
                  uv=Trim('F', u=u0 + 0.4 + j * seg), space=kit.Space(frame), ao_floor=0.0)
            at = at + d * seg
            at.z = max(at.z, 0.0)
    tyre, _ = lr.lathe([(0.52, -0.03), (0.56, -0.03), (0.56, 0.03), (0.52, 0.03)], 20, 'trim', strip='Iron',
                       closed=True, seed=seed + 90)
    lp.place(tyre, (0.0, 0.0, 0.0), (72.0, 0.0, 0.0))
    lp.place(tyre, (HX + 0.15, 1.9, 0.52))
    m.add(tyre)
    for k in range(8):
        x, y = rng.uniform(-HX + 0.6, HX - 0.6), rng.uniform(-HY + 0.6, HY - 0.6)
        rubble(m, (x, y, 0.0), (rng.uniform(0.2, 0.34), rng.uniform(0.16, 0.26), rng.uniform(0.1, 0.16)),
               rng.randrange(1 << 20))
    m.section('stalls and debris')
    obj = m.finish(fallback=100.0, ao_distance=0.8, preview=False)
    lr.soot_strip(obj, 'F', 0.6)            # the roof tin came down through the fire
    lr.preview([obj], name, view=(-1.0, -1.3, 0.85), fit=0.85)
    return obj


# --- Ruin_Derrick ---

def ruin_derrick(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    beam = Trim('C', lane='each')
    iron = Trim('H3', fit=True)
    # The mast's foot: a cross of sills with an iron pivot, where the mast still sits, tipped over.
    FOOT = Vector((-4.2, 0.2, 0.0))
    for a in (0.0, 90.0):
        d = Vector((math.cos(math.radians(a + 12.0)), math.sin(math.radians(a + 12.0)), 0.0))
        lift = Vector((0.0, 0.0, 0.15 + (0.3 if a else 0.0)))
        m.board(FOOT - d * 1.3 + lift, FOOT + d * 1.3 + lift, 0.3, 0.3, face=(0.0, 0.0, 1.0), uv=beam)
    m.cylinder(FOOT + Vector((0.0, 0.0, 0.6)), FOOT + Vector((0.0, 0.0, 0.78)), 0.14, sides=8, uv=iron)
    m.hull((2.7, 2.7, 0.62), at=(FOOT.x, FOOT.y, 0.31), rot=(0.0, 0.0, 12.0))
    # The mast: the lower half still on its pivot, lying along +X; it broke at 5.4 m; the top half lies askew.
    m0, m1 = FOOT + Vector((0.2, 0.0, 0.75)), Vector((1.2, 0.85, 0.22))
    m.board(m0, m1, 0.36, 0.36, face=(0.0, 0.0, 1.0), uv=beam, cuts=4)
    for t in (0.1, 0.5):   # iron bands
        c = m0.lerp(m1, t)
        m.box((0.08, 0.4, 0.4), matrix=kit.toward(m0, m1) @ Matrix.Translation(((m1 - m0).length * t, 0.0, 0.0)),
              uv=iron)
    splinter = (m1 - m0).normalized()
    for k in range(4):
        off = Vector((0.0, rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12)))
        m.board(m1 - splinter * 0.05 + off, m1 + splinter * rng.uniform(0.25, 0.5) + off * 1.4, 0.07, 0.06,
                face=(0.0, 0.0, 1.0), uv='C')
    t0, t1 = Vector((1.8, 1.55, 0.2)), Vector((5.6, 2.9, 0.19))
    m.board(t0, t1, 0.32, 0.32, face=(0.0, 0.0, 1.0), uv=beam, cuts=3)
    m.cylinder(t1 - (t1 - t0).normalized() * 0.25, t1 + (t1 - t0).normalized() * 0.02, 0.23, sides=8,
               uv=iron)
    for k in range(3):
        off = Vector((0.0, rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1)))
        d = (t1 - t0).normalized()
        m.board(t0 + d * 0.05 + off, t0 - d * rng.uniform(0.2, 0.42) + off * 1.3, 0.06, 0.05, face=(0.0, 0.0, 1.0),
                uv='C')
    for p0, p1 in ((m0, m1), (t0, t1)):
        d = (p1 - p0).normalized()
        side = d.cross(Vector((0.0, 0.0, 1.0))).normalized() * 0.2
        m.hull_points([p0 + side, p0 - side, p1 + side, p1 - side,
                       p0 + side + Vector((0.0, 0.0, 0.2)), p1 - side + Vector((0.0, 0.0, 0.2)),
                       p0 - side - Vector((0.0, 0.0, 0.2)), p1 + side - Vector((0.0, 0.0, 0.2))])
    m.section('mast')
    # The boom: whole, lying across in front, its heel by the foot and its sheave block and hook at the tip.
    b0, b1 = FOOT + Vector((0.5, -0.5, 0.42)), Vector((1.6, -3.9, 0.17))
    m.board(b0, b1, 0.28, 0.26, face=(0.0, 0.0, 1.0), uv=beam, cuts=3)
    d = (b1 - b0).normalized()
    side = d.cross(Vector((0.0, 0.0, 1.0))).normalized()
    m.box((0.5, 0.34, 0.34), matrix=kit.toward(b0, b1) @ Matrix.Translation((0.25, 0.0, 0.0)), uv=iron)
    block = b1 + d * 0.35 + side * 0.1
    m.box((0.36, 0.14, 0.5), at=(block.x, block.y, 0.25), rot=(0.0, 85.0, math.degrees(math.atan2(d.y, d.x))),
          uv=beam)
    for s in (-1.0, 1.0):
        m.box((0.4, 0.02, 0.54), at=(block.x + side.x * s * 0.08, block.y + side.y * s * 0.08, 0.26),
              rot=(0.0, 85.0, math.degrees(math.atan2(d.y, d.x))), uv=iron)
    hook_at = block + d * 0.5
    hook = lr.tube([hook_at + Vector((0.0, 0.0, 0.06)), hook_at + d * 0.12 + Vector((0.0, 0.0, 0.05)),
                    hook_at + d * 0.2 + side * 0.08 + Vector((0.0, 0.0, 0.05)), hook_at + d * 0.14 + side * 0.16 +
                    Vector((0.0, 0.0, 0.05)), hook_at + d * 0.06 + side * 0.12 + Vector((0.0, 0.0, 0.05))], 0.025,
                   sides=6, strip='Iron')
    m.add(hook)
    m.hull_points([b0 + side * 0.16, b0 - side * 0.16, b1 + side * 0.16, b1 - side * 0.16,
                   b0 + side * 0.16 + Vector((0.0, 0.0, 0.25)), b1 - side * 0.16 + Vector((0.0, 0.0, 0.2)),
                   b1 + side * 0.16 - Vector((0.0, 0.0, 0.15))])
    m.section('boom')
    # The bull wheel the derrick swung by: fallen off the foot, lying flat and tilted on its rim.
    bull = lr.wheel(1.05, 0.12, 8, seed + 5, hub=0.2, felloe=0.12, tyre=0.012, wood='trim', burnt=0.0)
    lp.place(bull, (0.0, 0.0, 0.0), (6.0, -4.0, 20.0))
    lp.place(bull, (-5.6, -2.3, 0.2))
    m.add(bull)
    m.hull_points([(-5.6 + 1.08 * math.cos(a), -2.3 + 1.08 * math.sin(a), z) for a in
                   (2.0 * math.pi * k / 10 for k in range(10)) for z in (0.0, 0.32)])
    # The hand winch on its skid: two iron side frames, a rope drum, a gear and two cranks.
    WX, WY = -6.6, 1.3
    for y in (-0.25, 0.25):
        m.board((WX - 0.9, WY + y * 2.2, 0.08), (WX + 0.9, WY + y * 2.2, 0.08), 0.16, 0.16, face=(0.0, 0.0, 1.0),
                uv=beam)
    for y in (-0.3, 0.3):
        m.box((0.7, 0.05, 0.12), at=(WX, WY + y, 0.22), uv=iron)
        for sx in (-0.28, 0.28):
            m.board((WX + sx, WY + y, 0.2), (WX + sx * 0.25, WY + y, 0.95), 0.08, 0.04, face=(0.0, -1.0, 0.0),
                    uv=iron)
    m.cylinder((WX, WY - 0.36, 0.6), (WX, WY + 0.36, 0.6), 0.15, sides=10, uv=Trim('A', lane=3), caps=(True, True))
    m.cylinder((WX + 0.2, WY - 0.45, 0.82), (WX + 0.2, WY + 0.45, 0.82), 0.022, sides=6, uv=iron)
    m.cylinder((WX, WY + 0.34, 0.6), (WX, WY + 0.4, 0.6), 0.34, sides=16, uv=iron)
    for s in (-1.0, 1.0):
        y = WY + s * 0.47
        m.board((WX + 0.2, y, 0.82), (WX + 0.2, y, 0.48), 0.05, 0.03, face=(0.0, -s, 0.0), uv=iron)
        m.cylinder((WX + 0.2, y, 0.5), (WX + 0.2, y + s * 0.22, 0.5), 0.024, sides=6, uv=Trim('C', lane=1))
    m.hull((1.9, 0.9, 1.0), at=(WX, WY, 0.5))
    # The stiff legs that braced the mast: one still propped across the fallen mast, its foot on its sill; the other
    # lying where it fell.
    l0, l1 = Vector((-0.9, 4.5, 0.14)), Vector((-2.75, 0.1, 0.96))
    m.board(l0, l1, 0.28, 0.28, face=(0.0, 0.0, 1.0), uv=beam, cuts=3)
    m.box((0.36, 0.32, 0.32), matrix=kit.toward(l1, l0) @ Matrix.Translation((0.2, 0.0, 0.0)), uv=iron)
    m.board((-2.1, 4.9, 0.12), (0.4, 4.0, 0.12), 0.24, 0.24, face=(0.0, 0.0, 1.0), uv=beam)
    d = (l1 - l0).normalized()
    side = d.cross(Vector((0.0, 0.0, 1.0))).normalized() * 0.16
    m.hull_points([l0 + side, l0 - side, l1 + side, l1 - side, l0 + side + Vector((0.0, 0.0, 0.2)),
                   l1 - side + Vector((0.0, 0.0, 0.2)), l0 - side - Vector((0.0, 0.0, 0.14))])
    g0, g1 = Vector((-3.4, 2.4, 0.14)), Vector((1.4, 4.4, 0.15))
    m.board(g0, g1, 0.28, 0.28, face=(0.0, 0.0, 1.0), uv=beam, cuts=3)
    m.box((0.36, 0.32, 0.32), matrix=kit.toward(g0, g1) @ Matrix.Translation((0.2, 0.0, 0.0)), uv=iron)
    # A coil of spare rope by the winch.
    coil = []
    for k in range(3 * 10 + 1):
        a = 2.0 * math.pi * k / 10
        r = 0.26 - 0.03 * (k // 10)
        coil.append((WX + 1.25 + r * math.cos(a), WY + 0.9 + r * math.sin(a), 0.03 + 0.035 * (k // 10) + 0.01 * k / 10))
    m.add(lr.tube(coil, 0.022, sides=5, lane=2, seed=seed + 7, caps=(False, False)), smooth=True)
    m.section('wheel and winch')
    # Rope: from the winch drum to the boom's heel sheave, slack on the ground; two guy ropes from the mast top.
    lines = [
        [(WX + 0.1, WY - 0.1, 0.72), (WX + 1.0, WY - 0.6, 0.05), (-4.6, -0.2, 0.04), (-3.4, -0.9, 0.08),
         (-1.2, -2.3, 0.06), (0.6, -3.4, 0.05), (block.x, block.y, 0.3)],
        [(t1.x, t1.y, 0.35), (6.5, 3.6, 0.04), (7.8, 2.9, 0.05), (8.6, 3.5, 0.04)],
        [(t1.x, t1.y, 0.3), (5.2, 4.3, 0.04), (4.6, 5.6, 0.04), (5.1, 6.3, 0.05)],
    ]
    for k, pts in enumerate(lines):
        dense = [Vector(pts[0])]
        for a, b in zip(pts, pts[1:]):
            a, b = Vector(a), Vector(b)
            steps = max(1, int((b - a).length / 0.5))
            for s in range(1, steps + 1):
                p = a.lerp(b, s / steps)
                if 0 < s < steps:
                    p += Vector((noise.noise(p * 1.7 + Vector((k, 0.0, 0.0))) * 0.12,
                                 noise.noise(p * 1.7 + Vector((0.0, k + 3.0, 0.0))) * 0.12, 0.0))
                dense.append(p)
        m.add(lr.tube(dense, 0.022, sides=6, lane=k % 4, seed=seed + 100 + k, wobble=0.002))
        end, before = dense[-1], dense[-2]
        if k:
            for part in lr.frayed(end, end - before, 0.022, seed + 110 + k, length=0.14):
                m.add(part)
    m.section('ropes')
    return m.finish(view=(-0.8, -1.5, 0.75), fit=0.8, fallback=100.0, ao_distance=0.8, out=lr.preview_path(name))


def dressed_block(name, seed, size, chips):
    """A squared quarry block: tool-dressed faces, its arrises chamfered and knocked, a corner or two broken off."""
    rng = random.Random(seed)
    part = lp.block(size, (0.0, 0.0, size[2] * 0.5 - 0.03), bevel=0.025)
    bm = bmesh.new()
    bm.from_mesh(part.data)
    for corner, depth in chips:
        n = Vector(corner).normalized()
        point = Vector((corner[0] * size[0] * 0.5, corner[1] * size[1] * 0.5, size[2] * 0.5 - 0.03 +
                        corner[2] * size[2] * 0.5)) - n * depth
        result = bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=point, plane_no=n,
                                        clear_outer=True)
        edges = [e for e in result['geom_cut'] if isinstance(e, bmesh.types.BMEdge)]
        if edges:
            bmesh.ops.edgeloop_fill(bm, edges=edges)
    bm.to_mesh(part.data)
    bm.free()
    lp.rough(part, 0.008, 4.0, seed)
    for v in part.data.vertices:
        v.co.z = max(v.co.z, -0.03)
    lt.assign(part, lr.material('granite'))
    lt.box_uv(part, 'RockGranite', seed=seed)
    obj = lp.join(name, [part])
    lp.hull_box(obj, (size[0] - 0.02, size[1] - 0.02, size[2] - 0.03), (0.0, 0.0, (size[2] - 0.03) * 0.5))
    lp.finish(obj, ao=0.4, nanite=False, smooth=40.0)
    obj['LODs'] = SMALL['LODs']
    lr.preview([obj], name, view=(-1.0, -1.5, 0.8), fit=1.0)
    return obj


# --- Burnt freight wagons ---

def wagon(m, seed, tipped=False):
    """A burnt freight wagon built level, its front toward -Y, without its wheels and tongue (the callers add them, as
    each lies differently): the running gear, the charred bed with its stake irons and, unless tipped, its bows.
    Returns the bed's floor height and half width and length."""
    BED_Z = 0.98
    HW, HL = 0.62, 1.78
    iron = Trim('H3', fit=True)
    # Running gear: axles, the bolsters (charred: the bed burnt on them), the reach and hounds (scorched).
    beam = Trim('C', lane='each')
    for y, r in ((-1.15, 0.45), (1.1, 0.6)):
        m.board((-0.82, y, r), (0.82, y, r), 0.12, 0.12, face=(0.0, 0.0, 1.0), uv=beam)
        lo, hi = r + 0.06, BED_Z - 0.105
        charred(m, (-0.72, y, (lo + hi) * 0.5), (0.72, y, (lo + hi) * 0.5), hi - lo, 0.14, seed + 2 + int(y * 10),
                face=(0.0, -1.0, 0.0))
    m.board((0.0, -1.5, 0.54), (0.0, 1.6, 0.69), 0.12, 0.12, face=(0.0, 0.0, 1.0), uv=beam)
    for s in (-1.0, 1.0):
        m.board((s * 0.5, -1.15, 0.53), (0.0, -0.1, 0.6), 0.08, 0.08, face=(0.0, 0.0, 1.0), uv='C')
        m.board((s * 0.5, 1.1, 0.68), (0.0, 0.3, 0.66), 0.08, 0.08, face=(0.0, 0.0, 1.0), uv='C')
        # The bed's sills along it, on the bolsters.
        charred(m, (s * 0.42, -HL - 0.02, BED_Z - 0.075), (s * 0.42, HL + 0.02, BED_Z - 0.075), 0.1, 0.1,
                seed + 5 + int(s), face=(0.0, 0.0, 1.0), cuts=4)
    # The bed: charred floor boards (some burnt through), side boards burnt to ragged heights, end gates, stakes.
    for k, x in enumerate(kit.frange(-HW + 0.1, HW, 0.2)):
        if k in (2,) and not tipped:
            charred(m, (x, -HL, BED_Z), (x, -0.4, BED_Z), 0.19, 0.05, seed + 10 + k, face=(0.0, 0.0, 1.0),
                    burn=(0.0, 0.5))
            charred(m, (x, 0.5, BED_Z), (x, HL, BED_Z), 0.19, 0.05, seed + 20 + k, face=(0.0, 0.0, 1.0),
                    burn=(0.4, 0.0))
            continue
        charred(m, (x, -HL, BED_Z), (x, HL, BED_Z), 0.19, 0.05, seed + 10 + k, face=(0.0, 0.0, 1.0), cuts=4)
    for s in (-1.0, 1.0):
        x = s * (HW + 0.03)
        for j, z in enumerate((BED_Z + 0.14, BED_Z + 0.36)):
            if j == 1 and s > 0:
                charred(m, (x, -HL + 0.05, z), (x, 0.3, z), 0.21, 0.04, seed + 40 + j, face=(s, 0.0, 0.0),
                        burn=(0.0, 0.6))
                continue
            charred(m, (x, -HL + 0.05, z), (x, HL - 0.05, z), 0.21, 0.04, seed + 30 + j + int(s * 3),
                    face=(s, 0.0, 0.0), burn=(0.25 if j else 0.0, 0.35 if j else 0.0), cuts=4)
        for y in (-1.4, -0.45, 0.5, 1.45):
            m.board((x + s * 0.03, y, BED_Z - 0.05), (x + s * 0.03, y, BED_Z + 0.5), 0.05, 0.03, face=(s, 0.0, 0.0),
                    uv=iron)
    for y, s in ((-HL - 0.02, -1.0), (HL + 0.02, 1.0)):
        for j, z in enumerate((BED_Z + 0.14, BED_Z + 0.36)):
            charred(m, (-HW, y, z), (HW, y, z), 0.21, 0.04, seed + 50 + j + int(s * 4), face=(0.0, s, 0.0),
                    burn=(0.0, 0.3) if j else (0.0, 0.0))
    # Bows that carried the canvas: two still arch over the bed, one snapped, one gone.
    bows = []
    for k, y in enumerate((-1.25, -0.4, 0.45, 1.3)):
        if k == 1 or tipped:
            continue
        pts = []
        top = BED_Z + 1.35
        reach = 1.0 if k != 0 else 0.42
        for j in range(9):
            a = math.pi * j / 8
            if j / 8 > reach:
                break
            x = -(HW + 0.08) * math.cos(a)
            z = BED_Z + 0.5 + (top - BED_Z - 0.5) * math.sin(a) ** 0.6
            pts.append((x, y, z))
        if len(pts) > 1:
            bow = lr.tube([(pts[0][0], y, BED_Z + 0.05)] + pts, 0.03, sides=5, key='charcoal', set_name='BarkOak',
                          seed=seed + 60 + k)
            bows.append(bow)
    for bow in bows:
        m.add(bow)
    return BED_Z, HW, HL


def soot_gear(obj, lo=0.3):
    """Scorches the running gear under the bed: the occlusion darkens above lo metres within the wagon's footprint."""
    lr.darken(obj, lambda p, n: 1.0 if p.z > lo and abs(p.x) < 1.0 and abs(p.y) < 2.0 else 0.0, strength=0.45)


def wagon_burnt_a(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    BED_Z, HW, HL = wagon(m, seed)
    m.board((0.0, -1.3, 0.48), (0.25, -4.1, 0.06), 0.1, 0.12, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    m.board((-0.45, -3.6, 0.1), (0.95, -3.85, 0.08), 0.06, 0.06, face=(0.0, 0.0, 1.0), uv='C')
    # Wheels: the front left burnt out and lies flat under its corner; the others stand, charred, spokes gone.
    for k, (x, y, r, burnt) in enumerate(((0.82, -1.15, 0.45, 0.35), (-0.82, 1.1, 0.6, 0.2), (0.82, 1.1, 0.6, 0.45))):
        w = lr.wheel(r, 0.075, 12, seed + 70 + k, hub=0.09, felloe=0.07, tyre=0.012, burnt=burnt,
                     gap=rng.uniform(0, 360) if k == 2 else None)
        lp.place(w, (0.0, 0.0, 0.0), (0.0, 90.0, rng.uniform(0, 30)))
        lp.place(w, (x + (0.06 if x > 0 else -0.06), y, r))
        m.add(w)
    # Slump: with its front left wheel burnt out, the wagon has dropped onto that corner: tip it all about the rear
    # right wheel's foot, then lay the burnt wheel flat under the corner.
    pivot = Vector((0.88, 1.1, 0.0))
    tilt = Matrix.Translation(pivot) @ Euler((math.radians(-4.0), math.radians(-6.5), 0.0)).to_matrix().to_4x4() @ \
        Matrix.Translation(-pivot)
    for v in m.bm.verts:
        v.co = tilt @ v.co
    flat = lr.wheel(0.45, 0.075, 12, seed + 80, hub=0.09, felloe=0.07, tyre=0.012, burnt=0.7, gap=200.0)
    lp.place(flat, (-1.0, -1.2, 0.06), (4.0, -6.0, 0.0))
    m.add(flat)
    corners = [tilt @ Vector((sx * (HW + 0.12), sy * (HL + 0.05), z)) for sx in (-1, 1) for sy in (-1, 1)
               for z in (BED_Z - 0.35, BED_Z + 0.55)]
    m.hull_points(corners)
    m.hull((0.22, 2.4, 0.5), at=(0.0, 0.0, 0.42))
    m.section('wagon')
    obj = m.finish(fallback=100.0, ao_distance=0.6, preview=False)
    soot_gear(obj)
    lr.preview([obj], name, view=(-1.0, -1.25, 0.6), fit=0.85)
    return obj


def wagon_burnt_b(name, seed):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    BED_Z, HW, HL = wagon(m, seed, tipped=True)
    # The right side ends up on top: its rear wheel (burnt half away) stays on its axle; the front one burnt off.
    w = lr.wheel(0.6, 0.075, 12, seed + 71, hub=0.09, felloe=0.07, tyre=0.012, burnt=0.45, gap=rng.uniform(0, 360))
    lp.place(w, (0.0, 0.0, 0.0), (0.0, 90.0, rng.uniform(0, 30)))
    lp.place(w, (0.88, 1.1, 0.6))
    m.add(w)
    # Tipped onto its left side, the open bed toward the front left where its cargo spilled: turn everything about
    # the long axis, onto the ground.
    turn = Euler((0.0, math.radians(-84.0), math.radians(-6.0))).to_matrix().to_4x4()
    for v in m.bm.verts:
        v.co = turn @ v.co
    lo = min(v.co.z for v in m.bm.verts)
    cx = (min(v.co.x for v in m.bm.verts) + max(v.co.x for v in m.bm.verts)) * 0.5
    for v in m.bm.verts:
        v.co += Vector((-cx, 0.0, -lo - 0.04))
    # The left side's wheels: one crushed flat half under the bed, one rolled clear; the tongue snapped off.
    for k, (at, rot, burnt, gap, r) in enumerate((((-1.05, 1.5, 0.05), (0.0, 0.0, 15.0), 0.6, 120.0, 0.6),
                                                  ((-1.9, -2.5, 0.05), (3.0, 5.0, 40.0), 0.4, None, 0.45))):
        w = lr.wheel(r, 0.075, 12, seed + 90 + k, hub=0.09, felloe=0.07, tyre=0.012, burnt=burnt, gap=gap)
        lp.place(w, at, rot)
        m.add(w)
    charred(m, (0.6, -2.1, 0.07), (1.7, -4.3, 0.06), 0.1, 0.12, seed + 95, face=(0.0, 0.0, 1.0), burn=(0.3, 0.0),
            mat='trim', uv=Trim('C', lane='each'))
    # Two bows broke off as it went over; their halves lie in front of the bed.
    for k, (cx_, cy_, yaw) in enumerate(((-1.25, -0.9, 20.0), (-0.95, 0.3, -35.0))):
        pts = []
        for j in range(6):
            a = math.pi * 0.5 * j / 5
            px, py = 0.72 * math.cos(a), 0.0
            pz = 0.62 * math.sin(a)
            c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
            # Lying flat: the arc's height becomes its reach across the ground.
            pts.append((cx_ + px * c - pz * s, cy_ + px * s + pz * c, 0.035))
        m.add(lr.tube(pts, 0.03, sides=5, key='charcoal', set_name='BarkOak', seed=seed + 80 + k))
    # Spilled cargo: two charred barrels and a burst crate.
    for k, (x, y, yaw) in enumerate(((-2.0, -0.5, 70.0), (-1.75, 1.0, 20.0))):
        barrel, bands = lr.lathe([(0.0, -0.42), (0.24, -0.42), (0.28, -0.2), (0.3, 0.0), (0.28, 0.2), (0.24, 0.42),
                                  (0.0, 0.42)], 12, 'charcoal', set_name='BarkOak', grain='up', seed=seed + 100 + k)
        lp.place(barrel, (0.0, 0.0, 0.0), (90.0, 0.0, yaw))
        lp.place(barrel, (x, y, 0.29))
        m.add(barrel)
        for z in (-0.3, 0.3):
            hoop, _ = lr.lathe([(0.29 - abs(z) * 0.08, z - 0.03), (0.305 - abs(z) * 0.08, z - 0.03),
                                (0.305 - abs(z) * 0.08, z + 0.03), (0.29 - abs(z) * 0.08, z + 0.03)], 12, 'trim',
                               strip='Iron', closed=True, seed=seed + 110 + k)
            lp.place(hoop, (0.0, 0.0, 0.0), (90.0, 0.0, yaw))
            lp.place(hoop, (x, y, 0.29))
            m.add(hoop)
        m.hull_points([(x + math.cos(math.radians(yaw + 90)) * 0.42 * s + math.cos(math.radians(yaw)) * 0.3 * t,
                        y + math.sin(math.radians(yaw + 90)) * 0.42 * s + math.sin(math.radians(yaw)) * 0.3 * t, z)
                       for s in (-1, 1) for t in (-1, 1) for z in (0.0, 0.58)])
    for k in range(4):
        charred(m, (-1.3 + k * 0.17, -2.75, 0.03), (-1.15 + k * 0.17, -2.15, 0.04 + 0.04 * k), 0.15, 0.03,
                seed + 120 + k, face=(0.0, 0.0, 1.0), burn=(0.0, 0.2), splinters=0)
    # Hull: the bed on its side.
    m.hull((1.5, 3.8, 1.35), at=(0.1, 0.0, 0.65), rot=(0.0, 0.0, -6.0))
    m.section('wagon')
    obj = m.finish(fallback=100.0, ao_distance=0.6, preview=False)
    soot_gear(obj, lo=0.2)
    lr.preview([obj], name, view=(-0.9, -1.3, 1.0), fit=0.85)
    return obj


# --- Dry-stone wall pieces ---

# Low remnants of the wall's profile (same points as Fences.py's wall_profile, the courses above gone): what's left in
# a breach, and the lower one, what's left of a fallen wall.
LOW = [(-0.36, -0.15), (-0.36, 0.02), (-0.33, 0.25), (-0.3, 0.34), (-0.25, 0.39), (-0.11, 0.42), (0.11, 0.42),
       (0.25, 0.39), (0.3, 0.34), (0.33, 0.25), (0.36, 0.02), (0.36, -0.15)]
LOWER = [(-0.37, -0.15), (-0.37, 0.02), (-0.34, 0.16), (-0.31, 0.22), (-0.24, 0.27), (-0.1, 0.3), (0.1, 0.3),
         (0.24, 0.27), (0.31, 0.22), (0.34, 0.16), (0.37, 0.02), (0.37, -0.15)]


def wall_sweep(path, length, height=lambda s: 1.0, top_jitter=None, step=0.1, uv='box', seed=0, low=LOW, bumps=None):
    """A dry-stone wall body swept along path(s) -> (point, side) for s in 0..length, with Fences.py's profile and
    stone bulges. height(s) blends the full profile (1) into a low remnant (0: low); at s = 0 and s = length a full
    ring is exactly SM_StoneWall's end ring. uv='arc' maps it by distance along the path (a corner) the way box_uv
    maps a straight wall; 'box' leaves it to box_uv. bumps(s, across, up) replaces Fences.py's stones() (a piece of
    another length blends its bulges back to the start's by its own end)."""
    rnd = random.Random(seed)
    full = W.wall_profile()
    bumps = bumps or W.stones
    steps = max(2, int(round(length / step)))
    bm = lp.new_bmesh()
    rings, frames = [], []
    for i in range(steps + 1):
        s = length * i / steps
        point, side = path(s)
        h = min(max(height(s), 0.0), 1.0)
        ring = []
        for j, ((a, b), (la, lb)) in enumerate(zip(full, low)):
            ba = b if h >= 1.0 else lb + (b - lb) * h
            aa = a if h >= 1.0 else la + (a - la) * h
            if top_jitter is not None and 0 < i < steps and 3 <= j <= 8:
                ba += top_jitter(s, j)
            if b > -0.1:
                n = Vector((0.0, aa, (ba - 0.3) * 0.6 if ba > W.BODY_TOP - 0.1 else 0.0)).normalized()
                bump = bumps(s, aa, ba)
                da, db = aa + n.y * bump, ba + n.z * bump
            else:
                da, db = aa, ba
            ring.append(bm.verts.new(point + side * da + Vector((0.0, 0.0, db))))
        rings.append(ring)
        frames.append((s, point, side))
    n = len(full)
    faces = []
    for i, (r0, r1) in enumerate(zip(rings, rings[1:])):
        for j in range(n - 1):
            faces.append((bm.faces.new((r0[j], r0[j + 1], r1[j + 1], r1[j])), i, j))
        faces.append((bm.faces.new((r0[n - 1], r0[0], r1[0], r1[n - 1])), i, n - 1))
    caps = [bm.faces.new(rings[0]), bm.faces.new(rings[-1])]
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.normal_update()
    if uv == 'arc':
        layer = bm.loops.layers.uv.active
        k = W.WALL_DENSITY / 1024.0
        for face, i, j in faces:
            # Each face is projected along its own stretch of the path, as box_uv projects a straight wall: U is the
            # distance along the path to the vertex (so faces beside a mitred corner aren't stretched), V its height
            # on the sides, or its distance across on the top. Which way the face looks decides which.
            s0, p0 = frames[i][0], frames[i][1]
            d = frames[i + 1][1] - p0
            d = d.normalized() if d.length > 1e-6 else Vector((1.0, 0.0, 0.0))
            left = Vector((0.0, 0.0, 1.0)).cross(d)
            lateral = face.normal.dot(left)
            upward = face.normal.z
            for loop in face.loops:
                off = loop.vert.co - p0
                u = s0 + off.dot(d)
                if abs(upward) > abs(lateral):
                    a = off.dot(left)
                    loop[layer].uv = (u * k, a * k if upward > 0.0 else -a * k)
                else:
                    loop[layer].uv = ((u if lateral < 0.0 else -u) * k, loop.vert.co.z * k)
        for cap in caps:
            # A joint's cap (hidden in a run, seen at an open end): projected on the plane it faces.
            across_x = abs(cap.normal.x) > abs(cap.normal.y)
            for loop in cap.loops:
                loop[layer].uv = ((loop.vert.co.y if across_x else loop.vert.co.x) * k, loop.vert.co.z * k)
    part = lp.mesh_object(bm)
    lt.assign(part, lr.material('stone'))
    return part


def straight(s):
    return Vector((s, 0.0, 0.0)), Vector((0.0, 1.0, 0.0))


def wall_stone(size, center, rotation, seed):
    """A loose dry-stone wall stone (StoneWall at the wall's own texel density); small ones are plain lumpy boxes."""
    part = lp.block(size, (0.0, 0.0, 0.0), bevel=min(size) * 0.2 if max(size) > 0.3 else 0.0)
    lp.rough(part, min(size) * 0.08, 6.0, seed)
    lp.place(part, center, rotation)
    lt.assign(part, lr.material('stone'))
    lt.box_uv(part, 'StoneWall', texel_density=W.WALL_DENSITY, seed=seed)
    return part


def scatter_stones(rnd, count, area, sizes, lie=0.0, coping=0.0, avoid=None):
    """Loose stones on the ground in area (x0, x1, y0, y1) or a function returning a spot; some are coping slabs."""
    parts = []
    placed = []
    tries = 0
    while len(parts) < count and tries < count * 30:
        tries += 1
        x, y = area() if callable(area) else (rnd.uniform(area[0], area[1]), rnd.uniform(area[2], area[3]))
        if avoid is not None and avoid(x, y):
            continue
        if any((Vector((x, y)) - Vector(p)).length < 0.2 for p in placed):
            continue
        placed.append((x, y))
        if rnd.random() < coping:
            size = (rnd.uniform(0.07, 0.11), rnd.uniform(0.34, 0.44), rnd.uniform(0.15, 0.25))
            rot = (rnd.uniform(-90, 90), rnd.choice((80.0, -80.0, 0.0)) + rnd.uniform(-10, 10), rnd.uniform(0, 180))
            z = 0.05
        else:
            size = (rnd.uniform(*sizes[0]), rnd.uniform(*sizes[1]), rnd.uniform(*sizes[2]))
            rot = (rnd.uniform(-15, 15), rnd.uniform(-15, 15), rnd.uniform(0, 180))
            z = size[2] * 0.32 + lie
        parts.append(wall_stone(size, (x, y, z), rot, rnd.randrange(1 << 20)))
    return parts


def stonewall_broken(name, seed):
    """A standing segment breached in the middle: full at both ends (chains with SM_StoneWall), its courses tumbled
    down to a low remnant between about 1.1 and 1.9 m, the stones lying on both sides."""
    rnd = random.Random(seed)
    L = W.WALL_LENGTH
    a0, a1, b0, b1 = 0.85, 1.3, 1.85, 2.3      # breaking down between a0 and a1, rising between b0 and b1

    def height(s):
        if s <= a0 or s >= b1:
            return 1.0
        if a1 <= s <= b0:
            return 0.0
        t = (s - a0) / (a1 - a0) if s < a1 else (b1 - s) / (b1 - b0)
        return round(t * 3.0 + 0.3) / 3.0        # in steps, like courses

    def jitter(s, j):
        return 0.05 * noise.noise(Vector((s * 7.0, j * 0.7, seed))) if a0 < s < b1 else 0.0
    body = wall_sweep(straight, L, height, top_jitter=jitter, seed=seed)
    parts = [body] + W.coping(0.0, a0 - 0.05, seed + 1) + W.coping(b1 + 0.02, L, seed + 2)
    for part in parts:
        lt.assign(part, lr.material('stone'))
    # Stones fallen out of the breach, both sides, and a few left sitting on its slopes.
    parts += scatter_stones(rnd, 9, (a0 - 0.4, b1 + 0.4, -1.1, -0.42), ((0.2, 0.36), (0.16, 0.26), (0.1, 0.16)),
                            coping=0.3)
    parts += scatter_stones(rnd, 7, (a0 - 0.2, b1 + 0.5, 0.42, 1.0), ((0.2, 0.34), (0.16, 0.26), (0.1, 0.16)),
                            coping=0.25)
    for k, (s, h) in enumerate(((a1 - 0.08, 0.5), (b0 + 0.1, 0.48), ((a1 + b0) * 0.5, 0.4))):
        parts.append(wall_stone((0.3, 0.24, 0.12), (s, rnd.uniform(-0.1, 0.1), h), (rnd.uniform(-8, 8),
                                rnd.uniform(-12, 12), rnd.uniform(0, 180)), seed + 30 + k))
    obj = lp.join(name, parts)
    lt.box_uv(obj, 'StoneWall', texel_density=W.WALL_DENSITY)
    top = W.BODY_TOP + 0.2
    profile = [(-0.36, -0.15), (0.36, -0.15), (-0.36, 0.02), (0.36, 0.02), (-0.25, top), (0.25, top)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (0.0, a0 + 0.1) for a, b in profile])
    lp.hull_points(obj, [Vector((x, a, b)) for x in (b1 - 0.1, L) for a, b in profile])
    low = [(-0.36, -0.15), (0.36, -0.15), (-0.36, 0.02), (0.36, 0.02), (-0.25, 0.38), (0.25, 0.38)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (a0, b1) for a, b in low])
    lp.finish(obj, ao=0.5, fallback=100, smooth=35.0)
    lr.preview([obj], name, view=(-1.0, -1.6, 0.6), fit=1.0)
    return obj


def stonewall_fallen(name, seed):
    """A stretch of fallen wall: a low, broken remnant of the bottom course (its low ends chain with the next
    StoneWall_Fallen), two courses still standing in the middle, loose stones on it, and the spill of the fallen
    courses on the back (+Y) side: a low mound strewn with stones, which carries on through a run of these."""
    rnd = random.Random(seed)
    L = W.WALL_LENGTH

    def height(s):
        hump = max(0.0, 1.0 - abs(s - 1.3) / 0.5)
        return round(0.62 * hump ** 0.6 * 3.0) / 3.0          # stepped, like courses

    def jitter(s, j):
        return 0.06 * noise.noise(Vector((s * 6.0, j * 0.9, seed)))
    body = wall_sweep(straight, L, height, top_jitter=jitter, seed=seed, low=LOWER)

    def lumps(s, a, f=2.2):
        """Noise along the wall that repeats every wall length, so chained pieces meet."""
        blend = min(max((s - (L - 0.6)) / 0.6, 0.0), 1.0)
        return (noise.noise(Vector((s * f, a * f + 31.0, seed * 0.1))) * (1.0 - blend) +
                noise.noise(Vector(((s - L) * f, a * f + 31.0, seed * 0.1))) * blend)
    bm = lp.new_bmesh()
    steps = 30
    across = [(-0.1, -0.1), (0.3, 0.2), (0.6, 0.27), (0.9, 0.2), (1.2, 0.1), (1.5, 0.03), (1.75, -0.05)]
    rows = []
    for i in range(steps + 1):
        s = L * i / steps
        row = []
        for j, (a, h) in enumerate(across):
            bump = lumps(s, a)
            reach = a + 0.3 * lumps(s, 9.0) * (j / (len(across) - 1)) ** 1.5      # a ragged outer edge
            edge = 1.0 if 0 < j < len(across) - 1 else 0.3
            row.append(bm.verts.new((s, reach, max(h + 0.09 * bump * edge, -0.06))))
        rows.append(row)
    for r0, r1 in zip(rows, rows[1:]):
        for j in range(len(across) - 1):
            bm.faces.new((r0[j], r1[j], r1[j + 1], r0[j + 1]))
    for row, flip in ((rows[0], False), (rows[-1], True)):
        # Its ends closed down to the ground (seen at a run's open ends).
        foot = [bm.verts.new((row[-1].co.x, row[-1].co.y, -0.08)), bm.verts.new((row[0].co.x, row[0].co.y, -0.08))]
        ring = list(row) + foot
        bm.faces.new(list(reversed(ring)) if flip else ring)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mound = lp.mesh_object(bm)
    lt.assign(mound, lr.material('stone'))
    parts = [body, mound]
    # Stones of the fallen courses: strewn over the spill (most of them), a few on the front, and some still lying on
    # the remnant, slid half off it.
    parts += scatter_stones(rnd, 15, (0.1, L - 0.1, 0.32, 1.75), ((0.24, 0.42), (0.18, 0.3), (0.1, 0.2)),
                            lie=0.1, coping=0.25)
    parts += scatter_stones(rnd, 4, (0.15, L - 0.15, -1.0, -0.42), ((0.18, 0.32), (0.14, 0.24), (0.1, 0.16)),
                            coping=0.2)
    for k in range(7):
        s = 0.25 + k * 0.42 + rnd.uniform(-0.08, 0.08)
        h = 0.3 + (W.BODY_TOP - 0.3) * height(s) * 0.62
        parts.append(wall_stone((rnd.uniform(0.26, 0.36), rnd.uniform(0.2, 0.28), rnd.uniform(0.11, 0.15)),
                                (s, rnd.uniform(-0.16, 0.2), h + 0.04), (rnd.uniform(-14, 14), rnd.uniform(-18, 18),
                                                                          rnd.uniform(0, 180)), seed + 40 + k))
    obj = lp.join(name, parts)
    lt.box_uv(obj, 'StoneWall', texel_density=W.WALL_DENSITY)
    low = [(-0.37, -0.15), (0.37, -0.15), (-0.37, 0.02), (0.37, 0.02), (-0.24, 0.3), (0.24, 0.3)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (0.0, L) for a, b in low])
    hump = [(-0.33, 0.25), (0.33, 0.25), (-0.25, 0.55), (0.25, 0.55)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (1.0, 1.6) for a, b in hump])
    lp.finish(obj, ao=0.5, fallback=100, smooth=35.0)
    lr.preview([obj], name, view=(-0.9, -1.6, 0.7), fit=1.0)
    return obj


def stonewall_corner(name, seed):
    """A square corner: in along +X from the pivot (where a StoneWall's end meets it), turning left at (1.5, 0) and out
    along +Y to (1.5, 1.5), where the next StoneWall starts turned 90 degrees. Mapped by distance along it the way
    box_uv maps a straight wall, so both joints are seamless."""
    rnd = random.Random(seed)
    L = W.WALL_LENGTH
    arm = L * 0.5
    miter = Vector((-1.0, 1.0, 0.0))

    def path(s):
        if s < arm - 1e-6:
            return Vector((s, 0.0, 0.0)), Vector((0.0, 1.0, 0.0))
        if s > arm + 1e-6:
            return Vector((arm, s - arm, 0.0)), Vector((-1.0, 0.0, 0.0))
        return Vector((arm, 0.0, 0.0)), miter
    body = wall_sweep(path, L, uv='arc', seed=seed)
    parts = [body]
    # Coping along both arms, and a big corner stone over the turn.
    for stone in W.coping(0.0, arm - 0.3, seed + 1):
        parts.append(stone)
    for stone in W.coping(0.0, arm - 0.3, seed + 2):     # along X from 0.08, turned onto +Y and moved up the arm
        lp.place(stone, (0.0, 0.0, 0.0), (0.0, 0.0, 90.0))
        lp.place(stone, (arm, 0.3, 0.0))
        parts.append(stone)
    for part in parts:
        lt.assign(part, lr.material('stone'))
    corner = lp.block((0.62, 0.6, 0.24), (0.0, 0.0, 0.12), bevel=0.04)
    lp.rough(corner, 0.02, 6.0, seed + 3)
    lp.place(corner, (arm - 0.02, 0.02, W.BODY_TOP - 0.07), (0.0, 2.0, 45.0))
    lt.assign(corner, lr.material('stone'))
    parts.append(corner)
    for p in parts[1:]:
        lt.box_uv(p, 'StoneWall', texel_density=W.WALL_DENSITY)
    obj = lp.join(name, parts)
    top = W.BODY_TOP + 0.2
    profile = [(-0.36, -0.15), (0.36, -0.15), (-0.36, 0.02), (0.36, 0.02), (-0.25, top), (0.25, top)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (0.0, arm + 0.36) for a, b in profile])
    lp.hull_points(obj, [Vector((-a + arm, y, b)) for y in (0.0, arm) for a, b in profile])
    lp.finish(obj, ao=0.5, fallback=100, smooth=35.0)
    lr.preview([obj], name, view=(1.4, -1.3, 0.8), fit=1.0)
    return obj


def mirror_box_uv(part, length):
    """box_uv's projection of a straight wall (StoneWall at the wall's density, no offset) with the distance along it
    folded back at the middle: the texture runs out from the start and back again, so a piece shorter than a whole
    wall still meets the next one's texture at both ends (at x = 0 and x = length it is exactly SM_StoneWall's at its
    joint). The fold is a mirror line, not a seam: no stone is cut."""
    k = W.WALL_DENSITY / 1024.0
    half = length * 0.5
    bm = bmesh.new()
    bm.from_mesh(part.data)
    bm.normal_update()
    layer = bm.loops.layers.uv.active or bm.loops.layers.uv.new('UVMap')
    for face in bm.faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        for loop in face.loops:
            x, y, z = loop.vert.co
            xm = x if x <= half else length - x
            if axis == 2:
                loop[layer].uv = (xm * k, (y if n.z > 0.0 else -y) * k)
            elif axis == 0:
                loop[layer].uv = ((y if n.x > 0.0 else -y) * k, z * k)
            else:
                loop[layer].uv = ((-xm if n.y > 0.0 else xm) * k, z * k)
    bm.to_mesh(part.data)
    bm.free()
    part.data.update()


def stonewall_half(name, seed):
    """Half a wall (1.5 m) for the sheep fold's gate side, where whole walls won't fit: its ends are SM_StoneWall's own
    end ring (its stone bulges blend back to the start's over its last 0.6 m, as SM_StoneWall's do), so it chains with
    whole walls, the corner, the broken piece and StoneWallEnd at either end. Its texture is mirrored at the middle
    (mirror_box_uv), where a through stone sticks out of both faces, as a dry-stone wall has every metre or so."""
    rnd = random.Random(seed)
    L = W.WALL_LENGTH * 0.5

    def bumps(s, a, b):
        blend = min(max((s - (L - 0.6)) / 0.6, 0.0), 1.0)
        return W.stones(s, a, b) * (1.0 - blend) + W.stones(s - L, a, b) * blend
    # A ring exactly at the middle, so no face straddles the fold.
    body = wall_sweep(straight, L, step=L / 16, seed=seed, bumps=bumps)
    mirror_box_uv(body, L)
    parts = W.coping(0.0, L, seed + 1)
    through = lp.block((0.24, 0.84, 0.11), (0.0, 0.0, 0.0), bevel=0.02)
    lp.rough(through, 0.012, 6.0, seed + 2)
    lp.place(through, (L * 0.5 + rnd.uniform(-0.03, 0.03), rnd.uniform(-0.02, 0.02), 0.43), (rnd.uniform(-3, 3),
             rnd.uniform(-4, 4), rnd.uniform(-6, 6)))
    parts.append(through)
    # Big face stones above and below it on both faces, set proud of the wall, cover the rest of the fold.
    for side in (-1.0, 1.0):
        for z, half_width, size in ((0.13, 0.345, (0.32, 0.12, 0.2)), (0.64, 0.268, (0.3, 0.1, 0.17))):
            stone = lp.block(size, (0.0, 0.0, 0.0), bevel=0.025)
            lp.rough(stone, 0.01, 7.0, seed + int(z * 100) + int(side * 7))
            lp.place(stone, (L * 0.5 + rnd.uniform(-0.04, 0.04), side * (half_width + size[1] * 0.5 - 0.06), z),
                     (rnd.uniform(-4, 4), rnd.uniform(-5, 5), rnd.uniform(-5, 5)))
            parts.append(stone)
    for part in parts:
        lt.assign(part, lr.material('stone'))
        lt.box_uv(part, 'StoneWall', texel_density=W.WALL_DENSITY)
    obj = lp.join(name, [body] + parts)
    top = W.BODY_TOP + 0.2
    profile = [(-0.36, -0.15), (0.36, -0.15), (-0.36, 0.02), (0.36, 0.02), (-0.25, top), (0.25, top)]
    lp.hull_points(obj, [Vector((x, a, b)) for x in (0.0, L) for a, b in profile])
    lp.finish(obj, ao=0.5, fallback=100, smooth=35.0)
    lr.preview([obj], name, view=(-1.0, -1.6, 0.6), fit=1.0)
    return obj


# --- Cairns ---

def cairn(name, seed, layers, stake=False):
    """A keeper's cairn: a rough pile of angular field stones laid in rings that close in toward the top, each ring
    settled into the gaps of the one below. layers: [(ring radius, stones, (smallest, largest), course height), ...]
    from the ground up (radius 0: one stone in the middle, or the capstone). stake wedges a crooked waymark stake
    into it with an iron hook a keeper's lantern hangs from."""
    rnd = random.Random(seed)
    parts = []
    z = -0.04
    for radius, count, (s0, s1), h in layers:
        start = rnd.uniform(0.0, 2.0 * math.pi)
        for k in range(count):
            a = start + 2.0 * math.pi * k / count + rnd.uniform(-0.25, 0.25)
            r = radius * rnd.uniform(0.85, 1.1)
            sx = rnd.uniform(s0, s1)
            size = (sx, sx * rnd.uniform(0.62, 0.88), h * rnd.uniform(0.85, 1.15))
            # Stones lean in toward the middle, the way a pile settles.
            lean = rnd.uniform(4.0, 12.0) if radius > 0.0 else rnd.uniform(-4.0, 4.0)
            rot = (lean * math.sin(a) * -1.0, lean * math.cos(a), math.degrees(a) + rnd.uniform(-30, 30))
            bevel = sx * 0.2 if sx > 0.19 else 0.0          # the small stones up top are plain lumps
            parts.append(lr.stone_block(size, (r * math.cos(a), r * math.sin(a), z + size[2] * 0.5), rot,
                                        seed=rnd.randrange(1 << 20), rough=sx * 0.11, bevel=bevel, key='granite',
                                        set_name='RockGranite'))
        z += h * 0.82
    top = z + 0.04
    base = layers[0][0]
    for k in range(3):
        # Stones fallen off the pile, half sunk.
        a = rnd.uniform(0.0, 2.0 * math.pi)
        r = base + rnd.uniform(0.16, 0.3)
        sx = rnd.uniform(0.14, 0.2)
        parts.append(lr.stone_block((sx, sx * rnd.uniform(0.6, 0.85), sx * 0.5), (r * math.cos(a), r * math.sin(a),
                                    sx * 0.12), (rnd.uniform(-12, 12), rnd.uniform(-12, 12), rnd.uniform(0, 180)),
                                    seed=rnd.randrange(1 << 20), rough=sx * 0.12, bevel=sx * 0.2, key='granite',
                                    set_name='RockGranite'))
    hull_points = [v.co.copy() for p in parts for v in p.data.vertices]
    if stake:
        st = lp.sweep([(0.05, 0.0, -0.1), (0.07, 0.02, top + 0.32), (0.05, 0.04, top + 0.6)],
                      lp.ngon(0.03, 6, jitter=0.12, seed=seed), scales=[1.0, 0.9, 0.75])
        lp.grain(st, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
        parts.append(st)
        parts.append(lr.tube([(0.06, 0.03, top + 0.53), (0.06, -0.09, top + 0.55), (0.06, -0.15, top + 0.51),
                              (0.06, -0.14, top + 0.45)], 0.009, sides=4, strip='Iron'))
    obj = lp.join(name, parts)
    lp.hull_points(obj, hull_points)
    lp.finish(obj, ao=0.35, nanite=False, smooth=40.0)
    obj['LODs'] = SMALL['LODs']
    lr.preview([obj], name, view=(-1.0, -1.5, 0.5), fit=1.1)
    return obj


# --- The hanging rope ---

def hanging_rope(name, seed):
    """The hanging tree's rope: two turns round a branch (along Y, its middle at the pivot, 8.5 cm round), hanging
    1.9 m to a hangman's knot, the noose below it and the frayed tail above. The pivot is the hanging point, so it can
    sway; no collision."""
    rnd = random.Random(seed)
    m = lr.Model(name, seed=seed)
    R_BRANCH = 0.085
    r = 0.016
    parts = []
    # The turns round the branch, drifting along it.
    wrap = []
    for k in range(25):
        a = -math.pi * 0.5 + 2.0 * math.pi * 2.0 * k / 24
        wrap.append((R_BRANCH * math.cos(a), -0.05 + 0.1 * k / 24, R_BRANCH * math.sin(a)))
    knot_top = -1.92
    hang = [wrap[-1]]
    for k in range(1, 6):
        t = k / 5
        hang.append((0.01 * math.sin(t * 3.0), 0.05 + 0.02 * math.sin(t * 2.0), -R_BRANCH + (knot_top + R_BRANCH) * t))
    parts.append(lr.tube(wrap + hang[1:], r, sides=6, lane=1, seed=seed, wobble=0.0015))
    # The knot: seven coils down the rope.
    coils = []
    for k in range(7 * 6 + 1):
        a = 2.0 * math.pi * k / 6
        coils.append((0.034 * math.cos(a), 0.05 + 0.034 * math.sin(a), knot_top - 0.2 * k / 42.0))
    parts.append(lr.tube(coils, r * 0.95, sides=4, lane=2, seed=seed + 1, caps=(False, False)))
    # The noose: a loop hanging from the knot's bottom.
    loop = []
    for k in range(19):
        a = 2.0 * math.pi * k / 18
        loop.append((0.15 * math.sin(a), 0.05, knot_top - 0.22 - 0.2 * (1.0 - math.cos(a))))
    parts.append(lr.tube(loop, r, sides=6, lane=1, seed=seed + 2, caps=(False, False)))
    # The tail out of the knot's top, frayed.
    tail = [(0.02, 0.06, knot_top + 0.01), (0.05, 0.08, knot_top + 0.09), (0.06, 0.08, knot_top + 0.17)]
    parts.append(lr.tube(tail, r * 0.9, sides=5, lane=2, seed=seed + 3, caps=(True, False)))
    parts += lr.frayed(tail[-1], Vector(tail[-1]) - Vector(tail[-2]), r * 0.9, seed + 4, count=5, length=0.07)
    for part in parts:
        m.add(part, smooth=True)
    return m.finish(view=(-1.0, -1.5, 0.3), fit=1.0, fallback=None, ao_distance=0.25, ground=False,
                    out=lr.preview_path(name), Nanite=0, LODs='50,25', Collision='None')


# Every model, by name. --only=Name,Name (after '--') builds just those, to look at a few previews quickly; the
# exporter always builds them all.
BUILDERS = [
    ('Ruin_Chimney', ruin_chimney, (301,)),
    ('Ruin_LanternHouse', ruin_lantern_house, (311,)),
    ('Ruin_Springhouse', ruin_springhouse, (321,)),
    ('Ruin_LiveryFooting', ruin_livery, (331,)),
    ('Ruin_Derrick', ruin_derrick, (341,)),
    ('DressedBlock_A', dressed_block, (351, (1.2, 0.75, 0.7), [((1, -1, 1), 0.12), ((-1, 1, 1), 0.07)])),
    ('DressedBlock_B', dressed_block, (352, (1.7, 0.8, 0.55), [((1, 1, 1), 0.1), ((-1, -1, -0.2), 0.09),
                                                               ((-1, 1, 1), 0.06)])),
    ('Wagon_BurntA', wagon_burnt_a, (361,)),
    ('Wagon_BurntB', wagon_burnt_b, (362,)),
    ('StoneWall_Broken', stonewall_broken, (371,)),
    ('StoneWall_Fallen', stonewall_fallen, (372,)),
    ('StoneWall_Corner', stonewall_corner, (373,)),
    # Ring radius, stones, (smallest, largest) stone, course height: from the ground up.
    ('Cairn_A', cairn, (381, [(0.25, 6, (0.24, 0.32), 0.17), (0.0, 1, (0.26, 0.3), 0.15), (0.16, 5, (0.2, 0.26), 0.15),
                              (0.09, 3, (0.17, 0.22), 0.14), (0.04, 2, (0.15, 0.18), 0.12),
                              (0.0, 1, (0.14, 0.16), 0.1)])),
    ('Cairn_B', cairn, (382, [(0.3, 8, (0.24, 0.32), 0.16), (0.0, 1, (0.3, 0.34), 0.14), (0.19, 5, (0.22, 0.28), 0.14),
                              (0.0, 1, (0.48, 0.52), 0.09)])),
    ('Cairn_C', cairn, (383, [(0.22, 6, (0.22, 0.3), 0.16), (0.0, 1, (0.24, 0.28), 0.14), (0.14, 4, (0.18, 0.24), 0.14),
                              (0.06, 2, (0.15, 0.19), 0.12), (0.0, 1, (0.14, 0.16), 0.1)], True)),
    ('HangingRope', hanging_rope, (391,)),
    # Added for the level's dressing; built last, so the pieces above come out exactly as before.
    ('StoneWall_Half', stonewall_half, (374,)),
]
ONLY = next((a.split('=', 1)[1].split(',') for a in kit._args() if a.startswith('--only=')), None)
models = [build(name, *args) for name, build, args in BUILDERS if ONLY is None or name in ONLY]
