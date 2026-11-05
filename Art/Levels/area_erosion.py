"""Erosion of an area's heights (layout.json "erosion"; numpy only, so it runs in Blender's Python): the last step of
Area.build() (area_shape.py), after the features, the boundary's rock and the seam band. An area without an "erosion"
block, or with "enabled": false, builds exactly as before (the tutorial island and TerrainTest have none).

The method, in passes from coarse to fine (erosion.passes: each its own working grid of "cell" cm, nesting in the
height raster's cells, with its own settings over the block's; Ransom's Rest: 2 m for the ravines and fans, then 1 m
for the gullies' detail; each pass starts from the ground the passes before it left):
- roughness (a pass's "roughness": amplitude, wavelength cm): seeded noise of three octaves added where erosion may
  work: the hollows of several sizes the rain gathers in, so gullies branch and differ instead of combing a smooth
  face into even parallel rills (the faces the generator makes are smooth, extruded profiles);
- drainage, every iteration: rain (seeded noise, "rain") runs downhill, each cell sending its water to one lower
  neighbour picked at random with weight slope^spread (stochastic D8: over the iterations the water spreads like
  multiple-flow routing, so fans fan out); the water a cell gathers (A, m2 of rain) by doubling along the receivers,
  its depth in their tree by binary lifting (numpy, no loop over cells);
- incision: stream power solved implicitly up each receiver tree (Braun and Willett 2013): new h = (h + F new h_r) /
  (1 + F), F = incision x erodibility x A'^areaExponent / L, stable at any strength; A' is the water past "threshold"
  m2 (a channel's head: sheet wash on a hillslope doesn't cut), easing off past "saturate" m2 (a big catchment's water
  spreads over a wider bed); rock (steeper than thermal.rock) wears at rockWear of that;
- deposition: the eroded soil routed down each tree; a cell keeps settle x what it gets beyond its capacity
  transport x A^transportExponent x S, so fans and aprons build where a gully meets gentler ground and pits fill;
- creep (m2 an iteration): a little diffusion, soft ridgelines and rounded gully walls;
- thermal: ground steeper than the angle of repose (thermal.repose) toward a lower neighbour slumps rate x half its
  largest excess onto those neighbours (talus, scree at the feet of steep faces, softened crests); over "rock" holds;
- every change times the erodibility (the protection weight on the pass's grid), at most maxStep cm an iteration, the
  pass never more than its maxCut / maxFill (cm) from where it started.
Then the shape is held (erosion.hold): the change's broad part (its blur over "reach" cm) is kept within "keep" cm, so
the faces keep the profiles and skylines the level was built and approved with, and what erosion leaves is its own
relief (the gullies, fans, talus, softened crests); the whole within the block's maxCut / maxFill. The change (on the
last pass's grid) is resampled onto the height raster and applied times the protection weight (area_erosion_masks.py):
  h' = h + w dh,  w = 0 exactly inside every mask (those cells keep their heights bit for bit), easing to 1 over
  "feather" cm outside them, and fading to 0 across the seam band, so the core still meets the surround ring.

Kept on the area (area.erosion): the weight (full raster); on the last pass's grid the change, the applied change
(the weight's share), cut, fill, its relief (the applied change against its 6 m mean), the flow (mean log10 A over
the last pass's last third of iterations), the erodibility and the masks' labels (a bit per area_erosion_masks.MASKS
name). area_macro.py paints from them
(erosion.paint), area_mesh.py densifies where it carved (erosion.mesh), area_erosion_maps.py writes the towns' flow and
low-point maps (erosion.maps). Deterministic: every random draw comes from erosion.seed, in a fixed order.
"""
import math
import time

import numpy as np

from area_erosion_masks import OFFSETS, protection, upsample
from area_math import Grid, blur, fbm_raster

DEFAULTS = {
    'enabled': True,
    'seed': 1,
    'strength': 1.0,        # scales every pass's incision
    'iterations': 120,
    'rain': {'amount': 1.0, 'variation': 0.4, 'wavelength': 7000},   # wavelength cm
    'roughness': {'amplitude': 0, 'wavelength': 2500},                 # cm
    # incision K on A'^areaExponent (per iteration); threshold, saturate m2; transport Kc on A^transportExponent x
    # slope; spread: the receiver's pick weight, slope^spread; creep m2 per iteration; maxStep cm per iteration.
    'hydraulic': {'incision': 0.0002, 'areaExponent': 1.0, 'threshold': 0.0, 'saturate': 1500.0, 'transport': 0.03,
                  'transportExponent': 1.0, 'settle': 0.3, 'spread': 2.0, 'creep': 0.01, 'maxStep': 4.0,
                  'rockWear': 0.35},
    'thermal': {'repose': 42.0, 'rock': 58.0, 'rate': 0.05},            # degrees; share of the excess per iteration
    'passes': [
        {'cell': 200, 'iterations': 150, 'maxCut': 500, 'roughness': {'amplitude': 60, 'wavelength': 2500},
         'rain': {'wavelength': 3000, 'variation': 0.5},
         'hydraulic': {'incision': 0.0003, 'areaExponent': 1.2, 'spread': 1.5, 'creep': 0.02, 'maxStep': 10.0}},
        {'cell': 100, 'iterations': 60, 'maxCut': 150, 'hydraulic': {'incision': 0.0001, 'creep': 0.03}},
    ],
    'maxCut': 600, 'maxFill': 250,                                     # cm from the ground before erosion
    'hold': {'reach': 2000, 'keep': 200},  # cm: the change's broad part (over reach) at most keep
    'feather': 600,                                                    # cm
    'masks': {'roads': 150, 'ramps': 1200, 'steps': 1400, 'trails': 150, 'footprints': 300, 'placements': 500,
              'town': 250, 'obstacles': 250, 'knobs': 150, 'zones': 300, 'encounters': 300, 'spots': 600,
              'water': 800, 'designed': 200, 'gullies': 300, 'bands': 400, 'lip': 800, 'boundaryOut': 800,
              'boundaryIn': 600, 'holes': 500, 'towns': 400, 'townSkirt': 3000},
    'keep': [],          # [{id, polygon [[X, Y] cm]}]: more ground kept as it is (the towns lanes' lots)
    'townZones': [],     # zone ids kept with masks.townSkirt round them (Main Street, the depot)
    'paint': True,       # area_macro.py: gully beds and fans in the macro map
    # area_mesh.py, off by default (it costs triangles): where erosion moved the ground over cut cm, vertices at
    # most spacing cm apart and the height tolerance x detail.
    'mesh': {'enabled': False, 'cut': 30, 'spacing': 240, 'detail': 1.0},
    'maps': {},          # area_erosion_maps.py: {folder, windows: [{id, X, Y}]}
}
PASS_KEYS = ('cell', 'iterations', 'maxCut', 'maxFill', 'roughness', 'rain', 'hydraulic', 'thermal', 'strength')


def _merged(base, over):
    out = dict(base)
    for key, value in (over or {}).items():
        out[key] = _merged(base[key], value) if isinstance(base.get(key), dict) and isinstance(value, dict) else value
    return out


def settings(area):
    """The layout's erosion block over DEFAULTS, or None when it has none or it's switched off."""
    spec = area.layout.get('erosion')
    if not spec:
        return None
    cfg = _merged(DEFAULTS, {k: v for k, v in spec.items() if k not in ('about', 'note')})
    return cfg if cfg.get('enabled', True) else None


def pass_settings(cfg, spec, index=0):
    """One pass's settings: the block's, with the pass's own over them, and its own seed."""
    out = _merged({k: cfg[k] for k in PASS_KEYS if k in cfg}, {k: v for k, v in spec.items() if k in PASS_KEYS})
    out['seed'] = int(cfg['seed']) * 31 + index
    return out


def apply(area, h, log=print):
    """Erodes h (the finished heights) as the layout's erosion block asks; h itself without one."""
    cfg = settings(area)
    if cfg is None:
        return h
    started = time.time()
    n, half = area.grid.n, area.half
    passes = [pass_settings(cfg, p, i) for i, p in enumerate(cfg['passes'])]
    sizes = []
    for p in passes:
        m = int(round(2.0 * half / (p['cell'] / 100.0)))
        if n % m:
            raise ValueError(f'erosion: a pass\'s cell {p["cell"]} cm must nest in the {n} height cells')
        sizes.append(m)
    last = sizes[-1]
    if any(last % m for m in sizes):
        raise ValueError('erosion: every pass\'s grid must nest in the last pass\'s')
    weight, labels = protection(area, cfg, last, log)
    log(f'erosion: {100.0 * float(np.mean(weight[area.edge > 0.0] <= 0.0)):.1f}% of the core kept as it is; passes '
        + ', '.join(f'{p["cell"]:g} cm x {p["iterations"]}' for p in passes))
    total = np.zeros((last, last))  # the change so far, on the last pass's grid
    fields = {}
    for p, m in zip(passes, sizes):
        base = _down(h, n // m) + _down(total, last // m)
        k = _down(weight, n // m)
        p_started = time.time()
        he, fields = solve(Grid(m, half), base, k, p, log)
        dh = blur((he - base).astype(np.float32), 1)
        total += upsample(dh, last // m)
        log(f'erosion: pass {p["cell"]:g} cm ({m} x {m}) in {time.time() - p_started:.0f} s: cut '
            f'{-dh.min() * 100:.0f} cm, fill {dh.max() * 100:.0f} cm')
    hold = cfg.get('hold') or {}
    if hold.get('reach', 0) > 0:
        # Hold the shape: the change's broad part is kept to within keep, the rest is erosion's own relief.
        px = 2.0 * half / last
        broad = blur(total.astype(np.float32), max(1, int(round(hold['reach'] / 100.0 / px / 3.0)))).astype(np.float64)
        keep = hold.get('keep', 50) / 100.0
        total = total - broad + np.clip(broad, -keep, keep)
    total = np.clip(total, -cfg['maxCut'] / 100.0, cfg['maxFill'] / 100.0)
    up = upsample(total.astype(np.float32), n // last)
    out = h + (weight * up).astype(h.dtype)
    out = np.where(weight > 0.0, out, h)  # the kept cells: their own heights, bit for bit
    del up
    applied = out - h
    fields.update(weight=weight, labels=labels, grid=Grid(last, half), change=total.astype(np.float32),
                  applied=_down(applied, n // last).astype(np.float32), k=_down(weight, n // last).astype(np.float32))
    fields['cut'] = np.maximum(-fields['applied'], 0.0)
    fields['fill'] = np.maximum(fields['applied'], 0.0)
    # The change's own relief (against its mean over about 6 m of the ground erosion worked, so a mask's edge, where
    # the change falls to 0, isn't a bed): gully beds below, ribs and fans above.
    radius = max(1, int(round(2.0 / (2.0 * half / last))))
    worked = (fields['k'] > 0.5).astype(np.float32)
    mean = blur(fields['applied'] * worked, radius) / np.maximum(blur(worked, radius), 1e-3)
    fields['relief'] = ((fields['applied'] - mean) * worked).astype(np.float32)
    area.erosion = fields
    log(f'erosion: done in {time.time() - started:.0f} s: cut up to {-applied.min() * 100:.0f} cm, fill up to '
        f'{applied.max() * 100:.0f} cm; {100.0 * float(np.mean(np.abs(applied[area.edge > 0.0]) > 0.05)):.1f}% of '
        'the core moved over 5 cm')
    return out


def _down(raster, f):
    """A raster on blocks of f x f cells (their means)."""
    if f == 1:
        return raster.astype(np.float64)
    m = raster.shape[0] // f
    return raster.astype(np.float64).reshape(m, f, m, f).mean(axis=(1, 3))


# --- A pass ---

def _views(p, m):
    """The 8 neighbours' values of a padded (m + 2) square array, as views."""
    return [p[1 + di:1 + di + m, 1 + dj:1 + dj + m] for di, dj in OFFSETS]


def _accumulate(recv, w0, depth_too=False):
    """The water each cell gathers, A = sum over d >= 0 of P^d w0 (P sends a cell's water to its receiver; recv holds
    flat indices, n for a sink), by doubling: v += P^(2^k) v, P^(2^(k+1)) = P^(2^k) P^(2^k), until every path has
    reached its sink (log2 of the longest path's hops passes). With depth_too also each cell's hops to its last cell
    before the sink (binary lifting on the same powers). The receivers must form a forest (each strictly lower)."""
    n = len(w0)
    r = np.concatenate([recv, [n]]).astype(np.int64)
    v = np.concatenate([w0, [0.0]])
    powers = []
    for _ in range(64):
        powers.append(r)
        v += np.bincount(r, weights=v, minlength=n + 1)
        v[n] = 0.0
        r = r[r]
        if r.min() == n:
            break
    else:
        raise RuntimeError('erosion: the receivers hold a cycle')
    if not depth_too:
        return v[:n], None
    cur = np.arange(n + 1, dtype=np.int64)
    depth = np.zeros(n + 1, dtype=np.int64)
    for k in range(len(powers) - 1, -1, -1):
        nxt = powers[k][cur]
        step = nxt != n
        cur = np.where(step, nxt, cur)
        depth += step.astype(np.int64) << k
    return v[:n], depth[:n]


class _Routing:
    """One iteration's drainage on the working grid: each cell's receiver (stochastic D8: a lower neighbour picked with
    weight slope^spread), the water it gathers (A) and its depth in the receivers' tree (hops to its sink), the cells
    grouped by depth (order, starts) with each one's place in its group (pos) for the ordered passes."""

    def __init__(self, h, m, inv_dist, valid, offset_flat, index, spread, rng, w0):
        n = m * m
        p = np.pad(h, 1, mode='edge')
        slope = np.stack([(h - view) * inv_dist[d] for d, view in enumerate(_views(p, m))]) * valid
        self.steepest = slope.max(axis=0)
        wts = np.maximum(slope, 0.0)
        if spread != 1.0:
            wts **= spread
        cum = np.cumsum(wts, axis=0)
        total = cum[-1]
        target = rng.random((m, m)) * total
        pick = np.minimum((cum < target[None]).sum(axis=0), 7)
        # The pick must fall strictly lower (a tie at 0 would let a cycle in): else the steepest way.
        chosen = np.take_along_axis(slope, pick[None], axis=0)[0]
        pick = np.where(chosen > 0.0, pick, np.argmax(slope, axis=0))
        sink = self.steepest <= 0.0
        self.recv = np.where(sink, n, index + offset_flat[pick]).ravel()
        self.length = np.where(sink, 1.0, 1.0 / inv_dist[pick]).ravel()
        self.sink = sink.ravel()
        self.area, depth = _accumulate(self.recv, w0, depth_too=True)
        levels = int(depth.max()) + 1
        self.order = np.argsort(depth.astype(np.int16 if levels < 32000 else np.int32), kind='stable')
        counts = np.bincount(depth, minlength=levels)
        self.starts = np.concatenate([[0], np.cumsum(counts)])
        self.levels = levels
        pos = np.empty(n, dtype=np.int64)
        pos[self.order] = np.arange(n) - np.repeat(self.starts[:-1], counts)
        self.pos = pos
        self.counts = counts

    def level(self, d):
        return self.order[self.starts[d]:self.starts[d + 1]]


def solve(grid, h0, k, p, log=print):
    """One pass on a working grid: erodes h0 (float64) where the erodibility k (0..1) lets it (see the module's doc).
    Returns the heights and the fields (the flow)."""
    m, px = grid.n, grid.px
    n = m * m
    hyd, th = p['hydraulic'], p['thermal']
    rng = np.random.default_rng(int(p['seed']))
    rain_spec = p['rain']
    rain = rain_spec['amount'] * (1.0 + rain_spec['variation'] * fbm_raster(
        grid, rain_spec['wavelength'] / 100.0, seed=int(p['seed']) * 7 + 701, octaves=2))
    cell_area = px * px
    w0 = (np.clip(rain, 0.05, None).astype(np.float64) * cell_area).ravel()
    dist = np.array([px * math.hypot(di, dj) for di, dj in OFFSETS])
    inv_dist = 1.0 / dist
    valid = np.ones((8, m, m))
    for d, (di, dj) in enumerate(OFFSETS):
        if di < 0:
            valid[d, 0, :] = 0.0
        if di > 0:
            valid[d, -1, :] = 0.0
        if dj < 0:
            valid[d, :, 0] = 0.0
        if dj > 0:
            valid[d, :, -1] = 0.0
    offset_flat = np.array([di * m + dj for di, dj in OFFSETS], dtype=np.int64)
    index = np.arange(n, dtype=np.int64).reshape(m, m)
    kf = k.ravel()
    free = k > 1e-3
    incision = hyd['incision'] * p.get('strength', 1.0)
    am, threshold, saturate = hyd['areaExponent'], float(hyd['threshold']), float(hyd['saturate'])
    transport, tm = hyd['transport'], hyd['transportExponent']
    share = hyd['settle'] * kf
    creep = min(float(hyd['creep']), 0.2 * px * px)  # m2 an iteration, under the explicit scheme's limit
    max_step = hyd['maxStep'] / 100.0
    rock_tan = math.tan(math.radians(th['rock']))
    low, high = h0 - p['maxCut'] / 100.0, h0 + p['maxFill'] / 100.0
    h = h0.copy()
    rough = p.get('roughness') or {}
    if rough.get('amplitude', 0) > 0:
        h += rough['amplitude'] / 100.0 * k * fbm_raster(grid, rough.get('wavelength', 2500) / 100.0,
                                                       seed=int(p['seed']) * 7 + 703, octaves=3, gain=0.55)
    flow_sum = np.zeros(n)
    flow_count = 0
    iterations = int(p['iterations'])
    for it in range(iterations):
        route = _Routing(h, m, inv_dist, valid, offset_flat, index, hyd['spread'], rng, w0)
        a = route.area
        hf = h.ravel()
        recv = route.recv
        wear = np.where(route.steepest.ravel() > rock_tan, hyd['rockWear'], 1.0)
        a_cut = np.maximum(a - threshold, 0.0)
        if saturate > 0.0:
            a_cut = a_cut / (1.0 + a_cut / saturate)
        f = np.where(route.sink, 0.0, incision * kf * wear * a_cut ** am / route.length)
        # Incision, up each tree from its sink (a level's receivers are all on the level before).
        hn = hf.copy()
        for d in range(1, route.levels):
            c = route.level(d)
            fc = f[c]
            hn[c] = (hf[c] + fc * hn[recv[c]]) / (1.0 + fc)
        # The eroded soil, routed down each tree from its tips: a level's soil sums onto the level after it.
        vol = (hf - hn) * cell_area
        hr = np.concatenate([hn, [0.0]])
        s = np.where(route.sink, 0.0, np.maximum(hn - hr[recv], 0.0) / route.length)
        cap = transport * a ** tm * s
        kept = np.zeros(n)
        carry = None
        for d in range(route.levels - 1, -1, -1):
            c = route.level(d)
            total = vol[c] if carry is None else vol[c] + carry
            dep = np.maximum(total - cap[c], 0.0) * share[c]
            kept[c] = dep
            if d > 0:
                carry = np.bincount(route.pos[recv[c]], weights=total - dep, minlength=int(route.counts[d - 1]))
        if it >= iterations * 2 // 3:
            flow_sum += np.log10(np.maximum(a, cell_area * 0.05))
            flow_count += 1
        change = ((hn - hf) + kept / cell_area).reshape(m, m)
        hnew = h + np.clip(change, -max_step, max_step)
        if creep > 0.0:
            nb = _views(np.pad(hnew, 1, mode='edge'), m)
            hnew += creep * (nb[1] + nb[3] + nb[4] + nb[6] - 4.0 * hnew) / (px * px) * k
        hnew = np.where(free, hnew, h)
        if th['rate'] > 0.0:
            hnew = _thermal(hnew, m, dist, valid, free, k, th)
        h = np.clip(hnew, low, high)
    return h, {'flow': (flow_sum / max(flow_count, 1)).reshape(m, m).astype(np.float32)}


def _thermal(h, m, dist, valid, free, k, th):
    """One thermal pass: each free cell steeper than the repose toward some lower free neighbours moves rate x half its
    largest excess onto them, shared by excess; rock (steeper than th.rock toward some neighbour) holds."""
    repose, rock = math.tan(math.radians(th['repose'])), math.tan(math.radians(th['rock']))
    nb = _views(np.pad(h, 1, mode='edge'), m)
    nfree = _views(np.pad(free, 1, mode='constant', constant_values=False), m)
    excess = np.empty((8, m, m))
    steep = np.zeros((m, m))
    for d in range(8):
        drop = (h - nb[d]) * valid[d]
        np.maximum(steep, drop / dist[d], out=steep)
        excess[d] = np.where(nfree[d], np.maximum(drop - repose * dist[d], 0.0), 0.0)
    total = excess.sum(axis=0)
    move = th['rate'] * 0.5 * excess.max(axis=0) * np.where(free & (steep <= rock), k, 0.0)
    share = np.where(total > 0.0, move / np.maximum(total, 1e-12), 0.0)
    gain = np.zeros((m + 2, m + 2))
    for d, (di, dj) in enumerate(OFFSETS):
        gain[1 + di:1 + di + m, 1 + dj:1 + dj + m] += excess[d] * share
    return h - move + gain[1:-1, 1:-1]
