# Phase 3 lanes (2026-11-03): changes 2-10 of the audit plan, in parallel on disjoint files

The plan: `Docs/Work/2026-11-03-Audit/AUDIT.md` (Phase 2 table). Change 1 (kill speed: health into `UCreatureKindSettings`, crit 2.0,
Unpaid head crit, pace bands) is built and committed by Main first; every lane starts from that HEAD. The user asked for parallel agents
(2026-11-03 evening): they still play one commit at a time, so every lane keeps a live switch or cvar for its feel change where one exists.

## Rules for every lane
- Touch only the files in your lane's list (new files go beside them, named in your note). Another lane's file: ask Main, never edit.
- No builds (`Tools\launch.ps1`), no editor, no PIE, no `Tools\mcp.ps1`, no editor Python, no commits, no CODEMAP/Pipeline edits: Main
  builds once for every lane, runs each lane's tests, commits lane by lane, and merges your note into CODEMAP and the pipeline.
- Your note: `Docs/Work/2026-11-03-Phase3/<LANE>.md`: files changed (new ones with their one-line CODEMAP text), every number before
  and after, the cvars/settings that switch it, the tests to run (`-Filter`), and the PIE check for the user in one or two lines
  ("fire the AR at the wall: the reticle climbs and settles in ~0.3 s").
- Numbers as tunables: a `Looter.*` cvar read live, or a `UDeveloperSettings` field (`Config = Game, DefaultConfig`, written to
  `Config/DefaultGame.ini`) when the number is a design fact. Defaults in one place. No new hardcoded constants in the hot path.
- Small, self-contained; no refactors beyond the lane; under ~500 lines a file (split a file you push over); comments say why.
- Tests: the lane's own test file(s) only. Every rule you add gets a plain-data test. Don't touch another lane's test file.
- Report in three sentences when done (what, where, what the user will notice), plus the note's path.

## Lanes and files

| Lane | Change | Owns (edit) | Tests (own) |
|---|---|---|---|
| HITS | 2: hit confirmation + pooled, merged damage numbers | `UI/HUD/HudHitMarker.h/.cpp`, `UI/HUD/PlayerHUDWidgetHitMarker.cpp`, `Combat/HealthComponent.cpp` (SpawnDamageNumber only) + `.h` if needed, `UI/World/DamageNumberActor.h/.cpp`, `DamageNumberActorStream.cpp`, `DamageNumberWidget.h/.cpp`, new `UI/World/DamageNumberSubsystem.h/.cpp` | `Tests/GunfeelImpactTests.cpp` (HitMarker.Rules), `Tests/ElementFeelTests.cpp` (DamageNumbers.Element), new `Tests/DamageNumberPoolTests.cpp` |
| RECOIL | 3: a rifle burst that climbs; one kick layer | `Weapons/WeaponRecoil.h/.cpp`, `Player/ViewKick.h/.cpp`, `Player/PlayerViewKicks.cpp`, `Player/PlayerViewAim.cpp`, `Weapons/GunfeelKickTuning.h/.cpp` (not the Crit* lines), `GunfeelKickKinds.cpp`, `GunfeelKickPattern.cpp`, `WeaponBaseKick.cpp` | `Tests/GunfeelKickTests.cpp` (not RevolverCrit), `GunfeelKickPatternTests.cpp`, `GunfeelIdentityTests.cpp`, `GunfeelIdentityFeelTests.cpp`, `GunfeelSteadyTests.cpp` |
| FIGHTS | 4: packs press together, smaller hits, enemies hear gunfire | `Creatures/FightTuning.h/.cpp`, `Creatures/CreatureBase.h` (PackAlertRadius default and a hearing hook only), `Creatures/CreatureBaseHurt.cpp`, `Creatures/CreatureBaseHunting.cpp`, `Creatures/FightPaceRules.cpp` (StandingDeathBands only), `Creatures/FightDirectorSubsystem.cpp`, `FightDirectorPlaces.cpp`, `Combat/BulletSubsystem.cpp` (a shot-fired broadcast only), the kinds' constructors for `PackAlertRadius` (`SpiderCreature.cpp:` the PackAlertRadius line only) | `Tests/FightTests.cpp`, `FightPaceTests.cpp` (the standing-death part only), `FightPressureTests.cpp`, `CreaturePursuitTests.cpp` |
| LOOT | 5: rarity that plays differently; every drop seen | `Weapons/WeaponParts.h/.cpp` (Pick), `Art/Models/Weapons/*.parts.csv` (Weight column of effect/element rows; Main reruns `setup_gun_parts.py`), `Weapons/LegendaryRules.h/.cpp`, `LegendaryEffectComponent.*`, `LegendarySubsystem.*` (a rolled Legendary's signature by seed, no save change), `Loot/LootDropMoments.h/.cpp`, `Loot/LootBeamCrowd.h/.cpp`, `Loot/LootFanfareSubsystem.cpp` (Rare/Uncommon stinger 2D) | `Tests/WeaponPartsTests.cpp`, `DropMomentTests.cpp`, `RoundThreeFixTests.cpp` (BeamCrowd), `LegendaryWeaponTests.cpp` |
| CAMERA | 6: the camera reacts to movement | new `Player/PlayerCameraMotion.h/.cpp` (landing dip by fall speed, step bob, jump kick, sprint sway, strafe roll as plain math), `Player/PlayerLocomotionViewModel.cpp` (wire it in `HandleLanded` and the per-frame eye), `Player/PlayerLocomotionComponent.h` (the `OnTraversalLanded` listener), `Player/PlayerLocomotionTraversal.cpp` (only if the broadcast needs the fall speed) | new `Tests/CameraMotionTests.cpp` |
| GAMEPAD | 7: pad look by delta time, aim assist, stick curve, invert-Y, mouse smoothing off | `Core/LooterCharacter.cpp` (`Look`), new `Player/AimAssist.h/.cpp` (friction + magnetism as math over the creatures in view), `Settings/ControlSettingsSubsystem.h/.cpp` (+ `ControlsSave`), `UI/Menus/SettingsMenuPages.cpp` + `SettingsMenuHelp.h/.cpp` (rows: Invert Y, Pad sensitivity, Aim assist), `Config/DefaultInput.ini` | new `Tests/AimAssistTests.cpp`, `Tests/SettingsTests.cpp` (rows) |
| MUZZLE | 8: the gun lights the hands and the ground | `Weapons/WeaponBase.cpp` (the muzzle light), `Weapons/WeaponBaseMuzzle.cpp`, `Weapons/VfxTuning.h/.cpp`, `Weapons/VfxRulesMuzzle.cpp`, `Weapons/WeaponFXImpacts.cpp` (an impact flash light on hard surfaces, pooled, budgeted) | `Tests/VfxTests.cpp` |
| PERF | 10: no per-frame world scans; a sustained-fire perf row | new `World/ActorIndexSubsystem.h/.cpp` (lists of pickups, creatures, interactables refreshed on a 0.1 s timer, never per frame), `UI/HUD/HudMinimapWidget.cpp` (NativeTick scans -> the index), `UI/World/LootLabelSubsystem.cpp` (+ `IsTickable` gate), `Inventory/WeaponManagerPickups.cpp` (`GatherInteractions` -> the index), `Combat/EnemyProjectileSubsystem.cpp` (use `UGroundIgnoreSubsystem`), new `Dev/PerfFireCommands.cpp` (`Looter.Perf.Fire <kind> <count> [seconds]`: the player firing an LMG/shotgun into a horde for perf.ps1 -Exec), `Config/DefaultEngine.ini` (gc.* only with a reason written in the note) | new `Tests/ActorIndexTests.cpp` |
| Main | 9: the fire timer carries its overshoot | `Weapons/WeaponBaseFiring.cpp` | `Tests/WeaponTriggerTests.cpp` |

Nobody touches: `Combat/CombatRules.h`, `Creatures/CreatureKindSettings.*`, `Creatures/UnpaidCreature*`, `Bestiary/*`, `CODEMAP.md`, `Docs/Pipeline/*`,
`Docs/Handoff.md`, any `.uasset`/`.umap`, `Art/Sounds/cues.json` (a 2D stinger is a code-side choice in LootFanfareSubsystem).

## What each lane must hit (the audit's numbers; AUDIT.md has the where and why)
- HITS: marker 0.18 s -> >= 0.35 s, crit red/white and >= 1.6x the body marker, kill distinct; damage numbers pooled (16-24, like
  `UXpPopupSubsystem`), one number per target per <= 80 ms window summing the hits (a shotgun blast = one big number), crit colour and
  the element/WEAK/RESIST looks kept, status streams kept; no `SpawnActor` per hit. `Looter.UI.HitMarker.*` and `Looter.UI.DamageNumbers.Merge*` cvars.
- RECOIL: `AimRecoveryWhileFiring` 0.35 -> ~0.1 (a cvar), the camera kick's spring slow enough to stack (14 Hz -> ~8 Hz for the rifle so
  `SustainCap` means something), per-shot aim kick 0.49 -> ~0.8 deg with a drift the player can see outside the hip cone, `Recoil.More`
  applied once, the `Looter.Gunfeel.Identity 0` path deleted (one layer), the revolver's aimed flip back near 5 deg, the shotgun's aimed
  push clamped under the eye-to-sight gap. Goal: a held 30-round Common rifle burst climbs 5-8 deg and wants pulling down; an Epic with
  +40% accuracy still kicks at >= 60% of a Common's (narrow `AccuracyCalm`'s range). Re-fit the tests to the new numbers; keep the spring math.
- FIGHTS: `MeleeTokens` 3 -> 5, `RangedTokens` 2 -> 4, `DamageScale` 1.8 -> 1.2, `RingRadius` 480 -> 300, feints 0.15 -> 0.08; Basic
  spiders/slimes/Dredgers answer a packmate's hurt within 15 m (`PackAlertRadius`), and any creature within 30 m that is idle hears a
  player's gunshot and hunts (a broadcast from `UBulletSubsystem` as a shot is fired; cvar `Looter.Fight.HearGunfire` [3000 cm], 0 off).
  Refit `StandingDeathBands` with the paper model (`Looter.Fight.Pace.Bands` must pass; target still 8-12 s standing). Main runs
  `Looter.Fight.Sim` after the build.
- LOOT: in `WeaponParts::Pick` at Rare and up one slot (by seed) always rolls a part of the gun's own tier or higher; effect-part rows'
  Weight 0.5-0.6 -> 3 and element rows 0.35 -> 1.5 in the CSVs (a Rare carries an effect ~40% of the time, an Epic ~70%, a Legendary
  always: by `LegendaryRules`, a rolled Legendary picks one of the three signatures by seed, shown on its card); Commons get a 250 cm
  beam and a soft landing cue; `LootBeamCrowd` counts only guns whose beam shows; Uncommon/Rare stingers play 2D. Save-safe: part keys unchanged.
- CAMERA: landing dip 0.4-1.6 deg pitch + 1-4 cm eye drop by fall speed (none under a step), step bob 0.15-0.35 deg at the stride,
  jump kick 0.3 deg up, sprint sway 0.2 deg roll at the stride, strafe roll 0.6 deg; all x the Camera shake setting; cvars `Looter.Camera.Motion.*`, master `Looter.Camera.Motion` [1].
- GAMEPAD: stick look = degrees per second x DeltaTime (a cvar `Looter.Look.PadDegreesPerSecond` [220 yaw / 160 pitch] x the sensitivity
  setting), a response curve (power 1.6, dead zone 0.18), invert-Y (Settings > Controls, saved), aim friction (sensitivity x 0.6 while the
  crosshair is within a creature's capsule-plus-20-cm) and magnetism (a pull of up to 2 deg/s toward the nearest creature within 2 deg
  while the stick moves), pad only; `bEnableMouseSmoothing=False`, `bEnableFOVScaling=False`.
- MUZZLE: the muzzle light 24 cd -> ~400 cd, 3.2 m -> 5 m, 40 ms, no shadows, first and third person; aimed flash 0.55 -> 0.8; an
  impact flash light (~150 cd, 60 ms, pooled 4, hard surfaces only, never nearer than 1 m to the eye). Cvars `Looter.Vfx.Muzzle.Light*`, `Looter.Vfx.Impact.Light*`.
- PERF: the three HUD/label/interaction scans read the index; `LootLabelSubsystem` sleeps with no labels; `Looter.Perf.Fire` exists and
  is in the note's perf recipe (`Tools\perf.ps1 -Label "fire" -Exec "Looter.Perf.Horde Spider 10 Basic 0 -1400 90,Looter.Perf.Fire LMG 10"`).
