"""Compares an area's terrain from the working tree's generator and layout with HEAD's, inside HEAD's playable boundary:
the proof that growing a map square (layout.json map.grownFrom) or adding ground elsewhere left the old ground as it
was. Run it with Blender's own Python, which has numpy and OpenImageIO (Tools/terrain_compare.ps1 finds it):

    Tools/terrain_compare.ps1 -Area RansomsRest [-Layout PATH] [-Macro] [-Except FILE] [-Fresh] [-Keep]
    <Blender>/<version>/python/bin/python.exe Tools/terrain_compare.py <Area> [--layout PATH] [--macro]
        [--except FILE] [--blender EXE] [--fresh] [--keep]

It builds the area twice, headless in Blender, through Tools/Blender/terrain_dump.py (which saves the rasters): with
HEAD's generator and layout (git archive of Art/Levels, Art/Models/Terrain and Tools/Blender into a temp folder) and
with the working tree's (or --layout, a scratch copy), --computed (and --macro) into temp folders, so nothing in the
repository is written. Then, on the cells inside HEAD's playable boundary (its distance inside the boundary, play_edge,
over 0):
- the heights, cell for cell where the two shape rasters share their cells (a grown square), else bilinear: the
  largest difference, the mean, the 99th percentile and how many cells differ by more than TOLERANCE_CM; then every
  patch of such cells (8-connected), largest first, with its extent in the area doc's meters (x east, y north), its
  size and its largest difference. With --except FILE (JSON: {"areas": [{"id", "polygon": [[X, Y] cm, ...]}]}, the
  edits meant to change the old ground), the numbers again outside those areas, and the patches inside each;
- the baked occlusion (ao) and the curvature the mesh is sized by, the same way;
- with --macro, the macro color map: both maps (or a split map's windows, macroMap.windows) averaged onto a 1 m grid
  over the old boundary, the largest, mean and 99th percentile differences in 8-bit levels per channel (a grown map's
  pixels needn't fall where the old one's did, so it is compared at 1 m, not pixel for pixel).
It prints PASS when no height inside HEAD's boundary (outside the --except areas) differs by more than TOLERANCE_CM,
else FAIL (exit code 1). Git's state is never touched: it only reads (rev-parse, show, archive). HEAD's build is kept
in %TEMP%/terrain_compare/<Area> by the tree ids it read (and whether it painted the macro map), so a second run against
the same commit skips it; --fresh builds it again, --keep keeps the working tree's outputs too.
"""
import argparse
import glob
import hashlib
import io
import json
import math
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time

import numpy as np

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
GENERATOR = ('Art/Levels', 'Art/Models/Terrain', 'Tools/Blender')
DUMP = os.path.join(REPO, 'Tools', 'Blender', 'terrain_dump.py')
TOLERANCE_CM = 1.0
PATCHES = 12            # how many patches of changed cells to list
MACRO_CELL = 1.0        # meters: the grid the macro maps are compared on


def git(*args, binary=False):
    result = subprocess.run(['git', '-C', REPO, *args], capture_output=True, check=True)
    return result.stdout if binary else result.stdout.decode('utf-8')


def find_blender(given):
    installed = sorted(glob.glob(r'C:\Program Files\Blender Foundation\*\blender.exe'), reverse=True)
    for path in [given, os.environ.get('BLENDER')] + installed:
        if path and os.path.exists(path):
            return path
    raise SystemExit('Blender not found: pass --blender, or set BLENDER to blender.exe')


def build(blender, tree, area, out, flags, layout=None):
    """Builds the area with tree's generator into out (rasters.npz, layout_computed.json, maps), logging to
    out/blender.log. Returns the build's seconds."""
    os.makedirs(out, exist_ok=True)
    helpers = os.path.join(REPO, 'Tools', 'Blender').replace('\\', '/')
    setup = f"import sys; sys.path.append('{helpers}'); import bpy; bpy.ops.wm.read_factory_settings(use_empty=True)"
    command = [blender, '-b', '--factory-startup', '--python-exit-code', '2', '--python-expr', setup, '--python', DUMP,
               '--', '--dump-tree', tree, '--dump-area', area, '--save-to', out, *flags]
    if layout:
        command += ['--layout', layout]
    # As Tools/artrun.ps1 does: wait while the main session measures performance (Saved/ArtPause.flag), then run
    # below normal priority, so the editor keeps the machine.
    flag = os.path.join(REPO, 'Saved', 'ArtPause.flag')
    waited = 0
    while os.path.exists(flag):
        if waited == 0:
            print('  paused while the main session measures performance (Saved/ArtPause.flag); waiting...', flush=True)
        if waited >= 1800:
            raise SystemExit('still paused (Saved/ArtPause.flag) after 30 minutes: run this again later')
        time.sleep(15)
        waited += 15
    priority = getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0)
    started = time.time()
    with open(os.path.join(out, 'blender.log'), 'w', encoding='utf-8', errors='replace') as log:
        code = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, creationflags=priority).returncode
    if code != 0 or not os.path.exists(os.path.join(out, 'rasters.npz')):
        with open(os.path.join(out, 'blender.log'), encoding='utf-8', errors='replace') as log:
            tail = log.read().splitlines()[-25:]
        raise SystemExit(f'Blender failed (exit code {code}); the end of {out}/blender.log:\n  ' + '\n  '.join(tail))
    return time.time() - started


def head_build(blender, area, work, flags, fresh):
    """HEAD's build, from its generator and layout taken out of git, cached by the tree ids it reads."""
    trees = git('rev-parse', *[f'HEAD:{folder}' for folder in GENERATOR]).split()
    key = hashlib.sha1((' '.join(trees) + ' '.join(flags)).encode()).hexdigest()[:12]
    folder = os.path.join(work, 'head-' + key)
    out = os.path.join(folder, 'out')
    if os.path.exists(os.path.join(out, 'rasters.npz')) and not fresh:
        return out, None
    shutil.rmtree(folder, ignore_errors=True)
    tree = os.path.join(folder, 'tree')
    os.makedirs(tree)
    with tarfile.open(fileobj=io.BytesIO(git('archive', '--format=tar', 'HEAD', *GENERATOR, binary=True))) as tar:
        tar.extractall(tree)
    return out, build(blender, tree, area, out, flags)


# --- Rasters ---

def cell_centers(n, half):
    return -half + (np.arange(n) + 0.5) * (2.0 * half / n)


def bilinear(raster, half, x, y):
    n = raster.shape[0]
    px = 2.0 * half / n
    fi = np.clip((x + half) / px - 0.5, 0.0, n - 1.000001)
    fj = np.clip((y + half) / px - 0.5, 0.0, n - 1.000001)
    i0, j0 = np.floor(fi).astype(np.int64), np.floor(fj).astype(np.int64)
    ti, tj = fi - i0, fj - j0
    i1, j1 = np.minimum(i0 + 1, n - 1), np.minimum(j0 + 1, n - 1)
    return ((raster[i0, j0] * (1 - tj) + raster[i0, j1] * tj) * (1 - ti)
            + (raster[i1, j0] * (1 - tj) + raster[i1, j1] * tj) * ti)


def onto_old(new, new_half, old_n, old_half):
    """The new raster on the old one's cells: cell for cell when they share them (a grown square on the same grid),
    else bilinear. Returns (raster, how)."""
    px_old, px_new = 2.0 * old_half / old_n, 2.0 * new_half / new.shape[0]
    offset = (new_half - old_half) / px_new
    if abs(px_old - px_new) < 1e-9 and abs(offset - round(offset)) < 1e-6 and offset >= 0:
        k = int(round(offset))
        return new[k:k + old_n, k:k + old_n], 'cell for cell'
    c = cell_centers(old_n, old_half)
    out = np.empty((old_n, old_n), np.float64)
    for a in range(0, old_n, 256):
        x, y = np.meshgrid(c[a:a + 256], c, indexing='ij')
        out[a:a + 256] = bilinear(new, new_half, x, y)
    return out, 'bilinear'


def points_in_polygon(x, y, poly):
    inside = np.zeros(np.shape(x), dtype=bool)
    a = np.asarray(poly, dtype=np.float64)
    b = np.roll(a, -1, axis=0)
    for (ax, ay), (bx, by) in zip(a, b):
        cond = (ax <= x) != (bx <= x)
        t = (x - ax) / np.where(bx - ax == 0.0, 1e-12, bx - ax)
        inside ^= cond & (y < ay + t * (by - ay))
    return inside


def polygon_cells(poly_m, n, half):
    c = cell_centers(n, half)
    mask = np.zeros((n, n), dtype=bool)
    for a in range(0, n, 256):
        x, y = np.meshgrid(c[a:a + 256], c, indexing='ij')
        mask[a:a + 256] = points_in_polygon(x, y, poly_m)
    return mask


def patches(changed, diff, n, half, limit=PATCHES):
    """The 8-connected patches of changed cells, largest difference first: (extent x0, x1, y0, y1 in the doc's meters
    (x east, y north), cells, area m², largest difference cm)."""
    px = 2.0 * half / n
    labels = np.zeros(changed.shape, dtype=np.int64)
    ii, jj = np.nonzero(changed)
    order = {}
    for i, j in zip(ii.tolist(), jj.tolist()):
        order[(i, j)] = True
    found = []
    label = 0
    for start in list(order):
        if labels[start] or not order.get(start):
            continue
        label += 1
        stack = [start]
        labels[start] = label
        cells = []
        while stack:
            i, j = stack.pop()
            cells.append((i, j))
            for di in (-1, 0, 1):
                for dj in (-1, 0, 1):
                    q = (i + di, j + dj)
                    if q in order and not labels[q]:
                        labels[q] = label
                        stack.append(q)
        a = np.array(cells)
        lx = -half + (a[:, 0] + 0.5) * px   # layout X: north
        ly = -half + (a[:, 1] + 0.5) * px   # layout Y: east
        found.append((float(ly.min()), float(ly.max()), float(lx.min()), float(lx.max()), len(cells),
                      len(cells) * px * px, float(np.max(diff[a[:, 0], a[:, 1]])) * 100.0))
    found.sort(key=lambda p: -p[6])
    return found[:limit], len(found)


def summarize(name, old, new, inside, unit_cm=True, except_mask=None, half=None, show_patches=False):
    """Prints the comparison of two rasters on the cells inside; returns the largest difference (cm or the raster's
    units) outside the --except areas."""
    diff = np.abs(new.astype(np.float64) - old.astype(np.float64))
    scale = 100.0 if unit_cm else 1.0
    unit = 'cm' if unit_cm else ''
    values = diff[inside] * scale
    tol = TOLERANCE_CM if unit_cm else 1e-4
    print(f'{name}: {int(inside.sum())} cells inside the old boundary: the largest difference {values.max():.3g} {unit}, '
          f'mean {values.mean():.3g}, 99th percentile {np.percentile(values, 99):.3g}; '
          f'{int((values > tol).sum())} cells over {tol:g} {unit}'.replace('  ', ' ').replace(' ,', ','))
    largest = float(values.max())
    if except_mask is not None:
        outside = inside & ~except_mask
        rest = diff[outside] * scale
        largest = float(rest.max()) if rest.size else 0.0
        print(f'  outside the --except areas ({int(outside.sum())} cells): the largest {largest:.4f} {unit}, '
              f'{int((rest > tol).sum())} cells over {tol:g} {unit}')
    if show_patches and unit_cm:
        changed = inside & (diff * 100.0 > TOLERANCE_CM)
        if changed.any():
            listed, total = patches(changed, diff, old.shape[0], half)
            print(f'  {total} patches of cells over {TOLERANCE_CM:g} cm, the largest differences first '
                  '(x east, y north, m):')
            for x0, x1, y0, y1, count, area_m2, worst in listed:
                print(f'    x {x0:7.1f} to {x1:7.1f}, y {y0:7.1f} to {y1:7.1f}: {area_m2:8.1f} m², up to {worst:8.2f} cm')
    return largest


# --- The macro maps ---

def read_png(path):
    import OpenImageIO as oiio
    config = oiio.ImageSpec()
    config.attribute('oiio:UnassociatedAlpha', 1)
    image = oiio.ImageInput.open(path, config)
    if image is None:
        raise SystemExit(f'cannot read {path}: {oiio.geterror()}')
    pixels = image.read_image(format='uint8')
    image.close()
    return pixels[::-1].astype(np.float32)  # row 0 = south, as the generator paints it


def macro_windows(folder, computed):
    """[(pixels (rows from the south), covers [[X0, Y0], [X1, Y1]] in cm)] for a computed layout's macro map(s)."""
    macro = computed['macroMap']
    windows = macro.get('windows') or [{'texture': macro['texture'], 'covers': macro['covers']}]
    out = []
    for w in windows:
        path = os.path.join(folder, os.path.basename(w['texture']))
        out.append((read_png(path), np.asarray(w['covers'], dtype=np.float64) / 100.0))
    return out


def macro_on(windows, x, y):
    """The maps' colors at points (layout meters), bilinear from whichever window covers each."""
    out = np.full(x.shape + (4,), np.nan, np.float32)
    for pixels, ((x0, y0), (x1, y1)) in windows:
        n = pixels.shape[0]
        on = (x >= x0) & (x < x1) & (y >= y0) & (y < y1)
        if not on.any():
            continue
        half = (x1 - x0) * 0.5
        cx, cy = (x0 + x1) * 0.5, (y0 + y1) * 0.5
        for k in range(4):
            out[on, k] = bilinear(pixels[..., k], half, x[on] - cx, y[on] - cy)
        del n
    return out


def compare_macro(old_folder, new_folder, old_computed, new_computed, poly_m):
    old, new = macro_windows(old_folder, old_computed), macro_windows(new_folder, new_computed)
    lo, hi = poly_m.min(axis=0), poly_m.max(axis=0)
    # Each 1 m cell's mean over a 5 x 5 grid of samples in it, inside the old boundary.
    xs = np.arange(math.floor(lo[0]), math.ceil(hi[0]), MACRO_CELL) + 0.5 * MACRO_CELL
    ys = np.arange(math.floor(lo[1]), math.ceil(hi[1]), MACRO_CELL) + 0.5 * MACRO_CELL
    gx, gy = np.meshgrid(xs, ys, indexing='ij')
    keep = points_in_polygon(gx, gy, poly_m)
    gx, gy = gx[keep], gy[keep]
    offsets = (np.arange(5) - 2) * (MACRO_CELL / 5.0)
    sums = {name: np.zeros((len(gx), 4)) for name in ('old', 'new')}
    for dx in offsets:
        for dy in offsets:
            sums['old'] += macro_on(old, gx + dx, gy + dy)
            sums['new'] += macro_on(new, gx + dx, gy + dy)
    diff = np.abs(sums['new'] - sums['old']) / 25.0
    good = np.all(np.isfinite(diff), axis=1)
    diff = diff[good]
    print(f'macro map, averaged over {MACRO_CELL:g} m cells inside the old boundary ({len(diff)} cells), differences in '
          '8-bit levels (R, G, B, A):')
    print('  largest ' + ', '.join(f'{v:.1f}' for v in diff.max(axis=0)) + '; mean '
          + ', '.join(f'{v:.2f}' for v in diff.mean(axis=0)) + '; 99th percentile '
          + ', '.join(f'{v:.1f}' for v in np.percentile(diff, 99, axis=0)))
    return diff


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('area')
    parser.add_argument('--layout', default='', help="the working tree's layout to build (a scratch copy)")
    parser.add_argument('--macro', action='store_true', help='paint and compare the macro maps too')
    parser.add_argument('--except', dest='except_file', default='', help='JSON of the areas meant to change')
    parser.add_argument('--blender', default='')
    parser.add_argument('--work', default=os.path.join(tempfile.gettempdir(), 'terrain_compare'))
    parser.add_argument('--fresh', action='store_true')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    blender = find_blender(args.blender)
    work = os.path.join(os.path.abspath(args.work), args.area)
    os.makedirs(work, exist_ok=True)
    flags = ['--computed'] + (['--macro'] if args.macro else [])
    print(f'Terrain compare: {args.area}, the working tree against HEAD ({git("rev-parse", "--short", "HEAD").strip()}),'
          f' inside HEAD\'s playable boundary', flush=True)
    started = time.time()
    old_out, seconds = head_build(blender, args.area, work, flags, args.fresh)
    print("  HEAD's build: " + ('kept from an earlier run against the same commit' if seconds is None else
                                f'{seconds:.0f} s'), flush=True)
    new_out = os.path.join(work, 'working-tree')
    shutil.rmtree(new_out, ignore_errors=True)
    seconds = build(blender, REPO, args.area, new_out, flags, os.path.abspath(args.layout) if args.layout else None)
    print(f"  the working tree's build: {seconds:.0f} s\n", flush=True)

    old, new = np.load(os.path.join(old_out, 'rasters.npz')), np.load(os.path.join(new_out, 'rasters.npz'))
    old_half, new_half = float(old['half']), float(new['half'])
    head_layout = json.loads(git('show', f'HEAD:Art/Levels/{args.area}/layout.json'))
    poly_m = np.asarray(head_layout['boundary']['polygon'], dtype=np.float64) / 100.0
    n = old['h'].shape[0]
    inside = old['play_edge'] > 0.0
    except_mask = None
    excepted = []
    if args.except_file:
        with open(args.except_file, encoding='utf-8') as file:
            excepted = json.load(file)['areas']
        except_mask = np.zeros((n, n), dtype=bool)
        for spot in excepted:
            except_mask |= polygon_cells(np.asarray(spot['polygon'], dtype=np.float64) / 100.0, n, old_half)
    print(f"The map square: HEAD's {2 * old_half:g} m ({n} cells), the working tree's {2 * new_half:g} m "
          f"({new['h'].shape[0]} cells)")
    h_new, how = onto_old(new['h'], new_half, n, old_half)
    print(f'  the heights compared {how}\n')
    worst = summarize('heights', old['h'], h_new, inside, except_mask=except_mask, half=old_half, show_patches=True)
    for spot in excepted:
        mask = inside & polygon_cells(np.asarray(spot['polygon'], dtype=np.float64) / 100.0, n, old_half)
        if mask.any():
            d = np.abs(h_new - old['h'])[mask] * 100.0
            print(f"  inside {spot['id']}: {int(mask.sum())} cells, the largest difference {d.max():.2f} cm, "
                  f"{int((d > TOLERANCE_CM).sum())} over {TOLERANCE_CM:g} cm")
    print()
    for name in ('ao', 'curvature'):
        o, w = old[name], new[name]
        mapped, how = onto_old(w, new_half, o.shape[0], old_half)
        inner = bilinear(old['play_edge'], old_half, *np.meshgrid(cell_centers(o.shape[0], old_half),
                                                                  cell_centers(o.shape[0], old_half),
                                                                  indexing='ij')) > 0.0
        ex = None
        if except_mask is not None:
            ex = np.zeros(o.shape, dtype=bool)
            for spot in excepted:
                ex |= polygon_cells(np.asarray(spot['polygon'], dtype=np.float64) / 100.0, o.shape[0], old_half)
        summarize(f'{name} ({how}, {o.shape[0]} and {w.shape[0]} cells)', o, mapped, inner, unit_cm=False,
                  except_mask=ex)
    if args.macro:
        print()
        with open(os.path.join(old_out, 'layout_computed.json'), encoding='utf-8') as file:
            old_computed = json.load(file)
        with open(os.path.join(new_out, 'layout_computed.json'), encoding='utf-8') as file:
            new_computed = json.load(file)
        compare_macro(old_out, new_out, old_computed, new_computed, poly_m)
    if not args.keep:
        shutil.rmtree(new_out, ignore_errors=True)
    verdict = worst <= TOLERANCE_CM
    print(f'\n{"PASS" if verdict else "FAIL"}: the heights inside HEAD\'s boundary'
          + (' (outside the --except areas)' if except_mask is not None else '')
          + f' differ by at most {worst:.4f} cm (allowed {TOLERANCE_CM:g} cm) ({time.time() - started:.0f} s)')
    return 0 if verdict else 1


if __name__ == '__main__':
    sys.exit(main())
