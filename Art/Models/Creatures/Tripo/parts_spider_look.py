"""The Meadow Wolf's texture set from the user's Tripo paint (Art/Textures/SpiderTripo; MI_SpiderBody on the World
master): parts_spider_build.py runs it once the parts are built. Not a model (Tools\\models.ps1 skips parts_*.py).

Why (the user, 2026-10-31: the spider "didn't import right", textures and colours flat): the Tripo build kept the
scripted spider's painted atlas (BROWN: near-black with pale bands), so the game showed a grey-black striped spider, not
the user's warm tan wolf spider with its chevroned abdomen, rayed carapace and speckled, dark-ringed legs. Now Tripo's
own paint is carried onto our parts, texel by texel:
  - each part has a template: the same faces and atlas UVs, its vertices where they sat on Tripo's surface before the
    rig's fit moved them (the shells are already there; a leg's rings round Tripo's own centre line, unstretched);
  - every atlas texel a template face covers takes the point on Tripo's surface nearest its place there, and Tripo's
    colour, roughness and normal map at that point (its 4K maps). The normal is turned from Tripo's tangent frame into
    the template's, so Tripo's modelled fur and plate relief stays on our UVs (DirectX green, as the World master reads).
  - The eight legs share one femur, coxa and tibia region (their UVs run joint to joint, so every leg's rings fall on
    its joints): one leg's paint (REP_LEG) fills them. The palps and chelicerae likewise (the left ones).
  - Eyes, bristles, fangs and the spinnerets keep the atlas's small solid patches (SOLID).
The atlas layout (Spider.py's ATLAS) is unchanged, so the Gravemother's pale hide and the Spitter's fallback, which use
the old SpiderBody set, are untouched; the old set is still painted for them."""
import math
import os

import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo as tripo

SET = 'SpiderTripo'
MATERIAL = 'SpiderBody'          # the slot keeps its name: MI_SpiderBody, now on this set
SIZE = 2048
SOURCE_SIZE = 4096               # Tripo's maps, read at full size
REP_LEG = 2                      # the left middle-front leg (2 * pair + side): fills the shared leg regions
# The small solid patches: (sRGB colour, roughness). Eyes glossy black; the rest matte, in the Tripo paint's darks.
SOLID = {'eye': (0x060505, 0.1), 'bristle': (0x2e2016, 0.8), 'fang': (0x1c130d, 0.45), 'misc': (0x5a4029, 0.82)}
NORMAL_STRENGTH = 1.0            # Tripo's own relief, as modelled
RELIEF = 0.35                    # plus the colour map's fine relief as a bump (fur strokes, specks), as looter_rebake does
METAL = 0.0                      # a spider has no metal: Tripo's metallic map is dropped
ROUGH_MAX = 0.88                 # Tripo paints it all but matte (0.96): a faint sheen on the plates reads less flat in sun


def log(message):
    print(f'SPIDER LOOK: {message}', flush=True)


def _srgb_to_linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def _linear_to_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1.0 / 2.4) - 0.055)


def _u8(image, size):
    px = tripo.pixels(image, size)[..., :3]
    return np.clip(px * 255.0 + 0.5, 0, 255).astype(np.uint8)


def _bilinear(img, uv):
    """img (rows top first, uint8) at UVs (n x 2, v up, wrapping), as floats 0..1."""
    h, w = img.shape[:2]
    x = (uv[:, 0] % 1.0) * w - 0.5
    y = (1.0 - uv[:, 1] % 1.0) * h - 0.5
    x0 = np.floor(x).astype(np.int64)
    y0 = np.floor(y).astype(np.int64)
    fx, fy = (x - x0)[:, None], (y - y0)[:, None]
    x0, x1 = x0 % w, (x0 + 1) % w
    y0, y1 = np.clip(y0, 0, h - 1), np.clip(y0 + 1, 0, h - 1)
    f = lambda yy, xx: img[yy, xx].astype(np.float32) / 255.0
    return (f(y0, x0) * (1 - fx) * (1 - fy) + f(y0, x1) * fx * (1 - fy) + f(y1, x0) * (1 - fx) * fy
            + f(y1, x1) * fx * fy)


def _tangents(P, UV):
    """Per triangle (P: n x 3 x 3 points, UV: n x 3 x 2): the tangent (along +u), bitangent (along +v), unnormalized."""
    e1, e2 = P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]
    d1, d2 = UV[:, 1] - UV[:, 0], UV[:, 2] - UV[:, 0]
    det = d1[:, 0] * d2[:, 1] - d2[:, 0] * d1[:, 1]
    det = np.where(np.abs(det) < 1e-12, 1e-12, det)
    T = (e1 * d2[:, 1:2] - e2 * d1[:, 1:2]) / det[:, None]
    B = (e2 * d1[:, 0:1] - e1 * d2[:, 0:1]) / det[:, None]
    return T, B


def _template_tris(mesh):
    """An lc.Mesh's faces as triangles: points (n x 3 x 3, its own space), atlas UVs (n x 3 x 2)."""
    uv_layer = mesh.uv
    P, UV = [], []
    for f in mesh.bm.faces:
        loops = list(f.loops)
        for k in range(1, len(loops) - 1):
            tri = (loops[0], loops[k], loops[k + 1])
            P.append([l.vert.co[:] for l in tri])
            UV.append([l[uv_layer].uv[:] for l in tri])
    return np.array(P, float), np.array(UV, float)


def _raster(UV, size):
    """The texels (rows top first) whose centres lie in each UV triangle: (triangle index, row, col, barycentrics)."""
    out_t, out_r, out_c, out_b = [], [], [], []
    px = np.stack([UV[..., 0] * size, (1.0 - UV[..., 1]) * size], -1)     # x = col, y = row
    for t, (a, b, c) in enumerate(px):
        lo = np.floor(np.minimum(np.minimum(a, b), c) - 0.5).astype(int)
        hi = np.ceil(np.maximum(np.maximum(a, b), c) + 0.5).astype(int)
        lo = np.clip(lo, 0, size - 1)
        hi = np.clip(hi, 0, size)
        if hi[0] <= lo[0] or hi[1] <= lo[1]:
            continue
        xs, ys = np.meshgrid(np.arange(lo[0], hi[0]) + 0.5, np.arange(lo[1], hi[1]) + 0.5)
        x, y = xs.ravel(), ys.ravel()
        den = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(den) < 1e-12:
            continue
        w0 = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / den
        w1 = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / den
        w2 = 1.0 - w0 - w1
        eps = -0.02            # a hair outside the edge too: no seam of empty texels between faces
        inside = (w0 >= eps) & (w1 >= eps) & (w2 >= eps)
        if not inside.any():
            continue
        out_t.append(np.full(inside.sum(), t))
        out_r.append((y[inside] - 0.5).astype(int))
        out_c.append((x[inside] - 0.5).astype(int))
        out_b.append(np.column_stack([w0[inside], w1[inside], w2[inside]]))
    if not out_t:
        return np.zeros(0, int), np.zeros(0, int), np.zeros(0, int), np.zeros((0, 3))
    return np.concatenate(out_t), np.concatenate(out_r), np.concatenate(out_c), np.concatenate(out_b)


class Look:
    """The atlas being filled: colour (linear), roughness, normal (OpenGL, unit) and which texels are known."""

    def __init__(self, src):
        self.src = src
        self.bc = np.zeros((SIZE, SIZE, 3), np.float32)
        self.rough = np.full((SIZE, SIZE), 0.8, np.float32)
        self.normal = np.zeros((SIZE, SIZE, 3), np.float32)
        self.normal[..., 2] = 1.0
        self.known = np.zeros((SIZE, SIZE), bool)
        maps = src.maps
        self.BC = _u8(maps['BC'], SOURCE_SIZE)
        self.RM = _u8(maps['RM'], SOURCE_SIZE) if maps.get('RM') is not None else None
        self.N = _u8(maps['N'], SOURCE_SIZE) if maps.get('N') is not None else None
        log(f'Tripo maps read at {SOURCE_SIZE} px (roughness {"yes" if self.RM is not None else "no"}, '
            f'normal {"yes" if self.N is not None else "no"})')

    def carry(self, name, mesh, V, labels_ok):
        """Fills the texels a part's template covers from Tripo's surface. mesh: the template (an lc.Mesh, its
        vertices in the space V holds Tripo's vertices in); labels_ok: which of Tripo's triangles it may take from."""
        src = self.src
        P, UV = _template_tris(mesh)
        tri, rows, cols, bary = _raster(UV, SIZE)
        if not len(tri):
            log(f'{name}: no texels')
            return
        idx = np.nonzero(labels_ok)[0]
        F = src.F[idx]
        tree = BVHTree.FromPolygons(V.tolist(), F.tolist(), all_triangles=True)
        place = np.einsum('nk,nkd->nd', bary, P[tri])
        hit_tri = np.empty(len(place), np.int64)
        hit_at = np.empty((len(place), 3))
        for i, p in enumerate(place):
            loc, _n, k, _d = tree.find_nearest(Vector(p))
            if k is None:
                hit_tri[i] = -1
                continue
            hit_tri[i] = k
            hit_at[i] = loc[:]
        ok = hit_tri >= 0
        tri, rows, cols, place, hit_tri, hit_at = tri[ok], rows[ok], cols[ok], place[ok], hit_tri[ok], hit_at[ok]
        # Tripo's UV and tangent frame where each texel's point lies.
        A, B, C = (V[F[hit_tri, k]] for k in range(3))
        v0, v1, v2 = B - A, C - A, hit_at - A
        d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
        d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
        den = np.where(np.abs(d00 * d11 - d01 * d01) < 1e-12, 1e-12, d00 * d11 - d01 * d01)
        b = (d11 * d20 - d01 * d21) / den
        c = (d00 * d21 - d01 * d20) / den
        w = np.column_stack([1.0 - b - c, b, c])
        tuv = src.UV[idx[hit_tri]]
        uv = np.einsum('nk,nkd->nd', w, tuv)
        colour = _srgb_to_linear(_bilinear(self.BC, uv))
        self.bc[rows, cols] = colour
        if self.RM is not None:
            self.rough[rows, cols] = _bilinear(self.RM, uv)[:, 1]
        if self.N is not None:
            n_ts = _bilinear(self.N, uv) * 2.0 - 1.0                               # OpenGL (glTF): +y up the texture
            Ts, Bs = _tangents(np.stack([A, B, C], 1), tuv)
            Ns = np.cross(B - A, C - A)
            Ns /= np.maximum(np.linalg.norm(Ns, axis=1, keepdims=True), 1e-12)
            Ns_v = self.vertex_normals(V, F)
            Nsm = np.einsum('nk,nkd->nd', w, np.stack([Ns_v[F[hit_tri, k]] for k in range(3)], 1))
            Nsm /= np.maximum(np.linalg.norm(Nsm, axis=1, keepdims=True), 1e-12)
            Nsm = np.where((Nsm * Ns).sum(1, keepdims=True) < 0, Ns, Nsm)
            Ts = Ts - Nsm * (Ts * Nsm).sum(1, keepdims=True)
            Ts /= np.maximum(np.linalg.norm(Ts, axis=1, keepdims=True), 1e-12)
            Bh = np.sign((np.cross(Nsm, Ts) * Bs).sum(1, keepdims=True))
            Bs = np.cross(Nsm, Ts) * np.where(Bh == 0, 1.0, Bh)
            n_obj = Ts * n_ts[:, :1] * NORMAL_STRENGTH + Bs * n_ts[:, 1:2] * NORMAL_STRENGTH + Nsm * n_ts[:, 2:3]
            n_obj /= np.maximum(np.linalg.norm(n_obj, axis=1, keepdims=True), 1e-12)
            # Into the template's frame: its tangent along +u, its bitangent along +v, about Tripo's smooth normal.
            To, Bo = _tangents(P, UV)
            To, Bo = To[tri], Bo[tri]
            To = To - Nsm * (To * Nsm).sum(1, keepdims=True)
            To /= np.maximum(np.linalg.norm(To, axis=1, keepdims=True), 1e-12)
            hand = np.sign((np.cross(Nsm, To) * Bo).sum(1, keepdims=True))
            Bo = np.cross(Nsm, To) * np.where(hand == 0, 1.0, hand)
            n = np.column_stack([(n_obj * To).sum(1), (n_obj * Bo).sum(1), (n_obj * Nsm).sum(1)])
            n[:, 2] = np.maximum(n[:, 2], 0.05)
            n /= np.linalg.norm(n, axis=1, keepdims=True)
            self.normal[rows, cols] = n
        self.known[rows, cols] = True
        off = np.linalg.norm(place - hit_at, axis=1)
        log(f'{name}: {len(rows)} texels from Tripo, the template {np.median(off):.2f} cm off its surface '
            f'(95% within {np.percentile(off, 95):.2f})')

    _normals = {}

    def vertex_normals(self, V, F):
        key = (id(V), len(F))
        if key not in self._normals:
            N = np.zeros_like(V)
            n = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
            for k in range(3):
                np.add.at(N, F[:, k], n)
            self._normals[key] = N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
        return self._normals[key]

    def solids(self, atlas):
        for region, (colour, rough) in SOLID.items():
            u0, u1, v0, v1 = atlas[region]
            r0, r1 = int(round((1.0 - v1) * SIZE)), int(round((1.0 - v0) * SIZE))
            c0, c1 = int(round(u0 * SIZE)), int(round(u1 * SIZE))
            self.bc[r0:r1, c0:c1] = np.array(lt.hex_color(colour)[:3], np.float32)      # linear
            self.rough[r0:r1, c0:c1] = rough
            self.normal[r0:r1, c0:c1] = (0.0, 0.0, 1.0)
            self.known[r0:r1, c0:c1] = True

    def write(self):
        """Dilates into the gutters and writes T_SpiderTripo_BC/_N/_ORM; returns the material."""
        import looter_rebake as rb
        known = self.known
        bc = rb._dilate(self.bc, known, steps=24)
        rough = rb._dilate(self.rough[..., None], known, steps=24)[..., 0]
        n = rb._dilate(self.normal, known, steps=24)
        n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
        # The colour's fine relief as a bump on top (the fur strokes and specks Tripo painted but modelled softly).
        luma = bc @ np.array([0.2126, 0.7152, 0.0722], np.float32)
        height = luma - lt.blur(luma, 3.0)
        gx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
        gy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
        n = n + np.stack([-gx * RELIEF * 8.0, gy * RELIEF * 8.0, np.zeros_like(gx)], -1)
        n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
        n[..., 1] = -n[..., 1]                                            # DirectX: green down
        # Occlusion: the crevices Tripo painted dark (its colour well under its neighbourhood's), softly.
        dark = np.clip((lt.blur(luma, 6.0) - luma) / np.maximum(lt.blur(luma, 6.0), 1e-3), 0.0, 1.0)
        occ = 1.0 - 0.5 * np.clip(dark * 2.0, 0.0, 1.0)
        orm = np.stack([occ, np.clip(rough, 0.35, ROUGH_MAX), np.full_like(rough, METAL)], -1)
        bc_path, orm_path, n_path = (lt.texture_path(SET, k) for k in ('BC', 'ORM', 'N'))
        os.makedirs(os.path.dirname(bc_path), exist_ok=True)
        lt.write_png(bc_path, lt.to8(_linear_to_srgb(bc)))
        lt.write_png(orm_path, lt.to8(orm))
        lt.write_png(n_path, lt.to8(n * 0.5 + 0.5))
        log(f'{SET}: wrote T_{SET}_BC, _ORM, _N ({SIZE} px), {known.mean() * 100:.0f}% of the atlas from Tripo; '
            f'roughness median {np.median(rough[known]):.2f}')
        return lt.material(SET, name=MATERIAL, master='World')


def material():
    """MI_SpiderBody on the Tripo set, as written (an export after a check run reuses the maps)."""
    return lt.material(SET, name=MATERIAL, master='World')
