# Gun feel pass: plan and contract (GUNFEEL planner, 2026-10-22)

The user's pick (2026-10-22): "Gun feel pass: sounds, recoil, impacts and reloads per gun kind, compared against top
shooters." Their grill answers (Handoff "Direction"): guns feel alike; gun sound and kick; enemy hit reactions. Recorded
royalty-free sounds allowed for guns and impacts only (licence kept with the pack); everything else script-made. Read
with `Docs/Pipeline/Gunplay.md` (what 12.1-12.5 built). Every new number is a live cvar; defaults in brackets.

## What top shooters do (general practice, nothing copied)
- Borderlands 3: each maker sounds and reloads its own way; loud layered shots; big chunky reloads with a hard last beat;
  a bright crit ping distinct from the body tick; enemy shields crack audibly.
- Destiny 2: each archetype has a voice and a reload rhythm; crit hit-marker and sound unlike a body hit; a heavy kill
  crunch; tight, learnable vertical recoil through the sights; the gun fires a beat before the reload animation ends.
- DOOM Eternal: huge transients and low end; the shotgun is the star (pump/shove); hits read by damage (bigger gore,
  stagger); screen shake small, the weight comes from sound and the gun model's kick.
- Apex: learnable recoil patterns per gun; empty reloads slower than tactical; reload commits when the mag seats;
  armour-coloured hit-markers and a loud shield-break crack; a low-ammo tick; gunfire distance versions carry info.

## Today, in short (what makes kinds feel alike)
1. Reload sounds shared: Sniper box `MagOut`/`MagIn` and the SMG's knob (`EReloadStep::Bolt`) play the rifle's cues
   (`Weapons/WeaponSounds.h:56-98`; `ReloadMotion.cpp:88-109`). The three core guns' reload sounds are synthesized
   (`Art/Sounds/recipes/guns.py:270-460`) while their shots are recordings: the reload sounds toy-like next to them.
2. One reload per gun: no empty vs tactical; rounds go in only at the end (`WeaponBaseReload.cpp:139`), a swap at 95%
   loses all of it (`CancelReload`); same step fractions for rifle/SMG/LMG (`ReloadMotion.cpp:6-12`: 0.16/0.62/0.70)
   at 2.1 s or 4.2 s; shotgun always pushes 4 shells (`ReloadMotion.cpp:77`, `ShellPushes == 4`) whatever the tube
   (EmberTube6) or the rounds missing.
3. The Ranchhand is a pump gun that never pumps between shots: the hull flies 0.15-0.35 s after (`WeaponBaseBrass.cpp:
   36-38, 81-84`), nothing moves, the only "rack" is a 0.03 s recorded clack inside its Mech layer (`guns_shots.py:157`).
4. The gun model kicks the same aimed or not (`PlayerLocomotionViewModel.cpp:322-326`): revolver flip 8 x Recoil 1.4 =
   11.2 deg in the iron sights (`create_revolver.py:25`); only the view kick halves through the sight (`ViewKick.cpp:172-180`).
5. Kick scales cluster: `Kick.*` 1.0-1.8, `Recoil.*` 1.0-1.6 (`GunfeelKickTuning.h:23-75`); side kick purely random
   (`AimKickSide`), no first-shot reward, no pattern to learn.
6. Impacts and creature hits sound the same from every gun: one surface cue at one volume (`BulletSubsystemImpacts.cpp:
   192-201`); the creature's hit thud at 0.8 (1.0 crit), 0.06 s apart (`CreatureVoiceComponent.cpp:154-157`, `.h:63`).
   Visuals do scale by `FBulletMarks::GunScale` (`BulletMarks.cpp:105-127`: shotgun 0.6, SMG 0.8, rifle 1, LMG 1.05,
   revolver 1.25, sniper 1.5).
7. Hit-marker: one cue `UI.HitMarker` (-5.5 dB) for every hit; a crit only pitches it 1.12 (`PlayerHUDWidget.cpp:383`,
   `LooterSoundRules.h:67`); no ward-hit or ward-break sound; the kill confirm `UI.Kill` is quiet (-12.5 dB, played by
   `CreatureVoiceComponent.cpp:191`). Only revolver/sniper crits ring (`CreatureHitReactionComponent.cpp:200`).
8. Low ammo: a quarter of every magazine clicks (`LowAmmo.Share` 0.25): the Thresher clicks 25 times, the Rustler 10-15.
   No last-round sound. Long guns share `Weapon.Equip`/`Weapon.Holster`/`Weapon.AimIn`.
9. Every own shot plays Core+Mech+Low+tail (`GunshotSound.cpp`): at 900 rpm that is 60 voices/s; tails (conc 6) cut each
   other and the burst's end has no long tail.

## Per gun kind (identity; today; changes in sound / kick / impacts / reload)
Base stats: SMG 11 dmg 900 rpm mag 40-60 reload 2.1; Sniper 150/45/5-8/3.0; LMG 16/650/80-150/4.2; Launcher 320/40/1-3/2.8
(`create_new_guns.py:41-70`); Revolver 52/140/6/2.3 (`create_revolver.py:24`); Rifle and Shotgun live in the
DA_AssaultRifle/DA_Shotgun assets only (Main's step 0 probe dumps them into this file).

**Bullpup (Rifle): "the workhorse: a crisp tight crack, a climb you can hold, accurate in short bursts."**
Today: view kick 0.32 deg x Kick 1.6 = 0.51/shot at 14 Hz; climb 0.06/shot to 1.8x; bloom 0.15; all sound layers.
- Sound: `Weapon.Rifle.LastRound` (bolt locking back, a hollow ping); low-ammo last 6 rounds; tail every 2nd shot in a
  burst + the burst-end tail. Reload steps re-made from recorded foley if the user agrees (decision 1).
- Kick: Kick 1.6 -> 1.3; first shot of a burst x `Kick.FirstShot` 1.3; pattern: straight up for 6, then drifts right
  0.04 deg/shot; random side x0.4; ADS model kick (all kinds) flip x0.45, roll x0.4, back x1.2.
- Impacts: weight 1 (the reference); every hit full.
- Reload: tactical x0.9 (no charging handle), empty x1.1 (charging handle slap); commit at `MagIn` seat.

**Ranchhand (Shotgun): "a cannon on the porch: one huge boom, a shove, the pump racked, bodies thrown."**
Today: 1.15 deg x 1.5 = 1.73/shot at 6.5 Hz; falloff 0.6-1.6 x range; knock-back; no pump between shots; 4 shells always.
- Sound: new `Weapon.Shotgun.Rack` (fore-end back and home, two beats) on the rack; Mech loses its recorded clack (the
  rack carries it); one `Impact.Scatter` pepper per blast + the surface cue louder (not a cue per pellet).
- Kick: Kick 1.5 -> 1.9 (2.2 deg hip); the rack adds a small view beat; settle 0.8 kept.
- Impacts: pellets 0.5 weight each, one dust sheet across the pattern's centroid (hard: spark scatter; soft: a fan).
- Reload: shell by shell: start 0.3 s, each shell 0.42 s and +1 round as it seats, pump 0.35 s only if empty; a trigger
  press finishes the shell in hand, then fires. Pump between shots: rack 0.45 of the shot interval (0.22-0.5 s), the
  hull flies at the rack's back point (replaces BrassHullShare 0.35 / 0.15-0.35).

**Drover (Revolver): "the gunslinger's iron: a sharp bark, the hammer and cylinder, a ringing crit."**
Today: 0.9 deg x 1.4 = 1.26/shot at 11 Hz; crit 1.75x, crit ring, hit-stop 0.09; cylinder swing reload, all six out.
- Sound: low ammo last 2 rounds; `Weapon.Revolver.LastRound` (hammer falls, the cylinder clicks on); crit ring kept.
- Kick: Kick 1.4 -> 1.6; ADS model flip x0.45 (11.2 -> 5 deg: the sight comes back on target before the next pull).
- Impacts: weight 1.3, an extra chunk burst on hard surfaces; `Impact.Heavy` from mark scale 1.2.
- Reload: unchanged rhythm (its identity); commit at `RoundsIn`; tactical = empty (all six dumped, live rounds back to
  the pool).

**Rustler (SMG): "a buzzing helix: a light fast hum, little kick, the glow running down."**
Today: 0.2 deg x 1.0 at 18 Hz; climb 0.03; bloom 0.09; reload knob = the rifle's bolt cue.
- Sound: tail every 3rd shot + burst-end tail; low ammo last 10 as a rising helix whine-tick; `Weapon.SMG.LastRound`
  (the helix winding down); own `Weapon.SMG.Charge`; `Weapon.Equip.Light`.
- Kick: Kick 1.0 -> 0.8; pattern: a vertical shimmy (alternating 0.03 deg side); first shot x1.3.
- Impacts: weight 0.6 (sparks small, stream thinning kept); never covers the target.
- Reload: tactical x0.9, empty x1.1 (+ `Weapon.SMG.Charge`); commit at `HelixIn`.

**Longhorn (Sniper): "one thunderclap across the valley: the bolt worked, the crit rings out."**
Today: 1.3 deg x 1.6 = 2.1 at 7.5 Hz; bolt cycle; crit 2x; box MagOut/MagIn = the rifle's.
- Sound: own `Weapon.Sniper.MagOut`/`MagIn` (a heavier box); low ammo last 1; `Weapon.Sniper.LastRound`;
  `Weapon.Equip.Heavy`.
- Kick: Kick 1.6 -> 2.0 (2.6 hip, 1.3 scoped); scope hidden from 0.5 strength, so the model kick stays.
- Impacts: weight 1.8: a punch ring, a tall dust column, big chunks; `Impact.Heavy` always; on creatures
  `Creature.Hit.Heavy` always.
- Reload: tactical x0.9 (box only, bolt stays home), empty x1.0 (bolt open/close); commit at `MagIn`.

**Thresher (LMG): "a spinning saw: winds up, chugs heavy, the barrel glows, keeps the pressure on."**
Today: 0.42 x 1.3 = 0.55 at 11 Hz; spin-up 0.6 -> 1 over 0.35 s; heat glow; box reload 4.2 s; 25 low-ammo clicks.
- Sound: tail every 2nd shot + burst-end tail; low ammo last 15; `Weapon.LMG.LastRound` (belt runs out, a rattle);
  `Weapon.LMG.HeatHiss` at burst end when heat > `Sound.HissFrom` 0.6; `Weapon.Equip.Heavy`.
- Kick: Kick 1.3 -> 1.2; pattern: a slow side sway (sine 1.5 Hz, 0.15 deg); sustained cap applies.
- Impacts: weight 1.05; every 3rd round a full chunk burst (the tracer's rhythm), the rest light.
- Reload: one rhythm (box always: x1.0); commit at `BoxIn`.

**Powderkeg (Launcher): "a keg of fire: the whoomph out, the rocket's hiss, a blast that shakes the ground."**
Today: 1.4 x 1.8 = 2.5 at 5 Hz; blast, view jolt, explosion near/far cues; rocket slide reload.
- Sound: kept; `Weapon.Equip.Heavy`. No low ammo (mag 1-3).
- Kick: Kick 1.8 -> 2.0 (2.8 hip, under the 3 deg stack cap).
- Impacts: its blast (VFX-2) kept.
- Reload: x1.0; commit at `RocketIn`.

## Cross-kind changes
- **Hit feedback (all guns)**: body `UI.HitMarker` softer; new `UI.HitMarker.Crit` (bright two-tone ping, +2 dB over the
  tick; revolver/sniper add their ring on top); `UI.HitMarker.Ward` (glassy tick) when a Warded enemy's ward takes it;
  `UI.WardBreak` (crack + shimmer) when it breaks; `UI.Kill` -12.5 -> about -8 dB with a low thump; `UI.Kill.Crit`
  for a crit kill. Marker: crit orange (`Color::Accent`, kept), ward cyan, kill red (kept); size 1 + 0.4 x
  min(damage share / 0.25, 1); a blast's pellets show one marker sized by pellets landed.
- **Creature hits (3D, at the wound)**: `Creature.Hit.Heavy` thump layer for mark scale >= 1.2 or a blast share >= 0.35;
  `Creature.Crit.Shell` (chitin crack: spiders, Spitter, Gravemother), `.Gel` (wet pop: slimes), `.Coal` (ember snap:
  Unpaid, miners, Foreman, Bell-Ringer, Abel; Clay uses Gel). VFX: crits by blood (Ichor: shell shards; DarkIchor: thick
  gobs; Gel: core flash ring; Embers: coal flare + soul-light gleam; MinerEmbers: rock chips; Clay: clods).
- **Impacts (13 surfaces)**: surface cue volume x clamp(0.75 + 0.25 x mark scale, 0.8, 1.2); `Impact.Heavy` (sub-thump
  + debris) under it from mark scale 1.2; `VfxRules::ImpactWeight(Kind)` counts as above; wood gets long splinters
  (today generic chips), sheet metal a spark shower, glass shards, hay a puff of straw. Decals: `BulletMarks` size already by
  gun; sniper/revolver marks get a crack ring on stone/wood.
- **Screen-shake budget**: per shot hip <= 2.8 deg (Launcher), ADS half; an auto's summed shot kicks held under
  `Kick.SustainCap` 1.2 deg; kills, blasts, hurts on top within the stack's 3 deg / 5% FOV (`ViewKick.h:57-58`); the
  Camera shake setting scales all (unchanged). The weight comes from sound and the model, not the camera.
- **Fire rate / spread**: first shot after 0.35 s rest x `Spread.FirstShot` 0.6 for rifle/SMG/LMG (tap fire rewarded);
  bloom numbers kept. Reload ready a beat early: `Reload.ReadyEarly` 0.08 s before the motion ends.
- **Fast-reload window: no** (a new system under the core-loop freeze; not Borderlands' way). Decision 4 if the user wants one.
- **Reload foley**: every reload starts with `Weapon.Reload.Cloth` (glove and sling); long guns' equip by weight.

## Shared contract (names; all cvars `Looter.Gunfeel.` + name)
Kick (GF-KICK): `Recoil.AimedFlip` [0.45], `Recoil.AimedRoll` [0.4], `Recoil.AimedBack` [1.2], `Kick.FirstShot` [1.3],
`Spread.FirstShot` [0.6], `Recoil.Pattern` [1], `Recoil.SideRandom` [0.4], `Kick.SustainCap` [1.2]; new defaults
`Kick.Rifle` 1.3, `.Shotgun` 1.9, `.Revolver` 1.6, `.SMG` 0.8, `.Sniper` 2.0, `.LMG` 1.2, `.Launcher` 2.0.
API: `GunfeelKick::PatternSide(EWeaponKind, int32 ShotInBurst)` (deg), `GunfeelKick::AimedModelKick(float AimAlpha,
FVector& BackFlipRoll)`, `FWeaponRecoil::AddShot(Profile, Random, float AimAlpha)` (old overload kept, AimAlpha 0).
Reload (GF-RELOAD): `Reload.Tactical.<Kind>` / `Reload.Empty.<Kind>` (Rifle 0.9/1.1, SMG 0.9/1.1, Sniper 0.9/1.0, LMG,
Revolver, Launcher 1.0/1.0), `Reload.CommitAtSeat` [1], `Reload.ReadyEarly` [0.08], `Reload.Shotgun.Start` [0.3],
`.Shell` [0.42], `.Pump` [0.35], `.Interrupt` [1], `Shotgun.RackShare` [0.45], `.RackMin` [0.22], `.RackMax` [0.5]
(shell/rack times x the gun's ReloadTime / its definition's base, so Reload parts and makers still count).
API: `struct FReloadPlan { bool bEmpty; int32 Shells; }`, `LooterReload::StepsFor(EWeaponReloadPart, const FReloadPlan&,
TArray<FReloadStepAt>&)`, `LooterReload::PumpCycleTravel(float)`, `PumpCycleViewPose(float, FVector&, FRotator&)`;
`WeaponSounds::ReloadStep(EWeaponKind, EReloadStep)` (kind-aware); `AWeaponBase::IsReloadEmpty()`,
`GetPumpCycle()` (-1 none). `EReloadStep` gains nothing (SMG charge = `Bolt` mapped by kind).
Sound (GF-SOUND-CODE): `Sound.AutoTailEvery.Rifle` [2], `.SMG` [3], `.LMG` [2], `Sound.BurstEndTail` [1],
`Sound.LowAmmo.<Kind>` rounds (Rifle 6, SMG 10, LMG 15, Shotgun 2, Revolver 2, Sniper 1, Launcher 0; pitch rule
`LowAmmo.PitchTop` kept), `Sound.LastRound` [1], `Sound.HissFrom` [0.6], `Sound.ImpactHeavyFrom` [1.2],
`Sound.CreatureHeavyFrom` [1.2]. API: `GunshotSound::PlayBurstEnd(const AWeaponBase&)`, `PlayRack(const AWeaponBase&)`.
Cues (`Audio/LooterSoundCues.h`, GF-SOUND-CODE adds all; GF-SOUND-ART makes all): `Weapon.<Rifle|SMG|LMG|Shotgun|Revolver|
Sniper>.LastRound`, `Weapon.Shotgun.Rack`, `Weapon.LMG.HeatHiss`, `Weapon.SMG.Charge`, `Weapon.Sniper.MagOut`,
`Weapon.Sniper.MagIn`, `Weapon.Reload.Cloth`, `Weapon.Equip.Light`, `Weapon.Equip.Heavy`, `Weapon.Holster.Heavy`,
`UI.HitMarker.Crit`, `UI.HitMarker.Ward`, `UI.WardBreak`, `UI.Kill.Crit`, `Creature.Hit.Heavy`, `Creature.Crit.Shell`,
`Creature.Crit.Gel`, `Creature.Crit.Coal`, `Impact.Heavy`, `Impact.Scatter`. Levels: own-gun layers still sum to
Fire within 0.5 dB; peaks under -1 dBFS (the revolver's -0.1 indoors: lower its Low/Mech 1 dB).
VFX (GF-IMPACT): `Looter.Vfx.Impact.Weight.<Kind>` (SMG 0.6, Rifle 1, LMG 1.05, Revolver 1.3, Sniper 1.8, Shotgun pellet
0.5), `Looter.Vfx.Impact.LMGFullEvery` [3], `Looter.Vfx.Hit.CritPop` [1]; `VfxRules::ImpactWeight(EWeaponKind)`.

## Jobs (disjoint files; C++ into holding trees `Saved/<Job>Code/...` with HASHES.txt; no builds, editor, commits)
**GF-SOUND-ART (Opus)**: `Art/Sounds/recipes/guns*.py`, `feedback.py`, `guns_impacts.py`, `Art/Sounds/check.py` (FREQUENT
gains the new hit and shot cues). Makes every cue above; Rack split out of shotgun Mech (re-check the Fire sum); renders
with `Tools\sounds.ps1` (no editor), 0 check flags, presence 2-5 kHz under the 8 limit; listening sheet PNG + WAVs for
the user. If decision 1 is yes: reloads of Rifle/Shotgun/Revolver rebuilt on `Foley_G36C_Mag`, `Rifle_Cock`,
`Shotgun_Saiga_Cocking`, `Revolver_M29_Cylinder` (already in `packs/Sonniss`). Test: `Looter.Audio` after Main's import.
**GF-RELOAD (Opus)**: `Weapons/ReloadMotion.*`, `WeaponBaseReload.cpp`, `WeaponSounds.h`, new `Weapons/ReloadTuning.h/.cpp`,
new `Weapons/WeaponBasePump.cpp` (rack between shots via `ApplyCyclePose` as the bolt's; the model's pump travel), the
hull timing in `WeaponBaseBrass.cpp`, `WeaponBase.h` (the only job that edits it), `Tests/WeaponTests.cpp` (ReloadMotion),
`Tests/RevolverTests.cpp`, new `Tests/GunfeelReloadTests.cpp`. Tests `Looter.Gunfeel.Reload.*` (plan steps empty vs
tactical, commit point, shells needed, interrupt, rack share), `Looter.Weapons.ReloadMotion`, `Looter.Weapons.Revolver.*`.
**GF-KICK (Sonnet)**: `Player/ViewKick.*`, `Player/PlayerViewKicks.cpp`, `Player/PlayerViewAim.cpp`, the kick lines of
`Player/PlayerLocomotionViewModel.cpp` (ADS model kick), `Weapons/WeaponRecoil.*`, `WeaponBaseKick.cpp`,
`GunfeelKickTuning.*`, `GunfeelKickRumble.cpp`, `Tests/GunfeelKickTests.cpp`, `Tests/FeedbackTests.cpp`. First-shot spread
through `GetEffectiveSpread`'s kind hook in WeaponBaseKick.cpp (a note for Main if it must touch WeaponBaseFiring).
Tests `Looter.Gunfeel.Kick.*` (pattern, first shot, sustain cap, ADS model), `Looter.Feedback.ViewKicks`, `Looter.Weapons.Recoil`.
**GF-SOUND-CODE (Sonnet)**: `Weapons/GunshotSound.*`, `GunfeelSoundTuning.*`, `Audio/LooterSoundCues.h`,
`Audio/CreatureVoiceComponent.*` (heavy layer by gun from DamageCauser, crit cue by `ECreatureBlood`, `UI.Kill.Crit`),
`Combat/BulletSubsystemImpacts.cpp` (impact loudness, `Impact.Heavy`, one `Impact.Scatter` per blast),
`Tests/GunfeelSoundTests.cpp`. LastRound inside `PlayLowAmmo` at 0 rounds; HeatHiss and the burst-end tail in
`PlayBurstEnd`. Tests `Looter.Gunfeel.Sound.*` (tail every N, low-ammo rounds by kind, last round, loudness rule).
**GF-IMPACT (Opus)**: `Weapons/WeaponFXImpacts.cpp` (494 lines: new code in new `WeaponFXImpactsKinds.cpp`),
`WeaponFXCreatures.cpp`, `VfxRules.h/.cpp` (not VfxRulesMuzzle.cpp), `VfxTuning.*`, `Weapons/BulletMarks.*`,
`UI/HUD/PlayerHUDWidget.*` + `PlayerHUDWidgetLayout.cpp` (marker colours/size, `UI.HitMarker.Crit`/`.Ward`,
`UI.WardBreak` calls), `Tests/VfxTests.cpp`, `Tests/HudTests.cpp`. Every quad on the engine Plane, no lights, no
particle assets, inside the Detail/stream budgets. Tests `Looter.Vfx.*`, `Looter.HUD.*` (marker rules).
**DEMO (Opus) play-check**, after the build; see below.
Main's own hooks (shared files): one line in `AWeaponBase::StopFire` (WeaponBaseFiring.cpp) -> `GunshotSound::PlayBurstEnd`;
anything a job reports it needs outside its files; CODEMAP lines; Gunplay.md "Gun feel pass" section; Handoff.

## Editor lane (one at a time)
0. Main: `Tools\perf.ps1 -Label "gunfeel before" -Exec "Looter.Quality Medium"` + the RR town horde
   (`Looter.Perf.Horde Spider 8 Basic 0 -1400 90`); read-only probe `Saved\GunfeelProbe.py` dumps DA_AssaultRifle and
   DA_Shotgun stats/recoil into this plan.
1. GF-SOUND-ART done -> `Tools/Unreal/build_sound_bank.py` (no PIE) -> commit (old code ignores the new cues).
2. GF-RELOAD + GF-KICK + GF-SOUND-CODE + GF-IMPACT trees copied (hash check), Main's hooks, one `Tools\launch.ps1 -Build`.
3. Tests, each job's filter once: `Looter.Gunfeel`, `Looter.Weapons`, `Looter.Feedback`, `Looter.Vfx`, `Looter.HUD`,
   `Looter.Audio`.
4. DEMO play-check; fixes; perf after (same two runs, target <= 8.3 ms, p95 no worse than +0.3 ms).
5. Commit per job (or one batch if built together), docs, push.

## Play-check (DEMO, RR, `Looter.GiveWeapon <Kind> Epic 5`, dummy + spiders + Unpaid; no capture batches)
- Sound: every kind in the open (Whitlock Fields), on Main Street, in the mine: tail changes; SMG/LMG 3 s bursts with
  no crackle or cut voices and one long tail at release; last-round sound, low-ammo click count per kind; LMG hiss when hot.
- Kick: revolver ADS 6 shots at a 15 m dummy, sights back on it before each pull; rifle 30 rounds held on a 20 m
  target; SMG hip at 10 m; first shot vs burst felt; Camera shake 0 stops all of it.
- Impacts: `Looter.Vfx.Impact 1 <Surface>` and real shots per kind on dirt, stone, wood, metal, water: sniper reads at
  40 m, SMG never hides the target; one dust sheet per shotgun blast.
- Reloads: tactical vs empty per kind (times from `Looter.Gunfeel.Reload.Debug 1`), swap after the seat keeps the
  rounds, shotgun interrupt fires after the shell, the pump racks visibly and audibly between shots, sounds on the hands.
- Hits: crit ping on every gun, ward tick and break crack on a Warded Unpaid, kill thump, pellets one marker; each blood's crit look.
- Report a verdict per kind ("still alike" list) under 150 words.

## Needs the user's choice
1. May reload/handling foley use the recorded pack? (Handoff says guns and impacts; reloads are gun sounds but today
   synthesized.) Recommended: yes.
2. More recordings: a new Sonniss GDC bundle part (same licence) for reload foley and wood/dirt/flesh impacts? The
   pack README says no new pack without the user's word. Recommended: only if 1 is yes and the four foley files fall short.
3. Shotgun shell-by-shell reload with interrupt, and the pump racked between shots (changes the Ranchhand's rhythm).
   Recommended: yes.
4. Empty reloads slower than tactical (x1.1 vs x0.9) and no fast-reload window. Recommended: yes, no window.
Later, not now: maker sound flavour (Roundhouse heavy, Saint Maud ringing, Windfall rattly, Meridian clean) as Mech
pitch/rattle per maker; enemy guns' distance versions (only Abel and Boot Hill bandits shoot today).

## The user's answers (2026-10-22 night)
1. Reload/handling foley from the recorded pack: **yes**.
2. One more part of the same free game-licensed bundle (reload handling, wood/dirt/flesh impacts): **yes** — Main names
   the file, source and size to the user and gets a yes before downloading (CLAUDE.md download rule); licence kept with it.
3. Shotgun shell by shell with interrupt + rack between shots: **yes**.
4. Empty reloads slower than tactical, no fast-reload window: **yes**.
