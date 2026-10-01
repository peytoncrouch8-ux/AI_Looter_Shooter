"""Three openable loot chests the user picked from the concept renders (2026-09-30), kept for later: nothing in the
game uses them yet. A scripted model file (Art/README.md) in Art/Backlog, where Tools/models.ps1 doesn't look; see
Art/Backlog/README.md for bringing them in.

  SupplyCrate        Uncommon. A 1.15 m olive steel crate on runners: steel edge frame, a cream stencil band like the
                     ammo cans, end handles, green lights on the two latches, a foam bed inside.
  SupplyCrate_Lid    its lid: an olive panel in a steel frame with two ribs and a stencil.
  Strongbox          Rare. A 0.82 m riveted steel safe in weathered teal paint on iron feet: framed corners, riveted
                     bands, a blue-ringed dial on the front, a blue seam under the lid, felt inside.
  Strongbox_Lid      its lid: a thick steel cap in its own riveted frame, two ribs.
  Strongbox_Wheel    the vault wheel (hub, three spokes, brass knobs), to spin as the box opens.
  Reliquary          Legendary. A 1.02 m granite chest on a stepped plinth: gold corner pillars and bands, glowing
                     orange rune lines round a gold diamond on the front, gold lining inside.
  Reliquary_Lid      its lid: two stepped granite slabs with a gold rim and ridge, a small orange gem.
  Reliquary_Crystal  the orange crystal that floats above the lid (meant to bob and spin).

Pivots: a chest's body sits on the ground at the middle of its footprint, front to -Y. A lid's origin is its hinge (the
back top edge of the body): attach it at the body's SOCKET_Lid and pitch it about the hinge, up to the opening angle
in OPEN_ANGLE (degrees). Other sockets: SOCKET_Loot (the middle of the floor inside: where loot and the loot beam come
from), SOCKET_Wheel (the strongbox's wheel, spinning about its forward axis) and SOCKET_Crystal (where the reliquary's
crystal hovers). Rarity shows only as the color of the lights and glow lines, so chests keep rarity colors as the
game's one color code.

    blender -b --factory-startup --python Art/Backlog/Loot/Chests.py -- --preview
"""
import math
import random

import bmesh
import bpy
from mathutils import Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp

# How far each lid opens (degrees about its hinge).
OPEN_ANGLE = {'SupplyCrate': 112.0, 'Strongbox': 102.0, 'Reliquary': 103.0}

# Tinted texture sets (Art/Textures): one worn-metal and one worn-paint texture make every metal and paint here.
OLIVE = lt.material('MetalRust', name='ChestOlive', tint=0x9aa266, uv_scale=2.5)
STEEL = lt.material('MetalRust', name='ChestSteel', tint=0x77756f)
SAFE = lt.material('MetalRust', name='ChestSafeSteel', tint=0xa3b1b2, uv_scale=1.8)
CREAM = lt.material('PaintWorn', name='ChestCream', tint=0xe9dfc4)
STENCIL = lt.material('PaintWorn', name='ChestStencil', tint=0x31352a)
IRON = lt.material('MetalWorn', name='ChestIron', tint=0x4a4540)
GOLD = lt.material('MetalWorn', name='ChestGold', tint=0xffc04a)
BRASS = lt.material('MetalWorn', name='ChestBrass', tint=0xd4ab66)
FOAM = lt.material('Polymer', name='ChestFoam', tint=0x4a4d4b)
FELT = lt.material('Polymer', name='ChestFelt', tint=0x4a3a52)
GRANITE = lt.material('RockGranite', name='ChestGranite')
RUNE_STONE = lt.material('RockGranite', name='ChestRuneStone', tint=0x4a423b)
# Lights and rune lines: opaque emissive surfaces (Nanite can't draw the additive Glow kind).
GREEN = lm.material('ChestGlowUncommon', 0x56e256, Glow=4.0, Variation=0.0)
BLUE = lm.material('ChestGlowRare', 0x6b95ff, Glow=4.0, Variation=0.0)
ORANGE = lm.material('ChestGlowLegendary', 0xffa62e, Glow=4.0, Variation=0.0)

TILEABLE = {OLIVE: 'MetalRust', STEEL: 'MetalRust', SAFE: 'MetalRust', CREAM: 'PaintWorn', STENCIL: 'PaintWorn',
            IRON: 'MetalWorn', GOLD: 'MetalWorn', BRASS: 'MetalWorn', FOAM: 'Polymer', FELT: 'Polymer',
            GRANITE: 'RockGranite', RUNE_STONE: 'RockGranite'}


# --- Parts ---

def part(obj, mat, seed=0):
    """Puts mat on a part and maps it at world scale (small parts: the texture's own density)."""
    lt.assign(obj, mat)
    if mat in TILEABLE:
        lt.box_uv(obj, TILEABLE[mat], seed=seed)
    return obj


def block(size, center, mat, seed=0, bevel=0.0, rotation=(0.0, 0.0, 0.0)):
    return part(lp.block(size, center, rotation, bevel=bevel), mat, seed)


def oriented(obj, center, normal):
    """Turns a part built round +Z to face normal, then moves it to center."""
    q = Vector(normal).normalized().to_track_quat('Z', 'Y')
    obj.data.transform(q.to_matrix().to_4x4())
    return lp.place(obj, center)


def rivet(center, normal, r=0.011, mat=IRON):
    obj = lp.lathe([(0.0, 0.0), (r, 0.0), (r * 0.85, r * 0.45), (r * 0.4, r * 0.65), (0.0, r * 0.7)], segments=8)[0]
    return oriented(part(obj, mat), center, normal)


def ring(major, minor, center, normal, mat, segments=20, sides=6):
    profile = [(major + math.cos(a) * minor, math.sin(a) * minor) for a in (2.0 * math.pi * k / sides for k in range(sides))]
    obj = lp.lathe(profile, segments=segments, closed=True)[0]
    return oriented(part(obj, mat), center, normal)


def disc(radius, depth, center, normal, mat, segments=20):
    obj = lp.lathe([(0.0, 0.0), (radius, 0.0), (radius, depth), (0.0, depth)], segments=segments)[0]
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


def text(body, size, center, rotation, mat):
    """Stencil lettering in the UI font (Chakra Petch), a hair proud of the surface."""
    font = bpy.data.fonts.load(lt.REPO.replace('\\', '/') + '/Content/UI/Fonts/ChakraPetch-Bold.ttf', check_existing=True)
    curve = bpy.data.curves.new('_text', 'FONT')
    curve.body, curve.font, curve.size = body, font, size
    curve.align_x, curve.align_y = 'CENTER', 'CENTER'
    curve.extrude = 0.0006
    src = bpy.data.objects.new('_text', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(src.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    obj = bpy.data.objects.new('_part', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.clear()
    mesh.materials.append(mat)
    for p in mesh.polygons:
        p.material_index = 0
    mesh.uv_layers.new(name='UVMap')
    return lp.place(obj, center, rotation)


def handle(points, radius):
    h = lp.sweep(points, lp.ngon(radius, 8), up=(0.0, 0.0, 1.0))
    return part(h, IRON)


def finish_lid(name, parts, hinge, open_angle):
    """Merges a lid and puts its origin on the hinge (y, z): the geometry stays where it was built."""
    lid = lp.join(name, parts)
    lp.place(lid, (0.0, -hinge[0], -hinge[1]))
    lid.location = (0.0, hinge[0], hinge[1])
    lid['OpenAngle'] = open_angle
    lm.smooth(lid, 35.0)
    lt.bake_vertex_ao(lid, distance=0.3, ground=False)
    lid['Fallback'] = 100
    return lid


# --- Uncommon: the supply crate ---

def supply_crate():
    rnd = random.Random(23)
    sx, sy, sz, base, wall = 1.15, 0.52, 0.34, 0.06, 0.02
    H = base + sz
    parts, lid = [], []
    for y in (-0.17, 0.17):
        parts.append(block((sx - 0.08, 0.07, base), (0.0, y, base * 0.5), STEEL, 1, bevel=0.006))
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, base + sz * 0.5), OLIVE, rnd.randint(0, 99)))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, base + sz * 0.5), OLIVE, rnd.randint(0, 99)))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.02), (0.0, 0.0, base + 0.01), OLIVE))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            parts.append(block((0.045, 0.045, sz), (x * (sx * 0.5 - 0.01), y * (sy * 0.5 - 0.01), base + sz * 0.5), STEEL,
                               rnd.randint(0, 99), bevel=0.004))
    for z in (base + 0.02, H - 0.02):
        parts += wrap(sx * 0.5, sy * 0.5, z, 0.04, 0.012, STEEL, gap_x=0.06, gap_y=0.06, bevel=0.003, seed=5)
    # The cream band and its stencils.
    for s in (-1.0, 1.0):
        parts.append(block((sx - 0.07, 0.005, 0.085), (0.0, s * (sy * 0.5 + 0.0025), base + sz * 0.5), CREAM, 2))
        parts.append(block((0.005, sy - 0.07, 0.085), (s * (sx * 0.5 + 0.0025), 0.0, base + sz * 0.5), CREAM, 3))
    parts.append(text('SUPPLY  •  07', 0.052, (0.0, -(sy * 0.5 + 0.0058), base + sz * 0.5), (90.0, 0.0, 0.0), STENCIL))
    parts.append(text('L-07', 0.05, (-(sx * 0.5 + 0.0058), 0.0, base + sz * 0.5), (90.0, 0.0, -90.0), STENCIL))
    for s in (-1.0, 1.0):
        x = s * (sx * 0.5 + 0.015)
        parts.append(handle([(x, -0.1, H - 0.08), (x + s * 0.045, -0.085, H - 0.08), (x + s * 0.045, 0.085, H - 0.08),
                             (x, 0.1, H - 0.08)], 0.011))
    # Latches: a plate and lever on the body (a green light in each plate), a catch on the lid.
    fy = -(sy * 0.5 + 0.012)
    for x in (-0.38, 0.38):
        parts.append(block((0.08, 0.012, 0.07), (x, fy - 0.006, H - 0.05), STEEL, 8, bevel=0.003))
        parts.append(block((0.022, 0.006, 0.014), (x + 0.025, fy - 0.014, H - 0.07), GREEN, bevel=0.002))
        parts.append(block((0.045, 0.01, 0.09), (x - 0.01, fy - 0.02, H - 0.02), IRON, 9, bevel=0.003))
        lid.append(block((0.06, 0.012, 0.03), (x, -((sy + 0.02) * 0.5 + 0.02), H + 0.02), STEEL, 9, bevel=0.003))
    parts.append(block((sx - 0.05, sy - 0.05, 0.16), (0.0, 0.0, base + 0.1), FOAM))
    body = lp.join('SupplyCrate', parts)
    lp.hull_box(body, (sx + 0.04, sy + 0.04, H), (0.0, 0.0, H * 0.5))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.18))
    hinge = ((sy + 0.02) * 0.5 + 0.012, H)
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lp.finish(body, ao=0.3)

    lid.append(block((sx + 0.02, sy + 0.02, 0.045), (0.0, 0.0, H + 0.0225), OLIVE, 9))
    lid += wrap((sx + 0.02) * 0.5, (sy + 0.02) * 0.5, H + 0.03, 0.06, 0.012, STEEL, bevel=0.003, seed=6)
    for x in (-0.3, 0.3):
        lid.append(block((0.07, sy + 0.02, 0.02), (x, 0.0, H + 0.055), STEEL, 7, bevel=0.004))
    lid.append(text('▲  THIS SIDE UP  ▲', 0.035, (0.0, -0.09, H + 0.0455), (0.0, 0.0, 0.0), STENCIL))
    cover = finish_lid('SupplyCrate_Lid', lid, hinge, OPEN_ANGLE['SupplyCrate'])
    lp.hull_box(cover, (sx + 0.05, sy + 0.05, 0.065), (0.0, -hinge[0], 0.0325))
    return [body, cover]


# --- Rare: the strongbox ---

def strongbox():
    rnd = random.Random(37)
    sx, sy, sz, base, wall, lid_t = 0.82, 0.6, 0.4, 0.045, 0.045, 0.15
    H = base + sz
    parts, lid = [], []
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            parts.append(block((0.09, 0.09, base), (x * (sx * 0.5 - 0.03), y * (sy * 0.5 - 0.03), base * 0.5), IRON, bevel=0.01))
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, base + sz * 0.5), SAFE, rnd.randint(0, 99), bevel=0.004))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, base + sz * 0.5), SAFE, rnd.randint(0, 99), bevel=0.004))
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
        parts += wrap(sx * 0.5, sy * 0.5, z, 0.05, 0.014, STEEL, gap_x=0.1, gap_y=0.1, bevel=0.003, seed=4)
        for s in (-1.0, 1.0):
            for k in range(6):
                parts.append(rivet((-0.25 + k * 0.1, s * (sy * 0.5 + 0.014), z), (0.0, s, 0.0), r=0.009))
    # The dial the wheel turns on, ringed in blue; the seam under the lid.
    wz, wy = base + sz * 0.5, -(sy * 0.5)
    parts.append(disc(0.11, 0.012, (0.0, wy, wz), (0.0, -1.0, 0.0), STEEL, segments=24))
    parts.append(ring(0.095, 0.006, (0.0, wy - 0.012, wz), (0.0, -1.0, 0.0), BLUE, segments=28))
    parts += wrap(sx * 0.5, sy * 0.5, H, 0.008, 0.003, BLUE, gap_x=0.15, gap_y=0.15)
    for x in (-0.24, 0.24):
        parts.append(disc(0.032, 0.13, (x - 0.065, sy * 0.5 + 0.03, H), (1.0, 0.0, 0.0), IRON, segments=12))
        parts.append(block((0.11, 0.012, 0.09), (x, sy * 0.5 + 0.012, H - 0.05), IRON, bevel=0.003))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.06), (0.0, 0.0, base + 0.06), FELT))
    body = lp.join('Strongbox', parts)
    lp.hull_box(body, (sx + 0.04, sy + 0.04, H), (0.0, 0.0, H * 0.5))
    hinge = (sy * 0.5 + 0.03, H)
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.09))
    lm.socket(body, 'Wheel', (0.0, wy - 0.012, wz))
    lp.finish(body, ao=0.3)

    lid.append(block((sx, sy, lid_t), (0.0, 0.0, H + lid_t * 0.5), SAFE, 12, bevel=0.008))
    frame(H, lid_t, lid)
    lid += wrap(sx * 0.5, sy * 0.5, H + lid_t - 0.025, 0.05, 0.014, STEEL, gap_x=0.1, gap_y=0.1, bevel=0.003, seed=4)
    for x in (-0.18, 0.18):
        lid.append(block((0.07, sy - 0.1, 0.02), (x, 0.0, H + lid_t + 0.01), STEEL, 13, bevel=0.005))
        for y in (-0.2, 0.0, 0.2):
            lid.append(rivet((x, y, H + lid_t + 0.02), (0.0, 0.0, 1.0)))
    for x in (-0.24, 0.24):
        lid.append(block((0.11, 0.012, 0.09), (x, sy * 0.5 + 0.012, H + 0.05), IRON, bevel=0.003))
    cover = finish_lid('Strongbox_Lid', lid, hinge, OPEN_ANGLE['Strongbox'])
    lp.hull_box(cover, (sx + 0.04, sy + 0.04, lid_t), (0.0, -hinge[0], lid_t * 0.5))

    # The wheel, round its own origin, facing -Y: a hub, three spokes, brass knobs.
    wheel_parts = [disc(0.045, 0.04, (0.0, 0.0, 0.0), (0.0, -1.0, 0.0), IRON, segments=16)]
    for k in range(3):
        a = math.radians(90.0 + k * 120.0)
        spoke = lp.block((0.014, 0.014, 0.12), (0.0, 0.0, 0.06))
        part(spoke, IRON)
        lp.place(spoke, (0.0, -0.033, 0.0), (0.0, 90.0 - math.degrees(a), 0.0))
        wheel_parts.append(spoke)
        wheel_parts.append(disc(0.018, 0.03, (math.cos(a) * 0.12, -0.048, math.sin(a) * 0.12), (0.0, -1.0, 0.0), BRASS, segments=10))
    wheel = lp.join('Strongbox_Wheel', wheel_parts)
    lm.smooth(wheel, 35.0)
    lt.bake_vertex_ao(wheel, distance=0.05, ground=False)
    wheel['Nanite'] = 0
    wheel['Collision'] = 'None'
    wheel.location = (0.0, wy - 0.012, wz)
    return [body, cover, wheel]


# --- Legendary: the reliquary ---

def reliquary():
    rnd = random.Random(53)
    sx, sy, sz, wall, base = 1.02, 0.62, 0.42, 0.07, 0.2
    H = base + sz
    ox, oy = sx * 0.5, sy * 0.5
    parts, lid = [], []
    parts.append(block((1.32, 0.88, 0.13), (0.0, 0.0, 0.065), GRANITE, 1, bevel=0.02))
    parts.append(block((1.2, 0.76, 0.07), (0.0, 0.0, 0.165), GRANITE, 2, bevel=0.015))
    parts += wrap(0.6, 0.38, 0.19, 0.02, 0.008, GOLD, bevel=0.002)
    for s in (-1.0, 1.0):
        parts.append(block((sx, wall, sz), (0.0, s * (sy - wall) * 0.5, base + sz * 0.5), GRANITE, rnd.randint(0, 99), bevel=0.008))
        parts.append(block((wall, sy - 2 * wall, sz), (s * (sx - wall) * 0.5, 0.0, base + sz * 0.5), GRANITE, rnd.randint(0, 99), bevel=0.008))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.04), (0.0, 0.0, base + 0.02), GRANITE, 7))
    parts.append(block((sx - 2 * wall, sy - 2 * wall, 0.03), (0.0, 0.0, base + 0.055), GOLD, 8, bevel=0.003))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            cx, cy = x * (ox + 0.005), y * (oy + 0.005)
            parts.append(disc(0.038, sz - 0.08, (cx, cy, base + 0.04), (0.0, 0.0, 1.0), GOLD, segments=14))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, base + 0.0225), GOLD, bevel=0.006))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, H - 0.0225), GOLD, bevel=0.006))
    for z in (base + 0.035, H - 0.035):
        parts += wrap(ox, oy, z, 0.025, 0.008, GOLD, gap_x=0.1, gap_y=0.1, bevel=0.002)
    # The rune: a glowing diamond in a gold frame, lines running off it to the corners and round the ends.
    fy, cz = -(oy + 0.004), base + sz * 0.5
    parts.append(block((0.2, 0.012, 0.2), (0.0, fy, cz), GOLD, rotation=(0.0, 45.0, 0.0), bevel=0.004))
    parts.append(block((0.15, 0.006, 0.15), (0.0, fy - 0.009, cz), RUNE_STONE, rotation=(0.0, 45.0, 0.0)))
    r = 0.07
    for xs in (-1.0, 1.0):
        for zs in (-1.0, 1.0):
            parts.append(block((r * 1.414 + 0.012, 0.004, 0.012), (xs * r * 0.5, fy - 0.013, cz + zs * r * 0.5), ORANGE,
                               rotation=(0.0, 45.0 * xs * zs, 0.0)))
    parts.append(gem(0.018, 0.03, (0.0, fy - 0.014, cz), (0.0, -1.0, 0.0), ORANGE, sides=4))
    for s in (-1.0, 1.0):
        parts.append(block((0.22, 0.004, 0.01), (s * 0.24, fy - 0.001, cz), ORANGE))
        parts.append(block((0.01, 0.004, 0.12), (s * 0.35, fy - 0.001, cz), ORANGE))
        parts.append(block((0.004, 0.16, 0.01), (s * (ox + 0.001), 0.0, cz), ORANGE))
    for x in (-0.3, 0.3):
        parts.append(disc(0.03, 0.14, (x - 0.07, oy + 0.045, H), (1.0, 0.0, 0.0), GOLD, segments=12))
    body = lp.join('Reliquary', parts)
    lp.hull_box(body, (1.32, 0.88, 0.2), (0.0, 0.0, 0.1))
    lp.hull_box(body, (sx + 0.04, sy + 0.04, sz), (0.0, 0.0, base + sz * 0.5))
    hinge = (oy + 0.045, H)
    lm.socket(body, 'Lid', (0.0, hinge[0], hinge[1]))
    lm.socket(body, 'Loot', (0.0, 0.0, base + 0.07))
    lm.socket(body, 'Crystal', (0.0, 0.0, H + 0.55))
    lp.finish(body, ao=0.3)

    lid.append(block((sx + 0.07, sy + 0.07, 0.07), (0.0, 0.0, H + 0.035), GRANITE, 11, bevel=0.012))
    lid += wrap(ox + 0.035, oy + 0.035, H + 0.035, 0.03, 0.008, GOLD, bevel=0.002)
    lid.append(block((sx - 0.14, sy - 0.16, 0.09), (0.0, 0.0, H + 0.115), GRANITE, 12, bevel=0.025))
    lid.append(block((sx - 0.4, 0.05, 0.035), (0.0, 0.0, H + 0.1775), GOLD, bevel=0.008))
    lid.append(gem(0.04, 0.03, (0.0, 0.0, H + 0.2), (0.0, 0.0, 1.0), ORANGE, sides=4))
    cover = finish_lid('Reliquary_Lid', lid, hinge, OPEN_ANGLE['Reliquary'])
    lp.hull_box(cover, (sx + 0.08, sy + 0.08, 0.16), (0.0, -hinge[0], 0.08))

    crystal = gem(0.075, 0.19, (0.0, 0.0, 0.0), (0.0, 0.0, 1.0), ORANGE, sides=6)
    crystal.name = crystal.data.name = 'Reliquary_Crystal'
    lm.smooth(crystal, 20.0)
    lt.bake_vertex_ao(crystal, samples=4, ground=False)
    crystal['Nanite'] = 0
    crystal['Collision'] = 'None'
    crystal.location = (0.0, 0.0, H + 0.55)
    return [body, cover, crystal]


def open_lids(models, opened):
    for obj in models:
        base = obj.name.replace('_Lid', '')
        if obj.name.endswith('_Lid'):
            obj.rotation_euler.x = -math.radians(OPEN_ANGLE[base]) if opened else 0.0


def preview(groups):
    for group in groups:
        name = group[0].name
        lt.preview(group, lt.preview_path('Backlog', name), view=(-0.42, -1.6, 0.9))
        open_lids(group, True)
        lt.preview(group, lt.preview_path('Backlog', name + '_Open'), view=(-0.42, -1.6, 0.9))
        open_lids(group, False)


groups = [supply_crate(), strongbox(), reliquary()]
for g, x in zip(groups, (-1.6, 0.0, 1.6)):
    for obj in g:
        obj.location.x = x   # side by side in the scene (positions don't matter to the export)
if lt.want_preview():
    preview(groups)
