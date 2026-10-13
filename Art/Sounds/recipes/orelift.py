"""The ore lift (AOreLift, its bell pulls and the ride; wave 2b): a steam winch in a timber house, a haul rope over one timber
tower's sheaves, an open iron ore cage hung from a grip, a bronze signal bell on a spring at each landing. All 3D.

The ride is a set piece, so the lift has weight. Iron and timber under load creak by stick-slip (kit.creak): the straps'
iron grinding high and slow, the timbers groaning low, the chains' links clinking as they shift. The rope is steel wire:
over the grip's rollers its strands pass at a steady rate and it sings a low hum (a pitched whirr: noise through a comb at
the strand rate, and the hum's few harmonics). The cage and the tower's wheel are cast iron: a clank is a hard strike on a
heavy, dense mode set, a wheel rings on a moment, the cable shudders in a low beat. The winch is a small steam engine:
four exhaust chuffs to a turn (two cylinders, strong and weak), the gear's whine at its mesh rate, the drum's rope paying
out, a leak's hiss, in a timber house. The whistle is a stopped pipe blown by steam: odd harmonics, a scoop up as the
valve opens and a sag as it shuts, breathy, carried across the valley like a gunshot. The signal bell is a bronze dome
(bells.py's hand bell, rounder and longer) struck by a hammer on a spring, which bounces once.

Loops (WinchRun, CableRun) are built in a circle (world.py's way): periodic noise, whole cycles of every tone and every
rhythm, events folded round the seam, filters and rooms applied circularly.

- World.OreLift.BellSignal (near): the cord creaking as it's pulled, the bell struck twice on its spring.
- World.OreLift.BellAnswer (carried far): the far landing's bell, one bright stroke, thin with distance, the valley's wash.
- World.OreLift.BellArrive (near): one clear stroke as the cage is set down, ringing on.
- World.OreLift.BellDead (near, short): the cord jerked, the hammer's dull clunk on a cold bell, nothing more.
- World.OreLift.Whistle (carried like a gunshot): two short blasts, breathy, the second a third lower.
- World.OreLift.WinchRun (loop, at the winch house): the engine's chuff, the gear's whine, the rope paying over the drum.
- World.OreLift.CableRun (loop, on the cage): the rope's hum over the grip, splices ticking past, timbers and chains
  creaking with the sway, a high wind through the cage.
- World.OreLift.TakeUp (on the cage): straps and timbers groaning as the rope takes the strain, a heavy jolt, chains
  snapping taut.
- World.OreLift.SheaveClank (at the tower, carried; three in about a second, five takes): the grip over a sheave, a heavy
  iron clank, the wheel's short ring, the cable's shudder.
- World.OreLift.CageLand (on the cage): a heavy wooden thump on the landing, chains slackening, an iron latch dropping.
- World.OreLift.GateLatch (near, short): the cage's iron gate swung shut, its hinge squealing, a clack and the latch.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, add_at, db2a
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from lib import granular as G
from lib import analysis as A
from recipes import cue
from recipes import kit
from recipes.bells import handbell, ring, _across
from recipes.creatures import _outside
from recipes.world import _fold, _pnoise, _slow, _circ

# The signal bell's strike note at each landing (a little apart, as two bells cast years apart would be).
BELL_PRIMES = (780.0, 815.0, 760.0)


# --- The signal bell ----------------------------------------------------------------------------------------------------

def _signal_bell(prime, r, t60=2.4):
    """The bronze dome: a hand bell's partials, rounder (the upper ones softer) and ringing longer."""
    return handbell(prime, r, t60=t60, beat=1.0, bright=0.82)


def _stroke(m, dur, r, bounce_db=-17.0):
    """One stroke of the spring hammer: the strike, and the hammer bouncing back onto the bell once, softly, a few
    hundredths of a second later."""
    hit = ring(m, dur, child(r, 'hit'), 0.00025, -20.0)
    back = ring(m, dur - 0.05, child(r, 'back'), 0.0006, -30.0)
    return normalize(layers((hit, 0.0, 0.0), (back, 0.035 + 0.015 * r.random(), bounce_db)))


def _cord(dur, r):
    """The bell cord drawn through its guide: hemp creaking in a wooden eye, a slow stick-slip, starting with the hand's grip on
    it (a soft knock of the rope going taut), so it speaks at once."""
    creak = kit.creak(dur, child(r, 'cord'), 40.0, 75.0, (380.0 * jitter(r, 1, 0.1), 900.0, 1800.0), 7.0, 0.35)
    grip = kit.noise_thump(0.05, child(r, 'grip'), 900.0, 220.0, 0.02, 1.0, 0.02, 0.0008)
    return F.filt(layers((grip, 0.0, -4.0), (creak, 0.0, 0.0)), F.peak(3300.0, -4.0, 0.9), extend=False)


def _bell_space(x, r, rel_db=-16.0):
    """The landing's open air: the canyon wall answering, then a short wash."""
    return _outside(x, r, rel_db, 0.09, 0.24, 0.9)


@cue('World.OreLift.BellSignal', variations=3, att='Near', cls='Effects', jitter=0.02, conc=2, level=-9.0)
def bell_signal(v, r):
    # The bell pull: the cord creaks as it's drawn, and the bell is struck twice on its spring (ding... ding), the
    # second a hair softer, each with the hammer's bounce.
    cord = _cord(0.32, child(r, 'cord'))
    m = _signal_bell(BELL_PRIMES[v] * jitter(r, 1, 0.005), child(r, 'bell'))
    gap = 0.42 + 0.06 * r.random()
    s1 = _stroke(m, 2.6, child(r, 's1'))
    s2 = _stroke(m, 2.6, child(r, 's2'))
    t = 0.12 + 0.03 * r.random()
    x = layers((cord, 0.0, -12.0), (s1, t, 0.0), (_cord(0.25, child(r, 'cord2')), t + 0.12, -16.0), (s2, t + gap, -1.5))
    x = F.filt(x, F.hp(80.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _bell_space(x, r)


@cue('World.OreLift.BellAnswer', variations=2, att='Gun', cls='Effects', jitter=0.01, conc=1, level=-11.0)
def bell_answer(v, r):
    # The far landing answering: one bright stroke heard from far across the canyon, thin (its lows and its top lost
    # to the distance), more of the valley's wash than of the bell.
    m = _signal_bell(BELL_PRIMES[(v + 1) % 3], child(r, 'bell'), 2.2)
    s = _stroke(m, 2.4, child(r, 's'))
    s = F.filt(s, F.hp(420.0, 0.7), F.lp(3600.0, 0.7), extend=False)
    return _across(normalize(s), r, 2.6, -7.0)


@cue('World.OreLift.BellArrive', variations=3, att='Near', cls='Effects', jitter=0.02, conc=1, level=-10.0)
def bell_arrive(v, r):
    # The cage set down: the landing's bell struck once, clear, left ringing on.
    m = _signal_bell(BELL_PRIMES[v] * jitter(r, 1, 0.005), child(r, 'bell'), 2.8)
    s = _stroke(m, 3.0, child(r, 's'))
    x = F.filt(s, F.hp(80.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _bell_space(x, r, -15.0)


@cue('World.OreLift.BellDead', variations=2, att='Near', cls='Effects', jitter=0.03, conc=1, level=-14.0)
def bell_dead(v, r):
    # The lift is closed: the cord jerked hard (a creak, the rope snapping straight), the hammer meeting the cold bell
    # with a dull, choked clunk (its spring caught), and nothing answering.
    cord = _cord(0.14, child(r, 'cord'))
    snap = kit.noise_thump(0.06, child(r, 'snap'), 900.0, 200.0, 0.025, 1.0, 0.02)
    m = _signal_bell(BELL_PRIMES[v], child(r, 'bell'), 0.22)
    clunk = normalize(M.strike(m, M.hammer(0.0018), 0.35))
    clunk = F.filt(clunk, F.lp(1600.0, 0.7), extend=False)
    knock = kit.thunk(260.0 * jitter(r, 1, 0.06), child(r, 'knock'), 0.05, 0.0015, 5, 0.06)
    t = 0.1 + 0.02 * r.random()
    x = layers((cord, 0.0, -6.0), (snap, 0.08, -8.0), (clunk, t, 0.0), (knock, t, -5.0))
    x = F.filt(x, F.hp(80.0), extend=False)
    return _bell_space(x, r, -20.0)


# --- The winch house -----------------------------------------------------------------------------------------------------

def _blast(f0, dur, r, fall=0.97):
    """One blast of the steam whistle: a stopped pipe's odd harmonics (a little of the evens: a real bell-mouth), the
    pitch scooping up as the valve opens and sagging as it shuts, fluttering with the steam's turbulence; the steam's
    breath round the note and its hiss above it."""
    n = ns(dur)
    t = times(n)
    scoop = 1.0 - 0.13 * np.exp(-t / 0.035)
    sag = 1.0 - (1.0 - fall) * np.clip((t - (dur - 0.1)) / 0.1, 0.0, 1.0)
    flutter = 1.0 + 0.0035 * N.smooth_random(n, child(r, 'flutter'), 18.0)
    f = f0 * scoop * sag * flutter
    tone = O.additive(f, [1.0, 0.14, 0.42, 0.08, 0.2, 0.04, 0.09])
    second = O.additive(f * 1.498, [1.0, 0.1, 0.3])
    am = 0.85 + 0.15 * np.abs(N.smooth_random(n, child(r, 'am'), 23.0))
    env = E.bp([(0.0, 0.0), (0.03, 0.75), (0.08, 1.0), (dur - 0.09, 0.95), (dur, 0.0)], n=n, curve='cos')
    breath = F.sweep(N.pink(n, child(r, 'breath')), 'bp', f, 6.0)
    hiss = N.band(n, child(r, 'hiss'), 1800.0, 9000.0)
    # Steam escapes as the valve cracks open, before the pipe speaks: a breath of hiss leads the note.
    hiss_env = E.bp([(0.0, 0.5), (0.02, 1.0), (0.1, 0.45), (dur - 0.08, 0.45), (dur, 0.0)], n=n, curve='cos')
    x = layers((normalize(tone) * env * am, 0.0, 0.0), (normalize(second) * env * am, 0.0, -13.0),
               (normalize(breath) * env, 0.0, -11.0), (normalize(hiss) * hiss_env, 0.0, -21.0))
    return normalize(D.drive(normalize(x), 4.0, 'tanh'), 0.0)


@cue('World.OreLift.Whistle', variations=2, att='Gun', cls='Effects', jitter=0.01, conc=1, level=-5.0)
def whistle(v, r):
    # The winch taking up: the house's steam whistle, two short blasts, a little breathy, the second a minor third lower
    # and a touch longer, thrown back off the canyon walls.
    f0 = [520.0, 495.0][v] * jitter(r, 1, 0.01)
    b1 = _blast(f0, 0.5 + 0.05 * r.random(), child(r, 'b1'))
    b2 = _blast(f0 / 1.189, 0.68 + 0.06 * r.random(), child(r, 'b2'), 0.95)
    x = layers((b1, 0.0, 0.0), (b2, 0.62 + 0.04 * r.random(), -1.0))
    x = F.filt(x, F.hp(150.0), F.peak(3300.0, -3.0, 0.8), F.highshelf(7000.0, -3.0), extend=False)
    return _across(x, r, 2.8, -10.0)


def _house(x, r, wet_db=-9.0):
    """The winch house's timber room, applied in a circle so the loop stays seamless."""
    ir = R.room(child(r, 'house'), t60=0.5, size=1.6, damp=0.35)
    L = x.size
    wet = np.fft.irfft(np.fft.rfft(x, L) * np.fft.rfft(ir[:L], L), L)
    return x + wet / (np.sqrt(np.mean(wet * wet)) + 1e-12) * np.sqrt(np.mean(x * x)) * float(db2a(wet_db))


def _lv(x, db):
    """x scaled so its loudest 200 ms sits at db (LUFS-like): loop layers mixed by how loud they sound."""
    return x * float(db2a(db - A.loudness_max(x)))


def _whole(f, dur):
    """f moved to the nearest whole number of cycles in the loop."""
    return round(f * dur) / dur


@cue('World.OreLift.WinchRun', variations=1, att='Creature', cls='Effects', jitter=0.0, conc=1, loop=True, level=-12.0,
     align=False)
def winch_run(v, r):
    # The winch running: the steam engine turning 1.6 times a second, four exhaust chuffs a turn (two cylinders, the
    # strokes strong and weak by turns), its valve gear ticking once a turn; the drum's gear whining at its mesh; the rope
    # rumbling over the drum and its sheave; a leak hissing; all in the timber house. Five seconds, eight turns.
    dur = 5.0
    L = ns(dur)
    t = times(L)
    turn = dur / 8.0
    events = []
    rc = child(r, 'chuffs')
    for k in range(32):
        # Half way between two chuffs at the seam, so the join falls in a lull.
        at = (k + 0.5) * turn / 4.0 + rc.normal(0.0, 0.003)
        strength = (1.0, 0.68, 0.9, 0.6)[k % 4] * rc.uniform(0.9, 1.1)
        puff = kit.burst(0.16, child(rc, 'puff', k), 0.1, lo=250.0, hi=3500.0, color=-2.0, attack=0.004)
        thump = kit.noise_thump(0.12, child(rc, 'thump', k), 320.0, 70.0, 0.06, 1.0, 0.05, 0.003)
        events.append((layers((puff, 0.0, 0.0), (thump, 0.0, -4.0)) * strength, at % dur))
    for k in range(8):
        tick = kit.clack(child(r, 'gear', k), 520.0, 1100.0, 3600.0, 0.03, 0.04, -4.0, -12.0)
        events.append((tick * 0.22, (k * turn + 0.31 * turn) % dur))
    rb = child(r, 'bearing')
    for k in range(12):
        knock = kit.thunk(300.0 * rb.uniform(0.95, 1.05), child(rb, k), 0.04, 0.001, 5, 0.06)
        events.append((knock * 0.12, (k * dur / 12.0 + 0.07) % dur))
    engine = _fold(L, events)
    # The gear's mesh (112 teeth on the drum's gear) and its pinion, swelling a little with each chuff's load.
    mesh = _whole(1.6 * 112.0, dur)
    load = 0.8 + 0.2 * np.cos(2 * np.pi * _whole(6.4, dur) * t)
    whine = (np.sin(2 * np.pi * mesh * t) + 0.4 * np.sin(2 * np.pi * 2 * mesh * t + 1.0)
             + 0.25 * np.sin(2 * np.pi * _whole(mesh * 3.0, dur) * t + 2.0)) * load
    pinion = np.sin(2 * np.pi * _whole(mesh * 3.0 * 1.003, dur) * t) * load
    rope = _pnoise(L, child(r, 'rope'), 120.0, 1800.0, -3.0) * _slow(L, child(r, 'rg'), 0.8, 0.6)
    leak = _pnoise(L, child(r, 'leak'), 2500.0, 9000.0, -1.0) * _slow(L, child(r, 'lg'), 1.5, 0.7)
    simmer = _pnoise(L, child(r, 'simmer'), 40.0, 260.0, -5.0)
    # Mixed by loudness (the loudest 200 ms), so the chuffs stand clear of the beds under them.
    x = (_lv(engine, 0.0) + _lv(whine, -15.0) + _lv(pinion, -24.0) + _lv(rope, -12.0) + _lv(leak, -22.0)
         + _lv(simmer, -16.0))
    x = _house(x, r, -10.0)
    return _circ(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


# --- The ride -------------------------------------------------------------------------------------------------------------

def _groan(dur, r, rate0=9.0, rate1=16.0, f=170.0):
    """Timber under load groaning: a slow stick-slip through a beam's low resonances."""
    return kit.creak(dur, child(r, 'timber'), rate0, rate1, (f * jitter(r, 1, 0.08), f * 2.5, f * 5.3), 7.0, 0.35)


def _strap(dur, r, rate0=22.0, rate1=36.0):
    """An iron strap or a shackle turning under load: iron grinding on iron, higher and harder than the wood."""
    s = kit.creak(dur, child(r, 'iron'), rate0, rate1, (620.0 * jitter(r, 1, 0.1), 1450.0, 2500.0), 11.0, 0.3)
    return F.filt(s, F.peak(3300.0, -5.0, 0.9), extend=False)


def _links(r, count, spread, lo=900.0, hi=3200.0, start=0.0):
    """Heavy chain links knocking as they shift: short, dull clinks of thick iron."""
    def grain(rr, i):
        m = M.parts(lo * rr.uniform(0.85, 1.15), hi, rr, 5, 0.04, 0.02, 0.5)
        return normalize(M.strike(m, M.hammer(0.00015), 0.08)) * rr.uniform(0.6, 1.0)
    return normalize(G.rattle(spread + 0.1, child(r, 'links'), count, spread, grain, start, 0.85), 0.0)


@cue('World.OreLift.CableRun', variations=1, att='Near', cls='Effects', jitter=0.0, conc=1, loop=True, level=-11.0,
     align=False)
def cable_run(v, r):
    # Riding the cage: the wire rope singing over the grip's rollers (a low hum at its strand rate with a pitched whirr in
    # it), a splice ticking through the grip now and then, the cage swaying (a four-second sway: the timbers groaning and
    # the straps grinding at each end of it, the chains clinking), and a high, gusty wind through the bars. Twelve
    # seconds, three sways, so the ride never audibly repeats.
    dur = 12.0
    L = ns(dur)
    t = times(L)
    f0 = _whole(96.0, dur)
    beat = _whole(96.55, dur)
    hum = (np.sin(2 * np.pi * f0 * t) + 0.7 * np.sin(2 * np.pi * beat * t + 0.7) + 0.5 * np.sin(2 * np.pi * 2 * f0 * t + 1.1)
           + 0.3 * np.sin(2 * np.pi * 3 * f0 * t + 2.3) + 0.16 * np.sin(2 * np.pi * 5 * f0 * t + 0.4))
    # The rope's load and the wind swell and ease with the cage's four-second sway.
    sway = 0.5 + 0.5 * np.cos(2 * np.pi * t / 4.0)
    hum = hum * (0.75 + 0.15 * _slow(L, child(r, 'hg'), 0.4, 0.0) + 0.1 * sway)
    whirr = _circ(_pnoise(L, child(r, 'whirr'), 80.0, 2500.0, -3.0), F.comb(1.0 / f0, 0.93, 1800.0))
    whirr = whirr * (0.75 + 0.25 * _slow(L, child(r, 'wg'), 0.7, 0.0))
    events = []
    # Splices: three, at uneven times.
    for k, at in enumerate((1.3, 5.6, 9.1)):
        tick = kit.clack(child(r, 'splice', k), 480.0, 1000.0, 3400.0, 0.04, 0.05, -2.0, -8.0)
        events.append((F.filt(tick, F.peak(3300.0, -4.0, 0.9), extend=False) * 0.45, at + 0.2 * r.random()))
    # The sway: an end every two seconds; the timbers groan at one end, the straps grind and the chains clink at the other.
    for k in range(6):
        at = k * 2.0 + r.uniform(-0.15, 0.15)
        rk = child(r, 'sway', k)
        if k % 2 == 0:
            events.append((_groan(rk.uniform(0.7, 1.0), rk, rk.uniform(8.0, 11.0), rk.uniform(13.0, 18.0)) * rk.uniform(0.5, 0.8),
                           at % dur))
            events.append((_links(rk, 3, 0.25) * 0.18, (at + 0.3) % dur))
        else:
            events.append((_strap(rk.uniform(0.4, 0.6), rk) * rk.uniform(0.25, 0.4), at % dur))
            events.append((_links(rk, 4, 0.3) * 0.28, (at + 0.1) % dur))
    creaks = _fold(L, events)
    gust = 0.45 + 0.55 * (0.5 + 0.5 * N.smooth_random(L, child(r, 'gust'), 0.25, periodic=True))
    wind = _pnoise(L, child(r, 'wind'), 300.0, 4500.0, -2.5) * gust ** 1.5 * (0.8 + 0.2 * sway)
    bars = np.zeros(L)
    for i, fc in enumerate((1100.0, 1650.0, 760.0)):
        src = _pnoise(L, child(r, 'bar', i), 400.0, 4000.0, 0.0)
        bars += _circ(src, F.bp(fc, 25.0), F.bp(fc, 25.0)) * _slow(L, child(r, 'bg', i), 0.2 + 0.07 * i, 0.0) ** 2
    # Mixed by loudness (the loudest 200 ms): the creaks rise over the rope's bed at each end of the sway.
    x = _lv(hum, 0.0) + _lv(whirr, -4.0) + _lv(creaks, 1.0) + _lv(wind, -5.0) + _lv(bars, -15.0)
    return _circ(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


def _jolt(r, f=120.0):
    """The cage's whole weight caught at once: a heavy push of air, the floor's timbers knocking, its iron frame
    clanking dully."""
    push = kit.noise_thump(0.35, child(r, 'push'), 260.0, 45.0, 0.18, 1.0, 0.12, 0.002)
    floor = kit.thunk(f * jitter(r, 1, 0.06), child(r, 'floor'), 0.16, 0.0015, 7, 0.1)
    frame = normalize(M.strike(M.plate(330.0 * jitter(r, 1, 0.06), child(r, 'frame'), 22, 0.3, 0.6, tilt=0.8),
                               M.hammer(0.0004), 0.5))
    frame = F.filt(frame, F.lp(2500.0, 0.7), extend=False)
    return normalize(layers((push, 0.0, 0.0), (floor, 0.0, -2.0), (frame, 0.002, -7.0)), 0.0)


@cue('World.OreLift.TakeUp', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-7.0)
def take_up(v, r):
    # The rope takes the strain: the iron straps and the cage's timbers groan as the load comes on, then a heavy jolt as
    # it lifts off the landing, the chains snapping taut one after another with a twang of the rope, and the cage
    # creaking as it settles on the line.
    groan = _groan(0.55, child(r, 'groan'), 7.0, 14.0, 150.0)
    strap = _strap(0.42, child(r, 'strap'), 16.0, 30.0)
    t = 0.36 + 0.05 * r.random()
    jolt = _jolt(child(r, 'jolt'))
    taut = _links(child(r, 'taut'), 5, 0.06, 800.0, 2800.0)
    twang = normalize(M.strike(M.bar(62.0 * jitter(r, 1, 0.05), child(r, 'rope'), 0.5, 0.3, 6, 0.01, 0.4), M.hammer(0.002), 0.7))
    twang = F.filt(twang, F.lp(900.0, 0.7), extend=False)
    after = _groan(0.6, child(r, 'after'), 12.0, 7.0, 190.0)
    # The shackle's first knock as the rope comes tight leads it, so the cue speaks at once.
    shackle = kit.clack(child(r, 'shackle'), 480.0, 1000.0, 3200.0, 0.04, 0.05, -2.0, -8.0, 0.00015)
    x = layers((shackle, 0.0, -12.0), (groan, 0.0, -6.0), (strap, 0.06, -10.0), (jolt, t, 0.0), (taut, t + 0.01, -7.0),
               (twang, t, -8.0), (after, t + 0.12, -12.0))
    x = F.filt(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _outside(x, r, -18.0, 0.11, 0.3, 0.8)


@cue('World.OreLift.SheaveClank', variations=5, att='Creature', cls='Effects', jitter=0.05, conc=3, level=-8.0)
def sheave_clank(v, r):
    # The grip riding over one of the tower's sheaves: a heavy iron clank (the grip's jaws on the wheel's rim), the cast
    # wheel ringing a moment, and the cable's shudder running away from it in a low, beating thrum. Three of these come in
    # about a second, so each take has its own wheel and its own weight.
    rim = M.plate(240.0 * jitter(r, 1, 0.12), child(r, 'rim'), 20, 0.12, 0.7, tilt=0.7)
    clank = normalize(M.strike(rim, M.hammer(0.00025), 0.3))
    clang = normalize(M.strike(M.parts(900.0, 3000.0, child(r, 'jaws'), 7, 0.05, 0.02, 0.4), M.hammer(0.0001), 0.1))
    wheel = normalize(M.strike(M.shell(520.0 * jitter(r, 1, 0.1), child(r, 'wheel'), 0.45, 6, 0.03), M.hammer(0.0003), 0.7))
    wheel = F.filt(wheel, F.peak(3300.0, -6.0, 0.8), F.highshelf(5000.0, -6.0), extend=False)
    weight = kit.noise_thump(0.2, child(r, 'weight'), 400.0, 60.0, 0.1, 1.0, 0.08, 0.002)
    n = ns(0.75)
    tt = times(n)
    fs = 48.0 * jitter(r, 1, 0.1)
    shudder = (np.sin(2 * np.pi * fs * tt) + 0.8 * np.sin(2 * np.pi * fs * 1.04 * tt) + 0.4 * np.sin(2 * np.pi * fs * 2.02 * tt))
    shudder = shudder * E.perc(0.75, 0.01, 0.55) * (0.7 + 0.3 * np.sin(2 * np.pi * 11.0 * tt))
    x = layers((clank, 0.0, 0.0), (clang, 0.0, -6.0 + 2.0 * r.random()), (weight, 0.0, -3.0), (wheel, 0.004, -11.0),
               (normalize(shudder), 0.01, -9.0))
    x = F.filt(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _across(x, r, 1.4, -15.0)


@cue('World.OreLift.CageLand', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-7.0)
def cage_land(v, r):
    # Set down: the cage's floor timbers coming down on the landing with a heavy thump and a bounce, the chains slumping
    # slack and clinking down, the timbers easing, then the iron latch dropping into its keeper.
    jolt = _jolt(child(r, 'jolt'), 105.0)
    bounce = kit.thunk(140.0 * jitter(r, 1, 0.06), child(r, 'bounce'), 0.1, 0.0015, 7, 0.1)
    slack = _links(child(r, 'slack'), 7, 0.4, 700.0, 2600.0, 0.03)
    ease = _groan(0.5, child(r, 'ease'), 14.0, 8.0, 180.0)
    t = 0.55 + 0.08 * r.random()
    latch = kit.clack(child(r, 'latch'), 520.0, 1100.0, 3800.0, 0.06, 0.05, -2.0, -8.0, 0.00015)
    latch = F.filt(latch, F.peak(3300.0, -4.0, 0.9), extend=False)
    x = layers((jolt, 0.0, 0.0), (bounce, 0.09 + 0.02 * r.random(), -10.0), (slack, 0.02, -9.0), (ease, 0.12, -13.0),
               (latch, t, -5.0))
    x = F.filt(x, F.hp(35.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _outside(x, r, -18.0, 0.1, 0.27, 0.7)


@cue('World.OreLift.GateLatch', variations=3, att='Near', cls='Effects', jitter=0.04, conc=1, level=-12.0)
def gate_latch(v, r):
    # The cage's iron gate swung shut behind you: its hinge squealing, the gate clacking against its post, the latch
    # bar dropping over.
    d = 0.26 + 0.06 * r.random()
    squeal = kit.creak(d, child(r, 'hinge'), 120.0, 200.0, (850.0 * jitter(r, 1, 0.08), 1900.0, 3100.0), 12.0, 0.25)
    squeal = F.filt(squeal, F.peak(3300.0, -6.0, 0.9), F.highshelf(6000.0, -6.0), extend=False)
    bar = normalize(M.strike(M.bar(430.0 * jitter(r, 1, 0.08), child(r, 'bar'), 0.35, 0.6, 6), M.hammer(0.0002), 0.4))
    bar = F.filt(bar, F.peak(3300.0, -5.0, 0.9), extend=False)
    clack = kit.clack(child(r, 'post'), 420.0, 1000.0, 3600.0, 0.05, 0.06, -1.0, -8.0, 0.00015)
    clack = F.filt(clack, F.peak(3300.0, -4.0, 0.9), extend=False)
    latch = kit.metal_click(child(r, 'latch'), 1200.0, 3800.0, 0.03, 0.0001, 5, -12.0)
    t = d - 0.03
    x = layers((squeal, 0.0, -9.0), (clack, t, 0.0), (bar, t, -8.0), (latch, t + 0.07 + 0.02 * r.random(), -7.0))
    x = F.filt(x, F.hp(70.0), extend=False)
    return _outside(x, r, -20.0, 0.08, 0.2, 0.5)
