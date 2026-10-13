"""The ore lift's fittings and the mine's manway gate (the player's hands on the iron): the steam winch's crank handle
fitted and turned, its boiler fed, the cable clamp's pin driven home, and a rusty grate gate opened in a mine passage. All 3D,
near the player (they are heard once, up close, over a few seconds, so each is a little event of its own).

Cast iron rings by modes (a hard strike on a heavy, dense mode set, a bar's stretched partials for a pin or a latch bar);
iron grinding on rust is stick-slip (kit.creak, the hinge's long groan slow and low, a thin squeal over it); coal is hard,
dull ticks and grit; a fire catching is a breath drawn in and thrust out (a soft-fronted low whoomp) over a roar that climbs
and settles (bosses.py's flame), with a steam hiss escaping. The mine's rock throws the gate's knock straight back
(miners.py's gallery); the winch cues sit in the open yard's short answer (creatures.py's _outside), like the rest of the lift.

- World.LiftFitting.Crank (near, 1.2 s): the handle's socket touching the square shaft and sliding down it, the heavy iron
  clunk of it seating, then three to five ratchet pawl clicks as it is turned, a little gear grind under them.
- World.LiftFitting.Boiler (near, 2 s): the iron fire door's latch lifted and the door swung open (hinge squealing, a clang
  on its stop), two shovelfuls of coal (the blade grating, the coal pouring and rattling), the fire catching with a low
  whoomp, a short steam hiss, the boiler's iron ticking.
- World.LiftFitting.Pin (near, 1 s): the cable clamp's pin driven home: two or three hammer knocks climbing in pitch as it
  seats, a last ringing clink.
- World.ManwayGate.Open (near, 1.8 s): a rusty iron grate gate in a mine passage: the latch bar lifted (a clack), a long low
  hinge groan, the bars rattling in their sockets, the gate ending in a dull knock against the rock.
"""
import numpy as np

from lib.core import ns, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, modal as M, granular as G
from recipes import cue
from recipes import kit
from recipes.bosses import _flame
from recipes.creatures import _outside
from recipes.miners import gallery


def _soft(x):
    """The 2-5 kHz band eased: iron struck close by is bright, and it comes again and again at a winch."""
    return F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)


# --- The crank ---------------------------------------------------------------------------------------------------------------

def _seat(r):
    """The crank's square socket meeting the shaft's shoulder: a hard strike on a heavy, dense cast-iron mode set, the
    handle's weight behind it, its long arm ringing dully for a moment."""
    body = normalize(M.strike(M.plate(250.0 * jitter(r, 1, 0.1), child(r, 'body'), 22, 0.28, 0.6, tilt=0.7), M.hammer(0.0003), 0.4))
    body = F.filt(body, F.lp(3200.0, 0.7), extend=False)
    weight = kit.noise_thump(0.22, child(r, 'weight'), 420.0, 60.0, 0.12, 1.0, 0.1, 0.002)
    sub = kit.thump(98.0 * jitter(r, 1, 0.06), 56.0, 0.2, 0.1)
    arm = normalize(M.strike(M.bar(430.0 * jitter(r, 1, 0.08), child(r, 'arm'), 0.3, 0.6, 5), M.hammer(0.0002), 0.4))
    arm = F.filt(arm, F.peak(3300.0, -5.0, 0.9), extend=False)
    return normalize(layers((body, 0.0, 0.0), (weight, 0.0, -3.0), (sub, 0.0, -9.0), (arm, 0.003, -9.0)), 0.0)


def _pawl(r):
    """One tooth of the ratchet passed: the pawl riding up the tooth (a faint dry tick) and dropping behind it (a sharp
    iron click, the frame's knock and a little weight under it)."""
    ride = kit.metal_click(child(r, 'ride'), 1100.0, 3200.0, 0.012, 0.0002, 4, -10.0)
    drop = kit.clack(child(r, 'drop'), 520.0 * jitter(r, 1, 0.06), 1000.0, 3800.0, 0.035, 0.05, -5.0, -9.0, 0.00012)
    return normalize(layers((ride, 0.0, -14.0), (drop, 0.007 + 0.002 * r.random(), 0.0)), 0.0)


@cue('World.LiftFitting.Crank', variations=4, att='Near', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def crank(v, r):
    # The crank handle fitted to the winch's square shaft and given a turn: the socket touches the shaft's end and slides down
    # it (a short gritty iron scrape), seats against the shoulder with a heavy clunk, then the pawl clicks over the ratchet's
    # teeth as the handle is hauled round (slow at first, quickening as the weight comes on, three to five clicks), the gear
    # grinding low under it, the grip knocking once as the turn ends.
    clicks = (4, 3, 5, 4)[v]
    touch = kit.metal_click(child(r, 'touch'), 800.0, 3000.0, 0.015, 0.00015, 4, -10.0)
    slide = kit.scrape(0.1, child(r, 'slide'), 1500.0 * jitter(r, 1, 0.1), 650.0, 9.0, 0.75, attack=0.08, release=0.35)
    t_seat = 0.11 + 0.025 * r.random()
    parts = [(touch, 0.0, -6.0), (slide, 0.0, -10.0), (_seat(child(r, 'seat')), t_seat, 0.0)]
    t0 = t_seat + 0.2 + 0.04 * r.random()
    t = t0
    gap = 0.18 * jitter(r, 1, 0.1)
    for i in range(clicks):
        parts.append((_pawl(child(r, 'pawl', i)), t, min(-9.0 + 1.3 * i, -5.0) + r.uniform(-1.0, 1.0)))
        last = t
        t += gap * r.uniform(0.92, 1.08)
        gap *= 0.9
    span = last + 0.07 - (t0 - 0.03)
    grind = kit.creak(span, child(r, 'grind'), 78.0, 42.0, (430.0 * jitter(r, 1, 0.08), 950.0, 1900.0), 9.0, 0.35)
    knock = kit.thunk(380.0 * jitter(r, 1, 0.08), child(r, 'grip'), 0.05, 0.0008, 5, 0.08)
    parts += [(grind, t0 - 0.03, -20.0), (knock, last + 0.09, -12.0)]
    x = F.filt(layers(*parts), F.hp(45.0), F.peak(3300.0, -4.0, 0.9), extend=False)
    return _outside(x, r, -19.0, 0.08, 0.2, 0.5)


# --- The boiler --------------------------------------------------------------------------------------------------------------

def _door(r):
    """The iron fire door swung open: its latch lifted (a short click), the hinge squealing on rust, the door's weight
    meeting its stop with a clang. Returns the sound and when the clang falls."""
    d = 0.26 + 0.05 * r.random()
    latch = kit.metal_click(child(r, 'latch'), 900.0, 3600.0, 0.03, 0.00012, 5, -10.0)
    squeal = kit.creak(d, child(r, 'hinge'), 95.0, 175.0, (780.0 * jitter(r, 1, 0.08), 1750.0, 3000.0), 12.0, 0.25)
    squeal = F.filt(squeal, F.peak(3300.0, -6.0, 0.9), F.highshelf(6000.0, -6.0), extend=False)
    plate = M.plate(300.0 * jitter(r, 1, 0.08), child(r, 'door'), 22, 0.5, 0.6, tilt=0.7)
    clang = F.filt(normalize(M.strike(plate, M.hammer(0.00022), 0.6)), F.lp(3400.0, 0.7), extend=False)
    weight = kit.noise_thump(0.2, child(r, 'weight'), 520.0, 70.0, 0.1, 1.0, 0.07, 0.001)
    t_clang = d
    # The squeal begins with the latch, not after it: a gap there reads as two separate sounds.
    return layers((latch, 0.0, -4.0), (squeal, 0.0, -8.0), (clang, t_clang, 0.0), (weight, t_clang, -4.0)), t_clang


def _scoop(r):
    """One shovelful of coal: the iron blade grating through the heap (a gritty scrape), then the coal pouring off it into
    the firebox: a dense shower of hard, dull ticks over a gravelly hiss."""
    blade = kit.scrape(0.15, child(r, 'blade'), 950.0 * jitter(r, 1, 0.1), 480.0, 8.0, 0.8, attack=0.12, release=0.4,
                       grit_rate=3500.0)
    gravel = normalize(G.grit(0.16, child(r, 'gravel'), 4200.0, 500.0, 4200.0, 1.5, 4) * E.ar(0.16, 0.05, 0.08, 1.5))

    def lump(rr, i):
        m = M.parts(rr.uniform(450.0, 900.0), 3600.0, rr, 4, 0.018, 0.007, 0.3)
        return normalize(M.strike(m, M.hammer(0.0002), 0.05)) * rr.uniform(0.3, 1.0)

    pour = normalize(G.rattle(0.3, child(r, 'pour'), 16, 0.26, lump, 0.0, 0.9))
    shower = kit.grit_burst(0.36, child(r, 'shower'), 5200.0, 600.0, 4200.0, 0.2)
    return normalize(layers((blade, 0.0, -2.0), (gravel, 0.02, -8.0), (pour, 0.13, -1.0), (shower, 0.13, -6.0)), 0.0)


def _catch(dur, r):
    """The coal taking: a breath drawn in and thrust out (a low whoomp, soft on its front) and the fire roaring up
    through the bed, then settling."""
    n = ns(dur)
    whump = kit.noise_thump(0.5, child(r, 'whump'), 380.0, 52.0, 0.3, 0.9, 0.22, 0.012)
    sub = kit.thump(78.0, 42.0, 0.45, 0.28, attack=0.01)
    breath = kit.whoosh(0.4, child(r, 'breath'), 200.0, 900.0, 0.8, 0.3, -3.0)
    roar = _flame(dur, child(r, 'roar'), [(0.0, 200.0), (0.25, 620.0 * jitter(r, 1, 0.08)), (0.7, 420.0), (dur, 260.0)],
                  14.0, 0.55, 0.9)
    roar = normalize(roar * E.bp([(0.0, 0.0), (0.05, 0.5), (0.3, 1.0), (dur * 0.6, 0.55), (dur, 0.0)], n=n, curve='cos'))
    return layers((whump, 0.0, 0.0), (sub, 0.0, -7.0), (breath, 0.0, -9.0), (roar, 0.02, -3.0))


def _steam(dur, r):
    """Steam escaping: a broad hiss, torn a little, swelling and fading."""
    n = ns(dur)
    x = N.band(n, child(r, 'steam'), 1600.0, 9000.0) * kit.turbulence(n, child(r, 'tur'), 90.0, 0.5)
    x = F.filt(x, F.highshelf(6500.0, -5.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return normalize(x * E.ar(dur, 0.12, 0.45, 1.6), 0.0)


@cue('World.LiftFitting.Boiler', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-8.0)
def boiler(v, r):
    # Feeding the winch boiler: the fire door's latch lifted and the iron door swung open (hinge squealing, a clang on its stop),
    # the embers breathing in the open firebox, two shovelfuls of coal (the blade grating through the heap, the coal pouring
    # and rattling in), the coal taking with a low whoomp and a roar that climbs and settles, a short steam hiss, and the
    # hot boiler iron ticking as it warms.
    door, t_clang = _door(child(r, 'door'))
    t1 = t_clang + 0.14 + 0.03 * r.random()
    t2 = t1 + 0.3 + 0.04 * r.random()
    t_catch = t2 + 0.34 + 0.04 * r.random()
    embers = _flame(1.2, child(r, 'embers'), [(0.0, 180.0), (0.6, 260.0), (1.2, 340.0)], 10.0, 0.6, 0.9)
    embers = normalize(embers * E.ar(1.2, 0.3, 0.3, 1.5))
    tick1 = kit.metal_click(child(r, 'tick1'), 700.0, 2500.0, 0.025, 0.0002, 4, -10.0)
    tick2 = kit.metal_click(child(r, 'tick2'), 800.0, 2700.0, 0.025, 0.0002, 4, -10.0)
    x = layers((door, 0.0, 0.0), (embers, t_clang, -22.0), (_scoop(child(r, 's1')), t1, -6.0),
               (_scoop(child(r, 's2')), t2, -9.0), (_catch(0.85, child(r, 'catch')), t_catch, -1.0),
               (_steam(0.75, child(r, 'steam')), t_catch + 0.1, -12.0), (tick1, t_catch + 0.38, -20.0),
               (tick2, t_catch + 0.63, -23.0))
    x = F.filt(x, F.hp(40.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _outside(x, r, -19.0, 0.08, 0.2, 0.45)


# --- The pin ------------------------------------------------------------------------------------------------------------------

def _drive(f, r, t60, clamp_f):
    """One blow of the hammer on the pin's head: the pin's own ring (a short iron bar; the clamp grips it, so it rings
    longer the further home it is), the hammer face's tick, a dull weight, and the clamp's iron body ringing low under it."""
    pin = normalize(M.strike(M.bar(f, child(r, 'pin'), t60, 0.6, 5, 0.01, 0.6), M.hammer(0.00015), t60 * 2.0 + 0.02))
    pin = F.filt(pin, F.peak(3300.0, -5.0, 0.9), extend=False)
    tick = kit.metal_click(child(r, 'tick'), 1500.0, 5200.0, 0.01, 0.00008, 4, -8.0)
    body = kit.noise_thump(0.07, child(r, 'body'), 800.0, 140.0, 0.03, 1.0, 0.03, 0.0006)
    clamp = normalize(M.strike(M.plate(clamp_f, child(r, 'clamp'), 18, 0.3, 0.6, tilt=0.8), M.hammer(0.0004), 0.35))
    clamp = F.filt(clamp, F.lp(2600.0, 0.7), extend=False)
    return normalize(layers((pin, 0.0, 0.0), (tick, 0.0, -9.0), (body, 0.0, -6.0), (clamp, 0.001, -11.0)), 0.0)


def _seated(r, clamp_f):
    """The pin's head bottoming on the clamp: a bright, ringing clink (a bar's stretched partials, doubled a hair apart so
    they beat), the clamp's iron answering low, the hammer bouncing once off the pin."""
    m = M.bar(1250.0 * jitter(r, 1, 0.06), child(r, 'pin'), 0.7, 0.55, 6, 0.012, 0.5)
    m = M.merge(m, M.scale(m, 1.0045, 0.9, 0.7))
    ring = normalize(M.strike(m, M.hammer(0.00008), 1.0))
    ring = F.filt(ring, F.peak(3300.0, -5.0, 0.9), F.highshelf(7000.0, -4.0), extend=False)
    clamp = normalize(M.strike(M.plate(clamp_f, child(r, 'clamp'), 20, 0.5, 0.6, tilt=0.7), M.hammer(0.0002), 0.6))
    clamp = F.filt(clamp, F.lp(2800.0, 0.7), extend=False)
    tick = kit.metal_click(child(r, 'tick'), 1500.0, 5200.0, 0.012, 0.00008, 4, -8.0)
    bounce = kit.metal_click(child(r, 'bounce'), 1400.0, 4600.0, 0.01, 0.0001, 4, -8.0)
    return normalize(layers((ring, 0.0, 0.0), (clamp, 0.0, -5.0), (tick, 0.0, -8.0), (bounce, 0.02 + 0.006 * r.random(), -17.0)), 0.0)


@cue('World.LiftFitting.Pin', variations=4, att='Near', cls='Effects', jitter=0.04, conc=1, level=-9.0)
def pin_home(v, r):
    # The cable clamp's pin driven home: two or three hammer blows on its head, each ringing higher and longer than the last
    # as the pin sinks into the clamp and its free length shortens, and then the last, ringing clink as its head bottoms.
    knocks = (3, 2, 3, 3)[v]
    clamp_f = 215.0 * jitter(r, 1, 0.08)
    f = 320.0 * jitter(r, 1, 0.08)
    gap = (0.21, 0.23, 0.2, 0.18)[v]
    parts = []
    t = 0.0
    for i in range(knocks):
        parts.append((_drive(f, child(r, 'drive', i), 0.045 + 0.04 * i, clamp_f), t, -4.0 + 2.0 * i / max(knocks - 1, 1)))
        f *= r.uniform(1.24, 1.32)
        t += gap * r.uniform(0.92, 1.08)
        gap *= 0.9
    parts.append((_seated(child(r, 'seated'), clamp_f), t + 0.02, 0.0))
    x = F.filt(layers(*parts), F.hp(60.0), extend=False)
    return _outside(x, r, -20.0, 0.08, 0.2, 0.5)


# --- The manway gate ---------------------------------------------------------------------------------------------------------

def _hinge(dur, r):
    """Rust-seized hinge pins turning under a heavy gate: a long, low stick-slip groan (quickening as the gate gets going,
    slowing as it swings wide) with a thin squeal of rust coming and going above it."""
    body = (130.0 * jitter(r, 1, 0.08), 330.0, 780.0, 1700.0)
    d1 = dur * 0.5
    a = kit.creak(d1 + 0.06, child(r, 'a'), 9.0, 21.0, body, 8.0, 0.35)
    b = kit.creak(dur - d1 + 0.06, child(r, 'b'), 21.0, 8.0, body, 8.0, 0.35)
    low = layers((a, 0.0, 0.0), (b, d1 - 0.06, 0.0))
    s = kit.creak(dur, child(r, 'squeal'), 110.0, 170.0, (700.0 * jitter(r, 1, 0.08), 1500.0, 2800.0), 14.0, 0.25)
    s = F.filt(s, F.peak(3300.0, -5.0, 0.9), F.highshelf(6000.0, -5.0), extend=False)
    s = s * np.clip(N.smooth_random(s.size, child(r, 'squeal_am'), 3.0), 0.0, None) ** 1.5
    x = layers((low, 0.0, 0.0), (s, 0.0, -9.0))
    swell = E.bp([(0.0, 0.6), (dur * 0.45, 1.0), (dur + 0.06, 0.7)], n=x.size, curve='cos')
    return normalize(x * swell, 0.0)


def _bars(dur, r, rate, start=0.0):
    """The gate's iron bars knocking in their loose sockets and against each other: short clinks of thin iron rods, at
    rate(t) a second."""
    def grain(rr, t, i):
        m = M.bar(rr.uniform(650.0, 1700.0), rr, 0.07, 0.6, 4, 0.01, 0.6)
        return normalize(M.strike(m, M.hammer(0.00015), 0.12)) * rr.uniform(0.25, 1.0)

    return G.cloud(dur, r, rate, grain, start=start)


@cue('World.ManwayGate.Open', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def manway_gate(v, r):
    # A rusty grate gate opened in a mine passage: the hand on the latch bar, the bar rubbing out of its staple and lifted with
    # a clack, then the long low groan of the hinges as the gate swings (the bars rattling in their sockets, more as it speeds
    # up, a trickle of rust flakes), and the gate's edge meeting the rock with a dull iron knock, its bars shivering after.
    # The passage's bare rock throws it back.
    touch = kit.metal_click(child(r, 'touch'), 700.0, 2600.0, 0.015, 0.00015, 4, -10.0)
    lift = kit.scrape(0.08, child(r, 'lift'), 650.0, 1400.0, 10.0, 0.8, attack=0.1, release=0.3)
    bar = normalize(M.strike(M.bar(300.0 * jitter(r, 1, 0.08), child(r, 'lbar'), 0.4, 0.6, 6), M.hammer(0.00018), 0.45))
    bar = F.filt(bar, F.peak(3300.0, -5.0, 0.9), extend=False)
    clack = kit.clack(child(r, 'latch'), 460.0 * jitter(r, 1, 0.06), 900.0, 3300.0, 0.045, 0.06, -2.0, -8.0, 0.00015)
    clack = F.filt(clack, F.peak(3300.0, -4.0, 0.9), extend=False)
    t_clack = 0.1
    t_hinge = t_clack + 0.05
    swing = 1.0 + 0.08 * r.random()
    hinge = _hinge(swing, child(r, 'hinge'))
    t_knock = t_hinge + swing - 0.05
    bars = _bars(swing + 0.1, child(r, 'bars'), lambda t: 5.0 + 16.0 * np.sin(np.pi * min(t / swing, 1.0)) ** 2, 0.04)
    flakes = normalize(G.grit(swing, child(r, 'flakes'), 260.0, 1200.0, 4500.0, 2.0, 4)
                       * E.ar(swing, swing * 0.3, swing * 0.4, 1.5))
    frame = kit.thunk(170.0 * jitter(r, 1, 0.08), child(r, 'frame'), 0.12, 0.0015, 7, 0.1)
    rock = kit.noise_thump(0.3, child(r, 'rock'), 600.0, 55.0, 0.14, 1.0, 0.06, 0.0008)
    iron = F.filt(normalize(M.strike(M.plate(210.0 * jitter(r, 1, 0.08), child(r, 'iron'), 18, 0.2, 0.6, tilt=0.8),
                                     M.hammer(0.0008), 0.3)), F.lp(1400.0, 0.7), extend=False)
    shiver = _bars(0.4, child(r, 'shiver'), lambda t: 40.0 * np.exp(-t / 0.15))
    dust = kit.grit_burst(0.4, child(r, 'dust'), 1500.0, 600.0, 3000.0, 0.25)
    x = layers((touch, 0.0, -8.0), (lift, 0.0, -9.0), (bar, t_clack, -6.0), (clack, t_clack, 0.0),
               (hinge, t_hinge, -5.0), (bars, t_hinge + 0.04, -14.0), (flakes, t_hinge + 0.1, -26.0),
               (frame, t_knock, -1.0), (rock, t_knock, -3.0), (iron, t_knock, -9.0), (shiver, t_knock + 0.02, -17.0),
               (dust, t_knock + 0.03, -16.0))
    x = F.filt(x, F.hp(45.0), extend=False)
    return gallery(x, r, -13.0, 0.55, 2.2)
