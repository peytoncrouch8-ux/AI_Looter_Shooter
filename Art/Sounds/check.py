"""Checks the rendered sounds, since nobody here can listen: numbers per file, flags for what's wrong, a picture per
cue (each variation's waveform with its dB level, and its spectrogram on a log-frequency scale), and a page to
listen on. Writes into Saved/SoundCheck: <Cue_With_Underscores>.png, report.txt, report.json and index.html.

Run by render.py --check (Tools/sounds.ps1 runs it after every render).
"""
import html
import json
import os

import numpy as np

from lib import analysis, wav
from lib import png as P
from lib.core import SR

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'Out')
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
CHECK = os.path.join(ROOT, 'Saved', 'SoundCheck')

# Sounds the player hears over and over: these must not bite in the 2-5 kHz band, where the ear is most sensitive.
FREQUENT = ['Weapon.Rifle.Fire', 'Weapon.Shotgun.Fire', 'Weapon.*.Core', 'Weapon.*.Mech', 'Weapon.*.Tail*', 'Impact.*', 'UI.HitMarker', 'UI.Click', 'UI.Hover', 'UI.Tab',
            'Player.Footstep.*', 'Loot.AmmoPickup', 'Weapon.Shotgun.ShellIn', 'Creature.Hit', 'Player.SlideLoop',
            'Weapon.AimIn', 'Creature.Spider.Hit', 'Creature.Slime.Hit', 'Creature.Unpaid.Hit',
            'Loot.Drop.Uncommon', 'Loot.Drop.Rare', 'Boss.Loot.Pop', 'Boss.Gravemother.CrackBurn', 'Boss.Gravemother.StoneHit',
            'Creature.Spider.Burst', 'Creature.Slime.Splat', 'UI.Equip', 'UI.Stow', 'Player.Melee.*', 'UI.Hold.Complete',
            'World.Gust.*', 'Creature.Chicken.Cluck', 'Weapon.Casing.*', 'Weapon.NearMiss.*', 'Creature.Spider.Twitch',
            'Creature.Slime.Bubble', 'Player.Splash', 'Loot.Tier.Pop.*', 'Creature.Trait.Mend', 'Creature.Trait.Rally',
            'Creature.Trait.WardHit', 'UI.HitMarker.*', 'Creature.Hit.Heavy', 'Creature.Crit.*', 'Weapon.Shotgun.Rack',
            'Weapon.Maker.*', 'Weapon.*.Punch', 'Weapon.*.AimIn', 'Weapon.AimOut', 'Creature.Spider.LegCrack',
            'Creature.Hit.Overkill']


def _frequent(name):
    import fnmatch
    return any(fnmatch.fnmatch(name, p) for p in FREQUENT)


def flags_for(spec, m, rate, x):
    f = []
    if rate != SR:
        f.append(f'RATE {rate}')
    if m['peak_db'] > -1.0:
        f.append(f'PEAK {m["peak_db"]:.2f}')
    if m['clipped']:
        f.append(f'CLIP x{m["clipped"]}')
    if m.get('clicks'):
        f.append(f'CLICK x{m["clicks"]}')
    if m['dc'] > 0.002:
        f.append(f'DC {m["dc"]:.4f}')
    if spec.space == '3D' and m['channels'] != 1:
        f.append('STEREO IN 3D')
    if not spec.loop:
        if m['head_ms'] > 5.0 and not spec.swell:
            f.append(f'LATE START {m["head_ms"]:.0f} MS')
        if m['tail_db'] > -45.0:
            f.append(f'CUT TAIL {m["tail_db"]:.0f} DB')
    else:
        # A loop's seam: no jump from the last sample to the first, and no click where it joins (the loop is turned
        # half way round so the join sits in the middle, where the click check can see it). A level change across the
        # join is reported but not flagged: a heartbeat starts on a beat and ends in a lull, and that's right.
        jump, lv = analysis.seam(x)
        m['seam_jump'] = jump
        m['seam_level_db'] = lv
        turned = np.roll(x, x.shape[-1] // 2, axis=-1)
        before = analysis.quiet_clicks(x, quiet_db=0.0, ratio=20.0)
        m['seam_clicks'] = analysis.quiet_clicks(turned, quiet_db=0.0, ratio=20.0) - before
        if jump > 6.0:
            f.append(f'SEAM JUMP {jump:.1f}')
        if m['seam_clicks'] > 0:
            f.append('SEAM CLICK')
    # Harsh: a frequent sound with too much of its energy at 2-5 kHz, or a sharp spike there that carries real weight.
    if _frequent(spec.name) and (m['bands']['presence'] > 0.35 or
                                 (m['spike_2_5k'] > 8.0 and m['bands']['presence'] > 0.15)):
        f.append(f'HARSH 2-5K {m["bands"]["presence"] * 100:.0f}% SPIKE {m["spike_2_5k"]:.1f} DB')
    return f


def _spec_db(x, w, h, dur):
    """The spectrogram's dB matrix (h, w) of mono x over dur seconds."""
    if dur <= 0.15:
        nfft = 256
    elif dur <= 0.6:
        nfft = 512
    elif dur <= 3.0:
        nfft = 1024
    else:
        nfft = 2048
    win = np.hanning(nfft)
    pad = np.concatenate([np.zeros(nfft // 2), x, np.zeros(nfft)])
    f = np.fft.rfftfreq(nfft, 1.0 / SR)
    rows = P.F_LO * (P.F_HI / P.F_LO) ** (np.arange(h)[::-1] / (h - 1))
    img = np.full((h, w), 1e-20)
    centers = ((np.arange(w) + 0.5) / w * dur * SR).astype(int)
    valid = centers < len(x) + nfft // 2
    idx = np.nonzero(valid)[0]
    if idx.size:
        frames = np.stack([np.pad(pad[c:c + nfft], (0, max(0, nfft - pad[c:c + nfft].size))) for c in centers[idx]])
        mag = np.abs(np.fft.rfft(frames * win, axis=1)) ** 2
        for j, i in enumerate(idx):
            img[:, i] = np.interp(rows, f, mag[j])
    return 10 * np.log10(img + 1e-20)


def sheet(spec, xs, metrics, flags, zoom=None):
    """One picture for a cue: each variation's waveform (blue), its RMS level in dB (orange, -60..0) and spectrogram
    (30 Hz - 22 kHz, 90 dB range against the cue's loudest moment), on a shared time scale with 100 ms grid lines.
    zoom: show only the first zoom seconds (10 ms grid), to see how the hits start."""
    W = 960
    left = 34
    pw = W - left - 8
    row_h = 12 + 48 + 104 + 8
    H = 22 + row_h * len(xs)
    dur = zoom if zoom else max(max(len(x) for x in xs) / SR, 0.05)
    c = P.Canvas(W, H)
    grid = 0.01 if dur <= 0.25 else (0.1 if dur <= 2.5 else 0.5)
    c.text(4, 6, f'{spec.name}   {spec.space} {spec.cls} {spec.att}   {dur:.2f} S   GRID {grid:g} S'
           + ('   LOOP' if spec.loop else ''), (255, 200, 120))
    dbs = [_spec_db(x, pw, 104, dur) for x in xs]
    top = max(float(d.max()) for d in dbs)
    for k, (x, m, fl, d) in enumerate(zip(xs, metrics, flags, dbs)):
        y = 22 + k * row_h
        label = (f'{k + 1:02d}  PK {m["peak_db"]:.1f}  L200 {m["l200"]:.1f}  {m["duration"]:.2f}S  '
                 f'C {m["centroid"]:.0f}  2-5K {m["bands"]["presence"] * 100:.0f}%')
        c.text(left, y + 2, label, (200, 200, 210))
        if fl:
            c.text(left + 6 * (len(label) + 2), y + 2, ' '.join(fl), (255, 90, 90))
        wy = y + 12
        c.rect(left, wy, left + pw, wy + 48, (26, 28, 36))
        sy = wy + 50
        v = (d - (top - 90.0)) / 90.0
        c.image(left, sy, P.heat(v))
        # Grid: time lines and the 100 Hz, 1 kHz and 10 kHz rows, and the 2-5 kHz band edges.
        for i in range(1, int(dur / grid) + 1):
            t = i * grid
            gx = left + int(t / dur * pw)
            c.blend_rect(gx, wy, gx + 1, sy + 104, (255, 255, 255), 0.4 if i % 10 == 0 else 0.18)
        for fr, lab in ((100.0, '100'), (1000.0, '1K'), (10000.0, '10K')):
            gy = sy + P.freq_row(fr, 104)
            c.blend_rect(left, gy, left + pw, gy + 1, (255, 255, 255), 0.25)
            c.text(4, gy - 3, lab, (150, 150, 160))
        for fr in (2000.0, 5000.0):
            gy = sy + P.freq_row(fr, 104)
            c.blend_rect(left, gy, left + pw, gy + 1, (120, 220, 255), 0.25)
        P.waveform(c, x, left, wy, pw, 48, dur)
    return c


def _audio_html(files, loop):
    out = []
    for f in files:
        out.append(f'<div class="f"><span>{html.escape(f[-6:-4])}</span><audio controls preload="none"'
                   f'{" loop" if loop else ""} src="../../Art/Sounds/Out/{html.escape(f)}"></audio></div>')
    return ''.join(out)


def page(registry, cues_json, results):
    groups = {}
    for spec in registry.values():
        groups.setdefault(spec.group, []).append(spec)
    parts = []
    for g, specs in groups.items():
        parts.append(f'<h2>{html.escape(g.title())}</h2>')
        for spec in specs:
            entry = cues_json.get(spec.name, {})
            files = entry.get('files', [])
            res = results.get(spec.name, {})
            fl = sorted({f for fs in res.get('flags', []) for f in fs})
            vol = entry.get('volume_db', 0.0)
            meta = (f'{spec.space} · {spec.cls} · {spec.att} · volume {vol:+.1f} dB · pitch ±{spec.jitter * 100:.0f}% · '
                    f'max {spec.conc}' + (' · loop' if spec.loop else ''))
            parts.append(
                f'<section data-vol="{vol}"><header><b>{html.escape(spec.name)}</b><small>{html.escape(meta)}</small>'
                f'<button onclick="playAll(this)">Play all</button></header>'
                + (f'<p class="flags">{html.escape(", ".join(fl))}</p>' if fl else '')
                + (f'<div class="files">{_audio_html(files, spec.loop)}</div>' if files else '<p>No files yet.</p>')
                + (f'<a href="{spec.stem}.png" target="_blank"><img loading="lazy" src="{spec.stem}.png"></a>'
                   if os.path.exists(os.path.join(CHECK, spec.stem + '.png')) else '')
                + '</section>')
    css = """
:root { --bg:#14151a; --panel:#1d1f27; --ink:#e8e6e1; --dim:#9a9aa3; --accent:#e8892f; --line:#3fb6c9; --bad:#ff6b6b; }
body { margin:0; background:var(--bg); color:var(--ink); font:14px/1.4 system-ui, sans-serif; }
main { max-width:1100px; margin:0 auto; padding:16px; }
h1 { color:var(--accent); font-size:22px; margin:8px 0; }
h2 { border-bottom:1px solid var(--line); padding-bottom:4px; margin-top:28px; }
section { background:var(--panel); border-left:3px solid var(--accent); margin:12px 0; padding:10px 12px; }
header { display:flex; flex-wrap:wrap; gap:10px; align-items:baseline; }
header small { color:var(--dim); }
button { background:#2b2e39; color:var(--ink); border:1px solid #444; padding:2px 10px; cursor:pointer; }
.files { display:flex; flex-wrap:wrap; gap:6px 14px; margin:8px 0; }
.f { display:flex; align-items:center; gap:4px; } .f span { color:var(--dim); font-size:12px; }
audio { height:30px; width:220px; }
img { max-width:100%; margin-top:6px; }
.flags { color:var(--bad); margin:4px 0; }
.bar { position:sticky; top:0; background:var(--bg); padding:8px 0; border-bottom:1px solid #333; z-index:2; }
"""
    js = """
function vol(section){ return document.getElementById('mix').checked ? Math.min(1, Math.pow(10, parseFloat(section.dataset.vol)/20)) : 1; }
function playAll(btn){ const s = btn.closest('section'); const list = [...s.querySelectorAll('audio')]; let i = 0;
  const next = () => { if (i >= list.length) return; const a = list[i++]; a.volume = vol(s); a.currentTime = 0; a.onended = () => setTimeout(next, 250); a.play(); };
  next(); }
document.addEventListener('play', e => { const s = e.target.closest('section'); if (s) e.target.volume = vol(s); }, true);
"""
    total = sum(len(cues_json.get(s.name, {}).get('files', [])) for s in registry.values())
    return (f'<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" '
            f'content="width=device-width, initial-scale=1"><title>Sound Check</title><style>{css}</style></head><body>'
            f'<main><h1>Revenant: sound check</h1><div class="bar"><label><input type="checkbox" id="mix" checked> '
            f'Play at the mix volume (cues.json)</label> · {len(cues_json)} cues, {total} files · synthesized by '
            f'Art/Sounds recipes</div>{"".join(parts)}</main><script>{js}</script></body></html>')


def run(registry, selected, zoom=None):
    os.makedirs(CHECK, exist_ok=True)
    cues_path = os.path.join(HERE, 'cues.json')
    cues_json = json.load(open(cues_path, encoding='utf-8')) if os.path.exists(cues_path) else {}
    report_path = os.path.join(CHECK, 'report.json')
    results = json.load(open(report_path, encoding='utf-8')) if os.path.exists(report_path) else {}
    chosen = {s.name for s in selected}
    for spec in registry.values():
        files = [f for f in spec.files() if os.path.exists(os.path.join(OUT, f))]
        if not files or (spec.name not in chosen and spec.name in results):
            continue
        xs, ms, fls = [], [], []
        for f in files:
            x, rate = wav.read(os.path.join(OUT, f))
            m = analysis.measure(x)
            fl = flags_for(spec, m, rate, x)
            xs.append(x if x.ndim == 1 else x.mean(axis=0))
            ms.append(m)
            fls.append(fl)
        results[spec.name] = {'files': files, 'metrics': ms, 'flags': fls}
        if spec.name in chosen:
            sheet(spec, xs, ms, fls).save(os.path.join(CHECK, spec.stem + '.png'))
            if zoom:
                sheet(spec, xs, ms, fls, zoom).save(os.path.join(CHECK, spec.stem + '_zoom.png'))
    # Drop cues that no longer exist.
    results = {k: v for k, v in results.items() if k in registry}
    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=1)
    lines = []
    bad = 0
    for spec in registry.values():
        res = results.get(spec.name)
        if not res:
            lines.append(f'{spec.name:28s} (no files)')
            continue
        for f, m, fl in zip(res['files'], res['metrics'], res['flags']):
            bad += bool(fl)
            b = m['bands']
            lines.append(f'{f:34s} {m["duration"]:5.2f}s pk {m["peak_db"]:6.2f} L200 {m["l200"]:6.1f} '
                         f'rms {m["rms_db"]:6.1f} c {m["centroid"]:6.0f}Hz  sub {b["sub"] * 100:3.0f} low '
                         f'{b["low"] * 100:3.0f} lmid {b["lowmid"] * 100:3.0f} mid {b["mid"] * 100:3.0f} pres '
                         f'{b["presence"] * 100:3.0f} hi {b["high"] * 100:3.0f} air {b["air"] * 100:3.0f}  '
                         f'head {m["head_ms"]:4.1f}ms tail {m["tail_db"]:5.0f}dB  {" ".join(fl)}')
    with open(os.path.join(CHECK, 'report.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    with open(os.path.join(CHECK, 'index.html'), 'w', encoding='utf-8') as f:
        f.write(page(registry, cues_json, results))
    print(f'  check: {sum(len(r["files"]) for r in results.values())} files, {bad} flagged; '
          f'{os.path.join(CHECK, "index.html")}')
    if any(s.name.startswith('Weapon.') for s in selected):
        gun_ladder(cues_json)


# --- The guns' own-gun sums (gun identity, 2026-10-28, Docs/Work/2026-10-28-Core/GUNS.md lane D) ----------------------
# The shooter's own gun plays Core + Mech + Low + a tail + Punch (+ a maker's extra layer) together
# (Weapons/GunshotSound.cpp), each at its cues.json volume times the code's multipliers below. The game can't limit their
# sum, so it is checked here: its loudness against the rifle's (the ladder) and its worst peak over every take
# combination, surroundings and maker. Keep these tables equal to the code's defaults: Weapons/GunfeelSoundTuning.h
# (KindMix, PunchGain) and Weapons/MakerSoundFlavor.cpp (Pitch, TailPitch, Core, Mech, Low, Tail, the extra cue).
GUN_KINDS = {'Rifle': (1.0, 1.0, 1.0), 'SMG': (1.2, 0.75, 0.85), 'LMG': (1.1, 1.2, 1.0), 'Revolver': (0.9, 1.1, 1.2),
             'Shotgun': (1.0, 1.25, 1.1), 'Sniper': (0.9, 1.2, 1.35), 'Launcher': (1.0, 1.2, 1.0)}
PUNCH_GAIN = {'Rifle': 0.63, 'SMG': 0.0}
# The loudness ladder: the plain own-gun sum in the open, its short-term (3 s) max loudness in LU against the rifle's.
LADDER = {'SMG': -3.0, 'Rifle': 0.0, 'LMG': 1.0, 'Revolver': 2.0, 'Shotgun': 3.0, 'Launcher': 3.0, 'Sniper': 4.0}
# pitch, tail pitch, core, mech, low, tail, the extra cue
MAKERS = {'plain': (1.0, 1.0, 1.0, 1.0, 1.0, 1.0, None),
          'Roundhouse': (0.93, 0.97, 1.0, 1.15, 1.3, 1.1, None),
          'SaintMaud': (1.04, 1.03, 1.0, 0.85, 0.9, 1.35, 'Weapon.Maker.SaintMaud.Ring'),
          'Windfall': (1.02, 1.0, 0.95, 1.5, 0.85, 1.0, 'Weapon.Maker.Windfall.Rattle'),
          'Meridian': (1.0, 1.0, 1.0, 0.75, 0.95, 0.75, None)}
SURROUNDINGS = ('TailOpen', 'TailTown', 'TailInside')
GUN_PEAK_DB = -1.0


def _takes(cues_json, name, gain, pitch=1.0):
    from lib.core import varispeed, db2a
    entry = cues_json.get(name) if name else None
    if not entry or gain <= 0.0:
        return []
    out = []
    for f in entry['files']:
        x, _ = wav.read(os.path.join(OUT, f))
        if x.ndim == 2:
            x = x.mean(axis=0)
        x = varispeed(x, pitch) if abs(pitch - 1.0) > 1e-6 else x
        out.append(x * float(db2a(entry['volume_db'])) * gain)
    return out


def _worst_peak(groups, n):
    """The loudest sample of the sum over every combination of one take per layer in the first n samples (where the
    layers' fronts meet), or a bound for the rest (each layer's loudest sample after n, added), whichever is higher."""
    total = np.zeros((1, n))
    later = 0.0
    for xs in groups:
        if not xs:
            continue
        a = np.zeros((len(xs), n))
        for i, x in enumerate(xs):
            m = min(n, x.size)
            a[i, :m] = x[:m]
        total = (total[:, None, :] + a[None, :, :]).reshape(-1, n)
        later += max((float(np.max(np.abs(x[n:]))) if x.size > n else 0.0) for x in xs)
    return max(float(np.max(np.abs(total))), later)


def _sum(parts):
    y = np.zeros(max(x.size for x in parts))
    for x in parts:
        y[:x.size] += x
    return y


def gun_ladder(cues_json):
    from lib.core import a2db
    rows = {}
    for kind, (k_mech, k_low, k_tail) in GUN_KINDS.items():
        if f'Weapon.{kind}.Core' not in cues_json:
            continue
        punch_gain = PUNCH_GAIN.get(kind, 1.0)
        worst = (-99.0, '')
        for maker, (pitch, tail_pitch, g_core, g_mech, g_low, g_tail, extra) in MAKERS.items():
            core = _takes(cues_json, f'Weapon.{kind}.Core', g_core, pitch)
            mech = _takes(cues_json, f'Weapon.{kind}.Mech', k_mech * g_mech, pitch)
            low = _takes(cues_json, f'Weapon.{kind}.Low', k_low * g_low, pitch)
            # The punch is the body's: counted at the maker's Low too (the heavier reading of the code).
            punch = _takes(cues_json, f'Weapon.{kind}.Punch', punch_gain * max(1.0, g_low), pitch)
            ext = _takes(cues_json, extra, 1.0, pitch)
            for where in SURROUNDINGS:
                tail = _takes(cues_json, f'Weapon.{kind}.{where}', k_tail * g_tail, tail_pitch)
                db = float(a2db(max(_worst_peak([core, low, punch, tail, mech, ext], int(0.12 * SR)), 1e-9)))
                if db > worst[0]:
                    worst = (db, f'{maker} {where[4:]}')
        # The ladder: the plain gun in the open, take k of every layer together, averaged over the core's takes.
        core = _takes(cues_json, f'Weapon.{kind}.Core', 1.0)
        mech = _takes(cues_json, f'Weapon.{kind}.Mech', k_mech)
        low = _takes(cues_json, f'Weapon.{kind}.Low', k_low)
        tail = _takes(cues_json, f'Weapon.{kind}.TailOpen', k_tail)
        punch = _takes(cues_json, f'Weapon.{kind}.Punch', punch_gain)
        s3, l200, pks = [], [], []
        for i in range(len(core)):
            parts = [core[i], mech[i % len(mech)], low[i % len(low)], tail[i % len(tail)]]
            if punch:
                parts.append(punch[i % len(punch)])
            y = _sum(parts)
            s3.append(analysis.loudness_max(y, 3.0))
            l200.append(analysis.loudness_max(y, 0.2))
            pks.append(float(a2db(np.max(np.abs(y)))))
        rows[kind] = (float(np.mean(s3)), float(np.mean(l200)), max(pks), worst)
    if 'Rifle' not in rows:
        return
    ref_s3, ref_l200 = rows['Rifle'][0], rows['Rifle'][1]
    lines = ['  guns, own-gun sum (Core+Mech+Low+tail+Punch at cues.json x the code\'s kind and maker gains):',
             f'  {"kind":9s} {"S3 LU":>6s} {"want":>5s} {"L200 LU":>7s} {"plain pk":>8s} {"worst pk":>8s}  worst case']
    bad = 0
    for kind in sorted(rows, key=lambda k: LADDER[k]):
        s3, l200, pk, (wpk, where) = rows[kind]
        d3, d200 = s3 - ref_s3, l200 - ref_l200
        fl = []
        if wpk > GUN_PEAK_DB:
            fl.append(f'GUN PEAK {wpk:+.2f}')
        if abs(d3 - LADDER[kind]) > 0.75:
            fl.append(f'LADDER {d3 - LADDER[kind]:+.1f}')
        bad += bool(fl)
        lines.append(f'  {kind:9s} {d3:+6.1f} {LADDER[kind]:+5.1f} {d200:+7.1f} {pk:8.2f} {wpk:8.2f}  {where:18s} '
                     f'{" ".join(fl)}')
    lines.append(f'  rifle reference: S3 {ref_s3:.1f} LUFS, L200 {ref_l200:.1f} LUFS; {bad} kinds flagged')
    print('\n'.join(lines))
    with open(os.path.join(CHECK, 'guns.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    # In Blender's Python: python Art/Sounds/check.py prints the guns' sums from the files and cues.json as they are.
    os.makedirs(CHECK, exist_ok=True)
    gun_ladder(json.load(open(os.path.join(HERE, 'cues.json'), encoding='utf-8')))
