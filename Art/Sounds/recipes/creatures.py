"""Creatures (3D): each kind sounds like what it is, down to the bullet going in.

- Any body (the fallback for a creature with no kind of its own): a meaty hit.
- The brown spiders are hard shells on thin legs, with no voice to speak of: chitin cracking under a bullet, mandibles
  chittering, a stridulating rasp (a ridged file scraped on a pick), hisses of air from the spiracles, legs skittering.
- The meadow slimes are blobs of living jelly round a core, with no voice and no bone: wet slaps and squelches, gel
  squishing through itself and wobbling, gloopy bloops as holes in it close, bubbles, sloshes, a splat into a puddle.
- The Unpaid are ghosts of the unpaid dead: a bullet meets something not quite there (a hollow thump, grave dust and
  old cloth, a cold ring); their voices are breathy wails of several voices at once, a rasping shriek, a rush of cold
  air, a long sinking death into the dark.

Each kind's bullet hit is its own cue, played at the wound for every bullet, so it stays short and gentle in the
ear's sharpest band; bigger and smaller kin play their kind's cues pitched down or up (the Gravemother, spiderlings,
Abel). The Unpaid's voices are formant synthesis (lib/voice.py): a glottal source shaped by moving vocal-tract
resonances, with breath, pitch wobble and roughness, so they sound alive rather than electronic.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono, reverse, smoothstep
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G, voice as V
from recipes import cue
from recipes import kit


def _outside(x, r, rel_db=-20.0, s1=0.07, s2=0.18, t60=0.5):
    ir = R.outdoor(child(r, 'ir'), dur=max(0.7, t60 * 1.4), slaps=((s1, -4.0), (s2, -9.0)), tail_t60=t60,
                   tail_db=-10.0, tail_start=0.02, tail_peak=0.08)
    wet = F.convolve(F.filt(x, F.hpn(200.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _haunt(x, r, rel_db=-8.0, t60=2.4, lp=3200.0):
    """The Unpaid's space: a wide, dark open-air wash, louder than for anything living."""
    ir = R.open_air(child(r, 'haunt'), t60=t60, lp=lp, predelay=0.02, density=0.8)
    wet = F.convolve(F.filt(x, F.hp(180.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.05))


# --- Any creature ---------------------------------------------------------------------------------------------------

@cue('Creature.Hit', variations=5, att='Creature', jitter=0.06, conc=6, level=-10.0)
def creature_hit(v, r):
    # A bullet into a body: a snap, a punch, the meaty slap of it, something wet, a crack of shell or bone.
    snap = kit.burst(0.005, child(r, 'snap'), 0.002, lo=1200.0, hi=7000.0)
    punch = kit.thump(160.0 * jitter(r, 1, 0.1), 70.0, 0.07, 0.04, drive_db=3.0)
    n = ns(0.08)
    slap = N.band(n, child(r, 'slap'), 600.0, 2400.0) * kit.turbulence(n, child(r, 'st'), 300.0, 0.6) * E.perc(0.08, 0.0005, 0.035)
    wet = np.zeros(1)
    for i in range(2 + int(r.integers(0, 3))):
        wet = layers(wet, (kit.bubble(r.uniform(450.0, 1300.0), r, 0.15), r.uniform(0.005, 0.04), r.uniform(-6.0, 0.0)))
    squish = kit.squelch(0.09, child(r, 'squish'), 1400.0, 400.0)
    crack = kit.grit_burst(0.04, child(r, 'crack'), 6000.0, 1800.0, 5500.0, 0.02)
    x = layers((snap, 0.0, -7.0), (punch, 0.0, -3.0), (normalize(slap), 0.0, 0.0), (normalize(wet), 0.0, -12.0),
               (squish, 0.004, -10.0), (crack, 0.0, -15.0))
    return F.filt(x, F.peak(3200.0, -3.0, 0.8))


# --- Spiders --------------------------------------------------------------------------------------------------------

def chitter(dur, r, rate0, rate1, lo=1700.0, hi=4800.0, spread=0.3, shape=1.0):
    """Mandibles clicking: a train of tiny chitin clicks at a rate gliding from rate0 to rate1, irregular, each its
    own small resonance."""
    clicks = [normalize(M.strike(M.parts(lo, hi, child(r, 'k', i), 3, 0.007, 0.004), M.hammer(0.00005), 0.02))
              for i in range(5)]
    out = np.zeros(ns(dur) + ns(0.03))
    t = 0.0
    while t < dur:
        u = (t / dur) ** shape
        rate = rate0 + (rate1 - rate0) * u
        k = ns(t)
        c = clicks[int(r.integers(0, 5))] * r.uniform(0.4, 1.0)
        out[k:k + c.size] += c[:max(0, out.size - k)]
        t += (1.0 / rate) * (1.0 + spread * (2 * r.random() - 1))
    return normalize(out)


def rasp(dur, r, rate0, rate1, body=(1300.0, 2700.0, 4100.0), q=2.5):
    """Stridulation: a ridged file scraped across a pick (how many spiders and insects make sound), a buzzing train
    of small hits rung through the body."""
    return kit.creak(dur, r, rate0, rate1, body, q, 0.15)


def _skitter(r, count, start, gap):
    """Legs tapping the ground fast and light."""
    out = np.zeros(1)
    t = start
    for i in range(count):
        tap = kit.thunk(r.uniform(900.0, 1500.0), child(r, 'tap', i), 0.012, 0.00015, 3, 0.1)
        out = layers(out, (tap, t, r.uniform(-8.0, 0.0)))
        t += gap * r.uniform(0.6, 1.4)
    return normalize(out)


def _chitin(r, f0=650.0, t60=0.03, cracks=3):
    """A plate of chitin cracking: the shell's hard, hollow knock (a few quick modes, like lacquered wood), brittle
    snaps as the crack runs across it, each fainter, and the crunch of it breaking up."""
    out = kit.thunk(f0, child(r, 'knock'), t60, 0.00015, 6, 0.12)
    t = 0.0
    for i in range(cracks):
        snap = normalize(M.strike(M.parts(1100.0, 6000.0, child(r, 'snap', i), 5, 0.009, 0.004), M.hammer(0.00006), 0.025))
        out = layers(out, (snap, t, -3.0 - 4.0 * i))
        t += r.uniform(0.006, 0.014)
    crunch = kit.grit_burst(0.07, child(r, 'crunch'), 8000.0, 800.0, 6500.0, 0.04, 0.0005)
    return normalize(layers(out, (crunch, 0.001, -2.0)), 0.0)


@cue('Creature.Spider.Hit', variations=5, att='Creature', jitter=0.06, conc=6, level=-10.0)
def spider_hit(v, r):
    # A bullet cracking chitin: the shell knocks, cracks and crunches as it breaks, the body's weight behind it, and a
    # small wet splat of what's under the shell.
    shell = _chitin(child(r, 'shell'), 650.0 * jitter(r, 1, 0.15), 0.03 * jitter(r, 1, 0.2), 2 + int(r.integers(0, 3)))
    punch = kit.thump(170.0 * jitter(r, 1, 0.1), 85.0, 0.06, 0.03, drive_db=2.0)
    splat = kit.squelch(0.07, child(r, 'splat'), 1000.0 * jitter(r, 1, 0.1), 350.0, 2.5)
    wet = np.zeros(1)
    for i in range(1 + int(r.integers(0, 2))):
        wet = layers(wet, (kit.bubble(r.uniform(500.0, 1200.0), r, 0.15), r.uniform(0.008, 0.03), r.uniform(-6.0, 0.0)))
    x = layers((shell, 0.0, 0.0), (punch, 0.0, -8.0), (splat, 0.004, -9.0), (normalize(wet), 0.0, -16.0))
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -3.0), extend=False)


@cue('Creature.Spider.Alert', variations=3, att='Creature', jitter=0.06, conc=3, level=-7.0)
def spider_alert(v, r):
    # It's seen you: the mandibles start clicking and speed up, the rasp swells, a hiss rises behind it.
    ch = chitter(0.7, child(r, 'chitter'), 12.0, 36.0, shape=0.7)
    ch = ch[:ns(0.75)] * E.ar(0.75, 0.05, 0.25)[:ch[:ns(0.75)].size]
    rs = rasp(0.65, child(r, 'rasp'), 85.0 * jitter(r, 1, 0.1), 140.0)
    n = ns(0.8)
    hs = V.hiss(n, child(r, 'hiss')) * E.swell(0.8, 0.55, 0.4) * (0.75 + 0.25 * np.sin(2 * np.pi * 26.0 * times(n)))
    throat = kit.creak(0.6, child(r, 'throat'), 28.0, 36.0, (240.0, 520.0), 4.0)
    x = layers((ch, 0.0, 0.0), (rs, 0.1, -6.0), (normalize(hs), 0.15, -9.0), (throat, 0.1, -19.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


@cue('Creature.Spider.Attack', variations=4, att='Creature', jitter=0.06, conc=3, level=-7.0)
def spider_attack(v, r):
    # The lunge: a sharp hiss, legs skittering, the mandibles snapping shut twice with a crunch.
    n = ns(0.3)
    hs = V.hiss(n, child(r, 'hiss'), (2400.0, 4000.0, 6500.0)) * E.perc(0.3, 0.012, 0.25)
    legs = _skitter(child(r, 'legs'), 6 + int(r.integers(0, 4)), 0.0, 0.035)
    t = 0.14 + 0.04 * r.random()
    snap = layers(kit.clack(child(r, 's1'), 900.0, 1500.0, 5000.0, 0.02, 0.03, -2.0, -8.0),
                  (kit.grit_burst(0.04, child(r, 'c1'), 8000.0, 1500.0, 5000.0, 0.02), 0.0, -8.0))
    snap2 = kit.clack(child(r, 's2'), 1000.0, 1600.0, 5200.0, 0.02, 0.025, -3.0, -10.0)
    ch = chitter(0.25, child(r, 'chitter'), 40.0, 25.0)
    x = layers((normalize(hs), 0.0, -4.0), (legs, 0.0, -8.0), (snap, t, 0.0), (snap2, t + 0.03, -3.0), (ch, t + 0.05, -10.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


@cue('Creature.Spider.Hurt', variations=4, att='Creature', jitter=0.06, conc=3, level=-8.0)
def spider_hurt(v, r):
    # Pain, the way a spider has it: a sharp hiss of air from its spiracles, a burst of harsh rasping as it works its
    # file in alarm, frantic clicking and legs scrabbling.
    n = ns(0.3)
    hs = V.hiss(n, child(r, 'hiss'), (2000.0, 3400.0, 6200.0), levels=(0.7, 0.5, 1.0)) * E.perc(0.3, 0.004, 0.2)
    rs = rasp(0.24, child(r, 'rasp'), 170.0 * jitter(r, 1, 0.1), 110.0, (1000.0, 2100.0, 3900.0), 3.0)
    rs = rs * E.perc(0.24, 0.005, 0.22)
    ch = chitter(0.3, child(r, 'chitter'), 50.0, 30.0)
    legs = _skitter(child(r, 'legs'), 5, 0.02, 0.03)
    x = layers((normalize(rs), 0.0, 0.0), (normalize(hs), 0.0, -6.0), (ch, 0.02, -5.0), (legs, 0.0, -12.0))
    return _outside(F.filt(x, F.peak(3300.0, -4.0, 0.8), extend=False), r, -21.0)


@cue('Creature.Spider.Death', variations=3, att='Creature', jitter=0.05, conc=3, level=-7.0)
def spider_death(v, r):
    # It gives out: a hiss as the air goes out of it, the rasp winding down into separate scrapes, the clicking slowing
    # and stopping, the shell settling with a dry crackle and a knock, the legs curling in with a few taps, a last hiss.
    n = ns(0.9)
    hs = V.hiss(n, child(r, 'hiss'), (2000.0, 3500.0, 6000.0)) * E.perc(0.9, 0.006, 0.7)
    hs = F.sweep(hs, 'lp', O.expsweep(8000.0, 1600.0, 0.9, n=n), 0.8)
    rs = rasp(0.9, child(r, 'rasp'), 140.0 * jitter(r, 1, 0.1), 18.0, (900.0, 1900.0, 3600.0), 3.0)
    rs *= np.linspace(1.0, 0.4, rs.size)
    ch = chitter(1.0, child(r, 'chitter'), 30.0, 4.0, shape=0.6)
    ch *= np.linspace(1.0, 0.3, ch.size)
    settle = kit.grit_burst(0.25, child(r, 'settle'), 900.0, 700.0, 4000.0, 0.18, 0.01)
    knock = kit.thunk(600.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.03, 0.0004)
    curl = _skitter(child(r, 'curl'), 5, 0.0, 0.11)
    m = ns(0.6)
    last = V.hiss(m, child(r, 'last'), (2000.0, 3500.0, 5500.0)) * E.swell(0.6, 0.1, 0.5)
    last = F.sweep(last, 'lp', O.expsweep(7000.0, 1500.0, 0.6, n=m), 0.8)
    x = layers((normalize(rs), 0.0, 0.0), (normalize(hs), 0.0, -5.0), (ch, 0.0, -6.0), (settle, 0.4, -14.0),
               (knock, 0.42, -10.0), (curl, 0.55, -10.0), (normalize(last), 1.0, -12.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


# --- Slimes ---------------------------------------------------------------------------------------------------------

def _wet_slap(dur, r, lo, hi, t60, attack=0.0008):
    """Gel smacking something (or being smacked): broadband but soft-edged, torn up by the liquid's turbulence."""
    n = ns(dur)
    x = N.band(n, r, lo, hi) * kit.turbulence(n, child(r, 'turb'), 260.0, 0.6)
    return normalize(x * E.perc(dur, attack, t60), 0.0)


def _squish(dur, r, rate, lo=600.0, hi=2200.0, start=0.0):
    """Gel squeezing through itself: a dense crackle of tiny wet pops, the smallest bubbles opening and snapping shut.
    It's what makes a thing sound squishy rather than just wet. rate: pops per second (a number, or a function of
    time)."""
    out = np.zeros(1)
    for t in N.times_poisson(dur, r, rate, start):
        f = float(np.exp(r.uniform(np.log(lo), np.log(hi))))
        out = layers(out, (kit.bubble(f, r, 0.3), t, r.uniform(-14.0, 0.0)))
    return normalize(out, 0.0) if out.size > 1 else out


def _bloop(f0, f1, dur, r, t60=None):
    """A cavity in the gel closing (where a bullet or the core went in): a tone gliding up as the hole shrinks, like
    a big, slow bubble, with a little of its second harmonic, fluttering as the thick walls slap together."""
    n = ns(dur)
    ph = 2.0 * np.pi * O.phase(O.expsweep(f0, f1, dur, 0.8, n=n), n)
    x = (np.sin(ph) + 0.25 * np.sin(2.0 * ph + 0.3)) * (0.75 + 0.25 * N.smooth_random(n, r, 60.0))
    return normalize(x * E.perc(dur, 0.003, t60 or dur * 0.8), 0.0)


def _wobble(dur, r, fc, rate, t60, depth=0.35, body_db=-8.0, attack=0.004):
    """The gel wobbling after it's struck, lands or flinches: its wet surface noise through a resonance that swings up
    and down as the blob squashes and stretches, pulsing with it, over a low, wavering note of the whole body; dying
    away."""
    n = ns(dur)
    wob = np.sin(2.0 * np.pi * rate * times(n) + 2.0 * np.pi * r.random())
    x = N.pink(n, r) * kit.turbulence(n, child(r, 'turb'), 200.0, 0.8)
    fcs = fc * (1.0 + depth * wob)
    y = F.sweep(x, 'bp', fcs, 3.0) + 0.5 * F.sweep(x, 'bp', np.minimum(fcs * 2.2, 12000.0), 3.5)
    y = normalize(y * (0.3 + 0.7 * (0.5 + 0.5 * wob)))
    body = np.sin(2.0 * np.pi * O.phase(fc * 0.3 * (1.0 + 0.1 * wob), n))
    return normalize(layers(y, (body, 0.0, body_db)) * E.perc(dur, attack, t60), 0.0)


def _stretch(dur, r, f0, f1):
    """Gel being drawn out: thick wet noise through a resonance that climbs as the jelly thins, sticky little pops in
    it, swelling until it lets go."""
    n = ns(dur)
    x = N.pink(n, r) * kit.turbulence(n, child(r, 'turb'), 90.0, 0.8)
    fc = O.expsweep(f0, f1, dur, 1.0, n=n)
    y = normalize(F.sweep(x, 'bp', fc, 4.0) + 0.4 * F.sweep(x, 'bp', np.minimum(fc * 2.3, 12000.0), 4.0))
    stick = _squish(dur, child(r, 'stick'), lambda t: 60.0 + 400.0 * t / dur, 500.0, 1600.0)
    y = layers(y, (stick, 0.0, -6.0))[:n]
    return normalize(y * E.ar(dur, dur * 0.75, dur * 0.12, 2.0), 0.0)


def _slosh(dur, r, f0, f1, rate0, rate1):
    """Liquid settling: low wet noise rising and falling in slow waves as it sloshes and spreads, its resonance
    sinking as it flattens out into a puddle."""
    n = ns(dur)
    x = N.pink(n, r) * kit.turbulence(n, child(r, 'turb'), 120.0, 0.7)
    fc = O.expsweep(f0, f1, dur, 1.0, n=n)
    y = F.sweep(x, 'bp', fc, 2.5) + 0.4 * F.sweep(x, 'bp', np.minimum(fc * 2.3, 12000.0), 3.0)
    waves = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(rate0, rate1, dur, 1.0, n=n), n))
    return normalize(y * (0.25 + 0.75 * waves ** 1.5) * E.ar(dur, 0.02, dur * 0.6), 0.0)


def _bubbles(dur, r, rate, lo, hi, start=0.0, decay=None):
    out = np.zeros(1)
    fn = rate if callable(rate) else (lambda t: rate)
    for t in N.times_poisson(dur, r, fn, start):
        a = 0.0 if decay is None else -20.0 * t / decay
        out = layers(out, (kit.bubble(r.uniform(lo, hi), r, 0.12), t, a + r.uniform(-8.0, 0.0)))
    return normalize(out) if out.size > 1 else out


@cue('Creature.Slime.Hit', variations=5, att='Creature', jitter=0.06, conc=6, level=-10.0)
def slime_hit(v, r):
    # A bullet into jelly: a soft wet slap on its skin, the gel squelching round the hole and squishing through
    # itself, the hole closing with a gloopy bloop, the whole blob wobbling, a few bubbles. No crack, no bone.
    slap = _wet_slap(0.05, child(r, 'slap'), 200.0, 2200.0, 0.025)
    weight = kit.noise_thump(0.06, child(r, 'weight'), 500.0, 90.0, 0.04)
    squelch = kit.squelch(0.11, child(r, 'squelch'), 1000.0 * jitter(r, 1, 0.15), 300.0, 3.0)
    squish = _squish(0.08, child(r, 'squish'), 260.0, 600.0, 2000.0)
    bloop = _bloop(170.0 * jitter(r, 1, 0.12), 380.0, 0.11, child(r, 'bloop'))
    wob = _wobble(0.22, child(r, 'wob'), 380.0 * jitter(r, 1, 0.1), 11.0 + 4.0 * r.random(), 0.16)
    bub = _bubbles(0.15, child(r, 'bub'), 18.0, 350.0, 1100.0, 0.02)
    x = layers((slap, 0.0, -2.0), (weight, 0.0, -6.0), (squelch, 0.002, 0.0), (squish, 0.004, -10.0),
               (bloop, 0.012, -3.0), (wob, 0.02, -11.0), (bub, 0.0, -14.0))
    return F.filt(x, F.lp(3500.0, 0.7), extend=False)


@cue('Creature.Slime.Hop', variations=4, att='Creature', jitter=0.08, conc=4, level=-13.0)
def slime_hop(v, r):
    # Landing from a hop (it plays on every landing of every slime, so it stays soft and short): the gel's wet plap on
    # the ground, its weight settling with a soft blop, a little squish, the blob wobbling still.
    plap = _wet_slap(0.06, child(r, 'plap'), 150.0, 1500.0, 0.035, 0.0015)
    weight = kit.noise_thump(0.09, child(r, 'weight'), 450.0 * jitter(r, 1, 0.1), 80.0, 0.05, 1.0, 0.04)
    blop = _bloop(110.0 * jitter(r, 1, 0.1), 190.0, 0.08, child(r, 'blop'), 0.06)
    squish = kit.squelch(0.07, child(r, 'squish'), 700.0 * jitter(r, 1, 0.1), 280.0, 2.5)
    wob = _wobble(0.2, child(r, 'wob'), 300.0 * jitter(r, 1, 0.1), 12.0 + 3.0 * r.random(), 0.14)
    x = layers((plap, 0.0, 0.0), (weight, 0.0, -3.0), (blop, 0.006, -8.0), (squish, 0.004, -6.0), (wob, 0.015, -12.0))
    if v % 2:
        x = layers(x, (kit.bubble(r.uniform(400.0, 900.0), r, 0.12), 0.05 + 0.04 * r.random(), -18.0))
    return _outside(F.filt(x, F.lp(2800.0, 0.7), extend=False), r, -26.0)


@cue('Creature.Slime.Attack', variations=3, att='Creature', jitter=0.06, conc=3, level=-9.0)
def slime_attack(v, r):
    # It gathers itself and throws itself at you: the gel peels off the ground and draws back with a sticky,
    # stretching squelch, flies, and lands on you in a heavy wet slap, bubbles churning.
    peel = _wet_slap(0.03, child(r, 'peel'), 300.0, 2000.0, 0.015)
    draw = _stretch(0.3, child(r, 'draw'), 280.0 * jitter(r, 1, 0.1), 900.0)
    t = 0.3 + 0.04 * r.random()
    air = kit.whoosh(0.16, child(r, 'air'), 250.0, 700.0, 1.0, 0.7)
    splat = layers(_wet_slap(0.08, child(r, 'slap'), 150.0, 2500.0, 0.05),
                   (kit.noise_thump(0.14, child(r, 'thump'), 900.0, 110.0, 0.07), 0.0, -2.0),
                   (kit.squelch(0.13, child(r, 'squelch'), 1500.0, 300.0), 0.003, -3.0))
    wob = _wobble(0.3, child(r, 'wob'), 340.0, 10.0 + 3.0 * r.random(), 0.22)
    bub = _bubbles(0.4, child(r, 'bub'), 30.0, 350.0, 1300.0, 0.0, 0.4)
    x = layers((peel, 0.0, -6.0), (draw, 0.0, -10.0), (air, t - 0.15, -16.0), (splat, t, 0.0), (wob, t + 0.02, -10.0),
               (bub, t + 0.02, -11.0))
    return _outside(F.filt(x, F.lp(3500.0, 0.7), extend=False), r)


@cue('Creature.Slime.Hurt', variations=4, att='Creature', jitter=0.06, conc=3, level=-10.0)
def slime_hurt(v, r):
    # It flinches: the gel clenches with a squelch that churns as it wobbles, a gloopy blorp from deep in it, bubbles
    # spurting from the wound.
    slap = _wet_slap(0.04, child(r, 'slap'), 200.0, 1800.0, 0.02)
    churn = _wobble(0.32, child(r, 'churn'), 650.0 * jitter(r, 1, 0.12), 9.0 + 3.0 * r.random(), 0.26, 0.45, -16.0)
    blorp = _bloop(140.0 * jitter(r, 1, 0.1), 320.0, 0.14, child(r, 'blorp'), 0.12)
    squish = _squish(0.12, child(r, 'squish'), 300.0, 600.0, 2000.0)
    bub = _bubbles(0.35, child(r, 'bub'), lambda t: 80.0 * np.exp(-t / 0.15), 300.0, 1500.0)
    x = layers((slap, 0.0, -4.0), (churn, 0.003, 0.0), (blorp, 0.03, -5.0), (squish, 0.0, -8.0), (bub, 0.02, -8.0))
    return _outside(F.filt(x, F.lp(3500.0, 0.7), extend=False), r, -22.0)


@cue('Creature.Slime.Death', variations=3, att='Creature', jitter=0.05, conc=3, level=-8.0)
def slime_death(v, r):
    # It bursts and collapses: a big wet splat, the jelly slumping and sloshing out into a puddle, the core dropping
    # into it with a deep bloop, bubbles rising slower and slower, the last drips.
    splat = layers(_wet_slap(0.1, child(r, 'slap'), 150.0, 3000.0, 0.06, 0.001),
                   (kit.noise_thump(0.3, child(r, 'thump'), 1400.0, 90.0, 0.14, 1.0, 0.08), 0.0, 0.0),
                   (kit.squelch(0.22, child(r, 'squelch'), 1600.0, 250.0, 2.5), 0.004, -2.0))
    squish = _squish(0.35, child(r, 'squish'), lambda t: 500.0 * np.exp(-t / 0.12), 500.0, 2200.0)
    wob = _wobble(0.5, child(r, 'wob'), 320.0, 9.0 + 2.0 * r.random(), 0.4)
    slosh = _slosh(1.0, child(r, 'slosh'), 700.0, 220.0, 5.0, 2.0)
    plop = _bloop(95.0 * jitter(r, 1, 0.08), 230.0, 0.2, child(r, 'plop'), 0.18)
    bub = _bubbles(1.2, child(r, 'bub'), lambda t: 45.0 * np.exp(-t / 0.4) + 3.0, 220.0, 1100.0, 0.1, 1.2)
    drips = layers(*[(kit.bubble(r.uniform(900.0, 2000.0), r, 0.15), 0.95 + 0.12 * i + 0.05 * r.random(), -6.0 * i)
                     for i in range(3)])
    x = layers((splat, 0.0, 0.0), (squish, 0.005, -8.0), (wob, 0.03, -10.0), (slosh, 0.06, -6.0), (plop, 0.32, -4.0),
               (bub, 0.0, -9.0), (normalize(drips), 0.0, -16.0))
    return _outside(F.filt(x, F.lp(3500.0, 0.7), extend=False), r)


# --- The Unpaid -----------------------------------------------------------------------------------------------------

def _choir(r, f0_points, dur, keys, count=3, detune=14.0, spread=0.04, breath=0.55, shift=1.1, vib=(5.0, 0.3),
           rough=0.0, tilt=0.0):
    """Several breathy voices on one cry, a little apart in pitch and time: one ghost sounds like many."""
    n = ns(dur)
    out = np.zeros(n + ns(spread * count))
    for i in range(count):
        rr = child(r, 'voice', i)
        cents = detune * (i - (count - 1) / 2.0) + rr.uniform(-3.0, 3.0)
        f0 = O.glide(f0_points, n=n) * 2.0 ** (cents / 1200.0)
        x = V.voice(f0, keys, rr, breath=breath, jitter=0.012, shimmer=0.12, tilt=tilt, shift=shift * rr.uniform(0.97, 1.03),
                    vib_rate=vib[0] * rr.uniform(0.85, 1.15), vib_depth=vib[1], rough=rough)
        k = ns(spread * i * rr.uniform(0.5, 1.0))
        out[k:k + n] += normalize(x) * (1.0 if i == 0 else rr.uniform(0.5, 0.8))
    return normalize(out)


def _cold(x, r, rate=33.0, depth=0.2):
    """A faint ring modulation: the voice not quite of this world."""
    n = x.size
    return x * (1.0 - depth + depth * np.sin(2 * np.pi * rate * times(n) + 2 * np.pi * r.random()))


def _hollow(r, f0, t60=0.12, damp=2200.0):
    """Something hollow struck: a puff of noise rung through a tube's comb of resonances, so the body sounds like an
    empty shape rather than flesh."""
    exc = N.pink(ns(0.006), r) * E.perc(0.006, 0.0005, 0.004)
    g = 0.001 ** (1.0 / (t60 * f0))
    y = F.filt(np.pad(exc, (0, ns(t60 * 1.2))), F.comb(1.0 / f0, g, damp), extend=False)
    return normalize(y * E.end_fade(np.ones(y.size), 0.01), 0.0)


def _ghost_ring(f0, dur, r):
    """A faint, cold ring left hanging: a glassy pair of tones beating slowly against each other with a stretched
    partial over them, ring-modulated so it isn't quite a note."""
    n = ns(dur)
    t = times(n)
    beat = 3.0 + 4.0 * r.random()
    x = np.sin(2 * np.pi * f0 * t) + 0.8 * np.sin(2 * np.pi * (f0 + beat) * t + 2 * np.pi * r.random())
    x += 0.25 * np.sin(2 * np.pi * f0 * 2.76 * t) * np.exp(-E.LN1000 * t / (dur * 0.4))
    x = _cold(x * E.perc(dur, 0.012, dur * 0.8), r, 41.0, 0.3)
    return normalize(x, 0.0)


@cue('Creature.Unpaid.Hit', variations=5, att='Creature', jitter=0.06, conc=6, level=-10.0)
def unpaid_hit(v, r):
    # A bullet through something not quite there: a cold, hollow thump (the body is an empty shape), a puff of grave
    # dust and a tear of old cloth, and a faint ring hanging after it, not of this world. Nothing meaty.
    f0 = [150.0, 170.0, 135.0, 185.0, 160.0][v] * jitter(r, 1, 0.04)
    hollow = _cold(_hollow(child(r, 'hollow'), f0, 0.12), child(r, 'hc'), 37.0, 0.25)
    thump = kit.thump(110.0 * jitter(r, 1, 0.1), 55.0, 0.08, 0.05)
    n = ns(0.14)
    dust = N.pink(n, child(r, 'dust'), lo=300.0, hi=2600.0) * kit.turbulence(n, child(r, 'dt'), 140.0, 0.7)
    dust = normalize(dust * E.perc(0.14, 0.004, 0.1))
    tear = kit.creak(0.07, child(r, 'tear'), 700.0, 350.0, (750.0, 1500.0, 2600.0), 2.5, 0.4)
    ring = _ghost_ring(f0 * 4.5 * jitter(r, 1, 0.05), 0.6, child(r, 'ring'))
    x = layers((normalize(hollow), 0.0, 0.0), (thump, 0.0, -4.0), (dust, 0.002, -5.0), (tear, 0.004, -11.0),
               (ring, 0.005, -18.0))
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.hp(70.0), extend=False)
    return _haunt(x, r, -17.0, 0.8, 2800.0)


@cue('Creature.Unpaid.Alert', variations=3, att='Creature', jitter=0.04, conc=3, level=-6.0, swell=True)
def unpaid_alert(v, r):
    # It has seen you: a low breath drawn in, then a wail of many voices rising and falling, cold and far-carrying.
    f = [250.0, 228.0, 275.0][v] * jitter(r, 1, 0.03)
    dur = 1.7
    wail = _choir(child(r, 'wail'), [(0.0, f * 0.9), (0.5, f * 1.19), (1.1, f * 1.12), (dur, f * 0.84)], dur,
                  [(0.0, 'u'), (0.45, 'o'), (0.9, 'a'), (dur, 'o')])
    wail = _cold(wail, child(r, 'cold')) * E.swell(wail.size / SR, 0.5, 1.4, 1.6)
    draw = V.whisper(0.45, [(0.0, 'h'), (0.45, 'u')], child(r, 'draw'), 0.9) * E.swell(0.45, 0.4, 0.1, 2.0)
    air = V.whisper(dur, [(0.0, 'h'), (0.8, 'a'), (dur, 'h')], child(r, 'air'), 1.1) * E.swell(dur, 0.7, 1.2)
    # The cold breath around the voices, high and thin.
    frost = F.filt(N.pink(ns(dur), child(r, 'frost'), hi=9000.0), F.hp(1800.0), extend=False)
    frost *= E.swell(dur, 0.8, 1.0) * (0.6 + 0.4 * N.smooth_random(ns(dur), child(r, 'fa'), 3.0))
    x = layers((normalize(draw), 0.0, -10.0), (wail, 0.25, 0.0), (normalize(air), 0.25, -12.0), (normalize(frost), 0.25, -22.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -7.0)


@cue('Creature.Unpaid.Shriek', variations=3, att='Creature', jitter=0.04, conc=2, level=-4.0)
def unpaid_shriek(v, r):
    # The shriek: a raw scream snapping up, wavering, torn at the edges, a cold buzz in it.
    f = [640.0, 700.0, 600.0][v] * jitter(r, 1, 0.03)
    dur = 1.05
    scream = _choir(child(r, 'scream'), [(0.0, f * 0.75), (0.1, f * 1.6), (0.5, f * 1.45), (dur, f * 1.05)], dur,
                    [(0.0, 'a'), (0.25, 'ae'), (0.6, 'e'), (dur, 'i')], 2, 30.0, 0.012, 0.35, 1.25, (7.5, 0.5), 0.45, 4.0)
    scream = D.drive(scream, 14.0, 'tanh', bias=0.15)
    scream = _cold(scream, child(r, 'cold'), 85.0, 0.3) * E.perc(scream.size / SR, 0.02, 0.9)
    n = ns(dur)
    rasp_ = N.white(n, child(r, 'rasp')) * kit.turbulence(n, child(r, 'rt'), 160.0, 0.8)
    rasp_ = F.sweep(rasp_, 'bp', fit_curve(O.glide([(0.0, f * 2.2), (0.1, f * 4.5), (dur, f * 3.0)], n=n)), 2.5) * E.perc(dur, 0.02, 0.7)
    x = layers((scream, 0.0, 0.0), (normalize(rasp_), 0.0, -11.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.7), F.hp(200.0), extend=False)
    return _haunt(x, r, -9.0, 2.0)


def fit_curve(c):
    return np.clip(c, 60.0, 16000.0)


@cue('Creature.Unpaid.Lunge', variations=3, att='Creature', jitter=0.05, conc=3, level=-7.0, swell=True)
def unpaid_lunge(v, r):
    # It rushes through the air at you: a cold whoosh swelling past, a hissing breath out, a low rumble under it.
    n = ns(0.75)
    rush = N.pink(n, child(r, 'rush'))
    fc = O.glide([(0.0, 450.0), (0.32, 2600.0), (0.75, 600.0)], n=n)
    rush = F.sweep(rush, 'bp', fc, 1.4) * E.swell(0.75, 0.33, 0.5, 2.0)
    breath = V.whisper(0.5, [(0.0, 'h'), (0.15, 'a'), (0.5, 'h')], child(r, 'breath'), 1.05) * E.perc(0.5, 0.03, 0.45)
    rumble = kit.noise_thump(0.6, child(r, 'rumble'), 400.0, 60.0, 0.4, 0.9, 0.3)
    f = 300.0 * jitter(r, 1, 0.08)
    cry = _choir(child(r, 'cry'), [(0.0, f), (0.3, f * 0.8)], 0.3, [(0.0, 'a'), (0.3, 'h')], 2, 20.0, 0.02, 0.7)
    cry = cry * E.perc(cry.size / SR, 0.02, 0.28)
    x = layers((normalize(rush), 0.0, 0.0), (normalize(breath), 0.18, -4.0), (rumble, 0.1, -10.0), (cry, 0.2, -12.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), extend=False)
    return _haunt(x, r, -10.0, 1.6)


@cue('Creature.Unpaid.Hurt', variations=4, att='Creature', jitter=0.05, conc=3, level=-8.0)
def unpaid_hurt(v, r):
    # A pained cry cut short, flickering as if the voice can't hold together.
    f = 360.0 * jitter(r, 1, 0.1)
    dur = 0.45
    cry = _choir(child(r, 'cry'), [(0.0, f * 1.1), (0.05, f * 1.2), (dur, f * 0.68)], dur, [(0.0, 'a'), (dur, 'u')], 2,
                 18.0, 0.015, 0.5, 1.1, (6.0, 0.3), 0.3)
    n = cry.size
    flicker = 0.5 + 0.5 * np.sign(np.sin(2 * np.pi * O.phase(30.0 * (1 + 0.3 * N.smooth_random(n, r, 6.0)), n)))
    flicker = F.filt(flicker, F.lp1(400.0), extend=False)
    cry = cry * (0.55 + 0.45 * flicker) * E.perc(n / SR, 0.01, 0.4)
    x = F.filt(cry, F.peak(3200.0, -3.0, 0.8), F.hp(150.0), extend=False)
    return _haunt(x, r, -10.0, 1.4)


@cue('Creature.Unpaid.Death', variations=3, att='Creature', jitter=0.04, conc=3, level=-6.0, swell=True)
def unpaid_death(v, r):
    # Laid to rest at last: a long wail sinking as its voices drift apart, turning to breath, the coal in its chest
    # crackling out, and a release of air rising away.
    f = [330.0, 300.0, 350.0][v] * jitter(r, 1, 0.03)
    dur = 2.4
    wail = _choir(child(r, 'wail'), [(0.0, f), (0.15, f * 1.12), (dur, f * 0.32)], dur, [(0.0, 'a'), (1.0, 'o'), (dur, 'u')],
                  3, 30.0, 0.05, 0.6, 1.1, (4.5, 0.35), 0.15)
    wail *= E.swell(wail.size / SR, 0.12, 2.2)
    breath = V.whisper(dur, [(0.0, 'a'), (1.2, 'u'), (dur, 'h')], child(r, 'breath'), 1.0) * E.swell(dur, 1.4, 1.2)
    n = ns(1.6)
    ember = G.grit(1.6, child(r, 'ember'), 220.0 * np.exp(-times(n) / 0.6), 1800.0, 6000.0, 3.0, 3)
    ember = F.filt(ember, F.lp(5000.0), extend=False)
    release = kit.whoosh(1.0, child(r, 'release'), 400.0, 5000.0, 1.1, 0.4, -2.0, 0.8)
    x = layers((wail, 0.0, 0.0), (normalize(breath), 0.3, -9.0), (normalize(ember), 0.1, -20.0), (release, 1.5, -14.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -6.0, 3.0)


# --- Death bursts (the body as it dies, under its kind's death cry; bigger kin play it lower) -----------------------

@cue('Creature.Spider.Burst', variations=4, att='Creature', jitter=0.06, conc=4, level=-9.0)
def spider_burst(v, r):
    # The shell gives way: a sharp, crunchy snap of chitin splitting open, the plates cracking and crunching apart,
    # the body's dull thump, and a wet squelch of what was inside with a few drips.
    snap = normalize(M.strike(M.parts(1500.0, 6500.0, child(r, 'snap'), 6, 0.01, 0.004), M.hammer(0.00005), 0.03))
    shell = _chitin(child(r, 'shell'), 520.0 * jitter(r, 1, 0.12), 0.04, 4 + int(r.integers(0, 3)))
    thump = kit.noise_thump(0.14, child(r, 'thump'), 750.0, 85.0, 0.07, 1.0, 0.05)
    squelch = kit.squelch(0.22, child(r, 'squelch'), 1200.0 * jitter(r, 1, 0.1), 280.0, 2.5)
    slap = _wet_slap(0.06, child(r, 'slap'), 250.0, 2000.0, 0.03)
    drips = _bubbles(0.3, child(r, 'drips'), lambda t: 30.0 * np.exp(-t / 0.15), 600.0, 1500.0, 0.06, 0.3)
    x = layers((snap, 0.0, -5.0), (shell, 0.0, 0.0), (thump, 0.0, -3.0), (squelch, 0.012, -4.0), (slap, 0.01, -8.0),
               (drips, 0.0, -15.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -3.0), extend=False)
    return _outside(x, r, -21.0)


def _blorp(f0, f1, dur, r, t60=None):
    """A blob's cavity tearing open as it bursts: a gloopy tone sinking as the hole widens (the bloop turned round),
    with its upper harmonics, the thick walls' flutter and wet noise through a resonance that sinks with it, so it's a
    gulp of jelly rather than a tone. Starts soft, under the splash, never as the hit itself."""
    n = ns(dur)
    f = O.expsweep(f0, f1, dur, 0.7, n=n)
    ph = 2.0 * np.pi * O.phase(f, n)
    x = (np.sin(ph) + 0.3 * np.sin(2.0 * ph + 0.4) + 0.12 * np.sin(3.0 * ph + 1.1)) * (0.7 + 0.3 * N.smooth_random(n, r, 70.0))
    wet = N.pink(n, child(r, 'wet')) * kit.turbulence(n, child(r, 'wt'), 200.0, 0.8)
    wet = F.sweep(wet, 'bp', np.minimum(f * 2.5, 12000.0), 4.0)
    return normalize((normalize(x) + 0.4 * normalize(wet)) * E.perc(dur, 0.006, t60 or dur * 0.8), 0.0)


@cue('Creature.Slime.Splat', variations=4, att='Creature', jitter=0.06, conc=4, level=-9.0)
def slime_splat(v, r):
    # The blob bursts: a wet splash as the jelly flies apart and squelches, a low blorp sinking as it tears open, the
    # gel squishing through itself, then drips pattering back into the puddle. All jelly.
    splash = layers(_wet_slap(0.09, child(r, 'slap'), 150.0, 3200.0, 0.05, 0.0008),
                    (kit.squelch(0.18, child(r, 'squelch'), 1500.0 * jitter(r, 1, 0.1), 260.0, 2.5), 0.003, -2.0))
    weight = kit.noise_thump(0.2, child(r, 'weight'), 900.0, 80.0, 0.1, 1.0, 0.06)
    blorp = _blorp(300.0 * jitter(r, 1, 0.1), 105.0, 0.18, child(r, 'blorp'))
    squish = _squish(0.2, child(r, 'squish'), lambda t: 450.0 * np.exp(-t / 0.08), 500.0, 2000.0)
    drips = np.zeros(1)
    dr = child(r, 'drips')
    t = 0.14
    for i in range(5 + int(dr.integers(0, 3))):
        t += dr.uniform(0.03, 0.08)
        drips = layers(drips, (kit.bubble(dr.uniform(700.0, 1500.0), dr, 0.1), t, -2.5 * i + dr.uniform(-3.0, 0.0)))
    x = layers((splash, 0.0, 0.0), (weight, 0.0, -4.0), (blorp, 0.01, -3.0), (squish, 0.005, -9.0), (normalize(drips), 0.0, -15.0))
    return _outside(F.filt(x, F.lp(3500.0, 0.7), extend=False), r, -22.0)


# A singing bowl's modes against its lowest (measured on bowls: roughly 1 : 2.7 : 5.1 : 8.2), and their strengths.
BOWL = [(1.0, 1.0), (2.71, 0.55), (5.12, 0.28), (8.21, 0.14)]


def _bowl(f0, dur, r, bend=0.25, beat=3.0):
    """A glassy singing bowl: its few clean modes, each a slowly beating pair, sliding up together by bend as the
    light goes; struck softly, then swelling a little as if bowed, and dying away."""
    n = ns(dur)
    t = times(n)
    glide = 1.0 + bend * smoothstep(t / dur)
    out = np.zeros(n)
    for i, (ratio, amp) in enumerate(BOWL):
        for s in (-0.5, 0.5):
            f = f0 * ratio * glide + s * beat * (1.0 + 0.4 * i) * r.uniform(0.7, 1.3)
            out += 0.5 * amp * np.sin(2.0 * np.pi * O.phase(f, n) + 2.0 * np.pi * r.random()) \
                * np.exp(-E.LN1000 * t / (dur * (1.3 - 0.2 * i)))
    env = E.bp([(0.0, 0.0), (0.005, 0.7), (0.45 * dur, 1.0), (dur, 0.0)], n=n, curve='cos')
    return normalize(out * env, 0.0)


@cue('Creature.Unpaid.Dissolve', variations=3, att='Creature', jitter=0.04, conc=3, level=-10.0, swell=True)
def unpaid_dissolve(v, r):
    # Its body lets go of the soul-light: a rising, airy whoosh, a glassy singing-bowl tone bending upward as the light
    # leaves, cold, and the faint crackle of the coal going out.
    dur = 1.2
    f0 = [560.0, 620.0, 520.0][v] * jitter(r, 1, 0.02)
    bowl = _cold(_bowl(f0, dur, child(r, 'bowl')), child(r, 'cold'), 23.0, 0.15)
    air = V.whisper(dur, [(0.0, 'h'), (0.6, 'u'), (dur, 'h')], child(r, 'air'), 1.2) * E.swell(dur, 0.7, 0.5, 1.6)
    rush = kit.whoosh(dur, child(r, 'rush'), 300.0, 4500.0, 1.0, 0.65, -2.0, 0.9)
    n = ns(1.0)
    ember = G.grit(1.0, child(r, 'ember'), 160.0 * np.exp(-times(n) / 0.4), 1800.0, 6000.0, 3.0, 3)
    ember = F.filt(ember, F.lp(5000.0), extend=False)
    x = layers((bowl, 0.0, 0.0), (normalize(air), 0.0, -6.0), (rush, 0.0, -8.0), (normalize(ember), 0.02, -22.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -12.0, 1.6, 3200.0)


# --- Ranks and ambushes (UCreaturePackComponent's rank sting; AAmbushSpawner's entrances) ---------------------------

@cue('Creature.RankSting', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def rank_sting(v, r):
    # "A high rank appears": something stronger than the rest has turned on you. One low, muffled drum and a dark iron
    # bell struck together (the bell's minor hum hanging, its top dulled so it booms rather than rings), and a cold
    # swell rising out of them, a sheet of iron bowed up into its modes with a breath drawn in under it, left unresolved
    # and gone in a little over a second. Short, dark and dry, so it can come often without tiring. (The score's
    # Music.Sting.Elite may land with it for the highest ranks: this one keeps off its strings.)
    from recipes import band as B
    drum = B.big_drum(child(r, 'drum'), 1.0, f0=[46.0, 42.0][v], t60=0.75)
    drum = F.filt(drum, F.lp(1600.0, 0.7), extend=False)
    prime = [196.0, 185.0][v]
    bell = M.strike(M.bell(prime, child(r, 'bell'), 2.4, 0.9, 0.55), M.hammer(0.0007), 1.6)
    bell = F.filt(normalize(bell), F.lp(2600.0, 0.6), F.peak(3000.0, -4.0, 1.0), extend=False)
    rise = 1.0
    n = ns(rise)
    plate = M.plate(140.0, child(r, 'plate'), 36, 1.4, 0.4, tilt=0.45)
    bow = N.pink(n, child(r, 'bow')) * np.linspace(0.0, 1.0, n) ** 2.0
    swell = M.strike(plate, bow / np.sqrt(np.sum(bow * bow)), rise)[:n]
    swell = F.sweep(swell, 'lp', O.expsweep(400.0, 4200.0, rise, 0.8, n=n), 0.9)
    env = E.bp([(0.0, 0.0), (0.82, 1.0), (rise, 0.0)], n=n, curve='cos')
    swell = normalize(F.filt(swell, F.hp(250.0, 0.7), extend=False) * env)
    draw = V.whisper(rise, [(0.0, 'h'), (0.5, 'u'), (rise, 'o')], child(r, 'draw'), 0.9) * env
    draw = normalize(F.filt(draw, F.hp(300.0), F.lp(5000.0), extend=False))
    # The drum's skin slapped as well, so the hit has an edge on small speakers, not only weight.
    skin = B.frame_drum(child(r, 'skin'), 0.9)
    hit = layers((drum, 0.0, 0.0), (bell, 0.0, -4.0), (skin, 0.0, -11.0))
    x = layers(S.widen(hit, child(r, 'wh'), 0.15), (S.widen(swell, child(r, 'ws'), 0.7), 0.06, -7.0),
               (S.widen(draw, child(r, 'wd'), 0.5), 0.06, -15.0))
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.4, lp=3000.0)
    wet = F.convolve(F.filt(mono(x), F.hp(200.0), extend=False), ir)
    x = layers(x, kit.set_level(wet, x, -14.0, 0.05))
    k = min(x.shape[-1], ns(1.45))
    return x[..., :k] * E.bp([(0.0, 1.0), (1.1, 1.0), (1.45, 0.0)], n=k, curve='cos')


@cue('Creature.Unpaid.Rise', variations=3, att='Creature', jitter=0.04, conc=3, level=-9.0)
def unpaid_rise(v, r):
    # The dead come up out of the ground: the earth heaves and splits (a low shove of soil, clods breaking, dirt pouring
    # off), a long, breathy, ghostly inhale as it draws itself up out of the grave with a faint many-voiced moan in it,
    # and the coal in its chest catching with a soft crackle.
    dur = 1.5
    n = ns(dur)
    heave = kit.noise_thump(0.7, child(r, 'heave'), 420.0, 45.0, 0.45, 1.1, 0.35)
    soil = N.shaped(n, child(r, 'soil'), -6.0, 35.0, 400.0)
    soil *= (0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'sg'), 9.0)))
    soil *= E.bp([(0.0, 0.0), (0.03, 1.0), (0.35, 0.4), (1.0, 0.0), (dur, 0.0)], n=n, curve='cos')
    crack = kit.grit_burst(0.35, child(r, 'crack'), 6000.0, 350.0, 2500.0, 0.2, 0.004)
    clods = np.zeros(1)
    for i in range(4 + int(r.integers(0, 3))):
        clod = kit.noise_thump(0.09, child(r, 'clod', i), 900.0, 140.0, 0.04, 1.0, 0.03)
        clods = layers(clods, (clod, r.uniform(0.03, 0.7), r.uniform(-10.0, -2.0)))
    m = ns(1.1)
    pour = G.grit(1.1, child(r, 'pour'), 2500.0 * np.exp(-times(m) / 0.45), 600.0, 3500.0, 2.0, 4)
    d = 1.15
    inhale = V.whisper(d, [(0.0, 'h'), (0.4, 'u'), (0.85, 'o'), (d, 'a')], child(r, 'inhale'), 0.95)
    swell = E.bp([(0.0, 0.0), (0.95, 1.0), (d, 0.0)], n=ns(d), curve='cos')
    inhale = normalize(F.filt(inhale * swell, F.hp(250.0), F.lp(6000.0), extend=False))
    f = [190.0, 175.0, 205.0][v] * jitter(r, 1, 0.03)
    moan = _choir(child(r, 'moan'), [(0.0, f * 0.85), (1.0, f * 1.08)], 1.1, [(0.0, 'u'), (1.1, 'o')], 3, 18.0, 0.03, 0.8,
                  1.05, (4.5, 0.25))
    moan = _cold(moan, child(r, 'cold')) * E.bp([(0.0, 0.0), (0.9, 1.0), (moan.size / SR, 0.0)], n=moan.size, curve='cos')
    k = ns(1.0)
    ember = G.grit(1.0, child(r, 'ember'), 30.0 + 140.0 * times(k), 1800.0, 6000.0, 3.0, 3)
    ember = F.filt(ember, F.lp(5000.0), extend=False) * E.ar(1.0, 0.3, 0.3)
    x = layers((heave, 0.0, 0.0), (normalize(soil), 0.0, -12.0), (crack, 0.005, -8.0), (normalize(clods), 0.0, -5.0),
               (normalize(pour), 0.05, -10.0), (inhale, 0.22, 0.0), (normalize(moan), 0.3, -9.0),
               (normalize(ember), 0.55, -18.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(40.0), extend=False)
    return _haunt(x, r, -13.0, 1.3, 3000.0)


@cue('Creature.Spider.Drop', variations=3, att='Creature', jitter=0.05, conc=3, level=-9.0)
def spider_drop(v, r):
    # A spider dropping on its silk out of the dark overhead: the thread paying out in a quick zip (silk running through
    # its spinnerets, a stick-slip whine falling as it slows), the air of the fall, then a dry, leggy landing (eight feet
    # striking in a scatter of ticks, the body's light thump, the shell knocking) and a skitter as it gathers itself.
    zd = 0.27 + 0.05 * r.random()
    n = ns(zd)
    zip_ = kit.creak(zd, child(r, 'zip'), 520.0, 200.0, (1300.0, 2300.0, 3600.0), 4.0, 0.15)
    zip_ = F.filt(zip_ * E.ar(zd, 0.008, 0.1, 1.5), F.peak(3000.0, -4.0, 0.9), extend=False)
    thread = N.band(n, child(r, 'thread'), 1500.0, 7000.0) * kit.turbulence(n, child(r, 'tt'), 300.0, 0.6)
    thread = normalize(F.sweep(thread, 'lp', O.expsweep(8000.0, 2500.0, zd, 1.0, n=n), 0.7) * E.ar(zd, 0.008, 0.1, 1.5))
    fall = kit.whoosh(zd, child(r, 'fall'), 280.0, 900.0, 1.0, 0.8, -3.0)
    t = zd + 0.01
    feet = _skitter(child(r, 'feet'), 8, 0.0, 0.0045)
    thump = kit.noise_thump(0.12, child(r, 'thump'), 800.0, 110.0, 0.05, 1.0, 0.04)
    knock = kit.thunk(620.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.03, 0.0003, 6, 0.12)
    dust = kit.grit_burst(0.12, child(r, 'dust'), 3000.0, 1000.0, 4500.0, 0.06)
    after = _skitter(child(r, 'after'), 4 + int(r.integers(0, 3)), 0.0, 0.035)
    ch = chitter(0.16, child(r, 'chitter'), 30.0, 20.0)
    x = layers((zip_, 0.0, -6.0), (thread, 0.0, -11.0), (fall, 0.0, -14.0), (feet, t, -3.0), (thump, t, 0.0),
               (knock, t + 0.002, -6.0), (dust, t, -14.0), (after, t + 0.1, -10.0), (ch, t + 0.12, -15.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r, -22.0)
