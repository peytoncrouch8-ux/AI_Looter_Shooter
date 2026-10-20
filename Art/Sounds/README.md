# Sounds

Every sound in the game is synthesized from scratch by the scripts here: numpy in Blender's bundled Python, no
recordings, no sample libraries, no generators. Change a sound by changing its recipe and rendering again; never edit a
WAV by hand.

## Files

```
Art/Sounds/lib/          the synthesis library (see lib/__init__.py for what each module holds)
Art/Sounds/recipes/      one recipe per cue, by group (below); kit.py holds the shared building blocks (clicks, clacks,
                         whooshes, cloth, scrapes, creaks, bubbles, chimes, plucked strings...), band.py the score's
                         instruments and mixer
Art/Sounds/render.py     renders, mixes and checks (run by Tools/sounds.ps1)
Art/Sounds/check.py      the check: numbers, flags, pictures and the listening page
Art/Sounds/cues.json     every cue's files and mix, read by Tools/Unreal/build_sound_bank.py
Art/Sounds/Out/          the rendered WAVs (Git LFS): <Cue_With_Underscores>_<NN>.wav
Saved/SoundCheck/        the check's output (not in git): index.html, <Cue>.png, report.txt, report.json
```

Groups (`recipes/<group>.py`; a new group's file must be listed in `recipes/__init__.py`'s `GROUPS` or it never renders):
- guns; impacts (also a cue per surface: dirt, grass, gravel, sand, stone, sheet metal, glass, cloth, hay, bone; and the
  ricochet layered on glancing hits); feedback; ui; loot (and the bench, the soul-motes); player (and the melee strike,
  the mantle and vault, a hard landing's thud, the splash in water); creatures (and the rank sting, the ambushes' entrances); world.
- fauna (the ambient birds, insects, tumbleweeds, dust devils, washing); bosses (the boss fights' stings and voices); gravemother_barrow (the Gravemother's Barrow fight: clod, stone, furrow,
  mounds; on bosses.py's helpers); tiers (the enemy tiers' notice stings, deaths and loot pops, and the traits' cues;
  Creature.Trait.Mend is a loop);
  ambience (beds by area and light, sweeteners); music (the score: themes, combat and boss layers, stingers).
- voices (the Unpaid's murmured barks, the idle calls, the living heard through walls); throwables (the grave-salt
  grenade); props (the lootable world: crates, barrels, graves, coffins, mailboxes, footlockers); economy (silver found,
  the vendor's counter: buy, sell, not enough).
- bells (the Bell-Ringer's hand bell: toll, cracked toll, lift, clang, raising, swipe); shrines (the vigil stones:
  rising, kneeling, the refusal, the iron bell's summons).
- givers (mission offers, the stamp, found letters; its paper and stamp helpers are shared); graves (a save point
  marked: the lantern, a bell's hum, the Gravewind); holds (the hold ring's click); lostloot (the depot's Lost Property
  window); missions (optional ticks, mission items, the turn-in's choice, readables, the porch plate, going
  underground).
- orelift (the ore lift: signal bells, the steam whistle, the winch and cable loops, the cage's creaks and clanks);
  watchfire (the tower's brazier); miners (the dead miners, Thaddeus, Hiram; its rock-gallery space is shared); foreman
  (the Foreman's fight, fuses and roof); roosts (Hob's roosts: found, the loop near one, the crows' carry and the death
  drag); fittings (the mountain's mission fittings: the lift's crank, boiler and clamp pin, the manway gate).
- windows (a lit window's life heard from the street: footsteps across the room, a curtain drawn; shares voices' wall);
  gusts (the wind passing, 2D, and what it knocks loose: a shutter slammed, a sign's chains and bracket).
- animals (the town's small animals: a hen's cluck and alarm squawk, the rooster's crow, the barn cat's meow and hiss;
  imports the fauna's wings and outdoor wash).
- reactions (creatures noticing and stirring: the spider's notice, emerging from the ground and a dead one's twitch; the
  slime's notice, a rising bubble and swelling up from a puddle; the Unpaid's notice and breaking into ash; the dead
  miner's and the Bell-Ringer's notice; imports the kinds' helpers from creatures, miners and bells).
- brass (spent casings by surface: stone, wood, metal, dirt, water, a shotgun hull; and the fly-bys: a bullet's whiz-crack
  past the ear, a venom glob's hiss).

## Render

```
Tools\sounds.ps1                     every cue (about a minute), then cues.json and the check
Tools\sounds.ps1 Weapon.*,UI.Hit*    just these cues (names or patterns, dots or underscores)
Tools\sounds.ps1 -Zoom 0.12 Impact.* also a picture of each cue's first 0.12 s (<Cue>_zoom.png)
Tools\sounds.ps1 -CheckOnly          only the mix and the check
```

- Like `Tools\artrun.ps1`, it waits while `Saved\ArtPause.flag` exists and runs at below-normal priority: safe beside a
  running editor.
- Several Blenders share the cues (`-Jobs`, default cores - 2).
- Deterministic: every random number comes from a generator seeded by the cue's name and variation, so the same recipe
  always gives the same files.
- After a render, Main runs `build_sound_bank.py` to bring them into Unreal.

## The format (the contract)

- 48 kHz, 16-bit PCM with triangular dither. Mono for 3D cues; 2D cues may be stereo.
- Each cue's variations share one gain: the loudest peak sits at -1.05 dBFS, under the contract's -1. Variations are
  matched in loudness first (unless the cue sets `align=False`), so the game's random pick never jumps in level.
- One-shots start at once (trimmed to the first sound) and end in silence (trimmed with a fade). Loops (`loop=True`) are
  built in a circle (periodic noise, circular filters, tails folded round to the start) and left whole.
- `cues.json`: one entry per cue with `files`, `volume_db`, `pitch_jitter`, `class`, `attenuation`, `loop`, `space`,
  `max_concurrent`. `volume_db` is the mix: each cue's `level` (how loud it should sound against the rifle's shot) less
  its measured loudness (the loudest 200 ms, K-weighted as in ITU-R BS.1770) against the rifle's, so the rifle plays at
  0 dB and everything else sits under it.

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
   adding a layer doesn't change the others. Return mono for 3D, mono or stereo `(2, n)` for 2D. `@cue`'s arguments are
   documented in `recipes/__init__.py` (`swell=True` for sounds meant to fade in, `loop=True` for loops).
3. Render it (`Tools\sounds.ps1 World.Windmill.*`), look at `Saved\SoundCheck\<Cue>.png` and its line in `report.txt`,
   listen on `index.html`, and fix what the flags say.

## The check's flags

- `PEAK` over -1 dBFS; `CLIP` full-scale samples; `DC` offset; `CLICK` a sharp spike where the sound is quiet (a layer
  cut off); `LATE START` more than 5 ms before the sound reaches -40 dB under its peak (feels laggy); `CUT TAIL` not
  ending near silence; `SEAM JUMP` / `SEAM CLICK` a loop's join; `STEREO IN 3D`.
- `HARSH 2-5K`: sounds heard over and over (guns, impacts, footsteps, hit markers, clicks) carrying too much in the
  ear's most sensitive band, or a sharp spike there.
- `report.txt` also lists each file's loudness, spectral centroid and band shares (sub, low, low-mid, mid, presence
  2-5 kHz, high, air).

## How the sounds are built

- **The score** (2026-10-08): original weird-west music in 6/8 at 75 in D, so the themes, the combat layer and the boss
  pieces line up on a 1.6 s bar and the director (`Audio/MusicDirectorSubsystem`) crossfades on bar lines. Themes:
  plucked strings in drop D, a reed calling between phrases, a far bell, cello and ghost strings. Combat: galloping
  toms, a bass ostinato, climbing tremolo strings. Kept about 10 dB under creature cries.
- **Ambience**: two loops per area and light (19 s and 13 s, played together, so nothing audibly repeats for minutes)
  and one-shot sweeteners placed round the listener; places that sound (creeks, ponds, falls, the windmill, the Rim's
  wind, the Sink) are 3D emitters placed by `Tools/Unreal/build_area_sound.py`.
- **Guns** (heavy and punchy, the user's pick; round 2, 2026-10-08, made them less synthetic, after Borderlands' big,
  punchy guns): modelled the way a real shot reaches a close microphone, not built from drum-machine parts.
  - A Friedlander pressure pulse (a near-instant rise, a positive phase of 1.3 ms for the rifle and 2.6 ms for the
    shotgun, a shallower negative phase, roughened per take) with the crack on its front.
  - The weight: one slow push and pull of air kept in its lows (no pitched sine sweep).
  - The gas: a dense spray of random impulses over turbulent noise, overdriven, its level flickering and puffing.
  - The ground bounce: two darker copies 2-8 ms behind.
  - A recorder model: lopsided overload clipping, a ringing input filter that turns it into a low heave, gain bloom
    after the overload, a proximity lift at 100-120 Hz.
  - The action is loud enough to hear on every shot: the rifle's hammer tick just before the blast, carrier and bolt
    clacks after; the shotgun's receiver ring, its forend's steel-on-wood knock, a faint pellet hiss.
  - The tail: two slap-backs then 18-26 echoes at random distances, each darker and more smeared the farther it is, over
    a rumble whose level wanders: about 1 s for the rifle, 1.5 s for the shotgun. A soft clip before the limiter keeps
    the snap.
  - Reloads are clanks: steel modes held to about 1-4 kHz, a polymer or wood knock and a low thud; slides kept dark and
    well under the hits.
  - The Drover revolver (2026-10-09): the same recipe with its own shot (a bigger, slower pulse than the rifle's, a
    smaller one than the shotgun's, the cylinder gap's spit) and a cue per reload step (cylinder out, eject, rounds in,
    cylinder home), each starting where `Weapons/ReloadMotion.cpp`'s step does.
- Each recipe module's docstring describes its sounds in full; below are only the choices that hold across cues.
- **Creatures** sound like what they are: a spider's shell cracks when hit and it clicks, rasps and hisses (no voice); a
  slime is all jelly (no voice); the Unpaid are ghostly voices, a bullet through one a hollow thump and a puff of grave
  dust. Each kind has its own hit (`Creature.<Kind>.Hit`).
- **The melee strike** (2026-10-09): `Player.Melee.Hit` is the blow's weight alone; on a creature one layer says what
  gave (`HitFlesh`, `HitShell` the spiders' chitin, `HitGel` the slimes' gel, built from the creatures' own materials);
  on the world the bullet's impact is the layer. The swing is a 2D rush of air crossing right to left, quiet as it
  starts.
- **Fauna** (2026-10-09, `recipes/fauna.py`): birds are syrinx notes (near-pure tones on curved pitch contours), not
  voices; the crow's caw is a formant voice; wings are pushes of air. The flies loop keeps a whole number of wingbeats per
  fly, so it loops without a seam.
- **Props** (2026-10-09, `recipes/props.py`): physical first; heard in every yard, so the cracks stay off the 2-5 kHz
  band.
- **Throwables** (2026-10-09, `recipes/throwables.py`): the grave-salt grenade's throw (2D, from the off hand), bounce,
  fuse (a loop), burst (built like a gun's shot, salt crackling on top), the sear on the Unpaid and the pickup.
- **Voices** (2026-10-09, `recipes/voices.py`): the Unpaid's barks are murmured, never spoken: the game plays one
  breathy syllable cue (`Creature.Unpaid.Murmur` voiced, `.Mutter` whispered) per syllable of the line shown over them,
  so each starts at once and fades soon. Spiders and slimes get a rare idle call; the living on Ransom's Rest are heard
  through their walls.
- **The economy** (2026-10-12, `recipes/economy.py`): silver is a thin struck coin, pitched low, its 2-5 kHz band cut
  since a fight's worth of pickups chimes over and over; the pickup is 2D. The vendor's counter: pine boards, a small
  brass till bell, leather and a purse.
- **The Bell-Ringer's bell** (2026-10-12, `recipes/bells.py`): bronze, inharmonic partials split into slowly beating
  pairs, the upper ones dying first; the toll carries across the fight like a gunshot; a Gravebound ringer's cracked
  bell splits its pairs wide and buzzes. The tell is `Lift`; `Raise` is whispering voices over the bell's long hum. The
  vigil stones' iron bell (`recipes/shrines.py`) is this bell made duller and shorter.
- **The Venom Spitter** (2026-10-12, `recipes/creatures.py`): the tell is the cue the player learns to move on, so
  `Creature.Spitter.Tell` rises clearly to its end, 0.9 s long, the tell's length in the game.
- **Windows and gusts** (2026-10-14, `recipes/windows.py`, `recipes/gusts.py`): `World.Window.Steps` is a plank's dull knock (a
  boot's contact is milliseconds long), the joists' hollow and a lighter toe, low-passed through `voices._wall`; the
  curtain's rings tick over a cloth bed and are heard through a pane (`_glass`: the room's box, then the top off). The
  gust (`World.Gust.Pass`, 2D stereo, played at 0.45-1 volume by strength) is a swell whose hiss filter follows its own
  envelope and tops out near 2.8 kHz (`World.Gust.*` is in check.py's frequent list), board edge tones whose pitch follows
  the wind's strength, and a pan across the ears. `World.Gust.Rattle`: a shutter's knocks with louvre chatter, or a sign's
  iron links (modes kept under 3.8 kHz) and a slow bracket creak.
- **The town's animals** (2026-10-14, `recipes/animals.py`): the chickens' calls are `lib/voice.py` voices with the tract
  scaled small (`shift` 1.1-1.4) and two sources a hair apart (the syrinx's two membranes: the beating gives the reedy
  edge). The vocal-tract cascade dies off fast above 2 kHz, so a hen's call gets a band of chaos noise (the cluck) or a high
  shelf (squawk, crow) to put the top back. The cluck (the one heard all day, in check.py's frequent list) is vocal-fry
  pulses on a soft crop thump with its 2-5 kHz share at 5-11%. The crow is four parts (a short 'cock', a higher 'a', a long
  climbing 'doodle', a long falling 'doo' that crossfades to a rough, driven voice at its end) thrown back by the town's
  slap-backs. The cat's meow is one voice (shift 1.55) with a lowpass opening on a nasal hum and a vowel glide; the hiss is
  three sinking noise resonances over a rough ~100 Hz growl.
- **Spent brass** (2026-10-14, `recipes/brass.py`): a case is a thin brass tube, so its rings are near-harmonic bending modes
  at 5-7 kHz over a muted lower note; it bounces by the restitution rule (each hit softer by e, each gap shorter by e), and
  the surface adds the rest (stone's dry knock, a hollow board, a sheet's low note, dirt's thud, water's bubble plip, a
  hull's tube clack, brass-head clink and slowing roll). Rings are kept above 5 kHz so a burst's worth of casings stays off
  the 2-5 kHz band (`Weapon.Casing.*` and `Weapon.NearMiss.*` are in check.py's frequent list; presence share 1-12% for the
  casings, 7-21% for the fly-bys). Only the three attenuation classes exist (Gun, Creature, Near): the casings and fly-bys use
  Near (about 15 m); a 5 m class would need `ATTENUATIONS` in `build_sound_bank.py`.
- **Footfall cues** (2026-10-14, `recipes/player.py`; the game plays them from `FootfallCue` in
  `Player/PlayerFootfallComponent.h`): `Player.Land.Hard` (3 takes, 0.35 s) goes under `Player.Land` on a hard landing: a sine
  thump falling from 84 to 36 Hz, a long dark noise thud, the knees giving a beat later (about 87% of its power in 60-250 Hz)
  and a little dirt (dust, grit, a few clods) set 12-15 dB under the thud's loudest 10 ms so it is heard through the weight;
  it keeps no gear and no breath, which `Player.Land` has. `Player.Splash` (4 takes, 0.5 s; in check.py's frequent list) is a
  boot or body in shallow water: a soft pat (1.5 ms attack, no click), the dull step on the bed, a low bloop and bubbles, a
  crown of water that dies in about 0.15 s, the shin's slosh, drops pattering back and a ripple fade that ends it; each take
  a little bigger than the last (a lower bloop); 2-5 kHz share 4-8%, a faint 5-9 kHz spray sizzle on top so it isn't dull.
  Mix: `Player.Splash` volume_db -9.5 (a landing at full volume sits with `Player.Land`'s -9; a sprint step at 0.5 is 1 dB
  under a dirt footstep's -14.5); `Player.Land.Hard` -11.5.
- **Struck things** are modal: a set of decaying sines whose frequencies follow the object's physics (free bars, plates,
  wood, shells, cast bells), struck by a force pulse as long as the contact (hard steel ~0.1 ms, a heel several ms).
- **Friction** (scrapes, creaks, stridulation) is stick-slip: trains of tiny slips rung through the thing's resonances.
- **Voices** (creatures, breath) are formant synthesis: a harmonic glottal source with jitter, shimmer and breath, each
  harmonic weighted by a cascade of vocal-tract resonances that glide between vowels.
- **Spaces** are convolution with impulse responses made from noise: band-wise decays (highs die first), sparse early
  reflections, outdoor slap-backs.
- **The interface** speaks in brass and steel string: small brass ticks and chimes, plucked strings on a wooden box,
  kept quiet and off the 2-5 kHz band so they never tire the ear.

## What's learned

- Saturate noise *before* shaping its envelope: driving an already-decaying burst flattens the decay and turns a shot
  into a long hiss.
- Set a reverb's level against the dry sound's loudest 20 ms, not its single peak sample, or a crack spike skews it.
- A five-formant cascade dies off far too fast above 4 kHz; the upper formants' falls are eased (`voice.EASE`) to stand
  in for the resonances the series leaves out.
- Brown noise's lows wander too much between seeds for a thump layer; pink noise with a low cut weighs the same every
  time.
- A pitched sine sweep under a shot is a drum machine's kick: the user heard the guns as fake. Real weight is a
  broadband pressure pulse and the microphone's overload; judge a shot's lows by how tonal they are (round 2: 0.85 ->
  0.42) and how alike they are take to take, as well as by their share.
- Keep a shot's lows in one deterministic layer: low noise under it adds to or cancels the sine at random, and swung the
  low share about 20 points between variations.
- Mix noise layers by their loudest 10 ms, not their peak sample, so every variation weighs the same.
- The mix is set against the rifle's loudness: a denser rifle moves every cue (2026-10-08 the whole set shifted -2.2 dB
  when World.ShutterSlam reached the 0 dB cap). ShutterSlam and Impact.Wood now set the headroom.
- Every layer cut at its length gets a short end fade (envelopes and modal rings do it themselves), or the check finds
  clicks in the quiet tails.
- A cue that opens on something slow (a rub, a creak, a whoosh) starts with the small physical touch that begins it (the
  fingertips on the paper, the rope going taut, the valve lever's clank), or the check calls it late; a stamp that must
  land on a card's slam (0.12 s) is led by the hand's swish (`givers.swish`, already moving at the first sample).
- A crackle that thins to a few grains a second flags `CLICK` in its quiet tail (each lone grain stands over 12x the local
  level, since "quiet" is measured against the cue's loudest sample): keep the grain rate's floor high (the Spitter's
  sizzle: 1500 a second) and let the hiss bed under it last as long as the fizz.
- CUT TAIL measures the last 10 ms against the peak, so a cue that ends on something falling 60 dB in under about 35 ms
  (a bubble, a lone tick, a short thump) always flags: the last 10 ms still holds the decay's first 30 dB. End every
  short cue on a slower decay (a ripple's fade, a wobble, a shell's hollow ringing 50 ms, a soft hush of rolling), and
  start the fast event earlier.
- A very short quiet cue has little loudness (it is measured over 200 ms), so its `level` against the rifle is reached by
  a high `volume_db`: keep `level` low enough that the written `volume_db` stays at or under 0, or the whole set shifts
  down to make room (casings: level -16 to -27 gives volume -14 to -21).
- Noise layers mixed by peak (`normalize`) bury a soft thump under them: a flare, a breath or a crumple normalised to peak 1
  has several times the thump's energy. Mix them with `kit.set_level(layer, thump, rel_db, 0.01)` (Unpaid.Notice, Ash).
- A loop made of loud, sparse events (an engine's chuffs) puts its seam in a lull between two of them: the seam check
  compares the join's jump with the median step, which the quiet between events keeps small.
- Mix a loop's layers by loudness (`analysis.loudness_max`, `orelift._lv`), not by peak: a creak's peak says little about
  how it sits over a continuous hum.
