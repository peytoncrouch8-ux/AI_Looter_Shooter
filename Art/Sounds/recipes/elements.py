"""The story elements' statuses on a creature (Phase 12.3, 2026-10-21 night; UElementStatusComponent): each element has an
Apply (a hit sets the status), a Tick (each damage tick; Soul-light's is the delayed smite bursting) and a Kill (the status
finishes the creature). All synthesized (only guns and impacts may use recordings), 3D at the creature.

- Ember (burn): fire. Apply is a whoomph (the flame catching: a soft low thud of air, a flame's rush rising and falling, the
  first crackle); Tick a few crisp pops over a fluttering flame; Kill a big flare roaring up and crackling out.
- Soul-light (smite, vs the undead): warm holy light, never the Unpaid's cold soul-light (Creature.Unpaid.Dissolve). Apply
  is a struck bell's clear ping with a shimmer and a breath of choir; Tick the smite bursting (a soft whump of light, a
  bright ring a major third wide, a short choir stab); Kill a choir's major chord swelling over a big bell.
- Grave-salt (slow, vs spirits): the grave-salt grenade's salt (recipes/throwables.py, same feel): Apply a crystalline
  crunch with a frosty hiss; Tick the salt hissing as it eats in, a few crystals ticking; Kill the crust shattering, its
  crystals raining down, a freezing crackle and a long hiss.
- Thunder (jolt, arcs to the next living thing): electricity. Apply is the jolt striking (a bright snap, a dense buzz of
  fast sparks, a short hum chopped by the arc, a zap falling away); Tick the burst leaping on (a snap, a crackling whip
  falling fast in pitch, a brief buzz); Kill a big crack and a low thud, then a long sizzling buzz sinking as it dies.
- Rot (spreads from the dying): wet and sick. Apply is the rot taking hold (a soft wet pop, gurgling, low bubbles, a wet
  fizz, a faint sick drone of two tones a semitone apart); Tick a few wet blisters popping and a short fizz; Kill the body
  collapsing (a wet burst and thud, a gush of bubbling, the rot's cloud hissing out over the drone).

Heard over and over in a fight: every cue keeps its 2-5 kHz band low (the bites go to 6-10 kHz), crackles keep a hiss bed
under them as long as they last (a lone late grain is a CLICK).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from lib import granular as G, voice as V
from recipes import cue
from recipes import kit


def _loud(x, win=0.01):
    p = kit.env_peak(x, win)
    return x / p if p > 0 else x


def _air(x, r, wet_db=-16.0, t60=0.9, lp=3500.0):
    """The open air round the creature: a short dark outdoor wash."""
    ir = R.open_air(child(r, 'air'), t60=t60, lp=lp, predelay=0.012, density=0.9)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, wet_db, 0.05))


def _calm(x):
    """The touchy band dipped and the rumble cut, the last step of every element cue."""
    return F.filt(x, F.peak(3300.0, -5.0, 0.8), F.hp(70.0), extend=False)


# --- Ember ---------------------------------------------------------------------------------------------------------------

def _flame(dur, r, f0, fpeak, f1, peak_at, q=1.2):
    """A flame's rush: pink noise through a band-pass sliding f0 -> fpeak -> f1, its level licking up and down fast."""
    n = ns(dur)
    fc = O.glide([(0.0, f0), (dur * peak_at, fpeak), (dur, f1)], n=n)
    x = F.sweep(N.pink(n, child(r, 'n')) * kit.turbulence(n, child(r, 't'), 40.0, 0.6), 'bp', fc, q)
    lick = 0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'lick'), 18.0)) ** 0.7
    return normalize(x * lick * E.end_fade(E.swell(dur, dur * peak_at, dur * (1.0 - peak_at) * 1.2, 1.5), 0.01), 0.0)


def _crackle(dur, r, rate0, tau, floor, lo=700.0, hi=2200.0):
    """Burning: sharp pops (sap bursting), each a wide click with a short knock in the low mids and a tick up high, dense
    at first and thinning to a floor that never gets sparse (lone grains read as clicks)."""
    n = ns(dur)
    t = times(n)
    body = G.grit(dur, child(r, 'body'), rate0 * np.exp(-t / tau) + floor, lo, hi, 2.5, 4)
    top = G.grit(dur, child(r, 'top'), 0.6 * (rate0 * np.exp(-t / tau) + floor), 6000.0, 11000.0, 2.0, 3)
    bed = F.filt(N.pink(n, child(r, 'bed')), F.bp(900.0, 0.7), extend=False) * kit.turbulence(n, child(r, 'bt'), 30.0, 0.5)
    x = layers((normalize(body), 0.0, 0.0), (normalize(top), 0.0, -8.0), (normalize(bed), 0.0, -16.0))
    return x * E.end_fade(E.ar(dur, 0.004, dur * 0.95, 1.6), 0.02)


def _pop(r, f0):
    """One sap pocket bursting: a hard click with a short woody knock."""
    knock = kit.thunk(f0, child(r, 'k'), 0.02, 0.0002, 4, 0.12)
    tick = kit.burst(0.004, child(r, 'tick'), 0.0012, lo=6000.0, hi=12000.0)
    return layers((_loud(knock, 0.003), 0.0, 0.0), (_loud(tick, 0.002), 0.0, -6.0))


@cue('Element.Ember.Apply', variations=4, att='Creature', jitter=0.05, conc=4, level=-11.0)
def ember_apply(v, r):
    # The flame catching: a pop as the ember strikes, a soft low thud of air (whoomph) with the flame's rush rising and
    # falling over it, and the first crackle starting.
    dur = 0.65
    pop = _pop(child(r, 'pop'), 900.0 * jitter(r, 1.0, 0.1))
    whoomph = kit.noise_thump(0.3, child(r, 'whoomph'), 650.0, 90.0, 0.14, 1.1, 0.08, 0.004)
    rush = _flame(dur, child(r, 'rush'), 250.0, 1200.0 * jitter(r, 1.0, 0.1), 500.0, 0.25)
    crack = _crackle(0.55, child(r, 'crackle'), 900.0, 0.2, 260.0)
    x = layers((pop, 0.0, -4.0), (_loud(whoomph), 0.0, 0.0), (rush, 0.01, -5.0), (crack, 0.06, -12.0))
    return _air(_calm(D.drive(normalize(x), 3.0, 'tanh')), r, -18.0)


@cue('Element.Ember.Tick', variations=5, att='Creature', jitter=0.06, conc=6, level=-16.0)
def ember_tick(v, r):
    # Burning on: two or three crisp pops over a short flutter of flame.
    dur = 0.38
    out = layers((_pop(child(r, 'p0'), 800.0 * jitter(r, 1.0, 0.15)), 0.0, 0.0))
    for i in range(1 + v % 3):
        out = layers(out, (_pop(child(r, 'p', i), 700.0 * jitter(r, 1.0, 0.2)), 0.05 + 0.25 * r.random(),
                           -3.0 - 3.0 * r.random()))
    flame = _flame(dur, child(r, 'flame'), 300.0, 900.0, 450.0, 0.3, 1.0)
    crack = _crackle(dur, child(r, 'crackle'), 600.0, 0.15, 220.0)
    x = layers((_loud(out, 0.003), 0.0, 0.0), (_loud(flame), 0.0, -9.0), (_loud(crack, 0.02), 0.0, -13.0))
    return _air(_calm(x), r, -20.0, 0.6)


@cue('Element.Ember.Kill', variations=3, att='Creature', jitter=0.04, conc=3, level=-8.0)
def ember_kill(v, r):
    # The body going up: a big whoomph (a deeper thud of air), a roar of flame swelling and dying, a dense crackle that
    # thins as it burns out, a last few pops.
    dur = 1.2
    pop = _pop(child(r, 'pop'), 700.0)
    whoomph = kit.noise_thump(0.5, child(r, 'whoomph'), 500.0, 60.0, 0.25, 1.1, 0.12, 0.004)
    roar = _flame(dur, child(r, 'roar'), 200.0, 900.0 * jitter(r, 1.0, 0.08), 300.0, 0.18, 0.9)
    roar = D.drive(roar, 5.0, 'tanh')
    crack = _crackle(dur, child(r, 'crackle'), 2400.0, 0.35, 300.0)
    tail = np.zeros(1)
    for i in range(4):
        tail = layers(tail, (_pop(child(r, 'tail', i), 600.0 * jitter(r, 1.0, 0.2)), 0.3 + 0.6 * r.random(),
                             -4.0 - 4.0 * r.random()))
    x = layers((pop, 0.0, -4.0), (_loud(whoomph), 0.0, 0.0), (_loud(roar), 0.02, -3.0), (_loud(crack, 0.02), 0.04, -9.0),
               (_loud(tail, 0.003), 0.0, -14.0))
    return _air(_calm(D.drive(normalize(x), 2.0, 'tanh')), r, -16.0, 1.2)


# --- Soul-light ----------------------------------------------------------------------------------------------------------
# One family in A major (A5's bell, C#6 its third, E6 the fifth), struck soft and kept warm; the Unpaid's own soul-light is
# cold and minor, so the two never blur.

def _bell(f0, r, t60, bright=0.6, contact=0.00012):
    """A small clear bell: a chime's note with a soft sine body under it."""
    c = kit.chime(f0, child(r, 'chime'), t60, bright, contact, 0.5, -30.0)
    body = O.sine(f0, n=ns(t60)) * E.perc(t60, 0.003, t60 * 0.7)
    return normalize(layers(c, (normalize(body), 0.0, -8.0)), 0.0)


def _voices(r, notes, dur, vowel='a', count=2, breath=0.5):
    """A few breathy voices on each note (a small choir, no words), swelling in and out: the light's warmth."""
    out = np.zeros(1)
    for i, f in enumerate(notes):
        for k in range(count):
            rr = child(r, 'v', i, k)
            n = ns(dur)
            f0 = O.glide([(0.0, f * 2.0 ** (rr.uniform(-8, 8) / 1200.0)), (dur, f * 2.0 ** (rr.uniform(-8, 8) / 1200.0))],
                         n=n)
            x = V.voice(f0, [(0.0, vowel), (dur, 'o' if vowel == 'a' else vowel)], rr, breath=breath, jitter=0.008,
                        shimmer=0.08, shift=1.15, vib_rate=5.0 * rr.uniform(0.9, 1.1), vib_depth=0.25)
            out = layers(out, (normalize(x), rr.uniform(0.0, 0.03), -2.0 * k))
    d = out.size / 48000.0
    return normalize(out * E.end_fade(E.swell(d, 0.3 * d, 0.8 * d, 1.6), 0.02), 0.0)


def _shimmer(r, count, start, spread, lo=5200.0, hi=8500.0):
    """Motes of light: tiny high glass taps scattered after start (above the touchy band)."""
    out = np.zeros(1)
    for i in range(count):
        f = float(np.exp(r.uniform(np.log(lo), np.log(hi))))
        c = kit.chime(f, child(r, 'tap', i), 0.25, 0.3, 0.0002, 0.4, -40.0)
        out = layers(out, (c, start + spread * r.random(), -1.5 * i - r.uniform(0.0, 3.0)))
    return normalize(out)


@cue('Element.SoulLight.Apply', variations=4, att='Creature', jitter=0.02, conc=4, level=-12.0)
def soul_light_apply(v, r):
    # The light marking the creature: a small bell's clear ping, motes of light shimmering off it, a breath of choir.
    f0 = kit.note(['A5', 'C#6', 'E6', 'A5'][v])
    ping = _bell(f0, child(r, 'ping'), 0.6, 0.6)
    over = _bell(f0 * 2.0, child(r, 'over'), 0.3, 0.4)
    sh = _shimmer(child(r, 'shimmer'), 6, 0.03, 0.3)
    choir = _voices(child(r, 'choir'), [f0 / 2.0], 0.55, 'a', 2, 0.6)
    x = layers((ping, 0.0, 0.0), (over, 0.0, -12.0), (sh, 0.0, -16.0), (choir, 0.02, -12.0))
    return _air(_calm(x), r, -14.0, 1.4, 5000.0)


@cue('Element.SoulLight.Tick', variations=4, att='Creature', jitter=0.02, conc=4, level=-10.0)
def soul_light_tick(v, r):
    # The smite bursting a second after the hit: a soft whump of light (air pushed out), a bright ring a major third wide,
    # a short choir stab on the chord, and the motes flying.
    root = kit.note(['A4', 'B4', 'C#5', 'A4'][v])
    whump = kit.noise_thump(0.25, child(r, 'whump'), 1600.0, 220.0, 0.08, 1.0, 0.05, 0.002)
    ring = layers((_bell(root * 2.0, child(r, 'r1'), 0.9, 0.7), 0.0, 0.0),
                  (_bell(root * 2.0 * 1.26, child(r, 'r2'), 0.8, 0.6), 0.004, -3.0),
                  (_bell(root * 4.0, child(r, 'r3'), 0.4, 0.4), 0.0, -12.0))
    rush = kit.whoosh(0.4, child(r, 'rush'), 900.0, 6000.0, 1.1, 0.12, -2.0, 0.8)
    choir = _voices(child(r, 'choir'), [root, root * 1.26, root * 1.5], 0.5, 'a', 1, 0.45)
    sh = _shimmer(child(r, 'shimmer'), 8, 0.02, 0.4)
    x = layers((_loud(whump), 0.0, -2.0), (ring, 0.0, -1.0), (_loud(rush), 0.0, -12.0), (choir, 0.01, -9.0),
               (sh, 0.0, -15.0))
    return _air(_calm(D.drive(normalize(x), 1.5, 'tanh')), r, -13.0, 1.6, 5000.0)


@cue('Element.SoulLight.Kill', variations=3, att='Creature', jitter=0.01, conc=2, level=-7.0)
def soul_light_kill(v, r):
    # The light taking it: a big bell struck (a cast bell's partials, warm), a choir's A major chord swelling up over it,
    # the motes rising away.
    dur = 1.7
    prime = kit.note(['A3', 'B3', 'G#3'][v])
    bell = M.strike(M.bell(prime, child(r, 'bell'), 2.2, 0.6, 0.8), M.hammer(0.0005), dur)
    bell = F.filt(bell, F.lp(5000.0, 0.7), extend=False)
    ping = _bell(prime * 4.0, child(r, 'ping'), 1.0, 0.6)
    choir = _voices(child(r, 'choir'), [prime * 2.0, prime * 2.52, prime * 3.0, prime * 4.0], 1.3, 'a', 2, 0.45)
    rise = kit.whoosh(1.1, child(r, 'rise'), 600.0, 7000.0, 1.0, 0.7, -2.0, 1.2)
    sh = _shimmer(child(r, 'shimmer'), 12, 0.15, 1.0)
    x = layers((_loud(bell, 0.02), 0.0, 0.0), (ping, 0.0, -6.0), (choir, 0.05, -6.0), (_loud(rise), 0.1, -16.0),
               (sh, 0.0, -14.0))
    return _air(_calm(x), r, -11.0, 2.0, 5000.0)


# --- Grave-salt ----------------------------------------------------------------------------------------------------------

def _crunch(dur, r, rate0, tau, lo=500.0, hi=2400.0):
    """Salt crust crushed: a dense, gritty crunch in the low mids, dying fast."""
    n = ns(dur)
    t = times(n)
    g = G.grit(dur, child(r, 'grit'), rate0 * np.exp(-t / tau) + rate0 * 0.05, lo, hi, 1.8, 5)
    return normalize(g * E.end_fade(E.perc(dur, 0.002, dur * 0.6), 0.01), 0.0)


def _frost(dur, r, lo=4500.0, hi=10000.0, attack=0.05):
    """A frosty hiss: cold air and salt eating in, band noise up high with a slow flutter."""
    n = ns(dur)
    x = N.band(n, child(r, 'frost'), lo, hi) * kit.turbulence(n, child(r, 'ft'), 25.0, 0.4)
    return normalize(x * E.end_fade(E.ar(dur, attack, dur * (1.0 - attack), 2.0), 0.02), 0.0)


def _crystals(dur, r, rate0, tau):
    """Crystals skittering: many tiny bright pings (each a few high modes rung for a hair) above the touchy band."""
    from recipes import throwables as T
    # Faded with their thinning, so the last lone grains sink under the hiss instead of clicking in the quiet.
    x = T._crystals(dur, child(r, 'c'), rate0, tau, 5200.0, 11000.0)
    return normalize(x * E.end_fade(np.exp(-6.9 * times(x.size) / (tau * 3.0)), 0.02), 0.0)


@cue('Element.GraveSalt.Apply', variations=4, att='Creature', jitter=0.05, conc=4, level=-11.0)
def grave_salt_apply(v, r):
    # The salt striking and crusting: a crystalline crunch, crystals skittering off, a frosty hiss as it bites in.
    knock = kit.thunk(520.0 * jitter(r, 1.0, 0.1), child(r, 'knock'), 0.03, 0.0004, 5, 0.12)
    crunch = _crunch(0.22, child(r, 'crunch'), 9000.0, 0.06)
    crys = _crystals(0.5, child(r, 'crys'), 900.0, 0.12)
    frost = _frost(0.55, child(r, 'frost'), attack=0.08)
    x = layers((_loud(knock, 0.003), 0.0, -3.0), (crunch, 0.0, 0.0), (crys, 0.005, -9.0), (frost, 0.01, -14.0))
    return _air(_calm(x), r, -18.0, 0.7)


@cue('Element.GraveSalt.Tick', variations=5, att='Creature', jitter=0.06, conc=6, level=-16.0)
def grave_salt_tick(v, r):
    # The salt eating in: a short frying hiss (colder than the sear's), a few crystals ticking, a tiny crunch at its start.
    dur = 0.4
    n = ns(dur)
    t = times(n)
    crunch = _crunch(0.08, child(r, 'crunch'), 6000.0, 0.03, 700.0, 2600.0)
    sizzle = G.grit(dur, child(r, 'sizzle'), 5000.0 * np.exp(-t / 0.15) + 1500.0, 5500.0, 10000.0, 2.0, 4)
    sizzle = normalize(sizzle * E.end_fade(E.perc(dur, 0.006, dur * 0.8), 0.02))
    frost = _frost(dur, child(r, 'frost'), attack=0.1)
    crys = _crystals(dur, child(r, 'crys'), 300.0, 0.15)
    x = layers((crunch, 0.0, 0.0), (sizzle, 0.0, -4.0), (frost, 0.0, -8.0), (crys, 0.02, -12.0))
    return _air(_calm(x), r, -20.0, 0.5)


@cue('Element.GraveSalt.Kill', variations=3, att='Creature', jitter=0.04, conc=3, level=-8.0)
def grave_salt_kill(v, r):
    # The crust shattering: a heavy crunch and a crack, a freezing crackle running over it (ice splitting), the crystals
    # raining down, a long hiss of salt settling.
    dur = 1.1
    n = ns(dur)
    t = times(n)
    crack = kit.noise_thump(0.2, child(r, 'crack'), 1800.0, 300.0, 0.06, 1.0, 0.04, 0.0006)
    crunch = _crunch(0.4, child(r, 'crunch'), 14000.0, 0.1, 400.0, 2200.0)
    freeze = G.grit(dur, child(r, 'freeze'), 900.0 * np.exp(-t / 0.3) + 500.0, 1200.0, 2400.0, 6.0, 3)
    freeze = normalize(freeze * E.end_fade(E.perc(dur, 0.01, dur * 0.5), 0.02))
    crys = _crystals(dur, child(r, 'crys'), 2400.0, 0.3)
    hiss = _frost(dur, child(r, 'hiss'), 4000.0, 10000.0, 0.15)
    x = layers((_loud(crack), 0.0, 0.0), (crunch, 0.0, -1.0), (freeze, 0.03, -12.0), (crys, 0.01, -7.0),
               (hiss, 0.02, -11.0))
    return _air(_calm(D.drive(normalize(x), 2.0, 'tanh')), r, -16.0, 1.0)


# --- Thunder -------------------------------------------------------------------------------------------------------------
# The arc's sparks sit in the low mids (1-3 kHz) and its fizz up high (6-11 kHz), so it reads bright without the ear's
# touchy band; the hum is the arc re-striking (a gate at 100-180 Hz), which is what makes it electric, not just noise.

def _snap(r, lo=1500.0):
    """The spark jumping: a hard bright crack (a shock front, a spit of hot air), a tick up high, a short knock of air."""
    front = kit.nwave(0.00025)
    crack = kit.burst(0.02, child(r, 'c'), 0.008, lo=lo, hi=12000.0)
    tick = kit.burst(0.004, child(r, 'tick'), 0.0012, lo=6000.0, hi=12000.0)
    knock = kit.noise_thump(0.06, child(r, 'k'), 2500.0, 300.0, 0.03, 1.0, 0.02, 0.0003)
    return normalize(layers((front, 0.0, -8.0), (_loud(crack, 0.002), 0.0, 0.0), (_loud(tick, 0.002), 0.0, -5.0),
                            (_loud(knock, 0.003), 0.0, -4.0)), 0.0)


def _zap(dur, r, g0, g1, lo=700.0, hi=2400.0, attack=0.002, t60=None, hum_db=-8.0, fizz_db=None):
    """An electric buzz: noise in the arc's band chopped by a fast gate (the arc striking again and again, g0 -> g1
    times a second, a little unsteady), with a hum at the gate's pitch under it and, if fizz_db, a sizzle up high (6-10
    kHz) on the same gate."""
    n = ns(dur)
    fg = O.expsweep(g0, g1, dur, 1.3, n=n) * (1.0 + 0.05 * N.smooth_random(n, child(r, 'w'), 30.0))
    gate = (0.5 + 0.5 * np.cos(2.0 * np.pi * O.phase(fg, n))) ** 6
    arc = normalize(N.band(n, child(r, 'n'), lo, hi) * gate * kit.turbulence(n, child(r, 't'), 200.0, 0.5))
    hum = normalize(F.filt(O.saw(fg, n=n), F.lp(1100.0, 0.7), extend=False))
    fizz = None
    if fizz_db is not None:
        fizz = (normalize(N.band(n, child(r, 'f'), 6000.0, 10000.0) * (0.3 + 0.7 * gate)), 0.0, fizz_db)
    env = E.end_fade(E.perc(dur, attack, t60 or dur * 0.8), 0.02)
    return normalize(layers((arc, 0.0, 0.0), (hum, 0.0, hum_db), fizz) * env, 0.0)


def _arc(dur, r, rate0, tau, floor, lo=1000.0, hi=2400.0, t60=None):
    """Sparks crackling: a dense spray of hard tiny clicks in the low mids with a fizz up high, thinning to a floor, over a
    buzzing bed held as long as they last; grains and bed fade together (t60), so no lone late grain clicks."""
    n = ns(dur)
    t = times(n)
    rate = rate0 * np.exp(-t / tau) + floor
    body = G.grit(dur, child(r, 'body'), rate, lo, hi, 3.0, 4)
    top = G.grit(dur, child(r, 'top'), 0.8 * rate, 6000.0, 11000.0, 2.0, 3)
    bed = _zap(dur, child(r, 'bed'), 150.0, 110.0, 800.0, 2000.0, 0.004, dur * 3.0, -12.0)
    x = layers((normalize(body), 0.0, 0.0), (normalize(top), 0.0, -4.0), (bed, 0.0, -14.0))
    return normalize(x * E.end_fade(E.perc(dur, 0.002, t60 or dur * 1.2), 0.02), 0.0)


def _dull(x):
    """Thunder's extra dip in the touchy band (on top of _calm's): its bite is kept up at 6-11 kHz."""
    return F.filt(x, F.peak(3600.0, -5.0, 0.6), extend=False)


def _whip(dur, r, f0, f1):
    """The arc leaping away: a crackling tone whose pitch falls fast (a whip of current), noise riding the same slide,
    chopped by the arc's gate."""
    n = ns(dur)
    f = O.expsweep(f0, f1, dur, 2.5, n=n)
    tone = O.fm(f, 0.5, 2.5 * E.perc(dur, 0.0005, dur * 0.6), n=n)
    hiss = F.sweep(N.white(n, child(r, 'n')), 'bp', f, 4.0)
    gate = 0.45 + 0.55 * (0.5 + 0.5 * np.cos(2.0 * np.pi * O.phase(140.0 * jitter(r, 1.0, 0.1), n))) ** 3
    x = layers((normalize(tone), 0.0, 0.0), (normalize(hiss), 0.0, -4.0)) * gate
    return normalize(x * E.end_fade(E.perc(dur, 0.0008, dur * 0.7), 0.01), 0.0)


@cue('Element.Thunder.Apply', variations=4, att='Creature', jitter=0.05, conc=4, level=-11.0)
def thunder_apply(v, r):
    # The jolt striking: a bright snap, a dense buzz of sparks, a short hum chopped by the arc, a zap falling away.
    snap = _snap(child(r, 'snap'))
    sparks = _arc(0.3, child(r, 'sparks'), 3200.0, 0.06, 450.0)
    hum = _zap(0.35, child(r, 'hum'), 140.0 * jitter(r, 1.0, 0.08), 115.0, 700.0, 2000.0, 0.003, 0.28, -12.0, -8.0)
    tail = _zap(0.28, child(r, 'tail'), 180.0, 70.0, 600.0, 1800.0, 0.01, 0.22, -6.0)
    x = layers((snap, 0.0, 0.0), (_loud(sparks, 0.01), 0.002, -6.0), (_loud(hum), 0.004, -7.0),
               (_loud(tail), 0.14 + 0.04 * r.random(), -12.0))
    return _air(_calm(_dull(D.drive(normalize(x), 3.0, 'tanh'))), r, -18.0)


@cue('Element.Thunder.Tick', variations=4, att='Creature', jitter=0.06, conc=6, level=-16.0)
def thunder_tick(v, r):
    # The burst and its arc leaping on: a snap, a crackling whip falling fast in pitch, a brief buzz.
    snap = _snap(child(r, 'snap'), 1800.0)
    whip = _whip(0.18 + 0.04 * r.random(), child(r, 'whip'), 1900.0 * jitter(r, 1.0, 0.1), 240.0 * jitter(r, 1.0, 0.1))
    sparks = _arc(0.2, child(r, 'sparks'), 2200.0, 0.05, 350.0)
    buzz = _zap(0.16, child(r, 'buzz'), 150.0 * jitter(r, 1.0, 0.1), 110.0, 700.0, 2000.0, 0.004, 0.12, -8.0, -6.0)
    x = layers((snap, 0.0, -2.0), (_loud(whip), 0.004, 0.0), (_loud(sparks, 0.01), 0.0, -9.0),
               (_loud(buzz), 0.03, -9.0))
    return _air(_calm(_dull(D.drive(normalize(x), 2.0, 'tanh'))), r, -20.0, 0.6)


@cue('Element.Thunder.Kill', variations=3, att='Creature', jitter=0.04, conc=3, level=-8.0)
def thunder_kill(v, r):
    # The jolt finishing it: a big crack with a low thud of air under it, sparks spraying, and a long sizzling buzz
    # sinking in pitch as the charge dies out.
    dur = 1.3
    snap = _snap(child(r, 'snap'), 1200.0)
    crack = kit.noise_thump(0.25, child(r, 'crack'), 2200.0, 250.0, 0.08, 1.0, 0.05, 0.0004)
    thud = kit.noise_thump(0.45, child(r, 'thud'), 450.0, 60.0, 0.25, 1.1, 0.1, 0.002)
    sparks = _arc(dur, child(r, 'sparks'), 4200.0, 0.25, 300.0, t60=0.75)
    sizzle = _zap(dur, child(r, 'sizzle'), 130.0 * jitter(r, 1.0, 0.08), 45.0, 600.0, 1900.0, 0.01, dur * 0.9, -10.0,
                  -6.0)
    x = layers((snap, 0.0, 0.0), (_loud(crack), 0.0, -2.0), (_loud(thud), 0.0, -4.0), (_loud(sparks, 0.02), 0.01, -9.0),
               (_loud(sizzle), 0.03, -6.0))
    return _air(_calm(_dull(D.drive(normalize(x), 2.0, 'tanh'))), r, -16.0, 1.2)


# --- Rot -----------------------------------------------------------------------------------------------------------------
# Wet and sick, all in the lows and low mids: gurgles and bubbles at 150-900 Hz, soft pops, a fizz kept above the touchy
# band, and a faint drone of two reedy tones a semitone apart (the sickness).

def _gurgle(dur, r, f0, f1, g0, g1, q=3.0):
    """Thick liquid gurgling: pink noise torn by turbulence through a liquid's formant sliding f0 -> f1, glugging g0 -> g1
    times a second."""
    n = ns(dur)
    x = N.pink(n, child(r, 'g')) * kit.turbulence(n, child(r, 'gt'), 60.0, 0.85)
    fc = O.expsweep(f0, f1, dur, 1.0, n=n) * (1.0 + 0.15 * N.smooth_random(n, child(r, 'fw'), 8.0))
    y = F.sweep(x, 'bp', fc, q) + 0.4 * F.sweep(x, 'bp', fc * 2.3, q * 1.2)
    pulse = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(g0, g1, dur, 1.0, n=n), n))
    y = normalize(y) * (0.3 + 0.7 * pulse ** 1.5)
    return normalize(y * E.end_fade(E.ar(dur, 0.03, dur * 0.6, 1.5), 0.02), 0.0)


def _bubbles(dur, r, rate, lo, hi, fall_db=0.0):
    """Bubbles rising through the rot: each a ring a little higher as it closes, lo..hi Hz, rate a second (a number or a
    function of time), each weaker by fall_db over dur."""
    out = np.zeros(1)
    for tt in N.times_poisson(dur, r, rate):
        f = float(np.exp(r.uniform(np.log(lo), np.log(hi))))
        out = layers(out, (kit.bubble(f, r, 0.12), tt, r.uniform(-8.0, 0.0) - fall_db * tt / dur))
    return normalize(out, 0.0) if out.size > 1 else out


def _wet_pop(r, f0):
    """A blister of rot bursting: a soft wet smack in the low mids, a bubble's ring, a dull knock."""
    smack = kit.squelch(0.06, child(r, 'sq'), f0 * 2.0, f0 * 0.6, 2.5)
    ring = kit.bubble(f0, child(r, 'b'), 0.15)
    knock = kit.thunk(f0 * 0.5, child(r, 'k'), 0.03, 0.001, 4, 0.12)
    return normalize(layers((smack, 0.0, 0.0), (normalize(ring), 0.002, -3.0), (knock, 0.0, -8.0)), 0.0)


def _fizz(dur, r, rate, attack, release=None):
    """The rot fizzing as it eats: a fine wet sizzle up high (above the touchy band), a soft breathy hiss there, and a
    duller wet hiss in the low mids."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'g'), rate, 5500.0, 10000.0, 2.0, 4)
    h = N.band(n, child(r, 'h'), 5000.0, 9000.0) * kit.turbulence(n, child(r, 't'), 25.0, 0.5)
    w = N.band(n, child(r, 'w'), 700.0, 1700.0) * kit.turbulence(n, child(r, 'wt'), 40.0, 0.6)
    x = layers((normalize(g), 0.0, 0.0), (normalize(h), 0.0, -5.0), (normalize(w), 0.0, -6.0))
    return normalize(x * E.end_fade(E.ar(dur, attack, release or dur - attack, 2.0), 0.02), 0.0)


def _drone(dur, r, f0, attack, release):
    """A faint sick drone: two low reedy tones a semitone apart, beating sourly and wavering."""
    n = ns(dur)
    w = 1.0 + 0.012 * N.smooth_random(n, child(r, 'w'), 3.0)
    a = O.saw(f0 * w, n=n)
    b = O.saw(f0 * 2.0 ** (1.0 / 12.0) * w * (1.0 + 0.008 * N.smooth_random(n, child(r, 'w2'), 2.0)), n=n)
    x = F.filt(a + b, F.lp(650.0, 0.7), extend=False)
    return normalize(x * E.end_fade(E.ar(dur, attack, release, 2.0), 0.02), 0.0)


@cue('Element.Rot.Apply', variations=4, att='Creature', jitter=0.05, conc=4, level=-11.0)
def rot_apply(v, r):
    # The rot taking hold: a soft wet pop as it lands, gurgling and low bubbles welling up, a wet fizz, a faint sick drone.
    dur = 0.7
    pop = _wet_pop(child(r, 'pop'), 520.0 * jitter(r, 1.0, 0.12))
    gurgle = _gurgle(0.6, child(r, 'gurgle'), 230.0, 460.0 * jitter(r, 1.0, 0.1), 7.0, 11.0)
    bub = _bubbles(0.6, child(r, 'bub'), 32.0, 250.0, 900.0, 6.0)
    fizz = _fizz(0.55, child(r, 'fizz'), 1500.0, 0.06)
    drone = _drone(dur, child(r, 'drone'), kit.note(['B2', 'C3', 'A2', 'Bb2'][v]), 0.15, 0.4)
    x = layers((_loud(pop, 0.003), 0.0, 0.0), (_loud(gurgle), 0.01, -4.0), (bub, 0.02, -6.0), (fizz, 0.02, -13.0),
               (drone, 0.0, -20.0))
    return _air(_calm(D.drive(normalize(x), 2.0, 'tanh')), r, -18.0)


@cue('Element.Rot.Tick', variations=5, att='Creature', jitter=0.06, conc=6, level=-16.0)
def rot_tick(v, r):
    # Rotting on: two or three wet blisters popping, a short fizz, a few small bubbles.
    dur = 0.4
    out = layers((_wet_pop(child(r, 'p0'), 480.0 * jitter(r, 1.0, 0.15)), 0.0, 0.0))
    for i in range(1 + v % 3):
        out = layers(out, (_wet_pop(child(r, 'p', i), 560.0 * jitter(r, 1.0, 0.25)), 0.05 + 0.2 * r.random(),
                           -3.0 - 4.0 * r.random()))
    fizz = _fizz(dur, child(r, 'fizz'), 1000.0, 0.03)
    bub = _bubbles(0.32, child(r, 'bub'), 25.0, 300.0, 850.0, 6.0)
    gurgle = _gurgle(dur, child(r, 'gurgle'), 260.0, 380.0, 9.0, 9.0)
    x = layers((_loud(out, 0.003), 0.0, 0.0), (fizz, 0.0, -12.0), (bub, 0.01, -9.0), (_loud(gurgle), 0.0, -12.0))
    return _air(_calm(x), r, -20.0, 0.6)


@cue('Element.Rot.Kill', variations=3, att='Creature', jitter=0.04, conc=3, level=-8.0)
def rot_kill(v, r):
    # The body collapsing: a wet burst and a soft thud, a gush of bubbling as it caves in, the rot's cloud hissing out and
    # fading over the sick drone sinking.
    dur = 1.4
    burst = kit.squelch(0.25, child(r, 'burst'), 1200.0 * jitter(r, 1.0, 0.1), 220.0, 2.5)
    thud = kit.noise_thump(0.35, child(r, 'thud'), 700.0, 70.0, 0.18, 1.1, 0.08, 0.002)
    pop = _wet_pop(child(r, 'pop'), 420.0)
    gush = _gurgle(1.0, child(r, 'gush'), 200.0, 520.0 * jitter(r, 1.0, 0.1), 8.0, 15.0, 2.5)
    bub = _bubbles(1.1, child(r, 'bub'), lambda t: 10.0 + 80.0 * np.exp(-t / 0.35), 180.0, 800.0, 10.0)
    cloud = _fizz(1.25, child(r, 'cloud'), 2500.0, 0.15, 1.0)
    drone = _drone(dur, child(r, 'drone'), kit.note(['A2', 'B2', 'Bb2'][v]), 0.08, 1.0)
    x = layers((_loud(burst), 0.0, 0.0), (_loud(thud), 0.0, -1.0), (_loud(pop, 0.003), 0.0, -4.0),
               (_loud(gush), 0.04, -3.0), (bub, 0.05, -6.0), (cloud, 0.06, -10.0), (drone, 0.0, -18.0))
    return _air(_calm(D.drive(normalize(x), 2.0, 'tanh')), r, -16.0, 1.2)
