"""The player's own feedback (2D): the hit marker's tick, the critical hit, the kill, and the stingers for a level, a
mission step and a mission's end, a gun's notch milestone and a lifted curse.

The voice is brass and steel string: small brass ticks and chimes, plucked strings on a wooden box. Hits are short and
crisp, kept off the ear's most sensitive band so a long firefight doesn't tire it; the kill and the stingers ring.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import voice as V
from recipes import cue
from recipes import kit


def _space(x, r, kind='room', wet_db=-18.0, t60=0.25, width=0.25):
    """A stereo space around a mono hit: the dry sound in the middle (with a little width), a reverb around it."""
    dry = S.widen(x, child(r, 'wide'), width)
    if kind == 'plate':
        ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=t60)
    else:
        ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=t60, size=0.6)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.02))


@cue('UI.HitMarker', variations=3, space='2D', cls='Interface', jitter=0.02, conc=6, level=-11.0)
def hit_marker(v, r):
    # A crisp brass tick: its note sits under 2 kHz, the click's air above 5 kHz, little in between, so a hundred of
    # them in a fight stay pleasant.
    tick = kit.brass_tick(child(r, 'tick'), 1650.0 * jitter(r, 1, 0.025), 0.028, 4, 0.00007, -10.0, -5.0, 0.2)
    tick = F.filt(tick, F.peak(3200.0, -5.0, 0.9), extend=False)
    air = kit.burst(0.006, child(r, 'air'), 0.0025, lo=6000.0, hi=12000.0)
    knock = kit.thump(300.0, 220.0, 0.02, 0.012, attack=0.0003)
    x = layers((tick, 0.0, 0.0), (air, 0.0, -13.0), (knock, 0.0, -13.0))
    return _space(x, r, 'room', -22.0, 0.15, 0.2)


@cue('UI.HitMarkerCrit', variations=3, space='2D', cls='Interface', jitter=0.015, conc=6, level=-8.0)
def hit_marker_crit(v, r):
    # Heavier: the tick, a thunk with weight under it, and a bright ting that rings a moment (a struck steel rod).
    tick = kit.brass_tick(child(r, 'tick'), 1900.0 * jitter(r, 1, 0.02), 0.03, 4, 0.00006, -8.0, -4.0, 0.3)
    tick = F.filt(tick, F.peak(3300.0, -4.0, 0.9), extend=False)
    thunk = kit.thump(210.0, 85.0, 0.09, 0.05, attack=0.0004, drive_db=3.0)
    knock = kit.noise_thump(0.07, child(r, 'knock'), 2200.0, 300.0, 0.035, 1.0, 0.02)
    rod = M.bar(2350.0 * jitter(r, 1, 0.02), child(r, 'rod'), 0.28, 0.7, 4, 0.004, 0.9)
    ting = normalize(M.strike(rod, M.hammer(0.00006), 0.4))
    ting = F.filt(ting, F.peak(3400.0, -4.0, 1.0), extend=False)
    x = layers((tick, 0.0, -1.0), (thunk, 0.0, -3.0), (knock, 0.0, -9.0), (ting, 0.004, -7.0))
    return _space(x, r, 'plate', -18.0, 0.5, 0.3)


@cue('UI.Kill', variations=3, space='2D', cls='Interface', jitter=0.0, conc=3, level=-6.0)
def kill(v, r):
    # The bounty paid: a deep punch, then two brass chimes climbing (ka-ching), and a sparkle of small ticks.
    punch = kit.thump(115.0, 44.0, 0.28, 0.15, attack=0.0006, drive_db=4.0)
    knock = kit.noise_thump(0.12, child(r, 'knock'), 2000.0, 220.0, 0.06, 1.0, 0.03)
    tick = kit.brass_tick(child(r, 'tick'), 1500.0, 0.03, 4, 0.00007, -10.0, -6.0, 0.2)
    first, second = [('E5', 'A5'), ('D5', 'A5'), ('G5', 'C6')][v]
    c1 = kit.chime(kit.note(first), child(r, 'c1'), 0.9, 0.8)
    c2 = kit.chime(kit.note(second), child(r, 'c2'), 1.3, 0.85)
    sparkle = np.zeros((2, 1))
    sr = child(r, 'sparkle')
    for i in range(7):
        t = 0.06 + 0.32 * sr.random() ** 1.4
        s = kit.metal_click(sr, 4200.0, 9500.0, 0.03, 0.00004, 3, -20.0)
        sparkle = layers(sparkle, (S.pan(s, sr.uniform(-0.8, 0.8)), t, sr.uniform(-10.0, -2.0)))
    body = layers((punch, 0.0, -2.0), (knock, 0.0, -10.0), (tick, 0.0, -6.0), (c1, 0.0, -5.0), (c2, 0.075, -3.0))
    x = layers(_space(body, r, 'plate', -15.0, 1.1, 0.3), (normalize(sparkle), 0.0, -22.0))
    return x


@cue('UI.LevelUp', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-4.0, swell=True)
def level_up(v, r):
    # A rising rush into a struck chord: a steel-string arpeggio climbing D major, a high chime over it, a low hit
    # under it, and a long shimmering tail.
    t0 = 0.42
    rise = kit.whoosh(0.46, child(r, 'rise'), 300.0, 4500.0, 1.2, 0.95, -1.0, 1.4)
    hit = kit.thump(90.0, 40.0, 0.6, 0.35, attack=0.002, drive_db=3.0)
    air = kit.noise_thump(0.3, child(r, 'air'), 1500.0, 200.0, 0.15, 0.9, 0.1)
    notes = ['D4', 'F#4', 'A4', 'D5', 'F#5']
    plucks = np.zeros((2, 1))
    for i, nm in enumerate(notes):
        p = kit.twang(kit.note(nm), 2.4, child(r, 'p', i), 0.65, 2.2)
        plucks = layers(plucks, (S.pan(p, -0.5 + 0.25 * i), t0 + 0.065 * i, -2.0 - 0.5 * i))
    c_hi = kit.chime(kit.note('D6'), child(r, 'c1'), 2.2, 0.8)
    c_top = kit.chime(kit.note('A6'), child(r, 'c2'), 1.6, 0.7)
    sh = np.zeros(1)
    sr = child(r, 'shimmer')
    for i in range(10):
        f = kit.note(['D7', 'A6', 'F#7', 'E7'][i % 4]) * (1 + 0.003 * sr.standard_normal())
        sh = layers(sh, (kit.chime(f, sr, 0.8, 0.5, 0.00008), t0 + 0.3 + 0.09 * i + 0.03 * sr.random(), -6.0 - i))
    core = layers((hit, t0, -3.0), (air, t0, -12.0), (c_hi, t0 + 0.3, -5.0), (c_top, t0 + 0.34, -11.0))
    mix = layers(S.widen(rise, child(r, 'w1'), 0.5), (S.widen(core, child(r, 'w2'), 0.2), 0.0, 0.0),
                 (plucks, 0.0, 0.0), (S.widen(normalize(sh), child(r, 'w3'), 0.7), 0.0, -18.0))
    ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=2.2)
    wet = F.convolve(F.filt(mono(mix), F.hp(250.0), extend=False), ir)
    return layers(mix, kit.set_level(wet, mix, -14.0, 0.05))


def _sting(r, plucks, chimes, low=None, wet_db=-14.0, t60=1.6):
    """A short stinger: plucked notes [(note, time, dB, pan)], chimes [(note, time, dB, ring)], an optional low hit
    (time, dB), in a plate."""
    out = np.zeros((2, 1))
    for i, (nm, t, db, pan) in enumerate(plucks):
        out = layers(out, (S.pan(kit.twang(kit.note(nm), 1.8, child(r, 'p', i), 0.6, 1.8), pan), t, db))
    for i, (nm, t, db, ring) in enumerate(chimes):
        out = layers(out, (S.widen(kit.chime(kit.note(nm), child(r, 'c', i), ring, 0.8), child(r, 'cw', i), 0.2), t, db))
    if low is not None:
        hit = kit.thump(95.0, 45.0, 0.45, 0.25, attack=0.002, drive_db=2.0)
        out = layers(out, (S.widen(hit, child(r, 'lw'), 0.05), low[0], low[1]))
    ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=t60)
    wet = F.convolve(F.filt(mono(out), F.hp(250.0), extend=False), ir)
    return layers(out, kit.set_level(wet, out, wet_db, 0.05))


@cue('UI.MissionStep', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-9.0, align=True)
def mission_step(v, r):
    # Two notes up a fifth, the second capped by a soft chime: something done, the road goes on.
    a, b, c = [('A4', 'E5', 'E6'), ('G4', 'D5', 'D6')][v]
    return _sting(r, [(a, 0.0, 0.0, -0.25), (b, 0.11, 0.0, 0.25)], [(c, 0.115, -9.0, 1.4)], None, -15.0, 1.4)


@cue('UI.MissionComplete', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-5.0)
def mission_complete(v, r):
    # A phrase that resolves: D, A, D climbing, then the whole chord strummed with a drum under it and chimes over.
    plucks = [('D4', 0.0, -1.0, -0.4), ('A4', 0.11, -1.0, 0.0), ('D5', 0.22, -1.0, 0.4)]
    strum = ['D3', 'A3', 'D4', 'F#4', 'A4', 'D5']
    for i, nm in enumerate(strum):
        plucks.append((nm, 0.4 + 0.018 * i, -3.0, -0.5 + 0.2 * i))
    chimes = [('D6', 0.42, -6.0, 2.0), ('A6', 0.46, -12.0, 1.6), ('F#6', 0.5, -14.0, 1.4)]
    x = _sting(r, plucks, chimes, (0.4, -2.0), -13.0, 2.2)
    drum = O.membrane(88.0, 0.8, child(r, 'drum'), 0.5, 0.15, 0.04)
    drum = F.filt(drum, F.lp(1800.0), extend=False)
    return layers(x, (S.widen(normalize(drum), child(r, 'dw'), 0.1), 0.4, -6.0))


@cue('UI.NotchMilestone', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-5.0)
def notch_milestone(v, r):
    # A knife cuts the notch into the stock (two hard strokes into wood), then the gun answers: a steel ring like a
    # struck anvil over a dark open fifth.
    def stroke(rr):
        cut = kit.scrape(0.075, rr, 2600.0, 1500.0, 6.0, 0.8, (1.0, 1.6, 2.4), 0.1, 0.5, 4000.0, -10.0)
        wood = kit.thunk(420.0, rr, 0.05, 0.002)
        return layers((cut, 0.0, 0.0), (wood, 0.065, -6.0))
    s1 = stroke(child(r, 's1'))
    s2 = stroke(child(r, 's2'))
    t = 0.34
    anvil = normalize(M.strike(M.bar(510.0, child(r, 'anvil'), 1.8, 0.5, 6, 0.006, 0.6), M.hammer(0.00008), 2.2))
    anvil = F.filt(anvil, F.peak(3000.0, -4.0, 1.0), extend=False)
    boom = kit.thump(75.0, 38.0, 0.8, 0.5, attack=0.002, drive_db=3.0)
    low = layers(kit.twang(kit.note('A1'), 2.6, child(r, 'l1'), 0.4, 2.6), (kit.twang(kit.note('E2'), 2.6, child(r, 'l2'), 0.4, 2.4), 0.01, -2.0))
    x = layers((S.pan(s1, -0.2), 0.0, -3.0), (S.pan(s2, 0.2), 0.14, -1.0), (S.widen(anvil, child(r, 'aw'), 0.25), t, -4.0),
               (S.widen(boom, child(r, 'bw'), 0.05), t, -4.0), (S.widen(normalize(low), child(r, 'lw'), 0.3), t + 0.01, -6.0))
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=2.0, lp=3000.0)
    wet = F.convolve(F.filt(mono(x), F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -15.0, 0.05))


@cue('UI.CurseLifted', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-6.0, swell=True)
def curse_lifted(v, r):
    # Something lets go: a dark whisper and a sour, beating drone with a rattle of chain, then a breath out into a
    # clear rising chime and a lift of air.
    dur = 1.05
    n = ns(dur)
    whisper = V.whisper(dur, [(0.0, 'h'), (0.3, 'a'), (0.7, 'u'), (1.0, 'h')], child(r, 'wh'), 0.85)
    whisper *= E.swell(dur, 0.55, 0.9)
    drone = O.sine(98.0, n=n) + 0.9 * O.sine(103.8, n=n) + 0.3 * O.sine(147.0, n=n)
    drone = F.filt(D.drive(normalize(drone), 6.0), F.lp(900.0), extend=False) * E.swell(dur, 0.7, 0.5)
    chain = kit.jingle(0.9, child(r, 'chain'), 9, 1600.0, 6000.0, 0.05, 0.7, 0.05)
    release = 0.95
    lift = kit.whoosh(0.7, child(r, 'lift'), 500.0, 7000.0, 1.0, 0.3, -1.0)
    exhale = V.whisper(0.6, [(0.0, 'a'), (0.6, 'h')], child(r, 'ex'), 1.0) * E.perc(0.6, 0.03, 0.5)
    arp = np.zeros((2, 1))
    for i, nm in enumerate(['C6', 'E6', 'G6', 'C7']):
        c = kit.chime(kit.note(nm), child(r, 'c', i), 1.8 - 0.2 * i, 0.8)
        arp = layers(arp, (S.pan(c, -0.4 + 0.27 * i), release + 0.06 * i, -2.0 - 2.0 * i))
    dark = layers((normalize(whisper), 0.0, -6.0), (normalize(drone), 0.0, -8.0), (chain, 0.15, -16.0))
    light = layers((S.widen(lift, child(r, 'lw'), 0.6), release - 0.12, -10.0), (S.widen(normalize(exhale), child(r, 'ew'), 0.3), release - 0.05, -12.0),
                   (arp, 0.0, 0.0))
    x = layers(S.widen(dark, child(r, 'dw'), 0.4), light)
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=2.4, lp=4000.0)
    wet = F.convolve(F.filt(mono(x), F.hp(250.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -13.0, 0.05))
