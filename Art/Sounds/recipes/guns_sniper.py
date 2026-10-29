"""The Longhorn sniper's sounds (Phase 12.3, 2026-10-21 night): a bolt-action long rifle whose job is the crit. Its shot is
the same layer set as the other guns' (recipes/guns_shots.py), built the same way from recordings; its bolt has a cue for
the cycle between shots and one per reload step.

- Core: a Barrett .338 recorded on the gun (the huge, clean front) over the same gun at 2 m, a Remington 700 or a Sako
  TRG at 1 m (the bark), a designed sniper shot's weight under them, a big synthesized supersonic crack and a long heave;
  lifted under 90 Hz, its bark at 1.2 kHz.
- Mech: the striker falling and the barrel and scope ringing from the blow (the bolt itself is BoltCycle).
- Low: the Barrett's and the designed shot's lows on a long heave: the shove in the shoulder.
- TailOpen: the valley answering: two slaps, then thirty-odd echoes out to 2.6 s over a long wandering roll, with the
  Barrett's own recorded field (the 2 m take's seconds after the shot) laid in. TailTown and TailInside as the others'.
- Distant: the Remington at 50 m, an FN FAL at 25 m, the Barrett made far.
- BoltCycle: lift (the cocking cam's clack), draw back (steel on steel, the case flicked out ringing), shove forward,
  lock down (the heaviest clack); BoltOpen and BoltClose are its two halves for the reload. A recorded rifle's cocking
  is laid on the synthesized clanks, and (2026-10-29, the 2026 bundle) a lock's tumbler click on the lift, a carabiner
  gate's snap on the stop and a deep latch's thunk under the lock-down.

Recordings: Art/Sounds/packs/Sonniss/Sniper_*.wav, Rifle_Cock.wav, Foley_Lock_Tinker.wav, Foley_Carabiner_Snap.wav,
Foley_Latch_Thunk.wav (sources in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, varispeed
from lib import noise as N, env as E, filters as F, dist as D, dynamics as Y, modal as M
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_shots as GS
from recipes.guns_rec import (SHOTS, _loud, _take, _roll, _gated, _land_wet, _with_roll, _master, _rec_room,
                               _synth_room, _far, _tick, _ping, _cap, _click)

SHOTS.update({
    'Sniper_Barrett_Gun.wav': [0.050, 1.350, 2.650, 3.950, 5.250, 6.550],
    'Sniper_Barrett_2m.wav': [0.050, 3.750, 7.450, 11.150, 14.850, 18.550],
    'Sniper_R700_1m.wav': [0.050, 2.950, 5.850, 8.750],
    'Sniper_R700_50m.wav': [0.060, 3.770, 7.480, 11.190],
    'Sniper_Sako_1m.wav': [0.050, 2.850, 5.650],
    'Sniper_GM_Shot.wav': [0.090],
    'Sniper_FAL_25m.wav': [0.095, 0.376],
})


def _sniper_dry(v, r):
    """The core before its master: the Barrett on the gun over a near bark (the Barrett at 2 m, a Remington or a Sako),
    a designed shot's weight, a big crack and a long heave."""
    front = _gated(_take('Sniper_Barrett_Gun.wav', v, 0.8), 0.08, 0.35, hp=35.0)
    name = ('Sniper_Barrett_2m.wav', 'Sniper_R700_1m.wav', 'Sniper_Sako_1m.wav')[v % 3]
    bark = _gated(_take(name, v, 0.9), 0.08, 0.35, lp=7000.0, hp=90.0)
    weight = _gated(_take('Sniper_GM_Shot.wav', 0, 0.9), 0.1, 0.4, lp=380.0, hp=35.0)
    crack = G._crack(child(r, 'crack'), 0.00045 * jitter(r, 1.0, 0.1), 0.009, 1300.0 * jitter(r, 1.0, 0.08), -2.0, -3.0,
                     1500.0, 10000.0)
    heave = G._heave(child(r, 'heave'), 0.011 * jitter(r, 1.0, 0.12), 110.0, 0.16)
    x = layers((_loud(front), 0.0, 0.0), (_loud(bark), 0.0, -1.0), (_loud(weight), 0.0, -9.0), (crack, 0.0, -5.0),
               (heave, 0.0, -6.0))
    x = Y.transient(x, 7.0, -3.0)
    x = F.filt(x, F.hp(40.0), F.lowshelf(90.0, 2.5), F.peak(300.0, -3.0, 0.9), F.peak(1200.0, 2.5, 1.0),
               F.peak(3300.0, -2.0, 0.9), F.highshelf(8500.0, -2.0), extend=False)
    x = D.drive(normalize(x), 4.0, 'tanh')
    return _loud(G._early(x, r, -12.0, 0.04, 4500.0), 0.002)


def _sniper_mech(v, r):
    """The striker's fall (a crisp sear and a dull knock) and the barrel and scope ringing from the blow."""
    tick = _tick(r, 1800.0, 6000.0, -6.0)
    striker = G._clank(child(r, 'striker'), (1100.0, 3800.0, 0.02), (540.0, 0.025), (180.0, 0.025), (0.0, -5.0, -9.0),
                       0.00006)
    f0 = 820.0 * jitter(r, 1.0, 0.05)
    barrel = M.strike(M.parts(f0, f0 * 2.4, child(r, 'barrel'), 7, 0.3, 0.12, 0.4), M.hammer(0.0002), 0.5)
    scope = M.strike(M.parts(2200.0, 5200.0, child(r, 'scope'), 4, 0.08, 0.04, 0.3), M.hammer(0.0001), 0.15)
    stock = kit.thunk(240.0 * jitter(r, 1.0, 0.06), child(r, 'stock'), 0.05, 0.0012)
    x = layers((_loud(tick, 0.005), 0.0, -8.0), (striker, 0.001, 0.0), (_loud(barrel, 0.005), 0.003, -12.0),
               (_loud(scope, 0.005), 0.004, -16.0), (_loud(stock, 0.005), 0.002, -6.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _sniper_low(v, r):
    rec = _take(('Sniper_Barrett_2m.wav', 'Sniper_GM_Shot.wav')[v % 2], v, 0.4)
    rec = F.filt(rec, F.hpn(26.0, 2), F.lpn(140.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.013 * jitter(r, 1.0, 0.12), 100.0, 0.2)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -2.0))
    x = D.drive(normalize(x), 4.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.3), 0.01)
    return _loud(F.filt(x, F.lpn(190.0, 2), extend=False), 0.01)


def _sniper_open(dry, v, r):
    # The valley, 2.5 s (gun identity, 2026-10-28: "one thunderclap down the valley"): the yard's slap close by, then the
    # slapback off the valley's far wall 0.35-0.5 s on (120 m: the shot coming back, nearly as loud as the slaps, its top
    # dulled by the air), a weaker second answer off the hills behind it, then the far side rolling out over a wandering
    # rumble; the Barrett's own recorded field under it. The near-field scatter that starts it is kept low: the Core is
    # the front, the valley its answer.
    s1 = 0.06 + 0.03 * r.random()
    back = 0.35 + 0.15 * r.random()
    slaps = ((s1, -4.0), (back, 0.0), (back + 0.22 + 0.12 * r.random(), -5.0))
    wet = _land_wet(dry, r, 2.5, slaps, 26, 0.3, 1.9, -2.5, 1.5, 450.0, 0.5, -15.0, 100.0, -10.0, 0.08)
    roll = _take('Sniper_Barrett_2m.wav', v + 1, 2.4, skip=0.2)
    roll = F.filt(S.gate_tail(roll, 1.0, 1.2), F.hpn(110.0, 2), F.lpn(3000.0, 2), extend=False)
    k = ns(0.06)
    roll[:k] *= np.linspace(0.0, 1.0, k)
    return _cap(_with_roll(wet, roll, 0.2, -1.5), 2.5, 0.4)


def _sniper_town(dry, v, r):
    s1 = 0.026 + 0.015 * r.random()
    gap = 0.023 + 0.012 * r.random()
    slaps = [(s1, -0.5), (s1 + 0.04 + 0.02 * r.random(), -2.5)] + [(s1 + gap * (k + 1), -3.5 - 2.0 * k) for k in range(8)]
    return _land_wet(dry, r, 1.7, slaps, 14, 0.18, 1.2, -5.0, 1.0, 650.0, 0.2, -13.0, 120.0, -5.0, 0.06)


def _sniper_inside(dry, v, r):
    # A big gun in a hall: the warehouse's long boom (the shotgun's or the pistol's take); our shot in a room first.
    if v % 2 == 0:
        rec = _rec_room('Shotgun_870_Warehouse.wav', v // 2, 2.6, 0.45, 1.7, 90.0, 5000.0)
    else:
        rec = _rec_room('Revolver_Colt_Warehouse.wav', v, 2.4, 0.4, 1.5, 110.0, 5500.0)
    room = _synth_room(dry, r, 0.75, 0.85, -10.0, 110.0)
    return layers(room, kit.set_level(rec, room, 3.0, 0.02))


def _sniper_distant(v, r):
    if v < 4:
        x = S.gate_tail(_take('Sniper_R700_50m.wav', v, 3.0), 0.4, 1.6)
    elif v == 4:
        x = S.gate_tail(_take('Sniper_FAL_25m.wav', 0, 2.0), 0.25, 1.1)
    else:
        x = _far(S.gate_tail(_take('Sniper_Barrett_2m.wav', 3, 3.0), 0.5, 1.6), 2000.0, 90.0, -9.0, 0.03)
    x = F.filt(x, F.hpn(80.0, 2), F.lpn(5000.0, 2), F.lowshelf(160.0, 3.0), F.peak(3100.0, -4.0, 0.9), extend=False)
    wet = G._land(x, r, 2.4, ((0.12 + 0.06 * r.random(), -3.0),), 18, 0.3, 2.0, -3.0, 1.6, 450.0, 0.3, -11.0, 100.0)
    return _master(layers(_loud(x), wet), 5.0, 0.0015, 0.06)


GS.KINDS['Sniper'] = dict(dry=_sniper_dry, mech=_sniper_mech, low=_sniper_low, open=_sniper_open, town=_sniper_town,
                          inside=_sniper_inside, distant=_sniper_distant, clip=7.0, look=0.0015, rel=0.06)
GS.MIX['Sniper'] = dict(mech=-16.0, low=-5.0)

# Fire 1.5 dB over the rifle's: the loudest gun, fired a shot at a time; a long valley tail.
GS._register('Sniper', 6, 6, dict(fire=1.5, core=3.2, mech=-19.5, low=-11.5, open=-4.0, town=-9.0, inside=-12.5,
                                  distant=-0.5, n_distant=6), 4, trim=1.2)


@cue('Weapon.Sniper.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def sniper_low_ammo(v, r):
    # The box magazine's last round: the follower's spring knocking up under it, a deeper steel tick.
    return _ping(r, [1050.0, 1110.0, 990.0][v], 0.05, -10.0)


# --- The bolt ------------------------------------------------------------------------------------------------------------

def _lift(r):
    """The handle lifted: the cocking cam rides up and the striker's spring takes the load: a crisp clack."""
    return G._clank(child(r, 'lift'), (1100.0, 3600.0, 0.03), (520.0 * jitter(r, 1.0, 0.05), 0.03), (180.0, 0.03),
                    (0.0, -4.0, -8.0))


def _back(r, dur):
    return G._slide(dur, child(r, 'back'), 640.0 * jitter(r, 1.0, 0.06), 1050.0, 0.15, 0.35)


def _case(r):
    """The empty case flicked out by the ejector: the bolt's stop clack, then the brass ringing off the receiver."""
    stop = G._clank(child(r, 'stop'), (1000.0, 3400.0, 0.035), (460.0, 0.035), (165.0, 0.04), (0.0, -3.0, -7.0))
    brass = M.strike(M.parts(5200.0, 7400.0, child(r, 'brass'), 5, 0.09, 0.05, 0.2), M.hammer(0.00005), 0.2)
    body = kit.brass_tick(child(r, 'head'), 1600.0, 0.03, 3, 0.0002, -12.0, -16.0, 0.15)
    return layers((stop, 0.0, 0.0), (_loud(brass, 0.005), 0.012, -16.0), (_loud(body), 0.012, -14.0))


def _forward(r, dur):
    return G._slide(dur, child(r, 'fwd'), 1050.0, 680.0 * jitter(r, 1.0, 0.06), 0.3, 0.2)


def _lock(r):
    """The handle shoved down: the lugs turn into their seats, the heaviest clack of the cycle, the stock's thud."""
    lock = G._clank(child(r, 'lock'), (720.0 * jitter(r, 1.0, 0.05), 2800.0, 0.06), (330.0, 0.06),
                    (120.0 * jitter(r, 1.0, 0.05), 0.08), (0.0, -1.0, -2.0), 0.0001, drive_db=6.0)
    thud = kit.thunk(210.0 * jitter(r, 1.0, 0.06), child(r, 'thud'), 0.06, 0.0015)
    return layers((lock, 0.0, 0.0), (_loud(thud, 0.005), 0.002, -8.0))


def _rec_cock(r, k, db_hp=250.0):
    return _loud(_gated(_take('Rifle_Cock.wav', k, 0.08), 0.03, 0.05, hp=db_hp), 0.005)


# Recorded handling from the 2026 bundle (2026-10-29), each under the synthesized clack it joins: an antique lock's
# tumbler click as the handle lifts (the cocking cam), a carabiner's gate snapping as the bolt hits its stop, a deep
# latch's thunk as the handle locks down (pitched a little lower: the lugs turning into their seats). Their streams are
# drawn after the synthesized layers', so those stay as they were.

def _steel(x, hp, shelf_db=-5.0):
    """A recorded click made to sit with the guns: its rumble cut, the ear's touchy band dipped, the fizz eased."""
    return F.filt(x, F.hpn(hp, 2), F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, shelf_db), extend=False)


def _rec_click(name, r, hp, before=0.001, after=0.05):
    k = int(r.integers(0, 6))
    x, at = _click(name, k, before, after, jitter(r, 1.0, 0.04))
    return _steel(x, hp), at


def _rec_thunk(r):
    """The deep latch's thunk, slowed a little (deeper), its body kept: (samples, the hit's time in them)."""
    x = S.load('Foley_Latch_Thunk.wav')
    t = int(np.argmax(np.abs(x[:ns(0.05)]))) / 48000.0
    a = max(0.0, t - 0.003)
    y = x[ns(a):ns(t + 0.15)].copy()
    y = y * E.end_fade(np.ones(y.size), 0.05)
    speed = 0.9 * jitter(r, 1.0, 0.04)
    y = F.filt(varispeed(y, speed), F.hpn(40.0, 2), F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -6.0), extend=False)
    return y, (t - a) / speed


def _bolt_open(v, r):
    t_back = 0.07 + 0.012 * r.random()
    t_stop = t_back + 0.11 + 0.02 * r.random()
    lift = _lift(r)
    back = _back(r, t_stop - t_back + 0.01)
    case = _case(r)
    tumbler, ta = _rec_click('Foley_Lock_Tinker.wav', child(r, 'tumbler'), 250.0)
    gate, ga = _rec_click('Foley_Carabiner_Snap.wav', child(r, 'gate'), 1500.0, 0.001, 0.04)
    return layers((lift, 0.0, -2.0), (_rec_cock(r, 0), 0.0, -6.0), (back, t_back, -17.0),
                  (case, t_stop, -3.0), (_rec_cock(r, 1), t_stop, -8.0),
                  (kit.set_level(tumbler, lift, -9.0), 0.0),
                  (kit.set_level(gate, case, -12.0), max(0.0, t_stop + 0.002 - ga)))


def _bolt_close(v, r):
    t_lock = 0.1 + 0.015 * r.random()
    # The palm meeting the handle starts it at once (a small steel tick), then the bolt rides forward.
    touch = kit.metal_click(child(r, 'touch'), 1600.0, 4800.0, 0.015, 0.0001, 4, -10.0)
    forward = _forward(r, t_lock - 0.01)
    home = G._clank(child(r, 'home'), (900.0, 3200.0, 0.03), (420.0, 0.03), (150.0, 0.03), (0.0, -4.0, -8.0))
    lock = _lock(r)
    thunk, ka = _rec_thunk(child(r, 'thunk'))
    tumbler, ta = _rec_click('Foley_Lock_Tinker.wav', child(r, 'tumbler'), 250.0)
    return layers((_loud(touch, 0.005), 0.0, -12.0), (forward, 0.002, -18.0), (home, t_lock - 0.03, -6.0),
                  (lock, t_lock, 0.0), (_rec_cock(r, 2), t_lock, -5.0),
                  (kit.set_level(tumbler, home, -10.0), max(0.0, t_lock - 0.03 - ta)),
                  (kit.set_level(thunk, lock, -4.0), max(0.0, t_lock - ka)))


@cue('Weapon.Sniper.BoltOpen', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def sniper_bolt_open(v, r):
    # The handle lifted (a crisp clack), the bolt drawn back on its rails, it hits the stop and the case rings out.
    return _bolt_open(v, r)


@cue('Weapon.Sniper.BoltClose', variations=3, att='Near', jitter=0.03, conc=2, level=-10.0)
def sniper_bolt_close(v, r):
    # The bolt shoved forward (the round stripped off the magazine), home with a clack, the handle locked down: CHUNK.
    return _bolt_close(v, r)


@cue('Weapon.Sniper.BoltCycle', variations=4, att='Near', jitter=0.03, conc=2, level=-10.0)
def sniper_bolt_cycle(v, r):
    # Between shots: lift, back, the case out, forward, lock, in about 0.6 s: the satisfying half of the gun.
    opening = _bolt_open(v, child(r, 'open'))
    gap = 0.08 + 0.03 * r.random()
    closing = _bolt_close(v, child(r, 'close'))
    return layers((opening, 0.0, 0.0), (closing, opening.size / 48000.0 - 0.06 + gap, 0.5))
