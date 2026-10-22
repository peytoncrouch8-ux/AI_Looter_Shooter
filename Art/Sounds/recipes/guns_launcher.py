"""The Powderkeg rocket launcher's sounds (Phase 12.3, 2026-10-21 night): a one-shot tube; the rocket flies at ~40 m/s and
bursts with a 3.5 m splash. Its launch is the same layer set as the other guns' (recipes/guns_shots.py), plus the
rocket's flight loop, its explosion near and far, and the two loading steps.

- Core (the launch): the motor's ignition (a short, hard pop), an automatic cannon's recorded thump for the shove (its
  crack rolled off: a tube, not a barrel), the backblast's torn roar, and the rocket leaving: a recorded designed
  explosion's approaching whoosh played backwards (an approach reversed is a departure), dulled as it goes.
- Mech: the trigger's clack and the tube ringing as the rocket's fins scrape out of it.
- Low: the cannon's lows and a long heave.
- Tails: the land answering the launch (the cannon's own recorded report as its roll); a room as the others'.
- Distant: the cannon made far, a far whoosh behind it.
- Explosion: a hard crack and a sharp recorded blast in front (a large explosion without its tail), the body of a larger
  one or a mine's (four recorded designed explosions, never two the same), a deep heave, the debris raining back (soil,
  stones, the casing's shards ringing), and the land's long roll. Explosion.Distant: a recorded far explosion or a cannon
  heard through a forest, the near blast made far under it.
- Flight (a loop): the motor's roar fluttering, a thin fin whistle.
- RocketOut / RocketIn: a rocket drawn from the back (canvas, the fins brushing steel), slid down the tube (a long
  steel scrape), seated with a hollow knock, the latch snapped over.

Recordings: Art/Sounds/packs/Sonniss/Launcher_*.wav, Explosion_*.wav (sources in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, db2a, varispeed, reverse
from lib import noise as N, env as E, filters as F, dist as D, dynamics as Y, modal as M
from lib import granular as GR
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_shots as GS
from recipes.guns_rec import (SHOTS, _loud, _take, _roll, _gated, _land_wet, _with_roll, _master, _rec_room,
                               _synth_room, _far, _tick, _ping)

SHOTS.update({
    'Launcher_MK44.wav': [0.116],
    'Explosion_Large.wav': [0.053],
    'Explosion_NoTail.wav': [0.076],
    'Explosion_MedTail.wav': [0.141],
    'Explosion_Mine2.wav': [0.172],
    'Explosion_Whoosh.wav': [0.16],
    'Explosion_Far.wav': [0.191],
    'Explosion_Cannon_Far.wav': [0.523, 2.843, 7.378, 9.387],
})


def _whoosh_away(r, dur, speed=1.0):
    """The rocket leaving: the recorded whoosh's approach (its last seconds, rising into the blast it led to) played
    backwards, so it falls away, its top dulling as the rocket goes."""
    w = S.load('Launcher_Whoosh.wav')
    w = w[ns(2.33 - dur - 0.03):ns(2.33 - 0.03)]
    w = reverse(w)
    w = varispeed(w, speed)
    n = w.size
    t = times(n)
    # A moving low-pass: band by band, the highs fade first (the air and the distance).
    lo = F.filt(w, F.lpn(900.0, 2), extend=False)
    hi = w - lo
    w = F.filt(lo, F.hpn(140.0, 2), extend=False) * np.exp(-t / (dur * 0.3)) + hi * np.exp(-t / (dur * 0.2))
    k = ns(0.004)
    w[:k] *= np.linspace(0.0, 1.0, k)
    return E.end_fade(w, 0.03)


def _launcher_dry(v, r):
    """The launch before its master: the ignition pop, the cannon's thump, the backblast, the rocket going."""
    thump = _gated(_take('Launcher_MK44.wav', 0, 1.2), 0.1, 0.5, lp=2600.0, hp=30.0)
    thump = varispeed(thump, [1.0, 0.94, 1.06, 0.9][v % 4])
    pop = G._crack(child(r, 'pop'), 0.0006 * jitter(r, 1.0, 0.12), 0.006, 900.0 * jitter(r, 1.0, 0.08), -4.0, -2.0,
                   1200.0, 8000.0)
    gas = G._gas(child(r, 'gas'), 0.5, 0.03, 0.32, 9000.0, 0.08, 500.0, 7000.0, -2.0, 8.0, -3.0, 0.45, 1800.0,
                 70.0, 0.5, 4)
    heave = G._heave(child(r, 'heave'), 0.012 * jitter(r, 1.0, 0.12), 100.0, 0.18)
    away = _whoosh_away(child(r, 'away'), 1.1 + 0.2 * r.random(), [1.0, 0.92, 1.08, 0.96][v % 4])
    x = layers((_loud(thump), 0.0, 0.0), (pop, 0.0, -9.0), (gas, 0.002, -5.0), (heave, 0.0, -1.0))
    x = Y.transient(x, 3.0, -1.0)
    x = F.filt(x, F.hp(30.0), F.lowshelf(100.0, 4.0), F.peak(350.0, -2.0, 0.9), F.peak(3200.0, -3.0, 0.9),
               F.highshelf(7000.0, -3.0), extend=False)
    x = D.drive(normalize(x), 3.5, 'tanh')
    # The rocket leaving is laid on after the drive, so the overdrive and the master don't hold it up into a drone.
    x = layers(x, (kit.set_level(away, x, -13.0, 0.05), 0.02))
    return _loud(G._early(x, r, -13.0, 0.04, 4000.0), 0.002)


def _launcher_mech(v, r):
    """The trigger's clack, then the tube ringing as the fins scrape out of its mouth."""
    tick = _tick(r, 1500.0, 5000.0, -8.0)
    trigger = G._clank(child(r, 'trigger'), (1000.0, 3400.0, 0.025), (500.0, 0.03), (170.0, 0.03), (0.0, -4.0, -8.0))
    f0 = 310.0 * jitter(r, 1.0, 0.05)
    tube = M.strike(M.shell(f0, child(r, 'tube'), 0.35, 8, 0.02), M.hammer(0.0003), 0.6)
    fins = kit.scrape(0.08, child(r, 'fins'), 900.0, 1400.0, 10.0, 0.6, (1.0, 1.47, 2.31), 0.1, 0.5, 2400.0, -32.0)
    x = layers((_loud(tick, 0.005), 0.0, -10.0), (trigger, 0.002, 0.0), (_loud(tube, 0.005), 0.01, -10.0),
               (_loud(fins, 0.005), 0.012, -16.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _launcher_low(v, r):
    rec = _take('Launcher_MK44.wav', 0, 0.5)
    rec = varispeed(rec, [1.0, 0.92, 1.05, 0.96][v % 4])
    rec = F.filt(rec, F.hpn(25.0, 2), F.lpn(130.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.016 * jitter(r, 1.0, 0.12), 90.0, 0.25)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -1.0))
    x = D.drive(normalize(x), 4.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0004, 0.32), 0.01)
    return _loud(F.filt(x, F.lpn(180.0, 2), extend=False), 0.01)


def _launcher_open(dry, v, r):
    # The launch answered by the hills; the cannon's own recorded report (the land's answer two seconds on) and its roll.
    s1 = 0.07 + 0.04 * r.random()
    s2 = s1 + 0.13 + 0.07 * r.random()
    wet = _land_wet(dry, r, 2.2, ((s1, -2.0), (s2, -4.0)), 24, 0.3, 1.7, -3.0, 1.5, 500.0, 0.3, -16.0, 110.0)
    roll = S.body_only(S.load('Launcher_MK44.wav'), 0.35 + 0.1 * (v % 2), 2.6, 0.05, 0.1)
    roll = F.filt(S.gate_tail(roll, 1.2, 1.2), F.hpn(100.0, 2), F.lpn(2600.0, 2), extend=False)
    return _with_roll(wet, roll, 0.15, -3.0)


def _launcher_town(dry, v, r):
    s1 = 0.025 + 0.015 * r.random()
    gap = 0.022 + 0.012 * r.random()
    slaps = [(s1, -0.5), (s1 + 0.04 + 0.025 * r.random(), -2.5)] + [(s1 + gap * (k + 1), -3.5 - 2.2 * k) for k in range(7)]
    return _land_wet(dry, r, 1.4, slaps, 10, 0.15, 0.8, -5.0, 0.8, 650.0, 0.15, -14.0, 120.0, -6.0, 0.06)


def _launcher_inside(dry, v, r):
    rec = _rec_room('Shotgun_870_Warehouse.wav', v + 1, 2.4, 0.4, 1.5, 90.0, 4500.0)
    room = _synth_room(dry, r, 0.7, 0.8, -10.0, 110.0)
    return layers(room, kit.set_level(rec, room, 3.0, 0.02))


def _launcher_distant(v, r):
    x = _far(S.gate_tail(_take('Launcher_MK44.wav', 0, 2.5), 0.3, 1.4), 1600.0, 70.0, -10.0, 0.03)
    x = varispeed(x, [1.0, 0.93, 1.06, 0.97][v % 4])
    away = F.filt(_whoosh_away(child(r, 'away'), 1.0, 0.95), F.lpn(1800.0, 2), extend=False)
    x = layers((_loud(x), 0.0, 0.0), (_loud(away, 0.05), 0.03, -12.0))
    x = F.filt(x, F.hpn(60.0, 2), F.lpn(4200.0, 2), F.lowshelf(150.0, 3.0), F.peak(3100.0, -4.0, 0.9), extend=False)
    wet = G._land(x, r, 1.8, ((0.1 + 0.06 * r.random(), -3.0),), 16, 0.25, 1.4, -4.0, 1.2, 450.0, 0.25, -11.0, 90.0)
    return _master(layers(_loud(x), wet), 4.0, 0.0015, 0.06)


GS.KINDS['Launcher'] = dict(dry=_launcher_dry, mech=_launcher_mech, low=_launcher_low, open=_launcher_open,
                            town=_launcher_town, inside=_launcher_inside, distant=_launcher_distant, clip=5.0,
                            look=0.0015, rel=0.06)
GS.MIX['Launcher'] = dict(mech=-15.0, low=-3.0)

GS._register('Launcher', 4, 4, dict(fire=0.5, core=-0.1, mech=-15.0, low=-6.5, open=-11.0, town=-10.0, inside=-7.0,
                                    distant=-2.5, n_distant=4), 2)


@cue('Weapon.Launcher.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def launcher_low_ammo(v, r):
    # An empty tube knocked: a hollow steel note. (A one-round tube never reaches the last quarter's rule; kept so the
    # kind has the whole layer set.)
    return _ping(r, [640.0, 680.0, 610.0][v], 0.09, -14.0)


# --- The explosion -------------------------------------------------------------------------------------------------------

_BODY = [('Explosion_Large.wav', 0), ('Explosion_MedTail.wav', 0), ('Explosion_Mine2.wav', 0), ('Explosion_Whoosh.wav', 0),
         ('Explosion_Large.wav', 0)]


def _debris(r, dur, count):
    """What the blast threw up raining back: soil pattering, stones knocking, the rocket's casing shards ringing, thinning
    out over a second and more."""
    n = ns(dur)
    t = times(n)
    soil = GR.grit(dur, child(r, 'soil'), 2600.0 * np.exp(-t / (dur * 0.35)) + 60.0, 400.0, 3500.0, 1.5, 4)
    soil = F.filt(soil, F.lp(4000.0), extend=False) * E.end_fade(E.ar(dur, 0.15, dur * 0.85, 2.0))

    def stone(rr, i):
        return kit.thunk(rr.uniform(500.0, 1400.0), rr, rr.uniform(0.02, 0.05), 0.0004, 5, 0.1) * rr.uniform(0.3, 1.0)

    stones = normalize(GR.rattle(dur * 0.8, child(r, 'stones'), count, dur * 0.7, stone, 0.12, 0.85), 0.0)

    def shard(rr, i):
        m = M.parts(rr.uniform(3500.0, 5500.0), 9000.0, rr, 4, rr.uniform(0.03, 0.08), None, 0.3)
        return M.strike(m, M.hammer(0.00005), 0.15) * rr.uniform(0.3, 1.0)

    shards = normalize(GR.rattle(dur * 0.6, child(r, 'shards'), max(3, count // 3), dur * 0.5, shard, 0.2, 0.8), 0.0)
    return layers((normalize(soil), 0.0, 0.0), (stones, 0.0, -4.0), (shards, 0.0, -12.0))


def _explosion_dry(v, r):
    """The blast near: a crack, the recorded sharp blast, a bigger recorded body under it, a deep heave."""
    front = _gated(_take('Explosion_NoTail.wav', 0, 1.2), 0.15, 0.6, hp=30.0)
    front = varispeed(front, [1.0, 0.9, 1.08, 0.95, 0.86][v % 5])
    name, k = _BODY[v % len(_BODY)]
    body = _take(name, k, 3.2)
    body = varispeed(body, [0.96, 1.04, 0.92, 1.0, 0.88][v % 5])
    body = F.filt(S.gate_tail(body, 0.6, 2.0), F.hpn(28.0, 2), extend=False)
    crack = G._crack(child(r, 'crack'), 0.0009 * jitter(r, 1.0, 0.1), 0.008, 700.0 * jitter(r, 1.0, 0.08), -4.0, -1.0,
                     900.0, 7000.0)
    heave = G._heave(child(r, 'heave'), 0.024 * jitter(r, 1.0, 0.12), 80.0, 0.35)
    x = layers((_loud(front), 0.0, 0.0), (_loud(body, 0.05), 0.0, -6.0), (crack, 0.0, -7.0), (heave, 0.0, -3.0))
    x = Y.transient(x, 6.0, -2.0)
    x = F.filt(x, F.hp(25.0), F.lowshelf(80.0, 4.0), F.peak(400.0, -2.0, 0.8), F.peak(3200.0, -3.0, 0.9),
               F.highshelf(8000.0, -3.0), extend=False)
    return D.drive(normalize(x), 5.0, 'tanh')


@cue('Weapon.Launcher.Explosion', variations=5, att='Gun', jitter=0.04, conc=4, level=3.5)
def launcher_explosion(v, r):
    # The rocket bursting: crack and blast, a bigger body under it, the debris raining back, the land's long roll.
    dry = _explosion_dry(v, child(r, 'dry'))
    deb = _debris(child(r, 'debris'), 1.8 + 0.3 * r.random(), 22 + 4 * v)
    s1 = 0.08 + 0.05 * r.random()
    wet = G._land(dry, child(r, 'land'), 3.0, ((s1, -2.0), (s1 + 0.18 + 0.1 * r.random(), -4.0)), 26, 0.35, 2.4, -1.0,
                  2.0, 380.0, 0.4, -12.0, 90.0)
    x = layers((dry, 0.0, 0.0), (kit.set_level(deb, dry, -16.0, 0.05), 0.12, 0.0), wet)
    return _master(x, 4.0, 0.002, 0.08)


@cue('Weapon.Launcher.Explosion.Distant', variations=4, att='Gun', jitter=0.04, conc=3, level=0.0)
def launcher_explosion_distant(v, r):
    # Far off: a dull boom, its top taken by the air, the land rolling it on for seconds (a recorded far explosion or a
    # cannon heard through a forest), the near blast made far under it.
    if v == 0:
        rec = S.gate_tail(_take('Explosion_Far.wav', 0, 3.4), 1.2, 1.6)
    else:
        rec = S.gate_tail(_take('Explosion_Cannon_Far.wav', v - 1, 2.6), 0.8, 1.6)
    rec = F.filt(rec, F.hpn(40.0, 2), F.lpn(3500.0, 2), extend=False)
    near = _far(_explosion_dry(v + 1, child(r, 'dry')), 1400.0, 50.0, -8.0, 0.04)
    x = layers((_loud(rec, 0.05), 0.0, 0.0), (_loud(near, 0.05), 0.0, -4.0))
    x = F.filt(x, F.lowshelf(140.0, 3.0), F.peak(3100.0, -4.0, 0.9), extend=False)
    wet = G._land(x, r, 2.6, ((0.14 + 0.06 * r.random(), -3.0),), 18, 0.3, 2.2, -2.0, 1.8, 350.0, 0.35, -10.0, 70.0)
    return _master(layers(x, wet), 4.0, 0.002, 0.08)


@cue('Weapon.Launcher.Flight', variations=1, att='Creature', jitter=0.03, conc=3, loop=True, level=-11.0)
def launcher_flight(v, r):
    # The rocket's motor burning as it flies: a torn roar fluttering many times a second over a steady hiss, and a thin
    # whistle off the fins. A loop of 1.2 s, built in a circle.
    dur = 1.2
    n = ns(dur)
    t = times(n)
    roar = N.shaped(n, child(r, 'roar'), -3.0, periodic=True)
    roar = F.filt(roar, F.hp(180.0, 0.7), F.lp(2600.0, 0.7), circular=True)
    flut = 0.55 + 0.45 * np.abs(N.smooth_random(n, child(r, 'flut'), 60.0, periodic=True)) ** 0.8
    roar = D.drive(normalize(roar * flut), 6.0, 'tanh')
    hiss = F.filt(N.shaped(n, child(r, 'hiss'), 0.0, periodic=True), F.hp(4500.0, 0.7), F.lp(10000.0, 0.7),
                  circular=True)
    f = 1650.0
    whistle = np.sin(2.0 * np.pi * np.round(f * dur) / dur * t + 0.4 * np.sin(2.0 * np.pi * 5.0 / dur * t))
    whistle = whistle * (0.7 + 0.3 * np.sin(2.0 * np.pi * 3.0 / dur * t))
    low = F.filt(N.shaped(n, child(r, 'low'), -6.0, periodic=True), F.lp(220.0, 0.7), F.hp(50.0, 0.7), circular=True)
    x = layers((normalize(roar), 0.0, 0.0), (normalize(hiss), 0.0, -14.0), (normalize(whistle), 0.0, -24.0),
               (normalize(low), 0.0, -8.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), circular=True)


# --- Loading -------------------------------------------------------------------------------------------------------------

@cue('Weapon.Launcher.RocketOut', variations=3, att='Near', jitter=0.03, conc=2, level=-14.0)
def launcher_rocket_out(v, r):
    # A rocket drawn from the pack on the back: the canvas flap's rustle, the steel fins brushing its lip, a hand closing
    # on the casing (a dull knock).
    flap = kit.cloth(0.22, child(r, 'flap'), 300.0, 2600.0, 120.0, 0.25, 0.6)
    fins = kit.scrape(0.12, child(r, 'fins'), 700.0, 1100.0, 9.0, 0.6, (1.0, 1.5, 2.4), 0.2, 0.5, 2200.0, -32.0)
    grip = kit.thunk(380.0 * jitter(r, 1.0, 0.06), child(r, 'grip'), 0.05, 0.001)
    tick = kit.metal_click(child(r, 'tick'), 1400.0, 4200.0, 0.02, 0.0001, 4, -10.0)
    return layers((_loud(tick, 0.005), 0.0, -14.0), (_loud(flap), 0.0, -8.0), (_loud(fins), 0.09, -12.0),
                  (_loud(grip, 0.005), 0.2, -2.0))


@cue('Weapon.Launcher.RocketIn', variations=3, att='Near', jitter=0.03, conc=2, level=-10.0)
def launcher_rocket_in(v, r):
    # The rocket's nose tapped into the tube's breech, slid down it (a long steel scrape with the tube ringing faintly),
    # seated against the stop with a hollow knock, and the latch snapped over it.
    tap = G._clank(child(r, 'tap'), (900.0, 3000.0, 0.03), (420.0, 0.04), (160.0, 0.04), (0.0, -3.0, -8.0))
    slide = kit.scrape(0.3, child(r, 'slide'), 520.0, 380.0, 6.0, 0.7, (1.0, 1.6, 2.5), 0.15, 0.4, 1800.0, -34.0)
    f0 = 300.0 * jitter(r, 1.0, 0.05)
    tube = M.strike(M.shell(f0, child(r, 'tube'), 0.25, 8, 0.02), M.hammer(0.0004), 0.5)
    seat = G._clank(child(r, 'seat'), (700.0, 2600.0, 0.06), (260.0 * jitter(r, 1.0, 0.05), 0.08),
                    (100.0 * jitter(r, 1.0, 0.05), 0.09), (-2.0, 0.0, -1.0), 0.00015, drive_db=5.0)
    latch = G._clank(child(r, 'latch'), (1400.0, 4200.0, 0.025), (760.0, 0.02), (250.0, 0.02), (0.0, -5.0, -11.0))
    t_seat = 0.33 + 0.03 * r.random()
    return layers((tap, 0.0, -6.0), (_loud(slide), 0.02, -18.0), (_loud(tube, 0.005), t_seat, -9.0), (seat, t_seat, 0.0),
                  (latch, t_seat + 0.07 + 0.01 * r.random(), -4.0))
