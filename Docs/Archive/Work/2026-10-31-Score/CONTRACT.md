# Score round 1 (2026-10-31): the menu theme and the boss fights

The user's calls (2026-10-31): music stays script-made (numpy synthesis, like every sound; no recordings, no samples, no
AI music); round 1 = the main menu theme and the boss fights; style "mix by moment": folk/ambient while exploring, rock in
fights, ORCHESTRAL for the bosses and the menu. Later rounds: intro scenes, Ransom's Rest by area, combat in rock, the
Gilded Lily (on hold). Quality bar: the best big-studio game scores. Original music only: no quoting or imitating any
existing piece (no famous western whistles, no film themes).

## Who does what (disjoint files)

| Lane | Model | Files (only these) |
|---|---|---|
| O: the orchestra | Opus | `Art/Sounds/recipes/orchestra.py` (new): instruments + section helpers on `band.py`'s Desk |
| M: menu theme | Opus | `Art/Sounds/recipes/score_menu.py` (new) |
| B1: beasts | Opus | `Art/Sounds/recipes/score_beasts.py` (new): Gravemother, Stone Sentinel |
| B2: men | Opus | `Art/Sounds/recipes/score_men.py` (new): Abel (Keeper), Cyrus Teague (Foreman), boss victory sting |
| Main | - | C++ (`Audio/Music*`, cues), `recipes/__init__.py` GROUPS, removing the two old boss recipes from `music.py`, docs |

Nobody edits `band.py`, `music.py`, `kit.py`, `lib/` (Main only; ask Main for a change). M/B1/B2 import `orchestra` and
`band`. Lane O lands first; M, B1, B2 start after it.

## Cues (names fixed; Main wires them)

All: `space='2D', cls='Music', variations=1, jitter=0.0, align=False`. Loops: `loop=True, conc=2`, rendered circularly
(`B.Desk(bars * m.bar)`: tails fold round, seamless). Bars/tempo below are what `MusicRules` uses: don't change them.

| Cue | Lane | Meter | Bars | Bar s | Loop s | level | What |
|---|---|---|---|---|---|---|---|
| `Music.Menu.Theme` | M | 4/4 at 72 | 32 | 3.3333 | 106.67 | -16 | The game's main theme, Teropa from orbit; loops forever in the menu |
| `Music.Boss.Gravemother` | B1 | 4/4 at 120 | 32 | 2.0 | 64.0 | -14 | Spider queen of the Sink |
| `Music.Boss.Gravemother.Rage` | B1 | same | 32 | 2.0 | 64.0 | -16 | Her last phase, on top |
| `Music.Boss.Sentinel` | B1 | 4/4 at 100 | 24 | 2.4 | 57.6 | -14 | The Stone Sentinel (four in the Rest: Boot Hill, the churchyard) |
| `Music.Boss.Sentinel.Rage` | B1 | same | 24 | 2.4 | 57.6 | -16 | Under half health, on top |
| `Music.Boss.Keeper` | B2 | 4/4 at 128 | 32 | 1.875 | 60.0 | -14 | Abel Ransom, the Keeper, Ellis's Pa (last boss of the Rest) |
| `Music.Boss.Keeper.Rage` | B2 | same | 32 | 1.875 | 60.0 | -16 | His last phase, on top |
| `Music.Boss.Foreman` | B2 | 4/4 at 112 | 32 | 2.142857 | 68.571 | -14 | Cyrus Teague, the Foreman, in the Providence mine |
| `Music.Boss.Foreman.Rage` | B2 | same | 32 | 2.142857 | 68.571 | -16 | His last phase, on top |
| `Music.Sting.BossVictory` | B2 | free | - | - | ~7 s | -13 | A boss falls: orchestral resolution to D major (one-shot, `conc=1`) |

- The `.Rage` stem plays sample-locked over its base (started together, faded in on a bar line when the last phase
  begins, kept to the end). Base alone must be a complete piece; base + rage must sound like one bigger arrangement
  (more drums, high brass/choir, double-time strings), never two pieces. Same reverb, same key. The rage stem has no
  low bass that doubles the base's (mud).
- Each boss piece has a 2-bar quieter "breath" somewhere, so a 60 s loop doesn't feel like a wall; the loop seam lands
  on bar 1's downbeat with something on it.
- Keys: the game's home is D minor (the Rest's theme, the stings in D). Gravemother D Phrygian (Eb), Keeper D minor,
  Foreman C minor, Sentinel A minor (stone, ancient), Menu D minor -> F major in its B section. The boss victory
  sting resolves to D major.
- Mix: music sits under effects. Every piece ends on `B.master_bus` (or orchestra's variant) and keeps 2-5 kHz
  restrained (`check.py` flags HARSH). Boss pieces sit under creature cries (the director ducks too).

## Themes (identity; the composers write the notes)

- **Teropa (menu)**: wide and lonely, a planet seen from orbit, a long road ahead, a dead man walking it. Opens with a
  low string drone, a far bell and a solo voice (solo horn, or a reed/whistle-like solo) stating a 6-8 note main
  motif; builds through horns and full strings with a wordless choir to a big but warm B section (F major), then
  thins back so the loop seam is quiet. The motif is the game's: the bosses may quote its first 4 notes. Lane O
  puts it in `orchestra.py` as `MAIN_MOTIF` (note names and beat lengths), so M, B1 and B2 share it.
- **Gravemother**: skittering, many-legged: col legno and pizzicato ostinatos, low brass clusters, taiko, a whispering
  choir, D Phrygian. Rage: shrieking high strings, full choir, double-time taiko.
- **Stone Sentinel**: ancient, heavy, slow-grinding: timpani and big drums on a lurching 3+3+2 accent, low brass and
  low choir "oh", stone percussion (heavy knocks, gravel), A minor. Rage: horns in fifths, faster timpani rolls.
- **Keeper (Abel)**: tragic and heroic, a son fighting his father: the chapel bell, full orchestra, choir, the reed's
  lament quoting the Rest's call (`music.py` RR_REED's opening) slowed; D minor. Rage: the whole orchestra on the
  lament, the bell every bar.
- **Foreman (Cyrus Teague)**: industrial, company cruelty: hammer on rail/anvil on the beat, low brass and tuba, a
  male work-chant choir ("hah"/"hm"), the mine bell, driving low strings, C minor. Rage: the hammers double, high brass.
- **Boss victory**: brass and strings resolving into D major, a bell, then a soft held chord; 6-8 s.

## Lane O: the orchestra (orchestra.py)

Physically modelled or carefully synthesized, like `band.py` (read it first; reuse its Desk, hall, human/lively,
ensemble, bowed, spiccato, big_drum, bell). Each instrument: `fn(note_or_hz, dur, r, vel=..., ...) -> mono or (2,n)`
plus a section helper that layers 2-6 detuned players with timing spread. Needed:
- brass: horn (round, dark), trombone (brassy at forte), trumpet (bright), tuba (low); velocity drives brightness (a
  lip-buzz source through a bell filter that opens with dynamics, the attack's blip, breath noise, vibrato only on
  long notes); staccato stabs and long swells; a section (`brass_section`).
- strings: legato section (long bows, smooth note changes), marcato, pizzicato, col legno, tremolo, a high shrieking
  cluster; may wrap band.ensemble/bowed.
- choir: wordless "ah"/"oh"/"mm"/"hah" (lib.voice formants), male/female ranges, a whisper choir; a section of 8+
  voices, breathy, no robotic sameness.
- timpani (tuned, with rolls), taiko/war drum, orchestral bass drum, suspended cymbal swell and crash, tam-tam,
  anvil/rail hammer, stone knocks.
- harp (plucked, glissando), celesta/glockenspiel (modal bars); a low piano-like strike optional.
- `orchestra_hall(r)`: a big scoring-stage reverb (longer than band.hall), and `orchestra_bus(x, circular)` master
  (like B.master_bus: low cut 35 Hz, gentle glue, presence restrained).
- A demo cue that plays every instrument in turn: `Music.Dev.Orchestra` (one-shot, ~40-60 s, level -16), in
  orchestra.py itself; Main deletes it before the commit.
- A short docstring at the top listing every instrument and its call, like band.py's.

## Rules for every lane

- Read `Art/Sounds/README.md`, `recipes/band.py`, `recipes/music.py` before writing.
- Every random choice from the passed generator (`child(r, ...)`): same recipe, same take.
- Render your cues with `Tools\sounds.ps1 <cues>` (PowerShell); read `Saved/SoundCheck/report.txt` for your cues and
  fix flags. Look at the spectrogram PNGs (`Saved/SoundCheck/<Cue>.png`). Say each cue's render time (keep a piece
  under ~5 min).
- Nobody can listen: judge by the numbers, the pictures, and musical craft (voice leading, orchestration ranges,
  dynamics, real instrument ranges and articulations, no parallel-fifth mud in the low end, a melody people hum).
- Don't run `build_sound_bank.py`, don't open or touch Unreal, don't commit. Main imports and commits.
- Done = your cues render clean, plus a short note: what each piece does, its sections bar by bar, any flag left and why.
