"""Helpers for Ransom's Rest's own frontier homes (Art/Models/Buildings/Homestead.py; Docs/Areas/RansomsRest.md).

Ransom's Rest gets houses of its own, each built once, so a player walking the valley never meets the same house twice
(the user's call, 2026-10-13: Skyreach keeps the older shared models). This module sits on looter_buildings (kit:
frames, boards, panels, roofs, hulls, sockets, baked AO) and looter_town (town: finishes, cheaper roofs and plinths,
doors, windows, crepe) and adds what a valley of different houses needs:

  - Home, a town Model that also knows WindowGlow (the lamplit panes of the lived-in houses: Farmhouse.py's material,
    one instance in Unreal), the ground's dirt (a dugout's earth), puts lamplit glass in while glazing() is on and moves
    the whole model at finish() so its pivot is the front door (below);
  - walls: shell() (the outer face, edges and reveals of a closed house's wall), laps() (real clapboards in any finish,
    each board one plank or board of its texture), battens() (board-and-batten), corner boards that wrap the
    clapboards' open ends, water tables;
  - openings: double-hung sash windows lit (WindowGlow, linen curtains) or dark, shuttered, hanging or boarded up;
    looter_town's doors, boarded up or hung with crepe;
  - roofs: plane() lays one roof plane on a wall, so a gable can be lopsided (a saltbox), a shed or a porch; hip roofs
    (hip() and hip_shakes()); a hole where a roof has fallen in (the deck cut round it, the rafters across it and a dark
    attic under it); a porch roof sagging at a lost post (Droop);
  - chimneys (fieldstone stacks inside and outside a wall, stovepipes), porch parts (posts plain or turned, railings,
    steps) and dressing (woodpiles, rain barrels, benches);
  - the previews: each house in Ransom's Rest's golden late afternoon, the kit in a row, and the look sheet.

Pivots. A home's pivot is its front door at ground level: the middle of the doorway, on the front wall's outer face,
with z = 0 the ground there (the floor and the threshold stand above it on the foundation, which goes 0.35 m below it,
so a slope never opens a gap under the walls). Its front faces -Y (+X in Unreal), as every building's does. A house is
built round the middle of its footprint, which is easier to think in, and moved at finish(); the pivot's offset from the
footprint's middle goes in the model's 'Footprint' report line.

Everything random comes from the model's seeded random.Random, and nothing iterates a set of mesh elements, so a house
comes out the same on every run.
"""
import math
import os
from contextlib import contextmanager

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_model as lm
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place

UP = Vector((0.0, 0.0, 1.0))
# The lamplit panes: Farmhouse.py's WindowGlow exactly (colour, day Glow, Variation), so the name maps to one material
# instance in Unreal; the level's AHouseLights raises its Glow at dusk and lights a lamp at the house's SOCKET_Light.
WINDOW_COLOR, WINDOW_GLOW_DAY, WINDOW_GLOW_DUSK = 0x2e2219, 1.0, 14.0
# Clean stretches of the trim sheet's cream plaster strip (G) along U, m (Farmhouse.py's CREAM_U): curtains' old linen.
CREAM_U = (4.2, 4.3, 4.4, 4.5, 4.6, 5.45, 5.55, 5.65, 5.75, 5.85)
BELOW = 0.35                 # how far foundations go under the ground at the pivot
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Homes')
SHEET_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets')
FONT = os.path.join(lt.REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Bold.ttf')
FONT_REGULAR = os.path.join(lt.REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Regular.ttf')

EXTRA = {
    'windowglow': lambda: lm.material('WindowGlow', WINDOW_COLOR, Glow=WINDOW_GLOW_DAY, Variation=0.04),
    'dirt': lambda: lt.material('GroundDirt'),
    # The train's canvas (looter_train.py's CANVAS_TINTS): one instance, MI_Canvas.
    'canvas': lambda: lt.material('Polymer', name='Canvas', tint=0xd6c9a8),
}
REPORTS = {}                 # each finished home's numbers, by name (for the previews and the look sheet)


def args():
    """The script's own arguments (after '--')."""
    import sys
    return sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


# --- The model ---

class Home(town.Model):
    """A town Model for a house: the town's finishes plus WindowGlow ('windowglow') and GroundDirt ('dirt'). lived marks a
    lived-in house (lamplit windows, SOCKET_Light). glazing(key) puts every pane built meanwhile (the trim sheet's glass,
    H4) on that material. finish(pivot) moves the model so pivot (x, y on the ground) is its origin. window_sockets puts
    SOCKET_Window_<n> on each lamplit pane (window_socket(): the game's window life plays there); off, nothing changes."""

    def __init__(self, name, seed, lived=False, window_sockets=False):
        super().__init__(name, seed)
        self.lived = lived
        self.glass = None
        self.report = {}
        self.window_sockets = window_sockets
        self.windows = 0

    def window_socket(self, space, at):
        """SOCKET_Window_<n> (from 1, in build order) at a lamplit pane's face, its middle (at, in space), turned as the
        wall is: its front (Unreal's +X) out through the glass, its top up. UWindowLifeSubsystem plays someone at home
        there (a shadow crossing behind the glass, a curtain twitching) on a card of its own; nothing is built for it, so
        the mesh stays exactly as it was. Only on a model made with window_sockets."""
        if not self.window_sockets:
            return
        self.windows += 1
        rotation = [math.degrees(a) for a in space.matrix.to_euler()]
        self.socket(f'Window_{self.windows}', space.world(at), rotation)

    def slot(self, key):
        if key not in self.slots:
            make = EXTRA.get(key)
            self.slots[key] = (len(self.slots), make() if make else town.material(key))
        return self.slots[key][0]

    def emit(self, tb, uv=None, mat='trim', place=None, space=None, *rest, **options):
        if self.glass is not None and mat == 'trim' and isinstance(uv, Trim) and uv.strip == 'H4':
            mat = self.glass
        super().emit(tb, uv, mat, place, space, *rest, **options)

    @contextmanager
    def glazing(self, glass):
        old = self.glass
        self.glass = glass
        try:
            yield
        finally:
            self.glass = old

    def finish(self, pivot=(0.0, 0.0), footprint=None, **options):
        """The model object, its origin at pivot (the front door on the ground). footprint (x0, x1, y0, y1: the walls'
        outer faces, in the frame the house was built in) goes into the report, measured from the pivot."""
        shift = Vector((-pivot[0], -pivot[1], 0.0))
        if shift.length > 1e-9:
            bmesh.ops.translate(self.bm, vec=shift, verts=self.bm.verts[:])
            self.hulls = [[Vector(p) + shift for p in points] for points in self.hulls]
            self.sockets = [(name, tuple(Vector(at) + shift), rotation) for name, at, rotation in self.sockets]
        if footprint is not None:
            x0, x1, y0, y1 = footprint
            self.report['footprint'] = (x0 - pivot[0], x1 - pivot[0], y0 - pivot[1], y1 - pivot[1])
        sockets = [s[0] for s in self.sockets]
        hulls = len(self.hulls)
        obj = super().finish(preview=False, **options)
        cos = [v.co for v in obj.data.vertices]
        lo = Vector((min(c.x for c in cos), min(c.y for c in cos), min(c.z for c in cos)))
        hi = Vector((max(c.x for c in cos), max(c.y for c in cos), max(c.z for c in cos)))
        self.report.update(name=obj.name, triangles=sum(len(p.vertices) - 2 for p in obj.data.polygons),
                           materials=[mt.name for mt in obj.data.materials], bounds=(tuple(lo), tuple(hi)),
                           sockets=sockets, hulls=hulls, lived=self.lived)
        fp = self.report.get('footprint')
        walls = f"; walls x {fp[0]:.2f}..{fp[1]:.2f}, y {fp[2]:.2f}..{fp[3]:.2f}" if fp else ''
        print(f"HOMES: {obj.name}: {self.report['triangles']} triangles, {hulls} hulls, sockets {', '.join(sockets)}; "
              f"materials {', '.join(self.report['materials'])}; bounds x {lo.x:.2f}..{hi.x:.2f}, "
              f"y {lo.y:.2f}..{hi.y:.2f}, z {lo.z:.2f}..{hi.z:.2f}{walls}", flush=True)
        REPORTS[obj.name] = self.report
        return obj


# --- Faces and simple parts ---

def quad(m, corners, uv, mat='trim', space=None, facing=None, cuts=0):
    """A one-sided flat polygon through corners (x, y, z) in space, turned to face facing (a direction) if given."""
    tb = kit._new_bmesh()
    verts = [tb.verts.new(c) for c in corners]
    face = tb.faces.new(verts)
    face.normal_update()
    if facing is not None and face.normal.dot(Vector(facing)) < 0.0:
        face.normal_flip()
    if cuts:
        a, b = Vector(corners[0]), Vector(corners[1])
        axis = (b - a).normalized()
        span = (b - a).length
        for k in range(1, cuts + 1):
            p = a + axis * span * k / (cuts + 1)
            bmesh.ops.bisect_plane(tb, geom=tb.verts[:] + tb.edges[:] + tb.faces[:], dist=1e-5, plane_co=p,
                                   plane_no=axis)
    m.emit(tb, uv, mat, None, space)


def vspans(outline, x):
    """The z intervals inside a polygon outline [(x, z), ...] at x (town.spans_at turned on its side)."""
    return town.spans_at([(z, xx) for xx, z in outline], x)


# --- Walls (in a kit wall space: x along the wall, z up from the floor, the outer face at y = 0, outside toward -y) ---

def shell(m, outline, holes, thick, uv, space, around=(), slices=1.2, mat='trim'):
    """A wall panel without its inner face (Cottage.py's shell_panel): the closed house is never seen from inside (its
    doors are shut and its glass opaque), so only the outer face, the edges and the reveals of its openings are kept,
    and its bottom (on the foundation) and top (under the roof deck) go too. Sliced as kit panels are, for the AO."""
    tb = kit._prism([list(outline)] + [list(h) for h in holes], thick)
    xs = [x for x, z in outline]
    zs = [z for x, z in outline]
    upright = getattr(uv, 'rotate', False)
    cut_x, cut_z = [], []
    if slices and not upright:
        cut_x += kit.frange(min(xs) + slices, max(xs) - 0.1, slices)
    for o in around:
        if o.z > 0.15 and round(o.z - 0.1, 2) not in cut_z:
            cut_z.append(round(o.z - 0.1, 2))
        if not upright:
            cut_x += [o.x - 0.2, o.x + o.w + 0.2]
    kit._slice(tb, 0, sorted(x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05))
    kit._slice(tb, 2, sorted(z for z in cut_z if min(zs) + 0.05 < z < max(zs) - 0.05))
    tb.normal_update()
    lo, hi = min(zs), max(zs)

    def hidden(f):
        flat = [v.co.z for v in f.verts]
        return (f.normal.y > 0.99 or f.normal.z < -0.99 and max(flat) < lo + 1e-4
                or f.normal.z > 0.99 and min(flat) > hi - 1e-4)
    bmesh.ops.delete(tb, geom=[f for f in tb.faces if hidden(f)], context='FACES_ONLY')
    m.emit(tb, uv, mat, None, space)


class Boards:
    """WoodPlanks at world scale in a part's own frame (a wall's: x along it, z up): its planks upright (rotate) or
    level. Each face is projected on the axis plane its normal faces most; a painted wall (a Wood* material) takes it."""

    def __init__(self, rotate=True, u=None):
        self.rotate, self.u = rotate, u

    def apply(self, obj, rng):
        info = lt.SETS['WoodPlanks']
        scale = info['density'] / float(info['size'])
        repeat = info['size'] / float(info['density'])
        u0 = self.u if self.u is not None else rng.uniform(0.0, repeat)
        v0 = rng.uniform(0.0, repeat)
        mesh = obj.data
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            n = p.normal
            axis = max(range(3), key=lambda i: abs(n[i]))
            for li in p.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                a, b = ((co.x, co.z) if axis == 1 else (co.y, co.z) if axis == 0 else (co.x, co.y))
                u, v = (b, -a) if self.rotate else (a, b)
                uv[li].uv = ((u + u0) * scale, (v + v0) * scale)


def lap_spec(look, height):
    """(uv spec, material key) for one clapboard height tall in a finish: a Wood* paint (one plank of WoodPlanks),
    'band_G' (the trim sheet's cream strip), 'H1'/'H2' (its teal and oxide-red painted boards) or 'A' (one of its
    weathered boards)."""
    if look in town.WOODS:
        return town.PlankRow(), look
    if look == 'band_G':
        return town.Band('G', width=height), 'trim'
    if look in ('H1', 'H2'):
        return Trim(look, fit=True), 'trim'
    return Trim(look, width=height), 'trim'


def lap_backing(look, rng):
    """(uv spec, material key) for the wall face behind clapboards in look: the same colour, so the little steps
    where the boards stop under a gable's rake show nothing different behind them."""
    if look in town.WOODS:
        return Boards(rotate=False), look
    if look in ('H1', 'H2'):
        # The painted strips are one board tall: mapped at world scale they'd cut the wall into 0.2 m bands. The tinted
        # planks of the same paint are near enough for the slivers that show.
        return Boards(rotate=False), 'woodoxide' if look == 'H2' else 'woodteal'
    if look == 'band_G':
        return Trim('G', world=True, u=rng.uniform(4.2, 5.2)), 'trim'
    return Trim(look, world=True, u=rng.uniform(0.0, kit.U_REPEAT)), 'trim'


def laps(m, space, outline, z0, z1, keep_out=(), look='band_G', exposure=0.2, out=0.026, thick=0.014, cut=3.0):
    """Lapped clapboards over a wall's face (looter_town's clapboards, each board mapped as one board of its texture):
    rows of exposure from z0 to z1 inside the outline, tilted so each lower edge stands out by out, stopping at the boxes
    in keep_out ((x0, x1, z0, z1): casings). Their ends are open (corner boards and casings cover them). Returns out."""
    tilt = -math.degrees(math.atan2(out - thick, exposure))
    z = z0
    while z < z1 - 0.03:
        h = min(exposure, z1 - z)
        spans = town.intersect(town.spans_at(outline, z + 0.01), town.spans_at(outline, z + h - 0.01))
        for x0, x1, k0, k1 in keep_out:
            if k0 < z + h - 0.01 and k1 > z + 0.01:
                spans = town.subtract(spans, (x0, x1))
        for a, b in spans:
            spec, key = lap_spec(look, h + 0.014)
            matrix = place(((a + b) * 0.5, -(out + thick) * 0.5 + 0.003, z + h * 0.5), (tilt, 0.0, 0.0))
            m.emit(kit._box((b - a, thick, h + 0.014), 0.0, int((b - a) / cut) if cut else 0, ('+y', '+z', '-x', '+x')),
                   spec, key, matrix, space)
        z += exposure
    return out


def battens(m, space, outline, x0, x1, keep_out=(), look='A', every=0.4, width=0.07, thick=0.025, z0=0.0):
    """Board-and-batten: a narrow batten over every joint (every meters from x0 to x1) up the wall face, from z0 to the
    outline's top, broken by the boxes in keep_out ((x0, x1, z0, z1))."""
    x = x0 + every * 0.5
    while x < x1 - 0.05:
        for a, b in vspans(outline, x):
            runs = [(max(a, z0), b - 0.04)]
            for k0, k1, c0, c1 in keep_out:
                if k0 - width * 0.5 < x < k1 + width * 0.5:
                    runs = town.subtract(runs, (c0, c1))
            for lo, hi in runs:
                if hi - lo > 0.12:
                    town.board(m, (x, 0.0, lo), (x, 0.0, hi), width, thick, look=look, space=space, lift=thick * 0.5,
                               drop=('+y',))
        x += every


def corners(m, space, length, top0, top1, look='C', out=0.0, width=0.14, wrap=True, bottom=-0.05, thick=0.04):
    """Corner boards at both ends of a wall, from the wall face out past its clapboards (out) by thick: on the walls that
    wrap, they run on past the corner by that much and cover the next wall's boards' edges (that wall's don't wrap)."""
    t = out + thick
    ext = t if wrap else 0.0
    for a, b, top in ((-ext, width, top0), (length - width, length + ext, top1)):
        town.trim_board(m, space, ((a + b) * 0.5, 0.0, bottom), ((a + b) * 0.5, 0.0, top), b - a, look, thick=t)


def band(m, space, spans, z, height, look='C', out=0.0, thick=0.04):
    """A horizontal trim board over the clapboards (a water table along the foot, a frieze under the eaves)."""
    for a, b in spans:
        if b - a > 0.1:
            town.trim_board(m, space, (a, 0.0, z), (b, 0.0, z), height, look, thick=out + thick)


def door_spans(items, length, margin=0.2):
    """The stretches of a wall's foot left by its doors."""
    spans = [(0.0, length)]
    for o in items:
        if o.kind == 'door':
            spans = town.subtract(spans, (o.x - margin, o.x + o.w + margin))
    return spans


def keep_out(items, casing=0.11):
    """The boxes clapboards and battens stop at: each opening with its casing, head and sill."""
    boxes = []
    for o in items:
        cw = o.opts.get('casing_w', casing if o.kind == 'window' else 0.14)
        if o.kind == 'door':
            boxes.append((o.x - cw - 0.02, o.x + o.w + cw + 0.02, -1.0, o.h + cw * 1.25))
        else:
            boxes.append((o.x - cw - 0.02, o.x + o.w + cw + 0.02, o.z - 0.07, o.z + o.h + cw * 1.35 + 0.05))
    return boxes


def plinth(m, x0, x1, y0, y1, top, out=0.06, below=BELOW):
    """A fieldstone foundation round the footprint (looter_town's, without its hidden faces), from below the ground up to
    the floor (top), standing out of the walls by out."""
    rng = m.rng
    deep = 0.3
    specs = [((x0 - out, y0 - out), (x1 + out, y0 - out), 0.0), ((x1 + out, y0 - out), (x1 + out, y1 + out), deep),
             ((x1 + out, y1 + out), (x0 - out, y1 + out), 0.0), ((x0 - out, y1 + out), (x0 - out, y0 - out), deep)]
    for p0, p1, inset in specs:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        m.box((length, deep, top + below), at=(inset + length * 0.5, deep * 0.5, (top - below) * 0.5),
              uv=Trim('D', world=True, u=rng.uniform(0.0, kit.U_REPEAT)), space=space, cuts=max(0, int(length / 2.4)),
              drop=('+y', '-z'))


def piers(m, points, top, size=0.36, below=BELOW):
    """Fieldstone piers under a house raised off the ground: a stack at each point, from below the ground to top."""
    rng = m.rng
    for x, y in points:
        kit.stone_stack(m, (x, y, -below), (size + rng.uniform(-0.03, 0.03), size + rng.uniform(-0.03, 0.03)),
                        top + below, taper=0.08, rot=rng.uniform(-7.0, 7.0), bevel=0.0)


# --- Windows and doors ---

def sash(m, space, o, depth, out=0.0):
    """A double-hung sash window in opening o: casings, a head with a drip cap and a thick sill over the wall's face
    (out: the clapboards' face), the glass half way into the wall, the sashes' stiles and rails, the meeting rail and the
    muntins. opts: paint (the casings' finish), sash (the sashes'), panes (columns, rows: (2, 2) a two-over-two,
    (3, 4) a six-over-six), casing_w, lit (WindowGlow glass with linen curtains), sill (the sill's finish)."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    paint = o.opts.get('paint', 'H1')
    sash_paint = o.opts.get('sash', paint)
    lit = o.opts.get('lit', False)
    cw = o.opts.get('casing_w', 0.11)
    ct = 0.04
    sill = o.opts.get('sill', 'C')
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        town.trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, paint, thick=ct, out=out)
    head = z0 + h + cw * 0.65
    town.trim_board(m, space, (x0 - cw - 0.02, 0.0, head), (x0 + w + cw + 0.02, 0.0, head), cw * 1.3, paint,
                    thick=ct + 0.008, out=out)
    town.box(m, (w + 2.0 * cw + 0.12, 0.06 + ct, 0.04), at=(x0 + w * 0.5, -out - (0.06 + ct) * 0.5,
                                                              z0 + h + cw * 1.3 + 0.02), look=sill, space=space)
    sill_depth = 0.1 + out + depth * 0.5
    town.box(m, (w + 2.0 * cw + 0.1, sill_depth, 0.06), at=(x0 + w * 0.5, -0.1 - out + sill_depth * 0.5, z0 - 0.03),
             rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), look=sill, space=space, drop=('+y',))
    glass_y = depth * 0.55
    m.box((w, 0.02, h), at=(x0 + w * 0.5, glass_y, z0 + h * 0.5), uv=Trim('H4', fit=True),
          mat='windowglow' if lit else 'trim', space=space, drop=town.GLASS)
    if lit and hasattr(m, 'window_socket'):
        # The pane's outer face (the glass is 2 cm thick about glass_y), its middle: the window life's card goes there.
        m.window_socket(space, (x0 + w * 0.5, glass_y - 0.01, z0 + h * 0.5))
    bar_y = glass_y - 0.03
    stile = 0.045
    for x in (x0 + stile * 0.5, x0 + w - stile * 0.5):
        town.board(m, (x, bar_y, z0), (x, bar_y, z0 + h), stile, 0.04, look=sash_paint, space=space, drop=('+y',))
    for z, rail in ((z0 + 0.035, 0.07), (z0 + h - 0.025, 0.05)):
        town.board(m, (x0 + stile, bar_y, z), (x0 + w - stile, bar_y, z), rail, 0.04, look=sash_paint, space=space,
                   drop=('+y', '-x', '+x'))
    cols, rows = o.opts.get('panes', (2, 2))
    single = o.opts.get('single', False)
    if not single:     # the meeting rail, where the lower sash closes against the upper
        town.board(m, (x0 + stile, bar_y - 0.012, z0 + h * 0.5), (x0 + w - stile, bar_y - 0.012, z0 + h * 0.5), 0.055,
                   0.05, look=sash_paint, space=space, drop=('+y', '-x', '+x'))
    for i in range(1, cols):
        x = x0 + w * i / cols
        town.board(m, (x, bar_y - 0.004, z0 + 0.07), (x, bar_y - 0.004, z0 + h - 0.05), 0.032, 0.035, look=sash_paint,
                   space=space, drop=('+y', '-x', '+x'))
    per = rows if single else max(1, rows // 2)
    for half in ((0.0,) if single else (0.0, 0.5)):
        for j in range(1, per):
            z = z0 + h * (half + (0.5 if not single else 1.0) * j / per)
            town.board(m, (x0 + stile, bar_y - 0.006, z), (x0 + w - stile, bar_y - 0.006, z), 0.03, 0.035,
                       look=sash_paint, space=space, drop=('+y', '-x', '+x'))
    if lit:
        curtains(m, space, o, depth)


class Cloth:
    """A curtain on the trim sheet's cream plaster strip G, which reads as old linen (Farmhouse.py's)."""

    def __init__(self, u0):
        self.u0 = u0

    def apply(self, obj, rng):
        mesh = obj.data
        v0, v1 = lt.TRIM_STRIPS['G']
        inset = 2.0 / lt.TRIM_SIZE
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        x_lo = min(v.co.x for v in mesh.vertices)
        z_lo = min(v.co.z for v in mesh.vertices)
        z_span = max(max(v.co.z for v in mesh.vertices) - z_lo, 1e-6)
        uv = mesh.uv_layers.active.data
        for loop in mesh.loops:
            co = mesh.vertices[loop.vertex_index].co
            uv[loop.index].uv = ((self.u0 + co.x - x_lo) * scale,
                                 v0 + inset + (co.z - z_lo) / z_span * (v1 - v0 - 2.0 * inset))


def sheet(m, rows, uv, mat='trim', space=None, facing=(0.0, -1.0, 0.0), smooth=True):
    """A one-sided cloth through a grid of points (rows of (x, y, z) in space), turned to face facing."""
    tb = kit._new_bmesh()
    grid = [[tb.verts.new(p) for p in row] for row in rows]
    for j in range(len(grid) - 1):
        for i in range(len(grid[j]) - 1):
            try:
                tb.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            except ValueError:
                pass
    tb.normal_update()
    if sum((f.normal * f.calc_area() for f in tb.faces), Vector()).dot(Vector(facing)) < 0.0:
        bmesh.ops.reverse_faces(tb, faces=tb.faces[:])
    m.emit(tb, uv, mat, None, space, smooth=smooth)


def curtains(m, space, o, depth):
    """Two linen curtains behind the sash bars, gathered at the head, tied back a little under half way, drawn aside to
    leave the middle of the window clear (Farmhouse.py's)."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    glass = depth * 0.55 - 0.01
    for side in (-1.0, 1.0):
        edge = x0 - 0.012 if side < 0 else x0 + w + 0.012
        cover = w * rng.uniform(0.28, 0.36) + 0.012
        rows = []
        for z, span, fold in ((z0 + h - 0.004, cover, 0.016), (z0 + h * 0.44, cover * 0.42, 0.007),
                              (z0 + 0.012, cover * 0.68, 0.012)):
            rows.append([(edge - side * span * i / 4.0, glass - 0.009 - (fold if i % 2 else 0.0), z) for i in range(5)])
        sheet(m, rows, Cloth(rng.choice(CREAM_U)), space=space)


def shutter_leaf(m, space, cx, cz, w, h, y, paint, battens=True):
    """A plank shutter leaf of three boards (w by h, its middle at cx, cz, its back at y in space), with two battens."""
    rng = m.rng
    boards = 3
    for k in range(boards):
        x = cx - w * 0.5 + w * (k + 0.5) / boards
        town.board(m, (x, y, cz - h * 0.5), (x, y, cz + h * 0.5 - rng.uniform(0.0, 0.02)), w / boards - 0.008, 0.035,
                   look=paint, space=space, lift=0.0175, drop=('+y',))
    if battens:
        for z in (cz - h * 0.3, cz + h * 0.3):
            town.board(m, (cx - w * 0.45, y - 0.035, z), (cx + w * 0.45, y - 0.035, z), 0.09, 0.025, look=paint,
                       space=space, lift=0.0125, drop=('+y', '-x', '+x'))


def window(m, space, o, depth, out=0.0):
    """A window in opening o, by its style: 'sash' (sash(); lit=True for a lamplit one), 'open' (a sash with its
    shutters folded back against the wall), 'shut' (casing and sill, its plank shutters closed over it), 'hanging' (one
    shutter closed, the other hanging askew from its last hinge), 'boarded' (a dark sash with planks nailed across)."""
    rng = m.rng
    style = o.opts.get('style', 'sash')
    paint = o.opts.get('paint', 'H1')
    shutter = o.opts.get('shutter', paint if paint in ('H1', 'H2', 'woodteal', 'woodoxide', 'woodblack') else 'A')
    cw = o.opts.get('casing_w', 0.11)
    sash(m, space, o, depth, out)
    x0, z0, w, h = o.x, o.z, o.w, o.h
    face = -out - 0.04 - 0.012                                   # in front of the casings
    if style == 'open':
        sw = w * 0.5 + 0.03
        for sign in (-1.0, 1.0):
            edge = x0 - cw if sign < 0 else x0 + w + cw
            shutter_leaf(m, space, edge + sign * (sw * 0.5 + 0.01), z0 + h * 0.5, sw, h + 0.06, -out - 0.006, shutter)
    elif style in ('shut', 'hanging'):
        sw = w * 0.5 + cw * 0.5
        for sign in (-1.0, 1.0):
            cx = x0 + w * 0.5 + sign * sw * 0.5
            if style == 'hanging' and sign > 0:
                # Hung from its top outer hinge alone, swung down askew.
                hinge = Vector((x0 + w + cw * 0.5, face, z0 + h + 0.03))
                turn = rng.uniform(14.0, 22.0)
                leaf = Space(space.matrix @ Matrix.Translation(hinge) @ Matrix.Rotation(math.radians(turn), 4, 'Y')
                             @ Matrix.Translation(-hinge))
                shutter_leaf(m, leaf, cx, z0 + h * 0.5, sw - 0.01, h + 0.08, face, shutter)
            else:
                shutter_leaf(m, space, cx, z0 + h * 0.5, sw - 0.01, h + 0.08, face, shutter)
    elif style == 'boarded':
        town.board_up(m, space, x0, z0, w, h, look=o.opts.get('boards', 'A'), out=out + 0.045,
                      level=rng.random() < 0.75, extra=0.18)


def door(m, space, o, depth, out=0.0):
    """looter_town's door in opening o (opts: style 'plank' or 'panel', finish, casing, hinge, knob, transom, casing_w),
    plus boarded=True (planks nailed across: an empty house) or crepe=True (a bow of black crepe: a house in
    mourning)."""
    town.door(m, space, o, depth, out)
    if o.opts.get('boarded'):
        town.board_up(m, space, o.x, 0.2, o.w, o.h - 0.45, look='A', out=out + 0.07, level=True, extra=0.26)
    if o.opts.get('crepe'):
        town.crepe_bow(m, space, (o.x + o.w * 0.5, 0.0, 1.55), scale=1.0, tails=0.62, y=depth * 0.3 - 0.075)


def openings(m, space, items, depth, out=0.0):
    for o in items:
        if o.kind == 'door':
            door(m, space, o, depth, out)
        else:
            window(m, space, o, depth, out)


# --- Roofs ---

class Droop(kit.Slope):
    """A slope whose low corner has dropped where its post went (a porch roof): besides the kit's sag, the eave corner at
    x = 0 (corner='left') or x = width ('right') drops by corner meters, easing off up the slope and along the eave."""

    def __init__(self, *rest, corner=0.0, side='left', **options):
        self.corner, self.side = corner, side
        super().__init__(*rest, **options)

    def drop(self, x, y):
        base = super().drop(x, y)
        u = min(max(x / self.width, 0.0), 1.0)
        u = 1.0 - u if self.side == 'left' else u
        t = 1.0 - min(max(y / self.length, 0.0), 1.0)
        return base + self.corner * u * u * t ** 1.3


def plane(m, x0, x1, wall_y, eave_z, pitch, run, toward=1.0, overhang=0.4, rake=(0.3, 0.3), deck=0.1, sag=0.0,
          frame=None, rake_boards=(True, True), deck_uv='A', cls=kit.Slope, hole=None, **slope_options):
    """One roof plane resting on a wall's outer top edge (wall_y, eave_z, from x0 to x1) and rising at pitch over run
    meters (to the ridge) away from it: toward +1 rises toward +y (a front slope), -1 toward -y (a back slope). Its deck
    reaches overhang past the wall and rake past each end (left, right as seen from the front), with rake boards where
    rake_boards says. The slope's frame (kit.Slope: x along the eave from its left end seen from outside, y up the slope,
    z out of it) is returned; frame turns the whole roof (a ridge along Y). hole (x0, x1, y0, y1 in the slope's frame)
    leaves the deck open there (holed_deck)."""
    theta = math.radians(pitch)
    t, c, s = math.tan(theta), math.cos(theta), math.sin(theta)
    length = (run + overhang) / c + deck * t
    width = x1 - x0 + rake[0] + rake[1]
    if toward > 0:
        origin = Vector((x0 - rake[0], wall_y - overhang, eave_z - overhang * t)) + Vector((0.0, -s, c)) * deck
        slope = cls(origin, (1.0, 0.0, 0.0), (0.0, c, s), width, length, sag, frame, **slope_options)
        sides = tuple(rake_boards)
    else:
        origin = Vector((x1 + rake[1], wall_y + overhang, eave_z - overhang * t)) + Vector((0.0, s, c)) * deck
        slope = cls(origin, (-1.0, 0.0, 0.0), (0.0, -c, s), width, length, sag, frame, **slope_options)
        sides = (rake_boards[1], rake_boards[0])
    if hole is not None:
        holed_deck(m, slope, deck, hole, deck_uv, sides)
    else:
        town.roof_deck(m, slope, deck, deck_uv, any(sides), sides)
    slope.deck = deck
    return slope


def holed_deck(m, slope, deck, hole, uv='A', rake_sides=(True, True)):
    """A roof deck with a hole where the roof fell in: the boards round it, the rafters across it, a few broken boards
    hanging into it and the rake boards."""
    rng = m.rng
    hx0, hx1, hy0, hy1 = hole
    width, length = slope.width, slope.length
    for a, b, c, d in ((0.0, width, 0.0, hy0), (0.0, width, hy1, length), (0.0, hx0, hy0, hy1), (hx1, width, hy0, hy1)):
        if b - a > 0.02 and d - c > 0.02:
            m.box((b - a, d - c, deck), at=((a + b) * 0.5, (c + d) * 0.5, -deck * 0.5), uv=Trim(uv, world=True),
                  space=slope, cuts=max(0, int((b - a) / 3.0)))
    for side, x in enumerate((-0.025, width + 0.025)):
        if rake_sides[side]:
            face = (-1.0, 0.0, 0.0) if side == 0 else (1.0, 0.0, 0.0)
            mid = (0.06 - deck - 0.05) * 0.5
            m.board((x, -0.02, mid), (x, length - 0.02, mid), deck + 0.11, 0.05, face=face, uv='C', space=slope)
    # The rafters, on edge under the deck, every 0.6 m across the hole, the length of the slope.
    x = hx0 + 0.18
    while x < hx1 - 0.1:
        m.board((x, 0.1, -deck - 0.07), (x, length - 0.1, -deck - 0.07), 0.14, 0.06, face=(1.0, 0.0, 0.0), uv='C',
                space=slope, cuts=0)
        x += 0.6
    # Broken deck boards hanging into the hole from its upper edge.
    for k in range(3):
        bx = rng.uniform(hx0 + 0.15, hx1 - 0.35)
        reach = rng.uniform(0.35, 0.8)
        m.box((0.19, reach, 0.025), matrix=place((bx, hy1 - reach * 0.45, -deck * 0.5 - 0.02),
                                                 (rng.uniform(-28.0, -12.0), 0.0, rng.uniform(-8.0, 8.0))),
              uv=Trim('A', width=0.19), space=slope)


def ragged(hole, seed, fray=0.4):
    """A shakes skip for a roof's hole (x0, x1, y0, y1 in its slope's frame) with broken edges: each course of shakes
    stops at its own place round it, the hole widest in the middle courses and narrowing toward its top and bottom, a
    shake or two hanging on into it. Its deck hole (holed_deck) is the plain rectangle, so the deck's sawn edge shows
    where the shakes have gone further."""
    import random
    rng = random.Random(seed)
    hx0, hx1, hy0, hy1 = hole
    courses = {}

    def skip(x, y):
        if not hy0 - 0.25 < y < hy1 + 0.2:
            return False
        key = round(y, 3)
        if key not in courses:
            t = min(max((y - hy0) / max(hy1 - hy0, 1e-6), 0.0), 1.0)
            narrow = (1.0 - math.sin(math.pi * t) ** 0.6) * (hx1 - hx0) * 0.3
            courses[key] = (hx0 - rng.uniform(-0.1, fray) + narrow, hx1 + rng.uniform(-0.1, fray) - narrow)
        a, b = courses[key]
        return a < x < b
    return skip


class Tiled:
    """A tileable set by world-scale cube projection (lt.box_uv) at a texel density of its own: Hay at 900 px/m reads
    as a sod roof's dry grass instead of straw."""

    def __init__(self, set_name, density):
        self.set_name, self.density = set_name, density

    def apply(self, obj, rng):
        lt.box_uv(obj, self.set_name, texel_density=self.density, seed=rng.randrange(1 << 30))


def attic(m, x0, x1, y0, y1, z, gables=()):
    """The dark attic seen through a roof's hole: a board floor at the wall plate (z) over the walls' inside, and the
    gable walls' inside faces (gables: triangles of three (x, y, z) points, wound any way; they face the attic)."""
    mid = Vector(((x0 + x1) * 0.5, (y0 + y1) * 0.5, z + 1.0))
    quad(m, [(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], Trim('A', world=True), facing=(0.0, 0.0, 1.0))
    for tri in gables:
        center = sum((Vector(p) for p in tri), Vector()) / 3.0
        quad(m, tri, Trim('A', world=True, rotate=True), facing=mid - center)


def hip_bounds(slope, theta, ends=(True, True), base=(0.0, 0.0)):
    """A hip roof plane's outline along its courses: the x range at slope height y, narrowing toward the top at the hip
    ends (ends: left, right) at 45 degrees in plan. base insets each end at the eave."""
    c = math.cos(theta)

    def bounds(y):
        lo = base[0] + (y * c if ends[0] else 0.0)
        hi = slope.width - base[1] - (y * c if ends[1] else 0.0)
        return lo, hi
    return bounds


def hip_deck(m, slope, bounds, deck, top):
    """The deck of a hip roof plane: a flat piece following its outline from the eave (y = 0) to top."""
    steps = 1
    left = [(bounds(top * k / steps)[0], top * k / steps) for k in range(steps + 1)]
    right = [(bounds(top * k / steps)[1], top * k / steps) for k in range(steps + 1)]
    outline = [left[0], right[0]] + right[1:] + list(reversed(left[1:]))
    clean = []
    for p in outline:
        if not clean or (Vector(p) - Vector(clean[-1])).length > 1e-4:
            clean.append(p)
    if (Vector(clean[0]) - Vector(clean[-1])).length < 1e-4:
        clean.pop()
    # Built in the XZ plane (x along the eave, z up the slope), turned so its face is the slope's top (+z).
    turn = Matrix(((1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, -1.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)))
    m.emit(kit._prism([clean], deck), Trim('A', world=True), 'trim', turn, slope)


def hip_shakes(m, slope, bounds, top, course=0.2, butt=0.03, piece=(1.6, 2.8), start=-0.08, missing=0.02):
    """Shakes on a hip roof plane: courses from the eave up, each cut square at the hips where the plane's outline
    (bounds) has narrowed at the course's middle (the hip caps cover the cut)."""
    rng = m.rng
    exposure = course - 0.02
    y = start
    k = 0
    tilt = -math.degrees(math.atan2(butt, course))
    while y < top - 0.04:
        h = min(course, top - y)
        lo, hi = bounds(min(max(y + h * 0.5, 0.0), top))
        lo -= 0.04
        hi += 0.04
        if hi - lo < 0.12:
            break
        x = lo
        while x < hi - 1e-3:
            w = rng.uniform(*piece)
            if hi - (x + w) < 0.5:
                w = hi - x
            spans = [(x, x + w)]
            if k >= 2 and w > 0.9 and rng.random() < missing * 3.0:
                gap = rng.uniform(0.2, 0.4)
                at = rng.uniform(x + 0.3, x + w - 0.3 - gap)
                spans = [(x, at), (at + gap, x + w)]
            for a, b in spans:
                drop = ['-z', '+y']
                matrix = place(((a + b) * 0.5, y + h * 0.5 + rng.uniform(-0.01, 0.01), butt * 0.5 + 0.004 * (k % 2)),
                               (tilt, rng.uniform(-0.4, 0.4), 0.0))
                m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=k % 4, u=rng.uniform(0.0, kit.U_REPEAT)),
                      space=slope, ao_floor=kit.ROOF_AO, drop=tuple(drop))
            x += w
        y += exposure
        k += 1


def cap_line(m, p0, p1, normal, width=0.24, thick=0.05, look='C', lift=0.04):
    """A cap board along a hip or a ridge from p0 to p1 (world), lying on the roof (normal: its outward up)."""
    n = Vector(normal).normalized()
    town.board(m, Vector(p0), Vector(p1), width, thick, look=look, face=n, lift=lift + thick * 0.5, drop=('+y',))


def gable_hull(m, x0, x1, y_front, y_back, eave_front, eave_back, ridge_y, ridge_z, overhang=0.35, rake=0.35,
               pitch_front=40.0, pitch_back=None):
    """The roof's collision: a prism from the eaves' overhang up to the ridge (a lopsided gable too)."""
    pb = pitch_front if pitch_back is None else pitch_back
    zf = eave_front - overhang * math.tan(math.radians(pitch_front)) + 0.22
    zb = eave_back - overhang * math.tan(math.radians(pb)) + 0.22
    pts = [(x, y_front - overhang, zf) for x in (x0 - rake, x1 + rake)]
    pts += [(x, y_back + overhang, zb) for x in (x0 - rake, x1 + rake)]
    pts += [(x, ridge_y, ridge_z + 0.25) for x in (x0 - rake, x1 + rake)]
    m.hull_points(pts)


def slope_hull(m, slope, thick=0.22, inset=0.0):
    """A thin box of collision on a roof plane (a porch roof, a lean-to)."""
    m.hull((slope.width - 2.0 * inset, slope.length, thick), at=(slope.width * 0.5, slope.length * 0.5, -0.03),
           space=slope)


# --- Chimneys ---

def flue_cap(m, x, y, top, size=(0.62, 0.72), socket='Smoke'):
    """A chimney's top: a slab of flat stones over the stack, a short top course and SOCKET_<socket> over it."""
    m.box((size[0] + 0.16, size[1] + 0.16, 0.1), at=(x, y, top + 0.05), uv=Trim('D', world=True), bevel=0.02)
    m.box((size[0] * 0.55, size[1] * 0.6, 0.18), at=(x, y, top + 0.19), uv='D')
    if socket:
        m.socket(socket, (x, y, top + 0.3))


def stack(m, x, y, z_from, top, size=(0.62, 0.72), taper=0.03, socket='Smoke'):
    """A fieldstone stack rising through a roof from z_from to top, with its cap (Cottage.py's chimney)."""
    kit.stone_stack(m, (x, y, z_from), size, top - z_from, taper=taper)
    flue_cap(m, x, y, top, size, socket)
    m.hull((size[0] + 0.02, size[1] + 0.02, top - z_from + 0.3), at=(x, y, (z_from + top + 0.3) * 0.5))


def outside_chimney(m, x_wall, y, outward, top, floor, width=1.3, deep=0.9, flue=(0.66, 0.78), socket='Smoke'):
    """A fieldstone chimney built against a gable wall's outside (x_wall, centred on y, outward +1 or -1 along X): a
    broad firebox stack from below the ground to past the floor's mantel height, shoulders stepping in, then the flue up
    past the ridge to top (LogCabin.py's). Returns nothing; its hulls are added."""
    rng = m.rng
    cx = x_wall + outward * deep * 0.5
    base_top = floor + 2.2
    kit.stone_stack(m, (cx, y, -BELOW), (deep, width), base_top + BELOW, taper=0.03, rot=0.0)
    shoulder = 0.55
    fx = x_wall + outward * flue[0] * 0.5 - outward * 0.02
    m.emit(kit._box((deep, width, shoulder)), Trim('D', world=True), 'trim', place((cx, y, base_top + shoulder * 0.5)),
           shape=lambda co: Vector((co.x * (1.0 - 0.3 * (co.z / shoulder + 0.5)) - outward * 0.12 * (co.z / shoulder + 0.5),
                                    co.y * (1.0 - 0.42 * (co.z / shoulder + 0.5)), co.z)))
    kit.stone_stack(m, (fx, y, base_top + shoulder - 0.1), flue, top - (base_top + shoulder - 0.1), taper=0.025,
                    rot=rng.uniform(-1.5, 1.5))
    flue_cap(m, fx, y, top, flue, socket)
    m.hull((deep, width, base_top + BELOW), at=(cx, y, (base_top - BELOW) * 0.5))
    m.hull((flue[0] + 0.04, flue[1] + 0.04, top - base_top + 0.3), at=(fx, y, (base_top + top + 0.3) * 0.5))


def stovepipe(m, x, y, z_from, top, radius=0.1, socket='Smoke', look='iron', lean=(0.0, 0.0), flashing=None):
    """A stovepipe from z_from up to top with a rain cap on three legs (Farmhouse.py's); lean (dx, dy) tilts its top
    off true; flashing (a Slope) lays a plate where it goes through that roof. Its smoke comes out under the cap. Black
    iron (IronBlack) by default: MetalRust's grey-green and orange read as loot colours on Ransom's Rest."""
    tile = Tile('MetalRust') if look == 'metal' else Tile('MetalWorn')
    mat = 'metal' if look == 'metal' else 'iron'
    tx, ty = x + lean[0], y + lean[1]
    m.cylinder((x, y, z_from), (tx, ty, top), radius, sides=10, uv=tile, mat=mat, caps=(False, True))
    m.cylinder((tx, ty, top - 0.35), (tx, ty, top - 0.28), radius * 1.25, sides=10, uv=tile, mat=mat)
    m.cylinder((tx, ty, top + 0.12), (tx, ty, top + 0.3), radius * 2.6, 0.03, sides=10, uv=tile, mat=mat)
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        m.box((0.02, 0.02, 0.16), at=(tx + radius * 0.9 * math.cos(a), ty + radius * 0.9 * math.sin(a), top + 0.06),
              uv=tile, mat=mat)
    if flashing is not None:
        slope, sx, sy = flashing
        m.box((radius * 5.0, radius * 5.5, 0.02), at=(sx, sy, 0.06), rot=(0.0, 0.0, m.rng.uniform(-4.0, 4.0)), uv='F',
              space=slope)
    if socket:
        m.socket(socket, (tx, ty, top + 0.08))


# --- Porches ---

def post(m, x, y, z0, z1, size=0.14, look='C', turned=False, cap=True):
    """A porch post from z0 to z1: a hewn square post with a base block and a cap, or a turned post (turned) in look."""
    if turned:
        h = z1 - z0
        town.box(m, (size + 0.02, size + 0.02, 0.28), at=(x, y, z0 + 0.14), look=look, drop=('-z',))
        # Closed at both ends (inside the blocks), so its faces turn outward.
        profile = [(0.0, 0.28), (size * 0.44, 0.28), (size * 0.3, 0.44), (size * 0.34, h * 0.48), (size * 0.48, h * 0.54),
                   (size * 0.3, h * 0.62), (size * 0.33, h - 0.4), (size * 0.44, h - 0.27), (0.0, h - 0.27)]
        town.lathe(m, profile, at=(x, y, z0), sides=6, look=look)
        town.box(m, (size + 0.02, size + 0.02, 0.26), at=(x, y, z1 - 0.13), look=look, drop=('+z',))
        return
    m.board((x, y, z0), (x, y, z1), size, size, uv=Trim('C', lane='each'))
    if cap:
        m.box((size + 0.06, size + 0.06, 0.05), at=(x, y, z0 + 0.025), uv='C', drop=('-z',))


def railing(m, p0, p1, height=0.9, every=0.14, look='C', picket='C', top_w=0.1, posts=False):
    """A railing from p0 to p1 (on the deck, z its top): a bottom rail, a top rail and flat pickets between."""
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    along = (p1 - p0).normalized()
    out = Vector((along.y, -along.x, 0.0))          # the railing's outside, right of p0 -> p1
    z = p0.z
    town.board(m, p0 + Vector((0.0, 0.0, 0.1)), p1 + Vector((0.0, 0.0, 0.1)), 0.06, 0.06, look=look, face=out,
               drop=('-x', '+x'))
    town.board(m, p0 + Vector((0.0, 0.0, height)), p1 + Vector((0.0, 0.0, height)), top_w, 0.05, look=look,
               face=(0.0, 0.0, 1.0), drop=('-x', '+x'))
    count = max(1, int(length / every))
    for k in range(count):
        q = p0 + along * (length * (k + 0.5) / count)
        town.board(m, (q.x, q.y, z + 0.13), (q.x, q.y, z + height - 0.025), 0.06, 0.022, look=picket, face=out,
                   drop=('-x', '+x'))


def steps(m, cx, width, y_edge, top, count, toward=-1.0, run=0.28, look='A', stringers=True):
    """Wooden steps down from a deck's edge at y_edge (top its floor) toward -y (toward=-1) or +y, count treads, each
    with its riser, between two stringers. Adds a hull per tread."""
    rise = top / (count + 1)
    for k in range(count):
        tz = top - rise * (k + 1)
        y = y_edge + toward * (run * (k + 0.5))
        town.board(m, (cx - width * 0.5, y, tz - 0.025), (cx + width * 0.5, y, tz - 0.025), run + 0.02, 0.05, look=look,
                   face=(0.0, 0.0, 1.0), drop=('-z',))
        ry = y + toward * run * 0.5
        rz = tz - rise * 0.5 - 0.02
        town.board(m, (cx - width * 0.5 + 0.02, ry, rz), (cx + width * 0.5 - 0.02, ry, rz), rise - 0.04, 0.025,
                   look=look, face=(0.0, toward, 0.0), drop=('+y',))
        y_lo = y_edge + toward * run * (k + 1)
        m.hull((width, run * (k + 1), tz), at=(cx, (y_edge + y_lo) * 0.5, tz * 0.5))
    if stringers:
        y_far = y_edge + toward * run * count
        for x in (cx - width * 0.5 - 0.03, cx + width * 0.5 + 0.03):
            town.board(m, (x, y_edge, top - 0.12), (x, y_far, 0.02), 0.22, 0.05, look='C',
                       face=(1.0 if x > cx else -1.0, 0.0, 0.0), drop=('+y',))


def stone_steps(m, cx, width, y_edge, top, count, toward=-1.0, run=0.32):
    """Fieldstone steps up to a door or a stoop, with their hulls."""
    rise = top / count
    for k in range(count):
        h = rise * (count - k)
        y0 = y_edge + toward * run * k
        y1 = y_edge + toward * run * (k + 1)
        kit.stone_stack(m, (cx, (y0 + y1) * 0.5 - toward * 0.02, -0.1), (width - 0.06 * k, abs(y1 - y0) + 0.04), h + 0.1,
                        bevel=0.02, rot=m.rng.uniform(-1.5, 1.5))
        m.hull((width - 0.06 * k, abs(y1 - y0), h), at=(cx, (y0 + y1) * 0.5, h * 0.5))


def shed_roof(m, x0, x1, y_low, z_low, y_high, z_high, overhang=0.3, rake=(0.2, 0.2), deck=0.07, sag=0.04,
              cls=kit.Slope, **options):
    """A shed (lean-to or porch) roof whose deck top runs from (y_low, z_low) up to (y_high, z_high), past y_low by
    overhang, along X from x0 to x1 plus rake at each end. Returns its Slope (covering not laid)."""
    toward = 1.0 if y_high > y_low else -1.0
    run = abs(y_high - y_low)
    theta = math.atan2(z_high - z_low, run)
    c, s = math.cos(theta), math.sin(theta)
    length = (run + overhang) / c
    width = x1 - x0 + rake[0] + rake[1]
    if toward > 0:
        origin = Vector((x0 - rake[0], y_low - overhang, z_low - overhang * math.tan(theta)))
        slope = cls(origin, (1.0, 0.0, 0.0), (0.0, c, s), width, length, sag, None, **options)
    else:
        origin = Vector((x1 + rake[1], y_low + overhang, z_low - overhang * math.tan(theta)))
        slope = cls(origin, (-1.0, 0.0, 0.0), (0.0, -c, s), width, length, sag, None, **options)
    town.roof_deck(m, slope, deck)
    slope.deck = deck
    slope.pitch = math.degrees(theta)
    return slope


def board_roof(m, slope, every=0.26, look='A', missing=(), battens=True, top=None):
    """A roof of boards running down the slope with a batten over every joint (a claim shack's): the boards lie on the
    deck's place (z = 0), each a little crooked; missing lists the boards (by number) that are gone."""
    rng = m.rng
    top = slope.length + 0.03 if top is None else top
    count = int(slope.width / every) + 1
    w = slope.width / count
    for k in range(count):
        if k in missing:
            continue
        x = w * (k + 0.5)
        y0 = -0.04 + rng.uniform(-0.02, 0.02)
        m.box((w - 0.012, top - y0, 0.025), matrix=place((x, (y0 + top) * 0.5, 0.0125 + rng.uniform(0.0, 0.004)),
                                                         (0.0, rng.uniform(-0.5, 0.5), rng.uniform(-0.15, 0.15))),
              uv=Trim(look, width=w), space=slope, ao_floor=kit.ROOF_AO, drop=('-z',), cuts=0)
        if battens and k < count - 1:
            m.box((0.07, top - 0.02, 0.025), at=(w * (k + 1), top * 0.5, 0.037), uv=Trim('C', width=0.07), space=slope,
                  ao_floor=kit.ROOF_AO, drop=('-z',))


def lapped_roof(m, slope, exposure=0.2, butt=0.025, piece=(1.6, 3.2), start=-0.06, top=None):
    """A roof of rough planks laid across the slope and lapped like siding (a shack's): rows from the eave up, each row
    long planks butted end to end (one of the trim sheet's weathered boards each), crooked and a little loose."""
    rng = m.rng
    top = slope.length + 0.02 if top is None else top
    h = exposure + 0.03
    tilt = -math.degrees(math.atan2(butt, h))
    y = start
    k = 0
    while y < top - 0.04:
        hh = min(h, top - y)
        x = -0.04 + rng.uniform(-0.8, 0.0)
        while x < slope.width + 0.04 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.04 - (x + w) < 0.5:
                w = slope.width + 0.04 - x
            a = max(x, -0.04)
            matrix = place(((a + x + w) * 0.5, y + hh * 0.5 + rng.uniform(-0.01, 0.01), butt * 0.5 + 0.004 * (k % 2)),
                           (tilt, rng.uniform(-0.6, 0.6), rng.uniform(-0.25, 0.25)))
            m.box((x + w - a, hh, butt), matrix=matrix, uv=Trim('A', width=hh), space=slope, ao_floor=kit.ROOF_AO,
                  drop=('-z', '+y'))
            x += w
        y += exposure
        k += 1


# --- Dressing ---

def woodpile(m, x_face, y0, y1, rows=4, depth=0.42, outward=-1.0, axis='x'):
    """Firewood stacked against a wall's foot between two stakes (Cottage.py's): rows of billets, sawn ends out, most of
    them split. x_face: the face it leans on (along x if axis is 'x': the pile runs along Y and its ends point toward
    outward along X); axis 'y' swaps the roles (a pile along X against a wall facing Y)."""
    rng = m.rng
    r = (y1 - y0) / 20.0

    def at(u, v, z):
        return (u, v, z) if axis == 'x' else (v, u, z)
    for row in range(rows):
        count = 10 - (1 if row % 2 else 0)
        start = y0 + r * (2 if row % 2 else 1)
        for k in range(count):
            if row == rows - 1 and rng.random() < 0.3:
                continue
            v = start + k * 2.0 * r + rng.uniform(-0.012, 0.012)
            z = r + row * r * 1.75 + rng.uniform(-0.01, 0.006)
            reach = depth + rng.uniform(-0.06, 0.03)
            kind = rng.random()
            if kind < 0.45:
                sides, radius = 3, r * 1.12
            elif kind < 0.75:
                sides, radius = 4, r * 0.98
            else:
                sides, radius = 6, r * rng.uniform(0.8, 0.95)
            m.cylinder(at(x_face + outward * 0.02, v, z), at(x_face + outward * reach, v, z + rng.uniform(-0.01, 0.01)),
                       radius, sides=sides, uv=Trim('C'), face=(0.0, -1.0, 0.0) if axis == 'x' else (-1.0, 0.0, 0.0),
                       phase=rng.uniform(0.0, 360.0), caps=(False, True), cap_uv=kit.EndGrain(), cap_mat='endgrain')
    for v in (y0 - 0.03, y1 + 0.03):
        m.board(at(x_face + outward * depth * 0.5, v, 0.0), at(x_face + outward * depth * 0.5, v, rows * r * 1.75 + 0.12),
                0.05, 0.05, face=(outward, 0.0, 0.0) if axis == 'x' else (0.0, outward, 0.0), uv='C')
    height = rows * r * 1.75 + 0.05
    cu, cv = x_face + outward * depth * 0.5, (y0 + y1) * 0.5
    size = (depth, y1 - y0 + 0.1, height) if axis == 'x' else (y1 - y0 + 0.1, depth, height)
    m.hull(size, at=at(cu, cv, height * 0.5))


def lathe(m, profile, at, sides, uv, mat='trim', space=None, smooth=True):
    """A turned part round a vertical axis through at: profile [(radius, z), ...] from the bottom up (a radius of 0
    closes an end), mapped with uv in its own frame."""
    tb = kit._new_bmesh()
    rings = []
    for r, z in profile:
        if r <= 1e-6:
            rings.append([tb.verts.new((0.0, 0.0, z))])
        else:
            rings.append([tb.verts.new((r * math.cos(2.0 * math.pi * k / sides), r * math.sin(2.0 * math.pi * k / sides),
                                        z)) for k in range(sides)])
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            try:
                if len(r0) == 1 and len(r1) > 1:
                    tb.faces.new((r0[0], r1[k1], r1[k]))
                elif len(r1) == 1 and len(r0) > 1:
                    tb.faces.new((r0[k], r0[k1], r1[0]))
                elif len(r0) > 1:
                    tb.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    m.emit(tb, uv, mat, Matrix.Translation(Vector(at)), space, smooth=smooth)


def barrel(m, at, height=0.9, radius=0.3, hull=True):
    """A rain barrel: upright staves of WoodPlanks round a slight belly, two iron hoops and a lid sunk in its top."""
    x, y, z = at
    lathe(m, [(0.0, 0.0), (radius * 0.9, 0.0), (radius, height * 0.5), (radius * 0.9, height),
              (radius * 0.8, height - 0.03), (0.0, height - 0.03)], (x, y, z), 10, Boards(rotate=True), 'planks')
    for hz in (height * 0.2, height * 0.8):
        r = radius * (0.93 + 0.07 * math.sin(math.pi * hz / height))
        town.cylinder(m, (x, y, z + hz - 0.025), (x, y, z + hz + 0.025), r + 0.008, sides=10, look='iron',
                      caps=(False, False), face=(0.0, 0.0, 1.0))
    if hull:
        m.hull((radius * 2.0, radius * 2.0, height), at=(x, y, z + height * 0.5))


def bench(m, cx, y_back, length=1.3, seat=0.44, deep=0.32, facing=-1.0):
    """A plank bench against a wall (its back edge at y_back, facing -y or +y)."""
    rng = m.rng
    cy = y_back + facing * deep * 0.5
    for dy in (-0.075, 0.075):
        town.board(m, (cx - length * 0.5, cy + dy, seat - 0.025), (cx + length * 0.5, cy + dy + rng.uniform(-0.004, 0.004),
                                                                   seat - 0.025), 0.15, 0.045, look='A',
                   face=(0.0, 0.0, 1.0))
    for x in (cx - length * 0.5 + 0.16, cx + length * 0.5 - 0.16):
        m.box((0.05, deep - 0.04, seat - 0.05), at=(x, cy, (seat - 0.05) * 0.5), uv=Trim('C', lane='each'))
    m.hull((length, deep, seat), at=(cx, cy, seat * 0.5))


# --- Previews: Ransom's Rest's golden late afternoon (Farmhouse.py's light and sky) ---

GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]
SUN_COLOR, SUN_STRENGTH = (1.0, 0.8, 0.56), 4.4


def sky(name, stops, sun, glows):
    """A painted sky (the posters' and Farmhouse.py's): colours by the view's height, glows round the sun."""
    world = bpy.data.worlds.get(name) or bpy.data.worlds.new(name)
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
    elements[0].position, elements[0].color = stops[0][0], lt.hex_color(stops[0][1])
    elements[1].position, elements[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun).normalized()
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in glows:
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


def figure():
    """A 1.8 m clay figure for scale, standing at its origin facing -Y (Backlots.py's)."""
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


def _ground_material():
    """The renders' ground (never exported): the street's dirt with a slow, broad mottling over it, so its tiling
    doesn't print a quilt across the far ground."""
    mat = lt.material('GroundDirt', name='_PreviewDirt')
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    color_in = bsdf.inputs['Base Color']
    source = color_in.links[0].from_socket if color_in.links else None
    coords = nodes.new('ShaderNodeTexCoord')
    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 0.035
    noise.inputs['Detail'].default_value = 3.0
    links.new(coords.outputs['Object'], noise.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = 0.3, (0.78, 0.74, 0.66, 1.0)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = 0.7, (1.08, 1.04, 0.96, 1.0)
    links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.blend_type = 'MULTIPLY'
    lt._socket(mix.inputs, 'Factor_Float').default_value = 1.0
    if source is not None:
        links.new(source, lt._socket(mix.inputs, 'A_Color'))
    links.new(ramp.outputs['Color'], lt._socket(mix.inputs, 'B_Color'))
    links.new(lt._socket(mix.outputs, 'Result_Color'), color_in)
    return mat


def stage(samples=64):
    """The renders' set: Eevee as the posters render (ray-traced screen-space light, AgX), the street's dirt
    underfoot, the hulls hidden, glowing glass glowing. Returns the scene and its sun."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = samples
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    if hasattr(ee, 'shadow_resolution_scale'):
        ee.shadow_resolution_scale = 1.0
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
            bsdf.inputs['Emission Strength'].default_value = float(mat['Glow'])
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-400.0, -400.0, 0.0), (400.0, -400.0, 0.0), (400.0, 400.0, 0.0), (-400.0, 400.0, 0.0)], [],
                     [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    lt.assign(ground, _ground_material())
    lt.box_uv(ground, 'GroundDirt', texel_density=160)
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    return scene, sun


def light(scene, sun, toward, strength=SUN_STRENGTH, color=SUN_COLOR):
    """The low golden sun (toward it) and its sky."""
    sun.data.energy, sun.data.color, sun.data.angle = strength, color, math.radians(1.5)
    sun.rotation_euler = (-Vector(toward).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.world = sky('_Golden', GOLDEN_SKY, toward, GOLDEN_GLOWS)


def shoot(scene, path, eye, target, lens, size, exposure=0.0):
    """Renders from eye toward target into path (PNG)."""
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.05, 3000.0
    cam.data.sensor_fit = 'HORIZONTAL'
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    lt._log(f'homes: rendered {path}')
    return path


def points_of(objs, step=1):
    """The objects' vertices in the world (every step-th), for framing a shot on what's really there."""
    pts = []
    for o in objs:
        if o.type == 'MESH':
            pts += [o.matrix_world @ v.co for v in list(o.data.vertices)[::step]]
        else:
            pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
    return pts


def frame_shot(objs, direction, size, lens, margin=1.08, height_bias=0.42):
    """An eye and a target framing objs from direction (toward the camera) at lens and size: the camera backs off until
    every vertex is inside the frame (with margin)."""
    pts = points_of(objs)
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    target = Vector(((lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, lo.z + (hi.z - lo.z) * height_bias))
    d = Vector(direction).normalized()
    sensor = 36.0
    tan_x = sensor * 0.5 / lens
    tan_y = tan_x * size[1] / size[0]
    forward = -d
    right = forward.cross(UP).normalized()
    up = right.cross(forward).normalized()
    dist = 1.0
    for p in pts:
        rel = p - target
        x, y, z = rel.dot(right), rel.dot(up), rel.dot(forward)
        # The distance at which this corner sits at the frame's edge: |x| / (dist + z) <= tan.
        dist = max(dist, abs(x) * margin / tan_x - z, abs(y) * margin / tan_y - z)
    return target + d * dist, target


def golden_sun(azimuth_deg, elevation_deg):
    """Toward the sun in the model's frame: azimuth from -Y (the front) toward +X, elevation above the ground."""
    a, e = math.radians(azimuth_deg), math.radians(elevation_deg)
    return Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e)))


VIEW = (-0.62, -1.0, 0.36)                    # the three-quarter front view, from the front-left
SUN = (38.0, 16.0)                            # the low sun: from the front-right, 16 degrees up
SHOT, ROW_SHOT = (1600, 1000), (2400, 1100)


def show(obj, on):
    """Shows or hides a model in the renders, with what's mounted on it for them (children named '_Mount...': a
    windmill's fan on its socket)."""
    obj.hide_render = not on
    for child in obj.children:
        if child.name.startswith('_Mount'):
            child.hide_render = not on


def mounted(obj):
    return [obj] + [c for c in obj.children if c.name.startswith('_Mount')]


def mount(model, part, socket_name):
    """A copy of part (a model of its own: a fan) hung on model's socket for the renders, as the game hangs it. It's
    named '_Mount...' so no export ever takes it, and it moves and hides with model."""
    socket = next(c for c in model.children if c.name.startswith('SOCKET_' + socket_name))
    copy = part.copy()
    copy.name = '_Mount_' + part.name
    bpy.context.scene.collection.objects.link(copy)
    copy.parent = model
    copy.matrix_parent_inverse = Matrix.Identity(4)
    copy.matrix_basis = socket.matrix_basis.copy()
    for child in list(copy.children):
        child.parent = None
    return copy


def _bounds(obj, local=False):
    cos = [v.co.copy() if local else obj.matrix_world @ v.co for v in obj.data.vertices]
    return (Vector((min(c.x for c in cos), min(c.y for c in cos), min(c.z for c in cos))),
            Vector((max(c.x for c in cos), max(c.y for c in cos), max(c.z for c in cos))))


def previews(models, captions, sheet=True, exposure=-0.15, shots_by_name=None, rows=None):
    """Each home in Ransom's Rest's golden late afternoon from the front three-quarter, with a 1.8 m figure beside it
    (Saved/ArtPreviews/RansomsRest/Homes/<name>.png); with sheet, the kit at one scale (rows: lists of names, back row
    first) and the look sheet too. shots_by_name: per home, a dict of view (toward the camera), sun (azimuth,
    elevation), figure ('left' or 'right' of the front) and exposure, over the defaults."""
    shots_by_name = shots_by_name or {}
    scene, sun = stage()
    human = figure()
    bpy.context.view_layer.update()          # the mounted parts' places in the world, for framing
    for o in models:
        show(o, False)
    shots = []
    for obj in models:
        opts = shots_by_name.get(obj.name, {})
        light(scene, sun, golden_sun(*opts.get('sun', SUN)))
        show(obj, True)
        lo, hi = _bounds(obj)
        if opts.get('figure', 'right') == 'right':
            human.location = (hi.x + 0.8, lo.y + 0.9, 0.0)
        else:
            human.location = (lo.x - 0.8, lo.y + 0.9, 0.0)
        human.hide_render = False
        lens = opts.get('lens', 35.0)
        eye, target = frame_shot(mounted(obj) + [human], opts.get('view', VIEW), SHOT, lens,
                                 margin=opts.get('margin', 1.04), height_bias=opts.get('height_bias', 0.42))
        shots.append(shoot(scene, os.path.join(PREVIEW_DIR, obj.name + '.png'), eye, target, lens, SHOT,
                           opts.get('exposure', exposure)))
        show(obj, False)
    if not sheet:
        return shots
    # The kit at one scale: rows of houses, fronts on a line, the big ones behind.
    light(scene, sun, golden_sun(*SUN))
    by_name = {o.name: o for o in models}
    rows = rows or [[o.name for o in models]]
    placed = []
    depth = 0.0
    for k, row in enumerate(rows):
        objs = [by_name[n] for n in row if n in by_name]
        if not objs:
            continue
        boxes = [_bounds(o, local=True) for o in objs]
        if k:
            depth = (min(o.location.y + _bounds(o, local=True)[0].y for o in placed) - max(hi.y for lo, hi in boxes)
                     - 6.0)
        x = -(sum(hi.x - lo.x for lo, hi in boxes) + 2.2 * (len(objs) - 1)) * 0.5
        for obj, (lo, hi) in zip(objs, boxes):
            obj.location = (x - lo.x, depth, 0.0)
            x += hi.x - lo.x + 2.2
            show(obj, True)
            placed.append(obj)
    human.location = (x - 1.0, depth - 1.0, 0.0)
    bpy.context.view_layer.update()
    framed = [part for obj in placed for part in mounted(obj)] + [human]
    eye, target = frame_shot(framed, (-0.18, -1.0, 0.62), ROW_SHOT, 40.0, margin=1.02, height_bias=0.3)
    row_png = shoot(scene, os.path.join(PREVIEW_DIR, 'Homes_row.png'), eye, target, 40.0, ROW_SHOT, exposure)
    for obj in models:
        obj.location = (0.0, 0.0, 0.0)
    row = row_png
    tiles = []
    for obj, png in zip(models, shots):
        info = REPORTS.get(obj.name, {})
        tris = info.get('triangles', 0)
        tiles.append((png, f'{obj.name}  {tris:,} tris', captions.get(obj.name, '')))
    total = sum(info.get('triangles', 0) for info in REPORTS.values())
    look_sheet(os.path.join(SHEET_DIR, '35_Homes.jpg'), "Ransom's Rest: frontier homes",
               "Ten houses of the valley's own, each built once: five lived in, five left when the\n"
               "saint went dark; and the Rest's own well, privy and windpump. Blender previews in\n"
               "the area's late-afternoon sun; nothing imported yet.",
               (row, f'The whole kit at one scale (the figure is 1.8 m): {len(REPORTS)} models,\n'
                     f'{total:,} triangles in all.'), tiles)
    return shots


# --- The look sheet (composed in Blender: the renders on emissive planes and the labels as text, shot orthographic) ---

def _image_plane(name, path, x, y, w, h, z=0.0):
    img = bpy.data.images.load(path, check_existing=False)
    mat = bpy.data.materials.new('_Sheet_' + name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Cubic'
    links.new(tex.outputs['Color'], emit.inputs['Color'])
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    mesh = bpy.data.meshes.new('_Sheet_' + name)
    mesh.from_pydata([(x, -y - h, z), (x + w, -y - h, z), (x + w, -y, z), (x, -y, z)], [], [(0, 1, 2, 3)])
    uv = mesh.uv_layers.new(name='UVMap')
    for loop, coord in zip(uv.data, ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))):
        loop.uv = coord
    mesh.materials.append(mat)
    obj = bpy.data.objects.new('_Sheet_' + name, mesh)
    return obj


def _rect(name, x, y, w, h, color, z=-0.5):
    mat = bpy.data.materials.new('_Sheet_' + name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(color)
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    mesh = bpy.data.meshes.new('_Sheet_' + name)
    mesh.from_pydata([(x, -y - h, z), (x + w, -y - h, z), (x + w, -y, z), (x, -y, z)], [], [(0, 1, 2, 3)])
    mesh.materials.append(mat)
    return bpy.data.objects.new('_Sheet_' + name, mesh)


def _text(name, body, x, y, size, color, bold=True, align='LEFT', z=0.5):
    curve = bpy.data.curves.new('_Sheet_' + name, 'FONT')
    curve.body = body
    path = FONT if bold else FONT_REGULAR
    try:
        curve.font = bpy.data.fonts.load(path, check_existing=True)
    except Exception:
        pass
    curve.size = size
    curve.align_x = align
    curve.align_y = 'TOP_BASELINE'
    mat = bpy.data.materials.new('_Sheet_' + name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs['Color'].default_value = lt.hex_color(color)
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    curve.materials.append(mat)
    obj = bpy.data.objects.new('_Sheet_' + name, curve)
    obj.location = (x, -y, z)
    return obj


def look_sheet(out_jpg, title, subtitle, banner, tiles, width=2000, columns=2, gap=18, pad=40):
    """Composes the look sheet: a title, a subtitle, a banner render across the top (banner: (png, caption)), then
    tiles (png, label, caption) in columns, every label large enough to read on a phone, on the sheets' dark ground.
    Rendered with Eevee at 1 unit = 1 px in a scene of its own and saved as JPEG."""
    from math import ceil
    scene = bpy.data.scenes.new('_Sheet')
    objs = []
    y = pad
    objs.append(_text('title', title, pad, y + 72, 84, 0xf2a64a))
    y += 100
    for k, line in enumerate(subtitle.split('\n')):
        objs.append(_text(f'subtitle{k}', line, pad, y + 40, 40, 0xc9c4ba, bold=False))
        y += 52
    y += 18
    inner = width - 2 * pad
    if banner:
        png, caption = banner
        img = bpy.data.images.load(png, check_existing=True)
        bw, bh = img.size
        h = inner * bh / bw
        objs.append(_image_plane('banner', png, pad, y, inner, h))
        y += h + 14
        for k, line in enumerate(caption.split('\n')):
            objs.append(_text(f'banner_caption{k}', line, pad, y + 40, 40, 0xe8e2d6, bold=False))
            y += 52
        y += 22
    tile_w = (inner - gap * (columns - 1)) / columns
    rows = ceil(len(tiles) / columns)
    for r in range(rows):
        row_h = 0.0
        for c in range(columns):
            k = r * columns + c
            if k >= len(tiles):
                break
            png, label, caption = tiles[k]
            img = bpy.data.images.load(png, check_existing=True)
            iw, ih = img.size
            th = tile_w * ih / iw
            x = pad + c * (tile_w + gap)
            # The label over the render (never on it: a fan or a sweep pole reaches the render's top edge).
            objs.append(_text(f'label{k}', label, x, y + 56, 58, 0xf2a64a, z=0.6))
            objs.append(_image_plane(f'tile{k}', png, x, y + 78, tile_w, th))
            lines = caption.split('\n')
            for j, line in enumerate(lines):
                objs.append(_text(f'cap{k}_{j}', line, x, y + 78 + th + 46 + 48 * j, 38, 0xe8e2d6, bold=False))
            row_h = max(row_h, 78 + th + 30 + 48 * len(lines))
        y += row_h + 34
    height = int(y + pad - 10)
    objs.append(_rect('ground', 0, 0, width, height, 0x16171a))
    # Label backing: a dark band behind each tile label so it reads on any render.
    for o in objs:
        scene.collection.objects.link(o)
    cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = width
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.location = (width * 0.5, -height * 0.5, 100.0)
    cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 16
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'None'
    scene.render.film_transparent = False
    world = bpy.data.worlds.new('_SheetWorld')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0x16171a)
    scene.world = world
    scene.render.image_settings.file_format = 'JPEG'
    scene.render.image_settings.quality = 90
    scene.render.image_settings.color_mode = 'RGB'
    os.makedirs(os.path.dirname(out_jpg), exist_ok=True)
    scene.render.filepath = out_jpg
    bpy.ops.render.render(write_still=True, scene=scene.name)
    lt._log(f'homes: look sheet {out_jpg} ({width} x {height})')
    return out_jpg
