"""The Thresher LMG's sounds (Phase 12.3, 2026-10-21 night): a belt-fed light machine gun (~650 rounds a minute) whose feed
drum spins up as the trigger is held. Its shot is the same layer set as the other guns' (recipes/guns_shots.py), built
the same way from recordings, plus the spin-up and its reload's three steps.

- Core: an AKM on the ground (the clean front) over a 7.62 L7A2 or a 5.56 L110A2 at 1 m, or a machine gun's first shot
  in a field (the chug), the synthesized crack and a deep heave; lifted under 110 Hz with a chug at 250 Hz and its top
  rolled off: deep and round where the rifle is tight.
- Mech: the bolt carrier slamming, the feed pawl knocking the belt over, a spent link clinking away, and the drum's
  short turn.
- Low: the two LMGs' lows on a long heave: a machine gun's thud in the chest.
- TailOpen: the machine gun's recorded field after a burst (a large field answering it) laid under synthesized echoes;
  inside: a Browning M2's warehouse after its burst, or the shotgun's hall.
- Distant: both LMGs at 50 m, the field gun made far.
- SpinUp: the feed drum winding up over 0.35 s: a rising whir with a gear whine, a ratchet ticking faster, caught by a
  clack as the first round chambers.
- BoxOut / BoxIn / BeltLay: the ammunition box unclipped and pulled off (its links rattling), a fresh one clipped on,
  the belt laid on the tray and the cover slammed.

Recordings: Art/Sounds/packs/Sonniss/LMG_*.wav (sources in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, dist as D, dynamics as Y, modal as M, osc as O
from lib import granular as GR
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_shots as GS
from recipes.guns_rec import (SHOTS, _loud, _take, _roll, _gated, _land_wet, _with_roll, _master, _rec_room,
                               _synth_room, _far, _tick, _ping)

SHOTS.update({
    'LMG_L7A2_1m.wav': [0.112, 0.223],
    'LMG_L110_1m.wav': [0.054, 2.030],
    'LMG_AKM_Ground.wav': [0.090, 0.319, 0.520],
    # A machine gun in a large field, five stretches: bursts' first shots (clean fronts) at indexes 0, 14 and 19; the
    # bursts' last shots, after which the field rolls, at 13 (to 6.1 s) and 31 (to the end).
    'LMG_MG_Field.wav': [0.030, 0.123, 0.213, 0.305, 0.395, 0.485, 0.577, 1.490, 1.577, 1.669, 1.758, 1.847, 2.020, 2.107,
                         6.514, 6.604, 6.692, 6.786, 6.882, 9.759, 9.864, 9.958, 10.051, 10.149, 10.249, 10.389, 11.154,
                         11.250, 11.346, 11.451, 11.707, 12.207],
    'LMG_L110_50m.wav': [0.059, 4.159],
    'LMG_L7A2_50m.wav': [0.055, 4.226],
})

_FIELD_FIRST = [0, 14, 19]
_BODIES = [('LMG_L7A2_1m.wav', 0), ('LMG_L110_1m.wav', 0), ('LMG_MG_Field.wav', 0), ('LMG_L110_1m.wav', 1),
           ('LMG_MG_Field.wav', 14), ('LMG_MG_Field.wav', 19)]


def _lmg_dry(v, r):
    """The core before its master: the AKM's clean front over a belt-fed gun's chug, a crack and a deep heave."""
    front = _gated(_take('LMG_AKM_Ground.wav', v, 0.2), 0.05, 0.16, hp=45.0)
    name, k = _BODIES[v % len(_BODIES)]
    body = _gated(_take(name, k, 0.25), 0.06, 0.2, lp=6000.0, hp=40.0)
    crack = G._crack(child(r, 'crack'), 0.0003 * jitter(r, 1.0, 0.12), 0.006, 1300.0 * jitter(r, 1.0, 0.08), -3.0, -3.0)
    heave = G._heave(child(r, 'heave'), 0.0085 * jitter(r, 1.0, 0.15), 120.0, 0.11)
    x = layers((_loud(front), 0.0, 0.0), (_loud(body), 0.0, -0.5), (crack, 0.0, -8.0), (heave, 0.0, -5.0))
    x = Y.transient(x, 6.0, -3.0)
    x = F.filt(x, F.hp(45.0), F.lowshelf(110.0, 3.0), F.peak(250.0, 2.0, 1.0), F.peak(600.0, -2.5, 0.9),
               F.peak(1500.0, 1.5, 1.0), F.peak(3200.0, -2.5, 0.9), F.highshelf(7000.0, -2.5), extend=False)
    x = D.drive(normalize(x), 4.5, 'tanh')
    return _loud(G._early(x, r, -13.0, 0.03, 4500.0), 0.002)


def _drum_turn(r, dur=0.045, rot=None):
    """The feed drum turning a link on: a low gear whine with a pawl's two ticks (kept under 1.8 kHz and over 5)."""
    rot = rot or 95.0 * jitter(r, 1.0, 0.06)
    n = ns(dur)
    t = times(n)
    whine = sum(np.sin(2.0 * np.pi * rot * h * t + r.uniform(0, 6.28)) / h for h in (6, 9, 12))
    whine = F.filt(whine, F.bp(900.0, 1.0), extend=False) * E.end_fade(E.perc(dur, 0.004, dur * 0.7))
    ticks = layers(kit.metal_click(child(r, 'p1'), 5200.0, 9500.0, 0.008, 0.00004, 3, -14.0),
                   (kit.metal_click(child(r, 'p2'), 5200.0, 9500.0, 0.008, 0.00004, 3, -14.0), 0.02, -3.0))
    return layers((_loud(whine, 0.005), 0.0, 0.0), (_loud(ticks, 0.003), 0.003, -4.0))


def _lmg_mech(v, r):
    """The carrier slamming home, the feed pawl knocking the belt over, a spent link clinking out, the drum's turn."""
    tick = _tick(r, 1800.0, 5600.0, -10.0)
    slam = G._clank(child(r, 'slam'), (800.0, 3000.0, 0.04), (380.0 * jitter(r, 1.0, 0.05), 0.04),
                    (140.0 * jitter(r, 1.0, 0.05), 0.05), (0.0, -1.0, -3.0), 0.0001, drive_db=5.0)
    pawl = G._clank(child(r, 'pawl'), (1300.0, 4000.0, 0.02), (700.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0))
    link = M.strike(M.parts(5400.0, 8200.0, child(r, 'link'), 4, 0.05, 0.03, 0.2), M.hammer(0.00004), 0.1)
    turn = _drum_turn(child(r, 'turn'))
    t_slam = 0.018 + 0.006 * r.random()
    x = layers((_loud(tick, 0.005), 0.0, -12.0), (slam, t_slam, 0.0), (pawl, t_slam + 0.012, -8.0),
               (_loud(link, 0.005), t_slam + 0.02 + 0.01 * r.random(), -18.0), (turn, t_slam + 0.006, -12.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _lmg_low(v, r):
    rec = _take(('LMG_L7A2_1m.wav', 'LMG_L110_1m.wav')[v % 2], v // 2, 0.3)
    rec = F.filt(rec, F.hpn(30.0, 2), F.lpn(160.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.009 * jitter(r, 1.0, 0.15), 115.0, 0.14)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -2.0))
    x = D.drive(normalize(x), 4.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.16), 0.01)
    return _loud(F.filt(x, F.lpn(210.0, 2), extend=False), 0.01)


def _lmg_open(dry, v, r):
    # Low and rolling; the machine gun's own recorded field after a burst (a large field) under the echoes.
    s1 = 0.055 + 0.03 * r.random()
    s2 = s1 + 0.1 + 0.06 * r.random()
    wet = _land_wet(dry, r, 1.6, ((s1, -2.0), (s2, -5.0)), 20, 0.24, 1.2, -4.0, 1.0, 550.0, 0.22, -18.0, 150.0)
    at, length = [(2.25, 1.6), (12.6, 1.8), (2.45, 1.5), (13.0, 1.7)][v % 4]
    roll = S.body_only(S.load('LMG_MG_Field.wav'), at, length, 0.05, 0.1)
    roll = F.filt(S.gate_tail(roll, 0.5, 1.0), F.hpn(140.0, 2), F.lpn(3000.0, 2), extend=False)
    return _with_roll(wet, roll, 0.11, -2.5)


def _lmg_town(dry, v, r):
    s1 = 0.024 + 0.015 * r.random()
    gap = 0.021 + 0.012 * r.random()
    slaps = [(s1, -1.0), (s1 + 0.036 + 0.02 * r.random(), -3.0)] + [(s1 + gap * (k + 1), -4.0 - 2.4 * k) for k in range(6)]
    return _land_wet(dry, r, 1.1, slaps, 9, 0.14, 0.6, -7.0, 0.55, 750.0, 0.13, -16.0, 160.0, -6.0, 0.055)


def _lmg_inside(dry, v, r):
    # A machine gun in a warehouse, as taped (a Browning M2's hall after its burst), or the shotgun's hall; our shot in a
    # synthesized room first.
    if v < 2:
        at = (0.84, 4.58)[v]
        rec = S.body_only(S.load('LMG_M2_Warehouse.wav'), at, 2.2, 0.006, 0.1)
        rec = np.concatenate([np.zeros(ns(0.003)), rec])
        rec = F.filt(S.gate_tail(rec, 0.3, 1.2), F.hpn(120.0, 2), F.lpn(5000.0, 2), extend=False)
    else:
        rec = _rec_room('Shotgun_870_Warehouse.wav', v, 2.0, 0.3, 1.2, 110.0, 5000.0)
    room = _synth_room(dry, r, 0.55, 0.7, -11.0, 140.0)
    return layers(room, kit.set_level(rec, room, 2.0, 0.02))


def _lmg_distant(v, r):
    if v < 4:
        name, k = [('LMG_L110_50m.wav', 0), ('LMG_L7A2_50m.wav', 0), ('LMG_L110_50m.wav', 1), ('LMG_L7A2_50m.wav', 1)][v]
        x = S.gate_tail(_take(name, k, 1.8), 0.25, 0.9)
    else:
        x = _far(S.gate_tail(_take('LMG_MG_Field.wav', _FIELD_FIRST[v - 3], 0.6), 0.15, 0.5), 2000.0, 100.0, -9.0, 0.025)
    x = F.filt(x, F.hpn(90.0, 2), F.lpn(4800.0, 2), F.lowshelf(170.0, 3.0), F.peak(3100.0, -4.0, 0.9), extend=False)
    wet = G._land(x, r, 1.4, ((0.09 + 0.05 * r.random(), -3.5),), 14, 0.22, 1.1, -5.0, 0.9, 500.0, 0.22, -11.0, 120.0)
    return _master(layers(_loud(x), wet), 4.5, 0.0015, 0.05)


GS.KINDS['LMG'] = dict(dry=_lmg_dry, mech=_lmg_mech, low=_lmg_low, open=_lmg_open, town=_lmg_town, inside=_lmg_inside,
                       distant=_lmg_distant, clip=6.5, look=0.0012, rel=0.035)
GS.MIX['LMG'] = dict(mech=-14.0, low=-5.0)

# Fire about a dB under the rifle's by measured loudness (its weight is in the lows, which loudness counts less, so
# it plays as big as the rifle or bigger); its tails a little under the rifle's (a long burst).
GS._register('LMG', 6, 6, dict(fire=-0.9, core=-1.4, mech=-14.5, low=-8.5, open=-15.0, town=-13.5, inside=-10.5,
                               distant=-3.5, n_distant=6), 8)


@cue('Weapon.LMG.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def lmg_low_ammo(v, r):
    # The belt's tail end running light in the tray: a loose link slapping with a low steel ring.
    return _ping(r, [900.0, 950.0, 860.0][v], 0.06, -12.0)


@cue('Weapon.LMG.SpinUp', variations=3, att='Near', jitter=0.02, conc=2, level=-11.0)
def lmg_spin_up(v, r):
    # The feed drum winding up as the trigger is held (0.35 s, the fire rate's ramp): a whir rising from a slow turn to
    # the firing rate with a gear whine over it, the pawl's ratchet ticking faster, then the catch's clack as the first
    # round chambers and the drum's hum settles into the burst.
    dur = 0.46
    n = ns(dur)
    t = times(n)
    u = np.clip(t / 0.35, 0.0, 1.0)
    rot = (40.0 + 60.0 * u ** 1.3) * jitter(r, 1.0, 0.04)
    ph = 2.0 * np.pi * np.cumsum(rot) / 48000.0
    whir = sum(np.sin(h * ph + r.uniform(0, 6.28)) / h ** 0.7 for h in range(2, 9))
    whir = F.filt(whir, F.hp(120.0), F.lp(1500.0), extend=False)
    whine = np.sin(12.0 * ph) + 0.5 * np.sin(18.0 * ph + 1.0)
    whine = F.filt(whine, F.bp(1000.0, 0.8), extend=False)
    rumble = F.filt(N.pink(n, child(r, 'rumble')), F.bp(300.0, 0.9), extend=False) * kit.turbulence(n, child(r, 'rt'),
                                                                                                    120.0, 0.5)
    body = layers((normalize(whir), 0.0, 0.0), (normalize(whine), 0.0, -12.0), (normalize(rumble), 0.0, -14.0))
    body = body * E.end_fade(np.minimum(1.0, 0.25 + 0.75 * u ** 0.8) * np.where(t < 0.355, 1.0, np.exp(-(t - 0.355) / 0.006)))
    # The ratchet: one tick per tooth, faster as the drum winds.
    ticks = np.zeros(n)
    teeth = np.floor(ph / (2.0 * np.pi / 3.0))
    hits = np.nonzero(np.diff(teeth) > 0)[0]
    rr = child(r, 'ratchet')
    for i, h in enumerate(hits):
        if t[h] > 0.36:
            break
        c = kit.metal_click(child(rr, 'c', i), 5200.0, 9800.0, 0.006, 0.00004, 3, -14.0)
        m = min(c.size, n - h)
        ticks[h:h + m] += c[:m] * (0.5 + 0.5 * u[h])
    # Leads with the trigger's own tick, so it starts at once.
    first = kit.metal_click(child(r, 'sear'), 2000.0, 6000.0, 0.01, 0.00005, 5, -8.0)
    catch = G._clank(child(r, 'catch'), (900.0, 3200.0, 0.03), (420.0, 0.03), (160.0, 0.035), (0.0, -3.0, -6.0))
    x = layers((_loud(first, 0.005), 0.0, -6.0), (_loud(body), 0.0, -2.0), (_loud(ticks, 0.003), 0.0, -12.0),
               (catch, 0.345, 0.0))
    return F.filt(x, F.peak(3200.0, -5.0, 0.8), extend=False)


# --- The box and the belt ----------------------------------------------------------------------------------------------

def _links(r, count, spread, start=0.0):
    """A belt's links shifting: a quick patter of small steel clinks and the rounds' dull brass knocks."""
    def grain(rr, i):
        m = M.parts(2600.0, 6200.0, rr, 4, rr.uniform(0.01, 0.025), None, 0.3)
        return M.strike(m, M.hammer(0.00006), 0.04) * rr.uniform(0.4, 1.0)
    clinks = normalize(GR.rattle(0.1, child(r, 'links'), count, spread, grain, start, 0.8), 0.0)
    return layers((clinks, 0.0, 0.0), (G._rounds(child(r, 'rounds'), max(2, count // 2), spread), start, -4.0))


@cue('Weapon.LMG.BoxOut', variations=3, att='Near', jitter=0.03, conc=2, level=-12.0)
def lmg_box_out(v, r):
    # The box's catch thumbed (a steel click), the box unhooked from under the gun with a hollow steel clack, pulled away
    # with the belt's tail rattling out of the tray.
    catch = G._clank(child(r, 'catch'), (1400.0, 4200.0, 0.02), (700.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0))
    f0 = 420.0 * jitter(r, 1.0, 0.05)
    box = M.strike(M.shell(f0, child(r, 'box'), 0.08, 6, 0.03), M.hammer(0.00015), 0.2)
    unhook = G._clank(child(r, 'unhook'), (800.0, 2800.0, 0.04), (300.0, 0.05), (130.0, 0.05), (0.0, -2.0, -4.0))
    t = 0.03 + 0.008 * r.random()
    return layers((catch, 0.0, -7.0), (unhook, t, 0.0), (_loud(box, 0.005), t + 0.001, -8.0),
                  (_links(child(r, 'belt'), 7, 0.12), t + 0.02, -9.0),
                  (G._slide(0.12, child(r, 'slide'), 480.0, 700.0, 0.2, 0.6), t + 0.03, -19.0))


@cue('Weapon.LMG.BoxIn', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def lmg_box_in(v, r):
    # A full box swung up under the gun (its rounds shifting), hooked on with a heavy steel clack and the box's hollow
    # ring, the catch snapping over.
    f0 = 400.0 * jitter(r, 1.0, 0.05)
    box = M.strike(M.shell(f0, child(r, 'box'), 0.1, 6, 0.03), M.hammer(0.00015), 0.25)
    hook = G._clank(child(r, 'hook'), (760.0, 2800.0, 0.05), (320.0 * jitter(r, 1.0, 0.05), 0.06),
                    (125.0 * jitter(r, 1.0, 0.05), 0.07), (0.0, 0.0, -2.0), 0.00012, drive_db=5.0)
    catch = G._clank(child(r, 'catch'), (1500.0, 4200.0, 0.025), (800.0, 0.02), (260.0, 0.02), (0.0, -6.0, -12.0))
    t = 0.06 + 0.01 * r.random()
    return layers((_links(child(r, 'belt'), 5, 0.05), 0.0, -12.0), (hook, t, 0.0), (_loud(box, 0.005), t + 0.001, -6.0),
                  (catch, t + 0.03 + 0.006 * r.random(), -5.0))


@cue('Weapon.LMG.BeltLay', variations=3, att='Near', jitter=0.03, conc=2, level=-10.0)
def lmg_belt_lay(v, r):
    # The belt's first rounds laid in the feed tray (links clattering across the steel), then the feed cover slammed
    # down and latched: the heaviest clack of the reload.
    lay = _links(child(r, 'lay'), 9, 0.14)
    tray = G._clank(child(r, 'tray'), (1100.0, 3600.0, 0.03), (560.0, 0.03), (200.0, 0.03), (0.0, -4.0, -9.0))
    t_slam = 0.24 + 0.03 * r.random()
    cover = G._clank(child(r, 'cover'), (680.0 * jitter(r, 1.0, 0.05), 2700.0, 0.08), (280.0, 0.08),
                     (110.0 * jitter(r, 1.0, 0.05), 0.09), (0.0, 0.0, -1.0), 0.00012, drive_db=6.0)
    latch = G._clank(child(r, 'latch'), (1400.0, 4200.0, 0.02), (720.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0))
    hinge = kit.creak(0.1, child(r, 'hinge'), 260.0, 140.0, (500.0, 1100.0, 1900.0), 8.0, 0.25)
    return layers((tray, 0.0, -6.0), (lay, 0.01, -6.0), (_loud(hinge), t_slam - 0.11, -20.0), (cover, t_slam, 0.0),
                  (latch, t_slam + 0.012, -6.0))
