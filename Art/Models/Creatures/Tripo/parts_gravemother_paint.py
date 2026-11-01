"""SK_Gravemother in the user's Tripo paint (the user, 2026-11-01: "Use Tripo's paint"): Tripo's own colours, roughness
and relief (Art/Models/Creatures/Tripo/Gravemother.glb: the brown striped abdomen, the mottled legs, the milky eyes) carried
onto her remeshed pieces' atlas, texel by texel, as the Spider's are (parts_spider_look.Look); her grave railings back on
her back. Gravemother.py runs it (LOOTER_GRAVEMOTHER_TRIPO, default on); LOOTER_GRAVEMOTHER_LOOK=0 reuses the maps on
disk. Not a model: Tools\\models.ps1 skips parts_*.py. Not one of parts_gravemother_build's cache sources either, so the
remeshed pieces (QuadriFlow, cached) stay as they are.

How a piece finds Tripo's paint: the build carried each piece's cut of Tripo (closed, the same vertices and faces) into
the rig's pose before remeshing it, and kept that carried mesh (source_V, source_F). This module cuts the same pieces
again in Tripo's own space (parts_gravemother.extract, close, split_plane: deterministic), so a source face has its twin
in Tripo's space. Every vertex of a remeshed piece goes to its nearest point on the carried mesh and from there, by the
same face and barycentrics, back onto Tripo's surface: that is the piece's template (its faces and atlas UVs, its
vertices on Tripo). Look.carry then fills the template's texels from Tripo's maps, restricted to the piece's own part of
Tripo (its labels: no leg paint on the body, no stone grey on the hide).
The atlas (Spider.py's) is shared as before: one leg's paint fills the femur, coxa and tibia regions (REP), the left palp
and chelicera theirs; the skulls take the head region (the Tripo body paints the whole cephalothorax on the carapace's);
eyes, fangs, spinnerets and railings take solid patches in Tripo's own colours (measured off its map).
The railings: Tripo's two iron grave railings (its RAILING faces, dropped by the build) placed by the headstone's own frame
(as the game seats the stone: looter_barrow.stone_frame) and set down onto her hide; rigid on the abdomen bone, outside
the hit hull. Her mud sleeves are left off (Tripo's legs are clean, mottled)."""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_barrow as lb
import looter_creatures as lc
import looter_textures as lt
import looter_tripo as tripo
import parts_gravemother as pg
import parts_gravemother_build as pb
import parts_gravemother_look as gl
import parts_gravemother_shape as gs
import parts_spider_look as sl

SET = 'GravemotherTripo'              # MI_GravemotherHide and MI_GravemotherAbdomen on it
BARE_SET = 'GravemotherBare'          # MI_GravemotherBare (build_grave_fight_materials.py): the same paint, print bare
REP = ('femur_1_l', 'tibia_1_l')      # the leg whose paint fills the shared leg regions (Tripo's left middle leg)
ROUGH_MAX = 0.9
RAILING_SINK = 1.5                    # cm: a railing's feet into her hide


def log(message):
    print(f'GRAVEMOTHER PAINT: {message}', flush=True)


# --- Tripo, its pieces in its own space ---

class Tripo:
    """Tripo's model (parts_gravemother_shape.Source: creature axes, Tripo units) with its maps kept, its labels, and the
    build's pieces cut again in its space."""

    def __init__(self):
        self.src = gs.Source()
        obj, maps = tripo.load(gs.GLB)
        mesh = obj.data
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)
        self.src.maps = maps
        self.labels, self.info = gs.segment(self.src)
        self.raw, self.roots = pg.extract(self.src, self.labels, self.info)
        self.pieces = self._pieces()

    def on(self, *keep):
        """Tripo's triangles all of whose corners carry one of these labels."""
        return np.isin(self.labels[self.src.F], keep).all(1)

    def _pieces(self):
        raw, roots, info = self.raw, self.roots, self.info
        out = {}
        V, F = raw['thorax']
        out['thorax'] = pg.close(V, F, V.mean(0), dome=0.35)
        for pair in range(4):
            for side, suffix in ((0, 'l'), (1, 'r')):
                leg = 2 * pg.LEG_FROM[pair] + side
                line = info['lines'][leg]
                V, F = raw[f'leg{leg}']
                end, no = roots[f'leg{leg}']
                hip_s, knee_s, tip_s, k = pg.knee_of(line)
                V, F = pg.close(V, F, V.mean(0), stub=(end, no, 2.2), dome=0.3)
                fdir = (knee_s - hip_s) / np.linalg.norm(knee_s - hip_s)
                tdir = (tip_s - knee_s) / np.linalg.norm(tip_s - knee_s)
                cut_no = (fdir + tdir) / np.linalg.norm(fdir + tdir)
                (Vf, Ff), (Vt, Ft) = pg.split_plane(V, F, knee_s, cut_no)
                out[f'femur_{pair}_{suffix}'] = pg.close(Vf, Ff, hip_s, dome=0.55)
                out[f'tibia_{pair}_{suffix}'] = pg.close(Vt, Ft, tip_s, dome=0.55)
        for name, _bone, _kind, _label in pb.SMALL:
            V, F = raw[name]
            end, no = roots[name]
            out[name] = pg.close(V, F, V.mean(0), stub=(end, no, 1.0), dome=0.3)
        V, F = raw['abdomen']
        out['abdomen'] = pg.close(V, F, V.mean(0), dome=0.3)
        for k in range(2):
            V, F = raw[f'skull{k}']
            out[f'skull{k}'] = pg.close(V, F, V.mean(0), dome=0.25)
        return out


LABELS = {'thorax': (gs.THORAX,), 'abdomen': (gs.ABDOMEN,), 'palp_l': (gs.PALP_L,), 'chel_l': (gs.CHEL_L,),
          'skull0': (gs.SKULL,), 'skull1': (gs.SKULL,)}


def piece_labels(name):
    if name.startswith(('femur', 'tibia')):
        pair, side = int(name.split('_')[1]), 0 if name.endswith('_l') else 1
        return (2 * pg.LEG_FROM[pair] + side,)
    return LABELS[name]


def _fan(F):
    return np.array([(f[0], f[k], f[k + 1]) for f in F for k in range(1, len(f) - 1)], np.int64)


def to_tripo(P, source_V, source_F, tripo_V):
    """Points near a piece's carried mesh (creature cm) onto its twin in Tripo's space: nearest point, same face and
    barycentrics. Returns (points in Tripo's space, distance off the carried mesh in cm)."""
    T = _fan(source_F)
    S = np.asarray(source_V, float)
    tree = BVHTree.FromPolygons(S.tolist(), T.tolist(), all_triangles=True)
    out = np.empty((len(P), 3))
    off = np.empty(len(P))
    for i, p in enumerate(P):
        loc, _n, k, d = tree.find_nearest(Vector(p))
        a, b, c = S[T[k]]
        v0, v1, v2 = b - a, c - a, np.array(loc[:]) - a
        d00, d01, d11, d20, d21 = v0 @ v0, v0 @ v1, v1 @ v1, v2 @ v0, v2 @ v1
        den = d00 * d11 - d01 * d01
        if abs(den) < 1e-12:
            w = np.array([1.0, 0.0, 0.0])
        else:
            y = (d11 * d20 - d01 * d21) / den
            z = (d00 * d21 - d01 * d20) / den
            w = np.array([1.0 - y - z, y, z])
        out[i] = w @ tripo_V[T[k]]
        off[i] = d
    return out, off


class Template:
    """An lc.Mesh-like template for Look.carry: the piece's faces and atlas UVs, its vertices on Tripo's surface."""

    def __init__(self, V, F, corner_uv):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        verts = [self.bm.verts.new(Vector(p)) for p in V]
        for f, uvs in zip(F, corner_uv):
            try:
                face = self.bm.faces.new([verts[int(k)] for k in f])
            except ValueError:
                continue
            for loop, uv in zip(face.loops, uvs):
                loop[self.uv].uv = uv


# --- The paint ---

def _solid_colour(src, mask, pick=None):
    """The median sRGB colour of Tripo's vertices in mask (pick: a narrower choice of them by value), as 0xRRGGBB."""
    C = src.C[mask]
    if pick is not None and len(C):
        C = C[pick(C)] if pick(C).any() else C
    if not len(C):
        return 0x303030
    c = np.clip(np.median(C, axis=0), 0.0, 1.0)
    return int(round(c[0] * 255)) << 16 | int(round(c[1] * 255)) << 8 | int(round(c[2] * 255))


def solids(t):
    """The solid patches in Tripo's own colours: (sRGB, roughness)."""
    src, labels = t.src, t.labels
    luma = lambda C: C @ np.array([0.299, 0.587, 0.114])
    eye = (labels == gs.THORAX) & (luma(src.C) > 0.55) & (src.V[:, 0] > 12.0)
    chel = labels == gs.CHEL_L
    # A chelicera's far half is its fang.
    if chel.any():
        line = t.info['lines'][gs.CHEL_L][:, 1:4]
        tip = line[0]
        d = np.linalg.norm(src.V - tip, axis=1)
        chel &= d < np.percentile(d[chel], 40)
    out = {
        'eye': (_solid_colour(src, eye), 0.12),
        'fang': (_solid_colour(src, chel, lambda C: luma(C) < np.median(luma(C))), 0.45),
        'misc': (_solid_colour(src, labels == gs.ABDOMEN, lambda C: luma(C) < np.percentile(luma(C), 30)), 0.8),
        'bristle': (_solid_colour(src, labels == gs.RAILING, lambda C: luma(C) < np.median(luma(C))), 0.55),
    }
    log('solid patches (Tripo\'s colours): ' + ', '.join(f'{k} #{v[0]:06x}' for k, v in out.items()))
    return out


def corner_uvs(name, piece, atlas):
    """A remeshed piece's per-face corner UVs on the atlas, as parts_gravemother_look lays them."""
    kind = piece['kind']
    if kind == 'thorax':
        V = piece['V']
        return gl.thorax_uvs(atlas, piece, gl.fit_ellipsoid(V), gl.fit_ellipsoid(V[V[:, 0] >= gl.X_HEAD - 6.0]))
    if kind == 'abdomen':
        return gl.abdomen_uvs(atlas, piece)
    if kind == 'skull':
        return gl.skull_uvs(atlas, piece)
    return gl.tube_uvs(atlas, piece)


def paint(t, pieces, atlas):
    """Fills the atlas from Tripo (t: Tripo) on the remeshed pieces ({name: piece}, creature cm, cached by the build) and
    writes SET and BARE_SET (the same paint). Returns the solids."""
    look = sl.Look(t.src)
    jobs = [('carapace and head', 'thorax'), ('abdomen', 'abdomen'), ('femur and coxa', REP[0]), ('tibia', REP[1]),
            ('palp', 'palp_l'), ('chelicera', 'chel_l'), ('skull', 'skull0')]
    for label, name in jobs:
        piece = pieces[name]
        Vt, Ft = t.pieces[name]
        if len(Vt) != len(piece['source_V']) or len(Ft) != len(piece['source_F']):
            raise RuntimeError(f'{name}: Tripo\'s cut ({len(Vt)} vertices, {len(Ft)} faces) is not the build\'s '
                               f'({len(piece["source_V"])}, {len(piece["source_F"])}): the build changed; rerun it')
        P, off = to_tripo(np.asarray(piece['V'], float), piece['source_V'], piece['source_F'], np.asarray(Vt, float))
        log(f'{name}: {len(P)} vertices onto Tripo (median {np.median(off):.2f} cm off the carried cut, 95% within '
            f'{np.percentile(off, 95):.2f})')
        template = Template(P, piece['F'], corner_uvs(name, piece, atlas))
        look.carry(label, template, t.src.V, t.on(*piece_labels(name)))
        template.bm.free()
    tidy_limbs(look, atlas, t)
    found = solids(t)
    sl_solid = sl.SOLID
    sl.SOLID = found
    try:
        look.solids(atlas)
    finally:
        sl.SOLID = sl_solid
    # Tripo's hide as it is on the abdomen, in both sets: Tripo painted its own root fibres, and under the stone's foot
    # (closed where its stone stood) the hide nearest it. The GRAVE look's pale print and root puckers are left off: the
    # bare set is the same paint, so the game's bare swap (MI_GravemotherBare) changes nothing visible.
    for name in (SET, BARE_SET):
        write(look, name)
    return found


LIMBS = ('coxa', 'femur', 'tibia', 'palp', 'chel')
LIMB_DETAIL = 0.2         # how much of the carried texture's mottling is kept over the limb's band colours
LIMB_TOP = 0.12           # lighter along the top, darker underneath (Spider.py's painted chitin)
LIMB_FLAT = 0.5           # Tripo's streaky relief on the limbs, halved
BANDS = 64                # bins along a limb for its colour


def limb_bands(t, label, part):
    """A Tripo limb's colour along it, body end first: (u 0..1, linear colours), from its own vertices' colours (each on
    its nearest station of the limb's centre line, a median per bin). part: 'femur' (hip to knee), 'tibia' (knee to
    tip) or 'whole'."""
    line = t.info['lines'][label]
    P = line[:, 1:4]                                                 # tip first
    if part == 'whole':
        seg = np.arange(len(P))[::-1]
    else:
        _hip, _knee, _tip, k = pg.knee_of(line)
        seg = np.arange(k, len(P))[::-1] if part == 'femur' else np.arange(0, k + 1)[::-1]
    S = P[seg]
    s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(S, axis=0), axis=1))])
    s /= max(s[-1], 1e-6)
    ids = np.nonzero(t.labels == label)[0]
    Vv = t.src.V[ids]
    near = np.argmin(np.linalg.norm(Vv[:, None, :] - P[None, :, :], axis=2), axis=1)   # nearest station on the line
    pos = {int(station): i for i, station in enumerate(seg)}
    keep = np.array([n in pos for n in near])
    u = s[[pos[int(n)] for n in near[keep]]]
    C = sl._srgb_to_linear(t.src.C[ids[keep]])
    centres = (np.arange(BANDS) + 0.5) / BANDS
    out = np.full((BANDS, 3), np.nan, np.float32)
    for b in range(BANDS):
        m = (u >= b / BANDS) & (u < (b + 1) / BANDS)
        if m.sum() >= 3:
            out[b] = np.median(C[m], axis=0)
    good = ~np.isnan(out[:, 0])
    for c in range(3):
        out[:, c] = np.interp(centres, centres[good], out[good, c])
    k = np.array([1.0, 2.0, 1.0], np.float32) / 4.0
    out = np.stack([np.convolve(np.pad(out[:, c], 1, mode='edge'), k, mode='valid') for c in range(3)], -1)
    return centres, out


def tidy_limbs(look, atlas, t):
    """The limbs in the game's crisp banded look (the in-game Spider's and Spitter's). Carried texel by texel, Tripo's
    thin legs came out smeared: its paint is grained like wood along them, and many texels land on its small UV islands'
    dark edges (dark femurs, chalky tips). So each limb region is repainted from Tripo's own limb: its colour along it
    (its vertices' colours binned along its centre line: the pale shafts, the dark joint rings), a little of the carried
    mottling over it, a painted light along the top; its streaky relief halved."""
    w = np.array([0.2126, 0.7152, 0.0722], np.float32)
    leg = piece_labels(REP[0])[0]
    femur = limb_bands(t, leg, 'femur')
    tibia = limb_bands(t, leg, 'tibia')
    palp = limb_bands(t, gs.PALP_L, 'whole')
    chel = limb_bands(t, gs.CHEL_L, 'whole')
    # Each region's U (0 its body end) as a share of its tube, and which bands it reads (parts_gravemother_look.tube_uvs).
    plan = {'coxa': (femur, lambda U: U * gl.COXA_SHARE), 'femur': (femur, lambda U: gl.COXA_SHARE + U * (1.0 - gl.COXA_SHARE)),
            'tibia': (tibia, lambda U: U), 'palp': (palp, lambda U: U), 'chel': (chel, lambda U: U * gl.FANG_FROM)}
    for region in LIMBS:
        (centres, bands), along = plan[region]
        sl_, U, V = lc.region_grid(atlas[region], sl.SIZE)
        bc, known = look.bc[sl_], look.known[sl_]
        u = along(U[0])                                                          # per column
        prof = np.stack([np.interp(u, centres, bands[:, c]) for c in range(3)], -1).astype(np.float32)
        masked = np.where(known, bc @ w, np.nan)
        col_l = np.nanmedian(masked, axis=0)
        col_l = np.where(np.isnan(col_l), np.nanmedian(col_l), col_l)
        detail = np.clip((bc @ w) / np.maximum(col_l, 1e-4)[None, :], 0.6, 1.5)
        _, top = lc.limb_axes(U, V)
        new = prof[None, :, :] * (1.0 + LIMB_DETAIL * (detail - 1.0))[..., None] * (1.0 + LIMB_TOP * top)[..., None]
        look.bc[sl_] = np.where(known[..., None], new, bc).astype(np.float32)
        n = look.normal[sl_]
        n = n * (1.0 - LIMB_FLAT) + np.array([0.0, 0.0, 1.0], np.float32) * LIMB_FLAT
        look.normal[sl_] = n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
        log(f'{region}: banded from Tripo\'s limb (median sRGB '
            f'{np.round(sl._linear_to_srgb(np.median(prof, axis=0)), 2).tolist()})')


def write(look, set_name):
    """Look.write's maps (dilated, the colour's fine relief as a bump, crevice occlusion) into set_name."""
    import looter_rebake as rb
    known = look.known
    bc = rb._dilate(look.bc, known, steps=24)
    rough = rb._dilate(look.rough[..., None], known, steps=24)[..., 0]
    n = rb._dilate(look.normal, known, steps=24)
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    luma = bc @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    height = luma - lt.blur(luma, 3.0)
    gx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
    gy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
    n = n + np.stack([-gx * sl.RELIEF * 8.0, gy * sl.RELIEF * 8.0, np.zeros_like(gx)], -1)
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    n[..., 1] = -n[..., 1]                                            # DirectX: green down
    dark = np.clip((lt.blur(luma, 6.0) - luma) / np.maximum(lt.blur(luma, 6.0), 1e-3), 0.0, 1.0)
    occ = 1.0 - 0.5 * np.clip(dark * 2.0, 0.0, 1.0)
    orm = np.stack([occ, np.clip(rough, 0.35, ROUGH_MAX), np.zeros_like(rough)], -1)
    paths = [lt.texture_path(set_name, k) for k in ('BC', 'ORM', 'N')]
    os.makedirs(os.path.dirname(paths[0]), exist_ok=True)
    lt.write_png(paths[0], lt.to8(sl._linear_to_srgb(bc)))
    lt.write_png(paths[1], lt.to8(orm))
    lt.write_png(paths[2], lt.to8(n * 0.5 + 0.5))
    log(f'{set_name}: wrote T_{set_name}_BC, _ORM, _N ({sl.SIZE} px), {known.mean() * 100:.0f}% of the atlas from Tripo')


def kept():
    """LOOTER_GRAVEMOTHER_LOOK=0 and both sets on disk: the maps are reused."""
    return os.environ.get('LOOTER_GRAVEMOTHER_LOOK', '1') == '0' and all(
        os.path.exists(lt.texture_path(s, k)) for s in (SET, BARE_SET) for k in ('BC', 'N', 'ORM'))


# --- The railings ---

def railings(t, stone_to_creature, hide_tree, atlas):
    """Tripo's grave railings as one piece on her back: (V creature cm, F, corner UVs on the railing patch), or None.
    Placed from Tripo's headstone frame into the game's stone frame (stone_to_creature: stone cm -> creature cm, a
    Matrix), uniformly scaled as the stone's width is, then each railing set down so its lowest point sits RAILING_SINK
    into her hide (hide_tree: a BVHTree of her abdomen, creature cm)."""
    src, labels = t.src, t.labels
    tris = src.F[t.on(gs.RAILING)]
    if not len(tris):
        log('no railings in Tripo\'s model')
        return None
    st = t.info['stone']
    c, (wx, th, up) = st['centre'], st['axes']
    wx = -wx                                                          # as parts_gravemother_build.stone_piece
    stone_V = t.raw['stone'][0]
    rel = stone_V - c
    h_cut = st['foot'] + 1.0
    k = pg.STONE_WIDTH / np.ptp(rel @ wx)
    cut_z = pg.STONE_TOP - k * ((rel @ up).max() - h_cut)
    high = (rel @ up) >= h_cut
    mid_x, mid_y = (rel[high] @ wx).mean(), (rel[high] @ th).mean()
    used = np.unique(tris)
    remap = -np.ones(len(src.V), np.int64)
    remap[used] = np.arange(len(used))
    r = src.V[used] - c
    Q = np.column_stack([(r @ wx - mid_x) * k, (r @ th - mid_y) * k, (r @ up - h_cut) * k + cut_z])
    M = np.array(stone_to_creature)
    P = Q @ M[:3, :3].T + M[:3, 3]
    F = remap[tris]
    # Each railing (a connected piece) down onto her hide.
    parent = np.arange(len(P))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a
    for f in F:
        for i in f[1:]:
            ra, rb_ = find(int(f[0])), find(int(i))
            if ra != rb_:
                parent[rb_] = ra
    comp = np.array([find(i) for i in range(len(P))])
    for root in np.unique(comp):
        ids = np.nonzero(comp == root)[0]
        if len(ids) < 12:
            continue
        low = ids[np.argsort(P[ids, 2])[:max(3, len(ids) // 20)]]
        drops = []
        for i in low:
            hit = hide_tree.ray_cast(Vector(P[i]) + Vector((0.0, 0.0, 40.0)), Vector((0.0, 0.0, -1.0)), 120.0)
            if hit[0] is not None:
                drops.append(P[i, 2] - hit[0].z)
        if drops:
            P[ids, 2] -= float(np.median(drops)) + RAILING_SINK
    log(f'railings: {len(F)} triangles in {len(np.unique(comp))} pieces, {np.ptp(P[:, 1]):.0f} cm across her back')
    return P, F, [gl.patch_uv(atlas, 'bristle', 3) for _ in F]
