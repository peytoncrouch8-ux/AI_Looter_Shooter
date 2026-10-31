"""The beasts' boss fights (score round 1, lane B1): orchestral loops on orchestra.py, each with a Rage stem that the
director lays sample-locked over its base from the fight's last phase on (faded in on a bar line).

    Music.Boss.Gravemother        32 bars of 4/4 at 120 (64 s), D Phrygian: the Sink's spider queen. Col legno and
                                  pizzicato ostinatos skittering on 16ths, taiko lurching 3+3+2, low brass clusters a
                                  semitone wide, a whispering choir, the horns' crawling theme; the motif's head with
                                  its fifth twisted to a tritone.
    Music.Boss.Gravemother.Rage   the same 32 bars on top: shrieking high strings (kept low), the full choir (the women
                                  on the theme), taiko in double time, violins' spiccato an octave over the legs.
    Music.Boss.Sentinel           24 bars of 4/4 at 100 (57.6 s), A minor: the graveyard statue. Timpani, bass drum and
                                  war drums on a lurching 3+3+2, stone knocks and grinding, a low brass chorale, the
                                  men's low "oh", the motif's head augmented in the trombones, the horns' theme.
    Music.Boss.Sentinel.Rage      on top: horns in fifths (organum), faster timpani rolls on the upper drums, violins and
                                  violas in double time, war drums in sixteenths, the women's choir in the toll.

A base and its stem are one score: the same tables (harmony, sections, intensity) and one function, rendered with
rage False or True, through the same hall (one seed) and bus. The stems keep out of the base's low end (no bass doubling,
a high-pass at 110-120 Hz on every stem track). Sections bar by bar are in each function's comments.
"""
import numpy as np

from lib.core import rng
from lib import filters as F
from recipes import cue
from recipes import band as B
from recipes import orchestra as O

_PC = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8,
       'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
ACC16 = (0, 3, 6, 8, 11, 14)   # 3+3+2 on sixteenths, twice a bar
ACC8 = (0, 6, 12)              # 3+3+2 on eighths: the lurch


def _midi(note):
    if not isinstance(note, str):
        return int(round(69 + 12 * np.log2(float(note) / 440.0)))
    acc = note[1] if len(note) > 2 and note[1] in '#b' else ''
    return 12 * (int(note[1 + len(acc):]) + 1) + _PC[note[0] + acc]


def _hz(m):
    return 440.0 * 2.0 ** ((m - 69) / 12.0)


def _fold(m, lo, hi):
    """m moved by octaves into lo..hi (names)."""
    lo, hi = _midi(lo), _midi(hi)
    while m < lo:
        m += 12
    while m > hi:
        m -= 12
    return m


class _Mode:
    """A mode from a tonic: deg(0) the tonic, deg(1) the next step, negative below."""

    def __init__(self, tonic, steps):
        self.tonic = _midi(tonic)
        self.steps = steps

    def deg(self, d):
        return self.tonic + 12 * (d // 7) + self.steps[d % 7]

    def moved(self, note, steps):
        m = _midi(note)
        d = next(d for d in range(-28, 29) if self.deg(d) == m)
        return _hz(self.deg(d + steps))


def _mel(pairs, t0, beat, vel, semis=0.0, gap=0.03, bar_beats=4):
    """A Line from (note, beats[, opts]) pairs, every note tongued (unless opts say slur), the notes that start a bar
    marcato, the long ones swelled."""
    ln = O.Line()
    t = 0.0
    for i, p in enumerate(pairs):
        note, beats = p[0], p[1]
        opts = dict(p[2]) if len(p) > 2 else {}
        if note is not None:
            if i > 0:
                opts.setdefault('slur', False)
            if abs(t / bar_beats - round(t / bar_beats)) < 1e-6:
                opts.setdefault('dyn', 'marc')
            elif beats >= 2.5:
                opts.setdefault('dyn', 'swell')
            ln.note(t0 + t * beat, beats * beat - gap, B.hz(note) * 2.0 ** (semis / 12.0), vel, **opts)
        t += beats
    return ln


class _Takes(O.Takes):
    """Takes on a coarser velocity grid (quarter steps, not tenths): each recipe's dynamics come from its own scaling
    of the take, so a 32-bar ostinato needs a few dozen renders, not hundreds."""

    def get(self, key, make):
        if isinstance(key, tuple) and key and isinstance(key[-1], float):
            key = key[:-1] + (round(key[-1] * 4.0) / 4.0,)
        return super().get(key, make)


def _hp(freq):
    return lambda x, circ: F.filt(x, F.hpn(freq, 4), extend=False, circular=circ)


def _bus(x, circular):
    return O.orchestra_bus(x, circular, presence_cut=-3.0)


def _runs(chords, parts, bars):
    """(first bar, bars, chord) for each run of one chord inside one section."""
    out = []
    for b in bars:
        if out and chords[b] == out[-1][2] and parts[b] == parts[out[-1][0]] and out[-1][0] + out[-1][1] == b:
            out[-1][1] += 1
        else:
            out.append([b, 1, chords[b]])
    return out


def _voice_line(m, runs, pick, vel_of, gap=0.0, rebow=None):
    """A legato Line through runs, each run's note pick(chord) held for its bars; rebow(bar) True starts a new bow
    (or breath) there."""
    ln = O.Line()
    for b0, nb, ch in runs:
        o = {'dyn': 'swell'} if nb > 1 else {}
        if rebow and rebow(b0):
            o['slur'] = False
        ln.note(m.at(b0), nb * m.bar - gap, pick(ch), vel_of(b0), **o)
    return ln


# --- The Gravemother --------------------------------------------------------------------------------------------------

GM = B.Meter(120.0, 4, 4)
GM_BARS = 32
GM_MODE = _Mode('D4', [0, 1, 3, 5, 7, 8, 10])
GM_ROOT = (['D'] * 8 + ['D', 'Bb', 'D', 'Eb', 'C', 'Bb', 'A', 'D'] + ['D', 'D'] + ['D', 'Bb', 'Eb', 'D', 'C', 'Eb'] +
           ['D', 'Bb', 'D', 'Eb', 'C', 'Bb', 'A', 'D'])
GM_DEG = {'D': 0, 'Eb': 1, 'C': -1, 'Bb': -2, 'A': -3}
GM_PART = ['stir'] * 4 + ['crawl'] * 4 + ['theme'] * 8 + ['breath'] * 2 + ['hunt'] * 6 + ['frenzy'] * 8
GM_LIFT = [0.62, 0.64, 0.66, 0.7, 0.7, 0.73, 0.76, 0.82, 0.8, 0.8, 0.82, 0.84, 0.84, 0.86, 0.88, 0.84, 0.32, 0.38,
           0.72, 0.75, 0.8, 0.8, 0.86, 0.93, 0.92, 0.9, 0.93, 0.95, 0.86, 0.9, 0.95, 1.0]
# The horns' theme, eight bars over D Bb D Eb C Bb A D: the flat second worried at in the 3+3+2 lurch, sinking an
# octave in the second phrase like something lowering itself on a thread (A Bb Eb over A: the tritone at the end).
GM_THEME = [('D4', 1.5), ('Eb4', 1.5), ('D4', 1.0), ('Bb3', 1.5), ('A3', 2.5),
            ('D4', 1.5), ('Eb4', 1.5), ('F4', 1.0), ('Eb4', 3.0), ('D4', 0.5), ('C4', 0.5),
            ('G4', 1.5), ('F4', 1.5), ('Eb4', 1.0), ('D4', 1.5), ('C4', 1.5), ('Bb3', 1.0),
            ('A3', 1.5), ('Bb3', 1.5), ('Eb4', 1.0), ('D4', 4.0)]
# The legs: scale steps from the bar's root, a sixteenth each (the second pattern every fourth bar).
GM_LEGS = ([0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 4, 2, 1, 0, -1], [0, 1, 2, 1, 4, 2, 1, 0, 0, 1, 0, 5, 4, 2, 1, 0])
GM_VLA = {0: 0, 3: 1, 6: 0, 8: 4, 11: 1, 14: 0}
GM_CRAWL = [0, 0, 1, 0, 0, 0, -1, 0, 0, 1, 0, -1, 0, 0, 1, 2]
# The choir's voicing per root (rage): men low, men high, women low, women high.
GM_CHOIR = {'D': ('D3', 'A3', 'F4', 'A4'), 'Bb': ('D3', 'F3', 'F4', 'Bb4'), 'Eb': ('Eb3', 'G3', 'G4', 'Bb4'),
            'C': ('C3', 'G3', 'Eb4', 'G4'), 'A': ('C3', 'A3', 'Eb4', 'A4')}


def _gm_quote():
    """The game's motif head (D A G Bb) an octave down with its fifth twisted to a tritone, the Bb sinking to A."""
    head = O.MAIN_MOTIF[:4]
    notes = [(_hz(_midi(n) - 12 - (1 if i == 1 else 0)), b) for i, (n, b) in enumerate(head)]
    return notes + [(_hz(_midi(head[3][0]) - 13), 3.0)]


def _gm_note(bar, off, lo, hi, shift=0):
    """The note off scale steps from bar's root, the root folded into lo..hi (the figure keeps its shape)."""
    rd = GM_DEG[GM_ROOT[bar]]
    root = _fold(GM_MODE.deg(rd) + shift, lo, hi)
    return _hz(root + GM_MODE.deg(rd + off) - GM_MODE.deg(rd))


def _gravemother(rage):
    # Bars (1-based):
    #  1-4   Stir: a cluster stab, the tam-tam and the o-daiko on 1; col legno legs (thin, then all sixteenths), pizz
    #        basses and cellos on the 3+3+2 lurch, the taiko's lurch; whispers from bar 2; a cluster swell into 5.
    #  5-8   Crawl: pizz violas join on the sixteenth 3+3+2; ghost taiko; whispers; a cluster swell and a cymbal into 9.
    #  9-16  Theme: four horns on the theme over D Bb D Eb C Bb A D; violas' pad; cluster stabs on the lurch in 12, 16.
    #  17-18 Breath: soft col legno on the accents only, whispers, ponticello tremolo D/Eb, a held low D, one soft drum.
    #  19-24 Hunt: the motif's head twisted (trombones, horns an octave up) out of the breath; cellos crawl in spiccato;
    #        the horns' answer, whispers again; a two-bar cluster crescendo, the horns climbing, a cymbal into 25.
    #  25-32 Frenzy: the theme ff, trombones a sixth under it, the tuba's roots, the bass drum; 32 builds a cluster,
    #        a taiko fill and a cymbal swell into the seam (bar 1's stab).
    # Rage: taiko in double time (3+3+2 on sixteenths, rims), the full choir (men and women on the harmony, the women
    # on the theme in the frenzy, a "hah" on 1, 19 and 25), violins' spiccato an octave over the legs, the shriek
    # clawing up through bars 7-8, 15-16, 23-24, 31-32; in the breath the women's "oo" a semitone wide, the men's hum.
    m = GM
    desk = B.Desk(GM_BARS * m.bar)
    R = lambda *k: rng('score_beasts', 'Gravemother', *k)
    tk = _Takes(R('takes', rage), count=3)
    bt = m.beat
    if not rage:
        desk.track('taiko', 0.0, 0.0, -14.0)
        desk.track('legno', -4.0, 0.0, -10.0)
        desk.track('pizz', -7.0, 0.0, -11.0)
        desk.track('low', -5.0, 0.0, -15.0)
        desk.track('crawl', -6.0, 0.0, -12.0)
        desk.track('horns', -1.0, 0.0, -8.0)
        desk.track('bones', -3.0, 0.0, -10.0)
        desk.track('tuba', -7.0, 0.45, -13.0)
        desk.track('pad', -11.0, 0.0, -8.0)
        desk.track('breath', -9.0, 0.0, -8.0)
        desk.track('whisper', -12.0, 0.0, -6.0)
        desk.track('metal', -13.0, 0.2, -8.0)
    else:
        desk.track('taiko', -1.0, 0.0, -13.0, _hp(110.0))
        desk.track('choir', -2.0, 0.0, -7.0, _hp(120.0))
        desk.track('hah', -5.0, 0.0, -8.0, _hp(120.0))
        desk.track('spic', -8.0, 0.0, -10.0, _hp(120.0))
        desk.track('shriek', -17.0, 0.0, -7.0, _hp(120.0))
        desk.track('breath', -8.0, 0.0, -6.0, _hp(120.0))

    rh = R('human', rage)
    for bar in range(GM_BARS):
        part, lift = GM_PART[bar], GM_LIFT[bar]
        t = m.at(bar)
        if not rage:
            # The taiko: the lurch on eighths, ghosts and fills; the breath one soft stroke and a rising rim.
            hits = []
            if part == 'breath':
                hits = [(0, 1.0, 0.45)] if bar == 16 else [(12, 0.62, 0.3), (13, 0.62, 0.36), (14, 0.62, 0.42),
                                                           (15, 0.62, 0.5)]
            else:
                hits = [(0, 1.0, 0.98), (6, 0.8, 0.8), (12, 0.8, 0.85)]
                if part in ('crawl', 'theme', 'hunt', 'frenzy'):
                    hits += [(10, 0.62, 0.38), (15, 0.62, 0.42)]
                if bar % 4 == 3 or bar == 31:
                    hits += [(13, 0.62, 0.55), (14, 0.62, 0.65), (15, 0.62, 0.75)]
                if bar == 31:
                    hits += [(s, 0.62, 0.45 + 0.04 * (s - 8)) for s in (8, 9, 10, 11)]
            for s, size, v in hits:
                desk.add('taiko', O.taiko(None, B.lively(rh, v * (0.75 + 0.3 * lift), 0.06), size, takes=tk),
                         m.at(bar, s) + B.human(rh, 0.004), pan={1.0: 0.0, 0.8: -0.2, 0.62: 0.25}[size])
            if part == 'frenzy' and bar % 2 == 0:
                desk.add('taiko', O.bass_drum(None, 0.8, 3.0, takes=tk), t)
            # The legs: col legno violins on sixteenths, the accents harder.
            legs = GM_LEGS[1 if bar % 4 == 3 else 0]
            for s in range(16):
                if part == 'breath' and s not in ACC16:
                    continue
                if bar < 2 and s % 2 and s not in ACC16:
                    continue
                v = 0.33 if part == 'breath' else (0.8 if s in ACC16 else 0.46) * (0.7 + 0.35 * lift)
                note = _gm_note(bar, legs[s], 'C4', 'B4')
                desk.add('legno', *O.string_col_legno('violins', note, None, B.lively(rh, v, 0.12), m.at(bar, s),
                                                       players=4, pan=(-0.6, -0.25, 0.05)[(bar + s) % 3], takes=tk))
            # Pizz violas on the sixteenth 3+3+2; cellos and basses on the eighth lurch (cellos crawl arco later).
            if bar >= 2 and part != 'breath':
                for s, off in GM_VLA.items():
                    note = _gm_note(bar, off, 'C3', 'B3')
                    desk.add('pizz', *O.string_pizz('violas', note, None, (0.85 if s in (0, 8) else 0.66) * lift,
                                                    m.at(bar, s), takes=tk))
            if part != 'breath':
                for s, v in zip(ACC8, (0.9, 0.72, 0.78)):
                    desk.add('low', *O.string_pizz('basses', _gm_note(bar, 0, 'A1', 'G#2'), None, v * (0.6 + 0.4 * lift),
                                                   m.at(bar, s), takes=tk))
                    if part in ('stir', 'crawl', 'theme'):
                        desk.add('low', *O.string_pizz('cellos', _gm_note(bar, 0, 'C3', 'B3'), None, v * lift,
                                                       m.at(bar, s), takes=tk))
            if part in ('hunt', 'frenzy'):
                for s in range(16):
                    note = _gm_note(bar, GM_CRAWL[s], 'C3', 'B3')
                    desk.add('crawl', *O.string_spiccato('cellos', note, None, (0.82 if s in ACC16 else 0.55) * lift,
                                                         m.at(bar, s), length=0.1, takes=tk))
            # The pad: violas on the root's third and fifth, under the theme.
            if part in ('theme', 'frenzy'):
                rd = GM_DEG[GM_ROOT[bar]]
                pad = [_hz(_fold(GM_MODE.deg(rd + k), 'F3', 'E4')) for k in (2, 4)]
                desk.add('pad', *O.string_sustain('violas', pad, m.bar, R('pad', bar), 0.45 * lift, t, dyn='swell',
                                                  attack=0.3, release=0.4))
        else:
            # Double-time taiko: 3+3+2 on sixteenths, rims between, fills at phrase ends; none in the breath.
            if part != 'breath':
                steps = (8, 11, 14) if part == 'stir' else ACC16
                for s in steps:
                    v = (0.88 if s in (0, 8) else 0.66) * (0.7 + 0.35 * lift)
                    desk.add('taiko', O.taiko(None, B.lively(rh, v, 0.06), 0.5, takes=tk),
                             m.at(bar, s) + B.human(rh, 0.004), pan=0.2 if s % 2 else -0.15)
                if part in ('theme', 'hunt', 'frenzy'):
                    rims = (2, 5, 10, 13) if part == 'frenzy' else (4, 12)
                    for s in rims:
                        desk.add('taiko', O.taiko_rim(None, 0.5 * lift, takes=tk), m.at(bar, s) + B.human(rh, 0.004),
                                 pan=0.35)
                if bar % 4 == 3:
                    for s in (12, 13, 14, 15):
                        desk.add('taiko', O.taiko(None, 0.5 + 0.08 * (s - 12), 0.45, takes=tk),
                                 m.at(bar, s) + B.human(rh, 0.004), pan=-0.3)
            # Violins' spiccato an octave over the legs.
            if part in ('theme', 'hunt', 'frenzy'):
                legs = GM_LEGS[1 if bar % 4 == 3 else 0]
                for s in range(16):
                    note = _gm_note(bar, legs[s], 'C5', 'B5', 12)
                    desk.add('spic', *O.string_spiccato('violins', note, None, (0.7 if s in ACC16 else 0.45) * lift,
                                                        m.at(bar, s), players=6, length=0.09, takes=tk))
            if bar in (6, 14, 22, 30):
                desk.add('shriek', *O.string_shriek(2 * m.bar, R('shriek', bar), 0.7, t, low='D6', high='Ab6',
                                                    players=8, rise=2.0))

    if not rage:
        # Low brass clusters: a semitone wide over the tuba's D, stabbed, or swelled into the next downbeat.
        rb = R('bones')
        desk.add('bones', *O.brass_section('trombone', ['D3', 'Eb3'], rb, 0.6, 0.92, 'sfz', 0.0, players=4))
        desk.add('tuba', O.tuba('D2', 0.8, R('tuba', 0), 0.85, 'sfz'), 0.0)
        for bar in (3, 7):
            desk.add('bones', *O.brass_section('trombone', ['D3', 'Eb3', 'A3'], R('swell', bar), m.bar, 0.8, 'cresc',
                                               m.at(bar)))
            desk.add('tuba', O.tuba('D2', m.bar, R('tuba', bar), 0.75, 'cresc'), m.at(bar))
        for bar in (11, 15):
            for s, v in zip(ACC8, (0.85, 0.75, 0.8)):
                desk.add('bones', *O.brass_section('trombone', ['D3', 'Eb3', 'G3'], None, 0.2, v, 'stac', m.at(bar, s),
                                                   takes=tk))
        for bar, chord in ((22, ['C3', 'D3', 'Eb3']), (23, ['D3', 'Eb3', 'F3']), (31, ['D3', 'Eb3', 'A3'])):
            desk.add('bones', *O.brass_section('trombone', chord, R('climb', bar), m.bar, 0.88, 'cresc', m.at(bar)))
            desk.add('tuba', O.tuba('C2' if bar == 22 else 'D2', m.bar, R('tuba', bar), 0.8, 'cresc'), m.at(bar))
        # The horns: the theme (mf, then ff in the frenzy), the twisted quote's octave, the answer, the climb.
        desk.add('horns', *O.brass_section('horn', _mel(GM_THEME, m.at(8), bt, 0.7), R('theme'), players=4))
        desk.add('horns', *O.brass_section('horn', _mel(GM_THEME, m.at(24), bt, 0.9), R('theme2'), players=4))
        quote = _gm_quote()
        desk.add('bones', *O.brass_section('trombone', _mel(quote, m.at(18), bt, 0.88), R('quote'), players=3))
        desk.add('horns', *O.brass_section('horn', _mel(quote, m.at(18), bt, 0.82, 12.0), R('quote2'), players=4))
        answer = [('Eb4', 1.5), ('F4', 1.5), ('Eb4', 1.0), ('D4', 3.0), ('C4', 0.5), ('Bb3', 0.5)]
        desk.add('horns', *O.brass_section('horn', _mel(answer, m.at(20), bt, 0.78), R('answer'), players=4))
        desk.add('horns', *O.brass_section('horn', ['G3', 'C4'], R('climb', 'h1'), m.bar, 0.72, 'swell', m.at(22)))
        desk.add('horns', *O.brass_section('horn', ['Bb3', 'Eb4'], R('climb', 'h2'), m.bar, 0.82, 'cresc', m.at(23)))
        sixth = [(GM_MODE.moved(n, -5), b) for n, b in GM_THEME]
        desk.add('bones', *O.brass_section('trombone', _mel(sixth, m.at(24), bt, 0.85), R('sixth'), players=3))
        for bar in range(24, 31):
            desk.add('tuba', O.tuba(_hz(_gm_root_low(bar)), m.bar - 0.05, R('troot', bar), 0.72, 'marc'), m.at(bar))
        # Whispers, the breath, the metal.
        for b0, nb in ((1, 3), (5, 3), (16, 2), (20, 4)):
            desk.add('whisper', *O.whisper_choir(nb * m.bar, R('whisper', b0), 8, 0.5, m.at(b0)))
        desk.add('breath', *O.string_tremolo('violas', ['D4', 'Eb4'], 2 * m.bar, R('trem'), 0.42, m.at(16),
                                             dyn='swell', pont=0.6))
        desk.add('breath', *O.string_sustain('basses', 'D2', 2 * m.bar + 0.3, R('pedal'), 0.4, m.at(16), dyn='swell',
                                             attack=0.5, release=0.8))
        desk.add('metal', O.tam_tam(R('tam', 0), 0.5, 8.0), 0.0)
        desk.add('metal', O.tam_tam(R('tam', 18), 0.4, 8.0), m.at(18))
        desk.add('metal', O.tam_tam(R('tam', 24), 0.7, 8.0), m.at(24))
        for bar, v in ((7, 0.45), (23, 0.65), (31, 0.75)):
            desk.add('metal', O.cymbal_swell(m.bar, R('swell', 'cym', bar), v, 2.5), m.at(bar))
    else:
        # The full choir: men and women on the harmony, legato through each section; the women on the theme in the
        # frenzy; the shout on 1, 19 and 25; the breath's semitone.
        bars = [b for b in range(GM_BARS) if GM_PART[b] not in ('breath',)]
        runs = _runs(GM_ROOT, GM_PART, bars)
        vel = lambda b: 0.42 + 0.3 * GM_LIFT[b] if GM_PART[b] in ('stir', 'crawl') else 0.5 + 0.3 * GM_LIFT[b]
        new = lambda b: b == 0 or GM_PART[b] != GM_PART[b - 1]
        women = [r_ for r_ in runs if GM_PART[r_[0]] != 'frenzy']
        for k in (0, 1):
            ln = _voice_line(m, runs, lambda ch: B.hz(GM_CHOIR[ch][k]), vel, 0.05, new)
            for note in ln.notes:
                if note[0] < m.at(8) - 0.01:
                    note[4]['vowel'] = 'oo'   # closed while she stirs, open from the theme on
            desk.add('choir', *O.choir_line('men', ln, R('men', k), 'ah', voices=6))
        for k in (2, 3):
            ln = _voice_line(m, [r_ for r_ in women if GM_PART[r_[0]] != 'stir'], lambda ch: B.hz(GM_CHOIR[ch][k]),
                             vel, 0.05, new)
            desk.add('choir', *O.choir_line('women', ln, R('women', k), 'oo' if k == 2 else 'ah', voices=6))
        desk.add('choir', *O.choir_line('women', _mel(GM_THEME, m.at(24), bt, 0.78, 12.0, gap=0.06), R('wtheme'),
                                        'ah', voices=8))
        for bar in (0, 18, 24):
            desk.add('hah', *O.choir('men', ['D3', 'A3'], 0.35, R('hah', bar), 'hah', 8, 0.9, m.at(bar)))
            desk.add('hah', *O.choir('women', ['D4', 'A4'], 0.35, R('hahw', bar), 'hah', 8, 0.8, m.at(bar)))
        desk.add('breath', *O.choir('women', ['A4', 'Bb4'], 2 * m.bar, R('oo'), 'oo', 6, 0.42, m.at(16), 'swell'))
        desk.add('breath', *O.choir('men', ['D3'], 2 * m.bar, R('mm'), 'mm', 6, 0.4, m.at(16), 'swell'))
    return desk.mix(O.orchestra_hall(R('hall')), _bus)


def _gm_root_low(bar):
    return _fold(GM_MODE.deg(GM_DEG[GM_ROOT[bar]]), 'A1', 'G#2')


@cue('Music.Boss.Gravemother', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-14.0,
     align=False)
def boss_gravemother(v, r):
    return _gravemother(False)


@cue('Music.Boss.Gravemother.Rage', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0,
     align=False)
def boss_gravemother_rage(v, r):
    return _gravemother(True)


# --- The Stone Sentinel -----------------------------------------------------------------------------------------------

SN = B.Meter(100.0, 4, 4)
SN_BARS = 24
SN_CHORD = ['A', 'A', 'F', 'E', 'A', 'G', 'F', 'E', 'A', 'Dm', 'Dm', 'E', 'A', 'A', 'A', 'F', 'C', 'G', 'Dm', 'E',
            'A', 'F', 'Dm', 'E']
SN_PART = ['wake'] * 4 + ['march'] * 4 + ['quote'] * 4 + ['breath'] * 2 + ['grind'] * 6 + ['toll'] * 4
SN_LIFT = [0.66, 0.68, 0.72, 0.78, 0.8, 0.8, 0.82, 0.86, 0.84, 0.84, 0.86, 0.88, 0.34, 0.42, 0.82, 0.84, 0.86, 0.88,
           0.9, 0.92, 0.95, 0.96, 0.98, 1.0]
# Per chord: the floor (tuba, basses), the timpani's root and second drum, the trombones' chorale (thirds kept over
# D3), the men's open fifth, the cellos', the violas' (and toll horns') thirds, the rage horns' fifth, the women's,
# the double-time strings' figure.
SN_CH = {
    'A': dict(root='A1', timp=('A2', 'E2'), bones=['E3', 'A3', 'C4'], men=('A2', 'E3'), cel=('A2', 'E3'),
              vla=['C4', 'E4'], fifth=('A3', 'E4'), women=('C5', 'E5'), arp=['A4', 'C5', 'E5', 'C5']),
    'F': dict(root='F1', timp=('F2', 'C3'), bones=['F3', 'A3', 'C4'], men=('F2', 'C3'), cel=('F2', 'C3'),
              vla=['C4', 'F4'], fifth=('F3', 'C4'), women=('C5', 'F5'), arp=['A4', 'C5', 'F5', 'C5']),
    'E': dict(root='E1', timp=('E2', 'B2'), bones=['E3', 'G#3', 'B3'], men=('E2', 'B2'), cel=('E2', 'B2'),
              vla=['B3', 'G#4'], fifth=('E3', 'B3'), women=('B4', 'E5'), arp=['G#4', 'B4', 'E5', 'B4']),
    'G': dict(root='G1', timp=('G2', 'D3'), bones=['D3', 'G3', 'B3'], men=('G2', 'D3'), cel=('G2', 'D3'),
              vla=['B3', 'D4'], fifth=('G3', 'D4'), women=('B4', 'D5'), arp=['G4', 'B4', 'D5', 'B4']),
    'Dm': dict(root='D2', timp=('D2', 'A2'), bones=['D3', 'F3', 'A3'], men=('D3', 'A3'), cel=('D3', 'A3'),
               vla=['A3', 'F4'], fifth=('D4', 'A4'), women=('D5', 'F5'), arp=['A4', 'D5', 'F5', 'D5']),
    'C': dict(root='C2', timp=('C3', 'G2'), bones=['E3', 'G3', 'C4'], men=('C3', 'G3'), cel=('C3', 'G3'),
              vla=['E4', 'G4'], fifth=('C4', 'G4'), women=('C5', 'E5'), arp=['G4', 'C5', 'E5', 'C5']),
}
# The horns' theme in the grind (bars 15-20) over A F C G Dm E, the cellos an octave under.
SN_THEME = [('A3', 1.5), ('C4', 1.5), ('B3', 1.0), ('A3', 2.0), ('G3', 1.0), ('F3', 1.0),
            ('E3', 1.5), ('G3', 1.5), ('C4', 1.0), ('B3', 2.0), ('D4', 2.0),
            ('F4', 1.5), ('E4', 1.5), ('D4', 1.0), ('E4', 4.0)]
# The rage horns' organum: the lower voice per bar on the 3+3+2 (a perfect fifth over each), grind then toll.
SN_ORGANUM = {14: ('A3', 'G3', 'A3'), 15: ('F3', 'G3', 'A3'), 16: ('C4', 'D4', 'C4'), 17: ('G3', 'A3', 'G3'),
              18: ('D4', 'C4', 'A3'), 20: ('A3', 'G3', 'A3'), 21: ('F3', 'A3', 'C4'), 22: ('D4', 'C4', 'A3')}


def _sn_quote():
    """The game's motif head (D A G Bb) down an octave and a fourth into A minor (A E D F), augmented to twice its
    length, the F sliding down onto a long E (the trombones' glissando: the stone sinking)."""
    head = O.MAIN_MOTIF[:4]
    out = [(_hz(_midi(n) - 17), 2.0 * b) for n, b in head]
    return out + [(_hz(_midi(head[3][0]) - 18), 6.0, {'slur': True, 'glide': 0.35, 'dyn': 'dim'})]


def _sentinel(rage):
    # Bars (1-based):
    #  1-4   Wake: on 1 the timpani, bass drum, a stone knock, the trombones' and tuba's sfz A minor, the tam-tam; the
    #        basses' low A and the stone grinding; the men's "oh" from 2; the timpani's 3+3+2 from 3 (A A F E).
    #  5-8   March: the drums' lurch (timpani, bass drum, war drum, stone), the trombone chorale fp-crescendo bar by
    #        bar (A G F E: the old Phrygian descent), the men, cellos' fifths, violas' thirds.
    #  9-12  Quote: the motif's head augmented in the trombones (A E D F, sliding to E), the horns an octave up, the
    #        tam-tam; the drums lighter (A Dm Dm E).
    #  13-14 Breath: one soft timpani stroke, the stone grinding, the men humming, violas' ponticello tremolo, the low A.
    #  15-20 Grind: the horns' theme with the cellos under it (A F C G Dm E), brass stabs on the lurch, the drums
    #        full with ghosts; a cymbal swell into 21.
    #  21-24 Toll: the tutti on A F Dm E, sfz chords bar by bar, the horns and violas on the thirds, pickups on the
    #        timpani; 24 rolls on E into the seam (bar 1's hit) under a cymbal swell.
    # Rage: horns in fifths (swells in the wake, march and quote; the organum on the 3+3+2 in the grind and toll; pp
    # in the breath), timpani rolls on the upper drums into every downbeat (faster, and into the beats in the toll;
    # a long roll out of the breath), violins and violas in double time (eighths, then sixteenths), war drums on the
    # sixteenth 3+3+2, the women's "oo" in the grind and "ah" in the toll.
    m = SN
    desk = B.Desk(SN_BARS * m.bar)
    R = lambda *k: rng('score_beasts', 'Sentinel', *k)
    tk = _Takes(R('takes', rage), count=3)
    bt = m.beat
    rh = R('human', rage)
    if not rage:
        desk.track('timp', 0.0, -0.08, -12.0)
        desk.track('drums', -2.0, 0.1, -14.0)
        desk.track('stone', -7.0, -0.2, -11.0)
        desk.track('grind', -13.0, 0.25, -9.0)
        desk.track('bones', -2.0, 0.0, -10.0)
        desk.track('tuba', -6.0, 0.45, -13.0)
        desk.track('horns', -2.0, 0.0, -8.0)
        desk.track('choir', -4.0, 0.0, -6.0)
        desk.track('basses', -8.0, 0.0, -14.0)
        desk.track('cellos', -5.0, 0.0, -10.0)
        desk.track('violas', -10.0, 0.0, -9.0)
        desk.track('metal', -13.0, 0.2, -8.0)
    else:
        desk.track('horns', 0.0, 0.0, -8.0, _hp(120.0))
        desk.track('timp', -3.0, -0.1, -12.0, _hp(110.0))
        desk.track('drums', -6.0, 0.15, -13.0, _hp(110.0))
        desk.track('spic', -7.0, 0.0, -10.0, _hp(120.0))
        desk.track('women', -6.0, 0.0, -6.0, _hp(120.0))

    for bar in range(SN_BARS):
        part, lift, ch = SN_PART[bar], SN_LIFT[bar], SN_CH[SN_CHORD[bar]]
        t = m.at(bar)
        if not rage:
            # Timpani on the lurch: root, the second drum, root; ghosts in the grind, pickups in the toll.
            if part == 'breath':
                hits = [(0, 0, 0.45)] if bar == 12 else [(12, 1, 0.32)]
            elif bar == 0:
                hits = [(0, 0, 1.0)]
            elif bar == 1:
                hits = [(0, 0, 0.72), (12, 1, 0.62)]
            else:
                hits = [(0, 0, 0.95), (6, 1, 0.74), (12, 0, 0.82)]
                if part == 'grind':
                    hits += [(9, 1, 0.4), (15, 1, 0.45)]
                if part == 'toll':
                    hits += [(14, 1, 0.55), (15, 1, 0.68)]
            for s, drum, v in hits:
                desk.add('timp', O.timpani(ch['timp'][drum], None, B.lively(rh, v * (0.7 + 0.35 * lift), 0.05),
                                           0.45, takes=tk), m.at(bar, s) + B.human(rh, 0.004))
            if bar == SN_BARS - 1:
                desk.add('timp', O.timpani_roll('E2', 6 * m.step, R('roll'), 0.3, 0.9, hit=False), m.at(bar, 10))
            # The bass drum on the downbeats, the war drum on the lurch, the stone knocking.
            if part in ('march', 'grind', 'toll') or bar in (0, 2, 8, 10):
                desk.add('drums', O.bass_drum(None, 0.88 * lift, 3.0, takes=tk), t + B.human(rh, 0.003))
            if part == 'toll':
                desk.add('drums', O.bass_drum(None, 0.7 * lift, 3.0, takes=tk), m.at(bar, 12))
            war = (6, 12) if part in ('march', 'grind', 'toll') else ((12,) if part == 'quote' or bar >= 2 else ())
            for s in war:
                if part == 'breath':
                    break
                desk.add('drums', O.taiko(None, (0.72 if s == 6 else 0.8) * lift, 1.2, takes=tk),
                         m.at(bar, s) + B.human(rh, 0.004), pan=0.2)
            knocks = []
            if bar == 0:
                knocks = [(0, 1.4, 0.9)]
            elif part in ('march', 'grind', 'toll'):
                knocks = [(6, 1.3, 0.8), (12, 1.0, 0.7)] + ([(3, 0.8, 0.5), (10, 0.8, 0.5)] if part == 'toll' else [])
            elif part in ('wake', 'quote'):
                knocks = [(12, 1.2, 0.65)] if bar >= 2 else []
            for s, size, v in knocks:
                desk.add('stone', O.stone(None, v * (0.75 + 0.3 * lift), size, takes=tk),
                         m.at(bar, s) + B.human(rh, 0.005), pan=-0.25 if size > 1.1 else 0.3)
            # The brass: chorale, stabs, the toll's sfz chords.
            if bar == 0:
                desk.add('bones', *O.brass_section('trombone', ch['bones'], R('b', bar), 1.2, 0.92, 'sfz', t))
                desk.add('tuba', O.tuba(ch['root'], 1.2, R('tu', bar), 0.88, 'sfz'), t)
            elif bar == 3 or bar == 19:
                desk.add('bones', *O.brass_section('trombone', ch['bones'], R('b', bar), m.bar, 0.78, 'cresc', t))
                desk.add('tuba', O.tuba(ch['root'], m.bar, R('tu', bar), 0.72, 'cresc'), t)
            elif part == 'march':
                desk.add('bones', *O.brass_section('trombone', ch['bones'], R('b', bar), m.bar - 0.05, 0.84, 'fp', t))
                desk.add('tuba', O.tuba(ch['root'], m.bar - 0.05, R('tu', bar), 0.78, 'fp'), t)
            elif part == 'grind':
                for s in (0, 12):
                    desk.add('bones', *O.brass_section('trombone', ch['bones'], None, 0.5, 0.78 * lift, 'marc',
                                                       m.at(bar, s), takes=tk))
                    desk.add('tuba', O.tuba(ch['root'], 0.5, R('tu', bar, s), 0.72 * lift, 'marc'), m.at(bar, s))
            elif part == 'toll':
                art = 'cresc' if bar == SN_BARS - 1 else 'sfz'
                desk.add('bones', *O.brass_section('trombone', ch['bones'], R('b', bar), m.bar - 0.05, 0.94, art, t))
                desk.add('tuba', O.tuba(ch['root'], m.bar - 0.05, R('tu', bar), 0.85, art), t)
                desk.add('horns', *O.brass_section('horn', ch['vla'], R('h', bar), m.bar - 0.05, 0.86, art, t))
            # The breath's soft colours.
            if bar == 12:
                desk.add('violas', *O.string_tremolo('violas', ['A3', 'C4'], 2 * m.bar, R('trem'), 0.36, t,
                                                     dyn='swell', pont=0.55))
                desk.add('grind', O.stone_grind(2 * m.bar, R('grind', bar), 0.55, 1.5), t)
            if bar in (1, 3, 19):
                at = t if bar == 1 else m.at(bar, 6)
                desk.add('grind', O.stone_grind(m.bar if bar == 1 else 1.4, R('grind', bar), 0.5, 1.4), at)
        else:
            # Faster timpani rolls on the upper drums: into each downbeat, into the beats in the toll, out of the breath.
            up = 'A3' if SN_CHORD[(bar + 1) % SN_BARS] in ('A', 'F', 'Dm') else 'E3'
            if bar == 13:
                desk.add('timp', O.timpani_roll('E3', m.bar, R('roll', bar), 0.12, 0.85, rate=17.0, hit=False), t)
            elif part != 'breath' and bar >= 2:
                desk.add('timp', O.timpani_roll(up, 4 * m.step, R('roll', bar), 0.28, 0.85, rate=18.0, hit=False),
                         m.at(bar, 12))
                if part in ('grind', 'toll'):
                    desk.add('timp', O.timpani_roll('E3', 3 * m.step, R('roll6', bar), 0.25, 0.7, rate=18.0, hit=True),
                             m.at(bar, 3))
                if part == 'toll':
                    desk.add('timp', O.timpani_roll('A3', 2 * m.step, R('roll12', bar), 0.3, 0.75, rate=19.0,
                                                    hit=True), m.at(bar, 8))
            # War drums on the sixteenth 3+3+2.
            if part in ('grind', 'toll') or (part == 'quote'):
                for s in (ACC16 if part != 'quote' else (8, 11, 14)):
                    desk.add('drums', O.taiko(None, (0.8 if s in (0, 8) else 0.6) * lift, 0.5, takes=tk),
                             m.at(bar, s) + B.human(rh, 0.004), pan=0.25 if s % 2 else -0.1)
            # Double-time strings: violins on the chord's figure, violas an octave under on the eighths.
            if part in ('march', 'quote', 'grind', 'toll') or bar == 3:
                step = 2 if part in ('march', 'quote', 'wake') else 1
                for k, s in enumerate(range(0, 16, step)):
                    v = (0.75 if s in ACC8 else 0.45) * lift * (0.6 + 0.4 * s / 16 if bar == 3 else 1.0)
                    note = ch['arp'][k % 4]
                    desk.add('spic', *O.string_spiccato('violins', note, None, v, m.at(bar, s), players=6,
                                                        length=0.1, takes=tk))
                    if s % 2 == 0 and part in ('grind', 'toll'):
                        low = _hz(_fold(_midi(note) - 12, 'A3', 'G#4'))
                        desk.add('spic', *O.string_spiccato('violas', low, None, v * 0.9, m.at(bar, s), players=4,
                                                            length=0.12, takes=tk))
            # The horns in fifths.
            if bar == 0:
                desk.add('horns', *O.brass_section('horn', list(ch['fifth']), R('f', bar), 1.4, 0.88, 'sfz', t))
            elif part in ('wake', 'march', 'quote') and bar not in (8,):
                desk.add('horns', *O.brass_section('horn', list(ch['fifth']), R('f', bar), m.bar - 0.05,
                                                   0.55 + 0.3 * lift, 'swell' if part != 'march' else 'fp', t))
            elif bar == 12:
                desk.add('horns', *O.brass_section('horn', ['A3', 'E4'], R('f', bar), 2 * m.bar, 0.4, 'swell', t))
            elif bar in SN_ORGANUM:
                for (s, beats), lo in zip(((0, 1.5), (6, 1.5), (12, 1.0)), SN_ORGANUM[bar]):
                    pair = [lo, _hz(_midi(lo) + 7)]
                    v = (0.86 if s == 0 else 0.76) * lift
                    desk.add('horns', *O.brass_section('horn', pair, R('o', bar, s), beats * bt - 0.04, v,
                                                       'marc', m.at(bar, s)))
            elif bar in (19, 23):
                desk.add('horns', *O.brass_section('horn', list(ch['fifth']), R('f', bar), m.bar - 0.05, 0.85,
                                                   'cresc', t))

    # Lines that run across bars: the men, the floor, the cellos, the violas, the quote, the theme (base); the women
    # (rage).
    vel = lambda b: 0.3 + 0.4 * SN_LIFT[b]
    new = lambda b: b == 0 or SN_PART[b] != SN_PART[b - 1]
    if not rage:
        sung = _runs(SN_CHORD, SN_PART, [b for b in range(1, SN_BARS) if SN_PART[b] != 'breath'])
        for k in (0, 1):
            ln = _voice_line(m, sung, lambda c: B.hz(SN_CH[c]['men'][k]), vel, 0.05, new)
            desk.add('choir', *O.choir_line('bass', ln, R('men', k), 'oh', voices=6))
        desk.add('choir', *O.choir('men', ['A2', 'E3'], 2 * m.bar, R('hum'), 'mm', 6, 0.36, m.at(12), 'swell'))
        floor = _runs(SN_CHORD, SN_PART, list(range(SN_BARS)))
        desk.add('basses', *O.string_legato('basses', _voice_line(m, floor, lambda c: B.hz(SN_CH[c]['root']),
                                                                  lambda b: 0.3 + 0.45 * SN_LIFT[b], 0.0, new),
                                            R('floor')))
        held = _runs(SN_CHORD, SN_PART, [b for b in range(SN_BARS) if SN_PART[b] in ('wake', 'march', 'quote', 'toll')])
        for k in (0, 1):
            ln = _voice_line(m, held, lambda c: B.hz(SN_CH[c]['cel'][k]), vel, 0.05, new)
            desk.add('cellos', *O.string_legato('cellos', ln, R('cel', k), players=3))
        inner = _runs(SN_CHORD, SN_PART, [b for b in range(SN_BARS) if SN_PART[b] in ('march', 'toll')])
        for k in (0, 1):
            ln = _voice_line(m, inner, lambda c: B.hz(SN_CH[c]['vla'][k]), vel, 0.05, new)
            desk.add('violas', *O.string_legato('violas', ln, R('vla', k), players=3))
        quote = _sn_quote()
        desk.add('bones', *O.brass_section('trombone', _mel(quote, m.at(8), bt, 0.88), R('quote'), players=3))
        desk.add('horns', *O.brass_section('horn', _mel(quote, m.at(8), bt, 0.8, 12.0), R('quote2'), players=4))
        for bar in (8, 9, 11):
            desk.add('tuba', O.tuba(SN_CH[SN_CHORD[bar]]['root'], (2 if bar == 9 else 1) * m.bar - 0.05,
                                    R('qt', bar), 0.7, 'swell'), m.at(bar))
        desk.add('horns', *O.brass_section('horn', _mel(SN_THEME, m.at(14), bt, 0.8), R('theme'), players=4))
        desk.add('cellos', *O.string_legato('cellos', _mel(SN_THEME, m.at(14), bt, 0.78, -12.0, gap=0.0), R('ctheme')))
        desk.add('metal', O.tam_tam(R('tam', 0), 0.6, 8.0), 0.0)
        desk.add('metal', O.tam_tam(R('tam', 8), 0.45, 8.0), m.at(8))
        desk.add('metal', O.tam_tam(R('tam', 20), 0.7, 8.0), m.at(20))
        for bar, v in ((19, 0.6), (23, 0.72)):
            desk.add('metal', O.cymbal_swell(m.bar, R('cym', bar), v, 2.5), m.at(bar))
    else:
        for k in (0, 1):
            grind = _runs(SN_CHORD, SN_PART, [b for b in range(SN_BARS) if SN_PART[b] in ('grind', 'toll')])
            ln = _voice_line(m, grind, lambda c: B.hz(SN_CH[c]['women'][k]), lambda b: 0.38 + 0.4 * SN_LIFT[b]
                             if SN_PART[b] == 'toll' else 0.4, 0.05, new)
            for note in ln.notes:
                if note[0] >= m.at(14) - 0.01 and note[0] < m.at(20) - 0.01:
                    note[4]['vowel'] = 'oo'
            desk.add('women', *O.choir_line('women', ln, R('women', k), 'ah', voices=6))
    return desk.mix(O.orchestra_hall(R('hall')), _bus)


@cue('Music.Boss.Sentinel', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-14.0,
     align=False)
def boss_sentinel(v, r):
    return _sentinel(False)


@cue('Music.Boss.Sentinel.Rage', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0,
     align=False)
def boss_sentinel_rage(v, r):
    return _sentinel(True)
