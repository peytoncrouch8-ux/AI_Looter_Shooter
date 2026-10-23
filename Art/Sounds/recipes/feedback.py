"""The player's own feedback (2D): the hit marker's tick, a critical hit's ping, a ward's glassy tick and its break
(2026-10-22), the kill and the crit kill,
the pings for a mission step and a mission's end, and the stingers for a level, a gun's notch milestone and a lifted
curse.

The voice is brass and steel string: small brass ticks and chimes, plucked strings on a wooden box. Hits are short and
crisp, kept off the ear's most sensitive band so a long firefight doesn't tire it; the mission pings are small, bright
and sweet, so they can come often; the kill and the stingers ring.
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


def _tick(r):
    """The hit marker's crisp brass tick: its note under 2 kHz, the click's air above 5 kHz, little in between."""
    tick = kit.brass_tick(child(r, 'tick'), 1650.0 * jitter(r, 1, 0.025), 0.028, 4, 0.00007, -10.0, -5.0, 0.2)
    tick = F.filt(tick, F.peak(3200.0, -5.0, 0.9), extend=False)
    air = kit.burst(0.006, child(r, 'air'), 0.0025, lo=6000.0, hi=12000.0)
    knock = kit.thump(300.0, 220.0, 0.02, 0.012, attack=0.0003)
    return layers((tick, 0.0, 0.0), (air, 0.0, -13.0), (knock, 0.0, -13.0))


# The gun feel pass (2026-10-22): the body tick a little softer (level -11 -> -13), so a crit's own ping, a ward's glass
# and a kill stand out from it; the game plays the crit cue instead of the pitched-up tick.
@cue('UI.HitMarker', variations=3, space='2D', cls='Interface', jitter=0.02, conc=6, level=-13.0)
def hit_marker(v, r):
    # A crisp brass tick, so a hundred of them in a fight stay pleasant.
    return _space(_tick(r), r, 'room', -22.0, 0.15, 0.2)


@cue('UI.HitMarker.Crit', variations=3, space='2D', cls='Interface', jitter=0.01, conc=4, level=-11.0)
def hit_marker_crit(v, r):
    # A critical hit: the tick with a bright two-tone ping on it, a brass bell's quick rising pair (its notes kept under
    # 2 kHz, its shine above 5), and a hard little knock under, 2 dB over the body tick. The revolver's and the sniper's
    # crit ring (Weapon.Revolver.CritRing) plays over it.
    lo, hi = [('E6', 'B6'), ('D#6', 'A#6'), ('D6', 'A6')][v]
    p1 = kit.chime(kit.note(lo), child(r, 'p1'), 0.16, 0.45, 0.00006, 0.4, -20.0)
    p2 = kit.chime(kit.note(hi), child(r, 'p2'), 0.24, 0.45, 0.00006, 0.4, -20.0)
    sparkle = kit.metal_click(child(r, 'sparkle'), 6500.0, 11000.0, 0.03, 0.00003, 4, -12.0)
    knock = kit.thump(420.0, 260.0, 0.025, 0.012, attack=0.0003)
    x = layers((_tick(r), 0.0, -3.0), (p1, 0.0, -2.0), (p2, 0.045, 0.0), (normalize(sparkle), 0.045, -12.0),
               (knock, 0.0, -10.0))
    x = F.filt(x, F.peak(3300.0, -6.0, 0.8), extend=False)
    return _space(x, r, 'room', -20.0, 0.25, 0.3)


@cue('UI.HitMarker.Ward', variations=3, space='2D', cls='Interface', jitter=0.03, conc=6, level=-14.0)
def hit_marker_ward(v, r):
    # A round spent on a Warded enemy's ward: a glassy tick (a thin pane's high, inharmonic modes, struck hard and
    # damped at once) and a faint cold shimmer behind it, no brass knock, so it reads as "not through yet".
    glass = M.strike(M.plate(5200.0 * jitter(r, 1.0, 0.04), child(r, 'glass'), 10, 0.05, 0.5, 1.41, 0.03),
                     M.hammer(0.00003), 0.12)
    glass = F.filt(glass, F.hp(1500.0), F.peak(3300.0, -8.0, 0.7), extend=False)
    ping = kit.chime(kit.note(['G6', 'A6', 'F#6'][v]), child(r, 'ping'), 0.2, 0.3, 0.00005, 1.2, -24.0)
    n = ns(0.25)
    shimmer = N.band(n, child(r, 'shimmer'), 6000.0, 12000.0) * E.end_fade(E.perc(0.25, 0.01, 0.15))
    x = layers((normalize(glass), 0.0, 0.0), (ping, 0.0, -8.0), (normalize(shimmer), 0.005, -22.0))
    return _space(x, r, 'plate', -18.0, 0.4, 0.35)


@cue('UI.WardBreak', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-7.0)
def ward_break(v, r):
    # A ward shattering: a sharp crack (a pane giving all at once, a low knock of force under it), shards tinkling away
    # across the ears, and a falling shimmer as the light goes out of it.
    crack = layers(kit.burst(0.02, child(r, 'crack'), 0.008, lo=1200.0, hi=10000.0),
                   (M.strike(M.plate(2400.0, child(r, 'pane'), 18, 0.12, 0.5, 1.37, 0.03), M.hammer(0.00004), 0.3), 0.0,
                    -4.0))
    crack = F.filt(normalize(crack), F.peak(3300.0, -6.0, 0.8), extend=False)
    boom = kit.thump(120.0, 50.0, 0.25, 0.12, attack=0.0005, drive_db=3.0)
    shards = np.zeros((2, 1))
    sr = child(r, 'shards')
    for i in range(14):
        at = 0.02 + 0.45 * sr.random() ** 1.6
        g = M.strike(M.parts(4500.0, 11000.0, child(sr, 'g', i), 4, 0.06, 0.03, 0.2), M.hammer(0.00003), 0.1)
        shards = layers(shards, (S.pan(normalize(g), sr.uniform(-0.9, 0.9)), at, sr.uniform(-14.0, -4.0) - 8.0 * at))
    dur = 0.9
    n = ns(dur)
    t = times(n)
    f = 2600.0 * np.exp(-t / 0.5) + 900.0
    fall = np.sin(2.0 * np.pi * np.cumsum(f) / SR) + 0.5 * np.sin(2.0 * np.pi * np.cumsum(f * 1.5) / SR)
    fall = fall * E.end_fade(E.swell(dur, 0.04, 0.6))
    x = layers((S.widen(crack, child(r, 'cw'), 0.3), 0.0, 0.0), (S.widen(boom, child(r, 'bw'), 0.05), 0.0, -4.0),
               (normalize(shards), 0.0, -8.0), (S.widen(normalize(fall), child(r, 'fw'), 0.5), 0.02, -20.0))
    ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=0.9)
    wet = F.convolve(F.filt(mono(x), F.hp(400.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -16.0, 0.05))


def _kill(v, r, crit):
    """The bounty paid: a deep punch with a low thump under it (2026-10-22: the kill louder, -12.5 -> about -8 dB, and
    heavier), two brass chimes climbing (ka-ching), a sparkle of small ticks; a crit kill adds a bright third chime on
    top, a longer ring and a deeper boom."""
    punch = kit.thump(115.0, 44.0, 0.28, 0.15, attack=0.0006, drive_db=4.0)
    low = kit.thump(68.0 if not crit else 58.0, 32.0, 0.45, 0.24, attack=0.0015, drive_db=3.0)
    knock = kit.noise_thump(0.12, child(r, 'knock'), 2000.0, 220.0, 0.06, 1.0, 0.03)
    tick = kit.brass_tick(child(r, 'tick'), 1500.0, 0.03, 4, 0.00007, -10.0, -6.0, 0.2)
    first, second, third = [('E5', 'A5', 'E6'), ('D5', 'A5', 'D6'), ('G5', 'C6', 'G6')][v]
    c1 = kit.chime(kit.note(first), child(r, 'c1'), 0.9, 0.8)
    c2 = kit.chime(kit.note(second), child(r, 'c2'), 1.3, 0.85)
    sparkle = np.zeros((2, 1))
    sr = child(r, 'sparkle')
    for i in range(7 if not crit else 11):
        t = 0.06 + (0.32 if not crit else 0.5) * sr.random() ** 1.4
        s = kit.metal_click(sr, 4200.0, 9500.0, 0.03, 0.00004, 3, -20.0)
        sparkle = layers(sparkle, (S.pan(s, sr.uniform(-0.8, 0.8)), t, sr.uniform(-10.0, -2.0)))
    body = [(punch, 0.0, -2.0), (low, 0.0, -3.0 if not crit else -1.0), (knock, 0.0, -10.0), (tick, 0.0, -6.0),
            (c1, 0.0, -5.0), (c2, 0.075, -3.0)]
    if crit:
        c3 = kit.chime(kit.note(third), child(r, 'c3'), 1.7, 0.7)
        body.append((c3, 0.15, -4.0))
    return layers(_space(layers(*body), r, 'plate', -15.0, 1.1 if not crit else 1.5, 0.3), (normalize(sparkle), 0.0, -22.0))


@cue('UI.Kill', variations=3, space='2D', cls='Interface', jitter=0.0, conc=3, level=-2.0)
def kill(v, r):
    return _kill(v, r, False)


@cue('UI.Kill.Crit', variations=3, space='2D', cls='Interface', jitter=0.0, conc=3, level=-1.0)
def kill_crit(v, r):
    # A kill by a critical hit: the kill's ka-ching climbing a third chime higher, a deeper boom, a longer sparkle.
    return _kill(v, r, True)


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


def _ping(name, r, ring=0.4, bright=0.8, body_db=-9.0):
    """One bright ping: a small brass chime struck with a hard little mallet, and a soft sine on its note under it for
    a round body, so it sings a clear, sweet note rather than a click."""
    f0 = kit.note(name)
    c = kit.chime(f0, child(r, 'chime'), ring, bright, 0.0001, 0.5, -26.0)
    body = O.sine(f0, n=ns(ring)) * E.perc(ring, 0.003, ring * 0.7)
    return normalize(layers(c, (normalize(body), 0.0, body_db)), 0.0)


def _pings(r, notes, strings=(), wet_db=-16.0, t60=0.5):
    """Pings [(note, time, dB, pan, ring)] and soft steel strings under them [(note, time, dB, pan, ring)], in a small
    bright plate."""
    out = np.zeros((2, 1))
    for i, (nm, t, db, pan, ring) in enumerate(notes):
        out = layers(out, (S.pan(_ping(nm, child(r, 'ping', i), ring), pan), t, db))
    for i, (nm, t, db, pan, ring) in enumerate(strings):
        # Plucked softly (bright 0.35) so the string adds warmth under the chime, not a twang of its own.
        s = kit.twang(kit.note(nm), ring * 1.4, child(r, 'string', i), 0.35, ring)
        out = layers(out, (S.pan(F.filt(s, F.lp(2200.0, 0.7), extend=False), pan), t, db))
    ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=t60)
    wet = F.convolve(F.filt(mono(out), F.hp(300.0), extend=False), ir)
    return layers(out, kit.set_level(wet, out, wet_db, 0.05))


@cue('UI.MissionStep', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-10.0)
def mission_step(v, r):
    # An objective done: two quick bright pings up a fifth, the second left ringing a moment. Small and sweet, so it
    # can come often without nagging.
    a, b = [('C6', 'G6'), ('D6', 'A6')][v]
    return _pings(r, [(a, 0.0, -2.0, -0.2, 0.28), (b, 0.085, 0.0, 0.2, 0.4)], (), -17.0, 0.45)


@cue('UI.MissionComplete', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-6.0)
def mission_complete(v, r):
    # A mission done: the step's ping grown into a three-note flourish up a major chord, each note warmed by a soft
    # steel string an octave down, the last one ringing over a low root with a faint octave shimmering above it, the
    # first two still sounding under it so it lands on the whole chord.
    pings = [('C6', 0.0, -3.0, -0.3, 0.45), ('E6', 0.09, -2.0, 0.0, 0.55), ('G6', 0.18, 0.0, 0.3, 0.85),
             ('C7', 0.2, -17.0, 0.1, 0.6)]
    strings = [('C5', 0.0, -13.0, -0.3, 0.5), ('E5', 0.09, -13.0, 0.0, 0.5), ('G5', 0.18, -11.0, 0.3, 1.0),
               ('C4', 0.18, -10.0, 0.0, 1.1)]
    return _pings(r, pings, strings, -15.0, 0.9)


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
