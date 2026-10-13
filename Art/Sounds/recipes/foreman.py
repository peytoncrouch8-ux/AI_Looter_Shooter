"""The Foreman (AForemanCreature, wave 2c): the Providence's dead foreman, a big man with a coal in his chest, his fuse
sticks, the roof he brings down and the shift he raises out of the rubble. All 3D, in the stope (a big rock chamber off
the drifts: miners.gallery, larger, with the drift's slap-backs for what's carried).

His voice is the Unpaid's choir (creatures.py) a big man's octave down, rough and driven: a bellow, not a wail. The mine
answers him in its own materials: a steam whistle shrieking down the stope (orelift.py's whistle, higher, a chime of two
pipes), a hammer on a steel plate, pine props cracking (props.py's boards, thicker), rock crashing and gravel sliding,
stone grinding overhead. A powder stick going off is built like the salt grenade (guns.py's blast, the recorder's
overload) with the tunnel's hard echoes and rock raining down instead of salt. Fuses and matches are fire's small
cousins: a fierce fizz of sparks, a match's scratch and flare.

- Boss.Foreman.Intro / Phase / Stagger / Death: the fight's show (a whistle and a bellow; a roar and a hammered plate; a
  grunt, buckles, the pick handle scraping; a long exhale that breaks, the mine settling, a drip).
- Boss.Foreman.Slam: the pick handle slammed on the floor, grit pattering down. CoalOpen: his coal bared, an ember roar.
- Boss.Foreman.FuseLight / FuseHiss (loop) / FuseThrow / FuseLand / FuseBurst: a fuse stick's life.
- Boss.Foreman.RoofStrike / RoofGroan / RockFall / PropSplinter: the roof coming down.
- Boss.Foreman.LanternsOut / LanternsBack: the dark, and the light again. ShiftRise: his shift out of the rubble.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, add_at
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.creatures import _choir, _cold
from recipes.bosses import _flame, _crackle, _debris, _snaps, _gutter
from recipes.miners import gallery, rock_hit
from recipes.orelift import _blast
from recipes.shrines import _grind


def _stope(x, r, rel_db=-9.0, t60=1.6):
    """The stope round a carried sound: the rock chamber's dense wash, and the drifts throwing it back."""
    ir = R.outdoor(child(r, 'drift'), dur=max(1.2, t60 * 1.2), slaps=((0.06, -5.0), (0.15, -9.0), (0.29, -14.0)),
                   slap_lp=2600.0, tail_t60=t60, tail_db=-10.0, tail_start=0.02, tail_peak=0.12, tail_lp=2400.0)
    wet = F.convolve(F.filt(x, F.hp(160.0), extend=False), ir)
    return gallery(layers(x, kit.set_level(wet, x, rel_db - 3.0, 0.03)), r, rel_db, t60 * 0.7, 3.4)


def _bellow(f, dur, r, keys, rough=0.55, drive_db=8.0, count=3):
    """A big dead man's bellow: the Unpaid's choir an octave down, rough (period-doubled) and driven, cold."""
    x = _choir(child(r, 'bellow'), f, dur, keys, count, 12.0, 0.02, 0.35, 0.86, (4.5, 0.2), rough, 2.0)
    x = D.drive(x, drive_db, 'tanh', bias=0.1)
    return _cold(normalize(x), child(r, 'cold'), 27.0, 0.15)


def _grunt(r, f=95.0, dur=0.28, keys=('uh', 'a')):
    """A short heavy grunt: one breathy, rough syllable pushed out."""
    g = V.voice(O.glide([(0.0, f * 1.1), (dur, f * 0.85)], n=ns(dur)), [(0.0, keys[0]), (dur, keys[1])], child(r, 'grunt'),
                breath=0.5, tilt=-1.0, shift=0.88, rough=0.5)
    g = D.drive(normalize(g), 6.0, 'tanh') * E.bp([(0.0, 0.0), (0.006, 0.6), (0.04, 1.0), (dur, 0.0)], n=ns(dur), curve='cos')
    return _cold(normalize(g), child(r, 'cold'), 29.0, 0.15)


def _rocks(dur, r, count, start, spread, lo=500.0, hi=2600.0):
    """Rock pieces knocking down: heavier stones than the chips (a low knock and a crack each), fewer and fainter."""
    def grain(rr, i):
        knock = kit.thunk(rr.uniform(lo * 0.4, hi * 0.3), rr, 0.05, 0.0004, 6, 0.12)
        crack = kit.burst(0.005, rr, 0.002, lo=lo, hi=hi * 2.5, attack=0.0002)
        return normalize(layers(knock, (crack, 0.0, -6.0))) * rr.uniform(0.5, 1.0)
    return normalize(G.rattle(dur, child(r, 'rocks'), count, spread, grain, start, 0.88), 0.0)


def _gravel(dur, r, rate=6000.0, tau=0.3, lo=500.0, hi=3500.0):
    """Gravel sliding and settling: dense grit thinning out."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'gravel'), rate * np.exp(-times(n) / tau), lo, hi, 1.6, 5)
    return normalize(F.filt(g, F.peak(3300.0, -4.0, 0.8), extend=False) * E.end_fade(np.ones(n), 0.05), 0.0)


def _dust(dur, r, rate=900.0, rise=True):
    """Dust and grit sifting down from the roof: a faint, dry patter growing (or thinning)."""
    n = ns(dur)
    shape = np.clip(times(n) / dur, 0.0, 1.0) ** 1.5 if rise else np.exp(-times(n) / (dur * 0.4))
    g = G.grit(dur, child(r, 'dust'), rate * shape + 20.0, 1500.0, 6000.0, 1.6, 3)
    # It thins away rather than stopping: the last of it fades over a fifth of its length (or 0.15 s as it builds).
    g = g * E.bp([(0.0, 1.0), (dur - (0.15 if rise else dur * 0.2), 1.0), (dur, 0.0)], n=n, curve='cos')
    return normalize(F.filt(g, F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False), 0.0)


def _timber_groan(dur, r, rate0=8.0, rate1=14.0, f=130.0):
    """A pine prop under the roof's weight groaning: slow stick-slip through a thick post's low resonances."""
    return kit.creak(dur, child(r, 'groan'), rate0, rate1, (f * jitter(r, 1, 0.08), f * 2.4, f * 5.1), 6.0, 0.35)


def _plate_ring(r, f0=330.0, t60=1.4):
    """A thick steel plate struck by a hammer: dense modes, a hard clang, ringing on."""
    m = M.plate(f0 * jitter(r, 1, 0.04), child(r, 'plate'), 26, t60, 0.55, tilt=0.6)
    ring = normalize(M.strike(m, M.hammer(0.00015), t60 * 1.2))
    clang = normalize(M.strike(M.parts(1200.0, 4000.0, child(r, 'clang'), 7, 0.05, 0.02, 0.4), M.hammer(0.00006), 0.1))
    return F.filt(layers((ring, 0.0, 0.0), (clang, 0.0, -8.0)), F.peak(3300.0, -5.0, 0.8), F.highshelf(6000.0, -4.0),
                  extend=False)


# --- The show -------------------------------------------------------------------------------------------------------

@cue('Boss.Foreman.Intro', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-4.0)
def intro(v, r):
    # His bar sweeps in: the mine's steam whistle shrieking down the stope (two pipes a third apart, long, scooping up),
    # and under it a big dead man's bellow rising out of his chest, the drifts throwing both back.
    whistle = layers((_blast(880.0, 1.5, child(r, 'w1'), 0.96), 0.0, 0.0), (_blast(740.0, 1.5, child(r, 'w2'), 0.96), 0.0, -2.0))
    dur = 1.9
    f = 88.0
    bellow = _bellow([(0.0, f * 0.85), (0.4, f * 1.15), (1.2, f * 1.08), (dur, f * 0.8)], dur, child(r, 'bellow'),
                     [(0.0, 'uh'), (0.3, 'a'), (1.2, 'a'), (dur, 'o')])
    bellow = bellow * E.bp([(0.0, 0.0), (0.02, 0.4), (0.4, 1.0), (1.4, 0.9), (dur, 0.0)], n=bellow.size, curve='cos')
    # The whistle's valve lever thrown leads it (a clank of iron), so the cue speaks at once.
    valve = kit.clack(child(r, 'valve'), 520.0, 1100.0, 3600.0, 0.04, 0.05, -2.0, -8.0)
    x = layers((valve, 0.0, -12.0), (normalize(whistle), 0.0, -7.0), (bellow, 0.3, 0.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -8.0, 2.0)


@cue('Boss.Foreman.Phase', variations=2, att='Gun', cls='Effects', jitter=0.02, conc=1, level=-4.0)
def phase(v, r):
    # A later phase: his roar of anger (a bellow driven hard, rising), and his hammer rung twice on a steel plate.
    dur = 1.3
    f = [96.0, 90.0][v]
    roar = _bellow([(0.0, f), (0.25, f * 1.3), (0.9, f * 1.2), (dur, f * 0.85)], dur, child(r, 'roar'),
                   [(0.0, 'a'), (0.5, 'ae'), (dur, 'o')], 0.7, 12.0)
    roar = roar * E.bp([(0.0, 0.0), (0.008, 0.5), (0.2, 1.0), (1.0, 0.8), (dur, 0.0)], n=roar.size, curve='cos')
    p1 = _plate_ring(child(r, 'p1'), 330.0 * jitter(r, 1, 0.03))
    p2 = _plate_ring(child(r, 'p2'), 330.0 * jitter(r, 1, 0.03))
    t = dur + 0.05
    x = layers((roar, 0.0, 0.0), (p1, t, -2.0), (p2, t + 0.38, -3.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -8.0, 1.8)


@cue('Boss.Foreman.Stagger', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0)
def stagger(v, r):
    # Knocked to a knee: a heavy grunt, the buckles on his coat rattling, and his pick handle scraping on stone as he
    # leans on it.
    grunt = _grunt(child(r, 'grunt'), [92.0, 86.0][v], 0.32, ('uh', 'uh'))
    knee = kit.noise_thump(0.25, child(r, 'knee'), 500.0, 60.0, 0.12, 1.0, 0.06, 0.0008)
    buckles = kit.jingle(0.35, child(r, 'buckles'), 6, 1300.0, 4500.0, 0.05, 0.25, 0.02)
    buckles = F.filt(buckles, F.peak(3300.0, -5.0, 0.8), extend=False)
    scrape = kit.scrape(0.4, child(r, 'scrape'), 500.0, 380.0, 4.0, 0.8, (1.0, 1.7, 2.6), 0.15, 0.5, 1800.0, -24.0)
    scrape = F.filt(scrape, F.lp(2500.0, 0.7), extend=False)
    x = layers((grunt, 0.0, 0.0), (knee, 0.05, -4.0), (buckles, 0.04, -10.0), (scrape, 0.22, -8.0))
    x = F.filt(x, F.hp(55.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -12.0, 1.4)


@cue('Boss.Foreman.Death', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-5.0)
def death(v, r):
    # His last breath: a long exhale out of that big chest that breaks once (a voiced catch in it) and runs out, the coal
    # crackling down; the mine settling round him (a timber easing, a trickle of grit), and far off a single drip.
    dur = 2.6
    n = ns(dur)
    breath = V.whisper(dur, [(0.0, 'a'), (0.9, 'o'), (1.8, 'u'), (dur, 'h')], child(r, 'breath'), 0.86)
    breath = breath * E.bp([(0.0, 0.0), (0.004, 0.5), (0.06, 1.0), (0.8, 0.75), (dur, 0.0)], n=n, curve='cos')
    # The break: the breath stops for a moment where the voice catches.
    brk = 0.95
    gap = np.clip(np.abs(times(n) - brk - 0.06) / 0.06, 0.0, 1.0) ** 2
    breath = breath * (0.15 + 0.85 * gap)
    catch = V.voice(O.glide([(0.0, 96.0), (0.14, 78.0)], n=ns(0.14)), [(0.0, 'uh'), (0.14, 'u')], child(r, 'catch'),
                    breath=0.6, tilt=-2.0, shift=0.88, rough=0.6) * E.perc(0.14, 0.006, 0.11)
    catch = _cold(normalize(catch), child(r, 'cold'), 27.0, 0.15)
    coal = _crackle(2.4, child(r, 'coal'), 240.0, 0.8)
    settle = _timber_groan(0.8, child(r, 'settle'), 12.0, 6.0, 120.0)
    trickle = _dust(1.0, child(r, 'trickle'), 500.0, rise=False)
    drip = kit.bubble(1400.0 * jitter(r, 1, 0.05), child(r, 'drip'), 0.35, 1.0)
    x = layers((normalize(breath), 0.0, 0.0), (catch, brk, -5.0), (coal, 0.05, -16.0), (settle, 1.6, -14.0),
               (trickle, 1.9, -22.0), (normalize(drip), 2.9, -18.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -7.0, 2.4)


# --- His moves ------------------------------------------------------------------------------------------------------

@cue('Boss.Foreman.Slam', variations=3, att='Gun', cls='Effects', jitter=0.04, conc=2, level=-4.0)
def slam(v, r):
    # His pick handle slammed end-down on the floor: a heavy crack of hard wood on stone, the weight behind it shoving
    # the air, then grit and small stones pattering down from the roof.
    knock = kit.thunk(210.0 * jitter(r, 1, 0.08), child(r, 'knock'), 0.12, 0.0006, 7, 0.1)
    crack = kit.burst(0.012, child(r, 'crack'), 0.005, lo=700.0, hi=7000.0, attack=0.0002)
    push = kit.noise_thump(0.45, child(r, 'push'), 600.0, 45.0, 0.24, 1.0, 0.1, 0.0006)
    hit = rock_hit(child(r, 'stone'), 1.3)
    patter = _dust(1.2, child(r, 'patter'), 1100.0, rise=False)
    stones = _rocks(1.2, child(r, 'stones'), 5, 0.25, 0.8, 600.0, 2400.0)
    x = layers((push, 0.0, 0.0), (knock, 0.0, -3.0), (crack, 0.0, -6.0), (hit, 0.0, -6.0), (patter, 0.2, -16.0),
               (stones, 0.0, -14.0))
    x = _dense(F.filt(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.8), extend=False))
    return _stope(x, r, -9.0, 1.6)


def _dense(x):
    """A soft clip, so a slam is dense enough to stand beside the rifle (bosses.py's big hits do the same)."""
    return D.drive(normalize(x), 4.0, 'tanh')


@cue('Boss.Foreman.CoalOpen', variations=2, att='Creature', cls='Effects', jitter=0.03, conc=1, level=-10.0)
def coal_open(v, r):
    # His coal bared: a low ember roar swelling up as the weak spot opens in his chest, crackling, a breath of heat.
    dur = 1.2
    n = ns(dur)
    crack = kit.noise_thump(0.12, child(r, 'open'), 900.0, 150.0, 0.05, 1.0, 0.04, 0.001)
    roar = _flame(dur, child(r, 'roar'), [(0.0, 150.0), (0.3, 420.0 * jitter(r, 1, 0.08)), (dur, 260.0)], 12.0, 0.5, 1.0)
    roar = normalize(roar * E.bp([(0.0, 0.3), (0.3, 1.0), (dur * 0.7, 0.7), (dur, 0.0)], n=n, curve='cos'))
    embers = _crackle(dur, child(r, 'embers'), 160.0, 0.6, 1500.0, 5000.0, 4500.0)
    x = layers((crack, 0.0, -6.0), (roar, 0.0, 0.0), (embers, 0.03, -14.0))
    x = F.filt(x, F.hp(50.0), F.lp(5000.0, 0.7), extend=False)
    return gallery(x, r, -14.0, 1.0, 3.0)


# --- The fuse sticks ------------------------------------------------------------------------------------------------

def _fizz(dur, r, rate=16000.0, periodic=False):
    """A fuse's sparks: a fierce fizz of tiny bright impulses."""
    return G.grit(dur, child(r, 'fizz'), rate, 2500.0, 9000.0, 1.8, 5, periodic=periodic)


@cue('Boss.Foreman.FuseLight', variations=2, att='Creature', cls='Effects', jitter=0.04, conc=2, level=-11.0)
def fuse_light(v, r):
    # A fuse lit: a match struck hard along his boot (a rasping scratch), its head flaring with a soft whoosh, then the
    # fuse catching with a spit of sparks and settling into its fizz.
    d = 0.11 + 0.02 * r.random()
    scratch = kit.scrape(d, child(r, 'scratch'), 1500.0, 1100.0, 3.0, 0.9, (1.0, 1.7, 2.9), 0.05, 0.4, 5000.0, -12.0)
    scratch = F.filt(scratch, F.peak(3300.0, -4.0, 0.8), extend=False)
    flare = layers((kit.whoosh(0.25, child(r, 'flare'), 500.0, 2400.0, 0.9, 0.2, -2.0), 0.0, 0.0),
                   (kit.noise_thump(0.1, child(r, 'head'), 900.0, 200.0, 0.04, 1.0, 0.03, 0.001), 0.0, -4.0))
    t = d + 0.35 + 0.1 * r.random()
    n = ns(0.5)
    spit = _fizz(0.5, child(r, 'spit'), 20000.0) * E.bp([(0.0, 0.0), (0.01, 1.0), (0.15, 0.5), (0.5, 0.0)], n=n, curve='cos')
    pops = layers(*[(kit.burst(0.03, child(r, 'pop', i), 0.012, lo=500.0, hi=3500.0), r.uniform(0.0, 0.2), r.uniform(-8.0, 0.0))
                    for i in range(3)])
    x = layers((scratch, 0.0, 0.0), (flare, d - 0.02, -6.0), (normalize(spit), t, -4.0), (normalize(pops), t, -8.0))
    x = F.filt(x, F.hp(150.0), F.peak(3300.0, -3.0, 0.8), F.highshelf(9000.0, -3.0), extend=False)
    return gallery(x, r, -15.0, 0.9)


@cue('Boss.Foreman.FuseHiss', variations=1, att='Creature', cls='Effects', jitter=0.0, conc=4, loop=True, level=-13.0,
     align=False)
def fuse_hiss(v, r):
    # A lit powder fuse: a steady, angry hiss of sparks (thicker and lower than the salt grenade's waxed fuse), its
    # burning core hissing under it, sparks spitting out of it now and then with small pops. Built in a circle.
    dur = 1.8
    n = ns(dur)
    fizz = _fizz(dur, child(r, 'fizz'), 12000.0, periodic=True)
    fizz *= 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'fa'), 30.0, periodic=True))
    hiss = N.shaped(n, child(r, 'hiss'), -1.0, 900.0, 8000.0, periodic=True)
    hiss *= 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'flutter'), 45.0, periodic=True))
    events = np.zeros(n)
    for i, t in enumerate(N.times_poisson(dur, child(r, 'spits'), 6.0, 0.0, 0.1)):
        k = ns(0.06)
        puff = N.band(k, child(r, 'puff', i), 400.0, 2600.0) * E.perc(0.06, 0.001, 0.03)
        events[(ns(t) + np.arange(k)) % n] += puff * r.uniform(0.4, 1.0)
    x = normalize(fizz) + 0.55 * normalize(hiss) + 0.6 * normalize(events)
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(9000.0, -3.0), F.hp(200.0), circular=True)


@cue('Boss.Foreman.FuseThrow', variations=3, att='Creature', cls='Effects', jitter=0.04, conc=2, level=-9.0)
def fuse_throw(v, r):
    # The stick lobbed: a grunt of effort and a short whoosh of his arm, the fuse's fizz swinging past in it.
    grunt = _grunt(child(r, 'grunt'), [100.0, 94.0, 106.0][v] * jitter(r, 1, 0.03), 0.24, ('uh', 'a'))
    whoosh = kit.whoosh(0.3, child(r, 'whoosh'), 300.0, 1500.0, 1.0, 0.45, -3.0, 1.0)
    fizz = _fizz(0.3, child(r, 'fizz'), 9000.0) * E.ar(0.3, 0.1, 0.15)
    x = layers((grunt, 0.0, 0.0), (whoosh, 0.06, -5.0), (normalize(fizz), 0.08, -18.0))
    x = F.filt(x, F.hp(80.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return gallery(x, r, -14.0, 0.9)


@cue('Boss.Foreman.FuseLand', variations=4, att='Creature', cls='Effects', jitter=0.06, conc=3, level=-12.0)
def fuse_land(v, r):
    # The stick landing on rock: a light wooden clack (a paper-wrapped stick of powder), a bounce, and a short skitter as
    # it rolls.
    clack = kit.thunk(620.0 * jitter(r, 1, 0.1), child(r, 'clack'), 0.04, 0.0003, 5, 0.12)
    tick = kit.burst(0.004, child(r, 'tick'), 0.0015, lo=1500.0, hi=6000.0, attack=0.0001)
    b = 0.08 + 0.04 * r.random()
    bounce = kit.thunk(680.0 * jitter(r, 1, 0.1), child(r, 'bounce'), 0.03, 0.0003, 5, 0.12)
    roll = G.grit(0.3, child(r, 'roll'), 1500.0, 500.0, 3000.0, 1.4, 3) * E.ar(0.3, 0.02, 0.2)
    x = layers((clack, 0.0, 0.0), (tick, 0.0, -10.0), (bounce, b, -8.0), (normalize(roll), b + 0.02, -14.0))
    x = F.filt(x, F.hp(120.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return gallery(x, r, -14.0, 0.9)


@cue('Boss.Foreman.FuseBurst', variations=4, att='Gun', cls='Effects', jitter=0.04, conc=3, level=0.0)
def fuse_burst(v, r):
    # A powder stick going off in the stope: a hard crack and a deep boom (built like the salt grenade's blast: the
    # pressure wave, the heave of air, the gas tearing out, the recorder overloading), the rock throwing it back hard,
    # then stones and gravel rattling down for a second.
    from recipes import guns as Gn
    blast = Gn._loud(Gn._friedlander(child(r, 'blast'), 0.005 * jitter(r, 1.0, 0.15), 0.9 * jitter(r, 1.0, 0.2), 0.08, 0.35),
                     0.003)
    heave = Gn._heave(child(r, 'heave'), 0.016 * jitter(r, 1.0, 0.15), 100.0, 0.2)
    crack = Gn._crack(child(r, 'crack'), 0.0008 * jitter(r, 1.0, 0.12), 0.012, 800.0 * jitter(r, 1.0, 0.08), -4.0, -2.0,
                      1200.0, 8000.0)
    gas = Gn._gas(child(r, 'gas'), 0.5, 0.012, 0.16 * jitter(r, 1.0, 0.12), 16000.0, 0.02, 160.0, 3800.0, -3.0, 8.0,
                  -6.0, 0.3, 1400.0)
    src = layers((blast, 0.0, 6.0), (heave, 0.0, 11.0), (crack, 0.0, -1.0), (gas, 0.0006, -1.0))
    g1 = 0.002 + 0.002 * r.random()
    src = Gn._ground(src, [(g1, -2.0, 3000.0), (g1 + 0.003 + 0.003 * r.random(), -5.0, 1500.0)])
    rec = Gn._loud(Gn._recorder(src, 13.0 + 1.5 * (2.0 * r.random() - 1.0), 0.35, 50.0 * jitter(r, 1.0, 0.08), 1.4, 7.0, 1.5,
                                0.08, 95.0))
    rocks = Gn._loud(_rocks(1.3, child(r, 'rocks'), 10, 0.15, 1.0, 500.0, 2600.0), 0.01)
    gravel = Gn._loud(_gravel(1.5, child(r, 'gravel'), 7000.0, 0.4), 0.02)
    dust = Gn._loud(_dust(1.6, child(r, 'dust'), 1200.0, rise=False), 0.02)
    dry = layers((rec, 0.0, 0.0), (rocks, 0.0, -16.0), (gravel, 0.05, -19.0), (dust, 0.3, -26.0))
    dry = Gn._early(dry, r, -8.0, 0.03, 3500.0)
    ir = R.outdoor(child(r, 'drift'), dur=2.0, slaps=((0.05, -2.0), (0.12, -5.0), (0.24, -9.0), (0.41, -14.0)),
                   slap_lp=2400.0, tail_t60=1.6, tail_db=-8.0, tail_start=0.02, tail_peak=0.15, tail_lp=1800.0)
    wet = F.convolve(F.filt(dry, F.hp(110.0), extend=False), ir)
    return Gn._master(layers(dry, kit.set_level(wet, dry, -12.0, 0.03)), 5.0, 2.0, 0.0015, 0.05)


# --- The roof -------------------------------------------------------------------------------------------------------

@cue('Boss.Foreman.RoofStrike', variations=3, att='Gun', cls='Effects', jitter=0.04, conc=2, level=-5.0)
def roof_strike(v, r):
    # His pick handle hammered into a timber prop: a deep wooden thud (a thick pine post struck), the prop groaning under
    # the roof's weight after it, a little grit shaken loose.
    post = kit.thunk(105.0 * jitter(r, 1, 0.08), child(r, 'post'), 0.26, 0.0012, 7, 0.08)
    knock = kit.thunk(260.0 * jitter(r, 1, 0.08), child(r, 'knock'), 0.07, 0.0006, 6, 0.1)
    push = kit.noise_thump(0.4, child(r, 'push'), 450.0, 50.0, 0.2, 1.0, 0.08, 0.0006)
    groan = _timber_groan(0.9, child(r, 'groan'), 10.0, 6.0, 120.0)
    grit = _dust(0.8, child(r, 'grit'), 700.0, rise=False)
    x = layers((push, 0.0, -1.0), (post, 0.0, 0.0), (knock, 0.0, -6.0), (groan, 0.12, -9.0), (grit, 0.1, -20.0))
    x = _dense(F.filt(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.8), extend=False))
    return _stope(x, r, -10.0, 1.6)


@cue('Boss.Foreman.RoofGroan', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0, swell=True)
def roof_groan(v, r):
    # The roof answering (about 2 s, building to the fall): the props creaking one after another, stone grinding against
    # stone overhead, dust starting to sift and then pouring.
    dur = 2.1
    creaks = np.zeros(1)
    for i, at in enumerate((0.0, 0.5, 0.9, 1.3, 1.6)):
        rr = child(r, 'creak', i)
        g = _timber_groan(rr.uniform(0.5, 0.8), rr, rr.uniform(8.0, 14.0), rr.uniform(14.0, 24.0), rr.uniform(110.0, 170.0))
        creaks = layers(creaks, (g, at + 0.1 * rr.random(), -6.0 + 1.5 * i))
    grind = _grind(dur, child(r, 'grind'), 45.0, 110.0)
    n = ns(dur)
    grind = grind * np.clip(times(grind.size) / dur, 0.0, 1.0) ** 1.2
    dust = _dust(dur, child(r, 'dust'), 2500.0, rise=True)
    snaps = _snaps(dur, child(r, 'snaps'), lambda t: 2.0 + 14.0 * (t / dur) ** 2, 400.0, 2400.0, 0.02, 0.2)
    x = layers((normalize(creaks), 0.0, -2.0), (normalize(grind), 0.0, -3.0), (dust, 0.0, -14.0), (normalize(snaps), 0.0, -12.0))
    x = x * E.bp([(0.0, 0.25), (dur * 0.8, 1.0), (dur + 1.0, 1.0)], n=x.size, curve='cos')
    x = F.filt(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -10.0, 1.6)


@cue('Boss.Foreman.RockFall', variations=3, att='Gun', cls='Effects', jitter=0.05, conc=3, level=-3.0)
def rock_fall(v, r):
    # Rock coming down from the roof onto the floor: a heavy crash (the slab's weight, its crack as it breaks), pieces
    # bouncing and knocking away, and a slide of gravel settling after.
    push = kit.noise_thump(0.6, child(r, 'push'), 380.0, 35.0, 0.3, 1.0, 0.12, 0.0006)
    hit = rock_hit(child(r, 'hit'), 1.5)
    crack = _snaps(0.25, child(r, 'crack'), lambda t: 120.0 * np.exp(-t / 0.06), 400.0, 2500.0, 0.02)
    pieces = _rocks(1.2, child(r, 'pieces'), 9, 0.02, 0.7, 450.0, 2400.0)
    gravel = _gravel(1.4, child(r, 'gravel'), 8000.0, 0.35)
    dust = _dust(1.2, child(r, 'dust'), 900.0, rise=False)
    x = layers((push, 0.0, 0.0), (hit, 0.0, -2.0), (normalize(crack), 0.002, -7.0), (pieces, 0.03, -9.0),
               (gravel, 0.04, -11.0), (dust, 0.3, -20.0))
    x = _dense(F.filt(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.8), extend=False))
    return _stope(x, r, -9.0, 1.8)


@cue('Boss.Foreman.PropSplinter', variations=3, att='Gun', cls='Effects', jitter=0.05, conc=2, level=-4.0)
def prop_splinter(v, r):
    # A timber prop giving way: a loud crack of thick pine splitting (two or three cracks a hair apart, fibres tearing
    # through the post), its weight shifting, and splinters clattering down.
    from recipes.props import _board_crack, _clatter
    f0 = 140.0 + 50.0 * r.random()
    parts = [(_board_crack(child(r, 'c1'), f0), 0.0, 0.0)]
    t = 0.0
    for i in range(2 + int(r.integers(0, 2))):
        t += r.uniform(0.02, 0.06)
        parts.append((_board_crack(child(r, 'c', i), f0 * r.uniform(1.1, 1.6)), t, r.uniform(-6.0, -2.0)))
    tear = kit.creak(0.3, child(r, 'tear'), 400.0, 120.0, (700.0, 1500.0, 2800.0), 4.0, 0.4)
    tear = F.filt(tear, F.peak(3300.0, -4.0, 0.8), extend=False)
    push = kit.noise_thump(0.4, child(r, 'push'), 500.0, 50.0, 0.2, 1.0, 0.08, 0.0006)
    clatter = _clatter(child(r, 'clatter'), 7, 0.6, 0.12, 300.0, 1100.0)
    x = layers(*parts, (tear, 0.01, -10.0), (push, 0.0, -3.0), (clatter, 0.0, -10.0))
    x = _dense(F.filt(x, F.hp(40.0), F.peak(3300.0, -4.0, 0.8), extend=False))
    return _stope(x, r, -10.0, 1.6)


# --- The dark and the shift -----------------------------------------------------------------------------------------

@cue('Boss.Foreman.LanternsOut', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-7.0)
def lanterns_out(v, r):
    # The stope's lanterns blown out at once: a gust tearing through the drift, every flame guttering and dying with a
    # puff, and then a sudden hush, the wind cut off.
    dur = 1.0
    n = ns(dur)
    gust = kit.whoosh(dur, child(r, 'gust'), 250.0, 1400.0, 0.8, 0.55, -3.0, 1.0)
    gust = gust * np.clip((dur - times(n)) / 0.12, 0.0, 1.0) ** 2
    # The gust starts at once: the wind's first push.
    shove = kit.noise_thump(0.2, child(r, 'shove'), 500.0, 90.0, 0.1, 0.9, 0.1, 0.002)
    out = np.zeros(1)
    for i, (at, fc) in enumerate([(0.25, 520.0), (0.33, 470.0), (0.42, 560.0), (0.5, 430.0)]):
        g = _gutter(0.32, child(r, 'gutter', i), fc, 0.24 + 0.02 * i)
        out = layers(out, (normalize(g), at, -1.5 * i))
    hush = kit.noise_thump(0.9, child(r, 'hush'), 220.0, 60.0, 0.6, 0.8, 0.3, 0.004)
    x = layers((shove, 0.0, -6.0), (gust, 0.0, 0.0), (normalize(out), 0.0, -5.0), (hush, dur - 0.05, -14.0))
    x = F.filt(x, F.hp(50.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -11.0, 1.4)


@cue('Boss.Foreman.LanternsBack', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def lanterns_back(v, r):
    # The lanterns catching again round the stope: soft whumps of flame one after another, each settling to a small
    # burn, and their glass chimneys ticking as they warm.
    out = np.zeros(1)
    for i, at in enumerate((0.0, 0.14, 0.26, 0.4)):
        rr = child(r, 'lamp', i)
        whump = kit.noise_thump(0.2, rr, 600.0, 110.0, 0.09, 0.9, 0.08, 0.002)
        flame = _flame(0.6, child(rr, 'flame'), [(0.0, 300.0), (0.08, 800.0), (0.6, 420.0)], 20.0, 0.5, 1.2)
        flame = flame * E.bp([(0.0, 0.3), (0.08, 1.0), (0.6, 0.0)], n=flame.size, curve='cos')
        out = layers(out, (whump, at, -1.0 * i), (normalize(flame), at, -6.0 - 1.0 * i))

    def tick(rr, t, i):
        m = M.parts(2400.0, 6500.0, rr, 4, 0.03, 0.015, 0.3)
        s = normalize(M.strike(m, M.hammer(0.00005), 0.06))
        return F.filt(s, F.peak(3300.0, -6.0, 0.8), extend=False) * rr.uniform(0.3, 1.0)
    ticks = G.cloud(1.6, child(r, 'ticks'), 6.0, tick, start=0.4)
    x = layers((out, 0.0, 0.0), (normalize(ticks), 0.0, -20.0))
    x = F.filt(x, F.hp(60.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -12.0, 1.4)


@cue('Boss.Foreman.ShiftRise', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0)
def shift_rise(v, r):
    # His shift rising out of the rubble: rocks shoved aside and tumbling off (heavy knocks, gravel sliding), and under
    # them several dead men's voices groaning low, cold, coming up together.
    shove = kit.noise_thump(0.3, child(r, 'shove'), 500.0, 60.0, 0.15, 1.0, 0.08, 0.0008)
    rocks = _rocks(1.6, child(r, 'rocks'), 12, 0.0, 1.3, 450.0, 2400.0)
    gravel = _gravel(1.8, child(r, 'gravel'), 4000.0, 0.7)
    grind = _grind(1.4, child(r, 'grind'), 60.0, 140.0)
    dur = 2.2
    voices = np.zeros(1)
    for i, f in enumerate([[104.0, 92.0, 118.0, 98.0], [96.0, 110.0, 88.0, 122.0]][v]):
        rr = child(r, 'voice', i)
        g = _choir(rr, [(0.0, f * 0.9), (0.8, f * 1.08), (dur, f * 0.82)], dur, [(0.0, 'u'), (0.7, 'o'), (1.5, 'uh'), (dur, 'u')],
                   2, 14.0, 0.03, 0.55, 0.92, (4.5, 0.2), 0.35, -1.0)
        g = _cold(g, child(rr, 'cold'), 29.0 + 3.0 * i, 0.2) * E.swell(g.size / SR, 0.9 + 0.2 * rr.random(), 1.3, 1.6)
        voices = layers(voices, (g, 0.15 + 0.12 * i, -2.0 * i))
    x = layers((shove, 0.0, 0.0), (rocks, 0.0, -4.0), (gravel, 0.05, -10.0), (normalize(grind), 0.0, -9.0),
               (normalize(voices), 0.0, -3.0))
    x = F.filt(x, F.hp(45.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _stope(x, r, -8.0, 2.0)
