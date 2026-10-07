"""The loot chests: openable chests the user picked from the concept renders on 2026-09-30, brought out of Art/Backlog
into the game on 2026-10-07 for Ransom's Rest's Ranger caches (Docs/Areas/RansomsRest.md, step 26): three Supply
Crates are Ranger Ruth Calder's caches (under the windmill, on the Sink's rim, on the bluff path) and the gang's
Strongbox stands in the sheriff's office. A scripted model file (Art/README.md); Tools/models.ps1 exports it into
/Game/Art/Loot (SM_SupplyCrate, SM_SupplyCrate_Lid, SM_Strongbox, SM_Strongbox_Lid, SM_Strongbox_Wheel).

  SupplyCrate        Uncommon. A 1.15 m olive steel crate on runners: steel edge frame, a cream stencil band like the
                     ammo cans, end handles on brackets, green lights on the two latches, two hinges at the back, a
                     black tray inside where the loot lies.
  SupplyCrate_Lid    its lid: an olive panel in a steel frame with two ribs and a stencil, the latches' catches.
  Strongbox          Rare. A 0.82 m riveted steel safe in weathered teal paint on iron feet: framed corners, riveted
                     bands, a blue-ringed dial on the front, a blue seam under the lid, an iron floor plate inside
                     (the backlog's felt, never seen in the dark inside, is gone).
  Strongbox_Lid      its lid: a thick steel cap in its own riveted frame, two ribs.
  Strongbox_Wheel    the vault wheel (hub, three spokes, brass knobs), to spin as the box opens.

The third design, the Reliquary (Legendary: a granite chest with gold and orange rune lines, its lid and a floating
crystal), isn't wanted in the game yet, and the chapel has its own smashed copy (Art/Models/Props/Reliquary.py,
SM_Reliquary_Smashed). This file builds Reliquary, Reliquary_Lid and Reliquary_Crystal only for its previews (with
--preview, as Art/Models/Vehicles/Train.py builds Locomotive_A), so the export holds the five models above.

Story touches, switches for the user to pick (both on):
  RANGER_MARK  the Supply Crate is the Rim Rangers': their badge (the frontier Rangers' five-point star in a ring,
               stencil-cut) in cream on the lid, so a cache reads from above, RIM RANGERS between two stars on the
               front band and R.R. on both ends in black, in plain block stencil letters built from geometry (Rye is
               for painted signs, and Art/Fonts holds no stencil face). Off: the backlog's SUPPLY . 07, L-07 and
               THIS SIDE UP in the UI font (Chakra Petch).
  ROBBED       the Strongbox shows it was forced in one of the gang's bank jobs: pry scratches round the dial and at
               the lid's front edge, two bullet dents (the front, the lid's top), the padlock hasp wrenched and hanging
               askew from the lid, its staple snapped off the body. Off: the backlog's clean safe.

Pivots: a chest's body sits on the ground at the middle of its footprint, front to -Y. A lid's origin is its hinge (the
back top edge of the body): attach it at the body's SOCKET_Lid and pitch it about the hinge, up to the opening angle
in OPEN_ANGLE (degrees; in Unreal a positive pitch of the lid relative to the socket, its front edge rising). Other
sockets: SOCKET_Loot (the middle of the floor inside: where loot and the loot beam come from), SOCKET_Wheel (the
strongbox's wheel, spinning about its forward axis: Blender -Y, Unreal +X) and SOCKET_Interact (the front, facing out:
where the prompt shows). Rarity shows only as the color of the lights (green latch lights, the blue dial ring and seam),
the game's one color code; nothing else on the chests uses a rarity color.

Materials (the brief's shared names where the game already has the look):
  ChestOlive, ChestSteel, ChestSafeSteel   MetalRust tinted olive 0x9aa266 (UV x2.5), grey 0x77756f, pale teal-grey
                                           0xa3b1b2 (UV x1.8): the chests' own painted, rusting steels
  ChestGlowUncommon, ChestGlowRare   emissive surfaces (Glow 4) in the rarity colors
  PaintCream, PaintBlack   shared: the crate's band and lid badge; its black stencils and tray
  IronBlack, BrassWorn     shared: the strongbox's rivets, feet, hinges and floor plate; the wheel's hub and knobs
Slots: SupplyCrate 5 (olive, steel, cream, black, green), its lid 3 (with RANGER_MARK off the lid's stencil is black,
still 3); Strongbox 4 (teal, steel, iron, blue), its lid 3, the wheel 2.
Small props: no Nanite, LODs at 50% and 25%. Collision: a box round each body and each lid; the wheel none. Every part
is mapped at world scale (box_uv), so every face has UV area for its tangents.

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Loot/Chests.py -- --preview
renders Saved/ArtPreviews/RansomsRest/Chests/ in Ransom's Rest's golden late afternoon (the sun at bearing 247.5, 15
degrees up; the chests face south): SupplyCrate.png and Strongbox.png (closed, a 1.8 m figure beside), their _Open.png,
SupplyCrate_Mark.png and Strongbox_Robbed.png (each story touch off and on), SupplyCrate_Windmill.png (a cache at the
windmill's foot, Intermediate/ArtExport_Buildings' SM_Windmill and Intermediate/ArtExport_RR_Scrub's scrub, when those
exports exist), Strongbox_Office.png (on the sheriff's office floor: a plank floor and fieldstone walls, the low sun
through a barred window), Reliquary.png (closed and open, for reference); and the sheet
Saved/ArtPreviews/RansomsRest/Chests_overview.png. The previews hold the lights' emission low (Eevee turns a strong one
white, without the game's bloom), so their rarity hue reads.
"""
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp

# --- Switches (the user's picks) ---
RANGER_MARK = True   # the Rim Rangers' badge and stencils on the Supply Crate (off: the backlog's stencils)
ROBBED = True        # the Strongbox's signs of the robbery (off: the backlog's clean safe)

# How far each lid opens (degrees about its hinge).
OPEN_ANGLE = {'SupplyCrate': 112.0, 'Strongbox': 102.0, 'Reliquary': 103.0}

# --- Materials ---
OLIVE = lt.material('MetalRust', name='ChestOlive', tint=0x9aa266, uv_scale=2.5)
STEEL = lt.material('MetalRust', name='ChestSteel', tint=0x77756f)
SAFE = lt.material('MetalRust', name='ChestSafeSteel', tint=0xa3b1b2, uv_scale=1.8)
CREAM = lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2)
STENCIL = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)
IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)
BRASS = lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56)
# Lights: opaque emissive surfaces (Nanite can't draw the additive Glow kind).
GREEN = lm.material('ChestGlowUncommon', 0x56e256, Glow=4.0, Variation=0.0)
BLUE = lm.material('ChestGlowRare', 0x6b95ff, Glow=4.0, Variation=0.0)

# The texture set each material maps at world scale (by name); the glows take MetalWorn's density, only so that
# every face has UV area (tangents need it).
TILEABLE = {'ChestOlive': 'MetalRust', 'ChestSteel': 'MetalRust', 'ChestSafeSteel': 'MetalRust',
            'PaintCream': 'PaintWorn', 'PaintBlack': 'PaintWorn', 'IronBlack': 'MetalWorn', 'BrassWorn': 'MetalWorn',
            'ChestGold': 'MetalWorn', 'ChestGranite': 'RockGranite', 'ChestRuneStone': 'RockGranite'}
LIFT = 0.0008   # how far stencils and scratches stand proud of the paint


# --- Parts ---

def part(obj, mat, seed=0):
    """Puts mat on a part and maps it at world scale (small parts: the texture's own density)."""
    lt.assign(obj, mat)
    lt.box_uv(obj, TILEABLE.get(mat.name, 'MetalWorn'), seed=seed)
    return obj


def block(size, center, mat, seed=0, bevel=0.0, rotation=(0.0, 0.0, 0.0)):
    return part(lp.block(size, center, rotation, bevel=bevel), mat, seed)


def oriented(obj, center, normal):
    """Turns a part built round +Z to face normal, then moves it to center."""
    q = Vector(normal).normalized().to_track_quat('Z', 'Y')
    obj.data.transform(q.to_matrix().to_4x4())
    return lp.place(obj, center)


def rivet(center, normal, r=0.011, mat=None):
    """A domed rivet head, open underneath where it sits on the plate: 15 triangles."""
    obj = lp.lathe([(r, 0.0), (r * 0.78, r * 0.42), (0.0, r * 0.62)], segments=5)[0]
    return oriented(part(obj, mat or IRON), center, normal)


def ring(major, minor, center, normal, mat, segments=24, sides=4):
    profile = [(major + math.cos(a) * minor, math.sin(a) * minor) for a in (2.0 * math.pi * k / sides for k in range(sides))]
    obj = lp.lathe(profile, segments=segments, closed=True)[0]
    return oriented(part(obj, mat), center, normal)


def disc(radius, depth, center, normal, mat, segments=20, back=True):
    """A short cylinder standing on center along normal; back=False leaves out the face against the surface."""
    profile = ([(0.0, 0.0)] if back else []) + [(radius, 0.0), (radius, depth), (0.0, depth)]
    obj = lp.lathe(profile, segments=segments)[0]
    return oriented(part(obj, mat), center, normal)


def gem(radius, height, center, normal, mat, sides=6):
    obj = lp.lathe([(0.0, -height), (radius, 0.0), (0.0, height)], segments=sides)[0]
    return oriented(part(obj, mat), center, normal)


def wrap(outer_x, outer_y, z, h, t, mat, gap_x=0.0, gap_y=0.0, bevel=0.0, seed=0):
    """Four plates hugging a box of half extents outer_x, outer_y: front and back along X, the ends along Y."""
    parts = []
    for s in (-1.0, 1.0):
        parts.append(block((2.0 * outer_x + 2.0 * t - gap_x, t, h), (0.0, s * (outer_y + t * 0.5), z), mat, seed, bevel))
        parts.append(block((t, 2.0 * outer_y - gap_y, h), (s * (outer_x + t * 0.5), 0.0, z), mat, seed, bevel))
    return parts


def handle(points, radius, mat):
    return part(lp.sweep(points, lp.ngon(radius, 8), up=(0.0, 0.0, 1.0)), mat)


def _area(points):
    return 0.5 * sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]))


def flat(polygons, center, rotation, mat, seed=0):
    """Flat faces from 2D polygons (metres, each convex), built facing +Z, turned by rotation (degrees, X then Y then
    Z) and moved to center: (90, 0, 0) lays them on a front face (2D x along X, y up), (0, 0, 0) on a top."""
    bm = lp.new_bmesh()
    for points in polygons:
        points = list(points)
        if _area(points) < 0.0:
            points.reverse()
        bm.faces.new([bm.verts.new((x, y, 0.0)) for x, y in points])
    return part(lp.place(lp.mesh_object(bm), center, rotation), mat, seed)


def slab(outlines, thick, matrix, mat, seed=0):
    """Plates of thickness thick from convex 2D outlines (built in XY, thickness up +Z), moved by matrix."""
    bm = lp.new_bmesh()
    for points in outlines:
        points = list(points)
        if _area(points) < 0.0:
            points.reverse()
        low = [bm.verts.new((x, y, 0.0)) for x, y in points]
        high = [bm.verts.new((x, y, thick)) for x, y in points]
        bm.faces.new(list(reversed(low)))
        bm.faces.new(high)
        n = len(points)
        for k in range(n):
            bm.faces.new((low[k], low[(k + 1) % n], high[(k + 1) % n], high[k]))
    obj = lp.mesh_object(bm)
    obj.data.transform(matrix)
    return part(obj, mat, seed)


# --- Stencils: plain block letters, cut like a stencil plate (the strokes stand apart where the plate would need
# bridges). Each glyph is (advance, convex polygons) in units of the cap height, from x = 0 at its left. ---

def _rect(x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def _star(cx, cy, r, inner=0.42, points=5):
    """A five-point star as triangles round its middle (the wanted posters' star: looter_posters.star)."""
    tips = [(cx + (r if k % 2 == 0 else r * inner) * math.cos(math.radians(90.0 + 180.0 * k / points)),
             cy + (r if k % 2 == 0 else r * inner) * math.sin(math.radians(90.0 + 180.0 * k / points)))
            for k in range(2 * points)]
    return [[(cx, cy), tips[k], tips[(k + 1) % len(tips)]] for k in range(len(tips))]


def _a_bar(y0, y1):
    """The A's crossbar, from its left leg's inner edge to its right leg's."""
    return [(0.2 + 0.13 * y0, y0), (0.52 - 0.13 * y0, y0), (0.52 - 0.13 * y1, y1), (0.2 + 0.13 * y1, y1)]


GLYPHS = {
    ' ': (0.34, []),
    '.': (0.2, [_rect(0.0, 0.0, 0.2, 0.2)]),
    '*': (1.0, _star(0.5, 0.5, 0.56)),
    'A': (0.72, [[(0.0, 0.0), (0.2, 0.0), (0.33, 1.0), (0.19, 1.0)], [(0.52, 0.0), (0.72, 0.0), (0.53, 1.0), (0.39, 1.0)],
                 _a_bar(0.26, 0.42)]),
    'E': (0.58, [_rect(0.0, 0.0, 0.2, 1.0), _rect(0.27, 0.8, 0.58, 1.0), _rect(0.27, 0.41, 0.52, 0.59),
                 _rect(0.27, 0.0, 0.58, 0.2)]),
    'G': (0.66, [[(0.0, 0.12), (0.12, 0.0), (0.2, 0.0), (0.2, 1.0), (0.12, 1.0), (0.0, 0.88)],
                 _rect(0.27, 0.8, 0.66, 1.0), _rect(0.27, 0.0, 0.66, 0.2), _rect(0.46, 0.2, 0.66, 0.52),
                 _rect(0.34, 0.36, 0.46, 0.52)]),
    'I': (0.2, [_rect(0.0, 0.0, 0.2, 1.0)]),
    'M': (0.86, [_rect(0.0, 0.0, 0.2, 1.0), _rect(0.66, 0.0, 0.86, 1.0),
                 [(0.24, 1.0), (0.36, 1.0), (0.40, 0.34), (0.30, 0.34)], [(0.50, 1.0), (0.62, 1.0), (0.56, 0.34), (0.46, 0.34)]]),
    'N': (0.66, [_rect(0.0, 0.0, 0.2, 1.0), _rect(0.46, 0.0, 0.66, 1.0), [(0.2, 1.0), (0.33, 1.0), (0.46, 0.0), (0.33, 0.0)]]),
    'R': (0.68, [_rect(0.0, 0.0, 0.2, 1.0), [(0.27, 0.8), (0.66, 0.8), (0.66, 0.9), (0.56, 1.0), (0.27, 1.0)],
                 _rect(0.46, 0.66, 0.66, 0.8), [(0.27, 0.48), (0.56, 0.48), (0.66, 0.58), (0.66, 0.66), (0.27, 0.66)],
                 [(0.36, 0.41), (0.56, 0.41), (0.68, 0.0), (0.48, 0.0)]]),
    'S': (0.62, [[(0.0, 0.8), (0.62, 0.8), (0.62, 1.0), (0.12, 1.0), (0.0, 0.88)], _rect(0.0, 0.59, 0.2, 0.8),
                 _rect(0.0, 0.41, 0.62, 0.59), _rect(0.42, 0.2, 0.62, 0.41), [(0.0, 0.0), (0.5, 0.0), (0.62, 0.12), (0.62, 0.2), (0.0, 0.2)]]),
}


def stencil(body, cap, center, rotation, mat, spacing=0.14):
    """Block stencil lettering (GLYPHS), cap metres tall, centred on center and laid by rotation as flat() does."""
    widths = [GLYPHS[c][0] for c in body]
    x = -0.5 * (sum(widths) + spacing * (len(body) - 1))
    polygons = []
    for c, w in zip(body, widths):
        polygons += [[((x + px) * cap, (py - 0.5) * cap) for px, py in poly] for poly in GLYPHS[c][1]]
        x += w + spacing
    return flat(polygons, center, rotation, mat)


def badge(radius, center, rotation, mat):
    """The Rim Rangers' badge: a five-point star in a ring, the ring cut by four stencil bridges."""
    polygons = _star(0.0, 0.0, radius * 0.7)
    segments, width = 32, radius * 0.16
    for k in range(segments):
        if k % 8 == 4:   # the bridges: one segment left out on each diagonal, between the star's points
            continue
        a0, a1 = 2.0 * math.pi * (k - 0.5) / segments, 2.0 * math.pi * (k + 0.5) / segments
        polygons.append([(math.cos(a) * r, math.sin(a) * r) for a, r in
                         ((a0, radius - width), (a1, radius - width), (a1, radius), (a0, radius))])
    return flat(polygons, center, rotation, mat)


def text(body, size, center, rotation, mat):
    """The backlog's lettering in the UI font (Chakra Petch), flat, a hair proud of the surface (RANGER_MARK off)."""
    path = os.path.join(lt.REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Bold.ttf').replace('\\', '/')
    font = bpy.data.fonts.load(path, check_existing=True)
    curve = bpy.data.curves.new('_text', 'FONT')
    curve.body, curve.font, curve.size = body, font, size
    curve.align_x, curve.align_y = 'CENTER', 'CENTER'
    curve.resolution_u = 3
    src = bpy.data.objects.new('_text', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(src.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    # The curve fill's polygons, cut into triangles here, leave a few slivers without area: gone, they can't upset the
    # tangents.
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.calc_area() < 1e-8], context='FACES_ONLY')
    bm.to_mesh(mesh)
    bm.free()
    while mesh.uv_layers:
        mesh.uv_layers.remove(mesh.uv_layers[0])
    mesh.uv_layers.new(name='UVMap')
    mesh.materials.clear()
    obj = bpy.data.objects.new('_part', mesh)
    bpy.context.scene.collection.objects.link(obj)
    return part(lp.place(obj, center, rotation), mat)


def finish_body(body):
    lp.finish(body, ao=0.3, nanite=False)
    body['LODs'] = '50,25'
    return body


def finish_lid(name, parts, hinge, open_angle):
    """Merges a lid and puts its origin on the hinge (y, z): the geometry stays where it was built."""
    lid = lp.join(name, parts)
    lp.place(lid, (0.0, -hinge[0], -hinge[1]))
    lid.location = (0.0, hinge[0], hinge[1])
    lid['OpenAngle'] = open_angle
    lm.smooth(lid, 35.0)
    lt.bake_vertex_ao(lid, distance=0.3, ground=False)
    lid['Nanite'] = 0
    lid['LODs'] = '50,25'
    return lid


# --- Uncommon: the supply crate ---

def supply_crate(mark=RANGER_MARK, prefix=''):
    rnd = random.Random(23)
    sx, sy, sz, base, wall = 1.15, 0.52, 0.34, 0.06, 0.02
    H = base + sz
    zc = base + sz * 0.5
    parts, lid = [], []
    for y in (-0.17, 0.17):
        parts.append(block((sx - 0.08, 0.07, base), (0.0, y, base * 0.5), STEEL, 1, bevel=0.006))
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, zc), OLIVE, rnd.randint(0, 99)))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, zc), OLIVE, rnd.randint(0, 99)))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.02), (0.0, 0.0, base + 0.01), OLIVE))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            parts.append(block((0.045, 0.045, sz), (x * (sx * 0.5 - 0.01), y * (sy * 0.5 - 0.01), zc), STEEL,
                               rnd.randint(0, 99), bevel=0.004))
    for z in (base + 0.02, H - 0.02):
        parts += wrap(sx * 0.5, sy * 0.5, z, 0.04, 0.012, STEEL, gap_x=0.06, gap_y=0.06, seed=5)
    # The cream band and its stencils.
    for s in (-1.0, 1.0):
        parts.append(block((sx - 0.07, 0.005, 0.085), (0.0, s * (sy * 0.5 + 0.0025), zc), CREAM, 2))
        parts.append(block((0.005, sy - 0.07, 0.085), (s * (sx * 0.5 + 0.0025), 0.0, zc), CREAM, 3))
    front, end = -(sy * 0.5 + 0.005 + LIFT), sx * 0.5 + 0.005 + LIFT
    if mark:
        parts.append(stencil('* RIM RANGERS *', 0.05, (0.0, front, zc), (90.0, 0.0, 0.0), STENCIL))
        for s in (-1.0, 1.0):
            parts.append(stencil('R.R.', 0.05, (s * end, 0.0, zc), (90.0, 0.0, 90.0 * s), STENCIL))
    else:
        parts.append(text('SUPPLY  •  07', 0.052, (0.0, front, zc), (90.0, 0.0, 0.0), STENCIL))
        parts.append(text('L-07', 0.05, (-end, 0.0, zc), (90.0, 0.0, -90.0), STENCIL))
    # End handles on brackets.
    for s in (-1.0, 1.0):
        x = s * (sx * 0.5 + 0.015)
        parts.append(handle([(x, -0.1, H - 0.08), (x + s * 0.045, -0.085, H - 0.08), (x + s * 0.045, 0.085, H - 0.08),
                             (x, 0.1, H - 0.08)], 0.011, STEEL))
        for y in (-0.1, 0.1):
            parts.append(block((0.02, 0.034, 0.045), (s * (sx * 0.5 + 0.01), y, H - 0.08), STEEL, 4))
    # Latches: a plate and lever on the body (a green light in each plate), a catch on the lid.
    plate_y = -(sy * 0.5 + 0.012)          # the plates stand on the wall, flush with the frame band's face
    for x in (-0.38, 0.38):
        parts.append(block((0.08, 0.024, 0.07), (x, plate_y, H - 0.05), STEEL, 8, bevel=0.003))
        parts.append(block((0.022, 0.006, 0.014), (x + 0.025, plate_y - 0.014, H - 0.07), GREEN))
        parts.append(block((0.045, 0.012, 0.09), (x - 0.01, plate_y - 0.018, H - 0.02), STEEL, 9, bevel=0.003))
        lid.append(block((0.06, 0.012, 0.03), (x, -((sy + 0.02) * 0.5 + 0.018), H + 0.02), STEEL, 9, bevel=0.003))
    # Hinges at the back: a knuckle on the hinge line, a leaf on the body's frame band and one on the lid's.
    hinge = ((sy + 0.02) * 0.5 + 0.012, H)
    for x in (-0.36, 0.36):
        parts.append(disc(0.013, 0.07, (x - 0.035, hinge[0], H), (1.0, 0.0, 0.0), STEEL, segments=8))
        parts.append(block((0.07, 0.006, 0.05), (x, sy * 0.5 + 0.012 + 0.003, H - 0.025), STEEL, 10))
        lid.append(block((0.07, 0.006, 0.04), (x, hinge[0] + 0.003, H + 0.028), STEEL, 10))
    # The tray inside, where the loot lies.
    parts.append(block((sx - 0.05, sy - 0.05, 0.16), (0.0, 0.0, base + 0.1), STENCIL))
    body = lp.join(prefix + 'SupplyCrate', parts)
    lp.hull_box(body, (sx + 0.04, sy + 0.04, H), (0.0, 0.0, H * 0.5))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.18))
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lm.socket(body, 'Interact', (0.0, -(sy * 0.5 + 0.03), H - 0.07))
    finish_body(body)

    lid.append(block((sx + 0.02, sy + 0.02, 0.045), (0.0, 0.0, H + 0.0225), OLIVE, 9))
    lid += wrap((sx + 0.02) * 0.5, (sy + 0.02) * 0.5, H + 0.03, 0.06, 0.012, STEEL, bevel=0.003, seed=6)
    for x in (-0.3, 0.3):
        lid.append(block((0.07, sy + 0.02, 0.02), (x, 0.0, H + 0.055), STEEL, 7, bevel=0.004))
    if mark:
        # Cream on the olive lid, so a cache reads from above and from afar: the frontier Rangers' star in a ring.
        lid.append(badge(0.105, (0.0, 0.0, H + 0.045 + LIFT), (0.0, 0.0, 0.0), CREAM))
    else:
        lid.append(text('▲  THIS SIDE UP  ▲', 0.035, (0.0, -0.09, H + 0.045 + LIFT), (0.0, 0.0, 0.0), STENCIL))
    cover = finish_lid(prefix + 'SupplyCrate_Lid', lid, hinge, OPEN_ANGLE['SupplyCrate'])
    lp.hull_box(cover, (sx + 0.05, sy + 0.05, 0.065), (0.0, -hinge[0], 0.0325))
    return [body, cover]


# --- Rare: the strongbox ---

def scratch(length, width, at, angle, rotation, lift_to):
    """A pry scratch: a thin tapered sliver of bare steel through the paint, turned by angle (degrees) in the plane
    flat() lays it on; at is its middle in that plane's 2D, lift_to the plane's 3D origin."""
    a = math.radians(angle)
    shape = [(-0.5 * length, 0.0), (-0.12 * length, -0.5 * width), (0.5 * length, 0.0), (0.1 * length, 0.5 * width)]
    points = [(at[0] + x * math.cos(a) - y * math.sin(a), at[1] + x * math.sin(a) + y * math.cos(a)) for x, y in shape]
    return flat([points], lift_to, rotation, STEEL)


def dent(rnd, at, rotation, lift_to, normal):
    """A bullet dent: the paint flaked off round it (a ragged chip of bare steel) and the strike's raised lip."""
    r = rnd.uniform(0.03, 0.036)
    corners = [(math.cos(2.0 * math.pi * k / 9) * r * rnd.uniform(0.65, 1.15),
                math.sin(2.0 * math.pi * k / 9) * r * rnd.uniform(0.65, 1.15)) for k in range(9)]
    chip = [[(at[0], at[1]), (at[0] + corners[k][0], at[1] + corners[k][1]),
             (at[0] + corners[(k + 1) % 9][0], at[1] + corners[(k + 1) % 9][1])] for k in range(9)]
    pieces = [flat(chip, lift_to, rotation, STEEL)]
    lip = lp.lathe([(0.016, 0.0), (0.0135, 0.0045), (0.008, 0.0034), (0.0, 0.001)], segments=8)[0]
    turn = Euler([math.radians(a) for a in rotation]).to_matrix()
    pieces.append(oriented(part(lip, STEEL), Vector(lift_to) + turn @ Vector((at[0], at[1], 0.0)), normal))
    return pieces


def strongbox(robbed=ROBBED, prefix=''):
    rnd = random.Random(37)
    sx, sy, sz, base, wall, lid_t = 0.82, 0.6, 0.4, 0.045, 0.045, 0.15
    H = base + sz
    parts, lid = [], []
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            parts.append(block((0.09, 0.09, base), (x * (sx * 0.5 - 0.03), y * (sy * 0.5 - 0.03), base * 0.5), IRON,
                               bevel=0.01))
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, base + sz * 0.5), SAFE, rnd.randint(0, 99)))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, base + sz * 0.5), SAFE, rnd.randint(0, 99)))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.03), (0.0, 0.0, base + 0.015), SAFE))

    def frame(z0, h, into):
        """Rounded steel corner posts, riveted on both faces."""
        for x in (-1.0, 1.0):
            for y in (-1.0, 1.0):
                into.append(block((0.075, 0.075, h), (x * (sx * 0.5 - 0.02), y * (sy * 0.5 - 0.02), z0 + h * 0.5), STEEL,
                                  rnd.randint(0, 99), bevel=0.012))
                for k in range(max(1, int(h / 0.09))):
                    z = z0 + 0.045 + k * 0.09
                    into.append(rivet((x * (sx * 0.5 - 0.02), y * (sy * 0.5 + 0.0175), z), (0.0, y, 0.0)))
                    into.append(rivet((x * (sx * 0.5 + 0.0175), y * (sy * 0.5 - 0.02), z), (x, 0.0, 0.0)))
    frame(base, sz, parts)
    for z in (base + 0.03, H - 0.03):
        parts += wrap(sx * 0.5, sy * 0.5, z, 0.05, 0.014, STEEL, gap_x=0.1, gap_y=0.1, seed=4)
        for s in (-1.0, 1.0):
            # Six rivets a band on the front, three on the back (seen least, often against a wall).
            for k in (range(6) if s < 0.0 else (0, 2.5, 5)):
                parts.append(rivet((-0.25 + k * 0.1, s * (sy * 0.5 + 0.014), z), (0.0, s, 0.0), r=0.009))
    # The dial the wheel turns on, ringed in blue; the seam under the lid.
    wz, wy = base + sz * 0.5, -(sy * 0.5)
    parts.append(disc(0.11, 0.012, (0.0, wy, wz), (0.0, -1.0, 0.0), STEEL, segments=20, back=False))
    parts.append(ring(0.095, 0.006, (0.0, wy - 0.012, wz), (0.0, -1.0, 0.0), BLUE))
    parts += wrap(sx * 0.5, sy * 0.5, H, 0.008, 0.003, BLUE, gap_x=0.15, gap_y=0.15)
    for x in (-0.24, 0.24):
        parts.append(disc(0.032, 0.13, (x - 0.065, sy * 0.5 + 0.03, H), (1.0, 0.0, 0.0), IRON, segments=12))
        parts.append(block((0.11, 0.012, 0.09), (x, sy * 0.5 + 0.012, H - 0.05), IRON, bevel=0.003))
    # The floor plate inside, where the loot lies (the backlog's felt was never seen in the dark inside).
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.06), (0.0, 0.0, base + 0.06), IRON))

    if robbed:
        # Its own seeds, so the safe under the damage is the same either way.
        hurt = random.Random(61)
        face = (0.0, wy - LIFT, 0.0)          # the front's plane: 2D (x, z)
        on_front = (90.0, 0.0, 0.0)
        # Pry scratches round the dial: short gouges off its rim, two long scrapes below it.
        for a in (18.0, 64.0, 128.0, 166.0, 212.0, 247.0, 301.0, 338.0):
            a += hurt.uniform(-12.0, 12.0)
            length = hurt.uniform(0.045, 0.09)
            r = 0.114 + length * 0.5 + hurt.uniform(0.0, 0.01)
            c = (math.cos(math.radians(a)) * r, wz + math.sin(math.radians(a)) * r)
            if abs(c[0]) < 0.33 and 0.11 < c[1] < 0.38:
                parts.append(scratch(length, hurt.uniform(0.007, 0.011), c, a + hurt.uniform(-14.0, 14.0), on_front, face))
        parts.append(scratch(0.2, 0.009, (-0.2, 0.15), -11.0, on_front, face))
        parts.append(scratch(0.14, 0.007, (-0.17, 0.124), -4.0, on_front, face))
        # Two bullet dents: low on the front, and (below) on the lid's top.
        parts += dent(hurt, (0.24, 0.17), on_front, face, (0.0, -1.0, 0.0))
        # The padlock hasp's staple, snapped: its plate on the top band, one leg a stub, the other bent down.
        band = -(sy * 0.5 + 0.014)
        parts.append(block((0.05, 0.006, 0.04), (0.0, band - 0.003, H - 0.03), STEEL, 14, bevel=0.002))
        parts.append(disc(0.0055, 0.011, (-0.013, band - 0.006, H - 0.03), (0.0, -1.0, 0.0), STEEL, segments=6))
        parts.append(part(lp.sweep([(0.013, band - 0.005, H - 0.03), (0.013, band - 0.019, H - 0.031),
                                    (0.009, band - 0.024, H - 0.045)], lp.ngon(0.0055, 6)), STEEL))

    body = lp.join(prefix + 'Strongbox', parts)
    lp.hull_box(body, (sx + 0.04, sy + 0.04, H), (0.0, 0.0, H * 0.5))
    hinge = (sy * 0.5 + 0.03, H)
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.09))
    lm.socket(body, 'Wheel', (0.0, wy - 0.012, wz))
    lm.socket(body, 'Interact', (0.0, wy - 0.09, wz))
    finish_body(body)

    lid.append(block((sx, sy, lid_t), (0.0, 0.0, H + lid_t * 0.5), SAFE, 12, bevel=0.008))
    frame(H, lid_t, lid)
    lid += wrap(sx * 0.5, sy * 0.5, H + lid_t - 0.025, 0.05, 0.014, STEEL, gap_x=0.1, gap_y=0.1, seed=4)
    for x in (-0.18, 0.18):
        lid.append(block((0.07, sy - 0.1, 0.02), (x, 0.0, H + lid_t + 0.01), STEEL, 13, bevel=0.005))
        for y in (-0.2, 0.0, 0.2):
            lid.append(rivet((x, y, H + lid_t + 0.02), (0.0, 0.0, 1.0)))
    for x in (-0.24, 0.24):
        lid.append(block((0.11, 0.012, 0.09), (x, sy * 0.5 + 0.012, H + 0.05), IRON, bevel=0.003))
    if robbed:
        face = (0.0, wy - LIFT, 0.0)
        on_front = (90.0, 0.0, 0.0)
        # Where the bar went in under the lid's front edge: gouges up from the seam.
        for x, a, length in ((-0.215, 78.0, 0.05), (-0.19, 97.0, 0.034), (-0.165, 70.0, 0.042), (0.16, 104.0, 0.046),
                             (0.19, 86.0, 0.03), (0.27, 64.0, 0.05)):
            z = H + 0.012 + length * 0.5 * math.sin(math.radians(a))
            lid.append(scratch(length, 0.006, (x, z), a, on_front, face))
        lid.append(scratch(0.15, 0.005, (0.13, H + 0.085), 3.0, on_front, face))
        lid += dent(hurt, (-0.03, -0.11), (0.0, 0.0, 0.0), (0.0, 0.0, H + lid_t + LIFT), (0.0, 0.0, 1.0))
        # The hasp, wrenched: its hinge plate and knuckle on the lid's front, the strap torn through its slot (two
        # ragged prongs left) and hanging askew, bent out from the box.
        lid.append(block((0.07, 0.008, 0.035), (0.0, wy - 0.004, H + 0.07), STEEL, 15, bevel=0.002))
        pivot = Vector((0.0, wy - 0.012, H + 0.055))
        lid.append(disc(0.009, 0.06, (-0.03, pivot.y, pivot.z), (1.0, 0.0, 0.0), STEEL, segments=8))
        strap = [_rect(-0.0225, -0.085, 0.0225, -0.004),
                 [(-0.0225, -0.085), (-0.009, -0.085), (-0.011, -0.104), (-0.0225, -0.116)],
                 [(0.009, -0.085), (0.0225, -0.085), (0.0225, -0.1), (0.012, -0.096)]]
        turn = Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(-33.0), 4, 'X') @ \
            Matrix.Rotation(math.radians(21.0), 4, 'Y') @ Matrix.Rotation(math.radians(90.0), 4, 'X') @ \
            Matrix.Translation((0.0, 0.0, -0.003))
        lid.append(slab(strap, 0.006, turn, STEEL, 16))
    cover = finish_lid(prefix + 'Strongbox_Lid', lid, hinge, OPEN_ANGLE['Strongbox'])
    lp.hull_box(cover, (sx + 0.04, sy + 0.04, lid_t), (0.0, -hinge[0], lid_t * 0.5))

    # The wheel, round its own origin, facing -Y: a hub, three spokes, brass knobs.
    wheel_parts = [disc(0.045, 0.04, (0.0, 0.0, 0.0), (0.0, -1.0, 0.0), IRON, segments=16)]
    for k in range(3):
        a = math.radians(90.0 + k * 120.0)
        spoke = lp.block((0.014, 0.014, 0.12), (0.0, 0.0, 0.06))
        lp.place(spoke, (0.0, -0.033, 0.0), (0.0, 90.0 - math.degrees(a), 0.0))
        wheel_parts.append(part(spoke, IRON))
        wheel_parts.append(disc(0.018, 0.03, (math.cos(a) * 0.12, -0.048, math.sin(a) * 0.12), (0.0, -1.0, 0.0), BRASS,
                                segments=10))
    wheel = lp.join(prefix + 'Strongbox_Wheel', wheel_parts)
    lm.smooth(wheel, 35.0)
    lt.bake_vertex_ao(wheel, distance=0.05, ground=False)
    wheel['Nanite'] = 0
    wheel['LODs'] = '50,25'
    wheel['Collision'] = 'None'
    wheel.location = (0.0, wy - 0.012, wz)
    return [body, cover, wheel]


# --- Legendary: the reliquary (previews only) ---

def reliquary():
    gold = lt.material('MetalWorn', name='ChestGold', tint=0xffc04a)
    granite = lt.material('RockGranite', name='ChestGranite')
    rune_stone = lt.material('RockGranite', name='ChestRuneStone', tint=0x4a423b)
    orange = lm.material('ChestGlowLegendary', 0xffa62e, Glow=4.0, Variation=0.0)
    rnd = random.Random(53)
    sx, sy, sz, wall, base = 1.02, 0.62, 0.42, 0.07, 0.2
    H = base + sz
    ox, oy = sx * 0.5, sy * 0.5
    parts, lid = [], []
    parts.append(block((1.32, 0.88, 0.13), (0.0, 0.0, 0.065), granite, 1, bevel=0.02))
    parts.append(block((1.2, 0.76, 0.07), (0.0, 0.0, 0.165), granite, 2, bevel=0.015))
    parts += wrap(0.6, 0.38, 0.19, 0.02, 0.008, gold, bevel=0.002)
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, base + sz * 0.5), granite, rnd.randint(0, 99), bevel=0.008))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, base + sz * 0.5), granite, rnd.randint(0, 99), bevel=0.008))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.04), (0.0, 0.0, base + 0.02), granite, 7))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.03), (0.0, 0.0, base + 0.055), gold, 8, bevel=0.003))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            cx, cy = x * (ox + 0.005), y * (oy + 0.005)
            parts.append(disc(0.038, sz - 0.08, (cx, cy, base + 0.04), (0.0, 0.0, 1.0), gold, segments=14))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, base + 0.0225), gold, bevel=0.006))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, H - 0.0225), gold, bevel=0.006))
    for z in (base + 0.035, H - 0.035):
        parts += wrap(ox, oy, z, 0.025, 0.008, gold, gap_x=0.1, gap_y=0.1, bevel=0.002)
    # The rune: a glowing diamond in a gold frame, lines running off it to the corners and round the ends.
    fy, cz = -(oy + 0.004), base + sz * 0.5
    parts.append(block((0.2, 0.012, 0.2), (0.0, fy, cz), gold, rotation=(0.0, 45.0, 0.0), bevel=0.004))
    parts.append(block((0.15, 0.006, 0.15), (0.0, fy - 0.009, cz), rune_stone, rotation=(0.0, 45.0, 0.0)))
    r = 0.07
    for xs in (-1.0, 1.0):
        for zs in (-1.0, 1.0):
            parts.append(block((r * 1.414 + 0.012, 0.004, 0.012), (xs * r * 0.5, fy - 0.013, cz + zs * r * 0.5), orange,
                               rotation=(0.0, 45.0 * xs * zs, 0.0)))
    parts.append(gem(0.018, 0.03, (0.0, fy - 0.014, cz), (0.0, -1.0, 0.0), orange, sides=4))
    for s in (-1.0, 1.0):
        parts.append(block((0.22, 0.004, 0.01), (s * 0.24, fy - 0.001, cz), orange))
        parts.append(block((0.01, 0.004, 0.12), (s * 0.35, fy - 0.001, cz), orange))
        parts.append(block((0.004, 0.16, 0.01), (s * (ox + 0.001), 0.0, cz), orange))
    for x in (-0.3, 0.3):
        parts.append(disc(0.03, 0.14, (x - 0.07, oy + 0.045, H), (1.0, 0.0, 0.0), gold, segments=12))
    body = lp.join('Reliquary', parts)
    lp.hull_box(body, (1.32, 0.88, 0.2), (0.0, 0.0, 0.1))
    lp.hull_box(body, (sx + 0.04, sy + 0.04, sz), (0.0, 0.0, base + sz * 0.5))
    hinge = (oy + 0.045, H)
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.07))
    lm.socket(body, 'Crystal', (0.0, 0.0, H + 0.55))
    finish_body(body)

    lid.append(block((sx + 0.07, sy + 0.07, 0.07), (0.0, 0.0, H + 0.035), granite, 11, bevel=0.012))
    lid += wrap(ox + 0.035, oy + 0.035, H + 0.035, 0.03, 0.008, gold, bevel=0.002)
    lid.append(block((sx - 0.14, sy - 0.16, 0.09), (0.0, 0.0, H + 0.115), granite, 12, bevel=0.025))
    lid.append(block((sx - 0.4, 0.05, 0.035), (0.0, 0.0, H + 0.1775), gold, bevel=0.008))
    lid.append(gem(0.04, 0.03, (0.0, 0.0, H + 0.2), (0.0, 0.0, 1.0), orange, sides=4))
    cover = finish_lid('Reliquary_Lid', lid, hinge, OPEN_ANGLE['Reliquary'])
    lp.hull_box(cover, (sx + 0.08, sy + 0.08, 0.16), (0.0, -hinge[0], 0.08))

    crystal = gem(0.075, 0.19, (0.0, 0.0, 0.0), (0.0, 0.0, 1.0), orange, sides=6)
    crystal.name = crystal.data.name = 'Reliquary_Crystal'
    lm.smooth(crystal, 20.0)
    lt.bake_vertex_ao(crystal, samples=4, ground=False)
    crystal['Nanite'] = 0
    crystal['Collision'] = 'None'
    crystal.location = (0.0, 0.0, H + 0.55)
    return [body, cover, crystal]


def report(models):
    for obj in models:
        lo, hi = lt._bounds([obj])
        slots = [s.material.name for s in obj.material_slots]
        lt._log(f'{obj.name}: {lp.tri_count(obj)} triangles, {(hi - lo).x:.3f} x {(hi - lo).y:.3f} x {(hi - lo).z:.3f} m, '
                f"materials {', '.join(slots)}")


crate = supply_crate()
safe = strongbox()
report(crate + safe)


# --- Previews ---

PREVIEWS = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Chests')
OVERVIEW = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Chests_overview.png')
BUILDINGS_EXPORT = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_Buildings')
SCRUB_EXPORT = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_RR_Scrub')


def toward_sun(bearing=247.5, elevation=15.0):
    """Toward Ransom's Rest's golden late-afternoon sun, in the stage's frame (x east, y north: the chests face south)."""
    b, e = math.radians(bearing), math.radians(elevation)
    return Vector((math.sin(b) * math.cos(e), math.cos(b) * math.cos(e), math.sin(e)))


# The level's painted sky (Farmhouse.py's and TownGate.py's): colours by the view's height, glows round the sun.
GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]


def sky(sun):
    world = bpy.data.worlds.get('_Golden') or bpy.data.worlds.new('_Golden')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = GOLDEN_SKY[0][0], lt.hex_color(GOLDEN_SKY[0][1])
    elements[1].position, elements[1].color = GOLDEN_SKY[-1][0], lt.hex_color(GOLDEN_SKY[-1][1])
    for position, color in GOLDEN_SKY[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = sun
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in GOLDEN_GLOWS:
        lift = nodes.new('ShaderNodeMath')
        lift.operation = 'POWER'
        links.new(clamp.outputs['Value'], lift.inputs[0])
        lift.inputs[1].default_value = power
        scale = nodes.new('ShaderNodeMath')
        scale.operation = 'MULTIPLY'
        links.new(lift.outputs['Value'], scale.inputs[0])
        scale.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(scale.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def white_col(mesh):
    col = mesh.color_attributes.get('Col') or mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def stage():
    """Eevee with screen-space ray tracing and AgX, the street's dirt for ground, the golden sun and sky, hulls hidden
    and the lights glowing."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 80
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    for mat in bpy.data.materials:
        if mat.get('Glow') and mat.use_nodes:
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
            # Eevee's AgX turns a strong emission white; held lower, the rarity's hue still reads, as it must.
            bsdf.inputs['Emission Strength'].default_value = min(float(mat['Glow']), 0.6)
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-60.0, -60.0, 0.0), (60.0, -60.0, 0.0), (60.0, 60.0, 0.0), (-60.0, 60.0, 0.0)], [], [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    lt.assign(ground, lt.material('GroundDirt'))
    lt.box_uv(ground, 'GroundDirt')
    white_col(mesh)
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.color, sun.data.angle = 4.4, (1.0, 0.8, 0.56), math.radians(2.0)
    sun.rotation_euler = (-toward_sun()).to_track_quat('-Z', 'Y').to_euler()
    scene.world = sky(toward_sun())
    return ground


def figure():
    """A 1.8 m figure for scale (TownGate.py's): a plain clay mannequin standing at the origin, facing -Y."""
    bm = bmesh.new()

    def limb(p0, p1, r0, r1, segments=10):
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        rot = Vector((0.0, 0.0, 1.0)).rotation_difference(d).to_matrix().to_4x4()
        bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=r0, radius2=r1, depth=d.length,
                              matrix=Matrix.Translation((p0 + p1) * 0.5) @ rot)

    def blob(c, r, scale=(1.0, 1.0, 1.0)):
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=r,
                                  matrix=Matrix.Translation(c) @ Matrix.Diagonal((*scale, 1.0)))
    for sx in (-1.0, 1.0):
        blob((sx * 0.1, -0.05, 0.045), 0.06, (0.95, 2.1, 0.75))
        limb((sx * 0.1, 0.0, 0.06), (sx * 0.1, 0.0, 0.5), 0.05, 0.062)
        limb((sx * 0.1, 0.0, 0.5), (sx * 0.1, 0.0, 0.93), 0.064, 0.088)
        blob((sx * 0.2, 0.0, 1.42), 0.068)
        limb((sx * 0.21, 0.0, 1.42), (sx * 0.24, 0.012, 1.12), 0.05, 0.044)
        limb((sx * 0.24, 0.012, 1.12), (sx * 0.255, -0.02, 0.86), 0.042, 0.034)
        blob((sx * 0.258, -0.022, 0.81), 0.045, (0.8, 0.9, 1.2))
    blob((0.0, 0.0, 0.96), 0.17, (1.12, 0.72, 0.62))
    limb((0.0, 0.0, 0.96), (0.0, 0.0, 1.45), 0.15, 0.19, segments=12)
    limb((0.0, 0.0, 1.46), (0.0, 0.0, 1.58), 0.055, 0.05)
    blob((0.0, -0.01, 1.68), 0.11, (0.9, 1.0, 1.09))
    bmesh.ops.scale(bm, vec=(1.0, 0.68, 1.0), verts=[v for v in bm.verts if 0.94 < v.co.z < 1.47 and abs(v.co.x) < 0.2])
    mesh = bpy.data.meshes.new('_Human')
    bm.to_mesh(mesh)
    bm.free()
    for poly in mesh.polygons:
        poly.use_smooth = True
    obj = bpy.data.objects.new('_Human', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mat = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x8a847a)
    bsdf.inputs['Roughness'].default_value = 0.85
    mesh.materials.append(mat)
    return obj


def socket_of(obj, name):
    return next(c for c in obj.children if c.name.split('.')[0] == 'SOCKET_' + name)


def set_up(group, at=(0.0, 0.0, 0.0), yaw=0.0, opened=0.0, spin=0.0):
    """Stands a chest (body, lid, wheel) at `at`, turned by yaw (degrees), its lid opened by `opened` (0..1 of its
    angle) on SOCKET_Lid and its wheel turned by spin (degrees) on SOCKET_Wheel."""
    body = group[0]
    body.matrix_world = Matrix.Translation(at) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
    bpy.context.view_layer.update()
    for obj in group[1:]:
        if obj.name.endswith('_Lid'):
            angle = OPEN_ANGLE[body.name.lstrip('_').replace('Before', '')] * opened
            obj.matrix_world = socket_of(body, 'Lid').matrix_world @ Matrix.Rotation(math.radians(-angle), 4, 'X')
        elif obj.name.endswith('_Wheel'):
            obj.matrix_world = socket_of(body, 'Wheel').matrix_world @ Matrix.Rotation(math.radians(spin), 4, 'Y')
        elif obj.name.endswith('_Crystal'):
            obj.matrix_world = socket_of(body, 'Crystal').matrix_world
    bpy.context.view_layer.update()


def shoot(path, shown, eye, target, lens, size=(1600, 1200), exposure=0.0, sky_light=1.0):
    """Renders the objects in shown (and their parts) from eye toward target; sky_light scales the sky's light (Eevee
    lets it leak through walls, so an interior turns it down)."""
    scene = bpy.context.scene
    next(n for n in scene.world.node_tree.nodes if n.type == 'BACKGROUND').inputs['Strength'].default_value = sky_light
    visible = set()
    for obj in shown:
        visible.add(obj)
        visible.update(c for c in obj.children_recursive if c.type == 'MESH' and not c.name.startswith('UCX_'))
    for o in scene.objects:
        if o.type == 'MESH':
            o.hide_render = o not in visible
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 2000.0
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    lt._log(f'chests: rendered {path}')
    return path


def load_png(path):
    import numpy as np
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1, :, :3].copy()


def shrink(img, k):
    h, w = img.shape[0] // k * k, img.shape[1] // k * k
    return img[:h, :w].reshape(h // k, k, w // k, k, 3).mean(axis=(1, 3))


def label(img, text, x, y, cap=40, color=(0.95, 0.65, 0.29)):
    import looter_posters as posters
    posters.sheet_label(img, text, x, y, cap, color=color)


def save_png(img, path):
    import numpy as np
    lt.write_png(path, lt.to8(np.clip(img, 0.0, 1.0)))
    lt._log(f'chests: {path}')


def pair(left, right, labels, out, gap=10):
    """Two renders side by side, labelled in Rye (TownGate.py's combine)."""
    import numpy as np
    a, b = load_png(left), load_png(right)
    row = np.concatenate([a, np.full((a.shape[0], gap, 3), 0.08, np.float32), b], axis=1)
    row[:96] *= 0.5   # a dark band behind the labels, so they read over a bright sky
    label(row, labels[0], 34, 70, 44)
    label(row, labels[1], a.shape[1] + gap + 34, 70, 44)
    save_png(row, out)
    return row


def manifest_materials(folder):
    """The materials of every manifest in an export folder, made again by name (the FBX brings only names)."""
    made = {}
    for name in sorted(os.listdir(folder)):
        if not name.endswith('.json'):
            continue
        with open(os.path.join(folder, name), encoding='utf-8') as file:
            for mat_name, info in json.load(file).get('materials', {}).items():
                textures = info.get('Textures', {}).get('BaseColorMap', '')
                if not textures.startswith('Art/Textures/'):
                    continue
                tint = info.get('Tint') or [1.0, 1.0, 1.0, 1.0]
                to_srgb = [round(255.0 * (12.92 * c if c <= 0.0031308 else 1.055 * c ** (1.0 / 2.4) - 0.055)) for c in tint[:3]]
                value = (to_srgb[0] << 16) | (to_srgb[1] << 8) | to_srgb[2]
                made[mat_name] = lt.material(textures.split('/')[2], name=mat_name, master=info.get('Master'),
                                             tint=None if value == 0xffffff else value,
                                             uv_scale=info.get('UVScale', 1.0))
    return made


def import_fbx(path, materials):
    """An exported model brought back (its hulls hidden, its materials made again, its vertex colours as 'Col')."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path, colors_type='LINEAR')
    found = sorted((o for o in bpy.data.objects if o not in before), key=lambda o: o.name)
    model = next(o for o in found if o.type == 'MESH' and o.name.startswith('SM_'))
    for o in found:
        if o is not model:
            o.hide_render = True
    for slot in model.material_slots:
        name = slot.material.name.split('.')[0] if slot.material else ''
        if name in materials:
            slot.material = materials[name]
    cols = model.data.color_attributes
    if cols and 'Col' not in cols:
        cols[0].name = 'Col'
    return model


def copy_of(src, at, yaw=0.0, scale=1.0):
    """A copy of an imported model moved from where it came in (its own transform kept) to at, turned and scaled."""
    obj = src.copy()
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = None
    home = Matrix([src['_home'][k * 4:k * 4 + 4] for k in range(4)])
    obj.matrix_world = Matrix.LocRotScale(Vector(at), Matrix.Rotation(math.radians(yaw), 3, 'Z'),
                                          Vector((scale,) * 3)) @ home
    obj.hide_render = False
    return obj


def office_stand_in():
    """The sheriff's office (FalseFronts.py's FalseFront_Sheriff has no interior): a plank floor, a fieldstone back
    wall and a west wall with a barred window, a board ceiling. The chest stands with its back to the back wall."""
    trim = lp.trim_material()
    pieces = []
    floor = lp.block((6.2, 5.4, 0.06), (0.35, -1.7, -0.03))
    lt.assign(floor, lt.material('WoodPlanks'))
    lt.box_uv(floor, 'WoodPlanks', along='long')
    pieces.append(floor)
    ceiling = lp.block((6.2, 5.4, 0.06), (0.35, -1.7, 3.03))
    lt.assign(ceiling, lt.material('WoodPlanks'))
    lt.box_uv(ceiling, 'WoodPlanks', along='long')
    pieces.append(ceiling)
    stone = []
    stone.append(lp.block((6.6, 0.3, 3.2), (0.4, 0.82, 1.5)))                 # the back wall, its face at y 0.67
    wx, wy0, wy1, sill, head = -2.45, -1.85, -0.95, 0.95, 2.15                # the west wall's window
    for y0, y1, z0, z1 in ((-4.4, wy0, -0.1, 3.1), (wy1, 0.97, -0.1, 3.1), (wy0, wy1, -0.1, sill), (wy0, wy1, head, 3.1)):
        stone.append(lp.block((0.3, y1 - y0, z1 - z0), (wx - 0.15, (y0 + y1) * 0.5, (z0 + z1) * 0.5)))
    for s in stone:
        lt.assign(s, trim)
        lt.trim_uv(s, None, 'Stone', align='world', cut=True)
    pieces += stone
    for k in range(5):
        y = wy0 + (wy1 - wy0) * (k + 1) / 6
        bar = lp.block((0.024, 0.024, head - sill), (wx - 0.12, y, (sill + head) * 0.5))
        lt.assign(bar, IRON)
        lt.box_uv(bar, 'MetalWorn')
        pieces.append(bar)
    room = lp.join('_Office', pieces)
    lm.smooth(room, 35.0)
    white_col(room.data)
    return room


def previews():
    os.makedirs(PREVIEWS, exist_ok=True)
    ground = stage()
    human = figure()
    before_crate = supply_crate(mark=not RANGER_MARK, prefix='_Before')
    before_safe = strongbox(robbed=not ROBBED, prefix='_Before')
    relic = reliquary()
    away = (0.0, 40.0, 0.0)
    for group in (before_crate, before_safe, relic):
        set_up(group, away)
    path = lambda name: os.path.join(PREVIEWS, name + '.png')
    shots = {}

    # Closed and open, a 1.8 m figure beside: turned so the low sun comes from the chest's front left, seen from its
    # front right three-quarter at a standing eye (the right end in shade gives the form).
    yaw = -22.5
    turn = lambda offset: Matrix.Rotation(math.radians(yaw), 3, 'Z') @ Vector(offset)
    for group, name, eye, target, stand in ((crate, 'SupplyCrate', (1.7, -2.95, 1.55), (-0.15, 0.0, 0.78), (-1.1, 0.6)),
                                            (safe, 'Strongbox', (1.35, -2.45, 1.45), (-0.12, 0.0, 0.75), (-0.95, 0.5))):
        to_eye = turn(eye) - turn((stand[0], stand[1], 0.0))
        human.matrix_world = Matrix.Translation(turn((stand[0], stand[1], 0.0))) @ \
            Matrix.Rotation(math.atan2(to_eye.x, -to_eye.y) - math.radians(25.0), 4, 'Z')
        for opened, suffix in ((0.0, ''), (1.0, '_Open')):
            set_up(group, (0.0, 0.0, 0.0), yaw=yaw, opened=opened, spin=90.0 * opened)
            shots[name + suffix] = shoot(path(name + suffix), group + [ground, human], turn(eye), turn(target), 36.0)
        set_up(group, away)
    human.location = away

    # Each story touch off and on, close (the switch's other setting is built only for this).
    halves = {}
    for group, on in ((before_crate, not RANGER_MARK), (crate, RANGER_MARK)):
        set_up(group, (0.0, 0.0, 0.0), yaw=yaw)
        halves[on] = shoot(path(f'_mark_{on}'), group + [ground], turn((0.55, -1.6, 1.3)), turn((0.0, 0.0, 0.3)), 42.0,
                           size=(1200, 1200))
        set_up(group, away)
    pair(halves[False], halves[True], ['BACKLOG STENCILS', 'RIM RANGERS MARK'], path('SupplyCrate_Mark'))
    temporary = list(halves.values())
    halves = {}
    for group, on in ((before_safe, not ROBBED), (safe, ROBBED)):
        set_up(group, (0.0, 0.0, 0.0), yaw=yaw)
        halves[on] = shoot(path(f'_robbed_{on}'), group + [ground], turn((0.42, -1.3, 1.12)), turn((0.0, 0.0, 0.36)),
                           42.0, size=(1200, 1200))
        set_up(group, away)
    pair(halves[False], halves[True], ['CLEAN', 'ROBBED'], path('Strongbox_Robbed'))
    temporary += list(halves.values())

    # A cache under the windmill: between its legs, in front of the pump, scrub round the footings.
    mill_fbx = os.path.join(BUILDINGS_EXPORT, 'SM_Windmill.fbx')
    if os.path.exists(mill_fbx):
        mill = import_fbx(mill_fbx, manifest_materials(BUILDINGS_EXPORT))
        lt._log(f'chests: windmill at {tuple(round(v, 3) for v in mill.matrix_world.translation)}, '
                f'size {tuple(round(v, 2) for v in mill.dimensions)}')
        extras = [mill]
        if os.path.isdir(SCRUB_EXPORT):
            scrub_mats = manifest_materials(SCRUB_EXPORT)
            plants = {n: import_fbx(os.path.join(SCRUB_EXPORT, f'SM_{n}.fbx'), scrub_mats)
                      for n in ('Sagebrush_A', 'Sagebrush_B', 'DryTuft_A', 'DryTuft_B', 'Rabbitbrush_A')}
            for p in plants.values():
                p['_home'] = [v for row in p.matrix_world for v in row]
                p.hide_render = True
            for name, at, turn_by, scale in (('Sagebrush_A', (-1.85, -1.75, 0.0), 20.0, 1.0),
                                             ('Sagebrush_B', (2.0, -0.7, 0.0), 140.0, 0.9),
                                             ('Rabbitbrush_A', (1.55, 1.7, 0.0), 60.0, 1.0),
                                             ('DryTuft_A', (-0.95, -1.55, 0.0), 0.0, 1.0),
                                             ('DryTuft_B', (0.95, -1.35, 0.0), 80.0, 1.1),
                                             ('DryTuft_A', (-0.85, 0.3, 0.0), 200.0, 0.9),
                                             ('DryTuft_B', (0.4, 0.95, 0.0), 30.0, 1.0),
                                             ('DryTuft_A', (1.45, -1.55, 0.0), 120.0, 0.8),
                                             ('DryTuft_B', (-1.55, -0.3, 0.0), 260.0, 1.0)):
                extras.append(copy_of(plants[name], at, turn_by, scale))
        # At the foot of the south face, between the front footings: outside the windmill's collision hull, which
        # wraps the whole lattice (a crate between the legs would be out of reach).
        set_up(crate, (0.08, -1.72, 0.0), yaw=yaw)
        human.location = away
        shots['SupplyCrate_Windmill'] = shoot(path('SupplyCrate_Windmill'), crate + [ground] + extras,
                                              (1.75, -4.55, 1.3), (-0.15, -1.35, 1.12), 28.0)
        set_up(crate, away)
        for o in extras:
            o.hide_render = True
            o.location = away
    else:
        lt._log(f'note: no {mill_fbx}: the windmill view needs the buildings export')

    # On the sheriff's office floor, the late sun through the barred west window.
    room = office_stand_in()
    set_up(safe, (1.45, 0.3, 0.0), yaw=-8.0)
    shots['Strongbox_Office'] = shoot(path('Strongbox_Office'), safe + [room], (3.05, -2.75, 1.5), (0.3, -0.05, 0.55),
                                      24.0, exposure=1.1, sky_light=0.35)
    set_up(safe, away)
    room.location = away

    # The reliquary, kept for later (never exported): closed and open.
    for opened, name in ((0.0, '_relic_closed'), (1.0, '_relic_open')):
        set_up(relic, (0.0, 0.0, 0.0), yaw=-14.0, opened=opened)
        temporary.append(shoot(path(name), relic + [ground], (-1.25, -2.3, 1.45), (0.0, 0.0, 0.6), 40.0,
                               size=(1200, 1200)))
    set_up(relic, away)
    pair(temporary[-2], temporary[-1], ['RELIQUARY (NOT EXPORTED)', 'OPEN'], path('Reliquary'))

    for p in temporary:
        if os.path.exists(p):
            os.remove(p)
    overview(shots)


def overview(shots):
    """The sheet: each chest closed, open and staged, then the two story touches."""
    import numpy as np
    gap = 12
    rows = []
    for names, labels in ((('SupplyCrate', 'SupplyCrate_Open', 'SupplyCrate_Windmill'),
                           ('SUPPLY CRATE', 'OPEN', 'A CACHE UNDER THE WINDMILL')),
                          (('Strongbox', 'Strongbox_Open', 'Strongbox_Office'),
                           ('STRONGBOX', 'OPEN', "THE SHERIFF'S OFFICE"))):
        panels = []
        for name, text in zip(names, labels):
            p = shots.get(name)
            img = shrink(load_png(p), 2) if p and os.path.exists(p) else np.full((600, 800, 3), 0.1, np.float32)
            img[:70] *= 0.5
            label(img, text, 24, 52, 30)
            panels.append(img)
        rows.append(np.concatenate([x for p in panels for x in (p, np.full((p.shape[0], gap, 3), 0.06, np.float32))][:-1],
                                   axis=1))
    width = rows[0].shape[1]
    touches = []
    for name in ('SupplyCrate_Mark', 'Strongbox_Robbed'):
        img = load_png(os.path.join(PREVIEWS, name + '.png'))
        touches.append(shrink(img, 2))
    row = np.concatenate([touches[0], np.full((touches[0].shape[0], gap, 3), 0.06, np.float32), touches[1]], axis=1)
    if row.shape[1] != width:
        pad = width - row.shape[1]
        row = np.concatenate([row, np.full((row.shape[0], max(pad, 0), 3), 0.06, np.float32)], axis=1)[:, :width]
    rows.append(row)
    sheet = np.concatenate([x for r in rows for x in (r, np.full((gap, width, 3), 0.06, np.float32))][:-1], axis=0)
    save_png(sheet, OVERVIEW)


if lt.want_preview():
    previews()
