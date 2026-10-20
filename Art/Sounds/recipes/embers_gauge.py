"""Gauge's ember powers (Embers/Powers: Slag Bomb, Iron Hide, Rail Spike): a track-laying engine's firebox, plates and
steam. Heavy iron, fire and pressure. What it does is its own (2D); what lands out in the world is placed (3D).

- The bomb lobbed (2D): the firebox door clanked open (a heavy iron latch and hinge), the draught's roar for a breath, the
  heave of the throw and the lob's hot sizzle going away.
- The bomb bursting (3D): a heavy, round thump (more shove than crack), molten slag spraying and frying on everything,
  iron bits clanging down, a long hiss of it settling.
- The slag pool (3D loop): thick bubbling, popping and hissing as it cools, built in a circle.
- A sear (3D, each burn): a hot hiss and a pop of something catching.
- The plates locking (2D): plates slammed shut one after another (three heavy clanks, each a little lower), bolts
  ratcheting home, and steam hissing out of the seams.
- A blow off the plates (2D): a heavy plate's dull, short ring with the knock of the blow.
- A biter burned (3D): teeth on hot iron: a short fierce sizzle and a clank.
- The plates opening (2D): steam let out in a long sigh, plates sliding back with a scrape and a last clank.
- The spike driven (2D): a steam piston's bang (a sharp pressure blast through a pipe), the spike's iron ring as it leaves,
  a long hiss of spent steam after.
- The spike through a creature (3D): a heavy iron punch and a crunch, a thud.
- The spike in the world (3D): iron biting into rock (a crack and grit) and ringing out.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S, dist as D
from lib import granular as G
from recipes import cue
from recipes import kit


# --- Iron, fire and steam ---------------------------------------------------------------------------------------------------

def _clank(r, f0=320.0, t60=0.25, weight_db=-4.0):
    """A heavy iron plate struck: a plate's dense modes rung short, a bar's note under it, the blow's weight."""
    plate = M.strike(M.plate(f0, child(r, 'plate'), 16, t60, 0.6, 1.3, 0.02, 0.6), M.hammer(0.0003), t60 * 1.4 + 0.02)
    bar = M.strike(M.bar(f0 * 0.62, child(r, 'bar'), t60 * 0.8, 0.6, 4), M.hammer(0.0004), t60 * 1.2 + 0.02)
    weight = kit.noise_thump(0.12, child(r, 'weight'), 500.0, 90.0, 0.05)
    return normalize(layers((normalize(plate), 0.0, 0.0), (normalize(bar), 0.0, -6.0), (weight, 0.0, weight_db)), 0.0)


def _steam(dur, r, attack=0.08, release=0.5, lo=1400.0, hi=9000.0):
    """Steam escaping: a broad, torn hiss swelling and fading."""
    n = ns(dur)
    x = N.band(n, child(r, 'steam'), lo, hi) * kit.turbulence(n, child(r, 'tur'), 90.0, 0.5)
    x = F.filt(x, F.highshelf(6500.0, -5.0), F.peak(3300.0, -4.0, 0.9), extend=False)
    return normalize(x * E.ar(dur, dur * attack, dur * release, 1.6), 0.0)


def _sizzle(dur, r, rate=10000.0, tau=0.25, lo=2000.0, hi=8000.0):
    """Hot metal frying: dense tiny pops dying away."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'sizzle'), rate * np.exp(-times(n) / tau), lo, hi, 2.0, 5)
    return normalize(g * E.perc(dur, 0.003, dur * 0.7), 0.0)


def _roar(dur, r, f0=180.0, f1=700.0):
    """A firebox's draught: a low, fluttering roar of flame."""
    n = ns(dur)
    x = F.sweep(N.pink(n, child(r, 'roar')) * kit.turbulence(n, child(r, 't'), 35.0, 0.6), 'lp',
                O.expsweep(f0 * 4.0, f1 * 2.0, dur, 1.0, n=n), 0.9)
    return normalize(x * E.ar(dur, dur * 0.2, dur * 0.6, 1.8), 0.0)


def _engine(x, r, wet_db=-16.0, t60=1.2):
    """Its own, in stereo, with a short iron-and-air tail."""
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=2800.0)
    return R.apply(x, ir, wet_db, 0.0, pre_hp=150.0)


def _yard(x, r, rel_db=-14.0, dur=1.4):
    """Out in the world: slap-backs and a dark tail (mono, for a placed sound)."""
    ir = R.outdoor(child(r, 'yard'), dur=dur, slaps=((0.1, -8.0), (0.25, -13.0)), tail_t60=1.0, tail_db=-13.0, tail_lp=2600.0)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _tame(x, hp=50.0):
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(hp), extend=False)


# --- Slag Bomb -------------------------------------------------------------------------------------------------------------

@cue('Ember.SlagBomb.Throw', variations=3, space='2D', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def slag_throw(v, r):
    # The firebox door clanked open (its latch and hinge), the draught's roar for a breath, the heave of the throw and the
    # lob's hot sizzle going away from it.
    latch = _clank(child(r, 'latch'), 520.0 * jitter(r, 1, 0.05), 0.12, -8.0)
    hinge = kit.creak(0.18, child(r, 'hinge'), 40.0, 70.0, (480.0, 1100.0, 2100.0), 9.0, 0.3)
    roar = _roar(0.5, child(r, 'roar'))
    heave = kit.whoosh(0.35, child(r, 'heave'), 200.0, 900.0, 0.8, 0.55, -4.0)
    going = _sizzle(0.6, child(r, 'going'), 6000.0, 0.3) * np.linspace(1.0, 0.2, ns(0.6))
    x = layers((latch, 0.0, 0.0), (hinge, 0.02, -12.0), (roar, 0.05, -5.0), (heave, 0.15, -6.0), (S.pan(normalize(going), 0.3), 0.32, -14.0))
    return _engine(_tame(x, 40.0), r, -17.0, 1.0)


@cue('Ember.SlagBomb.Burst', variations=3, att='Gun', jitter=0.04, conc=2, level=-3.0)
def slag_burst(v, r):
    # A heavy round thump, molten slag spraying and frying on everything round it, iron bits clanging down, and a long hiss
    # of it settling into the ground.
    thump = layers((kit.thump(70.0, 32.0, 0.8, 0.5, drive_db=4.0), 0.0, 0.0),
                   (kit.noise_thump(0.6, child(r, 'air'), 600.0, 60.0, 0.3), 0.0, -2.0))
    splash = kit.squelch(0.35, child(r, 'splash'), 1400.0, 400.0, 2.2)
    fry = _sizzle(1.3, child(r, 'fry'), 14000.0, 0.35)
    bits = G.rattle(0.6, child(r, 'bits'), 6, 0.6, lambda rr, i: _clank(child(rr, 'bit', i), rr.uniform(900.0, 1800.0), 0.05, -12.0), 0.08, 0.8)
    hiss = _steam(1.4, child(r, 'hiss'), 0.15, 0.6)
    x = layers((thump, 0.0, 0.0), (splash, 0.01, -3.0), (fry, 0.03, -3.0), (normalize(bits), 0.0, -12.0), (hiss, 0.15, -12.0))
    x = D.drive(normalize(x), 4.0, 'tanh')
    return _yard(_tame(normalize(x), 30.0), r, -10.0, 1.8)


@cue('Ember.SlagBomb.Pool', variations=1, att='Near', jitter=0.0, conc=2, loop=True, level=-16.0, align=False)
def slag_pool(v, r):
    # The pool: thick bubbling (low bubbles rising and bursting), popping and frying, a hiss under it, built in a circle.
    dur = 2.2
    n = ns(dur)
    bubbles = np.zeros(n)
    for i, t in enumerate(N.times_poisson(dur, child(r, 'bub'), 9.0, 0.0, 0.05)):
        b = kit.bubble(r.uniform(90.0, 260.0), child(r, 'b', i), 0.06, 1.0)
        bubbles[(ns(t) + np.arange(b.size)) % n] += b * r.uniform(0.4, 1.0)
    fry = G.grit(dur, child(r, 'fry'), 4500.0, 1800.0, 7000.0, 2.0, 4, periodic=True)
    fry *= 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'fa'), 12.0, periodic=True))
    hiss = N.shaped(n, child(r, 'hiss'), -2.0, 600.0, 6000.0, periodic=True)
    hiss *= 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'ha'), 6.0, periodic=True))
    x = normalize(bubbles) + 0.35 * normalize(fry) + 0.25 * normalize(hiss)
    return F.filt(x, F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -5.0), F.hp(60.0), circular=True)


@cue('Ember.SlagBomb.Sear', variations=4, att='Near', jitter=0.06, conc=3, level=-14.0)
def slag_sear(v, r):
    # A burn: a hot hiss and a pop of something catching.
    sizzle = _sizzle(0.45, child(r, 'sizzle'), 9000.0, 0.15)
    pop = kit.noise_thump(0.06, child(r, 'pop'), 1200.0, 300.0, 0.02)
    steam = _steam(0.4, child(r, 'steam'), 0.1, 0.6)
    x = layers((pop, 0.0, -4.0), (sizzle, 0.0, 0.0), (steam, 0.03, -10.0))
    return _yard(_tame(normalize(x), 150.0), r, -18.0, 0.8)


# --- Iron Hide -------------------------------------------------------------------------------------------------------------

@cue('Ember.IronHide.Lock', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-7.0)
def hide_lock(v, r):
    # Plates slammed shut one after another, each a little lower, bolts ratcheting home, steam hissing out of the seams.
    plates = layers((_clank(child(r, 'p1'), 300.0, 0.3), 0.0, 0.0), (_clank(child(r, 'p2'), 260.0, 0.3), 0.11, -1.0),
                    (_clank(child(r, 'p3'), 220.0, 0.35), 0.22, 0.0))
    ratchet = layers(*[(kit.metal_click(child(r, 'pawl', i), 1500.0, 5000.0, 0.02, 0.0001, 5, -10.0, 0.4), 0.3 + 0.035 * i, -2.0 * (i % 2))
                       for i in range(6)])
    steam = _steam(0.8, child(r, 'steam'), 0.1, 0.55)
    weight = kit.thump(60.0, 34.0, 0.5, 0.3)
    x = layers((S.widen(plates, child(r, 'pw'), 0.35), 0.0, 0.0), (S.pan(ratchet, -0.2), 0.0, -9.0), (S.widen(steam, child(r, 'sw'), 0.6), 0.2, -12.0),
               (weight, 0.22, -5.0))
    return _engine(_tame(x, 35.0), r, -16.0, 1.2)


@cue('Ember.IronHide.Clang', variations=4, space='2D', cls='Effects', jitter=0.05, conc=2, level=-14.0)
def hide_clang(v, r):
    # A blow off the locked plates: a heavy plate's dull short ring with the knock of the blow.
    clang = _clank(child(r, 'clang'), [380.0, 340.0, 420.0, 360.0][v], 0.16, -2.0)
    x = fade(_tame(clang, 80.0), 0.0, 0.05)
    return S.widen(normalize(x), child(r, 'w'), 0.25)


@cue('Ember.IronHide.Burn', variations=3, att='Near', jitter=0.06, conc=2, level=-12.0)
def hide_burn(v, r):
    # Teeth on hot iron: a short fierce sizzle and a clank.
    clank = _clank(child(r, 'clank'), 480.0 * jitter(r, 1, 0.08), 0.1, -6.0)
    sizzle = _sizzle(0.5, child(r, 'sizzle'), 13000.0, 0.12)
    puff = _steam(0.35, child(r, 'puff'), 0.08, 0.6)
    x = layers((clank, 0.0, -3.0), (sizzle, 0.005, 0.0), (puff, 0.03, -10.0))
    return _yard(_tame(normalize(x), 120.0), r, -17.0, 0.9)


@cue('Ember.IronHide.Unlock', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-10.0)
def hide_unlock(v, r):
    # Steam let out in a long sigh, the plates sliding back with an iron scrape, a last clank.
    valve = kit.metal_click(child(r, 'valve'), 900.0, 4000.0, 0.04, 0.0002, 5, -10.0, 0.4)
    sigh = _steam(0.9, child(r, 'sigh'), 0.03, 0.7, 900.0, 7000.0)
    scrape = kit.scrape(0.35, child(r, 'scrape'), 900.0, 600.0, 12.0, 0.5)
    last = _clank(child(r, 'last'), 280.0, 0.25, -6.0)
    x = layers((valve, 0.0, -2.0), (S.widen(sigh, child(r, 'sw'), 0.6), 0.005, 0.0), (S.pan(scrape, 0.2), 0.12, -9.0), (last, 0.45, -5.0))
    return _engine(_tame(x, 50.0), r, -18.0, 1.0)


# --- Rail Spike ------------------------------------------------------------------------------------------------------------

@cue('Ember.RailSpike.Fire', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-5.0)
def spike_fire(v, r):
    # A steam piston's bang (a sharp pressure blast through a pipe: a short hard shove and its crack), the spike's iron
    # ring as it leaves, a long hiss of spent steam after.
    from recipes import guns as Gn
    blast = Gn._loud(Gn._friedlander(child(r, 'blast'), 0.0025 * jitter(r, 1.0, 0.1), 0.7, 0.04, 0.3), 0.003)
    shove = kit.thump(95.0, 40.0, 0.3, 0.18, drive_db=4.0)
    ring = normalize(M.strike(M.bar(1150.0 * jitter(r, 1, 0.04), child(r, 'ring'), 0.5, 0.6, 5), M.hammer(0.0001), 0.7))
    whoosh = kit.whoosh(0.25, child(r, 'whoosh'), 2500.0, 600.0, 1.0, 0.15, -3.0)
    hiss = _steam(1.0, child(r, 'hiss'), 0.05, 0.7)
    x = layers((blast, 0.0, 0.0), (shove, 0.0, -3.0), (ring, 0.004, -10.0), (S.widen(whoosh, child(r, 'ww'), 0.4), 0.01, -10.0),
               (S.widen(hiss, child(r, 'hw'), 0.6), 0.03, -12.0))
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _engine(_tame(x, 35.0), r, -14.0, 1.4)


@cue('Ember.RailSpike.Hit', variations=4, att='Creature', jitter=0.05, conc=3, level=-9.0)
def spike_hit(v, r):
    # Through a body: a heavy iron punch, a crunch, a thud.
    punch = _clank(child(r, 'punch'), 600.0 * jitter(r, 1, 0.08), 0.06, -2.0)
    crunch = kit.grit_burst(0.12, child(r, 'crunch'), 6000.0, 600.0, 4000.0, 0.06)
    thud = kit.noise_thump(0.2, child(r, 'thud'), 500.0, 70.0, 0.09)
    x = layers((thud, 0.0, 0.0), (punch, 0.0, -2.0), (crunch, 0.004, -6.0))
    return _yard(_tame(normalize(x), 60.0), r, -16.0, 1.0)


@cue('Ember.RailSpike.Embed', variations=3, att='Creature', jitter=0.05, conc=2, level=-11.0)
def spike_embed(v, r):
    # Iron biting into rock: a hard crack and grit thrown, then the spike ringing out where it stands.
    crack = kit.burst(0.02, child(r, 'crack'), 0.008, lo=800.0, hi=7000.0, attack=0.0002)
    grit = kit.grit_burst(0.35, child(r, 'grit'), 5000.0, 700.0, 5000.0, 0.15)
    ring = normalize(M.strike(M.bar(950.0 * jitter(r, 1, 0.05), child(r, 'ring'), 0.9, 0.6, 5), M.hammer(0.0001), 1.2))
    knock = kit.noise_thump(0.15, child(r, 'knock'), 900.0, 150.0, 0.06)
    x = layers((crack, 0.0, 0.0), (knock, 0.0, -3.0), (grit, 0.005, -9.0), (ring, 0.003, -6.0))
    return _yard(_tame(normalize(x), 90.0), r, -15.0, 1.2)
