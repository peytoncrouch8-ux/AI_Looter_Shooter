"""Creatures noticing, stirring, rising and crumbling (all 3D, played through each creature's own voice; the game pitches
them for the creature's size). The kinds' voices are creatures.py's, the dead miners' miners.py's, the bell's bells.py's:
these are the small moments between their big cues.

- Creature.Spider.Notice (0.5 s): rearing at the sight of you: forelegs tapping down and lifting, chitin scraping on
  chitin (a ridged file drawn across a pick, rising), a dry hiss of air from the spiracles opening.
- Creature.Spider.Emerge (0.8 s): out of the ground: the crust splitting, soil heaving and crumbling off, legs scrabbling
  for a hold (quicker, then slower), the shell knocking free, grit pattering down.
- Creature.Spider.Twitch (quiet, 0.15 s): a dead spider's curled leg twitching: one faint dry tick of chitin and a smaller
  one as the joint settles. Heard often near a fallen nest, so it stays soft and off the 2-5 kHz band.
- Creature.Spider.Thud (0.5 s, 4 takes, each a little heavier): a dying spider's body slamming down at the end of its throe,
  on its death cry's knock: the weight arriving (a low thump, a dark thud of air), the shell's hollow chitin clack (the
  thorax, then the abdomen a beat later), the legs knocking down round it, dust and grit and a few clods pattering back.
- Creature.Slime.Notice (0.4 s): the jelly shivering as it senses you: a wet shudder (the surface noise swinging through a
  resonance, fast), tiny pops, a small bloop.
- Creature.Slime.Bubble (quiet, 0.3 s): a bubble rising through gel: a soft gloop that climbs, a smaller one behind it, the
  surface popping.
- Creature.Slime.Swell (0.5 s): a slime drawing itself up out of a puddle: a wet suck (gel pulled out thin), the plop as it
  lets go, the blob wobbling and settling.
- Creature.Unpaid.Notice (0.5 s): the coal in its chest flaring as it spots you: a soft whoomph of ignition, a flare of
  gas, a breath drawn in with a cold edge.
- Creature.Unpaid.Ash (1.4 s): breaking into ash: the body folding with a puff, a dry papery crumble, flakes ticking, a
  soft hiss of fine ash settling and a sigh.
- Creature.DeadMiner.Notice (0.6 s): iron dragged on stone (the pick's head scraping the floor), a gravelly grunt, the
  haft knocking in its grip; in the gallery's bare rock.
- Creature.BellRinger.Notice (0.5 s): one muffled clapper knock on the bronze hand bell, held against the ringer's coat.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M, voice as V, granular as G
from recipes import cue
from recipes import kit
from recipes.creatures import (rasp, _skitter, _outside, _haunt, _choir, _cold, _wet_slap, _squish, _bloop, _wobble,
                               _stretch, _bubbles)
from recipes.miners import gallery, pick_head, _haft
from recipes.bells import handbell, ring, PRIMES


def _scrabble(r, start, count, gap0, gap1, lo=800.0, hi=1500.0):
    """Legs scrabbling for a hold: chitin tips on dirt, each a tap and a claw's scratch, the pace gliding from gap0 to
    gap1 (seconds between taps), each tap a different strength."""
    out = np.zeros(1)
    t = start
    for i in range(count):
        u = i / max(count - 1, 1)
        tap = kit.thunk(r.uniform(lo, hi), child(r, 'tap', i), 0.012, 0.00015, 3, 0.1)
        claw = kit.grit_burst(0.018, child(r, 'claw', i), 6000.0, 1500.0, 5000.0, 0.008)
        out = layers(out, (layers(tap, (claw, 0.0, -8.0)), t, r.uniform(-10.0, 0.0)))
        t += (gap0 + (gap1 - gap0) * u) * r.uniform(0.7, 1.3)
    return normalize(out)


# --- Spiders ------------------------------------------------------------------------------------------------------------

@cue('Creature.Spider.Notice', variations=3, att='Creature', jitter=0.06, conc=3, level=-9.0)
def spider_notice(v, r):
    # It has seen you and rears: the forelegs tap down and lift, chitin scrapes on chitin as they rise, and a dry hiss
    # opens toward the end.
    touch = _skitter(child(r, 'touch'), 2 + v % 2, 0.0, 0.03)
    # The scrape is irregular (a wide rate wobble) and slower than the alert's rasp, so it reads as chitin dragged on
    # chitin, not as a ratchet.
    rs = kit.creak(0.26, child(r, 'rasp'), 42.0 * jitter(r, 1, 0.1), 95.0, (1200.0, 2500.0, 3900.0), 3.0, 0.55)
    n = ns(0.36)
    hs = V.hiss(n, child(r, 'hiss'), (2300.0, 3700.0, 6400.0), levels=(0.6, 1.0, 0.8))
    hs = normalize(hs * E.bp([(0.0, 0.0), (0.1, 0.35), (0.23, 1.0), (0.36, 0.0)], n=n, curve='cos')
                   * (0.8 + 0.2 * np.sin(2.0 * np.pi * 24.0 * times(n))))
    x = layers((touch, 0.0, -3.0), (rs, 0.02, -2.0), (hs, 0.08, -6.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r, -22.0, 0.05, 0.12, 0.25)


@cue('Creature.Spider.Emerge', variations=2, att='Creature', jitter=0.05, conc=3, level=-9.0)
def spider_emerge(v, r):
    # Out of the ground: the crust splits, soil heaves and crumbles off in lumps, legs scrabble for a hold (quick, then
    # slower), the shell knocks free, grit pours back. The soil is broken into crumbling patches so the legs stay clear.
    dur = 0.7
    n = ns(dur)
    t = times(n)
    crack = kit.burst(0.012, child(r, 'crack'), 0.005, lo=400.0, hi=3200.0, attack=0.0003)
    heave = kit.noise_thump(0.28, child(r, 'heave'), 420.0, 70.0, 0.14, 1.0, 0.16, 0.004)
    soil = N.shaped(n, child(r, 'soil'), -3.0, 90.0, 1800.0)
    crumble = np.clip(np.abs(N.smooth_random(n, child(r, 'sg'), 16.0)) * 2.2 - 0.25, 0.0, 1.0)
    soil *= crumble * E.bp([(0.0, 0.0), (0.02, 1.0), (0.3, 0.5), (0.62, 0.0), (dur, 0.0)], n=n, curve='cos')
    pour = G.grit(dur, child(r, 'pour'), 1800.0 * np.exp(-t / 0.3) + 150.0, 500.0, 3200.0, 2.0, 4)
    clods = np.zeros(1)
    for i in range(3 + int(r.integers(0, 3))):
        clods = layers(clods, (kit.noise_thump(0.07, child(r, 'clod', i), 700.0, 140.0, 0.03, 1.0, 0.03),
                               r.uniform(0.04, 0.5), r.uniform(-8.0, -2.0)))
    legs = _scrabble(child(r, 'legs'), 0.08, 9 + int(r.integers(0, 3)), 0.035, 0.065)
    body = kit.thunk(620.0 * jitter(r, 1, 0.1), child(r, 'body'), 0.03, 0.0003, 6, 0.12)
    x = layers((crack, 0.0, -4.0), (heave, 0.0, -9.0), (normalize(soil), 0.0, -11.0), (normalize(pour), 0.03, -10.0),
               (normalize(clods), 0.0, -9.0), (legs, 0.08, 0.0), (body, 0.45 + 0.05 * r.random(), -6.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r, -23.0, 0.05, 0.12, 0.25)


@cue('Creature.Spider.Twitch', variations=3, att='Near', jitter=0.08, conc=2, level=-24.0)
def spider_twitch(v, r):
    # A dead spider's curled leg twitching: one faint, dry tick of chitin (a few quick modes under 3 kHz and the shell's
    # hollow under them), and a smaller one as the joint settles.
    # The shell's hollow under the tick rings 50 ms (a quick decay in the cue's last 10 ms would read as cut off).
    def tick(rr, f, hollow_t60):
        m = M.parts(900.0, 2700.0, child(rr, 'p'), 4, 0.012, 0.005, 0.4)
        ring_ = normalize(M.strike(m, M.hammer(0.00012), 0.035), 0.0)
        hollow = normalize(kit.thunk(f, child(rr, 'k'), hollow_t60, 0.0003, 3, 0.1), 0.0)
        return layers((ring_, 0.0, 0.0), (hollow, 0.0, -4.0))
    gap = 0.045 + 0.035 * r.random()
    x = layers((tick(child(r, 't1'), r.uniform(650.0, 900.0), 0.025), 0.0, 0.0),
               (tick(child(r, 't2'), r.uniform(600.0, 800.0), 0.06), gap, -9.0 - 3.0 * r.random()))
    return F.filt(x, F.hp(250.0, 0.7), F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)


@cue('Creature.Spider.Thud', variations=4, att='Creature', jitter=0.05, conc=3, level=-10.0)
def spider_thud(v, r):
    # A dying spider's body slamming down at the end of its throe (the game plays it on its death cry's knock): the weight
    # arriving (a low thump falling away, a dark thud of air), the shell's hollow chitin clack as the thorax and then the
    # abdomen strike, a dry tick of chitin on it, the legs knocking down round it, a puff of dust and grit and a few clods
    # pattering back. Soft at the top: it lands under the cry, and a fallen nest hears it often. Each take a little heavier.
    size = jitter(r, 1.0, 0.06) * (0.92, 1.0, 1.06, 1.12)[v]
    mass = kit.thump(80.0 / size, 38.0, 0.45, 0.22, attack=0.0015, drive_db=2.0)
    body = kit.noise_thump(0.4, child(r, 'body'), 500.0 / size, 70.0, 0.17, 1.1, 0.06, 0.001)
    ground = kit.noise_thump(0.1, child(r, 'ground'), 950.0, 160.0, 0.03, 1.0, 0.02, 0.0008)
    core = layers((mass, 0.0, 0.0), (body, 0.0, -3.0), (ground, 0.0, -9.0))
    thorax = kit.thunk(560.0 * jitter(r, 1, 0.08) / size, child(r, 'thorax'), 0.04, 0.0004, 6, 0.12)
    abdomen = kit.thunk(380.0 * jitter(r, 1, 0.08) / size, child(r, 'abdomen'), 0.05, 0.0006, 6, 0.1)
    tick = normalize(M.strike(M.parts(900.0, 2600.0, child(r, 'tick'), 4, 0.012, 0.005, 0.4), M.hammer(0.00012), 0.035), 0.0)
    shell = layers((thorax, 0.0, 0.0), (tick, 0.001, -6.0), (abdomen, 0.012 + 0.012 * r.random(), -3.0))

    def leg(rr, i):
        return kit.thunk(rr.uniform(700.0, 1200.0), child(rr, 'leg', i), 0.012, 0.00015, 3, 0.1) * rr.uniform(0.5, 1.0)

    def clod(rr, i):
        return kit.thunk(rr.uniform(450.0, 1400.0), child(rr, 'clod', i), 0.012, 0.0008, 4, 0.15) * rr.uniform(0.4, 1.0)
    legs = normalize(G.rattle(0.25, child(r, 'legs'), 4 + int(r.integers(0, 3)), 0.14, leg, 0.02, 0.8), 0.0)
    clods = normalize(G.rattle(0.3, child(r, 'clods'), 4 + int(r.integers(0, 3)), 0.22, clod, 0.05, 0.8), 0.0)
    dust = kit.burst(0.35, child(r, 'dust'), 0.15, lo=160.0, hi=1400.0, color=-3.0, attack=0.002)
    grit = kit.grit_burst(0.28, child(r, 'grit'), 5000.0, 500.0, 2800.0, 0.14)

    # The shell, legs and dirt are set against the thud's loudest 10 ms, so they're heard through the weight.
    def lv(sig, rel_db):
        return kit.set_level(sig, core, rel_db, 0.01)
    x = layers((core, 0.0, 0.0), (lv(shell, -5.0), 0.002, 0.0), (lv(legs, -14.0), 0.0, 0.0), (lv(dust, -13.0), 0.003, 0.0),
               (lv(grit, -16.0), 0.004, 0.0), (lv(clods, -17.0), 0.0, 0.0))
    x = F.filt(x, F.highshelf(5000.0, -4.0), F.peak(3300.0, -3.0, 0.8), F.hp(30.0), extend=False)
    return _outside(x, r, -22.0, 0.05, 0.12, 0.3)


# --- Slimes -------------------------------------------------------------------------------------------------------------

@cue('Creature.Slime.Notice', variations=3, att='Creature', jitter=0.06, conc=3, level=-11.0)
def slime_notice(v, r):
    # The jelly shivers as it senses you: a small wet smack to start, then a fast trembling (the surface noise swinging
    # through its resonance), a scatter of tiny pops and a small bloop low in it.
    touch = _wet_slap(0.025, child(r, 'touch'), 250.0, 1800.0, 0.012)
    tremble = _wobble(0.38, child(r, 'tremble'), 520.0 * jitter(r, 1, 0.1), 19.0 + 7.0 * r.random(), 0.26, 0.5, -18.0)
    pops = _squish(0.25, child(r, 'pops'), lambda tt: 220.0 * np.exp(-tt / 0.12) + 20.0, 700.0, 2200.0)
    bloop = _bloop(150.0 * jitter(r, 1, 0.1), 280.0, 0.1, child(r, 'bloop'), 0.08)
    x = layers((touch, 0.0, -5.0), (tremble, 0.004, 0.0), (pops, 0.02, -8.0), (bloop, 0.1 + 0.05 * r.random(), -14.0))
    return _outside(F.filt(x, F.lp(3500.0, 0.7), extend=False), r, -24.0)


@cue('Creature.Slime.Bubble', variations=4, att='Near', jitter=0.08, conc=3, level=-19.0)
def slime_bubble(v, r):
    # A bubble rising through gel: a soft gloop that climbs as the bubble closes, a smaller one on its heels, and the
    # surface giving with a tiny wet pop. Quiet; they come and go all through a slime pack.
    # It ends on the gel's slow wobble, not on the pop: a cue's last 10 ms must not be a quick decay or the check reads it
    # as cut off.
    gloop = _bloop(r.uniform(190.0, 300.0), r.uniform(420.0, 580.0), 0.15 + 0.03 * r.random(), child(r, 'gloop'), 0.12)
    small = _bloop(r.uniform(400.0, 540.0), r.uniform(760.0, 960.0), 0.07, child(r, 'small'), 0.05)
    lip = kit.squelch(0.07, child(r, 'lip'), 900.0 * jitter(r, 1, 0.1), 380.0, 2.5)
    pop = kit.burst(0.006, child(r, 'pop'), 0.003, lo=500.0, hi=2600.0, attack=0.0004)
    body = kit.noise_thump(0.1, child(r, 'body'), 420.0, 110.0, 0.04, 1.0, 0.03, 0.002)
    t2 = 0.07 + 0.05 * r.random()
    wob = _wobble(0.22, child(r, 'wob'), 300.0 * jitter(r, 1, 0.1), 13.0 + 3.0 * r.random(), 0.15, 0.35, -12.0)
    x = layers((gloop, 0.0, 0.0), (body, 0.0, -9.0), (lip, 0.01, -12.0), (small, t2, -8.0),
               (pop, t2 + 0.04 + 0.03 * r.random(), -12.0), (wob, t2 + 0.02, -16.0))
    return F.filt(x, F.lp(2800.0, 0.7), F.hp(90.0), extend=False)


@cue('Creature.Slime.Swell', variations=2, att='Creature', jitter=0.06, conc=3, level=-11.0)
def slime_swell(v, r):
    # A slime drawing itself up out of its puddle: a sticky smack as the surface lets go, a wet suck (gel pulled out thin,
    # its resonance climbing), the plop as it lets go, the blob wobbling and settling with a bubble or two.
    smack = _wet_slap(0.03, child(r, 'smack'), 300.0, 2200.0, 0.012)
    suck = _stretch(0.26, child(r, 'suck'), 220.0 * jitter(r, 1, 0.1), 820.0)
    t = 0.22 + 0.03 * r.random()
    plop = layers((_bloop(150.0 * jitter(r, 1, 0.08), 330.0, 0.14, child(r, 'plop'), 0.1), 0.0, 0.0),
                  (_wet_slap(0.05, child(r, 'slap'), 150.0, 1800.0, 0.03), 0.0, -3.0),
                  (kit.noise_thump(0.1, child(r, 'weight'), 520.0, 85.0, 0.05, 1.0, 0.04), 0.0, -4.0))
    wob = _wobble(0.2, child(r, 'wob'), 330.0 * jitter(r, 1, 0.1), 11.0 + 3.0 * r.random(), 0.15)
    bub = _bubbles(0.14, child(r, 'bub'), 25.0, 350.0, 1100.0)
    x = layers((smack, 0.0, -6.0), (suck, 0.0, -2.0), (plop, t, 0.0), (wob, t + 0.02, -10.0), (bub, t + 0.08, -13.0))
    return _outside(F.filt(x, F.lp(3300.0, 0.7), extend=False), r, -24.0, 0.06, 0.14, 0.3)


# --- The Unpaid ---------------------------------------------------------------------------------------------------------

@cue('Creature.Unpaid.Notice', variations=3, att='Creature', jitter=0.05, conc=3, level=-8.0)
def unpaid_notice(v, r):
    # The coal in its chest flares as it spots you: a soft whoomph of ignition (air shoved by a flame catching), a flare
    # of gas rising through the dark, embers snapping, and a breath drawn in, cold at the edge.
    whoomph = kit.noise_thump(0.38, child(r, 'whoomph'), 420.0 * jitter(r, 1, 0.08), 55.0, 0.2, 1.0, 0.2, 0.006)
    flare = kit.whoosh(0.45, child(r, 'flare'), 220.0, 2600.0, 0.9, 0.3, -3.0, 0.9)
    flare = F.filt(flare, F.lp(4200.0, 0.7), extend=False)
    m = ns(0.4)
    embers = G.grit(0.4, child(r, 'ember'), 260.0 * np.exp(-times(m) / 0.15) + 40.0, 1600.0, 5500.0, 3.0, 3)
    embers = F.filt(embers, F.lp(5000.0), extend=False)
    pop = kit.burst(0.008, child(r, 'pop'), 0.003, lo=500.0, hi=3000.0, attack=0.0003)
    d = 0.46
    draw = V.whisper(d, [(0.0, 'h'), (0.3, 'u'), (d, 'o')], child(r, 'draw'), 0.95)
    draw = normalize(F.filt(draw * E.bp([(0.0, 0.1), (0.3, 1.0), (d, 0.0)], n=draw.size, curve='cos'), F.hp(300.0), F.lp(5500.0),
                            extend=False))
    draw = _cold(draw, child(r, 'cold'), 27.0 + 6.0 * r.random(), 0.15)
    # Mixed by loudness against the whoomph (noise layers normalised by peak would bury the ignition).
    def lv(sig, rel_db):
        return kit.set_level(sig, whoomph, rel_db, 0.01)
    x = layers((pop, 0.0, -7.0), (whoomph, 0.0, 0.0), (lv(flare, -5.0), 0.02, 0.0), (lv(normalize(embers), -11.0), 0.01, 0.0),
               (lv(draw, -6.0), 0.14, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return _haunt(x, r, -17.0, 0.3, 3000.0)


@cue('Creature.Unpaid.Ash', variations=2, att='Creature', jitter=0.05, conc=3, level=-11.0)
def unpaid_ash(v, r):
    # Breaking into ash: the body folds with a soft puff, a dry papery crumble runs through it, flakes tick as they break
    # off, a fine hiss of ash keeps settling as long as they do, and a sigh goes out of it. The flakes' rate never falls
    # under 500 a second and the hiss lasts as long, or each lone flake would stand out of the quiet tail like a click.
    dur = 1.4
    n = ns(dur)
    puff = kit.noise_thump(0.22, child(r, 'puff'), 360.0 * jitter(r, 1, 0.08), 70.0, 0.1, 1.0, 0.1, 0.002)

    def crinkle(rr, tt, i):
        # One scrap of paper giving way: a few milliseconds of band noise, torn by a fast random swell.
        g = rr.uniform(0.006, 0.025)
        k = ns(g)
        c = N.band(k, rr, rr.uniform(1500.0, 2600.0), rr.uniform(5000.0, 8000.0)) * (0.3 + 0.7 * np.abs(N.smooth_random(k, rr, 300.0)))
        return c * E.perc(g, 0.001, g * 0.6) * rr.uniform(0.3, 1.0)
    crumble = G.cloud(dur, child(r, 'crumble'), lambda tt: 90.0 * np.exp(-tt / 0.5) + 8.0, crinkle, lambda tt: np.exp(-tt / 0.9), 0.0)
    crumple = kit.cloth(0.7, child(r, 'crumple'), 1400.0, 6500.0, 260.0, 0.1, 0.6)
    hiss = F.filt(N.white(n, child(r, 'hiss')), F.hp(3200.0, 0.7), F.lp(9500.0, 0.7), extend=False)
    hiss = normalize(hiss * kit.turbulence(n, child(r, 'ht'), 90.0, 0.7)) * E.bp([(0.0, 0.0), (0.03, 1.0), (0.9, 0.5), (dur, 0.0)], n=n, curve='cos')
    d = 0.9
    sigh = V.whisper(d, [(0.0, 'h'), (0.35, 'o'), (d, 'h')], child(r, 'sigh'), 1.0)
    sigh = normalize(F.filt(sigh * E.bp([(0.0, 0.0), (0.3, 1.0), (d, 0.0)], n=sigh.size, curve='cos'), F.hp(250.0), F.lp(5000.0),
                            extend=False))

    def lv(sig, rel_db):
        return kit.set_level(sig, puff, rel_db, 0.01)
    x = layers((puff, 0.0, 0.0), (lv(crumple, -9.0), 0.0, 0.0), (lv(crumble, -3.0), 0.0, 0.0), (lv(hiss, -14.0), 0.02, 0.0),
               (lv(sigh, -8.0), 0.05, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return _haunt(x, r, -18.0, 0.8, 3000.0)


# --- The dead miners and the bell-ringer --------------------------------------------------------------------------------

@cue('Creature.DeadMiner.Notice', variations=3, att='Creature', jitter=0.04, conc=3, level=-9.0)
def deadminer_notice(v, r):
    # It looks up from its work: the pick's iron head dragged across the stone floor (a ring as it catches, a gritty
    # scrape), a gravelly grunt from deep in a dead chest, the haft knocking in its grip; bare rock round it.
    clink = pick_head(child(r, 'clink'), 700.0 * jitter(r, 1, 0.06), 0.09)
    drag = kit.scrape(0.38, child(r, 'drag'), 720.0, 430.0, 5.0, 0.7, (1.0, 1.6, 2.5), 0.12, 0.5, 1800.0, -22.0)
    grit = kit.grit_burst(0.34, child(r, 'grit'), 2500.0, 600.0, 3200.0, 0.15)
    f = [112.0, 104.0, 122.0][v] * jitter(r, 1, 0.03)
    grunt = _choir(child(r, 'grunt'), [(0.0, f * 1.1), (0.3, f * 0.85)], 0.34, [(0.0, 'uh'), (0.34, 'o')], 2, 14.0, 0.015, 0.55,
                   0.95, (4.5, 0.12), 0.6, 1.0)
    grunt = _cold(grunt, child(r, 'cold'), 31.0, 0.18) * E.bp([(0.0, 0.0), (0.05, 1.0), (0.2, 0.7), (grunt.size / SR, 0.0)],
                                                              n=grunt.size, curve='cos')
    knock = _haft(child(r, 'grip'))
    x = layers((clink, 0.0, -5.0), (drag, 0.0, 0.0), (grit, 0.02, -14.0), (grunt, 0.17, -3.0), (knock, 0.42, -10.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return gallery(x, r, -15.0, 0.4)


@cue('Creature.BellRinger.Notice', variations=2, att='Creature', jitter=0.04, conc=2, level=-11.0)
def bellringer_notice(v, r):
    # It has seen you: the clapper knocks the bell once, muffled (the bronze held against the ringer's coat so its upper
    # partials die at once and only a short, dull note is left), the wooden handle shifting in its fist.
    prime = PRIMES[v] * jitter(r, 1, 0.01)
    m = handbell(prime, child(r, 'bell'), t60=0.55, beat=0.9, bright=0.6)
    knock = ring(m, 0.9, child(r, 'knock'), 0.0008, -20.0)
    handle = kit.thunk(210.0 * jitter(r, 1, 0.1), child(r, 'handle'), 0.05, 0.001)
    coat = kit.cloth(0.15, child(r, 'coat'), 300.0, 1800.0, 120.0, 0.3, 0.5)
    x = layers((knock, 0.0, 0.0), (handle, 0.0, -9.0), (coat, 0.0, -20.0))
    x = F.filt(x, F.lp(3400.0, 0.7), F.peak(3200.0, -3.0, 0.8), F.hp(100.0), extend=False)
    return _outside(x, r, -20.0)
