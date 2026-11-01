"""Landfall's story cues (Docs/Work/2026-10-31-Wardens/CONTRACT.md, SOUND; the ship's own sounds are in landfall.py):
Cornelius Vane's radio and loudspeakers, the spyglass, the Great Lantern leaning toward a stolen soul, the soul smog,
Delia's old double-barrel shotgun and the card's turn-in stamp.

- Story.Vane.RadioIn (2D, ~0.5 s): the Company's radio keyed on: a relay's clack, a burst of squelch, a carrier locking
  with a rising tone, crackle, the mains hum under it; all through a small horn speaker and a coarse converter.
- Story.Vane.RadioOut (2D, ~0.35 s): keyed off: a fading burst of static and the relay's last click.
- Story.Vane.RadioHiss (2D loop, 6 s): the open channel's bed while he speaks: hiss, mains hum with its harmonics, crackle,
  slow fading.
- Story.Spyglass.Open (2D, ~0.9 s): the lens cap flicked off and a brass telescope drawn out in three slides, each stopping
  with a tick, a glass ring at the end.
- Story.Lantern.Lean (3D, ~2.7 s, swells): the Great Lantern's flame leaning toward a stolen soul: a gust, flame
  fluttering, a warm glass tone bending up, a chime, a far whisper in the flame.
- Ambience.Smog (2D loop, 16 s): soul smog over the valley: a low drone, thick air, a toxic hiss, the dredge barge's slow
  throbs, faint whispers of the dead, a tick of cinders.
- Story.Delia.Shotgun (3D 'Gun', 5 takes): Delia's old double-barrel: Ranchhand's recordings (guns_shots) slowed to a
  bigger bore, a long barrel's ring, black powder's spit; a heavier boom than the player's pump.
- Story.Delia.Reload (3D, 4 takes, ~2.2 s): the lever thumbed and the barrels dropped open, two hulls kicked out ringing on
  boards, two shells thumbed in, the gun snapped shut.
- UI.TurnIn.Stamp (2D, 3 takes, ~1.6 s): the turn-in card's stamp: a heavy rubber stamp slamming down on the card's slam
  (0.11 s), a purse of silver chinking, and three rising notes resolving on a major chord over a low steel string.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, fade, varispeed
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, granular as G, voice as V
from lib import reverb as R
from recipes import cue
from recipes import kit
from recipes import guns as GN
from recipes import guns_shots as GS
from recipes.guns_rec import _master as _gun_master, _loud
from recipes.creatures import _haunt
from recipes.world import _fold, _pnoise, _slow, _circ
from recipes.orelift import _lv, _whole
from recipes.givers import swish, stamp, STAMP_LANDS, calm
from recipes.feedback import _pings
from recipes.ui import _ui
from recipes.landfall import _loopify, _rel, _iron, _calm


# --- The radio ----------------------------------------------------------------------------------------------------------------

def _radio(x, lo=400.0, hi=3300.0, bits=9, drive_db=7.0):
    """What a small horn speaker and a coarse converter make of a sound: only the middle of it, pushed, crunchy."""
    y = F.filt(x, F.hpn(lo, 2), F.lpn(hi, 4), F.peak(1500.0, 4.0, 1.1), extend=False)
    y = D.drive(normalize(y), drive_db, 'tanh')
    return D.crush(y, bits)


def _hum(n, r, f=60.0, level=1.0):
    """Mains hum: a buzz of 60 Hz and its harmonics."""
    t = times(n)
    ph = r.uniform(0.0, 6.28)
    return level * sum(a * np.sin(2 * np.pi * f * k * t + ph * k) for k, a in enumerate((1.0, 0.6, 0.45, 0.3, 0.2), 1))


@cue('Story.Vane.RadioIn', variations=3, space='2D', cls='Effects', jitter=0.0, conc=1, level=-8.0)
def vane_radio_in(v, r):
    # Keyed on: the relay clacking, a rush of squelch as the carrier opens, a tone locking in (rising), crackle on the
    # line and the mains hum humming under it, a thump of the mouthpiece.
    n = ns(0.5)
    t = times(n)
    relay = kit.clack(child(r, 'relay'), 640.0 * jitter(r, 1, 0.06), 1200.0, 3600.0, 0.02, 0.03, -3.0, -8.0)
    squelch = kit.burst(0.22, child(r, 'sq'), 0.09, lo=450.0, hi=3400.0, color=-1.0, attack=0.001)
    carrier = O.sine(O.expsweep(620.0, 1350.0 * jitter(r, 1, 0.05), 0.12, 1.4)) * E.ar(0.12, 0.01, 0.07, 1.5)
    crackle = G.grit(0.5, child(r, 'crk'), 1100.0 * np.exp(-t / 0.2) + 60.0, 700.0, 4600.0, 1.4, 4)
    hum = _hum(n, child(r, 'hum')) * E.ar(0.5, 0.03, 0.3, 1.5)
    pop = kit.noise_thump(0.07, child(r, 'pop'), 800.0, 150.0, 0.03, 1.0, 0.03, 0.001)
    x = layers((relay, 0.0, 0.0), (pop, 0.0, -6.0), (_rel(squelch, relay, -4.0, 0.02), 0.01, 0.0),
               (_rel(carrier, relay, -10.0, 0.02), 0.06, 0.0), (_rel(normalize(crackle), relay, -12.0, 0.02), 0.03, 0.0),
               (_rel(hum, relay, -14.0, 0.05), 0.0, 0.0))
    return _ui(_radio(x), r, 0.2, -26.0, 0.15)


@cue('Story.Vane.RadioOut', variations=3, space='2D', cls='Effects', jitter=0.0, conc=1, level=-10.0)
def vane_radio_out(v, r):
    # Keyed off: a short fading burst of static (the carrier dropping, its band closing), the relay's last click.
    n = ns(0.3)
    sq = N.shaped(n, child(r, 'sq'), -1.0)
    sq = F.sweep(sq, 'lp', O.expsweep(3400.0, 650.0, 0.3, 1.0, n=n), 0.8) * E.perc(0.3, 0.002, 0.2)
    crackle = G.grit(0.3, child(r, 'crk'), 900.0 * np.exp(-times(n) / 0.15) + 200.0, 700.0, 4200.0, 1.4, 4)
    relay = kit.clack(child(r, 'relay'), 560.0 * jitter(r, 1, 0.06), 1100.0, 3400.0, 0.02, 0.03, -3.0, -9.0)
    x = layers((normalize(sq), 0.0, 0.0), (_rel(normalize(crackle), sq, -8.0, 0.02), 0.0, 0.0),
               (_rel(relay, sq, -3.0, 0.01), 0.12 + 0.02 * v, 0.0))
    return _ui(_radio(x), r, 0.2, -26.0, 0.15)


@cue('Story.Vane.RadioHiss', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, loop=True, level=-26.0,
     align=False)
def vane_radio_hiss(v, r):
    # The open channel while Vane speaks: carrier hiss that fades and swells, mains hum and its harmonics, crackle and the
    # odd pop, all through the horn speaker. Six seconds, whole cycles.
    dur = 6.0
    L = ns(dur)
    t = times(L)
    hiss = _pnoise(L, child(r, 'hiss'), 300.0, 4600.0, -1.0) * _slow(L, child(r, 'hg'), 0.8, 0.45)
    hum = sum(a * np.sin(2 * np.pi * _whole(60.0 * k, dur) * t + 1.3 * k) for k, a in enumerate((1.0, 0.6, 0.45, 0.3, 0.2), 1))
    hum = hum * (0.75 + 0.25 * _slow(L, child(r, 'ug'), 0.6, 0.0))
    crackle = G.grit(dur, child(r, 'crk'), 55.0, 700.0, 4800.0, 1.4, 4, periodic=True)
    pops = G.grit(dur, child(r, 'pops'), 3.0, 500.0, 2400.0, 1.0, 3, amp=(0.5, 1.0), periodic=True)
    x = _lv(hiss, 0.0) + _lv(hum, -9.0) + _lv(crackle, -7.0) + _lv(pops, -9.0)
    x = _circ(x, F.hpn(380.0, 2), F.lpn(3200.0, 4), F.peak(1500.0, 4.0, 1.1), F.peak(3300.0, -4.0, 0.9))
    return np.tanh(x / (np.max(np.abs(x)) + 1e-9) * 2.2)


# --- The spyglass --------------------------------------------------------------------------------------------------------------

def _slide(r, dur, f0, f1):
    """Brass sliding in brass: a smooth scrape with a fine grain, ringing a few tight resonances."""
    s = kit.scrape(dur, r, f0, f1, 20.0, 0.35, (1.0, 2.1, 3.3), 0.15, 0.4, 3500.0, -26.0)
    return F.filt(s, F.lp(5000.0, 0.7), F.peak(3300.0, -6.0, 0.8), extend=False)


@cue('Story.Spyglass.Open', variations=3, space='2D', cls='Effects', jitter=0.0, conc=1, level=-12.0)
def spyglass_open(v, r):
    # The lens cap flicked off (a soft leather tap and a tick), the brass tube drawn out in three slides each ending on a
    # crisp tick, a bright glass ring as the lens comes up to the eye.
    cap = layers(kit.burst(0.05, child(r, 'cap'), 0.02, lo=300.0, hi=2500.0, attack=0.003),
                 (kit.brass_tick(child(r, 'captick'), 1500.0 * jitter(r, 1, 0.05), 0.02, 3, 0.0002, -10.0, -8.0, 0.1), 0.01, -3.0))
    parts = [(cap, 0.0, 0.0)]
    t = 0.07
    for i in range(3):
        d = 0.15 + 0.02 * r.random()
        sl = _slide(child(r, 'slide', i), d, 1300.0 + 200.0 * i, 2300.0 + 300.0 * i)
        tick = kit.brass_tick(child(r, 'stop', i), (1900.0 + 280.0 * i) * jitter(r, 1, 0.03), 0.03, 4, 0.0001, -8.0, -7.0, 0.2)
        parts += [(_rel(sl, tick, -9.0, 0.03), t, 0.0), (tick, t + d, -2.0 + 1.0 * i)]
        t += d + 0.05
    glass = kit.chime(2700.0 * jitter(r, 1, 0.03), child(r, 'glass'), 0.7, 0.4, 0.0003, 0.7, -26.0)
    parts.append((glass, t, -16.0))
    return _ui(calm(layers(*parts)), r, 0.25, -22.0, 0.2)


# --- The Great Lantern ---------------------------------------------------------------------------------------------------------

@cue('Story.Lantern.Lean', variations=3, att='Creature', jitter=0.03, conc=1, level=-11.0, swell=True)
def lantern_lean(v, r):
    # The Great Lantern's flame leaning toward a stolen soul: a gust of air drawn across it, the flame fluttering and
    # crackling, a warm glass tone bending upward (a fifth and an octave over it), a soft chime at the top of the lean, and
    # a far whisper, someone's name, in the flame.
    dur = 2.7
    n = ns(dur)
    f0 = (196.0, 220.0, 185.0)[v]
    bend = O.glide([(0.0, f0 * 0.97), (dur * 0.6, f0 * 1.03), (dur, f0 * 1.02)], n=n)
    ph = O.phase(bend, n)
    vib = 1.0 + 0.004 * np.sin(2 * np.pi * 5.2 * times(n))
    tone = (np.sin(2 * np.pi * ph) + 0.5 * np.sin(3 * np.pi * ph * vib) + 0.25 * np.sin(4 * np.pi * ph)) * E.swell(dur, 1.4, 0.9, 1.6)
    gust = kit.whoosh(dur, child(r, 'gust'), 240.0, 800.0, 0.6, 0.35, -2.5)
    flutter = F.filt(N.shaped(n, child(r, 'fl'), -1.0) * kit.turbulence(n, child(r, 'flt'), 90.0, 0.8),
                     F.hp(500.0, 0.7), F.lp(5000.0, 0.7), extend=False) * E.ar(dur, 0.9, 1.5, 1.5)
    crackle = G.grit(dur, child(r, 'crk'), 90.0, 1600.0, 6500.0, 1.5, 4) * E.ar(dur, 0.6, 1.5, 1.5)
    chime = kit.chime(f0 * 4.0, child(r, 'chime'), 1.6, 0.5, 0.0004, 0.5, -30.0)
    wh = V.whisper(1.6, [(0.0, 'o'), (0.8, 'a'), (1.6, 'u')], child(r, 'wh'), shift=1.0) * E.ar(1.6, 0.6, 0.8, 2.0)
    wh = F.filt(wh, F.hp(260.0, 0.7), F.lp(2600.0, 0.7), extend=False)
    x = layers((normalize(tone), 0.0, 0.0), (_rel(gust, tone, -9.0, 0.1), 0.0, 0.0), (_rel(flutter, tone, -11.0, 0.1), 0.0, 0.0),
               (_rel(normalize(crackle), tone, -16.0, 0.1), 0.0, 0.0), (_rel(chime, tone, -10.0, 0.1), 1.0, 0.0),
               (_rel(wh, tone, -17.0, 0.1), 0.6, 0.0))
    return _haunt(_calm(x), r, -9.0, 1.8, 3000.0)


# --- Soul smog -------------------------------------------------------------------------------------------------------------------

@cue('Ambience.Smog', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-27.0, align=False)
def smog(v, r):
    # The valley under the soul smog: a low drone of detuned tones beating slowly, thick dark air drifting, a faint toxic
    # hiss, the dredge barge's slow throb every two seconds with the clank of its iron answering, whispers of the dead
    # that never quite say a word, cinders ticking. Sixteen seconds.
    dur = 16.0
    L = ns(dur)
    t = times(L)
    drone = np.zeros(L)
    for f, a, p in ((49.0, 1.0, 0.3), (49.45, 0.8, 1.1), (98.3, 0.35, 2.0), (73.6, 0.45, 0.6)):
        drone += a * np.sin(2 * np.pi * _whole(f, dur) * t + p)
    drone *= 0.7 + 0.3 * _slow(L, child(r, 'dg'), 0.2, 0.0)
    air = _circ(_pnoise(L, child(r, 'air'), 90.0, 1000.0, -5.0), F.peak(220.0, 4.0, 0.9)) * _slow(L, child(r, 'ag'), 0.13, 0.35)
    hiss = _pnoise(L, child(r, 'hiss'), 1800.0, 6500.0, -1.0) * _slow(L, child(r, 'hg'), 0.3, 0.0) ** 2
    events = []
    for k in range(8):
        rk = child(r, 'throb', k)
        thump = kit.noise_thump(0.9, rk, jitter(rk, 170.0, 0.1), 38.0, 0.6, 1.0, 0.4, 0.04)
        events.append((thump * rk.uniform(0.7, 1.0), k * 2.0 + 0.5 + rk.uniform(-0.05, 0.05)))
        if k % 2 == 1:
            plate = M.strike(M.plate(jitter(rk, 150.0, 0.1), child(rk, 'pl'), 18, 1.2, 0.5, tilt=0.8), M.hammer(0.001), 1.6)
            events.append((F.filt(normalize(plate), F.lp(900.0, 0.7), extend=False) * 0.4, k * 2.0 + 0.6))
    throbs = _fold(L, events)
    whisper_events = []
    for k in range(7):
        rk = child(r, 'wh', k)
        keys = [(0.0, ('o', 'u', 'a', 'oo')[k % 4]), (0.6, ('u', 'a', 'o', 'u')[k % 4]), (1.2, 'o')]
        w = V.whisper(1.3, keys, rk, shift=jitter(rk, 1.0, 0.1))
        w = F.filt(w * E.ar(1.3, 0.4, 0.7, 2.0), F.hp(260.0, 0.7), F.lp(2400.0, 0.7), extend=False)
        whisper_events.append((w * rk.uniform(0.6, 1.0), (k + rk.uniform(0.1, 0.8)) * dur / 7.0))
    whispers = _fold(L, whisper_events)
    embers = G.grit(dur, child(r, 'em'), 14.0, 2200.0, 7000.0, 2.0, 3, periodic=True)
    x = (_lv(drone, 0.0) + _lv(air, -4.0) + _lv(hiss, -14.0) + _lv(throbs, -6.0) + _lv(whispers, -10.0) + _lv(embers, -22.0))
    return _circ(x, F.hp(30.0), F.highshelf(6500.0, -4.0))


# --- Delia's shotgun -------------------------------------------------------------------------------------------------------------

@cue('Story.Delia.Shotgun', variations=5, att='Gun', jitter=0.03, conc=3, level=1.0)
def delia_shotgun(v, r):
    # An old double-barrel loaded with black powder: the Ranchhand's recorded blast slowed to a bigger bore (a deeper,
    # longer boom than the pump's), the hammer's click leading it, a long barrel ringing after, powder spitting, the
    # yard's answer and a deep push in the chest.
    dry = GS._shotgun_dry(v, child(r, 'dry'))
    big = varispeed(dry, 0.88 - 0.01 * (v % 3))
    big = F.filt(big, F.lowshelf(90.0, 3.0), extend=False)
    click = kit.metal_click(child(r, 'click'), 2500.0, 7000.0, 0.02, 0.00006, 4, -8.0)
    ring = normalize(M.strike(M.bar(jitter(r, 310.0, 0.05), child(r, 'ring'), 0.8, 0.6, 6, 0.01, 0.7), M.hammer(0.0003), 1.1))
    ring = F.filt(ring, F.lp(2800.0, 0.7), extend=False)
    n = ns(0.4)
    spit = G.grit(0.4, child(r, 'spit'), 7000.0 * np.exp(-times(n) / 0.07), 2000.0, 8000.0, 2.0, 4)
    spit = _loud(spit * E.perc(0.4, 0.004, 0.18) * kit.turbulence(n, child(r, 'sf'), 200.0, 0.6))
    low = GS._shotgun_low(v, child(r, 'low'))
    wet = GS._shotgun_open(dry, v, child(r, 'open'))
    lead = 0.012
    x = layers((click, 0.0, -14.0), (_loud(big), lead, 0.0), (_rel(ring, big, -17.0, 0.01), lead + 0.01, 0.0),
               (_rel(spit, big, -19.0, 0.01), lead + 0.005, 0.0), (low, lead, -4.0), (wet, lead, 0.0))
    return _gun_master(x, 5.0, 0.0015, 0.05, 2.0)


@cue('Story.Delia.Reload', variations=4, att='Near', jitter=0.03, conc=2, level=-10.0)
def delia_reload(v, r):
    # The lever thumbed over and the barrels dropped open (a clank, a hinge's creak), two spent hulls kicked out ringing and
    # bouncing on boards, two fresh shells thumbed into the chambers, the gun snapped shut with a heavy clack and a click.
    lever = GN._clank(child(r, 'lever'), (1400.0, 4000.0, 0.03), (520.0, 0.04), (180.0, 0.04), (0.0, -4.0, -8.0), 0.00008)
    drop = GN._clank(child(r, 'drop'), (900.0, 3000.0, 0.05), (330.0 * jitter(r, 1, 0.05), 0.06), (140.0, 0.06), (0.0, -2.0, -5.0))
    hinge = _iron(0.2, child(r, 'hinge'), 90.0, 50.0, 1100.0) * 0.4
    parts = [(lever, 0.0, -4.0), (drop, 0.12, 0.0), (hinge, 0.1, -10.0)]
    for i, t0 in enumerate((0.5 + 0.03 * v, 0.63 + 0.02 * v)):
        rr = child(r, 'hull', i)
        f = jitter(rr, 1700.0 + 150.0 * i, 0.05)
        ringing = normalize(M.strike(M.shell(f, child(rr, 'sh'), 0.22, 6), M.hammer(0.0004), 0.4))
        knock = kit.thunk(jitter(rr, 700.0, 0.1), child(rr, 'kn'), 0.05, 0.0004, 5, 0.1)
        parts += [(ringing, t0, -9.0), (knock, t0, -6.0)]
        for j, (gap, db) in enumerate(((0.09, -9.0), (0.07, -15.0))):
            t0 += gap * jitter(rr, 1, 0.2)
            parts.append((kit.brass_tick(child(rr, 'b', j), f * 0.6, 0.03, 4, 0.0002, -10.0, -8.0, 0.1), t0, db))
    for i, t0 in enumerate((1.1 + 0.03 * v, 1.35 + 0.03 * v)):
        rr = child(r, 'shell', i)
        thumb = kit.thunk(jitter(rr, 900.0, 0.1), child(rr, 'th'), 0.04, 0.0006, 5, 0.1)
        cloth = kit.cloth(0.12, child(rr, 'cl'), 300.0, 2600.0, 120.0, 0.4, 0.5)
        tick = kit.brass_tick(child(rr, 'tk'), 1500.0 * jitter(rr, 1, 0.1), 0.02, 3, 0.0002, -10.0, -8.0, 0.1)
        parts += [(cloth, t0 - 0.06, -27.0), (thumb, t0, -2.0), (tick, t0, -6.0)]
    shut = GN._clank(child(r, 'shut'), (700.0, 2600.0, 0.05), (360.0, 0.06), (150.0, 0.06), (0.0, -1.0, -4.0), 0.0001, drive_db=5.0)
    latch = kit.metal_click(child(r, 'latch'), 1800.0, 6000.0, 0.025, 0.00008, 5, -8.0)
    parts += [(shut, 1.78 + 0.03 * v, 0.0), (latch, 1.84 + 0.03 * v, -6.0),
              (kit.cloth(2.0, child(r, 'bed'), 250.0, 2200.0, 90.0, 0.3, 0.5), 0.0, -30.0)]
    x = layers(*parts)
    return F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)


# --- The turn-in stamp -----------------------------------------------------------------------------------------------------------

@cue('UI.TurnIn.Stamp', variations=3, space='2D', cls='Interface', jitter=0.0, conc=1, level=-8.0)
def turn_in_stamp(v, r):
    # The card's stamp coming down on its slam (0.11 s in): the hand's swish, a heavy rubber stamp's knock and thud, a
    # purse of silver tipped out and chinking, then three notes rising to a major chord, held over a low steel string.
    land = STAMP_LANDS
    s = layers((swish(r), 0.0, -13.0), (stamp(child(r, 'stamp'), 220.0 * jitter(r, 1, 0.04), -20.0), land, 2.0))
    s = _ui(calm(s, 55.0), child(r, 'room'), 0.25, -18.0, 0.3)
    silver = kit.jingle(0.5, child(r, 'silver'), 7, 2800.0, 8500.0, 0.06, 0.22, 0.0)
    silver = F.filt(silver, F.peak(3300.0, -5.0, 0.9), extend=False)
    a, b, c = [('D6', 'F#6', 'A6'), ('C6', 'E6', 'G6'), ('E6', 'G#6', 'B6')][v]
    low = ['D4', 'C4', 'E4'][v]
    pings = _pings(child(r, 'pings'), [(a, 0.0, -3.0, -0.3, 0.5), (b, 0.1, -2.0, 0.0, 0.6), (c, 0.2, 0.0, 0.3, 1.1)],
                   [(low, 0.0, -10.0, 0.0, 1.2)], -15.0, 0.9)
    return layers((s, 0.0, 0.0), (silver, land + 0.04, -14.0), (pings, land + 0.09, -3.0))
