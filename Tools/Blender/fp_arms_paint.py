"""The heroes' first-person texture sets (Tools/Blender/fp_arms_heroes.py), so first and third person look alike: each
hero's own procedural looks (the materials their third-person model is baked from: odessa_looks, crane_looks,
pike_looks, gauge_looks and looter_looks, with the colours their model scripts give them) baked onto these meshes by
looter_bake (colour, roughness, metal, normal, occlusion), then the hand's own detail painted over the bake in numpy
(FPArms.py's way, on fp_arms_hands.hand_fields): a glove's seams, stitched points and creases; a bare hand's nails, its
knuckles' wrinkles and palm lines; a fine weave on the cloth.

The looks read object space and height over the ground (dust and mud climb from the boots), so the arms are baked raised
RAISE metres, about where a body carries them. Each set is baked from the right side alone (the left is its mirror and
shares its texels). The bake is skipped when nothing it depends on changed: bake.digest beside the hero's first set
holds a fingerprint of these scripts, FPArms.py, the looks' modules, the model scripts the colours come from, the size
and Blender's version ('rebake' after '--' forces it; 'keep-textures' reuses the files whatever the digest says).
"""
import os

import bpy
import numpy as np
from mathutils import Matrix

import looter_bake as bk
import looter_textures as lt
from fp_arms_base import A, log

RAISE = 1.5
SIZE = 2048
rasterize, mesh_corners, texel_size = A['rasterize'], A['mesh_corners'], A['texel_size']


def _raise(objects, dz):
    for ob in objects:
        ob.data.transform(Matrix.Translation((0.0, 0.0, dz)))
        ob.data.update()


def digest_of(paths, size):
    return bk.digest([p for p in paths if os.path.exists(p)], f'size={size} blender={bpy.app.version_string} fp=3')


def _top(img):
    """A Blender image's pixels with row 0 at the top (V = 1), as numpy."""
    return bk._pixels(img)[::-1].copy()


def _dx_normal(height, px_m, strength=1.0):
    """A height field's tangent normal (DirectX: +green toward the image's bottom), float -1..1."""
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2.0 * px_m)
    drow = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / (2.0 * px_m)
    # no slope past about 50 degrees: a texel where the surface is squeezed in the atlas would otherwise turn sideways
    dx, drow = np.clip(dx * strength, -1.2, 1.2), np.clip(drow * strength, -1.2, 1.2)
    n = np.stack([-dx, -drow, np.ones_like(dx)], axis=-1)
    return n / np.linalg.norm(n, axis=-1, keepdims=True)


def _blend_normals(base, detail):
    """Whiteout blend of two tangent normals (both -1..1, the same convention)."""
    n = np.stack([base[..., 0] + detail[..., 0], base[..., 1] + detail[..., 1], base[..., 2] * detail[..., 2]], -1)
    return n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)


def rasterize_object(obj, co, nrm=None, extra=None):
    """Per-texel attributes of one object in the set's atlas (its packed 'UVMap'): positions co (n, 3: any space the
    caller works in), normals nrm and extra per-vertex columns. Returns (img (S, S, K), mask), rows top first."""
    uv, idx, _, me_nrm = mesh_corners(obj)
    cols = [co[idx]]
    cols.append((nrm if nrm is not None else me_nrm)[idx])
    if extra is not None:
        cols.append(np.asarray(extra, dtype=np.float64).reshape(len(co), -1)[idx])
    attrs = np.concatenate(cols, axis=2)
    img, mask, tri = rasterize(uv, attrs, SIZE)
    # each texel's triangle's metres per texel (a face squeezed in the atlas spans more of the surface a texel)
    a3 = 0.5 * np.linalg.norm(np.cross(co[idx[:, 1]] - co[idx[:, 0]], co[idx[:, 2]] - co[idx[:, 0]]), axis=1)
    e1, e2 = uv[:, 1] - uv[:, 0], uv[:, 2] - uv[:, 0]
    a2 = np.maximum(0.5 * np.abs(e1[:, 0] * e2[:, 1] - e1[:, 1] * e2[:, 0]), 1e-12)
    per = np.sqrt(a3 / a2) / SIZE
    rasterize_object.stretch = np.where(tri >= 0, per[np.maximum(tri, 0)], 0.0)
    return img, mask


class BakedSet:
    """A set baked from its looks: colour (sRGB 0..1), roughness, metal, occlusion, normal (DirectX -1..1), rows top
    first, and what's painted over it (height in metres for the detail normal)."""

    def __init__(self, name, paths):
        im = paths['images']
        self.name = name
        self.color = _top(im['COLOR'])[..., :3].astype(np.float64)
        self.rough = _top(im['ROUGH'])[..., 0].astype(np.float64)
        self.metal = _top(im['METAL'])[..., 0].astype(np.float64)
        orm = _top(im['ORM'])
        self.ao = orm[..., 0].astype(np.float64)
        n = _top(im['NORMAL'])[..., :3].astype(np.float64) * 2.0 - 1.0
        n[..., 1] = -n[..., 1]                              # OpenGL (Blender's bake) to DirectX
        # The looks' bumps never tilt past ~40 degrees; a texel baked past 60 is a face the decimation folded over
        # (its tangent frame turned against its neighbours'): flat there, not a dark smudge across the triangle.
        bad = n[..., 2] < 0.5
        if bad.any():
            n[bad] = (0.0, 0.0, 1.0)
            log(f'{name}: {int(bad.sum())} texels of folded faces baked flat')
        self.normal = n
        self.height = np.zeros(self.rough.shape)
        self.height_mask = np.zeros(self.rough.shape, bool)
        self.px_m = None

    def adjust(self, objs, normal=1.0, soften=0.0, rough_add=0.0):
        """Fits the third-person look to first-person distance, before the hand's detail goes on: its bumps scaled
        (normal: the share kept; the looks' creases were made to read across a room, and a hand a foot from the eye
        wore them as crazed plastic), its colour's finest lines softened toward a blur within the set's own islands
        (soften: how much), its roughness raised (rough_add)."""
        mask = np.zeros(self.rough.shape, bool)
        for obj in objs:
            me = obj.data
            co = np.array([v.co[:] for v in me.vertices])
            _, m = rasterize_object(obj, co)
            mask |= m
        if normal != 1.0:
            n = self.normal.copy()
            n[..., :2] *= normal
            n /= np.linalg.norm(n, axis=-1, keepdims=True)
            self.normal = np.where(mask[..., None], n, self.normal)
        if soften > 0.0:
            m = mask.astype(np.float32)
            num = lt.blur((self.color * m[..., None]).astype(np.float32), 2.2)
            den = lt.blur(m, 2.2)[..., None]
            soft = np.where(den > 1e-3, num / np.maximum(den, 1e-3), self.color)
            self.color = np.where(mask[..., None], self.color * (1.0 - soften) + soft * soften, self.color)
        if rough_add:
            self.rough = np.where(mask, self.rough + rough_add, self.rough)
        log(f'{self.name}: bumps x{normal:.2f}, colour softened {soften:.2f}, roughness {rough_add:+.2f}')

    def save(self, strength=1.0):
        h = self.height
        if self.height_mask.any() and self.px_m:
            # spread the painted height past its islands' edges (no false ridge where it meets the unpainted)
            filled = lt.fill_transparent(np.stack([h, h, h], -1).astype(np.float32),
                                         self.height_mask.astype(np.float32))[..., 0]
            filled = np.where(self.height_mask, h, filled)
            n = _blend_normals(self.normal, _dx_normal(filled, self.px_m, strength))
        else:
            n = self.normal
        normal = lt.to8(n * 0.5 + 0.5)
        lt.save_set(self.name, np.clip(self.color, 0, 1), np.zeros_like(self.rough), self.px_m or 1e-4,
                    np.clip(self.rough, 0.04, 1.0), np.clip(self.metal, 0, 1), ao=np.clip(self.ao, 0, 1),
                    normal=normal)


def bake_sets(sets, all_objects, digest_path, sources, force=False, keep=False):
    """sets: [(set name, [objects])] (each object wearing its looks, UVs made). Packs each set's atlas (always: the UVs
    must match the files), then bakes them all raised, or keeps the files when the digest matches. Returns ({name:
    BakedSet}, the digest to write once they're painted and saved), or (None, None) when the files were kept."""
    for name, objs in sets:
        bk.pack(objs, margin=0.006)
    want = digest_of(sources, SIZE)
    have = open(digest_path).read().strip() if os.path.exists(digest_path) else ''
    files = all(os.path.exists(lt.texture_path(n, k)) for n, _ in sets for k in ('BC', 'N', 'ORM'))
    if files and (keep or (want == have and not force)):
        log(f'textures kept ({"keep-textures" if keep else "digest matches"}): {", ".join(n for n, _ in sets)}')
        return None, None
    _raise(all_objects, RAISE)
    out = {}
    try:
        for name, objs in sets:
            paths = bk.bake_set(name, objs, size=SIZE, samples=1, ao_samples=32, ao_distance=0.06, margin=12)
            out[name] = BakedSet(name, paths)
    finally:
        _raise(all_objects, -RAISE)
    return out, (digest_path, want)


def write_digest(stamp):
    path, want = stamp
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as fh:
        fh.write(want + '\n')


def mix(a, b, t):
    t = np.asarray(t, dtype=np.float64)[..., None]
    return a * (1.0 - t) + np.asarray(b, dtype=np.float64) * t


# ------------------------------------------------------------------------------------------------ the hands' paint
def paint_hand(bs, obj, co, nrm, spec, style):
    """Paints a hand's detail over its baked look (bs: BakedSet), from fp_arms_hands.hand_fields. style: a dict --
    glove (True: seams, points; False: bare skin, nails), thread (sRGB), scuff (sRGB, how much), cavity (how dark the
    creases go), polish (how much the palm's roughness drops), nail colours (nail, free, lunula, fold), nail_rough,
    knuckle (sRGB multiplier, how much), dirt (sRGB, how much), crease_tint (sRGB multiplier in the creases)."""
    from fp_arms_hands import hand_fields
    img, mask = rasterize_object(obj, co, nrm)
    uv, idx, wco, _ = mesh_corners(obj)
    px_m = texel_size(uv, idx, co)
    bs.px_m = px_m if bs.px_m is None else min(bs.px_m, px_m)
    T = img[mask].astype(np.float64)
    P, N = T[:, 0:3], T[:, 3:6]
    N = N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-9)
    glove = style.get('glove', True)
    f = hand_fields(P, N, spec, glove=glove, nails=not glove)
    log(f'{bs.name}: hand detail on {mask.mean() * 100:.0f}% of the atlas, {px_m * 1000:.3f} mm a texel')
    col = bs.color[mask]
    rough = bs.rough[mask]
    cav = np.clip(f['cavity'], 0, 1)
    if glove:
        sc = np.clip(f['scuff'] * (0.55 + 0.45 * A['fbm3'](P, 0.004, 11, 2) * 2.0), 0, 1)
        col = mix(col, style['scuff'], sc * style.get('scuff_amount', 0.35))
        col = mix(col, style['thread'], np.clip(f['thread'], 0, 1))
        col = col * (1.0 - style.get('cavity', 0.45) * cav)[:, None]
        col = mix(col, col * 1.25 + 0.01, np.clip(f['ridges'], 0, 1) * 0.3)
        po = np.clip(f['polish'] * (0.6 + 0.4 * A['fbm3'](P, 0.008, 17, 2)), 0, 1)
        rough = rough - style.get('polish', 0.12) * po + 0.12 * sc + 0.08 * cav
        rough = np.where(f['thread'] > 0.5, style.get('thread_rough', 0.5), rough)
        height = f['height'] + f['thread_h'] * f['thread']
    else:
        tint = np.asarray(style.get('crease_tint', (0.78, 0.7, 0.68)))
        col = col * (1.0 - cav[:, None] * (1.0 - tint) * style.get('cavity', 1.0))
        col = mix(col, col * np.asarray(style['knuckle'][0]), np.clip(f['knuckle'], 0, 1) * style['knuckle'][1])
        col = mix(col, col * 1.12 + 0.012, np.clip(f['ridges'], 0, 1) * 0.25)
        if style.get('dirt'):
            dc, amt = style['dirt']
            grime = np.clip(cav * 1.4 + f['fold'] * 0.8, 0, 1) * (0.5 + 0.5 * np.clip(A['fbm3'](P, 0.006, 31, 2) * 2 + 0.3,
                                                                                    0, 1))
            col = mix(col, dc, grime * amt)
        # the nails: the plate over the bed (its colour through it), the paler lunula, the free edge, dirt under it
        nail = np.clip(f['nail'], 0, 1)
        nc = mix(np.tile(np.asarray(style['nail'], np.float64), (len(col), 1)), style['lunula'], f['lunula'])
        nc = mix(nc, style['free'], np.clip(f['free'], 0, 1))
        streak = 0.5 + 0.5 * np.sin(P[:, 0] * 4200.0 + P[:, 1] * 300.0) * np.sin(P[:, 1] * 2600.0)
        nc = nc * (0.94 + 0.06 * streak)[:, None]
        col = mix(col, nc, nail)
        col = mix(col, style.get('fold_color', col * 0.8), np.clip(f['fold'], 0, 1) * 0.5)
        rough = rough + 0.06 * cav
        rough = rough * (1.0 - nail) + style.get('nail_rough', 0.32) * nail
        height = f['height']
    # where a face is squeezed in the atlas (over twice the metres a texel) the fine detail fades out, not aliases
    ratio = rasterize_object.stretch[mask] / px_m
    keep = 1.0 - np.clip((ratio - 1.8) / 1.2, 0.0, 1.0)
    bs.color[mask] = mix(bs.color[mask], col, keep)
    bs.rough[mask] = bs.rough[mask] * (1.0 - keep) + rough * keep
    bs.ao[mask] = bs.ao[mask] * (1.0 - 0.3 * cav * keep)
    bs.height[mask] = height * 0.001 * keep
    bs.height_mask |= mask


def paint_cloth(bs, objs, weave=1.0, seed=5):
    """A fine weave and the fibres' fuzz over a cloth set's bake (image space: the sleeve's islands run round the arm
    and along it, so the weave follows the cloth's grain), not on metal."""
    for obj in objs:
        me = obj.data
        co = np.array([v.co[:] for v in me.vertices])
        img, mask = rasterize_object(obj, co)
        uv, idx, wco, _ = mesh_corners(obj)
        px_m = texel_size(uv, idx, co)
        bs.px_m = px_m if bs.px_m is None else min(bs.px_m, px_m)
        bs.height_mask |= mask
    size = bs.rough.shape[0]
    rows, cols = np.mgrid[0:size, 0:size]
    tw = np.sin((cols + rows) * (2.0 * np.pi / 3.3)) * 0.5 + 0.5
    tw2 = np.sin((cols - rows * 0.2) * (2.0 * np.pi / 9.0))
    fuzz = lt.noise((size, size), seed, 1.2)
    cloth = bs.height_mask & (bs.metal < 0.5)
    bs.height = np.where(cloth, bs.height + weave * (tw * 0.000022 + tw2 * 0.000008 + fuzz * 0.000006), bs.height)
    bs.color = np.where(cloth[..., None], bs.color * (1.0 - 0.035 * weave * tw)[..., None], bs.color)
