"""The Gravemother from the user's Tripo model (Art/Models/Creatures/Tripo/Gravemother.glb, copied from
Art/Concepts/Creatures/Gravemother/Gravemother.glb; the paid plan): reading its shape. Not a model: Tools\\models.ps1
skips parts_*.py; Gravemother.py and GravemotherStone.py run parts_gravemother.py (LOOTER_GRAVEMOTHER_TRIPO, default
on), which builds on this.

The GLB is one shell (14.6k vertices once its UV seams are merged, 29k triangles; Tripo's units, about 100 across the
legs), its front -Y. It has SIX legs (three pairs: the rig needs eight, so the middle leg is used twice), two short palps,
two chelicerae with fangs, milky eyes, a domed cephalothorax and a big round abdomen carrying an arched headstone (a
carved cross) with two iron grave railings behind it, two skulls at its foot and root fibres over the hide.
- Source: the mesh in creature axes (x forward, y left, z up; Tripo's units), its thickness at every vertex (a ray in
  along the normal) and every vertex's colour in Tripo's map.
- segment(): labels. The stone is the slab standing out of the abdomen (its top found above STONE_TOP, its frame by
  PCA); the legs, palps and chelicerae are the branches off the thick body, each grown from its tip along the surface
  to where its slices stop being a leg's (leg_line); the railings are dropped; the abdomen is what lies behind the
  pedicel (the narrowest slice between it and the cephalothorax).
Everything is numpy; distances along the surface are Dijkstra over the edges."""
import heapq
import math
import os

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo as tripo

GLB = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'Gravemother.glb')
THICK = 6.0           # Tripo units: thicker than this (a ray in along the normal) is the body
TIP_REACH = 38.0      # a leg's tip lies farther than this off the body's middle (across the ground)
STONE_TOP = 47.0      # only the headstone rises above this
# Labels.
LEGS = range(6)       # 0..5: 2 * pair + side (pairs 0 front, 1 middle, 2 back; side 0 left, 1 right)
CHEL_L, CHEL_R, PALP_L, PALP_R, CLAW_L, CLAW_R = 10, 11, 12, 13, 14, 15
THORAX, ABDOMEN, STONE, RAILING, SKULL = 20, 21, 22, 23, 24
# The skulls on her back at the stone's foot (creature axes, Tripo units: centre, radius), from the ortho views.
SKULLS = (((-9.9, -4.3, 37.0), 2.4), ((-9.9, 12.2, 35.8), 2.4))


def log(message):
    print(f'GRAVEMOTHER TRIPO: {message}', flush=True)


def neighbours(V, F):
    edges = set()
    for a, b, c in F:
        for p, q in ((a, b), (b, c), (c, a)):
            edges.add((min(p, q), max(p, q)))
    nb = [[] for _ in range(len(V))]
    for a, b in sorted(edges):
        length = float(np.linalg.norm(V[a] - V[b]))
        nb[a].append((b, length))
        nb[b].append((a, length))
    return nb


def dijkstra(nb, sources, allowed=None):
    """Distances along the surface from the source vertices through allowed ones (all when None)."""
    n = len(nb)
    dist = np.full(n, np.inf)
    heap = []
    for s in sources:
        dist[s] = 0.0
        heap.append((0.0, int(s)))
    heapq.heapify(heap)
    while heap:
        d, v = heapq.heappop(heap)
        if d > dist[v]:
            continue
        for w, length in nb[v]:
            if (allowed is None or allowed[w]) and d + length < dist[w]:
                dist[w] = d + length
                heapq.heappush(heap, (d + length, w))
    return dist


def components(nb, keep):
    comp = -np.ones(len(nb), int)
    count = 0
    for s in range(len(nb)):
        if not keep[s] or comp[s] >= 0:
            continue
        stack = [s]
        comp[s] = count
        while stack:
            v = stack.pop()
            for w, _ in nb[v]:
                if keep[w] and comp[w] < 0:
                    comp[w] = count
                    stack.append(w)
        count += 1
    return comp, count


def vertex_normals(V, F):
    N = np.zeros_like(V)
    n = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
    for k in range(3):
        np.add.at(N, F[:, k], n)
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


class Source:
    """The Tripo model in creature axes: V (n x 3), F (m x 3), N, thickness T, colour C (sRGB per vertex), its corner
    UVs (m x 3 x 2) and the colour map's pixels (for sampling at any UV)."""

    def __init__(self):
        obj, maps = tripo.load(GLB)
        me = obj.data
        me.calc_loop_triangles()
        G = np.empty(len(me.vertices) * 3)
        me.vertices.foreach_get('co', G)
        G = G.reshape(-1, 3)
        self.V = np.column_stack([-G[:, 1], G[:, 0], G[:, 2]])          # Blender (left +X, front -Y) to creature axes
        tris = np.empty(len(me.loop_triangles) * 3, dtype=np.int64)
        me.loop_triangles.foreach_get('vertices', tris)
        loops = np.empty(len(me.loop_triangles) * 3, dtype=np.int64)
        me.loop_triangles.foreach_get('loops', loops)
        self.F = tris.reshape(-1, 3)
        uv = np.empty(len(me.loops) * 2)
        me.uv_layers.active.data.foreach_get('uv', uv)
        uv = uv.reshape(-1, 2)
        self.UV = uv[loops].reshape(-1, 3, 2)
        first = np.zeros((len(self.V), 2))
        first[self.F.ravel()[::-1]] = self.UV.reshape(-1, 2)[::-1]
        self.pixels = tripo.pixels(maps['BC'], 2048)[..., :3]
        h, w = self.pixels.shape[:2]
        rows = np.clip(((1.0 - first[:, 1] % 1.0) * h).astype(int), 0, h - 1)
        cols = np.clip(((first[:, 0] % 1.0) * w).astype(int), 0, w - 1)
        self.C = self.pixels[rows, cols]
        self.N = vertex_normals(self.V, self.F)
        self.tree = BVHTree.FromPolygons(self.V.tolist(), self.F.tolist(), all_triangles=True)
        self.T = np.full(len(self.V), 60.0)
        for i, (p, n) in enumerate(zip(self.V, self.N)):
            hit = self.tree.ray_cast(Vector(p - n * 1e-3), Vector(-n), 60.0)
            if hit[0] is not None:
                self.T[i] = hit[3]
        self.nb = neighbours(self.V, self.F)
        bpy.data.objects.remove(obj)
        for image in list(maps.values()):
            if image is not None and image.users == 0:
                bpy.data.images.remove(image)
        log(f'{len(self.V)} vertices, {len(self.F)} triangles')

    def colour_at(self, tri, bary):
        """Tripo's colour (sRGB) at points on its triangles (barycentric weights)."""
        uv = (self.UV[tri] * bary[:, :, None]).sum(1)
        return tripo_sample(self.pixels, uv)


def tripo_sample(px, uv):
    h, w = px.shape[:2]
    rows = np.clip(((1.0 - uv[:, 1] % 1.0) * h).astype(int), 0, h - 1)
    cols = np.clip(((uv[:, 0] % 1.0) * w).astype(int), 0, w - 1)
    return px[rows, cols]


# --- The headstone ---

def stone_frame(src):
    """The slab's frame from its top (the vertices above STONE_TOP): centre, its axes (width, thickness, up: unit
    vectors, up pointing up, thickness toward her head) and its half sizes along width and thickness."""
    top = src.V[src.V[:, 2] > STONE_TOP]
    c = top.mean(0)
    _, _, vt = np.linalg.svd(top - c)
    thin = vt[2] * (1.0 if vt[2][0] > 0 else -1.0)            # out of the carved face, toward her head (+x)
    width = vt[0] if abs(vt[0][1]) > abs(vt[1][1]) else vt[1]   # across her (y)
    width = width * (1.0 if width[1] > 0 else -1.0)
    up = np.cross(thin, width)
    up = up * (1.0 if up[2] > 0 else -1.0)
    width = np.cross(up, thin)
    rel = top - c
    half_w = np.abs(rel @ width).max()
    half_t = np.abs(rel @ thin).max()
    return c, (width, thin, up), half_w, half_t


def segment(src):
    """Labels every vertex (the module's labels). Returns (labels, info): info has the legs' tips, the stone's frame and
    the pedicel's x."""
    V, nb = src.V, src.nb
    labels = np.full(len(V), THORAX)
    info = {}
    # The headstone: inside its slab's box above where it leaves her hide.
    c, (wx, th, up), half_w, half_t = stone_frame(src)
    rel = V - c
    in_box = (np.abs(rel @ wx) < half_w + 0.6) & (np.abs(rel @ th) < half_t + 0.6)
    # Its foot: going down the slab from under its arch, the first slice where a third of what the box holds faces up
    # (her hide coming in: the slab's faces and sides stand square to up).
    h = rel @ up
    foot = h[in_box].min()
    start = h[in_box].max() - half_w - 1.0
    for level in np.arange(start, h[in_box].min() - 1.0, -0.5):
        sl = in_box & (h > level - 0.5) & (h <= level)
        if sl.sum() < 4:
            continue
        if (src.N[sl] @ up > 0.6).mean() > 0.3:
            foot = level + 0.5
            break
    stone = in_box & (h > foot)
    labels[stone] = STONE
    info['stone'] = dict(centre=c, axes=(wx, th, up), half_w=half_w, half_t=half_t, foot=foot)
    log(f'headstone: {int(stone.sum())} vertices, {2 * half_w:.1f} x {2 * half_t:.1f} units, its foot {foot:.1f} under '
        f'its top\'s middle, leaning {math.degrees(math.atan2(-(up @ np.array([1.0, 0.0, 0.0])), up[2])):.1f} deg back')
    # The legs: from each tip (far off the body across the ground) along the surface to where it joins the body.
    body = src.T >= THICK
    far = np.hypot(V[:, 0] + 2.0, V[:, 1]) > TIP_REACH
    comp, count = components(nb, far & (labels == THORAX))
    tips = []
    for k in range(count):
        ids = np.nonzero(comp == k)[0]
        if len(ids) < 40:
            continue
        tips.append(int(ids[np.argmax(np.hypot(V[ids, 0] + 2.0, V[ids, 1]))]))
    order = {}
    for side, sign in ((0, 1.0), (1, -1.0)):
        mine = sorted([t for t in tips if np.sign(V[t, 1]) == sign], key=lambda t: -V[t, 0])
        for pair, t in enumerate(mine):
            order[2 * pair + side] = t
    info['tips'] = order
    info['lines'] = {}
    for leg, tip in sorted(order.items()):
        line, members = leg_line(src, tip, labels == THORAX)
        labels[members] = leg
        info['lines'][leg] = line
    log(f'{len(order)} legs (tips {[tuple(np.round(V[t], 1)) for _, t in sorted(order.items())]})')
    # The chelicerae and palps: the thin branches left at the front.
    for label, tip_at in ((PALP_L, (19.4, 9.7, 3.2)), (PALP_R, (19.4, -9.7, 3.2)), (CHEL_L, (16.5, 2.2, 8.5)),
                          (CHEL_R, (16.5, -2.2, 8.5))):
        d = np.linalg.norm(V - np.array(tip_at), axis=1)
        tip = int(np.argmin(np.where(labels == THORAX, d, np.inf)))
        line, members = leg_line(src, tip, labels == THORAX, least=1.8 if label in (PALP_L, PALP_R) else 2.6)
        labels[members] = label
        info['lines'][label] = line
    # A second, thinner pair of claws hangs between each palp and chelicera: the lowest points left at the front.
    for label, sign in ((CLAW_L, 1.0), (CLAW_R, -1.0)):
        free = (labels == THORAX) & (V[:, 0] > 12.0) & (V[:, 1] * sign > 0.5)
        tip = int(np.argmin(np.where(free, V[:, 2], np.inf)))
        line, members = leg_line(src, tip, labels == THORAX, least=1.8)
        labels[members] = label
        info['lines'][label] = line
        log(f'claw {label}: tip {np.round(V[tip], 1)}, {line[-1, 0]:.1f} long, {len(members)} vertices')
    # The railings (dropped) and the skulls: thin things standing on her back beside the stone.
    for (centre, r) in SKULLS:
        labels[(np.linalg.norm(V - np.array(centre), axis=1) < r) & (labels == THORAX)] = SKULL
    thin = (labels == THORAX) & ~body & (V[:, 2] > 30.0) & (V[:, 0] < 2.0)
    comp, count = components(nb, thin)
    for k in range(count):
        ids = np.nonzero(comp == k)[0]
        if len(ids) > 30 and np.ptp(V[ids, 2]) > 3.0:
            labels[ids] = RAILING
    # The pedicel: the narrowest slice of the body (its half width) between the cephalothorax and the abdomen.
    rest = labels == THORAX
    best = None
    for x in np.arange(-4.0, 3.0, 0.25):
        sl = rest & (np.abs(V[:, 0] - x) < 0.3)
        if sl.sum() < 8:
            continue
        r = np.abs(V[sl, 1]).max()
        if best is None or r < best[0]:
            best = (r, x)
    info['pedicel'] = best[1]
    labels[rest & (V[:, 0] < best[1])] = ABDOMEN
    log(f'pedicel at x {best[1]:.2f} (radius {best[0]:.1f}); railings {int((labels == RAILING).sum())} vertices, '
        f'skulls {int((labels == SKULL).sum())}')
    return labels, info


def leg_line(src, tip, allowed, step=0.6, least=2.6):
    """A branch's centre line from its tip back to the body: slices along the surface from the tip; the branch ends
    where a slice's radius jumps past both least and 1.3 times the widest of the few before it, and the next grows on
    (the body's flare; a leg's own taper and its noisy slices never do both). Returns (stations tip first: (n x 5:
    distance along the surface, centre xyz, radius)), member vertices)."""
    d = dijkstra(src.nb, [tip], allowed)
    rows = []
    for k in range(400):
        ring = np.nonzero((d >= k * step) & (d < (k + 1) * step))[0]
        if len(ring) < 3:
            if rows and k * step > rows[-1][0] + 4 * step:
                break
            continue
        cen = src.V[ring].mean(0)
        rad = np.linalg.norm(src.V[ring] - cen, axis=1).mean()
        rows.append((k * step, *cen, rad))
    if len(rows) < 8:
        raise RuntimeError(f'leg_line: the branch from vertex {tip} at {np.round(src.V[tip], 1)} has only {len(rows)} slices')
    rows = np.array(rows)
    # The slices' radii, a running median of three (single slices are noisy), compared with the radius 4 units back.
    raw = rows[:, 4]
    radius = np.array([np.median(raw[max(k - 1, 0):k + 2]) for k in range(len(raw))])
    back = int(round(4.0 / step))
    end = None
    for k in range(back, len(radius) - 2):
        if radius[k] > max(least, 1.5 * radius[k - back]) and radius[k + 2] > radius[k]:
            end = k
            break
    if end is None:
        raise RuntimeError(f'leg_line: the branch from {np.round(src.V[tip], 1)} never meets the body')
    # Cut where the flare starts: 1.8 units before it was found.
    cut = rows[end, 0] - 1.8
    keep = rows[:, 0] <= cut
    return rows[keep], np.nonzero(d < cut)[0]
