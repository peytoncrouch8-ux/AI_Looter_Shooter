# Lane SOUND: Landfall and the Dredgers (done, rendered, not imported)

## Files
- `Source/AI_Looter_Shooter/Audio/LooterSoundCues.h`: 30 flat constants at the end of `LooterSoundCue` (exact names and strings
  of the contract: `VaneRadioIn` ... `TurnInStamp`), each with a doc comment (length, takes, loop or not).
- `Art/Sounds/recipes/landfall.py` (the Lamplighter, grapples, tender, winch), `landfall_story.py` (radio, spyglass, lantern, smog,
  Delia, turn-in stamp; a third file so each stays about 450 lines), `dredger.py` (the Dredgers): all new.
- `Art/Sounds/recipes/__init__.py`: `'landfall', 'landfall_story', 'dredger'` appended to `GROUPS`.
- `Art/Sounds/README.md`: a group bullet and two lessons. `Art/Sounds/Out/*.wav` and `cues.json` were rendered (see Main's steps).
- All cues are synthesized, except the two guns, which reuse recordings already in `packs/` (no download): the Dredger rifle (L85
  and Tavor slowed 0.82-0.93x, plus synthesized crack, push, barrel ring and a spark flare) and Delia's shotgun (the Ranchhand's
  recorded blast, slowed 0.86-0.88x, with its low and tail layers).

## Cues (volume_db is cues.json's, mix against the rifle; the standing shift of -0.8 dB is ShutterSlam's, unchanged)
| Cue | Takes | Length | volume_db | Notes |
|---|---|---|---|---|
| `Story.Vane.RadioIn` | 3 | 0.57 s | -13.5 | 2D; relay, squelch, carrier lock, crackle |
| `Story.Vane.RadioOut` | 3 | 0.35 s | -17.5 | 2D |
| `Story.Vane.RadioHiss` | loop | 6 s | -32 | 2D; start at RadioIn, stop at RadioOut |
| `Ship.Lamplighter.Engine` | loop | 8 s | -19.5 | 3D Creature, at the ship; prop whump 5 Hz |
| `Ship.Lamplighter.Creak` | loop | 10 s | -22.5 | 3D Creature |
| `Ship.Lamplighter.Lurch` | 3 | 1.8 s | -6 | peak at 0 s; groan sinks as she stops |
| `Ship.Lamplighter.BagTear` | 3 | 2.4 s | -5.5 | pop 0, rip to ~1.5 s, stays snap at 0.6 and 0.95 s |
| `Ship.Lamplighter.HardLanding` | 3 | 4.1-4.3 s | -1.5 | 'Gun'; hit 0, skip-hit ~0.4 s, scrape to ~2.5 s, groan 2.3 s, last settle knock ~3.1 s |
| `Ship.Grapple.Fire` | 3 | 1.1 s | -3 | 'Gun' |
| `Ship.Grapple.Hit` | 3 | 0.9 s | -4.5 | 'Gun' |
| `Ship.Grapple.Strain` | loop | 6 s | -24.5 | 3D Creature, at each grapple; stop on cut |
| `Ship.Grapple.Cut` | 3 | 1.3 s | -2 | 'Gun'; blow at 0 |
| `Ship.Tender.Fall` | 2 | 6.7-6.9 s | -1 | 'Gun'; bang at 0, the fall, far crash at ~4.9 s |
| `Ship.Anchor.Crank` | loop | 4 s | -17.5 | 3D Creature; 8 strokes (0.5 s each), 3 pawl clicks per stroke |
| `Ship.Anchor.Set` | 3 | 2.7 s | -2 | flukes bite ~0.7 s, chain taut ~1.2 s, pawl locks ~1.9 s |
| `Story.Spyglass.Open` | 3 | 1.1 s | -10 | 2D |
| `Story.Lantern.Lean` | 3 | 3.1-3.3 s | -17 | 3D Creature, swells (soft start on purpose) |
| `Ambience.Smog` | loop | 16 s | -32 | 2D Ambience class |
| `Story.Delia.Shotgun` | 5 | 1.7 s | -1.5 | 'Gun'; the pump's recordings, bigger and darker |
| `Story.Delia.Reload` | 4 | 1.9 s | -5 | Near |
| `UI.TurnIn.Stamp` | 3 | 1.25 s | -16 | 2D Interface; stamp lands at 0.11 s (`givers.STAMP_LANDS`, like Accept's) |
| `Creature.Dredger.Rifle.Fire` | 5 | 1.0-1.1 s | -8.5 | 'Gun', conc 6; L200 is 5 LU over the player's shot, played 3 LU under it |
| `Creature.Dredger.Rifle.Reload` | 4 | 1.6 s | -5 | Near |
| `Creature.Dredger.Bark` | 6 | 0.6-0.9 s | -12.5 | Creature; masked shouts, no words; pick a take at random |
| `Creature.Dredger.Hurt` | 5 | 0.5-0.6 s | -12 | Creature |
| `Creature.Dredger.Death` | 4 | 1.65-1.75 s | -10.5 | Creature; knees ~0.6 s, body ~1.0 s |
| `Creature.Dredger.Hose.Latch` | 3 | 0.9 s | -4 | Creature |
| `Creature.Dredger.Hose.Drain` | loop | 6 s | -20 | 3D Creature, at the hose; stop on snap or death |
| `Creature.Dredger.Hose.Snap` | 3 | 1.1-1.2 s | -8 | Creature |
| `Creature.Dredger.Tank.Burst` | 3 | 2.8-3.0 s | -3.5 | 'Gun'; the satisfying pop |

The check flags nothing on any of them (no PEAK, CLICK, LATE START, CUT TAIL, SEAM, HARSH). The 7 loops have no seam jump. All
variations of a cue share one loudness (`align`) and one gain.

## Main's steps
1. Pull in the sounds: they are already rendered (`Art/Sounds/Out`, `cues.json` rewritten). If anything changes after a merge:
   `Tools\sounds.ps1 Ship.*,Story.Vane.*,Story.Spyglass.*,Story.Lantern.*,Story.Delia.*,Ambience.Smog,UI.TurnIn.*,Creature.Dredger.*`
   (the render prints "all shifted -0.8 dB": that is the standing value; a bigger shift means a cue of this set went over 0 dB).
2. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_sound_bank.py"`, then `... build_sound_bank.py check`: all 30
   names must be in the bank (they are in the header, so the usual by-hand comparison is only needed if a later lane renames one).
3. Commit `Art/Sounds/Out/*` (LFS), `cues.json`, the recipes, the header, the README.

## How the lanes should use them
- Loops: `LooterSound::Start`/`Stop` (the same as the ore lift's). `Hose.Drain` and `Grapple.Strain` are 3D mono loops with the
  'Creature' (35 m) attenuation. `VaneRadioHiss` is 2D.
- `HardLanding` and `Tender.Fall` carry like a gunshot ('Gun', about 60 m): play them at the ship or the tender. Time the camera
  shake to the hit at 0 s and the skip at ~0.4 s.
- `Bark`: one cue with 6 different shouts (take picked at random); rate-limit it as the Dredger's director allows.
- `DeliaShotgun` is the full shot (blast and tail in one), like Abel's pump; play it at her muzzle.

## Risks
- Nobody listened: every judgement is by numbers and spectrograms. Most likely to need the user's ear: the Dredgers' masked voices
  (formant shouts: could sound too narrow or too steady), the rifle's boom, the crash's balance of debris against the hit, the
  lantern's tone, the tender's fall.
- The Dredger rifle is the player's own rifle slowed down and layered, so it shares a family with it; the identity is the dark
  boom, the ring and the spark flare. Tuning knobs are in `dredger._rifle_dry` (varispeed 0.84, the flare level) and `level`.
- `Delia.Shotgun` takes differ in low-end share (the underlying recordings: 20-50 % under 100 Hz); the render aligns loudness only.
- Spectra: the bark, hurt and death are 80-95 % in 250-1000 Hz by design (a mask), not frequent sounds, so no 2-5 kHz flag applies.
