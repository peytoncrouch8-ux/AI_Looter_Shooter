"""The main menu's music: Teropa's theme, the first thing a player hears. The planet from orbit, a long road ahead and
a dead man walking it. Played by the orchestra (orchestra.py) on band.py's Desk, mixed to sit under the menu's clicks.

    Music.Menu.Theme   32 bars of 4/4 at 72 (a bar is 3.333 s, 106.67 s), a seamless loop (MusicRules::Menu)

Built on orchestra.MAIN_MOTIF (D minor) and its harmony; the B section states it in F major. Bar by bar (1-based):

    1       the low drone (basses D2, cellos A2) under a high violin A5, the far bell on D, a harp rising through Dm
    2-4     the solo horn states the motif over violas; a harp rolls A major under its open E
    5       the harp echoes the motif's head high; violas lean to G minor
    6-8     cellos and violas answer it (the motif turned home, ending on D) under a soft violin pad; a timpani roll
    9-11    the build: violins sing the motif, horns a counter-line under it, a choir 'oo' blended into the pad, the
            harp's eighths start walking, basses leave the drone
    12      a breath: horns alone fall F-E, the bass steps down
    13-16   the climb to F major: violins rise (Bb, Gm7, Csus4-C, C7), horns and choir ('ah') swell, trombones enter,
            timpani roll and a cymbal swell into...
    17-19   the B section: horns and violins on the motif in F major, a viola counter-line, trombones and choir
    20      violins' descant over C7, horns' tritone pulling to F, the second swell and roll
    21-23   the climax: horns, trumpets and sopranos on the motif, violins an octave up, full brass and strings,
            timpani, bass drum, a crash; the motif now cadences home on F (its A over F, warm)
    24      Bb to A: everything thins and turns back to D minor
    25-27   the close: the solo horn plays the answer over a fingerpicked guitar (the western colour, one line), the
            drone back, the far bell on D
    28      the celesta echoes the motif's head
    29-31   high violins sigh the motif's tail and settle on D over a hummed 'mm'
    32      quiet: the drone, the high A, violas on an open A, a soft harp roll on A into bar 1's bell

Real ranges and voicings throughout: thirds kept above C3 (the low end in octaves and fifths), voices led by step,
the choir only ever a pad or doubled by the horns (never bare), the glockenspiel and shriek unused.
"""
import numpy as np

from lib.core import SR, ns, child
from lib import filters as F
from recipes import cue
from recipes import band as B
from recipes import orchestra as O

M = B.Meter(72.0, 4, 4)
BEAT = M.beat
BAR = M.bar
BARS = 32


def T(bar, beat=0.0):
    """The time of a beat (quarter notes, from 0) in a bar (1-based, as the score above counts them)."""
    return (bar - 1) * BAR + beat * BEAT


def mel(bar, notes, vel=0.6, beat=0.0, **opts):
    """An orchestra Line from (note, beats[, vel[, options]]) laid end to end from bar/beat; a note None rests. The
    same note twice in a row is one held note (a pad's common tone stays bowed or sung through the chord change)."""
    ln = O.Line()
    t = T(bar, beat)
    for item in notes:
        note, beats = item[0], item[1]
        v = item[2] if len(item) > 2 and item[2] is not None else vel
        o = dict(opts)
        if len(item) > 3:
            o.update(item[3])
        if note is not None:
            f = B.hz(note)
            last = ln.notes[-1] if ln.notes else None
            if (last is not None and len(item) <= 3 and abs(last[2] - f) < 0.01 and
                    abs(last[0] + last[1] - t) < 1e-6):
                last[1] += beats * BEAT
            else:
                ln.note(t, beats * BEAT, f, v, **o)
        t += beats * BEAT
    return ln


def breathe(ln, sec=0.18):
    """The line's last note shortened, so a wind player (or the bow) breathes before the next phrase."""
    ln.notes[-1][1] = max(0.2, ln.notes[-1][1] - sec)
    return ln


# --- Harmony (bar, beat, chord): the harp and the guitar follow it ------------------------------------------------

HARMONY = [
    (1, 0, 'Dm'), (2, 3, 'Bb'), (3, 1, 'F'), (3, 2, 'Dm'), (3, 3, 'C'), (4, 0, 'A'), (5, 0, 'Dm'), (5, 2, 'Gm'),
    (6, 0, 'Dm'), (6, 3, 'Bb'), (7, 1, 'C'), (7, 2, 'F'), (7, 3, 'A'), (8, 0, 'Dm'),
    (9, 0, 'Dm'), (9, 3, 'Bb'), (10, 1, 'F'), (10, 2, 'Dm'), (10, 3, 'C'), (11, 0, 'A'), (12, 0, 'Dm'), (12, 2, 'C'),
    (13, 0, 'Bb'), (14, 0, 'Gm'), (15, 0, 'Csus'), (15, 2, 'C'), (16, 0, 'C7'),
    (17, 0, 'F'), (17, 3, 'Bb'), (18, 1, 'F'), (18, 2, 'Dm'), (18, 3, 'Gm'), (19, 0, 'C'), (20, 0, 'C7'),
    (21, 0, 'F'), (21, 3, 'Bb'), (22, 1, 'F'), (22, 2, 'Dm'), (22, 3, 'C7'), (23, 0, 'F'), (24, 0, 'Bb'), (24, 2, 'A'),
    (25, 0, 'Dm'), (25, 3, 'Bb'), (26, 1, 'C'), (26, 2, 'F'), (26, 3, 'A'), (27, 0, 'Dm'), (28, 0, 'Dm'),
    (29, 0, 'Bb'), (30, 0, 'Gm'), (30, 1, 'A'), (31, 0, 'Dm'), (32, 0, 'Asus'), (32, 2, 'A'),
]
_CHANGES = sorted((T(b, bt), c) for b, bt, c in HARMONY)


def chord_at(t):
    out = _CHANGES[0][1]
    for at, c in _CHANGES:
        if at <= t + 1e-6:
            out = c
        else:
            break
    return out


# The harp's walking eighths: four chord tones in the middle (D4-A5), up and back.
HARP_V = {'Dm': ['D4', 'A4', 'D5', 'F5'], 'Bb': ['D4', 'F4', 'Bb4', 'D5'], 'F': ['C4', 'F4', 'A4', 'C5'],
          'C': ['C4', 'G4', 'C5', 'E5'], 'C7': ['C4', 'G4', 'Bb4', 'E5'], 'Csus': ['C4', 'G4', 'C5', 'F5'],
          'A': ['A3', 'E4', 'A4', 'C#5'], 'Gm': ['D4', 'G4', 'Bb4', 'D5'], 'Asus': ['A3', 'E4', 'A4', 'D5']}
ARP = [0, 1, 2, 3, 2, 1, 2, 1]

# The guitar in drop D (as the Rest's theme plays it), low string first; None: not played.
GTR = {'Dm': ['D2', 'A2', 'D3', 'A3', 'D4', 'F4'], 'Bb': [None, 'Bb2', 'F3', 'Bb3', 'D4', 'F4'],
       'C': [None, 'C3', 'E3', 'G3', 'C4', 'E4'], 'F': ['F2', 'C3', 'F3', 'A3', 'C4', 'F4'],
       'A': [None, 'A2', 'E3', 'A3', 'C#4', 'E4'], 'Gm': ['G2', 'D3', 'G3', 'Bb3', 'D4', 'G4'],
       'Asus': [None, 'A2', 'E3', 'A3', 'D4', 'E4']}
# A lonely 4/4 fingerpicking: the thumb on 1 and 3, the fingers walking up and back between (eighth, string, vel).
PICK = [(0, 'bass', 0.5), (1, 3, 0.3), (2, 4, 0.36), (3, 5, 0.4), (4, 'alt', 0.4), (5, 4, 0.33), (6, 3, 0.3),
        (7, 4, 0.28)]

# The motif in F major (the B section), on the motif's own rhythm.
RHYTHM = [b for _, b in O.MAIN_MOTIF]
MOTIF_F = ['F4', 'C5', 'Bb4', 'D5', 'C5', 'A4', 'Bb4', 'G4']
# The answer (the strings' reply in bar 6, the horn's in the close): the same rhythm, turned home to D.
ANSWER = [('A3', 1.0), ('D4', 1.5), ('C4', 0.5), ('F4', 2.0), ('E4', 1.0), ('C4', 1.0), ('E4', 1.0), ('D4', 4.0)]


def _shape(notes, vels, opts=None):
    """(note, beats) pairs with a velocity each (and options by index)."""
    opts = opts or {}
    return [(n, b, v, opts.get(i, {})) for i, ((n, b), v) in enumerate(zip(notes, vels))]


def _up(notes, semis):
    """Note names moved by octaves (semis a multiple of 12)."""
    out = []
    for n, b in notes:
        name, octave = n[:-1], int(n[-1])
        out.append((f'{name}{octave + semis // 12}', b))
    return out


def _bus(x, circular):
    # The orchestra's master with the low-mid eased a little more: pads, the horns and the low strings pile up there.
    return O.orchestra_bus(x, circular, mud_cut=-2.0)


def _bell_fx(x, c):
    # Far off: the bell's low hum and its sharpest partials taken away by the distance.
    return F.filt(x, F.hp(150.0), F.lp(4200.0, 0.7), F.peak(3000.0, -3.0, 0.8), extend=False, circular=c)


def _guitar_fx(x, c):
    return F.filt(B.guitar_body(x, c), F.peak(3200.0, -3.0, 0.9), extend=False, circular=c)


@cue('Music.Menu.Theme', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0,
     align=False)
def menu_theme(v, r):
    # Teropa from orbit: wide and lonely. A low drone and a high thin violin with nothing between, the far bell, the
    # horn alone on the motif; the strings answer; horns, strings and a choir build to F major's big, warm statement;
    # it cadences home, thins, and the horn walks on alone over a guitar until the drone is all that's left.
    desk = B.Desk(BARS * BAR)
    desk.track('drone', -13.0, 0.0, -11.0)
    desk.track('bass', -3.5, 0.0, -13.0)
    desk.track('violas', -6.5, 0.0, -10.0)
    desk.track('cellos', -6.0, 0.0, -9.0)
    desk.track('vln', -0.5, 0.0, -9.0)
    desk.track('vlnpad', -12.0, 0.0, -6.0)
    desk.track('solo', -8.0, -0.22, -7.5)
    desk.track('horns', -1.5, 0.0, -9.0)
    desk.track('tbn', -6.0, 0.0, -10.0)
    desk.track('tpt', -8.0, 0.0, -9.0)
    desk.track('tuba', -11.0, 0.0, -13.0)
    desk.track('choir', -6.0, 0.0, -6.0)
    desk.track('timp', -5.0, -0.08, -10.0)
    desk.track('cym', -11.0, 0.2, -8.0)
    desk.track('bd', -10.0, 0.0, -11.0)
    desk.track('harp', -11.0, -0.5, -8.0)
    desk.track('bell', -14.0, -0.35, -2.0, _bell_fx)
    desk.track('celesta', -18.0, -0.4, -5.0)
    desk.track('guitar', -14.0, 0.32, -13.0, _guitar_fx)

    _drone(desk, child(r, 'drone'))
    _opening(desk, child(r, 'opening'))
    _build(desk, child(r, 'build'))
    _b_section(desk, child(r, 'b'))
    _close(desk, child(r, 'close'))
    _harp(desk, child(r, 'harp'))
    _percussion(desk, child(r, 'perc'))
    return desk.mix(O.orchestra_hall(child(r, 'hall'), t60=3.4), _bus)


def _drone(desk, r):
    # The low strings' drone, bars 25 to 8 across the loop's seam (it folds round): basses on D2, cellos on A2 (an
    # open fifth, no third down there). The cellos leave for the answer in bar 6.
    desk.add('drone', *O.string_sustain('basses', ['D2'], 16 * BAR, child(r, 'cb'), 0.5, T(25), players=2,
                                        attack=2.5, release=2.0, vib=0.5))
    desk.add('drone', *O.string_sustain('cellos', ['A2'], 13 * BAR, child(r, 'vc'), 0.4, T(25), players=2,
                                        attack=3.0, release=2.5, vib=0.5))
    # The thin high violin A5 above it (bars 31 to 4, across the seam): the wide, empty sky between.
    desk.add('vlnpad', *O.string_sustain('violins', ['A5'], 6 * BAR, child(r, 'hi'), 0.22, T(31), players=3,
                                         dyn='swell', attack=3.0, release=2.5, vib=0.6, width=0.6))


def _opening(desk, r):
    # Bars 1-8. The solo horn on the motif (bars 2-4), quiet and alone, a swell to the minor sixth and a sigh down.
    vels = [0.46, 0.53, 0.5, 0.6, 0.52, 0.48, 0.5, 0.46]
    horn = mel(2, _shape(O.MAIN_MOTIF, vels, {3: {'dyn': 'swell'}, 7: {'dyn': 'dim'}}))
    desk.add('solo', *O.brass_line('horn', breathe(horn), child(r, 'horn')))
    # Violas under it (bars 2-5), two soft lines through the motif's harmony over the drone, leaning to G minor in 5.
    v1 = mel(2, [('A3', 3), ('Bb3', 2), ('A3', 2), ('G3', 1), ('A3', 4), ('A3', 2), ('Bb3', 2.6)], 0.34)
    v2 = mel(2, [('F3', 7), ('E3', 5), ('F3', 2), ('G3', 2.6)], 0.32)
    for i, ln in enumerate((v1, v2)):
        desk.add('violas', *O.string_legato('violas', ln, child(r, 'vla', i), players=2, port=0.05, vib=0.7))
    # Bars 6-8: the strings answer, cellos and violas in unison in the cello's singing tenor.
    vels = [0.5, 0.56, 0.52, 0.62, 0.56, 0.52, 0.54, 0.5]
    ans = mel(6, _shape(ANSWER, vels, {0: {'slur': False}, 3: {'dyn': 'swell'}, 7: {'dyn': 'dim'}}))
    desk.add('cellos', *O.string_legato('cellos', ans, child(r, 'ans_vc'), players=4, port=0.2))
    desk.add('cellos', *O.string_legato('violas', ans.moved(vel=0.85), child(r, 'ans_va'), players=2, port=0.2))
    # Over them, a soft violin pad rising by step (A-Bb-C-C#-D under D-E-F-E-F).
    p1 = mel(6, [('A4', 3), ('Bb4', 2), ('C5', 2), ('C#5', 1), ('D5', 4)], 0.27)
    p2 = mel(6, [('D5', 5), ('E5', 1), ('F5', 1), ('E5', 1), ('F5', 4)], 0.25)
    for i, ln in enumerate((p1, p2)):
        desk.add('vlnpad', *O.string_legato('violins', ln, child(r, 'pad', i), players=3, port=0.05, vib=0.8,
                                            width=0.7))


def _build(desk, r):
    # Bars 9-16.
    # Violins sing the motif an octave up (9-11), then climb (13-16) to the edge of F major.
    vels = [0.54, 0.58, 0.55, 0.64, 0.58, 0.55, 0.57, 0.54]
    v = mel(9, _shape(_up(O.MAIN_MOTIF, 12), vels, {0: {'slur': False}, 3: {'dyn': 'swell'}, 7: {'dyn': 'dim'}}))
    desk.add('vln', *O.string_legato('violins', v, child(r, 'vln1'), players=8, port=0.15, detune=8.0))
    climb = mel(13, [('Bb4', 1, 0.56, {'slur': False}), ('D5', 1.5, 0.6), ('C5', 0.5, 0.58), ('F5', 2, 0.66),
                     ('D5', 1, 0.63), ('G5', 2, 0.7), ('F5', 2, 0.72), ('E5', 1, 0.68), ('D5', 1, 0.66),
                     ('C5', 1, 0.68), ('D5', 1, 0.72), ('E5', 2, 0.8, {'dyn': 'cresc'})])
    desk.add('vln', *O.string_legato('violins', climb, child(r, 'vln2'), players=8, port=0.15, detune=8.0))
    # Horns: a counter-line under the motif, falling where it rises (9-12), then holding the climb's chords.
    ha = mel(9, [('F4', 1.5), ('E4', 1), ('D4', 2.5), ('C4', 1), ('D4', 1), ('E4', 1), ('C#4', 2), ('D4', 1),
                 ('E4', 1), ('F4', 2, 0.5), ('E4', 2, 0.48)], 0.46)
    desk.add('horns', *O.brass_section('horn', breathe(ha), child(r, 'ha'), players=2, pan=-0.36))
    hb = mel(13, [('D4', 4, 0.52), ('D4', 4, 0.58), ('C4', 4, 0.64), ('Bb3', 4, 0.74, {'dyn': 'cresc'})])
    hc = mel(13, [('F4', 4, 0.54), ('G4', 4, 0.6), ('F4', 2, 0.66), ('E4', 2, 0.66),
                  ('G4', 4, 0.76, {'dyn': 'cresc'})])
    desk.add('horns', *O.brass_section('horn', breathe(hb, 0.1), child(r, 'hb'), players=2, pan=-0.32))
    desk.add('horns', *O.brass_section('horn', breathe(hc, 0.1), child(r, 'hc'), players=2, pan=-0.24))
    # Trombones join for the last two bars of the climb (C, then C7's Bb pulling to A).
    for i, notes in enumerate([[('G3', 4), ('Bb3', 4)], [('F3', 2), ('E3', 6)], [('C3', 8)]]):
        ln = mel(15, notes, 0.42, dyn='cresc')
        desk.add('tbn', *O.brass_section('trombone', breathe(ln, 0.1), child(r, 'tb', i), players=1,
                                         pan=0.26 + 0.07 * i))
    # The violas' pad, and the bass line (basses, cellos an octave up) leaving the drone.
    v1 = mel(9, [('A3', 3), ('Bb3', 2), ('A3', 2), ('G3', 1), ('A3', 4), ('A3', 2), ('G3', 2), ('Bb3', 8),
                 ('C4', 4), ('Bb3', 4, 0.6)], 0.4)
    v2 = mel(9, [('F3', 7), ('E3', 5), ('F3', 2), ('E3', 2), ('F3', 4), ('G3', 12, 0.55)], 0.38)
    for i, ln in enumerate((v1, v2)):
        desk.add('violas', *O.string_legato('violas', ln, child(r, 'vla', i), players=2, port=0.05))
    bass = [('D2', 3), ('Bb1', 2), ('A1', 1), ('D2', 1), ('C2', 1), ('A1', 4), ('D2', 2), ('C2', 2), ('Bb1', 4),
            ('G1', 4), ('C2', 8)]
    bv = [0.46, 0.46, 0.46, 0.46, 0.46, 0.46, 0.5, 0.5, 0.55, 0.6, 0.68]
    _bass(desk, 9, bass, bv, child(r, 'bass'))
    # The choir: a soft 'oo' blended into the pad, opening to 'ah' as it climbs.
    sop = mel(9, [('A4', 3), ('Bb4', 2), ('A4', 2), ('G4', 1), ('A4', 4), ('A4', 2), ('G4', 2),
                  ('F4', 4, 0.42, {'vowel': 'ah'}), ('G4', 4, 0.46, {'vowel': 'ah'}),
                  ('F4', 2, 0.5, {'vowel': 'ah'}), ('E4', 2, 0.5, {'vowel': 'ah'}),
                  ('G4', 4, 0.58, {'vowel': 'ah', 'dyn': 'cresc'})], 0.34)
    alt = mel(9, [('F4', 7), ('E4', 5), ('F4', 2), ('E4', 2), ('D4', 8, 0.44, {'vowel': 'ah'}),
                  ('C4', 4, 0.5, {'vowel': 'ah'}), ('Bb3', 4, 0.56, {'vowel': 'ah', 'dyn': 'cresc'})], 0.32)
    desk.add('choir', *O.choir_line('soprano', sop, child(r, 'sop'), 'oo', voices=4, pan=-0.12, width=0.8))
    desk.add('choir', *O.choir_line('alto', alt, child(r, 'alt'), 'oo', voices=4, pan=0.12, width=0.8))


def _bass(desk, bar, notes, vels, r):
    """The bass line: basses on the notes, cellos an octave up (octaves only down there)."""
    ln = mel(bar, [(n, b, v) for (n, b), v in zip(notes, vels)])
    desk.add('bass', *O.string_legato('basses', ln, child(r, 'cb'), players=3, port=0.05))
    desk.add('bass', *O.string_legato('cellos', ln.moved(12.0, vel=0.92), child(r, 'vc'), players=3, port=0.05))


def _b_section(desk, r):
    # Bars 17-24: F major, big and warm.
    rb = RHYTHM
    # Bars 17-19: horns in unison on the motif in F, violins an octave above.
    hv = [0.72, 0.76, 0.72, 0.8, 0.76, 0.72, 0.74, 0.7]
    h1 = mel(17, _shape(list(zip(MOTIF_F, rb)), hv, {0: {'slur': False}, 3: {'dyn': 'swell'}}))
    desk.add('horns', *O.brass_section('horn', breathe(h1, 0.12), child(r, 'h1'), players=4))
    # The climax (21-23): the motif again, now cadencing home: its last Bb over C7 resolving to A over F.
    climax = list(zip(MOTIF_F[:7], rb[:7])) + [('A4', 4.0)]
    cv = [0.84, 0.88, 0.84, 0.92, 0.88, 0.84, 0.86, 0.82]
    h2 = mel(21, _shape(climax, cv, {0: {'slur': False}, 3: {'dyn': 'swell'}, 7: {'dyn': 'dim'}}))
    desk.add('horns', *O.brass_section('horn', breathe(h2, 0.1), child(r, 'h2'), players=4))
    desk.add('tpt', *O.brass_section('trumpet', breathe(h2.moved(vel=0.68), 0.1), child(r, 'tp'), players=2))
    # Bar 20: the horns' C7 (E and Bb, the tritone pulling to F and A); bar 24: Bb to A, fading.
    for i, notes in enumerate([[('E4', 4)], [('Bb4', 4)]]):
        ln = mel(20, notes, 0.55, dyn='cresc')
        desk.add('horns', *O.brass_section('horn', breathe(ln, 0.12), child(r, 'h20', i), players=2,
                                           pan=-0.36 + 0.1 * i))
    for i, notes in enumerate([[('D4', 2, 0.6), ('C#4', 2, 0.48, {'dyn': 'dim'})],
                               [('F4', 2, 0.58), ('E4', 2, 0.46, {'dyn': 'dim'})]]):
        desk.add('horns', *O.brass_section('horn', breathe(mel(24, notes)), child(r, 'h24', i), players=2,
                                           pan=-0.36 + 0.1 * i))
    # Violins: the motif an octave up (17-19), the descant over C7 (20), the climax (21-23), Bb to A (24): one long
    # bow line, re-bowed at each statement.
    up = list(zip(_up([(n, 1) for n in MOTIF_F], 12), rb))
    vn = [(n, b) for (n, _), b in up]
    vln = [(n, b, v, o) for (n, b), v, o in zip(vn, [0.7, 0.74, 0.7, 0.78, 0.74, 0.7, 0.72, 0.68],
                                                 [{'slur': False}, {}, {}, {'dyn': 'swell'}, {}, {}, {}, {}])]
    vln += [('E5', 1, 0.66, {'slur': False}), ('F5', 1, 0.7), ('G5', 1.5, 0.76), ('E5', 0.5, 0.74)]
    vc = [(n, b) for (n, _), b in zip(_up([(n, 1) for n in MOTIF_F[:7]] + [('A4', 1)], 12), rb)]
    vln += [(n, b, v, o) for (n, b), v, o in zip(vc, [0.84, 0.88, 0.84, 0.92, 0.88, 0.84, 0.86, 0.84],
                                                 [{'slur': False}, {}, {}, {'dyn': 'swell'}, {}, {}, {}, {}])]
    vln += [('F5', 2, 0.6), ('E5', 2, 0.46, {'dyn': 'dim'})]
    desk.add('vln', *O.string_legato('violins', mel(17, vln), child(r, 'vln'), players=8, port=0.12, detune=8.0))
    # Violas: the counter-line, falling while the tune climbs, rising under its long notes.
    ctr = [('C4', 1), ('A3', 1), ('F3', 1), ('F3', 1), ('Bb3', 1), ('C4', 1), ('D4', 1), ('D4', 1), ('E4', 2),
           ('F4', 1), ('E4', 1)]
    ctr_v = [0.6, 0.58, 0.56, 0.58, 0.62, 0.64, 0.66, 0.64, 0.68, 0.64, 0.62]
    va = mel(17, [(n, b, v) for (n, b), v in zip(ctr, ctr_v)] + [('E4', 4, 0.62, {'dyn': 'cresc'})])
    desk.add('violas', *O.string_legato('violas', va, child(r, 'va1'), players=4, port=0.1))
    ctr2 = [('C4', 1), ('A3', 1), ('F3', 1), ('F3', 1), ('Bb3', 1), ('C4', 1), ('D4', 1), ('E4', 1), ('F4', 2),
            ('E4', 1), ('C4', 1), ('D4', 2), ('C#4', 2)]
    ctr2_v = [0.74, 0.72, 0.7, 0.72, 0.76, 0.78, 0.8, 0.8, 0.8, 0.74, 0.7, 0.58, 0.46]
    va2 = mel(21, [(n, b, v) for (n, b), v in zip(ctr2, ctr2_v)])
    desk.add('violas', *O.string_legato('violas', va2, child(r, 'va2'), players=4, port=0.1))
    # The bass line, F major's, with the tuba under the climax.
    bass = [('F1', 3), ('Bb1', 2), ('A1', 1), ('D2', 1), ('G1', 1), ('C2', 4), ('C2', 4),
            ('F1', 3), ('Bb1', 2), ('A1', 1), ('D2', 1), ('C2', 1), ('F1', 4), ('Bb1', 2), ('A1', 2)]
    bv = [0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.74, 0.84, 0.84, 0.84, 0.84, 0.84, 0.8, 0.6, 0.46]
    _bass(desk, 17, bass, bv, child(r, 'bass'))
    tu = mel(21, [('F2', 3, 0.58), ('Bb1', 2, 0.6), ('A1', 1, 0.58), ('D2', 1, 0.58), ('C2', 1, 0.6),
                  ('F2', 4, 0.55, {'dyn': 'dim'}), ('Bb1', 2, 0.45), ('A1', 2, 0.36, {'dyn': 'dim'})])
    desk.add('tuba', *O.brass_section('tuba', breathe(tu), child(r, 'tuba'), players=1))
    # Trombones: warm chords in the middle, led by step.
    tbn = [
        [('C4', 3), ('D4', 2), ('C4', 1), ('D4', 1), ('D4', 1), ('C4', 4), ('Bb3', 4),
         ('C4', 3), ('D4', 2), ('C4', 1), ('D4', 1), ('Bb3', 1), ('C4', 4), ('D4', 2), ('C#4', 2)],
        [('A3', 3), ('Bb3', 2), ('A3', 1), ('A3', 1), ('Bb3', 1), ('G3', 4), ('G3', 4),
         ('A3', 3), ('Bb3', 2), ('A3', 1), ('A3', 1), ('G3', 1), ('A3', 4), ('Bb3', 2), ('A3', 2)],
        [('F3', 7), ('G3', 1), ('E3', 4), ('E3', 4),
         ('F3', 7), ('E3', 1), ('F3', 4), ('F3', 2), ('E3', 2)],
    ]
    # Two phrases a player: bars 17-20 (the last chord swelling into the climax) and 21-24 (the last two fading).
    for i, (notes, k) in enumerate(zip(tbn, (7, 7, 4))):
        a = mel(17, [(n, b, 0.56) for n, b in notes[:k - 1]] + [(notes[k - 1][0], notes[k - 1][1], 0.62,
                                                                    {'dyn': 'cresc'})])
        rest = notes[k:]
        b2 = mel(21, [(n, b, 0.72) for n, b in rest[:-2]] + [(rest[-2][0], rest[-2][1], 0.52),
                                                             (rest[-1][0], rest[-1][1], 0.4, {'dyn': 'dim'})])
        for part, ln in (('a', a), ('b', b2)):
            desk.add('tbn', *O.brass_section('trombone', breathe(ln, 0.12), child(r, 'tbn', i, part), players=1,
                                             pan=0.26 + 0.07 * i))
    # The choir: soprano, alto and tenor pads; at the climax the sopranos sing the tune with the horns (doubled,
    # never bare).
    sop = mel(17, [('A4', 3), ('Bb4', 2), ('A4', 2), ('Bb4', 1), ('G4', 4), ('Bb4', 4, 0.6, {'dyn': 'cresc'})], 0.58)
    alt = mel(17, [('F4', 7), ('D4', 1), ('E4', 4), ('E4', 4, 0.6, {'dyn': 'cresc'}),
                   ('F4', 7, 0.72), ('E4', 1, 0.72), ('F4', 4, 0.66), ('F4', 2, 0.52), ('E4', 2, 0.4, {'dyn': 'dim'})],
              0.56)
    ten = mel(17, [('C4', 3), ('D4', 2), ('C4', 1), ('D4', 2), ('C4', 4), ('C4', 4, 0.58, {'dyn': 'cresc'}),
                   ('C4', 3, 0.7), ('D4', 2, 0.72), ('C4', 1, 0.7), ('D4', 1, 0.7), ('C4', 1, 0.7), ('C4', 4, 0.64),
                   ('D4', 2, 0.5), ('C#4', 2, 0.4, {'dyn': 'dim'})], 0.54)
    tune = mel(21, _shape(climax, [0.66, 0.7, 0.66, 0.74, 0.7, 0.66, 0.68, 0.64], {7: {'dyn': 'dim'}}) +
               [('Bb4', 2, 0.5), ('A4', 2, 0.4, {'dyn': 'dim'})])
    desk.add('choir', *O.choir_line('soprano', sop, child(r, 'sop'), 'ah', voices=4, pan=-0.12, width=0.8))
    desk.add('choir', *O.choir_line('soprano', tune, child(r, 'tune'), 'ah', voices=4, pan=-0.05, width=0.8))
    desk.add('choir', *O.choir_line('alto', alt, child(r, 'alt'), 'ah', voices=4, pan=0.12, width=0.8))
    desk.add('choir', *O.choir_line('tenor', ten, child(r, 'ten'), 'ah', voices=4, pan=0.2, width=0.8))


def _close(desk, r):
    # Bars 25-32: it thins back. The solo horn plays the answer (25-27) over the fingerpicked guitar.
    vels = [0.44, 0.5, 0.46, 0.54, 0.5, 0.46, 0.48, 0.42]
    ans = mel(25, _shape(ANSWER, vels, {3: {'dyn': 'swell'}, 7: {'dyn': 'dim'}}))
    desk.add('solo', *O.brass_line('horn', breathe(ans), child(r, 'horn')))
    gtr = B.Guitar(child(r, 'guitar'), bright=0.4)
    prev = None
    for bar in range(25, 31):
        scale = 0.85 if bar < 29 else (0.72 if bar == 29 else 0.6)
        for e, s, v in PICK:
            t = T(bar, e * 0.5)
            ch = chord_at(t)
            shape = GTR[ch]
            bass = next(i for i, x in enumerate(shape) if x)
            alt = bass + 2 if shape[bass + 2] else bass + 1
            string = bass if s == 'bass' else (alt if s == 'alt' else s)
            if ch != prev and s not in ('bass', 'alt'):
                # A new chord speaks in the bass.
                string = bass
            prev = ch
            if shape[string] is None:
                continue
            gtr.pick(t, string, shape[string], v * scale)
    # Bar 31: the last D minor, picked once and left to ring.
    gtr.pick(T(31), 0, 'D2', 0.4)
    gtr.pick(T(31, 0.5), 3, 'A3', 0.26)
    gtr.pick(T(31, 1.0), 5, 'F4', 0.28)
    gtr.damp(T(32, 2))
    gtr.render(desk, 'guitar')
    # Violas' soft pad under the horn (25-28), then the close's chords (29-32), ending on an open A.
    v1 = mel(25, [('A3', 3), ('Bb3', 2), ('G3', 1), ('A3', 6), ('A3', 4), ('Bb3', 4), ('Bb3', 1), ('A3', 3),
                  ('A3', 8, 0.24, {'dyn': 'dim'})], 0.32)
    v2 = mel(25, [('F3', 3), ('F3', 2), ('E3', 1), ('F3', 1), ('E3', 1), ('F3', 4), ('F3', 4), ('F3', 4),
                  ('G3', 1), ('E3', 3), ('F3', 4), ('E3', 4, 0.22, {'dyn': 'dim'})], 0.3)
    for i, ln in enumerate((v1, v2)):
        desk.add('violas', *O.string_legato('violas', breathe(ln, 0.4), child(r, 'vla', i), players=2, port=0.05,
                                            vib=0.7))
    # Bar 28: the celesta echoes the motif's head, far up.
    rc = child(r, 'cel')
    t = T(28)
    for k, (note, beats) in enumerate(O.MAIN_MOTIF[:4]):
        n5 = note[:-1] + str(int(note[-1]) + 1)
        desk.add('celesta', O.celesta(n5, child(rc, k), 0.36 if k != 3 else 0.42), t + B.human(rc, 0.004))
        t += beats * BEAT
    # Bars 29-31: high violins sigh the motif's tail (Bb A F G E) and settle on D, over a hummed 'mm'.
    tail = mel(29, [('Bb5', 2, 0.32, {'dyn': 'swell', 'slur': False}), ('A5', 1, 0.3), ('F5', 1, 0.29),
                    ('G5', 1, 0.3), ('E5', 3, 0.28), ('D5', 4, 0.26, {'dyn': 'dim'})])
    desk.add('vlnpad', *O.string_legato('violins', tail, child(r, 'tail'), players=4, port=0.15, width=0.6))
    hum_s = mel(29, [('Bb4', 4), ('Bb4', 1), ('A4', 3), ('A4', 8, 0.22, {'dyn': 'dim'})], 0.28)
    hum_a = mel(29, [('F4', 4), ('D4', 1), ('E4', 3), ('F4', 4), ('E4', 4, 0.2, {'dyn': 'dim'})], 0.26)
    desk.add('choir', *O.choir_line('soprano', breathe(hum_s, 0.5), child(r, 'hs'), 'mm', voices=4, pan=-0.1))
    desk.add('choir', *O.choir_line('alto', breathe(hum_a, 0.5), child(r, 'ha'), 'mm', voices=4, pan=0.1))


def _harp(desk, r):
    rh = child(r, 'human')
    tk = O.Takes(child(r, 'takes'), count=2)

    def pluck(note, vel, t):
        q = max(0.1, round(vel * 10.0) / 10.0)
        x = tk.get(('harp', note, q), lambda rr: O.harp(note, rr, q))
        desk.add('harp', x * (vel / q), t + B.human(rh, 0.005))

    def roll(notes, vel, t, gap=0.07):
        for k, note in enumerate(notes):
            pluck(note, vel * (0.9 + 0.05 * k), t + k * gap)

    # Bar 1: rising through D minor from D3, on eighths.
    for k, note in enumerate(['D3', 'A3', 'D4', 'E4', 'F4', 'A4', 'D5']):
        pluck(note, 0.42 + 0.025 * k, T(1, 0.5 * k))
    # Bar 4: A major rolled under the horn's open E (the C# only here).
    roll(['A2', 'E3', 'C#4', 'A4'], 0.32, T(4))
    # Bar 5: the motif's head echoed high.
    t = T(5)
    for k, (note, beats) in enumerate(O.MAIN_MOTIF[:4]):
        pluck(note[:-1] + str(int(note[-1]) + 1), 0.3 + 0.03 * k, t)
        t += beats * BEAT
    # Bar 8: a soft D minor roll under the answer's last note.
    roll(['D3', 'A3', 'D4', 'F4'], 0.3, T(8))
    # Bars 9-19: the walking eighths through the harmony, growing; a new chord speaks a little louder.
    prev = None
    for bar in list(range(9, 16)) + [17, 18, 19]:
        for e in range(8):
            t = T(bar, 0.5 * e)
            ch = chord_at(t)
            u = (bar - 9 + e / 8.0) / 11.0
            vel = (0.3 + 0.2 * u) * (1.15 if ch != prev else 1.0) * (0.92 if e % 2 else 1.0)
            prev = ch
            pluck(HARP_V[ch][ARP[e]], vel, t)
    # Glissandi up into bars 17 and 21 (F major), on the last beat.
    for bar, hi, vel in ((17, 'F5', 0.5), (21, 'C6', 0.58)):
        x, t0 = O.harp_gliss('F3', hi, BEAT * 0.95, child(r, 'gl', bar), vel,
                             scale=['F', 'G', 'A', 'Bb', 'C', 'D', 'E'], at=T(bar - 1, 3))
        desk.add('harp', x, t0)
    # Bars 21-24: rolled chords on the changes (the eighths would vanish under the tutti).
    for bar, beat, notes, vel in ((21, 0, ['F2', 'C3', 'F3', 'A3', 'C4', 'F4'], 0.62),
                                  (21, 3, ['Bb2', 'F3', 'Bb3', 'D4'], 0.5), (22, 2, ['D3', 'A3', 'D4', 'F4'], 0.5),
                                  (23, 0, ['F2', 'C3', 'F3', 'A3', 'C4', 'F4'], 0.56),
                                  (24, 0, ['Bb2', 'F3', 'Bb3', 'D4'], 0.42), (24, 2, ['A2', 'E3', 'A3', 'C#4'], 0.36)):
        roll(notes, vel, T(bar, beat), 0.05)
    # Bar 32: a soft A rolled on its third beat, leading into bar 1's bell.
    roll(['A2', 'E3', 'A3', 'C#4'], 0.26, T(32, 2), 0.09)


def _percussion(desk, r):
    tk = O.Takes(child(r, 'takes'), count=3)
    # The far bell: bar 1 (the theme begins), bar 9 (the build, softer, on A) and bar 25 (the close).
    for bar, note, vel in ((1, 'D4', 0.75), (9, 'A3', 0.4), (25, 'D4', 0.55)):
        desk.add('bell', B.bell(note, child(r, 'bell', bar), vel, t60=8.0, dur=8.0, bright=0.6), T(bar))
    # Timpani: a soft roll into bar 9; the rolls into 17 and 21; strokes through the B section.
    desk.add('timp', O.timpani_roll('D2', 2 * BEAT, child(r, 'roll8'), 0.12, 0.34, hit=False), T(8, 2))
    hits = [(9, 0, 'D2', 0.45), (15, 0, 'C2', 0.48), (17, 0, 'F2', 0.8), (18, 2, 'D2', 0.5), (19, 0, 'C2', 0.62),
            (21, 0, 'F2', 0.95), (21, 3, 'F2', 0.55), (22, 2, 'D2', 0.6), (22, 3, 'C2', 0.68), (23, 0, 'F2', 0.85),
            (24, 2, 'A2', 0.38), (25, 0, 'D2', 0.32)]
    for bar, beat, note, vel in hits:
        hard = 0.6 if vel > 0.9 else 0.42
        desk.add('timp', O.timpani(note, None, vel, hard, takes=tk), T(bar, beat))
    desk.add('timp', O.timpani_roll('C2', BAR - 0.05, child(r, 'roll16'), 0.18, 0.82, hit=False), T(16))
    desk.add('timp', O.timpani_roll('C2', BAR - 0.05, child(r, 'roll20'), 0.25, 0.9, hit=False), T(20))
    desk.add('timp', O.timpani_roll('F2', 3.2 * BEAT, child(r, 'roll23'), 0.42, 0.12, hit=False), T(23, 0.6))
    # Cymbal swells peaking on 17 and 21, a soft crash on each; the bass drum under them.
    desk.add('cym', O.cymbal_swell(BAR, child(r, 'sw16'), 0.55, 3.0), T(16))
    desk.add('cym', O.cymbal_swell(BAR, child(r, 'sw20'), 0.75, 3.0), T(20))
    desk.add('cym', O.cymbal_crash(child(r, 'cr17'), 0.42, 4.5), T(17))
    desk.add('cym', O.cymbal_crash(child(r, 'cr21'), 0.68, 5.5), T(21))
    for bar, vel in ((17, 0.45), (21, 0.78), (23, 0.5)):
        desk.add('bd', O.bass_drum(child(r, 'bd', bar), vel), T(bar))
