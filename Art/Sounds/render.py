"""Renders the game's sounds from their recipes, mixes them (cues.json) and checks them. Run in Blender's Python,
through Tools/sounds.ps1:

    blender -b --factory-startup --python Art/Sounds/render.py -- [--render] [--mix] [--check] [--shard i/n] [cues...]

cues are names or patterns (Weapon.*, UI.Hit*; dots or underscores), all cues when none. --render writes
Art/Sounds/Out/<Cue_With_Underscores>_<NN>.wav; --mix writes Art/Sounds/cues.json from every cue's files; --check
measures, draws and lists them in Saved/SoundCheck (index.html to listen). --shard i/n renders every n-th cue from
the i-th, so several Blenders can share the work.
"""
import fnmatch
import json
import os
import sys
import time
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import numpy as np  # noqa: E402

from lib import core, wav, analysis  # noqa: E402
from lib import filters as F  # noqa: E402
import recipes  # noqa: E402

OUT = os.path.join(HERE, 'Out')
CUES_JSON = os.path.join(HERE, 'cues.json')
REFERENCE = 'Weapon.Rifle.Fire'
PEAK_DB = -1.05  # a hair under the contract's -1 dBFS, so dither can't push a sample over


def parse(argv):
    args = argv[argv.index('--') + 1:] if '--' in argv else []
    opts = {'render': False, 'mix': False, 'check': False, 'shard': (0, 1), 'patterns': [], 'zoom': None}
    i = 0
    while i < len(args):
        a = args[i]
        if a in ('--render', '--mix', '--check'):
            opts[a[2:]] = True
        elif a == '--zoom':
            opts['zoom'] = float(args[i + 1])
            i += 1
        elif a == '--shard':
            k, n = args[i + 1].split('/')
            opts['shard'] = (int(k), int(n))
            i += 1
        else:
            opts['patterns'].extend(p for p in a.split(',') if p)
        i += 1
    if not (opts['render'] or opts['mix'] or opts['check']):
        opts['render'] = opts['mix'] = opts['check'] = True
    return opts


def select(registry, patterns):
    if not patterns:
        return list(registry.values())
    out = []
    for spec in registry.values():
        for p in patterns:
            p2 = p.replace('_', '.')
            if fnmatch.fnmatch(spec.name.lower(), p2.lower()) or fnmatch.fnmatch(spec.name.lower(), p.lower()):
                out.append(spec)
                break
    return out


def finish(x, spec):
    """The same last steps for every sound: mono for 3D, DC and rumble out, trimmed, faded (loops stay whole)."""
    x = np.asarray(x, dtype=float)
    if x.ndim == 2 and x.shape[0] == 1:
        x = x[0]
    if spec.space == '3D' and x.ndim == 2:
        x = x.mean(axis=0)
    if not np.all(np.isfinite(x)):
        raise ValueError(f'{spec.name}: the recipe made NaN or infinite samples')
    if spec.loop:
        return F.dc_block(x, circular=True)
    x = F.dc_block(x)
    x = core.trim_head(x, -54.0)
    x = core.trim_tail(x, -64.0)
    return core.fade(x, 0.0003, 0.0)


def render_cue(spec):
    xs = []
    for v in range(spec.variations):
        r = core.rng(spec.name, v)
        xs.append(finish(spec.fn(v, r), spec))
    if spec.align and len(xs) > 1:
        lv = [analysis.loudness_max(x) for x in xs]
        mid = float(np.median(lv))
        xs = [x * float(core.db2a(np.clip(mid - l, -6.0, 6.0))) for x, l in zip(xs, lv)]
    top = max(core.peak(x) for x in xs)
    g = float(core.db2a(PEAK_DB)) / top
    os.makedirs(OUT, exist_ok=True)
    for v, x in enumerate(xs):
        wav.write(os.path.join(OUT, spec.file(v)), x * g, seed=core.seed_of(spec.name, v, 'dither'))
    # Files from an older, longer run of this cue would linger: remove them.
    v = spec.variations
    while os.path.exists(os.path.join(OUT, spec.file(v))):
        os.remove(os.path.join(OUT, spec.file(v)))
        v += 1


def render(specs, shard):
    k, n = shard
    mine = specs[k::n]
    failed = []
    for spec in mine:
        t0 = time.time()
        try:
            render_cue(spec)
            print(f'  {spec.name}: {spec.variations} files, {time.time() - t0:.1f} s', flush=True)
        except Exception:
            failed.append(spec.name)
            print(f'  {spec.name}: FAILED', flush=True)
            traceback.print_exc()
    return failed


def mix(registry):
    """cues.json: every cue with files, its mix settings, and volume_db from its measured loudness and its level."""
    loud = {}
    for spec in registry.values():
        vals = []
        for f in spec.files():
            p = os.path.join(OUT, f)
            if os.path.exists(p):
                x, _ = wav.read(p)
                vals.append(analysis.loudness_max(x))
        if vals:
            loud[spec.name] = float(np.mean(vals))
    if REFERENCE not in loud:
        print('  mix: the reference cue has no files yet; volumes are against 0 LU')
    ref = loud.get(REFERENCE, -12.0)
    vol = {name: registry[name].level - (l - ref) for name, l in loud.items()}
    shift = -max(vol.values()) if vol and max(vol.values()) > 0 else 0.0
    out = {}
    for spec in registry.values():
        if spec.name not in loud:
            continue
        out[spec.name] = {
            'files': [f for f in spec.files() if os.path.exists(os.path.join(OUT, f))],
            'volume_db': round((vol[spec.name] + shift) * 2.0) / 2.0,
            'pitch_jitter': spec.jitter,
            'class': spec.cls,
            'attenuation': spec.att,
            'loop': spec.loop,
            'space': spec.space,
            'max_concurrent': spec.conc,
        }
    with open(CUES_JSON, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(out, f, indent=2)
        f.write('\n')
    print(f'  cues.json: {len(out)} cues' + (f' (all shifted {shift:+.1f} dB)' if shift else ''))


def main():
    opts = parse(sys.argv)
    registry = recipes.load_all()
    specs = select(registry, opts['patterns'])
    if opts['patterns'] and not specs:
        raise SystemExit(f'No cue matches {opts["patterns"]}')
    failed = []
    if opts['render']:
        t0 = time.time()
        failed = render(specs, opts['shard'])
        print(f'Rendered shard {opts["shard"][0]}/{opts["shard"][1]} in {time.time() - t0:.1f} s', flush=True)
    if opts['mix']:
        mix(registry)
    if opts['check']:
        import check
        check.run(registry, specs, opts['zoom'])
    if failed:
        raise RuntimeError('Failed: ' + ', '.join(failed))


main()
