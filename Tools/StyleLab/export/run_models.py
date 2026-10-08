"""Runs model_export.py over every model script the lab can use, one process per script (a few kits leave state
behind), a few at a time. A script is skipped when its outputs are newer than it and every kit in Tools/Blender
(--force runs them all).

    /home/user/bpyenv/bin/python Tools/StyleLab/export/run_models.py [--force] [--jobs 3] [Category/Name.py ...]
"""
import glob
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402

PY = sys.executable
# The terrain is exported by terrain_export.py; the tutorial island's and the test area's terrain aren't used.
SKIP = ('Terrain/RansomsRest.py', 'Terrain/TerrainTest.py', 'Terrain/TutorialIsland.py', 'Rocks/SkyIslands.py')
STAMP = os.path.join(lc.WORK, 'models', '_stamps.json')


def sources():
    out = []
    for path in sorted(glob.glob(os.path.join(lc.REPO, 'Art', 'Models', '*', '*.py'))):
        rel = os.path.relpath(path, os.path.join(lc.REPO, 'Art', 'Models')).replace(os.sep, '/')
        if rel not in SKIP:
            out.append(rel)
    return out


def kit_hash():
    return lc.sha1_of_files(glob.glob(os.path.join(lc.REPO, 'Tools', 'Blender', '*.py'))
                            + [os.path.join(lc.HERE, 'model_export.py'), os.path.join(lc.HERE, 'labcommon.py')])


def main():
    args = sys.argv[1:]
    force = '--force' in args
    jobs = int(args[args.index('--jobs') + 1]) if '--jobs' in args else 3
    wanted = [a for a in args if a.endswith('.py')] or sources()
    os.makedirs(os.path.join(lc.WORK, 'logs'), exist_ok=True)
    try:
        with open(STAMP) as f:
            stamps = json.load(f)
    except (OSError, ValueError):
        stamps = {}
    kits = kit_hash()
    todo = []
    for rel in wanted:
        key = lc.sha1_of_files([os.path.join(lc.REPO, 'Art', 'Models', rel)]) + kits
        if not force and stamps.get(rel, {}).get('key') == key:
            continue
        todo.append((rel, key))
    print(f'STYLELAB: {len(todo)} of {len(wanted)} model scripts to run, {jobs} at a time', flush=True)

    def one(item):
        rel, key = item
        log = os.path.join(lc.WORK, 'logs', rel.replace('/', '_') + '.log')
        started = time.time()
        with open(log, 'w') as f:
            code = subprocess.call([PY, os.path.join(lc.HERE, 'model_export.py'), rel], stdout=f,
                                   stderr=subprocess.STDOUT, cwd=lc.REPO,
                                   env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
        with open(log) as f:
            lines = [ln.strip() for ln in f if ln.startswith('STYLELAB:')]
        failed = [ln for ln in lines if 'FAILED' in ln] or ([] if code == 0 else [f'exit code {code}'])
        made = [ln.split()[2] for ln in lines if ln.startswith('STYLELAB: EXPORT ')]
        print(f'STYLELAB: {rel:34s} {len(made):3d} models {time.time() - started:6.1f} s'
              + (f'  FAILED: {failed[0][:200]}' if failed else ''), flush=True)
        return rel, key, made, failed

    with ThreadPoolExecutor(jobs) as pool:
        for rel, key, made, failed in pool.map(one, todo):
            if not failed:
                stamps[rel] = {'key': key, 'models': made}
            else:
                stamps.pop(rel, None)
            lc.write_json(STAMP, stamps, indent=1)
    print('STYLELAB: models done', flush=True)


if __name__ == '__main__':
    main()
