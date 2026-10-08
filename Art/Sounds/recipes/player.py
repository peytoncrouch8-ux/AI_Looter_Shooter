"""The player: boots on dirt, grass, boardwalk and stone; jumping and landing; getting hurt; the low-health heartbeat;
dying; the slide's rush and its scrape.

Footsteps are a heel strike and a lighter roll onto the toe, each made of the boot's thud and what the ground does
(dirt crunches, grass brushes, boards knock hollow and creak, stone clicks). They're the most repeated sound in the
game, so they're quiet, soft at the top and different every time.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G, voice as V
from recipes import cue
from recipes import kit


def _dirt(r, weight):
    thud = kit.noise_thump(0.08, child(r, 'thud'), 800.0, 150.0, 0.035, 0.9, 0.03)
    crunch = kit.grit_burst(0.14, child(r, 'crunch'), 5000.0, 700.0, 3500.0, 0.07)
    return layers((thud, 0.0, 0.0), (crunch, 0.002, -9.0 + 2.0 * weight))


def _grass(r, weight):
    thud = kit.noise_thump(0.07, child(r, 'thud'), 600.0, 130.0, 0.03, 0.9, 0.03)
    n = ns(0.2)
    blades = N.band(n, child(r, 'blades'), 1500.0, 7000.0) * kit.turbulence(n, child(r, 'am'), 90.0, 0.8)
    blades = blades * E.ar(0.2, 0.015, 0.16, 2.0)
    stems = kit.grit_burst(0.12, child(r, 'stems'), 1500.0, 1800.0, 5000.0, 0.06)
    return layers((thud, 0.0, -2.0), (normalize(blades), 0.0, -9.0), (stems, 0.004, -16.0))


def _wood(r, weight, creak=False):
    # A boot heel on a boardwalk plank: the heel's hard tok, the board's knock, the hollow under it.
    f0 = 250.0 + 70.0 * r.random()
    board = kit.thunk(f0, child(r, 'board'), 0.08, 0.0009, 6, 0.08)
    heel = kit.thunk(1150.0 * jitter(r, 1, 0.1), child(r, 'heel'), 0.014, 0.00018, 4, 0.06)
    hollow = kit.thump(f0 * 0.55, f0 * 0.45, 0.1, 0.06, attack=0.002)
    parts = [(board, 0.0, 0.0), (heel, 0.0, -3.0), (hollow, 0.0, -11.0)]
    if creak:
        parts.append((kit.creak(0.2, child(r, 'creak'), 45.0, 75.0, (460.0, 1100.0, 1900.0), 9.0), 0.03, -15.0))
    return layers(*parts)


def _stone(r, weight):
    click = kit.thunk(1500.0 * jitter(r, 1, 0.1), child(r, 'click'), 0.01, 0.00018, 4, 0.08)
    tick = kit.burst(0.004, child(r, 'tick'), 0.0015, lo=1200.0, hi=6000.0)
    knock = kit.thump(220.0, 130.0, 0.05, 0.03, attack=0.0008)
    sand = kit.grit_burst(0.1, child(r, 'sand'), 2500.0, 2500.0, 7000.0, 0.05)
    return layers((click, 0.0, 0.0), (tick, 0.0, -6.0), (knock, 0.0, -10.0), (sand, 0.003, -16.0))


def _footstep(r, v, ground):
    """Heel, then toe a moment later and lighter, with a little scuff between."""
    make = {'dirt': _dirt, 'grass': _grass, 'wood': _wood, 'stone': _stone}[ground]
    if ground == 'wood':
        heel = _wood(child(r, 'heel'), 1.0, creak=v in (2, 6))
    else:
        heel = make(child(r, 'heel'), 1.0)
    toe = make(child(r, 'toe'), 0.0) if ground != 'wood' else _wood(child(r, 'toe'), 0.0)
    gap = 0.03 + 0.04 * r.random()
    scuff = kit.cloth(0.06, child(r, 'scuff'), 600.0, 3000.0, 300.0, 0.3, 0.6)
    x = layers((heel, 0.0, 0.0), (toe, gap, -7.0 - 3.0 * r.random()), (scuff, gap * 0.5, -22.0))
    # The top is rounded off: steps are heard thousands of times.
    return F.filt(x, F.highshelf(5000.0, -4.0), F.peak(3000.0, -3.0, 0.8), F.hp(60.0))


@cue('Player.Footstep.Dirt', variations=8, att='Near', jitter=0.05, conc=4, level=-19.0)
def footstep_dirt(v, r):
    return _footstep(r, v, 'dirt')


@cue('Player.Footstep.Grass', variations=8, att='Near', jitter=0.05, conc=4, level=-20.0)
def footstep_grass(v, r):
    return _footstep(r, v, 'grass')


@cue('Player.Footstep.Wood', variations=8, att='Near', jitter=0.05, conc=4, level=-18.0)
def footstep_wood(v, r):
    return _footstep(r, v, 'wood')


@cue('Player.Footstep.Stone', variations=8, att='Near', jitter=0.05, conc=4, level=-19.0)
def footstep_stone(v, r):
    return _footstep(r, v, 'stone')


def _breath(r, dur, keys, db_shape=(0.01, 0.2), shift=1.05):
    """A short breath through the mouth (no voice): a whisper of the vowels."""
    x = V.whisper(dur, keys, r, shift) * E.perc(dur, db_shape[0], db_shape[1])
    return normalize(F.filt(x, F.hp(200.0), F.lp(6000.0), extend=False))


@cue('Player.Jump', variations=4, att='Near', jitter=0.04, conc=2, level=-16.0)
def jump(v, r):
    # Boots push off (a thud and a scuff of grit), cloth and gear lift, a short breath out.
    push = kit.noise_thump(0.1, child(r, 'push'), 700.0, 120.0, 0.04, 0.9, 0.04)
    grit = kit.grit_burst(0.12, child(r, 'grit'), 2500.0, 900.0, 4000.0, 0.06)
    cloth = kit.cloth(0.3, child(r, 'cloth'), 300.0, 3000.0, 150.0, 0.12, 0.6)
    gear = kit.jingle(0.2, child(r, 'gear'), 2 + int(r.integers(0, 2)), 2000.0, 6000.0, 0.04, 0.08, 0.03)
    breath = _breath(child(r, 'breath'), 0.22, [(0.0, 'h'), (0.08, 'uh'), (0.22, 'h')], (0.012, 0.18))
    x = layers((push, 0.0, -2.0), (grit, 0.0, -12.0), (cloth, 0.0, -6.0), (gear, 0.0, -17.0), (breath, 0.02, -13.0))
    return F.filt(x, F.highshelf(6000.0, -3.0), extend=False)


@cue('Player.Land', variations=4, att='Near', jitter=0.04, conc=2, level=-12.0)
def land(v, r):
    # Both boots hit together: a heavy thud with weight, dirt crunching, the gear and the coat settling.
    hit = kit.noise_thump(0.2, child(r, 'hit'), 900.0, 70.0, 0.09, 1.1, 0.06)
    weight = kit.thump(110.0, 50.0, 0.16, 0.08, attack=0.001, drive_db=3.0)
    second = kit.noise_thump(0.12, child(r, 'second'), 700.0, 100.0, 0.05, 1.0, 0.04)
    crunch = kit.grit_burst(0.25, child(r, 'crunch'), 6000.0, 800.0, 3800.0, 0.12)
    gear = kit.jingle(0.3, child(r, 'gear'), 3 + int(r.integers(0, 3)), 1800.0, 6000.0, 0.05, 0.1, 0.01)
    flap = kit.cloth(0.15, child(r, 'flap'), 250.0, 2500.0, 200.0, 0.1, 0.7)
    breath = _breath(child(r, 'breath'), 0.18, [(0.0, 'uh'), (0.18, 'h')], (0.008, 0.15))
    x = layers((hit, 0.0, 0.0), (weight, 0.0, -5.0), (second, 0.03 + 0.02 * r.random(), -7.0), (crunch, 0.002, -10.0),
               (gear, 0.01, -15.0), (flap, 0.0, -11.0), (breath, 0.03, -17.0))
    return F.filt(x, F.highshelf(6000.0, -3.0), extend=False)


@cue('Player.Hurt', variations=4, space='2D', cls='Effects', jitter=0.03, conc=2, level=-8.0)
def hurt(v, r):
    # Felt more than heard: a muffled blow in the body, a sharp sting, a caught breath, and the cold flicker of
    # something that isn't quite alive.
    blow = kit.noise_thump(0.15, child(r, 'blow'), 1200.0, 150.0, 0.07, 1.0, 0.05)
    body = kit.thump(140.0 * jitter(r, 1, 0.08), 60.0, 0.12, 0.06, attack=0.0008, drive_db=3.0)
    sting = kit.burst(0.03, child(r, 'sting'), 0.012, lo=1500.0, hi=6000.0)
    n = ns(0.26)
    f0 = O.glide([(0.0, 185.0 * jitter(r, 1, 0.06)), (0.06, 200.0), (0.26, 150.0)], n=n)
    gasp = V.voice(f0, [(0.0, 'uh'), (0.1, 'a'), (0.26, 'h')], child(r, 'gasp'), breath=0.8, jitter=0.02, shimmer=0.1,
                   tilt=-2.0, shift=1.05)
    gasp = normalize(F.filt(gasp * E.perc(0.26, 0.012, 0.22), F.hp(150.0), F.lp(5000.0), extend=False))
    chill = kit.whoosh(0.3, child(r, 'chill'), 2500.0, 700.0, 2.0, 0.3, -1.0)
    x = layers((blow, 0.0, 0.0), (body, 0.0, -4.0), (sting, 0.0, -11.0), (gasp, 0.025, -9.0), (chill, 0.0, -20.0))
    return S.widen(x, child(r, 'wide'), 0.25)


def _heartbeat(r, period, beats, strength, loop=True):
    """Lub-dub beats, period seconds apart. As a loop (beats * period seconds), tails that run past the end come round
    to the start."""
    L = ns(period * beats)
    out = np.zeros(L + ns(0.6))
    for b in range(beats):
        t = b * period
        lub = layers(kit.thump(72.0, 42.0, 0.3, 0.13, attack=0.006), (kit.noise_thump(0.22, child(r, 'l', b), 320.0, 60.0, 0.1, 0.9, 0.08), 0.0, -7.0),
                     (kit.thunk(170.0, child(r, 'lk', b), 0.06, 0.003, 4, 0.05), 0.002, -3.0))
        dub = layers(kit.thump(88.0, 52.0, 0.24, 0.1, attack=0.005), (kit.noise_thump(0.18, child(r, 'd', b), 360.0, 70.0, 0.08, 0.9, 0.06), 0.0, -8.0),
                     (kit.thunk(200.0, child(r, 'dk', b), 0.05, 0.003, 4, 0.05), 0.002, -4.0))
        a = strength * r.uniform(-0.8, 0.8)
        out = layers(out, (lub, t, a), (dub, t + 0.27, a - 4.0))
    if not loop:
        return out
    out = out[:L + ns(0.6)]
    looped = out[:L].copy()
    looped[:out.size - L] += out[L:]
    return looped


@cue('Player.LowHealth', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, loop=True, level=-15.0)
def low_health(v, r):
    # The heartbeat, heavy and close, with the blood's dull rush pulsing under it. Four beats 0.9 s apart (the HUD's
    # pulse), looping without a seam.
    period = 0.9
    beats = _heartbeat(child(r, 'beats'), period, 4, 1.0)
    L = beats.size
    # Muffled, but with enough of its knock above the deep lows to carry on small speakers.
    beats = F.filt(beats, F.lp(900.0, 0.7), F.peak(220.0, 5.0, 0.9), circular=True)
    rush = N.shaped(L, child(r, 'rush'), -6.0, 40.0, 260.0, periodic=True)
    pulse = np.abs(F.filt(np.abs(beats), F.lp1(6.0), circular=True))
    pulse /= pulse.max() + 1e-9
    rush *= 0.35 + 0.65 * pulse
    x = normalize(beats) + 0.12 * normalize(rush)
    # Width only above the lows, built circularly so the loop stays seamless.
    side = np.roll(F.filt(x, F.hp(150.0), circular=True), ns(0.004)) * 0.08
    return np.vstack([x + side, x - side])


@cue('Player.Death', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-5.0)
def death(v, r):
    # The body falls, the heart beats once more and stops, the last breath goes out, and something is pulled away
    # into the dark: a long airy rise and a ghostly tone sinking under it.
    fall = kit.noise_thump(0.3, child(r, 'fall'), 800.0, 60.0, 0.15, 1.1, 0.12)
    weight = kit.thump(95.0, 36.0, 0.4, 0.2, attack=0.002, drive_db=3.0)
    settle = kit.noise_thump(0.15, child(r, 'settle'), 600.0, 90.0, 0.07, 1.0, 0.05)
    gear = kit.jingle(0.4, child(r, 'gear'), 5, 1800.0, 6000.0, 0.05, 0.2, 0.02)
    beat = _heartbeat(child(r, 'beat'), 0.9, 1, 0.0, loop=False)
    beat = F.filt(beat, F.lp(400.0, 0.7), extend=False)
    breath = _breath(child(r, 'breath'), 0.8, [(0.0, 'a'), (0.4, 'uh'), (0.8, 'h')], (0.03, 0.7), 1.0)
    dur = 3.2
    n = ns(dur)
    f0 = O.glide([(0.0, 330.0), (0.4, 300.0), (dur, 95.0)], n=n)
    soul = V.voice(f0, [(0.0, 'u'), (1.2, 'oo'), (dur, 'u')], child(r, 'soul'), breath=0.7, jitter=0.01, shimmer=0.15,
                   tilt=-3.0, shift=1.2, vib_rate=4.5, vib_depth=0.25)
    soul = normalize(F.filt(soul, F.lp(3500.0), extend=False)) * E.swell(dur, 0.6, 1.8)
    rise = kit.whoosh(2.4, child(r, 'rise'), 250.0, 3500.0, 1.2, 0.55, -2.0, 0.8)
    sub = O.sine(38.0, n=ns(2.0)) * E.swell(2.0, 0.5, 1.5)
    body = layers((fall, 0.0, 0.0), (weight, 0.0, -3.0), (settle, 0.14, -8.0), (gear, 0.02, -15.0),
                  (normalize(beat), 0.3, -4.0), (breath, 0.2, -10.0))
    spirit = layers((normalize(soul), 0.45, -12.0), (rise, 0.5, -15.0), (normalize(sub), 0.3, -18.0))
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=3.0, lp=3000.0)
    wet = F.convolve(F.filt(spirit, F.hp(200.0), extend=False), ir)
    sp = layers(S.widen(spirit, child(r, 'sw'), 0.5), kit.set_level(wet, spirit, -6.0, 0.05))
    return layers(S.widen(body, child(r, 'bw'), 0.15), sp)


@cue('Player.Slide', variations=3, att='Near', jitter=0.04, conc=2, level=-11.0)
def slide(v, r):
    # Dropping into the slide: the body meets the ground, the coat rushes, grit sprays out ahead.
    drop = kit.noise_thump(0.12, child(r, 'drop'), 600.0, 100.0, 0.05, 1.0, 0.05)
    rush = kit.cloth(0.4, child(r, 'rush'), 250.0, 2500.0, 200.0, 0.08, 0.8)
    spray = kit.grit_burst(0.45, child(r, 'spray'), 9000.0, 800.0, 4200.0, 0.3)
    gravel = kit.grit_burst(0.35, child(r, 'gravel'), 2000.0, 300.0, 1400.0, 0.25)
    x = layers((drop, 0.0, -3.0), (rush, 0.0, -2.0), (spray, 0.01, -6.0), (gravel, 0.01, -9.0))
    return F.filt(x, F.highshelf(6000.0, -3.0), F.peak(3000.0, -2.0, 0.8), extend=False)


@cue('Player.SlideLoop', variations=1, att='Near', jitter=0.0, conc=1, loop=True, level=-16.0)
def slide_loop(v, r):
    # Sliding along the ground: grit grinding under the body, a low rumble, the coat dragging. Built in a circle so
    # it loops without a seam.
    dur = 2.0
    n = ns(dur)
    grit = G.grit(dur, child(r, 'grit'), 7000.0, 700.0, 4000.0, 2.0, 5, periodic=True)
    grit *= 0.55 + 0.45 * N.smooth_random(n, child(r, 'ga'), 9.0, periodic=True)
    stones = G.grit(dur, child(r, 'stones'), 900.0, 250.0, 1200.0, 3.0, 3, periodic=True)
    rumble = N.shaped(n, child(r, 'rumble'), -3.0, 60.0, 350.0, periodic=True)
    rumble *= 0.7 + 0.3 * N.smooth_random(n, child(r, 'ra'), 5.0, periodic=True)
    drag = N.shaped(n, child(r, 'drag'), -2.0, 350.0, 2200.0, periodic=True)
    drag *= 0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'da'), 30.0, periodic=True))
    x = normalize(grit) + 0.45 * normalize(stones) + 0.5 * normalize(rumble) + 0.3 * normalize(drag)
    return F.filt(x, F.highshelf(6000.0, -4.0), F.peak(3000.0, -3.0, 0.8), circular=True)
