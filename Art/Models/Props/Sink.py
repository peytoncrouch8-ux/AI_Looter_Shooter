"""The Sink's webs and egg sacs (Docs/Areas/RansomsRest.md: the Sink at (65, 46), its webbed ramp, the Gravemother's
den under Den Rock, the Webwood's dead trees on its north side, and Main 5: shoot down three egg sacs, each lets out
two spiders). A scripted model file (Art/README.md). Every model maps onto one texture set, Art/Textures/Webs
(Tools/Blender/looter_webs.py paints it; its cells are named below).

Web cards: masked, two-sided, swaying (material Web: the WorldFoliage master). No collision, no Nanite, no LODs (each
is a few dozen to six hundred triangles), and each one is trimmed to the web it shows: the sheets to their body plus
a strip along each anchor thread, the tangle to its convex outline, threads to the rows of their lane they fill. Place
them by hand, at most 20 in a view, with their shadows off (see the placement notes in the report).

  Web_Orb        an orb web 2 m across (Orb), torn at the lower left, anchor lines out to 2.4 m. Pivot at the hub;
                 it hangs in the XZ plane, front -Y. Span it between two blocks, across a gap in the rock, between
                 two limbs.
  Web_Corner     a mat of sheet web in the foot of a wall or rock (Hammock), 1.9 m along it: out over the floor (z = 0)
                 to 0.5 m, across the corner in a fillet and up the wall (the plane y = 0, behind) to 0.3 m, 1-3.5 cm
                 off both, its anchor threads glued 0.47 m up the wall and 0.67 m out. Pivot on the corner line. Lay
                 it along rock bases and block feet; turn it for a wall-wall corner.
  Web_Drape      sheet web over the top edge of a coffin or block (Hammock): glued 0.3 m back on its top, over the
                 edge, down its face (the plane y = 0, the object behind) and out to the floor 0.38 m in front.
                 Built for a 0.5 m edge: scale Z by 0.88 for a coffin, 1.1-1.4 for the dressed blocks. Pivot on the
                 floor at the foot of the face, mid-length.
  Web_Ground     a mat of sheet web lying on open floor, 2.6 x 1.3 m (Hammock), 1-4 cm up over the grit under it, its
                 anchor threads out on the ground, dusty (darker in the shade). Pivot at its middle, on the ground. At
                 the foot of a rock use Web_Corner, which climbs it.
  Web_Fork       sheet web in a fork of a dead tree (Hammock): the fork at the pivot, the limbs leaving it up-left
                 and up-right 48 degrees apart, the sheet glued along them from 8 cm to 2 m out and bellied back.
                 Scale X for a wider or narrower fork.
  Web_Tatters    four torn strips hanging 1.5-2.4 m from an edge along X (Tatters). Pivot at the edge's middle.
  Web_Strands    seven threads hanging 0.8-2.6 m from an edge along X, some with tufts (Strands). Pivot likewise.
  Web_Line       one 4 m line of twisted silk from the pivot to x = 4, sagging 10 cm (Strands' Rope): scale X to span
                 a gap (and Z by as much to keep the sag in proportion); set it up and down to hang a sac.
  Web_Funnel     the den's funnel: silk lining Den Rock's den mouth as a tube (Sheet), fitted to its measured mouth,
                 from just inside the jambs 3.2 m in, narrowing and darkening as Den Rock's den does, anchor lines
                 out onto the face. Its pivot is Den Rock's SOCKET_DenMouth (place it there, same rotation).
  Web_BranchWrap silk wound round a dead branch (Wrap), 1.3 m along +X from the pivot on the branch's axis (built
                 for a 7 cm radius: scale Y and Z by radius / 0.07), baggy and sagging under it, short threads
                 hanging off it. No long tatters, so it scales to any branch: hang Web_Tatters beside it for those.
  Web_Sling      the cradle that slings EggSac_B against a wall (Sheet): a bowl of silk round the sac's lower half,
                 torn open at the bottom, lines up to the wall. Same pivot as EggSac_B: give both one transform
                 (SOCKET_Sac marks it); the wall is the plane y = 0.66. When the sac is shot down the empty cradle
                 stays.
  Web_Snare      a cobweb tangle (Tangle) 2.4 x 1.2 m, two crossed cards, with a silk cord down to SOCKET_Lantern:
                 where the Keeper's Lantern hangs in Main 5 (its SOCKET_Grip goes there). Pivot at the tangle's middle.
  Web_Crown      tent webs in the forks of DeadTree_A's crown, where four of its main limbs leave the trunk (Tent):
                 pouches of dense silk glued along the trunk and the limb, fraying at their open tops, threads running
                 on up the trunk and out along the limbs. Fitted to that tree (DEAD_TREE_LIMBS, measured on
                 DeadTree.py): give it the tree's pivot and transform (uniform scale only). The Webwood's trees wear
                 it.

Egg sacs (Main 5): opaque silk (material EggSilk: the World master, the Sac cell's wound silk) in a thin fuzz of loose
silk, a few loose strands across it (both on Web). Each holds two curled spiders: they push lumps into the silk and
darken it where they lie close under it (baked into the vertex occlusion, which EggSilk's DiffuseAO 0.75 shows in
sunlight too). No Nanite, LODs at 50% and 25%. The two spiders come out at SOCKET_Spawn_1 and SOCKET_Spawn_2 (inside
the sac, each facing the way its spider's head points). Pivots at the bottom of the sac, where it lands.
  EggSac_A       a pendant teardrop 2.2 m tall with its stalk (bulb 1.3 m across), its silk gathered into the stalk in
                 folds: hangs from SOCKET_Silk, the top of its twisted stalk, under an overhang or from Web_Lines.
  EggSac_B       a round sac 1.5 m tall and across, its back flattened: sits in Web_Sling against a wall (or on the
                 floor). SOCKET_Silk on its top knot.
  EggSac_C       a gourd 2.4 m tall with its stalk: two lobes, one spider in each, the upper leaning off the lower, the
                 waist bound round with threads; hangs from SOCKET_Silk like A.
  EggSac_Burst   a hatched sac on the floor, deflated: the sac it was (1.45 m across, 1.9 m tall) fallen on its side
                 and collapsed in soft crumpled folds, 1.6 x 1.8 m and 0.58 m high, a little fuller at its closed end
                 (+Y) and held open at its torn mouth (toward -Y: turn that away from the wall). The mouth's edge is
                 ragged and stringy (Fringe, on Web), strands bridge the tear and trail 0.8 m on over the floor, a
                 fuzz of loose silk lies over the outside, and the inside (faces of its own: EggSilk is one-sided) is
                 darker and stained. SOCKET_Spawn_1/2 stand on the ground in front of the mouth, facing out: where the
                 two spiders come out when an intact sac lands.
Hulls: one convex hull round each intact sac's body (C: one per lobe), a low one round the burst sac; the threads have
none. Shots hit the hulls.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\Sink.py -Preview
"""
import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise

import looter_model as lm
import looter_props as lp
import looter_textures as lt
import looter_webs as lw

lw.register()
# Web cards sway a little: 4 cm at a weight of 1 (only tatter tips get near that), slower than leaves. DiffuseAO 0.6
# (the master's default is 0.4): the grime baked into the occlusion at attachments and low down shows in sunlight too.
WEB = lt.material('Webs', name='Web', master='WorldFoliage', WindStrength=4.0, WindSpeed=1.1, DiffuseAO=0.6)
# Egg sacs: the vertex occlusion carries the spiders' shadows, so it shows in direct light as well (default 0.4).
SILK = lt.material('Webs', name='EggSilk', master='World', DiffuseAO=0.75)
# The World master is one-sided, and the burst sac's inside is faces of its own laid back to back with the outside:
# previews cull back faces as Unreal does, or the two would fight.
SILK.use_backface_culling = True
if hasattr(SILK, 'use_backface_culling_shadow'):
    SILK.use_backface_culling_shadow = True
UP = Vector((0.0, 0.0, 1.0))
FRONT = Vector((0.0, -1.0, 0.0))
PX = 1.0 / lw.PX_M
PREVIEW = 'RansomsRest/Sink'


# --- Building cards ---

class Card:
    """A model being built: vertices with a position, a shading normal and their vertex colour (R wind weight, G sway
    phase, A occlusion), and faces with their own UVs per corner (a vertex can sit on two UV seams). Faces are turned
    so their winding agrees with their vertices' shading normals."""

    def __init__(self, name):
        self.name = name
        self.co, self.no, self.col = [], [], []
        self.faces, self.uvs, self.mats = [], [], []

    def vert(self, co, normal, wind=0.0, phase=0.0, occ=1.0):
        n = Vector(normal)
        self.co.append(Vector(co))
        self.no.append(n.normalized() if n.length > 1e-9 else UP.copy())
        self.col.append((min(max(wind, 0.0), 1.0), phase % 1.0, 0.0, min(max(occ, 0.0), 1.0)))
        return len(self.co) - 1

    def face(self, verts, uvs, mat=0, keep=False):
        """keep: the winding is already right (a surface wound by its grid): don't turn it to the normals, which a
        crumpled quad can fool."""
        verts, uvs = list(verts), [tuple(u) for u in uvs]
        p = [self.co[i] for i in verts]
        geo = (p[2] - p[0]).cross(p[-1] - p[1]) if len(p) == 4 else (p[1] - p[0]).cross(p[2] - p[0])
        if not keep and geo.dot(sum((self.no[i] for i in verts), Vector())) < 0.0:
            verts.reverse()
            uvs.reverse()
        self.faces.append(tuple(verts))
        self.uvs.append(uvs)
        self.mats.append(mat)

    def build(self, materials, custom_normals=True):
        mesh = bpy.data.meshes.new(self.name)
        mesh.from_pydata([tuple(c) for c in self.co], [], self.faces)
        uv = mesh.uv_layers.new(name='UVMap')
        for poly, uvs in zip(mesh.polygons, self.uvs):
            for li, value in zip(poly.loop_indices, uvs):
                uv.data[li].uv = value
        for mat in materials:
            mesh.materials.append(mat)
        mesh.polygons.foreach_set('material_index', self.mats)
        mesh.polygons.foreach_set('use_smooth', [True] * len(self.faces))
        loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loop_vert)
        lt._write_col(mesh, np.asarray(self.col, np.float32)[loop_vert])
        if custom_normals:
            mesh.normals_split_custom_set_from_vertices([tuple(n) for n in self.no])
        mesh.update()
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def shade_normal(surface, up=0.6):
    """A card's shading normal: its surface normal turned toward the sky. Both sides of a card take it (the master
    keeps back faces on it), so a web is lit like the ground it hangs over instead of going dark when its front faces
    away from the sun; the surface part keeps the normal map's tangent frame sound. Upright cards (orb, fork, crown,
    tatters, snare) lean hard (up 5: a fifth of the normal is the surface's). The level's sun is low (15 degrees), so
    with a milder lean the side of the normal decides the light: a web turned away from the sun goes dark grey against
    the sky, where real silk shines."""
    n = Vector(surface)
    if n.z < -1e-6 or (abs(n.z) < 0.25 and n.y > 0.0):
        n = -n
    return (n.normalized() + UP * up).normalized()


# The rows of each 32 px Strands lane its thread fills (from the lane's top), its waving and fluff included. A strip
# maps only these and narrows to match, so it isn't drawn wider than its thread; the tufted lane keeps more for tufts.
LANE_FILL = {'Cord': (8, 24), 'Rope': (8, 24), 'Tufted': (2, 30), 'Ribbon': (6, 26)}


def lane_rows(lane):
    """The V of the top and bottom of a lane's filled rows, and the share of the lane they are."""
    y0, _ = lw.LANES[lane]
    f0, f1 = LANE_FILL[lane]
    top = lw.REGIONS['Strands'][1]
    return 1.0 - (top + y0 + f0 + 0.5) / lw.SIZE, 1.0 - (top + y0 + f1 - 0.5) / lw.SIZE, (f1 - f0) / 32.0


def catenary(a, b, sag, steps=6):
    """Points of a thread strung from a to b, sagging by sag (m) in the middle."""
    a, b = Vector(a), Vector(b)
    return [a.lerp(b, i / steps) - UP * (4.0 * sag * (i / steps) * (1.0 - i / steps)) for i in range(steps + 1)]


def wiggle(points, amp, rnd, waves=1.5, keep_ends=True):
    """A thread made to wander a little: each point pushed sideways (across the thread, both ways) by up to amp (m), the
    push easing in and out along it; the ends stay put (keep_ends) where it's glued."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    ph = [rnd.uniform(0.0, 6.28) for _ in range(4)]
    out = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
        s1 = t.cross(UP if abs(t.z) < 0.9 else FRONT).normalized()
        s2 = t.cross(s1).normalized()
        f = i / max(n - 1, 1)
        ease = math.sin(math.pi * f) if keep_ends else min(1.0, 2.0 * f)
        out.append(p + (s1 * math.sin(2.0 * math.pi * waves * f + ph[0]) * 0.7 +
                        s1 * math.sin(2.0 * math.pi * waves * 2.3 * f + ph[1]) * 0.3 +
                        s2 * math.sin(2.0 * math.pi * waves * 1.3 * f + ph[2]) * 0.6) * (amp * ease))
    return out


def cord(card, points, width, lane, u0, wind, phase, crossed=True, occ=1.0, mat=0, taper=None):
    """A silk thread along a polyline: two crossed strips (or one) on a lane of the Strands cell, U along the thread from
    u0 px. width is the whole lane's width; the strip keeps only the rows the thread fills (LANE_FILL). wind: one weight
    per point (or one for all). taper: the width's share at the last point (it narrows toward there, a free end
    fraying to nothing), or one share per point; occ likewise one value or one per point."""
    pts = [Vector(p) for p in points]
    winds = list(wind) if isinstance(wind, (list, tuple)) else [wind] * len(pts)
    occs = list(occ) if isinstance(occ, (list, tuple)) else [occ] * len(pts)
    if taper is None:
        shares = [1.0] * len(pts)
    elif isinstance(taper, (list, tuple)):
        shares = list(taper)
    else:
        shares = [1.0 - (1.0 - taper) * (i / max(len(pts) - 1, 1)) ** 1.3 for i in range(len(pts))]
    va, vb, share = lane_rows(lane)
    width *= share
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    us = [(u0 + d * lw.PX_M) / lw.SIZE for d in s]
    frames = []
    for i in range(len(pts)):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        ref = UP if abs(t.z) < 0.9 else FRONT
        s1 = t.cross(ref).normalized()
        s2 = t.cross(s1).normalized()
        up_perp = UP - t * t.dot(UP)
        front = FRONT - t * t.dot(FRONT)
        if front.length < 0.2:
            front = Vector((1.0, 0.0, 0.0)) - t * t.x
        n = (up_perp + front.normalized() * 0.6)
        frames.append((s1, s2, n if n.length > 1e-6 else front))
    for k in range(2 if crossed else 1):
        rows = []
        for i, p in enumerate(pts):
            side = frames[i][k] * (width * 0.5 * shares[i])
            rows.append((card.vert(p - side, frames[i][2], winds[i], phase, occs[i]),
                         card.vert(p + side, frames[i][2], winds[i], phase, occs[i])))
        for i in range(len(pts) - 1):
            (a0, b0), (a1, b1) = rows[i], rows[i + 1]
            card.face((a0, b0, b1, a1), [(us[i], va), (us[i], vb), (us[i + 1], vb), (us[i + 1], va)], mat)


def hanging_thread(top, length, sway=(0.0, 0.0), steps=4):
    """Points of a thread hanging from top, bowing a little out of plumb."""
    pts = []
    for i in range(steps + 1):
        t = i / steps
        pts.append(Vector(top) + Vector((sway[0] * t * t, sway[1] * t * t, -length * t)))
    return pts


def sheet(card, region, xs, ys, keep, place, phase, mat=0):
    """A card over a grid of a cell's pixels: columns xs, rows ys. keep(i, j) says whether cell (i, j) holds any web;
    place(x, y) gives a grid point's (position, surface normal, wind, occlusion)."""
    index = {}

    def vid(i, j):
        if (i, j) not in index:
            co, n, wind, occ = place(xs[i], ys[j])
            index[(i, j)] = card.vert(co, n, wind, phase, occ)
        return index[(i, j)]
    for j in range(len(ys) - 1):
        for i in range(len(xs) - 1):
            if keep(i, j):
                card.face((vid(i, j), vid(i + 1, j), vid(i + 1, j + 1), vid(i, j + 1)),
                          (lw.uv(region, xs[i], ys[j]), lw.uv(region, xs[i + 1], ys[j]),
                           lw.uv(region, xs[i + 1], ys[j + 1]), lw.uv(region, xs[i], ys[j + 1])), mat)


def hammock_keep(lay, xs, ys, lens_margin=24.0):
    """Which cells of a grid over the Hammock cell's body hold web: the lens (with its ragged edge) and its frayed
    tips. The anchor threads outside the body get strips of their own (hammock_card)."""
    cy = lay['cy']

    def keep(i, j):
        x0, x1, y0, y1 = xs[i], xs[i + 1], ys[j], ys[j + 1]
        for x in np.linspace(x0, x1, 9):
            hh = float(np.interp(x, lay['xs'], lay['half']))
            if hh > 1.0 and y0 <= cy + hh + lens_margin and y1 >= cy - hh - lens_margin:
                return True
        for tip in (lay['xa'], lay['xb']):
            if x0 <= tip + 34.0 and x1 >= tip - 34.0 and y0 <= cy + 30.0 and y1 >= cy - 30.0:
                return True
        return False
    return keep


HAMMOCK_XS = [float(x) for x in range(0, 769, 48)]


def hammock_rows(lay):
    """The rows of the Hammock cell's body grid: eight bands from the top of its lens (and a margin for its ragged
    edge) to the bottom."""
    reach = max(lay['half']) + 24.0
    top, bottom = max(lay['cy'] - reach, 0.0), min(lay['cy'] + reach, float(lay['h']))
    return [top + (bottom - top) * k / 8.0 for k in range(9)]


def hammock_card(card, place, phase, strip_px=7.0, ys=None):
    """The Hammock cell as a card through place(x, y) -> (position, surface normal, wind, occlusion): its body (the
    lens and a margin for its ragged edge) as a grid of the cells that hold web, and every anchor thread beyond it as a
    strip 14 px wide following the thread out to its glue spot, so the fans' empty space between threads isn't drawn.
    ys: the body grid's rows (hammock_rows' span), where a card folds round a corner."""
    lay = lw.hammock_layout()
    ys = ys or hammock_rows(lay)
    top, bottom = ys[0], ys[-1]
    sheet(card, 'Hammock', HAMMOCK_XS, ys, hammock_keep(lay, HAMMOCK_XS, ys), place, phase)
    for fan in lay['fans']:
        edge = top if fan['side'] < 0 else bottom
        for line in fan['threads'] + fan['broken']:
            pts = [Vector((q[0], q[1], 0.0)) for q in line]
            beyond = (lambda q: q.y < edge) if fan['side'] < 0 else (lambda q: q.y > edge)
            out = []
            for a, b in zip(pts, pts[1:]):
                if beyond(a) != beyond(b):
                    t = (edge - a.y) / (b.y - a.y)
                    cut = a.lerp(b, t)
                    out.append(cut)
                if beyond(b):
                    out.append(b)
            if len(out) < 2:
                continue
            if line in fan['threads']:            # reach on past the glue spot to take its knot in
                d = (out[-1] - out[-2]).normalized()
                end = out[-1] + d * 5.0
                end.x = min(max(end.x, 0.5), lay['w'] - 0.5)
                end.y = min(max(end.y, 0.5), lay['h'] - 0.5)
                out[-1] = end
            mid = out[0].lerp(out[-1], 0.5) if len(out) == 2 else None
            if mid is not None:
                out = [out[0], mid, out[1]]
            rows = []
            for i, q in enumerate(out):
                t = (out[min(i + 1, len(out) - 1)] - out[max(i - 1, 0)]).normalized()
                n = Vector((-t.y, t.x, 0.0)) * strip_px
                row = []
                for s in (-1.0, 1.0):
                    x = min(max(q.x + n.x * s, 0.0), float(lay['w']))
                    y = min(max(q.y + n.y * s, 0.0), float(lay['h']))
                    co, normal, wind, occ = place(x, y)
                    row.append((card.vert(co, normal, wind, phase, occ), (x, y)))
                rows.append(row)
            for r0, r1 in zip(rows, rows[1:]):
                (a0, p0), (b0, q0) = r0
                (a1, p1), (b1, q1) = r1
                card.face((a0, b0, b1, a1), (lw.uv('Hammock', *p0), lw.uv('Hammock', *q0), lw.uv('Hammock', *q1),
                                             lw.uv('Hammock', *p1)))


def card_props(obj):
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    return obj


# --- The web cards ---

def web_orb(name='Web_Orb', seed=11):
    lay = lw.orb_layout()
    cx, cy = lay['center']
    card = Card(name)
    phase = random.Random(seed).random()

    def at(x, y, f):
        return Vector(((x - cx) * PX, 0.05 * (1.0 - f) ** 2, (cy - y) * PX))
    corners = lay['corners']
    outline = []
    for i, a in enumerate(corners):
        b = corners[(i + 1) % len(corners)]
        for t in (0.0, 1.0 / 3.0, 2.0 / 3.0):
            x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
            d = math.hypot(x - cx, y - cy)
            grow = 12.0 if t == 0.0 else 8.0
            outline.append((cx + (x - cx) * (d + grow) / d, cy + (y - cy) * (d + grow) / d))
    n = shade_normal(FRONT, 5.0)
    hub = card.vert(at(cx, cy, 0.0), n, 0.45, phase)
    rings = []
    for f in (0.36, 0.7, 1.0):
        ring = []
        for x, y in outline:
            px, py = cx + (x - cx) * f, cy + (y - cy) * f
            ring.append((card.vert(at(px, py, f), n, 0.45 * (1.0 - f), phase), (px, py)))
        rings.append(ring)
    m = len(outline)
    for k in range(m):
        (a, pa), (b, pb) = rings[0][k], rings[0][(k + 1) % m]
        card.face((hub, a, b), (lw.uv('Orb', cx, cy), lw.uv('Orb', *pa), lw.uv('Orb', *pb)))
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(m):
            (a, pa), (b, pb) = r0[k], r0[(k + 1) % m]
            (c, pc), (d, pd) = r1[(k + 1) % m], r1[k]
            card.face((a, b, c, d), (lw.uv('Orb', *pa), lw.uv('Orb', *pb), lw.uv('Orb', *pc), lw.uv('Orb', *pd)))
    # The anchor lines: a strip 16 px wide along each, from its frame corner out to the cell's edge.
    for k, line in enumerate(lay['anchors']):
        pts = [line[0], line[len(line) // 2], line[-1]]
        rows = []
        for i, (x, y) in enumerate(pts):
            j0, j1 = pts[max(i - 1, 0)], pts[min(i + 1, len(pts) - 1)]
            tx, ty = j1[0] - j0[0], j1[1] - j0[1]
            tl = math.hypot(tx, ty)
            sx, sy = -ty / tl * 8.0, tx / tl * 8.0
            row = []
            for side in (-1.0, 1.0):
                qx, qy = x + side * sx, y + side * sy
                row.append((card.vert(at(qx, qy, 1.0), n, 0.0, phase), (qx, qy)))
            rows.append(row)
        for r0, r1 in zip(rows, rows[1:]):
            (a, pa), (b, pb) = r0
            (c, pc), (d, pd) = r1[1], r1[0]
            card.face((a, b, c, d), (lw.uv('Orb', *pa), lw.uv('Orb', *pb), lw.uv('Orb', *pc), lw.uv('Orb', *pd)))
    return card_props(card.build([WEB]))


# Web_Corner's body rows: up the wall to row 120, across the corner to row 192 (bending at 156), on the floor after.
CORNER_ROWS = [39.0, 66.0, 93.0, 120.0, 156.0, 192.0, 230.0, 268.0, 307.0, 345.0]


def web_corner(name='Web_Corner', seed=12, length=1.9):
    """The Hammock's sheet as a mat in the foot of a wall: on the floor (z = 0), across the corner in a fillet and up
    the wall (the plane y = 0, behind) a little way, 1-3.5 cm off both, its anchor threads glued further up the wall
    and out on the floor. 360 px per metre across it."""
    card = Card(name)
    phase = random.Random(seed).random()
    ppm = 360.0
    wall_row, floor_row = CORNER_ROWS[3], CORNER_ROWS[5]
    leg = (floor_row - wall_row) / ppm / math.sqrt(2.0)       # the fillet leaves the wall and meets the floor this far
    into = Vector((0.0, 1.0, -1.0)).normalized()              # from the fillet toward the corner

    def place(x, y):
        along = (x - 384.0) / 768.0 * length
        bell = 0.0
        if y <= wall_row:
            p, n = Vector((along, 0.0, leg + (wall_row - y) / ppm)), Vector((0.0, -1.0, 0.0))
        elif y >= floor_row:
            p, n = Vector((along, -leg - (y - floor_row) / ppm, 0.0)), UP.copy()
        else:                                                  # sagging a little into the corner
            t = (y - wall_row) / (floor_row - wall_row)
            bell = 4.0 * t * (1.0 - t)
            p, n = Vector((along, -leg * t, leg * (1.0 - t))) + into * (0.025 * bell), -into
        # Lying over the grit and the rock's bumps; down to 4 mm at the anchors' glue spots on the cell's edges.
        glue = min(1.0, y / 39.0, (384.0 - y) / 39.0)
        lift = 0.004 + glue * (0.008 + 0.022 * (0.5 + 0.5 * noise.noise(Vector((along * 3.0, y / ppm * 3.0,
                                                                                  seed + 0.7)))))
        co = p + n * lift
        occ = 0.7 + 0.22 * min(1.0, math.hypot(co.y, co.z) / 0.45)          # grimy along the corner
        return co, shade_normal(n, 1.5), 0.05 * bell, occ
    hammock_card(card, place, phase, ys=CORNER_ROWS)
    return card_props(card.build([WEB]))


# The drape's section from its glued edge on the object's top (y > 0) over the edge (y = 0, z = H) to the floor in
# front: (y, z) at H = 0.5 m.
DRAPE = [(0.30, 0.505), (0.12, 0.53), (0.0, 0.545), (-0.07, 0.47), (-0.16, 0.275), (-0.28, 0.1), (-0.38, 0.005)]


def web_drape(name='Web_Drape', seed=13, length=1.8):
    card = Card(name)
    phase = random.Random(seed).random()
    path = [Vector((0.0, y, z)) for y, z in DRAPE]
    s = [0.0]
    for a, b in zip(path, path[1:]):
        s.append(s[-1] + (b - a).length)

    def along(t):
        d = t * s[-1]
        for k in range(len(path) - 1):
            if d <= s[k + 1] or k == len(path) - 2:
                f = (d - s[k]) / max(s[k + 1] - s[k], 1e-9)
                return path[k].lerp(path[k + 1], f), (path[k + 1] - path[k]).normalized()

    def place(x, y):
        t = y / 384.0
        p, tangent = along(t)
        co = Vector(((x - 384.0) / 768.0 * length, p.y, p.z))
        surface = Vector((1.0, 0.0, 0.0)).cross(tangent)
        free = max(0.0, min(1.0, (0.3 - p.y) / 0.3)) * max(0.0, min(1.0, p.z / 0.2))
        # Dusty, as the floor's mats are: the dense sheet faces the eye here, and at full brightness it read as a
        # white cloth from 10 m. Darkest where it meets the floor and along its glued edge.
        occ = 0.68 + 0.18 * min(1.0, p.z / 0.25)
        return co, shade_normal(surface, 1.5), 0.14 * free, occ * (0.84 + 0.16 * min(1.0, t * 4.0))
    hammock_card(card, place, phase)
    return card_props(card.build([WEB]))


def web_ground(name='Web_Ground', seed=14, size=(2.6, 1.3)):
    """The Hammock's sheet as a mat lying on the floor, 1-4 cm up over the grit under it, its anchor threads out to
    the ground."""
    card = Card(name)
    phase = random.Random(seed).random()

    def height(u, v):
        edge = max(0.0, 1.0 - (2.0 * v) ** 2) * max(0.0, 1.0 - (2.0 * u) ** 6)
        grit = max(0.0, noise.noise(Vector((u * size[0] * 2.7, v * size[1] * 2.7, seed + 0.3))))
        return 0.008 + edge * (0.012 + 0.03 * grit)

    def place(x, y):
        # Old floor silk is dusty: its occlusion darkens it in the shade, most where it lies flattest.
        u, v = (x - 384.0) / 768.0, (192.0 - y) / 384.0
        z = height(u, v)
        e = 0.004
        slope_x = (height(u + e, v) - height(u - e, v)) / (2.0 * e * size[0])
        slope_y = (height(u, v + e) - height(u, v - e)) / (2.0 * e * size[1])
        return (Vector((u * size[0], v * size[1], z)), Vector((-slope_x, -slope_y, 1.0)), 0.0,
                0.68 + 0.2 * min(1.0, (z - 0.008) / 0.035))
    hammock_card(card, place, phase)
    return card_props(card.build([WEB]))


def web_fork(name='Web_Fork', seed=15, length=2.0, spread=(110.0, 62.0)):
    card = Card(name)
    phase = random.Random(seed).random()
    da = Vector((math.cos(math.radians(spread[0])), 0.0, math.sin(math.radians(spread[0]))))
    db = Vector((math.cos(math.radians(spread[1])), 0.0, math.sin(math.radians(spread[1]))))

    def place(x, y):
        # From 8 cm up the limbs: at the fork itself both edges meet, and the cells there would have no area.
        s, t = 0.04 + 0.96 * x / 768.0, y / 384.0
        bell = math.sin(math.pi * t)
        co = (da.lerp(db, t)) * (s * length) + Vector((0.0, 0.12 * bell * s ** 0.7, -0.06 * bell * s))
        return co, shade_normal(FRONT, 5.0), 0.35 * bell * s, 0.8 + 0.2 * min(1.0, s * 3.0)     # grimy in the fork
    hammock_card(card, place, phase)
    return card_props(card.build([WEB]))


def tatter_strips(card, picks, phase_rng, top_z=0.0, scale=1.0, twist=28.0, along=Vector((1.0, 0.0, 0.0)),
                  origin=Vector(), mat=0, wind_top=0.0):
    """Strips of the Tatters cell hanging from a line: picks are (tatter index, offset along the line, how far down
    to show it (px, None for all)). Each strip follows its tatter's outline (14 px out), turns a little as it hangs
    and bows out of plumb."""
    lays = lw.tatter_layout()
    side_dir = along.normalized()
    out_dir = side_dir.cross(UP).normalized()
    for index, offset, show in picks:
        t = lays[index]
        rnd = random.Random(1000 + index * 7 + int(offset * 100))
        bottom = max(q[1] for line in t['loose'] for q in line) + 4.0
        end = bottom if show is None else min(show, bottom)
        ys = [0.0, 16.0] + [float(y) for y in range(64, int(end), 64)] + [end]
        ys = sorted(set(ys))
        ys = [y for y in ys if y <= end]
        phase = phase_rng.random()
        bow = rnd.uniform(-0.06, 0.06)
        turn = rnd.uniform(-twist, twist)
        rows = []
        for y in ys:
            if y <= 16.0:
                lo = min(min(q[0] for q in line) for line in t['top']) - 8.0
                hi = max(max(q[0] for q in line) for line in t['top']) + 8.0
                c = float(np.interp(y, t['ys'], t['centre']))
                h = float(np.interp(y, t['ys'], t['half'])) + 14.0
                lo, hi = min(lo, c - h), max(hi, c + h)
            elif y <= t['length']:
                span = [yy for yy in np.linspace(max(y - 64.0, 0.0), min(y + 64.0, t['length']), 9)]
                lo = min(float(np.interp(yy, t['ys'], t['centre'])) - float(np.interp(yy, t['ys'], t['half'])) for yy in span) - 14.0
                hi = max(float(np.interp(yy, t['ys'], t['centre'])) + float(np.interp(yy, t['ys'], t['half'])) for yy in span) + 14.0
            else:
                xs_loose = [q[0] for line in t['loose'] for q in line]
                lo, hi = min(xs_loose) - 8.0, max(xs_loose) + 8.0
            f = y / max(end, 1.0)
            a = math.radians(turn * f)
            row = []
            for x in (lo, hi):
                dx = (x - (t['x0'] + t['x1']) * 0.5) * PX * scale
                r = side_dir * (dx * math.cos(a)) + out_dir * (dx * math.sin(a) + bow * math.sin(math.pi * f) * scale)
                co = origin + side_dir * offset + r + UP * (top_z - y * PX * scale)
                surface = side_dir.cross(UP) * math.cos(a) - side_dir * math.sin(a)
                wind = wind_top + (0.9 - wind_top) * min(1.0, f) ** 1.2
                row.append((card.vert(co, shade_normal(surface, 5.0), wind, phase, 0.78 + 0.22 * min(1.0, y / 60.0)),
                            (x, y)))                  # grimy where glued at the top
            rows.append(row)
        for r0, r1 in zip(rows, rows[1:]):
            (a0, p0), (b0, q0) = r0
            (a1, p1), (b1, q1) = r1
            card.face((a0, b0, b1, a1), (lw.uv('Tatters', *p0), lw.uv('Tatters', *q0), lw.uv('Tatters', *q1),
                                         lw.uv('Tatters', *p1)), mat)


def web_tatters(name='Web_Tatters', seed=16):
    card = Card(name)
    tatter_strips(card, [(0, -0.5, None), (1, -0.17, None), (3, 0.15, None), (4, 0.48, None)], random.Random(seed))
    return card_props(card.build([WEB]))


def web_strands(name='Web_Strands', seed=17):
    rnd = random.Random(seed)
    card = Card(name)
    lanes = ('Cord', 'Tufted', 'Rope', 'Tufted', 'Cord', 'Tufted', 'Rope')
    for k, lane in enumerate(lanes):
        x = -0.6 + 1.2 * k / (len(lanes) - 1) + rnd.uniform(-0.05, 0.05)
        length = rnd.uniform(0.8, 2.6)
        pts = hanging_thread((x, rnd.uniform(-0.04, 0.04), 0.0), length, (rnd.uniform(-0.08, 0.08), rnd.uniform(-0.08, 0.08)),
                             steps=8)
        pts = wiggle(pts, 0.012 + 0.008 * length, rnd, waves=1.0 + 0.5 * length, keep_ends=False)
        winds = [0.95 * (i / (len(pts) - 1)) ** 1.2 for i in range(len(pts))]
        # Grimy where it's glued at the top, fraying to nothing at its free end.
        occs = [0.8 + 0.2 * min(1.0, i / 2.0) for i in range(len(pts))]
        cord(card, pts, 0.04 if lane != 'Rope' else 0.045, lane, rnd.uniform(0.0, 1500.0), winds, rnd.random(),
             occ=occs, taper=0.15)
    return card_props(card.build([WEB]))


def web_line(name='Web_Line', seed=18, length=4.0, sag=0.1):
    rnd = random.Random(seed)
    card = Card(name)
    pts, winds = [], []
    for i in range(9):
        s = i / 8.0
        pts.append((length * s, 0.0, -4.0 * sag * s * (1.0 - s)))
        winds.append(0.3 * 4.0 * s * (1.0 - s))
    cord(card, pts, 0.045, 'Rope', rnd.uniform(0.0, 600.0), winds, rnd.random())
    return card_props(card.build([WEB]))


# The den mouth's lining, fitted to Den Rock's den (measured on SM_DenRock): its loop round the mouth at the face, as
# (x, height over the den floor), from the floor at the left round the arch and back along the floor. It sits ~0.2 m
# inside the rock; the left side of the arch stands higher, as the den's does.
FUNNEL_LOOP = [(-3.2, 0.03), (-3.18, 1.4), (-3.02, 2.7), (-2.62, 3.75), (-1.86, 4.45), (-0.7, 4.62), (0.5, 4.42),
               (1.5, 4.02), (2.22, 3.48), (2.58, 2.62), (3.22, 1.4), (3.42, 0.03), (1.7, 0.03), (0.0, 0.03),
               (-1.6, 0.03)]
FUNNEL_RINGS = [(-0.62, 1.06, 1.0), (-0.2, 1.02, 1.0), (0.45, 1.0, 1.0), (1.25, 0.94, 0.93), (2.0, 0.86, 0.83),
                (2.6, 0.8, 0.73)]          # (depth y, scale across, scale up): from just inside the jambs, narrowing


def web_funnel(name='Web_Funnel', seed=19):
    rnd = random.Random(seed)
    card = Card(name)
    phase = rnd.random()
    loop = FUNNEL_LOOP + [FUNNEL_LOOP[0]]
    s = [0.0]
    for a, b in zip(loop, loop[1:]):
        s.append(s[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    us = [d * lw.PX_M / lw.SIZE for d in s]
    top = lw.REGIONS['Sheet'][1]
    depth_total = FUNNEL_RINGS[-1][0] - FUNNEL_RINGS[0][0]
    grid = []
    for y, sx, sz in FUNNEL_RINGS:
        f = (y - FUNNEL_RINGS[0][0]) / depth_total
        v = 1.0 - (top + 4.0 + f * 376.0) / lw.SIZE
        dark = max(0.0, min(1.0, (y + 0.3) / 6.8))
        occ = 1.0 - 0.8 * dark * dark * (3.0 - 2.0 * dark)          # Den Rock's own darkening into the den
        row = []
        for k, (x, h) in enumerate(loop):
            co = Vector((x * sx, y, h * sz if h > 0.1 else (h - 0.08 if y < -0.6 else h)))
            axis = Vector((0.0, y, 2.2 * sz))
            inward = (axis - co)
            inward.y = 0.0
            row.append((card.vert(co, shade_normal(inward, 0.3 + 0.6 * (1.0 - f)), 0.08 * (1.0 - f) if h > 0.1 else 0.0,
                                  phase, occ * (0.78 + 0.22 * min(1.0, h / 1.2))),        # grimy low down
                        (us[k], v)))
        grid.append(row)
    for r0, r1 in zip(grid, grid[1:]):
        for k in range(len(loop) - 1):
            (a, ua), (b, ub) = r0[k], r0[k + 1]
            (c, uc), (d, ud) = r1[k + 1], r1[k]
            card.face((a, b, c, d), (ua, ub, uc, ud))
    # Anchor lines from the rim out onto the face round the mouth.
    for x, h, out in ((-3.0, 4.1, (-1.0, -0.6, 0.6)), (-1.2, 4.65, (-0.3, -0.9, 0.5)), (0.9, 4.45, (0.4, -1.0, 0.5)),
                      (2.3, 3.5, (0.9, -0.5, 0.7)), (-3.4, 1.6, (-1.0, -0.5, 0.1)), (3.6, 1.4, (1.0, -0.4, 0.15)),
                      (-3.6, 0.3, (-0.9, -0.7, -0.05)), (3.7, 0.3, (0.9, -0.8, -0.05))):
        start = Vector((x * 1.06, -0.62, h))
        d = Vector(out).normalized() * rnd.uniform(0.8, 1.4)
        pts = wiggle(catenary(start, start + d, 0.04, 5), 0.01, rnd)
        cord(card, pts, 0.05, rnd.choice(('Rope', 'Cord')), rnd.uniform(0.0, 1500.0), 0.0, phase, crossed=False,
             occ=[1.0, 0.95, 0.9, 0.85, 0.8, 0.72])
    return card_props(card.build([WEB]))


def web_branch_wrap(name='Web_BranchWrap', seed=20, length=1.3, branch=0.07, sides=8, rings=10):
    rnd = random.Random(seed)
    card = Card(name)
    phase = rnd.random()
    top = lw.REGIONS['Wrap'][1]
    rows = []
    for i in range(rings + 1):
        s = i / rings
        x = length * s
        pinch = math.sin(math.pi * s) ** 0.6
        loose = 0.014 + 0.055 * pinch * (0.75 + 0.5 * noise.noise(Vector((s * 3.0, 0.2, seed))))
        r = branch + 0.006 + loose
        drop = loose * 0.75                 # the loose silk hangs below the branch
        ring = []
        for k in range(sides + 1):
            a = 2.0 * math.pi * k / sides
            wobble = 1.0 + 0.12 * noise.noise(Vector((s * 4.0, k * 0.7, seed + 3.0)))
            y, z = math.cos(a) * r * wobble, math.sin(a) * r * wobble - drop
            co = Vector((x, y, z))
            out = Vector((0.0, y, z + drop)).normalized()
            u = x * lw.PX_M / lw.SIZE
            v = 1.0 - (top + lw.WRAP_PAD + lw.WRAP_PERIOD * k / sides) / lw.SIZE
            ring.append((card.vert(co, shade_normal(out, 0.35), 0.1 * pinch, phase, 0.85 + 0.15 * max(0.0, -out.z)),
                         (u + 0.03, v)))
        rows.append(ring)
    for r0, r1 in zip(rows, rows[1:]):
        for k in range(sides):
            (a, ua), (b, ub) = r0[k], r0[k + 1]
            (c, uc), (d, ud) = r1[k + 1], r1[k]
            card.face((a, b, c, d), (ua, ub, uc, ud))
    # Short threads hanging off its underside (long tatters are Web_Tatters' job: a wrap scaled to a thick branch
    # would scale them with it).
    for x, length_, lane in ((0.22, 0.32, 'Tufted'), (0.6, 0.45, 'Cord'), (0.95, 0.25, 'Tufted'), (1.15, 0.38, 'Cord')):
        pts = hanging_thread((x, rnd.uniform(-0.03, 0.03), -0.1), length_, (rnd.uniform(-0.05, 0.05), 0.0), steps=6)
        pts = wiggle(pts, 0.012, rnd, keep_ends=False)
        cord(card, pts, 0.035, lane, rnd.uniform(0.0, 1500.0), [0.1 + 0.8 * i / 6 for i in range(7)], rnd.random(),
             taper=0.2)
    return card_props(card.build([WEB]))


# EggSac_B, which the sling holds: a body of these radii round SAC_B_CENTER (its back toward +y flattened).
SAC_B_CENTER = Vector((0.0, 0.0, 0.7))
SAC_B_RADII = (0.75, 0.62, 0.79)
WALL_Y = 0.66


def web_sling(name='Web_Sling', seed=21, segs=14, rings=5):
    rnd = random.Random(seed)
    card = Card(name)
    phase = rnd.random()
    top = lw.REGIONS['Sheet'][1]
    rx, ry, rz = (SAC_B_RADII[0] + 0.05, SAC_B_RADII[1] + 0.05, SAC_B_RADII[2] + 0.05)
    grid = []
    for j in range(rings + 1):
        t = j / rings
        row = []
        for k in range(segs + 1):
            a = math.radians(-152.0 + 304.0 * k / segs)        # round the front, from the wall on one side to the other
            front = math.cos(a)                                   # 1 at the front, toward -y
            theta = math.radians(26.0 + t * (46.0 + 30.0 * (1.0 - front) * 0.5 + 6.0 * noise.noise(Vector((k * 0.6, 0.4, seed)))))
            d = Vector((math.sin(a), -math.cos(a), 0.0))
            co = SAC_B_CENTER + Vector((d.x * rx * math.sin(theta), d.y * ry * math.sin(theta) * (0.86 if d.y > 0 else 1.0),
                                        -rz * math.cos(theta)))
            co.y = min(co.y, WALL_Y - 0.01)
            outward = co - SAC_B_CENTER
            u = 0.17 + (k / segs) * 4.4 * lw.PX_M / lw.SIZE
            v = 1.0 - (top + 380.0 - 376.0 * t) / lw.SIZE
            # Lit as an open dish, not by its outer face: from below it would go as dark as the sac's shadow.
            row.append((card.vert(co, shade_normal(outward.normalized() * 0.45 + UP, 0.0), 0.0, phase, 0.9), (u, v)))
        grid.append(row)
    for r0, r1 in zip(grid, grid[1:]):
        for k in range(segs):
            (a, ua), (b, ub) = r0[k], r0[k + 1]
            (c, uc), (d, ud) = r1[k + 1], r1[k]
            card.face((a, b, c, d), (ua, ub, uc, ud))
    # Lines from the rim up and back to the wall.
    rim = grid[-1]
    for k, rise in ((1, 0.9), (4, 1.2), (7, 1.05), (10, 1.25), (13, 0.85)):
        start = card.co[rim[k][0]].copy()
        end = Vector((start.x * 1.15 + rnd.uniform(-0.2, 0.2), WALL_Y - 0.01, start.z + rise))
        pts = wiggle(catenary(start, end, 0.07, 6), 0.008, rnd)
        cord(card, pts, 0.04, 'Cord', rnd.uniform(0.0, 1500.0), 0.0, phase, crossed=False,
             occ=[1.0, 1.0, 0.95, 0.9, 0.85, 0.8, 0.7])         # grimy where it's glued to the rock
    obj = card_props(card.build([WEB]))
    lm.socket(obj, 'Sac', (0.0, 0.0, 0.0))
    return obj


def web_snare(name='Web_Snare', seed=22):
    """The tangle twice, crossed 55 degrees, each a fan over the tangle's convex outline (a grid of cells trimmed to its
    threads came out larger: the tangle's threads cross nearly every cell, and the border cells stick out past it), and
    the cord the lantern hangs from."""
    rnd = random.Random(seed)
    lay = lw.tangle_layout()
    cx, cy = lay['center']
    card = Card(name)
    for turn in (0.0, 55.0):
        a = math.radians(turn)
        side = Vector((math.cos(a), math.sin(a), 0.0))
        surface = side.cross(UP)
        phase = rnd.random()
        n = shade_normal(surface, 5.0)

        def at(x, y):
            return side * ((x - cx) * PX) + UP * ((cy - y) * PX)
        hub = card.vert(at(cx, cy), n, 0.3, phase)
        ring = [(card.vert(at(x, y), n, 0.0, phase), (x, y)) for x, y in lay['hull']]
        for k in range(len(ring)):
            (p, pa), (q, pb) = ring[k], ring[(k + 1) % len(ring)]
            card.face((hub, p, q), (lw.uv('Tangle', cx, cy), lw.uv('Tangle', *pa), lw.uv('Tangle', *pb)))
    hang = Vector((0.06, -0.02, -0.34))
    cord(card, [Vector((0.02, 0.0, 0.08)), Vector((0.05, -0.01, -0.12)), hang + Vector((0.0, 0.0, 0.03))], 0.035,
         'Tufted', 300.0, [0.25, 0.3, 0.35], rnd.random())
    obj = card_props(card.build([WEB]))
    lm.socket(obj, 'Lantern', tuple(hang))
    return obj


# DeadTree_A's trunk and five main limbs (Art/Models/Vegetation/DeadTree.py, seed 71: the Webwood's trees), keyed by
# the order that script builds its tubes in (the trunk is 0), measured along each axis from its foot (a limb's starts
# where it leaves the trunk), about every 0.28 m: (x, y, z, radius). Web_Crown is fitted to them; if that tree
# changes, measure them again.
DEAD_TREE_LIMBS = {
    0: [(0.000, 0.000, -0.350, 0.340), (-0.032, 0.038, -0.130, 0.323), (-0.030, 0.050, 0.167, 0.307),
        (0.030, 0.018, 0.559, 0.288), (0.141, -0.041, 1.104, 0.264), (0.262, -0.077, 1.598, 0.244),
        (0.361, -0.071, 2.021, 0.228), (0.419, -0.035, 2.376, 0.215), (0.442, 0.004, 2.694, 0.203),
        (0.456, 0.010, 3.035, 0.191), (0.468, 0.015, 3.405, 0.179), (0.480, 0.044, 3.790, 0.166),
        (0.490, 0.089, 4.179, 0.153), (0.484, 0.141, 4.558, 0.141), (0.439, 0.193, 4.913, 0.129)],
    1: [(0.366, -0.068, 2.051, 0.177), (0.540, -0.120, 2.269, 0.161), (0.731, -0.164, 2.473, 0.145),
        (0.831, -0.117, 2.734, 0.129), (0.920, -0.101, 3.003, 0.113), (1.020, -0.057, 3.264, 0.096),
        (1.210, -0.000, 3.466, 0.080), (1.387, 0.079, 3.673, 0.064), (1.581, 0.139, 3.870, 0.048),
        (1.784, 0.182, 4.063, 0.032), (1.959, 0.211, 4.284, 0.016), (2.141, 0.263, 4.495, 0.0)],
    6: [(0.422, -0.030, 2.419, 0.155), (0.266, 0.082, 2.628, 0.141), (0.142, 0.238, 2.830, 0.127),
        (0.014, 0.367, 3.048, 0.113), (-0.109, 0.456, 3.288, 0.099), (-0.210, 0.512, 3.547, 0.085),
        (-0.382, 0.615, 3.748, 0.070), (-0.540, 0.718, 3.960, 0.056), (-0.711, 0.796, 4.173, 0.042),
        (-0.844, 0.887, 4.407, 0.028), (-0.975, 0.964, 4.647, 0.014), (-1.074, 1.020, 4.907, 0.0)],
    15: [(0.454, 0.009, 2.996, 0.130), (0.487, -0.140, 3.239, 0.118), (0.496, -0.304, 3.475, 0.106),
         (0.537, -0.442, 3.724, 0.094), (0.586, -0.574, 3.975, 0.083), (0.644, -0.704, 4.225, 0.071),
         (0.676, -0.802, 4.493, 0.059), (0.632, -0.962, 4.728, 0.047), (0.615, -1.130, 4.961, 0.035),
         (0.619, -1.271, 5.212, 0.024), (0.598, -1.301, 5.497, 0.012), (0.598, -1.339, 5.782, 0.0)],
    29: [(0.490, 0.092, 4.202, 0.111), (0.327, 0.083, 4.424, 0.101), (0.144, -0.030, 4.597, 0.091),
         (-0.025, -0.151, 4.779, 0.081), (-0.189, -0.245, 4.980, 0.070), (-0.361, -0.309, 5.186, 0.060),
         (-0.522, -0.388, 5.396, 0.050), (-0.695, -0.459, 5.598, 0.040), (-0.854, -0.546, 5.806, 0.030),
         (-1.029, -0.615, 6.008, 0.020), (-1.214, -0.656, 6.208, 0.010), (-1.387, -0.682, 6.422, 0.0)],
}
# Web_Crown's tents, one in the fork where each of these limbs leaves the trunk: (limb, how far it climbs the trunk
# (m), how far out along the limb (m), how far it bulges to either side at the top (m)).
CROWN_TENTS = ((1, 1.2, 1.5, 0.28), (6, 1.1, 1.4, 0.26), (15, 1.0, 1.3, 0.24), (29, 0.6, 1.1, 0.2))


def _limb_at(points, s):
    """The point at s metres along a limb's axis (DEAD_TREE_LIMBS) and its radius there."""
    acc = 0.0
    for i in range(len(points) - 1):
        a, b = Vector(points[i][:3]), Vector(points[i + 1][:3])
        seg = (b - a).length
        if s <= acc + seg or i == len(points) - 2:
            f = min(max((s - acc) / max(seg, 1e-9), 0.0), 1.0)
            return a.lerp(b, f), points[i][3] + (points[i + 1][3] - points[i][3]) * f
        acc += seg


def _limb_length(points):
    return sum((Vector(b[:3]) - Vector(a[:3])).length for a, b in zip(points, points[1:]))


def _arc_at_height(points, z):
    """How far along a limb's axis it reaches height z."""
    acc = 0.0
    for a, b in zip(points, points[1:]):
        a, b = Vector(a[:3]), Vector(b[:3])
        seg = (b - a).length
        if b.z >= z:
            return acc + seg * min(max((z - a.z) / max(b.z - a.z, 1e-9), 0.0), 1.0)
        acc += seg
    return acc


def web_crown(name='Web_Crown', seed=23):
    """Tent webs filling the forks of DeadTree_A's crown where its main limbs leave the trunk (CROWN_TENTS): each two
    curved cards bulging to either side of the fork like a pouch, glued along the trunk and along the limb, dense ivory
    silk (the Tent cell) fraying into fibres at its open top, a few threads running on up the trunk, out along the
    limb and up toward the twigs. Dense masses, so the tree reads as webbed from 30 m."""
    rnd = random.Random(seed)
    card = Card(name)
    trunk = DEAD_TREE_LIMBS[0]
    trunk_len = _limb_length(trunk)
    rows, cols = 7, 5
    for limb, climb, reach, belly in CROWN_TENTS:
        pts = DEAD_TREE_LIMBS[limb]
        base = Vector(pts[0][:3])
        s0 = _arc_at_height(trunk, base.z)
        heading = Vector(pts[-1][:3]) - base
        heading.z = 0.0
        heading.normalize()
        phase = rnd.random()

        def edges(u, pts=pts, s0=s0, heading=heading, climb=climb, reach=reach):
            """The fork's glued edges at u (0 the crotch, 1 the tent's top): on the trunk's face toward the limb, and
            on the limb's face toward the trunk."""
            pa, ra = _limb_at(trunk, min(s0 + 0.06 + climb * u, trunk_len - 0.05))
            pb, rb = _limb_at(pts, 0.22 + reach * u)
            back = pa - pb
            back.z = 0.0
            back = back.normalized() if back.length > 1e-6 else -heading
            return pa + heading * (ra * 0.95), pb + back * (rb * 0.95)
        normals = []
        for i in range(rows):
            u = i / (rows - 1)
            a, b = edges(u)
            a2, b2 = edges(u + 0.05)
            n_v = (b - a).cross((a2 + b2) - (a + b)).normalized()
            normals.append(n_v)
        for side in (1.0, -1.0):
            grid = []
            for i in range(rows):
                u = i / (rows - 1)
                a, b = edges(u)
                row = []
                for j in range(cols):
                    t = j / (cols - 1)
                    bell = math.sin(math.pi * t)
                    co = a.lerp(b, t) + normals[i] * (side * (0.05 + belly * u ** 0.8) * bell) - UP * (0.05 * bell * u)
                    occ = 0.8 + 0.2 * min(1.0, u * 2.5)                    # grimy in the crotch
                    row.append((card.vert(co, shade_normal(normals[i], 2.0), 0.15 * u * u * bell, phase, occ),
                                lw.uv('Tent', 1023.0 * u + 0.5, 255.0 * t + 0.5)))
                grid.append(row)
            for r0, r1 in zip(grid, grid[1:]):
                for j in range(cols - 1):
                    (va, ua), (vb, ub) = r0[j], r0[j + 1]
                    (vc, uc), (vd, ud) = r1[j + 1], r1[j]
                    card.face((va, vb, vc, vd), (ua, ub, uc, ud))
        # Threads on from its top: up the trunk, out along the limb, and up from its middle toward the twigs.
        a1, b1 = edges(1.0)
        a2, b2 = edges(1.45)
        mid = a1.lerp(b1, 0.5) + normals[-1] * (rnd.choice((-1.0, 1.0)) * belly * 0.6)
        for start, end in ((a1, a2), (b1, b2), (mid, (a2 + b2) * 0.5 + UP * rnd.uniform(0.25, 0.45))):
            line = wiggle(catenary(start, end, 0.03 + 0.03 * rnd.random(), 4), 0.01, rnd)
            cord(card, line, 0.035, 'Cord', rnd.uniform(0.0, 1500.0), [0.0, 0.08, 0.12, 0.08, 0.0], rnd.random(),
                 occ=[0.9, 1.0, 1.0, 1.0, 0.85])
    return card_props(card.build([WEB]))


# --- Egg sacs ---

def _ellipsoid_sdf(points, center, radii, rotation):
    """Distance (approximate, negative inside) from points (n x 3) to an ellipsoid."""
    p = (points - center) @ rotation          # into the ellipsoid's frame
    r = np.asarray(radii)
    k0 = np.linalg.norm(p / r, axis=1)
    k1 = np.linalg.norm(p / (r * r), axis=1)
    return k0 * (k0 - 1.0) / np.maximum(k1, 1e-9)


def _capsule_sdf(points, a, b, radius):
    ab = b - a
    t = np.clip(((points - a) @ ab) / max(ab @ ab, 1e-12), 0.0, 1.0)
    return np.linalg.norm(points - (a + t[:, None] * ab), axis=1) - radius


class Curled:
    """A spider curled up in its sac, at 0.8 of a Meadow Wolf (Spider.py): abdomen, cephalothorax and head, eight legs
    folded over and under the body. center (m), heading (degrees, the way its head points, round Z from +X), pitch
    and roll (degrees). Only used for the shapes and shadows it makes through the silk."""

    def __init__(self, center, heading, pitch=0.0, roll=0.0, scale=0.8):
        rot = (Matrix.Rotation(math.radians(heading), 3, 'Z') @ Matrix.Rotation(math.radians(-pitch), 3, 'Y') @
               Matrix.Rotation(math.radians(roll), 3, 'X'))
        self.center = Vector(center)
        self.rot = rot
        self.scale = scale
        r = np.array(rot)

        def place(p):
            return np.array(self.center + rot @ (Vector(p) * scale))
        self.ellipsoids = [(place((-0.42, 0.0, 0.03)), np.array((0.5, 0.37, 0.32)) * scale, r),
                           (place((0.2, 0.0, -0.03)), np.array((0.34, 0.28, 0.19)) * scale, r),
                           (place((0.48, 0.0, -0.08)), np.array((0.17, 0.16, 0.12)) * scale, r)]
        self.capsules = []
        for side in (-1.0, 1.0):
            for k in range(4):
                hip = (0.3 - 0.13 * k, side * 0.2, 0.0)
                knee = (0.38 - 0.3 * k, side * 0.4, 0.2 - 0.03 * k)
                tip = (0.2 - 0.32 * k, side * 0.16, -0.26)
                self.capsules.append((place(hip), place(knee), 0.065 * scale))
                self.capsules.append((place(knee), place(tip), 0.05 * scale))

    def sdf(self, points):
        d = np.full(len(points), np.inf)
        for c, radii, rot in self.ellipsoids:
            d = np.minimum(d, _ellipsoid_sdf(points, c, radii, rot))
        for a, b, radius in self.capsules:
            d = np.minimum(d, _capsule_sdf(points, a, b, radius))
        return d

    def socket(self, obj, name):
        """A spawn socket at its body, facing the way its head points (a socket's front is its -Y)."""
        heading = self.rot @ Vector((1.0, 0.0, 0.0))
        yaw = math.degrees(math.atan2(heading.y, heading.x)) + 90.0
        lm.socket(obj, name, tuple(self.center), (0.0, 0.0, yaw))


def resample(profile, count):
    """count points spaced evenly along a profile polyline [(radius, z)]."""
    pts = [Vector((r, z)) for r, z in profile]
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    out = []
    for i in range(count):
        d = s[-1] * i / (count - 1)
        k = max(j for j in range(len(s) - 1) if s[j] <= d + 1e-9) if d > 0 else 0
        k = min(k, len(s) - 2)
        f = (d - s[k]) / max(s[k + 1] - s[k], 1e-9)
        out.append(pts[k].lerp(pts[k + 1], f))
    return out


class Sac:
    """An egg sac's skin: a lathe of a profile [(radius, z)] from the bottom pole up, segs round, its spiders pushing
    lumps into it. The skin keeps clear of every spider by `clear` (the silk is stretched over them), then the bumps
    are softened; shadow is how close each vertex lies to a spider under the silk."""

    def __init__(self, profile, segs, rings, spiders, clear=0.06, squash_y=1.0, squash_back=1.0, wrinkle=0.02,
                 seed=0, closed_top=False, lean=None, pleats=None, wound=0.022):
        """lean (dx, z0, z1): the axis bends dx along X between heights z0 and z1 (a lobe leaning off). pleats
        (count, depth, start): the silk gathered into count folds that deepen from the fraction start of the height
        up to the neck, depth a share of the radius there. wound: how high (m) the wound bundles stand."""
        self.segs = segs
        prof = resample(profile, rings + 1)
        self.prof = prof
        rows, centers = [], []
        for j, q in enumerate(prof):
            dx = 0.0
            if lean is not None:
                t = min(max((q.y - lean[1]) / (lean[2] - lean[1]), 0.0), 1.0)
                dx = lean[0] * t * t * (3.0 - 2.0 * t)
            centers.append((dx, 0.0, q.y))
            row = []
            for k in range(segs):
                a = 2.0 * math.pi * k / segs
                x, y = math.cos(a) * q.x, math.sin(a) * q.x * squash_y
                if y > 0.0:
                    y *= squash_back
                row.append((x + dx, y, q.y))
            rows.append(row)
        P = np.array(rows, dtype=np.float64)                      # (rings+1, segs, 3)
        C = np.array(centers, dtype=np.float64)[:, None, :]
        # Outward directions (for the lumps): from the axis at each ring's height, the pole straight down/up.
        out = P - C
        lengths = np.linalg.norm(out, axis=2, keepdims=True)
        out = np.where(lengths > 1e-6, out / np.maximum(lengths, 1e-9), np.array([0.0, 0.0, -1.0]))
        flat = P.reshape(-1, 3)
        d = np.min([s.sdf(flat) for s in spiders], axis=0) if spiders else np.full(len(flat), 1.0)
        push = np.maximum(clear - d, 0.0).reshape(P.shape[:2])
        for _ in range(3):                                         # soften the bumps over their neighbours
            push = 0.5 * push + 0.125 * (np.roll(push, 1, axis=1) + np.roll(push, -1, axis=1) +
                                         np.vstack([push[:1], push[:-1]]) + np.vstack([push[1:], push[-1:]]))
        push[0] = push[0].mean()
        rnd = random.Random(seed)
        off = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
        wr = np.array([[noise.noise(Vector(P[j, k]) * 2.2 + off) + 0.4 * noise.noise(Vector(P[j, k]) * 6.0 - off)
                        for k in range(segs)] for j in range(len(rows))])
        fold = np.zeros(P.shape[:2])
        if pleats is not None:
            count, depth, start = pleats
            ph = rnd.uniform(0.0, 6.28)
            for j in range(len(rows)):
                f = j / (len(rows) - 1)
                t = min(max((f - start) / (1.0 - start), 0.0), 1.0)
                if t <= 0.0:
                    continue
                r = float(lengths[j].mean())
                for k in range(segs):
                    a = 2.0 * math.pi * k / segs
                    fold[j, k] = depth * r * t * t * (3.0 - 2.0 * t) * math.cos(count * a + ph + 0.8 * f)
        # Wound on in bundles: two families of ridges spiralling round it the opposite ways, lumpy along their length,
        # so it reads as silk wound round and round, not a sewn bag (fading out toward the poles).
        bands = np.zeros(P.shape[:2])
        if wound:
            ph1, ph2 = rnd.uniform(0.0, 6.28), rnd.uniform(0.0, 6.28)
            for j in range(len(rows)):
                fade = min(1.0, float(lengths[j].mean()) / 0.25)
                for k in range(segs):
                    a = 2.0 * math.pi * k / segs
                    z = float(P[j, k, 2])
                    b1 = max(math.sin(3.0 * a + 2.0 * math.pi * z / 0.5 + ph1), 0.0)
                    b2 = max(math.sin(-4.0 * a + 2.0 * math.pi * z / 0.62 + ph2), 0.0)
                    lump = 0.6 + 0.4 * noise.noise(Vector((a * 1.3, z * 2.0, seed + 7.0)))
                    bands[j, k] = wound * fade * lump * (b1 * b1 + b2 * b2 - 0.5)
        P = P + out * (push + wrinkle * wr + fold + bands)[..., None]
        if closed_top:
            P[-1] = P[-1].mean(axis=0)
        P[0] = P[0].mean(axis=0)
        self.P = P
        flat = P.reshape(-1, 3)
        dist = np.min([s.sdf(flat) for s in spiders], axis=0) if spiders else np.full(len(flat), 1.0)
        self.shadow = np.exp(-np.maximum(dist, 0.0) / 0.085).reshape(P.shape[:2])
        self.closed_top = closed_top

    def add(self, card, mat, v_repeats=3, u_offset=0.0):
        """Faces of the skin on the Sac cell: U once round it (the cell tiles along U), V up it, repeating v_repeats
        times (it must divide the rings, so each repeat starts on a ring). A pole is one vertex whose corners each take
        the UV of their own face's middle (no fan of UVs sharing one tangent). Returns the vertex grid."""
        top = lw.REGIONS['Sac'][1] + lw.SAC_PAD
        rows = len(self.P)
        per = (rows - 1) // v_repeats
        assert per * v_repeats == rows - 1, 'v_repeats must divide the rings'
        center = Vector((0.0, 0.0, float(self.P[:, :, 2].mean())))
        verts = []
        for j in range(rows):
            pole = j == 0 or (j == rows - 1 and self.closed_top)
            row = []
            for k in range(self.segs):
                if pole and k:
                    row.append(row[0])
                    continue
                co = Vector(self.P[j, k])
                n = (co - center) if pole else co - Vector((0.0, 0.0, co.z))
                row.append(card.vert(co, n, 0.0, 0.0, 1.0))
            verts.append(row)

        def v_at(j, upper):
            local = j % per
            if upper and local == 0:
                local = per
            return 1.0 - (top + lw.SAC_PERIOD * (1.0 - local / per)) / lw.SIZE

        for j in range(rows - 1):
            va, vb = v_at(j, False), v_at(j + 1, True)
            for k in range(self.segs):
                k1 = (k + 1) % self.segs
                u0, u1 = u_offset + k / self.segs, u_offset + (k + 1) / self.segs
                if j == 0:
                    card.face((verts[0][k], verts[1][k1], verts[1][k]), (((u0 + u1) * 0.5, va), (u1, vb), (u0, vb)), mat)
                elif j == rows - 2 and self.closed_top:
                    card.face((verts[j][k], verts[j][k1], verts[j + 1][k]), ((u0, va), (u1, va), ((u0 + u1) * 0.5, vb)), mat)
                else:
                    card.face((verts[j][k], verts[j][k1], verts[j + 1][k1], verts[j + 1][k]),
                              ((u0, va), (u1, va), (u1, vb), (u0, vb)), mat)
        return verts


def silk_rope(card, points, radius, sides, mat, twist=1.5, u0=0.0):
    """A twisted rope of silk (the sac's stalk) on the Sac cell: a tube whose UVs spiral round it."""
    pts = [Vector(p) for p in points]
    radii = list(radius) if isinstance(radius, (list, tuple)) else [radius] * len(pts)
    top = lw.REGIONS['Sac'][1] + lw.SAC_PAD
    rows, params = [], []
    s = 0.0
    for i, p in enumerate(pts):
        if i:
            s += (p - pts[i - 1]).length
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        ref = Vector((1.0, 0.0, 0.0)) if abs(t.x) < 0.9 else Vector((0.0, 1.0, 0.0))
        b1 = t.cross(ref).normalized()
        b2 = t.cross(b1)
        row = []
        for k in range(sides):
            a = 2.0 * math.pi * k / sides
            n = b1 * math.cos(a) + b2 * math.sin(a)
            row.append(card.vert(p + n * radii[i], n, 0.0, 0.0, 1.0))
        rows.append(row)
        params.append((s * twist * 0.08, 1.0 - (top + lw.SAC_PERIOD * min(s / 0.6, 1.0) * 0.9 + 10.0) / lw.SIZE))
    for i in range(len(rows) - 1):
        (s0, v0), (s1, v1) = params[i], params[i + 1]
        for k in range(sides):
            k1 = (k + 1) % sides
            u_a, u_b = u0 + k / sides * 0.08, u0 + (k + 1) / sides * 0.08
            card.face((rows[i][k], rows[i][k1], rows[i + 1][k1], rows[i + 1][k]),
                      ((u_a + s0, v0), (u_b + s0, v0), (u_b + s1, v1), (u_a + s1, v1)), mat)
    return rows


def finish_sac(card, name, spiders_shadow, hull_points, sockets, ground, ao_distance=0.7, lods='50,25', custom=False):
    """custom: keep the card's own vertex normals (the burst sac, whose flaps fold sharply and whose frayed edges are
    lit like cards); otherwise the skin is smoothed from its shape."""
    materials = [SILK, WEB] if 1 in card.mats else [SILK]       # no empty Web slot on a sac without threads
    if custom:
        obj = card.build(materials)
    else:
        obj = card.build(materials, custom_normals=False)
        lm.smooth(obj, 80.0)
    for points in hull_points:
        lp.hull_points(obj, points)
    lt.bake_vertex_ao(obj, samples=32, distance=ao_distance, ground=ground)
    if spiders_shadow is not None:
        mesh = obj.data
        raw = lt._read_col(mesh)
        loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loop_vert)
        raw[:, 3] *= np.asarray(spiders_shadow, np.float32)[loop_vert]
        lt._write_col(mesh, raw)
    for socket_name, location, rotation in sockets:
        lm.socket(obj, socket_name, location, rotation)
    obj['Nanite'] = 0
    obj['LODs'] = lods
    return obj


def support_points(points, count=48):
    """The points of a cloud that stick out furthest in count directions spread over the sphere (a hull's corners)."""
    P = np.asarray(points, np.float64)
    k = np.arange(count) + 0.5
    z = 1.0 - 2.0 * k / count
    r = np.sqrt(np.clip(1.0 - z * z, 0.0, 1.0))
    phi = k * math.pi * (3.0 - math.sqrt(5.0))
    dirs = np.stack([r * np.cos(phi), r * np.sin(phi), z], axis=1)
    picks = sorted(set(int(i) for i in np.argmax(P @ dirs.T, axis=0)))
    return [tuple(P[i]) for i in picks]


def surface_cord(card, points, normals, width, lane, u0, mat=1):
    """A thread lying on a surface: one strip turned flat to it (normals: the surface's, outward, per point)."""
    pts = [Vector(p) for p in points]
    va, vb, share = lane_rows(lane)
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    rows = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        n = Vector(normals[i]).normalized()
        side = n.cross(t).normalized() * (width * share * 0.5)
        rows.append((card.vert(p - side, n, 0.0, 0.0, 1.0), card.vert(p + side, n, 0.0, 0.0, 1.0),
                     (u0 + s[i] * lw.PX_M) / lw.SIZE))
    for (a0, b0, u_0), (a1, b1, u_1) in zip(rows, rows[1:]):
        card.face((a0, b0, b1, a1), ((u_0, va), (u_0, vb), (u_1, vb), (u_1, va)), mat)


def wound_threads(card, sac, rnd, count, rows=(0.2, 0.85), lift=0.012, turns=(0.35, 0.9)):
    """Threads wound round a sac's skin in loose spirals, lying on it: it reads as spun silk."""
    P = sac.P
    n_rows, segs = P.shape[0], P.shape[1]
    for _ in range(count):
        j0, j1 = rnd.uniform(*rows) * (n_rows - 1), rnd.uniform(*rows) * (n_rows - 1)
        a0 = rnd.uniform(0.0, segs)
        spin = rnd.uniform(*turns) * rnd.choice((-1.0, 1.0))
        pts, nors = [], []
        for i in range(17):
            f = i / 16.0
            j = j0 + (j1 - j0) * f
            k = a0 + spin * segs * f
            jl, kl = int(math.floor(j)), int(math.floor(k))
            fj, fk = j - jl, k - kl
            jl = min(max(jl, 1), n_rows - 2)
            corners = [Vector(P[jl + dj, (kl + dk) % segs]) for dj in (0, 1) for dk in (0, 1)]
            q = (corners[0].lerp(corners[1], fk)).lerp(corners[2].lerp(corners[3], fk), fj)
            n = (corners[1] - corners[0]).cross(corners[2] - corners[0])
            axis = Vector((0.0, 0.0, q.z))
            if n.dot(q - axis) < 0.0:
                n = -n
            n.normalize()
            pts.append(q + n * lift)
            nors.append(n)
        surface_cord(card, pts, nors, rnd.uniform(0.028, 0.04), rnd.choice(('Cord', 'Rope')), rnd.uniform(0.0, 1400.0))


def halo(card, sac, rnd, segs=15, rings=6, lift=0.04, top=0.88, repeats=5):
    """A thin fuzzy halo of loose silk just off a sac's skin, breaking its outline (the Wrap cell's wound strands, on
    Web): a low shell lift (m) outside the skin's furthest reach nearby, from under its bottom up to `top` of its
    height, V round it `repeats` times (the strands run round it), U up it. Its faces turn outward, so the occlusion
    bake sees through them from the skin."""
    P = sac.P
    skin = P.reshape(-1, 3)
    ring_z = P[:, :, 2].mean(axis=1)
    ring_c = P[:, :, :2].mean(axis=1)
    z_lo = float(skin[:, 2].min())
    z_hi = float(ring_z[0] + (ring_z[-1] - ring_z[0]) * top)

    def center_at(z):
        return float(np.interp(z, ring_z, ring_c[:, 0])), float(np.interp(z, ring_z, ring_c[:, 1]))
    u0 = lw.WRAP_SPLIT + rnd.uniform(16.0, 40.0)          # the fuzz half of the Wrap cell, 200 px per metre up it
    grid = []
    for j in range(rings + 1):
        z = z_lo + (z_hi - z_lo) * (0.07 + 0.93 * j / rings)
        cx, cy = center_at(z)
        dx, dy = skin[:, 0] - cx, skin[:, 1] - cy
        ang = np.arctan2(dy, dx)
        row = []
        for k in range(segs):
            a = 2.0 * math.pi * k / segs
            near = (np.abs(skin[:, 2] - z) < 0.14) & (np.abs((ang - a + math.pi) % (2.0 * math.pi) - math.pi) < 0.3)
            r = float(np.hypot(dx[near], dy[near]).max()) if near.any() else 0.05
            row.append(Vector((cx + math.cos(a) * (r + lift), cy + math.sin(a) * (r + lift), z)))
        grid.append(row)
    pole = Vector(center_at(z_lo) + (z_lo - lift,))

    def normal(j, k):
        c = Vector(center_at(grid[j][k].z) + (grid[j][k].z,))
        up = grid[min(j + 1, rings)][k] - (grid[j - 1][k] if j else pole)
        nn = (grid[j][(k + 1) % segs] - grid[j][(k - 1) % segs]).cross(up)
        return (nn if nn.dot(grid[j][k] - c) >= 0.0 else -nn).normalized()
    verts = [[card.vert(grid[j][k], normal(j, k)) for k in range(segs)] for j in range(rings + 1)]
    pole_v = card.vert(pole, Vector((0.0, 0.0, -1.0)))
    per = segs // repeats

    def uv(z, k, closing):
        local = k % per
        if closing and local == 0:
            local = per
        return lw.uv('Wrap', u0 + (z - z_lo + lift) * 200.0, lw.WRAP_PAD + lw.WRAP_PERIOD * local / per)
    for k in range(segs):
        k1 = (k + 1) % segs
        a0, a1 = uv(grid[0][k].z, k, False), uv(grid[0][k].z, k + 1, True)
        card.face((pole_v, verts[0][k1], verts[0][k]), (uv(pole.z, k, False)[:1] + ((a0[1] + a1[1]) * 0.5,), a1, a0), 1)
        for j in range(rings):
            card.face((verts[j][k], verts[j][k1], verts[j + 1][k1], verts[j + 1][k]),
                      (uv(grid[j][k].z, k, False), uv(grid[j][k1].z, k + 1, True),
                       uv(grid[j + 1][k1].z, k + 1, True), uv(grid[j + 1][k].z, k, False)), 1)


def loose_strands(card, sac, rnd, ends, count=3, lift=(0.06, 0.1)):
    """A few loose strands crossing the halo: each glued low on the sac, then spiralling up round it held off the
    skin, to one of `ends` (the stalk, the wall)."""
    P = sac.P
    n_rows, segs = P.shape[0], P.shape[1]
    for i in range(count):
        j0 = int(rnd.uniform(0.3, 0.55) * (n_rows - 1))
        k0 = rnd.randrange(segs)
        spin = rnd.uniform(0.15, 0.4) * rnd.choice((-1.0, 1.0))
        pts = []
        for s in range(4):
            f = s / 4.0
            j = int(j0 + (n_rows - 2 - j0) * f * 0.85)
            k = int(k0 + spin * segs * f) % segs
            q, c = Vector(tuple(P[j, k])), Vector(tuple(P[j].mean(axis=0)))
            out = q - c
            out.z = 0.0
            pts.append(q + out.normalized() * (0.01 if s == 0 else rnd.uniform(*lift)))
        pts.append(Vector(ends[i % len(ends)]))
        cord(card, pts, 0.04, 'Cord', rnd.uniform(0.0, 1500.0), [0.0, 0.05, 0.08, 0.05, 0.0], rnd.random(), mat=1)


def sac_vertex_shadow(card, sac, verts, strength=0.68):
    """The per-vertex multiplier (shadows of the spiders through the silk) for every vertex of the card."""
    out = np.ones(len(card.co), np.float32)
    for j, row in enumerate(verts):
        for k, v in enumerate(row):
            out[v] = 1.0 - strength * float(sac.shadow[j, k])
    return out


def egg_sac_a(name='EggSac_A', seed=31):
    rnd = random.Random(seed)
    spiders = [Curled((-0.29, 0.04, 0.72), 84.0, pitch=8.0, roll=-14.0, scale=0.86),
               Curled((0.3, -0.04, 0.8), 262.0, pitch=-6.0, roll=12.0, scale=0.86)]
    profile = [(0.0, 0.0), (0.26, 0.035), (0.45, 0.15), (0.55, 0.34), (0.59, 0.6), (0.57, 0.88), (0.51, 1.14),
               (0.4, 1.42), (0.27, 1.68), (0.16, 1.88), (0.1, 2.0)]
    sac = Sac(profile, 30, 21, spiders, seed=seed, pleats=(7, 0.16, 0.52))
    card = Card(name)
    verts = sac.add(card, 0, v_repeats=3)
    top = Vector(sac.P[-1].mean(axis=0))
    stalk = [top + Vector((0.0, 0.0, -0.04)), top + Vector((0.01, 0.0, 0.07)), top + Vector((-0.005, 0.01, 0.17))]
    silk_rope(card, stalk, [0.105, 0.075, 0.055], 6, 0)
    socket_at = stalk[-1] + Vector((0.0, 0.0, 0.02))
    halo(card, sac, rnd)
    loose_strands(card, sac, rnd, [stalk[1]])
    shadow = sac_vertex_shadow(card, sac, verts)
    body = sac.P.reshape(-1, 3)
    sockets = [('Silk', tuple(socket_at), (0.0, 0.0, 0.0))]
    obj = finish_sac(card, name, shadow, [support_points(body)], [], ground=False)
    spiders[0].socket(obj, 'Spawn_1')
    spiders[1].socket(obj, 'Spawn_2')
    for socket_name, location, rotation in sockets:
        lm.socket(obj, socket_name, location, rotation)
    return obj


def egg_sac_b(name='EggSac_B', seed=32):
    rnd = random.Random(seed)
    spiders = [Curled((0.05, -0.08, 0.5), 200.0, pitch=4.0, roll=8.0, scale=0.84),
               Curled((-0.06, 0.02, 1.06), 12.0, pitch=-10.0, roll=-6.0, scale=0.8)]
    rx, ry, rz = (r * 0.88 for r in SAC_B_RADII)
    cz = rz                                   # its bottom on the pivot
    profile = []
    for i in range(13):
        th = math.pi * i / 12.0
        profile.append((max(rx * math.sin(th), 0.0) * (1.0 + 0.04 * math.sin(3.0 * th)), cz - rz * math.cos(th)))
    profile[0] = (0.0, cz - rz)
    profile[-1] = (0.0, cz + rz)
    sac = Sac(profile, 30, 21, spiders, squash_y=ry / rx, squash_back=0.9, seed=seed, closed_top=True,
              pleats=(6, 0.14, 0.72))
    # The pivot is its bottom point (the lumps may have pushed it down a little).
    drop = float(sac.P[0, 0, 2])
    sac.P[..., 2] -= drop
    for s in spiders:
        s.center.z -= drop
    card = Card(name)
    verts = sac.add(card, 0, v_repeats=3)
    top = Vector(sac.P[-1].mean(axis=0))
    knot = [top + Vector((0.0, 0.0, -0.05)), top + Vector((0.01, 0.0, 0.05)), top + Vector((0.0, 0.01, 0.12))]
    silk_rope(card, knot, [0.12, 0.08, 0.04], 6, 0, twist=2.5)
    socket_at = knot[-1] + Vector((0.0, 0.0, 0.01))
    for k in range(4):                        # threads from its top, cut short (the sling's lines hold it)
        a = 2.0 * math.pi * (k + rnd.uniform(-0.2, 0.2)) / 4.0
        start = top + Vector((math.cos(a) * 0.06, math.sin(a) * 0.06, 0.06))
        pts = [start, start + Vector((math.cos(a) * 0.1, math.sin(a) * 0.1, 0.22)),
               start + Vector((math.cos(a) * 0.18, math.sin(a) * 0.18, 0.42))]
        cord(card, pts, 0.03, 'Cord', rnd.uniform(0.0, 1500.0), 0.0, rnd.random(), mat=1, taper=0.25)
    halo(card, sac, rnd)
    wall_z = float(sac.P[:, :, 2].max())
    loose_strands(card, sac, rnd, [(0.35, WALL_Y - 0.01, wall_z + 0.35), (-0.4, WALL_Y - 0.01, wall_z + 0.5),
                                   tuple(knot[1])])
    shadow = sac_vertex_shadow(card, sac, verts)
    obj = finish_sac(card, name, shadow, [support_points(sac.P.reshape(-1, 3))], [], ground=True)
    spiders[0].socket(obj, 'Spawn_1')
    spiders[1].socket(obj, 'Spawn_2')
    lm.socket(obj, 'Silk', tuple(socket_at))
    return obj


def egg_sac_c(name='EggSac_C', seed=33):
    rnd = random.Random(seed)
    # Two lobes, one spider in each; the upper lobe leans off the lower one, and the waist between them is bound round
    # with threads.
    spiders = [Curled((0.02, 0.0, 0.52), 20.0, pitch=-5.0, roll=10.0, scale=0.86),
               Curled((0.095, 0.02, 1.58), 205.0, pitch=8.0, roll=-8.0, scale=0.74)]
    profile = [(0.0, 0.0), (0.3, 0.045), (0.47, 0.2), (0.53, 0.45), (0.5, 0.72), (0.4, 0.96), (0.33, 1.1),
               (0.34, 1.22), (0.4, 1.38), (0.44, 1.58), (0.41, 1.8), (0.31, 1.98), (0.16, 2.12), (0.09, 2.2)]
    sac = Sac(profile, 30, 24, spiders, seed=seed, lean=(0.14, 1.1, 1.7), pleats=(6, 0.14, 0.8))
    card = Card(name)
    verts = sac.add(card, 0, v_repeats=3)
    waist = 1.12
    wound_threads(card, sac, rnd, 4, rows=(0.45, 0.53), turns=(1.2, 1.8), lift=0.01)
    top = Vector(sac.P[-1].mean(axis=0))
    stalk = [top + Vector((0.0, 0.0, -0.04)), top + Vector((-0.01, 0.0, 0.08)), top + Vector((0.0, 0.01, 0.18))]
    silk_rope(card, stalk, [0.1, 0.07, 0.05], 6, 0)
    socket_at = stalk[-1] + Vector((0.0, 0.0, 0.02))
    halo(card, sac, rnd)
    loose_strands(card, sac, rnd, [stalk[1]])
    shadow = sac_vertex_shadow(card, sac, verts)
    body = sac.P.reshape(-1, 3)
    lower = body[body[:, 2] <= waist + 0.08]
    upper = body[body[:, 2] >= waist - 0.08]
    obj = finish_sac(card, name, shadow, [support_points(lower, 40), support_points(upper, 40)], [], ground=False)
    spiders[0].socket(obj, 'Spawn_1')
    spiders[1].socket(obj, 'Spawn_2')
    lm.socket(obj, 'Silk', tuple(socket_at))
    return obj


BURST_MOUTH = 0.8           # where the sac tore open at its top end (share of its height arc), before the ragged edge


def _smooth01(t):
    t = min(max(t, 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def _fringe_v(y):
    """V of a row of the Fringe cell (px from its top)."""
    return 1.0 - (lw.REGIONS['Fringe'][1] + y) / lw.SIZE


def egg_sac_burst(name='EggSac_Burst', seed=34, segs=26, rings=12):
    """A hatched sac on the floor, deflated. The sac it was (1.45 m across, 1.9 m tall) lies fallen on its side, every
    cross-section resting on the floor and collapsed, slumped over toward +X in soft crumpled folds (folds along it as
    it went flat, creases across), still a little fuller at its closed end and held open at the mouth, its top end
    toward -Y. The mouth's edge is ragged and stringy (Fringe, on Web), strands bridge the tear and trail out over the
    floor, and the inside (faces of its own, back to back with the outside: EggSilk is one-sided) is darker and
    stained; the outside is grimy in patches, under a fuzz of loose silk. Nowhere a smooth dome."""
    rnd = random.Random(seed)
    card = Card(name)
    rx, rz = 0.72, 0.95
    n = 64
    rs = np.array([rx * math.sin(math.pi * i / n) for i in range(n + 1)])
    zs = np.array([rz * (1.0 - math.cos(math.pi * i / n)) for i in range(n + 1)])
    arc = np.concatenate([[0.0], np.cumsum(np.hypot(np.diff(rs), np.diff(zs)))])
    total = float(arc[-1])
    off = Vector((rnd.uniform(0.0, 40.0), rnd.uniform(0.0, 40.0), rnd.uniform(0.0, 40.0)))
    off2 = Vector((rnd.uniform(0.0, 40.0), rnd.uniform(0.0, 40.0), rnd.uniform(0.0, 40.0)))
    ct, st = math.cos(math.radians(80.0)), math.sin(math.radians(80.0))

    def shape(s, a):
        """The burst sac at s (share of the height arc of the sac it was, from its closed end) and a (round it)."""
        d = s * total
        r, z = float(np.interp(d, arc, rs)), float(np.interp(d, arc, zs))
        p0 = Vector((math.cos(a) * r, math.sin(a) * r, z))
        ridge = 1.0 - abs(noise.noise(p0 * 1.6 + off))
        r += min(1.0, r / 0.15) * (0.06 * (ridge * ridge - 0.45) +
                                    0.04 * math.sin(5.0 * a + 2.0 * noise.noise(p0 * 0.9 - off)) +
                                    0.014 * noise.noise(p0 * 5.0 + off))
        x, y = math.cos(a) * r, math.sin(a) * r
        # Fallen on its side: the axis turned from up to -Y (its top end), each cross-section on the floor.
        yy = y * ct - z * st
        lift = y * st + z * ct
        floor = -float(np.interp(d, arc, rs)) * st + z * ct
        # Deflated: the top fallen in onto the bottom, flattest in the middle, a little fuller at the closed end and
        # held open at the mouth; the top's middle sunk into a trough between two soft ridges, slumped over toward +X,
        # spread out over the floor unevenly along it.
        squash = 0.46 - 0.18 * math.sin(math.pi * min(s / BURST_MOUTH, 1.0)) + 0.1 * _smooth01((s - 0.6) / 0.2)
        top = max(math.sin(a), 0.0)
        trough = 1.0 - 0.5 * top * top * (1.0 - _smooth01((s - 0.62) / 0.16))
        h = max(lift - floor, 0.0) * squash * trough
        crease = 1.0 - abs(noise.noise(Vector((x, yy, h)) * 2.0 + off2))
        h += min(1.0, h / 0.1) * (0.085 * crease ** 3 - 0.03 + 0.02 * noise.noise(Vector((x, yy, 0.0)) * 4.5 - off2))
        spread = 1.14 + 0.12 * noise.noise(Vector((0.0, yy * 1.4, seed + 3.0))) - 0.08 * min(h / 0.35, 1.0)
        xx = x * spread + 0.55 * h
        h = max(h, 0.008 + 0.006 * noise.noise(Vector((xx * 3.0, yy * 3.0, seed + 1.0))))
        return Vector((xx, yy + 0.95, h))

    # The mouth's ragged edge: each column tore at its own height.
    mouth = [BURST_MOUTH + 0.07 * noise.noise(Vector((k * 0.55, 0.3, seed))) + 0.03 * ((k * 7) % 3 - 1)
             for k in range(segs)]
    angle = [2.0 * math.pi * k / segs for k in range(segs)]
    P = [[shape(mouth[k] * j / rings, angle[k]) for k in range(segs)] for j in range(rings + 1)]
    P[0] = [P[0][0]] * segs                                    # its closed end, one point on the floor
    centers = [sum(row, Vector()) / segs for row in P]

    def normal(j, k):
        """Outward by the grid's own handedness (round it counterclockwise, then on toward the mouth): the falling
        over and squashing keep it, while a crease could fool a guess from the middle."""
        if j == 0:
            return Vector((0.0, 1.0, -0.3)).normalized()
        up = P[min(j + 1, rings)][k] - P[j - 1][k]
        nn = (P[j][(k + 1) % segs] - P[j][(k - 1) % segs]).cross(up)
        if nn.length < 1e-9:
            nn = P[j][k] - centers[j]
        return nn.normalized()
    sac_top = lw.REGIONS['Sac'][1] + lw.SAC_PAD
    per = rings // 3

    def sac_v(j, upper):
        local = j % per
        if upper and local == 0:
            local = per
        return 1.0 - (sac_top + lw.SAC_PERIOD * (1.0 - local / per)) / lw.SIZE
    shells, shade = [], {}
    for inside in (False, True):
        rows = []
        for j in range(rings + 1):
            row = []
            for k in range(segs):
                if j == 0 and k:
                    row.append(row[0])
                    continue
                nn = normal(j, k)
                row.append(card.vert(P[j][k], -nn if inside else nn))
                if inside:
                    shade[row[-1]] = 0.3 + 0.12 * j / rings        # stained, darkest deep inside
                else:                                              # grimy in patches, where it lay and dragged
                    shade[row[-1]] = min(1.0, 0.88 + 0.12 * noise.noise(P[j][k] * 2.4 + off2))
            rows.append(row)
        shells.append(rows)
        u_off = 0.37 if inside else 0.0

        def uv(j, k, u, v):
            """The silk crumpled with the sac: its wound bands pushed about (a vertex's push the same on every face)."""
            q = Vector((j * 0.6, (k % segs) * 0.45, seed + 11.0))
            return u + 0.03 * noise.noise(q), v + 0.012 * noise.noise(q + Vector((0.0, 0.0, 6.0)))
        for j in range(rings):
            va, vb = sac_v(j, False), sac_v(j + 1, True)
            for k in range(segs):
                k1 = (k + 1) % segs
                u0, u1 = u_off + k / segs, u_off + (k + 1) / segs
                if j == 0:
                    vs, us = [rows[0][k], rows[1][k1], rows[1][k]], [uv(0, k, (u0 + u1) * 0.5, va), uv(1, k1, u1, vb),
                                                                     uv(1, k, u0, vb)]
                else:
                    vs = [rows[j][k], rows[j][k1], rows[j + 1][k1], rows[j + 1][k]]
                    us = [uv(j, k, u0, va), uv(j, k1, u1, va), uv(j + 1, k1, u1, vb), uv(j + 1, k, u0, vb)]
                if inside:                                         # the inside faces the other way
                    vs.reverse()
                    us.reverse()
                card.face(vs, us, keep=True)

    # A fuzz of loose silk 3 cm off the outside (the Wrap cell's fuzz half, on Web), like the intact sacs' halos: it
    # reads as spun silk, not paper. Twenty columns round (the fuzz four times over), from near the closed end to short
    # of the mouth's fringe, none where the sac lies on the floor. Its faces turn outward, so the occlusion bake sees
    # through them from the skin.
    fuzz_rnd = random.Random(seed + 100)
    cols, repeats = 20, 4
    per_fuzz = cols // repeats
    u_fuzz = lw.WRAP_SPLIT + fuzz_rnd.uniform(16.0, 40.0)        # 200 px per metre along the sac
    fuzz = []
    for j in range(1, rings):
        row = []
        for c in range(cols):
            k = c * segs / cols
            k0 = int(k)
            k1, f = (k0 + 1) % segs, k - k0
            q = P[j][k0].lerp(P[j][k1], f)
            nn = normal(j, k0).lerp(normal(j, k1), f).normalized()
            p = q + nn * 0.03
            p.z = max(p.z, 0.012)
            d = (mouth[k0] * (1.0 - f) + mouth[k1] * f) * j / rings * total
            row.append((card.vert(p, nn), q.z, u_fuzz + d * 200.0))
        fuzz.append(row)
    for j in range(len(fuzz) - 1):
        for c in range(cols):
            c1 = (c + 1) % cols
            corners = (fuzz[j][c], fuzz[j][c1], fuzz[j + 1][c1], fuzz[j + 1][c])
            if max(z for _, z, _ in corners) < 0.03:              # lying on the floor
                continue
            steps = (c % per_fuzz, c % per_fuzz + 1, c % per_fuzz + 1, c % per_fuzz)
            card.face([v for v, _, _ in corners],
                      [lw.uv('Wrap', u, lw.WRAP_PAD + lw.WRAP_PERIOD * loc / per_fuzz)
                       for (_, _, u), loc in zip(corners, steps)], 1)

    # The mouth's edge, ragged and stringy (Fringe, on Web; lit like the cards): out from the last ring, flaring and
    # drooping, lying on the floor where it reaches it.
    mouth_c = centers[rings]
    base, tips, norms = [], [], []
    for k in range(segs):
        q = P[rings][k]
        on = (q - P[rings - 1][k]).normalized()
        flare = q - mouth_c
        flare = flare.normalized() if flare.length > 1e-6 else on
        d = (on + flare * 0.45 - UP * 0.25).normalized()
        t = q + d * rnd.uniform(0.2, 0.3)
        t.z = max(t.z, 0.01)
        base.append(q)
        tips.append(t)
        norms.append(normal(rings, k))
    s = [0.0]
    for k in range(1, segs + 1):
        s.append(s[-1] + (base[k % segs] - base[k - 1]).length)
    phase = rnd.random()
    low = [card.vert(base[k], shade_normal(norms[k], 1.5), 0.0, phase) for k in range(segs)]
    high = [card.vert(tips[k], shade_normal(norms[k], 1.5), 0.06, phase) for k in range(segs)]
    u_start = rnd.uniform(0.0, 1500.0)
    for k in range(segs):
        k1 = (k + 1) % segs
        u0, u1 = (u_start + s[k] * lw.PX_M) / lw.SIZE, (u_start + s[k + 1] * lw.PX_M) / lw.SIZE
        card.face((low[k], low[k1], high[k1], high[k]),
                  ((u0, _fringe_v(121.0)), (u1, _fringe_v(121.0)), (u1, _fringe_v(5.0)), (u0, _fringe_v(5.0))), 1)

    # Strands bridging the tear, upper lip to lower, sagging; two snapped, hanging from the upper lip.
    order = sorted(range(segs), key=lambda k: (P[rings][k].z, k))
    lower, upper = order[:segs // 3], order[-(segs // 3):]
    for i in range(5):
        a = P[rings][upper[rnd.randrange(len(upper))]]
        b = P[rings][lower[rnd.randrange(len(lower))]]
        line = wiggle(catenary(a, b, rnd.uniform(0.04, 0.09), 5), 0.012, rnd)
        if i < 2:
            line = line[:4]
            cord(card, line, 0.035, 'Cord', rnd.uniform(0.0, 1500.0), [0.0, 0.08, 0.12, 0.15], rnd.random(), mat=1,
                 taper=0.2)
        else:
            cord(card, line, 0.035, rnd.choice(('Cord', 'Tufted')), rnd.uniform(0.0, 1500.0),
                 [0.0, 0.05, 0.08, 0.08, 0.05, 0.0], rnd.random(), mat=1)
    # Strands trailing from the lower lip out over the floor, wandering, fraying to nothing.
    for i in range(4):
        start = P[rings][lower[rnd.randrange(len(lower))]].copy()
        heading = math.radians(-90.0 + rnd.uniform(-55.0, 55.0))
        length = rnd.uniform(0.6, 1.0)
        bend = rnd.uniform(-0.5, 0.5)
        pts = [start]
        for t in (0.25, 0.5, 0.75, 1.0):
            a = heading + bend * t
            q = start + Vector((math.cos(a), math.sin(a), 0.0)) * (length * t)
            q.z = 0.012 + 0.004 * t
            pts.append(q)
        cord(card, pts, 0.03, 'Cord', rnd.uniform(0.0, 1500.0), 0.0, rnd.random(), crossed=False, mat=1, taper=0.2,
             occ=[0.85, 0.8, 0.78, 0.76, 0.75])

    multiplier = np.ones(len(card.co), np.float32)
    for v, value in shade.items():
        multiplier[v] = value
    body = [tuple(p) for row in P for p in row]
    sockets = [('Spawn_1', (0.62, -1.55, 0.0), (0.0, 0.0, -26.0)), ('Spawn_2', (-0.7, -1.45, 0.0), (0.0, 0.0, 30.0))]
    return finish_sac(card, name, multiplier, [support_points(body, 40)], sockets, ground=True, ao_distance=0.8,
                      custom=True)


# --- The kit ---

BUILDERS = [
    ('Web_Orb', web_orb), ('Web_Corner', web_corner), ('Web_Drape', web_drape), ('Web_Ground', web_ground),
    ('Web_Fork', web_fork), ('Web_Tatters', web_tatters), ('Web_Strands', web_strands), ('Web_Line', web_line),
    ('Web_Funnel', web_funnel), ('Web_BranchWrap', web_branch_wrap), ('Web_Sling', web_sling), ('Web_Snare', web_snare),
    ('Web_Crown', web_crown),
    ('EggSac_A', egg_sac_a), ('EggSac_B', egg_sac_b), ('EggSac_C', egg_sac_c), ('EggSac_Burst', egg_sac_burst),
]
models = [build() for _, build in BUILDERS]


def report(obj):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    co = np.array([obj.matrix_world @ Vector(c) for c in obj.bound_box])
    size = co.max(axis=0) - co.min(axis=0)
    sockets = [c.name.split('.')[0] for c in obj.children if c.name.startswith('SOCKET_')]
    hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
    print(f'SINK: {obj.name}: {tris} triangles, {size[0]:.2f} x {size[1]:.2f} x {size[2]:.2f} m, '
          f'materials {", ".join(m.name for m in obj.data.materials)}, {hulls} hulls, sockets {", ".join(sockets) or "-"}',
          flush=True)


for model in models:
    report(model)

if lt.want_preview():
    for mat in (WEB, SILK):                    # previews clip where Unreal does (1/3), not at a half
        for node in mat.node_tree.nodes:
            if node.type == 'MATH' and node.operation == 'GREATER_THAN':
                node.inputs[1].default_value = 1.0 / 3.0
    rows = [models[0:8], models[9:17], models[8:9]]          # the cards; wrap, sling, snare, crown, sacs; the funnel
    y = 0.0
    shown = []
    for row in rows:
        x = 0.0
        for obj in row:
            lo = min((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            hi = max((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            obj.location.x += x - lo
            x += hi - lo + 0.8
        for obj in row:
            obj.location.x -= x * 0.5
            obj.location.y = y
            obj.location.z = -min(Vector(c).z for c in obj.bound_box)        # all on the ground (the crown too)
        y += 5.0
        shown += row
    bpy.context.view_layer.update()
    lt.preview(shown, lt.preview_path(PREVIEW, 'Kit'), view=(-0.25, -1.6, 0.75), fit=0.62, resolution=(1800, 1100))
    for obj in models:
        obj.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
