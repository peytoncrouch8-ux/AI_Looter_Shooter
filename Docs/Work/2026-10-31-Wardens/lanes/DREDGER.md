# Lane DREDGER: Vane's Dredgers (riflemen + hose-men)

Built to CONTRACT.md "DREDGER". Not built, not run (lane rules): every risk below is unchecked.

## Files (all new; CODEMAP lines for Main)

- `Creatures/DredgerTypes.h`: `EDredgerRole` (UENUM: Rifleman, HoseMan; append only), `EDredgerTactic`, `EDredgerHoseState`, `EDredgerHoseBreak`, `EDredgerKeep`.
- `Creatures/DredgerTuning`: `FDredgerTuning` (`Live()`), the `Looter.Dredger.*` cvars (field values = defaults).
- `Creatures/DredgerRules`: `DredgerRules` (pure, tested): `HitChance`, `MissPoint`, `LeadPoint`, `ChooseKeep`, `FlankOffset`, `ScoreSpot` (`FDredgerSpotFacts`), `SpotCandidates`, `BurstRecovery`/`BurstCooldown`, `CanLatch`, `HoseBreak` (`FDredgerHoseFacts`), `DrainPerSecond`, `PullInput`, `TankBursts`, `HosePoint`.
- `Creatures/DredgerLook.h`: `DredgerLook`, every stand-in path and colour in one place (body mesh, hose-man body, company rifle def + seed, tank prop, shapes, master).
- `Creatures/DredgerBody`: `FDredgerBodyInput` (the body's frame for the anim: speed/way, the player's `FLooterStanceInput`, flinch, stagger, wound, lean, brace, notice, breath, fall), `DredgerBody` (clip paths, `WayBlend`, `GaitShares`, `CycleRate`, `ApplyCodeLayer`).
- `Creatures/DredgerBodyAnimInstance`: `UDredgerBodyAnimInstance` + `FDredgerBodyProxy`, no graph: the mannequin's 8-way walk/jog clips on one step cycle, MF_Rifle_Idle_ADS held on the upper body (spine kept in mesh space, the player's hold), MM_Rifle_Reload overlay, then `LooterStancePose::Apply` (crouch, aim pitch, left hand on foregrip, kick) and the code layer.
- `Creatures/DredgerCreature` (+ `DredgerCreatureKit.cpp` look and stand-in kit, `DredgerCreatureReact.cpp` hits/barks/notice/death): `ADredgerCreature : ACreatureBase` (Abstract), `Role`, `ClassFor`, `SetHoldsGround`, `GetBodyInput`, `GetMuzzle`, `Bark`, `ResetBarks`; mannequin in black rubber, gas mask (face, snout, filter, two lenses that flare as tells), cap; head = crit (a crit breaks a lens); held thing on `HandGrip_R` turned to the aim like the player's gun; ragdoll death thrown by kill style (code fall when off), soul-smog from the mask, sink.
- `Creatures/DredgerRifleman` (+ `DredgerRiflemanCover.cpp` distance and cover, `DredgerRiflemanFire.cpp` rounds/tracers/reload/butt): `ADredgerRifleman`, Ranged turns, cover search by traces (4 spots a frame), squad flank bearings, crouch in cover, burst tell (rise, lens flare, gold muzzle glint), 3 real rounds via `UEnemyProjectileSubsystem` + tracer streaks + impact puffs, 9-round magazine, reload (clip + magazine out/in + call), butt within 2.3 m, `DevFindSpot`, `DevStartReload`.
- `Creatures/DredgerHoseMan` (+ `DredgerHoseManTank.cpp`): `ADredgerHoseMan`, Melee turns, latch tell (plant, cyan charge at the nozzle, gauge and lenses flare), brain held while latched (braced, tugged, facing), tank + gauge crit zones, `NoteTankHit`/`BurstTank` (soul blast, ring, shake, splash on creatures, self stagger + damage next tick, hose torn, club mode).
- `Creatures/DredgerHoseComponent` (+ `DredgerHoseComponentDraw.cpp`): `UDredgerHoseComponent`: throw, seek, latch, drain (damage ticks 0.35 s), pull (share of the victim's walking input), slow, rattle (frost edge + arc), break rules, snap; both hoses as cylinders on one ISM, soul pulses drawn in, hold glow; `GetDrainOn(Pawn)` for the HUD.
- `Dev/DredgerDevCommands.cpp`: `Looter.Dredger.Spawn <Rifleman|HoseMan> [count] [rank] [level=n] [hold]`, `Looter.Dredger.State`, `.Latch`, `.Tank`, `.Reload`.
- `Tests/DredgerTests.cpp`: `Looter.Creatures.Dredger.Rules`, `.Classes`, `.Spawn`, `.Cover`, `.Latch`, `.Tank`.
- `Tools/Unreal/create_bestiary_pages.py`: added `DredgerRifleman`, `DredgerHoseMan` (ENEMY, sort 5/6, field-guide voice, stand SKM_Manny_Simple).

## Hooks for Main (exact edits in other lanes' files)

1. **Their spill** (else hits show the green ichor fallback). `Source/AI_Looter_Shooter/Weapons/CreatureBlood.cpp`:
   - after `#include "Creatures/DeadMinerCreature.h"` add `#include "Creatures/DredgerCreature.h"`
   - in `OfClass`, after the Stone Sentinel's `return ECreatureBlood::Clay; }` block add:
     ```cpp
     	// Vane's Dredgers: men in rubber coats, spilling dark blood and grit, never a spider's ichor (TintOf).
     	if (Class->IsChildOf(ADredgerCreature::StaticClass()))
     	{
     		return ECreatureBlood::Clay;
     	}
     ```
   - in `TintOf`, after the Stone Sentinel's `return GravemotherAssets::GraniteColor(); }` block add:
     ```cpp
     	// A Dredger's dark oxblood (a man's, darkened by the smog he breathes).
     	if (Cast<ADredgerCreature>(Actor))
     	{
     		return FLinearColor(0.16f, 0.025f, 0.02f);
     	}
     ```
2. **The cyan-black drain edge** (until then the rattle's pale frost stands in: `Looter.Dredger.Hose.FrostEdge 1`).
   `UI/HUD/HudScreenEdgeWidget.h`: after `UPROPERTY(Transient) TObjectPtr<UWidget> HealEdges;` add
   `UPROPERTY(Transient) TObjectPtr<UWidget> DrainInkEdges;` and `UPROPERTY(Transient) TObjectPtr<UWidget> DrainEdges;`;
   after `float ShownHealOpacity = -1.f;` add `float ShownDrainInkOpacity = -1.f;` and `float ShownDrainOpacity = -1.f;`.
   `UI/HUD/HudScreenEdgeWidget.cpp`:
   - after `#include "Player/PlayerRattleSubsystem.h"` add `#include "Creatures/DredgerHoseComponent.h"`
   - after `constexpr float HealGlowGone = 0.005f;` add `constexpr float DrainInkStrength = 0.7f;` and `constexpr float DrainCyanStrength = 0.45f;`
   - after `static const FPaintedIcon HealBottom = EdgeStrip(false, true, Color::Heal());` add eight strips:
     `DrainInkLeft/Right/Top/Bottom = EdgeStrip(..., Color::Ink())` and `DrainLeft/Right/Top/Bottom = EdgeStrip(..., Color::XPDark())` (same bAcrossX/bEdgeAtEnd pattern as the heal's four).
   - after `HealEdges = AddSet(TEXT("HudHealEdge"), HealLeft, HealRight, HealTop, HealBottom);` add
     `DrainInkEdges = AddSet(TEXT("HudDrainInkEdge"), DrainInkLeft, DrainInkRight, DrainInkTop, DrainInkBottom);` and
     `DrainEdges = AddSet(TEXT("HudDrainEdge"), DrainLeft, DrainRight, DrainTop, DrainBottom);`
   - after `ShownHealOpacity = 0.f;` add `ShownDrainInkOpacity = 0.f;` and `ShownDrainOpacity = 0.f;`
   - after `ShowEdgeLayer(HealEdges, HealGlow * HealEdgeStrength, ShownHealOpacity);` add:
     ```cpp
     	// The soul-hose draining them (Vane's hose-men): black creeping in with a cyan rim while the line holds.
     	const float Drain = UDredgerHoseComponent::GetDrainOn(Player ? Player->GetPawn() : nullptr);
     	ShowEdgeLayer(DrainInkEdges, Drain * DrainInkStrength, ShownDrainInkOpacity);
     	ShowEdgeLayer(DrainEdges, Drain * DrainCyanStrength, ShownDrainOpacity);
     ```
   - then in my `Creatures/DredgerTuning.h` set `int32 FrostEdge = 0;` (the latch still rattles once: sway, shake, arc).
3. **Optional, for perf hordes and fight sims** (`Dev/CreatureDevCommandsSpawn.cpp` `FindCreatureKind`, after the StoneSentinel branch; includes `Creatures/DredgerRifleman.h`, `Creatures/DredgerHoseMan.h`):
   ```cpp
   		else if (Kind.Equals(TEXT("DredgerRifleman"), ESearchCase::IgnoreCase) || Kind.Equals(TEXT("Rifleman"), ESearchCase::IgnoreCase))
   		{
   			Found.Class = ADredgerRifleman::StaticClass();
   		}
   		else if (Kind.Equals(TEXT("DredgerHoseMan"), ESearchCase::IgnoreCase) || Kind.Equals(TEXT("HoseMan"), ESearchCase::IgnoreCase))
   		{
   			Found.Class = ADredgerHoseMan::StaticClass();
   		}
   ```
   Then `Tools\perf.ps1 -Label "dredgers" -Exec "Looter.Quality Medium,Looter.Perf.Horde DredgerRifleman 5 Basic 0 -1400 90"`.
4. Sounds: I use SOUND's `LooterSoundCue::DredgerRifleFire`, `DredgerRifleReload`, `DredgerBark`, `DredgerHurt`, `DredgerDeath`, `DredgerHoseLatch`, `DredgerHoseDrain` (loop, started with `LooterSound::Start` on the victim), `DredgerHoseSnap`, `DredgerTankBurst`; plus existing `Melee::Hit` (the butt).

## For FLIGHT / WORLD (the API)

- Classes: `ADredgerCreature::ClassFor(EDredgerRole::Rifleman|HoseMan)`; paths `/Script/AI_Looter_Shooter.DredgerRifleman`, `/Script/AI_Looter_Shooter.DredgerHoseMan` (encounter groups' `CreatureClass`).
- Boarders: `ACreatureBase::SpawnAtRuntime(World, Class, Feet, Yaw, Spawn)` then `AlertTo(PlayerPawn)`. Tender riflemen: also `SetHoldsGround(true)` (no cover, no keep-away, the brain never walks them toward the player; they turn and fire). All Dredgers have `bCanWalkOffLedges = false`; they need walkable static collision under them (no navmesh). Pack tag `Dredger` (one shot one, all within 30 m turn).
- Levels: by area (RR 1-2), nothing special.

## Run order

1. Main applies hooks 1-2 (3 optional). 2. Build. 3. SOUND's `Tools\sounds.ps1` dredger patterns + `build_sound_bank.py` (a cue without a sound plays nothing; nothing breaks).
4. `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_bestiary_pages.py DredgerRifleman DredgerHoseMan"` (stand model is the mannequin, which exists, so both pages are made now).
5. `Tools\runtests.ps1 -Filter Looter.Creatures.Dredger` (and `Looter.Bestiary` once the pages exist).

## PIE check (Main)

- `Tools\pie.ps1 -Map RansomsRest`, then in town (open ground): `Looter.Dredger.Spawn Rifleman 2`. Watch: they take spots 10-25 m off (two different bearings), crouch behind fences/crates if any, rise with a lens flare and a gold glint before each burst, 3 visible gold tracers, rounds puff on the ground near you, whizz by; after 3 bursts the reload call and the magazine out/in. Shoot a head: crit, a lens goes dark, a stagger cuts a burst. Kill: ragdoll thrown by the gun (try the shotgun close), smog puff from the mask, sink after ~7 s. `Looter.Dredger.State` to read the tactic/spot.
- `Looter.Dredger.Spawn HoseMan`: he walks in; at ~8 m the tell (plant, cyan swell at the nozzle, gauge flare), the line flies; latched: pulled slower, drained ~5 HP/s, frost edge + hiss, pulses run back along the hose. Break it three ways: sprint 14 m, step behind a wall, crit his mask. Shoot the tank (crits): at 30% of his health it bursts (cyan blast, ring, shake), he staggers and clubs after. `Looter.Dredger.Latch` / `.Tank` force those moments.
- `Looter.Dredger.Spawn Rifleman 2 Restless hold`: they stand and fire from where they are.
- Look at: the kit's placement on the head (mask, lenses, cap: `DredgerCreatureKit.cpp` `PlaceKit` numbers), the rifle in both hands (left hand on its foregrip), the crouch, the feet sliding or not (tune the clip speeds), the tank on the back, the hose's sag.

## Numbers to tune (cvars `Looter.Dredger.*`)

Rifle: `.Rifle.BulletScale` 1 (7 a bullet x level/rank, then Looter.Fight.DamageScale 1.5: about 10.5 felt), `.Burst` 3, `.BurstGap` 0.13, `.Period` 2.2, `.Windup` 0.6, `.Magazine` 9, `.Reload` 2.4, `.Accuracy` 0.7 @15 m (`.AccuracyNear` 0.88 @8 m, `.AccuracyFar` 0.3 @30 m), `.MovingPenalty` 0.3, `.BulletSpeed` 7500, `.Near` 1000, `.Far` 2500, `.Preferred` 1600, `.CoverRecheck` 2.5, `.RelocateAfter` 3, `.FlankAngle` 40, `.ButtReach` 230.
Hose: `.Hose.LatchRange` 800, `.BreakRange` 1400, `.LineSpeed` 3200, `.Drain` 5, `.Pull` 0.35, `.Windup` 0.85, `.MaxSeconds` 8, `.Rest` 2.5, `.BlockedBreak` 0.4, `.FrostEdge` 1. Tank: `.Tank.Share` 0.3, `.Stagger` 1.6, `.Radius` 320, `.Splash` 0.15, `.Self` 0.15. Body: `.Body.WalkSpeed` 165, `.JogSpeed` 375 (clip speeds: guessed), `.Ragdoll` 1, `.Kit` 1; `.BarkGap` 1.6. Class stats (in code, the bestiary reads them): rifleman 140 HP, 7 dmg, 14 XP, walk 170/chase 360; hose-man 220 HP, 10 dmg (club; drain grows from it), 18 XP, size 1.03, walk 150/chase 290.

## Stand-ins (one path each, `Creatures/DredgerLook.h`)

Body `SKM_Manny_Simple` darkened (both jobs; `HoseManBodyMeshPath` separate), company rifle = `DA_AssaultRifle` parts at seed 1771, tank = `SM_Barrel_A` brass, nozzle/mask/lenses/cap = engine shapes. Tripo swap: point the paths at the rigged Dredger models, set `FDredgerTuning::Kit` default 0 (keeps their materials, hides the shape kit; the tank stays, it is the weak spot), re-run the bestiary pages with the new `model`.

## Risks I couldn't check

- Compiles unverified: watch `FCompactPose::GetRefPose`/`CopyBonesFrom`, `FAnimExtractContext(double)`, `SetAllPhysicsLinearVelocity`, `AddImpulseToAllBodiesBelow`, `OverlapAnyTestByObjectType`, `FHitResult::Component` assignment, mixed float/double in `FMath::Clamp`.
- `HandGrip_R` must be on SKM_Manny_Simple's skeleton (the player's hidden mannequin uses it); without it the held thing hangs off the hand untuned (a warning logs).
- The `MF_Unarmed_*` clip names for all 8 ways (from the content folder) and whether they carry root motion (the root is locked to rest anyway). Clip speeds guessed: foot sliding until tuned.
- PA_Mannequin as SKM_Manny_Simple's physics asset (hit zones and ragdoll). The ragdoll detaches the mesh from the capsule; the hit reaction's corpse knock then moves only the empty capsule.
- `LooterStancePose::Apply` was made for the player; fed a Dredger's input (crouch, aim, foregrip, kick, reload weight). Elbows/left hand may need the `Looter.Grip.*` values; seen only in PIE.
- Small decks: the cover search wants 10-25 m; on the Lamplighter (24 x 7 m) riflemen mostly stand Exposed/BackOff (they may fire backing off). If they crowd the player, lower `Looter.Dredger.Rifle.Near` for Landfall.
- Perf: each Dredger samples 3-6 clips a frame on a worker thread (one a gait far off), ~13 static mesh parts (rifle parts + kit), the hose ISM; a cover search is ~68 traces over ~4 frames every ~2.5 s per rifleman. Measure with hook 3's horde.
- The hose's drain goes through `ApplyDamage` (the ward first; it can kill); a test level deals none (the tests read `GetDrainDealt`).
