"""The guns' shots, built from recordings (the user's call, 2026-10-21: royalty-free gun and impact recordings are allowed,
processed through these scripts; everything else stays synthesized). The Bullpup rifle, the Ranchhand pump shotgun and the
Drover revolver each fire as layers the game plays together (Weapons/GunshotSound.cpp, GunshotSound::PlayShot):

    Weapon.<Kind>.Core        the punch: the shot itself, close, with the ground's first answer (everyone hears it)
    Weapon.<Kind>.Mech        the action cycling, close to the ear (the shooter's own gun only)
    Weapon.<Kind>.Low         the thump in the chest (the shooter's own gun only)
    Weapon.<Kind>.TailOpen    the land answering in the open: slap-backs, far echoes, a rolling field
    Weapon.<Kind>.TailTown    walls close by: strong slaps and a flutter between two of them, a shorter roll
    Weapon.<Kind>.TailInside  a room or a hall (recorded warehouse and room tails)
    Weapon.<Kind>.Distant     the shot from 50 m and more (recorded far takes), played instead of Core and a tail
    Weapon.<Kind>.LowAmmo     the rising click of the magazine's last quarter (the player's own; GunfeelKick's pitch)
    Weapon.<Kind>.Fire        the whole shot in one sound (Core + Mech + Low + TailOpen), kept for whoever plays it alone
                              (Abel's spectral pump plays the shotgun's)
    Weapon.Revolver.CritRing  2D: a short, bright steel ring when a revolver crit lands, for its shooter

How a layer is made: single shots sliced out of the takes (lib/sample.py; the shots' times below), the recording's own
room or field gated off so the tail layers can take over, two or three microphones or guns lined up on their onsets
(one for the crisp front, one for the bark and body, one for the weight), then shaped as the guns' own: transient shaping,
EQ to the gun's character (the rifle tight with a mid crack near 1.6 kHz; the shotgun's boom lifted under 120 Hz and its
mud cut; the revolver's bark at 700 Hz and a frame ring), the synthesized crack and pressure heave of the old recipes
(recipes/guns.py) under them so every take weighs the same, light overdrive, and the old master (a soft clip, a 3 kHz dip,
the limiter). Each layer's level in the game (`level`) is its loudness inside Fire's mix, so the layers played together
sound like Fire.

The recordings: Art/Sounds/packs/Sonniss/ (sources and licence in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, db2a, varispeed
from lib import noise as N, env as E, filters as F, dist as D, dynamics as Y, modal as M, reverb as R
from lib import granular as GR, stereo as ST
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes.guns_rec import (SHOTS, _loud, _take, _roll, _gated, _land_wet, _with_roll, _master, _rec_room,  # noqa: F401
                               _synth_room, _far, _tick, _ping)

# --- The Bullpup rifle: tight and punchy, a mid crack ---------------------------------------------------------------------

def _rifle_dry(v, r):
    """The rifle's core before its master: a FAMAS on the ground (the crisp front) over an L85 or an HK416 at 1 m (the
    bark and body), the old synthesized crack and heave under them."""
    snap = _gated(_take('Rifle_FAMAS_1m.wav', v, 0.21), 0.06, 0.18, hp=40.0)
    body_file = 'Rifle_L85_1m.wav' if v % 2 == 0 else 'Rifle_HK416_1m.wav'
    body = _gated(_take(body_file, v // 2 + v % 2, 0.23), 0.045, 0.2, lp=7500.0, hp=55.0)
    crack = G._crack(child(r, 'crack'), 0.00028 * jitter(r, 1.0, 0.12), 0.006, 1600.0 * jitter(r, 1.0, 0.08), -2.0, -4.0)
    heave = G._heave(child(r, 'heave'), 0.006 * jitter(r, 1.0, 0.15), 140.0, 0.08)
    x = layers((_loud(snap), 0.0, 0.0), (_loud(body), 0.0, -2.5), (crack, 0.0, -10.0), (heave, 0.0, -3.0))
    x = Y.transient(x, 5.0, -4.0)
    x = F.filt(x, F.hp(45.0), F.peak(160.0, 2.5, 0.9), F.peak(420.0, -3.0, 1.0), F.peak(1600.0, 3.0, 1.1),
               F.highshelf(9000.0, -3.0), extend=False)
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _loud(G._early(x, r, -15.0, 0.025, 5000.0), 0.002)


def _rifle_mech(v, r):
    """The action close to the ear: the hammer's tick, the carrier hitting the buffer, the bolt slamming home (the old
    synthesized clanks, the recorded rifle's own slam laid on the last)."""
    tick = _tick(r, 2000.0, 6500.0, -8.0)
    buffer = G._clank(child(r, 'buffer'), (1200.0, 4200.0, 0.03), (600.0, 0.025), (220.0, 0.02), (0.0, -5.0, -10.0),
                      0.00006)
    steel = M.strike(M.parts(900.0, 5200.0, child(r, 'home'), 10, 0.05, 0.02, 0.35), M.hammer(0.00005), 0.12)
    rec = _gated(_take('Rifle_Cock.wav', 1 + v % 2, 0.08), 0.03, 0.05, hp=250.0)
    home = _loud(layers((_loud(steel, 0.005), 0.0, -2.0), (_loud(rec, 0.005), 0.0, 0.0),
                        (G._clank(child(r, 'housing'), (1000.0, 3000.0, 0.02), (480.0, 0.03), (180.0, 0.03),
                                  (-6.0, 0.0, -6.0)), 0.0, -5.0)))
    t_buf = 0.026 + 0.006 * r.random()
    t_home = 0.058 + 0.008 * r.random()
    x = layers((_loud(tick, 0.005), 0.0, -10.0), (buffer, t_buf, -4.0), (home, t_home, 0.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _rifle_low(v, r):
    """The thump in the chest: the Tavor's and the L85's lows (the boom a close microphone takes) on the heave."""
    rec = _take('Rifle_Tavor_1m.wav' if v % 2 == 0 else 'Rifle_L85_1m.wav', v, 0.2)
    rec = F.filt(rec, F.hpn(32.0, 2), F.lpn(170.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.007 * jitter(r, 1.0, 0.15), 120.0, 0.12)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -4.0))
    x = D.drive(normalize(x), 3.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.14), 0.01)
    return _loud(F.filt(x, F.lpn(220.0, 2), extend=False), 0.01)


def _rifle_open(dry, v, r):
    # A barn close by and the farmhouse, then the hills out to ~160 m, kept low so full-auto stays clear; a recorded
    # rifle's field rolls under the far echoes.
    s1 = 0.05 + 0.03 * r.random()
    s2 = s1 + 0.09 + 0.06 * r.random()
    wet = _land_wet(dry, r, 1.4, ((s1, -2.0), (s2, -5.0)), 18, 0.22, 0.95, -6.0, 0.8, 700.0, 0.2, -20.0, 180.0)
    roll = _roll(*(('Rifle_G36C_50m.wav', 1.93, 1.05) if v % 2 == 0 else ('Rifle_Tavor_20m.wav', 0.6, 1.3)))
    roll = F.filt(roll, F.hpn(180.0, 2), F.lpn(3500.0, 2), extend=False)
    return _with_roll(wet, roll, 0.12, -3.0)


def _rifle_town(dry, v, r):
    # Walls close by: two strong slaps off the nearest fronts, a flutter between the street's two sides (a slap every
    # 20-30 ms, each a little fainter), a short roll.
    s1 = 0.022 + 0.015 * r.random()
    gap = 0.02 + 0.012 * r.random()
    slaps = [(s1, -1.0), (s1 + 0.035 + 0.02 * r.random(), -3.0)] + [(s1 + gap * (k + 1), -4.0 - 2.5 * k) for k in range(6)]
    return _land_wet(dry, r, 0.9, slaps, 8, 0.12, 0.5, -9.0, 0.45, 900.0, 0.12, -17.0, 200.0, -6.0, 0.05)


def _rifle_inside(dry, v, r):
    # A room or a barn: the HK416's small room (either microphone), else a warehouse's long hall cut short; our shot
    # in a synthesized room first.
    if v < 2:
        rec = _rec_room('Rifle_HK416_Room.wav', 0, 1.2, 0.25, 0.7, 150.0, 7000.0, ('left', 'right')[v])
    else:
        rec = _rec_room('Revolver_Colt_Warehouse.wav', v, 1.1, 0.15, 0.6, 180.0, 6000.0)
    room = _synth_room(dry, r, 0.45, 0.6, -12.0)
    return layers(room, kit.set_level(rec, room, 2.0, 0.02))


def _rifle_distant(v, r):
    # Someone else's rifle 20-55 m off: the recorded far takes, the shot's front softened by the air, the land's
    # roll after it.
    name, k = [('Rifle_G36C_50m.wav', 0), ('Rifle_FAMAS_55m.wav', 1), ('Rifle_G36C_50m.wav', 3),
               ('Rifle_Tavor_20m.wav', 0), ('Rifle_FAMAS_55m.wav', 5), ('Rifle_G36C_50m.wav', 6)][v]
    x = _take(name, k, 1.1)
    x = S.gate_tail(x, 0.09, 0.3)
    x = F.filt(x, F.hpn(110.0, 2), F.lpn(4500.0, 2), F.peak(3100.0, -5.0, 0.8), F.lowshelf(200.0, 3.0), extend=False)
    wet = G._land(x, r, 1.1, ((0.08 + 0.05 * r.random(), -4.0),), 12, 0.2, 0.9, -6.0, 0.7, 600.0, 0.2, -9.0, 150.0)
    return _master(layers(_loud(x), wet), 4.0, 0.0015, 0.05)


# --- The Ranchhand pump shotgun: a huge low boom ---------------------------------------------------------------------------

def _shotgun_dry(v, r):
    """The shotgun's core before its master: a Saiga slug at 1 m (the blast) over a SPAS-12's or a USAS-12's boom and a
    studio slug's weight, with the old crack and heave."""
    blast = _gated(_take('Shotgun_Saiga_1m.wav', v, 0.75), 0.09, 0.45, hp=35.0)
    boom = _take(('Shotgun_SPAS12_1m.wav', 'Shotgun_USAS12_1m.wav')[v % 2], v // 2, 0.7)
    boom = _gated(boom, 0.1, 0.5, lp=1100.0, hp=30.0)
    weight = _take(('Shotgun_TS_Slug.wav', 'Shotgun_TS_Fire.wav')[v % 2], 0, 0.8)
    weight = _gated(weight, 0.12, 0.55, lp=420.0, hp=30.0)
    crack = G._crack(child(r, 'crack'), 0.0005 * jitter(r, 1.0, 0.12), 0.008, 1100.0 * jitter(r, 1.0, 0.08), -3.0, -3.0,
                     1400.0, 9000.0)
    heave = G._heave(child(r, 'heave'), 0.009 * jitter(r, 1.0, 0.15), 120.0, 0.12)
    x = layers((_loud(blast), 0.0, -1.0), (_loud(boom), 0.0, 0.0), (_loud(weight), 0.0, -2.0), (crack, 0.0, -13.0),
               (heave, 0.0, 2.0))
    x = Y.transient(x, 3.0, -1.0)
    x = F.filt(x, F.hp(30.0), F.lowshelf(100.0, 6.0), F.peak(320.0, -3.5, 0.9), F.peak(900.0, 1.0, 0.8),
               F.highshelf(6000.0, -5.0), extend=False)
    x = D.drive(normalize(x), 4.0, 'tanh')
    return _loud(G._early(x, r, -12.0, 0.04, 4000.0), 0.002)


def _shotgun_mech(v, r):
    """The receiver ringing, the forend and action knocking steel-on-wood as the gun kicks, a faint pellet hiss. The
    recorded steel clack it once carried went to the rack (Weapon.Shotgun.Rack, guns_foley.py, 2026-10-22): the pump's
    beats are the rack's now, played between shots."""
    tick = _tick(r, 1600.0, 5200.0, -10.0)
    ring = _loud(M.strike(M.parts(500.0, 2500.0, child(r, 'ring'), 8, 0.25, 0.12, 0.4), M.hammer(0.0002), 0.4), 0.005)
    chunk = G._clank(child(r, 'chunk'), (700.0, 2600.0, 0.05), (260.0 * jitter(r, 1.0, 0.06), 0.07),
                     (110.0 * jitter(r, 1.0, 0.06), 0.07), (0.0, -1.0, -3.0), 0.0001, drive_db=5.0)
    n = ns(0.25)
    sizzle = GR.grit(0.25, child(r, 'sizzle'), 9000.0 * np.exp(-times(n) / 0.05), 3000.0, 9000.0, 2.0, 4)
    sizzle = _loud(sizzle * E.perc(0.25, 0.004, 0.14) * kit.turbulence(n, child(r, 'sflut'), 200.0, 0.6))
    t_chunk = 0.03 + 0.008 * r.random()
    x = layers((_loud(tick, 0.005), 0.0, -12.0), (ring, 0.002, -12.0), (chunk, t_chunk, 0.0),
               (sizzle, 0.003, -14.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _shotgun_low(v, r):
    """The shove in the chest: the SPAS-12's and the studio slug's lows on a long heave (60-120 Hz)."""
    rec = _take(('Shotgun_SPAS12_1m.wav', 'Shotgun_TS_Slug.wav', 'Shotgun_USAS12_1m.wav')[v % 3], v // 3, 0.35)
    rec = F.filt(rec, F.hpn(28.0, 2), F.lpn(150.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.011 * jitter(r, 1.0, 0.15), 110.0, 0.16)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -3.0))
    x = D.drive(normalize(x), 4.0, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.26), 0.01)
    return _loud(F.filt(x, F.lpn(200.0, 2), extend=False), 0.01)


def _shotgun_open(dry, v, r):
    # The yard's buildings, then hills and the valley out to ~250 m: a boom that rolls for a second and a half; the
    # studio slug's long field, or a far SPAS-12's or USAS-12's roll, under it.
    s1 = 0.07 + 0.04 * r.random()
    s2 = s1 + 0.12 + 0.08 * r.random()
    wet = _land_wet(dry, r, 2.1, ((s1, -2.0), (s2, -4.0)), 26, 0.3, 1.5, -3.0, 1.4, 500.0, 0.3, -17.0, 110.0)
    name, at, length = [('Shotgun_TS_Slug.wav', 0.45, 2.4), ('Shotgun_SPAS12_50m.wav', 0.95, 1.65),
                        ('Shotgun_USAS12_55m.wav', 0.55, 1.85), ('Shotgun_TS_Slug.wav', 0.6, 2.2)][v % 4]
    roll = F.filt(S.gate_tail(_roll(name, at, length, 0.08), 0.35, 1.1), F.hpn(90.0, 2), F.lpn(2500.0, 2), extend=False)
    return _with_roll(wet, roll, 0.15, -4.0)


def _shotgun_town(dry, v, r):
    s1 = 0.025 + 0.015 * r.random()
    gap = 0.022 + 0.012 * r.random()
    slaps = [(s1, -0.5), (s1 + 0.04 + 0.025 * r.random(), -2.5)] + [(s1 + gap * (k + 1), -3.5 - 2.2 * k) for k in range(7)]
    return _land_wet(dry, r, 1.3, slaps, 10, 0.15, 0.7, -6.0, 0.7, 700.0, 0.15, -14.0, 130.0, -6.0, 0.06)


def _shotgun_inside(dry, v, r):
    # A pump shotgun in a warehouse, as taped: the hall's long boom; our shot in a synthesized room first.
    rec = _rec_room('Shotgun_870_Warehouse.wav', v, 2.6, 0.4, 1.6, 90.0, 5000.0)
    room = _synth_room(dry, r, 0.7, 0.8, -10.0, 110.0)
    return layers(room, kit.set_level(rec, room, 3.0, 0.02))


def _shotgun_distant(v, r):
    if v < 3:
        name, k = [('Shotgun_SPAS12_50m.wav', 0), ('Shotgun_USAS12_55m.wav', 0), ('Shotgun_USAS12_55m.wav', 1)][v]
        x = S.gate_tail(_take(name, k, 2.0), 0.3, 1.2)
    else:
        name = ('Shotgun_TS_Slug.wav', 'Shotgun_Saiga_1m.wav')[v - 3]
        x = _far(S.gate_tail(_take(name, 2 * v, 2.0), 0.4, 1.4), 1800.0, 90.0, -10.0, 0.03)
    x = F.filt(x, F.hpn(70.0, 2), F.lpn(5000.0, 2), F.lowshelf(150.0, 3.0), F.peak(3100.0, -3.0, 0.9), extend=False)
    wet = G._land(x, r, 1.6, ((0.1 + 0.06 * r.random(), -3.0),), 14, 0.25, 1.3, -4.0, 1.1, 500.0, 0.25, -12.0, 100.0)
    return _master(layers(_loud(x), wet), 4.0, 0.0015, 0.06)


# --- The Drover revolver: a big .44 bark with a ring ---------------------------------------------------------------------

def _revolver_dry(v, r):
    """The revolver's core before its master: an M29 .44 on two microphones at once (1.5 m for the bark, a boundary
    microphone at 2 m for the weight), a designed revolver's low, the old crack, the cylinder gap's spit, and the frame's
    ring."""
    a = _gated(_take('Revolver_M29_1m5.wav', v, 0.6), 0.07, 0.3, hp=70.0)
    b = _gated(_take('Revolver_M29_2m_PZM.wav', v, 0.6), 0.06, 0.28, lp=1800.0, hp=50.0)
    gm = _gated(_take('Revolver_GM_Shot.wav', 0, 0.6), 0.08, 0.35, lp=500.0, hp=35.0)
    crack = G._crack(child(r, 'crack'), 0.0004 * jitter(r, 1.0, 0.12), 0.007, 1300.0 * jitter(r, 1.0, 0.08), -2.0, -3.0,
                     1500.0, 10000.0)
    heave = G._heave(child(r, 'heave'), 0.0075 * jitter(r, 1.0, 0.15), 130.0, 0.1)
    spit = N.band(ns(0.04), child(r, 'spit'), 2500.0, 9000.0) * E.perc(0.04, 0.0002, 0.012)
    spit = _loud(D.drive(normalize(spit), 6.0, 'tanh'), 0.003)
    # The frame and cylinder ringing from the blow: a few low steel modes and a bright pair above the touchy band.
    f0 = 1150.0 * jitter(r, 1.0, 0.06)
    ring = layers(M.strike(M.parts(f0, f0 * 1.7, child(r, 'ring'), 6, 0.22, 0.1, 0.4), M.hammer(0.0001), 0.45),
                  (0.5 * M.strike(M.parts(5600.0, 7200.0, child(r, 'shine'), 3, 0.12, 0.06, 0.3), M.hammer(0.00005),
                                  0.3), 0.0))
    x = layers((_loud(a), 0.0, 0.0), (_loud(b), 0.0, -6.0), (_loud(gm), 0.0, -14.0), (crack, 0.0, -9.0),
               (heave, 0.0, -7.0), (spit, 0.0002, -11.0), (_loud(ring, 0.005), 0.001, -14.0))
    x = Y.transient(x, 2.5, -1.0)
    x = F.filt(x, F.hp(55.0), F.peak(140.0, -2.0, 0.8), F.peak(700.0, 4.5, 0.9), F.peak(1250.0, 2.0, 1.0),
               F.highshelf(9000.0, -2.0), extend=False)
    x = D.drive(normalize(x), 4.0, 'tanh')
    return _loud(G._early(x, r, -12.0, 0.035, 4500.0), 0.002)


def _revolver_mech(v, r):
    """The hammer's fall, then the hand turning the cylinder a chamber on and the bolt catching it (a recorded M29's
    cylinder clicking home laid on the synthesized clicks)."""
    hammer = G._clank(child(r, 'hammer'), (1100.0, 4200.0, 0.02), (520.0, 0.02), (180.0, 0.02), (0.0, -6.0, -10.0),
                      0.00006)
    turn = _loud(kit.metal_click(child(r, 'turn'), 1800.0, 5200.0, 0.02, 0.0001, 5, -10.0), 0.005)
    stop = _loud(kit.metal_click(child(r, 'stop'), 1500.0, 4600.0, 0.025, 0.0001, 5, -8.0), 0.005)
    rec = _gated(_take('Revolver_M29_Cylinder.wav', v, 0.08), 0.025, 0.05, hp=300.0)
    t_turn = 0.11 + 0.02 * r.random()
    x = layers((hammer, 0.0, 0.0), (turn, t_turn, -8.0), (stop, t_turn + 0.03, -6.0), (_loud(rec, 0.005), t_turn + 0.03, -4.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), extend=False)


def _revolver_low(v, r):
    rec = _take(('Revolver_M29_2m_PZM.wav', 'Revolver_GM_Shot.wav')[v % 2], v, 0.3)
    rec = F.filt(rec, F.hpn(30.0, 2), F.lpn(160.0, 4), extend=False)
    heave = G._heave(child(r, 'heave'), 0.009 * jitter(r, 1.0, 0.15), 120.0, 0.14)
    x = layers((_loud(rec, 0.005), 0.0, 0.0), (heave, 0.0, -3.0))
    x = D.drive(normalize(x), 3.5, 'tanh') * E.end_fade(E.perc(x.size / 48000.0, 0.0003, 0.2), 0.01)
    return _loud(F.filt(x, F.lpn(210.0, 2), extend=False), 0.01)


def _revolver_open(dry, v, r):
    # The yard's buildings, then the hills out to ~200 m; the M29's own recorded field (another shot's) under it.
    s1 = 0.06 + 0.035 * r.random()
    s2 = s1 + 0.1 + 0.07 * r.random()
    wet = _land_wet(dry, r, 1.8, ((s1, -2.0), (s2, -4.5)), 22, 0.25, 1.2, -4.5, 1.1, 600.0, 0.25, -18.0, 140.0)
    roll = _take('Revolver_M29_1m5.wav', v + 2, 1.2, skip=0.12)
    roll = F.filt(S.gate_tail(roll, 0.5, 0.8), F.hpn(150.0, 2), F.lpn(3500.0, 2), extend=False)
    k = ns(0.04)
    roll[:k] *= np.linspace(0.0, 1.0, k)
    return _with_roll(wet, roll, 0.12, -2.0)


def _revolver_town(dry, v, r):
    s1 = 0.024 + 0.015 * r.random()
    gap = 0.021 + 0.012 * r.random()
    slaps = [(s1, -0.8), (s1 + 0.038 + 0.02 * r.random(), -3.0)] + [(s1 + gap * (k + 1), -4.0 - 2.4 * k) for k in range(6)]
    return _land_wet(dry, r, 1.1, slaps, 9, 0.14, 0.6, -7.0, 0.6, 800.0, 0.14, -15.0, 160.0, -6.0, 0.055)


def _revolver_inside(dry, v, r):
    # A handgun in a large warehouse, as taped (the .45's hall); our shot in a synthesized room first.
    rec = _rec_room('Revolver_Colt_Warehouse.wav', v, 2.2, 0.35, 1.3, 120.0, 6000.0)
    room = _synth_room(dry, r, 0.6, 0.7, -11.0, 130.0)
    return layers(room, kit.set_level(rec, room, 2.5, 0.02))


def _revolver_distant(v, r):
    if v < 2:
        x = S.gate_tail(_take('Revolver_M66_50m.wav', v, 1.6), 0.25, 0.9)
    else:
        x = _far(S.gate_tail(_take('Revolver_M29_1m5.wav', v + 1, 1.3), 0.35, 0.9), 2200.0, 120.0, -10.0, 0.025)
    x = F.filt(x, F.hpn(100.0, 2), F.lpn(5500.0, 2), F.lowshelf(180.0, 2.5), F.peak(3100.0, -3.0, 0.9), extend=False)
    wet = G._land(x, r, 1.4, ((0.09 + 0.05 * r.random(), -3.5),), 12, 0.22, 1.1, -5.0, 0.9, 550.0, 0.22, -12.0, 130.0)
    return _master(layers(_loud(x), wet), 4.0, 0.0015, 0.05)


# --- The kinds' recipes together, and the cues ---------------------------------------------------------------------------

KINDS = {
    'Rifle': dict(dry=_rifle_dry, mech=_rifle_mech, low=_rifle_low, open=_rifle_open, town=_rifle_town,
                  inside=_rifle_inside, distant=_rifle_distant, clip=4.0, look=0.001, rel=0.03),
    'Shotgun': dict(dry=_shotgun_dry, mech=_shotgun_mech, low=_shotgun_low, open=_shotgun_open, town=_shotgun_town,
                    inside=_shotgun_inside, distant=_shotgun_distant, clip=5.0, look=0.0015, rel=0.05),
    'Revolver': dict(dry=_revolver_dry, mech=_revolver_mech, low=_revolver_low, open=_revolver_open,
                     town=_revolver_town, inside=_revolver_inside, distant=_revolver_distant, clip=7.5, look=0.0012,
                     rel=0.04),
}

# Fire's mix (dB, each layer by its loudest 10 ms against the core's): the shooter's own gun plays the same layers at
# these levels (the cues' `level`s below are their loudness inside this mix).
MIX = {
    'Rifle': dict(mech=-15.5, low=-5.0),
    'Shotgun': dict(mech=-13.0, low=-3.0),
    # The revolver's Low and Mech 1 dB under their first mix (2026-10-22): its own-gun layers indoors peaked at -0.1 dBFS.
    'Revolver': dict(mech=-17.0, low=-10.5),
}


def _k(kind):
    return KINDS[kind]


def _core(kind, v, r):
    k = _k(kind)
    return _master(k['dry'](v, child(r, 'dry')), k['clip'], k['look'], k['rel'])


def _tail(kind, which, v, r):
    k = _k(kind)
    dry = k['dry'](v, child(r, 'dry'))
    return k[which](dry, v, child(r, which))


def _fire(kind, v, r):
    k = _k(kind)
    m = MIX[kind]
    dry = k['dry'](v, child(r, 'dry'))
    mech = k['mech'](v, child(r, 'mech'))
    low = k['low'](v, child(r, 'low'))
    wet = k['open'](dry, v, child(r, 'open'))
    x = layers(dry, (_loud(mech, 0.005), 0.0, m['mech']), (low, 0.0, m['low']), wet)
    return _master(x, k['clip'], k['look'], k['rel'])


def _register(kind, n_core, n_fire, levels, conc, trim=0.0):
    """The kind's cues. levels: Fire's and each layer's against the rifle's Fire (the mix's reference). trim (dB) lowers
    the shooter's whole set (Fire and the layers the game plays together, not Distant) alike, so the layers still add up
    to Fire while their sum's peak stays under the contract's -1 dBFS (2026-10-22: the revolver's, the sniper's and the
    SMG's own-gun sum reached -0.3 to +0.9 dBFS where a tail's or the Low's peak met the Core's)."""
    own = ('fire', 'core', 'mech', 'low', 'open', 'town', 'inside')
    lv = {k: (v - trim if k in own else v) for k, v in levels.items()}

    @cue(f'Weapon.{kind}.Fire', variations=n_fire, att='Gun', jitter=0.03, conc=conc, level=lv['fire'])
    def fire(v, r):
        return _fire(kind, v, r)

    @cue(f'Weapon.{kind}.Core', variations=n_core, att='Gun', jitter=0.03, conc=conc, level=lv['core'])
    def core(v, r):
        return _core(kind, v, r)

    @cue(f'Weapon.{kind}.Mech', variations=4, att='Near', jitter=0.04, conc=4, level=lv['mech'])
    def mech(v, r):
        return _k(kind)['mech'](v, r)

    @cue(f'Weapon.{kind}.Low', variations=4, att='Near', jitter=0.03, conc=4, level=lv['low'])
    def low(v, r):
        return _k(kind)['low'](v, r)

    @cue(f'Weapon.{kind}.TailOpen', variations=4, att='Gun', jitter=0.03, conc=6, level=lv['open'], swell=True)
    def tail_open(v, r):
        return _tail(kind, 'open', v, r)

    @cue(f'Weapon.{kind}.TailTown', variations=4, att='Gun', jitter=0.03, conc=6, level=lv['town'], swell=True)
    def tail_town(v, r):
        return _tail(kind, 'town', v, r)

    @cue(f'Weapon.{kind}.TailInside', variations=4, att='Gun', jitter=0.03, conc=6, level=lv['inside'], swell=True)
    def tail_inside(v, r):
        return _tail(kind, 'inside', v, r)

    @cue(f'Weapon.{kind}.Distant', variations=lv['n_distant'], att='Gun', jitter=0.04, conc=4, level=lv['distant'])
    def distant(v, r):
        return _k(kind)['distant'](v, r)


# The levels: Fire's as before (the rifle 0, the shotgun +1, the revolver +0.5); Core, Mech, Low and TailOpen at their
# loudness inside Fire's mix (measured: each layer's loudest 200 ms against Fire's, over four takes), so the shooter's
# layers add up to Fire; a town's tail 1.5-0.5 dB over the open one, a room's 4-5 dB over it (a room answers louder, but
# a rifle's burst indoors mustn't wash out); Distant 2-3 dB under Fire (the far takes carry their own field).
_register('Rifle', 6, 6, dict(fire=0.0, core=-0.6, mech=-14.1, low=-9.6, open=-15.1, town=-13.5, inside=-10.5,
                              distant=-3.0, n_distant=6), 8)
_register('Shotgun', 5, 5, dict(fire=1.0, core=0.2, mech=-13.8, low=-5.9, open=-12.2, town=-11.0, inside=-7.5,
                                distant=-2.0, n_distant=5), 4)
_register('Revolver', 6, 6, dict(fire=0.5, core=0.2, mech=-17.6, low=-12.1, open=-10.0, town=-9.3, inside=-5.8,
                                 distant=-2.5, n_distant=5), 6, trim=0.8)


# --- Low ammo and the crit ring ----------------------------------------------------------------------------------------

@cue('Weapon.Rifle.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def rifle_low_ammo(v, r):
    # The follower knocking up through a nearly empty polymer magazine: a hollow tick with a short ring.
    return _ping(r, [1250.0, 1320.0, 1180.0][v], 0.05)


@cue('Weapon.Shotgun.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def shotgun_low_ammo(v, r):
    # The magazine tube's spring with few shells left: a lower, rounder knock and a springy ring.
    return _ping(r, [820.0, 870.0, 780.0][v], 0.08, -14.0)


@cue('Weapon.Revolver.LowAmmo', variations=3, att='Near', jitter=0.02, conc=2, level=-12.0)
def revolver_low_ammo(v, r):
    # The cylinder's empty chambers coming round: a dry steel tick with a small ring.
    return _ping(r, [1500.0, 1580.0, 1440.0][v], 0.04, -8.0)


@cue('Weapon.Revolver.CritRing', variations=3, space='2D', jitter=0.02, conc=2, level=-8.0)
def revolver_crit_ring(v, r):
    # A crit landing, for its shooter: a struck steel plate's ring (a recorded heavy-metal impact's ringing modes, taken up
    # half an octave) under a clean bell-like ping and a quick sparkle above it, spread across the ears.
    ring = S.body_only(S.load('Impact_MetalHeavy.wav'), 0.075, 0.7, 0.004, 0.2)
    ring = F.filt(varispeed(ring, [1.45, 1.55, 1.38][v]), F.hpn(900.0, 2), F.peak(3300.0, -6.0, 0.8), extend=False)
    ring = ring * E.end_fade(E.perc(ring.size / 48000.0, 0.002, 0.45), 0.02)
    f0 = [1720.0, 1810.0, 1650.0][v]
    ping = M.strike(M.parts(f0, f0 * 1.02, child(r, 'ping'), 2, 0.6, 0.5, 0.05), M.hammer(0.00004), 0.8)
    over = M.strike(M.parts(f0 * 3.4, f0 * 4.6, child(r, 'over'), 3, 0.25, 0.15, 0.2), M.hammer(0.00003), 0.4)
    tick = kit.metal_click(child(r, 'tick'), 2400.0, 8000.0, 0.012, 0.00004, 5, -10.0)
    x = layers((_loud(ping, 0.005), 0.0, 0.0), (_loud(ring, 0.01), 0.0, -4.0), (_loud(over, 0.005), 0.004, -12.0),
               (_loud(tick, 0.005), 0.0, -8.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), extend=False)
    return ST.widen(x, child(r, 'wide'), 0.35)


# The four new kinds (Phase 12.3, 2026-10-21 night): each module adds its kind to KINDS and MIX and registers its cues
# with _register above, plus its own (reloads, the bolt, the spin-up, the rocket).
from recipes import guns_smg, guns_sniper, guns_lmg, guns_launcher  # noqa: E402,F401
