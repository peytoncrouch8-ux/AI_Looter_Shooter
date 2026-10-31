# orchestra.py API (lane O's report, 2026-10-31)

`Art/Sounds/recipes/orchestra.py` (import as `from recipes import orchestra as O`; band as `B`). Demo cue
`Music.Dev.Orchestra` (51.6 s render, clean: peak -1.05, L200 -11.6, centroid 552 Hz, presence 2%).

## Conventions
- Notes: names ('D3') or Hz. `vel` 0..1 drives loudness AND brightness together.
- Single players return mono starting at the note onset. Lines/sections return `(stereo, t0)` with t0 absolute:
  `desk.add(track, *result)`. Note-based calls take `at=`.
- Stereo results ignore the Desk track's pan: sections sit by `SEATS`; `pan=`/`width=` override.
- PITFALL: sections start 0.06-0.12 s before their notes; a one-shot Desk clamps negative times to 0 (notes land late).
  In one-shots put first notes at >= 0.2 s. Loops wrap correctly.
- Determinism: every random choice from `r`; with `takes=` given, `r` may be None.
- Cost: string player ~0.04 s per second of music, brass ~0.06, choir singer ~0.05; hall/mix/bus ~11 s per minute. A
  dense 64 s 10-track loop ~100 s. Use `takes=` for repeated short notes (64 taiko hits: 1.4 s instead of 25 s).

## Lines and helpers
- `class Line`: `.note(start, length, note, vel=0.7, **opts)`, `.span()`, `.moved(semis=0, dt=0, vel=1)`.
  Opts: `slur` (default True: joined if within 60 ms of last end), `dyn` (flat|swell|cresc|dim|fp|sfz|marc|chant),
  `accent`, `scoop`, `fall`, `bend` (negative climbs), `vib`, `glide`, `att`, `release`, `vowel`, `tongue`.
- `line(motif, t0, beat, vel=0.7, semis=0, slur=True, gap=0, count=None, **opts)`: a Line from (note, beats) pairs;
  `count=4` = the bosses' quote.
- `class Takes(r, count=5)` `.get(key, make)`: round-robin takes for short repeated notes; pass `takes=` to short
  articulations.
- Data: `MAIN_MOTIF`, `MOTIF_HARMONY`, `CHORDS`, `SEATS`, `D_MINOR`, `BRASS`, `STRINGS`, `PARTS`, `CHOIR_VOWELS`.

## MAIN_MOTIF (the game's theme)
`[('D4',1.0), ('A4',1.5), ('G4',0.5), ('Bb4',2.0), ('A4',1.0), ('F4',1.0), ('G4',1.0), ('E4',4.0)]` = three 4/4 bars:
rising fifth, dip, reach to the minor sixth (Bb over the bar line), sighing down by thirds, lift, ends open on E.
Bosses quote D A G Bb. `MOTIF_HARMONY` (beat, chord): (0 Dm) (3 Bb) (5 F) (6 Dm) (7 C) (8 A major, half cadence).
In F major (menu B section): F4 C5 Bb4 D5 C5 A4 Bb4 G4.

## Brass (kind: horn B1-F5, trombone E2-F5, trumpet F#3-D6, tuba D1-F4)
- `brass(kind, note, dur, r, vel=0.7, art='normal')`; `horn`/`trombone`/`trumpet`/`tuba(note, dur, r, vel, art)`.
  art: normal|stac (<=0.2 s)|marc|sfz|swell|cresc|dim|fp.
- `brass_line(kind, line, r)` -> (mono, t0): one player, lip slurs.
- `brass_section(kind, what, r, dur=1.0, vel=0.7, art='normal', at=0.0, players=None, pan=None, width=None,
  detune=5.0, timing=0.007, takes=None)` -> (stereo, t0); `what` = note (unison), chord list, or Line.
- Brightness doubles soft->loud; cuivre edge at ff; vibrato only on notes >= 0.85 s.

## Strings (section: violins|violas|cellos|basses)
- `string_legato(section, line, r, players=None, pan=None, width=None, pont=0.0, port=0.15, detune=6.0, vib=1.0)`
- `string_sustain(section, notes, dur, r, vel=0.6, at=0.0, players=None, dyn='flat', attack=0.35, release=0.6,
  pont=0.0, pan=None, width=None, detune=6.0, vib=1.0)`
- `string_marcato(section, notes, dur, r, vel=0.8, at=0.0, players=None, pan=None, width=None, takes=None)`
- `string_spiccato(section, notes, r, vel=0.7, at=0.0, players=None, length=0.11, pan=None, width=None, takes=None)`
- `string_tremolo(section, notes, dur, r, vel=0.6, at=0.0, players=None, rate=13.0, dyn='cresc', pont=0.0, pan=None,
  width=None, attack=0.25, release=0.3)` (pont 0..1 toward the bridge, glassy)
- `string_shriek(dur, r, vel=0.8, at=0.0, low='E6', high='A6', players=10, rise=2.0, rate=15.0, pan=-0.3, width=0.8)`
  (lives in 2-5 kHz: keep it LOW in the mix)
- `string_pizz(...)`, `string_col_legno(...)`: `(section, notes, r, vel=0.7, at=0.0, players=None, pan=None,
  width=None, takes=None)`

## Choir (part: bass|tenor|alto|soprano|men|women; vowel: ah|oh|oo|ee|er|mm|hah|hm)
- `choir(part, notes, dur, r, vowel='ah', voices=8, vel=0.6, at=0.0, dyn='swell', pan=0.0, width=0.85, breath=1.0)`
  (voices = singers per note)
- `choir_line(part, line, r, vowel='ah', voices=8, pan=0.0, width=0.85, breath=1.0)`
- `whisper_choir(dur, r, voices=10, vel=0.5, at=0.0, rate=2.6, dyn='swell', pan=0.0, width=0.9, hiss=0.3)`
- Biggest realism unknown: use the choir as a blended pad/colour rather than a bare exposed solo line.

## Percussion (mono)
`timpani(note, r, vel=0.8, hard=0.4, muffle=0.0, dur=None, takes=None)`, `timpani_roll(note, dur, r, vel=0.4,
to=0.8, rate=14.0, hit=True, hard=0.3, muffle=0.0)`, `taiko(r, vel=0.9, size=1.0, takes=None)`, `taiko_rim(r,
vel=0.7, takes=None)`, `bass_drum(r, vel=0.8, dur=3.2, takes=None)`, `bass_drum_roll(dur, r, vel=0.3, to=0.8)`,
`cymbal_crash(r, vel=0.8, dur=5.0)`, `cymbal_swell(dur, r, vel=0.8, tail=3.0)` (peaks at dur), `tam_tam(r, vel=0.8,
dur=9.0)`, `anvil(r, vel=0.8, kind='anvil'|'rail', f0=None, takes=None)`, `stone(r, vel=0.8, size=1.0, takes=None)`,
`stone_grind(dur, r, vel=0.6, size=1.0)`.

## Harp and keys
`harp(note, r, vel=0.6, dur=None)` mono; `harp_gliss(lo, hi, dur, r, vel=0.55, scale=None, up=True, at=0.0)` ->
(mono, t0) (scale: pitch-name list, D minor default); `celesta`, `glockenspiel` (2-5 kHz: keep low), `piano(note, r,
vel=0.7, dur=6.0)` (low strike A0-C4) mono.

## Room and bus
`orchestra_hall(r, t60=3.1, predelay=0.034, lp=6200.0, early_db=-3.5)` for `Desk.mix`;
`orchestra_bus(x, circular, low_cut=35.0, presence_cut=-2.0, mud_cut=-1.0, top_cut=-2.5, glue=True)` (loop-safe).
