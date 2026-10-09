# Sounds

Every sound in the game is synthesized from scratch by the scripts here: numpy in Blender's bundled Python, no
recordings, no sample libraries, no generators. Change a sound by changing its recipe and rendering again; never edit
a WAV by hand.

```
Art/Sounds/lib/          the synthesis library (see lib/__init__.py for what each module holds)
Art/Sounds/recipes/      one recipe per cue, by group: guns, impacts, feedback, ui, loot (and the bench), player,
                         creatures, world; kit.py holds the shared building blocks (clicks, clacks, whooshes, cloth,
                         scrapes, creaks, bubbles, chimes, plucked strings...)
Art/Sounds/render.py     renders, mixes and checks (run by Tools/sounds.ps1)
Art/Sounds/check.py      the check: numbers, flags, pictures and the listening page
Art/Sounds/cues.json     every cue's files and mix, read by Tools/Unreal/build_sound_bank.py
Art/Sounds/Out/          the rendered WAVs (Git LFS): <Cue_With_Underscores>_<NN>.wav
Saved/SoundCheck/        the check's output (not in git): index.html, <Cue>.png, report.txt, report.json
```

## Render

```
Tools\sounds.ps1                     every cue (about a minute), then cues.json and the check
Tools\sounds.ps1 Weapon.*,UI.Hit*    just these cues (names or patterns, dots or underscores)
Tools\sounds.ps1 -Zoom 0.12 Impact.* also a picture of each cue's first 0.12 s (<Cue>_zoom.png)
Tools\sounds.ps1 -CheckOnly          only the mix and the check
```

Like `Tools\artrun.ps1`, it waits while `Saved\ArtPause.flag` exists and runs at below-normal priority, so it's safe
beside a running editor. Several Blenders share the cues (`-Jobs`, default cores - 2). Rendering is deterministic:
every random number comes from a generator seeded by the cue's name and variation, so the same recipe always gives
the same files. After a render, Main runs `build_sound_bank.py` to bring them into Unreal.

## The format (the contract)

- 48 kHz, 16-bit PCM with triangular dither. Mono for 3D cues; 2D cues may be stereo.
- Each cue's variations share one gain: the loudest peak sits at -1.05 dBFS, under the contract's -1. Variations are
  matched in loudness first (unless the cue sets `align=False`), so the game's random pick never jumps in level.
- One-shots start at once (trimmed to the first sound) and end in silence (trimmed with a fade). Loops (`loop=True`)
  are built in a circle (periodic noise, circular filters, tails folded round to the start) and left whole.
- `cues.json`: one entry per cue with `files`, `volume_db`, `pitch_jitter`, `class`, `attenuation`, `loop`, `space`,
  `max_concurrent`. `volume_db` is the mix: each cue's `level` (how loud it should sound against the rifle's shot)
  less its measured loudness (the loudest 200 ms, K-weighted as in ITU-R BS.1770) against the rifle's, so the rifle
  plays at 0 dB and everything else sits under it.

## Add a cue

1. Name it in `Source/AI_Looter_Shooter/Audio/LooterSoundCues.h` (S1 or Main) and decide its group.
2. Write its recipe in `recipes/<group>.py`:

   ```python
   @cue('World.Windmill.Creak', variations=3, att='Creature', cls='Ambience', jitter=0.04, conc=2, level=-12.0)
   def windmill_creak(v, r):
       # What it is, physically, in a line or two.
       creak = kit.creak(0.8, child(r, 'creak'), 20.0, 45.0, (300.0, 700.0, 1400.0))
       return layers((creak, 0.0, 0.0))
   ```

   `v` is the variation (0..), `r` its seeded generator; give each layer its own stream with `child(r, 'name')` so
   adding a layer doesn't change the others. Return mono for 3D, mono or stereo `(2, n)` for 2D. `@cue`'s arguments
   are documented in `recipes/__init__.py` (`swell=True` for sounds meant to fade in, `loop=True` for loops).
3. Render it (`Tools\sounds.ps1 World.Windmill.*`), look at `Saved\SoundCheck\<Cue>.png` and its line in
   `report.txt`, listen on `index.html`, and fix what the flags say.

## The check's flags

`PEAK` over -1 dBFS, `CLIP` full-scale samples, `DC` offset, `CLICK` a sharp spike where the sound is quiet (a layer
cut off), `LATE START` more than 5 ms before the sound reaches -40 dB under its peak (feels laggy), `CUT TAIL` not
ending near silence, `SEAM JUMP` / `SEAM CLICK` a loop's join, `STEREO IN 3D`, and `HARSH 2-5K` for sounds heard over
and over (guns, impacts, footsteps, hit markers, clicks) carrying too much in the ear's most sensitive band or a sharp
spike there. `report.txt` also lists each file's loudness, spectral centroid and band shares (sub, low, low-mid, mid,
presence 2-5 kHz, high, air).

## How the sounds are built

- **Guns** (heavy and punchy, the user's pick; round 2 made them less synthetic, after Borderlands' big, punchy guns,
  2026-10-08): modelled the way a real shot reaches a close microphone, not built from drum-machine parts. A
  Friedlander pressure pulse (a near-instant rise, a positive phase of 1.3 ms for the rifle and 2.6 ms for the shotgun,
  a shallower negative phase, roughened per take) with the crack on its front; the weight as one slow push and pull of
  air kept in its lows (no pitched sine sweep); the gas as a dense spray of random impulses over turbulent noise,
  overdriven, its level flickering and puffing; the ground bounce two darker copies 2-8 ms behind; a recorder model
  (lopsided overload clipping, a ringing input filter that turns it into a low heave, gain bloom after the overload,
  a proximity lift at 100-120 Hz). The action is loud enough to hear on every shot (the rifle's hammer tick just
  before the blast, carrier and bolt clacks after; the shotgun's receiver ring, its forend's steel-on-wood knock, a
  faint pellet hiss). The tail is two slap-backs then 18-26 echoes at random distances, each darker and more smeared
  the farther it is, over a rumble whose level wanders: about 1 s for the rifle, 1.5 s for the shotgun. A soft clip
  before the limiter keeps the snap. Reloads are clanks: steel modes held to about 1-4 kHz, a polymer or wood knock
  and a low thud, with the slides kept dark and well under the hits.
- **Creatures** sound like what they are: a spider's shell cracks when hit and it clicks, rasps and hisses (no voice);
  a slime is all jelly (slaps, squelches, wobble, bubbles; no voice); the Unpaid are ghostly voices, and a bullet
  through one is a hollow thump and a puff of grave dust. Each kind has its own hit (`Creature.<Kind>.Hit`).
- **Struck things** are modal: a set of decaying sines whose frequencies follow the object's physics (free bars,
  plates, wood, shells, cast bells), struck by a force pulse as long as the contact (hard steel ~0.1 ms, a heel
  several ms).
- **Friction** (scrapes, creaks, stridulation) is stick-slip: trains of tiny slips rung through the thing's resonances.
- **Voices** (creatures, breath) are formant synthesis: a harmonic glottal source with jitter, shimmer and breath,
  each harmonic weighted by a cascade of vocal-tract resonances that glide between vowels.
- **Spaces** are convolution with impulse responses made from noise: band-wise decays (highs die first), sparse early
  reflections, outdoor slap-backs.
- **The interface** speaks in brass and steel string: small brass ticks and chimes, plucked strings on a wooden box,
  kept quiet and off the 2-5 kHz band so they never tire the ear.

## What's learned

- Saturate noise *before* shaping its envelope: driving an already-decaying burst flattens the decay and turns a shot
  into a long hiss.
- Set a reverb's level against the dry sound's loudest 20 ms, not its single peak sample, or a crack spike skews it.
- A five-formant cascade dies off far too fast above 4 kHz; the upper formants' falls are eased (`voice.EASE`) to
  stand in for the resonances the series leaves out.
- Brown noise's lows wander too much between seeds for a thump layer; pink noise with a low cut weighs the same every
  time.
- A pitched sine sweep under a shot is a drum machine's kick: the user heard the guns as fake. Real weight is a
  broadband pressure pulse and the microphone's overload; judge a shot's lows by how tonal they are (round 2: 0.85 ->
  0.42) and how alike they are take to take, as well as by their share.
- Keep a shot's lows in one deterministic layer: low noise under it adds to or cancels the sine at
  random, and swung the low share about 20 points between variations.
- Mix noise layers by their loudest 10 ms, not their peak sample, so every variation weighs the same.
- The mix is set against the rifle's loudness: a denser rifle moves every cue (2026-10-08 the whole set shifted
  -2.2 dB when World.ShutterSlam reached the 0 dB cap). ShutterSlam and Impact.Wood now set the headroom.
- Every layer cut at its length gets a short end fade (envelopes and modal rings do it themselves), or the check finds
  clicks in the quiet tails.
