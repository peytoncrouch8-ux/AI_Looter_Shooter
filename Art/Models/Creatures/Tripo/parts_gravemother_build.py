"""SK_Gravemother's pieces carried into Spider.py's rig and remeshed (parts_gravemother.py cuts them off the Tripo model;
its docstring has the maps). Not a model: Tools\\models.ps1 skips parts_*.py.

build(bones) -> ({name: dict(V, F, bone, kind, line?)}, extras): creature cm, the rig's rest pose, even quads, cached in
Intermediate/GravemotherModel/TripoQuads (QuadriFlow takes minutes; the key hashes the GLB, these scripts and the bones).
Pieces: thorax (on 'body'; the head's part is told by place), abdomen and skull0/1 (on 'abdomen'), femur_/tibia_<pair>_<side>,
palp_l/r and claw_l/r (on the palps), chel_l/r (on the fangs), stone (the headstone in stone cm, no bone). extras: the
fang bones' new places (each chelicera's root and its tip: the game turns a fang about its bone's head, so the bone sits
where Tripo's chelicera meets the face), the eyes' middle."""
import hashlib
import os

import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_barrow as lb
import looter_tripo_rig as tr
import parts_gravemother as pg
import parts_gravemother_shape as gs
import parts_gravemother_tube as pt

log = pg.log


def ray_exit(tree, knee, hip):
    """Where the line from knee toward hip first meets the surface (the femur's root), else hip."""
    d = Vector(hip) - Vector(knee)
    hit = tree.ray_cast(Vector(knee), d.normalized(), d.length * 1.3)
    if hit[0] is not None:
        return np.array(hit[0][:])
    return np.asarray(hip, float)


def apply(M, V):
    V = np.asarray(V, float)
    return (np.column_stack([V, np.ones(len(V))]) @ M.T)[:, :3]


def leg_pieces(raw, roots, info, bones, thorax_tree):
    """Each rig leg's femur and tibia: {'femur_<pair>_<side>': (V cm, faces, its line mapped, across scale), ...}."""
    out = {}
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
            Vf, Ff = pg.close(Vf, Ff, hip_s, dome=0.55)
            Vt, Ft = pg.close(Vt, Ft, tip_s, dome=0.55)
            n_s = np.cross(knee_s - hip_s, tip_s - knee_s)
            hip_r, knee_r = bones[f'femur_{pair}_{suffix}']
            foot_r = bones[f'tibia_{pair}_{suffix}'][1]
            n_r = np.cross(knee_r - hip_r, foot_r - knee_r)
            exit_r = ray_exit(thorax_tree, knee_r, hip_r)
            for name, Vp, Fp, (a_s, b_s), (a_r, b_r), sl in (
                    (f'femur_{pair}_{suffix}', Vf, Ff, (hip_s, knee_s), (exit_r, knee_r), slice(k, None)),
                    (f'tibia_{pair}_{suffix}', Vt, Ft, (knee_s, tip_s), (knee_r, foot_r), slice(0, k + 1))):
                along = np.linalg.norm(b_r - a_r) / np.linalg.norm(b_s - a_s)
                across = min(along, pg.BODY_SCALE * 1.08)
                M = tr.bone_map(tr.frame(a_s, b_s, n_s), tr.frame(a_r, b_r, n_r), along=along, across=across)
                stations = line[sl, 1:4][::-1]
                out[name] = (apply(M, Vp), Fp, apply(M, stations), across)
                log(f'{name}: from Tripo leg {leg}, stretched {along:.2f} along, {across:.2f} across')
    return out


def stone_piece(raw, info):
    """The headstone in its own frame (stone cm): its width and top the scripted slab's, its foot down to z = 0."""
    st = info['stone']
    c, (wx, th, up) = st['centre'], st['axes']
    # The stone's x runs to her right (looter_barrow.stone_frame turns it so): with y out of its face and z up, a turn,
    # not a mirror, so Tripo's faces keep facing out.
    wx = -wx
    V, F = raw['stone']
    rel = V - c
    h_cut = st['foot'] + 1.0
    k = pg.STONE_WIDTH / np.ptp(rel @ wx)
    cut_z = pg.STONE_TOP - k * ((rel @ up).max() - h_cut)
    co = c + up * h_cut
    V, F = pg.close(V, F, c, stub=(co, -up, cut_z / k), dome=0.02)
    # A stub's ring is drawn in 15%: put the foot back out to the slab's own outline (straight sides down to it).
    rel = V - c
    x, y, h = rel @ wx, rel @ th, rel @ up
    low = h < h_cut - 1e-4
    mid_x, mid_y = x[~low].mean(), y[~low].mean()
    x = np.where(low, mid_x + (x - mid_x) / 0.85, x)
    y = np.where(low, mid_y + (y - mid_y) / 0.85, y)

    def frame(x, y, h):
        return np.column_stack([(x - mid_x) * k, (y - mid_y) * k, (h - h_cut) * k + cut_z])
    S = frame(x, y, h)
    # Exactly the scripted slab's width and thickness (80 x 17 cm, its middle on the pivot): the stone's texture is laid
    # out as the scripted one's (looter_barrow_stone.REGIONS: its faces by x and z, its edge strip across its thickness).
    fit = []
    for axis, size in ((0, pg.STONE_WIDTH), (1, lb.STONE_T)):
        lo, hi = S[:, axis].min(), S[:, axis].max()
        fit.append((axis, (lo + hi) * 0.5, size / (hi - lo)))

    def place(P):
        """Tripo's stone (its space) into stone cm, as the slab was."""
        r = np.asarray(P, float) - c
        out = frame(r @ wx, r @ th, r @ up)
        for axis, mid, scale in fit:
            out[:, axis] = (out[:, axis] - mid) * scale
        return out
    for axis, mid, scale in fit:
        S[:, axis] = (S[:, axis] - mid) * scale
    log(f'headstone: {np.ptp(S[:, 0]):.1f} x {np.ptp(S[:, 1]):.1f} x {S[:, 2].max():.1f} cm (stone cm; its cut at '
        f'z {cut_z:.1f})')
    return S, F, place


def sphere_fit(P):
    A = np.column_stack([2 * P, np.ones(len(P))])
    b = (P ** 2).sum(1)
    sol, *_ = np.linalg.lstsq(A, b, rcond=None)
    c = sol[:3]
    return c, float(np.sqrt(sol[3] + c @ c))


def egg(P):
    """Spider.py's egg on the abdomen (looter_barrow.Abdomen's: narrower toward the pedicel, flatter underneath), about
    AB_CENTRE along x: so her hide is the very shape the headstone's roots, earth collar and painted marks are laid on."""
    rel = (np.asarray(P, float) - pg.AB_CENTRE) / pg.AB_RADII
    front = 1.0 - 0.12 * np.maximum(rel[:, 0], 0.0)
    rel[:, 1] *= front
    rel[:, 2] *= front * np.where(rel[:, 2] < 0.0, 0.8, 1.0)
    return rel * pg.AB_RADII + pg.AB_CENTRE


def eye_points(src, labels):
    """Tripo's milky eyes (pale, on the face), Tripo units."""
    luma = src.C @ np.array([0.299, 0.587, 0.114])
    eyes = (labels == gs.THORAX) & (luma > 0.55) & (src.V[:, 0] > 12.0)
    return src.V[eyes] if eyes.any() else np.array([[16.0, 0.0, 20.0]])


SMALL = (('palp_l', 'palp_l', 'palp', gs.PALP_L), ('palp_r', 'palp_r', 'palp', gs.PALP_R),
         ('chel_l', 'fang_l', 'chel', gs.CHEL_L), ('chel_r', 'fang_r', 'chel', gs.CHEL_R))


def layout(bones):
    """Every piece in creature cm, before remeshing, and the extras."""
    src = gs.Source()
    labels, info = gs.segment(src)
    raw, roots = pg.extract(src, labels, info)
    pieces = {}
    V, F = raw['thorax']
    V, F = pg.close(V, F, V.mean(0), dome=0.35)
    Vt = pg.to_body(V)
    pieces['thorax'] = dict(V=Vt, F=F, bone='body', kind='thorax')
    tree = BVHTree.FromPolygons(Vt.tolist(), [list(map(int, f)) for f in F])
    for name, (Vl, Fl, line, across) in leg_pieces(raw, roots, info, bones, tree).items():
        pieces[name] = dict(V=Vl, F=Fl, bone=name, kind=name.split('_')[0], line=line)
    fangs = {}
    for name, bone, kind, label in SMALL:
        V, F = raw[name]
        end, no = roots[name]
        V, F = pg.close(V, F, V.mean(0), stub=(end, no, 1.0), dome=0.3)
        line = pg.to_body(info['lines'][label][::-1, 1:4])
        pieces[name] = dict(V=pg.to_body(V), F=F, bone=bone, kind=kind, line=line)
        if kind == 'chel':
            fangs[bone] = (pg.to_body(end + no * 0.6), line[-1])
    V, F = raw['abdomen']
    V, F = pg.close(V, F, V.mean(0), dome=0.3)
    c_src, r_src = sphere_fit(V[V[:, 0] < info['pedicel'] - 2.0])
    c_src[1] = 0.0
    scale = pg.AB_RADII / r_src

    def to_ab(P):
        return egg((np.asarray(P, float) - c_src) * scale + pg.AB_CENTRE)
    pieces['abdomen'] = dict(V=to_ab(V), F=F, bone='abdomen', kind='abdomen')
    for k in range(2):
        V, F = raw[f'skull{k}']
        V, F = pg.close(V, F, V.mean(0), dome=0.25)
        pieces[f'skull{k}'] = dict(V=to_ab(V), F=F, bone='abdomen', kind='skull')
    S, F, place = stone_piece(raw, info)
    Vt, Ft, UVt = raw['stone_tripo']
    pieces['stone'] = dict(V=S, F=F, bone=None, kind='stone', tripo=dict(V=place(Vt), F=Ft, uv=UVt))
    log(f'abdomen: Tripo\'s ball (r {r_src:.2f} at {np.round(c_src, 2).tolist()}) x {np.round(scale, 2).tolist()}')
    eyes = pg.to_body(eye_points(src, labels))
    return pieces, dict(fangs=fangs, eyes=eyes.mean(0), eye_points=eyes)


# --- Even quads (cached) ---

def largest(V, F):
    """The biggest connected piece of a mesh (QuadriFlow can leave crumbs)."""
    parent = np.arange(len(V))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a
    for f in F:
        r0 = find(int(f[0]))
        for i in f[1:]:
            r = find(int(i))
            if r != r0:
                parent[r] = r0
    roots = np.array([find(i) for i in range(len(V))])
    used = np.zeros(len(V), bool)
    for f in F:
        used[np.asarray(f)] = True
    ids, counts = np.unique(roots[used], return_counts=True)
    keep = (roots == ids[np.argmax(counts)]) & used
    remap = -np.ones(len(V), int)
    remap[keep] = np.arange(keep.sum())
    return V[keep], [remap[np.asarray(f)] for f in F if keep[f[0]]]


# Limbs as tubes (parts_gravemother_tube): (sides, ring spacing cm).
TUBES = dict(femur=(14, 4.5, 0.4), tibia=(12, 4.0, 0.4), palp=(10, 2.2, 0.25), claw=(8, 1.6, 0.2), chel=(10, 2.0, 0.25))
# Where the painter draws each limb's joints (shares of its length: Spider.py's LIMB_SEGMENTS, the coxa its femur's first
# fifth): the tube narrows into a plate's neck there.
JOINTS = dict(femur=(0.0, 0.14, 0.2, 1.0), tibia=(0.0, 0.13, 0.51, 0.86, 1.0), palp=(0.0, 0.45, 0.6, 1.0))


def remesh(name, piece, eyes, bones):
    """(V, F, uv or None): limbs as tubes round their lines, the rest QuadriFlow's even quads (retried a little coarser
    or with another seed when it turns a voxel mesh down)."""
    kind = piece['kind']
    if kind == 'abdomen':
        # An egg of clean quads shrunk onto Tripo's hide (QuadriFlow made lace of it: the holes its stone and root
        # fibres left never closed watertight).
        V, F, uv = pt.egg_shell(piece['V'], piece['F'], pg.AB_CENTRE, pg.AB_RADII)
        return V, F, uv, None
    if kind in TUBES:
        sides, spacing, voxel = TUBES[kind]
        if kind in ('femur', 'tibia'):
            hip = bones['femur_' + name.split('_', 1)[1]][0]
            out = np.array([hip[0], hip[1], 0.0])
            pole = np.array([0.0, 0.0, 1.0]) + out / np.linalg.norm(out) * 0.4
        else:
            pole = np.array([1.0, 0.0, 0.3])
        V, F, uv, profile = pt.tube(piece['V'], piece['F'], piece['line'], pole, sides, spacing, voxel=voxel,
                                    joints=JOINTS.get(kind))
        return V, F, uv, profile
    regions = [(np.asarray(eyes) * 0.01, 1.7, 0.07, 0.16)] if kind == 'thorax' else []
    for attempt in range(4):
        try:
            Vq, Fq = tr.quad_remesh(np.asarray(piece['V']) * 0.01, piece['F'], pg.QUADS[kind], regions,
                                    voxel=pg.VOXEL[kind] * 0.01 * (1.0 + 0.12 * attempt), seed=3 + attempt,
                                    name=f'_gm_{name}')
            break
        except RuntimeError as error:
            log(f'{name}: {error}; again')
    else:
        raise RuntimeError(f'QuadriFlow failed on {name} four times')
    Vq, Fq = largest(Vq, Fq)
    # QuadriFlow keeps no winding of its own: faces turned out (its volume positive), or the game draws the inside.
    volume = sum(np.dot(Vq[f[0]], np.cross(Vq[f[k]], Vq[f[k + 1]])) for f in Fq for k in range(1, len(f) - 1))
    if volume < 0:
        Fq = [np.asarray(f)[::-1] for f in Fq]
    return Vq * 100.0, Fq, None, None


def _key(bones):
    h = hashlib.sha1()
    for path in (gs.GLB, gs.__file__, pg.__file__, __file__, tr.__file__, pt.__file__):
        with open(path, 'rb') as f:
            h.update(f.read())
    h.update(repr(sorted((k, np.round(np.asarray(v), 3).tolist()) for k, v in bones.items())).encode())
    return h.hexdigest()[:16]


def build(bones):
    """bones: {name: (head, tail)} creature cm (numpy). Returns ({name: piece}, extras); cached."""
    key = _key(bones)
    path = os.path.join(pg.CACHE, f'pieces_{key}.npz')
    if os.path.exists(path):
        data = np.load(path, allow_pickle=True)
        log(f'pieces from the cache ({os.path.basename(path)})')
        return data['pieces'].item(), data['extra'].item()
    pieces, extra = layout(bones)
    out = {}
    os.makedirs(pg.CACHE, exist_ok=True)
    for name, piece in pieces.items():
        # Each piece is kept as it's made, so a run that stops (QuadriFlow can run out of memory beside other Blenders)
        # picks up where it left off.
        one = os.path.join(pg.CACHE, f'piece_{key}_{name}.npz')
        if os.path.exists(one):
            out[name] = np.load(one, allow_pickle=True)['piece'].item()
            continue
        Vq, Fq, uv, profile = remesh(name, piece, extra['eyes'], bones)
        out[name] = dict(piece, V=Vq, F=[np.asarray(f) for f in Fq], uv=uv, profile=profile,
                         source_V=np.asarray(piece['V']), source_F=[np.asarray(f) for f in piece['F']])
        np.savez(one, piece=np.array(out[name], dtype=object))
        log(f'{name}: {len(Fq)} quads ({sum(len(f) - 2 for f in Fq)} triangles)')
    np.savez(path, pieces=np.array(out, dtype=object), extra=np.array(extra, dtype=object))
    for name in pieces:
        os.remove(os.path.join(pg.CACHE, f'piece_{key}_{name}.npz'))
    return out, extra
