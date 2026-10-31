"""The men's boss fights on the orchestra (score round 1, 2026-10-31): Abel Ransom, the Keeper, Ellis's Pa; Cyrus
Teague, the Foreman of the Providence mine; and the sting when any boss falls.

    Music.Boss.Keeper / .Rage     D minor, 4/4 at 128, 32 bars (60 s): the chapel bell, full orchestra and choir, the
                                  band's reed singing the Rest's call slowed into a lament; the game's motif in the horns
    Music.Boss.Foreman / .Rage    C minor, 4/4 at 112, 32 bars (68.57 s): hammers on rail and anvil on the beat, low
                                  brass and tuba, the dead shift's work chant, the mine bell, driving low strings; the
                                  motif stolen into a company march in the trombones
    Music.Sting.BossVictory       one-shot, ~7 s: brass and strings climb Bb, C and land on D major with the bell, then
                                  a soft held chord

Each fight is one score function that renders either part: 'base' (a whole piece alone) or 'rage' (the last phase's
stem, started with the base and faded in on a bar line). Both parts draw every random choice from generators named by
the piece and the layer (not by the cue), share the same hall and the same harmony, so together they are one bigger
arrangement. The rage stems carry no bass: every rage track goes through a 120 Hz high-pass.

Steps are sixteenths (band.Meter(bpm, 4, 4)); notes in the tables are (bar, step, sixteenths, note[, options]).
"""
import numpy as np

from lib.core import rng
from lib import filters as F
from recipes import cue
from recipes import band as B
from recipes import orchestra as O

BARS = 32
KM = B.Meter(128.0, 4, 4)
FM = B.Meter(112.0, 4, 4)


def _g(*parts):
    """A generator named by the piece and layer: the base and its rage stem draw the same numbers."""
    return rng('score_men', *parts)


def _hp(freq):
    return lambda x, c: F.filt(x, F.hpn(freq, 2), extend=False, circular=c)


def _spans(harm):
    """(bar, step, sixteenths, chord) from a harmony list: 'Dm' fills a bar, 'Dm|Bb' splits it in halves."""
    out = []
    for bar, h in enumerate(harm):
        parts = h.split('|')
        k = 16 // len(parts)
        out += [(bar, i * k, k, c) for i, c in enumerate(parts)]
    return out


def _line(m, notes, vel=0.7, semis=0.0, bar0=0, **common):
    """An orchestra Line from (bar, step, sixteenths, note[, options]); bar0 moves it; options may carry 'vel'."""
    ln = O.Line()
    k = 2.0 ** (semis / 12.0)
    for bar, step, length, note, *rest in notes:
        o = dict(common)
        o.update(rest[0] if rest else {})
        v = o.pop('vel', vel)
        ln.note(m.at(bar + bar0, step), length * m.step, B.hz(note) * k, v, **o)
    return ln


def _reed_phrase(m, notes, bar0=0):
    p = B.Phrase()
    for bar, step, length, note, *rest in notes:
        p.note(m.at(bar + bar0, step), length * m.step, note, **(rest[0] if rest else {}))
    return p


def _voice(m, spans, table, key, i, vel, lo, hi, semis=0.0, **opts):
    """One voice of a chord table as a Line over bars lo..hi-1, a held note where the chords share it (a chorale)."""
    ln = O.Line()
    k = 2.0 ** (semis / 12.0)
    for bar, step, steps, c in spans:
        if not lo <= bar < hi:
            continue
        val = table[c][key]
        f = B.hz(val if isinstance(val, str) else val[i]) * k
        t, length = m.at(bar, step), steps * m.step
        last = ln.notes[-1] if ln.notes else None
        if last and abs(last[2] - f) < 0.01 and abs(last[0] + last[1] - t) < 1e-6:
            last[1] += length
        else:
            ln.note(t, length, f, vel, **opts)
    return ln


def _chords(spans, lo, hi):
    """Spans in bars lo..hi-1 with repeated chords merged: (t-bar, step, sixteenths, chord)."""
    out = []
    for bar, step, steps, c in spans:
        if not lo <= bar < hi:
            continue
        if out and out[-1][3] == c and out[-1][0] * 16 + out[-1][1] + out[-1][2] == bar * 16 + step:
            out[-1] = (out[-1][0], out[-1][1], out[-1][2] + steps, c)
        else:
            out.append((bar, step, steps, c))
    return out


def _bell(tk, prime, vel, t60=6.0, dur=6.0, bright=0.85):
    """The chapel's (or the mine's) bell through round-robin takes."""
    q = max(0.1, round(vel * 10.0) / 10.0)
    x = tk.get(('bell', prime, t60, dur, bright, q), lambda rr: B.bell(prime, rr, q, t60=t60, dur=dur, bright=bright))
    return x * (vel / q)


def _semi(note, k):
    return B.hz(note) * 2.0 ** (k / 12.0)


# --- Abel Ransom, the Keeper ----------------------------------------------------------------------------------------

# A 0-7 the keeper stands (drive, horn chorale); B 8-15 the lament (the reed); 16-17 the breath (bell, reed, harp);
# C 18-25 the son answers (the motif in the horns, then the lament in the violins); D 26-31 to the door (stabs, build).
KEEPER_HARM = (['Dm', 'F', 'Gm', 'A', 'Bb', 'F', 'Gm', 'A'] * 2 + ['Dm', 'Dm'] +
               ['Dm', 'Dm|Bb', 'Bb|Gm', 'A', 'Dm', 'F', 'Gm', 'A'] + ['Bb', 'C', 'Dm', 'Bb', 'Gm', 'A'])

# Voicings: basses, cellos (the ostinato's root), horns, trombones, the men's and women's choir notes.
KEEPER_V = {
    'Dm': dict(cb='D2', vc='D3', timp='D2', hn=['A3', 'D4', 'F4'], tb=['D3', 'A3'], men=['D3', 'A3'], wom=['F4', 'D5']),
    'F': dict(cb='F2', vc='F2', timp='F2', hn=['A3', 'C4', 'F4'], tb=['C3', 'A3'], men=['C3', 'A3'], wom=['F4', 'C5']),
    'Gm': dict(cb='G1', vc='G2', timp='G2', hn=['Bb3', 'D4', 'G4'], tb=['D3', 'Bb3'], men=['D3', 'Bb3'],
               wom=['G4', 'D5']),
    'A': dict(cb='A1', vc='A2', timp='A2', hn=['A3', 'C#4', 'E4'], tb=['E3', 'C#4'], men=['E3', 'A3'], wom=['E4', 'C#5']),
    'Bb': dict(cb='Bb1', vc='Bb2', timp='Bb2', hn=['Bb3', 'D4', 'F4'], tb=['F3', 'D4'], men=['D3', 'Bb3'],
               wom=['F4', 'D5']),
    'C': dict(cb='C2', vc='C3', timp='C3', hn=['G3', 'C4', 'E4'], tb=['E3', 'C4'], men=['C3', 'G3'], wom=['E4', 'C5']),
}

# The lament: the Rest's call (RR_REED's opening, D C A G A) slowed to whole bars and grieving, sighing down to E over
# the dominant; then reaching to the F and down, the leading note left hanging for the next breath.
LAMENT_1 = [(0, 0, 12, 'D5', {'scoop': 1.0}), (0, 12, 4, 'C5'), (1, 0, 12, 'A4'), (1, 12, 4, 'G4'),
            (2, 0, 8, 'A4', {'vib': 0.22}), (2, 8, 4, 'G4'), (2, 12, 4, 'F4'), (3, 0, 12, 'E4', {'vib': 0.2})]
LAMENT_2 = [(4, 0, 12, 'F5', {'scoop': 1.0}), (4, 12, 4, 'D5'), (5, 0, 12, 'C5'), (5, 12, 4, 'A4'),
            (6, 0, 8, 'Bb4', {'vib': 0.2}), (6, 8, 4, 'A4'), (6, 12, 4, 'G4'), (7, 0, 8, 'A4'),
            (7, 8, 8, 'C#5', {'vib': 0.24})]
LAMENT_HOME = [(0, 0, 22, 'D5', {'vib': 0.18, 'fall': 1.0})]

# The game's motif (MAIN_MOTIF's D A G Bb) in the horns, doubled in length, heroic; then Bb A G to the half cadence.
KEEPER_QUOTE = [(18, 0, 8, 'D4'), (18, 8, 12, 'A4'), (19, 4, 4, 'G4'), (19, 8, 16, 'Bb4', {'dyn': 'swell'}),
                (20, 8, 4, 'A4'), (20, 12, 4, 'G4'), (21, 0, 16, 'A4', {'dyn': 'swell'})]


def _ksec(bar):
    return 'A' if bar < 8 else 'B' if bar < 16 else 'breath' if bar < 18 else 'C' if bar < 26 else 'D'


def _keeper(part):
    m = KM
    base = part == 'base'
    spans = _spans(KEEPER_HARM)
    V = KEEPER_V
    tk = O.Takes(_g('keeper', 'takes'), count=3)
    ts = O.Takes(_g('keeper', 'strings'), count=2)
    hum = _g('keeper', 'human', part)
    desk = B.Desk(BARS * m.bar)
    if base:
        desk.track('drums', 0.0, 0.0, -15.0)
        desk.track('timp', -4.0, -0.08, -12.0)
        desk.track('cym', -13.0, 0.15, -8.0, _hp(200.0))
        desk.track('bell', -7.0, -0.25, -6.0, _hp(100.0))
        desk.track('low', -4.0, 0.0, -16.0)
        desk.track('sus', -8.0, 0.0, -11.0)
        desk.track('pad', -8.0, 0.0, -10.0)
        desk.track('horn', -2.5, 0.0, -9.0)
        desk.track('tbn', -5.0, 0.0, -10.0)
        desk.track('tpt', -6.0, 0.0, -10.0)
        desk.track('vln', -2.5, 0.0, -9.0)
        desk.track('reed', -2.0, 0.15, -9.0)
        desk.track('choir', -5.0, 0.0, -7.0)
        desk.track('harp', -11.0, -0.5, -8.0)
    else:
        hp = _hp(120.0)
        desk.track('bell', -6.0, -0.25, -6.0, hp)
        desk.track('drums', -3.0, 0.1, -14.0, hp)
        desk.track('cym', -11.0, 0.15, -8.0, hp)
        desk.track('vln', 0.0, 0.0, -9.0, hp)
        desk.track('vla', -4.0, 0.0, -10.0, hp)
        desk.track('horn', -2.0, 0.0, -9.0, hp)
        desk.track('choir', -4.0, 0.0, -7.0, hp)
        desk.track('tpt', -4.0, 0.0, -9.0, hp)
        desk.track('spic', -7.0, 0.0, -12.0, hp)
        desk.track('trem', -12.0, 0.0, -8.0, hp)

    # --- Drums: the taiko's drive, the rims' backbeat, the bass drum on the big arrivals; the rage doubles in
    # small drums between them and rolls the last bar into the seam.
    for bar in range(BARS):
        sec = _ksec(bar)
        if base:
            if sec == 'A':
                hits = [(0, 0.95), (6, 0.7), (8, 0.85)] + ([(12, 0.6), (14, 0.75)] if bar % 4 == 3 else [])
            elif sec == 'B':
                hits = [(0, 0.7), (8, 0.5)] + ([(14, 0.45)] if bar % 4 == 3 else [])
            elif sec == 'C':
                hits = [(0, 0.95), (6, 0.72), (8, 0.88), (14, 0.7)]
            elif sec == 'D':
                hits = [(0, 0.95), (6, 0.75), (8, 0.85), (12, 0.7), (14, 0.8)]
                if bar == 31:
                    hits = [(0, 0.95), (6, 0.75)] + [(s, 0.5 + 0.06 * (s - 8)) for s in range(8, 16)]
            else:
                hits = []
            for s, vel in hits:
                desk.add('drums', O.taiko(None, vel, takes=tk), m.at(bar, s) + B.human(hum, 0.003))
            if sec in ('A', 'C', 'D') and bar != 31:
                for s in (4, 12):
                    desk.add('drums', O.taiko_rim(None, 0.45, takes=tk), m.at(bar, s) + B.human(hum, 0.003),
                             gain_db=-4.0)
            if bar in (0, 18, 22, 26):
                desk.add('drums', O.bass_drum(None, 0.8, takes=tk), m.at(bar))
        else:
            if sec == 'breath':
                continue
            if bar == 31:
                hits = [(s, 0.4 + 0.035 * s) for s in range(16)]
            elif sec == 'B':
                hits = [(10, 0.5), (11, 0.55)]
            else:
                hits = [(2, 0.6), (10, 0.5), (11, 0.58)] + ([(14, 0.6), (15, 0.68)] if sec in ('C', 'D') else [])
            for s, vel in hits:
                desk.add('drums', O.taiko(None, vel, size=0.55, takes=tk), m.at(bar, s) + B.human(hum, 0.003))
            if bar != 31:
                for s in (7, 15):
                    desk.add('drums', O.taiko_rim(None, 0.4, takes=tk), m.at(bar, s) + B.human(hum, 0.003),
                             gain_db=-5.0)

    # --- The chapel bell: every four bars in the base; the rage tolls the bars between, so together it is every bar.
    for bar in range(BARS):
        if base and bar % 4 == 0:
            desk.add('bell', _bell(tk, 'D4', 0.5 if bar == 16 else 0.85), m.at(bar))
        elif not base and bar % 4:
            chord = [c for b, s, k, c in spans if b == bar][0]
            desk.add('bell', _bell(tk, 'D4', 0.45 if chord == 'A' else 0.62), m.at(bar))

    if base:
        _keeper_base(desk, m, spans, V, tk, ts, hum)
    else:
        _keeper_rage(desk, m, spans, V, tk, ts, hum)
    return desk.mix(O.orchestra_hall(_g('keeper', 'hall')), O.orchestra_bus)


def _keeper_base(desk, m, spans, V, tk, ts, hum):
    g = lambda *p: _g('keeper', 'base', *p)
    # Timpani on the roots at each bar of the drive, the rolls into C and into the seam.
    for bar, step, steps, c in spans:
        sec = _ksec(bar)
        if sec in ('A', 'C', 'D') and step == 0:
            desk.add('timp', O.timpani(V[c]['timp'], None, 0.85 if bar in (0, 18) else 0.62, 0.45, takes=tk),
                     m.at(bar))
        elif sec == 'B' and step == 0 and bar % 2 == 0:
            desk.add('timp', O.timpani(V[c]['timp'], None, 0.45, 0.3, takes=tk), m.at(bar))
    desk.add('timp', O.timpani_roll('A2', 2 * m.beat, g('roll17'), 0.12, 0.75, hit=False), m.at(17, 8))
    desk.add('timp', O.timpani_roll('A2', m.bar, g('roll31'), 0.2, 0.85, hit=False), m.at(31))
    # Cymbals: a soft crash on the seam, crashes at the climax, swells into them.
    for bar, vel in ((0, 0.45), (18, 0.65), (22, 0.5)):
        desk.add('cym', O.cymbal_crash(g('crash', bar), vel, 4.5), m.at(bar))
    for bar in (18, 32):
        desk.add('cym', O.cymbal_swell(m.bar, g('swell', bar), 0.6, 1.5), m.at(bar - 1))

    # Low strings: cellos in eighths (root, octave, fifth), basses on the beats; sustained in the lament and the breath.
    for bar, step, steps, c in spans:
        sec = _ksec(bar)
        r0 = B.hz(V[c]['vc'])
        if sec in ('A', 'C', 'D'):
            for e in range(step // 2, (step + steps) // 2):
                if bar == 31 and e >= 4:
                    continue
                k = [0, 0, 12, 0, 7, 0, 12, 7][e]
                vel = 0.9 if e % 2 == 0 else 0.68
                desk.add('low', *O.string_spiccato('cellos', r0 * 2 ** (k / 12.0), None, vel, m.at(bar, 2 * e),
                                                   length=0.14, takes=ts))
            for b in range(step // 4, (step + steps) // 4):
                desk.add('low', *O.string_spiccato('basses', V[c]['cb'], None, 0.85 if b == 0 else 0.7,
                                                   m.at(bar, 4 * b), length=0.2, takes=ts))
        elif sec == 'B':
            for b in range(4):
                desk.add('low', *O.string_spiccato('cellos', r0, None, 0.55 if b == 0 else 0.42, m.at(bar, 4 * b),
                                                   length=0.16, takes=ts))
    # Bar 31: the cellos in sixteenths on A, climbing into the seam.
    for s in range(8, 16):
        desk.add('low', *O.string_spiccato('cellos', 'A2' if s < 12 else 'A3', None, 0.6 + 0.04 * (s - 8),
                                           m.at(31, s), length=0.1, takes=ts))
    desk.add('sus', *O.string_legato('basses', _voice(m, spans, V, 'cb', 0, 0.5, 8, 16,
                                                      dyn='swell'), g('cb_lament')))
    desk.add('sus', *O.string_sustain('basses', 'D2', 2 * m.bar, g('cb_breath'), 0.35, m.at(16), dyn='swell',
                                      attack=0.4, release=1.0))
    desk.add('sus', *O.string_sustain('cellos', ['D3', 'A3'], 2 * m.bar, g('vc_breath'), 0.32, m.at(16), dyn='swell',
                                      attack=0.5, release=1.0))

    # Pads: violas' tremolo on the horns' upper notes through the drive; in the lament violas held under the reed
    # with the altos' 'oo'; violins' tremolo over the motif; violins and violas swelling into the seam.
    for bar, step, steps, c in _chords(spans, 0, 8) + _chords(spans, 26, 30):
        desk.add('pad', *O.string_tremolo('violas', V[c]['hn'][1:], steps * m.step + 0.08, g('vla', bar, step), 0.42,
                                          m.at(bar, step), players=4, dyn='swell', attack=0.12))
    for bar, step, steps, c in _chords(spans, 8, 16):
        desk.add('pad', *O.string_sustain('violas', V[c]['hn'][:2], steps * m.step + 0.2, g('vla_l', bar), 0.36,
                                          m.at(bar, step), dyn='swell', attack=0.3, release=0.5))
        desk.add('choir', *O.choir('women', [V[c]['men'][1], V[c]['wom'][0]], steps * m.step + 0.15,
                                   g('oo', bar), 'oo', 4, 0.42, m.at(bar, step), 'swell'))
    for bar, step, steps, c in _chords(spans, 18, 22):
        desk.add('pad', *O.string_tremolo('violins', V[c]['wom'], steps * m.step + 0.08, g('vln_t', bar, step), 0.45,
                                          m.at(bar, step), dyn='cresc', attack=0.1))
    for bar, step, steps, c in _chords(spans, 30, 32):
        desk.add('pad', *O.string_tremolo('violins', V[c]['wom'], steps * m.step + 0.05, g('vln_up', bar), 0.55,
                                          m.at(bar, step), dyn='cresc', attack=0.1))
        desk.add('pad', *O.string_tremolo('violas', V[c]['hn'][1:], steps * m.step + 0.05, g('vla_up', bar), 0.5,
                                          m.at(bar, step), dyn='cresc', attack=0.1))

    # Horns: the chorale through the drive (three voices, held where the chords share a note), the motif at the
    # climax (unison), the chorale again under the violins' lament, stabs at the door.
    for lo, hi, vel in ((0, 8, 0.6), (22, 26, 0.55)):
        for i in range(3):
            desk.add('horn', *O.brass_section('horn', _voice(m, spans, V, 'hn', i, vel, lo, hi, dyn='swell'),
                                              g('chorale', lo, i), players=2))
    quote = _line(m, KEEPER_QUOTE, 0.82)
    desk.add('horn', *O.brass_section('horn', quote, g('quote'), players=4))
    desk.add('tbn', *O.brass_section('trombone', quote.moved(-12.0, vel=0.85), g('quote_tb'), players=3))
    reach = _line(m, [n for n in KEEPER_QUOTE if n[0] >= 19 and (n[0], n[1]) != (19, 4)], 0.7)
    desk.add('tpt', *O.brass_section('trumpet', reach, g('reach'), players=2))
    for bar in (0, 4, 22):
        c = [x[3] for x in spans if x[0] == bar][0]
        desk.add('tbn', *O.brass_section('trombone', V[c]['tb'], None, 3 * m.beat, 0.72, 'marc', m.at(bar),
                                         takes=tk))
    for bar, step, steps, c in spans:
        if _ksec(bar) != 'D':
            continue
        for s, vel in ((0, 0.85), (6, 0.72), (12, 0.8)):
            desk.add('horn', *O.brass_section('horn', V[c]['hn'], None, 0.3, vel, 'marc', m.at(bar, s), players=3,
                                              takes=tk))
            desk.add('tbn', *O.brass_section('trombone', V[c]['tb'], None, 0.3, vel, 'marc', m.at(bar, s),
                                             takes=tk))
        if bar == 31:
            desk.add('tpt', *O.brass_section('trumpet', ['A4', 'E5'], None, 0.25, 0.75, 'stac', m.at(31, 12),
                                             takes=tk))

    # The reed's lament (the Rest's call), and home on D in the breath.
    for i, (notes, bar0) in enumerate(((LAMENT_1, 8), (LAMENT_2, 8), (LAMENT_HOME, 16))):
        x, t0 = B.reed(_reed_phrase(m, notes, bar0), _g('keeper', 'reed', i))
        desk.add('reed', x, t0, -6.0 if bar0 == 16 else 0.0)
    # The violins take the lament up at the climax, an octave over the violas.
    lam = _line(m, LAMENT_1, 0.62, 12.0, bar0=22)
    desk.add('vln', *O.string_legato('violins', lam, g('vln_lament')))
    desk.add('vln', *O.string_legato('violas', lam.moved(-12.0, vel=0.95), g('vla_lament')))

    # Choir: the hum in the breath, the full choir on the climax, the build into the seam.
    desk.add('choir', *O.choir('men', ['D3', 'A3'], 2 * m.bar, g('hum'), 'mm', 6, 0.38, m.at(16), 'swell'))
    for bar, step, steps, c in _chords(spans, 18, 26) + _chords(spans, 30, 32):
        vel = 0.52 if bar < 22 else 0.6 if bar < 26 else 0.62
        dyn = 'cresc' if bar >= 30 else 'swell'
        desk.add('choir', *O.choir('men', V[c]['men'], steps * m.step + 0.15, g('men', bar, step), 'ah', 5, vel,
                                   m.at(bar, step), dyn))
        desk.add('choir', *O.choir('women', V[c]['wom'], steps * m.step + 0.15, g('wom', bar, step), 'ah', 5, vel,
                                   m.at(bar, step), dyn))
    # The harp in the breath: Dm rising and falling.
    rh = g('harp')
    for k, note in enumerate(['D3', 'A3', 'D4', 'F4', 'A4', 'F4', 'D4', 'A3']):
        desk.add('harp', O.harp(note, rh, 0.5 if k else 0.6), m.at(16, 4 * k) + B.human(rh, 0.004))


def _keeper_rage(desk, m, spans, V, tk, ts, hum):
    g = lambda *p: _g('keeper', 'rage', *p)
    # The whole orchestra on the lament: violins with the reed, violas and horns an octave under, the sopranos.
    lam = O.Line()
    for notes, bar0 in ((LAMENT_1, 0), (LAMENT_2, 0), (LAMENT_1, 8), (LAMENT_2, 8)):
        lam.notes += _line(m, notes, 0.66, bar0=bar0).notes
    lam.notes += _line(m, LAMENT_HOME, 0.6, bar0=16, dyn='dim').notes
    desk.add('vln', *O.string_legato('violins', lam, g('vln')))
    desk.add('vla', *O.string_legato('violas', lam.moved(-12.0), g('vla')))
    desk.add('horn', *O.brass_section('horn', lam.moved(-12.0, vel=0.95), g('horn'), players=4))
    desk.add('choir', *O.choir_line('women', lam.moved(0.0, vel=0.85), g('sop'), 'ah', 6))
    # At the climax, over the base's violins: horns under them and the sopranos with them.
    lam2 = _line(m, LAMENT_1, 0.66, bar0=22)
    desk.add('horn', *O.brass_section('horn', lam2.moved(-12.0), g('horn2'), players=4))
    desk.add('choir', *O.choir_line('women', lam2.moved(0.0, vel=0.85), g('sop2'), 'ah', 8))
    # High brass: the trumpets an octave over the horns' motif, stabs with them at the door; the sopranos held over it.
    desk.add('tpt', *O.brass_section('trumpet', _line(m, KEEPER_QUOTE, 0.6, 12.0), g('quote'), players=3))
    for bar, step, steps, c in spans:
        if _ksec(bar) != 'D':
            continue
        for s, vel in ((0, 0.75), (6, 0.62), (12, 0.7)):
            desk.add('tpt', *O.brass_section('trumpet', V[c]['wom'], None, 0.3, vel, 'marc', m.at(bar, s), players=3,
                                             takes=tk))
    for bar, step, steps, c in _chords(spans, 26, 32):
        desk.add('choir', *O.choir('women', V[c]['wom'], steps * m.step + 0.15, g('d', bar, step), 'ah', 6,
                                   0.5 if bar < 30 else 0.6, m.at(bar, step), 'cresc' if bar >= 30 else 'swell'))
    # Double-time strings: the second violins in sixteenths on the chord (not in the breath).
    for bar, step, steps, c in spans:
        if _ksec(bar) == 'breath':
            continue
        hn = V[c]['hn']
        for s in range(step, step + steps):
            note = hn[[1, 0, 2, 0][s % 4]]
            vel = 0.52 if s % 4 == 0 else 0.36
            desk.add('spic', *O.string_spiccato('violins', note, None, vel, m.at(bar, s), players=6, pan=-0.15,
                                                width=0.3, length=0.09, takes=ts))
    # The breath keeps a thread of tension: high violins' tremolo, very soft.
    desk.add('trem', *O.string_tremolo('violins', ['A4', 'D5'], 2 * m.bar, g('trem'), 0.3, m.at(16), dyn='swell',
                                       pont=0.3, attack=0.6, release=0.4))
    for bar in (8, 18, 32):
        desk.add('drums', O.cymbal_swell(m.bar, g('swell', bar), 0.55, 1.2), m.at(bar - 1))


@cue('Music.Boss.Keeper', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-14.0,
     align=False)
def boss_keeper(v, r):
    # Abel at the burial boards: tragic and heroic, a son fighting his father. Bars 1-8 the keeper stands: taiko
    # drive, cellos in eighths, the horns' chorale over violas' tremolo, the chapel bell every four bars. Bars 9-16 the
    # band's reed sings the Rest's call slowed into a lament over held strings and the altos' 'oo'. Bars 17-18 the
    # breath: the bell, the reed home on D, a harp, the men humming, a timpani roll. Bars 19-26 the son answers: the
    # game's motif in the horns and trombones, the trumpets on its reach, the full choir; the violins take the
    # lament. Bars 27-32 to the door: brass stabs 3+3+2, the roll and the swell into the seam's downbeat.
    return _keeper('base')


@cue('Music.Boss.Keeper.Rage', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0,
     align=False)
def boss_keeper_rage(v, r):
    # His last phase, on top of the base: the whole orchestra on the lament (violins, violas, horns, sopranos), the bell
    # every bar, double-time second violins, small drums between the taiko's, trumpets an octave over the motif and
    # stabbing at the door. No bass.
    return _keeper('rage')


# --- Cyrus Teague, the Foreman --------------------------------------------------------------------------------------

# A 0-7 the shift (hammers, chant on the one, the tuba); B 8-15 the company (the motif made a march in the trombones
# and tuba); 16-17 the breath (the mine bell's code, a hum, one far hammer); C 18-25 the dead shift (chant on 1 and 3,
# the horns' hymn); D 26-31 the charge (stabs 3+3+2, the chant every beat, the grind on the dominant).
FOREMAN_HARM = (['Cm', 'Cm', 'Ab', 'G', 'Cm', 'Cm', 'Db', 'G'] + ['Cm', 'Fm', 'Ab', 'G', 'Cm', 'Fm', 'Db', 'G'] +
                ['Cm', 'Cm'] + ['Cm', 'Ab', 'Fm', 'G', 'Cm', 'Ab', 'Db', 'G'] + ['Fm', 'Ab', 'Db', 'Db', 'G', 'G'])

FOREMAN_V = {
    'Cm': dict(cb='C2', tuba='C2', timp='C2', vc=['C3', 'C3', 'C3', 'Db3', 'C3', 'C3', 'Eb3', 'Db3'],
               tb=['C3', 'G3', 'Eb4'], hn=['G3', 'C4', 'Eb4'], men=['C3', 'G3'], ten=['G3', 'C4'],
               tpt=['G4', 'C5', 'Eb5'], vla=['Eb4', 'G4']),
    'Fm': dict(cb='F2', tuba='F1', timp='F2', vc=['F2', 'F2', 'C3', 'F2', 'F2', 'F2', 'Ab2', 'G2'],
               tb=['C3', 'Ab3', 'F4'], hn=['Ab3', 'C4', 'F4'], men=['F2', 'C3'], ten=['Ab3', 'C4'],
               tpt=['Ab4', 'C5', 'F5'], vla=['F4', 'Ab4']),
    'Ab': dict(cb='Ab1', tuba='Ab1', timp='Ab2', vc=['Ab2', 'Ab2', 'Eb3', 'Ab2', 'Ab2', 'Ab2', 'C3', 'Bb2'],
               tb=['Eb3', 'Ab3', 'C4'], hn=['Ab3', 'C4', 'Eb4'], men=['Ab2', 'Eb3'], ten=['Ab3', 'C4'],
               tpt=['Ab4', 'C5', 'Eb5'], vla=['Eb4', 'Ab4']),
    'Db': dict(cb='Db2', tuba='Db2', timp='Db3', vc=['Db3', 'Db3', 'Ab2', 'Db3', 'Db3', 'Db3', 'F3', 'Eb3'],
               tb=['Db3', 'Ab3', 'F4'], hn=['Ab3', 'Db4', 'F4'], men=['Db3', 'Ab3'], ten=['Ab3', 'Db4'],
               tpt=['Ab4', 'Db5', 'F5'], vla=['F4', 'Ab4']),
    'G': dict(cb='G2', tuba='G1', timp='G2', vc=['G2', 'G2', 'G2', 'Ab2', 'G2', 'G2', 'B2', 'Ab2'],
              tb=['B2', 'G3', 'D4'], hn=['G3', 'B3', 'D4'], men=['G2', 'D3'], ten=['G3', 'B3'],
              tpt=['G4', 'B4', 'D5'], vla=['D4', 'G4']),
}

# The company march (bars 9-16): the game's motif (D A G Bb, in the motif's own rhythm) taken down a step to C G F Ab,
# low and heavy, and carried on into the Foreman's own tune.
FOREMAN_MARCH = [(8, 0, 4, 'C3'), (8, 4, 6, 'G3'), (8, 10, 2, 'F3'), (8, 12, 8, 'Ab3'), (9, 4, 4, 'G3'),
                 (9, 8, 4, 'F3'), (9, 12, 4, 'Eb3'), (10, 0, 8, 'C3'), (10, 8, 4, 'Eb3'), (10, 12, 4, 'C3'),
                 (11, 0, 8, 'D3'), (11, 8, 4, 'B2'), (11, 12, 4, 'G2'), (12, 0, 4, 'C3'), (12, 4, 4, 'Eb3'),
                 (12, 8, 8, 'G3'), (13, 0, 8, 'Ab3'), (13, 8, 4, 'G3'), (13, 12, 4, 'F3'), (14, 0, 6, 'F3'),
                 (14, 6, 2, 'Eb3'), (14, 8, 8, 'Db3'), (15, 0, 6, 'C3'), (15, 6, 10, 'B2', {'dyn': 'swell'})]

# The horns' hymn over the dead shift (bars 19-26).
FOREMAN_HYMN = [(18, 0, 8, 'G4'), (18, 8, 4, 'F4'), (18, 12, 4, 'Eb4'), (19, 0, 8, 'C4'), (19, 8, 8, 'Eb4'),
                (20, 0, 8, 'F4'), (20, 8, 4, 'Ab4'), (20, 12, 4, 'G4'), (21, 0, 12, 'G4', {'dyn': 'swell'}),
                (22, 0, 8, 'C5'), (22, 8, 4, 'Bb4'), (22, 12, 4, 'Ab4'), (23, 0, 8, 'Ab4'), (23, 8, 4, 'G4'),
                (23, 12, 4, 'F4'), (24, 0, 8, 'F4'), (24, 8, 8, 'Ab4'), (25, 0, 16, 'G4', {'dyn': 'swell'})]


def _fsec(bar):
    return 'A' if bar < 8 else 'B' if bar < 16 else 'breath' if bar < 18 else 'C' if bar < 26 else 'D'


def _chant(tk, part, notes, vel, vowel='hah'):
    """A work-chant syllable ('hah' on the blow, 'hm' after it) through takes: (signal, offset)."""
    q = max(0.1, round(vel * 10.0) / 10.0)
    x, off = tk.get(('chant', part, tuple(notes), vowel, q),
                    lambda rr: O.choir(part, notes, 0.34, rr, vowel, 5, q, 0.0, 'chant', width=0.7, breath=1.3))
    return x * (vel / q), off


def _foreman(part):
    m = FM
    base = part == 'base'
    spans = _spans(FOREMAN_HARM)
    V = FOREMAN_V
    tk = O.Takes(_g('foreman', 'takes'), count=2)
    ts = O.Takes(_g('foreman', 'strings'), count=2)
    hum = _g('foreman', 'human', part)
    desk = B.Desk(BARS * m.bar)
    if base:
        desk.track('hammer', -2.0, 0.2, -11.0)
        desk.track('drums', 0.0, 0.0, -15.0)
        desk.track('timp', -5.0, -0.08, -12.0)
        desk.track('metal', -10.0, -0.1, -7.0, _hp(60.0))
        desk.track('bell', -8.0, 0.3, -6.0, _hp(150.0))
        desk.track('low', -2.0, 0.0, -16.0)
        desk.track('sus', -8.0, 0.0, -11.0)
        desk.track('pad', -10.0, 0.0, -10.0)
        desk.track('tuba', -4.0, 0.45, -12.0)
        desk.track('tbn', -2.0, 0.0, -10.0)
        desk.track('horn', -3.0, 0.0, -9.0)
        desk.track('chant', -3.0, 0.0, -8.0)
        desk.track('hum', -9.0, 0.0, -7.0)
    else:
        hp = _hp(120.0)
        desk.track('hammer', -1.0, -0.2, -11.0, hp)
        desk.track('drums', -4.0, 0.1, -14.0, hp)
        desk.track('tpt', 0.0, 0.0, -9.0, hp)
        desk.track('horn', -4.0, 0.0, -9.0, hp)
        desk.track('chant', -3.0, 0.0, -8.0, hp)
        desk.track('spic', -5.0, 0.0, -12.0, hp)
        desk.track('trem', -11.0, 0.0, -8.0, hp)
        desk.track('metal', -10.0, 0.1, -8.0, hp)

    # --- The hammers: a sledge on rail on 1 and 3, the anvil on 2 and 4 (tuned to the key's fifth, G); the rage's
    # second gang strikes between, on the offbeat eighths. In the breath one far hammer in the base, one miner still
    # digging in the rage.
    for bar in range(BARS):
        sec = _fsec(bar)
        if base:
            if sec == 'breath':
                if bar == 17:
                    desk.add('hammer', O.anvil(None, 0.4, 'rail', 392.0, takes=tk), m.at(17), gain_db=-6.0)
                continue
            for s in (0, 8):
                desk.add('hammer', O.anvil(None, 0.9 if s == 0 else 0.8, 'rail', 392.0, takes=tk),
                         m.at(bar, s) + B.human(hum, 0.004))
            for s in (4, 12):
                desk.add('hammer', O.anvil(None, 0.7, 'anvil', 784.0, takes=tk), m.at(bar, s) + B.human(hum, 0.004),
                         gain_db=-3.0)
        else:
            if sec == 'breath':
                for s in (0, 4, 8, 12):
                    desk.add('hammer', O.anvil(None, 0.4, 'anvil', 523.0, takes=tk), m.at(bar, s) + B.human(hum, 0.01),
                             gain_db=-8.0)
                continue
            for s in (2, 6, 10, 14):
                desk.add('hammer', O.anvil(None, 0.7 if s in (6, 14) else 0.6, 'anvil', 523.0, takes=tk),
                         m.at(bar, s) + B.human(hum, 0.004))
            if sec in ('C', 'D'):
                for s in (0, 8):
                    desk.add('hammer', O.anvil(None, 0.6, 'rail', 587.0, takes=tk), m.at(bar, s) + 0.012,
                             gain_db=-5.0)

    # --- Drums: taiko and bass drum stamping with the rail, the rage's small drums between.
    for bar in range(BARS):
        sec = _fsec(bar)
        if base:
            if sec == 'breath':
                continue
            hits = [(0, 0.95), (8, 0.8)]
            if sec in ('C', 'D'):
                hits += [(6, 0.6), (14, 0.7)]
            if bar == 31:
                hits = [(0, 0.95), (8, 0.8)] + [(s, 0.45 + 0.07 * (s - 9)) for s in range(9, 16)]
            for s, vel in hits:
                desk.add('drums', O.taiko(None, vel, takes=tk), m.at(bar, s) + B.human(hum, 0.003))
            if bar % 2 == 0 or sec == 'D':
                desk.add('drums', O.bass_drum(None, 0.75, takes=tk), m.at(bar), gain_db=-2.0)
        else:
            if sec == 'breath':
                continue
            hits = [(3, 0.55), (11, 0.55)] + ([(14, 0.6), (15, 0.66)] if sec in ('C', 'D') else [])
            if bar == 31:
                hits = [(s, 0.4 + 0.035 * s) for s in range(16)]
            for s, vel in hits:
                desk.add('drums', O.taiko(None, vel, size=0.55, takes=tk), m.at(bar, s) + B.human(hum, 0.003))

    if base:
        _foreman_base(desk, m, spans, V, tk, ts, hum)
    else:
        _foreman_rage(desk, m, spans, V, tk, ts, hum)
    return desk.mix(O.orchestra_hall(_g('foreman', 'hall'), t60=2.8), O.orchestra_bus)


def _foreman_base(desk, m, spans, V, tk, ts, hum):
    g = lambda *p: _g('foreman', 'base', *p)
    # Timpani on the phrase heads, the rolls into C and into the seam; the tam-tam on the seam and C; the mine bell.
    for bar, step, steps, c in spans:
        if _fsec(bar) != 'breath' and bar % 2 == 0:
            desk.add('timp', O.timpani(V[c]['timp'], None, 0.85 if bar in (0, 18) else 0.6, 0.5, takes=tk), m.at(bar))
    desk.add('timp', O.timpani_roll('G2', 2 * m.beat, g('roll17'), 0.15, 0.8, hit=False), m.at(17, 8))
    desk.add('timp', O.timpani_roll('G2', m.bar, g('roll31'), 0.2, 0.85, hit=False), m.at(31))
    desk.add('metal', O.tam_tam(g('tam', 0), 0.7, 8.0), m.at(0))
    desk.add('metal', O.tam_tam(g('tam', 18), 0.55, 8.0), m.at(18))
    desk.add('metal', O.cymbal_swell(m.bar, g('swell'), 0.55, 1.0), m.at(31))
    # The mine bell: once on the seam, two at the march, the three-bell code in the breath.
    for bar, step, vel in ((0, 0, 0.75), (8, 0, 0.6), (8, 3, 0.5), (16, 0, 0.7), (16, 4, 0.6), (16, 8, 0.6)):
        desk.add('bell', _bell(tk, 'C5', vel, t60=3.0, dur=3.5, bright=0.9), m.at(bar, step))

    # Low strings: cellos' grinding eighths everywhere but the breath; basses on the beats but not under the march.
    for bar, step, steps, c in spans:
        sec = _fsec(bar)
        if sec == 'breath':
            continue
        vc = V[c]['vc']
        for e in range(8):
            if bar >= 30:
                continue
            vel = 0.92 if e % 2 == 0 else 0.7
            desk.add('low', *O.string_spiccato('cellos', vc[e], None, vel, m.at(bar, 2 * e), length=0.14, takes=ts))
        if sec != 'B':
            for b in range(4):
                desk.add('low', *O.string_spiccato('basses', V[c]['cb'], None, 0.85 if b == 0 else 0.68,
                                                   m.at(bar, 4 * b), length=0.2, takes=ts))
    # Bars 31-32: the cellos in sixteenths grinding G against Ab into the seam.
    for bar in (30, 31):
        for s in range(16):
            note = ['G2', 'G2', 'Ab2', 'G2'][s % 4]
            desk.add('low', *O.string_spiccato('cellos', note, None, 0.62 + 0.012 * s + 0.08 * (bar - 30), m.at(bar, s),
                                               length=0.1, takes=ts))
    desk.add('sus', *O.string_sustain('basses', 'C2', 2 * m.bar, g('cb_breath'), 0.4, m.at(16), dyn='swell',
                                      attack=0.5, release=0.8))
    desk.add('sus', *O.string_sustain('cellos', 'G2', 2 * m.bar, g('vc_breath'), 0.32, m.at(16), dyn='swell',
                                      attack=0.6, release=0.8))
    # Violas' tremolo: the chords' upper notes through the hymn, sul ponticello rising out of the breath, the charge.
    desk.add('pad', *O.string_tremolo('violas', ['Eb4', 'G4'], m.bar, g('vla_breath'), 0.42, m.at(17), dyn='cresc',
                                      pont=0.5, attack=0.4))
    for bar, step, steps, c in _chords(spans, 18, 32):
        desk.add('pad', *O.string_tremolo('violas', V[c]['vla'], steps * m.step + 0.06, g('vla', bar), 0.42,
                                          m.at(bar, step), players=4, dyn='swell' if bar < 26 else 'cresc', attack=0.12))

    # The tuba: long roots through the shift, the march an octave under the trombones, roots through the hymn,
    # stabs at the charge.
    for bar, step, steps, c in _chords(spans, 0, 8) + _chords(spans, 18, 26):
        desk.add('tuba', O.tuba(V[c]['tuba'], steps * m.step - 0.05, _g('foreman', 'tuba', bar), 0.62, 'swell'),
                 m.at(bar, step))
    march = _line(m, FOREMAN_MARCH, 0.82, dyn='marc')
    x, t0 = O.brass_line('tuba', march.moved(-12.0, vel=0.9), _g('foreman', 'tuba', 'march'))
    desk.add('tuba', x, t0)
    desk.add('tbn', *O.brass_section('trombone', march, g('march'), players=3))
    # Trombones' chords: marcato on the shift's phrase heads, long under the hymn; stabs 3+3+2 at the charge.
    for bar in (0, 2, 4, 6):
        c = [x[3] for x in spans if x[0] == bar][0]
        desk.add('tbn', *O.brass_section('trombone', V[c]['tb'], None, 3 * m.beat, 0.75, 'marc', m.at(bar), takes=tk))
    for bar, step, steps, c in _chords(spans, 18, 26):
        desk.add('tbn', *O.brass_section('trombone', V[c]['tb'], g('tb_hymn', bar), steps * m.step - 0.05, 0.5,
                                         'swell', m.at(bar, step)))
    for bar, step, steps, c in spans:
        if _fsec(bar) != 'D':
            continue
        for s, vel in ((0, 0.88), (6, 0.74), (12, 0.82)):
            desk.add('tbn', *O.brass_section('trombone', V[c]['tb'], None, 0.32, vel, 'marc', m.at(bar, s), takes=tk))
            desk.add('horn', *O.brass_section('horn', V[c]['hn'], None, 0.32, vel * 0.95, 'marc', m.at(bar, s),
                                              players=3, takes=tk))
        desk.add('tuba', O.tuba(V[c]['tuba'], 0.4, _g('foreman', 'tuba', 'stab', bar), 0.85, 'marc'), m.at(bar))
    # Horns: dark swells on the dominants of the shift, held chords under the march, the hymn over the dead shift.
    for bar in (3, 7):
        desk.add('horn', *O.brass_section('horn', V['G']['hn'], g('hn_g', bar), m.bar - 0.05, 0.6, 'fp', m.at(bar),
                                          players=3))
    for bar, step, steps, c in _chords(spans, 8, 16):
        desk.add('horn', *O.brass_section('horn', V[c]['hn'], g('hn_m', bar), steps * m.step - 0.05, 0.42, 'swell',
                                          m.at(bar, step), players=3))
    desk.add('horn', *O.brass_section('horn', _line(m, FOREMAN_HYMN, 0.78), g('hymn'), players=4))

    # The work chant: the men on the blow (the rail), 'hah'; on 1 in the shift and the march, 1 and 3 in the dead
    # shift, every beat at the charge; the hum in the breath.
    for bar, step, steps, c in spans:
        sec = _fsec(bar)
        if sec == 'breath':
            continue
        beats = (0,) if sec in ('A', 'B') else (0, 8) if sec == 'C' else (0, 4, 8, 12)
        for s in beats:
            vel = 0.85 if s == 0 else 0.72
            x, off = _chant(tk, 'men', V[c]['men'], vel)
            desk.add('chant', x, m.at(bar, s) + off + B.human(hum, 0.006))
    desk.add('hum', *O.choir('men', ['C3', 'G3'], 2 * m.bar, g('hum'), 'hm', 8, 0.4, m.at(16), 'swell'))
    for bar, step, steps, c in _chords(spans, 8, 16):
        desk.add('hum', *O.choir('men', V[c]['men'], steps * m.step, g('hum_m', bar), 'mm', 4, 0.32,
                                 m.at(bar, step), 'swell'))


def _foreman_rage(desk, m, spans, V, tk, ts, hum):
    g = lambda *p: _g('foreman', 'rage', *p)
    # High brass: trumpets an octave over the march, with the horns on the hymn (and high horns over it), stabbing
    # the offbeats of the shift and the charge.
    desk.add('tpt', *O.brass_section('trumpet', _line(m, FOREMAN_MARCH, 0.72, 12.0, dyn='marc'), g('march'),
                                     players=3))
    hymn = _line(m, FOREMAN_HYMN, 0.7)
    desk.add('tpt', *O.brass_section('trumpet', hymn, g('hymn'), players=3))
    for bar, step, steps, c in spans:
        sec = _fsec(bar)
        if sec == 'A':
            stabs = ((4, 0.68), (12, 0.62))
        elif sec == 'D':
            stabs = ((0, 0.78), (6, 0.66), (12, 0.74), (14, 0.7))
        else:
            continue
        for s, vel in stabs:
            desk.add('tpt', *O.brass_section('trumpet', V[c]['tpt'], None, 0.25, vel, 'marc', m.at(bar, s), players=3,
                                             takes=tk))
    for bar, step, steps, c in _chords(spans, 26, 32):
        desk.add('horn', *O.brass_section('horn', V[c]['hn'][1:] + [_semi(V[c]['hn'][0], 12)], g('hn', bar),
                                          steps * m.step - 0.05, 0.6, 'cresc', m.at(bar, step), players=3))
    # The tenors answer the men: 'hah' between their blows.
    for bar, step, steps, c in spans:
        sec = _fsec(bar)
        if sec == 'breath':
            continue
        beats = (8,) if sec in ('A', 'B') else (4, 12) if sec == 'C' else (2, 6, 10, 14)
        for s in beats:
            x, off = _chant(tk, 'tenor', V[c]['ten'], 0.72)
            desk.add('chant', x, m.at(bar, s) + off + B.human(hum, 0.006))
    # Double-time strings: violins and violas in sixteenths on the chord (not in the breath).
    for bar, step, steps, c in spans:
        if _fsec(bar) == 'breath':
            continue
        hn = V[c]['hn']
        for s in range(16):
            vel = 0.55 if s % 4 == 0 else 0.38
            desk.add('spic', *O.string_spiccato('violins', _semi(hn[[2, 1, 2, 0][s % 4]], 12 if s % 8 == 0 else 0),
                                                None, vel, m.at(bar, s), players=6, pan=-0.3, width=0.3,
                                                length=0.09, takes=ts))
            if s % 2 == 0:
                desk.add('spic', *O.string_spiccato('violas', hn[[1, 0][(s // 2) % 2]], None, vel * 0.9, m.at(bar, s),
                                                    players=4, length=0.1, takes=ts))
    # The breath: the violins' thin high tremolo; swells into C and the seam.
    desk.add('trem', *O.string_tremolo('violins', ['G4', 'C5'], 2 * m.bar, g('trem'), 0.3, m.at(16), dyn='swell',
                                       pont=0.6, attack=0.6, release=0.4))
    for bar, step, steps, c in _chords(spans, 26, 32):
        desk.add('trem', *O.string_tremolo('violins', V[c]['tpt'][1:], steps * m.step, g('trem', bar), 0.45,
                                           m.at(bar, step), dyn='cresc', attack=0.1))
    for bar in (18, 32):
        desk.add('metal', O.cymbal_swell(m.bar, g('swell', bar), 0.55, 1.2), m.at(bar - 1))


@cue('Music.Boss.Foreman', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-14.0,
     align=False)
def boss_foreman(v, r):
    # Cyrus Teague in the Providence: industrial, the company's cruelty. Bars 1-8 the shift: a sledge on rail on 1 and
    # 3, the anvil on 2 and 4, taiko and bass drum, cellos grinding in eighths, the tuba's long roots, the men's 'hah'
    # on the blow, the mine bell on the seam. Bars 9-16 the company: the game's motif made a march (C G F Ab) in the
    # trombones over the tuba, horns and the men's hum held under it. Bars 17-18 the breath: the hammers stop, the
    # mine bell's three-bell code, the hum, one far hammer, a ponticello tremolo and a roll. Bars 19-26 the dead
    # shift: the tam-tam, the horns' hymn, the chant on 1 and 3. Bars 27-32 the charge: stabs 3+3+2, the chant every
    # beat, the cellos grinding G against Ab into the seam.
    return _foreman('base')


@cue('Music.Boss.Foreman.Rage', variations=1, space='2D', cls='Music', jitter=0.0, conc=2, loop=True, level=-16.0,
     align=False)
def boss_foreman_rage(v, r):
    # His last phase, on top: the hammers double (a second gang on the offbeats), trumpets an octave over the march
    # and with the horns' hymn, stabs; the tenors' answering chant; violins and violas in sixteenths. No bass.
    return _foreman('rage')


# --- A boss falls ---------------------------------------------------------------------------------------------------

@cue('Music.Sting.BossVictory', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-13.0, align=False)
def sting_boss_victory(v, r):
    # Brass and strings climb out of the minor (Bb, then C, a swell on each) and land on D major with the chapel bell,
    # the timpani and a cymbal; the brass eases off, and the strings and a soft choir hold the D major chord while the
    # horn sings the motif's rising fifth over it, and everything lets go.
    g = lambda *p: _g('victory', *p)
    desk = B.Desk(None, tail=6.0)
    desk.track('brass', 0.0, 0.0, -9.0)
    desk.track('tpt', -3.0, 0.0, -9.0)
    desk.track('low', -2.0, 0.0, -12.0)
    desk.track('str', -2.0, 0.0, -9.0)
    desk.track('choir', -6.0, 0.0, -6.0)
    desk.track('timp', -4.0, -0.08, -12.0)
    desk.track('cym', -12.0, 0.15, -7.0, _hp(200.0))
    desk.track('bell', -6.0, -0.25, -5.0, _hp(100.0))
    desk.track('harp', -9.0, -0.5, -7.0)
    desk.track('solo', -4.0, -0.3, -8.0)
    t1, t2, t3 = 0.3, 0.95, 1.6  # Bb, C, D major
    # The climb: horns and trombones, a swell on each chord; tremolo strings under them; a roll to the arrival.
    # Each chord speaks forte, drops back and swells into the next (fp); the timpani strike the first.
    for t, dur, hn, tb, vel in ((t1, t2 - t1, ['Bb3', 'D4', 'F4'], ['Bb2', 'F3', 'D4'], 0.7),
                                (t2, t3 - t2, ['C4', 'E4', 'G4'], ['C3', 'G3', 'E4'], 0.74)):
        desk.add('brass', *O.brass_section('horn', hn, g('hn', t), dur + 0.03, vel, 'fp', t, players=6))
        desk.add('brass', *O.brass_section('trombone', tb, g('tb', t), dur + 0.03, vel, 'fp', t))
        desk.add('str', *O.string_tremolo('violins', [hn[1], hn[2]], dur + 0.03, g('vt', t), 0.5, t, dyn='cresc'))
        desk.add('low', *O.string_sustain('cellos', tb[:2], dur + 0.03, g('vc', t), 0.6, t, dyn='cresc', attack=0.05,
                                          release=0.12))
    desk.add('timp', O.timpani('Bb2', g('timp0'), 0.9, 0.6), t1 - 0.05)  # leads the bows' and breaths' early starts
    desk.add('timp', O.timpani_roll('A2', t3 - t2 - 0.03, g('roll'), 0.3, 0.75, hit=False), t2)
    desk.add('harp', *O.harp_gliss('D3', 'D5', t3 - t2, g('gliss'), 0.5, scale=['C', 'D', 'E', 'G', 'A'],
                                   at=t2))
    # D major: everything on the arrival, the brass easing off over two seconds.
    desk.add('brass', *O.brass_section('horn', ['D4', 'F#4', 'A4', 'D5'], g('arr_hn'), 2.2, 0.85, 'sfz', t3,
                                       players=8))
    desk.add('brass', *O.brass_section('trombone', ['D3', 'A3', 'F#4'], g('arr_tb'), 2.2, 0.8, 'sfz', t3))
    desk.add('tpt', *O.brass_section('trumpet', ['A4', 'D5', 'F#5'], g('arr_tp'), 1.6, 0.78, 'sfz', t3))
    desk.add('brass', O.tuba('D2', 2.2, g('tuba'), 0.75, 'sfz'), t3)
    desk.add('str', *O.string_sustain('violins', ['D5', 'F#5', 'A5'], 3.9, g('vln'), 0.62, t3, players=12,
                                      dyn='dim', attack=0.05, release=1.2))
    desk.add('str', *O.string_sustain('violas', ['A3', 'F#4'], 3.9, g('vla'), 0.6, t3, dyn='dim', attack=0.06,
                                      release=1.2))
    desk.add('low', *O.string_sustain('cellos', ['D3', 'A3'], 3.8, g('vc_arr'), 0.65, t3, dyn='dim', attack=0.05,
                                      release=1.2))
    desk.add('low', *O.string_sustain('basses', 'D2', 2.4, g('cb'), 0.65, t3, dyn='dim', attack=0.05, release=1.0))
    desk.add('timp', O.timpani('D2', g('timp'), 0.9, 0.6), t3)
    desk.add('timp', O.timpani('A2', g('timp2'), 0.45, 0.4), t3 + 0.55)
    desk.add('cym', O.cymbal_crash(g('crash'), 0.6, 4.5), t3)
    desk.add('bell', B.bell('D4', g('bell'), 0.85, t60=4.0, dur=5.0, bright=0.8), t3)
    # The soft held chord: the choir comes in under the strings as the brass lets go; the horn's rising fifth.
    desk.add('choir', *O.choir('women', ['F#4', 'A4', 'D5'], 3.5, g('cw'), 'oo', 6, 0.5, t3 + 0.5, 'swell'))
    desk.add('choir', *O.choir('men', ['D3', 'A3'], 3.5, g('cm'), 'oo', 6, 0.5, t3 + 0.5, 'swell'))
    solo = O.Line().note(t3 + 1.5, 0.7, 'D4', 0.5).note(t3 + 2.2, 1.9, 'A4', 0.55, dyn='swell')
    x, t0 = O.brass_line('horn', solo, g('solo'))
    desk.add('solo', x, t0)
    return desk.mix(O.orchestra_hall(g('hall')), O.orchestra_bus)
