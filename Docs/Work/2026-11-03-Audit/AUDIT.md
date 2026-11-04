# Core-loop audit, 2026-11-03 (Phase 1: nothing changed; Phase 2: the plan, awaiting the user's order)

Brief: the user's template (UE 5.8, C++-first, Borderlands feel; "not happy with how it's turning out"). The bracketed fields
were left unfilled, so the brief is the 10-21 grill ("guns feel alike", "mobs not frightening", "something's off") and the 41
notes of 2026-10-27. Six read-only audit agents (gunplay, enemies, loot, movement, UI, perf) + Main's own verification of every
headline number below in the source. Editor closed: data-asset values read from the asset files and the scripts that write them.

## The root cause (say it first)
310k lines of non-test .cpp, 1,100 console variables (373 `Looter.Gunfeel.*`, 159 `Looter.Unpaid.*`), 0 curve assets. Every feel
system since 10-21 (kick rounds 1-4, gun identity, hit feel, the Unpaid's ~17 layers, elements, hands, the 10-31 creature rework)
was tuned by agents reading log lines and single screenshot grabs ("Feel itself not judged (no recording)", RESULTS.md). The user
last reported playing the 10-23 build (notes 10-27) and three items on 10-30. The docs mark at least ten feel systems "not checked
in play / not heard by ear / not seen by the user" (Gunplay.md l.33, 153, 156, 193, 196, 318, 500, 593; Handoff l.149, 186, 201, 255).
Nobody has sat with a mouse and tuned one number by feel. That is why "something's off" survives every pass.

## Top problems, ranked by how much they hurt the player

1. **Kills are slow, and the pace rules ask for it** (Gunplay & feel). `DA_AssaultRifle` 18 dmg, 650 rpm, 0.8 deg; `DA_PumpShotgun`
   8 x 12, 75 rpm; Drover 52 at 140 rpm (`create_revolver.py`). Spider 230 HP (`SpiderCreature.cpp`), Unpaid 160 (`UnpaidCreature.cpp:83`).
   Rifle: 13 hits = 1.2 s at 100% hits, 2.0 s at the design's own 60%; Restless Unpaid (x2 HP, 10% defense) 22 hits. `FightPaceRules.cpp`
   `BasicKillBands` l.124-142 targets 2.0-2.5 s spider / 1.6-2.2 s Unpaid / 4-6 s Restless; Borderlands is 0.5-1.5 s. Crit 1.5x
   (`CombatRules.h:9`; BL ~2x); the Unpaid has NO head crit (`UnpaidCreature.cpp:86`, coal only, 12 cm, within 60 deg of its front)
   though a head hit knocks its hat off. Shotgun 120 dps and revolver 121 dps are under the rifle's 180-195: point blank the shotgun
   needs 3 blasts on a Basic spider (2 blasts = 192 of 230). Health sits just over breakpoints: Unpaid 160 vs 3 revolver body shots 156;
   Restless 348.8 vs 4 blasts 345.6. "One more shot" kills read as misses.
2. **Threat is capped by tokens, not the pack; the common enemy barely lands a hit** (Enemies & AI). `FightTuning.h:19-21` MeleeTokens 3,
   RangedTokens 2 per player, ring 480 cm (l.33), feints 15%: a 9-pack hits like 3 for three times as long ("nine stood in a line",
   PLAYTEST.md). Chapel yard: 59 Unpaid attacks started, 2 landed. `DamageScale` 1.8 (l.97) makes the few hits huge: spider bite 19.8 at L1,
   a still L3 player dead in 3.2 s; Unpaid 10-12 s. Perception 360 deg, one trace / 0.2 s to 31 m, nothing hears gunfire, `PackAlertRadius`
   0 (`CreatureBase.h:374`): no fight spills over. Enemies +9%/level compounding, player health +6% (`LevelRules.h:21-22`).
3. **The rifle is a hose and the four kick rounds fight** (Gunplay & feel). One Common shot: aim kick 0.35 x 1.4 = 0.49 deg
   (`WeaponBaseKick.cpp:31`), camera kick 0.55 deg at 14 Hz, damping 0.6 (`ViewKick.cpp:249-254`): gone in ~100 ms, before the next
   round (92 ms), so `Kick.SustainCap` never engages. The aim recovers at 35% rate WHILE firing (`WeaponRecoil.cpp:13`): a held 30-round
   burst rises ~1.6 deg then plateaus at 2.3-2.6 deg with no pull-down needed; sideways never past +-0.35 deg (inside the 0.8-1.26 deg
   hip cone: the drift pattern is invisible). `Recoil.More` 1.4 is divided back out of the camera kick by the 10-28 table but still
   multiplies aim and model; the revolver's aimed flip is back to 11 deg (10-22 cut it to 5). Rarity's spread multiplier alone calms
   kick through `AccuracyCalm` (Epic 0.72, Legendary 0.59): an Epic with +40% accuracy kicks at 30% of a Common. `Identity 0/1`
   keeps two kick layers alive (`GunfeelKickKinds.cpp:180-219`).
4. **Hit confirmation is thin** (UI/UX + VFX). Hit marker: four 11x4 px ticks, 0.18 s, pop 0.3 for body AND crit; crit = pale gold
   (`HudHitMarker.h:36-46`), camouflaged on RR's grass; damage numbers: one actor + UWidgetComponent per pellet, no pooling, no
   per-target merge (`HealthComponent.cpp:214-239`, `DamageNumberActor.cpp:53`), so a shotgun piles 8 numbers on one chest.
   Muzzle flash shrunk three ways (star 0.5, tongues 0.3, bloom 0.5; aimed 0.55 x 0.6 glow for 40 ms, `VfxTuning.h:39-50`), muzzle light
   24 cd / 3.2 m (`WeaponBase.cpp:76-82`) invisible in daylight, impacts have no light. Full-auto also loses rounds: the next shot's timer
   is set fresh each shot (`WeaponBaseFiring.cpp:257`) so 92.3 ms rounds to the frame: 650 rpm fires 600.
5. **Rarity buys a stat line, not a different gun; Commons are invisible** (Loot). `WeaponParts.cpp:96-104`: any part with MinRarity <=
   Rarity, Common parts weight 1, effect parts 0.5-0.6: a Rare Bullpup carries a behaviour effect 6.7% of the time (Epic 28%); Ranchhand,
   Thresher, Powderkeg 0% at Rare; a rolled Legendary has no signature (only 3 named guns do). Rarity itself (`WeaponDefinition.cpp:25-29`):
   Rare x1.25 dmg, Epic x1.45, Legendary x1.75 = ~4 levels of +9%. Part deltas +-2..8% (`Bullpup.parts.csv`) are smaller than the
   +-10% seed roll. Commons: no beam, no stinger (`LootDropMoments.cpp`), and `LootBeamCrowd.cpp:65-71` counts every pickup gun, beam or
   not, so ammo/silver within 3 m of a Common lose THEIR beam (bug). Rare stinger -21 dB 3D on the creature attenuation: inaudible at
   20 m under fire; Epic/Legendary are 2D. Kills: 24% drop a gun, 3.5% Rare+; chests/rewards pay ~6 Epics an hour. Level-up gives +6%
   health, nothing else (skills owned from the start, no ranks; slot 4 after the area ends).
6. **The camera is dead and the gamepad is broken** (Movement). `PlayerLocomotionViewModel.cpp:233` `HandleLanded` only pushes the gun
   spring; no landing dip, step bob, jump kick or strafe roll (sprint FOV +6 deg and slide roll 5 deg are the only camera motion);
   `OnTraversalLanded` has no listener. `LooterCharacter.cpp:139-148` adds the stick straight to yaw/pitch with no delta-time
   (turn speed doubles at 120 fps); no aim assist, no stick curve, no invert-Y; `DefaultInput.ini` mouse smoothing on. Speeds are fine
   (walk 459, sprint 711, ADS 298; stop in ~0.1 s, engine-default accel/brake: crisp to stiff).
7. **Hands and reloads are code-posed with a hard ceiling** (Animation). 6,286 lines in `Player/FirstPersonHands*`, every curve
   `SmoothStep` (`ReloadMotion.cpp:39-51`), three placement systems kept as fallbacks (tables, cup-fit rays, marker sockets; only the
   AR has markers). No anticipation, overshoot, secondary motion or a thrown magazine. Part of "guns feel alike, reloads weightless".
8. **Enemies don't read** (Enemies & art). 5 of 14 regular kinds are clay placeholders (36%, 25 bodies, two side missions). The Unpaid's
   teleport is decided in 7 places, up to ~50 blinks/min, flanks land 3.3 m behind at 105-180 deg; its idle is a floating arms-out
   scarecrow (screenshots n_unpaid.png, pt_gf1.png). ~17 behaviour layers on a 160 HP body that lives 1.3 s; the layers contradict
   (opening-punish frenzy vs the close-call ease on the same flask drink; camping punish vs blind turns given up). Four flee rules,
   two gap systems, four screams. Kills never respawn within a session (`EncounterSpawner.h:68`): nothing to farm in one sitting.

Performance & jank (not top-8): margin 0.07 ms at the heaviest view (8.26 vs 8.33); gunfire is in no perf row (`Looter.Perf.Horde`
never fires); six `TActorIterator` world scans per frame (minimap 3, loot labels 2, interaction 1); no gc.* settings; cold first-use
loads (Niagara cinder, chest tier meshes, pellet mesh); worst frames 28-34 ms at MineDrift/Aerial_Dusk undiagnosed; no Shipping build
ever made. Architecture, only where it blocks Phase 3: 1,100 cvars with C++ defaults and no ini (a tuned value dies with the session;
shipping it is a recompile); 320 `case EWeaponKind::` in 29 files; 0 `UCurveFloat`/data tables; parallel fallback paths (hands x3,
kick x2).

## Working well: protect
Recorded gun cores + the surroundings tail probe; exact frame-rate-independent spring recoil (`FWeaponRecoil::FSpring`); the hit
reaction chain (flinch on every hit, slow, pressure stagger, shotgun knock, kill styles, hit-stop); the pickup flow (tap/hold, card
with keys under it, pile folding, 1-click backpack equip); Epic/Legendary drop moments and chest tiers; the early legendary cap;
the effect parts themselves (Twin, Burst, Overcharge, Ricochet, Piercing, Explosive); telegraphs (lunge locks aim at the wind-up's
end, no homing), the rise ambush, boss attack windows + hurt-zone debug; tick/IsTickable discipline, pooling where applied
(XP popups, fanfare, bullets as data, ISM tracers), perf rows per change; 1,288 tests; the Tripo spider.

## The one change first
Make a Basic kill fast and precision pay: Basic enemies die in 1.0-1.3 s at 60% hits (two shotgun blasts, three revolver shots),
health set just UNDER the guns' breakpoints, crit 2.0x, an Unpaid head crit. Numbers only + one bone list; redraw `BasicKillBands`.
Every other feel layer fires per kill, so this multiplies all of them. Touches the user's "leave the spiders" call (packs die faster): ask.

## Phase 2 plan (proposed order; each small, data first)
| # | Change | Files | Player notices | Risk |
|---|---|---|---|---|
| 1 | Kill speed + crits: spider 230->~170, Unpaid 160->~140 (3 revolver / 2 blasts / ~9 rifle), crit 1.5->2.0, Unpaid head crit zone, bands redrawn | `SpiderCreature.cpp`, `UnpaidCreature.cpp:83-86`, `CombatRules.h:9`, `FightPaceRules.cpp:124-142`; move base health/damage per kind into `UCreatureRankSettings` or `DefaultGame.ini` (data) | kills in a second, headshots matter | Pace tests' guards; Ledger stats; spider packs softer (the user's call) |
| 2 | Hit confirmation: marker 0.18->0.35 s, crit red + 1.6x; pool damage numbers (XpPopup pattern) and merge per target per 80 ms | `HudHitMarker.h` FTuning, `HealthComponent.cpp:214-239` -> a pooled subsystem, `DamageNumberActor*` | every shot confirmed, one big shotgun number | low; a perf win |
| 3 | Recoil that climbs: `AimRecoveryWhileFiring` 0.35->0.1, camera kick 14->8 Hz so kicks stack and the cap works, per-shot aim 0.49->~0.8 with visible drift, `Recoil.More` applied once, drop `Identity 0` | `WeaponRecoil.cpp:13`, `ViewKick.cpp:247-266`, `GunfeelKickTuning.cpp`, `GunfeelKickKinds.cpp`; per-kind FKindFeel into a data asset with `UCurveFloat` climb/recovery | a held burst climbs 5-8 deg and wants pulling down; accuracy stat felt | Gunfeel.Kick/Identity tests re-fit; two layers collapse to one |
| 4 | Fights press: MeleeTokens 3->5, RangedTokens 2->4, DamageScale 1.8->1.2, ring 480->300, Basic `PackAlertRadius` 0->1500, enemies hear gunfire within 30 m | `FightTuning.h` (cvars live), `CreatureBase.h:374`, `CreatureBaseHurt.cpp:56-71`; FFightTuning into `UEncounterSettings` (data) | packs come together, many small hits, the ward matters | Fight.Pace/Sim bands; spiders (ask) |
| 5 | Loot that differs: effect-part weight 0.5->3 at Rare+, one slot forced to the gun's tier, rolled Legendary gets a signature; Common beam 250 cm + the beam-crowd fix; Rare stinger 2D | `WeaponParts.cpp:96-104`, parts CSVs (data), `LegendaryDrops`/`LegendaryRules`, `LootDropMoments.cpp`, `LootBeamCrowd.cpp:65` | every blue+ "does something"; every drop seen | save-safe (part keys kept); CSV weights are data already |
| 6 | Camera locomotion layer: landing dip (by fall speed), step bob, jump kick, sprint sway on `FViewKick` | `PlayerLocomotionViewModel.cpp:233`, `OnTraversalLanded` listener, `PlayerViewKicks.cpp` | movement has weight | low; scaled by the camera-shake setting |
| 7 | Gamepad: ScaleByDeltaTime on the stick (IMC_Default asset), aim friction + magnetism in `PlayerViewAim.cpp`, stick curve, invert-Y, mouse smoothing off | `LooterCharacter.cpp:139-148`, `DefaultInput.ini:97`, IMC asset | pad playable at any fps | low |
| 8 | Muzzle: light 24->~400 cd for 40 ms, aimed flash 0.55->0.8, impact flash light on hard surfaces | `WeaponBase.cpp:76-82`, `VfxTuning.h:39-50`, `WeaponFXImpacts.cpp` | the gun lights hands and ground | perf: one unshadowed light per local gun |
| 9 | Fire timer carries overshoot (next shot due = last due + interval) | `WeaponBaseFiring.cpp:254-258` | 650 rpm is 650 rpm | tiny |
| 10 | Perf hygiene: a shared 0.1 s actor registry for minimap/labels/interaction; `Looter.Perf.Fire` sustained-fire row; gc.* defaults reviewed | `HudMinimapWidget.cpp:260-274`, `LootLabelSubsystem.cpp:166-184`, `WeaponManagerPickups.cpp:129`, `DefaultEngine.ini` | no hitch in a firefight | low |
| 11 | Unpaid: one teleport gate (`MayStep`) for all 7 paths, blink cap ~15/min, one flee rule; art: a hunched drifting idle instead of arms-out | `UnpaidCreature*`, `CreatureTraitComponentPhase.cpp`, `Unpaid.py` rest pose | the common enemy reads | bigger; after 1-5 |
| 12 | Reload feel: spring overshoot slaps, a thrown magazine; later an authored-clip path for hands | `ReloadMotion.cpp:39-51`, `FirstPersonHandsReload.cpp` | reloads with snap | big; last |

Deferred (needs art/money or a design call): placeholders -> real kinds; farmable respawns per session; level-up rewards (skill ranks).
Tuning surface: persist tuned cvars in `DefaultEngine.ini [ConsoleVariables]` so a PIE tune ships without a recompile (process, 0 code).

Phase 3 rule: one change, build, the USER plays it in PIE with the editor focused, then the next. No agent log-line "PASS" counts as feel.
