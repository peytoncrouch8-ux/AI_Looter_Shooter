"""The flow and low-point maps for the towns' ground (layout.json erosion.maps; the 2026-11-04 Towns plan, section 6:
the GROUND lane places puddles and wet ground by them). Written with --computed (area_model.py), from the finished
heights on the height raster (12.5 cm on Ransom's Rest), one pair per window (erosion.maps.windows: id, X [from, to],
Y [from, to] cm), into erosion.maps.folder beside the layout (or the run's --save-to folder):

- flow_<id>.png, 16-bit grey, north up (row 0 the window's north edge, column 0 its west edge): the water gathered
  from upstream (A, m2 of ground draining through the cell, rain 1 everywhere), as round(65535 x clip((log10 A + 2) / 8,
  0, 1)): 0.01 m2 at 0, 1e6 m2 at 65535. The drainage is a priority flood (Barnes, Lehman and Mulla 2014) over the
  window and 30 m round it, whose edge drains out: every cell sends its water to the neighbour the flood reached it
  from, so water crosses flats and fills pits on its way out;
- lows_<id>.png, 16-bit grey, the same frame: the depression's depth (how deep water would stand before it spills),
  millimeters (0..65535).
maps.json beside them says all that, with each window's covers, size and cell. Numpy only, plus OpenImageIO (in
Blender's Python) for the 16-bit PNGs.
"""
import heapq
import json
import os
import time

import numpy as np

PAD = 30.0   # meters round a window the drainage reaches into (upstream water)
FLOW_LOG = (-2.0, 6.0)  # log10 A over the 16 bits


def write_maps(area, folder, log=print):
    """Writes the windows' flow and low-point maps (erosion.maps) into folder/<maps.folder>; nothing without them."""
    import area_erosion
    cfg = area_erosion.settings(area)
    spec = (cfg or {}).get('maps') or {}
    windows = spec.get('windows') or []
    if not windows:
        return None
    started = time.time()
    out_dir = os.path.join(folder, spec.get('folder', 'erosion'))
    os.makedirs(out_dir, exist_ok=True)
    grid, h = area.grid, area.h
    px = grid.px
    written = []
    for w in windows:
        x0, x1 = (v / 100.0 for v in w['X'])
        y0, y1 = (v / 100.0 for v in w['Y'])
        i0, i1 = grid.index_range(min(x0, x1), max(x0, x1))
        j0, j1 = grid.index_range(min(y0, y1), max(y0, y1))
        p0, p1 = grid.index_range(min(x0, x1) - PAD, max(x0, x1) + PAD)
        q0, q1 = grid.index_range(min(y0, y1) - PAD, max(y0, y1) + PAD)
        sub = h[p0:p1, q0:q1].astype(np.float64)
        filled, recv = flood(sub)
        a = accumulate(recv, np.full(sub.size, px * px))
        a = a.reshape(sub.shape)[i0 - p0:i1 - p0, j0 - q0:j1 - q0]
        lows = (filled - sub)[i0 - p0:i1 - p0, j0 - q0:j1 - q0]
        flow16 = np.round(65535.0 * np.clip((np.log10(np.maximum(a, 1e-9)) - FLOW_LOG[0])
                                            / (FLOW_LOG[1] - FLOW_LOG[0]), 0.0, 1.0)).astype(np.uint16)
        lows16 = np.round(np.clip(lows * 1000.0, 0.0, 65535.0)).astype(np.uint16)
        for name, image in (('flow', flow16), ('lows', lows16)):
            _png16(os.path.join(out_dir, f'{name}_{w["id"]}.png'), image[::-1, :])
        covers = [[round((-area.half + i0 * px) * 100.0, 2), round((-area.half + j0 * px) * 100.0, 2)],
                  [round((-area.half + i1 * px) * 100.0, 2), round((-area.half + j1 * px) * 100.0, 2)]]
        written.append({'id': w['id'], 'flow': f'flow_{w["id"]}.png', 'lows': f'lows_{w["id"]}.png',
                        'covers': covers, 'size': [int(j1 - j0), int(i1 - i0)], 'cell': round(px * 100.0, 4),
                        'pits': int(np.sum(lows > 0.01)), 'deepest': round(float(lows.max()) * 100.0, 1)})
        log(f'erosion maps: {w["id"]} {j1 - j0} x {i1 - i0} px, {written[-1]["pits"]} cells holding over 1 cm of '
            f'standing water, the deepest {written[-1]["deepest"]} cm')
    about = {
        'about': 'The towns\' flow and low-point maps (Art/Levels/area_erosion_maps.py), from the finished heights '
                 '(erosion included). 16-bit grey PNGs, north up: row 0 is the window\'s north edge (X covers[1][0]), '
                 'column 0 its west edge (Y covers[0][1]); each pixel one height-raster cell (cell, cm). flow: '
                 f'round(65535 x clip((log10 A + {-FLOW_LOG[0]:g}) / {FLOW_LOG[1] - FLOW_LOG[0]:g}, 0, 1)), A the m2 '
                 'draining through the cell (a priority flood\'s drainage over the window and 30 m round it); lows: '
                 'millimeters of standing water before the hollow spills. Puddles: lows over 10 (1 cm) where flow is '
                 'high (the Towns plan, section 6). covers: [[X0, Y0], [X1, Y1]] cm; size [columns, rows].',
        'flowLog10': list(FLOW_LOG), 'windows': written}
    with open(os.path.join(out_dir, 'maps.json'), 'w', encoding='utf-8', newline='\n') as file:
        json.dump(about, file, indent=2)
        file.write('\n')
    log(f'erosion maps: wrote {len(written)} windows into {out_dir} in {time.time() - started:.0f} s')
    return out_dir


def flood(h):
    """Priority flood over a raster whose edge drains out: the filled heights (each cell raised to where its water
    spills) and each cell's receiver (flat index of the neighbour the flood reached it from; h.size for the edge)."""
    rows, cols = h.shape
    n = rows * cols
    flat = h.ravel()
    filled = flat.copy()
    recv = np.full(n, n, dtype=np.int64)
    seen = np.zeros(n, dtype=bool)
    heap = []
    edge = np.zeros(h.shape, dtype=bool)
    edge[0, :] = edge[-1, :] = edge[:, 0] = edge[:, -1] = True
    for c in np.flatnonzero(edge.ravel()):
        heapq.heappush(heap, (flat[c], int(c)))
        seen[c] = True
    pit = []  # cells under the spill level: filled to it, breadth first (no heap needed)
    steps = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]
    while heap or pit:
        if pit:
            c = pit.pop()
            level = filled[c]
        else:
            level, c = heapq.heappop(heap)
        r, q = divmod(c, cols)
        for dr, dq in steps:
            rr, qq = r + dr, q + dq
            if rr < 0 or qq < 0 or rr >= rows or qq >= cols:
                continue
            nb = rr * cols + qq
            if seen[nb]:
                continue
            seen[nb] = True
            recv[nb] = c
            if flat[nb] <= level:
                filled[nb] = level
                pit.append(nb)
            else:
                heapq.heappush(heap, (flat[nb], nb))
    return filled.reshape(h.shape), recv


def accumulate(recv, w0):
    """A = sum over d >= 0 of P^d w0 for receivers recv (flat indices, n for an outlet), by doubling (area_erosion)."""
    n = len(w0)
    r = np.concatenate([recv, [n]]).astype(np.int64)
    v = np.concatenate([w0, [0.0]])
    for _ in range(64):
        v += np.bincount(r, weights=v, minlength=n + 1)
        v[n] = 0.0
        r = r[r]
        if r.min() == n:
            break
    return v[:n]


def _png16(path, image):
    import OpenImageIO as oiio
    out = oiio.ImageOutput.create(path)
    if out is None:
        raise RuntimeError(f'erosion maps: cannot write {path}: {oiio.geterror()}')
    # The type as OpenImageIO's own constant and the pixels as (rows, columns, channels): a 'uint16' string or a 2D
    # array crashed Blender 4.4's binding (an access violation).
    out.open(path, oiio.ImageSpec(image.shape[1], image.shape[0], 1, oiio.UINT16))
    out.write_image(np.ascontiguousarray(image[..., None], dtype=np.uint16))
    out.close()
