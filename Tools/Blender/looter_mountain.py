"""Helpers for the mountain's models on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md, section 6): the ore lift
(Art/Models/Buildings/OreLift.py), Calder's Post (CalderPost.py), the Providence's Diggings (Diggings.py), the ore
tramway (Art/Models/Props/Tramway.py) and the mine's kit (Art/Models/Mine/Mine.py, with looter_mine.py).

It sits on looter_homes (Home: the town's finishes, glazing, a pivot moved at finish, the report line), looter_town and
looter_buildings, and adds what a mountain of timber, iron and rock needs:

  - Model: a Home that also knows the mountain's shared materials by key: 'charcoal' (Charcoal), 'granite'
    (RockGranite), 'rockden' (RockCliffDen) and 'denfloor' (DenFloor) as Den Rock makes them, 'rope' (RopeHemp, as
    Lootables.py makes it), 'ballast' (Ballast, as looter_rail makes it); with the town's ('trim', 'planks', 'iron'
    IronBlack, 'brass', 'cream', 'black', 'oxide', 'woodblack'...) and the homes' ('canvas', 'dirt', 'windowglow').
    No MetalRust or tin anywhere: they read as loot colours on Ransom's Rest, so iron is IronBlack;
  - timber: hewn beams and posts on the trim sheet's beam strip (C), round logs (B), planks on WoodPlanks, wedges;
  - iron: plates, straps, bolts, rods, a spoked sheave wheel with a grooved rim, a crank;
  - small things: lanterns with their own socket names, a bronze bell with its pull rope, a speaking tube's mouth,
    ledgers and loose papers, and chalk scrawled in strokes (CHALK_FONT: a hand's capitals, not a typeface);
  - previews in Ransom's Rest's golden late afternoon (looter_homes' stage, sky and framing), cutaways for the
    interiors (a copy cut above a height) and lamp-lit shots, and the reports the look sheet is made from
    (Tools/Blender/mountain_sheet.py composes Saved/ArtPreviews/RansomsRest/LookSheets/37_Mountain.jpg).

Everything random comes from a model's seeded random.Random, and nothing iterates a set of mesh elements, so a model
comes out the same on every run.
"""
import json
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_homes as homes
import looter_model as lm
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place

UP = Vector((0.0, 0.0, 1.0))
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Mountain')
SHEET_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets')
REPORTS = {}

# The shared names (one name, one look, in every model that uses it): made exactly as their first families make them.
EXTRA = {
    'charcoal': lambda: lt.material('BarkOak', name='Charcoal', tint=0x4a4440),
    'granite': lambda: lt.material('RockGranite', MossAmount=0.6),
    # Den Rock's den (Art/Models/Rocks/Outcrops.py den_material, den_floor_material): the mine's rock and floor.
    'rockden': lambda: lt.material('RockCliff', name='RockCliffDen', tint=0xdcd6ce, MossAmount=0.0, DiffuseAO=0.85),
    'denfloor': lambda: lt.material('GroundDirt', name='DenFloor', tint=0xc9bfb2, MossAmount=0.0, DiffuseAO=0.85),
    'rope': lambda: lt.material('Hay', name='RopeHemp', tint=0x9a7a50),
    'ballast': lambda: lt.material('GroundDirt', name='Ballast', tint=0x9c968e),
}
IRON = Tile('MetalWorn')
# The same keys as finishes for looter_town's parts (town.box(look='granite')...): a world-scale box mapping of the set.
# (looter_town's own table, extended for this process only.)
for _key, _set in (('charcoal', 'BarkOak'), ('granite', 'RockGranite'), ('rockden', 'RockCliff'),
                   ('denfloor', 'GroundDirt'), ('rope', 'Hay'), ('ballast', 'GroundDirt'), ('canvas', 'Polymer'),
                   ('dirt', 'GroundDirt')):
    town.FINISHES.setdefault(_key, _set)


def args():
    """The script's own arguments (after '--'), split at commas (artrun.ps1 hands a comma list over as one)."""
    import sys
    raw = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    return [part for arg in raw for part in arg.split(',') if part]


class Model(homes.Home):
    """A Home (looter_homes) that also knows the mountain's materials (EXTRA)."""

    def slot(self, key):
        if key not in self.slots and key in EXTRA:
            self.slots[key] = (len(self.slots), EXTRA[key]())
        return super().slot(key)


# --- Timber ---

def beam(m, p0, p1, w, d, face=(0.0, -1.0, 0.0), look='C', space=None, lift=0.0, cuts=None, drop=()):
    """A hewn timber from p0 to p1 (its middle line): w across (perpendicular to face), d toward face. Strip C's beam
    faces, each side its own, so the grain runs along it."""
    length = (Vector(p1) - Vector(p0)).length
    if cuts is None:
        cuts = int(length / 2.5)
    matrix = kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, -lift, 0.0))
    spec = Trim(look, lane='each', width=max(w, d)) if look in ('A', 'C') else town.finish(look, width=max(w, d))[0]
    key = 'trim' if look in ('A', 'C') else town.finish(look, width=max(w, d))[1]
    m.emit(kit._box((length, d, w), 0.0, cuts, drop), spec, key, matrix, space)


def post(m, x, y, z0, z1, size=0.25, look='C', space=None, turn=0.0):
    """An upright hewn post from z0 to z1, square (size), turned turn degrees about Z."""
    a = math.radians(turn)
    face = (math.sin(a), -math.cos(a), 0.0)
    beam(m, (x, y, z0), (x, y, z1), size, size, face=face, look=look, space=space)


def plank(m, p0, p1, width, thick=0.05, face=(0.0, 0.0, 1.0), look='planks', space=None, lift=0.0, drop=()):
    """A plank (one plank of WoodPlanks, or a finish) from p0 to p1, its broad side toward face."""
    town.board(m, p0, p1, width, thick, look=look, face=face, space=space, lift=lift, drop=drop)


def deck(m, x0, x1, y0, y1, top, thick=0.05, along='x', space=None, look='planks'):
    """A plank floor as one slab (WoodPlanks at world scale shows its planks, along X or Y), its top at `top`, cut
    across every metre or so for the baked shading: a whole floor for the triangles of a few planks."""
    lx, ly = x1 - x0, y1 - y0
    if along == 'y':
        m.emit(kit._box((ly, lx, thick), 0.0, max(0, int(ly / 1.1)), ('-z',)), homes.Boards(rotate=False), look,
               place(((x0 + x1) * 0.5, (y0 + y1) * 0.5, top - thick * 0.5), (0.0, 0.0, 90.0)), space)
    else:
        m.emit(kit._box((lx, ly, thick), 0.0, max(0, int(lx / 1.1)), ('-z',)), homes.Boards(rotate=False), look,
               place(((x0 + x1) * 0.5, (y0 + y1) * 0.5, top - thick * 0.5)), space)


def log(m, p0, p1, r=0.16, out=(0.0, -1.0, 0.0), r1=None, sides=8, space=None, caps=(True, True)):
    """A round timber (strip B), its sawn ends on WoodEndGrain."""
    kit.log(m, p0, p1, r=r, out=out, r1=r1, sides=sides, caps=caps, space=space)


def wedge(m, at, size=(0.18, 0.12, 0.06), turn=0.0, look='C', space=None):
    """A blocking wedge (a short block) driven between a timber and the rock."""
    town.box(m, size, at=at, rot=(0.0, 0.0, turn), look=look, space=space)


# --- Iron ---

def iron_box(m, size, at, rot=(0.0, 0.0, 0.0), space=None, look='iron', bevel=0.0):
    town.box(m, size, at=at, rot=rot, look=look, space=space, bevel=bevel)


def rod(m, p0, p1, r=0.02, sides=6, look='iron', space=None, caps=(True, True)):
    town.cylinder(m, p0, p1, r, sides=sides, look=look, space=space, caps=caps,
                  face=(0.0, 0.0, 1.0) if abs((Vector(p1) - Vector(p0)).normalized().z) < 0.9 else (0.0, -1.0, 0.0))


def strap(m, p0, p1, width=0.06, thick=0.012, face=(0.0, -1.0, 0.0), space=None, lift=0.0):
    """An iron strap (the trim sheet's riveted strip, H3) lying on a face."""
    town.board(m, p0, p1, width, thick, look='H3', face=face, space=space, lift=lift + thick * 0.5)


def bolt(m, at, normal=(0.0, -1.0, 0.0), size=0.045, space=None):
    """A square bolt head (or a nut) proud of a face."""
    n = Vector(normal).normalized()
    c = Vector(at) + n * 0.008
    m.emit(kit._box((size, 0.016, size)), Tile('MetalWorn'), 'iron',
           kit.toward(c, c + (n.cross(UP) if abs(n.z) < 0.9 else Vector((1.0, 0.0, 0.0))), -n), space)


def ring(m, profile, center, axis, sides=16, look='iron', space=None):
    """A turned part round an axis through center (axis: a direction): profile [(radius, h), ...] along the axis, a
    closed loop for a ring (repeat the first point last) or open toward the axis (radius 0)."""
    a = Vector(axis).normalized()
    rot = Vector((0.0, 0.0, 1.0)).rotation_difference(a).to_matrix().to_4x4()
    if space is not None:
        frame = Matrix.Translation(Vector(center)) @ rot
        town.lathe(m, profile, at=(0.0, 0.0, 0.0), sides=sides, look=look, space=Space(space.matrix @ frame))
    else:
        town.lathe(m, profile, at=center, sides=sides, look=look, axis=rot)


def sheave(m, center, axis, radius, width=0.14, spokes=6, look='iron', hub=None, space=None, groove=True):
    """A spoked iron sheave: a rim grooved for the cable (or a flat flywheel with groove=False), a hub on the axle and
    spokes between them, in the plane across axis."""
    a = Vector(axis).normalized()
    c = Vector(center)
    rim = 0.07 * radius / 0.6 + 0.03
    w = width * 0.5
    if groove:
        profile = [(radius - rim, -w), (radius, -w), (radius - 0.035, -w * 0.25), (radius - 0.035, w * 0.25),
                   (radius, w), (radius - rim, w), (radius - rim, -w)]
    else:
        profile = [(radius - rim, -w), (radius, -w), (radius, w), (radius - rim, w), (radius - rim, -w)]
    ring(m, profile, c, a, sides=24 if radius > 0.5 else 16, look=look, space=space)
    hub_r = hub if hub is not None else max(0.06, radius * 0.16)
    town.cylinder(m, c - a * (w + 0.04), c + a * (w + 0.04), hub_r, sides=10, look=look, space=space,
                  face=(0.0, 0.0, 1.0) if abs(a.z) < 0.9 else (0.0, -1.0, 0.0))
    # Spokes in the wheel's plane: flat bars from the hub to the rim.
    e1 = a.cross(UP) if abs(a.z) < 0.9 else a.cross(Vector((1.0, 0.0, 0.0)))
    e1.normalize()
    e2 = a.cross(e1).normalized()
    for k in range(spokes):
        t = 2.0 * math.pi * (k + 0.5) / spokes
        d = e1 * math.cos(t) + e2 * math.sin(t)
        p0 = c + d * (hub_r * 0.8)
        p1 = c + d * (radius - rim * 0.6)
        m.emit(kit._box(((p1 - p0).length, width * 0.35, radius * 0.07 + 0.02)), Tile('MetalWorn'), look if look in
               ('iron', 'brass') else 'iron', kit.toward(p0, p1, a) @ Matrix.Translation(((p1 - p0).length * 0.5, 0.0, 0.0)),
               space)


def lantern(m, at, name='Light', size=0.2, space=None):
    """A square iron lantern with lit glass (looter_rail's, on the trim sheet's iron) whose socket is SOCKET_<name>, so
    a model may hold several (each socket name once)."""
    x, y, z = at
    s = size
    iron = Trim('H3', fit=True)
    m.box((s * 0.7, s * 0.7, s * 1.15), at=(x, y, z), uv=Trim('H4', fit=True), mat='glow', space=space)
    m.box((s * 1.1, s * 1.1, s * 0.2), at=(x, y, z + s * 0.67), uv=iron, space=space, bevel=0.008)
    roof = kit._new_bmesh()
    corners = [roof.verts.new((sx * s * 0.55, sy * s * 0.55, 0.0)) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    tip = roof.verts.new((0.0, 0.0, s * 0.42))
    for p, q in zip(corners, corners[1:] + corners[:1]):
        roof.faces.new((p, q, tip))
    m.emit(roof, iron, 'trim', place((x, y, z + s * 0.77)), space)
    m.box((s * 0.16, s * 0.05, s * 0.3), at=(x, y, z + s * 1.27), uv=iron, space=space)
    for dx in (-1.0, 1.0):
        for dy in (-1.0, 1.0):
            m.box((s * 0.12, s * 0.12, s * 1.25), at=(x + dx * s * 0.38, y + dy * s * 0.38, z), uv=iron, space=space)
    m.box((s * 1.0, s * 1.0, s * 0.18), at=(x, y, z - s * 0.66), uv=iron, space=space, bevel=0.006)
    point = space.world(Vector(at)) if space is not None else Vector(at)
    m.socket(name, tuple(point))


def bell(m, at, radius=0.13, space=None, rope=1.2, look='brass'):
    """A bronze hand bell hung mouth down at `at` (its crown), a clapper inside, and a pull rope down from its lever."""
    x, y, z = at
    h = radius * 1.35
    town.lathe(m, [(0.0, 0.0), (radius * 0.45, -0.01), (radius * 0.55, -h * 0.35), (radius * 0.68, -h * 0.75),
                   (radius, -h), (radius * 0.9, -h - 0.012), (radius * 0.55, -h * 0.8), (0.0, -h * 0.7)],
               at=(x, y, z), sides=12, look=look, space=space)
    town.cylinder(m, (x, y, z - h * 0.6), (x, y, z - h - 0.05), 0.012, sides=6, look='iron', space=space,
                  face=(0.0, -1.0, 0.0))
    town.box(m, (0.03, 0.03, 0.06), at=(x, y, z + 0.03), look='iron', space=space)
    if rope:
        town.box(m, (0.28, 0.025, 0.025), at=(x + 0.14, y, z + 0.07), look='iron', space=space)
        town.cylinder(m, (x + 0.27, y, z + 0.06), (x + 0.27, y, z + 0.06 - rope), 0.013, sides=5, look='rope',
                      space=space, face=(0.0, -1.0, 0.0))
        town.box(m, (0.05, 0.05, 0.12), at=(x + 0.27, y, z + 0.0 - rope), look='rope', space=space)


def tube_mouth(m, at, facing=(0.0, -1.0, 0.0), rise=1.4, space=None):
    """A speaking tube: an iron pipe up out of the ground (or down a wall) to rise, turning out toward facing into a
    brass mouthpiece with a hinged whistle cap."""
    x, y, z = at
    f = Vector(facing).normalized()
    top = Vector((x, y, z + rise))
    town.cylinder(m, (x, y, z - 0.3), tuple(top), 0.035, sides=8, look='iron', space=space, caps=(False, False),
                  face=tuple(f))
    bend = top + f * 0.12 + Vector((0.0, 0.0, 0.04))
    town.cylinder(m, tuple(top - Vector((0.0, 0.0, 0.02))), tuple(bend), 0.035, sides=8, look='iron', space=space,
                  caps=(False, False), face=(0.0, 0.0, 1.0))
    mouth = bend + f * 0.12
    town.cylinder(m, tuple(bend), tuple(mouth), 0.04, 0.075, sides=10, look='brass', space=space, caps=(False, True),
                  face=(0.0, 0.0, 1.0))
    town.box(m, (0.09, 0.012, 0.09), at=tuple(mouth + f * 0.01 + Vector((0.0, 0.0, 0.07))),
             rot=(0.0, 0.0, math.degrees(math.atan2(f.x, -f.y))), look='brass', space=space)
    return mouth


def book(m, at, size=(0.24, 0.32, 0.05), turn=0.0, cover='black', space=None, open_=False):
    """A ledger lying on a surface (at: the middle of its underside): boards in cover, the page block cream."""
    x, y, z = at
    w, d, t = size
    rot = (0.0, 0.0, turn)
    a = math.radians(turn)
    if open_:
        for side in (-1.0, 1.0):
            off = Vector((math.cos(a), math.sin(a), 0.0)) * side * w * 0.5
            town.box(m, (w, d, 0.008), at=(x + off.x, y + off.y, z + 0.004), rot=rot, look=cover, space=space)
            town.box(m, (w - 0.02, d - 0.02, 0.016), at=(x + off.x * 0.98, y + off.y * 0.98, z + 0.016), rot=rot,
                     look='band_G', space=space)
        return
    town.box(m, (w, d, 0.008), at=(x, y, z + 0.004), rot=rot, look=cover, space=space)
    town.box(m, (w - 0.012, d - 0.016, t - 0.016), at=(x + 0.004 * math.cos(a), y + 0.004 * math.sin(a), z + t * 0.5),
             rot=rot, look='band_G', space=space)
    town.box(m, (w, d, 0.008), at=(x, y, z + t - 0.004), rot=rot, look=cover, space=space)
    back = Vector((-math.cos(a), -math.sin(a), 0.0)) * (w * 0.5 - 0.004)
    town.box(m, (0.012, d, t), at=(x + back.x, y + back.y, z + t * 0.5), rot=rot, look=cover, space=space)


def paper(m, corner, along, up, w, h, u=None, space=None):
    """A sheet of paper (the trim sheet's cream plaster strip, old paper) from corner along `along` and `up`."""
    homes.quad(m, [tuple(Vector(corner)), tuple(Vector(corner) + Vector(along) * w),
                   tuple(Vector(corner) + Vector(along) * w + Vector(up) * h), tuple(Vector(corner) + Vector(up) * h)],
               Trim('G', fit=True, u=u if u is not None else m.rng.uniform(4.2, 5.8)), space=space,
               facing=tuple(Vector(along).cross(Vector(up))))


# --- Chalk (scrawled capitals: strokes in a 1 x 1 box, x right, y up; each a polyline) ---

CHALK_FONT = {
    'A': [[(0.0, 0.0), (0.45, 1.0), (0.9, 0.0)], [(0.2, 0.42), (0.7, 0.42)]],
    'B': [[(0.0, 0.0), (0.0, 1.0), (0.55, 0.95), (0.6, 0.6), (0.0, 0.52), (0.7, 0.4), (0.65, 0.05), (0.0, 0.0)]],
    'C': [[(0.8, 0.85), (0.45, 1.0), (0.05, 0.75), (0.05, 0.25), (0.45, 0.0), (0.8, 0.15)]],
    'D': [[(0.0, 0.0), (0.0, 1.0), (0.55, 0.9), (0.8, 0.5), (0.55, 0.08), (0.0, 0.0)]],
    'E': [[(0.75, 1.0), (0.0, 1.0), (0.0, 0.0), (0.75, 0.0)], [(0.0, 0.52), (0.55, 0.52)]],
    'F': [[(0.75, 1.0), (0.0, 1.0), (0.0, 0.0)], [(0.0, 0.52), (0.55, 0.52)]],
    'G': [[(0.8, 0.85), (0.45, 1.0), (0.05, 0.75), (0.05, 0.25), (0.45, 0.0), (0.8, 0.2), (0.8, 0.45), (0.45, 0.45)]],
    'H': [[(0.0, 0.0), (0.0, 1.0)], [(0.75, 0.0), (0.75, 1.0)], [(0.0, 0.5), (0.75, 0.5)]],
    'I': [[(0.2, 0.0), (0.2, 1.0)]],
    'J': [[(0.6, 1.0), (0.6, 0.2), (0.35, 0.0), (0.05, 0.15)]],
    'K': [[(0.0, 0.0), (0.0, 1.0)], [(0.7, 1.0), (0.0, 0.45), (0.75, 0.0)]],
    'L': [[(0.0, 1.0), (0.0, 0.0), (0.7, 0.0)]],
    'M': [[(0.0, 0.0), (0.05, 1.0), (0.45, 0.4), (0.85, 1.0), (0.9, 0.0)]],
    'N': [[(0.0, 0.0), (0.0, 1.0), (0.75, 0.0), (0.75, 1.0)]],
    'O': [[(0.4, 1.0), (0.05, 0.75), (0.05, 0.25), (0.4, 0.0), (0.75, 0.25), (0.75, 0.75), (0.4, 1.0)]],
    'P': [[(0.0, 0.0), (0.0, 1.0), (0.6, 0.92), (0.65, 0.6), (0.0, 0.5)]],
    'Q': [[(0.4, 1.0), (0.05, 0.75), (0.05, 0.25), (0.4, 0.0), (0.75, 0.25), (0.75, 0.75), (0.4, 1.0)],
          [(0.45, 0.3), (0.85, -0.05)]],
    'R': [[(0.0, 0.0), (0.0, 1.0), (0.6, 0.92), (0.65, 0.6), (0.0, 0.5), (0.7, 0.0)]],
    'S': [[(0.75, 0.85), (0.4, 1.0), (0.05, 0.8), (0.15, 0.55), (0.65, 0.42), (0.75, 0.15), (0.4, 0.0), (0.0, 0.15)]],
    'T': [[(0.0, 1.0), (0.8, 1.0)], [(0.4, 1.0), (0.4, 0.0)]],
    'U': [[(0.0, 1.0), (0.0, 0.25), (0.35, 0.0), (0.7, 0.25), (0.7, 1.0)]],
    'V': [[(0.0, 1.0), (0.4, 0.0), (0.8, 1.0)]],
    'W': [[(0.0, 1.0), (0.2, 0.0), (0.45, 0.6), (0.7, 0.0), (0.9, 1.0)]],
    'X': [[(0.0, 1.0), (0.75, 0.0)], [(0.75, 1.0), (0.0, 0.0)]],
    'Y': [[(0.0, 1.0), (0.4, 0.5), (0.8, 1.0)], [(0.4, 0.5), (0.4, 0.0)]],
    'Z': [[(0.0, 1.0), (0.75, 1.0), (0.0, 0.0), (0.75, 0.0)]],
    '0': [[(0.35, 1.0), (0.05, 0.75), (0.05, 0.25), (0.35, 0.0), (0.65, 0.25), (0.65, 0.75), (0.35, 1.0)]],
    '1': [[(0.1, 0.8), (0.35, 1.0), (0.35, 0.0)]],
    '2': [[(0.05, 0.8), (0.35, 1.0), (0.65, 0.8), (0.0, 0.0), (0.7, 0.0)]],
    '3': [[(0.05, 0.9), (0.6, 0.95), (0.25, 0.55), (0.65, 0.35), (0.5, 0.0), (0.0, 0.08)]],
    '4': [[(0.55, 0.0), (0.55, 1.0), (0.0, 0.3), (0.75, 0.3)]],
    '5': [[(0.7, 1.0), (0.1, 1.0), (0.05, 0.55), (0.55, 0.6), (0.7, 0.3), (0.45, 0.0), (0.0, 0.1)]],
    '6': [[(0.6, 0.95), (0.2, 0.8), (0.05, 0.3), (0.3, 0.0), (0.65, 0.2), (0.6, 0.48), (0.1, 0.42)]],
    '7': [[(0.0, 1.0), (0.7, 1.0), (0.25, 0.0)]],
    '8': [[(0.35, 0.55), (0.1, 0.8), (0.35, 1.0), (0.6, 0.8), (0.35, 0.55), (0.05, 0.25), (0.35, 0.0), (0.68, 0.25),
           (0.35, 0.55)]],
    '9': [[(0.6, 0.55), (0.1, 0.6), (0.1, 0.92), (0.5, 1.0), (0.65, 0.7), (0.55, 0.0)]],
    '.': [[(0.1, 0.0), (0.12, 0.06)]],
    '-': [[(0.05, 0.5), (0.5, 0.5)]],
}
CHALK_ADVANCE = {'I': 0.45, '.': 0.3, '1': 0.55, ' ': 0.55}


def chalk(m, text, origin, along, up, height, look='cream', width=None, space=None, seed=None, out=0.004):
    """Chalk scrawled on a face: text in CHALK_FONT's strokes, its baseline from origin along `along`, letters `height`
    tall toward `up`; each stroke a thin flat bar (two triangles) out of the face (along x up) by `out`, every point
    nudged a little, so it reads as a hand's. Lines break at '\\n'. Returns the width written."""
    import random
    rng = random.Random(seed if seed is not None else m.rng.randrange(1 << 30))
    along, up = Vector(along).normalized(), Vector(up).normalized()
    normal = along.cross(up)
    width = width if width is not None else height * 0.11
    tb = kit._new_bmesh()
    x0 = 0.0
    line = 0
    widest = 0.0
    for ch in text:
        if ch == '\n':
            widest = max(widest, x0)
            x0, line = 0.0, line + 1
            continue
        strokes = CHALK_FONT.get(ch.upper(), [])
        slant = rng.uniform(-0.06, 0.1)
        lift = rng.uniform(-0.05, 0.05) - line * 1.5
        for stroke in strokes:
            pts = []
            for px, py in stroke:
                pts.append(Vector(origin) + along * ((x0 + px * 0.8 + slant * py) * height + rng.uniform(-0.01, 0.01) * height)
                           + up * ((py + lift) * height + rng.uniform(-0.015, 0.015) * height) + normal * out)
            for p, q in zip(pts, pts[1:]):
                d = q - p
                if d.length < 1e-4:
                    continue
                side = normal.cross(d).normalized() * (width * rng.uniform(0.8, 1.15) * 0.5)
                ext = d.normalized() * width * 0.3
                verts = [tb.verts.new(v) for v in (p - ext - side, q + ext - side, q + ext + side, p - ext + side)]
                face = tb.faces.new(verts)
                face.normal_update()
                if face.normal.dot(normal) < 0.0:
                    face.normal_flip()
        x0 += CHALK_ADVANCE.get(ch.upper(), 0.95)
    widest = max(widest, x0)
    spec, key = town.finish(look)
    m.emit(tb, Tile('PaintWorn') if look in ('cream', 'black', 'oxide') else spec, key, None, space)
    return widest * height


def tally(m, origin, along, up, count, height=0.22, look='cream', space=None, seed=None):
    """Tally marks in fives (four strokes and one across) scrawled on a face."""
    import random
    rng = random.Random(seed if seed is not None else m.rng.randrange(1 << 30))
    along, up = Vector(along).normalized(), Vector(up).normalized()
    normal = along.cross(up)
    width = height * 0.09
    tb = kit._new_bmesh()

    def bar(p, q):
        d = q - p
        side = normal.cross(d).normalized() * width * 0.5
        verts = [tb.verts.new(v) for v in (p - side, q - side, q + side, p + side)]
        face = tb.faces.new(verts)
        face.normal_update()
        if face.normal.dot(normal) < 0.0:
            face.normal_flip()
    x = 0.0
    o = Vector(origin) + normal * 0.004
    for k in range(count):
        if k % 5 == 4:
            g = x - 4 * 0.12 * height / 0.22 + 0.02
            bar(o + along * (g - 0.04) + up * height * 0.25, o + along * (x - 0.06) + up * height * 0.75)
            x += 0.16 * height / 0.22
            continue
        lean = rng.uniform(-0.03, 0.03)
        bar(o + along * (x + lean) + up * rng.uniform(-0.01, 0.02), o + along * (x - lean) + up * (height + rng.uniform(-0.02, 0.02)))
        x += 0.12 * height / 0.22
    m.emit(tb, Tile('PaintWorn'), look, None, space)


def transform_model(m, matrix):
    """Moves everything built so far in a Model (mesh, hulls, sockets) by matrix (a rotation about Z and a move): a part
    built in one frame and pivoted in another."""
    matrix = Matrix(matrix)
    bmesh.ops.transform(m.bm, matrix=matrix, verts=m.bm.verts[:])
    m.hulls = [[matrix @ Vector(p) for p in points] for points in m.hulls]
    turn = math.degrees(matrix.to_euler().z)
    m.sockets = [(name, tuple(matrix @ Vector(at)), (rot[0], rot[1], rot[2] + turn)) for name, at, rot in m.sockets]


# --- Collision and reports ---

def hull_between(m, p0, p1, w, h, below=0.0):
    """A box hull along the segment p0-p1 (level or sloped), w wide and h tall above it (a beam, a rail)."""
    p0, p1 = Vector(p0), Vector(p1)
    d = p1 - p0
    side = d.cross(UP).normalized() * (w * 0.5) if Vector((d.x, d.y, 0.0)).length > 1e-6 else Vector((w * 0.5, 0.0, 0.0))
    m.hull_points([p + s + Vector((0.0, 0.0, z)) for p in (p0, p1) for s in (side, -side) for z in (-below, h)])


def report(name, **info):
    REPORTS.setdefault(name, {}).update(info)


def save_reports(family):
    """Writes this script's model reports (from looter_homes' REPORTS and ours) for the look sheet."""
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    out = {}
    for name, rep in homes.REPORTS.items():
        out[name] = {k: v for k, v in rep.items() if k in ('name', 'triangles', 'materials', 'sockets', 'hulls',
                                                         'bounds', 'footprint')}
    for name, rep in REPORTS.items():
        out.setdefault(name, {}).update(rep)
    path = os.path.join(PREVIEW_DIR, f'{family}.report.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(out, f, indent=1, default=lambda v: list(v) if hasattr(v, '__iter__') else str(v))
    lt._log(f'mountain: reports {path}')
    return path


def describe(obj):
    """A model object's numbers for the report (for models not made through Home.finish)."""
    mesh = obj.data
    cos = [v.co for v in mesh.vertices]
    lo = Vector((min(c.x for c in cos), min(c.y for c in cos), min(c.z for c in cos)))
    hi = Vector((max(c.x for c in cos), max(c.y for c in cos), max(c.z for c in cos)))
    sockets = [c.name[7:].split('.')[0] for c in obj.children if c.name.startswith('SOCKET_')]
    hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
    info = dict(name=obj.name, triangles=sum(len(p.vertices) - 2 for p in mesh.polygons),
                materials=[mt.name for mt in mesh.materials], bounds=(tuple(lo), tuple(hi)), sockets=sockets,
                hulls=hulls)
    REPORTS[obj.name] = info
    print(f"MOUNTAIN: {obj.name}: {info['triangles']} triangles, {hulls} hulls, sockets {', '.join(sockets)}; "
          f"materials {', '.join(info['materials'])}; bounds x {lo.x:.2f}..{hi.x:.2f}, y {lo.y:.2f}..{hi.y:.2f}, "
          f"z {lo.z:.2f}..{hi.z:.2f}", flush=True)
    return info


# --- Previews ---

def stage(samples=64):
    return homes.stage(samples)


def shot(scene, sun, objs, name, view=homes.VIEW, sun_dir=homes.SUN, lens=35.0, size=homes.SHOT, margin=1.06,
         height_bias=0.42, exposure=-0.15, figure=None, folder=None):
    """A golden-hour render of objs framed from view into Saved/ArtPreviews/RansomsRest/Mountain/<name>.png; figure: a
    1.8 m clay figure's location (x, y, z) or None."""
    homes.light(scene, sun, homes.golden_sun(*sun_dir))
    framed = list(objs) + ([figure] if figure is not None else [])
    eye, target = homes.frame_shot(framed, view, size, lens, margin=margin, height_bias=height_bias)
    return homes.shoot(scene, os.path.join(folder or PREVIEW_DIR, name + '.png'), eye, target, lens, size, exposure)


def cut_copy(obj, z=None, plane=None, keep_below=True):
    """A copy of a model with everything above height z (or on the far side of plane: (point, normal)) cut away: a
    roof-off view of an interior. The copy is named '_Cut_...' so no export takes it."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = '_Cut_' + obj.name
    for child in list(copy.children):
        child.parent = None
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    if plane is None:
        plane = ((0.0, 0.0, z), (0.0, 0.0, 1.0))
    co, no = Vector(plane[0]), Vector(plane[1])
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=co, plane_no=no,
                           clear_outer=keep_below, clear_inner=not keep_below)
    bm.to_mesh(copy.data)
    bm.free()
    copy.data.update()
    return copy


def point_light(name, at, energy=60.0, color=(1.0, 0.72, 0.42), radius=0.08):
    light = bpy.data.objects.new(name, bpy.data.lights.new(name, 'POINT'))
    light.data.energy = energy
    light.data.color = color
    light.data.shadow_soft_size = radius
    light.location = Vector(at)
    bpy.context.scene.collection.objects.link(light)
    return light


def dark_world(strength=0.02):
    """A near-black world for lamp-lit shots underground."""
    world = bpy.data.worlds.get('_Dark') or bpy.data.worlds.new('_Dark')
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    bg.inputs['Color'].default_value = (0.05, 0.06, 0.08, 1.0)
    bg.inputs['Strength'].default_value = strength
    return world


def show_only(objs):
    keep = set()
    for o in objs:
        keep.add(o)
        for c in o.children_recursive:
            keep.add(c)
    for o in bpy.context.scene.objects:
        if o.type in ('MESH', 'CURVE', 'FONT'):
            o.hide_render = o not in keep or o.name.startswith(('UCX_', '_Sheet'))
    for o in keep:
        if o.name.startswith('UCX_'):
            o.hide_render = True
