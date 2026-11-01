"""Vane's Dredgers (Docs/Work/2026-10-31-Wardens/CONTRACT.md, SOUND; ADredgerRifleman, ADredgerHoseMan): company men in
black rubber coats and gas masks, with rifles and soul-hoses. All 3D at the man (the hose cues at the hose's end).

A Dredger's voice is a man's, heard through a rubber mask and a filter canister: no words, only shouts, grunts and groans
(formant voices, lib/voice.py), their highs gone, a boxy resonance from the mask's cavity, a comb from the canister's
tube, a rubber valve fluttering on every breath. His coat is rubber: it squeaks and slaps. The company rifle is nothing
like the player's guns: a big bore (the recorded rifles slowed down for a heavier, darker boom with a longer barrel's
ring) and soul-coal powder (a flare of crackling sparks after the blast, like a sparkler). The soul-hose is a pump that
sucks: wet gulps, a hungry whistle of air, a low hum in the tube, and a whisper in it, a soul being drawn along. The
tank on the hose-man's back is a pressure vessel full of that soul-oil: when it bursts it bangs, the metal rings and
throws shards, the gas blows out, and the souls it held go up in a long wail.

- Creature.Dredger.Rifle.Fire (5 takes, ~1.6 s, 'Gun'): the company rifle.
- Creature.Dredger.Rifle.Reload (4 takes, ~1.8 s): bolt back, a spent case dropping, a clip pressed in, the rounds thumbed
  down, the bolt forward.
- Creature.Dredger.Bark (6 takes, ~0.4-0.8 s): a shout through the mask: a call, a sighting, an order; never a word.
- Creature.Dredger.Hurt (5 takes, ~0.4 s): a grunt through the mask, the coat's slap.
- Creature.Dredger.Death (4 takes, ~2.2 s): a last gasp, a falling groan, the valve wheezing, the man dropping to his knees,
  then down, the rifle clattering.
- Creature.Dredger.Hose.Latch (3 takes, ~0.8 s): the nozzle clamping on, its twist-lock ratcheting, a wet suck and the
  valve's hiss opening.
- Creature.Dredger.Hose.Drain (loop, 6 s): the hungry suction, the pump's pulse, the whistle of air, a soul's whisper.
- Creature.Dredger.Hose.Snap (3 takes, ~1.2 s): the line breaking: a whip crack, the valve spitting, the hose thrashing on
  the ground, pressure hissing out, a soul's short cry.
- Creature.Dredger.Tank.Burst (3 takes, ~3.2 s, 'Gun'): the tank on his back going up.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, varispeed
from lib import noise as N, env as E, osc as O, filters as F, dist as D, dynamics as Y, modal as M, granular as G
from lib import voice as V
from recipes import cue
from recipes import kit
from recipes import guns as GN
from recipes.guns_rec import _take, _gated, _loud, _master as _gun_master
from recipes.creatures import _outside, _haunt
from recipes.world import _fold, _pnoise, _slow, _circ
from recipes.orelift import _lv, _whole
from recipes.gravemother_barrow import _tonks
from recipes.bosses import _pressure, _master, _drum
from recipes.landfall import _rel, _calm, _loopify


# --- The company rifle ------------------------------------------------------------------------------------------------------

def _rifle_dry(v, r):
    """The shot before its master: a recorded rifle's body and another's front slowed down to a big bore, the old synthesized
    crack and push of air, a barrel's long ring, soul-coal powder's flare (a sparkler's crackle) after it."""
    body = varispeed(_gated(_take('Rifle_L85_1m.wav', v, 0.3), 0.07, 0.25, lp=6500.0, hp=40.0), 0.84 - 0.01 * (v % 3))
    snap = varispeed(_gated(_take('Rifle_Tavor_1m.wav', v + 1, 0.2), 0.05, 0.15, hp=60.0), 0.93)
    crack = GN._crack(child(r, 'crack'), 0.0004 * jitter(r, 1.0, 0.12), 0.008, 1250.0 * jitter(r, 1.0, 0.08), -3.0, -5.0, 1500.0, 9000.0)
    heave = GN._heave(child(r, 'heave'), 0.008 * jitter(r, 1.0, 0.15), 120.0, 0.1)
    ring = M.strike(M.bar(jitter(r, 520.0, 0.04), child(r, 'ring'), 0.5, 0.6, 6, 0.01, 0.7), M.hammer(0.0003), 0.7)
    ring = F.filt(ring, F.lp(3000.0, 0.7), extend=False)
    n = ns(0.5)
    flare = G.grit(0.5, child(r, 'flare'), 5200.0 * np.exp(-times(n) / 0.12) + 200.0, 2500.0, 9000.0, 2.0, 4)
    flare = flare * E.perc(0.5, 0.003, 0.26) * kit.turbulence(n, child(r, 'flf'), 220.0, 0.6)
    flare = F.filt(flare, F.hp(2200.0, 0.7), extend=False)
    x = layers((_loud(body), 0.0, 0.0), (_loud(snap), 0.0, -3.0), (crack, 0.0, -8.0), (heave, 0.0, -1.0),
               (_rel(normalize(ring), body, -17.0, 0.01), 0.002, 0.0), (_rel(flare, body, -21.0, 0.01), 0.012, 0.0))
    x = Y.transient(x, 4.0, -2.0)
    x = F.filt(x, F.hp(40.0), F.peak(110.0, 3.0, 0.9), F.peak(700.0, 2.5, 1.0), F.peak(3100.0, -4.0, 0.9),
               F.highshelf(8000.0, -4.0), extend=False)
    return _loud(D.drive(normalize(x), 3.0, 'tanh'), 0.002)


@cue('Creature.Dredger.Rifle.Fire', variations=5, att='Gun', jitter=0.04, conc=6, level=-3.0)
def dredger_rifle_fire(v, r):
    # The company rifle: a heavy, dark boom (the big bore), a flat crack on its front, a barrel's long ring, a flare of
    # sparks after it, the yard's answer rolling off.
    dry = _rifle_dry(v, r)
    s1 = 0.06 + 0.03 * r.random()
    wet = GN._land(dry, child(r, 'land'), 1.3, ((s1, -3.0), (s1 + 0.1 + 0.05 * r.random(), -6.0)), 14, 0.2, 0.9,
                   -5.0, 0.8, 650.0, 0.2, -13.0, 150.0)
    return _gun_master(layers(dry, wet), 4.0, 0.001, 0.03, 2.0)


@cue('Creature.Dredger.Rifle.Reload', variations=4, att='Near', jitter=0.03, conc=2, level=-11.0)
def dredger_rifle_reload(v, r):
    # The bolt thumbed and drawn back (a slide and a clack), the spent case dropping and ringing on the ground, a clip of
    # rounds pressed in and the rounds thumbed down one by one, the bolt driven forward and locked; a rubber coat creaking
    # as he works.
    parts = []
    thumb = GN._clank(child(r, 'thumb'), (1500.0, 4200.0, 0.03), (560.0, 0.03), (200.0, 0.03), (0.0, -4.0, -8.0), 0.00008)
    back_slide = GN._slide(0.16, child(r, 'bs'), 900.0, 1500.0, 0.2, 0.4)
    back = GN._clank(child(r, 'back'), (900.0, 3400.0, 0.05), (420.0, 0.05), (160.0, 0.06), (0.0, -3.0, -7.0))
    parts += [(thumb, 0.0, -6.0), (back_slide, 0.04, -18.0), (back, 0.22 + 0.01 * v, 0.0)]
    f = jitter(r, 1700.0, 0.05)
    case = normalize(M.strike(M.shell(f, child(r, 'case'), 0.2, 6), M.hammer(0.0004), 0.35))
    knock = kit.thunk(jitter(r, 800.0, 0.1), child(r, 'cknock'), 0.04, 0.0004, 5, 0.1)
    t = 0.5 + 0.02 * v
    parts += [(case, t, -8.0), (knock, t, -6.0)]
    for j, (gap, db) in enumerate(((0.1, -9.0), (0.08, -14.0))):
        t += gap * jitter(r, 1, 0.2)
        parts.append((kit.brass_tick(child(r, 'cb', j), f * 0.6, 0.03, 4, 0.0002, -10.0, -8.0, 0.1), t, db))
    clip = np.zeros(1)
    clip = layers(GN._rounds(child(r, 'clip'), 5, 0.1), (kit.metal_click(child(r, 'clipclick'), 1800.0, 5500.0, 0.03, 0.00008, 5, -8.0), 0.12, -4.0))
    parts.append((clip, 0.78 + 0.02 * v, -5.0))
    for i in range(5):
        rr = child(r, 'round', i)
        parts.append((kit.thunk(jitter(rr, 1100.0, 0.08) * (0.97 ** i), child(rr, 't'), 0.025, 0.0004, 4, 0.1), 1.0 + 0.085 * i, -6.0 - 0.8 * i))
    fwd_slide = GN._slide(0.12, child(r, 'fs'), 1300.0, 800.0, 0.2, 0.4)
    lock = GN._clank(child(r, 'lock'), (800.0, 3000.0, 0.05), (380.0, 0.06), (150.0, 0.06), (0.0, -1.0, -4.0), 0.0001, drive_db=5.0)
    parts += [(fwd_slide, 1.46, -18.0), (lock, 1.57 + 0.01 * v, -1.0)]
    cloth = kit.cloth(1.7, child(r, 'cloth'), 250.0, 2200.0, 90.0, 0.4, 0.5)
    squeak = _rubber(child(r, 'squeak'), 0.4)
    parts += [(cloth, 0.0, -29.0), (squeak, 0.7, -25.0)]
    x = layers(*parts)
    return F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)


# --- Rubber and masks ---------------------------------------------------------------------------------------------------------

def _rubber(r, dur):
    """A rubber coat creaking: a high squeal of stick-slip rubber on rubber over a soft slap."""
    sq = kit.creak(dur, child(r, 'sq'), 70.0, 130.0, (1500.0, 2700.0, 4100.0), 10.0, 0.4)
    return F.filt(sq, F.peak(3300.0, -5.0, 0.9), F.lp(5200.0, 0.7), extend=False)


def _mask(x, r, wet=0.5):
    """A voice heard through a rubber gas mask and its canister: the highs gone, the mask cavity's boxy resonance, a comb
    from the filter tube, a small leak of rubber slap."""
    y = F.filt(x, F.hpn(140.0, 2), F.lpn(2000.0, 4), F.peak(300.0, 3.0, 1.0), F.peak(560.0, 4.0, 1.2),
               F.peak(950.0, 3.0, 1.5), extend=False)
    tube = F.filt(y, F.comb(0.0037 * jitter(r, 1.0, 0.04), 0.45, 2400.0), extend=False)
    return layers((normalize(y), 0.0, 0.0), (normalize(tube), 0.0, float(20.0 * np.log10(max(wet, 1e-3)))))


def _valve(r, db=-14.0, dur=0.07):
    """The exhale valve's rubber flap: a burst of breath fluttering."""
    n = ns(dur)
    x = N.shaped(n, r, -1.0) * (0.4 + 0.6 * np.abs(N.smooth_random(n, child(r, 'fl'), 120.0)))
    x = F.filt(x, F.hp(500.0, 0.7), F.lp(3000.0, 0.7), extend=False) * E.perc(dur, 0.004, dur * 0.6)
    return normalize(x, 0.0) * float(db2a(db))


def _shout(r, dur, f0, f1, v1, v2, tilt=2.0, breath=0.22, rough=0.12, shift=0.9, attack=0.025):
    """One shouted syllable: a man's voice on a pitch glide and a vowel glide, pressed and bright, shaped like a shout."""
    n = ns(dur)
    f = np.geomspace(f0, f1, n)
    keys = [(0.0, v1), (dur * 0.5, v1), (dur, v2)]
    x = V.voice(f, keys, r, breath=breath, jitter=0.012, shimmer=0.1, tilt=tilt, shift=shift, rough=rough)
    # A shout is accented: it bursts out and falls away, not held level.
    env = E.bp([(0.0, 0.0), (attack, 1.0), (dur * 0.35, 0.82), (dur * 0.8, 0.45), (dur, 0.0)], n=n, curve='cos')
    return normalize(x * env, 0.0)


def _consonant(r, kind, db=-12.0):
    """A shout's first consonant: a hard 't' or 'k' burst, or a 'th' hiss."""
    if kind == 'th':
        x = V.hiss(ns(0.05), r, (2400.0, 3600.0, 5200.0)) * E.ar(0.05, 0.003, 0.03, 2.0)
    else:
        x = N.band(ns(0.012), r, 1200.0 if kind == 'k' else 2200.0, 4800.0) * E.perc(0.012, 0.0004, 0.006)
    return normalize(x, 0.0) * float(db2a(db))


# (syllables: (dur, f0, f1, vowel1, vowel2, onset)) per take: a hail, a sighting, an order...
BARKS = [
    [(0.36, 150.0, 190.0, 'e', 'ae', 'k')],
    [(0.22, 128.0, 148.0, 'o', 'o', 'k'), (0.30, 168.0, 128.0, 'a', 'uh', 't')],
    [(0.30, 175.0, 215.0, 'e', 'e', 'th')],
    [(0.2, 118.0, 138.0, 'u', 'u', 'k'), (0.32, 142.0, 170.0, 'uh', 'a', 't')],
    [(0.26, 145.0, 190.0, 'ae', 'ae', 'k'), (0.2, 185.0, 150.0, 'e', 'uh', 't')],
    [(0.24, 138.0, 150.0, 'uh', 'uh', 'k'), (0.34, 172.0, 118.0, 'er', 'er', 'k')],
]


@cue('Creature.Dredger.Bark', variations=6, att='Creature', jitter=0.05, conc=3, level=-9.0)
def dredger_bark(v, r):
    # A Dredger shouting through his gas mask: a hail, a sighting, an order (never a word): a man's voice in a shout (a
    # hard consonant, a pressed vowel gliding up or down), heard through rubber and a canister, the valve flapping as he
    # breathes out.
    out = np.zeros(1)
    t = 0.0
    for i, (dur, f0, f1, v1, v2, on) in enumerate(BARKS[v]):
        rr = child(r, 'syl', i)
        syl = _shout(rr, dur * jitter(rr, 1.0, 0.06), f0 * jitter(rr, 1.0, 0.05), f1 * jitter(rr, 1.0, 0.05), v1, v2)
        cons = _consonant(child(rr, 'cons'), on)
        out = layers(out, (cons, t, -6.0), (syl, t + 0.012, 0.0))
        t += dur + 0.07
    x = _mask(out, r, 0.55)
    x = layers(x, (_valve(child(r, 'valve'), -13.0), t - 0.03, 0.0))
    return _outside(x, r, -19.0, 0.07, 0.18, 0.5)


@cue('Creature.Dredger.Hurt', variations=5, att='Creature', jitter=0.05, conc=3, level=-9.0)
def dredger_hurt(v, r):
    # Hit: a grunt punched out of him through the mask (a short vowel falling in pitch, rough), a breath over it, the
    # rubber coat's slap and a squeak of it.
    dur = 0.24 + 0.04 * (v % 3)
    f0 = jitter(r, 150.0, 0.12)
    n = ns(dur)
    f = np.geomspace(f0, f0 * 0.66, n)
    keys = [(0.0, 'uh'), (dur * 0.6, 'uh'), (dur, 'o')]
    voice = V.voice(f, keys, child(r, 'voice'), breath=0.35, jitter=0.015, shimmer=0.12, tilt=1.0, shift=0.9, rough=0.3)
    voice = normalize(voice * E.ar(dur, 0.012, dur * 0.55, 1.5), 0.0)
    gasp = V.whisper(0.09, [(0.0, 'a'), (0.09, 'uh')], child(r, 'gasp'), shift=0.95) * E.ar(0.09, 0.01, 0.05, 1.5)
    slap = kit.noise_thump(0.12, child(r, 'slap'), 500.0, 90.0, 0.05, 1.0, 0.05, 0.001)
    cloth = kit.cloth(0.16, child(r, 'cloth'), 250.0, 1900.0, 120.0, 0.3, 0.6)
    x = layers((normalize(gasp), 0.0, -9.0), (voice, 0.02, 0.0))
    x = layers(_mask(x, r, 0.5), (slap, 0.0, -9.0), (cloth, 0.02, -13.0), (_valve(child(r, 'v'), -15.0), dur + 0.03, 0.0))
    return _outside(x, r, -19.0, 0.07, 0.18, 0.5)


@cue('Creature.Dredger.Death', variations=4, att='Creature', jitter=0.04, conc=3, level=-8.0)
def dredger_death(v, r):
    # Down: a last gasp drawn through the mask, a long groan falling in pitch (rough, breathy), the valve wheezing and
    # chattering, the knees hitting the ground, then the body, the rubber coat slapping, the rifle clattering, one more
    # breath leaking out of the canister.
    gasp = V.whisper(0.28, [(0.0, 'a'), (0.28, 'o')], child(r, 'gasp'), shift=0.95)
    gasp = normalize(gasp * E.ar(0.28, 0.12, 0.1, 1.5), 0.0)
    gd = 0.9 + 0.1 * v
    gn = ns(gd)
    f = np.geomspace(jitter(r, 140.0, 0.1), 62.0, gn)
    keys = [(0.0, 'o'), (gd * 0.5, 'o'), (gd, 'u')]
    groan = V.voice(f, keys, child(r, 'groan'), breath=0.5, jitter=0.02, shimmer=0.15, tilt=-1.0, shift=0.9, rough=0.35)
    groan = normalize(groan * E.bp([(0.0, 0.0), (0.08, 1.0), (gd * 0.6, 0.8), (gd, 0.0)], n=gn, curve='cos'), 0.0)
    wn = ns(0.55)
    wheeze = F.filt(N.shaped(wn, child(r, 'wh'), -1.0), F.hp(700.0, 0.7), F.lp(2600.0, 0.7), extend=False) \
        * (0.35 + 0.65 * np.abs(np.sin(np.pi * 20.0 * times(wn)))) * E.perc(0.55, 0.01, 0.35)
    voicebed = layers((gasp, 0.0, -2.0), (groan, 0.26, 0.0), (normalize(wheeze), 0.4, -9.0))
    x = _mask(voicebed, r, 0.5)
    kneel = kit.noise_thump(0.2, child(r, 'kn'), 420.0, 70.0, 0.1, 1.0, 0.08, 0.002)
    fall = kit.noise_thump(0.4, child(r, 'fall'), 420.0, 45.0, 0.2, 1.0, 0.16, 0.002)
    slap = kit.cloth(0.2, child(r, 'slap'), 200.0, 1800.0, 100.0, 0.1, 0.7)
    rifle = GN._clank(child(r, 'rifle'), (900.0, 3400.0, 0.06), (400.0, 0.06), (150.0, 0.06), (0.0, -2.0, -6.0))
    roll = layers(*[(kit.brass_tick(child(r, 'rl', i), 1400.0 * (0.9 ** i), 0.03, 3, 0.0002, -10.0, -8.0, 0.1), 0.1 * i + 0.1, -9.0 - 3.0 * i) for i in range(3)])
    tk = 0.55 + 0.03 * v
    shift = kit.noise_thump(0.06, child(r, 'shift'), 600.0, 120.0, 0.03, 1.0, 0.03, 0.001)
    x = layers((x, 0.0, 0.0), (shift, 0.0, -9.0), (kneel, tk, -3.0), (slap, tk, -16.0), (fall, tk + 0.4, 3.0), (kit.cloth(0.25, child(r, 'slap2'), 200.0, 1800.0, 100.0, 0.1, 0.7), tk + 0.4, -10.0),
               (rifle, tk + 0.5, -5.0), (roll, tk + 0.6, -3.0), (_valve(child(r, 'last'), -13.0, 0.25), tk + 0.75, 0.0))
    return _outside(x, r, -17.0, 0.07, 0.18, 0.7)


# --- The soul-hose ----------------------------------------------------------------------------------------------------------

@cue('Creature.Dredger.Hose.Latch', variations=3, att='Creature', jitter=0.04, conc=2, level=-6.0)
def hose_latch(v, r):
    # The nozzle thrown onto the player: a heavy clamp slamming shut, the twist-lock ratcheting round (three quick clicks), a
    # wet suck as the seal takes, the valve's hiss opening, and under it a soul stirring (a whisper).
    clamp = GN._clank(child(r, 'clamp'), (700.0, 2800.0, 0.06), (300.0 * jitter(r, 1, 0.05), 0.07), (120.0, 0.07), (0.0, -1.0, -4.0), 0.0001, drive_db=5.0)
    parts = [(clamp, 0.0, 0.0)]
    for j in range(3):
        click = kit.clack(child(r, 'twist', j), 680.0 * jitter(r, 1, 0.04), 1300.0, 3800.0, 0.02, 0.03, -3.0, -9.0)
        parts.append((click, 0.07 + 0.06 * j, -6.0 - 1.0 * j))
    suck = kit.squelch(0.14, child(r, 'suck'), 380.0, 1900.0)
    inhale = kit.whoosh(0.3, child(r, 'inhale'), 350.0, 2600.0, 0.7, 0.8, -3.0)
    hiss = kit.burst(0.5, child(r, 'hiss'), 0.35, lo=1500.0, hi=6500.0, color=-1.0, attack=0.08)
    wh = V.whisper(0.4, [(0.0, 'u'), (0.4, 'a')], child(r, 'wh'), shift=1.0) * E.ar(0.4, 0.12, 0.2, 1.5)
    wh = F.filt(wh, F.hp(260.0, 0.7), F.lp(2400.0, 0.7), extend=False)
    parts += [(_rel(suck, clamp, -8.0, 0.02), 0.2, 0.0), (_rel(inhale, clamp, -12.0, 0.05), 0.22, 0.0),
              (_rel(hiss, clamp, -17.0, 0.05), 0.28, 0.0), (_rel(wh, clamp, -16.0, 0.05), 0.3, 0.0)]
    return _master(_outside(_calm(layers(*parts)), r, -18.0, 0.07, 0.17, 0.5), 3.0, 2.0, 0.0015, 0.05)


@cue('Creature.Dredger.Hose.Drain', variations=1, att='Creature', cls='Effects', jitter=0.0, conc=2, loop=True,
     level=-16.0, align=False)
def hose_drain(v, r):
    # A hungry suction: a pump's pulse four times a second under a low hum in the tube, wet gulps and bubbles each half
    # second, a thin whistle of air drawn along it that rises and falls, and a soul's whisper going by (a breathy voice
    # sliding upward), a faint glint of cold ringing over all of it. Six seconds, whole cycles.
    dur = 6.0
    L = ns(dur)
    t = times(L)
    pulse = (0.5 + 0.5 * np.cos(2 * np.pi * _whole(4.0, dur) * t)) ** 2
    pump = _circ(_pnoise(L, child(r, 'pump'), 60.0, 420.0, -4.0), F.peak(140.0, 4.0, 0.9)) * (0.3 + 0.7 * pulse)
    hum = np.zeros(L)
    for k, a in enumerate((1.0, 0.5, 0.3, 0.18), 1):
        hum += a * np.sin(2 * np.pi * _whole(118.0 * k, dur) * t + 0.8 * k)
    hum += 0.6 * np.sin(2 * np.pi * _whole(119.5, dur) * t + 2.0)
    events = []
    rg = child(r, 'gulp')
    for k in range(12):
        rk = child(rg, 'g', k)
        at = k * 0.5 + rk.uniform(-0.04, 0.04) + 0.15
        sq = kit.squelch(rk.uniform(0.14, 0.24), rk, rk.uniform(400.0, 700.0), rk.uniform(1200.0, 2000.0))
        events.append((sq * rk.uniform(0.6, 1.0), at))
        events.append((kit.bubble(rk.uniform(500.0, 1200.0), rk, 0.15, rk.uniform(0.3, 0.7)), at + rk.uniform(0.05, 0.2)))
    gulps = _fold(L, events)
    fc = 1100.0 + 450.0 * np.sin(2 * np.pi * _whole(0.5, dur) * t) + 200.0 * np.sin(2 * np.pi * _whole(1.5, dur) * t + 1.0)
    whistle = F.sweep(_pnoise(L, child(r, 'wh'), 500.0, 4500.0, 0.0), 'bp', fc, 7.0) * (0.35 + 0.65 * pulse)
    souls = []
    for k, at in enumerate((0.4, 2.3, 4.2)):
        rk = child(r, 'soul', k)
        sd = 1.5
        f = np.geomspace(260.0 * jitter(rk, 1.0, 0.08), 470.0 * jitter(rk, 1.0, 0.08), ns(sd))
        voice = V.voice(f, [(0.0, 'oo'), (sd * 0.5, 'o'), (sd, 'a')], rk, breath=0.75, jitter=0.02, shimmer=0.15, tilt=-3.0, shift=1.0, vib_rate=5.0, vib_depth=0.2)
        voice = F.filt(voice * E.ar(sd, 0.5, 0.7, 2.0), F.hp(240.0, 0.7), F.lp(2600.0, 0.7), extend=False)
        souls.append((voice, at))
    whisper = _fold(L, souls)
    glint = (np.sin(2 * np.pi * _whole(1318.0, dur) * t) + 0.7 * np.sin(2 * np.pi * _whole(1760.5, dur) * t + 1.0)) * _slow(L, child(r, 'gl'), 0.5, 0.0) ** 2
    x = _lv(pump, 0.0) + _lv(hum, -7.0) + _lv(gulps, -2.0) + _lv(whistle, -9.0) + _lv(whisper, -12.0) + _lv(glint, -30.0)
    return _circ(x, F.hp(45.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


@cue('Creature.Dredger.Hose.Snap', variations=3, att='Creature', jitter=0.04, conc=2, level=-6.0)
def hose_snap(v, r):
    # The line breaks: a whip crack as it parts, the nozzle's valve spitting, the loose hose thrashing and slapping on the
    # ground (two floppy slaps, rubber on dirt), pressure hissing out, a soul's short cry rising and gone.
    crack = layers(kit.nwave(0.0004), (kit.burst(0.02, child(r, 'crk'), 0.008, lo=700.0, hi=6000.0), 0.0, -3.0),
                   (kit.noise_thump(0.2, child(r, 'th'), 500.0, 80.0, 0.1, 1.0, 0.08, 0.001), 0.0, -3.0))
    spit = kit.squelch(0.08, child(r, 'spit'), 1500.0, 500.0)
    slaps = layers(*[(kit.noise_thump(0.14, child(r, 'slap', i), 380.0 - 60.0 * i, 70.0, 0.06, 1.0, 0.05, 0.002), 0.14 + 0.15 * i, -2.0 * i) for i in range(3)])
    slapcloth = layers(*[(kit.cloth(0.18, child(r, 'sc', i), 200.0, 2000.0, 100.0, 0.1, 0.7), 0.14 + 0.15 * i, -10.0) for i in range(3)])
    hiss = kit.burst(0.9, child(r, 'hiss'), 0.5, lo=1200.0, hi=6500.0, color=-1.0, attack=0.01)
    sd = 0.45
    f = np.geomspace(300.0 * jitter(r, 1.0, 0.06), 620.0, ns(sd))
    cry = V.voice(f, [(0.0, 'oo'), (sd, 'a')], child(r, 'cry'), breath=0.55, jitter=0.02, shimmer=0.12, tilt=-1.0, shift=1.0, vib_rate=6.0, vib_depth=0.25)
    cry = F.filt(cry * E.ar(sd, 0.1, 0.3, 1.5), F.hp(240.0, 0.7), F.lp(3200.0, 0.7), extend=False)
    x = layers((crack, 0.0, 0.0), (_rel(spit, crack, -8.0, 0.02), 0.01, 0.0), (_rel(slaps, crack, -6.0, 0.05), 0.0, 0.0),
               (_rel(slapcloth, crack, -14.0, 0.05), 0.0, 0.0), (_rel(hiss, crack, -15.0, 0.1), 0.02, 0.0),
               (_rel(normalize(cry), crack, -14.0, 0.1), 0.1, 0.0))
    return _master(_haunt(_outside(_calm(x), r, -18.0, 0.07, 0.17, 0.5), r, -16.0, 1.0, 3200.0), 3.0, 3.0, 0.0015, 0.05)


# --- The tank --------------------------------------------------------------------------------------------------------------

@cue('Creature.Dredger.Tank.Burst', variations=3, att='Gun', jitter=0.03, conc=2, level=-1.0)
def tank_burst(v, r):
    # The tank on his back bursts: a flat bang and a heave of air, the steel ringing, shards flying off whining and
    # pinging away, the gas blowing out in a roar of vapour, the cold fizz of soul-oil, and the souls it held rising in a
    # wail that thins into the dark.
    fr = GN._friedlander(child(r, 'fr'), 0.003 * jitter(r, 1.0, 0.1), 1.0, 0.04)
    crack = GN._crack(child(r, 'crack'), 0.0004, 0.01, 1100.0 * jitter(r, 1.0, 0.08), -3.0, -4.0, 1400.0, 9000.0)
    push = _pressure(child(r, 'push'), 0.014, 0.4, 200.0)
    heave = GN._heave(child(r, 'heave'), 0.012, 110.0, 0.2)
    plate = M.strike(M.plate(jitter(r, 230.0, 0.06), child(r, 'plate'), 24, 0.9, 0.5, tilt=0.8), M.hammer(0.0003), 1.2)
    plate = F.filt(normalize(plate), F.lp(2600.0, 0.7), extend=False)
    gas = GN._gas(child(r, 'gas'), 1.8, 0.03, 0.55, 9000.0, 0.25, 800.0, 7000.0, -1.0, 8.0, -6.0, 1.0, 2500.0)
    shards = np.zeros(1)
    for i in range(9):
        rr = child(r, 'shard', i)
        at = rr.uniform(0.02, 0.9) ** 1.2
        f = rr.uniform(2200.0, 6500.0) * (1.0 - 0.35 * at)
        m = M.parts(f, f * 2.4, rr, 4, 0.12, 0.05, 0.3)
        ping = normalize(M.strike(m, M.hammer(0.00008), 0.25))
        shards = layers(shards, (F.filt(ping, F.peak(3300.0, -6.0, 1.0), extend=False), at, -2.0 - 0.8 * i - rr.uniform(0.0, 3.0)))
    n = ns(1.7)
    fizz = G.grit(1.7, child(r, 'fizz'), 5200.0 * np.exp(-times(n) / 0.45) + 300.0, 3000.0, 9000.0, 2.0, 4)
    fizz = fizz * E.perc(1.7, 0.02, 1.0) * kit.turbulence(n, child(r, 'ff'), 150.0, 0.7)
    vapour = kit.whoosh(1.5, child(r, 'vap'), 600.0, 220.0, 0.6, 0.15, -2.0)
    # The souls: three voices rising together, one a fifth over, thinning as they rise.
    wd = 1.9
    wail = np.zeros(1)
    for i, (f0, f1, vow) in enumerate(((290.0, 520.0, 'oo'), (350.0, 640.0, 'o'), (435.0, 780.0, 'a'))):
        rr = child(r, 'wail', i)
        f = np.geomspace(f0 * jitter(rr, 1.0, 0.03), f1 * jitter(rr, 1.0, 0.03), ns(wd))
        voice = V.voice(f, [(0.0, vow), (wd * 0.6, 'a'), (wd, 'o')], rr, breath=0.5, jitter=0.02, shimmer=0.15, tilt=-1.0, shift=1.0, vib_rate=5.5, vib_depth=0.3)
        wail = layers(wail, (voice * E.bp([(0.0, 0.0), (0.3, 1.0), (wd * 0.5, 0.8), (wd, 0.0)], n=ns(wd), curve='cos'), 0.0, -2.0 * i))
    wail = F.filt(wail, F.hp(240.0, 0.7), F.lp(3500.0, 0.7), extend=False)
    bang = layers((fr, 0.0, 4.0), (crack, 0.0, 0.0), (_rel(push, crack, -2.0, 0.02), 0.0, 0.0), (_rel(heave, crack, -3.0, 0.02), 0.0, 0.0))
    boom = _drum(jitter(r, 40.0, 0.05), 1.2, child(r, 'boom'), 0.8, 0.1, 0.003)
    x = layers((bang, 0.0, 0.0), (boom, 0.0, -6.0), (_rel(normalize(plate), crack, -7.0, 0.02), 0.002, 0.0),
               (_rel(gas, bang, -9.0, 0.05), 0.01, 0.0), (_rel(shards, bang, -12.0, 0.02), 0.02, 0.0),
               (_rel(fizz, bang, -21.0, 0.1), 0.05, 0.0), (_rel(vapour, bang, -17.0, 0.1), 0.05, 0.0),
               (_rel(normalize(wail), bang, -16.0, 0.1), 0.18, 0.0),
               (_rel(_tonks(1.4, child(r, 'debris'), 14, 0.15, 1.2, 300.0, 1600.0, 0.86, 0.05), bang, -18.0, 0.02), 0.3, 0.0))
    x = _haunt(_outside(_calm(x), r, -13.0, 0.1, 0.26, 1.1), r, -16.0, 2.0, 3200.0)
    return _master(x, 4.0, 3.0, 0.0015, 0.06)
