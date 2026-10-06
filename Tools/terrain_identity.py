"""Checks that the terrain generator still builds an area exactly as committed. Run it with Blender's own Python, which
has numpy and OpenImageIO (Tools/terrain_identity.ps1 finds it):

    Tools/terrain_identity.ps1 [-Area TutorialIsland] [-Fresh] [-Keep]
    <Blender>/<version>/python/bin/python.exe Tools/terrain_identity.py [Area] [--blender EXE] [--fresh] [--keep]

It builds the area twice, headless in Blender: with the committed generator (HEAD's Art/Levels, Art/Models/Terrain and
Tools/Blender, taken with git archive into a temp folder) for the meshes, and with the working tree's for everything
(--computed --macro --save-to, so nothing in the repository is written). Then it compares:
- layout_computed.json against HEAD's (git show): every number within 0.1 cm, every text and count equal. Keys that
  HEAD's file lacks are listed as new (and allowed); a key it has must still be there. layoutSha1 is left out: it
  records which layout the file came from, not what the generator built (terrain_check.py checks it).
- The meshes (<Area>_Tile_*, <Area>_Underside_*, <Area>_Water): the same names, vertex and triangle counts, and hashes
  of their positions, faces, UVs, vertex colors, normals, transforms, materials and properties
  (Tools/Blender/terrain_fingerprint.py).
- The macro color map and the scatter mask against HEAD's (their Git LFS objects), pixel for pixel.
It prints each result, the time and peak memory of both builds, and PASS or FAIL (exit code 1). Git's state is never
touched: it only reads (rev-parse, show, archive, lfs smudge).

HEAD's meshes are kept in the temp folder (%TEMP%/terrain_identity/<Area>) by the tree ids of those three folders, so
a second check against the same commit skips that build; --fresh builds them again. --keep keeps the working tree's
outputs there too (the maps, layout_computed.json, previews/ and the Blender logs).
"""
import argparse
import glob
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time

import numpy as np

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
GENERATOR = ('Art/Levels', 'Art/Models/Terrain', 'Tools/Blender')  # what a build of an area reads
HOOK = os.path.join(REPO, 'Tools', 'Blender', 'terrain_fingerprint.py')
TOLERANCE_CM = 0.1


def git(*args, binary=False, stdin=None):
    result = subprocess.run(['git', '-C', REPO, *args], input=stdin, capture_output=True, check=True)
    return result.stdout if binary else result.stdout.decode('utf-8')


def find_blender(given):
    installed = sorted(glob.glob(r'C:\Program Files\Blender Foundation\*\blender.exe'), reverse=True)
    for path in [given, os.environ.get('BLENDER')] + installed:
        if path and os.path.exists(path):
            return path
    raise SystemExit('Blender not found: pass --blender, or set BLENDER to blender.exe')


def run_blender(blender, tree, area, flags, log_path):
    """Builds the area with the terrain model under tree (a repository's root) and fingerprints its meshes. Returns
    the fingerprint (the meshes, and the run's time and memory)."""
    helpers = os.path.join(tree, 'Tools', 'Blender').replace('\\', '/')
    fingerprint = os.path.join(os.path.dirname(log_path), 'fingerprint.json')
    # A scripted model starts from an empty scene, as models.ps1 runs it.
    setup = f"import sys; sys.path.append('{helpers}'); import bpy; bpy.ops.wm.read_factory_settings(use_empty=True)"
    command = [blender, '-b', '--factory-startup', '--python-exit-code', '2', '--python-expr', setup,
               '--python', os.path.join(tree, 'Art', 'Models', 'Terrain', area + '.py'), '--python', HOOK, '--',
               *flags, '--fingerprint-out', fingerprint, '--fingerprint-prefix', area + '_']
    with open(log_path, 'w', encoding='utf-8', errors='replace') as log:
        code = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT).returncode
    if code != 0 or not os.path.exists(fingerprint):
        with open(log_path, encoding='utf-8', errors='replace') as log:
            tail = log.read().splitlines()[-25:]
        raise SystemExit(f'Blender failed (exit code {code}); the end of {log_path}:\n  ' + '\n  '.join(tail))
    with open(fingerprint, encoding='utf-8') as file:
        return json.load(file)


def head_meshes(blender, area, work, fresh):
    """HEAD's meshes: its generator taken out of git into the temp folder and built there (meshes only, so it writes
    nothing). Cached by the tree ids of the folders the build reads."""
    trees = git('rev-parse', *[f'HEAD:{folder}' for folder in GENERATOR]).split()
    key = hashlib.sha1(' '.join(trees).encode()).hexdigest()[:12]
    folder = os.path.join(work, 'head-' + key)
    cached = os.path.join(folder, 'fingerprint.json')
    if os.path.exists(cached) and not fresh:
        with open(cached, encoding='utf-8') as file:
            return json.load(file), True
    shutil.rmtree(folder, ignore_errors=True)
    tree = os.path.join(folder, 'tree')
    os.makedirs(tree)
    with tarfile.open(fileobj=io.BytesIO(git('archive', '--format=tar', 'HEAD', *GENERATOR, binary=True))) as tar:
        tar.extractall(tree)
    if not os.path.exists(os.path.join(tree, 'Art', 'Models', 'Terrain', area + '.py')):
        raise SystemExit(f'HEAD has no Art/Models/Terrain/{area}.py to compare with')
    return run_blender(blender, tree, area, [], os.path.join(folder, 'blender.log')), False


def committed(path, out_folder):
    """A file as HEAD has it: a Git LFS file's content comes from the local LFS store (or git lfs smudge, which may
    download it)."""
    data = git('show', f'HEAD:{path}', binary=True)
    if data.startswith(b'version https://git-lfs'):
        oid = next(line.split(':', 1)[1] for line in data.decode().splitlines() if line.startswith('oid sha256:'))
        store = os.path.join(REPO, git('rev-parse', '--git-common-dir').strip(), 'lfs', 'objects', oid[:2], oid[2:4],
                             oid)
        if os.path.exists(store):
            with open(store, 'rb') as file:
                data = file.read()
        else:
            data = git('lfs', 'smudge', binary=True, stdin=data)
    out = os.path.join(out_folder, 'HEAD_' + os.path.basename(path))
    with open(out, 'wb') as file:
        file.write(data)
    return out


# --- Comparisons ---

def compare_json(old, new, where=''):
    """Walks two JSON values: returns (numbers compared, largest difference, differences, keys only the new one has)."""
    if isinstance(old, dict) and isinstance(new, dict):
        totals = [0, 0.0, [], [f'{where}.{k}'.lstrip('.') for k in new if k not in old]]
        for key in old:
            if key not in new:
                totals[2].append(f'{where}.{key}'.lstrip('.') + ': missing')
                continue
            count, largest, differences, added = compare_json(old[key], new[key], f'{where}.{key}')
            totals = [totals[0] + count, max(totals[1], largest), totals[2] + differences, totals[3] + added]
        return tuple(totals)
    if isinstance(old, list) and isinstance(new, list):
        if len(old) != len(new):
            return 0, 0.0, [f'{where.lstrip(".")}: {len(old)} items, now {len(new)}'], []
        totals = [0, 0.0, [], []]
        for i, (a, b) in enumerate(zip(old, new)):
            count, largest, differences, added = compare_json(a, b, f'{where}[{i}]')
            totals = [totals[0] + count, max(totals[1], largest), totals[2] + differences, totals[3] + added]
        return tuple(totals)
    numbers = (int, float)
    if isinstance(old, numbers) and isinstance(new, numbers) and not isinstance(old, bool) \
            and not isinstance(new, bool):
        difference = abs(float(old) - float(new))
        bad = [f'{where.lstrip(".")}: {old} -> {new}'] if difference > TOLERANCE_CM + 1e-9 else []
        return 1, difference, bad, []
    if old != new or type(old) is not type(new):
        return 0, 0.0, [f'{where.lstrip(".")}: {old!r} -> {new!r}'], []
    return 0, 0.0, [], []


def read_pixels(path):
    """The PNG's stored values as uint8 (height, width, channels), alpha left unassociated (OpenImageIO would
    otherwise premultiply it, and two different images could compare equal)."""
    import OpenImageIO as oiio
    config = oiio.ImageSpec()
    config.attribute('oiio:UnassociatedAlpha', 1)
    image = oiio.ImageInput.open(path, config)
    if image is None:
        raise SystemExit(f'cannot read {path}: {oiio.geterror()}')
    pixels = image.read_image(format='uint8')
    image.close()
    return pixels


def compare_png(old_path, new_path):
    """(passed, a line saying how they compare)."""
    if not os.path.exists(new_path):
        return False, 'not written'
    with open(old_path, 'rb') as a, open(new_path, 'rb') as b:
        same_bytes = a.read() == b.read()
    old, new = read_pixels(old_path), read_pixels(new_path)
    size = f'{old.shape[1]} x {old.shape[0]}, {old.shape[2]} channels'
    if old.shape != new.shape:
        return False, f'{size}, now {new.shape[1]} x {new.shape[0]}, {new.shape[2]} channels'
    if np.array_equal(old, new):
        return True, f'{size}: every pixel equal' + (' (the files are byte for byte the same)' if same_bytes else '')
    differ = np.any(old != new, axis=-1)
    largest = int(np.abs(old.astype(np.int16) - new.astype(np.int16)).max())
    return False, f'{size}: {int(differ.sum())} pixels differ (by up to {largest} of 255)'


def compare_meshes(old, new):
    """(passed, lines): one line per mesh."""
    lines, passed = [], True
    width = max([len(n) for n in list(old) + list(new)] + [4])
    for name in sorted(set(old) | set(new)):
        if name not in new or name not in old:
            lines.append(f'  {name:<{width}}  only in {"HEAD" if name in old else "the working tree"}')
            passed = False
            continue
        a, b = old[name], new[name]
        changed = [key for key in sorted(set(a) | set(b)) if a.get(key) != b.get(key)]
        counts = f"{b['vertices']:>6} vertices {b['triangles']:>6} triangles"
        verdict = 'DIFFERS: ' + ', '.join(changed) if changed else 'identical'
        lines.append(f'  {name:<{width}}  {counts}  {verdict}')
        passed &= not changed
    return passed, lines


def run_line(label, stats, cached=False):
    if not stats:
        return f'  {label}: (no timing on this system)'
    return (f"  {label}: {stats['seconds']:.0f} s ({stats['cpu_seconds']:.0f} s CPU), peak memory "
            f"{stats['peak_working_set_mb']} MB working set, {stats['peak_private_mb']} MB private"
            + (' (from an earlier run against the same commit)' if cached else ''))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('area', nargs='?', default='TutorialIsland')
    parser.add_argument('--blender', default='')
    parser.add_argument('--work', default=os.path.join(tempfile.gettempdir(), 'terrain_identity'))
    parser.add_argument('--fresh', action='store_true', help="build HEAD's meshes again even if cached")
    parser.add_argument('--keep', action='store_true', help="keep the working tree's outputs in the temp folder")
    args = parser.parse_args()
    area = args.area
    blender = find_blender(args.blender)
    work = os.path.join(os.path.abspath(args.work), area)
    os.makedirs(work, exist_ok=True)
    head = git('rev-parse', '--short', 'HEAD').strip()
    print(f'Terrain identity: {area}, the working tree against HEAD ({head}), with {blender}', flush=True)

    started = time.time()
    old, cached = head_meshes(blender, area, work, args.fresh)
    print("  HEAD's meshes: " + ('kept from an earlier check against the same commit' if cached else
                                 f'built with its generator ({time.time() - started:.0f} s)'), flush=True)
    new_folder = os.path.join(work, 'working-tree')
    shutil.rmtree(new_folder, ignore_errors=True)
    os.makedirs(new_folder)
    new = run_blender(blender, REPO, area, ['--computed', '--macro', '--save-to', new_folder],
                      os.path.join(new_folder, 'blender.log'))
    print("  the working tree's generator built the meshes, layout_computed.json and the maps", flush=True)
    print(run_line("HEAD's build (meshes)", old.get('_run'), cached))
    print(run_line("working tree's build (meshes, computed layout, maps)", new.get('_run')))
    results = []

    # layout_computed.json
    path = f'Art/Levels/{area}/layout_computed.json'
    old_layout = json.loads(git('show', f'HEAD:{path}'))
    with open(os.path.join(new_folder, 'layout_computed.json'), encoding='utf-8') as file:
        new_layout = json.load(file)
    for computed in (old_layout, new_layout):
        computed.pop('layoutSha1', None)  # provenance, not output
    count, largest, differences, added = compare_json(old_layout, new_layout)
    passed = not differences
    results.append(passed)
    print(f'\n{path}: {"PASS" if passed else "FAIL"}')
    print(f'  {count} numbers compared, the largest difference {largest:.3g} cm (allowed {TOLERANCE_CM} cm); '
          f'{len(differences)} differences')
    for line in differences[:20]:
        print('  ' + line)
    if len(differences) > 20:
        print(f'  ... and {len(differences) - 20} more')
    if added:
        print('  new in the working tree (not in HEAD): ' + ', '.join(added))

    # The meshes
    passed, lines = compare_meshes(old['meshes'], new['meshes'])
    results.append(passed)
    print(f'\nMeshes ({len(new["meshes"])}): {"PASS" if passed else "FAIL"}')
    print('\n'.join(lines))
    totals = [sum(m[k] for m in new['meshes'].values()) for k in ('vertices', 'triangles')]
    print(f'  in all {totals[0]} vertices, {totals[1]} triangles')

    # The maps
    print()
    for texture in (old_layout['macroMap']['texture'], old_layout['macroMap']['scatterMap']['texture']):
        passed, line = compare_png(committed(texture, new_folder),
                                   os.path.join(new_folder, os.path.basename(texture)))
        results.append(passed)
        print(f'{texture}: {"PASS" if passed else "FAIL"}\n  {line}')

    if not args.keep:
        shutil.rmtree(new_folder, ignore_errors=True)
    verdict = all(results)
    print(f'\n{"PASS" if verdict else "FAIL"}: {area} {"regenerates identically" if verdict else "changed"} '
          f'({time.time() - started:.0f} s)')
    return 0 if verdict else 1


if __name__ == '__main__':
    sys.exit(main())
