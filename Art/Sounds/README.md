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

- **Guns**: the muzzle's shock front (an N-wave crack), the gas burst (noise saturated at a steady level, then shaped,
  bright at first and darkening, with turbulence so it tears rather than hisses), the weight (a short sine thump and a
  dark burst of air), the action (small steel modes), early reflections off the ground, and an outdoor tail: distinct
  slap-backs, then a rolling diffuse tail, its lows cut so they don't drone. The rifle is tight and bright; the
  shotgun a wide, dark boom with a longer roll.
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
- Every layer cut at its length gets a short end fade (envelopes and modal rings do it themselves), or the check finds
  clicks in the quiet tails.
