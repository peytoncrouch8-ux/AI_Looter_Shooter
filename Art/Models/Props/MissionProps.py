"""Ransom's Rest's small mission props (Docs/Areas/RansomsRest_Expansion.md, sections 3 and 4; the wave 4 contract): the
things the missions send the player to take, read or use. A scripted model file (Art/README.md); Tools/models.ps1 exports
it into /Game/Art/Props (SM_<name>). They glow orange when they're an objective (UObjectiveHighlightSubsystem draws the
overlay on their own mesh), so each has one clear silhouette, readable at a glance.

Pivots: a thing that lies about has its pivot in the middle of its footprint on the surface it lies on (a dropped mission
item lands with its actor on the ground: AMissionItem::SpawnDrop), front -Y. Things that hang have it at the point that
hangs on the hook (CellarKey, MinersTag), the thing hanging plumb below. Sizes are width x depth x height (X, Y, Z).

  Mission items (AMissionItem, Item_<Id>):
  LanternBail       the Keeper's Lantern's bail (BurialDeck.py's keepers_lantern: the same iron wire and turned grip),
                    snapped off at its ears in Abel's fist: one end's hook still holds the torn brass ear, the other
                    is ripped open. Lying on its side. SOCKET_Grip at the grip's middle (its X along the grip).
  WinchCrank        the ore lift's winch crank (Dead Lift): a forged octagonal boss round the square seat that fits the
                    winch's pinion (OreLift.py's SOCKET_Crank seat), a tapered arm, a turned wooden grip, and the
                    sheriff's impound tag tied on with twine. Resting on its boss and the arm's end, the grip up.
                    SOCKET_Fit at the seat's mouth (its front, -Y, points out of the bore along the shaft).
  CoalSack          a canvas sack (the coal stage's: DepotRR.py's Canvas and PaintBlack coal) filled with railroad coal,
                    its neck tied with hemp, lumps heaped in its mouth. SOCKET_Grip at the tie.
  ClampPin          the sheave's clamp pin (Dead Lift; Abe Haskett wore it on a cord round his neck): an iron T-headed
                    pin with its cotter, the cord wound round it and lying in a loop. SOCKET_Fit at its tip (its front
                    points along the pin, the way it goes in).
  CellarKey         Thaddeus Bright's cellar key: a big iron key on a hemp cord loop, a brass fob stamped CELLAR. Pivot:
                    the top of the cord loop (it hangs on WorkshopBright.py's SOCKET_MeasuringHook), the key plumb below,
                    its face to -Y.
  TapeMeasure       Thaddeus's tape: a leather reel in brass rims, its folding crank on top, the linen tape run out and
                    curled on the ground to its brass end ring. Pivot: the reel's middle on the ground.
  GraveCoins        Fenn's fare: two old gold coins (a worn profile; a sun with rays), 6.4 cm apart centre to centre, as
                    they lie on a dead man's eyes, so the same model goes on FennCoffin's SOCKET_Eyes.
  GravemotherFang   her fang, 0.4 m: a curved black fang on the stump of its bone-white chelicera, torn off at its root,
                    bristles. Lying on its side.
  LampOil           a coal-oil can (Calder's Post): oxide-red tin, a cream label band lettered COAL OIL, the spout's cap,
                    a wire handle with a wooden grip. SOCKET_Grip at the grip.
  CacheTag          a Rim Rangers' cache tag (CacheTag1..3, one model): a tin tag with a brass grommet, the Rangers'
                    star in a ring painted on it in cream, a twine loop.
  PayrollChest      the Providence's pay chest (Back Pay; Mine.py's SOCKET_PayrollChest in the stope): an iron-strapped
                    plank chest, corner irons, iron drop handles, a hasp and brass padlock, P.M.CO. and PAYROLL stencilled
                    on its front. One closed model (not the Strongbox: that's a Rare loot chest, and its blue lights are
                    the game's rarity code). Pivot: the middle of its foot.

  Readables (AReadableProp) and artifacts:
  SheriffBadge      Wade Odell's six-pointed star, ball-tipped, SHERIFF in black on its raised middle; one point bent up
                    and its ball chewed off. Lying face up.
  SheriffHat        his hat, chewed: black felt, a cattleman's crease, an oxblood band and buckle, three bites out of its
                    brim (the big one toward the front left) and a crushed dent in its crown. Lying crown up, tilted.
  Journal           Ruth Calder's journal: oxblood leather, wrapped twice with its thong, a pencil on it, a red ribbon
                    marker, R.C. stamped on its cover. 14 x 20 x 3 cm (the cabin's baked book's size).
  CodeBook          Ruth's signal code book: small and thick, blue cloth, leather spine and corners, a brass clasp, the
                    Rangers' star and SIGNALS in gilt, a folded signal card sticking out of it.
  PostLog           Calder's Post's log, open: red cloth boards, two curved page blocks ruled in red with inked entries
                    (the right page half empty, for the next name), a pencil on a string.
  PayrollBook       the Foreman's payroll ledger, closed: black cloth boards, an oxblood leather spine with raised bands
                    and corners, a paper label PAYROLL / P.M.CO., two pay slips sticking out.
  DeaconsSlip       the Deacon's slip (in the gang's Strongbox, Calder's Caches): a sheet folded once, half open like a
                    tent, his handwriting inside.
  KeepersRoll       the keepers' roll in the chapel's nave: a board of black-painted planks in a frame under an arched
                    crest with the keepers' sun, KEEPERS / OF SAINT ADA and the names in gilt, oldest first: ROLL_NAMES,
                    then ABEL, and under him ELLIS painted out in fresher black brush strokes the letters still show
                    through. Pivot: the middle of its back's bottom edge (hang it on the wall, its bottom about 1 m up),
                    front -Y.

  The seeds (2026-10-16, Saved/Seeds/CONTRACT.md; readables placed by build_area_seeds.py):
  LedgerPage        a loose leaf of Sexton's ledger, 20 x 32 cm (long side along Y, Unreal's X), lying flat: ruled in
                    blue, a red header and date and money columns, his copperplate as scrawl ribbons (a heading, ten
                    entries with dates and sums, a slantwise note in the margin), the far right corner lifted 2.8 cm.
                    One model for all six pages.
  DeaconLetter      the Deacon's letter, folded in three and sealed, 11 x 18 cm (long side along Y): the outer flap
                    over the packet, the address in his hand on it, red wax on the flap's edge. The letter in the
                    mooring cleft and the one in the saloon's post rack. Pivot of both: the middle of the footprint.
  PostRack          the saloon's post rack, 60 x 42 x 7.5 cm, pine: eight pigeonholes in two rows behind slats painted
                    1-8, nailed to the wall; two old letters baked into slots 6 and 8. Sockets Slot_1..Slot_8 (1-4 the
                    upper row left to right): a standing letter's bottom middle on the slot's floor, +Z up the slot
                    (leaning back 8 degrees), front out of the rack. Pivot: the middle of its back face.
  ReachesMap        the depot's wall map, 90 x 60 cm: one sheet with its own texture (MI_ReachesMap, set ReachesMap,
                    painted by Art/Textures/ReachesMap/make_reaches_map.py), its back and edges SextonLedgerPages, a
                    bulge between the four brass pins and its bottom edge curling out. Pivot: the middle of its back.

  Interactables with their own tag:
  FennCoffin        Josiah Fenn's pine shipping coffin in the depot's baggage room (SM_RR_Depot's SOCKET_BaggageCoffin:
                    its pivot is the middle of its foot, long along X, the head at +X): six-sided, GRAVESEND stencilled
                    on its lid, J. FENN on its front, FARE PAID chalked beside it, rope handles in wooden cleats, a
                    shipping tag on the head handle. A couch lid: the foot part is nailed down, the head part
                    (FennCoffin_Lid) is hinged along its back edge. Inside under the head lid: a linen lining, a
                    pillow, Fenn under his shroud with a face cloth over his head. Sockets: Lid (the hinge), Eyes (where
                    GraveCoins go, its X across his face), Speaker (Fenn's voice), Interact.
  FennCoffin_Lid    the head lid: its origin is on its hinge's axis (attach it at SOCKET_Lid); a positive pitch in
                    Unreal (about the socket's Y, Blender's X) raises its free front edge: about 100 degrees open.

  SM_PorchPlate already exists (Art/Models/Buildings/Farmhouse.py: /Game/Art/Buildings/SM_PorchPlate), so it isn't here.

Every model carries SOCKET_Interact where the player uses it (its front, -Y, toward the player). Collision: one simple box
round each (a little generous on the tiny ones, so the Interact key's line finds them); the item and readable actors
answer only the Visibility channel on it, so the player walks through them. FennCoffin_Lid has none. Small props: no
Nanite; the bigger ones (700 triangles and up) LODs at 50% and 25%, but not the coins, the badge and the cache tag (a
reduction would eat their tiny relief and lettering).

Materials: only the shared ones the game already has (MI_<name>), each made exactly as its first family makes it:
IronBlack, BrassWorn, AmmoCaseBrass (the gold), MailboxTin, PaintBlack, PaintCream, PaintOxide, WoodBlack, WoodCream,
MourningCrepe, Canvas, RopeHemp, BoneDry, ClothLinen, ClothBlue, ClothRed, SextonLedgerCover (leather),
SextonLedgerPages (paper) and HouseTrim (turned wooden grips on its beam strip); WoodPlanks (the post rack's pine) and the
map's own ReachesMap.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\MissionProps.py -Preview     every model, its render and the sheet
                                      (Saved/ArtPreviews/RansomsRest/MissionProps/, LookSheets/42_MissionProps.jpg);
                                      --only=Name,Name builds just those
    Tools\\artrun.ps1 -Export MissionProps -Family RR_MissionProps          export test
Everything random comes from seeded random.Random and mathutils noise, nothing iterates a set of mesh elements, and the
lumps are built here (bmesh's UV sphere puts its faces in a different order on every run), so every export is the same.
"""
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_homes as homes
import looter_model as lm
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt
import looter_town as town
from looter_buildings import Space

ARGS = kit._args()
PREVIEW = lt.want_preview()
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
UP = Vector((0.0, 0.0, 1.0))
SEED = 4242
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'MissionProps')
SHEET = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '42_MissionProps.jpg')
REPORT = {}
# The keepers before Abel on the roll, oldest first (the story names none yet: the user may rename them; the readable's
# text should list the same).
ROLL_NAMES = ('SILAS', 'HANNAH', 'JONAS')


# --- Materials: the shared names, made exactly as their first families make them ---

MAKERS = {
    'IronBlack': lambda: lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a),
    'BrassWorn': lambda: lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56),
    'AmmoCaseBrass': lambda: lt.material('MetalWorn', name='AmmoCaseBrass', tint=0xf4c870),
    'MailboxTin': lambda: lt.material('MetalWorn', name='MailboxTin', tint=0xc9c6bd),
    'PaintBlack': lambda: lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622),
    'PaintCream': lambda: lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2),
    'PaintOxide': lambda: lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c),
    'WoodBlack': lambda: lt.material('WoodPlanks', name='WoodBlack', tint=0x48423c),
    'WoodCream': lambda: lt.material('WoodPlanks', name='WoodCream', tint=0xfffcf6),
    'MourningCrepe': lambda: lt.material('Polymer', name='MourningCrepe', tint=0x161518),
    'Canvas': lambda: lt.material('Polymer', name='Canvas', tint=0xd6c9a8),
    'RopeHemp': lambda: lt.material('Hay', name='RopeHemp', tint=0x9a7a50),
    'BoneDry': lambda: lt.material('Polymer', name='BoneDry', tint=0xe6d9bf, RoughnessOffset=0.3, DiffuseAO=0.85),
    'ClothLinen': lambda: lt.material('Polymer', name='ClothLinen', tint=0xe4ddcc),
    'ClothBlue': lambda: lt.material('Polymer', name='ClothBlue', tint=0x7d8fa0),
    'ClothRed': lambda: lt.material('Polymer', name='ClothRed', tint=0xa65a4b),
    'SextonLedgerCover': lambda: lt.material('Polymer', name='SextonLedgerCover', tint=0x6E3226),
    'SextonLedgerPages': lambda: lt.material('Polymer', name='SextonLedgerPages', tint=0xE9DFC6),
    'HouseTrim': lambda: lt.material('HouseTrim'),
    # The Reaches map's own: its painted sheet (Art/Textures/ReachesMap/make_reaches_map.py) on the World master.
    'ReachesMap': lambda: lt.material('ReachesMap', name='ReachesMap', master='World'),
}
# The texture set each is mapped for.
SETS = {'IronBlack': 'MetalWorn', 'BrassWorn': 'MetalWorn', 'AmmoCaseBrass': 'MetalWorn', 'MailboxTin': 'MetalWorn',
        'PaintBlack': 'PaintWorn', 'PaintCream': 'PaintWorn', 'PaintOxide': 'PaintWorn', 'WoodBlack': 'WoodPlanks',
        'WoodCream': 'WoodPlanks', 'MourningCrepe': 'Polymer', 'Canvas': 'Polymer', 'RopeHemp': 'Hay',
        'BoneDry': 'Polymer', 'ClothLinen': 'Polymer', 'ClothBlue': 'Polymer', 'ClothRed': 'Polymer',
        'SextonLedgerCover': 'Polymer', 'SextonLedgerPages': 'Polymer', 'HouseTrim': 'HouseTrim',
        'ReachesMap': 'ReachesMap'}
# Metal and paint on small things: their wear at a finer scale than on a building (texels per metre).
FINE = 1280.0


def mat(name):
    found = bpy.data.materials.get(name)
    if found is None or found.get('Master') is None:
        found = MAKERS[name]()
    return found


class Model(lr.Model):
    """lr.Model (looter_ruins: kit parts, lp parts merged with add()) that knows these shared names as keys too, and
    finishes a small prop: its own sliver threshold (tiny lettering stays), smoothing, baked occlusion, hulls, sockets."""

    def slot(self, key):
        if key not in self.slots:
            if key in MAKERS:
                found = mat(key)
            elif key in lr._MAKERS:
                found = lr.material(key)
            elif key in town.MATERIALS:
                found = town.material(key)
            else:
                found = bpy.data.materials[key]
            self.slots[key] = (self._index_of(found), found)
        return self.slots[key][0]

    def add_all(self, parts, smooth=None):
        for part in parts:
            self.add(part, smooth=smooth)

    def done(self, pivot, ao=0.1, ground=True, smooth=40.0, min_height=0.0015, crumb=0.008, lods=None, **props):
        bpy.data.objects.remove(self._obj)
        bpy.data.meshes.remove(self._mesh)
        mesh = bpy.data.meshes.new(self.name)
        lr.triangulate_clean(self.bm, min_height=min_height, crumb=crumb)
        self.bm.to_mesh(mesh)
        self.bm.free()
        by_index = {index: material for index, material in self.slots.values()}
        for index in sorted(by_index):
            mesh.materials.append(by_index[index])
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
        obj['Nanite'] = 0
        if lods if lods is not None else triangles >= 700:
            obj['LODs'] = '50,25'
        for key, value in props.items():
            obj[key] = value
        lm.smooth(obj, smooth)
        if '--no-ao' not in ARGS:
            lt.bake_vertex_ao(obj, ground=ground, distance=ao)
        if '_aofloor' in mesh.attributes:
            mesh.attributes.remove(mesh.attributes['_aofloor'])
        for points in self.hulls:
            hb = bmesh.new()
            verts = [hb.verts.new(p) for p in points]
            result = bmesh.ops.convex_hull(hb, input=verts)
            inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
            bmesh.ops.delete(hb, geom=list(dict.fromkeys(inside)), context='VERTS')
            hull_mesh = bpy.data.meshes.new('UCX_' + self.name)
            hb.to_mesh(hull_mesh)
            hb.free()
            hull = bpy.data.objects.new('UCX_' + self.name, hull_mesh)
            bpy.context.scene.collection.objects.link(hull)
            hull.parent = obj
            hull.display_type = 'WIRE'
        for name, location, rotation in self.sockets:
            lm.socket(obj, name, location, rotation)
        report(obj, pivot)
        return obj


# --- Reporting ---

def socket_list(obj):
    out = []
    for child in sorted(obj.children, key=lambda o: o.name):
        if child.name.startswith('SOCKET_'):
            name = child.name[7:].split('.')[0]
            loc = tuple(round(c, 4) for c in child.location)
            rot = tuple(round(math.degrees(a), 1) for a in child.rotation_euler)
            out.append((name, loc, rot))
    return out


def report(obj, pivot):
    mesh = obj.data
    triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
    co = [v.co for v in mesh.vertices]
    lo = Vector((min(c.x for c in co), min(c.y for c in co), min(c.z for c in co)))
    hi = Vector((max(c.x for c in co), max(c.y for c in co), max(c.z for c in co)))
    size = hi - lo
    sockets = socket_list(obj)
    hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
    REPORT[obj.name] = dict(triangles=triangles, materials=[m.name for m in mesh.materials],
                            size_cm=[round(s * 100.0, 1) for s in size], bounds_m=[list(map(lambda v: round(v, 4), lo)),
                                                                                    list(map(lambda v: round(v, 4), hi))],
                            sockets=[dict(name=n, location_m=l, rotation_deg=r) for n, l, r in sockets], hulls=hulls,
                            pivot=pivot, nanite=int(obj.get('Nanite', 1)), lods=obj.get('LODs', ''))
    lt._log(f"MISSIONPROPS {obj.name}: {triangles} triangles, {len(mesh.materials)} slots "
            f"({', '.join(m.name for m in mesh.materials)}), {size.x * 100:.1f} x {size.y * 100:.1f} x {size.z * 100:.1f} cm, "
            f"{hulls} hull(s), sockets " + '; '.join(f'{n} {l} rot {r}' for n, l, r in sockets))


# --- Part helpers (parts are lp-style objects, their mesh already in model space) ---

def paint(part, name, set_name=None, density=None, seed=0, uv=True):
    """Makes name the part's only material and (uv) maps it at world scale on its set (density: texels per metre)."""
    mesh = part.data
    mesh.materials.clear()
    mesh.materials.append(mat(name))
    for p in mesh.polygons:
        p.material_index = 0
    if uv:
        lt.box_uv(part, set_name or SETS[name], texel_density=density, seed=seed)
    return part


def paint_faces(part, names, density=None, seed=0):
    """Gives each face its material (names: one per face) and maps each material's faces on its set."""
    mesh = part.data
    order = sorted(set(names))
    mesh.materials.clear()
    for name in order:
        mesh.materials.append(mat(name))
    for i, p in enumerate(mesh.polygons):
        p.material_index = order.index(names[i])
    for k, name in enumerate(order):
        faces = [i for i, n in enumerate(names) if n == name]
        lt.box_uv(part, SETS[name], texel_density=density if SETS[name] in ('MetalWorn', 'PaintWorn') else None,
                  faces=faces, seed=seed + k)
    return part


def scale_uvs(part, k, faces=None):
    uv = part.data.uv_layers.active.data
    polys = part.data.polygons
    for i in (faces if faces is not None else range(len(polys))):
        for li in polys[i].loop_indices:
            uv[li].uv = uv[li].uv * k


def lathe(profile, segments, name, strip='Beams', density=None, grain='around', seed=0, at=(0.0, 0.0, 0.0),
          rot=(0.0, 0.0, 0.0), closed=False, band_mats=None):
    """A turned part (lp.lathe around Z, then turned by rot and moved to at), mapped round its axis: name on its set
    (HouseTrim on strip), band_mats {band: name} for bands of another material."""
    part, bands = lp.lathe(profile, segments=segments, closed=closed)
    band_mats = band_mats or {}
    order = [name] + sorted({n for n in band_mats.values() if n != name})
    mesh = part.data
    mesh.materials.clear()
    for n in order:
        mesh.materials.append(mat(n))
    for i, p in enumerate(mesh.polygons):
        p.material_index = order.index(band_mats.get(bands[i], name))
    for k, n in enumerate(order):
        faces = [i for i, p in enumerate(mesh.polygons) if p.material_index == k]
        if not faces:
            continue
        target = strip if n == 'HouseTrim' else SETS[n]
        lp.lathe_uv(part, target, faces=faces, grain=grain, seed=seed + k)
        if density and n != 'HouseTrim':
            scale_uvs(part, density / lt.SETS[target]['density'], faces)
    lr.split_poles(part, grain)
    lp.place(part, at, rot)
    return part


def tube(points, radius, name, sides=6, radii=None, caps=(True, True), seed=0, density=None, wobble=0.0):
    """A round rod, wire, cord or rope along points, mapped along it on its set."""
    set_name = SETS[name]
    part = lr.tube([tuple(p) for p in points], radius, sides=sides, key='trim', radii=radii, caps=caps,
                   set_name=set_name, seed=seed, wobble=wobble)
    paint(part, name, uv=False)
    if density:
        scale_uvs(part, density / lt.SETS[set_name]['density'])
    return part


def block(size, at, name, rot=(0.0, 0.0, 0.0), bevel=0.0, density=None, seed=0):
    return paint(lp.block(size, at, rot, bevel=bevel), name, density=density, seed=seed)


def flat(loops, z0, z1, name, density=None, seed=0, bevel=0.0):
    """A flat piece lying in the XY plane: loops of (x, y) (the outline counterclockwise from above, then any holes),
    from z0 up to z1, its edges chamfered by bevel."""
    tb = kit._prism([list(loop) for loop in loops], z1 - z0)
    if bevel > 0.0:
        tb.normal_update()
        sharp = [e for e in tb.edges if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > 0.5]
        bmesh.ops.bevel(tb, geom=sharp, offset=bevel, segments=1, affect='EDGES', clamp_overlap=True)
    part = lp.mesh_object(tb)
    # The prism is built in XZ facing -Y, thick toward +Y: its front becomes the top at z1, its back the bottom at z0.
    part.data.transform(Matrix(((1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, -1.0, 0.0, z1), (0.0, 0.0, 0.0, 1.0))))
    part.data.update()
    return paint(part, name, density=density, seed=seed)


def upright(loops, y0, y1, name, density=None, seed=0):
    """A flat piece standing in the XZ plane, facing -Y: loops of (x, z), from y0 back to y1."""
    tb = kit._prism([list(loop) for loop in loops], y1 - y0)
    part = lp.mesh_object(tb)
    part.data.transform(Matrix.Translation((0.0, y0, 0.0)))
    part.data.update()
    return paint(part, name, density=density, seed=seed)


def blob(center, radii, seed, name, rot=(0.0, 0.0, 0.0), segs=6, rings=4, lumpy=0.12, freq=1.6, density=None,
         shape=None):
    """A lumpy ellipsoid built pole, rings and faces in a fixed order (bmesh's UV sphere's order changes from run to
    run). shape(co) reshapes a point of the unit sphere scaled to radii before it's turned and moved."""
    bm = lp.new_bmesh()
    top = bm.verts.new((0.0, 0.0, 1.0))
    bands = []
    for j in range(1, rings):
        t = math.pi * j / rings
        bands.append([bm.verts.new((math.cos(2.0 * math.pi * i / segs) * math.sin(t),
                                    math.sin(2.0 * math.pi * i / segs) * math.sin(t), math.cos(t))) for i in range(segs)])
    bottom = bm.verts.new((0.0, 0.0, -1.0))
    for i in range(segs):
        i1 = (i + 1) % segs
        bm.faces.new((top, bands[0][i], bands[0][i1]))
        for upper, lower in zip(bands, bands[1:]):
            bm.faces.new((upper[i], lower[i], lower[i1], upper[i1]))
        bm.faces.new((bottom, bands[-1][i1], bands[-1][i]))
    off = Vector((seed * 1.7, seed * 0.3, seed * 2.9))
    for v in bm.verts:
        k = 1.0 + lumpy * noise.noise(v.co * freq + off)
        co = Vector((v.co.x * radii[0], v.co.y * radii[1], v.co.z * radii[2])) * k
        v.co = shape(co) if shape is not None else co
    part = lp.mesh_object(bm)
    lp.place(part, center, rot)
    return paint(part, name, density=density, seed=seed)


def slab(rows, thick, name, down=None, density=None, seed=0):
    """A thin solid (paper, a ribbon, a card) from its top surface: rows of points, each row running the same way; its
    underside thick below, along down (one vector, or a function of (row, column) giving one; default -Z)."""
    bm = lp.new_bmesh()
    R, C = len(rows), len(rows[0])
    top = [[bm.verts.new(Vector(p)) for p in row] for row in rows]
    bot = []
    for j, row in enumerate(rows):
        line = []
        for i, p in enumerate(row):
            d = down(j, i) if callable(down) else (Vector(down) if down is not None else Vector((0.0, 0.0, -1.0)))
            line.append(bm.verts.new(Vector(p) + d.normalized() * thick))
        bot.append(line)
    for j in range(R - 1):
        for i in range(C - 1):
            bm.faces.new((top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i]))
            bm.faces.new((bot[j][i], bot[j + 1][i], bot[j + 1][i + 1], bot[j][i + 1]))
    for i in range(C - 1):
        bm.faces.new((top[0][i], bot[0][i], bot[0][i + 1], top[0][i + 1]))
        bm.faces.new((top[R - 1][i + 1], bot[R - 1][i + 1], bot[R - 1][i], top[R - 1][i]))
    for j in range(R - 1):
        bm.faces.new((top[j + 1][0], bot[j + 1][0], bot[j][0], top[j][0]))
        bm.faces.new((top[j][C - 1], bot[j][C - 1], bot[j + 1][C - 1], top[j + 1][C - 1]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    part = lp.mesh_object(bm)
    return paint(part, name, density=density, seed=seed)


def sheet(rows, name, facing, density=None, seed=0):
    """A one-sided surface through rows of points, each face turned to face facing (a direction, or a function of the
    face's middle giving one): cloth seen from one side."""
    bm = lp.new_bmesh()
    grid = [[bm.verts.new(Vector(p)) for p in row] for row in rows]
    for j in range(len(grid) - 1):
        for i in range(len(grid[0]) - 1):
            f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            f.normal_update()
            want = facing(f.calc_center_median()) if callable(facing) else Vector(facing)
            if f.normal.dot(want) < 0.0:
                f.normal_flip()
    part = lp.mesh_object(bm)
    return paint(part, name, density=density, seed=seed)


def ribbon(points, width, normal, name='PaintBlack', density=FINE, seed=0):
    """A one-sided strip along points facing normal (ink strokes, rules, brush strokes)."""
    bm = lp.new_bmesh()
    n = Vector(normal).normalized()
    pts = [Vector(p) for p in points]
    left, right = [], []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        s = t.cross(n).normalized() * (width * 0.5)
        left.append(bm.verts.new(p - s))
        right.append(bm.verts.new(p + s))
    for i in range(len(pts) - 1):
        f = bm.faces.new((left[i], right[i], right[i + 1], left[i + 1]))
        f.normal_update()
        if f.normal.dot(n) < 0.0:
            f.normal_flip()
    part = lp.mesh_object(bm)
    return paint(part, name, density=density, seed=seed)


def letters(m, body, at, height, face, up, look='black', tol=0.05, lift=0.0004, width=None, rot=0.0, align='center'):
    """Lettering (Rye, looter_town.text) on a surface at at, facing face, its letters' up toward up."""
    f, u = Vector(face).normalized(), Vector(up).normalized()
    r = (-f).cross(u)
    space = Space(kit.basis(at, r, -f, u))
    return town.text(m, body, (0.0, 0.0, 0.0), height=height, width=width, space=space, look=look, tol=tol, lift=lift,
                     rot=rot, align=align)


def aim(front, up=(0.0, 0.0, 1.0)):
    """A socket's rotation (degrees) whose front (-Y) points along front."""
    d = Vector(front).normalized()
    q = d.to_track_quat('-Y', 'Z' if abs(d.z) < 0.99 else 'X')
    return tuple(math.degrees(a) for a in q.to_euler())


def spline(points, per=4, closed=False, floor=None):
    """A Catmull-Rom curve through points, per steps a span (a closed one ends on its first point again); floor keeps
    every point at least that high (a rope's radius over the ground)."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    out = []
    for i in range(n if closed else n - 1):
        p0 = pts[(i - 1) % n] if closed else pts[max(i - 1, 0)]
        p1, p2 = pts[i % n], pts[(i + 1) % n]
        p3 = pts[(i + 2) % n] if closed else pts[min(i + 2, n - 1)]
        for k in range(per):
            t = k / per
            out.append(0.5 * (2.0 * p1 + (p2 - p0) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t * t
                              + (3.0 * p1 - p0 - 3.0 * p2 + p3) * t * t * t))
    out.append(out[0].copy() if closed else pts[-1])
    if floor is not None:
        for p in out:
            p.z = max(p.z, floor)
    return out


def transform(parts, matrix):
    for p in parts:
        p.data.transform(matrix)
        p.data.update()


def bounds(parts):
    pts = [v.co for p in parts for v in p.data.vertices]
    lo = Vector((min(c.x for c in pts), min(c.y for c in pts), min(c.z for c in pts)))
    hi = Vector((max(c.x for c in pts), max(c.y for c in pts), max(c.z for c in pts)))
    return lo, hi


def settle(parts, matrix=None, center=True):
    """Turns the parts by matrix, then moves them so their lowest point is on the ground and (center) the middle of
    their footprint is at the origin. Returns the whole move (to carry sockets along) and the bounds after it."""
    matrix = matrix if matrix is not None else Matrix.Identity(4)
    transform(parts, matrix)
    lo, hi = bounds(parts)
    shift = Vector((-(lo.x + hi.x) * 0.5 if center else 0.0, -(lo.y + hi.y) * 0.5 if center else 0.0, -lo.z))
    move = Matrix.Translation(shift)
    transform(parts, move)
    return move @ matrix, lo + shift, hi + shift


def box_hull(m, lo, hi, pad=0.005, least=0.06, grounded=True):
    """A box hull round lo..hi, padded, at least `least` along each axis (the tiny ones get a bigger target)."""
    size = [max(hi[i] - lo[i] + 2.0 * pad, least) for i in range(3)]
    center = [(lo[i] + hi[i]) * 0.5 for i in range(3)]
    if grounded:
        center[2] = lo[2] - pad + size[2] * 0.5
    m.hull(tuple(size), at=tuple(center))


def rounded_rect(w, d, r, n=3):
    """A rounded rectangle's outline (x, y), counterclockwise, its corners n segments."""
    pts = []
    for cx, cy, a0 in ((w / 2 - r, -d / 2 + r, -90.0), (w / 2 - r, d / 2 - r, 0.0), (-w / 2 + r, d / 2 - r, 90.0),
                       (-w / 2 + r, -d / 2 + r, 180.0)):
        for k in range(n + 1):
            a = math.radians(a0 + 90.0 * k / n)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def circle(cx, cy, r, n, start=0.0, reverse=False):
    pts = [(cx + r * math.cos(start + 2.0 * math.pi * k / n), cy + r * math.sin(start + 2.0 * math.pi * k / n))
           for k in range(n)]
    return list(reversed(pts)) if reverse else pts


def star(cx, cy, r_out, r_in, points, start=90.0):
    return [(cx + (r_out if k % 2 == 0 else r_in) * math.cos(math.radians(start + 180.0 * k / points)),
             cy + (r_out if k % 2 == 0 else r_in) * math.sin(math.radians(start + 180.0 * k / points)))
            for k in range(2 * points)]


def pencil(at, yaw, seed, length=0.13):
    """A red cedar pencil lying on a surface: at is the middle of its underside, yaw its turn from +Y."""
    r = 0.0036
    part = lathe([(0.0, 0.0), (0.0008, 0.0), (0.0014, 0.003), (0.0034, 0.017), (r, 0.018), (r, length), (0.0, length)], 6,
                 'PaintOxide', density=FINE, seed=seed, band_mats={0: 'PaintBlack', 1: 'PaintBlack', 2: 'WoodCream'})
    lp.place(part, (0.0, 0.0, 0.0), (-90.0, 0.0, 0.0))
    lp.place(part, (0.0, -length * 0.5, 0.0))
    lp.place(part, (at[0], at[1], at[2] + r), (0.0, 0.0, yaw))
    return part


def nail_head(at, name='IronBlack', size=0.011):
    return block((size, size, 0.003), (at[0], at[1], at[2] + 0.0012), name, density=FINE)


# --- The lantern's bail ---

def lantern_bail():
    m = Model('LanternBail', seed=SEED + 10)
    R, H = 0.079, 0.205
    # Built standing as it hung on the lantern (the arch in XZ), then laid on its side. The snapped side (-x) was
    # pulled wider in the fall; its end is ripped open. The other end's hook still holds the torn-off brass ear.
    arch = []
    for k in range(17):
        t = math.pi * (16 - k) / 16
        c = math.cos(t)
        arch.append(Vector((R * c * (1.07 if c < 0.0 else 1.0), 0.0, H * math.sin(t) ** 0.85)))
    snapped = [Vector((-R * 1.07 - 0.011, 0.007, -0.017)), Vector((-R * 1.07 - 0.004, 0.003, -0.007))]
    hook = [Vector((R - 0.002, 0.0, -0.01)), Vector((R - 0.012, 0.0, -0.018)), Vector((R - 0.022, 0.0, -0.012))]
    wire = tube(spline(snapped + arch + hook, per=2), 0.0045, 'IronBlack', sides=6, density=FINE, seed=SEED + 11)
    grip = lathe([(0.0, -0.05), (0.011, -0.05), (0.015, -0.035), (0.016, 0.0), (0.015, 0.035), (0.011, 0.05),
                  (0.0, 0.05)], 10, 'HouseTrim', strip='Beams', grain='up', seed=SEED + 12, at=(0.0, 0.0, H),
                 rot=(0.0, 90.0, 0.0))
    ear = [(-0.008, -0.029), (-0.004, -0.025), (-0.001, -0.031), (0.002, -0.024), (0.005, -0.03), (0.008, -0.022),
           (0.008, 0.0), (-0.008, 0.0)]
    plate = upright([ear], -0.0015, 0.0015, 'BrassWorn', density=FINE, seed=SEED + 13)
    lp.place(plate, (R - 0.014, 0.0, -0.012), (0.0, 0.0, 28.0))
    parts = [wire, grip, plate]
    # On its side: the arch flat on the ground, tipped 3.2 degrees so the wire and the thicker grip both touch it.
    lay = Matrix.Rotation(math.radians(3.2), 4, 'X') @ Matrix.Rotation(math.radians(-90.0), 4, 'X')
    move, lo, hi = settle(parts, lay)
    m.add_all(parts)
    grip_at = move @ Vector((0.0, 0.0, H))
    m.socket('Grip', tuple(grip_at), (0.0, 0.0, 0.0))
    m.socket('Interact', tuple(grip_at + Vector((0.0, 0.0, 0.04))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi)
    return m.done('the middle of its footprint on the ground (lying on its side)', ao=0.05)


# --- The winch crank ---

def winch_crank():
    m = Model('WinchCrank', seed=SEED + 20)
    ARM = 0.38
    # Built standing: the shaft's axis along +Z through the origin, the boss under the arm, the arm along +X.
    o, c = 0.041, 0.012
    octagon = [(o, -o + c), (o, o - c), (o - c, o), (-o + c, o), (-o, o - c), (-o, -o + c), (-o + c, -o), (o - c, -o)]
    seat = [(0.0225, -0.0225), (0.0225, 0.0225), (-0.0225, 0.0225), (-0.0225, -0.0225)]
    boss = flat([octagon, seat], -0.09, 0.0, 'IronBlack', density=640.0, seed=SEED + 21, bevel=0.003)
    end = [(ARM + 0.022 * math.cos(math.radians(a)), 0.022 * math.sin(math.radians(a))) for a in (-60, -30, 0, 30, 60)]
    arm = flat([[(-0.02, -0.032), (ARM, -0.022)] + end + [(ARM, 0.022), (-0.02, 0.032)]], 0.0, 0.02, 'IronBlack',
               density=640.0, seed=SEED + 22, bevel=0.0025)
    screw = block((0.016, 0.008, 0.016), (0.0, o + 0.004, -0.045), 'IronBlack', bevel=0.002, density=FINE, seed=SEED + 23)
    collar = lathe([(0.0, 0.021), (0.0135, 0.021), (0.0135, 0.031), (0.0, 0.031)], 10, 'IronBlack', density=FINE,
                   seed=SEED + 24, at=(ARM, 0.0, 0.0))
    grip = lathe([(0.0, 0.031), (0.016, 0.031), (0.019, 0.045), (0.021, 0.1), (0.019, 0.16), (0.016, 0.178), (0.0, 0.178)],
                 12, 'HouseTrim', strip='Beams', grain='up', seed=SEED + 25, at=(ARM, 0.0, 0.0))
    cap = lathe([(0.0, 0.178), (0.012, 0.178), (0.0115, 0.186), (0.006, 0.191), (0.0, 0.192)], 10, 'IronBlack',
                density=FINE, seed=SEED + 26, at=(ARM, 0.0, 0.0))
    parts = [boss, arm, screw, collar, grip, cap]
    # Set down as it would rest: on the boss's foot and the arm's far end, the grip standing up at the far end.
    lay = Matrix.Rotation(math.atan2(0.09, ARM + 0.022 - o), 4, 'Y')
    move, lo, hi = settle(parts, lay)
    # The sheriff's impound tag, tied round the arm with twine and lying on the ground in front of it.
    loop = [move @ Vector((0.15 + 0.002 * math.sin(3.0 * a), 0.034 * math.cos(a), 0.01 + 0.015 * math.sin(a)))
            for a in (2.0 * math.pi * k / 12 for k in range(13))]
    twine = tube(loop, 0.0016, 'RopeHemp', sides=4, caps=(False, False), seed=SEED + 27)
    low = min(loop, key=lambda p: p.z - p.y * 0.3)
    card_c = Vector((low.x + 0.03, low.y - 0.08, 0.0))
    yaw = math.radians(-18.0)
    ax, ay = Vector((math.cos(yaw), math.sin(yaw), 0.0)), Vector((-math.sin(yaw), math.cos(yaw), 0.0))
    rows = [[card_c + ax * (x * 0.045) + ay * (y * 0.028) + Vector((0.0, 0.0, 0.0006)) for x in (-1.0, 0.0, 1.0)]
            for y in (-1.0, 0.0, 1.0)]
    card = slab(rows, 0.0005, 'SextonLedgerPages', seed=SEED + 28)
    hole = card_c + ay * 0.022 + Vector((0.0, 0.0, 0.0013))
    mid = low.lerp(hole, 0.6)
    tail = tube(spline([low, Vector((mid.x, mid.y, 0.004)), hole], per=4, floor=0.0016), 0.0016, 'RopeHemp', sides=4,
                seed=SEED + 29)
    ink = [ribbon([card_c + ax * (x0 + (x1 - x0) * s) + ay * (y + 0.002 * math.sin(s * 9.0)) + Vector((0.0, 0.0, 0.0012))
                   for s in (0.0, 0.33, 0.66, 1.0)], 0.0016, UP, seed=SEED + 30 + k)
           for k, (x0, x1, y) in enumerate(((-0.025, 0.03, 0.012), (-0.025, 0.018, 0.0), (-0.025, 0.026, -0.012)))]
    parts += [twine, card, tail] + ink
    lo, hi = bounds(parts)
    m.add_all(parts)
    mouth = move @ Vector((0.0, 0.0, -0.09))
    out = (move.to_3x3() @ Vector((0.0, 0.0, -1.0))).normalized()
    m.socket('Fit', tuple(mouth), aim(out))
    m.socket('Interact', tuple(move @ Vector((0.2, 0.0, 0.011)) + Vector((0.0, 0.0, 0.05))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi)
    return m.done('the middle of its footprint on the ground (resting on its boss and the arm\'s end, grip up)',
                  ao=0.08)


# --- The coal sack ---

def coal_sack():
    m = Model('CoalSack', seed=SEED + 30)
    rnd = random.Random(SEED + 30)
    LEAN, TIE = 0.04, 0.425

    def lean(v):
        return Vector((v.x + LEAN * (max(v.z, 0.0) / 0.45) ** 2, v.y, v.z))
    # A full sack slumped on its wide foot, bulging over the lumps inside, its neck gathered into folds by the tie,
    # its mouth flopping open over the coal heaped in it.
    prof = [(0.0, 0.0), (0.17, 0.0), (0.2, 0.02), (0.216, 0.075), (0.212, 0.16), (0.19, 0.25), (0.158, 0.315),
            (0.132, 0.365), (0.115, 0.402), (0.108, TIE), (0.114, 0.44), (0.135, 0.462), (0.16, 0.484), (0.168, 0.472),
            (0.15, 0.466), (0.12, 0.46), (0.0, 0.456)]
    part, bands = lp.lathe(prof, segments=24)
    off, off2 = Vector((3.1, 7.7, 1.9)), Vector((8.3, 1.2, 5.5))
    for v in part.data.vertices:
        x, y, z = v.co
        r = math.hypot(x, y)
        a = math.atan2(y, x)
        if r > 1e-6:
            k = 1.0
            if 0.012 < z < 0.41:                                    # lumps of coal pressing out
                k += (0.15 * noise.noise(v.co * 4.5 + off) + 0.05 * noise.noise(v.co * 11.0 + off)) * min(1.0, z / 0.06)
            twist = 0.9 * noise.noise(v.co * 4.0 + off2)
            near = math.exp(-((z - TIE) / 0.06) ** 2) * (1.0 - math.exp(-((z - TIE) / 0.012) ** 2))
            k += 0.15 * near * math.sin(a * 8.0 + twist)            # folds gathered at the tie
            if 0.22 < z < TIE:                                      # running down over its shoulders, fading
                k += 0.045 * ((z - 0.22) / (TIE - 0.22)) ** 2 * math.sin(a * 8.0 + twist)
            if z > 0.445 and r > 0.1:                               # the mouth flopping open, uneven
                k += 0.1 * math.cos(a * 3.0 + 1.0) + 0.05 * noise.noise(v.co * 8.0 + off2)
                z += 0.026 * math.cos(a * 2.0 + 0.4) + 0.01 * noise.noise(v.co * 9.0 + off)
            x, y = x * k, y * k * 0.87                              # a full sack is flatter front to back
        elif z > 0.4:
            z = 0.5                                                 # the coal heaped in its mouth
        v.co = lean(Vector((x, y, z)))
    part.data.update()
    last = len(prof) - 2
    names = ['PaintBlack' if bands[i] == last else 'Canvas' for i in range(len(part.data.polygons))]
    paint_faces(part, names, density=FINE, seed=SEED + 31)
    canvas = [i for i, n in enumerate(names) if n == 'Canvas']
    lp.lathe_uv(part, 'Polymer', faces=canvas, seed=SEED + 32)
    parts = [part]
    for k in range(12):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        r = 0.105 * math.sqrt(rnd.uniform(0.0, 1.0))
        s = rnd.uniform(0.022, 0.04)
        c = lean(Vector((r * math.cos(a), r * math.sin(a) * 0.87, 0.468 + (0.105 - r) * 0.3 + s * 0.35)))
        parts.append(blob(c, (s, s * rnd.uniform(0.7, 0.95), s * rnd.uniform(0.6, 0.85)), SEED + 33 + k, 'PaintBlack',
                          rot=(rnd.uniform(-30, 30), rnd.uniform(-30, 30), rnd.uniform(0, 180)), segs=5, rings=3,
                          lumpy=0.22, density=FINE))
    for k, (x, y) in enumerate(((0.17, -0.22), (0.26, -0.09), (-0.05, -0.25))):    # lumps spilt on the ground
        s = rnd.uniform(0.018, 0.03)
        parts.append(blob((x, y, s * 0.55), (s, s * 0.8, s * 0.65), SEED + 45 + k, 'PaintBlack', rot=(0, 0, k * 40.0),
                          segs=5, rings=3, lumpy=0.22, density=FINE))
    # The neck tied with two turns of hemp, a knot in front and its ends hanging.
    rt = 0.108 + 0.0055
    turns = [lean(Vector((rt * math.cos(a), (rt - 0.0055) * 0.87 * math.sin(a) + 0.0055 * math.sin(a),
                          TIE - 0.006 + 0.012 * (a + math.pi * 0.5) / (4.0 * math.pi))))
             for a in (4.0 * math.pi * k / 24 - math.pi * 0.5 for k in range(25))]
    parts.append(tube(turns, 0.0055, 'RopeHemp', sides=5, seed=SEED + 47))
    knot = lean(Vector((0.0, -(rt - 0.0055) * 0.87 - 0.0095, TIE)))
    parts.append(blob(knot, (0.012, 0.009, 0.011), SEED + 48, 'RopeHemp', segs=6, rings=4, lumpy=0.15))
    for k, dx in enumerate((-0.012, 0.014)):
        parts.append(tube([knot + Vector((dx * 0.4, -0.004, -0.004)), knot + Vector((dx, -0.012, -0.035)),
                           knot + Vector((dx * 1.4, -0.018, -0.07))], 0.005, 'RopeHemp', sides=5,
                          radii=[0.0052, 0.005, 0.0042], seed=SEED + 49 + k))
    m.add_all(parts)
    m.socket('Grip', tuple(knot + Vector((0.0, -0.01, 0.0))), aim((0.0, -1.0, 0.0)))
    m.socket('Interact', (0.02, -0.23, 0.26), aim((0.0, -1.0, 0.0)))
    m.hull((0.46, 0.46, 0.52), at=(0.02, 0.0, 0.26))
    return m.done('the middle of its foot on the ground', ao=0.25)


# --- The clamp pin ---

def clamp_pin():
    m = Model('ClampPin', seed=SEED + 40)
    AX = 0.0175                                              # its axis over the ground
    shaft = lathe([(0.0, -0.02), (0.016, -0.02), (0.0175, -0.016), (0.0175, -0.004), (0.016, 0.0), (0.016, 0.255),
                   (0.0145, 0.272), (0.011, 0.29), (0.0, 0.296)], 12, 'IronBlack', density=640.0, grain='up',
                  seed=SEED + 41, at=(0.0, 0.0, AX), rot=(0.0, 90.0, 0.0))
    tbar = lathe([(0.0, -0.068), (0.013, -0.068), (0.015, -0.062), (0.015, -0.05), (0.012, -0.045), (0.012, 0.045),
                  (0.015, 0.05), (0.015, 0.062), (0.013, 0.068), (0.0, 0.068)], 10, 'IronBlack', density=640.0,
                 grain='up', seed=SEED + 42, at=(-0.033, 0.0, AX - 0.001), rot=(90.0, 0.0, 0.0))
    cotter = tube([(0.262, -0.033, AX - 0.003), (0.266, -0.02, AX), (0.266, 0.02, AX), (0.262, 0.027, AX),
                   (0.268, 0.033, AX), (0.274, 0.027, AX), (0.27, 0.02, AX), (0.27, -0.02, AX),
                   (0.277, -0.033, AX - 0.003)], 0.0018, 'IronBlack', sides=4, density=FINE, seed=SEED + 43)
    # The cord: three turns round the shaft behind the collar, then a loose loop lying on the ground, round to the start.
    rr = 0.016 + 0.0055
    wrap = [Vector((0.012 + 0.028 * t, rr * math.cos(2.0 * math.pi * 3.0 * t - math.pi * 0.5),
                    AX + rr * math.sin(2.0 * math.pi * 3.0 * t - math.pi * 0.5))) for t in (k / 27.0 for k in range(28))]
    g = 0.0055
    # The loose loop that hung round his neck, fallen slack: out from the wrap, round past the T's end and back.
    loose = [Vector(p) for p in ((0.075, -0.045, g), (0.095, -0.11, g), (0.045, -0.172, g), (-0.05, -0.19, g),
                                 (-0.14, -0.15, g), (-0.175, -0.07, g), (-0.155, 0.0, g), (-0.115, 0.055, g),
                                 (-0.075, 0.1, g), (-0.025, 0.098, g), (0.012, 0.062, g), (0.016, 0.026, g + 0.004))]
    path = wrap + spline([wrap[-1]] + loose + [wrap[0]], per=3, floor=g)[1:-1]
    cord = tube(path + [path[0]], 0.0055, 'RopeHemp', sides=5, caps=(False, False), seed=SEED + 44)
    knot = blob(wrap[-1] + Vector((0.004, 0.0, 0.004)), (0.011, 0.01, 0.009), SEED + 45, 'RopeHemp', segs=6, rings=4,
                lumpy=0.15)
    parts = [shaft, tbar, cotter, cord, knot]
    move, lo, hi = settle(parts)
    m.add_all(parts)
    tip = move @ Vector((0.296, 0.0, AX))
    m.socket('Fit', tuple(tip), aim((1.0, 0.0, 0.0)))
    m.socket('Interact', tuple(move @ Vector((0.12, 0.0, AX + 0.05))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi)
    return m.done('the middle of its footprint on the ground (lying)', ao=0.06)


# --- The cellar key ---

def cellar_key():
    m = Model('CellarKey', seed=SEED + 50)
    BOW = Vector((0.0, 0.0, -0.058))
    # The cord loop over the hook: its top's inner side is the pivot. It passes front to back through the bow under its
    # top bar, and its strands twist a quarter turn as they rise, so its top lies across the key's plane and a peg
    # pointing out of the wall at the viewer (along Y) passes through it, clear of both strands (the peg up to 12 mm
    # wide, its top at the pivot).
    rc = 0.0022
    cord = [(0.0, 0.0, -0.0492), (-0.001, -0.0058, -0.047), (-0.004, -0.006, -0.032), (-0.0085, -0.003, -0.018),
            (-0.0095, 0.0, -0.006), (-0.0085, 0.0, 0.0003), (-0.004, 0.0, 0.0021), (0.0, 0.0, rc), (0.004, 0.0, 0.0021),
            (0.0085, 0.0, 0.0003), (0.0095, 0.0, -0.006), (0.0085, 0.003, -0.018), (0.004, 0.006, -0.032),
            (0.001, 0.0058, -0.047), (0.0, 0.0, -0.0492)]
    loop = tube(cord, rc, 'RopeHemp', sides=4, caps=(False, False), seed=SEED + 51, density=640.0)
    bow = upright([circle(0.0, BOW.z, 0.025, 20), circle(0.0, BOW.z, 0.013, 12, reverse=True)], -0.0035, 0.0035,
                  'IronBlack', density=FINE, seed=SEED + 52)
    shaft = lathe([(0.0, -0.172), (0.0045, -0.172), (0.0055, -0.168), (0.0055, -0.095), (0.0078, -0.092),
                   (0.0078, -0.086), (0.0058, -0.083), (0.0058, -0.078), (0.0, -0.078)], 8, 'IronBlack', density=FINE,
                  seed=SEED + 53)
    bit = upright([[(0.004, -0.168), (0.026, -0.168), (0.026, -0.16), (0.019, -0.16), (0.019, -0.155), (0.026, -0.155),
                    (0.026, -0.146), (0.017, -0.146), (0.017, -0.141), (0.004, -0.141)]], -0.003, 0.003, 'IronBlack',
                  density=FINE, seed=SEED + 54)
    # The brass fob: a level oval hung by its top from the bow on a twist of twine, behind the shaft.
    fc = Vector((0.03, 0.0072, -0.1))
    fob = upright([[(fc.x + 0.022 * math.cos(a), fc.z + 0.014 * math.sin(a)) for a in
                    (2.0 * math.pi * k / 18 for k in range(18))],
                   circle(fc.x, fc.z + 0.0085, 0.0022, 8, reverse=True)], fc.y - 0.00075, fc.y + 0.00075, 'BrassWorn',
                  density=FINE, seed=SEED + 55)
    twist = tube([(0.022, 0.0, -0.058), (0.027, 0.0045, -0.072), (0.03, 0.0072, -0.0905), (0.031, 0.005, -0.075),
                  (0.024, 0.002, -0.06)], 0.0011, 'RopeHemp', sides=4, seed=SEED + 56)
    parts = [loop, bow, shaft, bit, fob, twist]
    letters(m, 'CELLAR', (fc.x, fc.y - 0.00075, fc.z - 0.003), 0.0062, (0.0, -1.0, 0.0), UP, look='black', tol=0.045,
            lift=0.0003, width=0.031)
    m.add_all(parts)
    m.socket('Interact', (0.0, -0.02, BOW.z), aim((0.0, -1.0, 0.0)))
    m.hull((0.075, 0.03, 0.18), at=(0.012, 0.0, -0.088))
    return m.done('the top of its cord loop (the hook), the key hanging plumb below, its face to -Y', ao=0.02,
                  ground=False, min_height=0.0002, crumb=0.0008)


# --- Thaddeus's tape ---

def tape_measure():
    m = Model('TapeMeasure', seed=SEED + 60)
    parts = [
        lathe([(0.0, 0.0015), (0.052, 0.0015), (0.0565, 0.003), (0.0575, 0.006), (0.0575, 0.028), (0.0565, 0.031),
               (0.05, 0.0325), (0.0, 0.0325)], 24, 'SextonLedgerCover', seed=SEED + 61),
        lathe([(0.0565, 0.0), (0.0605, 0.0), (0.0605, 0.0075), (0.0565, 0.0075)], 24, 'BrassWorn', density=FINE,
              seed=SEED + 62, closed=True),
        lathe([(0.0555, 0.0265), (0.0605, 0.0265), (0.0605, 0.0345), (0.049, 0.0345), (0.049, 0.031)], 24, 'BrassWorn',
              density=FINE, seed=SEED + 63, closed=True),
        lathe([(0.0, 0.032), (0.012, 0.032), (0.012, 0.0355), (0.009, 0.038), (0.0, 0.038)], 16, 'BrassWorn',
              density=FINE, seed=SEED + 64),
        block((0.04, 0.008, 0.003), (0.018, 0.0, 0.0385), 'BrassWorn', bevel=0.001, density=FINE, seed=SEED + 65),
        block((0.005, 0.024, 0.011), (0.0605, 0.0, 0.017), 'BrassWorn', density=FINE, seed=SEED + 66),
    ]
    lp.place(parts[4], (0.0, 0.0, 0.0), (0.0, 0.0, 35.0))
    knob_at = Vector((0.036 * math.cos(math.radians(35.0)), 0.036 * math.sin(math.radians(35.0)), 0.0))
    parts.append(lathe([(0.0, 0.039), (0.004, 0.039), (0.005, 0.045), (0.004, 0.05), (0.0, 0.051)], 8, 'HouseTrim',
                       strip='Beams', grain='up', seed=SEED + 67, at=tuple(knob_at)))
    # The linen tape run out of its slot, down to the ground and lying there in a lazy curl, to its brass end ring.
    path = [Vector(p) for p in ((0.0612, 0.0, 0.017), (0.07, 0.0, 0.0145), (0.08, 0.002, 0.007), (0.095, 0.006, 0.0016),
                                (0.13, 0.012, 0.0006), (0.175, 0.012, 0.0006), (0.215, 0.0, 0.0006), (0.245, -0.02, 0.0006),
                                (0.262, -0.05, 0.0006))]
    tape = lp.sweep([tuple(p) for p in path], [(-0.008, -0.0003), (0.008, -0.0003), (0.008, 0.0003), (-0.008, 0.0003)],
                    up=(0.0, 0.0, 1.0))
    parts.append(paint(tape, 'ClothLinen', seed=SEED + 68))
    # Inch ticks along the flat stretch, on its top.
    lengths = [0.0]
    for a, b in zip(path, path[1:]):
        lengths.append(lengths[-1] + (b - a).length)

    def at_length(s):
        for i in range(len(path) - 1):
            if lengths[i + 1] >= s:
                t = (s - lengths[i]) / max(lengths[i + 1] - lengths[i], 1e-9)
                return path[i].lerp(path[i + 1], t), (path[i + 1] - path[i]).normalized()
        return path[-1], (path[-1] - path[-2]).normalized()
    for k in range(9):
        p, d = at_length(0.06 + 0.0254 * k)
        side = d.cross(UP).normalized()
        long = 0.008 if k % 2 == 0 else 0.0045
        parts.append(ribbon([p + side * 0.008 + UP * 0.00034, p + side * (0.008 - long) + UP * 0.00034], 0.0011, UP,
                            seed=SEED + 70 + k))
    end, d = path[-1], (path[-1] - path[-2]).normalized()
    plate = block((0.012, 0.016, 0.0012), tuple(end + d * 0.006 + UP * 0.0001), 'BrassWorn', density=FINE,
                  seed=SEED + 80)
    lp.place(plate, (0.0, 0.0, 0.0))
    ring_c = end + d * 0.02
    ring = lathe([(0.0064, 0.0008), (0.008, 0.0), (0.0096, 0.0008), (0.008, 0.0016)], 12, 'BrassWorn', density=FINE,
                 seed=SEED + 81, at=(ring_c.x, ring_c.y, 0.0), closed=True)
    parts += [plate, ring]
    m.add_all(parts)
    m.socket('Interact', (0.0, 0.0, 0.06), aim((0.0, -1.0, 0.0)))
    m.hull((0.2, 0.13, 0.05), at=(0.035, 0.0, 0.025))
    return m.done("the reel's middle on the ground (lying on its back, the tape run out toward +X)", ao=0.03,
                  min_height=0.0002, crumb=0.0008)


# --- Fenn's fare ---

HEAD = [(0.35, -0.9), (0.3, -0.55), (0.4, -0.35), (0.55, -0.05), (0.55, 0.3), (0.35, 0.62), (0.05, 0.75), (-0.25, 0.62),
        (-0.38, 0.4), (-0.42, 0.25), (-0.62, 0.08), (-0.45, 0.0), (-0.5, -0.1), (-0.44, -0.18), (-0.46, -0.32),
        (-0.32, -0.42), (-0.22, -0.55), (-0.25, -0.9)]


def grave_coins():
    m = Model('GraveCoins', seed=SEED + 70)
    R, T, RIM, SINK, RELIEF = 0.018, 0.0024, 0.0022, 0.0004, 0.00045
    profile = [(0.0, SINK), (R - RIM, SINK), (R - RIM * 0.6, 0.0), (R, 0.0), (R, T), (R - RIM * 0.6, T),
               (R - RIM, T - SINK), (0.0, T - SINK)]
    face = T - SINK
    parts = []
    for k, (x, y, yaw, side) in enumerate(((-0.032, 0.0015, 12.0, 'head'), (0.032, -0.002, -40.0, 'sun'))):
        coin = [paint(lp.lathe(profile, segments=22)[0], 'AmmoCaseBrass', density=2048.0, seed=SEED + 71 + k)]
        if side == 'head':
            coin.append(flat([[(px * 0.0118, py * 0.0118) for px, py in HEAD]], face, face + RELIEF, 'AmmoCaseBrass',
                             density=2048.0, seed=SEED + 73))
        else:
            coin.append(flat([circle(0.0, 0.0, 0.0068, 14), circle(0.0, 0.0, 0.0047, 10, reverse=True)], face,
                             face + RELIEF, 'AmmoCaseBrass', density=2048.0, seed=SEED + 74))
            for j in range(8):
                a = 2.0 * math.pi * j / 8
                d, s = Vector((math.cos(a), math.sin(a), 0.0)), Vector((-math.sin(a), math.cos(a), 0.0))
                tri = [tuple(d * 0.0076 - s * 0.0016)[:2], tuple(d * 0.0124)[:2], tuple(d * 0.0076 + s * 0.0016)[:2]]
                coin.append(flat([tri], face, face + RELIEF, 'AmmoCaseBrass', density=2048.0, seed=SEED + 75 + j))
        transform(coin, Matrix.Translation((x, y, 0.0)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
        parts += coin
    m.add_all(parts)
    m.socket('Interact', (0.0, 0.0, 0.012), aim((0.0, -1.0, 0.0)))
    m.hull((0.11, 0.05, 0.015), at=(0.0, 0.0, 0.0075))
    return m.done('midway between the two coins on the surface; the coins 6.4 cm apart along X (as on two eyes)',
                  ao=0.004, smooth=35.0, min_height=0.00012, crumb=0.0004, lods=False)


# --- The miner's tag ---

def miners_tag():
    m = Model('MinersTag', seed=SEED + 80)
    R = 0.02
    zc = -0.0167                                            # the disc's middle: its hole's top is the hang point
    disc = upright([circle(0.0, zc, R, 24), circle(0.0, zc + 0.0135, 0.0032, 10, reverse=True)], -0.0015, 0.0015,
                   'BrassWorn', density=1536.0, seed=SEED + 81)
    m.add(disc)
    letters(m, '7', (0.0, -0.0015, zc - 0.0035), 0.0165, (0.0, -1.0, 0.0), UP, look='black', tol=0.03, lift=0.00025)
    m.socket('Interact', (0.0, -0.01, zc), aim((0.0, -1.0, 0.0)))
    m.hull((0.065, 0.025, 0.065), at=(0.0, 0.0, zc))
    return m.done("the top of its hole (the hook), the tag hanging plumb, its face to -Y (the tag board's 4 cm brass "
                  "disc)", ao=0.004, ground=False, min_height=0.0002, crumb=0.0008)


# --- The Gravemother's fang ---

def gravemother_fang():
    m = Model('GravemotherFang', seed=SEED + 90)
    rnd = random.Random(SEED + 90)
    AX = 0.054                                              # the chelicera's axis over the ground
    # The chelicera's end, bone white with one of her dark rings, torn off at its root (a ragged dark end toward -X);
    # its axis along X, the fang's root at its tip.
    prof = [(0.0, -0.175), (0.036, -0.175), (0.041, -0.168), (0.047, -0.15), (0.051, -0.12), (0.053, -0.09),
            (0.054, -0.075), (0.053, -0.058), (0.05, -0.04), (0.045, -0.022), (0.038, -0.008), (0.03, 0.0), (0.0, 0.006)]
    radius_at = [(z, r) for r, z in prof]
    stump = lathe(prof, 12, 'BoneDry', seed=SEED + 92, band_mats={0: 'MourningCrepe', 6: 'MourningCrepe'})
    off = Vector((1.3, 4.1, 2.2))
    for v in stump.data.vertices:
        a = math.atan2(v.co.y, v.co.x)
        if abs(v.co.z + 0.175) < 1e-6 and math.hypot(v.co.x, v.co.y) > 0.03:
            v.co.z -= 0.004 + 0.013 * (0.5 + 0.5 * math.sin(a * 5.0 + 0.7))
        elif min(abs(v.co.z + 0.075), abs(v.co.z + 0.058)) < 1e-6:          # the dark ring's ragged edges
            v.co.z += 0.007 * math.sin(a * 3.0 + v.co.z * 90.0) + 0.004 * noise.noise(v.co * 40.0 + off)
        v.co += Vector((noise.noise(v.co * 14.0 + off), noise.noise(v.co * 14.0 + off * 2.0), 0.0)) * 0.0045
    stump.data.update()
    lp.place(stump, (0.0, 0.0, 0.0), (0.0, 90.0, 0.0))                 # its axis along X, the torn end toward -X
    lp.place(stump, (0.0, 0.0, AX))
    # The fang: a black sickle from the tip, curving round toward +Y and down onto the ground, lying on its side.
    C, RC, n = Vector((0.0, 0.2, 0.0)), 0.2, 18
    pts, scales = [], []
    for i in range(n + 1):
        t = i / n
        phi = math.radians(100.0) * t - 0.07
        r = 0.034 * (1.0 - t) ** 0.8 + 0.0015
        z = r * 0.85 + (AX - 0.034 * 0.85) * max(0.0, 1.0 - t / 0.3) ** 2
        pts.append(tuple(C + Vector((RC * math.sin(phi), -RC * math.cos(phi), z))))
        scales.append(r)
    fang = lp.sweep(pts, lp.ngon(1.0, 8, squash=0.85, start=math.pi / 8), up=(0.0, 0.0, 1.0), scales=scales)
    paint(fang, 'MourningCrepe', seed=SEED + 91)
    parts = [stump, fang]
    # Stiff black bristles along its back, leaning toward the torn end.
    for k in range(16):
        x = rnd.uniform(-0.155, -0.02)
        r = next(rr for zz, rr in radius_at[::-1] if zz <= x + 1e-9) if x > -0.175 else 0.04
        a = math.radians(rnd.uniform(-85.0, 85.0))
        out = Vector((0.0, math.sin(a), math.cos(a)))
        base = Vector((x, 0.0, AX)) + out * (r - 0.002)
        d = (out * 0.75 + Vector((-0.65, 0.0, 0.0))).normalized()
        bristle = lathe([(0.0028, 0.0), (0.0, rnd.uniform(0.04, 0.07))], 3, 'MourningCrepe', seed=SEED + 93 + k)
        bristle.data.transform(Matrix.LocRotScale(base, UP.rotation_difference(d), None))
        bristle.data.update()
        parts.append(bristle)
    move, lo, hi = settle(parts, Matrix.Rotation(math.radians(-20.0), 4, 'Z'))
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((-0.06, 0.0, AX + 0.08))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi)
    return m.done('the middle of its footprint on the ground (lying on its side)', ao=0.08)


# --- Sheriff Odell's badge ---

def sheriff_badge():
    m = Model('SheriffBadge', seed=SEED + 100)
    RO, RI, T = 0.04, 0.0235, 0.0022
    outline = star(0.0, 0.0, RO, RI, 6)
    outline[6] = (outline[6][0] * 0.9, outline[6][1] * 0.9)          # the bottom point bitten short
    body = flat([outline], 0.0, T, 'BrassWorn', density=1536.0, seed=SEED + 101)
    parts = [body]
    for k in range(6):
        if k == 3:
            continue                                                  # its ball chewed off
        a = math.radians(90.0 + 60.0 * k)
        parts.append(blob((math.cos(a) * (RO + 0.0032), math.sin(a) * (RO + 0.0032), T * 0.5), (0.0052, 0.0052, 0.0052),
                          SEED + 102 + k, 'BrassWorn', segs=10, rings=6, lumpy=0.0, density=1536.0))
    parts.append(lathe([(0.0, T), (0.0185, T), (0.0185, T + 0.0011), (0.0168, T + 0.0013), (0.0162, T + 0.0006),
                        (0.0, T + 0.0006)], 24, 'BrassWorn', density=1536.0, seed=SEED + 108))
    # The upper right point bent up where it was chewed.
    d = Vector((math.cos(math.radians(30.0)), math.sin(math.radians(30.0)), 0.0))
    hinge, bend = 0.025, math.radians(16.0)
    for p in parts:
        for v in p.data.vertices:
            s = v.co.dot(d)
            if s > hinge:
                along, zz = s - hinge, v.co.z
                rest = v.co - d * s - UP * zz
                v.co = rest + d * (hinge + along * math.cos(bend) - zz * math.sin(bend)) + \
                    UP * (along * math.sin(bend) + zz * math.cos(bend))
        p.data.update()
    m.add_all(parts)
    letters(m, 'SHERIFF', (0.0, 0.0, T + 0.0006), 0.0052, UP, (0.0, 1.0, 0.0), look='black', tol=0.04, lift=0.0002,
            width=0.027)
    m.socket('Interact', (0.0, 0.0, 0.012), aim((0.0, -1.0, 0.0)))
    m.hull((0.1, 0.1, 0.02), at=(0.0, 0.0, 0.01))
    return m.done('its middle on the surface (lying face up)', ao=0.006, smooth=50.0, min_height=0.0002, crumb=0.0006,
                  lods=False)


# --- Sheriff Odell's hat ---

def sheriff_hat():
    m = Model('SheriffHat', seed=SEED + 110)
    SEG = 40
    profile = [(0.0, 0.0), (0.236, 0.0), (0.242, 0.006), (0.236, 0.012), (0.2, 0.0135), (0.1, 0.0145), (0.094, 0.02),
               (0.093, 0.05), (0.088, 0.07), (0.079, 0.122), (0.064, 0.138), (0.034, 0.135), (0.0, 0.128)]
    hat, bands = lp.lathe(profile, segments=SEG)
    bites = [(248.0, 24.0, 0.072), (318.0, 14.0, 0.045), (95.0, 10.0, 0.03)]
    for v in hat.data.vertices:
        x, y, z = v.co
        r = math.hypot(x, y)
        a = math.atan2(y, x)
        k = int(round((a % (2.0 * math.pi)) / (2.0 * math.pi / SEG))) % SEG
        if r > 0.1 and z < 0.02:
            f = (r - 0.1) / 0.14
            # the brim: its sides (x) curl up, front and back (y) dip
            for at, half, depth in bites:
                da = abs((math.degrees(a) - at + 180.0) % 360.0 - 180.0)
                if da < half:
                    w = math.cos(math.pi * 0.5 * da / half) ** 2
                    tooth = 0.3 * depth * (1.0 if k % 2 == 0 else -0.4)
                    r -= (depth * w + tooth * w) * min(1.0, (r - 0.1) / 0.13)
                    z -= 0.006 * w
            r = max(r, 0.105)
            z += 0.05 * f * f * math.cos(a) ** 2 - 0.01 * f * abs(math.sin(a))
        if z > 0.09:                                           # the cattleman's crease, front to back
            z -= 0.042 * math.exp(-(x / 0.024) ** 2) * min(1.0, (z - 0.09) / 0.035) * (1.0 - 0.3 * max(0.0, y) / 0.08)
        if z > 0.06 and y < -0.02:                             # pinched at the front
            r *= 1.0 - 0.22 * math.exp(-((z - 0.105) / 0.03) ** 2) * min(1.0, (-y - 0.02) / 0.04)
        da = abs((math.degrees(a) - 200.0 + 180.0) % 360.0 - 180.0)
        if 0.06 < z < 0.14 and da < 18.0:                      # a crushed dent in its crown
            r *= 1.0 - 0.2 * math.cos(math.pi * 0.5 * da / 18.0) ** 2 * math.sin(math.pi * (z - 0.06) / 0.08)
        v.co = Vector((r * math.cos(a), r * math.sin(a), z))
    hat.data.update()
    names = ['SextonLedgerCover' if bands[i] == 6 else 'MourningCrepe' for i in range(len(hat.data.polygons))]
    paint_faces(hat, names, seed=SEED + 111)
    lp.lathe_uv(hat, 'Polymer', faces=[i for i, n in enumerate(names) if n == 'MourningCrepe'], seed=SEED + 112)
    lp.lathe_uv(hat, 'Polymer', faces=[i for i, n in enumerate(names) if n != 'MourningCrepe'], seed=SEED + 113)
    a = math.radians(200.0)
    buckle = block((0.004, 0.014, 0.024), (0.0, 0.0, 0.0), 'BrassWorn', bevel=0.001, density=FINE, seed=SEED + 114)
    lp.place(buckle, (math.cos(a) * 0.0955, math.sin(a) * 0.0955, 0.035), (0.0, 0.0, math.degrees(a)))
    parts = [hat, buckle]
    tilt = Matrix.Rotation(math.radians(-5.0), 4, 'Y') @ Matrix.Rotation(math.radians(4.0), 4, 'X')
    move, lo, hi = settle(parts, tilt)
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((0.0, 0.0, 0.16))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi)
    return m.done('the middle of its footprint on the ground (crown up)', ao=0.12, smooth=50.0)


# --- Books ---

def journal():
    m = Model('Journal', seed=SEED + 120)
    W, D, T, CT = 0.14, 0.2, 0.03, 0.0028
    parts = [block((W, D, CT), (0.0, 0.0, CT * 0.5), 'SextonLedgerCover', bevel=0.0008, seed=SEED + 121),
             block((W, D, CT), (0.0, 0.0, T - CT * 0.5), 'SextonLedgerCover', bevel=0.0008, seed=SEED + 122),
             tube([(-W * 0.5 + 0.003, -D * 0.5, T * 0.5), (-W * 0.5 + 0.003, D * 0.5, T * 0.5)], T * 0.5,
                  'SextonLedgerCover', sides=10, seed=SEED + 123),
             block((W - 0.008, D - 0.008, T - 2.0 * CT), (0.003, 0.0, T * 0.5), 'SextonLedgerPages', seed=SEED + 124)]
    # The thong: two turns round it (in its section: round the fore-edge's two corners, over the top, round the spine,
    # back under), then its end along the top cover and off the head onto the table.
    x1, z0, z1, rr, rs = W * 0.5 + 0.0012, -0.0011, T + 0.0011, 0.004, T * 0.5 + 0.0012
    xs = -W * 0.5 + 0.003
    loop = []
    for k in range(4):                                          # the fore-edge's bottom corner
        a = math.radians(-90.0 + 90.0 * k / 3)
        loop.append((x1 - rr + rr * math.cos(a), z0 + rr + rr * math.sin(a)))
    for k in range(4):                                          # its top corner
        a = math.radians(90.0 * k / 3)
        loop.append((x1 - rr + rr * math.cos(a), z1 - rr + rr * math.sin(a)))
    for k in range(7):                                          # round the spine, top to bottom
        a = math.radians(90.0 + 180.0 * k / 6)
        loop.append((xs + rs * math.cos(a), T * 0.5 + rs * math.sin(a)))
    path = []
    for turn in range(2):
        for k, (x, z) in enumerate(loop if turn == 0 else loop[:8]):
            y = 0.02 + 0.026 * (turn + k / len(loop))
            path.append(Vector((x, y, z)))
    path += [Vector((0.035, 0.068, z1 + 0.0002)), Vector((0.0, 0.086, z1 + 0.0002)), Vector((-0.02, 0.098, z1)),
             Vector((-0.03, 0.106, T * 0.6)), Vector((-0.035, 0.113, 0.004)), Vector((-0.04, 0.14, 0.0008)),
             Vector((-0.045, 0.165, 0.0008))]
    thong = lp.sweep([tuple(p) for p in path], [(-0.0006, -0.003), (0.0006, -0.003), (0.0006, 0.003), (-0.0006, 0.003)],
                     up=(0.0, 1.0, 0.0))
    parts.append(paint(thong, 'SextonLedgerCover', seed=SEED + 125))
    parts.append(pencil((-0.005, -0.06, T + 0.0011), -62.0, SEED + 126))
    rib = lp.sweep([(0.02, -0.096, 0.012), (0.022, -0.103, 0.0115), (0.024, -0.108, 0.006), (0.026, -0.112, 0.0006),
                    (0.03, -0.135, 0.0005), (0.036, -0.15, 0.0005)],
                   [(-0.0035, -0.0002), (0.0035, -0.0002), (0.0035, 0.0002), (-0.0035, 0.0002)], up=(0.0, 0.0, 1.0))
    parts.append(paint(rib, 'ClothRed', seed=SEED + 127))
    m.add_all(parts)
    letters(m, 'R.C.', (0.02, -0.004, T), 0.012, UP, (0.0, 1.0, 0.0), look='black', tol=0.05, lift=0.0002)
    m.socket('Interact', (0.0, 0.0, T + 0.02), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.02, D + 0.02, T + 0.004), at=(0.0, 0.0, (T + 0.004) * 0.5))
    return m.done('the middle of its underside on the table', ao=0.03, min_height=0.0003, crumb=0.001)


def code_book():
    m = Model('CodeBook', seed=SEED + 130)
    W, D, T, CT = 0.12, 0.17, 0.04, 0.003
    parts = [block((W, D, CT), (0.0, 0.0, CT * 0.5), 'ClothBlue', bevel=0.0008, seed=SEED + 131),
             block((W, D, CT), (0.0, 0.0, T - CT * 0.5), 'ClothBlue', bevel=0.0008, seed=SEED + 132),
             tube([(-W * 0.5 + 0.004, -D * 0.5 - 0.0005, T * 0.5), (-W * 0.5 + 0.004, D * 0.5 + 0.0005, T * 0.5)],
                  T * 0.5 + 0.0005, 'SextonLedgerCover', sides=10, seed=SEED + 133),
             block((W - 0.008, D - 0.008, T - 2.0 * CT), (0.003, 0.0, T * 0.5), 'SextonLedgerPages', seed=SEED + 134)]
    for z in (T + 0.0003, -0.0003):                             # the leather onto both boards along the spine
        parts.append(block((0.024, D + 0.001, 0.0008), (-W * 0.5 + 0.012, 0.0, z), 'SextonLedgerCover', seed=SEED + 135))
    for sy in (-1.0, 1.0):                                      # leather corners at the fore-edge
        tri = [(W * 0.5 + 0.0006, sy * (D * 0.5 + 0.0006)), (W * 0.5 - 0.024, sy * (D * 0.5 + 0.0006)),
               (W * 0.5 + 0.0006, sy * (D * 0.5 - 0.024))]
        if sy > 0.0:
            tri.reverse()
        parts.append(flat([tri], T, T + 0.0008, 'SextonLedgerCover', seed=SEED + 136))
    clasp = lp.sweep([(W * 0.5 - 0.028, 0.0, T + 0.0009), (W * 0.5 + 0.0012, 0.0, T + 0.0009), (W * 0.5 + 0.0032, 0.0, T * 0.5),
                      (W * 0.5 + 0.0012, 0.0, -0.0009), (W * 0.5 - 0.02, 0.0, -0.0009)],
                     [(-0.0006, -0.006), (0.0006, -0.006), (0.0006, 0.006), (-0.0006, 0.006)], up=(0.0, 1.0, 0.0))
    parts.append(paint(clasp, 'BrassWorn', density=FINE, seed=SEED + 137))
    parts.append(block((0.009, 0.015, 0.0028), (W * 0.5 - 0.024, 0.0, T + 0.0016), 'BrassWorn', bevel=0.0008,
                       density=FINE, seed=SEED + 138))
    # The Rangers' star in its ring, and SIGNALS, in gilt.
    sx, sy = 0.004, 0.028
    parts.append(flat([circle(sx, sy, 0.02, 24), circle(sx, sy, 0.0172, 24, reverse=True)], T, T + 0.0004, 'BrassWorn',
                      density=FINE, seed=SEED + 139))
    parts.append(flat([star(sx, sy, 0.0155, 0.0062, 5)], T, T + 0.0004, 'BrassWorn', density=FINE, seed=SEED + 140))
    # The folded signal card sticking out of its top end, dots and dashes on it.
    def card_z(y):
        return T * 0.5 + 0.009 + 0.08 * (y - D * 0.5)
    card_rows = [[Vector((x, y, card_z(y))) for x in (-0.03, 0.0, 0.03)]
                 for y in (D * 0.5 - 0.03, D * 0.5 - 0.005, D * 0.5 + 0.042)]
    parts.append(slab(card_rows, 0.0006, 'SextonLedgerPages', seed=SEED + 141))
    for k, (x, length) in enumerate(((-0.022, 0.003), (-0.016, 0.009), (-0.004, 0.003), (0.002, 0.003),
                                     (0.008, 0.009), (0.02, 0.003))):
        for row, y in enumerate((D * 0.5 + 0.018, D * 0.5 + 0.03)):
            xx = x + (0.004 if row else 0.0)
            parts.append(ribbon([(xx, y, card_z(y) + 0.0004), (xx + length, y, card_z(y) + 0.0004)], 0.0022,
                                (0.0, -0.08, 1.0), seed=SEED + 142 + k))
    m.add_all(parts)
    letters(m, 'SIGNALS', (sx, -0.03, T), 0.0095, UP, (0.0, 1.0, 0.0), look='brass', tol=0.05, lift=0.0003, width=0.085)
    m.socket('Interact', (0.0, 0.0, T + 0.02), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.02, D + 0.07, T + 0.006), at=(0.0, 0.021, (T + 0.006) * 0.5))
    return m.done('the middle of its underside on the shelf', ao=0.03, min_height=0.0003, crumb=0.001)


def post_log():
    m = Model('PostLog', seed=SEED + 150)
    rnd = random.Random(SEED + 150)
    PW, PD, CT = 0.22, 0.3, 0.003
    parts = []
    for sx in (-1.0, 1.0):
        parts.append(block((PW + 0.006, PD + 0.006, CT), (sx * (PW * 0.5 + 0.003 + 0.002), 0.0, CT * 0.5), 'ClothRed',
                           bevel=0.0008, seed=SEED + 151 + int(sx > 0)))
    parts.append(block((0.012, PD + 0.004, 0.0012), (0.0, 0.0, 0.0006), 'ClothRed', seed=SEED + 153))

    def page_z(xp):
        """The page block's top over the cover, by the distance from the gutter."""
        rise = min(1.0, xp / 0.035)
        z = 0.0055 + 0.0085 * (3.0 * rise * rise - 2.0 * rise ** 3)
        if xp > 0.198:
            z -= 0.003 * ((xp - 0.198) / 0.014) ** 2
        return CT + z
    xs = [0.0015, 0.008, 0.018, 0.03, 0.05, 0.09, 0.13, 0.17, 0.198, 0.212]
    ys = [-0.147, -0.07, 0.0, 0.07, 0.147]
    for sx in (-1.0, 1.0):
        # The block: its top surface over a flat bottom on the board.
        bm = lp.new_bmesh()
        top = [[bm.verts.new((sx * x, y, page_z(x))) for x in xs] for y in ys]
        bot = [[bm.verts.new((sx * x, y, CT)) for x in xs] for y in ys]
        R, C = len(ys), len(xs)
        for j in range(R - 1):
            for i in range(C - 1):
                bm.faces.new((top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i]))
                bm.faces.new((bot[j][i], bot[j + 1][i], bot[j + 1][i + 1], bot[j][i + 1]))
        for i in range(C - 1):
            bm.faces.new((top[0][i], bot[0][i], bot[0][i + 1], top[0][i + 1]))
            bm.faces.new((top[R - 1][i + 1], bot[R - 1][i + 1], bot[R - 1][i], top[R - 1][i]))
        for j in range(R - 1):
            bm.faces.new((top[j + 1][0], bot[j + 1][0], bot[j][0], top[j][0]))
            bm.faces.new((top[j][C - 1], bot[j][C - 1], bot[j + 1][C - 1], top[j + 1][C - 1]))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        parts.append(paint(lp.mesh_object(bm), 'SextonLedgerPages', seed=SEED + 154 + int(sx > 0)))

    def on_page(x, y, lift=0.00035):
        return Vector((x, y, page_z(abs(x)) + lift))
    # Red rules (a header line, the date column) and the inked entries: the left page full, the right half full.
    for sx in (-1.0, 1.0):
        parts.append(ribbon([on_page(sx * x, 0.128) for x in (0.02, 0.06, 0.12, 0.2)], 0.0011, UP, 'ClothRed',
                            density=None, seed=SEED + 156))
        parts.append(ribbon([on_page(sx * 0.163, y) for y in (0.14, 0.0, -0.14)], 0.0011, UP, 'ClothRed', density=None,
                            seed=SEED + 157))
        rows = 11 if sx < 0.0 else 5
        for k in range(rows):
            y = 0.112 - 0.022 * k
            date = [on_page(sx * x, y + 0.0015 * math.sin(x * 400.0)) for x in (0.172, 0.18, 0.19)]
            parts.append(ribbon(date, 0.0013, UP, seed=SEED + 160 + k))
            end = rnd.uniform(0.04, 0.09)
            line = [on_page(sx * x, y + 0.0018 * math.sin(x * 260.0 + k)) for x in (0.155, 0.12, 0.09, end)]
            parts.append(ribbon(line, 0.0014, UP, seed=SEED + 180 + k))
    # The pencil on the right page, its string from the head of the spine.
    p_at = Vector((0.11, -0.045, page_z(0.11) + 0.0003))
    parts.append(pencil(tuple(p_at), 38.0, SEED + 200))
    yaw = math.radians(38.0)
    p_end = p_at + Vector((-math.sin(yaw), math.cos(yaw), 0.0)) * 0.065 + Vector((0.0, 0.0, 0.0036))
    string = [Vector((0.0, PD * 0.5 + 0.004, 0.006)), on_page(0.03, 0.13, 0.0013), on_page(0.06, 0.07, 0.0013),
              on_page(0.07, 0.03, 0.0013), p_end]
    parts.append(tube(string, 0.0011, 'RopeHemp', sides=4, seed=SEED + 201, density=640.0))
    m.add_all(parts)
    m.socket('Interact', (0.0, 0.0, 0.04), aim((0.0, -1.0, 0.0)))
    m.hull((2.0 * PW + 0.03, PD + 0.02, 0.025), at=(0.0, 0.0, 0.0125))
    return m.done('the middle of its spine on the table (open, its pages left and right, the reader at -Y)', ao=0.04,
                  min_height=0.0003, crumb=0.001)


def payroll_book():
    m = Model('PayrollBook', seed=SEED + 210)
    W, D, T, CT = 0.26, 0.36, 0.045, 0.004
    parts = [block((W, D, CT), (0.0, 0.0, CT * 0.5), 'MourningCrepe', bevel=0.001, seed=SEED + 211),
             block((W, D, CT), (0.0, 0.0, T - CT * 0.5), 'MourningCrepe', bevel=0.001, seed=SEED + 212),
             tube([(-W * 0.5 + 0.004, -D * 0.5 - 0.0005, T * 0.5), (-W * 0.5 + 0.004, D * 0.5 + 0.0005, T * 0.5)],
                  T * 0.5 + 0.0006, 'SextonLedgerCover', sides=12, seed=SEED + 213),
             block((W - 0.01, D - 0.01, T - 2.0 * CT), (0.004, 0.0, T * 0.5), 'SextonLedgerPages', seed=SEED + 214)]
    for z in (T + 0.0004, -0.0004):
        parts.append(block((0.036, D + 0.001, 0.0009), (-W * 0.5 + 0.018, 0.0, z), 'SextonLedgerCover', seed=SEED + 215))
    for k, y in enumerate((-0.11, 0.0, 0.11)):                 # raised bands across the spine
        band = [(-W * 0.5 + 0.004 + (T * 0.5 + 0.0018) * math.cos(a), y, T * 0.5 + (T * 0.5 + 0.0018) * math.sin(a))
                for a in (math.radians(80.0 + 200.0 * j / 10) for j in range(11))]
        parts.append(tube(band, 0.0028, 'SextonLedgerCover', sides=5, seed=SEED + 216 + k))
    for sy in (-1.0, 1.0):
        tri = [(W * 0.5 + 0.0008, sy * (D * 0.5 + 0.0008)), (W * 0.5 - 0.04, sy * (D * 0.5 + 0.0008)),
               (W * 0.5 + 0.0008, sy * (D * 0.5 - 0.04))]
        if sy > 0.0:
            tri.reverse()
        parts.append(flat([tri], T, T + 0.0009, 'SextonLedgerCover', seed=SEED + 219))
    # The paper label, ruled round, and two pay slips sticking out of the fore-edge.
    lx, ly = 0.018, 0.06
    parts.append(block((0.13, 0.078, 0.0006), (lx, ly, T + 0.0003), 'SextonLedgerPages', seed=SEED + 220))
    corners = [(lx - 0.059, ly - 0.033), (lx + 0.059, ly - 0.033), (lx + 0.059, ly + 0.033), (lx - 0.059, ly + 0.033)]
    for a, b in zip(corners, corners[1:] + corners[:1]):
        parts.append(ribbon([(a[0], a[1], T + 0.00075), (b[0], b[1], T + 0.00075)], 0.0011, UP, seed=SEED + 221))
    for k, (y, turn, out) in enumerate(((-0.08, 7.0, 0.048), (0.03, -4.0, 0.03))):
        x0 = W * 0.5 - 0.05
        rows = [[Vector((x0 + (0.05 + out) * s, y + d, T * 0.5 - 0.004 + 0.007 * k)) for s in (0.0, 0.5, 1.0)]
                for d in (-0.035, 0.0, 0.035)]
        slip = slab(rows, 0.0005, 'SextonLedgerPages', seed=SEED + 222 + k)
        transform([slip], Matrix.Translation((x0, y, 0.0)) @ Matrix.Rotation(math.radians(turn), 4, 'Z')
                  @ Matrix.Translation((-x0, -y, 0.0)))
        parts.append(slip)
    m.add_all(parts)
    letters(m, 'PAYROLL', (lx, ly + 0.008, T + 0.0006), 0.016, UP, (0.0, 1.0, 0.0), look='black', tol=0.05, lift=0.0002,
            width=0.105)
    letters(m, 'P.M.CO.', (lx, ly - 0.018, T + 0.0006), 0.0095, UP, (0.0, 1.0, 0.0), look='black', tol=0.05, lift=0.0002)
    m.socket('Interact', (0.0, 0.0, T + 0.02), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.07, D + 0.01, T + 0.004), at=(0.025, 0.0, (T + 0.004) * 0.5))
    return m.done('the middle of its underside on the desk', ao=0.05, min_height=0.0003, crumb=0.001)


# --- The Deacon's slip ---

def deacons_slip():
    m = Model('DeaconsSlip', seed=SEED + 230)
    L, H, OPEN = 0.17, 0.055, math.radians(58.0)
    # A sheet folded once across its middle: the lower leaf flat, the upper raised OPEN round the fold (along X at
    # y = 0), its corners curling a little.
    us = [L * k / 6 - L * 0.5 for k in range(7)]
    vs = [-H + H * k / 3 for k in range(4)] + [H * k / 3 for k in range(1, 4)]

    def point(u, v):
        curl = 0.004 * (abs(u) / (L * 0.5)) ** 3
        if v <= 0.0:
            return Vector((u, v * (1.0 - 0.02 * abs(u) / L), 0.0006 + curl * (-v / H) ** 2))
        lift = v + curl * 0.0
        return Vector((u, math.cos(OPEN) * lift, 0.0006 + math.sin(OPEN) * lift + curl * (v / H) ** 2))

    def down(j, i):
        v = vs[j]
        if v <= 0.0:
            return Vector((0.0, 0.0, -1.0))
        return Vector((0.0, math.sin(OPEN), -math.cos(OPEN)))
    rows = [[point(u, v) for u in us] for v in vs]
    parts = [slab(rows, 0.0004, 'SextonLedgerPages', down=down, seed=SEED + 231)]
    # His hand: lines of scrawl on the inside of both leaves.
    rnd = random.Random(SEED + 232)
    n_low = UP
    n_up = Vector((0.0, -math.sin(OPEN), math.cos(OPEN)))
    for k, v in enumerate((-0.012, -0.024, -0.036)):
        x0, x1 = -0.065 + rnd.uniform(0.0, 0.01), 0.06 - (0.05 if k == 2 else rnd.uniform(0.0, 0.02))
        pts = [point(x0 + (x1 - x0) * s, v + 0.0015 * math.sin(s * 23.0 + k)) + n_low * 0.0003 for s in
               (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)]
        parts.append(ribbon(pts, 0.0016, n_low, seed=SEED + 233 + k))
    for k, v in enumerate((0.038, 0.026, 0.014)):
        x0, x1 = -0.065, 0.065 - rnd.uniform(0.0, 0.03)
        pts = [point(x0 + (x1 - x0) * s, v + 0.0015 * math.sin(s * 19.0 + k)) + n_up * 0.0003 for s in
               (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)]
        parts.append(ribbon(pts, 0.0016, n_up, seed=SEED + 240 + k))
    sig = [point(0.02 + 0.04 * s, -0.047 + 0.004 * math.sin(s * 9.0)) + n_low * 0.0003 for s in (0.0, 0.25, 0.5, 0.75, 1.0)]
    parts.append(ribbon(sig, 0.0018, n_low, seed=SEED + 246))
    move, lo, hi = settle(parts)
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((0.0, 0.0, 0.05))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi, least=0.05)
    return m.done('the middle of its footprint on the surface (folded, half open)', ao=0.03, min_height=0.0002,
                  crumb=0.0006)


# --- The seeds (2026-10-16): the Ledger's loose page, the Deacon's letter, the saloon's post rack, the Reaches map ---

def ribbon_on(points, width, normals, name='PaintBlack', density=FINE, seed=0):
    """ribbon(), but each point with its own normal: ink on a curling sheet."""
    bm = lp.new_bmesh()
    pts = [Vector(p) for p in points]
    left, right = [], []
    for i, p in enumerate(pts):
        n = Vector(normals[i]).normalized()
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        s = t.cross(n).normalized() * (width * 0.5)
        left.append(bm.verts.new(p - s))
        right.append(bm.verts.new(p + s))
    for i in range(len(pts) - 1):
        f = bm.faces.new((left[i], right[i], right[i + 1], left[i + 1]))
        f.normal_update()
        if f.normal.dot(Vector(normals[i])) < 0.0:
            f.normal_flip()
    part = lp.mesh_object(bm)
    return paint(part, name, density=density, seed=seed)


def scrawl(rnd, x0, x1, y, h, step=0.002, slant=0.0007, gaps=True):
    """Copperplate at a distance: words as zigzags of pen strokes along y (2D, x right, y up the page), now and then
    a loop twice as tall (an ascender) or dropping below (a descender). Returns a list of words, each a list of
    (x, y) points; every stroke at least step long, so no triangle is under the kit's 1 mm^2."""
    words = []
    x = x0
    while x < x1 - 0.006:
        end = min(x1, x + rnd.uniform(0.012, 0.032)) if gaps else x1
        pts, k = [], 0
        while x <= end:
            up = k % 2 == 0
            a = h * 0.4
            r = rnd.random()
            if up and r < 0.16:
                a = h * 0.95
            elif not up and r < 0.1:
                a = h * 0.85
            pts.append((x + (slant if up else -slant), y + (a if up else -a * 0.55) + h * 0.15))
            x += step
            k += 1
        if len(pts) >= 2:
            words.append(pts)
        x += rnd.uniform(0.0028, 0.0042)
    return words


def ledger_page():
    """A loose leaf of Sexton's ledger: ruled in blue, a red header and money columns, his copperplate on it, its far
    right corner lifted off the surface."""
    m = Model('LedgerPage', seed=SEED + 500)
    rnd = random.Random(SEED + 500)
    W, D = 0.2, 0.32

    def surface(x, y):
        """The page's top surface over (x, y): flat but for a faint cockle and the far right corner lifting."""
        s = max(0.0, (x / (W * 0.5) + y / (D * 0.5) - 1.1) / 0.9)
        lift = 0.028 * s ** 2.2
        cockle = 0.0005 * math.sin(x * 37.0 + 0.6) * math.sin(y * 23.0 + 1.1)
        return Vector((x, y, 0.0005 + lift + cockle))

    def normal(x, y):
        e = 0.002
        dx = surface(x + e, y) - surface(x - e, y)
        dy = surface(x, y + e) - surface(x, y - e)
        return dx.cross(dy).normalized()

    def on(x, y, lift=0.00025):
        return surface(x, y) + normal(x, y) * lift
    xs = [W * (k / 8.0 - 0.5) for k in range(9)]
    ys = [D * (k / 12.0 - 0.5) for k in range(13)]
    rows = [[surface(x, y) for x in xs] for y in ys]
    parts = [slab(rows, 0.0003, 'SextonLedgerPages', seed=SEED + 501)]

    def line(points2d, width, name, lift=0.00025, density=FINE, seed=0):
        pts = [on(x, y, lift) for x, y in points2d]
        return ribbon_on(pts, width, [normal(x, y) for x, y in points2d], name, density=density, seed=seed)

    def across(x0, x1, n):
        return [x0 + (x1 - x0) * k / (n - 1) for k in range(n)]
    # The ruling: blue lines every 9 mm, the red header (doubled) and the date and money columns.
    for k in range(28):
        y = 0.112 - 0.009 * k
        parts.append(line([(x, y) for x in across(-0.096, 0.096, 7)], 0.0007, 'ClothBlue', density=None,
                          seed=SEED + 502))
    for y in (0.123, 0.1185):
        parts.append(line([(x, y) for x in across(-0.096, 0.096, 7)], 0.0009, 'ClothRed', lift=0.0003, density=None,
                          seed=SEED + 503))
    for x in (-0.064, 0.05, 0.057):
        parts.append(line([(x, y) for y in across(0.123, -0.156, 9)], 0.0008, 'ClothRed', lift=0.0003, density=None,
                          seed=SEED + 504))
    ink = []
    # The heading (the saint and the town), bigger, over the header rule.
    ink += scrawl(rnd, -0.07, 0.02, 0.131, 0.0062, step=0.0024, slant=0.001)
    # The entries: a date, a line of his hand, a sum in the money column; the last few lines left short.
    for k in range(10):
        y = 0.104 - 0.009 * k
        ink += scrawl(rnd, -0.09, -0.07, y, 0.0036, gaps=False)
        ink += scrawl(rnd, -0.058, 0.044 - (rnd.uniform(0.0, 0.03) if k % 3 == 2 else 0.0), y, 0.0038)
        if k % 2 == 0 or k == 9:
            ink += scrawl(rnd, 0.061, 0.088, y, 0.0036, gaps=False)
    # The sum, ruled off twice under it.
    for y in (0.012, 0.0095):
        parts.append(line([(0.059, y), (0.074, y), (0.09, y)], 0.0008, 'PaintBlack', lift=0.0003))
    ink += scrawl(rnd, 0.061, 0.09, 0.0025, 0.0042, gaps=False)
    # The margin's note, written slantwise low on the page, underlined.
    turn = math.radians(5.0)
    for k, y in enumerate((-0.11, -0.122, -0.134)):
        for word in scrawl(rnd, -0.06, 0.035 - 0.02 * k, y, 0.0036):
            ink.append([(x * math.cos(turn) - (yy + 0.12) * math.sin(turn), (yy + 0.12) * math.cos(turn) +
                         x * math.sin(turn) - 0.12) for x, yy in word])
    parts.append(line([(-0.06, -0.152), (-0.02, -0.1505), (0.03, -0.148)], 0.0009, 'PaintBlack', lift=0.0003))
    for k, word in enumerate(ink):
        parts.append(line(word, 0.0009, 'PaintBlack', lift=0.0003, seed=SEED + 510 + k % 7))
    move, lo, hi = settle(parts)
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((0.0, 0.0, 0.05))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi, least=0.05)
    return m.done('the middle of its footprint on the surface (lying flat, the far right corner lifted)', ao=0.03,
                  min_height=0.0002, crumb=0.0006, lods=False)


def deacon_letter():
    """The Deacon's letter, folded in three and sealed: the outer fold's flap over the packet, the address in his hand
    on it, a blob of red wax on the flap's edge. Pivot: the middle of its footprint, lying flat, the address reading
    from -Y (its top toward +Y)."""
    m = Model('DeaconLetter', seed=SEED + 560)
    rnd = random.Random(SEED + 560)
    W, D, FLAP = 0.11, 0.18, -0.03

    def packet(x, y):
        bow = 0.0012 * (1.0 - (2.0 * x / W) ** 2) * (1.0 - 0.6 * (2.0 * y / D) ** 2)
        return Vector((x, y, 0.0006 + bow))

    def flap(x, y):
        """The outer fold's flap: over the packet from its top edge down to FLAP, its free edge springing up."""
        rise = 0.0016 * max(0.0, (0.02 - (y - FLAP)) / 0.02) ** 2
        return packet(x, y) + Vector((0.0, 0.0, 0.0004 + rise))
    xs = [W * (k / 4.0 - 0.5) for k in range(5)]
    ys = [D * (k / 6.0 - 0.5) for k in range(7)]
    parts = [slab([[packet(x, y) for x in xs] for y in ys], 0.0006, 'SextonLedgerPages', seed=SEED + 561)]
    fys = [FLAP + (D * 0.5 - FLAP) * k / 5.0 for k in range(6)]
    fxs = [x * 0.995 for x in xs]
    parts.append(slab([[flap(x, y) for x in fxs] for y in fys], 0.0003, 'SextonLedgerPages', seed=SEED + 562))
    # The address: three lines in his hand, the last indented, the town's name underlined.
    ink = []
    for k, (x0, x1) in enumerate(((-0.032, 0.03), (-0.022, 0.036), (-0.008, 0.04))):
        ink += [(w, 0.0007) for w in scrawl(rnd, x0, x1, 0.048 - 0.013 * k, 0.0042)]
    ink.append(([(-0.006, 0.014), (0.016, 0.0135), (0.04, 0.0145)], 0.0008))
    for k, (word, width) in enumerate(ink):
        pts = [flap(x, y) + UP * 0.0003 for x, y in word]
        parts.append(ribbon(pts, width + 0.0002, UP, seed=SEED + 563 + k % 5))
    # The wax seal on the flap's edge: a squashed red blob with a stamped ring.
    seal = blob((0.0, FLAP + 0.002, 0.0028), (0.0095, 0.0085, 0.0018), SEED + 570, 'PaintOxide', segs=10, rings=4,
                lumpy=0.1, density=FINE)
    parts.append(seal)
    ring_pts = [(0.0055 * math.cos(2.0 * math.pi * k / 12), FLAP + 0.002 + 0.005 * math.sin(2.0 * math.pi * k / 12),
                 0.0047) for k in range(13)]
    parts.append(ribbon(ring_pts, 0.0011, UP, 'PaintOxide', seed=SEED + 571))
    move, lo, hi = settle(parts)
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((0.0, 0.0, 0.05))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi, least=0.05)
    return m.done('the middle of its footprint on the surface (lying flat, sealed; the address reads from -Y)',
                  ao=0.03, min_height=0.0002, crumb=0.0006, lods=False)


# The post rack's pigeonholes: x of each slot's middle, and the floor each row's letters stand on (z, in the rack's
# frame: its pivot the middle of its back face, front -Y). Slots 1-4 the upper row left to right, 5-8 the lower.
RACK_W, RACK_H, RACK_D = 0.6, 0.42, 0.075
RACK_SLOT_X = (-0.2175, -0.0725, 0.0725, 0.2175)
RACK_FLOORS = (0.0075, -0.195)
RACK_LEAN = 8.0                      # degrees a letter leans back against the rack in its slot


def rack_slot(floor, x):
    """SOCKET_Slot_<n>'s place and rotation (DATA's contract): where a standing letter's bottom middle sits on the
    slot's floor, near the slat; its +Z up the slot (leaning back RACK_LEAN degrees against the rack), its front (-Y)
    out of the rack."""
    return (x, -0.056, floor + 0.0004), (-RACK_LEAN, 0.0, 0.0)


def post_rack():
    """The saloon's post rack: a pine box of eight pigeonholes in two rows, a slat across each row's front with the
    slots' numbers painted on it, nailed to the boarded front. Two old letters nobody came for stand in slots 6 and 8;
    the readable letter (SM_DeaconLetter) goes in slot 3 at SOCKET_Slot_3 (see rack_slot)."""
    m = Model('PostRack', seed=SEED + 600)
    W, H, Dp = RACK_W, RACK_H, RACK_D
    T = 0.015
    look = 'planks'
    # The back: two wide boards on edge.
    for z in (-H * 0.25, H * 0.25):
        town.board(m, (-W * 0.5, -T * 0.5, z), (W * 0.5, -T * 0.5, z), H * 0.5 - 0.002, T, look=look,
                   face=(0.0, -1.0, 0.0), drop=('+y',))
    # The sides, the top, the bottom and the middle shelf.
    for sx in (-1.0, 1.0):
        town.board(m, (sx * (W * 0.5 - T * 0.5), -T - (Dp - T) * 0.5, -H * 0.5),
                   (sx * (W * 0.5 - T * 0.5), -T - (Dp - T) * 0.5, H * 0.5), Dp - T, T, look=look,
                   face=(sx, 0.0, 0.0))
    town.board(m, (-W * 0.5 - 0.008, -Dp * 0.5 - 0.004, H * 0.5 + T * 0.5), (W * 0.5 + 0.008, -Dp * 0.5 - 0.004,
                                                                            H * 0.5 + T * 0.5),
               Dp + 0.008, T, look=look, face=(0.0, 0.0, 1.0))
    for z in (-H * 0.5 + T * 0.5, 0.0):
        town.board(m, (-W * 0.5 + T, -T - (Dp - T) * 0.5, z), (W * 0.5 - T, -T - (Dp - T) * 0.5, z), Dp - T, T,
                   look=look, face=(0.0, 0.0, 1.0))
    # Each row: three dividers and a slat across its front.
    SH = 0.075
    for floor in RACK_FLOORS:
        for x in ((RACK_SLOT_X[0] + RACK_SLOT_X[1]) * 0.5, 0.0, (RACK_SLOT_X[2] + RACK_SLOT_X[3]) * 0.5):
            town.board(m, (x, -T - (Dp - T) * 0.5, floor), (x, -T - (Dp - T) * 0.5, floor + 0.13), Dp - T - 0.012,
                       0.01, look=look, face=(1.0, 0.0, 0.0))
        town.board(m, (-W * 0.5 + T, -Dp + 0.006, floor + SH * 0.5), (W * 0.5 - T, -Dp + 0.006, floor + SH * 0.5),
                   SH, 0.012, look=look, face=(0.0, -1.0, 0.0))
    # The slots' numbers, painted black on the slats.
    for r, floor in enumerate(RACK_FLOORS):
        for c, x in enumerate(RACK_SLOT_X):
            letters(m, str(1 + c + 4 * r), (x, -Dp, floor + SH * 0.52), 0.03, (0.0, -1.0, 0.0), UP, look='black',
                    tol=0.015, lift=0.0005)
    parts = []
    # Nailed to the wall through the back, a nail in each corner pigeonhole.
    for x in (-W * 0.5 + 0.04, W * 0.5 - 0.04):
        for z in (H * 0.5 - 0.035, -H * 0.5 + 0.05):
            parts.append(block((0.011, 0.003, 0.011), (x, -T - 0.0012, z), 'IronBlack', density=FINE,
                               seed=SEED + 601))
    # Two old letters nobody came for: plain folded packets leaning in slots 6 and 8.
    for k, slot in enumerate((6, 8)):
        x = RACK_SLOT_X[(slot - 1) % 4]
        loc, rot = rack_slot(RACK_FLOORS[(slot - 1) // 4], x)
        w, d = 0.095 - 0.01 * k, 0.15 + 0.012 * k
        rows = [[Vector((sx * w * 0.5, -0.0012 - 0.001 * (1.0 - sx * sx) * t, d * t)) for sx in (-1.0, 0.0, 1.0)]
                for t in (0.0, 0.5, 1.0)]
        card = slab(rows, 0.0006, 'SextonLedgerPages', down=(0.0, 1.0, 0.0), seed=SEED + 602 + k)
        turn = Matrix.Rotation(math.radians(rot[0] - 2.0 + 4.0 * k), 4, 'X')
        transform([card], Matrix.Translation(Vector(loc) + Vector((0.004 - 0.008 * k, 0.0, 0.0))) @ turn)
        parts.append(card)
    m.add_all(parts)
    for r, floor in enumerate(RACK_FLOORS):
        for c, x in enumerate(RACK_SLOT_X):
            loc, rot = rack_slot(floor, x)
            m.socket(f'Slot_{1 + c + 4 * r}', loc, rot)
    m.hull((W + 0.016, Dp + 0.004, H + T), at=(0.0, -(Dp + 0.004) * 0.5, T * 0.5))
    return m.done("the middle of its back face (on the wall), front -Y", ao=0.06, ground=False)


MAP_W, MAP_H = 0.9, 0.6
MAP_PINS = ((-0.436, 0.286), (0.436, 0.286), (-0.436, -0.286), (0.436, -0.286))


def reaches_map():
    """The depot's wall map of the Reaches: a paper sheet pinned at its corners (its own texture, MI_ReachesMap), a
    little proud of the wall between the pins and its bottom edge curling out. Pivot: the middle of its back, on the
    wall; front -Y, north up."""
    m = Model('ReachesMap', seed=SEED + 650)

    def front(x, z):
        """The printed face over (x, z): out from the wall (-y) by its thickness, a bulge between the pins and the
        bottom edge's curl."""
        u, v = 2.0 * x / MAP_W, 2.0 * z / MAP_H
        bulge = 0.0035 * (1.0 - u * u) * (1.0 - v * v)
        curl = 0.009 * max(0.0, -v) ** 3 * (1.0 - u * u) ** 1.5
        return Vector((x, -0.0007 - bulge - curl, z))
    xs = [MAP_W * (k / 10.0 - 0.5) for k in range(11)]
    zs = [MAP_H * (k / 8.0 - 0.5) for k in range(9)]
    rows = [[front(x, z) for x in xs] for z in zs]
    face = sheet(rows, 'SextonLedgerPages', (0.0, -1.0, 0.0))
    face.data.materials.clear()
    face.data.materials.append(mat('ReachesMap'))
    uv = face.data.uv_layers.active.data
    for poly in face.data.polygons:
        for li in poly.loop_indices:
            co = face.data.vertices[face.data.loops[li].vertex_index].co
            uv[li].uv = (co.x / MAP_W + 0.5, co.z / MAP_H + 0.5)
    # Its back and edges: plain paper a hair behind the printed face.
    back = slab([[front(x, z) + Vector((0.0, 0.0002, 0.0)) for x in xs] for z in zs], 0.0005, 'SextonLedgerPages',
                down=(0.0, 1.0, 0.0), seed=SEED + 651)
    parts = [face, back]
    # The four brass pins, domed.
    for k, (x, z) in enumerate(MAP_PINS):
        at = front(x, z)
        pin = lathe([(0.0, 0.0), (0.0062, 0.0), (0.006, 0.0011), (0.005, 0.0024), (0.003, 0.0034), (0.0, 0.0037)], 10,
                    'BrassWorn', density=FINE, seed=SEED + 652 + k, at=(at.x, at.y + 0.0002, at.z), rot=(90.0, 0.0, 0.0))
        parts.append(pin)
    m.add_all(parts)
    m.socket('Interact', (0.0, -0.06, 0.0), aim((0.0, -1.0, 0.0)))
    m.hull((MAP_W + 0.01, 0.02, MAP_H + 0.01), at=(0.0, -0.01, 0.0))
    return m.done("the middle of its back (on the wall), front -Y, north up", ao=0.03, ground=False, lods=False)


# --- The coal-oil can ---

def lamp_oil():
    m = Model('LampOil', seed=SEED + 250)
    W, D, H = 0.15, 0.1, 0.235
    outline = rounded_rect(W, D, 0.016, 3)
    body = lp.mesh_object(kit._prism([outline], H))
    body.data.transform(Matrix(((1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, -1.0, 0.0, H), (0.0, 0.0, 0.0, 1.0))))
    body.data.update()
    lp.slice_at(body, (0.0, 0.0, 1.0), (0.07, 0.12, 0.17))
    names = []
    for p in body.data.polygons:
        band = abs(p.normal.z) < 0.5 and 0.07 < p.center.z < 0.17
        names.append('PaintCream' if band else 'PaintOxide')
    paint_faces(body, names, density=2560.0, seed=SEED + 251)
    parts = [body]
    wide = rounded_rect(W + 0.005, D + 0.005, 0.0185, 3)
    for z0, z1 in ((0.0, 0.008), (H - 0.006, H + 0.003)):      # the seams round its foot and top
        parts.append(flat([wide], z0, z1, 'PaintOxide', density=2560.0, seed=SEED + 252))
    parts.append(lathe([(0.0, H), (0.013, H), (0.013, H + 0.015), (0.011, H + 0.017), (0.011, H + 0.023),
                        (0.0145, H + 0.023), (0.0145, H + 0.037), (0.012, H + 0.04), (0.0, H + 0.04)], 10, 'IronBlack',
                       density=FINE, seed=SEED + 253, at=(0.04, 0.02, 0.0)))
    # The handle: a wire bail between two ears, a wooden grip on it.
    hy = -0.015
    for sx in (-1.0, 1.0):
        parts.append(block((0.012, 0.01, 0.014), (sx * 0.055, hy, H + 0.009), 'IronBlack', density=FINE,
                           seed=SEED + 254))
    arch = [(0.055 * math.cos(math.pi * k / 14), hy, H + 0.012 + 0.06 * math.sin(math.pi * k / 14)) for k in range(15)]
    parts.append(tube(arch, 0.0026, 'IronBlack', sides=5, density=FINE, seed=SEED + 255))
    parts.append(lathe([(0.0, -0.03), (0.007, -0.03), (0.0095, -0.022), (0.01, 0.0), (0.0095, 0.022), (0.007, 0.03),
                        (0.0, 0.03)], 8, 'HouseTrim', strip='Beams', grain='up', seed=SEED + 256,
                       at=(0.0, hy, H + 0.072), rot=(0.0, 90.0, 0.0)))
    m.add_all(parts)
    letters(m, 'COAL OIL', (0.0, -D * 0.5, 0.122), 0.03, (0.0, -1.0, 0.0), UP, look='black', tol=0.05, lift=0.0004,
            width=0.104)
    m.socket('Grip', (0.0, hy, H + 0.072), (0.0, 0.0, 0.0))
    m.socket('Interact', (0.0, -0.08, 0.15), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.01, D + 0.01, H + 0.075), at=(0.0, 0.0, (H + 0.075) * 0.5))
    return m.done('the middle of its foot on the shelf', ao=0.08)


# --- A Ranger cache tag ---

def cache_tag():
    m = Model('CacheTag', seed=SEED + 260)
    T = 0.0015
    outline = [(-0.0375, -0.012), (-0.026, -0.0225), (0.0375, -0.0225), (0.0375, 0.0225), (-0.026, 0.0225),
               (-0.0375, 0.012)]
    parts = [flat([outline, circle(-0.028, 0.0, 0.004, 10, reverse=True)], 0.0, T, 'MailboxTin', density=FINE,
                  seed=SEED + 261),
             lathe([(0.004, T), (0.0068, T), (0.0068, T + 0.0008), (0.004, T + 0.0008)], 12, 'BrassWorn', density=FINE,
                   seed=SEED + 262, at=(-0.028, 0.0, 0.0), closed=True)]
    cx = 0.008
    parts.append(flat([circle(cx, 0.0, 0.0148, 24), circle(cx, 0.0, 0.0124, 24, reverse=True)], T, T + 0.0003,
                      'PaintCream', density=FINE, seed=SEED + 263))
    parts.append(flat([star(cx, 0.0, 0.0115, 0.0046, 5)], T, T + 0.0003, 'PaintCream', density=FINE, seed=SEED + 264))
    g = 0.0018
    twine = [(-0.028, 0.0, T + 0.0003), (-0.033, -0.001, T + g), (-0.0395, -0.002, T + g * 0.5), (-0.046, -0.004, g),
             (-0.062, -0.016, g), (-0.084, -0.014, g), (-0.097, 0.006, g), (-0.088, 0.028, g), (-0.064, 0.03, g),
             (-0.046, 0.012, g), (-0.0395, 0.004, T + g * 0.5), (-0.033, 0.002, T + g)]
    loop = spline(twine, per=3, closed=True, floor=g)
    parts.append(tube(loop, g, 'RopeHemp', sides=4, caps=(False, False), seed=SEED + 265, density=640.0))
    move, lo, hi = settle(parts)
    m.add_all(parts)
    m.socket('Interact', tuple(move @ Vector((0.0, 0.0, 0.012))), aim((0.0, -1.0, 0.0)))
    box_hull(m, lo, hi, least=0.05)
    return m.done('the middle of its footprint on the surface (lying flat, the twine loop toward -X)', ao=0.006,
                  min_height=0.0002, crumb=0.0006, lods=False)


# --- The pay chest ---

def payroll_chest():
    m = Model('PayrollChest', seed=SEED + 270)
    W, D, HB, HL = 0.56, 0.34, 0.22, 0.075
    m.emit(kit._box((W, D, HB), 0.006, 3), homes.Boards(rotate=False), 'WoodBlack', kit.place((0.0, 0.0, HB * 0.5)))
    m.emit(kit._box((W + 0.01, D + 0.01, HL), 0.008, 3), homes.Boards(rotate=False), 'WoodBlack',
           kit.place((0.0, 0.0, HB + HL * 0.5)))
    top = HB + HL
    fy, ly = -D * 0.5, -(D + 0.01) * 0.5
    for x in (-0.17, 0.17):                                     # two iron straps over it, front to back
        town.box(m, (0.04, 0.004, HB), at=(x, fy - 0.002, HB * 0.5), look='iron', drop=('+y',))
        town.box(m, (0.04, 0.004, HL), at=(x, ly - 0.002, HB + HL * 0.5), look='iron', drop=('+y',))
        town.box(m, (0.04, D + 0.018, 0.004), at=(x, 0.0, top + 0.002), look='iron', drop=('-z',))
        town.box(m, (0.04, 0.004, HL), at=(x, -ly + 0.002, HB + HL * 0.5), look='iron', drop=('-y',))
        town.box(m, (0.04, 0.004, HB), at=(x, -fy + 0.002, HB * 0.5), look='iron', drop=('-y',))
        for z in (0.04, 0.11, 0.18, HB + 0.038):
            y = (ly if z > HB else fy) - 0.0045
            town.box(m, (0.009, 0.004, 0.009), at=(x, y, z), look='iron', drop=('+y',))
    for sx in (-1.0, 1.0):                                      # corner irons on the body's foot and the lid's top
        for sy in (-1.0, 1.0):
            for z0, h, wd, dp in ((0.0, 0.06, W, D), (HB + HL - 0.055, 0.055, W + 0.01, D + 0.01)):
                town.box(m, (0.05, 0.003, h), at=(sx * (wd * 0.5 - 0.025), sy * (dp * 0.5 + 0.0015), z0 + h * 0.5),
                         look='iron', drop=('+y',) if sy < 0 else ('-y',))
                town.box(m, (0.003, 0.05, h), at=(sx * (wd * 0.5 + 0.0015), sy * (dp * 0.5 - 0.025), z0 + h * 0.5),
                         look='iron', drop=('+x',) if sx < 0 else ('-x',))
    # The hasp down from the lid over its staple, and the brass padlock hung in it.
    town.box(m, (0.032, 0.004, 0.085), at=(0.0, ly - 0.004, HB + 0.0275), look='iron', drop=('+y',))
    parts = [tube([(0.0, fy - 0.004, 0.2), (0.0, fy - 0.016, 0.2), (0.0, fy - 0.016, 0.186), (0.0, fy - 0.004, 0.186)],
                  0.003, 'IronBlack', sides=4, density=FINE, seed=SEED + 271)]
    lock_y = fy - 0.02
    parts.append(block((0.044, 0.017, 0.046), (0.0, lock_y, 0.151), 'BrassWorn', bevel=0.005, density=FINE,
                       seed=SEED + 272))
    shackle = [(-0.012, lock_y, 0.172)] + [(0.012 * math.cos(math.pi * (1.0 - k / 8)), lock_y,
                                            0.183 + 0.012 * math.sin(math.pi * k / 8)) for k in range(9)] + \
              [(0.012, lock_y, 0.172)]
    parts.append(tube(shackle, 0.0034, 'IronBlack', sides=5, density=FINE, seed=SEED + 273))
    parts.append(block((0.004, 0.002, 0.012), (0.0, lock_y - 0.0088, 0.145), 'PaintBlack', density=FINE,
                       seed=SEED + 274))
    for sx in (-1.0, 1.0):                                      # iron drop handles on its ends
        x = sx * (W * 0.5 + 0.004)
        for y in (-0.06, 0.06):
            parts.append(block((0.008, 0.016, 0.022), (x, y, 0.172), 'IronBlack', density=FINE, seed=SEED + 275))
        u = [(x + sx * 0.012, -0.06, 0.17), (x + sx * 0.016, -0.06, 0.135), (x + sx * 0.018, -0.03, 0.118),
             (x + sx * 0.018, 0.03, 0.118), (x + sx * 0.016, 0.06, 0.135), (x + sx * 0.012, 0.06, 0.17)]
        parts.append(tube(u, 0.0055, 'IronBlack', sides=6, density=FINE, seed=SEED + 276))
    m.add_all(parts)
    letters(m, 'P.M.CO.', (0.0, fy, 0.098), 0.04, (0.0, -1.0, 0.0), UP, look='cream', tol=0.05, lift=0.0006,
            width=0.27)
    letters(m, 'PAYROLL', (0.0, fy, 0.042), 0.022, (0.0, -1.0, 0.0), UP, look='cream', tol=0.05, lift=0.0006, width=0.27)
    m.socket('Interact', (0.0, -0.26, 0.26), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.04, D + 0.02, top + 0.004), at=(0.0, 0.0, (top + 0.004) * 0.5))
    return m.done('the middle of its foot on the floor', ao=0.15)


# --- Josiah Fenn's coffin ---

COFFIN = [(-0.975, -0.17), (0.429, -0.30), (0.975, -0.22), (0.975, 0.22), (0.429, 0.30), (-0.975, 0.17)]
COFFIN_H, FLOOR, WALL, LID_T, OVER, SPLIT = 0.36, 0.03, 0.025, 0.03, 0.015, 0.2


def offset(poly, w):
    """A convex counterclockwise polygon moved outward by w (inward when w < 0)."""
    n = len(poly)
    lines = []
    for i in range(n):
        a, b = Vector(poly[i]), Vector(poly[(i + 1) % n])
        d = (b - a).normalized()
        out = Vector((d.y, -d.x))
        lines.append((a + out * w, d))
    pts = []
    for i in range(n):
        (p1, d1), (p2, d2) = lines[i - 1], lines[i]
        den = d1.x * d2.y - d1.y * d2.x
        t = ((p2.x - p1.x) * d2.y - (p2.y - p1.y) * d2.x) / den
        q = p1 + d1 * t
        pts.append((q.x, q.y))
    return pts


def clip(poly, x, keep_above):
    """A convex polygon cut by the line X = x, keeping the side above or below it."""
    out = []
    n = len(poly)
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        ina = a[0] >= x if keep_above else a[0] <= x
        inb = b[0] >= x if keep_above else b[0] <= x
        if ina:
            out.append(a)
        if ina != inb:
            t = (x - a[0]) / (b[0] - a[0])
            out.append((x, a[1] + (b[1] - a[1]) * t))
    return out


def coffin_shell():
    """The box: outer walls, rim, inner walls, the floor inside and the bottom, a closed shell (bmesh)."""
    inner = offset(COFFIN, -WALL)
    tb = kit._new_bmesh()
    n = len(COFFIN)
    ob = [tb.verts.new((x, y, 0.0)) for x, y in COFFIN]
    ot = [tb.verts.new((x, y, COFFIN_H)) for x, y in COFFIN]
    it = [tb.verts.new((x, y, COFFIN_H)) for x, y in inner]
    ib = [tb.verts.new((x, y, FLOOR)) for x, y in inner]
    for i in range(n):
        j = (i + 1) % n
        tb.faces.new((ob[i], ob[j], ot[j], ot[i]))
        tb.faces.new((ot[i], ot[j], it[j], it[i]))
        tb.faces.new((it[i], it[j], ib[j], ib[i]))
    tb.faces.new(ib)
    tb.faces.new(list(reversed(ob)))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    kit._slice(tb, 0, [-0.7, -0.45, -0.2, 0.05, SPLIT, 0.45, 0.7])
    kit._slice(tb, 2, [0.12, 0.24])
    return tb


def rope_handle(edge0, edge1, s, seed):
    """A rope handle in two wooden cleats on the outer wall from edge0 to edge1 (s along it from edge0)."""
    a = (Vector(edge1) - Vector(edge0)).normalized()
    out = Vector((a.y, -a.x, 0.0))
    q = Vector(edge0) + a * s + Vector((0.0, 0.0, 0.25))
    parts = []
    yaw = math.degrees(math.atan2(a.y, a.x))
    for k, ds in enumerate((-0.085, 0.085)):
        parts.append(block((0.045, 0.022, 0.045), tuple(q + a * ds + out * 0.011), 'WoodCream', rot=(0.0, 0.0, yaw),
                           bevel=0.003, seed=seed + k))
    rope = [q + a * ds + out * (0.022 + b) + Vector((0.0, 0.0, dz)) for ds, dz, b in
            ((-0.085, 0.0, 0.0), (-0.06, -0.035, 0.008), (-0.02, -0.055, 0.012), (0.02, -0.055, 0.012),
             (0.06, -0.035, 0.008), (0.085, 0.0, 0.0))]
    parts.append(tube(rope, 0.0065, 'RopeHemp', sides=5, seed=seed + 3))
    return parts, q + out * 0.034 + Vector((0.0, 0.0, -0.055)), out


def fenn_coffin():
    m = Model('FennCoffin', seed=SEED + 300)
    rnd = random.Random(SEED + 300)
    m.emit(coffin_shell(), kit.Tile('WoodPlanks'), 'WoodCream')
    lid = offset(COFFIN, OVER)
    foot = clip(lid, SPLIT, False)
    parts = [flat([foot], COFFIN_H, COFFIN_H + LID_T, 'WoodCream', seed=SEED + 301)]
    lp.slice_at(parts[0], (1.0, 0.0, 0.0), (-0.7, -0.45, -0.2, 0.0))
    # Nails along the nailed-down foot lid's edges.
    inset = offset(foot, -0.014)
    for a, b in zip(inset, inset[1:] + inset[:1]):
        if abs(a[0] - SPLIT) < 1e-3 and abs(b[0] - SPLIT) < 1e-3:
            continue
        length = (Vector(b) - Vector(a)).length
        count = max(1, int(length / 0.2))
        for k in range(count):
            p = Vector(a).lerp(Vector(b), (k + 0.5) / count)
            parts.append(nail_head((p.x, p.y, COFFIN_H + LID_T)))
    # Rope handles: two on each long side, one on each end; a shipping tag tied to the head end's.
    for e0, e1, ss in (((COFFIN[0]), COFFIN[1], (0.3, 1.1)), (COFFIN[4], COFFIN[5], (0.3, 1.1)),
                       (COFFIN[5], COFFIN[0], (0.17,)), (COFFIN[2], COFFIN[3], (0.22,))):
        for s in ss:
            handle, low, out = rope_handle((e0[0], e0[1], 0.0), (e1[0], e1[1], 0.0), s, SEED + 310 + int(s * 100))
            parts += handle
    head_low = Vector((0.975, 0.0, 0.0)) + Vector((0.034, 0.0, 0.195))
    hole = head_low + Vector((0.004, 0.0, -0.03))
    parts.append(tube([head_low, head_low + Vector((0.006, 0.004, -0.014)), hole], 0.0014, 'RopeHemp', sides=4,
                      seed=SEED + 330))
    tag_rows = [[hole + Vector((0.0015, y, z)) for y in (-0.028, 0.0, 0.028)] for z in (0.0, -0.045, -0.09)]
    parts.append(slab(tag_rows, 0.0005, 'SextonLedgerPages', down=(-1.0, 0.0, 0.0), seed=SEED + 331))
    for k, z in enumerate((-0.025, -0.04, -0.055)):
        parts.append(ribbon([hole + Vector((0.0025, y, z + 0.002 * math.sin(y * 200.0))) for y in (-0.02, 0.0, 0.016)],
                            0.0018, (1.0, 0.0, 0.0), seed=SEED + 332 + k))
    # The hinges' knuckles and the body's leaves, on the head lid's back edge (from the shoulder to the head, +Y side).
    S, Hd = Vector((lid[4][0], lid[4][1])), Vector((lid[3][0], lid[3][1]))
    hinge_dir = (Hd - S).normalized()
    hinge_out = Vector((-hinge_dir.y, hinge_dir.x))              # away from the coffin (its +Y side)
    axis0 = S.to_3d() + hinge_out.to_3d() * 0.004 + Vector((0.0, 0.0, COFFIN_H))
    axis1 = Hd.to_3d() + hinge_out.to_3d() * 0.004 + Vector((0.0, 0.0, COFFIN_H))
    mid = (axis0 + axis1) * 0.5
    yaw = math.degrees(math.atan2(hinge_dir.y, hinge_dir.x))
    for t in (0.22, 0.78):
        c = axis0.lerp(axis1, t)
        parts.append(tube([c - hinge_dir.to_3d() * 0.025, c + hinge_dir.to_3d() * 0.025], 0.0055, 'IronBlack', sides=8,
                          density=FINE, seed=SEED + 340))
        tab = c - hinge_out.to_3d() * 0.0095 + Vector((0.0, 0.0, -0.0035))
        parts.append(block((0.05, 0.02, 0.003), tuple(tab), 'IronBlack', rot=(0.0, 0.0, yaw), density=FINE,
                           seed=SEED + 341))
        wall_y = c - hinge_out.to_3d() * (0.004 + OVER - 0.0015) + Vector((0.0, 0.0, -0.028))
        parts.append(block((0.05, 0.003, 0.05), tuple(wall_y), 'IronBlack', rot=(0.0, 0.0, yaw), density=FINE,
                           seed=SEED + 342))
    # Inside under the head lid: the lining's pleated top edge, the pillow, Fenn under his shroud and face cloth.
    inner = offset(COFFIN, -WALL)
    ring = []
    n = len(inner)
    for i in range(n):
        a, b = Vector(inner[i]), Vector(inner[(i + 1) % n])
        steps = max(1, int((b - a).length / 0.025))
        for k in range(steps):
            p = a.lerp(b, k / steps)
            if p.x >= 0.15:
                d = (b - a).normalized()
                ring.append((p, Vector((-d.y, d.x))))
    # (In the inner outline's order: from x = 0.15 on the -Y side round the head end to x = 0.15 on the +Y side.)
    rows = []
    for z, extra in ((0.3, 0.0), (0.355, 0.006)):
        row = []
        for k, (p, nin) in enumerate(ring):
            pleat = 0.002 + 0.0045 * (0.5 + 0.5 * math.sin(k * math.pi * 0.5))
            q = p + nin * (pleat + extra)
            row.append(Vector((q.x, q.y, z + (0.004 * math.sin(k * 1.7) if extra else 0.0))))
        rows.append(row)
    parts.append(sheet(rows, 'ClothLinen', lambda c: Vector((0.55 - c.x, -c.y, 0.0))))   # facing in
    parts.append(blob((0.86, 0.0, FLOOR + 0.045), (0.085, 0.16, 0.05), SEED + 350, 'ClothLinen', segs=10, rings=6,
                      lumpy=0.06))
    HEAD_C = Vector((0.8, 0.0, FLOOR + 0.1))
    HEAD_R = (0.112, 0.082, 0.06)

    def face(co):
        x, y, z = co
        if z > 0.0:
            z += 0.018 * math.exp(-((x + 0.028) ** 2 + y ** 2) / 0.00016)          # his nose under the cloth
            z += 0.006 * math.exp(-((x - 0.012) ** 2) / 0.0002) * math.exp(-(y ** 2) / 0.003)
            z -= 0.004 * math.exp(-((x - 0.0) ** 2 + (abs(y) - 0.032) ** 2) / 0.0002)
            z -= 0.01 * math.exp(-((x + 0.07) ** 2) / 0.0006)                    # under the chin
        else:
            x, y, z = x * 1.5, y * 1.6, z * 0.6                                  # the cloth falling to the pillow
        return Vector((x, y, z))
    parts.append(blob(tuple(HEAD_C), HEAD_R, SEED + 351, 'ClothLinen', segs=12, rings=8, lumpy=0.03, shape=face))
    # The shroud from his neck to past the split, the body's shape under it, tucked down at the walls.
    xs = [0.05, 0.15, 0.25, 0.35, 0.45, 0.55, 0.62, 0.68, 0.71]
    rows = []
    for j in range(7):
        row = []
        for x in xs:
            half = 0.3 - 0.08 * max(0.0, (x - 0.429) / 0.546) if x > 0.429 else 0.17 + 0.13 * (x + 0.975) / 1.404
            half -= WALL + 0.012
            y = -half + 2.0 * half * j / 6
            body_w = 0.2 if x > 0.5 else 0.17
            height = {0.05: 0.13, 0.15: 0.15, 0.25: 0.16, 0.35: 0.17, 0.45: 0.165, 0.55: 0.15, 0.62: 0.13, 0.68: 0.09,
                      0.71: 0.07}[x]
            k = max(0.0, 1.0 - (y / body_w) ** 2)
            z = FLOOR + 0.012 + height * k ** 0.6
            row.append(Vector((x, y, z)))
        rows.append(row)
    parts.append(sheet(rows, 'ClothLinen', (0.0, 0.0, 1.0), seed=SEED + 352))
    m.add_all(parts)
    # Lettering: GRAVESEND on the lid, J. FENN on the front by the head, FARE PAID chalked by the foot.
    letters(m, 'GRAVESEND', (-0.37, 0.0, COFFIN_H + LID_T), 0.08, UP, (0.0, 1.0, 0.0), look='black', tol=0.05,
            lift=0.0005, width=0.95)
    f0, f1 = Vector(COFFIN[0]), Vector(COFFIN[1])
    a = (f1 - f0).normalized()
    out = Vector((a.y, -a.x, 0.0))
    letters(m, 'FARE PAID', tuple(f0.to_3d() + a.to_3d() * 0.7 + Vector((0.0, 0.0, 0.115))), 0.055, tuple(out), UP,
            look='cream', tol=0.06, lift=0.0006, rot=-3.0)
    h0, h1 = Vector(COFFIN[1]), Vector(COFFIN[2])
    a2 = (h1 - h0).normalized()
    out2 = Vector((a2.y, -a2.x, 0.0))
    letters(m, 'J. FENN', tuple(h0.to_3d() + a2.to_3d() * 0.28 + Vector((0.0, 0.0, 0.2))), 0.058, tuple(out2), UP,
            look='black', tol=0.05, lift=0.0006, width=0.4)
    m.socket('Lid', tuple(mid), (0.0, 0.0, yaw))
    eye_z = HEAD_C.z + HEAD_R[2] * math.sqrt(1.0 - (0.032 / HEAD_R[1]) ** 2) - 0.002
    m.socket('Eyes', (0.8, 0.0, eye_z), (0.0, 0.0, 90.0))
    m.socket('Speaker', (0.8, 0.0, 0.3), aim((0.0, -1.0, 0.0)))
    m.socket('Interact', (0.62, -0.34, 0.4), aim((0.0, -1.0, 0.0)))
    m.hull_points([(x, y, z) for x, y in lid for z in (0.0, COFFIN_H + LID_T)])
    obj = m.done("the middle of its foot on the floor (on the depot's skid), long along X, its head toward +X",
                 ao=0.3, lods=True)
    return obj, mid, yaw, S, Hd


def fenn_coffin_lid(mid, yaw, S, Hd):
    m = Model('FennCoffin_Lid', seed=SEED + 360)
    lid = offset(COFFIN, OVER)
    head = clip(lid, SPLIT + 0.003, True)
    panel = flat([head], COFFIN_H, COFFIN_H + LID_T, 'WoodCream', seed=SEED + 361)
    lp.slice_at(panel, (1.0, 0.0, 0.0), (0.45, 0.7))
    parts = [panel]
    hinge_dir = (Hd - S).normalized()
    hinge_out = Vector((-hinge_dir.y, hinge_dir.x))
    axis0 = S.to_3d() + hinge_out.to_3d() * 0.004 + Vector((0.0, 0.0, COFFIN_H))
    axis1 = Hd.to_3d() + hinge_out.to_3d() * 0.004 + Vector((0.0, 0.0, COFFIN_H))
    for t in (0.22, 0.78):
        c = axis0.lerp(axis1, t)
        leaf = c - hinge_out.to_3d() * 0.0025 + Vector((0.0, 0.0, 0.0155))
        parts.append(block((0.05, 0.003, 0.026), tuple(leaf), 'IronBlack', rot=(0.0, 0.0, yaw), density=FINE,
                           seed=SEED + 362))
    # An iron ring pull on its top by the front edge.
    pull = Vector((0.66, -0.2, COFFIN_H + LID_T))
    parts.append(lathe([(0.0105, 0.0025), (0.0135, 0.0), (0.0165, 0.0025), (0.0135, 0.005)], 12, 'IronBlack',
                       density=FINE, seed=SEED + 363, at=tuple(pull), closed=True))
    parts.append(block((0.012, 0.006, 0.004), tuple(pull + Vector((0.0, 0.0135, 0.002))), 'IronBlack', density=FINE,
                       seed=SEED + 364))
    frame = Matrix.Translation(mid) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
    transform(parts, frame.inverted())
    m.add_all(parts)
    m.socket('Interact', tuple(frame.inverted() @ Vector((0.62, -0.1, COFFIN_H + LID_T + 0.05))), (0.0, 0.0, 0.0))
    return m.done("on its hinge's axis (attach at FennCoffin's SOCKET_Lid; pitch up to open)", ao=0.06, ground=False,
                  Collision='None')


# --- The keepers' roll ---

def keepers_roll():
    m = Model('KeepersRoll', seed=SEED + 400)
    W, H, PT = 0.8, 1.2, 0.022
    face_y = -PT
    for k in range(4):                                          # four upright planks painted black
        x = -W * 0.5 + 0.1 + 0.2 * k
        town.board(m, (x, -PT * 0.5, 0.0), (x, -PT * 0.5, H), 0.196, PT, look='woodblack', face=(0.0, -1.0, 0.0))
    crest = [(-W * 0.5 - 0.02, H)] + [(math.cos(math.pi * (1.0 - k / 10)) * (W * 0.5 + 0.02),
                                       H + 0.14 * math.sin(math.pi * k / 10)) for k in range(1, 10)] + \
            [(W * 0.5 + 0.02, H)]
    town.prism(m, crest, PT, look='woodblack', y=-PT)
    # The frame: a moulding round the panel, a gilt bead inside it.
    FW, FT = 0.055, 0.026
    for p0, p1 in (((-W * 0.5 + FW * 0.5, 0.0), (-W * 0.5 + FW * 0.5, H)), ((W * 0.5 - FW * 0.5, 0.0), (W * 0.5 - FW * 0.5, H)),
                   ((-W * 0.5, FW * 0.5), (W * 0.5, FW * 0.5)), ((-W * 0.5, H - FW * 0.5), (W * 0.5, H - FW * 0.5))):
        town.board(m, (p0[0], face_y - FT * 0.5, p0[1]), (p1[0], face_y - FT * 0.5, p1[1]), FW, FT, look='woodblack',
                   face=(0.0, -1.0, 0.0), drop=('+y',))
    parts = []
    bead = [(-W * 0.5 + FW + 0.004, FW + 0.004), (W * 0.5 - FW - 0.004, FW + 0.004), (W * 0.5 - FW - 0.004, H - FW - 0.004),
            (-W * 0.5 + FW + 0.004, H - FW - 0.004)]
    for a, b in zip(bead, bead[1:] + bead[:1]):
        parts.append(ribbon([(a[0], face_y - 0.0006, a[1]), (b[0], face_y - 0.0006, b[1])], 0.007, (0.0, -1.0, 0.0),
                            'BrassWorn', density=FINE, seed=SEED + 401))
    # The keepers' sun in the crest: a ring and twelve rays, in gilt.
    sc = (0.0, H + 0.062)
    parts.append(upright([circle(sc[0], sc[1], 0.034, 24), circle(sc[0], sc[1], 0.024, 24, reverse=True)],
                         -PT - 0.002, -PT, 'BrassWorn', density=FINE, seed=SEED + 402))
    for k in range(12):
        a = 2.0 * math.pi * k / 12
        d, s = Vector((math.cos(a), math.sin(a))), Vector((-math.sin(a), math.cos(a)))
        long = 0.062 if k % 2 == 0 else 0.05
        tri = [tuple(Vector(sc) + d * 0.038 - s * 0.006), tuple(Vector(sc) + d * long), tuple(Vector(sc) + d * 0.038 + s * 0.006)]
        parts.append(upright([list(reversed(tri))], -PT - 0.0015, -PT, 'BrassWorn', density=FINE, seed=SEED + 403 + k))
    parts.append(ribbon([(-0.25, face_y - 0.0007, 1.0), (0.25, face_y - 0.0007, 1.0)], 0.005, (0.0, -1.0, 0.0),
                        'BrassWorn', density=FINE, seed=SEED + 416))
    front = (0.0, -1.0, 0.0)
    letters(m, 'KEEPERS', (0.0, face_y, 1.1), 0.072, front, UP, look='brass', tol=0.05, lift=0.0008, width=0.62)
    letters(m, 'OF SAINT ADA', (0.0, face_y, 1.035), 0.036, front, UP, look='brass', tol=0.06, lift=0.0008, width=0.5)
    names = list(ROLL_NAMES) + ['ABEL', 'ELLIS']
    z = 0.9
    widths = []
    for name in names:
        w, _ = letters(m, name, (0.0, face_y, z), 0.058, front, UP, look='brass', tol=0.055, lift=0.0008)
        widths.append((name, z, w))
        z -= 0.118
    # ELLIS painted out: strokes of fresher black over the gilt, the letters showing through between them.
    _, ez, ew = widths[-1]
    rnd = random.Random(SEED + 420)
    for k in range(7):
        zz = ez - 0.036 + 0.012 * k + rnd.uniform(-0.0012, 0.0012)
        x0, x1 = -ew * 0.5 - 0.035 + rnd.uniform(-0.01, 0.01), ew * 0.5 + 0.035 + rnd.uniform(-0.01, 0.01)
        pts = [(x0 + (x1 - x0) * s, face_y - 0.0016 - 0.0002 * k, zz + 0.0018 * math.sin(s * 7.0 + k)) for s in
               (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)]
        stroke = ribbon(pts, 0.0072 - 0.0016 * (k % 2), (0.0, -1.0, 0.0), 'PaintBlack', density=None, seed=SEED + 421 + k)
        # Ragged, thinning ends.
        for v in stroke.data.vertices:
            s = (v.co.x - x0) / (x1 - x0)
            taper = min(1.0, min(s, 1.0 - s) / 0.08)
            v.co.z = zz + (v.co.z - zz) * (0.35 + 0.65 * taper)
        stroke.data.update()
        parts.append(stroke)
    m.add_all(parts)
    m.socket('Interact', (0.0, -0.06, 0.62), aim((0.0, -1.0, 0.0)))
    m.hull((W + 0.04, 0.06, H + 0.14), at=(0.0, -0.03, (H + 0.14) * 0.5))
    return m.done("the middle of its back's bottom edge (on the wall), front -Y", ao=0.12, lods=True)


# --- Building ---

BUILDERS = [('LanternBail', lantern_bail), ('WinchCrank', winch_crank), ('CoalSack', coal_sack),
            ('ClampPin', clamp_pin), ('CellarKey', cellar_key), ('TapeMeasure', tape_measure),
            ('GraveCoins', grave_coins), ('MinersTag', miners_tag), ('GravemotherFang', gravemother_fang),
            ('SheriffBadge', sheriff_badge), ('SheriffHat', sheriff_hat), ('Journal', journal),
            ('CodeBook', code_book), ('PostLog', post_log), ('PayrollBook', payroll_book),
            ('PayrollChest', payroll_chest), ('LampOil', lamp_oil), ('CacheTag', cache_tag),
            ('DeaconsSlip', deacons_slip), ('KeepersRoll', keepers_roll), ('LedgerPage', ledger_page),
            ('DeaconLetter', deacon_letter), ('PostRack', post_rack), ('ReachesMap', reaches_map)]


def build():
    for obj in list(bpy.data.objects):                         # an empty scene's leftovers, if any
        bpy.data.objects.remove(obj)
    built = {}
    for name, builder in BUILDERS:
        if ONLY is None or name in ONLY:
            built[name] = builder()
    if ONLY is None or 'FennCoffin' in ONLY or 'FennCoffin_Lid' in ONLY:
        coffin, mid, yaw, S, Hd = fenn_coffin()
        built['FennCoffin'] = coffin
        built['FennCoffin_Lid'] = fenn_coffin_lid(mid, yaw, S, Hd)
    return built


# --- Previews and the sheet (--preview) ---

CAPTIONS = {
    'LanternBail': "Pa's lantern's bail, snapped off; a torn ear on its hook",
    'WinchCrank': "The lift's crank, the sheriff's impound tag on it",
    'CoalSack': 'Railroad coal in the depot\'s canvas sack',
    'ClampPin': "The sheave's clamp pin, on Abe's neck cord",
    'CellarKey': 'Cellar key on its cord and CELLAR fob (hangs)',
    'TapeMeasure': "Thaddeus's tape, run out to its end ring",
    'GraveCoins': "Fenn's fare: two old gold coins",
    'MinersTag': "No. 7, J. Odell's tag (the board's disc)",
    'GravemotherFang': 'Her fang on its torn chelicera, 40 cm',
    'SheriffBadge': "Odell's star: a point bent, a ball bitten off",
    'SheriffHat': "Odell's hat, chewed: two bites, a dent",
    'Journal': "Ruth's journal, its thong wound twice",
    'CodeBook': "Ruth's signal book, a signal card in it",
    'PostLog': "Larkspur post's log, the right page half empty",
    'PayrollBook': "The Foreman's payroll ledger, slips in it",
    'PayrollChest': "The Providence's pay chest, locked",
    'LampOil': 'Coal oil can for the watch fire',
    'CacheTag': "A Rim Rangers' cache tag (x3)",
    'DeaconsSlip': "The Deacon's slip, folded, his hand inside",
    'FennCoffin': "Fenn's coffin, closed: GRAVESEND, FARE PAID",
    'FennCoffin_Open': 'Head lid open, the coins on his eyes',
    'KeepersRoll': 'The roll: ELLIS painted out under ABEL',
    'LedgerPage': "A loose page of Sexton's ledger, a corner lifted",
    'DeaconLetter': "The Deacon's letter, folded and sealed",
    'PostRack': "The saloon's post rack, two old letters in it",
    'ReachesMap': 'The depot wall map of the Reaches, pinned',
}
HANGING = ('CellarKey', 'MinersTag')


def _plane(name, corners, mat_name, set_name, density=None):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(corners, [], [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.assign(obj, mat(mat_name))
    lt.box_uv(obj, set_name, texel_density=density)
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    return obj


def show(keep):
    keep = list(keep)
    for o in bpy.context.scene.objects:
        if o.type in ('MESH', 'CURVE', 'FONT'):
            o.hide_render = not (o.name == '_Ground' or o in keep or (o.parent in keep and not o.name.startswith('UCX_')))
        if o.name.startswith('UCX_'):
            o.hide_render = True


def shoot(name, objs, extra=(), view=(-0.55, -1.0, 0.62), sun=(40.0, 38.0), lens=55.0, size=(1500, 880), margin=1.15,
          bias=0.45, exposure=-0.1):
    scene = bpy.context.scene
    show(list(objs) + list(extra))
    homes.light(scene, SUN_OBJ[0], homes.golden_sun(*sun))
    eye, target = frame(list(objs), view, size, lens, margin, bias)
    return homes.shoot(scene, os.path.join(PREVIEW_DIR, name + '.png'), eye, target, lens, size, exposure)


def frame(objs, direction, size, lens, margin, bias):
    """homes.frame_shot without its 1 m least distance: a coin fills the frame as a coffin does."""
    pts = homes.points_of(objs)
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    target = Vector(((lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, lo.z + (hi.z - lo.z) * bias))
    d = Vector(direction).normalized()
    tan_x = 18.0 / lens
    tan_y = tan_x * size[1] / size[0]
    forward = -d
    right = forward.cross(UP).normalized()
    up = right.cross(forward).normalized()
    dist = 0.05
    for p in pts:
        rel = p - target
        x, y, z = rel.dot(right), rel.dot(up), rel.dot(forward)
        dist = max(dist, abs(x) * margin / tan_x - z, abs(y) * margin / tan_y - z)
    return target + d * dist, target


SUN_OBJ = []


def socket_matrix(obj, name):
    sock = next(c for c in obj.children if c.name.split('.')[0] == 'SOCKET_' + name)
    return obj.matrix_world @ sock.matrix_local


def previews(built):
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    scene, sun = homes.stage(samples=48)
    SUN_OBJ.append(sun)
    tiles = []
    for name, obj in built.items():
        if name == 'FennCoffin_Lid':
            continue
        if name in HANGING:
            obj.location = (0.0, 0.0, 1.3)
            bpy.context.view_layer.update()
            back = _plane('_Backdrop_' + name, [(-0.25, 0.012, 1.05), (0.25, 0.012, 1.05), (0.25, 0.012, 1.45),
                                               (-0.25, 0.012, 1.45)], 'WoodCream', 'WoodPlanks')
            peg = lathe([(0.0, 0.0), (0.004, 0.0), (0.004, 0.024), (0.0, 0.024)], 8, 'IronBlack', at=(0.0, 0.012, 1.3015),
                        rot=(90.0, 0.0, 0.0))
            peg.name = '_Peg_' + name
            path = shoot(name, [obj], [back, peg], view=(-0.35, -1.0, 0.18), lens=70.0)
        elif name in ('PostRack', 'ReachesMap'):
            obj.location = (0.0, 0.0, 1.5)
            bpy.context.view_layer.update()
            wall = _plane('_Wall_' + name, [(-1.0, 0.001, 0.8), (1.0, 0.001, 0.8), (1.0, 0.001, 2.2), (-1.0, 0.001, 2.2)],
                          'WoodCream', 'WoodPlanks')
            path = shoot(name, [obj], [wall], view=(-0.3, -1.0, 0.12), lens=45.0, sun=(25.0, 32.0), margin=1.1)
        elif name == 'KeepersRoll':
            obj.location = (0.0, 0.0, 0.95)
            bpy.context.view_layer.update()
            wall = _plane('_Wall', [(-1.4, 0.001, 0.0), (1.4, 0.001, 0.0), (1.4, 0.001, 2.8), (-1.4, 0.001, 2.8)],
                          'WoodCream', 'WoodPlanks')
            path = shoot(name, [obj], [wall], view=(-0.3, -1.0, 0.12), lens=45.0, sun=(25.0, 32.0), margin=1.08)
        elif name == 'FennCoffin':
            lid = built.get('FennCoffin_Lid')
            if lid is not None:
                lid.matrix_world = socket_matrix(obj, 'Lid')
                bpy.context.view_layer.update()
            path = shoot(name, [obj] + ([lid] if lid else []), view=(-0.45, -1.0, 0.55), lens=40.0, margin=1.08)
            tiles.append((path, f"{name} {REPORT[name]['triangles']:,}", caption(name)))
            if lid is not None:
                lid.matrix_world = socket_matrix(obj, 'Lid') @ Matrix.Rotation(math.radians(-100.0), 4, 'X')
                coins = built.get('GraveCoins')
                extra = []
                if coins is not None:
                    copy = coins.copy()
                    copy.name = '_CoinsOnEyes'
                    for child in list(copy.children):
                        child.parent = None
                    bpy.context.scene.collection.objects.link(copy)
                    copy.matrix_world = socket_matrix(obj, 'Eyes')
                    extra.append(copy)
                bpy.context.view_layer.update()
                path = shoot('FennCoffin_Open', [obj, lid], extra, view=(0.15, -0.75, 1.0), lens=50.0, sun=(30.0, 55.0),
                             margin=1.0, bias=0.3)
                tiles.append((path, f"FennCoffin_Lid {REPORT['FennCoffin_Lid']['triangles']:,}",
                              caption('FennCoffin_Open')))
                lid.matrix_world = socket_matrix(obj, 'Lid')
            continue
        else:
            path = shoot(name, [obj])
        tiles.append((path, f"{name} {REPORT[name]['triangles']:,}", caption(name)))
    return tiles


def caption(name):
    if name == 'FennCoffin_Open':
        info = REPORT['FennCoffin_Lid']
    else:
        info = REPORT[name]
    w, d, h = info['size_cm']
    socks = ', '.join(s['name'] for s in info['sockets'])
    return f"{CAPTIONS[name]}\n{w:g} x {d:g} x {h:g} cm, {len(info['materials'])} mats; {socks}"


def lineup(built):
    """Every model at one scale on the ground: the coffin, the roll and the chest at the back, the small things in front."""
    order = ['FennCoffin', 'KeepersRoll', 'PayrollChest', 'CoalSack', 'SheriffHat', 'LampOil', 'PostLog', 'PayrollBook',
             'WinchCrank', 'GravemotherFang', 'ClampPin', 'TapeMeasure', 'Journal', 'CodeBook', 'LanternBail',
             'DeaconsSlip', 'SheriffBadge', 'GraveCoins', 'CacheTag', 'CellarKey', 'MinersTag']
    rows = [order[:5], order[5:11], order[11:]]
    y = 0.0
    shown = []
    for r, row in enumerate(rows):
        x = 0.0
        depth = 0.0
        for name in row:
            obj = built.get(name)
            if obj is None:
                continue
            obj.rotation_euler = (math.radians(-90.0), 0.0, 0.0) if name in HANGING else (0.0, 0.0, 0.0)
            obj.location = (0.0, 0.0, 0.0)
            bpy.context.view_layer.update()
            pts = [obj.matrix_world @ v.co for v in obj.data.vertices]
            lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
            hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
            obj.location = (x - lo.x, y - hi.y, -lo.z)
            x += (hi.x - lo.x) + 0.08
            depth = max(depth, hi.y - lo.y)
            shown.append(obj)
            if name == 'FennCoffin' and built.get('FennCoffin_Lid') is not None:
                bpy.context.view_layer.update()
                built['FennCoffin_Lid'].matrix_world = socket_matrix(obj, 'Lid')
                shown.append(built['FennCoffin_Lid'])
        y -= depth + 0.1
    bpy.context.view_layer.update()
    return shoot('Lineup', shown, view=(-0.2, -1.0, 0.85), lens=35.0, size=(2400, 820), margin=1.04, bias=0.3,
                 sun=(35.0, 40.0))


if __name__ == '__main__':
    BUILT = build()
    total = sum(info['triangles'] for info in REPORT.values())
    lt._log(f'MISSIONPROPS: {len(REPORT)} models, {total:,} triangles')
    if PREVIEW:
        os.makedirs(PREVIEW_DIR, exist_ok=True)
        with open(os.path.join(PREVIEW_DIR, 'MissionProps.report.json'), 'w', encoding='utf-8') as f:
            json.dump(REPORT, f, indent=1)
        TILES = previews(BUILT)
        if '--no-sheet' not in ARGS and ONLY is None:
            BANNER = lineup(BUILT)
            homes.look_sheet(SHEET, "Ransom's Rest: the mission props",
                             f"Wave 4's small props, Blender previews in the area's late-afternoon light; nothing imported "
                             f"yet.\n{len(REPORT)} models, {total:,} triangles in all; shared materials only, no new "
                             f"textures.\nThe orange highlight goes on each one's own mesh: a clear silhouette apiece.",
                             (BANNER, 'All of them at one scale: the coffin, the roll and the chest at the back, the small '
                                      'things in front\n(the key and the tag laid down here; they hang in the game).'),
                             TILES, width=2400, columns=3)
