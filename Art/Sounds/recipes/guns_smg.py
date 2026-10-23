"""The Rustler SMG's sounds (Phase 12.3, 2026-10-21 night): a fast, bright little gun (~900 rounds a minute) fed by a
helix magazine laid along its top. Its shot is the same layer set as the other guns' (recipes/guns_shots.py: Fire, Core,
Mech, Low, TailOpen, TailTown, TailInside, Distant, LowAmmo; Weapons/GunshotSound.cpp plays them), built the same way
from recordings, and its reload's two steps.

- Core: an MP7 recorded on the gun (the crisp, fast front) over a 9 mm's bark at 1-2 m (an Uzi, an MP5, a Vz61 or the
  MP7 off axis), the synthesized crack and a small heave under them; tight (gated at 35 ms), its brightness put up at
  6-9 kHz rather than in the ear's touchy 2-5 kHz, since it fires fifteen times a second.
- Mech: the open bolt slamming (a recorded TMP's bolt going home, on synthesized clacks) and the helix's whirr: the spring
  turning the column a round on, a few quick ratchet ticks over a tiny rotary hum.
- Low: an MP5's and an Uzi's lows on a short heave: a small push, so a burst doesn't boom.
- Tails: shorter and quieter than the rifle's (a burst lays them over each other): the open tail rolls with the far Uzi's
  and MP5's fields; inside, the HK416's small room or the warehouse.
- Distant: far MP5, Uzi, Vz61 and P90 takes, and two near takes made far.
- HelixOut / HelixIn: the helix unlatched and slid off (a recorded G36C's polymer magazine under synthesized clacks), and a
  fresh one seated with its spring winding up (a rising ratchet).

Recordings: Art/Sounds/packs/Sonniss/SMG_*.wav, Foley_G36C_Mag.wav (sources in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, dist as D, dynamics as Y, modal as M
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_shots as GS
from recipes.guns_rec import (SHOTS, _loud, _take, _roll, _gated, _land_wet, _with_roll, _master, _rec_room,
                               _synth_room, _far, _tick, _ping)

SHOTS.update({
    'SMG_MP7_Gun.wav': [0.070, 0.255, 0.431, 0.611],
    'SMG_MP7_1m.wav': [0.088, 0.271, 0.449, 0.628],
    'SMG_Uzi_1m.wav': [0.099, 0.328, 0.553, 0.806, 1.055],
    'SMG_MP5_2m.wav': [0.094, 0.299],
    'SMG_Vz61_1m5.wav': [0.096, 0.340],
    'SMG_MP5_75m.wav': [0.116, 0.318],
    'SMG_Uzi_75m.wav': [0.107, 0.334, 0.559, 0.813, 1.062],
    'SMG_Vz61_50m.wav': [0.135, 0.379],
    'SMG_P90_75m.wav': [0.111, 0.308],
    # The TMP's bolt drawn back and let go, six times; the slams (the bolt going home) are indexes 1, 4, 7, 9, 11, 13.
    'SMG_TMP_Bolt.wav': [0.091, 0.488, 1.052, 1.147, 1.473, 2.038, 2.136, 2.796, 3.466, 4.453, 5.113, 5.547, 6.057,
                         6.654],
    'Foley_G36C_Mag.wav': [0.191, 0.301, 0.436, 0.564, 1.359, 1.713, 1.818, 2.471, 2.598, 2.687, 3.405, 3.616, 3.796,
                           4.446, 4.559, 5.332, 5.463, 6.386, 6.543, 7.506, 7.590],
})

_SLAMS = [1, 4, 7, 9, 11, 13]
_BODIES = [('SMG_Uzi_1m.wav', 0), ('SMG_MP5_2m.wav', 0), ('SMG_Vz61_1m5.wav', 0), ('SMG_MP7_1m.wav', 1),
           ('SMG_Uzi_1m.wav', 2), ('SMG_MP5_2m.wav', 1)]


def _smg_dry(v, r):
    """The core before its master: the MP7 on the gun (front) over a 9 mm's bark, the crack and a small heave."""
    # The gun-mounted microphone also hears the bolt coming back 50 ms on: gated off before it.
    snap = _gated(_take('SMG_MP7_Gun.wav', v, 0.16), 0.03, 0.05, hp=60.0)
    name, k = _BODIES[v % len(_BODIES)]
    body = _gated(_take(name, k + v // len(_BODIES), 0.18), 0.03, 0.14, lp=8000.0, hp=70.0)
    crack = G._crack(child(r, 'crack'), 0.00022 * jitter(r, 1.0, 0.12), 0.005, 2000.0 * jitter(r, 1.0, 0.08), -2.0, -5.0)
    heave = G._heave(child(r, 'heave'), 0.0045 * jitter(r, 1.0, 0.15), 160.0, 0.06)
    x = layers((_loud(snap), 0.0, 0.0), (_loud(body), 0.0, -2.5), (crack, 0.0, -9.0), (heave, 0.0, -5.0))
    x = Y.transient(x, 6.0, -5.0)
    x = F.filt(x, F.hp(70.0), F.peak(230.0, 1.5, 0.9), F.peak(480.0, -3.0, 1.0), F.peak(1400.0, 2.0, 1.1),
               F.peak(3300.0, -2.0, 0.9), F.highshelf(6500.0, 1.5), extend=False)
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _loud(G._early(x, r, -16.0, 0.02, 5500.0), 0.002)


def _whirr(r, dur=0.05, rot=None, db=-6.0):
    """The helix turning a round on: a few quick ratchet ticks (the follower's pawl over the teeth) over a tiny rotary
    hum, kept between 700 Hz and 1.8 kHz and above 6 kHz, out of the touchy band."""
    rot = rot or 340.0 * jitter(r, 1.0, 0.08)
    n = ns(dur)
    t = times(n)
    hum = sum(np.sin(2.0 * np.pi * rot * h * t + r.uniform(0, 6.28)) / h for h in (3, 4, 5))
    hum = F.filt(hum * (0.6 + 0.4 * np.sin(2.0 * np.pi * rot * t)), F.bp(1200.0, 1.2), extend=False)
    hum = hum * E.end_fade(E.perc(dur, 0.003, dur * 0.7))
    ticks = np.zeros(n)
    for i in range(4):
        c = kit.metal_click(child(r, 'pawl', i), 6000.0, 11000.0, 0.006, 0.00003, 3, -16.0)
        at = ns(0.004 + i / rot * 1.5)
        m = min(c.size, n - at)
        if m > 0:
            ticks[at:at + m] += c[:m] * (0.9 ** i)
    return layers((_loud(hum, 0.005), 0.0, 0.0), (_loud(ticks, 0.003), 0.0, db))


def _smg_mech(v, r):
    """The open bolt slamming home (the TMP's recorded bolt on a synthesized clack) and the helix turning a round on."""
    tick = _tick(r, 2400.0, 7000.0, -8.0)
    rec = _gated(_take('SMG_TMP_Bolt.wav', _SLAMS[v % len(_SLAMS)], 0.07), 0.02, 0.04, hp=300.0)
    slam = G._clank(child(r, 'slam'), (1300.0, 4400.0, 0.02), (720.0 * jitter(r, 1.0, 0.06), 0.02), (240.0, 0.02),
                    (0.0, -4.0, -9.0), 0.00005)
    home = _loud(layers((_loud(slam, 0.005), 0.0, -2.0), (_loud(rec, 0.005), 0.0, 0.0)))
    whirr = _whirr(child(r, 'whirr'))
    t_home = 0.012 + 0.004 * r.random()
    x = layers((_loud(tick, 0.005), 0.0, -10.0), (home, t_home, 0.0), (whirr, t_home + 0.008, -9.0))
    return F.filt(x, F.peak(3200.0, -5.0, 0.8), extend=False)


def _smg_low(v, r):
    rec = _take(('SMG_MP5_2m.wav', 'SMG_Uzi_1m.wav', 'SMG_Vz61_1m5.wav')[v % 3], v, 0.15)
    rec = F.filt(rec, F.hpn(40.0, 2), F.lpn(190.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.005 * jitter(r, 1.0, 0.15), 140.0, 0.08)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -4.0))
    x = D.drive(normalize(x), 3.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.09), 0.01)
    return _loud(F.filt(x, F.lpn(240.0, 2), extend=False), 0.01)


def _smg_open(dry, v, r):
    # The yard and the hills, kept short and low so a long burst rolls rather than piles up; a far Uzi's or MP5's field
    # under the echoes.
    s1 = 0.045 + 0.03 * r.random()
    s2 = s1 + 0.08 + 0.05 * r.random()
    wet = _land_wet(dry, r, 1.1, ((s1, -2.5), (s2, -6.0)), 14, 0.2, 0.8, -7.0, 0.6, 750.0, 0.18, -21.0, 200.0)
    roll = _roll(*(('SMG_Uzi_75m.wav', 1.3, 0.95) if v % 2 == 0 else ('SMG_MP5_75m.wav', 0.6, 1.0)))
    roll = F.filt(roll, F.hpn(200.0, 2), F.lpn(3500.0, 2), extend=False)
    return _with_roll(wet, roll, 0.1, -4.0)


def _smg_town(dry, v, r):
    s1 = 0.02 + 0.014 * r.random()
    gap = 0.019 + 0.011 * r.random()
    slaps = [(s1, -1.5), (s1 + 0.032 + 0.018 * r.random(), -3.5)] + [(s1 + gap * (k + 1), -4.5 - 2.8 * k) for k in range(5)]
    return _land_wet(dry, r, 0.75, slaps, 7, 0.1, 0.42, -10.0, 0.38, 950.0, 0.1, -18.0, 220.0, -6.0, 0.045)


def _smg_inside(dry, v, r):
    if v < 2:
        rec = _rec_room('Rifle_HK416_Room.wav', 0, 0.9, 0.18, 0.5, 170.0, 7000.0, ('left', 'right')[v])
    else:
        rec = _rec_room('Revolver_Colt_Warehouse.wav', v + 1, 0.9, 0.12, 0.5, 200.0, 6500.0)
    room = _synth_room(dry, r, 0.38, 0.5, -13.0, 170.0)
    return layers(room, kit.set_level(rec, room, 1.5, 0.02))


def _smg_distant(v, r):
    if v < 4:
        name, k = [('SMG_MP5_75m.wav', 0), ('SMG_Uzi_75m.wav', 1), ('SMG_Vz61_50m.wav', 0), ('SMG_P90_75m.wav', 0)][v]
        x = S.gate_tail(_take(name, k, 0.9), 0.08, 0.3)
    else:
        name = ('SMG_Uzi_1m.wav', 'SMG_MP5_2m.wav')[v - 4]
        x = _far(S.gate_tail(_take(name, v, 0.9), 0.1, 0.3), 2600.0, 140.0, -9.0, 0.02)
    x = F.filt(x, F.hpn(130.0, 2), F.lpn(5000.0, 2), F.peak(3100.0, -5.0, 0.8), F.lowshelf(220.0, 2.0), extend=False)
    wet = G._land(x, r, 0.9, ((0.07 + 0.04 * r.random(), -4.5),), 10, 0.18, 0.75, -7.0, 0.55, 650.0, 0.18, -10.0, 170.0)
    return _master(layers(_loud(x), wet), 4.0, 0.0012, 0.04)


GS.KINDS['SMG'] = dict(dry=_smg_dry, mech=_smg_mech, low=_smg_low, open=_smg_open, town=_smg_town, inside=_smg_inside,
                       distant=_smg_distant, clip=7.5, look=0.0008, rel=0.025)
GS.MIX['SMG'] = dict(mech=-14.5, low=-6.0)

# Fire 2 dB under the rifle's (a smaller round, fired faster); the layers at their loudness inside Fire's mix.
GS._register('SMG', 6, 6, dict(fire=-2.0, core=-2.6, mech=-15.0, low=-12.0, open=-18.5, town=-17.0, inside=-14.0,
                               distant=-4.5, n_distant=6), 8, trim=1.0)


@cue('Weapon.SMG.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def smg_low_ammo(v, r):
    # The helix nearly spent: its spring slack, the follower ticking round loose in the column with a thin ring.
    return _ping(r, [1400.0, 1480.0, 1340.0][v], 0.04, -8.0)


# --- The helix's reload ------------------------------------------------------------------------------------------------

@cue('Weapon.SMG.HelixOut', variations=3, att='Near', jitter=0.03, conc=2, level=-13.0)
def smg_helix_out(v, r):
    # The thumb presses the helix's latch (a small steel click), the long polymer magazine lifts off its catch with a
    # hollow clack, slides back off the receiver's rails, and the rounds left in it shift in the column.
    latch = G._clank(child(r, 'latch'), (1500.0, 4400.0, 0.018), (760.0, 0.02), (260.0, 0.02), (0.0, -6.0, -12.0),
                     tick_db=-12.0)
    lift = G._clank(child(r, 'lift'), (900.0, 2800.0, 0.03), (520.0 * jitter(r, 1.0, 0.05), 0.05), (190.0, 0.04),
                    (-4.0, 0.0, -5.0))
    rec = _gated(_take('Foley_G36C_Mag.wav', [1, 5, 8, 11][v % 4], 0.12), 0.04, 0.06, hp=180.0)
    slide = G._slide(0.12, child(r, 'slide'), 560.0 * jitter(r, 1.0, 0.08), 820.0, 0.25, 0.5)
    rounds = G._rounds(r, 3, 0.05)
    spring = _whirr(child(r, 'spring'), 0.04, 260.0, -10.0)
    t1 = 0.02 + 0.006 * r.random()
    return layers((latch, 0.0, -8.0), (lift, t1, 0.0), (_loud(rec, 0.005), t1 + 0.002, -4.0), (slide, t1 + 0.02, -17.0),
                  (rounds, t1 + 0.03, -15.0), (spring, t1 + 0.012, -14.0))


@cue('Weapon.SMG.HelixIn', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def smg_helix_in(v, r):
    # A fresh helix laid on the rails (a light polymer tap and a slide), shoved forward till its catch snaps over (a
    # firm clack with the gun's knock under it), then its spring winds the column up against the first round: a quick
    # rising ratchet.
    tap = G._clank(child(r, 'tap'), (1200.0, 3600.0, 0.02), (640.0, 0.025), (230.0, 0.025), (0.0, -3.0, -10.0))
    slide = G._slide(0.08, child(r, 'slide'), 760.0, 520.0 * jitter(r, 1.0, 0.08), 0.3, 0.3)
    seat = G._clank(child(r, 'seat'), (900.0, 3200.0, 0.045), (360.0 * jitter(r, 1.0, 0.05), 0.05),
                    (140.0 * jitter(r, 1.0, 0.05), 0.06), (0.0, 0.0, -3.0), 0.00012, drive_db=5.0)
    rec = _gated(_take('Foley_G36C_Mag.wav', [3, 9, 14, 18][v % 4], 0.12), 0.04, 0.06, hp=160.0)
    n = ns(0.2)
    wind = np.zeros(n)
    rr = child(r, 'wind')
    at = 0.0
    k = 0
    while at < 0.105:
        c = kit.metal_click(child(rr, 'c', k), 5500.0, 10500.0, 0.006, 0.00003, 3, -16.0)
        s0 = ns(at)
        m = min(c.size, n - s0)
        wind[s0:s0 + m] += c[:m] * (0.15 + 0.85 * np.sin(np.pi * min(at / 0.115, 1.0)) ** 1.5)
        at += 0.022 * (1.0 - 0.6 * at / 0.15)
        k += 1
    hum = _whirr(child(r, 'hum'), 0.24, 300.0 * jitter(r, 1.0, 0.05), -60.0)
    hum = hum * np.linspace(0.3, 1.0, hum.size)
    t = 0.07 + 0.01 * r.random()
    return layers((tap, 0.0, -10.0), (slide, 0.004, -19.0), (seat, t, 0.0), (_loud(rec, 0.005), t + 0.001, -5.0),
                  (_loud(wind, 0.005), t + 0.03, -27.0), (_loud(hum, 0.005), t + 0.03, -20.0))
