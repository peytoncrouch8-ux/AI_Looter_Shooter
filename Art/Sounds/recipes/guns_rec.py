"""The recorded shots' building blocks for recipes/guns_shots.py: each pack file's shots, slicing one out, a far take's
field roll, gating, the land's answer without the direct sound, recorded and synthesized rooms, a near take made far, the
Mech layer's opening tick and the low-ammo click. See guns_shots.py for how a shot's layers use them.
"""
import numpy as np

from lib.core import ns, times, layers, child, db2a, normalize, jitter, varispeed  # noqa: F401
from lib import noise as N, filters as F, dynamics as Y, modal as M, reverb as R, env as E
from lib import sample as S
from recipes import kit
from recipes import guns as G

# Each pack file's shots (s): lib/sample.onsets' finds (Art/Sounds/packs/make_pack.py prints them), checked against the
# spectrograms; echoes and the mechanism's clacks it also found are left out.
SHOTS = {
    'Rifle_FAMAS_1m.wav': [0.028, 0.249, 0.447, 0.643],
    'Rifle_L85_1m.wav': [0.018, 0.262, 0.514, 0.738, 0.949],
    'Rifle_HK416_1m.wav': [0.028, 0.268, 0.483, 0.717, 0.944, 1.155],
    'Rifle_Tavor_1m.wav': [0.05, 0.312, 0.487, 0.613],
    'Rifle_G36C_50m.wav': [0.065, 0.287, 0.607, 0.885, 1.196, 1.546, 1.799],
    'Rifle_FAMAS_55m.wav': [0.020, 0.220, 0.412, 0.593, 0.793, 1.003],
    'Rifle_Tavor_20m.wav': [0.031, 0.284, 0.458],
    'Rifle_HK416_Room.wav': [0.068],
    'Rifle_Cock.wav': [0.044, 0.310, 0.410],
    'Shotgun_Saiga_1m.wav': [0.059, 2.764, 5.460, 8.161, 10.868],
    'Shotgun_SPAS12_1m.wav': [0.079, 0.488],
    'Shotgun_USAS12_1m.wav': [0.102, 0.394],
    'Shotgun_TS_Slug.wav': [0.055],
    'Shotgun_TS_Fire.wav': [0.0],
    'Shotgun_SPAS12_50m.wav': [0.121],
    'Shotgun_USAS12_55m.wav': [0.095, 0.388],
    'Shotgun_870_Warehouse.wav': [0.051, 4.740, 9.373, 13.992],
    'Shotgun_Saiga_Cocking.wav': [0.069, 0.397, 1.117, 1.227],
    'Revolver_M29_1m5.wav': [0.038, 1.290, 3.138, 4.372, 6.235, 8.093],
    'Revolver_M29_2m_PZM.wav': [0.057, 1.309, 3.157, 4.391, 6.255, 8.113],
    'Revolver_M66_50m.wav': [0.035, 3.338],
    'Revolver_GM_Shot.wav': [0.047],
    'Revolver_Colt_Warehouse.wav': [0.039, 3.860, 7.654, 11.477],
    'Revolver_M29_Cylinder.wav': [0.167, 0.907, 1.697, 2.416, 3.375],
}

_loud = G._loud


def _take(name, i, length, channel='mix', skip=0.0):
    """The i-th listed shot of a pack file (wrapping round), mono, at most length seconds and never into the next shot;
    skip seconds of its front left out (a tail without the shot that made it)."""
    ts = SHOTS[name]
    k = i % len(ts)
    x = S.load(name, channel=channel)
    until = ts[k + 1] - 0.003 if k + 1 < len(ts) else None
    y = S.slice_at(x, ts[k] + skip, length, until=until, search=0.006 if skip == 0.0 else 0.0)
    return y


def _roll(name, start, length, fade_in=0.05):
    """A stretch of a far take's field (the land answering a shot), from start seconds: a texture for the tails."""
    x = S.load(name)
    return S.body_only(x, start, length, fade_in, 0.08)


def _gated(x, after, t60, lp=None, hp=None):
    y = S.gate_tail(x, after, t60)
    st = []
    if hp:
        st.append(F.hpn(hp, 2))
    if lp:
        st.append(F.lpn(lp, 2))
    return F.filt(y, *st, extend=False) if st else y


def _land_wet(dry, r, dur, slaps, far, first, last, rumble_db, rumble_t60, rumble_lp, rumble_peak, rel_db, hp,
              near_db=-5.0, near_t60=0.1):
    """The land's answer alone (no direct sound): guns._land's slap-backs, far echoes and wandering rumble, led by the
    ground and grass close by scattering the shot back within a few milliseconds (near_db under the land's loudest
    moment), so the tail starts with the shot when the game plays it beside Core."""
    wet = G._land(dry, r, dur, slaps, far, first, last, rumble_db, rumble_t60, rumble_lp, rumble_peak, rel_db, hp)
    n = ns(0.25)
    ir = N.white(n, child(r, 'near')) * np.exp(-6.9 * times(n) / near_t60)
    ir[:ns(0.001)] = 0.0
    ir = F.filt(ir, F.lpn(3500.0, 2), extend=False)
    near = F.convolve(F.filt(dry, F.hpn(hp, 2), extend=False), ir)
    return layers(wet, kit.set_level(near, wet, near_db, 0.02))


def _with_roll(wet, roll, at, rel_db):
    """A recorded field roll laid into a synthesized tail at at seconds, its loudest 50 ms rel_db against the tail's."""
    return layers(wet, (kit.set_level(roll, wet, rel_db, 0.05), at))


def _master(x, clip_db=4.0, lookahead=0.001, release=0.03, push_db=2.0):
    return G._master(x, clip_db, push_db, lookahead, release)


def _rise(x, rise):
    """A layer's front eased in over rise seconds (gun identity, 2026-10-28): the Low under a heavy gun's Core builds as
    the Core's crack passes, so the two tops don't land together (their sum, which the game can't limit, kept under
    -1 dBFS); the shove is felt a beat after the crack, as a blast's is."""
    if rise <= 0.0:
        return x
    t = times(x.size)
    return x * np.clip(t / rise, 0.0, 1.0) ** 1.5


def _rec_room(name, i, length, after, t60, hp=120.0, lp=7000.0, channel='mix'):
    """A recorded room's or hall's answer to a shot: the take from 4 ms after its onset (the direct sound left out),
    faded in over 3 ms, its lows thinned (a room's boom would muddy the core) and cut short with a decay."""
    y = _take(name, i, length, channel, skip=0.004)
    y = np.concatenate([np.zeros(ns(0.003)), y])
    y = F.filt(S.gate_tail(y, after, t60), F.hpn(hp, 2), F.lpn(lp, 2), extend=False)
    k = ns(0.006)
    y[:k] *= np.linspace(0.0, 1.0, k)
    return y


def _synth_room(dry, r, t60, size, rel_db, hp=150.0):
    """Our own shot in the room for its first moments, so the recorded room answers this gun and not the one taped."""
    ir = R.room(child(r, 'room'), t60, size)
    wet = F.convolve(F.filt(dry, F.hpn(hp, 2), extend=False), ir)
    return kit.set_level(wet, dry, rel_db, 0.02)


def _far(x, lp, hp=120.0, direct_db=-8.0, direct_len=0.025):
    """A near take made far: the direct sound dulled and pushed under the field it raised (at 50 m the land is as loud
    as the shot), the top rolled off by the air."""
    n = x.size
    g = np.ones(n)
    k = min(n, ns(direct_len))
    g[:k] = float(db2a(direct_db))
    g = Y.smooth(g, 0.004)
    return F.filt(x * g, F.hpn(hp, 2), F.lpn(lp, 2), F.highshelf(3500.0, -4.0), extend=False)


def _cap(x, length, fade=0.15):
    """A tail held to length seconds: past length - fade it eases to silence (a cosine), so a layer the game lays over
    the next shots never runs longer than the kind's tail (gun identity, 2026-10-28: the SMG's 0.6 s, the sniper's 2.5 s)."""
    n = ns(length)
    if x.size <= n:
        return x
    y = x[:n].copy()
    k = ns(fade)
    y[n - k:] *= 0.5 + 0.5 * np.cos(np.linspace(0.0, np.pi, k))
    return y


def _tick(r, lo, hi, db):
    """The sear or hammer's tick at the very start of a Mech layer: it sets where the layer begins, so it lines up with
    the shot when the game plays them together."""
    return kit.metal_click(child(r, 'sear'), lo, hi, 0.01, 0.00005, 5, db)


def _ping(r, f0, t60, db=-10.0):
    """The low-ammo click: a light knock with a short hollow ring, kept below 2 kHz and above 5, so the ear catches it
    over the gun without the 2-5 kHz band biting."""
    knock = kit.thunk(f0 * 0.5, child(r, 'knock'), 0.03, 0.0004, 5, 0.15)
    ring = M.strike(M.parts(f0, f0 * 1.45, child(r, 'ring'), 4, t60, t60 * 0.5, 0.25), M.hammer(0.00008), t60 * 2.0)
    shine = M.strike(M.parts(5600.0, 7800.0, child(r, 'shine'), 3, t60 * 0.5, t60 * 0.3, 0.2), M.hammer(0.00005),
                     t60 * 1.2)
    x = layers((_loud(knock, 0.005), 0.0, 0.0), (_loud(ring, 0.005), 0.0, -2.0), (_loud(shine, 0.005), 0.0, db))
    return F.filt(x, F.peak(3300.0, -8.0, 0.7), F.hp(300.0), extend=False)


# --- The 2026 bundle's handling foley (2026-10-29: cloth and clicks only; Art/Sounds/packs/README.md) -------------------
# Recorded cloth under the sights' synthesized ticks and taps (recipes/guns_identity.py), a latch's thunk, a lock's
# tumblers and a carabiner's gate in the sniper's bolt (guns_sniper.py), a counting machine's clacks and a spring wire's
# clatter in the SMG's and the LMG's action (guns_smg.py, guns_lmg.py). Each sits under the layers it joins.

GAP = 0.25  # the silence make_pack.py puts between an excerpt's windows
# The evenly cut click excerpts: name: (window length, lead to its click, windows).
CLICKS = {
    'Foley_Lock_Tinker.wav': (0.07, 0.02, 6),
    'Foley_Carabiner_Snap.wav': (0.075, 0.015, 6),
    'Foley_Spring_Clatter.wav': (0.16, 0.02, 6),
}
# The cloth takes' usable stretches (s into the excerpt): the swishes' fuller middles, not their near-silent starts.
# Take k of a cue uses stretch k (wrapping round), so a cue's variations never share one. Each stretch has its own
# low-pass (Hz), measured so all four land near a 1.3-1.5 kHz centroid with under a fifth of themselves at 2-5 kHz:
# the slow swishes (Move) are much brighter than the soft rustle (take 24's first), and a cue's takes must match.
CLOTH = [
    ('Foley_Cloth_Move.wav', 0.12, 0.66, 2200.0),
    ('Foley_Cloth_Rustle.wav', 0.30, 0.50, 4500.0),
    ('Foley_Cloth_Move.wav', 0.99, 1.35, 2200.0),
    ('Foley_Cloth_Rustle.wav', 0.78, 1.04, 2400.0),
]


def _click(name, k, before, after, speed=1.0):
    """Take k of an evenly cut click excerpt, from before seconds ahead of its loudest moment to after seconds past it,
    softly edged and played speed times faster; (samples, the click's time in them)."""
    length, lead, count = CLICKS[name]
    x = S.load(name)
    start = (k % count) * (length + GAP)
    s0, s1 = ns(start + lead - 0.012), ns(start + lead + 0.012)
    t = (s0 + int(np.argmax(np.abs(x[s0:s1])))) / 48000.0
    a, b = max(start, t - before), min(start + length, t + after)
    y = x[ns(a):ns(b)].copy()
    y = y * E.end_fade(np.ones(y.size), min(0.02, 0.3 * (b - a)))
    if ns(t - a) > 48:
        y[:48] *= np.linspace(0.0, 1.0, 48)
    if speed != 1.0:
        y = varispeed(y, speed)
    return y, (t - a) / speed


def _cloth_rec(r, dur, k, speed=1.15, attack=0.3, release=0.5, hp=220.0):
    """A recorded T-shirt's swish dur seconds long (take k: the cue's variation): a stretch of a slow swish played speed
    times faster (shorter, a little brighter), shaped like kit.cloth (rising over attack, falling over release, shares
    of dur), its rumble cut and its top rolled off steeply with the ear's touchy band dipped (close-miked cloth puts
    40-50% of itself at 2-5 kHz; the sights come up hundreds of times a session); peak at 0 dB like kit.cloth, so it
    takes a synthesized swish's place."""
    name, a, b, lp = CLOTH[k % len(CLOTH)]
    span = dur * speed
    a0 = a + r.uniform(0.0, max(0.0, b - a - span))
    x = S.load(name)[ns(a0):ns(min(b, a0 + span))]
    y = varispeed(x, speed)
    d = y.size / 48000.0
    y = y * E.ar(d, d * attack, d * release, 2.0)[:y.size]
    y = F.filt(y, F.hpn(hp, 2), F.lpn(lp, 4), F.peak(3300.0, -8.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return normalize(y, 0.0)


def _pat(r, lp=2400.0):
    """The palm's pat on a T-shirt (a recorded one): the soft, dark thud of the stock or the cheek meeting cloth.
    (samples, the pat's time in them)."""
    x = S.load('Foley_Cloth_Pat.wav')
    t = int(np.argmax(np.abs(x[:ns(0.1)]))) / 48000.0
    a = max(0.0, t - 0.004)
    y = x[ns(a):ns(t + 0.12)].copy() * E.end_fade(np.ones(ns(t + 0.12) - ns(a)), 0.05)
    y = varispeed(y, jitter(r, 1.0, 0.06))
    y = F.filt(y, F.hpn(70.0, 2), F.lpn(lp, 2), extend=False)
    return normalize(y, 0.0), t - a
