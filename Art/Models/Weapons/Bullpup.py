"""The bullpup assault rifle the user chose (2026-09-30) as the base of every non-legendary AR, with interchangeable
parts: 8 bodies, 8 barrels, 8 muzzle devices, 8 magazines, 8 sights and 8 stocks. Every assault rifle in the game is
built from them (DA_AssaultRifle). Legendary ARs will get their own unique models and names later. Since 2026-10-21 it wears
the frontier-tech look of the seven families (sand shells, gunmetal, brass, ember heat glow in vents and ports, cyan
optics); rarer parts bring more brass and glow.

How the parts fit (gun space, cm: u along the gun from the back of the butt, v up from the bore; see looter_guns):

  BullpupBody_<Key>      the shell: receiver, thumbhole grip loop, trigger, charging handle, a Picatinny rail on the
                         optic mount, a rarity accent line on each side (GunAccentGlow); the styles differ in shape,
                         materials and fittings (plates, armor, windows). Sockets: Stock (the butt face), Barrel (the
                         barrel nut on the nose), Magazine (the well behind the grip), Sight (the rail's middle), Grip,
                         Foregrip, in the same place on every body, so every part fits every body.
  BullpupBarrel_<Key>    origin at the barrel nut; SOCKET_Muzzle at its tip (barrels differ in length).
  BullpupMuzzle_<Key>    origin at its back face (screws onto a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its own
                         tip, where the flash comes from.
  BullpupMagazine_<Key>  origin where it seats.
  BullpupSight_<Key>     origin on top of the rail, at its middle; SOCKET_Aim on its line of sight (the dot, the
                         eyepiece, or the irons' front post), which aiming down sights puts before the eye.
  BullpupStock_<Key>     origin at the middle of the butt face; extends backward (and, for cheek risers, forward
                         over the shell).

A rolled AR picks one option per slot. Each option's display name, the word it can lend the gun's name, the lowest
rarity it appears on and its stat ranges live in Bullpup.parts.csv beside this file, which
Tools/Unreal/setup_gun_parts.py reads into DA_AssaultRifle. PARTS below was its first draft and now only lays the parts
out in the scene: a new part needs a model here and a row there, under the same key.

The element magazines (Ember30, Saint30, Salt30: the 30's shape)
wear their element (2026-10-22, looter_guns' element pieces): Ember heat-scored metal and ember vents, Soul-light
pale gold filigree, saint's medals or lanterns and a gold-white glow, Grave-salt salt crystals and cyan-white inlays.
`-- --elements` renders Saved/ArtPreviews/Weapons/Bullpup_Elements.png (a row per element: first person, side, the part
alone).

    blender -b --factory-startup --python Art/Models/Weapons/Bullpup.py -- --preview
Tripo parts (2026-10-27): parts the user remodeled in Tripo (image-to-3D) from the concept views
(Art/Concepts/Weapons/AR/<Slot>_<Key>) replace their scripted models after the script has built them, fitted by
looter_tripo from Art/Models/Weapons/Tripo/parts_set<N>.py (one file per part set; see "Tripo parts" below) with their
baked maps (texture sets TripoBullpup<Slot><Key> on the Gun master), glow slots and a paintable shell slot. Same mesh
names, keys and sockets. LOOTER_BULLPUP_TRIPO=0 builds the scripted ones.
"""
import math
import os

import bpy
from mathutils import Matrix, Vector

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.glow('GunReticle', 0xff3b2e, 6.0)
SHELL = lg.POLY_SAND
# The frontier-tech look all seven families share (2026-10-21 restyle): sand shells, gunmetal, brass fittings, ember heat
# glow seen through vents and ports, cyan optics. Commons keep a hint of it; rarer parts get more brass and glow.
METAL = lg.BLUED
EMBER, CYAN, BRASS = lg.EMBER, lg.CYAN, lg.BRASS

# Where the parts attach (gun space, cm).
STOCK_AT = (0.0, -3.3)
BARREL_AT = (51.9, 0.0)
MAG_AT = (14.0, -5.6)
SIGHT_AT = (23.0, 6.15)
MUZZLE_AT = (100.0, 0.0)   # muzzle devices are modeled here, around their own origin

g = lg.Gun('Bullpup')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Tripo parts: the user's image-to-3D models (2026-10-27) in place of scripted parts ---
# Each Art/Models/Weapons/Tripo/parts_set<N>.py (one per part set, so a set's fitting stays in its own file) holds
# PARTS = {'<Slot>_<Key>': spec}; every one found is merged (a key in two files is an error). A spec replaces that
# part's scripted model after the script has built it, keeping its name, key and sockets:
#   glb       the model file, beside the parts files (Bullpup<Slot>_<Key>.glb)
#   box       ((u0, u1), (v0, v1), w or (w0, w1)): where its bounds go in gun space, cm (the concept's size table)
#   axes      looter_tripo.fit's forward / left / up: which of the file's axes the muzzle, the gun's left and up face
#   glows     {glow material name: colour test}: faces painted that colour become that glow slot (a test is a
#             looter_tripo is_<name> or a function of (r, g, b)); Gun*Glow and GunReticle take the element's colour
#   island_glows  {glow material name: rule(island) -> bool}: whole pieces of the mesh as that glow slot
#   paint     dict(slot='GunPolymerSand', test='sand', tint=0xe6d8b6): the pieces mostly that colour become the
#             paintable slot <slot>_TripoBullpup<Slot><Key> (the gun's paint of <slot> recolours them); by='faces'
#             sorts face by face (a mesh of one piece), mode='luma' for dark or saturated shells (looter_tripo.paint_set)
#   sockets   {name: ((u, v), w)}: sockets replacing the scripted part's of that name (the others are kept)
#   prepare(ctx), finish(ctx): hooks before the faces are sorted (move the mesh, measure; may set ctx.sockets) and
#             after it is added (extra pieces: ctx.add(obj, ...)). ctx: key, obj, maps, mat, tripo, lg, lt, at (the
#             attach points), glow(name) -> material, log(text), sockets, add(*objs).
# A spec may instead be a function fn(bp, m): bp is this script's namespace (g, tripo_load, tripo_add, body_sockets,
# the attach points, the glow materials ...), m the model's name; it builds the whole part and adds every socket the
# part needs (the scripted part's are dropped).
# LOOTER_BULLPUP_TRIPO=0 builds every part from the script; =1,3 only sets 1 and 3 from Tripo.

TRIPO_DIR = 'Art/Models/Weapons/Tripo/'
TRIPO_LODS = '50,20'              # LOD1, LOD2 (percent of triangles; Unreal reduces), for guns lying about
TRIPO_LOD_SCREENS = (0.3, 0.12)   # a whole body's switch sizes; a smaller part's scale with its size, so all switch together
BODY_LENGTH = 0.522               # m


def tripo_specs():
    import glob
    import re
    import runpy
    wanted = os.environ.get('LOOTER_BULLPUP_TRIPO', '')
    if wanted == '0':
        return {}
    sets = {int(s) for s in wanted.split(',') if s.strip().isdigit()}
    specs = {}
    for path in sorted(glob.glob(os.path.join(lt.REPO, TRIPO_DIR, 'parts_set*.py'))):
        number = int(re.search(r'parts_set(\d+)', os.path.basename(path)).group(1))
        if sets and number not in sets:
            continue
        for key, spec in runpy.run_path(path)['PARTS'].items():
            if key in specs:
                raise ValueError(f'{key} is in {os.path.basename(path)} and {specs[key]["file"]}')
            specs[key] = (spec, os.path.basename(path))
    return specs


TRIPO = tripo_specs()


def tripo_glow(name):
    known = {m.name: m for m in (ACCENT, RETICLE, EMBER, CYAN, lg.AMMO_GLOW, lg.HEAT_GLOW, lg.SOUL, lg.SALT_GLOW)}
    mat = known.get(name) or bpy.data.materials.get(name)
    if mat is None:
        raise KeyError(f'no glow material {name} (looter_guns or Bullpup.py makes them)')
    return mat


def tripo_load(name, box, **axes):
    """Loads Art/Models/Weapons/Tripo/Bullpup<name>.glb, writes its maps as the texture set TripoBullpup<name without
    '_'> (material GunTripoBullpup...), fits it onto box (u, v, w as looter_tripo.fit); returns (tripo, obj, maps, mat)."""
    import looter_tripo as tripo
    path = TRIPO_DIR + f'Bullpup{name}.glb'
    set_name = 'TripoBullpup' + name.replace('_', '')
    obj, maps = tripo.load(path)
    mat = tripo.write_set(maps, set_name, 'Gun' + set_name, path)
    tripo.fit(obj, *box, **axes)
    return tripo, obj, maps, mat


def tripo_add(m, tripo, obj, mat, glows=None, maps=None, paint=None):
    """Adds obj to model m: glows {material: faces} as their own slots; with paint (or maps alone: the sand shell), the
    pieces mostly paint['test']'s colour as the paintable slot; the rest textured with Tripo's maps."""
    if maps is not None and paint is None:
        paint = dict(slot=SHELL.name, test='sand', tint=0xe6d8b6)
    taken = set()
    unique = {}
    for k, faces in (glows or {}).items():   # a face in two groups goes to the first
        faces = [f for f in dict.fromkeys(faces) if f not in taken]
        if faces:
            unique[k] = faces
            taken.update(faces)
    glows = unique
    keep = [mat]
    if paint:
        test = paint.get('test', 'sand')
        test = test if callable(test) else getattr(tripo, 'is_' + test)
        # A piece of the mesh mostly that colour (by area) is shell all over: single faces sampled in Tripo's atlas
        # miss on seams and stray texels, which left unpainted patches on a painted gun.
        hit = set(tripo.faces_where(obj, maps['BC'], test, share=0.6 if paint.get('by') != 'faces' else 0.5))
        shell = []
        if paint.get('by') == 'faces':
            # A mesh of one piece: each face by its own colour, then cleaned by its neighbours.
            shell = [f for f in tripo.majority(obj, hit) if f not in taken]
        else:
            area = [p.area for p in obj.data.polygons]
            for isl in tripo.islands(obj):
                if sum(area[f] for f in isl.faces if f in hit) > 0.5 * sum(area[f] for f in isl.faces):
                    shell += [f for f in isl.faces if f not in taken]
        if shell:
            painted = tripo.paint_set(maps, mat.name[len('Gun'):], paint.get('slot', SHELL.name),
                                      paint.get('tint', 0xe6d8b6), obj['TripoFile'],
                                      test=None if paint.get('test', 'sand') == 'sand' else test,
                                      mode=paint.get('mode', 'rgb'))
            glows[painted] = shell
            keep.append(painted)
            taken.update(shell)
    rest = [i for i in range(len(obj.data.polygons)) if i not in taken]
    g.add(m, *tripo.split(obj, {mat: rest, **glows}, keep_uv=tuple(keep)))


def tripo_build(key, spec):
    """Replaces the scripted model key with the Tripo model spec describes (see above)."""
    import looter_tripo as tripo
    from types import SimpleNamespace
    for old in g.parts.get(key, []):
        bpy.data.objects.remove(old)
    g.parts[key] = []
    if callable(spec):
        from types import SimpleNamespace as Namespace
        g.sockets = [s for s in g.sockets if s[0] != key]
        spec(Namespace(**globals()), key)
        return
    path = TRIPO_DIR + spec['glb']
    set_name = 'TripoBullpup' + key.replace('_', '')
    obj, maps = tripo.load(path)
    mat = tripo.write_set(maps, set_name, 'Gun' + set_name, path)
    tripo.fit(obj, *spec['box'], **spec.get('axes', {}))
    at = dict(STOCK_AT=STOCK_AT, BARREL_AT=BARREL_AT, MAG_AT=MAG_AT, SIGHT_AT=SIGHT_AT, MUZZLE_AT=MUZZLE_AT, VENTS=VENTS)
    ctx = SimpleNamespace(key=key, obj=obj, maps=maps, mat=mat, tripo=tripo, lg=lg, lt=lt, at=at, glow=tripo_glow,
                          log=lambda text: lt._log(f'Tripo {key}: {text}'), sockets=dict(spec.get('sockets', {})),
                          glow_faces={}, add=lambda *objs: g.add(key, *objs))
    if spec.get('prepare'):
        spec['prepare'](ctx)
    glows = {}
    for name, faces in ctx.glow_faces.items():   # faces prepare picked itself
        glows.setdefault(tripo_glow(name), []).extend(faces)
    for name, test in spec.get('glows', {}).items():
        test = test if callable(test) else getattr(tripo, 'is_' + test)
        faces = tripo.faces_where(obj, maps['BC'], test) or tripo.faces_where(obj, maps['BC'], test, corners=False)
        glows.setdefault(tripo_glow(name), []).extend(faces)
    for name, rule in spec.get('island_glows', {}).items():
        for isl in tripo.islands(obj):
            if rule(isl):
                glows.setdefault(tripo_glow(name), []).extend(isl.faces)
    tripo_add(key, tripo, obj, mat, glows, maps, spec.get('paint'))
    if spec.get('finish'):
        spec['finish'](ctx)
    g.sockets = [s for s in g.sockets if not (s[0] == key and s[1] in ctx.sockets)]
    for name, (uv, w) in ctx.sockets.items():
        g.socket(key, name, uv, w=w)


def body_sockets(m):
    """Every body's sockets, in the same places, so every part fits every body."""
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Grip', (22.5, -10.0)), ('Foregrip', (44.0, -4.5))):
        g.socket(m, name, uv)


def tripo_override():
    for key, (spec, file) in TRIPO.items():
        if key not in g.parts:
            raise KeyError(f'{file}: {key} is no Bullpup part')
        tripo_build(key, spec)


def tripo_lods(built):
    """LODs on the Tripo parts (about 10k triangles each, against a few hundred for a scripted part): first person keeps
    LOD0 (a held gun fills the screen); a part's switch sizes scale with its size against a body's, so a gun's parts
    switch together."""
    for key in TRIPO:
        share = max(built[key].dimensions) / BODY_LENGTH
        built[key]['LODs'] = TRIPO_LODS
        built[key]['LODScreens'] = ','.join(f'{s * share:.4f}' for s in TRIPO_LOD_SCREENS)


# --- Bodies: the shell and its fittings, all with the same sockets ---

SHELL_OUTLINE = [(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (40, 3.2), (47, 2.4), (51, 0.6), (51.5, -1.8), (49, -3.2),
                 (38.5, -3.5), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.6)]
LOOP_OUTLINE = [(36.8, -3.8), (37.6, -12.0), (36.4, -14.0), (34, -14.8), (25, -15.2), (22.0, -14.8), (20.6, -12.5),
                (19.6, -7.0), (19.4, -3.8)]
LOOP_HOLE = [(25.0, -5.8), (34.4, -5.8), (35.4, -7.0), (35.6, -11.8), (34.2, -13.0), (27.0, -13.0), (25.4, -11.8),
             (24.2, -7.2)]
WALNUT_DARK = lg.material('GunWalnutDark', 'GunWood', 0xa48670)


VENTS = (40.6, 42.8, 45.0)   # the nose vents' back edges


def vent_cut(u, w0, w1):
    return lg.cslab([(u, -1.6), (u + 0.9, -1.6), (u + 2.2, 1.2), (u + 1.3, 1.2)], w0, w1, round=0.3)


def shell(m, mat, outline=SHELL_OUTLINE, width=5.0, round=1.0, bevel=1.1, vents=True, extra_cuts=()):
    obj = lg.slab(outline, width, bevel=bevel, segments=3, mat=mat, round=round)
    cuts = [lg.cbox(2.6, 7.4, -1.0, 1.0, -3.0, -2.0)] + list(extra_cuts)
    if vents:
        for w0, w1 in ((-3.0, -2.1), (2.1, 3.0)):
            cuts += [vent_cut(u, w0, w1) for u in VENTS]
        # The heat behind them: an ember sheet at the vents' floor, hidden inside the shell elsewhere.
        for side in (-1.0, 1.0):
            g.add(m, lg.box(40.4, 47.6, -1.7, 1.3, 0.04, w=side * 2.12, bevel=0.0, mat=EMBER))
    g.add(m, lg.cut(obj, cuts))


def grip_loop(m, mat, outline=LOOP_OUTLINE, round=1.0, bevel=0.9):
    loop = lg.slab(outline, 3.6, bevel=bevel, segments=3, mat=mat, round=round)
    g.add(m, lg.cut(loop, [lg.cslab(LOOP_HOLE, -3.0, 3.0, round=1.0)]))


def side_plates(m, mat, rivet_mat, outline=((8, -4.8), (18.2, -4.8), (19.4, 1.6), (9.4, 1.6)), round=0.8, w=2.55):
    for side in (-1.0, 1.0):
        g.add(m, lg.slab(list(outline), 0.3, w=side * w, bevel=0.1, mat=mat, round=round))
        for u, v in ((9.6, -4.0), (17.6, -4.0), (10.6, 0.8), (18.2, 0.8)):
            g.add(m, lg.rivet(u, v, side * (w + 0.17), side, r=0.28, mat=rivet_mat))


def scroll(m, cu, cv, r0, side, w, turns=1.6, mat=BRASS):
    """A curl of inlaid engraving on a side face (w: the face's distance from the middle)."""
    pts = []
    for k in range(18):
        t = k / 17.0
        a = 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append((cu + side * r * math.cos(a), cv + r * math.sin(a), side * w))
    g.add(m, lg.pipe(pts, 0.06, mat=mat, sides=5))


def fittings(m, bolt=lg.BLACK, mount=SHELL, rail=lg.BLACK, accent=(21.0, 37.0, -2.75), accent_w=2.53, nut_ring=None,
             handle=lg.BLACK):
    """What every body has: the bolt in the port, the accent line, the stock plate, trigger, charging handle, the optic
    mount and rail, the barrel nut (nut_ring: a collar round it, brass on rarer shells) and sling loop, and the
    sockets."""
    if nut_ring is not None:
        g.add(m, lg.tube(50.9, BARREL_AT[0], 1.42, segments=20, mat=nut_ring))
    g.add(m, lg.box(2.8, 7.2, -0.9, 0.9, 4.2, bevel=0.1, mat=bolt))
    u0, u1, v = accent
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v, v + 0.25, 0.08, w=side * accent_w, bevel=0.0, mat=ACCENT))
    g.add(m, lg.box(-0.3, 0.3, -8.4, 1.8, 4.4, bevel=0.15, mat=lg.BLACK))
    g.add(m, lg.slab([(26.4, -5.8), (27.4, -5.8), (27.2, -7.0), (26.8, -8.0), (26.2, -8.2), (26.5, -7.0)], 0.6,
                     bevel=0.12, mat=lg.BLACK))
    g.add(m, lg.slab([(38.8, 2.9), (44.0, 2.6), (44.0, 3.6), (39.6, 3.9)], 1.0, w=1.9, bevel=0.2, mat=handle))
    if mount is not None:
        g.add(m, lg.slab([(12, 3.0), (34, 3.0), (34, 4.4), (32.5, 5.2), (13.5, 5.2), (12, 4.4)], 3.6, bevel=0.6,
                         segments=3, mat=mount, round=0.5))
    g.add(m, lg.rail(14, 32, 5.2, mat=rail))
    g.add(m, lg.tube(50.6, BARREL_AT[0], 1.25, segments=20, mat=lg.BLACK))
    g.add(m, lg.pipe([(43.0, -3.0), (43.6, -4.6), (45.6, -4.6), (46.2, -3.0)], 0.25, mat=lg.BLACK))
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Grip', (22.5, -10.0)), ('Foregrip', (44.0, -4.5))):
        g.socket(m, name, uv)


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # the sand shell the user picked, gunmetal plates
    shell(m, SHELL)
    grip_loop(m, SHELL)
    side_plates(m, METAL, lg.BLACK)
    fittings(m)

    m = B('Heritage')                                          # the sand shell dressed in brass: a gunmetal loop and mount
    shell(m, SHELL)
    grip_loop(m, METAL)
    side_plates(m, METAL, BRASS)
    for side in (-1.0, 1.0):                                   # a brass scroll on the nose, ahead of the vents
        scroll(m, 48.8, -1.2, 0.75, side, 2.53)
        g.add(m, lg.box(21.0, 37.0, 2.2, 2.32, 0.06, w=side * 2.53, bevel=0.0, mat=BRASS))   # a brass pinstripe
    fittings(m, mount=METAL, rail=METAL, nut_ring=BRASS, handle=BRASS)

    m = B('Carbon')                                            # dark and angular, sand plates
    angular = [(0.8, -9.6), (0, -8.8), (0, 2.4), (2.6, 3.4), (39, 3.4), (49.4, 1.0), (51.6, -0.6), (51.6, -2.2),
               (49.4, -3.4), (38.5, -3.6), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.8)]
    shell(m, lg.POLY_BLACK, outline=angular, round=0.3, bevel=0.5)
    grip_loop(m, lg.POLY_BLACK, outline=[(36.8, -3.8), (37.8, -12.6), (35.6, -14.8), (22.6, -15.2), (20.4, -13.0),
                                         (19.4, -3.8)], round=0.4, bevel=0.5)
    side_plates(m, SHELL, lg.BLACK, outline=((7.4, -5.0), (19.0, -5.0), (20.6, 1.8), (10.4, 1.8)), round=0.2)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(38, 1.2), (48.4, 0.6), (49.6, -0.6), (38, -0.2)], 0.3, w=side * 2.55, bevel=0.05,
                         mat=SHELL))
        g.add(m, lg.box(21.0, 37.0, 2.2, 2.34, 0.06, w=side * 2.53, bevel=0.0, mat=EMBER))   # an ember pinstripe
    fittings(m, mount=lg.POLY_BLACK)

    m = B('Salvager')                                          # welded rusty panels, a pipe trigger guard
    shell(m, lg.SALVAGE, round=0.3, bevel=0.2, vents=False)
    for side in (-1.0, 1.0):
        w = side * 2.55
        for u0, u1 in ((1.0, 18.0), (21.0, 46.0)):
            g.add(m, lg.slab([(u0, -3.6), (u1, -3.6), (u1, 2.4), (u0, 2.4)], 0.3, w=w, bevel=0.05, mat=lg.SALVAGE))
            for u in range(int(u0) + 1, int(u1), 4):
                for v in (-2.9, 1.7):
                    g.add(m, lg.rivet(u, v, w + side * 0.17, side, r=0.3, mat=lg.STEEL))
        g.add(m, lg.pipe([(0.6, 3.0, w * 0.98), (25, 3.1, w * 0.98), (48, 2.4, w * 0.98)], 0.2, mat=lg.COPPER, sides=6))
    # A jury-rigged coolant line along the right of the nose, glowing with the heat it carries.
    g.add(m, lg.pipe([(29.0, -2.4, -2.88), (38.0, -2.2, -2.88), (46.0, -1.6, -2.88), (48.0, -0.6, -2.7)], 0.17,
                     mat=EMBER, sides=6))
    for u in (31.0, 41.5):
        g.add(m, lg.box(u - 0.4, u + 0.4, -2.85, -1.6, 0.4, w=-2.86, bevel=0.05, mat=lg.STEEL))   # its clamps
    g.add(m, lg.slab([(19.6, -5.2), (24.4, -5.2), (23.0, -15.4), (21.6, -16.2), (18.4, -15.8), (17.8, -14.6)], 3.4,
                     bevel=0.8, segments=3, mat=lg.TAPE, round=0.6))           # taped grip
    g.add(m, lg.pipe([(24.6, -5.4), (26.0, -13.2), (34.6, -13.2), (36.4, -4.6)], 0.45, mat=lg.STEEL))   # guard
    fittings(m, bolt=lg.STEEL, mount=lg.SALVAGE)

    m = B('Armored')                                           # gunmetal armor on the sand shell, brass rivets, ember slits
    shell(m, SHELL)
    grip_loop(m, SHELL)
    for side in (-1.0, 1.0):
        w0, w1 = (2.0, 3.6) if side > 0 else (-3.6, -2.0)
        g.add(m, lg.slab([(5, -5.4), (20.0, -5.0), (21.0, 2.6), (6.0, 2.6)], 0.7, w=side * 2.75, bevel=0.15, mat=METAL,
                         round=0.5))
        nose = lg.slab([(38, -3.0), (49.8, -2.2), (50.6, 0.0), (38, 2.4)], 0.7, w=side * 2.75, bevel=0.15, mat=METAL,
                       round=0.5)
        g.add(m, lg.cut(nose, [vent_cut(u, w0, w1) for u in VENTS]))   # the shell's vents show through the armor
        g.add(m, lg.box(5.6, 20.4, 2.62, 2.8, 0.74, w=side * 2.75, bevel=0.05, mat=BRASS))   # brass edge on the plate
        for u, v in ((6.6, -4.4), (19.2, -4.0), (7.0, 1.8), (19.8, 1.8), (39.4, -2.2), (39.4, 1.6), (48.6, -1.4)):
            g.add(m, lg.rivet(u, v, side * 3.12, side, r=0.34, mat=BRASS))
    fittings(m, accent=(22.0, 36.5, -2.75), nut_ring=BRASS, handle=BRASS)

    m = B('Skeleton')                                          # windows onto the action, an ember-hot spring guide
    windows = [lg.cslab([(9.0, -4.0), (19.0, -4.0), (19.0, 1.4), (9.0, 1.4)], -3.0, 3.0, round=1.0),
               lg.cslab([(39.0, -2.4), (47.4, -2.0), (47.4, 1.0), (39.0, 1.6)], -3.0, 3.0, round=0.8)]
    shell(m, SHELL, vents=False, extra_cuts=windows)
    g.add(m, lg.box(8.6, 19.4, -3.6, 1.0, 1.8, bevel=0.2, mat=BRASS))         # the bolt carrier
    g.add(m, lg.tube(38.6, 48.0, 0.9, v=-0.4, mat=EMBER))                      # recoil spring guide, glowing
    for u in range(39, 48):
        g.add(m, lg.tube(u, u + 0.35, 1.05, v=-0.4, segments=12, mat=METAL))
    grip_loop(m, SHELL)
    fittings(m, nut_ring=BRASS)

    m = B('Sleek')                                             # a longer, smoother two-tone shell, an ember pinstripe
    sleek = [(1.6, -9.4), (0, -8.0), (0, 1.6), (3.0, 3.4), (36, 3.6), (45, 2.8), (50.4, 1.0), (51.8, -1.2), (50.0, -3.0),
             (38.5, -3.6), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3.6, -8.4)]
    shell(m, SHELL, outline=sleek, round=2.0, bevel=1.4, vents=False)
    g.add(m, lg.slab([(1.4, -9.2), (3.6, -8.4), (9.5, -5.8), (37, -4.6), (38.6, -3.4), (49.6, -2.8), (50, -1.6),
                      (38, -2.0), (10, -3.6), (1.0, -7.0)], 5.2, bevel=1.0, segments=3, mat=lg.POLY_GREY, round=1.2))
    for side in (-1.0, 1.0):
        g.add(m, lg.box(4.0, 44.0, 1.1, 1.4, 0.06, w=side * 2.52, bevel=0.0, mat=EMBER))
    grip_loop(m, lg.POLY_GREY, round=1.6)
    fittings(m, accent=(6.0, 47.0, -2.35), accent_w=2.6, nut_ring=BRASS)

    m = B('Marksman')                                          # a built-in cheek rest and a bipod rail under the nose
    shell(m, SHELL)
    grip_loop(m, SHELL)
    side_plates(m, METAL, BRASS)
    g.add(m, lg.slab([(0.2, 2.0), (11.0, 3.0), (11.6, 4.8), (0.6, 4.6)], 4.4, bevel=0.6, segments=3, mat=lg.POLY_GREY,
                     round=0.9))
    g.add(m, lg.rail(39.5, 49.0, -3.3, width=2.0, mat=lg.BLACK, down=True))
    g.add(m, lg.slab([(36.6, -3.6), (39.0, -3.6), (39.0, -6.4), (37.8, -6.8), (36.8, -5.6)], 3.0, bevel=0.5,
                     segments=3, mat=SHELL, round=0.5))                         # a thumb rest ahead of the loop
    fittings(m, nut_ring=BRASS)


# --- Barrels: from the barrel nut forward ---

def barrels():
    u0 = BARREL_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    def shroud(m, length, r, slots=(), collar=METAL):
        """A chunky gunmetal shroud from the barrel nut, its nose rounded, slots cut along its upper sides over an ember
        core; a collar ring at its nose (brass on the commons too: the frontier look's brass tip)."""
        obj = lg.turned([(0.0, r), (length - 0.8, r), (length - 0.25, r - 0.3), (length, r - 0.6)], u0, segments=20,
                        mat=METAL)
        if slots:
            # slots on the upper left and right, so the glow shows from the side and from behind the gun
            obj = lg.cut(obj, [lg.cbox(u0 + a, u0 + a + 1.1, 0.2, r + 0.5, -0.45, 0.45, rotate_u=t)
                               for a in slots for t in (-55.0, 55.0)])
            g.add(m, lg.tube(u0 + slots[0] - 0.2, u0 + slots[-1] + 1.3, r - 0.35, segments=12, mat=EMBER))
        g.add(m, obj)
        g.add(m, lg.tube(u0 + length - 1.4, u0 + length - 0.9, r + 0.06, segments=20, mat=collar))

    m = model('Barrel', 'Stub', BARREL_AT)                      # a stubby gunmetal shroud, the barrel just showing
    g.add(m, lg.tube(u0, u0 + 6.0, 0.82, mat=lg.BLACK))
    shroud(m, 4.6, 1.5, collar=BRASS)
    tip(m, 6.0)

    m = model('Barrel', 'Carbine', BARREL_AT)                   # the standard barrel in a slotted shroud, ember in its slots
    g.add(m, lg.tube(u0, u0 + 12.0, 0.8, mat=lg.BLACK))
    shroud(m, 9.0, 1.6, slots=(1.4, 3.2, 5.0, 6.8), collar=BRASS)
    tip(m, 12.0)

    m = model('Barrel', 'Fluted', BARREL_AT)                    # longer, with six flutes, a brass collar
    fluted = lg.tube(u0, u0 + 16.0, 0.9, segments=24, mat=lg.STEEL)
    g.add(m, lg.cut(fluted, [lg.cbox(u0 + 3.0, u0 + 14.2, 0.62, 1.3, -0.17, 0.17, rotate_u=a) for a in range(0, 360, 60)]))
    g.add(m, lg.tube(u0, u0 + 2.2, 1.2, segments=20, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 2.2, u0 + 2.7, 1.05, segments=20, mat=BRASS))
    tip(m, 16.0)

    m = model('Barrel', 'Marksman', BARREL_AT)                  # long and thin, brass bands, a folding front post
    g.add(m, lg.tube(u0, u0 + 24.0, 0.75, segments=20, mat=lg.BLACK))
    g.add(m, lg.slab([(u0 + 2.0, -1.0), (u0 + 4.6, -1.0), (u0 + 4.6, 1.4), (u0 + 2.4, 1.4)], 2.0, bevel=0.25,
                     mat=METAL, round=0.3))
    g.add(m, lg.box(u0 + 2.5, u0 + 4.5, 1.38, 1.5, 0.3, bevel=0.0, mat=EMBER))    # a gas-block heat line
    for u in (8.0, 14.0):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.6, 0.86, segments=20, mat=BRASS))
    g.add(m, lg.box(u0 + 19.6, u0 + 22.0, -0.6, 1.1, 1.8, bevel=0.2, mat=METAL))
    g.add(m, lg.slab([(u0 + 20.2, 1.1), (u0 + 21.6, 1.1), (u0 + 21.3, 4.2), (u0 + 20.6, 4.2)], 0.5, bevel=0.1,
                     mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 20.48, u0 + 20.62, 0.13, v=3.9, segments=10, mat=CYAN))  # a cyan dot facing the eye
    g.add(m, lg.tube(u0 + 22.6, u0 + 23.4, 0.84, segments=20, mat=BRASS))
    tip(m, 24.0)

    m = model('Barrel', 'Heavy', BARREL_AT)                     # a thick bull barrel with ember heat rings
    g.add(m, lg.turned([(0, 1.25), (16.6, 1.25), (17.0, 1.05)], u0, segments=24, mat=lg.BLACK))
    for u in (10.4, 12.0, 13.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.45, 1.3, segments=16, mat=EMBER))
    tip(m, 17.0)

    m = model('Barrel', 'Shrouded', BARREL_AT)                  # in a ported cooling shroud, the barrel hot inside
    g.add(m, lg.tube(u0, u0 + 16.5, 0.8, mat=lg.STEEL))
    g.add(m, lg.tube(u0 + 1.6, u0 + 12.6, 1.05, segments=12, mat=EMBER))
    shroud = lg.tube(u0 + 0.4, u0 + 14.0, 1.9, segments=24, mat=lg.BLACK)
    holes = []
    for u in (2.4, 5.0, 7.6, 10.2):
        holes += [lg.cpin(u0 + u, 0.0, -2.5, 2.5, 0.55), lg.upright(u0 + u + 1.3, -2.5, 2.5, 0.55)]
    g.add(m, lg.cut(shroud, holes))
    g.add(m, lg.tube(u0 + 13.6, u0 + 14.6, 2.05, segments=24, mat=lg.STEEL))
    tip(m, 16.5)

    m = model('Barrel', 'Railed', BARREL_AT)                    # a sand handguard, ember in its slots, a rail beneath
    guard = lg.slab([(u0 - 0.6, 0.6), (u0 + 11.4, 0.6), (u0 + 12.6, -0.2), (u0 + 12.6, -2.4), (u0 + 11.6, -3.2),
                     (u0 - 0.6, -3.2)], 4.2, bevel=0.8, segments=3, mat=SHELL, round=0.8)
    slots = [lg.cslab([(u0 + u, -2.2), (u0 + u + 2.4, -2.2), (u0 + u + 2.4, -0.8), (u0 + u, -0.8)], w0, w1, round=0.4)
             for u in (1.5, 5.0, 8.5) for w0, w1 in ((-3, -1.7), (1.7, 3))]
    g.add(m, lg.cut(guard, slots))
    g.add(m, lg.box(u0 + 1.3, u0 + 11.1, -2.3, -0.7, 3.44, bevel=0.0, mat=EMBER))   # seen only through the slots
    g.add(m, lg.rail(u0 + 0.4, u0 + 11.0, -3.2, width=2.0, mat=lg.BLACK, down=True))
    g.add(m, lg.tube(u0, u0 + 16.0, 0.8, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 12.6, u0 + 13.2, 0.95, segments=20, mat=BRASS))
    tip(m, 16.0)

    m = model('Barrel', 'Piston', BARREL_AT)                    # a gas piston tube over the barrel, brass fittings
    g.add(m, lg.tube(u0, u0 + 15.0, 0.78, mat=lg.BLACK))
    g.add(m, lg.tube(u0, u0 + 10.6, 0.48, v=1.75, mat=lg.STEEL))
    g.add(m, lg.slab([(u0 + 9.8, -1.0), (u0 + 12.2, -1.0), (u0 + 12.2, 2.5), (u0 + 10.6, 2.6)], 2.0, bevel=0.25,
                     mat=METAL, round=0.3))
    g.add(m, lg.tube(u0, u0 + 1.4, 0.65, v=1.75, segments=12, mat=BRASS))
    g.add(m, lg.tube(u0 + 5.0, u0 + 5.5, 0.6, v=1.75, segments=12, mat=BRASS))
    tip(m, 15.0)


# --- Muzzle devices: around their own origin, screwed onto the barrel's tip ---

def muzzles():
    u0 = MUZZLE_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Muzzle', 'Cap', MUZZLE_AT)                       # a brass thread protector
    g.add(m, lg.turned([(0, 0.92), (1.5, 0.92), (1.8, 0.7)], u0, segments=20, mat=BRASS))
    tip(m, 1.8)

    m = model('Muzzle', 'Birdcage', MUZZLE_AT)                  # slotted flash hider, closed underneath
    cage = lg.turned([(0, 0.98), (5.4, 0.98), (5.8, 0.82)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(cage, [lg.cbox(u0 + 1.6, u0 + 5.0, 0.55, 1.3, -0.16, 0.16, rotate_u=a) for a in (-72, -36, 0, 36, 72)]))
    g.add(m, lg.tube(u0 + 5.3, u0 + 5.8, 0.9, segments=16, mat=BRASS))   # a brass crown
    tip(m, 5.8)

    m = model('Muzzle', 'Prong', MUZZLE_AT)                     # three-prong flash hider
    g.add(m, lg.turned([(0, 0.98), (1.8, 0.98)], u0, segments=20, mat=lg.BLACK))
    for a in (0, 120, 240):
        g.add(m, lg.mapped(lg.cbox(u0 + 1.6, u0 + 6.2, 0.42, 1.0, -0.3, 0.3, rotate_u=a), lg.BLACK))
    g.add(m, lg.tube(u0 + 0.2, u0 + 0.7, 1.04, segments=20, mat=BRASS))
    tip(m, 6.2)

    m = model('Muzzle', 'Brake', MUZZLE_AT)                     # two side ports and a brass ring
    brake = lg.turned([(0, 1.15), (4.8, 1.15), (5.2, 0.95)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + 1.0, u0 + 2.1, -0.5, 0.5, -2, 2), lg.cbox(u0 + 2.8, u0 + 3.9, -0.5, 0.5, -2, 2)]))
    g.add(m, lg.tube(u0, u0 + 0.5, 1.22, segments=20, mat=BRASS))
    tip(m, 5.2)

    m = model('Muzzle', 'Compensator', MUZZLE_AT)               # ports on top push the muzzle down; ember in them
    comp = lg.turned([(0, 1.1), (4.6, 1.1), (5.4, 0.8)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(comp, [lg.cbox(u0 + u, u0 + u + 0.7, 0.2, 1.6, -0.36, 0.36) for u in (1.0, 2.2, 3.4)]))
    g.add(m, lg.tube(u0 + 0.8, u0 + 4.3, 0.62, segments=12, mat=EMBER))
    g.add(m, lg.tube(u0, u0 + 0.5, 1.17, segments=20, mat=BRASS))
    tip(m, 5.4)

    m = model('Muzzle', 'BoxBrake', MUZZLE_AT)                  # a big boxy brake, ember blast in its ports
    box = lg.box(u0, u0 + 7.4, -1.6, 1.6, 3.6, bevel=0.35, mat=lg.BLACK)
    g.add(m, lg.cut(box, [lg.cbox(u0 + u, u0 + u + 1.0, -1.2, 1.2, -3, 3) for u in (1.3, 3.2, 5.1)]))
    g.add(m, lg.tube(u0 + 1.0, u0 + 6.4, 0.5, segments=12, mat=EMBER))
    g.add(m, lg.box(u0 + 6.9, u0 + 7.25, -1.68, 1.68, 3.75, bevel=0.1, mat=BRASS))
    tip(m, 7.4)

    m = model('Muzzle', 'Suppressor', MUZZLE_AT)                # a short can in a sand sleeve, a brass end
    g.add(m, lg.turned([(0, 0.95), (0.7, 1.6), (9.4, 1.6), (10.0, 1.2)], u0, segments=24, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 2.0, u0 + 7.6, 1.68, segments=24, mat=SHELL))
    g.add(m, lg.tube(u0 + 9.0, u0 + 9.8, 1.66, segments=24, mat=BRASS))
    tip(m, 10.0)

    m = model('Muzzle', 'LongSuppressor', MUZZLE_AT)            # a long can in a sand sleeve, brass bands, ember vents
    can = lg.turned([(0, 0.95), (1.0, 1.85), (17.0, 1.85), (17.8, 1.3)], u0, segments=24, mat=lg.BLACK)
    g.add(m, lg.cut(can, [lg.cbox(u0 + u, u0 + u + 0.5, 0.5, 2.3, -0.3, 0.3, rotate_u=a)
                          for u in (14.9, 15.9) for a in (-50, 0, 50)]))
    g.add(m, lg.tube(u0 + 14.4, u0 + 16.8, 1.5, segments=16, mat=EMBER))
    g.add(m, lg.tube(u0 + 3.0, u0 + 14.0, 1.97, segments=24, mat=SHELL))
    for u in (3.0, 8.3, 13.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 2.05, segments=24, mat=BRASS))
    tip(m, 17.8)


# --- Magazines: around where they seat ---

def mag(m, front, rear, width, mat, ribs=0, rib_top=-17.0, steps=10, base=None):
    """base: the base plate's material (the magazine's own by default; brass on rarer ones)."""
    outline = lg.curved_outline(front, rear, steps)
    g.add(m, lg.slab(outline, width, bevel=0.3, mat=mat))
    fr, rr = outline[:steps + 1], list(reversed(outline[steps + 1:]))
    a, b = fr[-1], rr[-1]
    g.add(m, lg.slab([b, a, (a[0] + 0.25, a[1] - 0.8), (b[0] + 0.25, b[1] - 0.8)], width + 0.4, bevel=0.2,
                     mat=base or mat))
    for k in range(ribs):
        v = rib_top - k * 1.3
        # The rib spans the magazine's width at that height (edges found on the outline's curves).
        f = min(fr, key=lambda p: abs(p[1] - v))
        r = min(rr, key=lambda p: abs(p[1] - v))
        g.add(m, lg.box(r[0] - 0.2, f[0] + 0.2, v - 0.35, v + 0.35, width + 0.3, bevel=0.15, mat=mat))


def magazines():
    top = MAG_AT[1]
    F, R = 17.6, 10.4

    m = model('Magazine', '20', MAG_AT)
    mag(m, [(F, top), (17.9, -10), (18.3, -16.0)], [(R, top), (10.6, -10), (10.9, -15.6)], 2.4, lg.POLY_BLACK, 2, -12.5)

    m = model('Magazine', '30', MAG_AT)
    mag(m, [(F, top), (18.3, -15.0), (19.8, -24.0)], [(R, top), (10.9, -15.0), (12.0, -23.4)], 2.4, lg.POLY_BLACK, 4)

    m = model('Magazine', '30Window', MAG_AT)                   # smoke polymer with a window onto the rounds
    mag(m, [(F, top), (18.3, -15.0), (19.8, -24.0)], [(R, top), (10.9, -15.0), (12.0, -23.4)], 2.4, lg.POLY_GREY, 0)
    g.parts[m][0] = lg.cut(g.parts[m][0], [lg.cbox(13.0, 15.4, -21.0, -8.0, -2.0, -0.7)])
    g.add(m, lg.box(13.0, 15.4, -21.0, -8.0, 1.0, w=-0.3, bevel=0.3, mat=lg.BRASS))

    m = model('Magazine', '40', MAG_AT)                         # a long curved steel magazine, a brass base
    mag(m, [(F, top), (18.9, -17.0), (22.2, -30.0)], [(R, top), (11.3, -17.0), (14.2, -29.4)], 2.5, lg.BLUED, 0,
        base=BRASS)
    outline = lg.curved_outline([(F, top), (18.9, -17.0), (22.2, -30.0)], [(R, top), (11.3, -17.0), (14.2, -29.4)])
    mid = [((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5) for a, b in zip(outline[:11], reversed(outline[11:]))][2:-1]
    for side in (-1.0, 1.0):
        g.add(m, lg.strip(mid, 0.3, 1.0, w=side * 1.3, mat=lg.BLUED))

    m = model('Magazine', 'Coupled', MAG_AT)                    # two 30s clamped side by side
    for w, dv in ((0.0, 0.0), (2.7, -1.0)):
        o = lg.slab(lg.curved_outline([(F, top + dv), (18.3, -15.0 + dv), (19.8, -24.0 + dv)],
                                      [(R, top + dv), (10.9, -15.0 + dv), (12.0, -23.4 + dv)]), 2.4, w=w, bevel=0.3,
                    mat=lg.POLY_BLACK)
        g.add(m, o)
    for v in (-10.5, -18.5):
        g.add(m, lg.box(10.2, 19.4, v - 0.8, v + 0.8, 5.6, w=1.35, bevel=0.3, mat=BRASS))

    m = model('Magazine', '60', MAG_AT)                         # a wide quad-stack casket, a brass base
    g.add(m, lg.box(R, F, -9.0, top, 2.4, bevel=0.2, mat=lg.POLY_BLACK))
    mag(m, [(F + 0.1, -8.6), (18.6, -18.0), (19.8, -30.0)], [(R - 0.1, -8.6), (10.9, -18.0), (11.8, -29.6)], 3.8,
        lg.POLY_BLACK, 3, -24.0, base=BRASS)

    def ember_ring(u, v, w0, side, r):
        """A glowing ring round a drum's cap (the feed's heat), proud of the drum's face."""
        a = w0 if side > 0 else w0 - 0.06
        g.add(m, lg.pin(u, v, a, a + 0.06, r, segments=24, mat=EMBER))

    m = model('Magazine', 'Drum', MAG_AT)                       # a 75-round drum: sand faces, brass caps, an ember ring
    g.add(m, lg.slab([(R, top), (F, top), (F + 0.4, -10.0), (R - 0.4, -10.0)], 2.4, bevel=0.2, mat=METAL))
    g.add(m, lg.pin(14.0, -16.4, -2.4, 2.4, 6.8, segments=32, mat=SHELL,
                    profile=[(0, 6.0), (0.5, 6.8), (4.3, 6.8), (4.8, 6.0)]))
    for side in (-1.0, 1.0):
        w0 = 2.4 if side > 0 else -2.9
        ember_ring(14.0, -16.4, 2.4 if side > 0 else -2.4, side, 3.3)
        g.add(m, lg.pin(14.0, -16.4, w0, w0 + 0.5, 2.6, segments=20, mat=BRASS))

    m = model('Magazine', 'TwinDrum', MAG_AT)                   # a 100-round twin drum, as the drum
    g.add(m, lg.slab([(R, top), (F, top), (F + 0.3, -20.0), (R - 0.3, -20.0)], 2.4, bevel=0.25, mat=METAL))
    for side in (-1.0, 1.0):
        w0 = 1.1 if side > 0 else -4.3
        g.add(m, lg.pin(14.0, -14.5, w0, w0 + 3.2, 6.2, segments=32, mat=SHELL,
                        profile=[(0, 5.6), (0.4, 6.2), (2.8, 6.2), (3.2, 5.6)]))
        ember_ring(14.0, -14.5, 4.3 if side > 0 else -4.3, side, 2.9)
        cap = 4.3 if side > 0 else -4.7
        g.add(m, lg.pin(14.0, -14.5, cap, cap + 0.4, 2.2, segments=20, mat=BRASS))


# --- Element magazines (2026-10-22): the 30-rounder's shape (so they seat like it), dressed in their element ---

FRONT30 = [(17.6, MAG_AT[1]), (18.3, -15.0), (19.8, -24.0)]
REAR30 = [(10.4, MAG_AT[1]), (10.9, -15.0), (12.0, -23.4)]
HALF30 = 1.2   # its side faces


def edge_u(curve, v):
    """Where a magazine's edge curve [(u, v) start, control, end] is at height v (u)."""
    pts = lg.bezier(*curve, n=40)
    for (u0, v0), (u1, v1) in zip(pts, pts[1:]):
        if min(v0, v1) <= v <= max(v0, v1) and v0 != v1:
            return u0 + (u1 - u0) * (v - v0) / (v1 - v0)
    return pts[-1][0]


def mid_u(v):
    return (edge_u(REAR30, v) + edge_u(FRONT30, v)) * 0.5


def element_magazines():
    M = lambda key: model('Magazine', key, MAG_AT)
    rake = (-0.17, 1.0, 0.0)   # up along the magazine (it leans forward at its foot)

    m = M('Ember30')                                           # heat-scored steel, ember vents a side, temper bands
    mag(m, FRONT30, REAR30, 2.4, lg.SCORCHED, base=METAL)
    cuts = []
    for v in (-9.2, -12.2, -15.2, -18.2):
        r, f = edge_u(REAR30, v), edge_u(FRONT30, v)
        slot = [(r + 1.4, v - 0.45), (f - 1.6, v - 0.45), (f - 1.0, v + 0.45), (r + 2.0, v + 0.45)]
        cuts += [lg.cslab(slot, -1.8, -HALF30 + 0.3, round=0.25), lg.cslab(slot, HALF30 - 0.3, 1.8, round=0.25)]
    g.parts[m][0] = lg.cut(g.parts[m][0], cuts)
    heat = [(edge_u(REAR30, -8.4) + 1.1, -8.4), (edge_u(FRONT30, -8.4) - 0.9, -8.4),
            (edge_u(FRONT30, -19.2) - 0.9, -19.2), (edge_u(REAR30, -19.2) + 1.1, -19.2)]
    for side in (-1.0, 1.0):   # the heat behind the vents, hidden inside the magazine elsewhere
        g.add(m, lg.slab(heat, 0.04, w=side * (HALF30 - 0.26), bevel=0.0, mat=EMBER))
        g.add(m, lg.strip([(12.5, -23.95), (19.6, -24.45)], 0.04, 0.24, w=side * 1.42, mat=EMBER))   # a slit in the base
    for v in (-6.9, -21.4):                                    # temper-blued bands where the heat scored it
        g.add(m, lg.box(edge_u(REAR30, v) - 0.2, edge_u(FRONT30, v) + 0.2, v - 0.4, v + 0.4, 2.7, bevel=0.12,
                        mat=lg.TEMPER))

    m = M('Saint30')                                           # blued steel in pale gold: a saint's medal a side,
    mag(m, FRONT30, REAR30, 2.4, METAL, base=lg.GILT)          # filigree, a gold-white glow band down its front
    for side in (-1.0, 1.0):
        w = side * HALF30
        g.add(m, *lg.medal((mid_u(-13.2), -13.2, w), (0.0, 0.0, side), 1.7, up=rake))
        g.add(m, lg.curl((mid_u(-8.6), -8.6, w), (0.0, 0.0, side), 1.2, up=rake, flip=side, turns=1.5))
        g.add(m, *lg.vine((mid_u(-15.6), -15.6, w), (mid_u(-22.0), -22.0, w), (0.0, 0.0, side), 0.5, waves=1.5,
                          curls=False))
        band = [(edge_u(FRONT30, v) - 0.6, v, w) for v in (-6.0, -9.0, -12.0, -15.0, -18.0, -21.0, -23.2)]
        g.add(m, lg.pipe(band, 0.12, mat=lg.SOUL, sides=4))
    g.add(m, lg.strip([(edge_u(FRONT30, v) + 0.04, v) for v in (-6.0, -10.0, -14.0, -18.0, -22.0, -23.6)], 0.9, 0.12,
                      mat=lg.SOUL))                            # and down the front spine

    m = M('Salt30')                                            # pale smoke polymer crusted with salt: crystals at its
    mag(m, FRONT30, REAR30, 2.4, lg.POLY_GREY, 2, -19.5, base=lg.STEEL)   # foot and flanks, cyan-white veins
    g.add(m, *lg.salt_cluster(301, (16.4, -24.6, 0.0), (0.08, -1.0, 0.0), 2.4, 7, length=(1.23, 2.8), r=(0.3, 0.54),
                              up=(1.0, 0.0, 0.0)))
    for side in (-1.0, 1.0):
        w = side * HALF30
        g.add(m, *lg.salt_cluster(311 + int(side), (17.4, -21.0, w), (0.25, -0.35, side), 1.5, 5, length=(0.88, 1.93),
                                  r=(0.21, 0.39)))
        g.add(m, *lg.salt_cluster(321 + int(side), (12.0, -12.0, w), (-0.3, 0.0, side), 0.8, 3, length=(0.61, 1.23),
                                  r=(0.15, 0.27), glow_every=2))
        vein = [(13.0, -6.4, w), (14.6, -9.0, w), (13.6, -11.6, w), (15.6, -14.6, w), (14.6, -17.4, w),
                (16.8, -20.4, w)]
        g.add(m, lg.pipe(vein, 0.09, mat=lg.SALT_GLOW, sides=5))
        g.add(m, lg.pipe([(14.6, -9.0, w), (16.6, -10.4, w), (17.2, -12.4, w)], 0.07, mat=lg.SALT_GLOW, sides=5))


# --- See-through sights: aiming down sights puts the eye 22 cm behind SOCKET_Aim, looking along the axis ---

WALL = 0.12   # cm: thin tube walls keep the clear opening as wide as the tube


def hollow(profile, u0, v, segments, mat, wall=WALL):
    """A tube turned round an axis along u (profile [(du, r), ...] as lg.turned) but open at both ends: a thin wall
    that follows the outline inside, so the eye looks down the tube instead of at an end cap."""
    outer = [(r * lg.CM, du * lg.CM) for du, r in profile]
    inner = [((r - wall) * lg.CM, du * lg.CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(lg.at(u0, v)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return lg.mapped(obj, mat)


def optics(m, u_eye, u_obj, v, r_eye, r_obj, dot, wall=WALL):
    """A tube sight's glass: a lens just inside the eyepiece and one inside the objective, and the reticle just
    behind the objective lens, a dot (dot=True) or a fine cross, tiny because it sits over the screen's crosshair."""
    g.add(m, lg.tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=lg.LENS))
    g.add(m, lg.tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=lg.LENS))
    u = u_obj - 0.4
    if dot:
        g.add(m, lg.pin(u, v, -0.06, 0.06, 0.06, mat=RETICLE, segments=10))
    else:
        g.add(m, lg.box(u, u + 0.02, v - 0.075, v + 0.075, 0.02, bevel=0.0, mat=RETICLE))
        g.add(m, lg.box(u, u + 0.02, v - 0.01, v + 0.01, 0.15, bevel=0.0, mat=RETICLE))


# --- Sights: around the middle of the rail ---

def sights():
    v0 = SIGHT_AT[1]
    S = lambda key: model('Sight', key, SIGHT_AT)

    m = S('Iron')                                                # flip-up leaves: a wide aperture, an A-frame post
    # (2026-10-27 redesign: lg's iron sight notes.) The rear leaf is a rounded plate round an aperture 1.8 cm across, the
    # front an A-frame whose slanted ears stop under the line round a post with its cyan dot on it; both stand up from
    # sand hinge blocks.
    h = v0 + 2.6
    ur, uf = 16.8, 30.8
    for u in (ur, uf):
        g.add(m, lg.box(u - 1.2, u + 1.2, v0, v0 + 0.6, 2.4, bevel=0.15, mat=lg.BLACK))      # the clamp
        g.add(m, lg.pin(u + 0.55, v0 + 0.85, -1.05, 1.05, 0.32, mat=SHELL, segments=14))     # the hinge
    leaf = lg.face([(-1.2, v0 + 0.55), (1.2, v0 + 0.55), (1.45, h - 0.3), (1.45, h + 0.75), (0.95, h + 1.35),
                    (-0.95, h + 1.35), (-1.45, h + 0.75), (-1.45, h - 0.3)], ur - 0.25, ur + 0.25, bevel=0.1,
                   mat=lg.BLACK, round=0.25)
    g.add(m, lg.cut(leaf, [lg.tube(ur - 1.0, ur + 1.0, 0.9, v=h, segments=28)]))   # the aperture
    g.add(m, hollow([(0.0, 0.98), (0.06, 0.98)], ur - 0.3, h, 28, SHELL, wall=0.08))   # a sand lip round it
    lg.post_ears(g, m, uf, v0 + 0.55, h, half=1.35, gap=0.55, drop=0.45, thick=0.7, slant=0.45)
    lg.front_post(g, m, uf, v0 + 1.0, h)
    g.socket(m, 'Aim', (ur, h))   # the aperture's center: the front post's dot sits in its middle

    m = S('RedDot')                                              # a 1x tube, a cyan LED knob, a sand sunshade
    g.add(m, lg.box(19.4, 26.6, v0, v0 + 1.3, 2.2, bevel=0.25, mat=lg.BLACK))
    g.add(m, hollow([(0, 1.45), (1.0, 1.45), (1.4, 1.25), (6.6, 1.25), (7.0, 1.45), (8.0, 1.45)], 19.0, v0 + 2.8, 20,
                    lg.BLACK))
    g.add(m, hollow([(0, 1.55), (1.6, 1.6)], 26.6, v0 + 2.8, 20, SHELL, wall=0.15))
    g.add(m, lg.pin(23.0, v0 + 2.8, -2.0, -1.2, 0.55, mat=lg.BLACK))
    g.add(m, lg.pin(23.0, v0 + 2.8, -2.1, -2.0, 0.34, mat=CYAN))
    optics(m, 19.0, 27.0, v0 + 2.8, 1.45, 1.45, dot=True)
    g.socket(m, 'Aim', (19.2, v0 + 2.8))   # the eyepiece lens

    m = S('Reflex')                                              # an open window in a sand hood, a cyan emitter strip
    g.add(m, lg.box(19.5, 26.5, v0, v0 + 1.0, 2.6, bevel=0.25, mat=lg.BLACK))
    frame = lg.slab([(21.0, v0 + 1.0), (26.4, v0 + 1.0), (26.4, v0 + 4.4), (25.0, v0 + 5.2), (22.4, v0 + 5.2),
                     (21.0, v0 + 4.4)], 2.4, bevel=0.2, mat=SHELL, round=0.3)
    g.add(m, lg.cut(frame, [lg.cbox(20.0, 27.4, v0 + 1.6, v0 + 4.6, -0.95, 0.95)]))
    g.add(m, lg.box(25.6, 25.75, v0 + 1.6, v0 + 4.6, 1.9, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.box(24.9, 26.1, v0 + 1.0, v0 + 1.15, 1.5, bevel=0.0, mat=CYAN))
    # The dot is a disc facing the eye, 4 mm across (was a 2 mm pin across the gun, lost in the tall window: PIE
    # 2026-10-28), with a thin ring round it (1.1 cm across) to catch the eye; both GunReticle, so the element tints them.
    g.add(m, lg.tube(25.46, 25.5, 0.2, v=v0 + 3.1, segments=14, mat=RETICLE))
    g.add(m, hollow([(0.0, 0.55), (0.04, 0.55)], 25.46, v0 + 3.1, 24, RETICLE, wall=0.07))
    g.socket(m, 'Aim', (25.5, v0 + 3.1))   # the dot

    m = S('Holo')                                                # a holographic box with a hood, a sand cap, a cyan bar
    g.add(m, lg.box(17.5, 28.5, v0, v0 + 1.3, 3.0, bevel=0.25, mat=lg.BLACK))
    hood = lg.slab([(17.5, v0 + 1.2), (28.5, v0 + 1.2), (28.5, v0 + 5.4), (27.0, v0 + 6.3), (19.0, v0 + 6.3),
                    (17.5, v0 + 5.4)], 3.4, bevel=0.35, segments=3, mat=lg.BLACK, round=0.4)
    g.add(m, lg.cut(hood, [lg.cbox(16.5, 29.5, v0 + 1.9, v0 + 5.5, -1.25, 1.25)]))
    g.add(m, lg.box(20.0, 26.0, v0 + 6.2, v0 + 6.65, 1.9, bevel=0.15, mat=SHELL))
    g.add(m, lg.box(20.4, 25.6, v0 + 6.6, v0 + 6.75, 0.5, bevel=0.0, mat=CYAN))
    g.add(m, lg.box(27.0, 27.15, v0 + 1.9, v0 + 5.5, 2.5, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(26.9, v0 + 3.7, -0.12, 0.12, 0.12, mat=RETICLE))
    g.socket(m, 'Aim', (26.9, v0 + 3.7))   # the dot

    m = S('Prism')                                               # a compact 2x prism: sand top, cyan objective ring
    body = lg.slab([(18, v0), (28, v0), (28, v0 + 4.0), (26.5, v0 + 4.6), (19.5, v0 + 4.6), (18, v0 + 4.0)], 3.0,
                   bevel=0.35, segments=3, mat=lg.BLACK, round=0.5)
    g.add(m, lg.cut(body, [lg.tube(17.0, 29.0, 1.05, v=v0 + 2.6, segments=20)]))   # the bore
    g.add(m, lg.box(27.55, 27.57, v0 + 2.525, v0 + 2.675, 0.02, bevel=0.0, mat=RETICLE))   # a fine cross
    g.add(m, lg.box(27.55, 27.57, v0 + 2.59, v0 + 2.61, 0.15, bevel=0.0, mat=RETICLE))
    for u in (17.9, 28.0):
        g.add(m, lg.tube(u, u + 0.12, 1.1, v=v0 + 2.6, segments=20, mat=lg.LENS))
    g.add(m, lg.slab([(20.0, v0 + 4.4), (26.0, v0 + 4.4), (25.4, v0 + 4.95), (20.6, v0 + 4.95)], 2.4, bevel=0.15,
                     mat=SHELL, round=0.3))
    g.add(m, hollow([(0, 1.3), (0.3, 1.3)], 27.9, v0 + 2.6, 16, CYAN, wall=0.24))
    g.add(m, hollow([(0, 1.3), (0.3, 1.3)], 17.8, v0 + 2.6, 16, BRASS, wall=0.24))
    g.add(m, lg.upright(23.0, v0 + 4.9, v0 + 5.6, 0.7, mat=lg.BLACK, segments=14))
    g.add(m, lg.upright(23.0, v0 + 5.6, v0 + 5.8, 0.55, mat=BRASS, segments=14))
    g.socket(m, 'Aim', (17.9, v0 + 2.6))   # the rear lens

    m = S('ACOG')                                                # a 4x combat optic: a cyan strip on a sand ridge
    g.add(m, lg.box(18.0, 26.0, v0, v0 + 1.4, 2.4, bevel=0.25, mat=lg.BLACK))
    g.add(m, hollow([(0, 1.35), (2.4, 1.35), (3.0, 1.15), (6.0, 1.15), (7.0, 1.6), (11.0, 1.75)], 17.0, v0 + 2.7, 20,
                    lg.BLACK))
    g.add(m, lg.box(19.5, 25.0, v0 + 3.9, v0 + 4.6, 1.0, bevel=0.15, mat=SHELL))
    g.add(m, lg.box(20.0, 24.5, v0 + 4.5, v0 + 4.7, 0.5, bevel=0.0, mat=CYAN))           # fiber-optic strip
    g.add(m, hollow([(0, 1.8), (0.3, 1.8)], 27.7, v0 + 2.7, 20, CYAN, wall=0.22))
    g.add(m, hollow([(0, 1.4), (0.35, 1.4)], 17.0, v0 + 2.7, 20, BRASS, wall=0.22))
    optics(m, 17.0, 28.0, v0 + 2.7, 1.35, 1.75, dot=False)
    g.socket(m, 'Aim', (17.2, v0 + 2.7))   # the eyepiece lens

    def scope_trim(m, u_eye, u_obj, v, r_eye, r_obj, r_tube, line):
        """The frontier-tech dress on a long scope: a brass eyepiece ring, a cyan objective ring and an ember line on a
        sand saddle along the tube's top (line = its ends)."""
        g.add(m, hollow([(0, r_eye + 0.06), (0.4, r_eye + 0.06)], u_eye, v, 24, BRASS, wall=0.2))
        g.add(m, hollow([(0, r_obj + 0.06), (0.35, r_obj + 0.06)], u_obj - 0.35, v, 24, CYAN, wall=0.22))
        g.add(m, lg.box(line[0], line[1], v + r_tube - 0.2, v + r_tube + 0.05, 0.8, bevel=0.05, mat=SHELL))
        g.add(m, lg.box(line[0] + 0.2, line[1] - 0.2, v + r_tube + 0.0, v + r_tube + 0.14, 0.3, bevel=0.0, mat=EMBER))

    m = S('Variable')                                            # a 3-9x scope on two brass rings
    g.add(m, hollow([(0, 1.8), (4.0, 1.8), (5.5, 1.25), (13.5, 1.25), (15.5, 2.0), (19.5, 2.0)], 13.5, v0 + 3.9, 24,
                    lg.BLACK))
    for u in (18.5, 27.0):   # rings round the tube, open in the middle like it
        g.add(m, hollow([(0.0, 1.45), (1.2, 1.45)], u - 0.6, v0 + 3.9, 20, BRASS, wall=0.2))
        g.add(m, lg.box(u - 0.9, u + 0.9, v0, v0 + 2.7, 2.0, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(23.0, v0 + 5.1, v0 + 6.4, 0.85, mat=lg.BLACK, segments=16))
    g.add(m, lg.upright(23.0, v0 + 6.4, v0 + 6.65, 0.66, mat=BRASS, segments=16))
    g.add(m, lg.pin(23.0, v0 + 3.9, -2.6, -1.18, 0.85, mat=lg.BLACK, segments=16))
    scope_trim(m, 13.5, 33.0, v0 + 3.9, 1.8, 2.0, 1.25, (19.4, 26.1))
    optics(m, 13.5, 33.0, v0 + 3.9, 1.8, 2.0, dot=False)
    g.socket(m, 'Aim', (13.7, v0 + 3.9))   # the eyepiece lens

    m = S('LongRange')                                           # an 8x scope with a sunshade and an offset dot
    g.add(m, hollow([(0, 1.9), (4.5, 1.9), (6.0, 1.35), (14.0, 1.35), (16.5, 2.6), (26.0, 2.6)], 11.0, v0 + 4.6, 28,
                    lg.BLACK))
    for u in (17.0, 27.5):   # rings round the tube, open in the middle like it
        g.add(m, hollow([(0.0, 1.55), (1.4, 1.55)], u - 0.7, v0 + 4.6, 20, BRASS, wall=0.2))
        g.add(m, lg.box(u - 1.0, u + 1.0, v0, v0 + 3.3, 2.2, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(22.0, v0 + 5.9, v0 + 7.6, 1.0, mat=lg.BLACK, segments=16))
    g.add(m, lg.upright(22.0, v0 + 7.6, v0 + 7.85, 0.78, mat=BRASS, segments=16))
    g.add(m, lg.pin(22.0, v0 + 4.6, -3.0, -1.28, 1.0, mat=lg.BLACK, segments=16))
    scope_trim(m, 11.0, 37.0, v0 + 4.6, 1.9, 2.6, 1.35, (17.8, 20.6))
    optics(m, 11.0, 37.0, v0 + 4.6, 1.9, 2.6, dot=False)
    g.add(m, lg.box(13.0, 15.8, v0 + 6.1, v0 + 7.3, 1.6, bevel=0.2, mat=SHELL))      # the offset red dot
    g.add(m, lg.box(15.2, 15.3, v0 + 6.4, v0 + 7.1, 1.2, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.box(13.2, 15.0, v0 + 7.25, v0 + 7.4, 0.4, bevel=0.0, mat=CYAN))
    g.socket(m, 'Aim', (11.2, v0 + 4.6))   # the eyepiece lens (the offset dot on top is a backup, not aimed)


# --- Stocks: on the butt face, backward ---

def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    def pad(m, u_back, thick=1.6, flare=0.0, mat=lg.RUBBER):
        g.add(m, lg.slab([(u_back, -8.8), (u_back - thick, -8.8 - flare), (u_back - thick, 2.0 + flare * 0.3),
                          (u_back, 2.0)], 5.2, bevel=0.5, mat=mat, round=0.6))

    m = S('Pad')                                                 # a plain rubber butt pad
    pad(m, 0.3)

    m = S('Recoil')                                              # a thick ribbed recoil pad
    thick = lg.slab([(0.3, -9.0), (-3.0, -9.5), (-3.0, 2.4), (0.3, 2.1)], 5.4, bevel=0.8, mat=lg.RUBBER, round=1.0)
    g.add(m, lg.cut(thick, [lg.cbox(-3.6, -2.5, v, v + 0.45, -3.0, 3.0) for v in (-7.5, -5.0, -2.5, 0.0)]))

    m = S('Spacers')                                             # length-of-pull spacers and a pad
    for k, mat in enumerate((SHELL, lg.POLY_GREY)):
        g.add(m, lg.slab([(0.3 - k * 1.3, -8.6), (-1.0 - k * 1.3, -8.6), (-1.0 - k * 1.3, 1.9), (0.3 - k * 1.3, 1.9)],
                         4.8, bevel=0.3, mat=mat, round=0.5))
    for v in (-6.0, -0.6):
        g.add(m, lg.pin(-1.0, v, -2.6, -2.35, 0.35, mat=BRASS))
    pad(m, -2.3)

    m = S('Cheek')                                               # a pad and a raised cheek rest on brass posts
    pad(m, 0.3)
    g.add(m, lg.slab([(0.4, 3.6), (11.0, 4.0), (11.4, 5.4), (0.8, 5.4)], 3.8, bevel=0.5, segments=3,
                     mat=lg.POLY_GREY, round=0.8))
    for u in (2.6, 8.8):
        g.add(m, lg.upright(u, 3.0, 4.0, 0.35, mat=BRASS))

    m = S('Skeleton')                                            # an open extension frame
    frame = lg.slab([(0.3, -8.4), (-7.5, -9.4), (-8.6, -8.0), (-8.6, 1.4), (-7.5, 2.4), (0.3, 2.0)], 3.6, bevel=0.6,
                    segments=3, mat=SHELL, round=1.0)
    g.add(m, lg.cut(frame, [lg.cslab([(-1.4, -6.8), (-6.8, -7.4), (-6.8, 0.2), (-1.4, 0.2)], -3, 3, round=1.0)]))
    pad(m, -8.4)

    m = S('Padded')                                              # a cushioned cheek wrap and a thick pad
    g.add(m, lg.slab([(-1.6, 0.6), (9.0, 1.6), (9.6, 4.2), (-1.6, 3.6)], 5.6, bevel=1.0, segments=3, mat=lg.POLY_OLIVE,
                     round=1.2))
    for u in (1.0, 6.5):
        g.add(m, lg.slab([(u, 1.0), (u + 0.9, 1.1), (u + 0.9, 4.2), (u, 4.0)], 5.8, bevel=0.2, mat=METAL))
        for side in (-1.0, 1.0):                                 # brass studs on the straps
            g.add(m, lg.rivet(u + 0.45, 2.6, side * 2.9, side, r=0.26, mat=BRASS))
    pad(m, 0.3, thick=2.6, flare=0.3)

    m = S('Monopod')                                             # a pad over a fold-down monopod leg, a brass collar
    pad(m, 0.3)
    g.add(m, lg.box(-1.4, 2.6, -10.4, -8.4, 2.4, bevel=0.3, mat=METAL))
    g.add(m, lg.upright(0.6, -17.6, -10.2, 0.45, mat=lg.STEEL))
    g.add(m, lg.upright(0.6, -14.0, -10.4, 0.6, mat=lg.BLACK))
    g.add(m, lg.upright(0.6, -14.4, -14.0, 0.66, mat=BRASS, segments=14))
    g.add(m, lg.upright(0.6, -18.4, -17.4, 1.0, mat=lg.RUBBER, segments=16))

    m = S('Plate')                                               # a brass butt plate with a sling loop, angled pad
    g.add(m, lg.slab([(0.3, -8.8), (-1.0, -9.2), (-1.0, 2.3), (0.3, 2.0)], 5.0, bevel=0.3, mat=BRASS, round=0.4))
    g.add(m, lg.slab([(-1.0, -9.2), (-2.8, -10.0), (-2.4, 2.6), (-1.0, 2.3)], 5.2, bevel=0.5, mat=lg.RUBBER, round=0.6))
    g.add(m, lg.pipe([(-0.4, -9.2), (-0.6, -10.9), (-2.4, -11.2), (-2.6, -9.6)], 0.28, mat=lg.STEEL))


# --- The design: names, rarities and stats (percent changes; magazines give their rounds) ---

R_COMMON, R_UNCOMMON, R_RARE, R_EPIC = 'Common', 'Uncommon', 'Rare', 'Epic'


def p(key, name, word, rarity, **stats):
    return dict(key=key, name=name, word=word, rarity=rarity, stats=stats)


PARTS = {
    'Body': [
        p('Standard', 'Standard shell', '', R_COMMON),
        p('Heritage', 'Heritage shell', 'Vintage', R_UNCOMMON, accuracy=4, damage=3),
        p('Carbon', 'Carbon shell', 'Carbon', R_UNCOMMON, handling=8, recoil=4),
        p('Salvager', 'Salvager shell', 'Scrapped', R_UNCOMMON, damage=6, accuracy=-4),
        p('Skeleton', 'Skeleton shell', 'Light', R_RARE, handling=12, recoil=6),
        p('Sleek', 'Sleek shell', 'Sleek', R_RARE, fire_rate=6, handling=6),
        p('Marksman', 'Marksman shell', 'Marksman', R_RARE, accuracy=10, recoil=-6, handling=-6),
        p('Armored', 'Armored shell', 'Armored', R_EPIC, recoil=-15, damage=5, handling=-10),
    ],
    'Barrel': [
        p('Stub', 'Stub barrel', 'Compact', R_COMMON, damage=-8, accuracy=-10, range=-25, fire_rate=8, handling=15),
        p('Carbine', 'Carbine barrel', '', R_COMMON),
        p('Fluted', 'Fluted barrel', 'Fluted', R_UNCOMMON, accuracy=8, range=10, handling=5),
        p('Heavy', 'Heavy barrel', 'Heavy', R_UNCOMMON, damage=8, accuracy=12, recoil=-10, handling=-15),
        p('Shrouded', 'Cooled shroud', 'Cooled', R_UNCOMMON, fire_rate=10, accuracy=4, handling=-5),
        p('Railed', 'Railed handguard', 'Tactical', R_RARE, accuracy=6, range=5, recoil=-8, handling=8),
        p('Piston', 'Gas piston', 'Piston', R_RARE, fire_rate=15, recoil=-5, damage=-4),
        p('Marksman', 'Marksman barrel', 'Marksman', R_EPIC, damage=12, accuracy=20, range=40, fire_rate=-10,
          handling=-20),
    ],
    'Muzzle': [
        p('Cap', 'Thread cap', '', R_COMMON),
        p('Birdcage', 'Birdcage flash hider', '', R_COMMON, accuracy=4, recoil=-3),
        p('Prong', 'Three-prong flash hider', 'Flashless', R_UNCOMMON, accuracy=6),
        p('Brake', 'Two-port brake', 'Braced', R_UNCOMMON, recoil=-15, accuracy=3),
        p('Compensator', 'Compensator', 'Steady', R_RARE, recoil=-12, accuracy=8),
        p('BoxBrake', 'Box brake', 'Thumping', R_RARE, recoil=-25, handling=-8),
        p('Suppressor', 'Suppressor', 'Silent', R_RARE, accuracy=6, range=5, damage=-3),
        p('LongSuppressor', 'Long suppressor', 'Whisper', R_EPIC, accuracy=12, range=15, damage=-5, handling=-10),
    ],
    'Magazine': [
        p('20', '20-round magazine', 'Light', R_COMMON, magazine=20, reload=-15, handling=5),
        p('30', '30-round magazine', '', R_COMMON, magazine=30),
        p('30Window', '30-round windowed', 'Ready', R_UNCOMMON, magazine=30, reload=-10),
        p('40', '40-round steel', 'Extended', R_UNCOMMON, magazine=40, reload=8),
        p('Coupled', 'Coupled 30s', 'Jungle', R_RARE, magazine=30, reload=-35),
        p('60', '60-round casket', 'Hungry', R_RARE, magazine=60, reload=20, handling=-10),
        p('Drum', '75-round drum', 'Drummer', R_EPIC, magazine=75, reload=35, handling=-15),
        p('TwinDrum', '100-round twin drum', 'Endless', R_EPIC, magazine=100, reload=50, handling=-25, accuracy=-5),
        p('Ember30', '30-round ember rounds', 'Smoldering', R_UNCOMMON, magazine=30, damage=-4),
        p('Saint30', "30-round saint's rounds", 'Saintlit', R_UNCOMMON, magazine=30, damage=-4),
        p('Salt30', '30-round salt rounds', 'Salted', R_UNCOMMON, magazine=30, damage=-4),
    ],
    'Sight': [
        p('Iron', 'Flip-up irons', '', R_COMMON, zoom=1.0, handling=10),
        p('RedDot', 'Red dot', 'Dotted', R_COMMON, zoom=1.25, accuracy=5),
        p('Reflex', 'Reflex sight', 'Reflex', R_UNCOMMON, zoom=1.25, accuracy=6, handling=5),
        p('Holo', 'Holographic sight', 'Holo', R_UNCOMMON, zoom=1.5, accuracy=10),
        p('Prism', '2x prism', 'Prism', R_RARE, zoom=2.0, accuracy=12, handling=-5),
        p('ACOG', '4x combat optic', 'Scoped', R_RARE, zoom=4.0, accuracy=18, range=10, handling=-10),
        p('Variable', '3-9x scope', 'Hunter', R_EPIC, zoom=6.0, accuracy=22, range=20, handling=-15),
        p('LongRange', '8x long-range scope', 'Longshot', R_EPIC, zoom=8.0, accuracy=28, range=30, handling=-25,
          fire_rate=-5),
    ],
    'Stock': [
        p('Pad', 'Butt pad', '', R_COMMON),
        p('Recoil', 'Recoil pad', 'Cushioned', R_COMMON, recoil=-10),
        p('Spacers', 'Spacer stock', 'Measured', R_UNCOMMON, accuracy=6, handling=-5),
        p('Cheek', 'Cheek riser', 'Steady', R_UNCOMMON, accuracy=8, recoil=-4),
        p('Skeleton', 'Skeleton stock', 'Nimble', R_UNCOMMON, handling=12, recoil=5),
        p('Padded', 'Padded stock', 'Padded', R_RARE, recoil=-15, handling=-5),
        p('Monopod', 'Monopod stock', 'Anchored', R_RARE, accuracy=12, handling=-12),
        p('Plate', 'Butt plate', 'Slung', R_RARE, handling=8, recoil=-6),
    ],
}
DEFAULT = {'Body': 'Standard', 'Barrel': 'Carbine', 'Muzzle': 'Birdcage', 'Magazine': '30', 'Sight': 'RedDot', 'Stock': 'Pad'}


bodies()
barrels()
muzzles()
magazines()
element_magazines()
sights()
stocks()
tripo_override()
lg.fill_uvs(g)   # glows and glass get UVs, so Unreal builds their tangents quietly
built = g.build(origins)
tripo_lods(built)


def socket_at(obj, name):
    s = next(c for c in obj.children if c.name.startswith('SOCKET_' + name))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the body at `at` and the chosen parts (slot -> key) on its sockets; returns the objects."""
    body_obj = built['Body_' + choice.get('Body', 'Standard')]
    body_obj.location = Vector(at)
    shown = [body_obj]
    barrel = built['Barrel_' + choice['Barrel']]
    barrel.location = socket_at(body_obj, 'Barrel')
    shown.append(barrel)
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    shown.append(muzzle)
    for slot in ('Magazine', 'Sight', 'Stock'):
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body_obj, slot)
        shown.append(obj)
    bpy.context.view_layer.update()   # so a preview frames the parts where they are now
    return shown


# In the scene: the default rifle assembled, every other part laid out in rows behind it by slot.
assemble(DEFAULT)
for row, slot in enumerate(('Body', 'Barrel', 'Muzzle', 'Magazine', 'Sight', 'Stock')):
    for k, part in enumerate(PARTS[slot]):
        obj = built[f'{slot}_{part["key"]}']
        if part['key'] != DEFAULT[slot]:
            obj.location = Vector((0.0, -k * 0.3, (row + 1) * 0.4))

if lt.want_preview():
    out = lambda name: lt.preview_path('Weapons', name)
    lt.preview(assemble(DEFAULT), out('Bullpup'), view=(-1.0, -0.35, 0.3), ground=False, lens=60.0, fit=0.95)
    lt.preview(assemble(DEFAULT), out('Bullpup_Side'), view=(-1.0, -0.02, 0.05), ground=False, lens=80.0, fit=0.9)
    lt.preview(assemble(DEFAULT), out('Bullpup_FP'), view=(0.42, 1.0, 0.32), ground=False, lens=45.0, fit=0.8)
    assemble(DEFAULT)

if lg.want_elements():   # -- --elements: Bullpup_Elements.png, the plain 30 and its element magazines
    lg.element_preview('Bullpup_Elements', assemble, DEFAULT, 'Magazine', ['30', 'Ember30', 'Saint30', 'Salt30'], built)
