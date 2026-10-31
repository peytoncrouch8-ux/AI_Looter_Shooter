"""The score: original weird-west music, played by the band (band.py) and mixed to sit under the game.

Every piece is a seamless loop that the music director (Audio/MusicDirectorSubsystem) crossfades on bar lines, so the
pieces that layer share a tempo and a key: the exploration themes and the combat layer are in 6/8 at 75 dotted
quarters a minute (a bar is 1.6 s) and in D, Skyreach's open and major (Mixolydian), Ransom's Rest's minor (Dorian, its
dominant borrowed major); the combat layer is built on a D pedal so it fits under either. The bosses have their own
tempos in 4/4 and play the orchestra (score_beasts.py, score_men.py; since 2026-10-31). Bars per piece are the director's (MusicRules::Pieces): change both together.

    Music.Skyreach.Explore     32 bars: fingerpicked guitar, a whistled tune, a bowed drone, a cello line in the middle
    Music.RansomsRest.Explore  32 bars: fingerpicked guitar in drop D, a harmonica-like reed, a far bell, low strings
    Music.Combat               16 bars: galloping toms and hooves, a big drum, a spiccato bass ostinato, tremolo strings
    Music.Sting.*              one-shots: a high-rank creature appears, a boss's phase turns, a fight is won

The music sits under the effects: the master bus clears the sub-bass and dips the 2-5 kHz band, and each cue's level
is set well under the rifle's.
"""
import numpy as np

from lib.core import SR, ns, times, normalize, child, db2a, fit, layers
from lib import noise as N, env as E, osc as O, filters as F, dist as D, stereo as S
from recipes import cue
from recipes import band as B

# The 6/8 pieces: the dotted quarter at 75 (a bar is 1.6 s).
SIX_EIGHT = B.Meter(75.0, 2, 3)
EXPLORE_BARS = 32
COMBAT_BARS = 16


def _reverb(r, t60=2.4):
    return B.hall(child(r, 'hall'), t60=t60)


# --- Guitar shapes (drop D: D2 A2 D3 G3 B3 E4 open), low string first; None: not played ---

SHAPES = {
    'Dm': ['D2', 'A2', 'D3', 'A3', 'D4', 'F4'],
    'D': ['D2', 'A2', 'D3', 'A3', 'D4', 'F#4'],
    'Dsus2': ['D2', 'A2', 'D3', 'A3', 'D4', 'E4'],
    'C': [None, 'C3', 'E3', 'G3', 'C4', 'E4'],
    'Bb': [None, 'Bb2', 'F3', 'Bb3', 'D4', 'F4'],
    'F': ['F2', 'C3', 'F3', 'A3', 'C4', 'F4'],
    'A': [None, 'A2', 'E3', 'A3', 'C#4', 'E4'],
    'Asus4': [None, 'A2', 'E3', 'A3', 'D4', 'E4'],
    'Gm': ['G2', 'D3', 'G3', 'Bb3', 'D4', 'G4'],
    'G': ['G2', 'B2', 'D3', 'G3', 'B3', 'G4'],
    'G/D': ['D2', 'B2', 'D3', 'G3', 'B3', 'G4'],
    'Bm': [None, 'B2', 'F#3', 'B3', 'D4', 'F#4'],
    'Em': ['E2', 'B2', 'E3', 'G3', 'B3', 'E4'],
    'D/F#': ['F#2', 'A2', 'D3', 'A3', 'D4', 'F#4'],
}

# Fingerpicking patterns over a 6/8 bar: (eighth, string or 'bass'/'alt', velocity). The thumb keeps the bass on the
# beats, the fingers fill between.
SPARSE = [(0, 'bass', 0.52), (2, 4, 0.4), (4, 5, 0.44)]
FULL = [(0, 'bass', 0.55), (1, 3, 0.4), (2, 4, 0.46), (3, 'alt', 0.44), (4, 5, 0.5), (5, 4, 0.38)]
ROLL = [(0, 'bass', 0.54), (1, 3, 0.38), (2, 4, 0.42), (3, 5, 0.5), (4, 4, 0.4), (5, 3, 0.36)]


def _pick_bar(gtr, m, bar, shape, pattern, scale=1.0):
    notes = SHAPES[shape]
    bass = next(i for i, x in enumerate(notes) if x)
    # The thumb's other note: the octave or fifth two strings up.
    alt = bass + 2 if notes[bass + 2] else bass + 1
    for step, s, v in pattern:
        string = bass if s == 'bass' else (alt if s == 'alt' else s)
        if notes[string] is None:
            continue
        gtr.pick(m.at(bar, step), string, notes[string], v * scale)


def _strum_bar(gtr, m, bar, shape, vel=0.5, ups=True):
    notes = [x for x in SHAPES[shape] if x]
    gtr.strum(m.at(bar, 0), notes, vel, True, 0.045)
    if ups:
        gtr.strum(m.at(bar, 2), notes[-3:], vel * 0.55, False, 0.02)
    gtr.strum(m.at(bar, 3), notes, vel * 0.8, True, 0.035)
    if ups:
        gtr.strum(m.at(bar, 5), notes[-3:], vel * 0.5, False, 0.02)


def _phrase(m, notes):
    """A Phrase from (bar, eighth, length in eighths, note, options)."""
    p = B.Phrase()
    for bar, step, length, note, *opts in notes:
        p.note(m.at(bar, step), length * m.step, note, **(opts[0] if opts else {}))
    return p


# --- Ransom's Rest ---------------------------------------------------------------------------------------------------

RR_CHORDS = (['Dm', 'Dm', 'C', 'Dm', 'Bb', 'F', 'C', 'A'] * 2 +
             ['Bb', 'F', 'C', 'Dm', 'Bb', 'F', 'Gm', 'A'] +
             ['Dm', 'Dm', 'C', 'Dm', 'Bb', 'F', 'C', 'A'])

# The reed's tune, in breaths: the lonesome call (bars 9-16), its return climbing and settling (bars 25-31).
RR_REED = [
    [(8, 0, 3, 'D5', {'scoop': 1.0}), (8, 3, 1, 'C5'), (8, 4, 2, 'A4'), (9, 0, 1, 'G4'), (9, 1, 4, 'A4', {'vib': 0.2})],
    [(10, 0, 2, 'G4'), (10, 2, 1, 'A4'), (10, 3, 3, 'C5'), (11, 0, 2, 'D5'), (11, 2, 3, 'A4', {'bend': 0.25})],
    [(12, 0, 3, 'F5', {'scoop': 1.0}), (12, 3, 2, 'D5'), (12, 5, 1, 'C5'), (13, 0, 1, 'C5'), (13, 1, 4, 'A4')],
    [(14, 0, 2, 'G4'), (14, 2, 1, 'A4'), (14, 3, 2, 'C5'), (14, 5, 1, 'E5'), (15, 0, 5, 'C#5', {'vib': 0.22})],
    [(24, 0, 3, 'D5', {'scoop': 1.0}), (24, 3, 1, 'E5'), (24, 4, 2, 'F5'), (25, 0, 1, 'E5'), (25, 1, 4, 'D5')],
    [(26, 0, 2, 'C5'), (26, 2, 1, 'D5'), (26, 3, 3, 'E5'), (27, 0, 2, 'D5'), (27, 2, 3, 'A4', {'bend': 0.2})],
    [(28, 0, 2, 'F5'), (28, 2, 1, 'E5'), (28, 3, 3, 'D5'), (29, 0, 2, 'C5'), (29, 2, 4, 'A4')],
    [(30, 0, 3, 'G4'), (30, 3, 3, 'E4', {'fall': 1.5, 'vib': 0.12})],
]

# The cello sings the middle (bars 17-24).
RR_CELLO = [
    [(16, 0, 3, 'F3'), (16, 3, 3, 'Bb3'), (17, 0, 4, 'A3'), (17, 4, 1, 'G3'), (17, 5, 1, 'F3'),
     (18, 0, 3, 'E3'), (18, 3, 3, 'G3'), (19, 0, 2, 'F3'), (19, 2, 1, 'E3'), (19, 3, 3, 'D3')],
    [(20, 0, 3, 'D4'), (20, 3, 2, 'C4'), (20, 5, 1, 'Bb3'), (21, 0, 6, 'A3'),
     (22, 0, 3, 'Bb3'), (22, 3, 2, 'A3'), (22, 5, 1, 'G3'), (23, 0, 3, 'A3'), (23, 3, 3, 'C#4')],
]

RR_BASS = ['Bb1', 'F2', 'C2', 'D2', 'Bb1', 'F2', 'G1', 'A1']


def _bowed_line(desk, track, m, r, notes, vel=0.7, **kw):
    """A legato bowed line from phrase notes: one bow's pitch curve and swell through _pitch_line."""
    p = _phrase(m, notes)
    t0, t1 = p.span()
    t0 -= 0.1
    n = ns(t1 - t0 + 0.6)
    f0, amp = B._pitch_line(p.notes, n, t0, glide=0.06, vib_rate=5.2, vib_delay=0.25, r=r)
    env = np.convolve(amp, np.ones(ns(0.05)) / ns(0.05), mode='same')
    x = B.bowed(f0, n / SR, r, vel, env=env, **kw)
    desk.add(track, x, t0)


@cue('Music.RansomsRest.Explore', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-17.0,
     align=False)
def ransoms_rest_explore(v, r):
    # "Dust to Dust": the Rest's own tune. A lone guitar in drop D, its low D ringing under everything; a far bell; a
    # harmonica-like reed calling across the canyon with space between its phrases; the cello taking the middle over
    # a low bowed bass; the call coming back, climbing, and settling on the guitar alone before it starts again.
    m = SIX_EIGHT
    desk = B.Desk(EXPLORE_BARS * m.bar)
    desk.track('guitar', 0.0, -0.18, -17.0, B.guitar_body, 1.0)
    desk.track('reed', -1.5, 0.15, -9.0)
    desk.track('cello', -3.0, 0.25, -11.0, B.cello_body)
    desk.track('drone', -10.0, 0.0, -12.0, B.cello_body)
    desk.track('bass', -8.0, 0.0, -14.0, B.cello_body)
    desk.track('bell', -15.0, -0.35, -3.0, lambda x, c: F.filt(x, F.hp(110.0), extend=False, circular=c))
    desk.track('ghost', -13.0, 0.0, -5.0, B.cello_body)
    desk.track('heart', -16.0, 0.0, -16.0)

    gtr = B.Guitar(child(r, 'guitar'), bright=0.42)
    for bar, shape in enumerate(RR_CHORDS):
        if bar < 8:
            _pick_bar(gtr, m, bar, shape, SPARSE)
        elif bar < 16:
            _pick_bar(gtr, m, bar, shape, FULL, 0.92)
        elif bar < 24:
            _strum_bar(gtr, m, bar, shape, 0.38, ups=bar % 2 == 1)
        elif bar < 31:
            _pick_bar(gtr, m, bar, shape, ROLL if bar % 2 else FULL, 0.95)
        else:
            _pick_bar(gtr, m, bar, shape, SPARSE, 0.85)
    gtr.render(desk, 'guitar')

    rr = child(r, 'reed')
    for i, notes in enumerate(RR_REED):
        x, t0 = B.reed(_phrase(m, notes), child(rr, 'p', i))
        desk.add('reed', x, t0)

    rc = child(r, 'cello')
    for i, notes in enumerate(RR_CELLO):
        _bowed_line(desk, 'cello', m, child(rc, 'p', i), notes, 0.8, bright=0.55, kmax=40)

    rd = child(r, 'drone')
    for i, (bar, length, notes) in enumerate([(0, 8, ['D2']), (8, 8, ['D2', 'A2']), (24, 8, ['D2', 'A2'])]):
        for j, note in enumerate(notes):
            x = B.bowed(note, length * m.bar + 1.5, child(rd, i, j), 0.8 if j == 0 else 0.55, attack=3.0, release=3.5,
                        vib=0.05, bright=0.3, kmax=36)
            desk.add('drone', x, m.at(bar) - 0.4)
    rb = child(r, 'bass')
    for i, note in enumerate(RR_BASS):
        x = B.bowed(note, m.bar + 0.5, child(rb, i), 0.8, attack=0.35, release=0.6, vib=0.06, bright=0.35, kmax=30)
        desk.add('bass', x, m.at(16 + i) - 0.05)

    for bar in (0, 24):
        desk.add('bell', B.bell('D4', child(r, 'bell', bar), 0.8, t60=8.0, dur=8.0, bright=0.7), m.at(bar))
    desk.add('bell', B.bell('A3', child(r, 'bell', 16), 0.45, t60=7.0, dur=7.0, bright=0.6), m.at(16, 3))

    rg = child(r, 'ghost')
    gl = ns(8 * m.bar + 3.0) / SR
    for j, note in enumerate(['A4', 'D5']):
        x = B.ensemble(note, gl, child(rg, j), 0.5, voices=3, attack=4.0, release=3.5, bright=0.2, vib=0.08,
                       width=0.9, kmax=16)
        desk.add('ghost', x, m.at(24) - 0.5)

    rh = child(r, 'heart')
    for bar in range(16, 24):
        desk.add('heart', B.frame_drum(child(rh, bar, 0), 0.5), m.at(bar, 0) + B.human(rh))
        desk.add('heart', B.frame_drum(child(rh, bar, 1), 0.3), m.at(bar, 3) + B.human(rh))

    return desk.mix(_reverb(r), B.master_bus)


# --- Skyreach --------------------------------------------------------------------------------------------------------

SKY_CHORDS = (['D', 'G/D', 'D', 'A', 'D', 'G/D', 'C', 'A'] * 2 +
              ['G', 'D/F#', 'Em', 'A', 'G', 'D/F#', 'C', 'Asus4'] +
              ['D', 'G/D', 'D', 'A', 'D', 'G/D', 'C', 'A'])

# The whistled tune, in breaths: the open call (bars 9-16) and its return reaching up before it comes home (25-31).
SKY_WHISTLE = [
    [(8, 0, 3, 'A5', {'scoop': 0.6}), (8, 3, 2, 'F#5'), (8, 5, 1, 'E5'), (9, 0, 1, 'D5'), (9, 1, 2, 'E5'),
     (9, 3, 3, 'G5'), (10, 0, 5, 'F#5', {'vib': 0.25})],
    [(11, 0, 2, 'E5'), (11, 2, 1, 'D5'), (11, 3, 3, 'C#5')],
    [(12, 0, 2, 'A5'), (12, 2, 1, 'B5'), (12, 3, 2, 'A5'), (12, 5, 1, 'F#5'), (13, 0, 3, 'G5'), (13, 3, 3, 'B5')],
    [(14, 0, 2, 'A5'), (14, 2, 1, 'G5'), (14, 3, 3, 'E5'), (15, 0, 5, 'E5', {'vib': 0.25})],
    [(24, 0, 3, 'A5', {'scoop': 0.6}), (24, 3, 2, 'F#5'), (24, 5, 1, 'E5'), (25, 0, 1, 'D5'), (25, 1, 2, 'E5'),
     (25, 3, 3, 'G5'), (26, 0, 2, 'A5'), (26, 2, 4, 'D6', {'vib': 0.28})],
    [(27, 0, 3, 'C#6'), (27, 3, 2, 'B5'), (27, 5, 1, 'A5'), (28, 0, 6, 'F#5', {'vib': 0.25})],
    [(29, 0, 2, 'G5'), (29, 2, 1, 'F#5'), (29, 3, 3, 'E5'), (30, 0, 2, 'E5'), (30, 2, 4, 'D5', {'fall': 0.6})],
]

SKY_CELLO = [
    [(16, 0, 3, 'B3'), (16, 3, 3, 'D4'), (17, 0, 6, 'A3'), (18, 0, 3, 'G3'), (18, 3, 3, 'B3'), (19, 0, 3, 'A3'),
     (19, 3, 3, 'C#4')],
    [(20, 0, 3, 'D4'), (20, 3, 2, 'B3'), (20, 5, 1, 'G3'), (21, 0, 6, 'F#3'), (22, 0, 3, 'E3'), (22, 3, 3, 'G3'),
     (23, 0, 6, 'A3')],
]

SKY_BASS = ['G2', 'F#2', 'E2', 'A2', 'G2', 'F#2', 'C2', 'A1']


@cue('Music.Skyreach.Explore', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-17.0,
     align=False)
def skyreach_explore(v, r):
    # "Open Sky": the island's tune, in the same tempo and key as the Rest's but open and major. Bright fingerpicking
    # in drop D under a soft open-fifth drone; a whistled tune with room around it; the cello's line in the middle over
    # a bowed bass, a shaker keeping the walk; the tune again, reaching up to the high D, with a shimmer of high strings
    # and small brass chimes; home on the guitar alone.
    m = SIX_EIGHT
    desk = B.Desk(EXPLORE_BARS * m.bar)
    desk.track('guitar', 0.0, -0.2, -16.0, B.guitar_body, 1.0)
    desk.track('whistle', -2.0, 0.18, -8.0)
    desk.track('cello', -3.5, 0.25, -11.0, B.cello_body)
    desk.track('drone', -11.0, 0.0, -12.0, B.cello_body)
    desk.track('bass', -8.5, 0.0, -14.0, B.cello_body)
    desk.track('air', -14.0, 0.0, -5.0, B.cello_body)
    desk.track('chime', -16.0, 0.3, -6.0, lambda x, c: F.filt(x, F.highshelf(6000.0, -6.0), extend=False, circular=c))
    desk.track('shaker', -19.0, 0.3, -18.0)
    desk.track('heart', -17.0, 0.0, -16.0)

    gtr = B.Guitar(child(r, 'guitar'), bright=0.55)
    for bar, shape in enumerate(SKY_CHORDS):
        if bar < 8:
            _pick_bar(gtr, m, bar, shape, SPARSE)
        elif bar < 16:
            _pick_bar(gtr, m, bar, shape, FULL, 0.92)
        elif bar < 24:
            _pick_bar(gtr, m, bar, shape, ROLL, 0.9)
        elif bar < 31:
            _pick_bar(gtr, m, bar, shape, FULL if bar % 2 == 0 else ROLL, 0.95)
        else:
            _pick_bar(gtr, m, bar, shape, SPARSE, 0.85)
    gtr.render(desk, 'guitar')

    rw = child(r, 'whistle')
    for i, notes in enumerate(SKY_WHISTLE):
        x, t0 = B.whistle(_phrase(m, notes), child(rw, 'p', i))
        desk.add('whistle', x, t0)

    rc = child(r, 'cello')
    for i, notes in enumerate(SKY_CELLO):
        _bowed_line(desk, 'cello', m, child(rc, 'p', i), notes, 0.8, bright=0.55, kmax=40)

    rd = child(r, 'drone')
    for i, (bar, length) in enumerate([(0, 8), (8, 8), (24, 8)]):
        for j, note in enumerate(['D2', 'A2']):
            x = B.bowed(note, length * m.bar + 1.5, child(rd, i, j), 0.8 if j == 0 else 0.6, attack=3.0, release=3.5,
                        vib=0.05, bright=0.35, kmax=36)
            desk.add('drone', x, m.at(bar) - 0.4)
    rb = child(r, 'bass')
    for i, note in enumerate(SKY_BASS):
        x = B.bowed(note, m.bar + 0.5, child(rb, i), 0.8, attack=0.35, release=0.6, vib=0.06, bright=0.4, kmax=30)
        desk.add('bass', x, m.at(16 + i) - 0.05)

    ra = child(r, 'air')
    al = ns(8 * m.bar + 3.0) / SR
    for j, note in enumerate(['D5', 'F#5', 'A5']):
        x = B.ensemble(note, al, child(ra, j), 0.5, voices=3, attack=4.0, release=3.5, bright=0.25, vib=0.08,
                       width=0.9, kmax=14)
        desk.add('air', x, m.at(24) - 0.5)

    rch = child(r, 'chime')
    for k, (bar, note) in enumerate([(24, 'D6'), (26, 'A5'), (28, 'F#6'), (30, 'E6'), (8, 'A5'), (12, 'D6')]):
        x = B.kit.chime(B.hz(note), child(rch, k), t60=2.4, bright=0.7)
        desk.add('chime', x, m.at(bar, 0) + B.human(rch))

    rs = child(r, 'shaker')
    for bar in range(16, 24):
        for step in range(6):
            vel = [0.6, 0.32, 0.42, 0.55, 0.32, 0.42][step]
            desk.add('shaker', B.shaker(child(rs, bar, step), B.lively(rs, vel)), m.at(bar, step) + B.human(rs, 0.004))
        desk.add('heart', B.frame_drum(child(rs, 'f', bar), 0.45), m.at(bar, 0) + B.human(rs))

    return desk.mix(_reverb(r, 2.6), B.master_bus)


# --- Combat ----------------------------------------------------------------------------------------------------------

def _combat_bass_note(bar, beat, step):
    """The ostinato's note: a D pedal galloping under everything, lifting to the fifth and seventh, and in the last
    bar of each half leaning on the flat second (the tension that turns it round)."""
    if bar % 8 == 7 and beat == 1:
        return ['D2', None, 'Eb2', 'Eb2', 'Eb2', None][step]
    if beat == 0:
        return ['D2', None, 'D2', 'D2', 'A2', None][step]
    return ['D2', None, 'D2', 'D2', 'C3' if bar % 2 else 'A2', None][step]


@cue('Music.Combat', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0, align=False)
def combat(v, r):
    # The fight: the exploration themes' tempo and key, built on a D pedal so it sits under either. A big drum on the
    # beats, toms galloping like riders (long-short-short), hooves clopping on wood in the second half, a shaker, a
    # spiccato bass ostinato galloping with them, and tremolo strings climbing a half step every two bars. It opens on
    # a hit, so the fight comes in with one.
    m = B.Meter(75.0, 2, 6)
    desk = B.Desk(COMBAT_BARS * m.bar)
    desk.track('big', 0.0, 0.0, -14.0)
    desk.track('toms', -3.0, 0.0, -15.0)
    desk.track('hooves', -11.0, 0.25, -14.0)
    desk.track('frame', -8.0, -0.15, -15.0)
    desk.track('shaker', -15.0, 0.3, -20.0)
    desk.track('bass', -4.0, 0.0, -18.0, B.cello_body)
    desk.track('strings', -4.0, 0.0, -10.0, B.cello_body)
    desk.track('hit', -4.0, 0.0, -8.0, B.cello_body)
    rr = child(r, 'perc')
    for bar in range(COMBAT_BARS):
        full = bar >= 4
        # The big drum: on the downbeat, the second beat as it builds, a pickup into each half.
        desk.add('big', B.big_drum(child(rr, 'b', bar, 0), 1.0 if bar % 8 == 0 else 0.88),
                 m.at(bar, 0) + B.human(rr, 0.003))
        if bar % 4 in (2, 3) or bar >= 8:
            desk.add('big', B.big_drum(child(rr, 'b', bar, 6), 0.75), m.at(bar, 6) + B.human(rr, 0.003))
        if bar % 8 == 7:
            for k, s in enumerate((9, 10, 11)):
                desk.add('big', B.big_drum(child(rr, 'p', bar, s), 0.55 + 0.12 * k, f0=66.0),
                         m.at(bar, s) + B.human(rr, 0.003))
        for beat in range(2):
            base = beat * 6
            if full:
                steps = [(0, 'G2', 0.9), (2, 'C3', 0.55), (3, 'C3', 0.5), (4, 'G2', 0.7)]
            else:
                steps = [(0, 'G2', 0.8), (4, 'G2', 0.55)]
            for s, note, vel in steps:
                desk.add('toms', B.tom(note, child(rr, 't', bar, base + s), B.lively(rr, vel, 0.1)),
                         m.at(bar, base + s) + B.human(rr, 0.004), pan=-0.2 if note == 'G2' else 0.2)
        if full:
            desk.add('frame', B.frame_drum(child(rr, 'f', bar), B.lively(rr, 0.7)), m.at(bar, 6) + B.human(rr, 0.004))
            if bar >= 8:
                desk.add('frame', B.frame_drum(child(rr, 'g', bar), 0.3), m.at(bar, 9) + B.human(rr, 0.004))
                desk.add('frame', B.frame_drum(child(rr, 'h', bar), 0.25), m.at(bar, 11) + B.human(rr, 0.004))
            for s in range(12):
                vel = [0.6, 0.3, 0.42, 0.3, 0.5, 0.3][s % 6]
                desk.add('shaker', B.shaker(child(rr, 's', bar, s), B.lively(rr, vel)),
                         m.at(bar, s) + B.human(rr, 0.003))
        if bar >= 8:
            # Hooves on hard ground: clip-clop on every eighth, the beats leaning.
            for k, s in enumerate((0, 2, 4, 6, 8, 10)):
                f0 = 980.0 if k % 2 == 0 else 760.0
                vel = 0.7 if s in (0, 6) else 0.45
                desk.add('hooves', B.wood_block(child(rr, 'w', bar, s), B.lively(rr, vel), f0),
                         m.at(bar, s) + B.human(rr, 0.005))

    rb = child(r, 'bass')
    for bar in range(COMBAT_BARS):
        for beat in range(2):
            for s in range(6):
                note = _combat_bass_note(bar, beat, s)
                # It enters on the beats alone, then fills in.
                if note is None or (bar < 2 and s != 0) or (bar < 4 and s in (2, 3)):
                    continue
                vel = 0.95 if s == 0 else 0.7
                x = B.spiccato(note, child(rb, bar, beat, s), B.lively(rb, vel, 0.08), 0.2 if s in (0, 4) else 0.12)
                desk.add('bass', x, m.at(bar, beat * 6 + s) + B.human(rb, 0.004))

    rs = child(r, 'strings')
    # Tremolo: the low pair through the first half, the climbing top line through the second.
    lines = [(0, 8, 'D3', 0.55), (0, 6, 'A3', 0.45), (6, 2, 'Bb3', 0.5), (8, 8, 'D4', 0.6), (8, 2, 'A4', 0.5),
             (10, 2, 'Bb4', 0.55), (12, 2, 'B4', 0.6), (14, 2, 'C5', 0.68)]
    for k, (bar, length, note, vel) in enumerate(lines):
        dur = length * m.bar + 0.3
        n = ns(dur)
        rise = np.linspace(0.45, 1.0, n) ** 1.5
        env = fit(E.ar(dur, 0.5, 0.35, 2.0), n) * rise
        x = B.ensemble(note, dur, child(rs, k), vel, voices=4, tremolo=13.0, env=env, bright=0.6, width=0.8, kmax=24)
        desk.add('strings', x, m.at(bar) - 0.02)

    # The opening hit: the low strings bite on the downbeat with the big drum.
    rh = child(r, 'hit')
    for j, note in enumerate(['D2', 'A2', 'D3']):
        env = fit(E.perc(2.0, 0.01, 1.1), ns(2.0))
        desk.add('hit', B.ensemble(note, 2.0, child(rh, j), 0.9, voices=3, env=env, bright=0.7, kmax=30), m.at(0))
    return desk.mix(_reverb(r, 1.8), B.master_bus)


# --- Bosses ----------------------------------------------------------------------------------------------------------
# The boss themes moved to the orchestra on 2026-10-31 (score_beasts.py, score_men.py on orchestra.py); the band's first
# takes of Abel's and the Gravemother's are in git history (this file before that day).

def _hp_fx(freq):
    return lambda x, c: F.filt(x, F.hp(freq), extend=False, circular=c)


# --- Stingers --------------------------------------------------------------------------------------------------------

def _sting_bus(x, circular):
    return B.master_bus(x, circular, glue=False)


@cue('Music.Sting.Elite', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-12.0, align=False)
def sting_elite(v, r):
    # Something that has fed long on the dark has seen you: a low string bite and a big drum together, a dissonant
    # cluster over them, a high tremolo shiver dying away.
    desk = B.Desk(None, tail=4.0)
    desk.track('big', 0.0, 0.0, -10.0)
    desk.track('low', -1.0, 0.0, -9.0, B.cello_body)
    desk.track('high', -9.0, 0.0, -6.0, B.cello_body)
    desk.add('big', B.big_drum(child(r, 'b'), 1.0, f0=50.0, t60=1.2), 0.0)
    for j, note in enumerate(['D2', 'A2', 'Eb3']):
        env = fit(E.perc(2.6, 0.012, 1.6), ns(2.6))
        desk.add('low', B.ensemble(note, 2.6, child(r, 'l', j), 0.9, voices=3, env=env, bright=0.75, kmax=30), 0.0)
    for j, note in enumerate(['D5', 'Eb5', 'A5']):
        env = fit(E.perc(2.8, 0.05, 1.8), ns(2.8))
        desk.add('high', B.ensemble(note, 2.8, child(r, 'h', j), 0.5, voices=3, tremolo=16.0, env=env, bright=0.85,
                                    kmax=14), 0.02)
    return desk.mix(_reverb(r, 2.4), _sting_bus)


@cue('Music.Sting.Phase', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-11.0, align=False,
     swell=True)
def sting_phase(v, r):
    # A boss turns: a drum roll gathering under a rising swell, then everything on one hit (big drums, the bell, low
    # strings and an iron strike), the strings shivering after it.
    hit = 0.75
    desk = B.Desk(None, tail=4.5)
    desk.track('roll', -4.0, 0.0, -12.0)
    desk.track('swell', -9.0, 0.0, -8.0)
    desk.track('big', 0.0, 0.0, -10.0)
    desk.track('bell', -6.0, 0.0, -5.0, _hp_fx(100.0))
    desk.track('low', -2.0, 0.0, -9.0, B.cello_body)
    desk.track('iron', -12.0, 0.2, -6.0)
    desk.track('high', -9.0, 0.0, -6.0, B.cello_body)
    t = 0.0
    k = 0
    while t < hit - 0.03:
        desk.add('roll', B.tom('G2' if k % 2 else 'C3', child(r, 'roll', k), 0.35 + 0.6 * (t / hit)), t,
                 pan=-0.2 if k % 2 else 0.2)
        t += 0.11 * (1.0 - 0.55 * t / hit)
        k += 1
    desk.add('swell', B.swell(hit, child(r, 'sw'), 250.0, 8000.0), 0.0)
    desk.add('big', B.big_drum(child(r, 'b1'), 1.0, f0=50.0, t60=1.3), hit)
    desk.add('big', B.big_drum(child(r, 'b2'), 0.8, f0=68.0, t60=0.9), hit + 0.004)
    desk.add('bell', B.bell('D4', child(r, 'bell'), 0.9, t60=5.0, dur=5.0), hit)
    for j, note in enumerate(['D2', 'D3', 'A3']):
        env = fit(E.perc(3.0, 0.01, 1.8), ns(3.0))
        desk.add('low', B.ensemble(note, 3.0, child(r, 'l', j), 0.9, voices=3, env=env, bright=0.75, kmax=30), hit)
    desk.add('iron', B.anvil(child(r, 'iron'), 0.8), hit)
    for j, note in enumerate(['D5', 'F5', 'A5']):
        env = fit(E.ar(3.2, 0.3, 2.4, 2.0), ns(3.2))
        desk.add('high', B.ensemble(note, 3.2, child(r, 'h', j), 0.5, voices=3, tremolo=15.0, env=env, bright=0.8,
                                    kmax=14), hit + 0.05)
    return desk.mix(_reverb(r, 2.4), _sting_bus)


@cue('Music.Sting.Victory', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-13.0, align=False)
def sting_victory(v, r):
    # The fight is won: an open D major strummed and let ring, the reed climbing to the D and holding it, a small
    # bell, low strings settling under it. Major after all that minor: the breath out.
    desk = B.Desk(None, tail=4.0)
    desk.track('guitar', 0.0, -0.2, -12.0, B.guitar_body)
    desk.track('reed', -2.0, 0.15, -8.0)
    desk.track('low', -8.0, 0.0, -10.0, B.cello_body)
    desk.track('bell', -13.0, 0.3, -6.0)
    gtr = B.Guitar(child(r, 'g'), bright=0.55)
    shape = [x for x in SHAPES['D'] if x]
    gtr.strum(0.0, shape, 0.75, True, 0.05)
    gtr.strum(0.4, shape[-4:], 0.45, False, 0.03)
    gtr.strum(0.8, shape, 0.6, True, 0.06)
    gtr.render(desk, 'guitar', max_ring=4.0)
    p = B.Phrase()
    p.note(0.05, 0.22, 'A4').note(0.27, 0.22, 'B4').note(0.49, 0.26, 'C#5').note(0.75, 2.3, 'D5', vib=0.24)
    x, t0 = B.reed(p, child(r, 'reed'))
    desk.add('reed', x, t0)
    for j, note in enumerate(['D2', 'A2', 'F#3']):
        desk.add('low', B.bowed(note, 3.6, child(r, 'l', j), 0.8, attack=0.6, release=1.8, vib=0.06, bright=0.4), 0.2)
    desk.add('bell', B.kit.chime(B.hz('D6'), child(r, 'bell'), t60=2.5, bright=0.7), 0.75)
    return desk.mix(_reverb(r, 2.4), _sting_bus)
