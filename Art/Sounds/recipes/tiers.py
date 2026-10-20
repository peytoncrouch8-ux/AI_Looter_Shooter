"""Enemy tiers and their traits (all 3D, heard at the creature): the stings and bursts that tell Hallowed and Ethereal
enemies from the rest, and the sound of each trait's moment. In the family of the Unpaid's voices, the bell-ringer's bell
and the ember powers: formant choirs, bronze, glass, flame, breath, with the same wash and cold ring.

Hallowed is saint-gold: major, bright, bells and a choir, but wrong (a cracked bell, a semitone in the chord). Ethereal is
ghostly red: low and cold, a minor second, embers and heat, a wail.

- Creature.Tier.Notice.Hallowed (1.2 s): it notices you: a cracked bell struck once (its pairs split wide, a sour stray a
  semitone off, the crack's buzz), a choir breathing in a chord with a semitone clash and cut short, a gold glint.
- Creature.Tier.Notice.Ethereal (1.9 s): a low swell of beating tones with a minor second in it rising and falling, a
  red-hot whisper (breath with a sizzle) and a thin wail sinking, an ember's snap at the start and a cold ring hanging.
- Creature.Tier.Death.Hallowed (2.8 s): the soul breaking free: a heavy thump, crack and a bell's crash, then the pillar of
  light: a choir swelling, bowls of glass a fifth apart climbing on each other, a rush up, bright notes climbing and sparkle.
- Creature.Tier.Death.Ethereal (3.9 s): the same, deeper and hotter: a deeper thump, a hot crack, flame roaring up, a cracked
  iron toll with a tritone in it, a many-voiced wail, cold bowls bending up, a rush and a shriek-tone climbing the pillar.
- Loot.Tier.Pop.Hallowed / .Ethereal (0.8 s, 4 takes each): one piece thrown out of the shower, a step above the boss
  loot's pop: the hollow pop, a rush up, a glint with a round body and a bell's hum under it (Ethereal: lower, with embers,
  a cold ring and a short ghost "ah"). Each take its own notes: the pieces come out one after another.
- Creature.Trait.WardBreak (0.6 s): the soul ward shattering: a crack and a pop of air, a glass ring bending down as it
  breaks, a shower of shards tinkling (thick at first, thinning), a breath of the soul let out.
- Creature.Trait.WardHit (0.25 s, 5 takes): a shot on the soul ward's cage: a small glass ping (soft mallet, low upper
  partials), a round body, a faint cage knock and cold shimmer; a different pentatonic note per take. Frequent, quiet.
- Creature.Trait.Swell (1 s): Grave-burst's warning: pressure building in the body: a tone climbing, a heartbeat running
  faster, the body straining, a whistle of pressure, all growing about 17 dB, then the world holds its breath (a hush over
  the last seventh).
- Creature.Trait.Burst (0.9 s): the blast: crack, push of air and a heave, the shards and clods, dirt and dust raining
  back, something wet, a ghost's cry going out in it. Through a soft clip and a limiter like the bosses' big hits.
- Creature.Trait.Phase (0.7 s): a blink away: a whoosh run backwards into a snap of air, then glassy shimmer.
- Creature.Trait.Enrage (1 s): Vengeful's flare: a flame catching (crack, whoomph, flare, embers) with a rough snarl.
- Creature.Trait.Mend (loop, 2.4 s): a soft mending shimmer: ten glass notes climbing a major pentatonic, ringing on over
  each other, over a warm pad and fine air. Notes are wrapped round the loop and the pad is silent at the seam, so it loops
  without a join and, played once, breathes in and out. Quiet.
- Creature.Trait.Rally (0.7 s): an aura pulse: a soft low drum and frame drum, a warm low fifth swelling with a short low
  "oh" of two voices in it, and a ring of air going out. Low and soft-edged: it repeats.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, reverse
from lib import noise as N, env as E, osc as O, filters as F, dist as D, dynamics as Y, modal as M, reverb as R
from lib import granular as G, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C
from recipes import band as B
from recipes.bells import handbell, ring
from recipes.loot import _sparkle, _glint
from recipes.bosses import _flame, _crackle, _pressure


# --- Shared pieces -----------------------------------------------------------------------------------------------------

def _gold(x, r, rel_db=-13.0, t60=1.5, lp=4600.0):
    """Hallowed's space: the open air with its top left on, so gold rings where the Unpaid's wash is grey."""
    ir = R.open_air(child(r, 'gold'), t60=t60, lp=lp, predelay=0.015, density=0.9)
    wet = F.convolve(F.filt(x, F.hp(220.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.05))


def _dense(x, clip_db=2.0, push_db=0.5):
    """The big hits' last stage (as the bosses'): a soft clip and a limiter, so they stand beside the rifle."""
    x = D.drive(normalize(x), clip_db, 'tanh')
    return Y.limit(normalize(x, 0.0) * db2a(push_db), -1.0, 0.002, 0.06)


def _limit(x):
    """Only the limiter: a long, swelling cue keeps its shape, nothing is squashed flat under its first hit."""
    return Y.limit(normalize(x, 0.0), -1.0, 0.002, 0.08)


def _drone(freqs, dur, r, beat=1.5, harmonics=(1.0, 0.45, 0.15)):
    """Low tones, each a pair a little apart so it breathes, with a few harmonics so small speakers hear it too."""
    n = ns(dur)
    t = times(n)
    x = np.zeros(n)
    for i, f in enumerate(freqs):
        for s in (-0.5, 0.5):
            ph = 2.0 * np.pi * r.random()
            for k, a in enumerate(harmonics, 1):
                x += (a / (1.0 + 0.3 * i)) * np.sin(2.0 * np.pi * (f + s * beat) * k * t + ph * k)
    return normalize(x, 0.0)


def _cracked(prime, r, t60, dur, crack, buzz_db=-13.0, strays=(), bright=0.9):
    """A cracked hand bell struck once: its pairs split wide and beating, sour strays, and a buzz riding the ring's level."""
    m = handbell(prime, child(r, 'bell'), t60=t60, beat=1.2, bright=bright, crack=crack)
    if strays:
        m = M.merge(m, M.modes([prime * s for s in strays], [t60 * 0.5] * len(strays), [0.4] * len(strays)))
    rung = ring(m, dur, child(r, 'ring'), 0.0004, -18.0)
    level = np.clip(F.filt(np.abs(rung), F.lp(30.0), extend=False), 0.0, None)
    level /= max(float(np.max(level)), 1e-9)
    gate = 0.5 + 0.5 * np.tanh(6.0 * np.sin(2.0 * np.pi * (70.0 + 30.0 * r.random()) * times(rung.size) + 2.0 * np.pi * r.random()))
    buzz = N.band(rung.size, child(r, 'buzz'), 900.0, 3600.0) * gate * level ** 1.5
    return layers((rung, 0.0, 0.0), (normalize(buzz), 0.0, buzz_db))


def _lv(sig, ref, rel_db, win=0.05):
    return kit.set_level(sig, ref, rel_db, win)


def _env(x, points):
    """x shaped by a breakpoint curve over its own length (points are (seconds, level))."""
    return x * E.bp(points, n=x.size, curve='cos')


# --- Notice -----------------------------------------------------------------------------------------------------------

@cue('Creature.Tier.Notice.Hallowed', variations=3, att='Creature', jitter=0.04, conc=2, level=-8.0)
def notice_hallowed(v, r):
    # It has seen you, and the holy is wrong in it: a cracked bell struck once (a sour stray a semitone off its strike
    # note, a buzz in the ring), a choir drawing a chord in with a semitone clashing inside it and cutting it short, and
    # a gold glint over it all. The bell leads; the choir sits under its ring.
    prime = (392.0, 370.0, 415.3)[v] * jitter(r, 1, 0.01)
    bell = _cracked(prime, r, 1.1, 1.3, 0.5, -12.0, strays=(1.06, 3.17), bright=1.05)
    f = prime * 0.5
    dur = 0.85
    keys = [(0.0, 'o'), (0.4, 'a'), (dur, 'u')]
    chord = layers(C._choir(child(r, 'ca'), [(0.0, f), (0.5, f * 1.03)], dur, keys, 3, 18.0, 0.03, 0.7, 1.0, (5.0, 0.25)),
                   (C._choir(child(r, 'cb'), [(0.0, f * 1.0595), (0.5, f * 1.09)], dur, keys, 2, 18.0, 0.03, 0.7, 1.0, (5.0, 0.25)), 0.0, -4.0),
                   (C._choir(child(r, 'cc'), [(0.0, f * 1.5), (0.5, f * 1.55)], dur, keys, 2, 18.0, 0.03, 0.7, 1.0, (5.0, 0.25)), 0.0, -6.0))
    breath_env = [(0.0, 0.0), (0.42, 1.0), (0.58, 0.85), (0.8, 0.0), (dur + 0.2, 0.0)]
    chord = normalize(_env(chord, breath_env))
    breath = V.whisper(dur, [(0.0, 'h'), (0.4, 'a'), (dur, 'h')], child(r, 'breath'), 1.0)
    breath = normalize(F.filt(_env(breath, breath_env), F.hp(300.0), F.lp(6000.0), extend=False))
    glint = kit.chime(kit.note('E6') * jitter(r, 1, 0.01), child(r, 'glint'), 0.6, 0.6, 0.0002, 0.5, -30.0)
    x = layers((bell, 0.0, 0.0), (_lv(chord, bell, -12.0), 0.06, 0.0), (_lv(breath, bell, -16.0), 0.06, 0.0),
               (_lv(glint, bell, -13.0), 0.0, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return _gold(x, r, -12.0, 1.1)


@cue('Creature.Tier.Notice.Ethereal', variations=3, att='Creature', jitter=0.04, conc=2, level=-7.5)
def notice_ethereal(v, r):
    # It has seen you, colder than the rest and burning: a low swell of tones beating against each other with a minor
    # second in it (rising and falling), a red-hot whisper (breath with a sizzle) and a thin wail sinking over it, an
    # ember's snap at the start to catch the ear, and a cold ring left hanging.
    f = (55.0, 49.0, 58.3)[v] * jitter(r, 1, 0.01)
    dur = 1.15
    swell = _drone([f, f * 1.5, f * 2.0, f * 1.0595], dur, child(r, 'swell'), 1.6, (1.0, 0.8, 0.4))
    swell = normalize(_env(swell, [(0.0, 0.0), (0.5, 1.0), (0.8, 0.6), (dur, 0.0)]))
    d = 0.8
    whisper = V.whisper(d, [(0.0, 'h'), (0.3, 'i'), (d, 'o')], child(r, 'whisper'), 1.15, 2.5)
    whisper = normalize(F.filt(_env(whisper, [(0.0, 0.0), (0.28, 0.9), (0.45, 1.0), (d, 0.0)]), F.hp(500.0), F.lp(6500.0),
                               extend=False))
    m = ns(0.8)
    sizzle = G.grit(0.8, child(r, 'sizzle'), 700.0 * np.exp(-times(m) / 0.3) + 100.0, 2200.0, 6500.0, 2.0, 4)
    sizzle = _env(F.filt(sizzle, F.lp(6000.0), extend=False), [(0.0, 0.0), (0.1, 1.0), (0.45, 0.8), (0.8, 0.0)])
    wf = 640.0 * jitter(r, 1, 0.03)
    wail = C._choir(child(r, 'wail'), [(0.0, wf), (0.5, wf * 0.95), (1.0, wf * 0.74)], 1.0, [(0.0, 'i'), (0.5, 'u'), (1.0, 'o')], 2,
                    35.0, 0.04, 0.85, 1.25, (6.0, 0.4))
    wail = _env(C._cold(wail, child(r, 'cold'), 41.0, 0.35), [(0.0, 0.0), (0.25, 0.7), (0.5, 1.0), (1.0, 0.0)])
    ghost = C._ghost_ring(1480.0 * jitter(r, 1, 0.04), 1.0, child(r, 'ghost'))
    body = layers((swell, 0.0, 0.0), (_lv(whisper, swell, -2.0), 0.06, 0.0), (_lv(normalize(sizzle), swell, -10.0), 0.1, 0.0),
                  (_lv(wail, swell, -6.0), 0.15, 0.0), (_lv(ghost, swell, -14.0), 0.02, 0.0))
    snap = layers(kit.burst(0.012, child(r, 'snap'), 0.005, lo=1800.0, hi=8000.0, attack=0.0002),
                  (normalize(M.strike(M.parts(1200.0, 3200.0, child(r, 'tick'), 4, 0.02, 0.01, 0.4), M.hammer(0.0001), 0.05)), 0.0, -6.0))
    x = layers((body, 0.0, 0.0), (_lv(snap, body, -3.0, 0.005), 0.0, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(40.0), extend=False)
    return C._haunt(x, r, -10.0, 1.2, 2800.0)


# --- Death (the soul breaking free) -------------------------------------------------------------------------------------

@cue('Creature.Tier.Death.Hallowed', variations=3, att='Creature', jitter=0.03, conc=2, level=-5.5)
def death_hallowed(v, r):
    # The soul breaks free with the heavier burst: a thump, a crack and a bell's crash, and then the pillar of light
    # builds over them: a choir swelling in a chord, bowls of glass a fifth apart climbing on each other, a rush upward,
    # bright notes climbing and a sparkle of chimes.
    root = (329.6, 293.7, 370.0)[v] * jitter(r, 1, 0.005)
    weight = layers((kit.thump(66.0, 34.0, 0.8, 0.36, attack=0.002, drive_db=2.0), 0.0, 0.0),
                    (kit.noise_thump(0.6, child(r, 'air'), 650.0, 60.0, 0.28, 1.1, 0.12, 0.002), 0.0, -3.0))
    crack = kit.burst(0.02, child(r, 'crack'), 0.007, lo=900.0, hi=7500.0, attack=0.0002)
    crash = normalize(M.strike(M.bell(root, child(r, 'bell'), 2.2, 0.9, 0.9), M.hammer(0.0004), 1.9))
    crash = F.filt(crash, F.lp(3800.0, 0.7), extend=False)
    bowls = layers((C._bowl(root * 2.0, 1.8, child(r, 'b1'), 0.10, 2.0), 0.0, 0.0),
                   (C._bowl(root * 3.0, 1.8, child(r, 'b2'), 0.12, 2.4), 0.12, 1.0),
                   (C._bowl(root * 4.0, 1.6, child(r, 'b3'), 0.14, 2.8), 0.24, -2.0))
    dur = 1.8
    keys = [(0.0, 'o'), (0.7, 'a'), (dur, 'u')]
    choir = layers(*[(C._choir(child(r, 'ch', i), [(0.0, root * k * 0.84), (0.9, root * k)], dur, keys, 2, 14.0, 0.03, 0.55, 1.0,
                               (5.0, 0.25)), 0.0, -1.5 * i) for i, k in enumerate((0.5, 0.75, 1.0))])
    choir = normalize(_env(choir, [(0.0, 0.0), (0.6, 1.0), (1.1, 0.9), (dur + 0.1, 0.0)]))
    rush = kit.whoosh(1.2, child(r, 'rush'), 350.0, 6200.0, 0.9, 0.5, -2.0, 0.8)
    glints = layers(*[(kit.chime(root * 2.0 * k, child(r, 'g', i), 1.0, 0.7, 0.0002, 0.5, -30.0), 0.12 * i, -1.0 * i)
                      for i, k in enumerate((1.0, 1.26, 1.5, 2.0))])
    spark = _sparkle(child(r, 'spark'), 10, 0.0, 1.2)
    x = layers((weight, 0.0, 0.0), (_lv(crack, weight, -6.0, 0.01), 0.0, 0.0), (_lv(crash, weight, -3.0), 0.0, 0.0),
               (_lv(normalize(bowls), weight, -4.0), 0.12, 0.0), (_lv(choir, weight, -5.0), 0.15, 0.0),
               (_lv(rush, weight, -9.0), 0.1, 0.0), (_lv(normalize(glints), weight, -6.0), 0.4, 0.0),
               (_lv(spark, weight, -13.0), 0.5, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(35.0), extend=False)
    return _limit(_gold(x, r, -11.0, 1.9, 4600.0))


@cue('Creature.Tier.Death.Ethereal', variations=3, att='Creature', jitter=0.03, conc=2, level=-4.5)
def death_ethereal(v, r):
    # The soul breaks free, deeper and hotter: a deeper thump, a hot crack, flame roaring up, a cracked iron toll with a
    # tritone in it, then the pillar builds: a many-voiced wail, cold bowls bending up, a dark rush and a shriek-tone
    # climbing it, a cold ring left hanging.
    root = (155.6, 138.6, 174.6)[v] * jitter(r, 1, 0.005)
    weight = layers((kit.thump(54.0, 28.0, 1.0, 0.5, attack=0.002, drive_db=3.0), 0.0, 0.0),
                    (kit.noise_thump(0.8, child(r, 'air'), 700.0, 50.0, 0.36, 1.1, 0.15, 0.002), 0.0, -2.0))
    crack = kit.burst(0.03, child(r, 'crack'), 0.01, lo=700.0, hi=8000.0, attack=0.0002)
    roar = _flame(1.4, child(r, 'flame'), [(0.0, 400.0), (0.35, 1800.0), (1.4, 500.0)], 26.0, 0.7, 1.2)
    roar = _env(roar, [(0.0, 0.0), (0.04, 1.0), (0.4, 0.9), (1.4, 0.0)])
    embers = _crackle(1.8, child(r, 'ember'), 380.0, 0.7, 1500.0, 6500.0, 5500.0)
    toll = F.filt(_cracked(root * 1.5, r, 2.0, 2.2, 0.8, -11.0, strays=(1.414, 2.83), bright=0.8), F.lp(3000.0, 0.7), extend=False)
    wf = root * 2.0
    dur = 2.2
    wail = C._choir(child(r, 'wail'), [(0.0, wf), (0.5, wf * 1.5), (1.2, wf * 1.78), (dur, wf * 1.2)], dur,
                    [(0.0, 'o'), (0.6, 'a'), (1.4, 'i'), (dur, 'u')], 4, 28.0, 0.05, 0.7, 1.05, (5.0, 0.35), 0.35)
    wail = _env(C._cold(wail, child(r, 'cold'), 33.0, 0.3), [(0.0, 0.0), (0.5, 0.7), (1.0, 1.0), (1.7, 0.6), (dur + 0.1, 0.0)])
    cold_bowl = C._cold(C._bowl(root * 4.0, 2.0, child(r, 'bowl'), 0.22, 3.5), child(r, 'cb'), 27.0, 0.25)
    rush = kit.whoosh(1.4, child(r, 'rush'), 200.0, 2600.0, 0.9, 0.55, -2.0, 0.9)
    m = ns(1.5)
    t = times(m)
    shriek_f = O.expsweep(root * 3.0, root * 11.0, 1.5, 0.8, n=m) * (1.0 + 0.012 * np.sin(2.0 * np.pi * 6.0 * t))
    shriek = O.additive(shriek_f, [1.0, 0.4, 0.2], n=m)
    shriek = C._cold(_env(normalize(shriek), [(0.0, 0.0), (0.3, 0.5), (0.9, 1.0), (1.5, 0.0)]), child(r, 'sc'), 51.0, 0.3)
    ghost = C._ghost_ring(root * 6.0, 1.6, child(r, 'ghost'))
    x = layers((weight, 0.0, 0.0), (_lv(crack, weight, -5.0, 0.01), 0.0, 0.0), (_lv(roar, weight, -7.0), 0.0, 0.0),
               (_lv(normalize(embers), weight, -17.0), 0.02, 0.0), (_lv(toll, weight, -3.0), 0.0, 0.0),
               (_lv(wail, weight, -5.0), 0.2, 0.0), (_lv(cold_bowl, weight, -7.0), 0.25, 0.0), (_lv(rush, weight, -9.0), 0.15, 0.0),
               (_lv(shriek, weight, -13.0), 0.45, 0.0), (_lv(ghost, weight, -14.0), 1.0, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(30.0), extend=False)
    return _limit(C._haunt(x, r, -7.0, 2.8, 2800.0))


# --- Loot pops ----------------------------------------------------------------------------------------------------------

@cue('Loot.Tier.Pop.Hallowed', variations=4, att='Creature', jitter=0.03, conc=6, level=-11.0)
def pop_hallowed(v, r):
    # One piece thrown out of the shower, a step above the boss loot's pop: the hollow pop, a rush up, a gold glint with
    # a round body and a bell's hum under it, a second glint a fifth up, a few sparkles. Each take a different pair of
    # notes, since the pieces come out one after another.
    a, b = (('E6', 'B6'), ('G#6', 'B6'), ('B5', 'E6'), ('E6', 'G#6'))[v]
    pop = C._hollow(child(r, 'pop'), 540.0 * jitter(r, 1, 0.08), 0.06, 2800.0)
    thud = kit.noise_thump(0.08, child(r, 'thud'), 1100.0, 180.0, 0.035, 1.0, 0.02)
    up = kit.whoosh(0.28, child(r, 'up'), 600.0, 4200.0, 1.0, 0.35, -2.0, 0.9)
    g1 = _glint(a, child(r, 'g1'), 0.6, 0.65, -8.0, 0.0002, -14.0)
    g2 = _glint(b, child(r, 'g2'), 0.75, 0.65, -8.0, 0.0002, -14.0)
    spark = _sparkle(child(r, 'spark'), 5, 0.04, 0.3)
    x = layers((pop, 0.0, 0.0), (thud, 0.0, -6.0), (up, 0.0, -9.0), (g1, 0.012, -3.0), (g2, 0.07, -2.0), (spark, 0.04, -21.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.lp(8500.0, 0.7), extend=False)
    return _gold(x, r, -17.0, 0.9)


@cue('Loot.Tier.Pop.Ethereal', variations=4, att='Creature', jitter=0.03, conc=6, level=-10.0)
def pop_ethereal(v, r):
    # The same, redder and lower: a deeper pop, a longer rush up, a glass glint (its round body and hum under it, a cold
    # ring on it, a little tink an octave up), embers crackling out and a short ghost "ah".
    a = ('E5', 'G5', 'B5', 'D6')[v]
    pop = C._hollow(child(r, 'pop'), 430.0 * jitter(r, 1, 0.08), 0.07, 2600.0)
    thud = kit.noise_thump(0.09, child(r, 'thud'), 900.0, 140.0, 0.04, 1.0, 0.02)
    up = kit.whoosh(0.34, child(r, 'up'), 400.0, 3600.0, 1.0, 0.4, -2.0, 0.9)
    glint = _glint(a, child(r, 'g'), 0.85, 0.8, -8.0, 0.0002, -11.0)
    glint = layers((glint, 0.0, 0.0), (C._cold(glint, child(r, 'cg'), 37.0, 0.3), 0.0, -4.0))
    tink = kit.chime(kit.note(a) * 2.0, child(r, 'tink'), 0.35, 0.5, 0.0002, 0.5, -34.0)
    embers = _crackle(0.4, child(r, 'ember'), 500.0, 0.12, 1800.0, 6500.0, 5000.0)
    f = kit.note(a) * 0.5
    ah = C._choir(child(r, 'ah'), [(0.0, f), (0.28, f * 1.12)], 0.28, [(0.0, 'o'), (0.28, 'a')], 2, 20.0, 0.02, 0.6)
    ah = C._cold(_env(ah, [(0.0, 0.0), (0.06, 1.0), (0.28, 0.0)]), child(r, 'ca'), 33.0, 0.25)
    spark = _sparkle(child(r, 'spark'), 5, 0.03, 0.3, 2800.0, 5200.0)
    x = layers((pop, 0.0, 0.0), (thud, 0.0, -6.0), (up, 0.0, -8.0), (glint, 0.012, -2.0), (tink, 0.03, -8.0),
               (normalize(embers), 0.02, -13.0), (normalize(ah), 0.05, -10.0), (spark, 0.04, -17.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.lp(8000.0, 0.7), extend=False)
    return C._haunt(x, r, -16.0, 0.9, 3000.0)


# --- Traits -------------------------------------------------------------------------------------------------------------

@cue('Creature.Trait.WardBreak', variations=3, att='Creature', jitter=0.05, conc=3, level=-9.0)
def ward_break(v, r):
    # A soul ward shattering: a crack and a pop of air, the glass's ring bending down as it breaks, a shower of shards
    # tinkling down (thick at first, thinning), and a breath of the soul let out.
    crack = kit.burst(0.014, child(r, 'crack'), 0.006, lo=2200.0, hi=9500.0, attack=0.0001)
    pop = layers((kit.noise_thump(0.12, child(r, 'pop'), 1300.0, 180.0, 0.05, 1.0, 0.04, 0.0008), 0.0, 0.0),
                 (kit.thump(180.0, 80.0, 0.1, 0.05), 0.0, -4.0))
    glass = C._bowl(1100.0 * jitter(r, 1, 0.05), 0.6, child(r, 'glass'), -0.3, 5.0)

    def shard(rr, t, i):
        m = M.parts(rr.uniform(1800.0, 4200.0), rr.uniform(7000.0, 11000.0), rr, 4, 0.05, 0.015, 0.2)
        return normalize(M.strike(m, M.hammer(0.00006), 0.07)) * rr.uniform(0.3, 1.0) * np.exp(-t / 0.22)
    shards = normalize(G.cloud(0.55, child(r, 'shards'), lambda tt: 230.0 * np.exp(-tt / 0.16) + 14.0, shard, start=0.0))
    tink = kit.chime(2400.0 * jitter(r, 1, 0.08), child(r, 'tink'), 0.3, 0.5, 0.00008, 0.5, -30.0)
    breath = kit.whoosh(0.4, child(r, 'breath'), 900.0, 3500.0, 0.9, 0.2, -3.0, 0.8)
    x = layers((pop, 0.0, 0.0), (_lv(crack, pop, -2.0, 0.01), 0.0, 0.0), (_lv(glass, pop, -5.0, 0.02), 0.0, 0.0),
               (_lv(shards, pop, -3.0, 0.02), 0.01, 0.0), (_lv(tink, pop, -7.0, 0.02), 0.0, 0.0),
               (_lv(breath, pop, -12.0, 0.02), 0.02, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(120.0), extend=False)
    # A soft clip takes the crack's edge down so the glass body under it is heard (a spike over a quiet shower is thin).
    x = D.drive(normalize(x), 4.0, 'tanh')
    return C._outside(x, r, -20.0)


@cue('Creature.Trait.WardHit', variations=5, att='Near', jitter=0.04, conc=4, level=-16.0)
def ward_hit(v, r):
    # A shot strikes the soul ward's cage of light: one small glass ping (a soft mallet, the upper partials kept low so a
    # whole fight of them stays off the 2-5 kHz band), a round sine body under it, a faint knock of the cage and a cold
    # shimmer. Each take a different note of a pentatonic, so any order of them sounds right.
    f = (880.0, 988.0, 1175.0, 1319.0, 740.0)[v] * jitter(r, 1, 0.01)
    ping = kit.chime(f, child(r, 'ping'), 0.2, 0.65, 0.0002, 0.5, -26.0)
    body = normalize(O.sine(f, n=ns(0.18)) * E.perc(0.18, 0.003, 0.12))
    ping = C._cold(layers((ping, 0.0, 0.0), (body, 0.0, -8.0)), child(r, 'cold'), 37.0, 0.15)
    knock = kit.thunk(520.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.03, 0.0004, 4, 0.1)
    shimmer = kit.metal_click(child(r, 'shimmer'), 5000.0, 9000.0, 0.03, 0.00004, 3, -20.0)
    x = layers((normalize(ping), 0.0, 0.0), (knock, 0.0, -11.0), (shimmer, 0.0, -15.0))
    return F.filt(x, F.peak(3300.0, -5.0, 0.8), F.lp(9000.0, 0.7), extend=False)


@cue('Creature.Trait.Swell', variations=2, att='Creature', jitter=0.03, conc=3, level=-7.0, swell=True)
def trait_swell(v, r):
    # Grave-burst's warning, one second: pressure building in a body that is about to go. A tone climbing, a heartbeat
    # running faster and faster, the body straining (stick-slip groans quickening), a whistle of pressure rising, all
    # growing about 17 dB; then the hush of the last seventh, as the world holds its breath before the blast.
    dur = 1.0
    n = ns(dur)
    t = times(n)
    u = t / dur
    f = O.expsweep(62.0 * (1.0, 1.07)[v], 300.0, dur, 1.0, n=n)
    tone = normalize(O.additive(f, [1.0, 0.7, 0.5, 0.3, 0.2], n=n)) * (0.15 + 0.85 * u ** 2)
    beats = np.zeros(1)
    tt, gap = 0.0, 0.2
    while tt < 0.86:
        beat = layers((kit.thump(84.0, 46.0, 0.13, 0.06), 0.0, 0.0),
                      (kit.thunk(230.0 * jitter(r, 1, 0.05), child(r, 'knock', int(tt * 1000)), 0.03, 0.0006, 4, 0.1), 0.0, -7.0))
        beats = layers(beats, (beat, tt, 0.0))
        tt += gap
        gap = max(gap * 0.85, 0.05)
    groan = kit.creak(0.9, child(r, 'groan'), 22.0, 120.0, (200.0, 460.0, 980.0), 6.0, 0.3)
    whistle = F.sweep(N.pink(n, child(r, 'whistle')), 'bp', O.expsweep(500.0, 3600.0, dur, 1.0, n=n), 8.0) * u ** 2.2
    x = layers((normalize(tone), 0.0, 0.0), (normalize(beats), 0.0, -3.0), (normalize(groan), 0.0, -6.0),
               (normalize(whistle), 0.0, -8.0))
    tx = times(x.size)
    # It all grows, then the hush: a fall that eases to nothing (so the last milliseconds are silent, not cut).
    grow = 0.14 + 0.86 * np.clip(tx / 0.86, 0.0, 1.0) ** 1.5
    s = np.clip((tx - 0.86) / 0.14, 0.0, 1.0)
    hush = np.cos(0.5 * np.pi * s) ** 4
    x = F.filt(x * grow * hush, F.peak(3200.0, -3.0, 0.8), F.hp(40.0), extend=False)
    return x


@cue('Creature.Trait.Burst', variations=3, att='Creature', jitter=0.04, conc=3, level=-4.0)
def trait_burst(v, r):
    # The grave-burst's blast: a crack and the push of air, a heave in the chest, bone shards and clods thrown out, dirt
    # and dust raining back, something wet, and a ghost's cry going out in it. Through a soft clip and a limiter.
    crack = layers(kit.nwave(0.0003), (kit.burst(0.02, child(r, 'snap'), 0.008, lo=700.0, hi=6000.0, attack=0.0002), 0.0, -4.0))
    push = _pressure(child(r, 'push'), 0.006, 0.12, 380.0)
    heave = kit.noise_thump(0.6, child(r, 'heave'), 1400.0, 45.0, 0.38, 1.0, 0.12, 0.0006)
    weight = kit.thump(68.0 * jitter(r, 1, 0.06), 28.0, 0.9, 0.42, attack=0.001, drive_db=3.0)

    def chunk(rr, i):
        return kit.thunk(rr.uniform(500.0, 1600.0), child(rr, 'ck', i), 0.02, 0.0005, 4, 0.12) * rr.uniform(0.4, 1.0)
    chunks = normalize(G.rattle(0.5, child(r, 'chunks'), 7 + int(r.integers(0, 4)), 0.4, chunk, 0.03, 0.82), 0.0)
    dirt = kit.grit_burst(0.9, child(r, 'dirt'), 6000.0, 400.0, 3000.0, 0.45)
    dust = kit.burst(0.6, child(r, 'dust'), 0.25, lo=150.0, hi=1500.0, color=-3.0, attack=0.002)
    wet = kit.squelch(0.18, child(r, 'wet'), 1500.0, 350.0)
    gas = kit.whoosh(0.5, child(r, 'gas'), 300.0, 1800.0, 0.9, 0.12, -2.0, 0.8)
    f = 240.0 * jitter(r, 1, 0.1)
    cry = C._choir(child(r, 'cry'), [(0.0, f * 1.2), (0.5, f * 0.6)], 0.5, [(0.0, 'a'), (0.5, 'u')], 3, 26.0, 0.03, 0.7, 1.0, (5.0, 0.3), 0.3)
    cry = _env(C._cold(cry, child(r, 'cold'), 33.0, 0.2), [(0.0, 0.0), (0.05, 1.0), (0.5, 0.0)])
    core = layers((crack, 0.0, 0.0), (_lv(push, crack, 1.0, 0.01), 0.0, 0.0), (_lv(heave, crack, 0.0, 0.01), 0.0, 0.0),
                  (_lv(weight, crack, 1.0, 0.01), 0.0, 0.0))
    x = layers((core, 0.0, 0.0), (_lv(chunks, core, -8.0, 0.01), 0.02, 0.0), (_lv(dirt, core, -14.0, 0.01), 0.02, 0.0),
               (_lv(dust, core, -13.0, 0.01), 0.0, 0.0), (_lv(wet, core, -12.0, 0.01), 0.02, 0.0),
               (_lv(gas, core, -12.0, 0.01), 0.0, 0.0), (_lv(cry, core, -14.0, 0.02), 0.04, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(28.0), extend=False)
    return _dense(C._outside(x, r, -16.0, 0.09, 0.22, 0.7), 4.0, 2.0)


@cue('Creature.Trait.Phase', variations=3, att='Creature', jitter=0.05, conc=3, level=-11.0, swell=True)
def trait_phase(v, r):
    # A blink away: a whoosh run backwards, drawn in (its band climbing) into the snap of air where it was, then a glassy
    # shimmer left behind, fading.
    d = 0.2
    sucked = reverse(kit.whoosh(d, child(r, 'in'), 2600.0, 500.0, 1.1, 0.08, -1.0, 1.0))
    snap = layers(kit.burst(0.012, child(r, 'snap'), 0.004, lo=1200.0, hi=7000.0, attack=0.0001),
                  (kit.noise_thump(0.08, child(r, 'puff'), 900.0, 200.0, 0.03, 1.0, 0.02), 0.0, -3.0))
    spark = _sparkle(child(r, 'spark'), 8, 0.0, 0.35, 1700.0, 4200.0)
    ghost = C._ghost_ring(1500.0 * jitter(r, 1, 0.06), 0.5, child(r, 'ghost'))
    x = layers((sucked, 0.0, 0.0), (_lv(snap, sucked, -3.0, 0.01), d, 0.0), (_lv(spark, sucked, -5.0, 0.02), d + 0.01, 0.0),
               (_lv(ghost, sucked, -8.0, 0.02), d, 0.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.highshelf(6000.0, -4.0), F.hp(150.0), extend=False)
    return C._outside(x, r, -22.0)


@cue('Creature.Trait.Enrage', variations=3, att='Creature', jitter=0.05, conc=2, level=-7.0)
def trait_enrage(v, r):
    # Vengeful's flare: a flame catching in its chest (a crack, a whoomph of air, a flare rising, embers snapping) and a
    # rough snarl over it, torn and gargling.
    crack = kit.burst(0.01, child(r, 'crack'), 0.004, lo=1200.0, hi=6000.0, attack=0.0002)
    whoomph = kit.noise_thump(0.3, child(r, 'whoomph'), 480.0 * jitter(r, 1, 0.08), 60.0, 0.16, 1.0, 0.14, 0.003)
    flare = F.filt(kit.whoosh(0.5, child(r, 'flare'), 250.0, 3200.0, 0.9, 0.3, -3.0, 0.9), F.lp(4200.0, 0.7), extend=False)
    m = ns(0.5)
    embers = G.grit(0.5, child(r, 'ember'), 300.0 * np.exp(-times(m) / 0.2) + 60.0, 1600.0, 6000.0, 3.0, 3)
    embers = F.filt(embers, F.lp(5000.0), extend=False)
    f = (118.0, 104.0, 132.0)[v] * jitter(r, 1, 0.04)
    snarl = C._choir(child(r, 'snarl'), [(0.0, f * 0.9), (0.12, f * 1.25), (0.45, f * 0.95)], 0.5,
                     [(0.0, 'uh'), (0.2, 'a'), (0.5, 'er')], 2, 20.0, 0.01, 0.35, 0.9, (7.0, 0.35), 0.8, 3.0)
    snarl = D.drive(normalize(snarl), 10.0, 'tanh', bias=0.2)
    gargle = 0.6 + 0.4 * np.sin(2.0 * np.pi * (36.0 + 6.0 * r.random()) * times(snarl.size))
    snarl = _env(snarl * gargle, [(0.0, 0.0), (0.03, 1.0), (0.3, 0.9), (snarl.size / SR, 0.0)])
    tail = _crackle(0.6, child(r, 'tail'), 160.0, 0.25, 1500.0, 5500.0, 5000.0)
    x = layers((crack, 0.0, 0.0), (_lv(whoomph, crack, 2.0, 0.01), 0.0, 0.0), (_lv(flare, whoomph, -7.0, 0.02), 0.02, 0.0),
               (_lv(normalize(embers), whoomph, -13.0, 0.02), 0.02, 0.0), (_lv(normalize(snarl), whoomph, 0.0, 0.03), 0.02, 0.0),
               (_lv(tail, whoomph, -24.0, 0.02), 0.3, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(60.0), extend=False)
    return C._outside(x, r, -19.0)


def _wrap_add(dst, src, at):
    """src added into the circle dst at a time, running on past the end back round to the start: a loop's ring-out."""
    np.add.at(dst, (ns(at) + np.arange(src.size)) % dst.size, src)


@cue('Creature.Trait.Mend', variations=1, att='Near', jitter=0.0, conc=4, loop=True, level=-22.0, align=False)
def trait_mend(v, r):
    # Mending: ten soft glass notes climbing a major pentatonic (D F# A B D E F#) in little steps back and forth, each
    # ringing on over the next so they blur into a shimmer, over a warm pad and a fine air. The notes are wrapped round the
    # circle and the pad is silent at the seam, so it loops without a join and, played once, breathes in and out.
    dur = 2.4
    n = ns(dur)
    t = times(n)
    x = np.zeros(n)
    scale = (587.3, 740.0, 880.0, 987.8, 1174.7, 1318.5, 1480.0)
    walk = (0, 2, 1, 3, 2, 4, 3, 5, 4, 6)
    for i, k_note in enumerate(walk):
        f = scale[k_note]
        ringing = 1.3
        k = ns(ringing)
        tk = times(k)
        ph = 2.0 * np.pi * r.random()
        ping = (np.sin(2.0 * np.pi * f * tk + ph) + 0.7 * np.sin(2.0 * np.pi * (f + 1.3) * tk)) * np.exp(-6.9 * tk / ringing)
        ping += 0.3 * np.sin(2.0 * np.pi * f * 2.0 * tk) * np.exp(-6.9 * tk / (ringing * 0.5))
        ping += 0.1 * np.sin(2.0 * np.pi * f * 3.01 * tk) * np.exp(-6.9 * tk / (ringing * 0.3))
        a = ns(0.02)
        ping[:a] *= 0.5 - 0.5 * np.cos(np.pi * np.arange(a) / a)
        gain_db = r.uniform(-4.0, 0.0) - 0.8 * k_note
        _wrap_add(x, ping * float(db2a(gain_db)), (i + 0.35) / len(walk) * dur + r.uniform(-0.03, 0.03))
    x = normalize(x)
    pad = np.zeros(n)
    for f, a in ((146.8, 1.0), (220.0, 0.6), (293.7, 0.35)):
        fc = round(f * dur) / dur
        pad += a * np.sin(2.0 * np.pi * fc * t) + 0.5 * a * np.sin(2.0 * np.pi * (fc + 1.0 / dur) * t)
    pad = normalize(pad) * np.sin(np.pi * t / dur) ** 2
    air = N.shaped(n, child(r, 'air'), 0.0, 3000.0, 9000.0, periodic=True)
    air = normalize(air * (0.35 + 0.65 * np.abs(N.smooth_random(n, child(r, 'af'), 9.0, periodic=True))) * np.sin(np.pi * t / dur) ** 2)
    y = x + 0.18 * pad + 0.08 * air
    return F.filt(y, F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), F.hp(110.0), circular=True)


@cue('Creature.Trait.Rally', variations=3, att='Creature', jitter=0.04, conc=2, level=-14.0)
def trait_rally(v, r):
    # An aura pulse, low and short: a soft low drum and a frame drum's papery slap, a warm low fifth swelling in them
    # (brass-like, its top rolled off), and a ring of air going out from the creature with a soft bowl's note in it. It
    # repeats, so it is soft-edged and stays out of the 2-5 kHz band.
    root = (98.0, 92.5, 104.0)[v] * jitter(r, 1, 0.02)
    drum = F.filt(B.big_drum(child(r, 'drum'), 0.8, f0=root * 0.75, t60=0.35), F.lp(900.0, 0.7), extend=False)
    skin = F.filt(B.frame_drum(child(r, 'skin'), 0.7), F.lp(1500.0, 0.7), extend=False)
    dur = 0.6
    chord = _drone([root, root * 1.5], dur, child(r, 'chord'), 1.0, (1.0, 0.7, 0.5, 0.35, 0.25))
    chord = F.filt(_env(normalize(chord), [(0.0, 0.0), (0.06, 1.0), (0.2, 0.7), (dur, 0.0)]), F.lp(2200.0, 0.7), extend=False)
    ringout = kit.whoosh(0.4, child(r, 'ring'), 160.0, 900.0, 0.9, 0.35, -3.0, 1.0)
    bowl = C._bowl(root * 4.0, 0.5, child(r, 'bowl'), 0.0, 1.5)
    # The call in it: a short low "oh" of two voices, so there is a body in the mids for small speakers too.
    call = C._choir(child(r, 'call'), [(0.0, root * 1.5), (0.4, root * 1.5)], 0.4, [(0.0, 'o'), (0.4, 'u')], 2, 15.0, 0.02, 0.4, 1.0,
                    (4.0, 0.2))
    call = _env(call, [(0.0, 0.0), (0.08, 1.0), (0.2, 0.8), (0.42, 0.0)])
    x = layers((drum, 0.0, 0.0), (_lv(skin, drum, -6.0, 0.02), 0.0, 0.0), (_lv(normalize(chord), drum, -5.0), 0.0, 0.0),
               (_lv(ringout, drum, -9.0), 0.03, 0.0), (_lv(bowl, drum, -10.0), 0.04, 0.0), (_lv(normalize(call), drum, -8.0), 0.02, 0.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(40.0), extend=False)
    return C._outside(x, r, -22.0)
