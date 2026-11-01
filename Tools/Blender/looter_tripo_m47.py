"""Mains 4-7's Tripo models (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md "Mains 4-5" M45-1 and "Mains 6-7" M67-3, lane
TRIPO_M47; prompts in Art/Tripo/specs/m47.json, GLBs in Art/Tripo/<Name>/) made into game models: Art/Models/Tripo/
WardensMains47.py builds them with this module, Tools/Blender/looter_export.py exports them, Looter.ImportModels brings them
in as /Game/Art/Tripo/SM_<Name> (the paths the game's code and build scripts already try first; the stand-ins show until
then).

- SM_ReliquaryWhole: Saint Ada's Reliquary, whole (AChapelReliquary, set by build_area_chapel.RELIQUARY_MESHES), on the
  chapel's SOCKET_Reliquary: the apse plinth's 1.56 x 1.02 m top, 1.56 across the nave. So the chest's length runs across
  (UE Y), its front (+X) faces the nave. Sockets Ember (the lid recess's floor, +Z up; the code lays her ember 3 cm over
  it) and Interact (the front's diamond plaque).
- SM_SoulCoal: the barge's soul coal (ASoulCoalLump::ModelPath, ASoulCoalField, build_area_main7.coal_mesh, the barge's
  coal rain BargeLooks::TripoSoulCoal). All of them size it by its bounds, so only its shape matters; it lies with its
  veined face up. Its cyan veins are their own emissive slot (SoulCoalVein: no translucency), cut out of Tripo's paint
  per face after the faces along the veins are split finer.

Each starts as looter_tripo_wardens.build makes a prop (Tripo's mesh cleaned, turned, scaled, its colour map graded and
sharpened, roughness floored, MI_Tripo<Name> on M_World, no Nanite, LODs), then its fit here.

    import looter_tripo_m47 as m47
    for name in m47.names():
        m47.build(name)
"""
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector

import looter_model as lm
import looter_tripo as tripo
import looter_tripo_buildings as tb
import looter_tripo_wardens as tw

BASE = {
    # GLB: length along X (0.71), depth along Y (0.41), height 0.5, the diamond plaque on -Y: already Blender's front.
    'ReliquaryWhole': dict(category='Tripo', kind='prop', master='World', maps=2048, turn=0.0, size=('width', 1.2),
                           pivot='bottom', boxes=[]),
    # GLB: a thick disc 0.3 x 0.3, 0.22 through along X, its veins on the +X face: tipped so that face looks up.
    'SoulCoal': dict(category='Tripo', kind='prop', master='World', maps=1024, tilt=('Y', -90.0), size=('max', 0.30),
                     pivot='bottom', boxes=[]),
}

ORDER = ['ReliquaryWhole', 'SoulCoal']

# The Reliquary's box (m): TRIPO_MODELS M45-1 asks 1.2 x 0.75, 0.6 high for the chest; Tripo's is a little taller with
# its base and lid (uniformly scaled it would stand 0.85): pressed to 0.72 (the stand-in's chest and lid: 0.66), its
# depth to 0.74 (the stand-in's 74), so it sits on the plinth's 1.02 m depth with 14 cm to spare each side.
RELIQUARY = dict(length=1.2, depth=0.74, height=0.72)
RECESS_DEPTH = 0.04

# The soul coal's veins: the game's VeinCyan (SoulCoal.cpp, linear 0.45, 0.9, 1.0) as an emissive surface, as the
# lantern glass is (LanternGlow). Bright enough to read across the valley at dusk, small enough not to bloom into a ball.
VEIN_NAME = 'SoulCoalVein'
VEIN_COLOR = 0x52dcff
VEIN_GLOW = 4.0
# LODs for the coal (a 16-42 cm lump: past a few metres it's a few pixels; ~120 of them over the Rest).
COAL_LODS, COAL_LOD_SCREENS = '40,15,6', '0.4,0.18,0.07'
# The vein splits (fit_coal): one per share. Measured on Tripo's 3,691 triangles (vein faces): none 3,691 (267, blotchy);
# (0.5,) 4,821 (951, follows the cracks: kept); (0.2,) 5,521 (1,053); (0.2, 0.5) 9,503 (3,895). LOOTER_COAL_SPLITS tries
# another ('' none, '0.2,0.5').
COAL_SPLITS = (0.5,)


def names():
    only = [n for n in os.environ.get('LOOTER_TRIPO_ONLY', '').split(',') if n]
    return [n for n in ORDER if not only or n in only]


def log(message):
    print(f'LOOTER: {message}', flush=True)


def verts(obj):
    V = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get('co', V)
    return V.reshape(-1, 3)


def set_verts(obj, V):
    obj.data.vertices.foreach_set('co', np.ascontiguousarray(V, dtype=np.float64).ravel())
    obj.data.update()


def centres(obj):
    C = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('center', C)
    return C.reshape(-1, 3)


def normals(obj):
    N = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('normal', N)
    return N.reshape(-1, 3)


def socket(obj, report, name, p_blender, yaw=0.0):
    lm.socket(obj, name, tuple(float(v) for v in p_blender), (0.0, 0.0, -yaw))
    report.setdefault('sockets', {})[name] = tuple(round(v, 1) for v in tw.blender_to_ue(p_blender))


def hull(obj, points):
    if len(points) < 8:
        return None
    return tb._new_hull(obj, tb.kdop([Vector(p) for p in points]))


# --- The Reliquary ---

def _gold(rgb):
    """Tripo's gold: bright, yellow (red and green well over blue)."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (r > 0.42) & (g > 0.32) & (b < 0.62 * g) & (r - b > 0.2)


def fit_reliquary(obj, report, maps):
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    k = np.array([RELIQUARY['length'] / (hi[0] - lo[0]), RELIQUARY['depth'] / (hi[1] - lo[1]),
                  RELIQUARY['height'] / (hi[2] - lo[2])])
    centre = np.array([(lo[0] + hi[0]) * 0.5, (lo[1] + hi[1]) * 0.5, lo[2]])
    V = (V - centre) * k
    set_verts(obj, V)
    report['stretch'] = k.round(3).tolist()
    lo, hi = V.min(0), V.max(0)
    bvh = tb.tree(obj)

    # The lid's recess: rays straight down over the lid's middle; the lid's top is the commonest high hit, the recess the
    # hits more than 2 cm under it, its middle their centroid (refined to the biggest pit: the lid's carved slots are thin).
    xs = np.linspace(lo[0] * 0.8, hi[0] * 0.8, 61)
    ys = np.linspace(lo[1] * 0.8, hi[1] * 0.8, 41)
    hits = []
    for x in xs:
        for y in ys:
            h = bvh.ray_cast(Vector((x, y, hi[2] + 1.0)), Vector((0, 0, -1)), 5.0)[0]
            if h is not None:
                hits.append((x, y, h.z))
    hits = np.array(hits)
    lid_top = float(np.percentile(hits[:, 2], 75))
    pit = hits[hits[:, 2] < lid_top - 0.02]
    middle = pit[:, :2].mean(axis=0)
    for _ in range(3):
        near = pit[np.hypot(pit[:, 0] - middle[0], pit[:, 1] - middle[1]) < 0.22]
        middle = near[:, :2].mean(axis=0)
    floor = float(np.median(near[:, 2]))
    radius = float(np.sqrt(len(near) * (xs[1] - xs[0]) * (ys[1] - ys[0]) / np.pi))
    was = round((lid_top - floor) * 100, 1)
    # Tripo's recess is a 10 cm deep bowl: her ember (an 11 cm glow 3 cm over the socket while she gutters) would hide
    # under its rim from the nave (eyes ~0.3 m over the lid, metres away). Its floor and inner wall are raised to
    # RECESS_DEPTH under the lid's top (the "shallow round recess" of M45-1), so the ember shows over the gold ring.
    V = verts(obj)
    inside = (np.hypot(V[:, 0] - middle[0], V[:, 1] - middle[1]) < radius + 0.015) & (V[:, 2] < lid_top - 0.005)
    V[inside, 2] = lid_top - (lid_top - V[inside, 2]) * (RECESS_DEPTH / max(lid_top - floor, 1e-3))
    set_verts(obj, V)
    floor = lid_top - RECESS_DEPTH
    socket(obj, report, 'Ember', (middle[0], middle[1], floor))
    report['recess'] = dict(lid_top_cm=round(lid_top * 100, 1), floor_cm=round(floor * 100, 1),
                            radius_cm=round(radius * 100, 1), depth_was_cm=was, raised_verts=int(inside.sum()))

    # The front's diamond plaque: the gold faces on the front (facing -Y) in its middle third; else the front's middle.
    C, N = centres(obj), normals(obj)
    front = np.nonzero((N[:, 1] < -0.6) & (np.abs(C[:, 0]) < 0.2) & (C[:, 2] > 0.15 * hi[2]) & (C[:, 2] < 0.75 * hi[2]))[0]
    plaque = tw._faces_by_colour(obj, maps['BC'], front, _gold) if maps.get('BC') is not None else []
    if len(plaque) >= 6:
        at = C[plaque].mean(axis=0)
        at[1] = C[plaque][:, 1].min()
        report['plaque faces'] = len(plaque)
    else:
        z = 0.45 * hi[2]
        h = bvh.ray_cast(Vector((0.0, -3.0, z)), Vector((0, 1, 0)), 6.0)[0]
        at = np.array([0.0, h.y if h is not None else lo[1], z])
        log('warning: ReliquaryWhole: no gold plaque found on the front; Interact at the front\'s middle')
    socket(obj, report, 'Interact', at)

    # One box round it all (it is a box, its lid 3 cm proud): the player bumps it, the Interact key's line finds it,
    # bullets stop on it.
    V = verts(obj)
    tb.add_box_hull(obj, Vector(V.min(0)), Vector(V.max(0)))


# --- The soul coal ---

def _vein(rgb):
    """Tripo's glowing cyan in the cracks: green and blue high, red well under blue."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (b > 0.42) & (g > 0.38) & (r < 0.62 * b) & (b - r > 0.18)


def _vein_faces(obj, image, share):
    return tripo.faces_where(obj, image, lambda c: bool(_vein(np.array(c))), size=2048, corners=True, share=share)


def fit_coal(obj, report, maps):
    image = maps['BC']
    # Split the faces along the veins finer, so the glowing slot follows the painted cracks, not Tripo's big triangles;
    # the UVs are carried by the split. (Collapsing the coal round the split veins back down tore its shape and stretched
    # its paint: not done.)
    # COAL_SPLITS: one split per share, of the faces with at least that share of their 4 samples on a vein.
    splits = os.environ.get('LOOTER_COAL_SPLITS')
    for share in ([float(s) for s in splits.split(',') if s] if splits is not None else COAL_SPLITS):
        near = set(_vein_faces(obj, image, share))
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bm.faces.ensure_lookup_table()
        edges = {e for i in near for e in bm.faces[i].edges}
        bmesh.ops.subdivide_edges(bm, edges=list(edges), cuts=1, use_grid_fill=True)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
    glow = _vein_faces(obj, image, 0.5)
    vein = lm.material(VEIN_NAME, VEIN_COLOR, Glow=VEIN_GLOW, Variation=0.03)
    import looter_textures as lt
    lt.assign(obj, vein, glow)
    report['vein faces'] = len(glow)
    obj['LODs'] = COAL_LODS
    obj['LODScreens'] = COAL_LOD_SCREENS
    # One hull: the Interact key's line (ASoulCoalLump blocks Visibility only); the fields and the rain turn it off.
    hull(obj, verts(obj))


FITS = {'ReliquaryWhole': fit_reliquary, 'SoulCoal': fit_coal}


def build(name):
    tw.SPECS[name] = BASE[name]
    # tw.build loads the GLB and hands its maps to write_maps; the fits want Tripo's own colour map too: kept here.
    kept = {}
    load = tripo.load

    def keep_load(path, *a, **kw):
        obj, maps = load(path, *a, **kw)
        kept.update(maps)
        return obj, maps
    tripo.load = keep_load
    try:
        obj, report = tw.build(name)
    finally:
        tripo.load = load
    FITS[name](obj, report, kept)
    V = verts(obj)
    report['size m'] = (V.max(0) - V.min(0)).round(3).tolist()
    report['tris'] = len(obj.data.polygons)
    report['hulls'] = len([c for c in obj.children if c.name.startswith('UCX_')])
    tw.describe(name, report)
    return obj, report
