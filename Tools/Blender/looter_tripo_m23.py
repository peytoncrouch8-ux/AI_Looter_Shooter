"""Mains 2-3's Tripo models (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md "Mains 2-3", lane TRIPO_M23; prompts in
Art/Tripo/specs/m23.json, GLBs in Art/Tripo/<Name>/) made into game models: Art/Models/Tripo/WardensMains23.py builds them
with this module, Tools/Blender/looter_export.py exports them, Looter.ImportModels brings them in as /Game/Art/Tripo/SM_<Name>
(the paths the game's code and build scripts already try first; the stand-ins show until then).

Each model starts as looter_tripo_wardens.build makes the Landfall props (Tripo's mesh cleaned, turned to face -Y, scaled,
its colour map graded and sharpened, roughness floored, MI_Tripo<Name> on M_World, no Nanite, LODs 50/25/12%), then a fit of
its own puts what the game's code reads where the code reads it (MAIN23.md; the C++ constants are quoted at each fit): the
ferry's length, beam and gunwale, the cart's axle and tank, the board's face and height, the banner's flat cloth. Hulls are
k-DOPs (looter_tripo_buildings.kdop) round the parts the player or bullets meet; held-free props only.

Look (the user's rule: Tripo matches the game): the prompts name the town palette; looter_tripo_wardens.write_maps grades
every map; GRADE_TO pulls a model's colour statistics onto a game texture set's (the buildings lane's grade) where Tripo's
paint came out off-palette.

    import looter_tripo_m23 as m23
    for name in m23.names():
        m23.build(name)
"""
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_model as lm
import looter_tripo_buildings as tb
import looter_tripo_wardens as tw

# Base specs for looter_tripo_wardens.build (boxes [] = no hulls from it: each fit makes its own). Unreal: X front, Y right;
# Blender: front -Y, right -X (ue_to_blender).
BASE = {
    # Ferry: GLB length along +Y, bow (pointed) +Y, lantern pole on the transom (-Y). Turned +90: bow to Blender -X (UE +Y,
    # ASextonsFerry's bow), the transom and pole to UE -Y beside Sexton's seat.
    'SextonsFerry': dict(category='Tripo', kind='prop', master='World', maps=2048, turn=90.0, size=('width', 9.0),
                         pivot='center', boxes=[]),
    # Shroud: GLB length along X. Turned so it lies along Blender Y (UE X), head end +X (fit_shroud turns it if not).
    'ShroudedDead': dict(category='Tripo', kind='prop', master='World', maps=1024, turn=90.0, size=('length', 1.8),
                         pivot='bottom', boxes=[]),
    # Cart: GLB tank along X, valve hand-wheel at -X. Turned +90: the valve end to Blender -Y (UE +X, the handles' way,
    # where ACompanyHoseCart's ValveAt is).
    'CompanyHoseCart': dict(category='Tripo', kind='prop', master='World', maps=2048, turn=90.0, size=('length', 2.2),
                            pivot='bottom', boxes=[]),
    # Sandbags: GLB wall along Y (12 rows, as tall as long: fit_sandbags cuts it to waist height). Its length stays on
    # Blender Y = UE X (build_area_main3 turns the actor's X along the gate's side).
    'CompanySandbags': dict(category='Tripo', kind='prop', master='World', maps=1024, turn=0.0, size=('length', 2.0),
                            pivot='bottom', boxes=[]),
    # Board: GLB face toward +X, width along Y. Turned -90: the face to Blender -Y (UE +X, where ACompanySign draws its
    # words), width along Blender X (UE Y).
    'CompanyCheckpointSign': dict(category='Tripo', kind='prop', master='World', maps=1024, turn=-90.0,
                                  size=('width', 3.0), pivot='bottom', boxes=[]),
    # Banner: GLB length along Y, its faces +-X. Turned +90: length along Blender X (UE Y), faces +-Blender Y (UE +-X).
    'CompanyBanner': dict(category='Tripo', kind='prop', master='World', maps=2048, turn=90.0, size=('width', 12.0),
                          pivot='center', boxes=[]),
}

ORDER = ['SextonsFerry', 'ShroudedDead', 'CompanyHoseCart', 'CompanySandbags', 'CompanyCheckpointSign', 'CompanyBanner']


def names():
    only = [n for n in os.environ.get('LOOTER_TRIPO_ONLY', '').split(',') if n]
    return [n for n in ORDER if n in BASE and (not only or n in only)]


def log(message):
    print(f'LOOTER: {message}', flush=True)


# --- Mesh helpers (the model object sits at the origin, unrotated: its mesh space is the model's) ---

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


def piecewise(values, knots):
    """values mapped through the piecewise-linear knots [(from, to), ...] (sorted by from; outside: the end slopes)."""
    xs = np.array([k[0] for k in knots])
    ys = np.array([k[1] for k in knots])
    out = np.interp(values, xs, ys)
    lo_slope = (ys[1] - ys[0]) / max(xs[1] - xs[0], 1e-9)
    hi_slope = (ys[-1] - ys[-2]) / max(xs[-1] - xs[-2], 1e-9)
    out = np.where(values < xs[0], ys[0] + (values - xs[0]) * lo_slope, out)
    return np.where(values > xs[-1], ys[-1] + (values - xs[-1]) * hi_slope, out)


def prune(obj):
    """Removes vertices left without faces (after a cut)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.to_mesh(obj.data)
    bm.free()


def cut_above(obj, z):
    """Cuts the mesh at height z and drops what's over it (the cut left open)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, z), plane_no=(0, 0, 1),
                           clear_outer=True)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def hull(obj, points):
    """A convex hull (k-DOP) round points (N, 3) in the model's space; skipped when there are too few."""
    if len(points) < 8:
        return None
    return tb._new_hull(obj, tb.kdop([Vector(p) for p in points]))


def box_hull(obj, lo, hi):
    return tb.add_box_hull(obj, Vector(lo), Vector(hi))


def socket(obj, report, name, p_blender, yaw=0.0):
    lm.socket(obj, name, tuple(float(v) for v in p_blender), (0.0, 0.0, -yaw))
    report.setdefault('sockets', {})[name] = tuple(round(v, 1) for v in tw.blender_to_ue(p_blender))


def glass_faces(obj, region, test=None):
    """Faces in region (Blender box) whose colour reads as lantern glass (looter_tripo_wardens._glass by default)."""
    me = obj.data
    image = None
    for mat in me.materials:
        if mat and mat.use_nodes:
            image = next((n.image for n in mat.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image
                          and n.image.name.upper().find('_BC') >= 0), None) or image
    if image is None:
        return []
    return tw._faces_by_colour(obj, image, tw._faces_in(centres(obj), region), test or tw._glass)


def to_glow(obj, faces):
    if faces:
        glow = lm.material(tw.GLOW_NAME, 0xffc56e, Glow=5.0, Variation=0.04)
        import looter_textures as lt
        lt.assign(obj, glow, faces)


# --- The fits ---

def fit_ferry(obj, report):
    """ASextonsFerry (SextonsFerry.cpp): FerryLength 900 along +Y (bow +Y), FerryBeam 160 across X, pivot at the hull's
    middle on its gunwale line (the stand-in's sides top at +12.5, floor -80, so keel about -85); SeatAt (0, -320, 8),
    BowEye/SternEye (0, +-440, 10), LanternAt (0, 530, 135). Tripo's hull came out 2.4:1 (stretched to 5.6:1 along the
    planks, which hides it) with its lantern pole on the transom: the pole and lantern keep their proportions (the stern's
    end is squeezed back along the length), its glass goes on LanternGlow. Code edits for Main: TRIPO_M23.md."""
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    beam = hi[1] - lo[1]
    # The gunwale: the top of the sides amidships (vertices within 15% of the beam's edges, middle third of the length).
    mid = np.abs(V[:, 0]) < (hi[0] - lo[0]) / 6.0
    side = np.abs(V[:, 1]) > 0.35 * beam
    gunwale = np.percentile(V[mid & side][:, 2], 97)
    keel = lo[2]
    depth = gunwale - keel
    # Squash beam to 1.6 m and the depth to 0.9 m (the stand-in's), round the gunwale.
    ky, kz = 1.6 / beam, 0.9 / depth
    V[:, 1] *= ky
    V[:, 2] = (V[:, 2] - gunwale) * kz
    # The pole and its lantern above the transom (stern +X here): what stands over the gunwale by more than 15 cm in the
    # stern's last 15%. Squeezed along X by the same squash as the beam, so its round pole stays round.
    stern = V[:, 0] > hi[0] - 0.15 * (hi[0] - lo[0])
    pole = stern & (V[:, 2] > 0.15) & (V[:, 2] < np.percentile(V[stern][:, 2], 99) - 0.25)
    report['pole x'] = (round(float(V[pole][:, 0].min()), 3), round(float(V[pole][:, 0].max()), 3)) if pole.any() else None
    if pole.any():
        # Everything aft of a squeezed by the beam's squash (about a); the 9 m length is restored below by stretching
        # the whole, so the hull ahead takes the length back.
        a = V[pole][:, 0].min() - 0.06
        V[:, 0] = piecewise(V[:, 0], [(lo[0], lo[0]), (a, a), (hi[0], a + (hi[0] - a) * ky)])
    # Recentre on the hull (bow tip to transom) and the gunwale.
    V2 = V
    hull_part = V2[:, 2] < 0.05
    xl, xh = V2[hull_part][:, 0].min(), V2[hull_part][:, 0].max()
    V2[:, 0] -= (xl + xh) * 0.5
    V2[:, 0] *= 9.0 / (xh - xl)
    V2[:, 1] -= (V2[:, 1].min() + V2[:, 1].max()) * 0.5
    set_verts(obj, V2)
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    report['ferry'] = dict(gunwale_was=round(float(gunwale), 3), squash=(round(ky, 3), round(kz, 3)),
                           hull=(round(float(xl), 2), round(float(xh), 2)))

    # The lantern's glass (pale panes high on the pole) on LanternGlow; its middle is the Lantern socket.
    glass = glass_faces(obj, ((hi[0] - 1.2, -0.4, 0.15), (hi[0] + 0.1, 0.4, hi[2])))
    to_glow(obj, glass)
    report['glass faces'] = len(glass)
    C = centres(obj)
    if glass:
        socket(obj, report, 'Lantern', C[glass].mean(axis=0))
    # Sexton's seat: the last thwart before the pole's little stern deck (2.8-3.65 m aft of the middle; SeatAt wants
    # 3.2), its top: the highest up-facing faces there inside the sides.
    N = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('normal', N)
    N = N.reshape(-1, 3)
    seat = (C[:, 0] > 2.8) & (C[:, 0] < 3.65) & (np.abs(C[:, 1]) < 0.3) & (N[:, 2] > 0.9) & (C[:, 2] < 0.2) & (C[:, 2] > -0.6)
    if seat.any():
        # The highest band of up-facing faces: a thwart's top (the floor boards lie far lower).
        top = C[seat][:, 2].max()
        band = seat & (C[:, 2] > top - 0.03)
        socket(obj, report, 'Seat', (C[band][:, 0].mean(), 0.0, top), 180.0)
    # Chain eyes: the hull's two ends at the gunwale.
    for name, end in (('BowEye', lo[0]), ('SternEye', xh - (xl + xh) * 0.5)):
        near = (np.abs(V[:, 0] - end) < 0.25) & (V[:, 2] < 0.3)
        socket(obj, report, name, (V[near][:, 0].mean(), 0.0, V[near][:, 2].max()))
    # Hulls: the hull in five convex slices along its length (solid to the gunwale: nobody stands in it), the pole.
    below = V[:, 2] < 0.08
    edges = np.linspace(lo[0], hi[0], 6)
    for a, b in zip(edges, edges[1:]):
        hull(obj, V[below & (V[:, 0] >= a - 0.02) & (V[:, 0] <= b + 0.02)])
    upper = V[:, 2] > 0.1
    if upper.any():
        hull(obj, V[upper & (V[:, 0] > hi[0] - 1.0)])


def fit_shroud(obj, report):
    """build_area_main2.place_shrouds: lies along the model's longer axis (UE X here), scaled to SHROUD_LENGTH 175 by its
    bounds, set on the bier by its lowest point. One hull round it (bullets; the crew's hose aims at its top)."""
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    # Head end +X (Blender -Y), as SM_Cocoon_Lying: the head is the end whose last 20% stands taller.
    span = hi[1] - lo[1]
    head_low = V[V[:, 1] < lo[1] + 0.2 * span][:, 2].max()
    head_high = V[V[:, 1] > hi[1] - 0.2 * span][:, 2].max()
    if head_high > head_low:
        V[:, 0] *= -1.0
        V[:, 1] *= -1.0
        set_verts(obj, V)
        report['turned'] = 180
    hull(obj, verts(obj))


def fit_cart(obj, report):
    """ACompanyHoseCart (CompanyHoseCart.cpp): X toward the handles, Z up from the ground, pivot on the ground under the
    middle (Tripo made it four-wheeled: no single axle). The valve's hold spot stays the stand-in's (TRIPO_MODELS: about
    0.4 m forward, 1.8 m up): the cart is scaled evenly so its hand-wheel's hub stands 1.8 m up. TankAt/ReelAt and the
    TankHit capsule are measured here for Main (the Tripo tank sits lower than the stand-in's)."""
    V = verts(obj)
    top = V[:, 2].max()
    rim = V[:, 2] > top - 0.03
    hub = np.array([V[rim][:, 0].mean(), V[rim][:, 1].mean(), top - 0.015])
    s = 1.8 / hub[2]
    V *= s
    hub *= s
    set_verts(obj, V)
    socket(obj, report, 'Valve', hub)
    # The tank, by rays (Tripo's big flat faces leave few vertices on it): its top straight down behind the valve's stem,
    # its side from the left at heights under that (widest = its middle), its ends along its axis.
    bvh = tb.tree(obj)

    def ray(origin, direction):
        hit = bvh.ray_cast(Vector(origin), Vector(direction).normalized(), 10.0)
        return hit[0]

    y_mid = hub[1] + 0.35
    tank_top = ray((0.0, y_mid, hub[2] - 0.3), (0, 0, -1)).z
    best = (9.0, tank_top - 0.4)
    for z in np.arange(tank_top - 0.05, tank_top - 0.65, -0.02):
        hit = ray((-3.0, y_mid, z), (1, 0, 0))
        if hit is not None and hit.x < best[0]:
            best = (hit.x, z)
    radius, centre_z = -best[0], best[1]
    y0 = ray((0.0, -3.0, centre_z), (0, 1, 0)).y
    y1 = ray((0.0, 3.0, centre_z), (0, -1, 0)).y
    socket(obj, report, 'Tank', ((0.0), (y0 + y1) * 0.5, centre_z))
    # The hose leaves from the tank's back end (+Y here, UE -X), two thirds up it.
    socket(obj, report, 'Reel', (0.0, y1, centre_z + radius * 0.33))
    report['tank'] = dict(radius_cm=round(radius * 100.0, 1), length_cm=round((y1 - y0) * 100.0, 1),
                          top_cm=round(tank_top * 100.0, 1))
    # One hull round the cart up to the tank's top (wheels, bed, tank): cover for the hose-man; the valve stem is open.
    hull(obj, V[V[:, 2] < tank_top + 0.03])


def fit_sandbags(obj, report):
    """build_area_main3.place_checkpoint: a 2 m wall along the actor's X, waist high. Tripo made one fused mesh of 12 rows,
    as tall as long: it is cut at the seam over the sixth row and under the slumped top row (each seam where the fewest
    vertices are, near 6/12 and 11/12 of its height), the five rows between dropped and the top row set down on the
    sixth (the cuts' open edges hide under it). One hull (solid cover; tagged Obstacle by the build)."""
    import bmesh
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    row = (hi[2] - lo[2]) / 12.0

    def seam(near):
        zs = np.linspace(near - 0.4 * row, near + 0.4 * row, 41)
        counts = [np.count_nonzero(np.abs(V[:, 2] - z) < 0.06 * row) for z in zs]
        return float(zs[int(np.argmin(counts))])

    z_low, z_top = seam(lo[2] + 6.0 * row), seam(hi[2] - 1.0 * row)
    keep = bmesh.new()
    keep.from_mesh(obj.data)
    bmesh.ops.bisect_plane(keep, geom=keep.verts[:] + keep.edges[:] + keep.faces[:], plane_co=(0, 0, z_low),
                           plane_no=(0, 0, 1), clear_outer=True)
    top = bmesh.new()
    top.from_mesh(obj.data)
    bmesh.ops.bisect_plane(top, geom=top.verts[:] + top.edges[:] + top.faces[:], plane_co=(0, 0, z_top),
                           plane_no=(0, 0, 1), clear_inner=True)
    # Set down so the top row's underside rests a little into the sixth row (bags squash into each other).
    drop = z_top - z_low + 0.12 * row
    for v in top.verts:
        v.co.z -= drop
    temp = bpy.data.meshes.new('_TopRow')
    top.to_mesh(temp)
    top.free()
    keep.from_mesh(temp)
    bpy.data.meshes.remove(temp)
    keep.to_mesh(obj.data)
    keep.free()
    obj.data.update()
    V = verts(obj)
    V[:, 2] -= V[:, 2].min()
    set_verts(obj, V)
    report['seams z'] = (round(z_low - lo[2], 3), round(z_top - lo[2], 3))
    hull(obj, V)


def fit_sign(obj, report):
    """ACompanySign (CompanySign.cpp) with the model: no posts or scaling of its own, the words drawn at X +4 cm (BoardThick
    0.5 + WordsOff 0.25), centred at BoardLift + BoardSize.Y/2; build_area_main3 gives the board SIGN_SIZE 300 x 130 and
    lift 80. So: the board 3 m wide, from 0.8 to 2.1 m up, its painted face flattened onto the plane UE X = +3 cm (Tripo's
    face is lumpy: letters would sink into it), the posts stretched to the ground."""
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    W = hi[0] - lo[0]
    zs = np.linspace(lo[2], hi[2], 160)
    width = []
    for a, b in zip(zs, zs[1:]):
        band = V[(V[:, 2] >= a) & (V[:, 2] < b)]
        width.append(band[:, 0].max() - band[:, 0].min() if len(band) else 0.0)
    rows = np.nonzero(np.array(width) > 0.8 * W)[0]
    zb0, zb1 = zs[rows.min()], zs[rows.max() + 1]
    k = 1.3 / (zb1 - zb0)
    V[:, 2] = piecewise(V[:, 2], [(lo[2], 0.0), (zb0, 0.8), (zb1, 2.1), (hi[2], 2.1 + (hi[2] - zb1) * k)])
    # The face, by rays at a grid over the board between the posts: its front (median of the hits from the front) and its
    # back (from behind); every vertex of the face in front of the slab's middle goes onto the front plane (Tripo's
    # dents ran 5+ cm deep and showed as black pits).
    set_verts(obj, V)
    bvh = tb.tree(obj)
    cx = (lo[0] + hi[0]) * 0.5
    fronts, backs = [], []
    for x in np.linspace(cx - 0.12 * W, cx + 0.12 * W, 5):
        for z in np.linspace(1.0, 1.9, 5):
            f = bvh.ray_cast(Vector((x, -5.0, z)), Vector((0, 1, 0)), 10.0)[0]
            b = bvh.ray_cast(Vector((x, 5.0, z)), Vector((0, -1, 0)), 10.0)[0]
            if f is not None and b is not None:
                fronts.append(f.y)
                backs.append(b.y)
    plane, back = float(np.median(fronts)), float(np.median(backs))
    inner = (np.abs(V[:, 0] - cx) < 0.47 * W) & (V[:, 2] > 0.86) & (V[:, 2] < 2.04)
    flat = inner & (V[:, 1] < (plane + back) * 0.5)
    report['board depth cm'] = round((back - plane) * 100.0, 1)
    V[flat, 1] = plane
    V[:, 1] += -0.03 - plane
    V[:, 0] -= (lo[0] + hi[0]) * 0.5
    set_verts(obj, V)
    # The posts' tops over the board: Tripo's repaint made them grey stone knobs. Cut flush with the frame's top (out of
    # sight from the ground: 2.1 m up).
    cut_above(obj, 2.12)
    V = verts(obj)
    # The face in the game's black stencil paint (PaintBlack: PaintWorn tinted, as Graves.py, Lootables.py, Chests.py make
    # it), mapped at world scale: Tripo's repaint, steered by our style picture, laid pale parquet boards there instead
    # of black (2026-11-01), and the words are drawn white.
    import looter_textures as lt
    C = centres(obj)
    N = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('normal', N)
    N = N.reshape(-1, 3)
    face = np.nonzero((np.abs(C[:, 0]) < 0.47 * W) & (C[:, 2] > 0.86) & (C[:, 2] < 2.04) & (C[:, 1] < -0.025)
                      & (N[:, 1] < -0.6))[0].tolist()
    paint = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)
    lt.box_uv(obj, 'PaintWorn', faces=face)
    lt.assign(obj, paint, face)
    report['board'] = dict(rows_z=(round(float(zb0), 3), round(float(zb1), 3)), flattened=int(flat.sum()),
                           black_faces=len(face))
    socket(obj, report, 'Words', (0.0, -0.04, 1.45))
    board = (V[:, 2] > 0.75) & (V[:, 2] < 2.15)
    hull(obj, V[board])
    for sign in (-1.0, 1.0):
        hull(obj, V[(V[:, 2] <= 0.8) & (V[:, 0] * sign > 0.0)])


def fit_banner(obj, report):
    """ACompanyBanner (CompanyBanner.cpp): the cloth centred on the pivot, ClothLength 1200 along Y, ClothHeight 140, its
    words drawn flat on both faces 2.6 cm off the middle plane (ClothThick 4): so the cloth is pressed to 3 cm deep (Tripo
    hung it in deep folds), stretched to 12 x 1.4 m with a slight sag, its top corners at (+-600, 70) where the code ties
    its ropes. Tripo's own rope ties are cut (the code draws the ropes). No collision (the code's)."""
    rope = glass_faces(obj, ((-99, -99, -99), (99, 99, 99)),
                       test=lambda rgb: (rgb[..., 0] - rgb[..., 2] > 0.16) & (rgb[..., 0] > 0.25))
    import looter_tripo as tripo
    tripo._delete_faces(obj, rope)
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.to_mesh(obj.data)
    bm.free()
    report['rope faces cut'] = len(rope)
    V = verts(obj)
    lo, hi = V.min(0), V.max(0)
    V[:, 0] = (V[:, 0] - (lo[0] + hi[0]) * 0.5) * (12.0 / (hi[0] - lo[0]))
    V[:, 1] = (V[:, 1] - np.median(V[:, 1])) * (0.03 / max(hi[1] - lo[1], 1e-6))
    V[:, 2] = (V[:, 2] - (lo[2] + hi[2]) * 0.5) * (1.4 / (hi[2] - lo[2]))
    V[:, 2] -= 0.1 * (1.0 - (V[:, 0] / 6.0) ** 2)
    ends = np.abs(V[:, 0]) > 5.7
    V[:, 2] += 0.7 - V[ends][:, 2].max()
    set_verts(obj, V)
    for name, side in (('RopeLeft', -1.0), ('RopeRight', 1.0)):
        # UE Y -600 is Blender X +6.
        socket(obj, report, name, (-side * 6.0, 0.0, 0.7))


FITS = {'SextonsFerry': fit_ferry, 'ShroudedDead': fit_shroud, 'CompanyHoseCart': fit_cart,
        'CompanySandbags': fit_sandbags, 'CompanyCheckpointSign': fit_sign, 'CompanyBanner': fit_banner}


def build(name):
    tw.SPECS[name] = BASE[name]
    obj, report = tw.build(name)
    FITS[name](obj, report)
    if not [c for c in obj.children if c.name.startswith('UCX_')]:
        obj['Collision'] = 'None'
    V = verts(obj)
    report['size m'] = (V.max(0) - V.min(0)).round(3).tolist()
    report['tris'] = len(obj.data.polygons)
    report['hulls'] = len([c for c in obj.children if c.name.startswith('UCX_')])
    tw.describe(name, report)
    return obj, report
