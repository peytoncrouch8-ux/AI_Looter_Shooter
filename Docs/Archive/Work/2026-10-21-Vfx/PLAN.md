# VFX pass (12.5, 2026-10-21 night): plan, what's written, merge

The user, after playing: hit, death, muzzle and power effects look weak/cheap; wants Borderlands-like punch (chunky readable
hits, satisfying deaths, muzzle flashes with weight, powers that look like powers) inside 8.3 ms on Medium. Rules kept: no
new lights, no Nanite glow (every new quad is on the engine Plane), no particle assets (code-drawn, FWeaponFX), cvars for
every knob (`Looter.Vfx.*`), plain rules tested (`Looter.Vfx.*`). Code in `Saved/VfxCode` (holding tree, unbuilt);
merge notes in `Saved/VfxCode/README.md`.

## Why it reads cheap today (from the code)

1. **One shape language.** Everything is a round soft disc (Flash), a thin streak (Spark/Streak) or a soft smoke disc. No
   rings, no starbursts outside the gun's own flash, no fire that cools: impacts and deaths read as "a blob of light".
2. **Smoke is perfect gradient discs** (M_FX_Smoke): puffs look like sprites, never like billowing dust; they fade
   uniformly instead of breaking up.
3. **Short and small.** Flashes 0.05-0.1 s, 14-30 cm; hit spurts 7-12 tiny drops (1.6-2.8 cm) that vanish in 0.5 s; a kill
   is ~30 particles round the body for ~1 s. At 20-40 m most of it is sub-pixel: the hit doesn't read.
4. **No weight.** Nothing big is thrown high and watched falling; nothing lands and stays (except splinters for 1-2 s);
   no ground mark under a kill.
5. **No hierarchy.** A crit is the same hit plus one gold disc; a Gravebound kill looks like a Regular's; an SMG and a
   sniper hit look the same (only `MarkScale` sizes them); every gun kind's muzzle is the same 5 quads.
6. **Powers' hits leave nothing on the body.** `UEmberPowerBase::Hurt` applied damage only: no spurt, no pop. Activations
   were a soul-light burst at the feet (mostly out of view in first person).
7. **The rocket's blast** (GUNS-CODE) is SpawnFlash + the SoulLight death burst + puffs: a big round glow, not fire.

## The new tools (shared by everything below)

| Tool | What | Cost |
|---|---|---|
| `M_FX_Shape` (new master, `build_vfx_materials.py`) on a new FWeaponFX "Shapes" ISM | One quad each: **ring** (soft band, bright leading edge, noise-broken; flat on the ground or facing the eye), **fireball** (noise ball cooling white-hot -> its colour -> dull ember, eroded by noise), **starburst** (3-16 uneven spikes, hot core). Without the material: stand-ins on M_FX_Glow (ring = 10-28 streak pieces, fireball = heat-coloured disc, star = its 6-point star) | +1 draw call; small additive quads; procedural HLSL ~40-60 ALU/px |
| `M_FX_Puff` (new master) replaces M_FX_Smoke on FWeaponFX's smoke when present | Two octaves of value noise, drifting; **erodes into wisps** as it ages (custom data seed + erosion); denser = darker; soft depth fade. Water rings get seed -1 = plain disc | Same draw; ~+40 ALU/px of translucent smoke overdraw |
| `M_FX_SplatDecal` (new master) + `FDeathSplats` pool in `UVfxSubsystem` | Procedural DBuffer decal: noisy-edged pool spreading out (Spread param animated), droplets past the edge, wet = glossy. Ichor, dark ichor, gel (slime colour), ash, scorch | <=12 small decals (`Looter.Vfx.Splat.Max`), ~0.05 ms |
| Detail rules (`VfxRules`, `FVfxHitStream`) | Pieces x1 within 15 m, easing to x0.35 at 60 m; a **stream** into one spot (SMG/LMG) thins each next hit 1/sqrt(n-1) to x0.25; the hit's pop (one quad) is never thinned | Saves the new guns' cost |
| Drop streak stretch (Draw) | Drops/gel drawn 1.3x-3.5x long along their speed: spurts read as **spray**, not beads | free |

## Items: today -> great (all written unless marked "proposal")

### Creature hits by blood (`WeaponFXCreatures.cpp`; `Looter.Vfx.Hit.Scale`, `.Hit.Pop`)
Today: per blood a small flash, 7-9 drops/bits, 1 mist puff, exit spray (Gunfeel); crit = gold disc.
Great (written): every hit gets (1) a **splat** - a dense puff of the spill bursting 8->46 cm and eroding in 0.22 s (the
Borderlands blood puff), (2) a **starburst pop** sized by the round (pellet 0.6x, SMG 0.9x, sniper 1.5x; pale/wet for
ichor & gel, hot for coal): the thing that reads at 40 m, never thinned, (3) **2-3 heavy gobs** lobbed high that arc and
splat (weight), (4) drops as streaks of spray. Ichor: green-pale; Dark ichor: dark gobs; Gel: fat blobs in its tint;
Embers (Unpaid): a hot 7-spike star + a **coal flare fireball**; Miner: same + rock dust splat. **Heavy rounds** (revolver,
sniper): a pale ring snapping out + exit spray x1.6. **Crit**: gold 10-spike starburst + a gold ring 6->55 cm facing the eye
(on top of today's pop). Pellets keep the blast budget (a spent pellet still throws nothing; tests unchanged).
Cost: +4-6 particles a hit (+3 shapes on a crit), thinned by distance/stream. SMG/LMG streams: ~0.5x today's per-hit count
after the 3rd round.

### Surface impacts by material (`WeaponFXImpacts.cpp`; `Looter.Vfx.Impact.Scale`)
Today: per material flash, 2-14 sparks, 1-3 puffs, chips/bits; no column, no shape.
Great (written): **soft ground** throws a **geyser** (puffs shot up the normal, each slower, stacking into a 60-90 cm column
that slumps) + a **skirt** rolling out over the surface + 2 big clods: Dirt (3+3+2), Grass (2 + torn turf), Sand (4-puff
spray column + skirt), Hay (chaff jet), Water (taller drop column + mist jet). **Hard** surfaces pop a starburst: Stone (8
spikes, dust jet, 2 big chips), Gravel (small pop, grit jet), Metal (10-spike white-hot pop, a **ping ring**, 3 long-bouncing
sparks), Sheet metal (pop + small ping), Glass (cold 12-spike glint), Bone (pale pop). Wood: sawdust jet + 2 long splinters.
Crit adds a gold starburst. Cost: +3-7 pieces a hit; streams thinned (test: ten rounds into one spot < 8 lone hits).

### Kills and deaths by kind and tier (`WeaponFXWorld.cpp` bursts, `WeaponFXKills.cpp` kill layers, `CreatureKillLook`, the React files)
Today: the kind's burst (Shell ~28, Gel ~23, SoulLight ~29 particles) + revolver/sniper crit bigger at the crit spot; tiers
Hallowed/Ethereal have their own show (pillar, ring, motes), Restless/Gravebound nothing extra; nothing on the ground.
Great (written):
- **Base bursts** (every caller: kills, powers' flares, globs, the rocket today): Shell + a wet starburst pop, an ichor splat
  bursting to 120 cm, 18 plates (4 big 8-12 cm thrown high), 12 drops. Gel + a gel splat and a **crown** of 10 blobs thrown
  out low. SoulLight + a white 12-spike crack, **two fireballs** of its colour rising and cooling, a **shock ring** facing the
  eye (size >= 50 cm).
- **Kill layers** (on the kind's death, `CreatureKillLook::Play`, weighted by `VfxRules::KillWeight`: gun x crit x overkill;
  thrown along the shot by `KillCarry`: shotgun 0.85 .. rifle 0.35): spiders 8 ichor gobs + 6 shell shards + 3 leg pieces
  landing and lying on the ground, a wet mist that hangs and settles, a dust skirt; slimes 8 fat blobs + gel mist + low spray;
  Unpaid/miners a **ring of the coal's light racing over the ground** (30->230 cm), ash collapsing into a heap of dust, 14
  embers fountaining (+ rock dust and stones for miners); a rocket kill adds soot; a crit adds a big gold starburst + ring at
  the wound. **Tiers**: Restless 1 ground ring + star + 6 motes in its colour; Gravebound 2 rings (second slower, wider) +
  bigger star + 12 motes; Hallowed/Ethereal only a star (their own show is the moment); Soulfed 2 wide rings, big star, 16 motes;
  bosses nothing (their show).
- **Ground splats**: spiders' ichor pools out from under the body as it lands (1.4 s spread, wet sheen), the Gravemother's
  darker; slimes a gel puddle splashed out in their colour; Unpaid/miners a ring of ash. Hold 9 s, fade 3 s (the corpses lie
  10 s). Kills > 90 m from every view play no layers/splats.
Cost: a kill ~+35-55 particles + 0-5 shapes + 1 decal (was ~28). 8 kills in one second: ~+400 particles for ~1.5 s.

### Muzzle flashes per gun kind (proposal for GUNS-CODE's `WeaponBaseFiring.cpp`; FWeaponFX side written)
Today: same 5 quads (core 11, star 34, 3 tongues 26) for every kind, 0.055 s, scaled by `MuzzleFlashScale`; 1 point light.
Great: per-kind flash from `VfxRules::MuzzleLook` + `FWeaponFX::SpawnMuzzleBurst` (written): Rifle today's + spark/smoke
every 2nd shot; Shotgun wide 5-tongue bloom, 9 powder sparks, 3 puffs; Revolver 8-point star + cylinder-gap spit; **Rustler
SMG** 4-point flicker 0.035 s, sparks/smoke every 3rd; **Longhorn sniper** big star, brake **side jets** + their smoke rolling
sideways, a smoke **blast ring** and a faint hot-air ring (3rd person), 4 hanging puffs; **Thresher LMG** heavy 4-tongue
flash, smoke every 2nd; **Powderkeg** 12-spike gout, 200 cm blast ring, 5 puffs. First person keeps smoke 45 cm out and no
ring. Exact code in `Saved/VfxCode/Proposals/GUNS_CODE_muzzle_rocket_tracers.md`. Cost: 3-14 particles a shot.

### Tracers (proposal; `Combat/BulletSubsystem.cpp` DrawTracer, LOOT-CODE's hooks file)
Today: streak + hot head + smoke thread, colour/width per gun definition.
Great: SMG thin short needles; LMG every 3rd a full tracer, others half glow (belt read); sniper 1.6x width, white-hot head,
a vapour thread 2.5x wide/long that hangs (signature); revolver 1.2x; pellets 0.6x long; a round head glow on tracers past
15 m (reads as a dot at range). Cost: +1 quad per far tracer.

### The rocket's blast (proposal for GUNS-CODE's `WeaponRocketBlastEffects.cpp`; `SpawnExplosion` written)
Great: white 14-spike crack + flash, 5 fireballs blooming white -> ember orange -> dull red and eroding, a ring racing over the
ground to 1.6x the reach, a faint air shock, soot skirt (12) and 3 s column (6), 22 embers arcing and skittering, the ground
thrown in its own look (geyser, chips), 10 debris clods, a scorch decal (9 s). ~90 particles + 8 shapes + 1 decal.

### Ember powers (written: `EmberVfx` helpers; `EmberPowerBase::Hurt` -> every power's blow shows on the body)
Shared: `Activation` (a ring of the power's colour racing 30->340 cm over the ground from the feet + a slower fainter one +
10 rising motes: reads in first person, unlike a burst at the feet), `BodyHit` (the creature's own spill sized by the blow,
damage 20 = a rifle round, + a starburst of the power's colour), `Strike` (starburst + ring facing the eye), `Blast`
(SpawnExplosion in the power's colour; soul = pale smoke, rising motes), `Column` (a shaft of light falling onto a spot + ring).
- Dust Devil: start ring (0.8) + arrival ring (0.45); each struck creature spills + a wind strike.
- Sundown: a wide amber ring (1.3) with the existing dusk burst.
- Borrowed Time: violet ring on open; a strike on each kill that pays the debt; a darker, shorter ring when the debt is called in.
- Lucky Streak: gold starburst on each lucky pull's muzzle; a gold strike where each ricochet lands; a gold ring on use.
- Fifty-Two Pickup: bodies spill per frame of cuts (Hurt); 3+ cards together add a strike of its colour (bigger per card).
- Double or Nothing: heads = a starburst of its colour over the flash; the coin's catch a gold star + ring.
- Toll Chain: the bite's strike at the chest (and the spill).
- Grave Ward: a ring racing out exactly to the ward's radius + activation ring.
- Last Rites: the burst becomes a **soul blast** (fireballs of its colour, ring to 1.1x its reach); the strike a **column of
  light** (9 m) falling onto the marked creature.
- Slag Bomb: the burst becomes a **molten blast** (fireballs white -> slag orange, ground ring, soot, embers) over today's flash,
  dirt, globs and pool.
- Iron Hide: a forge-orange ring as the plates slam shut (no flashes on the player's own body: it'd blind first person).
- Rail Spike: pierce adds a white-hot starburst + a ball of heat blown out the far side; embed a strike + a flat iron ring
  on the surface.
- Spyglass: a ping (small strike) over each creature as the sweep marks it.
- Tripwire: a strike where the wire catches.
- Long Shot: hits worth 1.3x+ add a teal strike sized by the bonus.
Cost: per use +10-30 particles + 2-4 shapes; per blow +6-10; Slag/Last Rites blasts ~70-90 for ~3 s.

## Perf estimate (RX 580, 1080p Medium; today: 10 spiders fighting in town 7.5-7.7 ms)
- CPU: more live particles in a fight (typ. 250 -> ~400, cap 900 -> 1000 `Looter.Vfx.MaxParticles`); FWeaponFX Redraw ~0.3-0.5 us
  each -> +0.05-0.1 ms; 1 more ISM (Shapes); 1 trace per kill (ground), decals (<=12) in DBuffer.
- GPU: translucent overdraw of more and noise-shaded puffs is the cost: est. +0.15-0.3 ms in a busy fight, +0.4-0.6 ms for
  about a second after a rocket or Slag Bomb at close range. Additive shapes are small, short (<0.5 s): <0.05 ms.
- Expected: fight ~7.8-8.0 ms (inside 8.3), spikes to ~8.3-8.5 ms for ~1 s on blasts within ~5 m of the camera.
- Knobs if over: `Looter.Vfx.Smoke 0.7` (opacity), `Looter.Vfx.Detail.Near 1000`, `.Detail.Min 0.25`, `Looter.Vfx.Puffs 0`
  (plain smoke shader), `Looter.Vfx.Blast.Scale 0.8`, `Looter.Vfx.MaxParticles 700`. Measure: `Tools\perf.ps1 -Label "vfx pass"
  -Exec "Looter.Quality Medium"` + `Looter.Perf.Horde Spider 8` in town (`0 -1400 90`) firing the SMG and LMG, and a rocket.

## Not done / risks
- Nothing built, run or seen: every number is a first guess to tune by eye in PIE (single `grab.ps1` grabs).
- M_FX_Shape/M_FX_Puff/M_FX_SplatDecal: HLSL written blind (Custom nodes); first run may need a fix in the editor log
  (`VFXMATS` lines). Without them the code draws stand-ins and lays no splats, so the merge can build first.
- The water rings' dashes on M_FX_Puff use its plain-disc path: check they look as before.
- `EmberDustDevil.cpp` was 742 lines, now 755 (over the ~500 guide before this pass; split by topic later).
- Element looks (LOOT-CODE's `WeaponFXElements.cpp`) can use `SpawnFireball` / `SpawnStarburst` / `SpawnShockRing` once merged
  (Ember burn = small fireballs, Soul-light smite = starburst + ring, Grave-salt = cold starburst).
