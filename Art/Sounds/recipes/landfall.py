"""The Lamplighter and her landing (Docs/Work/2026-10-31-Wardens/CONTRACT.md, SOUND; ALamplighterShip, ACompanyTender,
ALandfallGrapple, ALandfallWinch, the HardLanding scene): a timber airship under a patched gas bag, hung with rope and
iron, her engine a slow soul-coal heart turning a big wooden propeller. All 3D at the ship (the engine, the creaks, the
grapples, the winch are loops or one-shots she carries; the landing and the tender's fall reach down the canyon like a
gunshot). The story cues (radio, lantern, smog, spyglass, Delia, the turn-in stamp) are in landfall_story.py.

Wood groans by stick-slip (kit.creak) through a hull's low resonances, rope sings higher and thinner, iron grinds higher
still; a taut cable hums at its own pitch; a hard thing meeting a hard thing is a modal strike. The big moments (the
lurch, the tear, the crash, the tender) are built from a push of air, a thud with no pitch to it, the splintering
and the fittings answering, then the canyon throwing it back (creatures._outside).

- Ship.Lamplighter.Engine (loop, 8 s): the propeller's slow whump (5 beats a second), the heart's drone and chuffs, the
  gearing's whine, the rigging singing in the wind.
- Ship.Lamplighter.Creak (loop, 10 s): timbers groaning, rope taking the strain, a squeaking iron fitting, between slow
  sways.
- Ship.Lamplighter.Lurch (~1.9 s): the grapples take her: chains snap taut, the hull shudders and groans as she stops, the
  cables twang, loose gear rattles.
- Ship.Lamplighter.BagTear (~2.6 s): the gas bag's canvas ripping, a great gas rush, lines snapping.
- Ship.Lamplighter.HardLanding (~4.6 s, 'Gun'): the crash onto the ledge: the keel hits, timbers splinter, she scrapes on
  stone, fittings clang, debris rains, dust, a last groan as she settles.
- Ship.Grapple.Fire (~1.2 s): the Company's harpoon gun: a pop of air, the chain whipping out.
- Ship.Grapple.Hit (~1.2 s): the grapnel biting into the deck, the chain settling.
- Ship.Grapple.Strain (loop, 6 s): a taut cable's hum, links groaning, timbers.
- Ship.Grapple.Cut (~1.3 s): the chain parted: the cutter's blow, the link cracking, the cable whipping away.
- Ship.Tender.Fall (~7.5 s, 'Gun'): the tender falling smoking into the canyon: a groan, the engine coughing, a long
  fall into the distance and a far crash.
- Ship.Anchor.Crank (loop, 4 s): the windlass: a pawl ratcheting, the chain's links, the gear groaning.
- Ship.Anchor.Set (~2.3 s): the chain running out, the flukes biting stone, the chain snapping taut, the pawl locking.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, modal as M, granular as G
from recipes import cue
from recipes import kit
from recipes.creatures import _outside
from recipes.world import _fold, _pnoise, _slow, _circ
from recipes.orelift import _lv, _whole, _links
from recipes.gravemother_barrow import _tonks, _lumps, _earth, _snap_run
from recipes.bosses import _pressure, _rumble, _drum, _master


def _dense(x, clip_db=4.0, push_db=3.0):
    """A short, sharp event pushed into a soft clip and a limiter (as the big hits are), so it is as loud as it should
    sound against the rifle without more volume (the mix can't go over 0 dB)."""
    return _master(x, clip_db, push_db, 0.0015, 0.05)


def _rel(x, ref, db, win=0.01):
    """x set db against ref's loudest short window, so a noise layer never buries a thud."""
    return kit.set_level(x, ref, db, win)


def _timber(dur, r, rate0, rate1, f=170.0, q=7.0):
    """Timber under load groaning: a slow stick-slip through a hull's low resonances."""
    return kit.creak(dur, child(r, 'timber'), rate0, rate1, (f * jitter(r, 1, 0.08), f * 2.5, f * 5.3), q, 0.35)


def _rope(dur, r, rate0, rate1, f=720.0):
    """Rope or a stay taking the strain: higher and thinner than the timbers."""
    s = kit.creak(dur, child(r, 'rope'), rate0, rate1, (f * jitter(r, 1, 0.08), f * 2.3, f * 4.0), 10.0, 0.3)
    return F.filt(s, F.peak(3300.0, -5.0, 0.9), extend=False)


def _iron(dur, r, rate0, rate1, f=1000.0):
    """An iron fitting turning on its pin: a squeak and a grind."""
    s = kit.creak(dur, child(r, 'iron'), rate0, rate1, (f * jitter(r, 1, 0.08), f * 1.9, f * 3.1), 14.0, 0.25)
    return F.filt(s, F.peak(3300.0, -5.0, 0.9), extend=False)


def _cracks(dur, r, count, f_lo=180.0, f_hi=900.0, start=0.0, db=-6.0):
    """Timber cracking and splintering: a run of sharp snaps each with a dull knock under it, crowded early."""
    out = np.zeros(1)
    for i in range(count):
        rr = child(r, 'crack', i)
        t = start + dur * rr.random() ** 1.5
        snap = kit.burst(0.012, child(rr, 's'), 0.005, lo=600.0, hi=5500.0)
        body = kit.thunk(rr.uniform(f_lo, f_hi), child(rr, 'b'), 0.06, 0.0005, 5, 0.1)
        a = rr.uniform(-7.0, 0.0) + db
        out = layers(out, (snap, t, a - 3.0), (body, t, a))
    return out


def _loopify(x, xf):
    """A long signal made into a seamless loop: its last xf seconds fade into its first xf seconds (equal power)."""
    k = ns(xf)
    n = x.size - k
    u = np.linspace(0.0, 1.0, k)
    y = x[:n].copy()
    y[:k] = x[:k] * np.sin(0.5 * np.pi * u) + x[n:n + k] * np.cos(0.5 * np.pi * u)
    return y


def _calm(x):
    """Frequent ship sounds stay soft in the ear's sharpest band and the very top."""
    return F.filt(x, F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -3.0), extend=False)


# --- Loops: the engine and the creaks ------------------------------------------------------------------------------------

@cue('Ship.Lamplighter.Engine', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-17.0,
     align=False)
def ship_engine(v, r):
    # The Lamplighter under way: a big slow wooden propeller (a whump five times a second, the air beating through the
    # blades), the soul-coal heart's low drone with a chuff every pulse of its piston, the gears' whine, and the rigging
    # singing in the slipstream. Eight seconds, whole cycles of everything.
    dur = 8.0
    L = ns(dur)
    t = times(L)
    blade = 5.0
    ph = (t * blade) % 1.0
    pulse = 0.22 + 0.78 * np.exp(-((ph - 0.5) / 0.2) ** 2)
    air = _circ(_pnoise(L, child(r, 'air'), 80.0, 1500.0, -3.5), F.peak(300.0, 4.0, 0.9))
    prop = air * pulse * (0.8 + 0.2 * _slow(L, child(r, 'pg'), 0.5, 0.0))
    f0 = _whole(46.0, dur)
    drone = np.zeros(L)
    for k, a in enumerate((1.0, 0.7, 0.5, 0.35, 0.22, 0.12), 1):
        drone += a * np.sin(2 * np.pi * _whole(f0 * k * (1.0 + 0.004 * k), dur) * t + 1.7 * k)
    drone *= 0.85 + 0.15 * _slow(L, child(r, 'dg'), 0.35, 0.0)
    events = []
    rc = child(r, 'chuffs')
    for k in range(20):
        at = (k + 0.5) * dur / 20.0 + rc.normal(0.0, 0.004)
        strength = (1.0, 0.7)[k % 2] * rc.uniform(0.9, 1.1)
        puff = kit.burst(0.15, child(rc, 'puff', k), 0.09, lo=200.0, hi=3000.0, color=-2.0, attack=0.004)
        thump = kit.noise_thump(0.14, child(rc, 'thump', k), 300.0, 60.0, 0.07, 1.0, 0.05, 0.003)
        events.append((layers((puff, 0.0, 0.0), (thump, 0.0, -3.0)) * strength, at % dur))
    chuffs = _fold(L, events)
    mesh = _whole(612.0, dur)
    whine = (np.sin(2 * np.pi * mesh * t) + 0.4 * np.sin(2 * np.pi * _whole(mesh * 2.0, dur) * t + 1.0)) \
        * (0.8 + 0.2 * np.cos(2 * np.pi * _whole(10.0, dur) * t))
    gust = _slow(L, child(r, 'rg'), 0.4, 0.15) ** 1.5
    rig = _circ(_pnoise(L, child(r, 'rig'), 800.0, 4000.0, 0.0), F.bp(1350.0, 22.0), F.bp(1350.0, 22.0)) * gust
    rig2 = _circ(_pnoise(L, child(r, 'rig2'), 800.0, 4000.0, 0.0), F.bp(2150.0, 25.0), F.bp(2150.0, 25.0)) \
        * _slow(L, child(r, 'rg2'), 0.3, 0.1) ** 2
    x = _lv(prop, 0.0) + _lv(drone, -4.0) + _lv(chuffs, -2.5) + _lv(whine, -23.0) + _lv(rig, -17.0) + _lv(rig2, -22.0)
    return _circ(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(6500.0, -4.0))


@cue('Ship.Lamplighter.Creak', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-22.0,
     align=False)
def ship_creak(v, r):
    # Her timbers working as she sails: a groan every second or two (hull, a rope taking the strain, an iron fitting
    # squeaking on its pin), and a low hum swelling with the slow sway. Ten seconds.
    dur = 10.0
    L = ns(dur)
    events = []
    rc = child(r, 'ev')
    kinds = ['timber', 'rope', 'timber', 'iron', 'rope', 'timber', 'iron', 'rope', 'timber']
    for k, kind in enumerate(kinds):
        rk = child(rc, 'k', k)
        at = (k + rk.uniform(0.05, 0.8)) * dur / len(kinds)
        if kind == 'timber':
            x = _timber(rk.uniform(0.8, 1.6), rk, rk.uniform(8.0, 12.0), rk.uniform(4.0, 7.0), rk.uniform(130.0, 230.0)) \
                * rk.uniform(0.6, 1.0)
        elif kind == 'rope':
            x = _rope(rk.uniform(0.5, 0.9), rk, rk.uniform(24.0, 36.0), rk.uniform(14.0, 22.0), rk.uniform(600.0, 900.0)) \
                * rk.uniform(0.35, 0.6)
        else:
            x = _iron(rk.uniform(0.2, 0.4), rk, rk.uniform(70.0, 120.0), rk.uniform(40.0, 80.0), rk.uniform(1500.0, 2200.0)) \
                * rk.uniform(0.25, 0.4)
        events.append((x, at))
    for k in range(5):
        rk = child(rc, 'tick', k)
        events.append((kit.thunk(rk.uniform(120.0, 260.0), rk, 0.08, 0.001, 5, 0.1) * rk.uniform(0.3, 0.6), rk.uniform(0.0, dur)))
    creaks = _fold(L, events)
    sway = 0.5 + 0.5 * np.cos(2 * np.pi * times(L) / 5.0)
    hum = _circ(_pnoise(L, child(r, 'hum'), 50.0, 320.0, -5.0), F.peak(110.0, 4.0, 0.9)) * (0.5 + 0.5 * sway)
    x = _lv(creaks, 0.0) + _lv(hum, -13.0)
    return _circ(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


# --- The lurch, the tear and the crash --------------------------------------------------------------------------------------

@cue('Ship.Lamplighter.Lurch', variations=3, att='Creature', jitter=0.04, conc=2, level=-3.0)
def ship_lurch(v, r):
    # The grapples take her: both chains snap taut with a bang and a thud of weight, the hull shudders to a stop and groans
    # (the groan sinking as she stills), the cables twang, a rush of air as she brakes, loose gear rattling on deck.
    bang = kit.clack(child(r, 'bang'), 380.0 * jitter(r, 1, 0.06), 700.0, 3200.0, 0.1, 0.12, -2.0, -6.0)
    thud = kit.noise_thump(0.45, child(r, 'thud'), 520.0, 48.0, 0.22, 1.0, 0.2, 0.002)
    boom = kit.thump(jitter(r, 76.0, 0.06), 34.0, 0.8, 0.4, drive_db=2.0)
    twang = F.filt(O.string(jitter(r, 66.0, 0.08), 1.0, child(r, 'twang'), 0.08, 0.9, 0.25), F.lp(1300.0, 0.7), extend=False)
    twang2 = F.filt(O.string(jitter(r, 74.0, 0.08), 0.8, child(r, 'twang2'), 0.1, 0.7, 0.25), F.lp(1300.0, 0.7), extend=False)
    groan = _timber(1.5, child(r, 'groan'), 13.0, 3.2, 150.0 + 12.0 * v)
    groan2 = _timber(1.0, child(r, 'groan2'), 9.0, 3.0, 215.0)
    rope = _rope(0.7, child(r, 'rope'), 40.0, 16.0, 760.0)
    brake = kit.whoosh(1.2, child(r, 'brake'), 900.0, 170.0, 0.7, 0.18, -3.0)
    gear = kit.jingle(0.7, child(r, 'gear'), 7, 700.0, 4200.0, 0.07, 0.5, 0.0)
    cracks = _cracks(0.7, child(r, 'cr'), 6, 150.0, 700.0, 0.05, -7.0)
    x = layers((bang, 0.0, 0.0), (thud, 0.0, 1.0), (boom, 0.0, -4.0), (_rel(twang, thud, -12.0, 0.05), 0.01, 0.0),
               (_rel(twang2, thud, -14.0, 0.05), 0.015, 0.0), (_rel(groan, thud, -13.0, 0.1), 0.08, 0.0),
               (_rel(groan2, thud, -18.0, 0.1), 0.2, 0.0), (_rel(rope, thud, -20.0, 0.1), 0.04, 0.0),
               (_rel(brake, thud, -17.0, 0.1), 0.0, 0.0), (_rel(gear, thud, -19.0, 0.05), 0.12, 0.0),
               (_rel(cracks, thud, -15.0, 0.02), 0.0, 0.0))
    return _dense(_outside(_calm(x), r, -17.0, 0.09, 0.22, 0.7), 3.0, 3.0)


def _rip(dur, r, rate0, rate1):
    """Canvas ripping: stick-slip micro-tears (a dense run of clicks that quickens as the rip opens), rung through a few
    bands, in surges as the hands pull."""
    n = ns(dur)
    t = times(n)
    rate = rate0 * (rate1 / rate0) ** (t / dur)
    surge = 0.35 + 0.65 * np.abs(N.smooth_random(n, child(r, 'surge'), 9.0)) ** 1.2
    grit = G.grit(dur, child(r, 'grit'), rate * surge, 900.0, 5500.0, 1.4, 5)
    weave = F.filt(N.shaped(n, child(r, 'weave'), -2.0) * surge, F.hp(500.0, 0.7), F.lp(3600.0, 0.7), extend=False)
    return normalize(normalize(grit) + 0.3 * normalize(weave), 0.0)


@cue('Ship.Lamplighter.BagTear', variations=3, att='Creature', jitter=0.04, conc=2, level=-4.0)
def ship_bag_tear(v, r):
    # The gas bag torn: a pop as the first seam goes, canvas ripping along it in surges, a great rush of gas escaping
    # (a whump, a long hiss), two stays snapping with a whip and a twang, the hull groaning as she loses lift.
    pop = layers(kit.burst(0.02, child(r, 'pop'), 0.008, lo=500.0, hi=5000.0),
                 (kit.noise_thump(0.2, child(r, 'popthump'), 600.0, 80.0, 0.1, 1.0, 0.08, 0.002), 0.0, -3.0))
    rip = _rip(1.5 + 0.1 * v, child(r, 'rip'), 250.0, 2200.0)
    rip = rip * E.ar(1.5 + 0.1 * v, 0.1, 0.5, 1.5)
    gas = kit.whoosh(2.2, child(r, 'gas'), 600.0, 3200.0, 0.45, 0.18, -2.0)
    n = ns(2.2)
    flutter = F.filt(N.shaped(n, child(r, 'gf'), -1.0) * kit.turbulence(n, child(r, 'gft'), 140.0, 0.8),
                     F.hp(700.0, 0.7), F.lp(6500.0, 0.7), extend=False) * E.perc(2.2, 0.05, 1.4)
    whump = kit.noise_thump(0.6, child(r, 'whump'), 380.0, 60.0, 0.3, 1.0, 0.25, 0.01)
    stay1 = layers(kit.nwave(0.0004), (F.filt(O.string(jitter(r, 82.0, 0.1), 0.6, child(r, 'st1'), 0.1, 0.5, 0.3), F.lp(1400.0, 0.7), extend=False), 0.0, -4.0))
    stay2 = F.filt(O.string(jitter(r, 98.0, 0.1), 0.5, child(r, 'st2'), 0.1, 0.4, 0.3), F.lp(1400.0, 0.7), extend=False)
    groan = _timber(1.4, child(r, 'groan'), 8.0, 4.0, 160.0)
    x = layers((pop, 0.0, 0.0), (_rel(rip, pop, -3.0, 0.05), 0.02, 0.0), (_rel(gas, rip, -3.0, 0.1), 0.15, 0.0),
               (_rel(flutter, rip, -7.0, 0.1), 0.2, 0.0), (_rel(whump, pop, -2.0, 0.05), 0.12, 0.0),
               (_rel(stay1, pop, -4.0, 0.02), 0.6, 0.0), (_rel(stay2, pop, -7.0, 0.02), 0.95, 0.0),
               (_rel(groan, rip, -10.0, 0.1), 0.8, 0.0))
    return _outside(_calm(x), r, -18.0, 0.1, 0.24, 0.8)


@cue('Ship.Lamplighter.HardLanding', variations=3, att='Gun', jitter=0.03, conc=1, level=2.0)
def ship_hard_landing(v, r):
    # The crash onto the Mooring Ledge: the keel hits (a huge thud and a boom with no pitch in it), timbers split in a
    # crowd of cracks, she skids on stone (a long grinding scrape sinking as she slows, grit flying), iron fittings
    # clang, debris rains down on the deck, dust hisses out, and she settles with a groan and a last knock.
    thud = kit.noise_thump(0.7, child(r, 'thud'), 800.0, 36.0, 0.45, 1.0, 0.3, 0.002)
    boom = _drum(jitter(r, 38.0, 0.05), 1.5, child(r, 'boom'), 1.0, 0.1, 0.003)
    push = _pressure(child(r, 'push'), 0.03, 0.4, 160.0)
    timber = _cracks(0.9, child(r, 'timber'), 22, 120.0, 800.0, 0.0, -2.0)
    splinter = kit.grit_burst(0.9, child(r, 'spl'), 2600.0, 700.0, 4200.0, 0.4, 0.002)
    scrape = kit.scrape(2.2, child(r, 'scrape'), 170.0, 85.0, 3.0, 0.8, (1.0, 1.7, 2.9), 0.04, 0.55, 2200.0, -30.0)
    scrape = F.filt(scrape, F.lp(3400.0, 0.7), extend=False)
    grit = kit.grit_burst(2.0, child(r, 'grit'), 2800.0, 500.0, 3600.0, 1.2, 0.01)
    keel = _earth(2.0, child(r, 'keel'), 70.0, 700.0, 1.0, 0.01, 30.0, 0.8)
    clang1 = M.strike(M.plate(jitter(r, 330.0, 0.06), child(r, 'cl1'), 22, 0.55, 0.6, tilt=0.8), M.hammer(0.0004), 0.9)
    clang2 = M.strike(M.plate(jitter(r, 470.0, 0.06), child(r, 'cl2'), 22, 0.45, 0.6, tilt=0.8), M.hammer(0.0005), 0.8)
    clang1, clang2 = F.filt(clang1, F.lp(2600.0, 0.7), extend=False), F.filt(clang2, F.lp(2600.0, 0.7), extend=False)
    rain = _tonks(1.8, child(r, 'rain'), 26, 0.5, 1.7, 220.0, 1400.0, 0.86, 0.05)
    lumps = _lumps(1.6, child(r, 'lumps'), 8, 0.4, 1.3, 120.0, 330.0, 0.84, 0.03)
    dust = kit.burst(2.2, child(r, 'dust'), 1.3, lo=150.0, hi=2200.0, color=-3.0, attack=0.1)
    groan = _timber(1.6, child(r, 'groan'), 7.0, 3.0, 140.0)
    settle = kit.thunk(jitter(r, 90.0, 0.08), child(r, 'settle'), 0.3, 0.003, 6, 0.1)
    rumble = _rumble(3.0, child(r, 'rumble'), 200.0, 1.8, 0.1, 3.0)
    # She skips and lands again a third of a second later, softer.
    hit2 = layers(kit.noise_thump(0.5, child(r, 'thud2'), 600.0, 42.0, 0.3, 1.0, 0.2, 0.002),
                  (_drum(jitter(r, 42.0, 0.05), 1.0, child(r, 'boom2'), 0.7, 0.1, 0.003), 0.0, -6.0))
    ref = thud
    x = layers((thud, 0.0, 2.0), (boom, 0.0, -4.0), (push, 0.0, -2.0), (_rel(hit2, ref, -6.0, 0.1), 0.36 + 0.04 * v, 0.0),
               (_rel(timber, ref, -6.0, 0.02), 0.0, 0.0),
               (_rel(splinter, ref, -16.0, 0.05), 0.02, 0.0), (_rel(scrape, ref, -13.0, 0.1), 0.45, 0.0),
               (_rel(grit, ref, -22.0, 0.1), 0.5, 0.0), (_rel(keel, ref, -18.0, 0.1), 0.3, 0.0),
               (_rel(clang1, ref, -12.0, 0.05), 0.12, 0.0), (_rel(clang2, ref, -15.0, 0.05), 0.8 + 0.1 * v, 0.0),
               (_rel(rain, ref, -16.0, 0.05), 0.7, 0.0), (_rel(lumps, ref, -15.0, 0.05), 0.7, 0.0),
               (_rel(dust, ref, -24.0, 0.1), 0.2, 0.0), (_rel(groan, ref, -17.0, 0.1), 2.3, 0.0),
               (_rel(settle, ref, -13.0, 0.05), 3.1 + 0.15 * v, 0.0), (rumble, 0.0, -14.0))
    return _dense(_outside(_calm(x), r, -12.0, 0.12, 0.31, 1.1), 3.0, 2.0)


# --- The grapples ----------------------------------------------------------------------------------------------------------

@cue('Ship.Grapple.Fire', variations=3, att='Gun', jitter=0.04, conc=2, level=-2.5)
def grapple_fire(v, r):
    # The tender's harpoon gun: a pop of air as the charge goes, a sharp metallic tonk, the heavy chain whipping out
    # (links rattling faster and faster, a whine of cable), a rush of air after the grapnel.
    pop = layers(_pressure(child(r, 'pop'), 0.004, 0.12, 400.0), (kit.burst(0.015, child(r, 'crk'), 0.006, lo=800.0, hi=6000.0), 0.0, -4.0))
    tonk = kit.clack(child(r, 'tonk'), 700.0 * jitter(r, 1, 0.05), 900.0, 4000.0, 0.06, 0.07, -2.0, -6.0)
    out = np.zeros(1)
    t = 0.04
    gap = 0.07
    for i in range(16):
        rr = child(r, 'link', i)
        m = M.parts(rr.uniform(800.0, 1300.0), 3200.0, rr, 5, 0.035, 0.015, 0.5)
        link = normalize(M.strike(m, M.hammer(0.00015), 0.07)) * rr.uniform(0.6, 1.0)
        out = layers(out, (link, t, -2.0 - 0.15 * i))
        t += gap
        gap = max(0.012, gap * 0.86)
    n = ns(0.9)
    wh = F.sweep(N.pink(n, child(r, 'whine')), 'bp', O.expsweep(2600.0, 900.0, 0.9, 1.2, n=n), 9.0) * E.ar(0.9, 0.05, 0.6, 1.5)
    air = kit.whoosh(0.8, child(r, 'air'), 1500.0, 350.0, 0.7, 0.2, -3.0)
    x = layers((pop, 0.0, 0.0), (tonk, 0.0, -2.0), (_rel(out, pop, -10.0, 0.05), 0.0, 0.0),
               (_rel(wh, pop, -17.0, 0.05), 0.05, 0.0), (_rel(air, pop, -12.0, 0.05), 0.05, 0.0))
    return _dense(_outside(_calm(x), r, -16.0, 0.1, 0.24, 0.7), 4.0, 4.0)


@cue('Ship.Grapple.Hit', variations=3, att='Gun', jitter=0.04, conc=2, level=-3.0)
def grapple_hit(v, r):
    # The grapnel bites the deck: a heavy knock of iron into timber, planks splitting, the flukes ringing dully, the
    # chain's slack running out and settling in links, then the cable twanging taut.
    knock = kit.thunk(jitter(r, 140.0, 0.06), child(r, 'knock'), 0.14, 0.0008, 7, 0.1)
    thud = kit.noise_thump(0.3, child(r, 'thud'), 500.0, 55.0, 0.15, 1.0, 0.12, 0.001)
    split = _cracks(0.25, child(r, 'split'), 6, 160.0, 700.0, 0.0, -4.0)
    ring = normalize(M.strike(M.bar(jitter(r, 410.0, 0.05), child(r, 'ring'), 0.45, 0.8, 5, 0.01, 0.8), M.hammer(0.0003), 0.6))
    ring = F.filt(ring, F.lp(3000.0, 0.7), extend=False)
    slack = _links(child(r, 'slack'), 9, 0.4, 700.0, 2600.0, 0.0)
    twang = F.filt(O.string(jitter(r, 70.0, 0.08), 0.8, child(r, 'tw'), 0.09, 0.7, 0.25), F.lp(1200.0, 0.7), extend=False)
    x = layers((knock, 0.0, 0.0), (thud, 0.0, 0.0), (_rel(split, thud, -8.0, 0.02), 0.0, 0.0),
               (_rel(ring, thud, -11.0, 0.02), 0.0, 0.0), (_rel(slack, thud, -14.0, 0.02), 0.08, 0.0),
               (_rel(twang, thud, -13.0, 0.05), 0.35, 0.0))
    return _dense(_outside(_calm(x), r, -17.0, 0.09, 0.2, 0.6), 3.0, 3.0)


@cue('Ship.Grapple.Strain', variations=1, att='Creature', cls='Effects', jitter=0.0, conc=2, loop=True, level=-20.0,
     align=False)
def grapple_strain(v, r):
    # A cable taut under a ship's weight: it hums at its own pitch (a low note and its overtones, swelling a little with
    # the wind), links groan against each other, the timbers it pulls on groan, a tick of iron now and then. Six seconds.
    dur = 6.0
    L = ns(dur)
    t = times(L)
    f0 = _whole(87.0, dur)
    hum = np.zeros(L)
    for k, a in enumerate((1.0, 0.55, 0.38, 0.2, 0.12), 1):
        hum += a * np.sin(2 * np.pi * _whole(f0 * k * (1.0 + 0.003 * k), dur) * t + 0.9 * k) \
            * (0.8 + 0.2 * _slow(L, child(r, 'hg', k), 0.5 + 0.2 * k, 0.0))
    wind = _circ(_pnoise(L, child(r, 'wind'), 600.0, 4500.0, -1.0), F.bp(2300.0, 18.0), F.bp(2300.0, 18.0)) \
        * _slow(L, child(r, 'wg'), 0.45, 0.1) ** 2
    events = []
    rc = child(r, 'ev')
    for k, at in enumerate((0.3, 1.5, 2.6, 3.8, 5.0)):
        rk = child(rc, 'iron', k)
        events.append((_iron(rk.uniform(0.4, 0.8), rk, rk.uniform(28.0, 50.0), rk.uniform(16.0, 30.0), rk.uniform(750.0, 1100.0))
                       * rk.uniform(0.5, 0.9), at + rk.uniform(0.0, 0.3)))
    for k, at in enumerate((0.8, 3.3)):
        rk = child(rc, 'timber', k)
        events.append((_timber(rk.uniform(1.0, 1.6), rk, rk.uniform(7.0, 10.0), rk.uniform(4.0, 6.0), 150.0 + 40.0 * k) * 0.7, at))
    for k, at in enumerate((0.1, 1.1, 2.2, 3.1, 4.3, 5.4)):
        rk = child(rc, 'tick', k)
        events.append((kit.clack(rk, 520.0, 1000.0, 3200.0, 0.03, 0.04, -4.0, -10.0) * rk.uniform(0.1, 0.25), at))
    creaks = _fold(L, events)
    x = _lv(hum, 0.0) + _lv(creaks, 1.0) + _lv(wind, -12.0)
    return _circ(x, F.hp(45.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


@cue('Ship.Grapple.Cut', variations=3, att='Gun', jitter=0.04, conc=2, level=-4.0)
def grapple_cut(v, r):
    # The grapple line cut: the cutter's blow ringing on iron, the link cracking through, the freed cable whipping away
    # with a crack and a hiss of air, chain rattling off over the rail, the last of it knocking down the hull.
    blow = kit.clack(child(r, 'blow'), 520.0, 1100.0, 4200.0, 0.14, 0.1, -2.0, -6.0, 0.00008)
    ring = normalize(M.strike(M.bar(jitter(r, 860.0, 0.05), child(r, 'ring'), 0.5, 0.7, 5, 0.01, 0.7), M.hammer(0.00015), 0.7))
    ring = F.filt(ring, F.lp(3400.0, 0.7), extend=False)
    crack = layers(kit.nwave(0.0004), (kit.burst(0.02, child(r, 'crk'), 0.008, lo=700.0, hi=6500.0), 0.0, -3.0))
    whip = kit.whoosh(0.4, child(r, 'whip'), 3000.0, 600.0, 0.8, 0.15, -2.0)
    out = np.zeros(1)
    t = 0.06
    gap = 0.018
    for i in range(18):
        rr = child(r, 'fall', i)
        m = M.parts(rr.uniform(700.0, 1100.0) * (0.985 ** i), 2800.0, rr, 5, 0.04, 0.015, 0.5)
        link = normalize(M.strike(m, M.hammer(0.00018), 0.08)) * rr.uniform(0.6, 1.0)
        out = layers(out, (link, t, -2.5 - 0.3 * i))
        t += gap
        gap *= 1.09
    knock = kit.thunk(jitter(r, 230.0, 0.08), child(r, 'hullknock'), 0.12, 0.001, 5, 0.1)
    x = layers((blow, 0.0, 0.0), (_rel(ring, blow, -7.0, 0.02), 0.0, 0.0), (_rel(crack, blow, 0.0, 0.01), 0.005, 0.0),
               (_rel(whip, blow, -13.0, 0.05), 0.01, 0.0), (_rel(out, blow, -17.0, 0.02), 0.04, 0.0),
               (_rel(knock, blow, -15.0, 0.02), t + 0.05, 0.0))
    return _dense(_outside(_calm(x), r, -17.0, 0.09, 0.22, 0.7), 4.0, 4.0)


# --- The tender --------------------------------------------------------------------------------------------------------------

@cue('Ship.Tender.Fall', variations=2, att='Gun', jitter=0.03, conc=1, level=-3.0)
def tender_fall(v, r):
    # The Company tender cut loose and falling, smoking, into the canyon: her hull groans and cracks as the grapples let
    # go, the engine coughs and dies (chuffs slowing and dropping), the long fall (a whistle of air sinking in pitch and
    # level as she drops away, a trail of rattling and creaking), and, five seconds on, the far, dark crash rolling up the
    # walls.
    dur = 7.6
    n = ns(dur)
    t = times(n)
    groan = _timber(1.2, child(r, 'groan'), 9.0, 4.0, 140.0)
    cracks = _cracks(0.9, child(r, 'cr'), 10, 140.0, 800.0, 0.0, -5.0)
    chuffs = np.zeros(1)
    tt, gap = 0.1, 0.22
    for i in range(10):
        rr = child(r, 'chuff', i)
        puff = kit.burst(0.18, child(rr, 'p'), 0.1, lo=180.0, hi=2400.0, color=-2.0, attack=0.005)
        th = kit.noise_thump(0.16, child(rr, 'th'), 260.0 * (0.93 ** i), 50.0, 0.08, 1.0, 0.06, 0.003)
        chuffs = layers(chuffs, (layers((puff, 0.0, 0.0), (th, 0.0, -2.0)), tt, -1.0 * i))
        tt += gap
        gap *= 1.17
    fall_dur = 5.6
    fn = ns(fall_dur)
    fcut = O.expsweep(2400.0, 220.0, fall_dur, 1.1, n=fn)
    air = F.sweep(N.pink(fn, child(r, 'air')), 'lp', fcut, 0.8)
    fall_env = E.bp([(0.0, 0.0), (0.7, 1.0), (2.0, 0.55), (fall_dur, 0.0)], n=fn, curve='cos')
    whistle = F.sweep(N.pink(fn, child(r, 'wh')), 'bp', O.expsweep(1500.0, 260.0, fall_dur, 1.0, n=fn), 11.0) * fall_env
    air = air * fall_env
    trail = _timber(3.0, child(r, 'trail'), 10.0, 4.0, 170.0) * E.ar(3.0, 0.2, 2.0, 1.5)
    smoke = kit.burst(3.0, child(r, 'smoke'), 1.8, lo=500.0, hi=3500.0, color=-3.0, attack=0.3)
    far = _drum(jitter(r, 44.0, 0.05), 2.4, child(r, 'far'), 1.4, 0.1, 0.004)
    farcrash = layers(kit.noise_thump(0.9, child(r, 'fthud'), 380.0, 38.0, 0.5, 1.0, 0.35, 0.004),
                      (_rel(_snap_run(0.5, child(r, 'fsnap'), 60.0, 10.0, 200.0, 1800.0, 0.02), far, -6.0, 0.02), 0.0, 0.0),
                      (_rel(_tonks(1.4, child(r, 'ftonk'), 14, 0.1, 1.2, 200.0, 900.0, 0.86, 0.05), far, -10.0, 0.02), 0.1, 0.0))
    farcrash = F.filt(layers((far, 0.0, -2.0), (farcrash, 0.0, 0.0)), F.lpn(900.0, 2), extend=False)
    rum = _rumble(2.8, child(r, 'rum'), 180.0, 1.6, 0.3, 3.0)
    gone = 4.9 + 0.1 * v
    # The grapples let go with a bang, so the cue speaks at once.
    bang = kit.clack(child(r, 'bang'), 360.0, 700.0, 3000.0, 0.1, 0.12, -2.0, -6.0)
    ref = layers((bang, 0.0, 0.0), (kit.noise_thump(0.4, child(r, 'bth'), 500.0, 50.0, 0.2, 1.0, 0.15, 0.002), 0.0, 0.0))
    x = layers((ref, 0.0, 0.0), (_rel(groan, ref, -9.0, 0.1), 0.05, 0.0), (_rel(cracks, ref, -10.0, 0.02), 0.0, 0.0),
               (_rel(chuffs, ref, -10.0, 0.1), 0.3, 0.0), (_rel(air, ref, -18.0, 0.1), 0.3, 0.0),
               (_rel(whistle, ref, -17.0, 0.1), 0.3, 0.0), (_rel(trail, ref, -15.0, 0.1), 0.6, 0.0),
               (_rel(smoke, ref, -26.0, 0.1), 0.3, 0.0), (_rel(farcrash, ref, -8.0, 0.1), gone, 0.0),
               (_rel(rum, ref, -14.0, 0.1), gone, 0.0))
    return _outside(_calm(x), r, -13.0, 0.18, 0.46, 1.4)


# --- The winch ----------------------------------------------------------------------------------------------------------------

@cue('Ship.Anchor.Crank', variations=1, att='Creature', cls='Effects', jitter=0.0, conc=1, loop=True, level=-15.0,
     align=False)
def anchor_crank(v, r):
    # The windlass cranked by hand: eight strokes in four seconds, each a heave (the gear groaning under load, the chain
    # chunking up a link or two) and the pawl ratcheting over its teeth three times as the handle comes back.
    dur = 4.0
    L = ns(dur)
    events = []
    rc = child(r, 'ev')
    stroke = dur / 8.0
    for s in range(8):
        t0 = s * stroke
        rs = child(rc, 's', s)
        events.append((_timber(0.4, rs, 15.0, 9.0, 190.0) * rs.uniform(0.7, 1.0), t0))
        events.append((_iron(0.18, rs, 80.0, 50.0, 1500.0) * 0.25, t0 + 0.05))
        events.append((_links(rs, 2, 0.1, 700.0, 2800.0, 0.0) * rs.uniform(0.35, 0.6), t0 + 0.08))
        events.append((_links(rs, 2, 0.1, 700.0, 2800.0, 0.0) * rs.uniform(0.3, 0.5), t0 + 0.26))
        for j, tc in enumerate((0.04, 0.17, 0.3)):
            click = kit.clack(child(rs, 'pawl', j), 760.0 * jitter(rs, 1, 0.05), 1200.0, 3800.0, 0.025, 0.04, -3.0, -8.0)
            events.append((click * (0.8 - 0.1 * j) * rs.uniform(0.85, 1.0), t0 + tc))
    work = _fold(L, events)
    rumble = _circ(_pnoise(L, child(r, 'rum'), 50.0, 240.0, -4.0), F.peak(120.0, 3.0, 0.9)) \
        * (0.6 + 0.4 * np.cos(2 * np.pi * 8.0 * times(L) / dur))
    x = _lv(work, 0.0) + _lv(rumble, -12.0)
    return _circ(x, F.hp(45.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


@cue('Ship.Anchor.Set', variations=3, att='Creature', jitter=0.04, conc=1, level=-3.0)
def anchor_set(v, r):
    # The anchor let go and set: chain running out over the gypsy (links rattling faster as it falls), the flukes meeting
    # the ledge's stone with an iron clang and a crunch, the cable snapping taut, the whole hull groaning as she comes up
    # on it, the pawl dropping home in three clicks.
    run = np.zeros(1)
    t, gap = 0.0, 0.07
    for i in range(22):
        rr = child(r, 'run', i)
        m = M.parts(rr.uniform(700.0, 1200.0), 3000.0, rr, 5, 0.04, 0.015, 0.5)
        link = normalize(M.strike(m, M.hammer(0.0002), 0.08)) * rr.uniform(0.6, 1.0)
        run = layers(run, (link, t, -1.5 - 0.05 * i))
        t += gap
        gap = max(0.014, gap * 0.9)
    land = t + 0.02
    iron = normalize(M.strike(M.bar(jitter(r, 190.0, 0.05), child(r, 'iron'), 0.9, 0.6, 6, 0.01, 0.7), M.hammer(0.0004), 1.0))
    iron = F.filt(iron, F.lp(3000.0, 0.7), extend=False)
    thud = kit.noise_thump(0.35, child(r, 'thud'), 600.0, 50.0, 0.18, 1.0, 0.14, 0.001)
    crunch = kit.grit_burst(0.4, child(r, 'crunch'), 3000.0, 500.0, 3500.0, 0.2, 0.002)
    twang = F.filt(O.string(jitter(r, 70.0, 0.08), 1.0, child(r, 'tw'), 0.09, 0.9, 0.25), F.lp(1300.0, 0.7), extend=False)
    snap = kit.clack(child(r, 'snap'), 400.0, 700.0, 3000.0, 0.1, 0.1, -2.0, -6.0)
    groan = _timber(1.2, child(r, 'groan'), 11.0, 4.0, 160.0)
    x = layers((_rel(run, thud, -6.0, 0.05), 0.0, 0.0), (thud, land, 2.0), (_rel(iron, thud, -4.0, 0.02), land, 0.0),
               (_rel(crunch, thud, -9.0, 0.02), land, 0.0), (snap, land + 0.55, 0.0),
               (_rel(twang, thud, -9.0, 0.05), land + 0.56, 0.0), (_rel(groan, thud, -12.0, 0.1), land + 0.6, 0.0))
    for j, tc in enumerate((0.0, 0.1, 0.19)):
        click = kit.clack(child(r, 'pawl', j), 760.0, 1200.0, 3800.0, 0.025, 0.04, -3.0, -8.0)
        x = layers(x, (click, land + 1.2 + tc + 0.02 * v, -8.0))
    return _outside(_calm(x), r, -17.0, 0.09, 0.22, 0.7)
