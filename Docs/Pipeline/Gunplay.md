# Pipeline: Gunfeel (kick, hit reactions, sounds), loot (guns, elements), VFX pass

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Gunfeel: kick and handling (built 2026-10-21)

Code: `Weapons/GunfeelKickTuning.h/.cpp` (cvars, accessors, plain rules; `GunfeelKick::Default::*` = shipped numbers),
`GunfeelKickRumble.cpp`, `WeaponBaseKick.cpp`. Tests: `Tests/GunfeelKickTests.cpp`, `Looter.Gunfeel.Kick.*`. All cvars
`Looter.Gunfeel.*`, read when asked (live in PIE), defaults in brackets.
- View kick: `ViewKicks::ForShot(...)` x `Kick.Rifle/.Shotgun/.Revolver` [1.6/1.5/1.4] in `AddShotKick` (the pure function and
  its tests are unchanged). The stack still holds 3 degrees and 5% FOV.
- Gun model kick (back, flip, twist, roll) x `Recoil.<Kind>` [1.4/1.5/1.4] in `AWeaponBase::GetRecoilProfile`; aim kick untouched;
  `Recoil.ShotgunSettle` [0.8] x its snappiness. Muzzle flash x `Muzzle.Scale1P` [1.4] when drawn first person.
- Rifle: aim kick grows `Recoil.RifleClimb` [0.06] per shot of a burst up to `Recoil.RifleClimbMax` [1.8]x (a pause over 0.35 s
  starts over; `FWeaponRecoil::GetClimb`). Hip bloom: `Spread.RifleBloom` [0.15] per shot (a quarter through the sight), cap
  `Spread.BloomMax` [2.0]x, closes at `Spread.BloomRecovery` [3.0] as an exponential rate per second. Only a player's rifle blooms;
  the crosshair follows `GetEffectiveSpread`.
- Shotgun falloff (`BulletSubsystem::ResolveHit` via `GunfeelKick::DamageShareAtDistance`): full to `Shotgun.FalloffStart` [0.6] x
  range, `FalloffFloor` [0.25] at `FalloffEnd` [1.6] x range. Every other gun keeps `FWeaponStats::DamageAtDistance` (full to range,
  0.5 at 2x range). A pellet whose gun is gone before it lands falls off the gentle way.
- Revolver crit `Crit.Revolver` [1.75]: `Shot.CritMultiplier = max(curse's, kind's)`; damage numbers need nothing.
- Handling: sights up in `BaseAimSeconds` (0.2 s) / Handling x `Aim.Time.<Kind>` [1.0/1.1/0.8]; aimed walk = 0.65 x
  `Aim.Move.<Kind>` [1.0/0.9/1.1] (`PlayerLocomotionComponent.cpp`); holster (`HolsterSeconds`, 0.14-0.32 s) and draw
  (`ReadySeconds`, 0.35 s / Handling) x `Swap.<Kind>` [1.0/1.1/0.8].
- Low ammo: after every shot `GunshotSound::PlayLowAmmo`; clicks for the last `LowAmmo.Share` [0.25] of the magazine (floor, at
  least the last round), pitch 1.0 at the first click up to `LowAmmo.PitchTop` [1.35] at the last round; magazines under 4 never;
  the player's own gun only; cue `Weapon.<Kind>.LowAmmo` (GUNFEEL-SOUND makes the sound).
- Rumble: `Rumble.Rifle` [0.35 for 0.06 s], `.Shotgun` [0.9 / 0.18 s], `.Revolver` [0.7 / 0.12 s] (`.RifleSeconds` etc.), hit taken
  `Rumble.Hurt` [0.6] at a quarter of health or more (a scratch a quarter of that, 0.1-0.3 s), all x `Rumble.Scale` [1.0] x the
  "Controller rumble" setting (Settings > Controls, default 100%, `ULooterControlsSave::ControllerRumble`, old saves read 100%;
  `Looter.ControllerRumble [0-1]`). Engine `PlayDynamicForceFeedback`, no assets. Only a local player whose last input was a
  gamepad's: a Slate input pre-processor (`FLastInputWatch`, started by `UControlSettingsSubsystem::Initialize`) notes it.
- Didn't check (needs a build and play): the feel numbers, rumble on a real pad, the extra settings row's layout, the first-person
  gun motion size, the shotgun's settle.

## Gunfeel: hit reactions (built 2026-10-21)

Code: `Combat/GunfeelReactTuning.h/.cpp` (`FGunfeelReactTuning`: cvars, `Live()`, field values = defaults), `Combat/HitReaction`
(`FHitReaction` rules, `FCreatureHit`), `Creatures/CreatureHitReactionComponent` (`TakeHit`), `CreatureKnockComponent` (Shove
style), each kind's `OnHurt`, `Weapons/WeaponFXCreatures.cpp`. Tests: `Tests/GunfeelReactTests.cpp`, `Looter.Gunfeel.React.*`.
Cvars `Looter.Gunfeel.*`, read at each hit (live in PIE), defaults in brackets.
- Order of a hit: `ACreatureBase::HandleDamaged` -> `HitReaction->TakeHit` (the component no longer binds `OnDamaged` itself) ->
  `LastHit` stored, `bFresh` -> the kind's `OnHurt`. A flinch a boss plays itself (the Gravemother's sink, stone jolts call
  `ASpiderCreature::OnHurt` straight) isn't fresh: strength 1, the spider's old 7/14 cm, 5/7 degree bases.
- Main's two fixes at the build: a boss flinches today's size x `Flinch.BossScale` (1) whatever the hit (share-sized, its
  huge health made it ~0.24x); the killing hit flinches at strength 1 (`FCreatureHit::bKilled`): at 2.5x it fought the
  spider's death rear (`Looter.Creatures.Anim.SpiderDeathCurl` reared 7 cm, needs 8+).
- Bug seen at the first build: an anonymous-namespace `Defaults` in a .cpp breaks the build (C4459 as an error: hides
  UnrealType.h's parameter of that name). Name file-private globals specifically (`ReactDefaults`).
- Gun from the hit: `DamageCauser` cast to `AWeaponBase` -> `GetInstance().Definition->Kind` (`UCreatureHitReactionComponent::KindOf`).
- Flinch: `Flinch.Scale` [1] x clamp(0.6 + `Flinch.DamageGain` [3] x burst share, 0.6, `Flinch.Max` [2.5]), crit x `CritScale`
  [1.4], boss x `BossScale` [0.4]. Burst share: a blast's last pellet carries the blast's (kinds keep the strongest kick). Bases
  raised so a rifle round reads at 20 m: spider kick 9/18 cm (was 7/14), tilt 7/10 degrees (5/7), caps 40 cm, 28 degrees, abdomen 22;
  slime lean 10 (8), squash dent scaled (no lower than 0.55), core jiggle up to 2.5x; Unpaid flinch kick capped 3.8, arm fling up
  to 2x, shroud up to 2x, a body hit stirs the coal 0.25 x strength. Miner, Bell-Ringer, Abel, Foreman: through the Unpaid's;
  Spitter, Gravemother: the spider's. Numbers: a 300-health spider, rifle 12 = 0.72, revolver 52 = 1.12, a third = 1.6.
- Slow: any non-boss hit, `Flinch.Slow` [0.55] for `SlowSeconds` [0.12] via `UMovementSlowComponent` (strongest wins, no stacking).
  The creature's brain sets `MaxWalkSpeed` in its own frame (actor ticks PostPhysics), so the slow component made on a creature is
  moved to `TG_PostPhysics` after its actor; otherwise the brain's speed won every frame.
- Stagger: heavy = crit or burst >= `Stagger.HeavyShare` [0.12]; Basic `Stagger.Seconds` [0.4], ranks in the old proportions;
  `Stagger.Cooldown` [0.7]; shove `Stagger.Shove` [420]. Hit-stops: `HitStop.Crit` [0.05], `.Heavy` [0.035] (shares the crit's
  0.25 s cooldown), `.Kill` [0.075], `.CritKill` [0.1]; melee unchanged.
- Rifle pressure: `Rifle.PressureHits` [5] within `PressureWindow` [1 s] = a stagger, same cooldown as any stagger.
- Shotgun power: once a blast, burst >= `Shotgun.KnockShare` [0.35], shooter within `KnockRange` [600 cm]: size <= `KnockSize`
  [1.2] knocked back (`UCreatureKnockComponent::Knock`, Shove style: rocked back 24 degrees, no stars; launch `KnockSpeed` [520]
  across, `KnockHeight` [40] up = about 3 m, then `KnockHold` [0.25 s]), else staggered. Skips the stagger cooldown (each takes a
  third of its health); a stagger the same blast began gives way to it. Never Legendary or boss.
- Revolver: crit hit-stop `Revolver.CritHitStop` [0.09]; `GunshotSound::PlayCritRing` for a local player's revolver crit (bosses too).
- Kills: corpse knock by gun (`CorpseKnockFor`): shotgun `Kill.ShotgunKnock` [650] lifted `ShotgunLift` [35 cm] in a sine arc
  over the 0.35 s slide (ground rules go by the unlifted spot; set down when it stops), rifle `Kill.RifleKnock` [180], else 260;
  a crit 380 where larger. Revolver crit kill: death burst x `Kill.RevolverBurst` [1.6] at the crit spot (within 2.5 m of the body)
  plus a crit spurt there.
- Spurts: x `Spurt.Scale` [1.5]; exit spray `Spurt.Exit` [0.6] of the entry's thrown along the shot from 8 cm in; pellets (mark
  scale under 0.8) want half a hit each and share `Spurt.BlastBudget` [2] full hits per blast (80 ms, 6 m); past it, nothing but
  a crit's pop. The spiders' ichor now spawns here (was `SpawnImpact`'s Flesh, which ignored pellet size).
- Didn't check (needs a build and play): every feel number, the shove knock's look, the slow on a creature's steering, the corpse
  lift on slopes, the exit spray's look through thin parts (legs), stagger frequency in packs (no stun-lock seen on paper).

## Gunfeel: gun sounds (GUNFEEL-SOUND, written 2026-10-21, unbuilt and untested in play)

Code: `Weapons/GunshotSound.cpp` (`PlayShot`), `Weapons/GunfeelSoundTuning.h/.cpp` (cvars, `Default`, plain rules), cue names
`LooterSoundCue::Gunshot::*Core`, `Revolver::CritRing`. Tests: `Tests/GunfeelSoundTests.cpp`, `Looter.Gunfeel.Sound.Variables`,
`.Surroundings`, `.Layers`. Sounds: `Art/Sounds/recipes/guns_shots.py` (+ `guns_rec.py`), `guns_impacts.py`, `lib/sample.py`, `packs/`
(Sounds section above; `Art/Sounds/README.md` "Guns, round 3").
- Layers per kind, cue `Weapon.<Kind>.<Layer>` (Rifle 6 takes, Shotgun 5, Revolver 6; layers 4, Distant 6/5/5): Core, Mech, Low,
  TailOpen, TailTown, TailInside, Distant, LowAmmo; Fire = Core + Mech + Low + TailOpen in one sound (Abel's pump, old banks).
  `Weapon.Revolver.CritRing` 2D (a recorded steel plate's ring taken up half an octave under a struck ping).
- `PlayShot`: definition's `FireSound` wins; bank without `Weapon.<Kind>.Core` -> Fire alone (as before). Own gun (locally and
  player controlled) -> Core + Mech x `Sound.Mech` [1] + Low x `Sound.Low` [1] + tail x `Sound.Tail` [1], all on the gun.
  Others -> Core (on the gun) + tail at the shot; past `Sound.DistantFrom` [3000 cm] from the first local player's ears ->
  Distant alone at the shot.
- Surroundings: per gun, every 0.25 s at most (`GunfeelSound::ProbeSeconds`): world-static line traces (`StaticGeometryParams`,
  holder ignored, simple collision) of `Sound.ProbeRange` [800 cm] from the gun: straight up, then 70 degrees up ahead if that
  missed (roof); level at the holder's yaw +0/90/180/270 (walls). Roof and walls on 2+ sides -> TailInside; walls on 2+ or a roof
  alone -> TailTown (a porch, a ledge: a warehouse tail there would be wrong; the contract said roof = Inside); else TailOpen.
- Levels (cues.json volume_db after the render; Fire level 0 / +1 / +0.5): each layer's `level` = its loudness inside Fire's mix,
  so the own-gun sum = Fire within 0.5 dB (checked with the rendered files at their volumes). Own-gun sum peaks: rifle -1.8 dBFS,
  shotgun -1.1, revolver -0.1 worst case (Inside tail). Town tail 0.5-1.5 dB over Open, Inside 4-5 dB over; Distant 2-3 dB
  under Fire. Rifle Fire's L200 -18.1 (was -17.9): no other cue's volume moved.
- Impacts: recorded layers under the synthesized ones (their own random stream, `rng(cue, v, 'rec')`, so the rest of each take
  is unchanged): Impact.Stone (concrete, -1 dB), Impact.World stone takes (-3), Impact.Metal (heavy steel, -2), Creature.Hit
  (body thump + punch, -1), Creature.Spider.Hit (body, -7 under the shell), Creature.Slime.Hit (wet punch, -5),
  Weapon.NearMiss.Whiz (a recorded pass + a supersonic crack, -3). Impact.Ricochet, the soft surfaces, Unpaid's hit unchanged
  (nothing in the packs suits them).
- Check: `Tools\sounds.ps1 Weapon.*,Impact.*,Creature.Hit,Creature.Spider.Hit,Creature.Slime.Hit`: 0 flags. Presence (2-5 kHz)
  shares: rifle Core 15-17%, shotgun Core 6-8%, revolver Core 6-8%, Revolver.TailOpen up to 18% (spike 7.9, under the 8 limit).
  The new layer cues aren't in check.py's FREQUENT list yet (`Weapon.*.Core`, `.Mech`, `.Tail*` should be).
- Didn't check (needs the bank built and play): how it sounds; the probe's choices in town, the mine and the barns; the far
  shot's distance; concurrency in a long rifle burst (Core 8, tails 6).

## Loot: the four new guns (GUNS-CODE, built 2026-10-22)

Kinds `EWeaponKind::SMG, Sniper, LMG, Launcher` (appended; saved by name), ammo `EAmmoType::Rocket` (appended: max 12, box 2),
reload parts `Bolt, Box, Rocket, Helix`. Definitions `DA_SMG` (Rustler), `DA_Sniper` (Longhorn), `DA_LMG` (Thresher),
`DA_Launcher` (Powderkeg) from `Tools/Unreal/create_new_guns.py` (stats, recoil, reload part, `ProjectileMesh` =
`SM_PowderkegRocket`, default loot table weights 1.0/0.9/0.8/0.6, rockets in its ammo at 0.5). Order after a C++ build and
the parts' import: `build_gun_gauge_material.py` -> `create_new_guns.py` -> `setup_gun_parts.py` -> `create_rank_assets.py`.
Code: `Weapons/GunKindsTuning` (cvars + rules), `WeaponBaseKinds.cpp`, `WeaponModelKinds.cpp`, `WeaponRocket`,
`WeaponRocketBlast(+Effects)`; kind switches everywhere (hip holds, swaps, view kicks, rumble, hands, marks, brass, smoke,
nicknames, icons, kill reactions). Tests `Looter.Weapons.NewGuns.*`. Defaults in brackets, all `Looter.Gunfeel.*`, live.
- Base stats (parts move them): SMG 11 dmg 900 rpm 1.4 deg mag 40-60 reload 2.1; Sniper 150 dmg 45 rpm 4.0 deg hip mag 5-8
  reload 3.0; LMG 16 dmg 650 rpm 1.6 deg box 80-150 reload 4.2; Launcher 320 burst 40 rpm mag 1-3 reload 2.8. Rarity never
  changes their capacity. Ranges and handling from setup_gun_parts (22/90/35/40 m; 1.25/0.85/0.8/0.8).
- Kick by kind: `Kick.SMG/.Sniper/.LMG/.Launcher` [1.0/1.6/1.3/1.8], `Recoil.*` [1.0/1.5/1.3/1.6], rumble [0.25 0.04 s / 0.85
  0.16 / 0.45 0.07 / 1.0 0.3], `Aim.Time.*` [0.75/1.35/1.4/1.2], `Aim.Move.*` [1.15/0.85/0.8/0.85], `Swap.*` [0.7/1.2/1.35/1.3].
  SMG and LMG climb (`Recoil.SMGClimb` 0.03, `LMGClimb` 0.04) and bloom (`Spread.SMGBloom` 0.09, `LMGBloom` 0.12).
- SMG: the helix's GunAmmoGlow lit by the rounds left (`SMG.Glow` 4, pulsing under `SMG.GlowLowShare` 0.25 at
  `SMG.GlowPulse` 5 Hz). The slot gets a per-gun instance of `M_GunGauge` (Fill along UV0.x, Glow, the art's Color);
  without the master it only dims.
- Sniper: `Crit.Sniper` [2.0] (Main's unattended call, as the revolver's role allowed). Bolt cycles after every shot:
  waits `Sniper.BoltDelay` [0.12 s], takes `BoltShare` [0.7] of the rest of the interval (at most `BoltMaxSeconds` 0.85), cue
  `Weapon.Sniper.BoltCycle`, case out when fully back, the trigger hand rides the knob (`FHandsGun::BoltHand/BoltOpen`),
  the 1P gun cants (`ApplyCyclePose` on the model). Spread: its hip spread whole, x`Sniper.AimedSpread` [0.04] scoped (no
  halving mercy). Scope sway moves the real aim (control rotation) in a figure eight (`Sniper.Sway` 0.3 deg, `SwayRate`
  0.35 Hz); sprint key held while aimed holds the breath (`BreathSeconds` 3, sway x`BreathSteady` 0.1), then winded
  (`WindedSeconds` 2.5, sway x`WindedSway` 1.8), refill x`BreathRefill` 1.5. Crit kill burst x`Kill.SniperBurst` 1.3 of the
  revolver's; crit ring and crit hit-stop as the revolver's. Scope picture: aim past 0.75 fades in `UHudScopeWidget`
  (full at 0.95; local player, first person, zoom > 1.5; lens radius 0.46 of screen height) at viewport Z -1, under the
  HUD. From 0.5 strength the gun hides itself (`bScopeHidden`); `UpdateScopeSway` must not bail on IsHidden then, or
  the picture flickers on and off each frame.
- LMG: rate from `LMG.SpinStart` [0.6] to full over `LMG.SpinUp` [0.35 s] of trigger (the SpinUp cue is cut to 0.35 s: keep
  them together), down over `SpinDown` [0.5]; spin-up heard below 0.35 spin. Heat +`HeatPerShot` [0.012] a shot, held
  `HeatDelay` [0.5 s], cools `HeatCool` [0.3/s]; GunHeatGlow at heat x `HeatGlow` [6]. Rifle pressure counts LMG hits.
- Launcher: `AWeaponRocket` from the eye along the aim (seen from the muzzle, closing on its path at 12/s), `RocketSpeed`
  [4000], `RocketGravity` [120], `RocketLife` [5 s], sweeps the Weapon channel (r 7), flame + smoke puffs via FWeaponFX,
  `Weapon.Launcher.Flight` loop. Burst (`WeaponRocketBlast::Detonate`): reach `BlastRadius` [350] to a body's nearest side,
  full within `BlastCore` [80], `BlastEdgeShare` [0.2] at the edge, never through static geometry (GraveSaltBurst::CanSee),
  what it struck takes it whole, shooter x`SelfScale` [0.5] and at most `SelfCap` [0.35] of max health (a point-blank rocket killed a full
  100-hp player before the cap; no other player), living within `KnockShare` [0.6] thrown
  (never bosses), physics shoved (HitImpulse 90000 radial), `GunshotSound::PlayExplosion`, view jolt. Kill knock
  `Kill.LauncherKnock` [900] + lift `Kill.LauncherLift` [140]. Back-blast from SOCKET_Backblast. The loaded rocket's nose
  (magazine part on SOCKET_Rocket) shows while loaded; the reload brings `SM_PowderkegRocket` up in front of the mouth and
  slides it in. Rocket ammo: kill pickups 1-3 (`LooterLoot::RocketsFor`: <=24 rounds 1, <=30 2, else 3), chests/boss boxes 3.
- Hooks for LOOT-CODE (marked `LOOT-CODE hook` in the code): part-effect PlanFor in `AWeaponBase::HandleFiring`, drop
  moments in `RefreshLootBeam`, element TryApply per body in `WeaponRocketBlast::Detonate` (rockets skip OnBulletLanding).
- Icons: `Art/Icons/InkedIcons.py` rustler/longhorn/thresher/powderkeg + AmmoRocket (inside the rounds' box; no old icon
  moved); run with Blender's python. Rocket pickup `SM_AmmoRocket` from `Art/Models/Loot/Ammo.py` (three finned rockets).
- Seen in PIE 2026-10-22: Rustler and Thresher aimed through the Holo sight see its solid back (art: open the
  frontier_sight Holo window); the Rustler's helix sits under the gun, unseen in first person (its glow shows in third
  person only). GUNS-CODE ran only the new guns' part setup: LOOT-CODE reruns the full `setup_gun_parts.py` after its C++.
- Packaged builds: `M_GunGauge` is read by path: keep /Game/Art/Materials/Masters cooked.

## Loot: elements, part effects, drop moments (LOOT-CODE, built 2026-10-22)

The user's calls (2026-10-21 night): story elements Ember (burn), Soul-light (vs undead), Grave-salt (slow, vs spirits),
each status dealing damage and stronger on the monsters weakest to it; part effects that change behaviour; bigger drop
moments. Code: CODEMAP "Elements", `Weapons/PartEffect*`, `Loot/LootDropMoments`, `LootFanfareShows.cpp`.
- **Parts**: the parts CSVs gained `Effect`, `Element`, `Model` (Model = an existing key of the same slot whose mesh the row
  wears until it has its own; `setup_gun_parts.py` reads all three, empty = none). Element parts are Uncommon, weight 0.35,
  word Smoldering / Saintlit / Salted, NamePriority 38, damage -6..-2%: magazines on Bullpup (Ember30/Saint30/Salt30), Ranchhand
  (EmberTube6/SaintTube6/SaltTube6), Rustler (Ember40...), Thresher (Ember100...), Longhorn (Ember5...); cylinders on the Drover
  (Ember6/Saint6/Salt6); Powderkeg rockets (Incendiary = Ember, Saintfire, Saltburst). Effect parts Epic weight 0.6 (a few Rare
  0.5), NamePriority 39 (an effect's word names the gun over an element's): Caroming (Ricochet), BoreThrough/ThroughAndThrough
  (Piercing), Triplet (Burst), Twinport/TwinBore6 (Twin), Blasting* (Explosive), Thunder* (Overcharge). A gun's element and
  effect: its first part in slot order with one (`WeaponParts::ElementOf/EffectOf`). Never rename a key.
- **Small fixes (2026-10-22)**: the SMG helix's GunAmmoGlow takes the fitted element part's `ElementRules::FxColor` (Ember orange, Soul-light gold-white, Grave-salt cyan; `MakeGauge(Slot, Source, Element)`, ammo gauge only; plain helixes keep the art's orange; test `HelixColor`); seen in PIE, Saintlit Rustler pale. The Ranchhand's element tubes sit under the pump, unseen in first person, so each also dresses a plate on the receiver's left flank (`flank_dress` in Ranchhand.py: Ember scorched with glowing slits, Saint gilt with a medal and gold-white bands, Salt steel with crystals; u 5.8-14, v -2.2..1.3, in the Magazine part; the right flank holds the ejection port). Seen in PIE (Saintlit Ranchhand).
- **Weakness table** (`ElementRules`, by class: spiders incl. Spitter and Gravemother = chitin; slimes = gel; AUnpaidCreature =
  Unpaid; dead miners, Foreman, Bell-Ringer, Abel = the rest of the dead): Ember weak chitin+gel, resisted by the Unpaid;
  Soul-light weak Unpaid+dead, resisted by slimes; Grave-salt weak spirits (Warded or Phasing trait, Ethereal tier, an
  Unpaid's coal = a crit on it), resisted by chitin. A weakness wins. Weak x1.75, Resist x0.5 to the status's damage and
  chance. The bestiary page shows `WeaknessLine` (accent, under the tier line).
- **Chance a hit sets it** (per bullet, `Looter.Element.Chance.*`): rifle 0.20, shotgun pellet 0.07, revolver 0.28, SMG 0.15,
  sniper 0.35, LMG 0.17, rocket 0.5 a body; +0.02 a rarity step past Uncommon; at most 0.9.
- **Statuses** (`FElementStatus`, pieces every 0.5 s, the last as it ends, so the share is exact): Ember 40% of the hit over 3 s,
  up to 3 burns (a 4th replaces the weakest if stronger); Soul-light 60% of the hit 1 s later, up to 4 waiting (more gathers into
  the last), leaps for half to the nearest of the dead within 4 m; Grave-salt 25% over 4 s and speed x0.7 (UMovementSlowComponent),
  one salting (the stronger and longer kept); a bullet on a salted creature's standing ward x1.5. Bosses: half the time (half a
  smite), never slowed. Damage types `UEmberElementDamageType`, `USoulLightElementDamageType`, salt deals the grenade's
  `UGraveSaltDamageType`; the causer is the gun that set it (its notch). A piece never flashes the body white
  (`IsDealingNow`, CreatureReactComponent): the status's look shows instead.
- **Looks** (FWeaponFX, no lights): every element hit spits its colour (embers and a flame lick / white-gold motes and rays /
  salt crystals and frost), fuller when the status is set, fuller and brighter on a weak creature; tracers lean 55% to the
  element's colour. Burning: embers and licks (24/s x sqrt(burns)) and a flickering fire-glow; smiting: motes drawn in, a
  pulsing heart-glow, then a burst; salted: crystals dropping and a pale sheen. Body glows sit on the body's near side
  (at the heart the body hid them, 2026-10-22).
- **Effects** (`UPartEffectSubsystem` on the bullet hooks; `Looter.PartEffect.*`): Ricochet: any hit bounces once to the
  nearest creature in sight within 8 m (3 looks), 60% damage, gen 1 makes no more. Piercing: on through 1 creature, 85% after.
  Explosive: 60% to what it hit, 40% splashed r 150 (edge 0.3 of the middle; what it hit too, never the shooter, never through
  walls), a pop (`Weapon.Launcher.Explosion` at 0.3 volume, pitch 1.6, 80 ms apart). Twin: two bullets 0.8° apart, 65% each,
  one round. Burst: 3 rounds 0.6x the interval apart, 1.8x between pulls. Overcharge: semi-auto, a pull winds 0.6 s and fires
  x2 (let go early: what it reached), cycling at half rate; holster or reload drops the charge. Trigger side:
  `WeaponTrigger::PlanFor` in AWeaponBase (StartFireAt, StopFire, HandleFiring).
- **Drop moments** (`LootDropMoments`, `Looter.Loot.Drop.*`): beams 450 / 900 / 1400 / 2200 cm (was 350 / 650 / 1000 / 1600),
  glow 2/2/3/3, radius 6/6/9/11; Rare+ stinger at landing (Epic+ flat); Epic+ spin 900°/s easing over 0.9 s and a ground ring
  (Epic 260, Legendary 420 cm, 0.7 s); Legendary: `Loot.Drop.Legendary` the moment it drops, the world at 0.3 for 0.25 s (only
  from full speed: never in Sundown or a boss's death slow), and on landing `Loot.Drop.Legendary.Land`, a 45 m column (r 30,
  glow 3, stays while it lies there), its name popping up 150 cm over it for 4.5 s (the label's `SetPop`), 24 motes.
- **Dev**: `Looter.Element.Apply <Ember|SoulLight|GraveSalt|off> [radius m] [every s] [hit damage]`; `Looter.GiveWeapon
  Revolver Epic 5 Cylinder=Salt6` picks parts; `Looter.Feedback.Drop <Rarity>`.
- **Checked in PIE (2026-10-22)**: Ember on spiders (weak, burns read, ticks 9), Grave-salt on a Warded Unpaid (weak), Soul-light
  on the Unpaid (weak, smite killed one), Ember on a slime (weak), Soul-light on slimes never came up in ~8 hits (resist: 0.15);
  Ricochet bounced off a wall into a spider; Explosive splash hurt a second spider; Twin SMG; Rare/Epic/Legendary drops.
  Perf RR town, 8 spiders all burning (re-applied every 2 s): 7.6 ms, p95 8.2.
- **Element feel (2026-10-22)**: damage numbers (`ElementCallout`): a burn/smite/salt piece is the element's colour; a crit of an
  element gun settles 65% toward it (still white-hot first); a plain gun hit stays white. The first weak hit of a fight on a
  creature pops a WEAK tag (element colour, 1.9x -> 1x over 0.14 s) beside a number 1.2x bigger; a resist pops a greyed RESIST
  (size unchanged). Once per creature + element + side; a fight ends after 20 s with no element hit. The gun's element reaches
  the number through `FHitElementScope` (BulletSubsystem::ResolveHit, WeaponRocketBlast); a status's through
  `UElementStatusComponent::DealingElement()` (set in SendDamage). A status kill: `FCreatureHit::Element` set, Kind None (a burn
  is no shotgun blast: no stagger/knock either), no corpse knock, no kind death burst; `CreatureKillLook::Play` lays
  `ElementDeathLook` (Ember: embers and a fountain from the foot; Soul-light: burst of light; Grave-salt: crystals shed and
  crumbling) and the `Element.<E>.Kill` cue; bosses/non-creatures keep the status component's own burst.
  Tests: `Looter.UI.DamageNumbers.Element`, `Looter.Elements.Callout/.StatusKill/.DeathLook`.
- **Not done**: element parts wear other parts' models (the gun's glow isn't tinted by element).
  Tests: `Looter.Elements.*`, `Looter.PartEffects.*`, `Looter.Loot.DropMoments.*`.

## VFX pass (12.5, built 2026-10-22)

The user after playing: hits, deaths, muzzles and powers looked weak/cheap; asked for Borderlands-like punch inside 8.3 ms
Medium. Plan and per-item notes: `Docs/Archive/Work` once archived (was `Saved/Vfx/PLAN.md`). All code-drawn on FWeaponFX, no
particle assets, no lights, every new quad on the engine Plane (never Nanite). Cvars `Looter.Vfx.*` (`FVfxTuning`, `Live()`;
field values = defaults): `Hit.Scale`, `Hit.Pop`, `Impact.Scale`, `Kill.Scale`, `Kill.Ring`, `Muzzle.Scale`, `Power.Scale`,
`Blast.Scale`, `Glow`, `Smoke`, `Detail.Near` [1500 cm] / `.Far` [6000] / `.Min` [0.35], `Stream.Window` [0.3 s] / `.Reach`
[150 cm], `MaxParticles` [1000], `Shapes`, `Puffs`, `Splats`, `Splat.Max` [12] / `.Hold` [9 s] / `.Fade` [3 s]. Tests
`Looter.Vfx.*` (Rules.Detail/.Stream/.Kills/.Tiers/.Muzzles/.Shapes/.Splats, Effects).
- **Masters** (`Tools/Unreal/build_vfx_materials.py`, /Game/Art/Materials/Masters; FWeaponFX uses them once they exist, stand-ins
  before): `M_FX_Shape` additive, ISM custom data 0-2 colour, 3 strength, 4 shape (`EVfxShape`: 0 ring, 1 fireball, 2 starburst),
  5 param (ring thickness share / fireball heat / spikes), 6 seed; `M_FX_Puff` translucent noise smoke, 0-2 colour, 3 opacity, 4
  seed (<0 plain disc: the water rings' dashes), 5 erosion (age); `M_FX_SplatDecal` DBuffer decal, DecalColor = colour + opacity,
  MID `Spread`/`Seed`/`Wet`. Its first run logs "DeferredDecal ... only Translucent" from the domain being set before the
  blend mode; a second run is clean (harmless).
- **Detail**: pieces x1 within 15 m to x0.35 at 60 m (`FWeaponFX::DetailAt`, the camera of the last Tick); a stream into one spot
  thins 1/sqrt(n-1) to x0.25 (`FVfxHitStream`); a hit's pop is never thinned; pellets keep the Gunfeel blast budget.
- **Hits** (`WeaponFXCreatures.cpp`): a splat puff of the spill, a starburst pop (pale for wet, hot for coal), 2-3 gobs, drops
  drawn stretched by speed (spray); heavy rounds (mark scale >= 1.2) a ring and exit x1.6; crits a gold star and ring.
- **Impacts** (`WeaponFXImpacts.cpp`): soft ground a geyser (puffs up the normal, each slower) + skirt + big clods; hard surfaces a
  starburst pop (metal a ping ring and long-bouncing sparks); geyser colours darker than the hanging dust.
- **Kills**: base bursts (`SpawnDeathBurst`, shared by powers, globs, Grave-burst) gained a wet pop/splat and big plates, a gel
  crown, a soul-light starburst, two fireballs and a facing ring. Kill layers (`CreatureKillLook::Play` -> `SpawnKillLayers`:
  Spider OnDied, Slime OnDied, Unpaid's first dead tick) by `KillWeight` (gun, crit, overkill) and `KillCarry` (shotgun 0.85 ..
  rifle 0.35): gore landing on the ground, mist, dust skirt; the Unpaid's coal ring over the ground, ash, embers; tiers by
  `TierLayers` (Restless 1 ring, Gravebound 2 + star + motes, the top two a star under their own show). Splats: spider ichor on
  the body's landing (`LaySplat`), gel and ash at once. Bosses nothing.
- **Muzzles** (`WeaponBaseMuzzle.cpp`, moved out of WeaponBaseFiring): quads per kind from `VfxRules::MuzzleLook` (core, star,
  tongues spread 180/n, sniper brake jets, revolver gap spit; `FlashDuration` per kind) + `SpawnMuzzleBurst` (sparks, smoke 45 cm
  out in first person, the sniper's and launcher's blast ring third person only; fast guns every 2nd/3rd shot,
  `MuzzleShotsInBurst`, 0.35 s gap). Tracers per kind in `FireShot` (`VfxRules::TracerLook`: SMG thin, sniper wide and bright,
  LMG every third round full, the rest half glow by rounds left).
- **Rocket**: `WeaponRocketBlast::SpawnEffects` = `SpawnExplosion` (crack, fireballs, ground ring, air shock, soot, embers, the
  ground's own impact, debris) + a scorch splat; the motor drops a small fireball each smoke puff.
- **Powers** (`EmberVfx`): `Hurt` shows every blow (`BodyHit`: the creature's spill sized by damage/20 + a star of the power's
  colour); activation ground rings (Dust Devil, Sundown, Borrowed Time, Lucky Streak, Double or Nothing, Grave Ward, Iron Hide),
  strikes (Toll Chain, Tripwire, Lucky's ricochets, Fifty-Two's 3+ card hits, Long Shot 1.3x+, Spyglass pings, Rail Spike embed),
  blasts (Slag Bomb fire, Last Rites soul), Last Rites' strike a falling column.
- **Lessons**: additive fire over a daylight scene washes to a white cloud past a strength of ~2-3 (the tonemapper desaturates):
  fireballs spawn at 1-3 and dim with their heat, the shader only whitens above heat 0.75; a true orange fireball in sunlight
  would need a translucent fire material. Unlit puffs read paler than their colour suggests (a geyser's soil needs ~0.2 linear).
  Grabbing effects: `slomo 0.04-0.1` before the shot, grab ~0.5-1 s after the console call (each call takes ~1.5 s real);
  console overhead makes early-life grabs show effects smaller than they peak. `Looter.Tier.Hit <dmg>` sets up a kill;
  `Looter.Hero Gauge` for Gauge's powers in a no-session play; 3 slots: restart PIE for more guns.
- **Perf** (2026-10-22, quiet PC, ArtPause on, Medium 1080p, RR town horde `Looter.Perf.Horde Spider 8 Basic 0 -1400 90`): 7.5 ms
  (p95 8.0, GPU 6.2); with `Looter.Hero Gauge` + `Looter.Embers.Repeat SlagBomb 4`: 7.9 ms (p95 8.6, GPU 6.7). Knobs if a scene
  goes over: `Looter.Vfx.Smoke 0.7`, `Detail.Near 1000`, `Blast.Scale 0.8`, `Puffs 0`, `MaxParticles 700`.
- Not checked by eye: tier kill rings, slimes' gel splat, Last Rites column, the stand-ins without the masters.

# VFX-2 (2026-10-22): notes for Docs/Pipeline "VFX pass" (merge them there; the docs split was running)

Uncommitted. Before/after sheet: `Saved/Vfx2/compare.png`. Tests passed: Looter.Vfx (8), Looter.Weapons.FX (6),
Looter.Embers.Power (15), Looter.Creatures.React (5).

## Fire that reads in sun
- New master `M_FX_Fire` (`Tools/Unreal/build_vfx_materials.py fire`; args name masters to build, none = all four): unlit,
  translucent (alpha-blended), two-sided, ISM. Custom data (FireFloats 10): 0-2 fire colour, 3-5 smoke colour (light in it),
  6 opacity, 7 heat (1 white-hot .. 0 smoke), 8 seed, 9 erosion. One noise (two octaves; cheap on purpose: blasts cost overdraw).
  Heat ramp: soot -> deep red -> fire x1.9 -> white-yellow x4; rim burns dark first. Scalars Glow, SoftDepth (40).
- Why: additive fire (M_FX_Shape fireballs) washes to a pale disc in daylight; alpha-blended emissive keeps its hue.
- `FWeaponFX::SpawnFlame(FVfxFlame)` (`WeaponFXFire.cpp`, type in `VfxTypes.h`): a puff burning for `Burn` of its life, then
  soot; `Delay` (spawned with negative age: not moved or drawn until 0). Drawn on the "Fire" ISM, sorted far to near
  (`CollectFlames`), TranslucentSortPriority 1 (over the dust). Without the material: heat-tinted puffs on the smoke.
  Fades: camera inside (Near), and size/distance 0.8 -> 1.6 (`FlameCoverFrom/To`): a puff over half the view is overdraw.
- `SpawnExplosion` moved to `WeaponFXFire.cpp`. Fire blast: small white crack (9), core flame (0.65 -> 1.0 x reach, 0.75 s,
  burns 40%), `Fireballs` fire puffs rolling out/up (burn 24-34% of 1.5-2.1 s, then soot rising), soot column
  (SmokeColumn x 3/5, delay 0.15-0.55 s, 2.4-3.2 s, slow rise), faint ground ring (3.5, was 14), faint air shock (2.5),
  dust skirt (SmokeRing x 2/3, ground colour 0.27/0.22/0.165 x light), embers, debris, the ground's impact at 0.7 (pellet's
  half). Soot 0.075 x DustLight. Fire sized by min(reach, 380) (`FlameReachMost`) and smaller when the eye is near
  (`BlastCloseShare` 2.5 reaches -> down to 0.4). Soul blasts (Last Rites) keep the old additive look.
- Slag Bomb: additive flash 2.4 -> 1.1 (washed the fire white); no SpawnDirt on top; pool haze 9 -> 5/s and smaller
  (55-85 -> 115-160 cm), pool smoke 4 -> 2.5/s.

## The white ghost
- Cause: the hit flash (M_FX_HitFlash, additive overlay, Glow 1.7) started by the killing shot was held at full by the kill's
  hit-stop (CustomTimeDilation 0.02 for 75-100 ms) and faded over 90 ms after: the whole body white for ~10 frames, longer in
  slomo. Fix: `UCreatureReactComponent::HandleStateChanged(Dead)` -> `FCreatureHitFlash::Stop()` + overlay off. Kills show the
  kind's own death only.
- Slag Bomb's horde looked like ghosts too: every burn tick flashed white, then its orange tint (0.35 + 0.9 flare, additive)
  held. Now `UCreatureReactComponent::FQuietHits` (scope) around burn ticks: no white flash and no `EmberVfx::BodyHit` spill;
  tint 0 + 0.22 flare (a warm pulse). `Looter.Vfx.BurnTint 0` drops the tint (measured: costs ~0.03 ms GPU).
- Test added to Looter.Creatures.React.HitFlash (quiet tick, kill clears the overlay).

## Dirt and stone hits that read at 10-30 m
- `VfxRules::ImpactViewScale(distance)`: puff and chunk sizes x1 within 10 m -> x1.8 at 30 m+ (pixel size kept; counts are
  still DetailAt's). `FWeaponFX::ImpactShade`: dust colours x the light at the hit (FDustLight / 2: 1 in sun, bluish ~0.35 in
  shade), one sun trace per 0.25 s / 4 m.
- Dense geysers (`Geyser(..., bDense)`): billows of M_FX_Fire with no heat instead of soft puffs (the soft round puffs read as
  pale blobs on a sunlit road). Dirt: 5 soil billows (0.17/0.105/0.055, 30 -> 130 cm, thrown 520 cm/s), 9 clods 3.5-6.5 cm,
  12 soil crumbs (round, drawn as spray), 3 big clods 7-11 cm, bigger dust. Grass: 3 billows. Stone: pale jet billows
  (0.7/0.67/0.62) with a dark grit core (dense), chips 3.5-6.5 cm, pale flakes, 3 big chips 7-10 cm. All puffs ~1.6x bigger.

## Cost (RR town, Medium 1080p, `Looter.Perf.Horde Spider 8 Basic 0 -1400 90`, + `Looter.Hero Gauge,Looter.Embers.Repeat SlagBomb 4`)
- Before (current HEAD + nothing): horde 7.6 ms (p95 8.0-8.1); Slag 8.0-8.3 (p95 8.9-9.2). GPU translucency horde 0.19,
  Slag avg 0.72 / p95 1.58 / max 2.31; GPU p95 7.58.
- After: Slag translucency avg 0.43 / p95 0.65 / max 1.28; GPU p95 6.98 (horde 6.66-6.76). Rockets every 2 s (`Looter.Vfx.Blast
  2`): translucency avg 0.34 / p95 0.58, GPU p95 7.03. Clean frame runs mid-pass: Slag 7.8-8.0 (p95 8.5-8.7). The final frame
  times are void: an art agent's Blender terrain job (`RansomsRest.py --macro`, started 06:44 while ArtPause was off between
  builds) held a core, game thread +2 ms in every run (horde alone 8.3-8.6, p95 10.5+). Re-measure the frame with nothing
  else running; the remaining Slag p95 is game thread (first burst: hurts, numbers, kills) and render-thread visibility wait.
- Also cut for every fight: FX instances kept in blocks of 32 (`FWeaponFX::DrawInstances`, `WeaponFXInstances.cpp`): a
  particle born or gone no longer rebuilds the ISM proxy every frame (render thread 8.0 -> 7.7 in the Slag run); FX ISMs off
  distance fields, dynamic indirect, ray tracing. Kill crowd thinning (`KillCrowdDetail`: kills within 0.6 s / 8 m thin as a
  stream: 1, 1, 0.71, 0.58, 0.5...). Smoke puffs with no opacity are not drawn.
- Knobs: `Looter.Vfx.Fire 0` (heat-tinted puffs), `Looter.Vfx.BurnTint 0`, `Looter.Vfx.Blast.Scale`, `Looter.Vfx.Smoke`.

## Commands (`Weapons/VfxCommands.cpp`, not shipping)
- `Looter.Vfx.Blast [seconds | off]`: a rocket's blast where the view's aim meets the world; repeats for perf captures.
- `Looter.Vfx.Impact [scale] [surface]`: a round's hit at the aim in that surface's look, or the named one's (`1 Stone`).

## Lessons
- Never run a material script while PIE runs: the asset can't be saved ("Editor is in a play mode") and an "Overwrite Existing
  Object" modal blocks MCP until answered (Cancel at 348,200 of that 403x230 dialog).
- Grabbing timed effects: PIE's world time runs slow when the editor isn't in front; use `slomo 0.05-0.1`, then
  `console.ps1 ... -Wait 0.05` and grabs back to back. A blast's smoke column needs a view with open sky (the town sign hid it).
- `BugItGo` leaves ghost mode; `Walk` drops you to the ground (aim changes). z 170 near the town sign put the view under the
  street once; use 260.
- The Slag Bomb lands on the creature in sights: in a horde that is at the feet, inside its own fire (hence the eye scaling).
- Rocks near town for stone hits: SM_CliffFace_* round (-3500, 2700); the stone house at (-2680, 3100) reads as Wood
  (no Surface tag), so `Looter.Vfx.Impact 1 Stone` there.
