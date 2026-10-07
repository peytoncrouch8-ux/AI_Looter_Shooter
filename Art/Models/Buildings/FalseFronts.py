"""The false fronts of Main Street on Ransom's Rest (Docs/Areas/RansomsRest.md, Main Street): four frontier shops, each a
tall square facade with a lettered sign board hiding a gable roof behind it, built on the house trim sheet, and the
shutters that slam as Ellis walks by.

  FalseFront_Undertaker  "Bright & Daughter, Undertakers": the town's most cared-for facade, cream clapboards (the house
                         trim's cream strip G) and black trim (WoodBlack), gilt lettering (BrassWorn), an arched parapet
                         with urns. Tilly Bright talks through the left window, its lower sash pushed up
                         (SOCKET_Speaker); behind it a shallow room with black drapes, a dim lamp (LanternGlow,
                         SOCKET_Light), a coffin on display, and a card on a string: "Back after the funeral". The
                         window's panes are lamplit (WindowGlow). Black crepe on the door. A plain back door and a
                         loading step face the undertaker's yard and the depot. SOCKET_Smoke tops the chimney.
  FalseFront_Saloon      "The Gilt Spur": two storeys with a balcony over the boardwalk on four posts, a stepped parapet
                         with a spur on it. Boarded up: planks nailed across every door and window, and a board across the
                         doors reading CLOSED FOR MOURNING.
  FalseFront_Store       "Pruitt's General Store": the widest front, flat under a bracketed cornice with a low pediment,
                         two big display windows round glazed double doors, and a deep porch (a plank deck that chains
                         with the boardwalk, posts and a shake roof). The display windows show the shop
                         (shop_window()): shelves of tins, jars and packets against its lamplit back wall, a dress on
                         a stand and bolts of cloth on one side, sacks, a keg and tools on the other; the doors' panes
                         are lamplit too (WindowGlow), and SOCKET_Light is the shop's lamp, behind the doors. Windows
                         down both sides carry shutter sockets (SOCKET_Shutter_L1..L4 / R1..R4). SOCKET_Smoke tops the
                         stovepipe.
  FalseFront_Sheriff     the empty sheriff's office: a stone storey (a jail) with quoins, barred windows and a plank
                         door hung with black crepe, under a wooden false front lettered SHERIFF with a star. The door
                         stands open on a walk-in front office (sheriff_office(); Side 1, the gang's Strongbox is here):
                         plaster walls, a board floor at the walk's height, the desk with its lamp lit (SOCKET_Light),
                         the chair knocked over, an empty gun rack, a barred cell door standing open on a dark cell.
                         SOCKET_Strongbox is the Strongbox's spot on the floor against the partition, facing the door;
                         SOCKET_Decal and SOCKET_Decal_2 are bare plaster for wanted posters. SOCKET_Smoke tops the
                         (cold) stovepipe.
  Shutter_Left/Right     a plank shutter (0.5 x 1.4 m) whose pivot is its hinge: the game hangs one on each
                         SOCKET_Shutter_L<n> / R<n>, where it hangs closed, and swings it open through 180 degrees (its
                         free edge out toward the street) until it lies flat against the wall. No collision.

Every building's floor and door thresholds are at the boardwalk's height (looter_town.DECK_TOP, 0.38 m), and its
pivot is the middle of its walls' footprint on the ground; the facade faces the front (-Y). A boardwalk (Boardwalk.py)
runs along the facade: the facade's front face is at y = -depth / 2 (FACADE_Y below), so the walk's pivot line goes
1.5 m in front of it. The store brings its own 3 m porch in place of the walk. Big roofs are shakes (the trim sheet's
tin reads as loot colors over a large area). Faces nobody sees are left out (walls' insides, the shakes' and
clapboards' hidden sides, the backs of trim), which keeps the sources at 5.4-9.3k triangles, and Nanite's fallback
keeps all of them (FALLBACK).

The lamplit windows (the store's and Tilly's) are the lived-in farmhouse's WindowGlow (Farmhouse.py): by day dark warm
glass with a faint glow, and at dusk the level's AHouseLights raises its Glow (1 to 14) and lights a lamp at the
building's SOCKET_Light.

A scripted model (Art/README.md) built with looter_buildings and looter_town.
"""
import math
import random
from contextlib import contextmanager

import bmesh

import looter_buildings as kit
import looter_model as lm
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Tile, Trim, Vector

BASE = town.DECK_TOP        # floors and thresholds at the boardwalk's walking surface
THICK = 0.14                # walls
FT = 0.16                   # the false front's thickness
FACADE_Y = {}               # each building's facade front (y), for the street preview and the report


def siding(rng, rotate=False):
    """Weathered siding (strip A) at world scale, slid along the strip so walls don't match."""
    return Trim('A', world=True, rotate=rotate, u=rng.uniform(0.0, 6.4))


# Nanite's fallback (what Medium and Low draw) keeps every triangle: Unreal's own reduction of these meshes left
# vertices with zero tangents (the saloon at 61.8%), and the sources are lean enough to be drawn whole.
FALLBACK = 100.0

# The lamplit panes: Farmhouse.py's WindowGlow exactly (its colour, its day Glow and Variation), so the name maps to one
# material instance in Unreal. Glow is the emissive multiplier of the colour; AHouseLights sets the dusk value.
WINDOW_COLOR, WINDOW_GLOW_DAY, WINDOW_GLOW_DUSK = 0x2e2219, 1.0, 14.0
_LATE = 30          # faces marked for WindowGlow while the building is put together (finish() gives them its slot)


class Refit(town.Model):
    """A town Model for changing part of a building in place (the lamplit windows, the sheriff's office), which leaves
    the rest of it exactly as it was: the same vertices, corners and faces in the same order, with the same baked AO.

    While glazing(glass) is on, the openings' panes (the trim sheet's H4 strip) go to glass: 'windowglow' (the WindowGlow
    slot), or (key, move) to be moved too (move, a matrix in the opening's space: a display window's pane goes back to
    become the shop's back wall). While moved(move, picks) is on, the parts built (those numbered in picks, counting
    from 0, or all of them) are moved (the sheriff's door, swung open). Everything is still built in the same order with
    the same random numbers, and a moved part moves only once the building is cut into triangles, so it's cut as before.
    WindowGlow's slot comes after all the others, so theirs keep their order. own(seed) builds new parts on a random
    stream of their own, into a mesh of their own that finish() adds after the building's own faces (cut with them, the
    new faces would take numbers among theirs). The AO the building's own faces keep is baked without the new parts,
    with the moved parts where they were: Blender's ray tree, built over more faces, breaks ties between touching
    surfaces otherwise and shifts the AO at scattered corners all over the building. The new parts and the moved ones
    take the AO of the whole model."""

    glass = None
    mover = None                    # (move, picks, [parts counted]) while moved() is on
    late = None

    def __init__(self, name, seed=1):
        super().__init__(name, seed)
        self.moves = []             # (vertex, its move in the model's space), for finish()
        self.main = self.bm         # the building's own mesh (self.bm is the new parts' while own() is on)

    def slot(self, key):
        if key == 'windowglow':
            return _LATE
        return super().slot(key)

    def emit(self, tb, uv=None, mat='trim', place=None, space=None, *args, **kwargs):
        move = None
        if self.glass and mat == 'trim' and isinstance(uv, Trim) and uv.strip == 'H4':
            mat, move = (self.glass, None) if isinstance(self.glass, str) else self.glass
        if self.mover is not None:
            how, picks, count = self.mover
            if picks is None or count[0] in picks:
                move = how
            count[0] += 1
        first = len(self.bm.verts)
        super().emit(tb, uv, mat, place, space, *args, **kwargs)
        if move is not None:
            frame = (space or kit.WORLD).matrix
            self.bm.verts.ensure_lookup_table()
            self.moves += [(v, frame @ move @ frame.inverted()) for v in self.bm.verts[first:]]

    def triangles(self):
        return sum(len(f.verts) - 2 for bm in (self.main, self.late) if bm is not None and bm.is_valid
                   for f in bm.faces)

    @contextmanager
    def glazing(self, glass):
        self.glass = glass
        try:
            yield
        finally:
            self.glass = None

    @contextmanager
    def moved(self, move, picks=None):
        self.mover = (move, picks, [0])
        try:
            yield
        finally:
            self.mover = None

    @contextmanager
    def own(self, seed):
        if self.late is None:
            self.late = bmesh.new()
            self.late.loops.layers.uv.new('UVMap')
            self.late.faces.layers.float.new('_aofloor')
        main = (self.rng, self.bm, self.uv, self.floor)
        self.rng, self.bm = random.Random(seed), self.late
        self.uv, self.floor = self.late.loops.layers.uv['UVMap'], self.late.faces.layers.float['_aofloor']
        try:
            yield self.rng
        finally:
            self.rng, self.bm, self.uv, self.floor = main

    def finish(self, **kwargs):
        late = self.late
        marked = [f for bm in (self.bm, late) if bm is not None for f in bm.faces if f.material_index == _LATE]
        if marked:
            index = len(self.slots)
            self.slots['windowglow'] = (index, lm.material('WindowGlow', WINDOW_COLOR, Glow=WINDOW_GLOW_DAY,
                                                           Variation=0.04))
            for f in marked:
                f.material_index = index
        cut, bake = kit._triangulate, lt.bake_vertex_ao
        was = {}                    # moved vertices by number: where they were
        own = []                    # the building's own vertex and face counts

        def cut_then_add(bm):
            cut(bm)
            moved = []
            for v, move in self.moves:
                moved.append((v, v.co.copy()))
                v.co = move @ v.co
            bm.verts.index_update()
            was.update((v.index, before) for v, before in moved)
            # The building read back in order (no gaps left by the cut for new elements to fill), then the new parts,
            # cut on their own, after it.
            mesh = kit.bpy.data.meshes.new('_refit')
            bm.to_mesh(mesh)
            bm.clear()
            bm.from_mesh(mesh)
            own.extend((len(bm.verts), len(bm.faces)))
            if late is not None:
                cut(late)
                late.to_mesh(mesh)
                late.free()
                bm.from_mesh(mesh)
            kit.bpy.data.meshes.remove(mesh)

        def bake_own(obj, *args, **kw):
            import numpy as np
            bake(obj, *args, **kw)
            # The building's own faces alone, the moved parts put back, baked again: their AO as it always was.
            tb = bmesh.new()
            tb.from_mesh(obj.data)
            tb.verts.ensure_lookup_table()
            bmesh.ops.delete(tb, geom=tb.verts[own[0]:], context='VERTS')
            tb.verts.ensure_lookup_table()
            for i, co in was.items():
                tb.verts[i].co = co
            alone = kit.bpy.data.objects.new('_refit_ao', kit.bpy.data.meshes.new('_refit_ao'))
            tb.to_mesh(alone.data)
            tb.free()
            kit.bpy.context.scene.collection.objects.link(alone)
            alone.matrix_world = obj.matrix_world
            bake(alone, *args, **kw)
            mesh = obj.data
            polygons = list(mesh.polygons)[:own[1]]
            corners = sum(p.loop_total for p in polygons)
            keep = np.ones(corners, dtype=bool)
            for p in polygons:
                if any(i in was for i in p.vertices):
                    keep[p.loop_start:p.loop_start + p.loop_total] = False
            raw = lt._read_col(mesh)
            raw[:corners][keep, 3] = lt._read_col(alone.data)[:corners][keep, 3]
            lt._write_col(mesh, raw)
            data = alone.data
            kit.bpy.data.objects.remove(alone)
            kit.bpy.data.meshes.remove(data)

        if late is None and not self.moves:
            return super().finish(**kwargs)
        kit._triangulate, lt.bake_vertex_ao = cut_then_add, bake_own
        try:
            return super().finish(**kwargs)
        finally:
            kit._triangulate, lt.bake_vertex_ao = cut, bake
            self.late, self.moves = None, []


# --- The parts every false front shares ---

def false_front(m, x0, x1, y0, top, items, finish_siding=None, bottom=-BASE, covered=False):
    """The facade panel: its front face at y = y0 from x0 to x1, from the ground (bottom, in the facade's space whose
    z = 0 is the floor) up to the top profile (points (x, z) left to right in the facade's space), with holes for the
    openings (doors included: the panel runs on below them to the ground). covered: clapboards hide its face, so it
    isn't cut for the baked shading round the openings. Returns its space."""
    space = kit.wall_space((x0, y0), (x1, y0), BASE)
    width = x1 - x0
    outline = [(0.0, bottom), (width, bottom)] + list(reversed(top))
    holes = [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items]
    m.panel(outline, holes, FT, finish_siding or siding(m.rng), space=space, around=() if covered else items,
            slices=0)
    return space


def shell(m, X0, X1, YF, Y1, wall_h, pitch, sides, back, rotate=False, water_table=True, corner='C', stone=False):
    """The walls behind a false front: the two side (eave) walls from the facade's back (YF) to Y1 and the back gable
    wall, on a fieldstone plinth, with their openings, corner boards and a water table along the bottom (stone: walls
    of fieldstone instead, without the boards). sides holds the right and left walls' openings with x measured from the
    front; back holds the back wall's (x from its left seen from behind). Returns the spaces by name; each space's
    items are its openings in its own coordinates."""
    rng = m.rng
    town.plinth(m, X0, X1, YF + 0.1, Y1, BASE, out=0.06)
    if stone:
        water_table, corner = False, None
    width = X1 - X0
    rise = width * 0.5 * math.tan(math.radians(pitch))
    length = Y1 - YF
    walls = {
        'right': ((X1, YF), (X1, Y1), None, sides.get('right', [])),
        'back': ((X1, Y1), (X0, Y1), (width * 0.5, wall_h + rise), back),
        'left': ((X0, Y1), (X0, YF), None, [Opening(o.kind, length - o.x - o.w, o.z, o.w, o.h, **o.opts)
                                            for o in sides.get('left', [])]),
    }
    spaces = {}
    for name, (p0, p1, apex, items) in walls.items():
        space = kit.wall_space(p0, p1, BASE)
        space.items = items
        spaces[name] = space
        span = (Vector(p1) - Vector(p0)).length
        uv = Trim('D', world=True, u=rng.uniform(0.0, 6.4)) if stone else siding(rng, rotate)
        town.panel(m, kit.wall_outline(span, wall_h, items, apex), kit.holes(items), THICK, uv, space=space,
                   around=items, slices=4.0, back=False)
        town.openings(m, space, items, THICK)
        top = wall_h if apex is None else wall_h - 0.02
        for x in ((0.07, span - 0.07) if apex is not None else (span - 0.07,) if name == 'right' else (0.07,)):
            if corner:
                town.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), 0.15, corner, thick=0.045)
        if water_table:
            spans = [(0.1, span - 0.1)]
            for o in items:
                if o.z <= 1e-4:
                    spans = town.subtract(spans, (o.x - 0.2, o.x + o.w + 0.2))
            for a, b in spans:
                town.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, 'C', thick=0.045)
        if apex is None and not stone:
            town.trim_board(m, space, (0.12, 0.0, wall_h - 0.14), (span - 0.12, 0.0, wall_h - 0.14), 0.24, 'A',
                            thick=0.035)
    return spaces


def hulls(m, XF0, XF1, Y0, X0, X1, Y1, eave, pitch, facade_h, back_over=0.3, overhang=0.28):
    """The usual collision: the facade, the walls and the roof."""
    m.hull((XF1 - XF0, FT + 0.04, facade_h), at=((XF0 + XF1) * 0.5, Y0 + FT * 0.5, facade_h * 0.5))
    m.hull((X1 - X0 + 0.1, Y1 - Y0 - FT, eave), at=((X0 + X1) * 0.5, (Y0 + FT + Y1) * 0.5, eave * 0.5))
    tan = math.tan(math.radians(pitch))
    ridge = eave + (X1 - X0) * 0.5 * tan + 0.25
    low = eave - overhang * tan + 0.2
    m.hull_points([(x, y, low) for x in (X0 - overhang, X1 + overhang) for y in (Y0 + FT, Y1 + back_over)] +
                  [((X0 + X1) * 0.5, y, ridge) for y in (Y0 + FT, Y1 + back_over)])


def roof_height(X0, X1, eave, pitch):
    """The roof's top surface over world x (for the props behind a facade)."""
    tan = math.tan(math.radians(pitch))
    half = (X1 - X0) * 0.5
    mid = (X0 + X1) * 0.5
    return lambda x: eave + max(0.0, half - abs(x - mid)) * tan + 0.16


def arch(x0, x1, z0, rise, segments=8):
    """Points of a segmental arch from (x0, z0) to (x1, z0), rising by rise in the middle (both ends included)."""
    chord = x1 - x0
    radius = (chord * chord * 0.25 + rise * rise) / (2.0 * rise)
    cx, cz = (x0 + x1) * 0.5, z0 + rise - radius
    a0 = math.asin(chord * 0.5 / radius)
    return [(cx + radius * math.sin(-a0 + 2.0 * a0 * k / segments), cz + radius * math.cos(-a0 + 2.0 * a0 * k / segments))
            for k in range(segments + 1)]


def urn(m, space, at, scale=1.0, look='woodblack'):
    """A turned urn finial standing on at."""
    s = scale
    town.lathe(m, [(0.0, 0.0), (0.13 * s, 0.0), (0.13 * s, 0.05 * s), (0.075 * s, 0.085 * s), (0.065 * s, 0.14 * s),
                   (0.13 * s, 0.21 * s), (0.155 * s, 0.29 * s), (0.125 * s, 0.37 * s), (0.065 * s, 0.41 * s),
                   (0.075 * s, 0.45 * s), (0.035 * s, 0.5 * s), (0.0, 0.53 * s)], at=at, sides=8, look=look, space=space)


def lamp_room_drapes(m, space, o):
    """Black drapes over the lamplit panes of window o (style 'open', as town.window builds it): in front of the
    pushed-up lower sash's pane, inside its frame, one gathered at each side and a scalloped valance under its top
    rail, in two halves either side of its muntin. They carry on the lamp room's drapes seen below them, so the glowing
    panes read as the lit room behind drapes, not as a panel."""
    glass_y = FT * 0.55 - 0.05              # the lower sash's pane; its face is 1 cm in front, its frame 2.25 cm
    y = glass_y - 0.0145                    # 2.5 mm in front of the pane's face, behind the frame's
    stile, rail = 0.052, 0.082              # the sash frame's stiles and top rail (with 2 mm to spare)
    x0, x1, xc = o.x + stile, o.x + o.w - stile, o.x + o.w * 0.5
    low, top = o.z + o.h * 0.5 + 0.04, o.z + o.h - rail
    for side in (1.0, -1.0):
        edge = x0 if side > 0 else x1
        rows = []
        for z, cover in ((low, 0.2), ((low + top) * 0.5, 0.25), (top, 0.3)):
            rows.append([(edge + side * cover * i / 5.0, y + (0.002 if i % 2 else -0.002), z) for i in range(6)])
        town.sheet(m, rows, look='crepe', space=space)
    for a, b in ((x0, xc - 0.022), (xc + 0.022, x1)):
        bottom = [(a + (b - a) * i / 8.0, y - 0.005, top - 0.12 - 0.045 * math.sin(math.pi * (i % 4) / 4.0))
                  for i in range(9)]
        town.sheet(m, [bottom, [(x, y - 0.005, top) for x, _, _ in bottom]], look='crepe', space=space)


# --- Bright & Daughter, Undertakers ---

def undertaker():
    m = Refit('FalseFront_Undertaker', seed=301)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -3.5, 3.5, -6.0, 6.0
    X0, X1 = -3.2, 3.2
    YF = Y0 + FT
    WALL, PITCH = 3.3, 38.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    # The parapet: flat shoulders and a segmental arch over the middle, all above the ridge (5.8 m over the floor).
    SHOULDER = 6.15
    top = [(0.0, SHOULDER), (1.75, SHOULDER)] + arch(1.75, 5.25, SHOULDER, 0.8)[1:-1] + [(5.25, SHOULDER),
                                                                                         (WF, SHOULDER)]
    TILLY = Opening('window', 0.75, 0.8, 1.6, 1.95, style='open', casing='woodblack', sash='woodblack', panes=(2, 2),
                    head='pediment', casing_w=0.14)
    RIGHT = Opening('window', 4.65, 0.8, 1.6, 1.95, style='glass', casing='woodblack', sash='woodblack', panes=(2, 2),
                    head='pediment', casing_w=0.14)
    DOOR = Opening('door', 2.95, 0.0, 1.1, 2.78, style='panel', finish='woodblack', casing='woodblack', knob='brass',
                   transom=0.4, casing_w=0.15, panels='world_G')
    UPPER = [Opening('window', 1.25, 4.95, 0.66, 0.92, style='blind', casing='woodblack', sash='woodblack', panes=(1, 2),
                     casing_w=0.11),
             Opening('window', 5.09, 4.95, 0.66, 0.92, style='blind', casing='woodblack', sash='woodblack', panes=(1, 2),
                     casing_w=0.11)]
    items = [TILLY, RIGHT, DOOR] + UPPER
    front = false_front(m, XF0, XF1, Y0, top, items, covered=True)
    m.section('facade panel')

    # Clapboards in cream over the face, stopping at the casings, the pilasters and the sign board.
    outline = [(0.0, -BASE), (WF, -BASE)] + list(reversed(top))
    keep = [(o.x - 0.16, o.x + o.w + 0.16, o.z - 0.1, o.z + o.h + (0.55 if o.opts.get('head') else 0.25))
            for o in items]
    keep += [(-0.1, 0.33, -1.0, 9.0), (WF - 0.33, WF + 0.1, -1.0, 9.0), (0.25, WF - 0.25, 3.12, 4.62)]
    lap = town.clapboards(m, front, outline, 0.3, 7.2, keep, look='band_G', exposure=0.2)
    # A black kick board along the foot, broken by the door.
    for a, b in ((0.0, DOOR.x - 0.15), (DOOR.x + DOOR.w + 0.15, WF)):
        town.board(m, (a, 0.0, 0.0), (b, 0.0, 0.0), 0.62, 0.05, look='woodblack', space=front, lift=0.025)
    # Pilasters at the corners, up to the sign band, and black corner boards on up to the shoulders.
    for x in (0.18, WF - 0.18):
        town.pilaster(m, front, x, 0.3, 3.1, width=0.3, look='woodblack', depth=0.07)
        town.board(m, (x, 0.0, 3.16), (x, 0.0, SHOULDER - 0.02), 0.3, 0.05, look='woodblack', space=front, lift=0.025)
    m.section('clapboards')

    # Tilly's window's panes are lamplit, so the room reads lit at dusk (with drapes over them, lamp_room_drapes).
    for o in items:
        with m.glazing('windowglow' if o is TILLY else None):
            town.openings(m, front, [o], FT, out=lap)
    # Crepe: a bow and tails on the door, a swag over its head.
    town.crepe_bow(m, front, (DOOR.x + DOOR.w * 0.5, 0.0, 1.92), scale=1.4, tails=0.85, y=-0.02)
    town.crepe_swag(m, front, DOOR.x - 0.2, DOOR.x + DOOR.w + 0.2, DOOR.h + 0.42, sag=0.16, y=-0.12)
    m.section('openings')

    # The sign: a black field framed in black with a cream bead, gilt letters.
    sx0, sx1, sz0, sz1 = 0.3, WF - 0.3, 3.18, 4.52
    town.box(m, (sx1 - sx0, 0.05, sz1 - sz0), at=((sx0 + sx1) * 0.5, -lap - 0.025, (sz0 + sz1) * 0.5), look='woodblack',
             space=front, cuts=4)
    field_y = -lap - 0.05
    for z in (sz0 + 0.06, sz1 - 0.06):
        town.board(m, (sx0 - 0.06, field_y, z), (sx1 + 0.06, field_y, z), 0.14, 0.05, look='woodblack', space=front,
                   lift=0.025)
    for x in (sx0 + 0.02, sx1 - 0.02):
        town.board(m, (x, field_y, sz0 + 0.12), (x, field_y, sz1 - 0.12), 0.14, 0.05, look='woodblack', space=front,
                   lift=0.025)
    for z in (sz0 + 0.15, sz1 - 0.15):
        town.board(m, (sx0 + 0.12, field_y, z), (sx1 - 0.12, field_y, z), 0.025, 0.012, look='band_G', space=front,
                   lift=0.006)
    for x in (sx0 + 0.11, sx1 - 0.11):
        town.board(m, (x, field_y, sz0 + 0.14), (x, field_y, sz1 - 0.14), 0.025, 0.012, look='band_G', space=front,
                   lift=0.006)
    town.text(m, 'BRIGHT & DAUGHTER', (WF * 0.5, field_y, 4.1), height=0.36, width=5.6, space=front, look='brass',
              tol=0.028)
    town.text(m, 'UNDERTAKERS', (WF * 0.5, field_y, 3.56), height=0.25, space=front, look='brass', tol=0.028)
    # A ledge over the sign on small brackets.
    town.box(m, (WF - 0.2, 0.22, 0.1), at=(WF * 0.5, -lap - 0.11, 4.62), look='woodblack', space=front)
    for k in range(7):
        town.bracket(m, front, 0.45 + (WF - 0.9) * k / 6, 4.57, reach=0.17, drop_h=0.22, thick=0.06, look='woodblack',
                     out=lap)
    m.section('sign')

    # Over the ledge: an oval medallion with the firm's monogram between the two blind windows.
    mx, mz = WF * 0.5, 5.45
    ring = [(mx + 0.5 * math.cos(2.0 * math.pi * k / 16), mz + 0.36 * math.sin(2.0 * math.pi * k / 16))
            for k in range(16)]
    town.prism(m, ring, 0.04, look='woodblack', space=front, y=-lap - 0.04)
    inner = [(mx + 0.41 * math.cos(2.0 * math.pi * k / 16), mz + 0.28 * math.sin(2.0 * math.pi * k / 16))
             for k in range(16)]
    town.prism(m, inner, 0.01, look='band_G', space=front, y=-lap - 0.05)
    town.text(m, 'B&D', (mx, -lap - 0.05, mz), height=0.26, space=front, look='brass', tol=0.028)
    # The parapet's cap and cornice, urns on the shoulders and a ball over the arch.
    town.cap_profile(m, front, top, depth=0.24, height=0.14, look='woodblack', out=lap, back=FT + 0.06)
    town.cap_profile(m, front, [(x, z - 0.14) for x, z in top], depth=0.15, height=0.1, look='woodblack', out=lap,
                     back=0.0)
    for x in (0.18, WF - 0.18):
        urn(m, front, (x, -0.05, SHOULDER + 0.14), scale=1.1)
    apex = max(z for _, z in top)
    town.lathe(m, [(0.0, 0.0), (0.06, 0.0), (0.06, 0.04), (0.035, 0.07), (0.09, 0.14), (0.1, 0.2), (0.07, 0.27),
                   (0.0, 0.3)], at=(WF * 0.5, -0.05, apex + 0.14), sides=8, look='woodblack', space=front)
    m.section('parapet')

    # Behind Tilly's window: a shallow lit room. Pale walls (the black drapes and the coffin stand out on them), a plank
    # floor, the lamp.
    rx0, rx1, depth = 0.55, 2.55, 1.5
    ry0, ry1 = FT, FT + depth
    town.box(m, (rx1 - rx0, 0.04, 3.0), at=((rx0 + rx1) * 0.5, ry1 + 0.02, 1.5), look='world_G', space=front)
    for x in (rx0 - 0.02, rx1 + 0.02):
        town.box(m, (0.04, depth, 3.0), at=(x, (ry0 + ry1) * 0.5, 1.5), look='world_G', space=front)
    town.box(m, (rx1 - rx0, depth, 0.04), at=((rx0 + rx1) * 0.5, (ry0 + ry1) * 0.5, 3.0), look='woodblack', space=front)
    for k in range(int((rx1 - rx0) / 0.2)):
        x = rx0 + 0.1 + 0.2 * k
        town.board(m, (x, ry0, -0.02), (x, ry1, -0.02), 0.19, 0.04, look='A', face=(0.0, 0.0, 1.0), space=front)
    # Drapes gathered at each side of the window, and a valance across its head.
    for x_a, x_b in ((TILLY.x - 0.08, TILLY.x + 0.3), (TILLY.x + TILLY.w - 0.3, TILLY.x + TILLY.w + 0.08)):
        rows = []
        for j in range(5):
            z = TILLY.z - 0.02 + (TILLY.h + 0.05) * j / 4
            squeeze = 0.65 + 0.35 * (j / 4)   # gathered toward the bottom where they're tied back
            row = []
            for i in range(9):
                t = i / 8
                mid = (x_a + x_b) * 0.5
                x = mid + (x_a + (x_b - x_a) * t - mid) * squeeze
                row.append((x, FT + 0.12 + 0.035 * math.sin(t * math.pi * 4.0), z))
            rows.append(row)
        town.sheet(m, rows, look='crepe', space=front)
    town.crepe_swag(m, front, TILLY.x - 0.05, TILLY.x + TILLY.w + 0.05, TILLY.z + TILLY.h + 0.02, sag=0.1, width=0.2,
                    y=FT + 0.1)
    # The lamp on a small round table: a brass font, and a glowing globe.
    lx, ly = TILLY.x + TILLY.w * 0.36, FT + 0.55
    town.lathe(m, [(0.0, 0.0), (0.2, 0.0), (0.2, 0.03), (0.04, 0.05), (0.035, 0.6), (0.24, 0.62), (0.24, 0.66),
                   (0.0, 0.67)], at=(lx, ly, 0.2), sides=8, look='woodblack', space=front)
    town.lathe(m, [(0.0, 0.0), (0.075, 0.0), (0.075, 0.02), (0.03, 0.05), (0.025, 0.09), (0.085, 0.14), (0.08, 0.2),
                   (0.03, 0.23), (0.0, 0.24)], at=(lx, ly, 0.87), sides=8, look='brass', space=front)
    town.lathe(m, [(0.0, 0.0), (0.04, 0.0), (0.095, 0.06), (0.11, 0.13), (0.09, 0.21), (0.05, 0.25), (0.0, 0.26)],
               at=(lx, ly, 1.1), sides=8, look='glow', space=front)
    lamp = front.world((lx, ly, 1.23))
    m.socket('Light', lamp)
    # A coffin stood on its foot against the back wall, for show.
    cx = TILLY.x + TILLY.w * 0.66
    coffin = [(-0.19, 0.0), (0.19, 0.0), (0.29, 1.3), (0.25, 1.88), (-0.25, 1.88), (-0.29, 1.3)]
    town.prism(m, [(cx + x, z + 0.02) for x, z in coffin], 0.3, look='woodblack', space=front, y=ry1 - 0.32)
    town.prism(m, [(cx + x * 0.8, 0.12 + z * 0.9) for x, z in coffin], 0.02, look='woodblack', space=front, y=ry1 - 0.34)
    for z in (0.7, 1.4):
        town.box(m, (0.5 if z > 1.0 else 0.44, 0.03, 0.035), at=(cx, ry1 - 0.35, z), look='brass', space=front)
    # The card on a string from the raised sash: "Back after the funeral".
    card_y = FT * 0.55 - 0.1
    card_z = TILLY.z + TILLY.h * 0.5 - 0.27
    for dx in (-0.1, 0.1):
        town.box(m, (0.006, 0.006, 0.2), at=(lx + dx * 0.9 + 0.02, card_y + 0.004, card_z + 0.18), rot=(0.0, dx * 40.0, 0.0),
                 look='band_G', space=front)
    town.box(m, (0.31, 0.008, 0.19), at=(lx + 0.02, card_y, card_z), rot=(0.0, 1.5, 0.0), look='band_G', space=front)
    town.text(m, 'Back after', (lx + 0.02, card_y - 0.004, card_z + 0.042), width=0.25, height=0.065, space=front,
              look='woodblack', tol=0.05, rot=1.5)
    town.text(m, 'the funeral', (lx + 0.02, card_y - 0.004, card_z - 0.045), width=0.26, height=0.07, space=front,
              look='woodblack', tol=0.05, rot=1.5)
    m.socket('Speaker', front.world((TILLY.x + TILLY.w * 0.5, -0.12, TILLY.z + TILLY.h * 0.3)))
    m.section('window room')

    # The walls behind, the back door and its loading step, the roof and a fieldstone chimney.
    side_window = dict(style='glass', casing='A', panes=(2, 2))
    sides = {'right': [Opening('window', 3.4, 0.95, 0.9, 1.3, **side_window),
                       Opening('window', 8.6, 0.95, 0.9, 1.3, style='boarded')],
             'left': [Opening('window', 4.2, 0.95, 0.9, 1.3, **side_window)]}
    BACK_DOOR = Opening('door', 3.9, 0.0, 1.05, 2.15, hinge='right', casing='C')
    back = [BACK_DOOR, Opening('window', 1.3, 1.0, 0.8, 1.1, style='glass', casing='A', panes=(2, 2)),
            Opening('window', 2.78, 3.7, 0.84, 0.75, style='boarded')]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    m.section('walls')
    # The loading step: a plank platform at the floor on a sill, with one step down to the yard.
    bs = spaces['back']
    step_x0, step_x1 = BACK_DOOR.x - 0.35, BACK_DOOR.x + BACK_DOOR.w + 0.35
    for k in range(int(round((step_x1 - step_x0) / 0.2))):
        x = step_x0 + 0.1 + 0.2 * k
        town.board(m, (x, -1.05, 0.0 - 0.025 + rng.uniform(-0.004, 0.0)), (x, 0.02, -0.025), 0.19, 0.05, look='A',
                   face=(0.0, 0.0, 1.0), space=bs)
    for y in (-1.0, -0.1):
        town.box(m, (step_x1 - step_x0, 0.1, BASE - 0.05), at=((step_x0 + step_x1) * 0.5, y, -BASE * 0.5 - 0.05 + 0.02),
                 look='C', space=bs)
    for x in (step_x0 + 0.03, step_x1 - 0.03):
        town.board(m, (x, -1.06, -0.2), (x, 0.0, -0.2), 0.3, 0.05, look='A', face=(1.0 if x > 1.0 else -1.0, 0.0, 0.0),
                   space=bs)
    tread_z = -BASE * 0.5
    for k in range(2):
        town.board(m, (step_x0 + 0.15, -1.14 - 0.15 * k, tread_z - 0.025), (step_x1 - 0.15, -1.14 - 0.15 * k,
                   tread_z - 0.025), 0.15, 0.05, look='A', face=(0.0, 0.0, 1.0), space=bs)
    town.board(m, (step_x0 + 0.15, -1.36, tread_z * 0.5 - BASE * 0.25), (step_x1 - 0.15, -1.36, tread_z * 0.5 - BASE * 0.25),
               BASE * 0.5 - 0.02, 0.03, look='A', space=bs)
    m.section('loading step')
    CHIMNEY = (-1.7, 3.6)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.35, EAVE, PITCH, covering='shakes', overhang=0.3, sag=0.07,
                      chimney=(CHIMNEY[0], CHIMNEY[1], 0.42, 0.4))
    m.section('roof')
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    chimney_top = EAVE + (X1 - X0) * 0.5 * math.tan(math.radians(PITCH)) + 0.7
    foot = roof_z(CHIMNEY[0]) - 0.7
    kit.stone_stack(m, (CHIMNEY[0], CHIMNEY[1], foot), (0.6, 0.7), chimney_top - foot, taper=0.03)
    m.box((0.76, 0.86, 0.1), at=(CHIMNEY[0], CHIMNEY[1], chimney_top + 0.05), uv=Trim('D', world=True), bevel=0.02)
    m.box((0.34, 0.42, 0.16), at=(CHIMNEY[0], CHIMNEY[1], chimney_top + 0.18), uv='D')
    m.socket('Smoke', (CHIMNEY[0], CHIMNEY[1], chimney_top + 0.3))
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    # A stone footing under the facade's returns, where the boardwalk doesn't reach.
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('chimney, facade back')
    # Drapes over Tilly's lamplit pane, built last on a stream of their own (the rest stays as it was).
    with m.own(311):
        lamp_room_drapes(m, front, TILLY)
    m.section('lamplit window')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + SHOULDER + 0.14, back_over=0.35, overhang=0.3)
    m.hull_points([(XF0 + x, y, BASE + z + 0.14) for x, z in top if 1.7 < x < 5.3 for y in (Y0 - 0.1, Y0 + FT)] +
                  [(XF0 + x, y, BASE + SHOULDER) for x in (1.75, 5.25) for y in (Y0 - 0.1, Y0 + FT)])
    m.hull((step_x1 - step_x0, 1.1, BASE), at=(X1 - (step_x0 + step_x1) * 0.5, Y1 + 0.55, BASE * 0.5))
    m.hull((step_x1 - step_x0 - 0.3, 0.32, BASE * 0.5), at=(X1 - (step_x0 + step_x1) * 0.5, Y1 + 1.24, BASE * 0.25))
    m.hull((0.62, 0.72, chimney_top - foot + 0.3), at=(CHIMNEY[0], CHIMNEY[1], (foot + chimney_top) * 0.5))
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


# --- The Gilt Spur ---

def saloon():
    m = town.Model('FalseFront_Saloon', seed=302)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -4.2, 4.2, -7.0, 7.0
    X0, X1 = -3.9, 3.9
    YF = Y0 + FT
    WALL, PITCH = 6.0, 18.0          # two storeys of 3 m
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    UP = 3.0                          # the second floor over the first
    top = [(0.0, 6.75), (1.4, 6.75), (1.4, 7.15), (2.7, 7.15), (2.7, 7.55), (5.7, 7.55), (5.7, 7.15), (7.0, 7.15),
           (7.0, 6.75), (WF, 6.75)]
    boarded = dict(style='boarded', casing='A', boards='planks', panes=(1, 1))
    LEFT = Opening('window', 0.75, 0.75, 1.9, 1.8, **boarded)
    RIGHT = Opening('window', 5.75, 0.75, 1.9, 1.8, **boarded)
    DOORS = Opening('door', 3.55, 0.0, 1.3, 2.62, style='double', finish='planks', casing='C', knob='H3', transom=0.4,
                    glass=True)
    UP_LEFT = Opening('window', 1.0, UP + 0.75, 1.2, 1.5, **boarded)
    UP_RIGHT = Opening('window', 6.2, UP + 0.75, 1.2, 1.5, **boarded)
    UP_DOOR = Opening('door', 3.7, UP, 1.0, 2.2)           # the balcony door's hole; the door is built a storey up
    items = [LEFT, RIGHT, DOORS, UP_LEFT, UP_RIGHT, UP_DOOR]
    front = false_front(m, XF0, XF1, Y0, top, items, covered=True)
    m.section('facade panel')
    town.openings(m, front, [LEFT, RIGHT, DOORS, UP_LEFT, UP_RIGHT], FT)
    upper = kit.wall_space((XF0, Y0), (XF1, Y0), BASE + UP)
    town.door(m, upper, Opening('door', UP_DOOR.x, 0.0, UP_DOOR.w, UP_DOOR.h, style='plank', finish='A', casing='C'),
              FT)
    # Everything boarded: the doors and the balcony door get their planks too, and the doors the mourning board.
    town.board_up(m, front, DOORS.x, 0.0, DOORS.w, DOORS.h - 0.1, look='planks', out=0.07, level=False)
    town.board_up(m, upper, UP_DOOR.x, 0.0, UP_DOOR.w, UP_DOOR.h, look='planks', out=0.06)
    sign_z = 1.42
    town.board(m, (DOORS.x - 0.45, -0.17, sign_z - 0.06), (DOORS.x + DOORS.w + 0.45, -0.17, sign_z + 0.06), 0.36, 0.04,
               look='planks', space=front)
    for end in (-1.0, 1.0):
        x = DOORS.x + DOORS.w * 0.5 + end * (DOORS.w * 0.5 + 0.33)
        town.box(m, (0.035, 0.014, 0.035), at=(x, -0.196, sign_z + end * 0.05), look='H3', space=front)
    town.text(m, 'CLOSED FOR MOURNING', (DOORS.x + DOORS.w * 0.5, -0.19, sign_z - 0.005), height=0.15, width=2.0,
              space=front, look='black', tol=0.03, rot=-math.degrees(math.atan2(0.12, DOORS.w + 0.9)))
    # Corner boards up the facade.
    for x in (0.1, WF - 0.1):
        town.trim_board(m, front, (x, 0.0, -BASE), (x, 0.0, 6.6), 0.2, 'C', thick=0.05)
    m.section('openings')

    # The sign: oxide-red boards in a weathered frame, faded cream letters; a spur over it on the parapet.
    sx0, sx1, sz0, sz1 = 0.55, WF - 0.55, 5.42, 6.42
    count = 5
    for k in range(count):
        z = sz0 + 0.06 + (sz1 - sz0 - 0.12) * (k + 0.5) / count
        town.board(m, (sx0, -0.02, z), (sx1, -0.02, z), (sz1 - sz0 - 0.12) / count - 0.006, 0.035, look='H2',
                   space=front, lift=0.0)
    for z in (sz0 + 0.03, sz1 - 0.03):
        town.board(m, (sx0 - 0.08, -0.05, z), (sx1 + 0.08, -0.05, z), 0.12, 0.05, look='C', space=front)
    for x in (sx0 - 0.02, sx1 + 0.02):
        town.board(m, (x, -0.05, sz0 + 0.09), (x, -0.05, sz1 - 0.09), 0.12, 0.05, look='C', space=front)
    town.text(m, 'THE GILT SPUR', (WF * 0.5, -0.05, (sz0 + sz1) * 0.5), height=0.48, width=6.4, space=front,
              look='cream', tol=0.028)
    # The spur's rowel: an eight-pointed star round a hub, and the heel band under it.
    cx, cz = WF * 0.5, 7.06
    star = []
    for k in range(16):
        r = 0.3 if k % 2 == 0 else 0.11
        a = math.pi * 0.5 + math.pi * k / 8
        star.append((cx + r * math.cos(a), cz + r * math.sin(a)))
    town.prism(m, star, 0.03, look='cream', space=front, y=-0.03)
    town.prism(m, [(cx + 0.07 * math.cos(2 * math.pi * k / 8), cz + 0.07 * math.sin(2 * math.pi * k / 8))
                   for k in range(8)], 0.02, look='H2', space=front, y=-0.04)
    band = []
    for k in range(9):
        a = math.radians(200.0 + 140.0 * k / 8)
        band.append((cx + 0.62 * math.cos(a), cz + 0.12 + 0.42 * math.sin(a)))
    for k in range(8, -1, -1):
        a = math.radians(200.0 + 140.0 * k / 8)
        band.append((cx + 0.52 * math.cos(a), cz + 0.12 + 0.33 * math.sin(a)))
    town.prism(m, band, 0.03, look='cream', space=front, y=-0.03)
    # A cornice over the sign on brackets, and caps along the stepped parapet.
    town.cornice(m, front, 0.0, WF, 6.72, look='C', project=0.28, height=0.26, brackets=1.2, frieze=0.0)
    town.cap_profile(m, front, top, depth=0.16, height=0.12, look='C', back=FT + 0.05)
    m.section('sign, parapet')

    # The balcony over the boardwalk: a deck at the second floor on a beam and four posts, a railing of square
    # balusters, a board ceiling under it.
    porch = kit.wall_space((XF0, Y0), (XF1, Y0), 0.0)
    deck_z = BASE + UP + 0.06
    edge = -town.WALK_DEPTH
    post_y = edge + 0.16
    posts = [0.2, 2.95, WF - 2.95, WF - 0.2]
    rail_top = deck_z + 1.02
    # The deck runs across the facade; town.deck works in the model's frame, so it's built in world coordinates.
    town.deck(m, XF0, XF1, Y0 + edge, Y0 + 0.02, top=deck_z, posts=(), sill=False, joists=0, back=False, plank_cuts=0)
    town.box(m, (WF, -edge, 0.03), at=(0.0, Y0 + edge * 0.5, deck_z - 0.27), look='world_A', cuts=6)
    town.board(m, (XF0, Y0 + post_y, deck_z - 0.2), (XF1, Y0 + post_y, deck_z - 0.2), 0.26, 0.2, look='C',
               space=None, face=(0.0, -1.0, 0.0))
    for x in (XF0 + 0.05, XF1 - 0.05):
        town.board(m, (x, Y0 + edge, deck_z - 0.17), (x, Y0, deck_z - 0.17), 0.22, 0.06, look='C',
                   face=(1.0 if x > 0 else -1.0, 0.0, 0.0))
    for x in posts:
        town.board(m, (x, post_y, -0.05), (x, post_y, rail_top + 0.12), 0.18, 0.18, look='C', space=porch,
                   face=(0.0, -1.0, 0.0), bevel=0.015)
        town.box(m, (0.26, 0.26, 0.06), at=(x, post_y, rail_top + 0.15), look='C', space=porch)
        town.box(m, (0.24, 0.24, 0.05), at=(x, post_y, BASE + 0.02), look='C', space=porch)
        for side in (-1.0, 1.0):
            if 0.3 < x + side * 0.5 < WF - 0.3:
                town.board(m, (x + side * 0.08, post_y, deck_z - 0.75), (x + side * 0.6, post_y, deck_z - 0.3), 0.11,
                           0.09, look='C', space=porch)
    # Railing: rails along the front and the two ends, balusters between.
    runs = [((0.2, post_y), (WF - 0.2, post_y)), ((0.1, post_y), (0.1, -0.05)), ((WF - 0.1, post_y), (WF - 0.1, -0.05))]
    for (ax, ay), (bx, by) in runs:
        for z, h in ((rail_top - 0.04, 0.09), (deck_z + 0.13, 0.07)):
            town.board(m, (ax, ay, z), (bx, by, z), h, 0.07, look='C', space=porch,
                       face=(0.0, -1.0, 0.0) if ay == by else (-1.0 if ax < 1.0 else 1.0, 0.0, 0.0))
        span = math.hypot(bx - ax, by - ay)
        count = int(span / 0.26)
        for k in range(1, count):
            t = k / count
            px, py = ax + (bx - ax) * t, ay + (by - ay) * t
            if any(abs(px - p) < 0.15 for p in posts) and ay == by:
                continue
            town.box(m, (0.05, 0.05, rail_top - deck_z - 0.22), at=(px, py, (rail_top + deck_z) * 0.5 + 0.03),
                     look='C', space=porch, drop=('-z', '+z'))
    m.section('balcony')

    # The walls behind: two storeys of weathered siding, windows on both floors, half of them boarded.
    glass = dict(style='glass', casing='A', panes=(2, 2))
    sides = {'right': [Opening('window', 2.5, 0.9, 0.9, 1.4, **glass),
                       Opening('window', 8.2, 0.9, 0.9, 1.4, style='boarded', boards='planks'),
                       Opening('window', 2.5, UP + 0.8, 0.9, 1.3, style='boarded', boards='planks'),
                       Opening('window', 8.2, UP + 0.8, 0.9, 1.3, **glass)],
             'left': [Opening('window', 3.0, 0.9, 0.9, 1.4, style='boarded', boards='planks'),
                      Opening('window', 6.5, UP + 0.8, 0.9, 1.3, **glass)]}
    back = [Opening('door', 4.9, 0.0, 1.0, 2.15, casing='C', hinge='left'),
            Opening('window', 1.6, 0.95, 0.9, 1.3, style='boarded', boards='planks'),
            Opening('window', 3.45, UP + 0.8, 0.9, 1.3, **glass)]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    town.board_up(m, spaces['back'], 4.9, 0.0, 1.0, 2.15, look='planks', out=0.07)
    # A belt course between the storeys on the side walls.
    for name in ('right', 'left'):
        span = Y1 - YF
        town.trim_board(m, spaces[name], (0.1, 0.0, UP - 0.12), (span - 0.1, 0.0, UP - 0.12), 0.18, 'C', thick=0.05)
    m.section('walls')
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, covering='shakes', overhang=0.28, sag=0.06)
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('roof')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + 6.87, back_over=0.3, overhang=0.28)
    m.hull((5.7 - 1.4, FT + 0.04, 0.4), at=(XF0 + 4.2, Y0 + FT * 0.5, BASE + 6.87 + 0.2))
    m.hull((3.0, FT + 0.04, 0.42), at=(XF0 + 4.2, Y0 + FT * 0.5, BASE + 7.27 + 0.21))
    m.hull((WF, -edge, rail_top + 0.1 - (deck_z - 0.3)), at=(0.0, Y0 + edge * 0.5,
                                                             (rail_top + 0.1 + deck_z - 0.3) * 0.5))
    for x in posts:
        m.hull((0.2, 0.2, deck_z - 0.3), at=(XF0 + x, Y0 + post_y, (deck_z - 0.3) * 0.5))
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


# --- The shop behind Pruitt's display windows ---

SHOP_DEPTH = 0.95           # from the facade's back face to the shop's back wall
SHOP_MARGIN = 0.22          # how far a display runs on past its window at each side (it shows at a slant)
SHELVES = (0.5, 0.95, 1.4)  # the shelves' middles over the display's floor (the window's sill)
SHOP_LAMP = (0.65, 2.25)    # SOCKET_Light behind the doors: depth from the facade's front, height over the floor


def quad(m, space, corner, along, up, length, height, cols, rows, uv, mat):
    """A one-sided grid from corner, length along `along` and height along `up` (unit vectors in space), facing
    along x up; cut into cols x rows for its baked shading, mapped in its own plane."""
    a, u = Vector(along), Vector(up)
    m.emit(kit._sheet(length, height, cols, rows), uv, mat, kit.basis(Vector(corner), a, u.cross(a), u), space)


def turned(m, profile, at, sides, uv, mat, space, phase=0.0, scale=(1.0, 1.0), turn=None):
    """town.lathe with a mapping of its own (town's finishes can't fit a part into a trim strip): profile [(radius, z),
    ...] from bottom to top around a vertical axis through at; a radius of 0 closes it; scale stretches it across
    (x, y), and turn (a matrix) turns it about at (a hat hung on a wall)."""
    tb = kit._new_bmesh()
    angles = [phase + 2.0 * math.pi * k / sides for k in range(sides)]
    rings = [[tb.verts.new((0.0, 0.0, z))] if r <= 1e-6 else
             [tb.verts.new((r * math.cos(a), r * math.sin(a), z)) for a in angles] for r, z in profile]
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            if len(r0) == 1:
                tb.faces.new((r0[0], r1[k1], r1[k]))
            elif len(r1) == 1:
                tb.faces.new((r0[k], r0[k1], r1[0]))
            else:
                tb.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    stretch = Matrix.Diagonal((scale[0], scale[1], 1.0, 1.0))
    m.emit(tb, uv, mat, Matrix.Translation(Vector(at)) @ (turn or Matrix.Identity(4)) @ stretch, space, smooth=True)


def paint(name):
    """(mapping, material key) for the goods: a strip of the trim sheet fitted into it ('H1' teal, 'H2' oxide red, 'G'
    cream, 'C' wood, 'H4' dark glass) or one of the store's own tileable materials ('iron', 'cream', 'planks')."""
    tiles = {'iron': 'MetalWorn', 'cream': 'PaintWorn', 'planks': 'WoodPlanks'}
    if name in tiles:
        return Tile(tiles[name]), name
    return Trim(name, fit=True), 'trim'


def goods_box(m, space, size, at, look, yaw=0.0, drop=('-z', '+y')):
    """A box of goods standing on its foot at `at` (a packet, a crate, a folded bolt of cloth)."""
    spec, key = paint(look)
    m.box(size, at=(at[0], at[1], at[2] + size[2] * 0.5), rot=(0.0, 0.0, yaw), uv=spec, mat=key, space=space, drop=drop)


def shelf_row(m, space, x0, x1, y_back, z, count, room):
    """count goods spread along a shelf from x0 to x1, standing on z with their backs to the wall at y_back, none
    taller than room: packets, stacked boxes, rows of square tins, jars. The gaps between them are random, so the
    lamplit wall shows through in a different rhythm on every shelf."""
    rng = m.rng
    items = []
    for _ in range(count):
        kind = rng.choice(('packet', 'packet', 'packet', 'stack', 'stack', 'tins', 'jars'))
        if kind == 'tins':
            n, s = rng.choice((2, 3, 3)), rng.uniform(0.085, 0.11)
            items.append((kind, n * s + (n - 1) * 0.01, (n, s, min(room, rng.uniform(0.12, 0.18)))))
        elif kind == 'jars':
            r = rng.uniform(0.06, 0.075)
            items.append((kind, 2.0 * r, (r, min(room, rng.uniform(0.18, 0.28)))))
        elif kind == 'packet':
            items.append((kind, rng.uniform(0.15, 0.3), (rng.uniform(0.12, 0.22), min(room, rng.uniform(0.18, 0.33)))))
        else:
            items.append((kind, rng.uniform(0.28, 0.42), (rng.uniform(0.18, 0.25), rng.uniform(0.12, 0.17))))
    while items and sum(w for _, w, _ in items) > (x1 - x0) * 0.8:
        items.pop()
    gaps = [rng.uniform(0.3, 1.7) for _ in range(len(items) + 1)]
    spare = (x1 - x0) - sum(w for _, w, _ in items)
    x = x0 + spare * gaps[0] / sum(gaps)
    for k, (kind, w, size) in enumerate(items):
        if kind == 'tins':
            n, s, h = size
            look = rng.choice(('H2', 'H1', 'iron', 'cream'))
            for i in range(n):
                goods_box(m, space, (s, s, h), (x + s * 0.5 + i * (s + 0.01), y_back - s * 0.5, z), look)
        elif kind == 'jars':
            r, h = size
            spec, key = paint('H4')
            m.cylinder((x + r, y_back - r - 0.01, z), (x + r, y_back - r - 0.01, z + h), r, sides=5, uv=spec, mat=key,
                       space=space, caps=(False, True))
        elif kind == 'packet':
            depth, h = size
            goods_box(m, space, (w, depth, h), (x + w * 0.5, y_back - depth * 0.5, z),
                      rng.choice(('G', 'H1', 'H2', 'C', 'cream')), yaw=rng.uniform(-3.0, 3.0))
        else:
            depth, h = size
            goods_box(m, space, (w, depth, h), (x + w * 0.5, y_back - depth * 0.5, z), rng.choice(('C', 'planks', 'G')))
            goods_box(m, space, (w * 0.8, depth * 0.85, h * 0.9), (x + w * 0.5, y_back - depth * 0.5, z + h),
                      rng.choice(('H1', 'H2', 'G')), yaw=rng.uniform(-6.0, 6.0))
        x += w + spare * gaps[k + 1] / sum(gaps)


def lean(m, space, foot, top, head, head_w, head_look, head_thick=0.02):
    """A long-handled thing leant against a wall (a shovel, a broom): its head from the foot up the handle's line,
    then a round-ish handle (a square stick) on up to top."""
    foot, top = Vector(foot), Vector(top)
    up = (top - foot).normalized()
    neck = foot + up * head
    face = (0.0, -1.0, 0.0)
    town.board(m, foot, neck, head_w, head_thick, look=head_look, space=space, face=face)
    town.board(m, neck, top, 0.034, 0.034, look='C', space=space, face=face, drop=('-x', '+x'))


def dry_goods(m, space, x0, x1, y0, y1, z):
    """The dry goods window: a red calico dress on a stand, bolts of cloth, a crate with a folded blanket on it, a
    broom against the wall."""
    cx, cy = x0 + 0.62, y0 + 0.4
    goods_box(m, space, (0.34, 0.34, 0.04), (cx, cy, z), 'C', drop=('-z',))
    turned(m, [(0.27, 0.12), (0.13, 0.86), (0.15, 1.04), (0.15, 1.2), (0.045, 1.27), (0.035, 1.36), (0.0, 1.37)],
           (cx, cy, z), 6, Trim('H2', fit=True), 'trim', space, scale=(1.25, 0.85))
    bx = x0 + 1.55
    for k, (look, yaw) in enumerate((('H1', 4.0), ('G', -7.0), ('H2', 3.0))):
        goods_box(m, space, (0.62 - 0.05 * k, 0.3, 0.09), (bx + 0.03 * k, y0 + 0.3, z + 0.09 * k), look, yaw=yaw,
                  drop=('-z',))
    kx = x1 - 0.95
    goods_box(m, space, (0.5, 0.36, 0.34), (kx, y0 + 0.4, z), 'planks', yaw=8.0, drop=('-z',))
    goods_box(m, space, (0.42, 0.3, 0.08), (kx + 0.02, y0 + 0.4, z + 0.34), 'H1', yaw=-4.0, drop=('-z',))
    lean(m, space, (x1 - 0.34, y0 + 0.56, z), (x1 - 0.03, y0 + 0.62, z + 1.28), 0.26, 0.24, 'band_G', head_thick=0.06)


def hardware(m, space, x0, x1, y0, y1, z):
    """The feed and hardware window: two sacks of flour, a keg, two shovels against the wall."""
    sack = [(-0.2, 0.0), (0.2, 0.0), (0.23, 0.3), (0.12, 0.55), (-0.13, 0.53), (-0.23, 0.28)]
    # Flour sacks on the plaster strip G where it has no stains (Farmhouse.py's curtains' stretches of it).
    for sx, sy, yaw, s, u in ((x0 + 0.55, y0 + 0.34, -8.0, 0.9, 4.2), (x0 + 1.0, y0 + 0.44, 14.0, 0.8, 5.45)):
        place = (Matrix.Translation((sx, sy, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @
                 Matrix.Diagonal((s, s, s, 1.0)) @ Matrix.Translation((0.0, -0.15, 0.0)))
        m.emit(kit._prism([sack], 0.3), Trim('G', fit=True, u=u), 'trim', place, space)
    kx, ky = x0 + 1.85, y0 + 0.42
    turned(m, [(0.19, 0.0), (0.22, 0.27), (0.19, 0.54), (0.0, 0.54)], (kx, ky, z), 8, Tile('WoodPlanks'), 'planks',
           space, phase=math.pi / 8.0)
    for hz in (0.07, 0.43):
        m.cylinder((kx, ky, z + hz), (kx, ky, z + hz + 0.04), 0.222, sides=8, uv=Tile('MetalWorn'), mat='iron',
                   space=space, caps=(False, False))
    lean(m, space, (x1 - 0.42, y0 + 0.36, z), (x1 - 0.03, y0 + 0.42, z + 1.3), 0.3, 0.22, 'iron')
    lean(m, space, (x1 - 0.55, y0 + 0.62, z), (x1 - 0.03, y0 + 0.7, z + 1.18), 0.26, 0.2, 'iron')


def door_blinds(m, space, o, drawn=0.42):
    """Linen blinds drawn part way down the lamplit panes of double door o (the plaster strip G where it has no chips,
    as Farmhouse.py's curtains take it), each weighted by a wooden slat, 6 mm in front of the glass (the panes laid out
    as town.door lays out a framed leaf's: one pane in the upper half of a leaf up to 0.7 m wide), so the doors read as
    a shop's by day and their panes glow under the blinds at dusk."""
    leaf_h = o.h - o.opts['transom'] - 0.06
    lw = o.w * 0.5
    stile = 0.12 if lw > 0.7 else 0.09
    pane_w = lw - 2.0 * stile - 0.06
    z_lo, z_hi = leaf_h * 0.42 + 0.08, leaf_h - 0.15
    z_b = z_hi - (z_hi - z_lo) * drawn
    y = FT * 0.3 - 0.015 - 0.006            # the glass's face, less 6 mm
    for lx, u in ((o.x, 4.3), (o.x + lw, 5.5)):
        px = lx + stile + 0.03 + pane_w * 0.5
        quad(m, space, (px - pane_w * 0.5, y, z_b), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), pane_w, z_hi - z_b, 1, 1,
             Trim('G', fit=True, u=u), 'trim')
        m.box((pane_w + 0.01, 0.01, 0.025), at=(px, y - 0.006, z_b), uv=Trim('C', fit=True), space=space, drop=('+y',))


def shop_back(o):
    """Where display window o's pane goes (a move for Refit.glazing, in the facade's space): back to the shop's back
    wall, stretched across the display. Its face, as town.window builds it, is 1 cm in front of FT * 0.55."""
    cx = o.x + o.w * 0.5
    return (Matrix.Translation((cx, FT + SHOP_DEPTH - (FT * 0.55 - 0.01), 0.0)) @
            Matrix.Diagonal(((o.w + 2.0 * SHOP_MARGIN) / o.w, 1.0, 1.0, 1.0)) @ Matrix.Translation((-cx, 0.0, 0.0)))


def shop_window(m, space, o, goods):
    """The shop seen through display window o (in the facade's space): a shallow box behind the facade, one-sided
    (nothing sees it from outside): its back wall the window's pane, lamplit (moved there by shop_back(): dark warm by
    day, glowing at dusk), its sides boards, a plank floor at the window's sill and a board ceiling at its head; shelves
    of goods along the back wall, and goods(m, space, x0, x1, y0, y1, z) on the floor in front of them."""
    x0, x1 = o.x - SHOP_MARGIN, o.x + o.w + SHOP_MARGIN
    y0, y1 = FT, FT + SHOP_DEPTH
    z0, z1 = o.z, o.z + o.h
    w, d, h = x1 - x0, y1 - y0, z1 - z0
    quad(m, space, (x0, y0, z0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), d, h, 1, 2, Trim('A', world=True), 'trim')
    quad(m, space, (x1, y1, z0), (0.0, -1.0, 0.0), (0.0, 0.0, 1.0), d, h, 1, 2, Trim('A', world=True), 'trim')
    quad(m, space, (x0, y0, z0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), w, d, 4, 1, Tile('WoodPlanks'), 'planks')
    quad(m, space, (x0, y1, z1), (1.0, 0.0, 0.0), (0.0, -1.0, 0.0), w, d, 2, 1, Trim('A', world=True), 'trim')
    # The shelves: three uprights (one each end, one in the middle) and boards across, the goods on them.
    depth = 0.3
    yc = y1 - depth * 0.5
    for x in (x0 + 0.08, (x0 + x1) * 0.5, x1 - 0.08):
        town.board(m, (x, yc, z0), (x, yc, z1), depth, 0.05, look='C', space=space, face=(-1.0, 0.0, 0.0),
                   drop=('-x', '+x', '+z'))
    for k, sz in enumerate(SHELVES):
        town.board(m, (x0, yc, z0 + sz), (x1, yc, z0 + sz), depth, 0.035, look='C', space=space, face=(0.0, 0.0, 1.0),
                   drop=('-x', '+x', '+z'))
        room = (SHELVES[k + 1] - sz if k + 1 < len(SHELVES) else z1 - z0 - sz) - 0.12
        for a, b in ((x0 + 0.11, (x0 + x1) * 0.5 - 0.03), ((x0 + x1) * 0.5 + 0.03, x1 - 0.11)):
            shelf_row(m, space, a, b, y1 - 0.012, z0 + sz + 0.0175, 2, room)
    goods(m, space, x0, x1, y0, y1, z0)


# --- Pruitt's General Store ---

def store(shutters):
    m = Refit('FalseFront_Store', seed=303)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -4.7, 4.7, -6.5, 6.5
    X0, X1 = -4.4, 4.4
    YF = Y0 + FT
    WALL, PITCH = 3.8, 24.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    TOP = 6.45
    # The widest front on the street: flat, under a heavy bracketed cornice, a low pediment in the middle.
    top = [(0.0, TOP), (3.5, TOP), (WF * 0.5, TOP + 0.5), (WF - 3.5, TOP), (WF, TOP)]
    display = dict(style='glass', casing='H1', sash='H1', panes=(3, 2), casing_w=0.14)
    LEFT = Opening('window', 0.62, 0.6, 2.85, 2.0, **display)
    RIGHT = Opening('window', WF - 0.62 - 2.85, 0.6, 2.85, 2.0, **display)
    DOORS = Opening('door', WF * 0.5 - 0.65, 0.0, 1.3, 2.85, style='double', finish='planks', casing='H1', knob='H3',
                    transom=0.4, glass=True)
    items = [LEFT, RIGHT, DOORS]
    front = false_front(m, XF0, XF1, Y0, top, items)
    m.section('facade panel')
    # The doors' panes are lamplit; the display windows' go back to be the shop's lamplit back wall (shop_window()).
    for o in items:
        with m.glazing(('windowglow', shop_back(o)) if o.kind == 'window' else 'windowglow'):
            town.openings(m, front, [o], FT)
    # Bulkheads under the display windows: two teal boards each, and pilasters at the corners.
    for o in (LEFT, RIGHT):
        for z in (0.16, 0.38):
            town.board(m, (o.x - 0.05, 0.0, z), (o.x + o.w + 0.05, 0.0, z), 0.2, 0.04, look='H1', space=front, lift=0.02)
    for x in (0.15, WF - 0.15):
        town.pilaster(m, front, x, 0.0, 3.25, width=0.28, look='C', depth=0.06)
    m.section('openings')

    # The sign: weathered boards in a teal frame, cream letters.
    sx0, sx1, sz0, sz1 = 0.55, WF - 0.55, 3.6, 5.24
    rows = 8
    for k in range(rows):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / rows
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / rows - 0.005, 0.035, look='A', space=front,
                   lift=0.0175)
    field_y = -0.035
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.1, field_y, z), (sx1 + 0.1, field_y, z), 0.14, 0.05, look='H1', space=front, lift=0.025)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, field_y, sz0 + 0.07), (x, field_y, sz1 - 0.07), 0.14, 0.05, look='H1', space=front,
                   lift=0.025)
    town.text(m, "PRUITT'S", (WF * 0.5, field_y, 4.74), height=0.62, width=6.5, space=front, look='cream', tol=0.028)
    town.text(m, 'GENERAL STORE', (WF * 0.5, field_y, 3.98), height=0.34, width=6.8, space=front, look='cream',
              tol=0.028)
    town.cornice(m, front, 0.0, WF, TOP, look='C', project=0.32, height=0.3, brackets=1.05, frieze=0.3)
    town.cap_profile(m, front, top[1:4], depth=0.2, height=0.12, look='C', back=FT + 0.05)
    m.section('sign, cornice')

    # The deep porch: a plank deck like the boardwalk's (it chains with it), four posts with knee braces, a beam and a
    # shake roof running back to the facade under the sign.
    deck_front = Y0 - town.WALK_DEPTH
    town.deck(m, XF0, XF1, deck_front, Y0, posts=[XF0 + 1.0 + 2.0 * k for k in range(5)], back=False, plank_cuts=0)
    porch = kit.wall_space((XF0, Y0), (XF1, Y0), 0.0)
    post_y = -town.WALK_DEPTH + 0.18
    beam_z = 3.08
    posts = [0.22, 3.25, WF - 3.25, WF - 0.22]
    for x in posts:
        town.board(m, (x, post_y, town.DECK_TOP), (x, post_y, beam_z), 0.16, 0.16, look='C', space=porch, bevel=0.015)
        town.box(m, (0.22, 0.22, 0.05), at=(x, post_y, town.DECK_TOP + 0.025), look='C', space=porch)
        for side in (-1.0, 1.0):
            if 0.3 < x + side * 0.5 < WF - 0.3:
                town.board(m, (x + side * 0.07, post_y, beam_z - 0.55), (x + side * 0.5, post_y, beam_z - 0.05), 0.1,
                           0.09, look='C', space=porch)
    town.board(m, (-0.1, post_y, beam_z + 0.11), (WF + 0.1, post_y, beam_z + 0.11), 0.22, 0.18, look='C', space=porch)
    wall_z = BASE + 3.42
    theta = math.atan2(wall_z - (beam_z + 0.22), -post_y)
    p_deck, p_over = 0.06, 0.34
    normal = Vector((0.0, -math.sin(theta), math.cos(theta)))
    origin = Vector((XF0 - 0.12, Y0 + post_y - p_over, beam_z + 0.22 - p_over * math.tan(theta))) + normal * p_deck
    p_len = (-post_y + p_over) / math.cos(theta)
    porch_roof = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), WF + 0.24, p_len, sag=0.05)
    town.roof_deck(m, porch_roof, p_deck)
    town.shakes(m, porch_roof, piece=(4.5, 6.0), top=p_len - 0.03)
    town.board(m, (XF0 - 0.12, Y0 - 0.03, wall_z + 0.06), (XF1 + 0.12, Y0 - 0.03, wall_z + 0.06), 0.16, 0.05, look='C')
    fascia_z = beam_z + 0.22 - p_over * math.tan(theta) - 0.02
    town.board(m, (XF0 - 0.14, Y0 + post_y - p_over - 0.02, fascia_z), (XF1 + 0.14, Y0 + post_y - p_over - 0.02, fascia_z),
               0.2, 0.04, look='H1')
    m.section('porch')

    # The walls behind: windows down both sides with shutters (their sockets and pintles), a back door.
    side = dict(style='glass', casing='H1', sash='H1', panes=(2, 2))
    sides = {'right': [Opening('window', 2.6, 1.0, 0.74, 1.3, **side), Opening('window', 8.4, 1.0, 0.74, 1.3, **side)],
             'left': [Opening('window', 2.6, 1.0, 0.74, 1.3, **side), Opening('window', 8.4, 1.0, 0.74, 1.3, **side)]}
    BACK_DOOR = Opening('door', 5.6, 0.0, 1.05, 2.2, casing='C', hinge='left')
    back = [BACK_DOOR, Opening('window', 1.6, 1.0, 0.9, 1.2, style='boarded', casing='A')]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    number = 1
    for name in ('left', 'right'):
        for o in sorted(spaces[name].items, key=lambda o: o.x):
            shutter_sockets(m, spaces[name], o, number)
            number += 1
    bs = spaces['back']
    town.box(m, (BACK_DOOR.w + 0.5, 0.42, BASE * 0.5), at=(BACK_DOOR.x + BACK_DOOR.w * 0.5, -0.21, -BASE * 0.75),
             look='world_D', space=bs, bevel=0.02)
    m.section('walls')

    # The roof, and a stovepipe through it near the back with a rain cap.
    PIPE = (2.4, 3.9)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, overhang=0.28, sag=0.06,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    pipe_base = roof_z(PIPE[0]) - 0.35
    pipe_top = roof_z(PIPE[0]) + 1.25
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_base), (PIPE[0], PIPE[1], pipe_top), 0.1, sides=10, look='iron',
                  caps=(False, True))
    town.cylinder(m, (PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.12), (PIPE[0], PIPE[1], roof_z(PIPE[0]) + 0.1), 0.22, 0.11,
                  sides=10, look='iron', caps=(False, False))
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_top + 0.12), (PIPE[0], PIPE[1], pipe_top + 0.3), 0.26, 0.03, sides=10,
                  look='iron')
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        town.box(m, (0.02, 0.02, 0.16), at=(PIPE[0] + 0.09 * math.cos(a), PIPE[1] + 0.09 * math.sin(a), pipe_top + 0.06),
                 look='iron')
    m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.08))
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('roof')

    # The shop behind the display windows, built last on a stream of its own (the rest stays as it was), and the shop's
    # lamp in the middle behind the doors, high, where it reaches into both windows.
    with m.own(313):
        shop_window(m, front, LEFT, dry_goods)
        shop_window(m, front, RIGHT, hardware)
        door_blinds(m, front, DOORS)
    m.socket('Light', front.world((WF * 0.5, SHOP_LAMP[0], SHOP_LAMP[1])))
    m.section('shop windows')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + TOP, back_over=0.3, overhang=0.28)
    m.hull_points([(XF0 + x, y, BASE + z + 0.12) for x, z in top[1:4] for y in (Y0 - 0.1, Y0 + FT)] +
                  [(XF0 + x, y, BASE + TOP - 0.1) for x in (3.5, WF - 3.5) for y in (Y0 - 0.1, Y0 + FT)])
    m.hull((WF, town.WALK_DEPTH, town.DECK_TOP), at=(0.0, Y0 - town.WALK_DEPTH * 0.5, town.DECK_TOP * 0.5))
    for x in posts:
        m.hull((0.18, 0.18, beam_z - town.DECK_TOP), at=(XF0 + x, Y0 + post_y, (beam_z + town.DECK_TOP) * 0.5))
    m.hull((porch_roof.width, p_len, 0.2), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)
    obj = m.finish(preview=False, fallback=FALLBACK)
    if lt.want_preview():
        # Seen with its shutters hung: some swung open against the wall, some shut.
        copies = mount_shutters(obj, shutters)
        lt.preview([obj] + copies, lt.preview_path('Buildings', m.name), view=(-1.0, -1.7, 0.42), fit=0.78)
        for copy in copies:
            kit.bpy.data.objects.remove(copy)
    return obj


# --- The sheriff's office ---

# The walk-in front office (Docs/Areas/RansomsRest.md, Side 1: the gang's Strongbox is here), in the model's frame.
OFFICE_DEPTH = 3.04         # from the front wall's inside to the partition, clear of the right wall's cell window
OFFICE_CEILING = 2.95       # over the floor (the stone storey stands 3.3)
STRONGBOX = (0.25, 0.5)     # SOCKET_Strongbox: across from the middle, and this far off the partition (the box's
                            # back hinges stand 0.36 behind its pivot and its lid swings 12 cm past them)
CELL_DOOR = (-1.95, 0.9, 2.1)   # the cell door in the partition: its hinge side (x), width, height
# The front door's leaf as town.door builds a plank leaf (parts in order): its five boards, two battens and brace (0-7),
# the two straps (8, 10; the pintles on the wall, 9 and 11, stay), the latch (12, 13); then the casing and threshold.
DOOR_LEAF = frozenset(range(9)) | {10, 12, 13}


def wall_face(m, space, outline, holes, uv, mat='trim'):
    """A wall's face alone, holes and all (the office's plaster, inside walls nobody sees from behind): outline (x, z)
    counterclockwise seen from the front in space, which faces -Y there."""
    tb = kit._prism([list(outline)] + [list(h) for h in holes], 0.01)
    tb.normal_update()
    bmesh.ops.delete(tb, geom=[f for f in tb.faces if f.normal.y > -0.99], context='FACES')
    m.emit(tb, uv, mat, None, space)


def sheriff_office(m, x_in, y_in, y_p):
    """The front office behind the sheriff's door, a week after the raid (built in the model's frame): plastered walls
    and partition over a board floor under a board ceiling on two joists; through the partition a barred cell door
    standing open on a dark cell (its back wall and ceiling are the front windows' panes, moved there); the
    sheriff's desk with his lamp still lit, black crepe over it and papers on the floor, his chair knocked over
    behind it; an empty gun rack; two blank bills on the wall; his hat on a peg by the door. The floor where the
    Strongbox goes stays clear. x_in: the side walls' inside; y_in, y_p: the front wall's inside and the partition's
    face. Returns the places the sockets and hulls need."""
    plaster = Trim('G', world=True, rotate=True, u=5.3)  # its grain upright, from a clean stretch: few chips
    boards = Trim('A', world=True)
    top = OFFICE_CEILING
    depth = y_p - y_in
    up = (0.0, 0.0, 1.0)
    # Floor (on under the cell doorway) and ceiling, two joists across.
    quad(m, None, (-x_in, y_in, BASE), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), 2.0 * x_in, depth + 0.12, 4, 2, boards, 'trim')
    quad(m, None, (-x_in, y_p, BASE + top), (1.0, 0.0, 0.0), (0.0, -1.0, 0.0), 2.0 * x_in, depth, 2, 2, boards, 'trim')
    for y in (y_in + 0.75, y_in + 2.25):
        town.board(m, (-x_in, y, BASE + top - 0.09), (x_in, y, BASE + top - 0.09), 0.18, 0.14, look='C',
                   drop=('-x', '+x', '+z'))
    # The plaster: the front wall's inside round the door and windows, the side walls (the right one round its cell
    # window, 2.3 m from the front wall's inside), the partition round the cell door, with its doorway's reveals.
    front = kit.wall_space((x_in, y_in), (-x_in, y_in), BASE)
    openings = [(-0.5, 0.5, 0.0, 2.25), (-2.3, -1.4, 0.95, 2.15), (1.4, 2.3, 0.95, 2.15)]
    wall_face(m, front, [(0.0, -0.02), (2.0 * x_in, -0.02), (2.0 * x_in, top), (0.0, top)],
              [kit.rect(x_in - b, z0, b - a, z1 - z0) for a, b, z0, z1 in openings], plaster)
    left = kit.wall_space((-x_in, y_in), (-x_in, y_p), BASE)
    quad(m, left, (0.0, 0.0, -0.02), (1.0, 0.0, 0.0), up, depth, top + 0.02, 2, 1, plaster, 'trim')
    right = kit.wall_space((x_in, y_p), (x_in, y_in), BASE)
    wall_face(m, right, [(0.0, -0.02), (depth, -0.02), (depth, top), (0.0, top)],
              [kit.rect(y_p - (y_in + 3.0), 1.55, 0.7, 0.55)], plaster)
    partition = kit.wall_space((-x_in, y_p), (x_in, y_p), BASE)
    cx, cw, ch = CELL_DOOR
    wall_face(m, partition, [(0.0, -0.02), (2.0 * x_in, -0.02), (2.0 * x_in, top), (0.0, top)],
              [kit.rect(cx + x_in, 0.0, cw, ch)], plaster)
    quad(m, None, (cx, y_p, BASE), (0.0, 1.0, 0.0), up, 0.12, ch, 1, 1, plaster, 'trim')
    quad(m, None, (cx + cw, y_p + 0.12, BASE), (0.0, -1.0, 0.0), up, 0.12, ch, 1, 1, plaster, 'trim')
    quad(m, None, (cx, y_p + 0.12, BASE + ch), (1.0, 0.0, 0.0), (0.0, -1.0, 0.0), cw, 0.12, 1, 1, plaster, 'trim')
    m.section('office: room')

    # The cell: stone sides and floor round the moved panes, a plank bunk against the back; its door open.
    stone = Trim('D', world=True)
    c0, c1, cy0, cy1, ctop = cx - 0.25, cx + cw + 0.25, y_p + 0.12, y_p + 1.22, 2.4
    quad(m, None, (c0, cy0, BASE), (0.0, 1.0, 0.0), up, cy1 - cy0, ctop, 1, 1, stone, 'trim')
    quad(m, None, (c1, cy1, BASE), (0.0, -1.0, 0.0), up, cy1 - cy0, ctop, 1, 1, stone, 'trim')
    quad(m, None, (c0, cy0, BASE), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), c1 - c0, cy1 - cy0, 1, 1, stone, 'trim')
    town.box(m, (c1 - c0 - 0.2, 0.5, 0.06), at=((c0 + c1) * 0.5, cy1 - 0.26, BASE + 0.45), look='C', drop=('+y',))
    cell_door = kit.Space(Matrix.Translation((cx, y_p - 0.03, BASE)) @ Matrix.Rotation(math.radians(-100.0), 4, 'Z'))
    for x in (0.03, cw - 0.03):
        town.board(m, (x, 0.0, 0.02), (x, 0.0, ch - 0.04), 0.05, 0.025, look='iron', space=cell_door,
                   drop=('-x', '+x'))
    for z in (0.1, 1.05, ch - 0.12):
        town.board(m, (0.0, 0.0, z), (cw, 0.0, z), 0.06, 0.025, look='iron', space=cell_door, drop=('-x', '+x'))
    for k in range(4):
        x = 0.21 + 0.18 * k
        town.board(m, (x, 0.0, 0.1), (x, 0.0, ch - 0.12), 0.022, 0.022, look='iron', space=cell_door,
                   drop=('-x', '+x'))
    town.box(m, (0.09, 0.06, 0.13), at=(cw - 0.05, 0.0, 1.05), look='iron', space=cell_door)
    m.section('office: cell')

    # The desk, toward the right window, facing the door; the lamp on it, still lit; crepe over its front edge.
    dx, dy, dz = 1.65, y_in + 2.08, BASE + 0.76
    town.box(m, (1.36, 0.7, 0.04), at=(dx, dy, dz - 0.02), look='C', drop=('-z',))
    for s in (-1.0, 1.0):
        town.box(m, (0.36, 0.62, 0.72), at=(dx + s * 0.47, dy, BASE + 0.36), look='C', drop=('-z',))
    town.box(m, (0.58, 0.02, 0.5), at=(dx, dy - 0.3, BASE + 0.47), look='C', drop=('-z',))
    lamp = (dx + 0.42, dy + 0.1)                 # toward the right window, seen through it
    town.cylinder(m, (lamp[0], lamp[1], dz), (lamp[0], lamp[1], dz + 0.1), 0.065, 0.045, sides=6, look='iron',
                  caps=(False, True))
    turned(m, [(0.032, 0.0), (0.058, 0.07), (0.03, 0.17), (0.0, 0.17)], (lamp[0], lamp[1], dz + 0.1), 6,
           Trim('H4', fit=True), 'glow', None)
    light = (lamp[0], lamp[1], dz + 0.18)
    sash = [(dx - 0.36, dy + 0.18, dz + 0.004), (dx - 0.22, dy - 0.351, dz + 0.004),
            (dx - 0.21, dy - 0.365, dz - 0.05), (dx - 0.19, dy - 0.37, dz - 0.34)]
    town.sheet(m, [[(x - 0.08, y, z), (x + 0.08, y, z)] for x, y, z in sash], look='crepe')
    # Papers swept off it onto the floor; the chair knocked over on its side behind it.
    for (px, py), a, u in (((dx - 0.75, dy - 0.62), 18.0, 4.4), ((dx - 0.45, dy - 0.86), -31.0, 5.6)):
        r = math.radians(a)
        quad(m, None, (px, py, BASE + 0.003), (math.cos(r), math.sin(r), 0.0), (-math.sin(r), math.cos(r), 0.0),
             0.22, 0.29, 1, 1, Trim('G', fit=True, u=u), 'trim')
    chair = kit.Space(Matrix.Translation((dx - 0.6, dy + 0.66, BASE + 0.2175)) @
                      Matrix.Rotation(math.radians(-5.0), 4, 'Z') @ Matrix.Rotation(math.radians(90.0), 4, 'Y'))
    town.box(m, (0.42, 0.42, 0.04), at=(0.0, 0.0, 0.45), look='C', space=chair)
    town.box(m, (0.42, 0.03, 0.42), at=(0.0, 0.195, 0.69), look='C', space=chair)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            town.board(m, (sx * 0.18, sy * 0.18, 0.0), (sx * 0.18, sy * 0.18, 0.43), 0.035, 0.035, look='C',
                       space=chair, drop=('-x', '+x'))
    m.section('office: desk')

    # The gun rack on the left wall, empty: a back board, a butt rest, a rail with pegs between the guns that were.
    rack_x = 0.95
    town.box(m, (1.0, 0.03, 0.78), at=(rack_x, -0.015, 1.22), look='C', space=left, drop=('+y',))
    town.box(m, (1.0, 0.16, 0.05), at=(rack_x, -0.08, 0.78), look='C', space=left, drop=('+y',))
    town.box(m, (1.0, 0.1, 0.07), at=(rack_x, -0.05, 1.62), look='C', space=left, drop=('+y',))
    for k in range(4):
        town.box(m, (0.035, 0.08, 0.09), at=(rack_x - 0.36 + 0.24 * k, -0.14, 1.66), look='C', space=left,
                 drop=('+y', '-z'))
    # Two blank bills on the right wall over the desk; his hat on a peg left of the door.
    for lx, z, a, u in ((1.05, 1.42, -3.0, 4.3), (1.6, 1.5, 4.0, 5.5)):
        r = math.radians(a)
        quad(m, right, (lx, -0.002, z), (math.cos(r), 0.0, math.sin(r)), (-math.sin(r), 0.0, math.cos(r)), 0.42,
             0.56, 1, 1, Trim('G', fit=True, u=u), 'trim')
    town.cylinder(m, (-0.95, y_in, BASE + 1.72), (-0.95, y_in + 0.11, BASE + 1.74), 0.014, sides=4, look='C',
                  caps=(False, True), face=(0.0, 0.0, 1.0))
    turned(m, [(0.19, 0.0), (0.19, 0.012), (0.085, 0.03), (0.08, 0.13), (0.0, 0.12)], (-0.95, y_in + 0.05, BASE + 1.6),
           6, Tile('Polymer'), 'crepe', None, turn=Matrix.Rotation(math.radians(-78.0), 4, 'X'))
    m.section('office: rack, bills, hat')
    return dict(light=light, desk=(dx, dy, dz), cell_door=cell_door)


def sheriff():
    m = Refit('FalseFront_Sheriff', seed=304)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -3.3, 3.3, -5.0, 5.0       # the wooden false front
    X0, X1 = -3.05, 3.05                          # the stone storey and walls
    STONE_T = 0.3
    WALL, PITCH = 3.3, 32.0
    EAVE = BASE + WALL
    WF, WS = XF1 - XF0, X1 - X0
    FACADE_Y[m.name] = Y0
    LOW = WALL                                    # the stone storey's top, over the floor
    top = [(0.0, 5.85), (2.2, 5.85), (2.2, 6.3), (WF - 2.2, 6.3), (WF - 2.2, 5.85), (WF, 5.85)]
    X_IN, Y_IN = X1 - THICK, Y0 + STONE_T         # the office: inside the side walls, behind the front wall
    Y_P = Y_IN + OFFICE_DEPTH                     # the partition's face
    # The stone storey: a jail's walls, stone jambs and lintels round a heavy plank door and two barred windows.
    stone_front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    DOOR = Opening('door', WS * 0.5 - 0.5, 0.0, 1.0, 2.25, style='plank', finish='A', casing='D', hinge='right',
                   casing_w=0.18)
    barred = dict(style='barred', casing='D', panes=(1, 1), bars=6, casing_w=0.16)
    WIN_L = Opening('window', 0.75, 0.95, 0.9, 1.2, **barred)
    WIN_R = Opening('window', WS - 0.75 - 0.9, 0.95, 0.9, 1.2, **barred)
    items = [DOOR, WIN_L, WIN_R]
    outline = [(0.0, -BASE), (WS, -BASE), (WS, LOW), (0.0, LOW)]
    town.panel(m, outline, [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items], STONE_T,
               Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space=stone_front, around=items, slices=0, back=False)
    # The door stands open: its leaf swung in on its hinge side right round against the wall's inside face (rehung
    # there: the leaf's back, 2.5 cm behind the middle of the wall, 3 mm off the plaster), clear of the doorway and of
    # the desk seen through it; the pintles stay on the wall. The windows' panes go into the cell behind the office
    # (the left one its back wall, the right one its ceiling), so the windows show the office through their bars.
    hinge = DOOR.x + DOOR.w
    leaf_back = STONE_T * 0.3 + 0.025
    swing = (Matrix.Translation((hinge, STONE_T + 0.003, 0.0)) @ Matrix.Rotation(math.radians(180.0), 4, 'Z') @
             Matrix.Translation((-hinge, -leaf_back, 0.0)))
    cell = (CELL_DOOR[0] - 0.25 - X0, CELL_DOOR[0] + CELL_DOOR[1] + 0.25 - X0,
            Y_P + 0.12 - Y0, Y_P + 1.22 - Y0)                    # the cell's plan in the stone front's space
    pane_face = STONE_T * 0.55 - 0.01                            # town.window's pane face, inside the wall
    cell_w, cell_d = cell[1] - cell[0], cell[3] - cell[2]
    panes = {}
    for o in (WIN_L, WIN_R):
        middle = Matrix.Translation((-(o.x + o.w * 0.5), -pane_face, -(o.z + o.h * 0.5)))
        if o is WIN_L:
            panes[o] = (Matrix.Translation(((cell[0] + cell[1]) * 0.5, cell[3], 1.2)) @
                        Matrix.Diagonal((cell_w / o.w, 1.0, 2.4 / o.h, 1.0)) @ middle)
        else:
            panes[o] = (Matrix.Translation(((cell[0] + cell[1]) * 0.5, (cell[2] + cell[3]) * 0.5, 2.4)) @
                        Matrix.Diagonal((cell_w / o.w, cell_d / o.h, 1.0, 1.0)) @
                        Matrix.Rotation(math.radians(90.0), 4, 'X') @ middle)
    for o in items:
        if o is DOOR:
            with m.moved(swing, DOOR_LEAF):
                town.openings(m, stone_front, [o], STONE_T)
        else:
            with m.glazing(('trim', panes[o])):
                town.openings(m, stone_front, [o], STONE_T)
    # Quoins: dressed blocks up both corners, long and short in turn, wrapping round onto the side walls.
    for x_edge, sign in ((0.0, 1.0), (WS, -1.0)):
        z, k = -BASE + 0.05, 0
        while z < LOW - 0.12:
            h = min(0.34 if k % 2 == 0 else 0.26, LOW - z)
            length = 0.5 if k % 2 == 0 else 0.32
            town.box(m, (length, 0.05, h - 0.025), at=(x_edge + sign * (length * 0.5 - 0.02), -0.025, z + h * 0.5),
                     look='D', space=stone_front)
            town.box(m, (0.05, 0.32 if k % 2 == 0 else 0.5, h - 0.025),
                     at=(x_edge - sign * 0.025, (0.32 if k % 2 == 0 else 0.5) * 0.5 - 0.04, z + h * 0.5), look='D',
                     space=stone_front)
            z += h
            k += 1
    # Black crepe on the door (a bow and tails, gone in with it), and a swag over the lintel.
    with m.moved(swing):
        town.crepe_bow(m, stone_front, (DOOR.x + DOOR.w * 0.5, 0.0, 1.78), scale=1.1, tails=0.7,
                       y=STONE_T * 0.3 - 0.07)
    town.crepe_swag(m, stone_front, DOOR.x - 0.3, DOOR.x + DOOR.w + 0.3, DOOR.h + 0.34, sag=0.15, y=-0.1)
    m.section('stone storey')

    # The wooden false front on the stone: siding, the sign, a star in the raised middle, a molded cornice.
    upper = kit.wall_space((XF0, Y0), (XF1, Y0), BASE + LOW)
    up_top = [(x, z - LOW) for x, z in top]
    town.panel(m, [(0.0, 0.0), (WF, 0.0)] + list(reversed(up_top)), (), FT, siding(rng), space=upper, slices=0)
    town.board(m, (-0.04, 0.0, 0.03), (WF + 0.04, 0.0, 0.03), 0.22, 0.09, look='C', space=upper, lift=0.045)
    for x in (0.1, WF - 0.1):
        town.trim_board(m, upper, (x, 0.0, 0.14), (x, 0.0, up_top[0][1] - 0.3), 0.2, 'C', thick=0.05)
    sx0, sx1, sz0, sz1 = 1.0, WF - 1.0, 0.42, 1.26
    for k in range(4):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / 4
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / 4 - 0.005, 0.035, look='H1', space=upper,
                   lift=0.0175)
    field_y = -0.035
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.09, field_y, z), (sx1 + 0.09, field_y, z), 0.12, 0.05, look='C', space=upper, lift=0.025)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, field_y, sz0 + 0.06), (x, field_y, sz1 - 0.06), 0.12, 0.05, look='C', space=upper, lift=0.025)
    town.text(m, 'SHERIFF', (WF * 0.5, field_y, (sz0 + sz1) * 0.5), height=0.46, width=3.9, space=upper, look='cream',
              tol=0.028)
    star = []
    for k in range(10):
        r = 0.21 if k % 2 == 0 else 0.085
        a = math.pi * 0.5 + math.pi * k / 5
        star.append((WF * 0.5 + r * math.cos(a), 2.62 + r * math.sin(a)))
    town.prism(m, star, 0.03, look='cream', space=upper, y=-0.03)
    town.cornice(m, upper, 0.0, WF, up_top[0][1], look='C', project=0.24, height=0.22, brackets=0, frieze=0.2)
    town.cap_profile(m, upper, up_top[1:5], depth=0.18, height=0.1, look='C', back=FT + 0.04)
    m.section('false front')

    # Stone walls behind, small barred cell windows, an iron-strapped back door; a shake roof and a stovepipe.
    cell = dict(style='barred', casing='D', panes=(1, 1), bars=4, casing_w=0.12)
    sides = {'right': [Opening('window', 2.3, 1.55, 0.7, 0.55, **cell), Opening('window', 6.4, 1.55, 0.7, 0.55, **cell)],
             'left': [Opening('window', 4.4, 1.55, 0.7, 0.55, **cell)]}
    back = [Opening('door', 3.6, 0.0, 1.0, 2.15, style='plank', finish='A', casing='D', hinge='left'),
            Opening('window', 1.0, 1.5, 0.7, 0.55, **cell)]
    shell(m, X0, X1, Y0 + STONE_T, Y1, WALL, PITCH, sides, back, stone=True)
    m.section('walls')
    PIPE = (-1.3, 2.9)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, overhang=0.3, sag=0.05, rake=0.3,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    pipe_top = roof_z(PIPE[0]) + 1.1
    town.cylinder(m, (PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.35), (PIPE[0], PIPE[1], pipe_top), 0.09, sides=10,
                  look='iron', caps=(False, True))
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_top + 0.1), (PIPE[0], PIPE[1], pipe_top + 0.26), 0.22, 0.03, sides=10,
                  look='iron')
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        town.box(m, (0.02, 0.02, 0.14), at=(PIPE[0] + 0.08 * math.cos(a), PIPE[1] + 0.08 * math.sin(a), pipe_top + 0.05),
                 look='iron')
    m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.07))
    town.facade_back(m, upper, WF, up_top, lambda x: roof_z(XF0 + x) - BASE - LOW, FT)
    m.section('roof')

    # The office, built last on a stream of its own (the rest stays as it was), and what the game hangs on it.
    with m.own(305):
        office = sheriff_office(m, X_IN, Y_IN, Y_P)
    m.socket('Light', office['light'])
    m.socket('Strongbox', (STRONGBOX[0], Y_P - STRONGBOX[1], BASE))
    m.socket('Decal', (2.2, Y_P - 0.001, BASE + 1.6))
    m.socket('Decal_2', (-X_IN + 0.001, Y_P - 0.7, BASE + 1.6), (0.0, 0.0, 90.0))
    m.section('office')

    # Collision: the office walkable, its floor at the walk's height on out through the doorway (1 x 2.25 m clear),
    # the front wall either side and over the door, the side walls, the ceiling, the desk and the open cell door; the
    # rest of the building solid behind the partition.
    mid = (Y_IN + Y_P) * 0.5
    m.hull((2.0 * X_IN, OFFICE_DEPTH, BASE), at=(0.0, mid, BASE * 0.5))
    m.hull((DOOR.w, STONE_T, BASE), at=(X0 + DOOR.x + DOOR.w * 0.5, Y0 + STONE_T * 0.5, BASE * 0.5))
    for a, b in ((X0 - 0.05, X0 + DOOR.x), (X0 + DOOR.x + DOOR.w, X1 + 0.05)):
        m.hull((b - a, STONE_T, EAVE), at=((a + b) * 0.5, Y0 + STONE_T * 0.5, EAVE * 0.5))
    m.hull((DOOR.w, STONE_T, EAVE - BASE - DOOR.h), at=(X0 + DOOR.x + DOOR.w * 0.5, Y0 + STONE_T * 0.5,
                                                        (EAVE + BASE + DOOR.h) * 0.5))
    for s in (-1.0, 1.0):
        m.hull((X1 + 0.05 - X_IN, OFFICE_DEPTH, EAVE), at=(s * (X_IN + X1 + 0.05) * 0.5, mid, EAVE * 0.5))
    ceiling = BASE + OFFICE_CEILING
    m.hull((2.0 * X_IN, OFFICE_DEPTH, EAVE - ceiling), at=(0.0, mid, (EAVE + ceiling) * 0.5))
    m.hull((WS + 0.1, Y1 - Y_P, EAVE), at=(0.0, (Y_P + Y1) * 0.5, EAVE * 0.5))
    dx, dy, dz = office['desk']
    m.hull((1.36, 0.7, dz - BASE), at=(dx, dy, (dz + BASE) * 0.5))
    m.hull((CELL_DOOR[1], 0.05, CELL_DOOR[2]), at=(CELL_DOOR[1] * 0.5, 0.0, CELL_DOOR[2] * 0.5),
           space=office['cell_door'])
    m.hull((WF, FT + 0.06, 5.85 - LOW + 0.12), at=(0.0, Y0 + FT * 0.5, EAVE + (5.85 - LOW + 0.12) * 0.5))
    m.hull((WF - 4.4, FT + 0.04, 0.55), at=(0.0, Y0 + FT * 0.5, BASE + 5.85 + 0.27))
    tan = math.tan(math.radians(PITCH))
    m.hull_points([(x, y, EAVE - 0.3 * tan + 0.2) for x in (X0 - 0.3, X1 + 0.3) for y in (Y0 + FT, Y1 + 0.3)] +
                  [(0.0, y, EAVE + WS * 0.5 * tan + 0.25) for y in (Y0 + FT, Y1 + 0.3)])
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


def mount_shutters(building, shutters, open_every=2):
    """Copies of the shutters hung on a building's sockets for its preview: every other pair swung open flat against the
    wall. Remove them before an export (they'd be models of their own)."""
    copies = []
    kit.bpy.context.view_layer.update()
    sockets = sorted((c for c in building.children if c.name.startswith('SOCKET_Shutter_')), key=lambda c: c.name)
    for k, empty in enumerate(sockets):
        tag = empty.name[len('SOCKET_Shutter_')]
        number = int(''.join(ch for ch in empty.name.split('.')[0] if ch.isdigit()))
        copy = shutters[tag].copy()
        kit.bpy.context.scene.collection.objects.link(copy)
        swing = 0.0 if number % open_every else (-178.0 if tag == 'L' else 178.0)
        copy.matrix_world = empty.matrix_world @ Matrix.Rotation(math.radians(swing), 4, 'Z')
        copies.append(copy)
    return copies


# --- Shutters ---

SHUTTER_W, SHUTTER_H = 0.5, 1.4
# The pivot is the hinge: its pin stands 2 cm in front of the shutter's back face, so a shutter hung on a casing's face
# that swings open through 180 degrees lies flat against the wall instead of in it.
SHUTTER_BACK = 0.02


def shutter(name, side, seed, paint='H1'):
    """A plank shutter hung on its hinge at the origin, closed and facing -Y: three upright boards, two battens and a
    brace on the outside, strap hinges over the battens. side +1 reaches right of its hinge (it hangs at the left of a
    window), -1 left."""
    s = kit.Model(name, seed=seed)
    w, h = SHUTTER_W, SHUTTER_H
    y_board = SHUTTER_BACK - 0.0175
    boards = 3
    bw = (w - 0.01) / boards
    for k in range(boards):
        x = side * (0.01 + bw * (k + 0.5))
        s.board((x, y_board, 0.0), (x, y_board, h - s.rng.uniform(0.0, 0.02)), bw - 0.008, 0.035, uv=paint)
    for z in (0.22, h - 0.22):
        s.board((side * 0.04, y_board - 0.035, z), (side * (w - 0.03), y_board - 0.035, z), 0.12, 0.03, uv=paint)
    s.board((side * (w - 0.08), y_board - 0.035, 0.32), (side * 0.08, y_board - 0.035, h - 0.32), 0.1, 0.028, uv=paint)
    for z in (0.22, h - 0.22):
        s.board((side * -0.012, y_board - 0.055, z), (side * w * 0.68, y_board - 0.055, z), 0.06, 0.012,
                uv=Trim('H3', fit=True))
        s.cylinder((0.0, y_board - 0.05, z - 0.05), (0.0, y_board - 0.05, z + 0.05), 0.017, sides=6,
                   uv=Trim('H3', fit=True), face=(0.0, -1.0, 0.0))
    s.box((0.03, 0.02, 0.09), at=(side * (w - 0.06), y_board - 0.05, h * 0.5), uv=Trim('H3', fit=True))
    obj = s.finish(ground=False, preview=False, Nanite=0, LODs='50,25', Collision='None')
    return obj


def shutter_sockets(m, space, o, number, casing_w=0.13, casing_t=0.045):
    """SOCKET_Shutter_L<number> and _R<number> at the hinges of window o (closed shutters there), and the pintles they
    hang on."""
    for tag, x, side in (('L', o.x - casing_w, 1.0), ('R', o.x + o.w + casing_w, -1.0)):
        y = -casing_t - SHUTTER_BACK
        z = o.z - 0.05
        rotation = [math.degrees(a) for a in space.matrix.to_euler()]
        m.socket(f'Shutter_{tag}{number}', space.world((x, y, z)), rotation)
        for zz in (0.22, SHUTTER_H - 0.22):
            m.box((0.045, 0.05, 0.05), at=(x - side * 0.01, -casing_t - 0.025, z + zz - 0.07),
                  uv=Trim('H3', fit=True), space=space)


SHUTTERS = {'L': shutter('Shutter_Left', 1.0, 401), 'R': shutter('Shutter_Right', -1.0, 402)}
models = [undertaker(), saloon(), store(SHUTTERS), sheriff()]

if lt.want_preview():
    for obj in SHUTTERS.values():
        lt.preview([obj], lt.preview_path('RansomsRest/Town', obj.name), view=(-0.8, -1.6, 0.5), fit=0.9)
if __name__ == '__overview__':
    # Side by side for the overview, facades on one line.
    x = 0.0
    for obj in models:
        obj.location.x = x
        x += obj.dimensions.x + 3.0
    for k, obj in enumerate(SHUTTERS.values()):
        obj.location = (x + k * 0.8, -6.0, 0.0)
