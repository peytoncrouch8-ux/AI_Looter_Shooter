# Sounds README: cut text

Moved verbatim from `Art/Sounds/README.md`, "How the sounds are built", when it was condensed (2026-10-13). Not superseded:
each recipe module's docstring (`Art/Sounds/recipes/<group>.py`) describes the same sounds in full, so the README keeps
only the choices that hold across cues. Never load by default.

- **Creatures** sound like what they are: a spider's shell cracks when hit and it clicks, rasps and hisses (no voice);
  a slime is all jelly (slaps, squelches, wobble, bubbles; no voice); the Unpaid are ghostly voices, and a bullet
  through one is a hollow thump and a puff of grave dust. Each kind has its own hit (`Creature.<Kind>.Hit`).
- **The melee strike** (2026-10-09) layers: `Player.Melee.Hit` is the blow's weight alone (a dull knock and a shove
  of air, driven into one punch), and on a creature one layer says what gave (`HitFlesh` a meaty slap, `HitShell` the
  spiders' chitin cracking, `HitGel` the slimes' springy squelch, built from the creatures' own materials); on the
  world the bullet's impact is the layer. The swing is a 2D rush of air crossing right to left, quiet as it starts.
- **Fauna** (2026-10-09, `recipes/fauna.py`): birds are syrinx notes, not voices: a near-pure tone on a curved pitch
  contour (chevrons, slurs, warbles, rolled 'rup's), weak harmonics, a breath of noise on the note, membrane flutter,
  louder where the pitch is highest; a sparrow take is two or three birds at their own pitches and distances trading
  notes, a swallow's twitter swoops past with its Doppler. The crow's caw is a formant voice (beaky tract, rough,
  rasped, overdriven). Wings are pushes of air (a whump and a feather rustle per downstroke; a crow's few, a sparrow's
  a twenty-a-second whirr). The flies loop keeps a whole number of wingbeats per fly, so it loops without a seam.
- **Props** (2026-10-09, `recipes/props.py`): the lootable world, physical first: crates and barrels bursting (boards
  cracking, nails squealing, staves and a hoop springing apart, the pieces clattering down), a grave dug, a coffin pried and
  its lid shoved off, a mailbox's tin door, a footlocker's lid. Heard in every yard, so the cracks stay off the 2-5 kHz band.
- **Throwables** (2026-10-09, `recipes/throwables.py`): the grave-salt grenade, a steel tin packed with salt: the throw
  (2D, a rush of air from the off hand), the bounce (a shell ringing, salt rattling behind), the fuse (a loop of fizz and
  sputter), the burst (built like a gun's shot, with salt crackling on top), the sear on the Unpaid and the pickup.
- **Voices** (2026-10-09, `recipes/voices.py`): the Unpaid's barks are murmured, never spoken: the game plays one breathy
  syllable cue (`Creature.Unpaid.Murmur` voiced, `.Mutter` whispered) per syllable of the line shown over them, so each
  starts at once and fades soon. Spiders and slimes get a rare idle call. The living on Ransom's Rest are heard through
  their walls (hushed voices, coughs, a music box, a latch, a floorboard, Tilly's saw, a dog far off).
- **The economy** (2026-10-12, `recipes/economy.py`): silver is a thin coin, struck: a small plate's few inharmonic
  modes, short and bright, with a hard tick of metal on metal, pitched low for a coin, with the 2-5 kHz band cut since a
  fight's worth of pickups chimes over and over. The pickup (2D) is a few coins dropping into a palm and a small two-note
  chime for the find. The vendor's counter is pine boards, a small brass till bell, leather and a purse: a purchase counts
  coins onto the counter and slides the goods across, a sale sets the gun down with a knock and counts coins into the hand,
  and a refusal shakes a light purse and pushes one coin back.
- **The Bell-Ringer's bell** (2026-10-12, `recipes/bells.py`): an oversized bronze hand bell, thin-walled, so struck it rings
  partials that are not a harmonic series, each split into a slowly beating pair by the casting's roundness, the upper
  ones dying first, with little hum; the iron clapper's contact is short and bright. Rung in the hand it keeps swinging, so
  the ring waxes and wanes. The toll is carried across the fight like a gunshot (slap-backs off the buildings and ridges);
  a Gravebound ringer's cracked bell splits its pairs wide and buzzes, sour and choked. The tell (`Lift`) is the clapper
  knocking about as the bell is shaken, faster toward the stroke, and a cold breath drawn in; `Raise` is whispering voices
  drawing up out of the ground over the bell's long hum. The vigil stones' iron bell (`recipes/shrines.py`) is this bell
  made duller and shorter, its upper partials dying fast, with candle flames (combustion's breath, low and soft) and stone
  grinding up through earth (a rough judder with grit and clods).
- **The Venom Spitter** (2026-10-12, `recipes/creatures.py`): the spider's hiss and rasp, wetter, with the sac sloshing,
  and its venom a thick bubbling liquid that splats, spreads and sizzles. The tell is the cue the player learns to move
  on, so `Creature.Spitter.Tell` rises clearly to its end (a glug quickening and climbing, bubbles coming faster, a hiss
  rising to the fangs), 0.9 s long, the tell's length in the game.
