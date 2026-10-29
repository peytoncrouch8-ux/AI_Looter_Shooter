"""The hit-reaction pass's sounds (HITS, 2026-10-28; Docs/Work/2026-10-28-Core/HITS.md; names in CreatureHitCue,
Creatures/CreatureHitCues.h). Each plays on top of the hit sounds that already play (Creature.<Kind>.Hit, Crit.*,
Hit.Heavy), at the creature, 3D ('Creature' attenuation, about 35 m) like every other creature cue. Stylized, no gore:
chitin cracking, a ghost's hollow gasp, an ember guttering, a soul tearing like wind through cloth, a wet gel pop. All
synthesized (no recordings), on creatures.py's materials so a kind's new sounds are made of what its old ones are.

    Creature.Spider.LegCrack    a leg hit: a thin chitin segment snapping (a dry, hollow tube crack), 6 dB under the hit
    Creature.Spider.Stagger     staggered: a hiss-shriek (stridulation torn into a rising, then collapsing hiss)
    Creature.Unpaid.HatOff      a hat knocked off: the felt crown's dull thump, the brim flapping as it spins away
                                (the softer takes are a shroud's cloth flap, which plays the same cue)
    Creature.Unpaid.Stagger     staggered: a choked, hollow gasp (breath caught in an empty throat)
    Creature.Unpaid.Gutter      a wounded coal guttering (one-shot, the game plays it about every 3 s): a sputter of
                                sparks over a low flame flutter, dying back
    Creature.Unpaid.Snuff       snuffed by a coal crit: the ember pinched out (a sharp suck of air, a hiss quenching to
                                nothing), the hollow body letting go
    Creature.Unpaid.Tear        torn apart by a blast: the soul ripped like wind through old cloth, a gust carrying the
                                tatters off, the voices flung after it
    Creature.Slime.Stagger      staggered: a wet wobble, the jelly quivering flat
    Creature.Slime.Pop          popped by a core crit: the gel's skin bursting with a deep wet pop, the jelly
                                deflating with a gulp, drips
    Creature.Hit.Overkill       a kill far past what was needed, on a blast kill's sounds: a heavy concussive whump and
                                the body's matter thrown wide (a wet, gritty scatter flying downrange)

Leg cracks and overkills come often in a fight: their 2-5 kHz band is eased (check.py's frequent list).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, reverse
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.creatures import (rasp, chitter, _chitin, _outside, _haunt, _choir, _cold, _hollow, _wet_slap, _squish,
                               _bloop, _wobble, _blorp, _bubbles)


def _ease(x, cut_db=-5.0, shelf_db=-3.0):
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(8000.0, shelf_db), extend=False)


def _fade(x, sec=0.03):
    return x * E.end_fade(np.ones(x.shape[-1]), sec)


# --- Spiders ---------------------------------------------------------------------------------------------------------

@cue('Creature.Spider.LegCrack', variations=5, att='Creature', jitter=0.06, conc=4, level=-16.0)
def spider_leg_crack(v, r):
    # A round through a leg: a thin chitin tube snapping. Higher and hollower than the body's shell (a narrow segment,
    # its knock up near 1.1 kHz), two or three brittle snaps as the crack runs round it, the joint's tendon twanging
    # dully, a short crunch, and the leg's hollow ringing a moment (so it ends softly, not on the snap).
    f0 = [1050.0, 1180.0, 980.0, 1250.0, 1120.0][v] * jitter(r, 1.0, 0.04)
    tube = kit.thunk(f0, child(r, 'tube'), 0.022, 0.0001, 6, 0.1)
    snaps = np.zeros(1)
    t = 0.0
    for i in range(2 + int(r.integers(0, 2))):
        s = normalize(M.strike(M.parts(1400.0, 6500.0, child(r, 'snap', i), 4, 0.006, 0.003), M.hammer(0.00004), 0.02))
        snaps = layers(snaps, (s, t, -2.0 - 5.0 * i))
        t += r.uniform(0.004, 0.011)
    tendon = kit.thunk(380.0 * jitter(r, 1.0, 0.08), child(r, 'tendon'), 0.04, 0.0008, 4, 0.04)
    crunch = kit.grit_burst(0.035, child(r, 'crunch'), 9000.0, 1200.0, 7000.0, 0.018, 0.0004)
    hollow = M.strike(M.parts(f0 * 0.6, f0 * 1.9, child(r, 'hollow'), 5, 0.06, 0.03, 0.4), M.hammer(0.0002), 0.12)
    x = layers((tube, 0.0, 0.0), (normalize(snaps), 0.0, -2.0), (tendon, 0.003, -12.0), (crunch, 0.001, -9.0),
               (normalize(hollow), 0.004, -16.0))
    x = _fade(_ease(F.filt(x, F.hp(180.0), extend=False), -6.0, -4.0), 0.02)
    return _outside(x, r, -24.0, 0.05, 0.13, 0.25)


@cue('Creature.Spider.Stagger', variations=4, att='Creature', jitter=0.05, conc=3, level=-8.0)
def spider_stagger(v, r):
    # Staggered: a hiss-shriek. The mandibles clack once (it opens at once), the stridulation file scraped hard and fast
    # rising to a rasp that tears into a hiss as it rises (the breath forced out through the spiracles), then it
    # collapses: the hiss sinking and the rasp slowing to a few clicks.
    dur = 0.55 + 0.08 * r.random()
    clack = _chitin(child(r, 'clack'), 820.0 * jitter(r, 1.0, 0.08), 0.02, 2)
    scr = rasp(dur * 0.8, child(r, 'rasp'), 90.0, 260.0, (1200.0, 2300.0, 3600.0), 3.0)
    scr = scr * E.bp([(0.0, 0.3), (0.35 * dur, 1.0), (0.8 * dur, 0.0)], n=scr.size, curve='cos')
    n = ns(dur)
    t = times(n)
    fc = np.interp(t, [0.0, 0.3 * dur, dur], [1800.0, 4800.0, 1500.0])
    hiss = F.sweep(N.white(n, child(r, 'hiss')), 'bp', fc, 1.4) * kit.turbulence(n, child(r, 'ht'), 140.0, 0.5)
    hiss = normalize(hiss) * E.bp([(0.0, 0.0), (0.04, 0.4), (0.3 * dur, 1.0), (dur, 0.0)], n=n, curve='cos')
    clicks = chitter(dur * 0.5, child(r, 'clicks'), 30.0, 12.0, 1500.0, 4200.0)
    x = layers((clack, 0.0, 0.0), (normalize(scr), 0.005, -4.0), (normalize(hiss), 0.0, -7.0), (clicks, 0.45 * dur, -14.0))
    x = _fade(F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(250.0), extend=False), 0.04)
    return _outside(x, r, -20.0)


# --- The Unpaid ------------------------------------------------------------------------------------------------------

@cue('Creature.Unpaid.HatOff', variations=5, att='Creature', jitter=0.06, conc=3, level=-12.0)
def unpaid_hat_off(v, r):
    # A hat knocked off: the round's dull thump into the felt crown (dense, soft, no ring), the brim flapping as the
    # hat spins away (cloth slapped by the air a few times, each fainter) and a short whirl of air. Takes 3 and 4 are
    # softer, more cloth than felt: the same cue plays when a round tugs a shroud.
    shroud = v >= 3
    thump = kit.noise_thump(0.12, child(r, 'felt'), 1400.0 if not shroud else 1900.0, 180.0, 0.05, 0.8, 0.03, 0.0006)
    knock = kit.thunk(240.0 * jitter(r, 1.0, 0.08), child(r, 'crown'), 0.03, 0.002, 4, 0.05)
    flaps = np.zeros(1)
    t = 0.02
    gap = 0.045 + 0.015 * r.random()
    for i in range(3 + int(r.integers(0, 2))):
        f = kit.cloth(0.05, child(r, 'flap', i), 300.0, 2600.0, 220.0, 0.12, 0.5, -2.0)
        flaps = layers(flaps, (f, t, -3.0 * i))
        t += gap * (1.0 + 0.25 * i)
    whirl = kit.whoosh(0.3, child(r, 'whirl'), 500.0, 1600.0, 1.1, 0.35, -3.0, 1.0)
    if shroud:
        x = layers((thump, 0.0, -4.0), (normalize(flaps), 0.0, 0.0), (whirl, 0.0, -14.0))
    else:
        x = layers((thump, 0.0, 0.0), (knock, 0.0, -6.0), (normalize(flaps), 0.01, -5.0), (whirl, 0.03, -10.0))
    x = _fade(_ease(F.filt(x, F.hp(90.0), extend=False), -5.0, -4.0), 0.03)
    return _haunt(x, r, -20.0, 0.6, 2800.0)


@cue('Creature.Unpaid.Stagger', variations=4, att='Creature', jitter=0.04, conc=3, level=-9.0)
def unpaid_stagger(v, r):
    # Staggered: a choked gasp from an empty throat. The hollow body knocked (a cold tube's thump starts it), breath
    # dragged in hard through a closing throat (an 'h' tightening to 'u', rising), a voice catching on it for a moment
    # and choking off (a short cry of several voices cut by a flutter), then a little air let out.
    dur = 0.5
    f0 = [170.0, 190.0, 155.0, 205.0][v] * jitter(r, 1.0, 0.03)
    knock = _cold(_hollow(child(r, 'knock'), f0, 0.08), child(r, 'kc'), 31.0, 0.25)
    gasp = V.whisper(0.32, [(0.0, 'h'), (0.15, 'a'), (0.32, 'u')], child(r, 'gasp'), 1.15)
    gasp = normalize(gasp) * E.bp([(0.0, 0.0), (0.02, 0.5), (0.24, 1.0), (0.32, 0.0)], n=gasp.size, curve='cos')
    f = 330.0 * jitter(r, 1.0, 0.06)
    cry = _choir(child(r, 'cry'), [(0.0, f), (0.1, f * 1.15), (0.2, f * 0.8)], 0.2, [(0.0, 'a'), (0.2, 'u')], 2, 20.0,
                 0.01, 0.6, 1.1, (7.0, 0.4), 0.4)
    n = cry.size
    choke = 0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 26.0 * times(n)))
    cry = cry * F.filt(choke, F.lp1(300.0), extend=False) * E.perc(n / SR, 0.01, 0.12)
    out = V.whisper(0.2, [(0.0, 'u'), (0.2, 'h')], child(r, 'out'), 1.1)
    out = normalize(out) * E.bp([(0.0, 0.0), (0.05, 1.0), (0.2, 0.0)], n=out.size, curve='cos')
    x = layers((normalize(knock), 0.0, -4.0), (gasp, 0.004, -2.0), (normalize(cry), 0.2, -6.0), (out, 0.36, -12.0))
    x = _fade(F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False), 0.03)
    return _haunt(x, r, -12.0, 1.1)


def _flame(n, r, rate=9.0):
    """A small flame's flutter: low noise in a dark band, its level jumping at a few times a second."""
    x = F.filt(N.pink(n, child(r, 'flame')), F.bp(420.0, 0.9), F.lpn(1200.0, 2), extend=False)
    flick = 0.35 + 0.65 * np.abs(N.smooth_random(n, child(r, 'flick'), rate)) ** 0.8
    return normalize(x * flick)


@cue('Creature.Unpaid.Gutter', variations=5, att='Creature', jitter=0.06, conc=3, level=-17.0)
def unpaid_gutter(v, r):
    # A wounded Unpaid's coal guttering: a soft pop as a crack opens in it, a sputter of sparks (dense, thinning but
    # never to lone grains, with a fine hiss under them so the quiet tail doesn't click), the coal's little flame
    # fluttering low and dying back. Played about every 3 s on a wounded one, so it stays quiet and short.
    dur = 0.8 + 0.15 * r.random()
    n = ns(dur)
    t = times(n)
    pop = kit.noise_thump(0.06, child(r, 'pop'), 1600.0, 220.0, 0.02, 0.9, 0.015, 0.0004)
    sparks = G.grit(dur, child(r, 'sparks'), 2200.0 * np.exp(-t / 0.18) + 900.0, 2800.0, 9000.0, 3.0, 3)
    hiss = N.band(n, child(r, 'hiss'), 3500.0, 9000.0)
    env = E.bp([(0.0, 1.0), (0.12 * dur, 0.7), (dur, 0.0)], n=n, curve='cos')
    sparks = layers(normalize(sparks), (normalize(hiss), 0.0, -10.0)) * env
    flame = _flame(n, child(r, 'fl'), 7.0 + 3.0 * r.random()) * E.bp([(0.0, 0.0), (0.03, 1.0), (0.5 * dur, 0.6),
                                                                      (dur, 0.0)], n=n, curve='cos')
    x = layers((pop, 0.0, 0.0), (sparks, 0.002, -10.0), (flame, 0.0, -6.0))
    x = _fade(_ease(F.filt(x, F.hp(110.0), extend=False), -6.0, -2.0), 0.05)
    return _haunt(x, r, -16.0, 0.7, 3000.0)


@cue('Creature.Unpaid.Snuff', variations=4, att='Creature', jitter=0.04, conc=3, level=-8.0)
def unpaid_snuff(v, r):
    # Snuffed: the coal pinched out by the crit. A sharp crack of the coal splitting, the air sucked in at once (a
    # reversed puff, rising), a hiss quenching to nothing (its band closing down), the hollow body's knock as it lets
    # go and a last breath leaving it, cold.
    crack = kit.burst(0.004, child(r, 'crack'), 0.0012, lo=1500.0, hi=9000.0)
    coal = M.strike(M.parts(800.0, 5200.0, child(r, 'coal'), 6, 0.02, 0.008, 0.4), M.hammer(0.00005), 0.06)
    suck = reverse(kit.whoosh(0.16, child(r, 'suck'), 2200.0, 600.0, 1.0, 0.25, -3.0, 1.0))
    n = ns(0.5)
    t = times(n)
    fc = 6500.0 * np.exp(-t / 0.12) + 500.0
    quench = F.sweep(N.white(n, child(r, 'quench')), 'bp', fc, 1.2) * kit.turbulence(n, child(r, 'qt'), 90.0, 0.5)
    quench = normalize(quench) * E.bp([(0.0, 0.0), (0.015, 1.0), (0.15, 0.6), (0.5, 0.0)], n=n, curve='cos')
    f0 = [140.0, 160.0, 130.0, 150.0][v] * jitter(r, 1.0, 0.03)
    body = _cold(_hollow(child(r, 'body'), f0, 0.14), child(r, 'bc'), 29.0, 0.25)
    breath = V.whisper(0.45, [(0.0, 'o'), (0.45, 'h')], child(r, 'breath'), 1.0)
    breath = normalize(breath) * E.bp([(0.0, 0.0), (0.08, 1.0), (0.45, 0.0)], n=breath.size, curve='cos')
    x = layers((crack, 0.0, -4.0), (normalize(coal), 0.0, -6.0), (suck, 0.0, -8.0), (quench, 0.12, -6.0),
               (normalize(body), 0.15, -3.0), (breath, 0.25, -12.0))
    x = _fade(F.filt(x, F.peak(3200.0, -4.0, 0.8), F.hp(110.0), extend=False), 0.04)
    return _haunt(x, r, -11.0, 1.4, 3000.0)


@cue('Creature.Unpaid.Tear', variations=5, att='Creature', jitter=0.04, conc=3, level=-7.0)
def unpaid_tear(v, r):
    # Torn apart by a blast: the soul ripped like wind through old cloth. The rip itself (cloth tearing: stick-slip
    # bursts racing through a fabric band, a second rip right behind it), a gust tearing past and carrying the tatters
    # off (a whoosh sweeping up), loose strips flapping in it, the Unpaid's voices flung after it and falling away, and
    # a hollow knock of the body giving under the blast.
    dur = 1.2
    rip = kit.creak(0.22, child(r, 'rip'), 900.0, 350.0, (900.0, 1700.0, 2900.0), 2.2, 0.5)
    rip2 = kit.creak(0.16, child(r, 'rip2'), 700.0, 300.0, (750.0, 1450.0, 2500.0), 2.2, 0.5)
    n = ns(0.25)
    shred = N.band(n, child(r, 'shred'), 700.0, 5000.0) * (0.2 + 0.8 * np.abs(N.smooth_random(n, child(r, 'st'), 260.0)))
    shred = normalize(shred) * E.perc(0.25, 0.001, 0.14)
    gust = kit.whoosh(dur, child(r, 'gust'), 300.0, 2600.0, 0.8, 0.22, -2.0, 0.8)
    flaps = np.zeros(1)
    t = 0.12
    for i in range(5):
        f = kit.cloth(0.05, child(r, 'flap', i), 400.0, 3000.0, 260.0, 0.12, 0.5, -2.0)
        flaps = layers(flaps, (f, t, -2.5 * i))
        t += 0.06 + 0.04 * r.random()
    f = [400.0, 370.0, 430.0, 390.0, 415.0][v] * jitter(r, 1.0, 0.03)
    wail = _choir(child(r, 'wail'), [(0.0, f * 1.2), (0.08, f * 1.35), (0.9, f * 0.55)], 0.9, [(0.0, 'a'), (0.9, 'u')],
                  3, 26.0, 0.03, 0.65, 1.1, (6.0, 0.4), 0.2)
    wail = wail * E.bp([(0.0, 0.0), (0.04, 1.0), (0.9, 0.0)], n=wail.size, curve='cos')
    knock = _cold(_hollow(child(r, 'knock'), 120.0 * jitter(r, 1.0, 0.05), 0.12), child(r, 'kc'), 33.0, 0.25)
    x = layers((normalize(shred), 0.0, -2.0), (rip, 0.0, 0.0), (rip2, 0.07 + 0.03 * r.random(), -4.0),
               (normalize(knock), 0.0, -4.0), (gust, 0.02, -5.0), (normalize(flaps), 0.0, -12.0),
               (normalize(wail), 0.1, -9.0))
    x = _fade(F.filt(x, F.peak(3200.0, -4.0, 0.8), F.hp(120.0), extend=False), 0.06)
    return _haunt(x, r, -10.0, 1.6, 3000.0)


# --- The meadow slime ------------------------------------------------------------------------------------------------

@cue('Creature.Slime.Stagger', variations=4, att='Creature', jitter=0.06, conc=3, level=-11.0)
def slime_stagger(v, r):
    # Staggered: a wet wobble. A soft slap as it's knocked flat, the jelly quivering fast (its surface's wet noise through
    # a resonance swinging at 14 Hz, the body's low note under it), squishing through itself, a low bloop as the core
    # sinks, and the quiver dying.
    slap = _wet_slap(0.06, child(r, 'slap'), 200.0, 2600.0, 0.03, 0.0008)
    wob = _wobble(0.55, child(r, 'wob'), 300.0 * jitter(r, 1.0, 0.08), 13.0 + 2.0 * r.random(), 0.42, 0.45, -6.0)
    squish = _squish(0.25, child(r, 'squish'), lambda t: 320.0 * np.exp(-t / 0.1), 500.0, 2000.0)
    plop = _bloop(110.0 * jitter(r, 1.0, 0.08), 210.0, 0.16, child(r, 'plop'), 0.14)
    x = layers((slap, 0.0, -2.0), (wob, 0.0, 0.0), (squish, 0.004, -10.0), (plop, 0.08, -5.0))
    x = _fade(F.filt(x, F.lp(3800.0, 0.7), F.hp(70.0), extend=False), 0.04)
    return _outside(x, r, -22.0)


@cue('Creature.Slime.Pop', variations=5, att='Creature', jitter=0.06, conc=3, level=-8.0)
def slime_pop(v, r):
    # Popped by a core crit: the gel's skin bursting under the pressure (a deep wet pop: a big bubble's ring snapping
    # open on a soft slap), the jelly deflating (a gulping blorp sinking, air squeezed out through it as a wet hiss),
    # the core landing in the puddle with a plop, a few drips.
    pop = kit.bubble(r.uniform(150.0, 210.0), child(r, 'pop'), 0.06)
    slap = _wet_slap(0.08, child(r, 'slap'), 150.0, 3000.0, 0.04, 0.0006)
    weight = kit.noise_thump(0.18, child(r, 'weight'), 800.0, 75.0, 0.09, 1.0, 0.05)
    blorp = _blorp(260.0 * jitter(r, 1.0, 0.08), 80.0, 0.35, child(r, 'blorp'), 0.3)
    n = ns(0.4)
    air = F.sweep(N.pink(n, child(r, 'air')), 'bp', np.linspace(1600.0, 500.0, n), 2.0)
    air = normalize(air * kit.turbulence(n, child(r, 'at'), 160.0, 0.7)) * E.bp([(0.0, 0.0), (0.05, 1.0), (0.4, 0.0)],
                                                                              n=n, curve='cos')
    squish = _squish(0.3, child(r, 'squish'), lambda t: 420.0 * np.exp(-t / 0.12), 450.0, 1900.0)
    plop = _bloop(90.0 * jitter(r, 1.0, 0.06), 200.0, 0.18, child(r, 'plop'), 0.16)
    drips = layers(*[(kit.bubble(r.uniform(800.0, 1700.0), r, 0.12), 0.55 + 0.1 * i + 0.04 * r.random(), -5.0 * i)
                     for i in range(3)])
    x = layers((normalize(pop), 0.0, 0.0), (slap, 0.0, -3.0), (weight, 0.0, -4.0), (blorp, 0.03, -4.0), (air, 0.05, -12.0),
               (squish, 0.01, -10.0), (plop, 0.4, -7.0), (normalize(drips), 0.0, -16.0))
    x = _fade(F.filt(x, F.lp(3800.0, 0.7), F.hp(55.0), extend=False), 0.05)
    return _outside(x, r, -21.0)


# --- Any creature ----------------------------------------------------------------------------------------------------

@cue('Creature.Hit.Overkill', variations=5, att='Creature', jitter=0.05, conc=3, level=-9.0)
def hit_overkill(v, r):
    # Far more than needed: a heavy concussive whump through the body (a deep, dense thud of air, not a tone), a
    # crunchy wide scatter of the body's stuff flying downrange (grit thrown, fast then thinning, with a wet edge), and a
    # rush of air after it. Kind-neutral: the kind's own burst plays with it.
    whump = kit.noise_thump(0.3, child(r, 'whump'), 900.0, 60.0, 0.14, 1.1, 0.06, 0.0006)
    thud = kit.thump(120.0 * jitter(r, 1.0, 0.08), 48.0, 0.18, 0.09, drive_db=5.0)
    scatter = kit.grit_burst(0.35, child(r, 'scatter'), 7000.0, 600.0, 5500.0, 0.18, 0.001)
    wet = kit.squelch(0.2, child(r, 'wet'), 1300.0 * jitter(r, 1.0, 0.1), 300.0, 2.2)
    rush = kit.whoosh(0.45, child(r, 'rush'), 900.0, 250.0, 0.9, 0.15, -3.0, 1.0)
    x = layers((whump, 0.0, 0.0), (thud, 0.0, -4.0), (scatter, 0.004, -8.0), (wet, 0.008, -10.0), (rush, 0.01, -12.0))
    x = D.drive(normalize(x), 3.0, 'tanh')
    x = _fade(_ease(F.filt(x, F.hp(40.0), extend=False), -6.0, -4.0), 0.05)
    return _outside(x, r, -18.0, 0.07, 0.2, 0.6)
