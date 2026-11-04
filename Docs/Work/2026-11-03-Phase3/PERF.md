# PERF lane (change 10): no per-frame world scans, and a sustained-fire perf row

Written, not built, not run (the lane rules). Everything below is by reading the code; Main's build and runs confirm it.

## Files

New (CODEMAP text):
- `World/ActorIndexSubsystem.h/.cpp`: `UActorIndexSubsystem`, the world's loot (guns, ammo, grenade, silver, ward pickups), creatures (with a sentinel flag), non-creature characters, volumes and playable areas as lists; one TActorIterator pass per list at play start and every `Looter.ActorIndex.Interval` s (0.1), plus the spawn hook; the HUD map, loot labels, weapon manager and enemy shots read it instead of scanning every frame. `FIndexedCreature`.
- `Dev/PerfFireCommands.cpp`: `Looter.Perf.Fire <Rifle|Shotgun|LMG|SMG|Revolver|Sniper> [seconds] [kill]`: the player takes that gun in hand (given if they have none), aims at the nearest creature and clicks every frame for N seconds, unhurtable, horde kept on its feet; logs bullets, hits, crits, damage. Not in shipping.
- `Tests/ActorIndexTests.cpp`: `Looter.World.ActorIndex.Lists/.Gone/.Reads/.Rate/.Gate`.

Edited:
- `UI/HUD/HudMinimapWidget.cpp`: NativeTick's three scans read the index.
- `UI/World/LootLabelSubsystem.cpp` + `.h`: the two scans read the index; `IsTickable` gate (`NeedsTick` is its plain rule). The `.h` change is needed to declare the override (contract named the `.cpp`; one line + one static + a doc paragraph).
- `Inventory/WeaponManagerPickups.cpp`: `GatherInteractions` and, in the same file, `UpdateLootLabels` (a 10 Hz scan not in the audit's six) read the index.
- `Combat/EnemyProjectileSubsystem.cpp`: `GatherVictims` and `MakeWorldQuery` read the index.
- `Config/DefaultEngine.ini`: NOT touched (see gc below).

Contract mismatches to know: the contract table says `Looter.Perf.Fire <kind> <count> [seconds]`; the task and the contract's own recipe (`Looter.Perf.Fire LMG 10`) use `<kind> <seconds>`: built as `<kind> [seconds] [kill]` (default 10 s, 1-300).

## What each scan became

| Was (per frame unless noted) | Now |
|---|---|
| Minimap NativeTick: `TActorIterator` over `AAmmoPickup`, `AWeaponBase`, `ACreatureBase` + a `Cast<AStoneSentinelCreature>` per creature | `GetAmmoPickups/GetGuns/GetCreatures`; the sentinel is a flag set when the creature is indexed (`bSentinel`); `IsPickup/IsDead/IsHidden/IsDormant` still asked live |
| `LootLabelSubsystem::Tick`: `AWeaponBase`, `AShieldPickup` passes, no gate | index lists; `IsTickable` = a gun/ward label shows OR `Shown`/`Folded` still hold state (one last tick puts them back, then asleep) |
| `GatherInteractions` (from `UInteractionComponent::TickComponent`): `AWeaponBase` pass | `GetGuns()` |
| `UpdateLootLabels` (10 Hz): `AWeaponBase` pass | `GetGuns()` |
| `EnemyProjectileSubsystem` per tick while shots fly: `ACharacter`, `AVolume`, `APlayableArea` passes | `GetCharacters()` (non-creature characters: the players, friends), `GetVolumes()`, `GetPlayableAreas()` |

Passes: before up to 6 a frame (720/s at 120 fps) + 3 per enemy-shot tick + 10/s labels; after 8 per look x 10 looks/s = 80/s, whatever the frame rate. Reads cost a loop over a few dozen weak pointers.

How it stays right (`ActorIndexSubsystem.h` has the long version): the hook (`AddOnActorPreSpawnInitialization`, as `UGroundIgnoreSubsystem`) lists a spawned actor at once, so a gun dropped this frame is offered/labelled this frame; the timer's look rebuilds each list from the world (catches level-placed and streamed-in actors within 0.1 s, drops the destroyed). First read of a world that never began play does a look itself (test worlds). Readers must skip nulls and ask what changes between looks (IsPickup, IsDead, IsHidden) and not hold a list reference across a call that may spawn. The creature list is "living at the last look" (a creature that gets up again is listed at the next look).

Two calls that differ from the brief:
1. Interactables: not in the index. `UInteractionSubsystem` already is their registry (register/unregister, no scan); a second list would be a copy. (`UEncounterSubsystem::GetTrackedCreatures` is likewise an existing spawn-hooked creature list; the index keeps its own so the HUD does not hang on the encounter system and to carry the sentinel flag. Main may fold the two later.)
2. Enemy shots do NOT use `UGroundIgnoreSubsystem`'s list. That list ignores whole volume actors; the scatter's trees/rocks are components of the PCG volume's actor (memory note scatter-collision, Enemies.md 2026-10-28), so a shot would fly through every tree. The old code ignored only each volume's brush component (comment in the file), so the index keeps volumes and areas and the same ignore is built from it. Same behaviour, no scan. If Main wants one cache, `UGroundIgnoreSubsystem` needs an `AddIgnoredBrushesTo(Params)`; not my file.

## Switches / numbers
- `Looter.ActorIndex.Interval` [0.1] seconds between looks; read live (a change takes hold at the next look, clamped 0.02-2). Default lives once in `UActorIndexSubsystem::DefaultInterval`.
- No old-path switch: the old scans are deleted, not kept as a fallback (the audit counts parallel fallback paths as debt). The risk it removes is the per-frame cost, so there is nothing to A/B in game; A/B is by commit (below).
- `Looter.Perf.Fire` constants (dev-only, top of the file): wait for player/gun 30 s, for a horde 4 s (then fires at what is ahead), target within 60 m, ammo and invulnerability topped up every 0.5 s, gun is Common level 5.
- gc.*: left alone. No capture in `Docs/Performance.md` shows a GC spike (the 28-34 ms worst frames at MineDrift/Aerial_Dusk are still undiagnosed and nobody has tied them to the collector); a gc.* change without that is a guess, and the lane's rule is a stated reason. If Main wants one: check a worst-frame capture for a GC row first (`gc.TimeBetweenPurgingPendingKillObjects`, `gc.IncrementalReachabilityTimeLimit` are the candidates), then set it in `[SystemSettings]` and log the capture here.

## Perf recipe for Main (editor closed, `Saved\ArtPause.flag` set, PowerShell, end on Medium)
Frames: ~130 fps, so `-Frames 2400 -Skip 600` is a window of ~18 s that sits inside a 30 s fire.

1. Scan saving (A/B by commit; the horde in view is what loaded the minimap/labels): at the commit before the PERF commit, then at it:
   `Tools\perf.ps1 -Label "PERF before: 10 Basic spiders in town" -Map /Game/Maps/Lvl_RansomsRest -Frames 1500 -Skip 600 -Exec "Looter.Quality Medium,Looter.Perf.Horde Spider 10 Basic 0 -1400 90"` and the same with "PERF after". Expect Game ms down (the three HUD scans and the labels' two are gone; the horde makes the creature scan the biggest), p95 down a little.
2. Sustained fire, new rows (after only; no gunfire row existed): 
   `Tools\perf.ps1 -Label "PERF fire: LMG 30 s into 10 Basic spiders" -Map /Game/Maps/Lvl_RansomsRest -Frames 2400 -Skip 600 -Exec "Looter.Quality Medium,Looter.Perf.Horde Spider 10 Basic 0 -1400 90,Looter.Perf.Fire LMG 30"`; repeat with `Rifle`, `Shotgun`, `SMG`. Compare frame, p95 and Game with row 1: the gap is what firing costs. The log (`Saved\Logs\Perf.log` under `%LOCALAPPDATA%\UnrealEngine\5.8\Saved`) would end with `Looter.Perf.Fire: LMG, done: 30.0 s of fire, N bullets, M hits (...)`, but the capture's window ends inside the 30 s and `-ExitAfterCsvProfiling` leaves before the line prints (2026-11-04 rows): the proof of a fire row is `Cup fit, EWeaponKind::<gun>` at frame 2 (the gun given) and the game thread ~1 ms over the idle row.
3. The 8.3 ms line: the margin was 0.07 ms at the heaviest view; the fire rows are the first to say whether a firefight stays inside it. If a fire row is over, the stage costs are the next question (bullets, impacts, numbers, FIGHTS' gunfire hearing broadcast, MUZZLE's lights are all in it).
4. The tour is unchanged: `Tools\tour.ps1` needs no new run for this lane.

## Tests (run once, at commit)
- New: `Tools\runtests.ps1 -Filter Looter.World.ActorIndex` (Lists: spawn hook puts each kind in its list once, a look agrees and doubles nothing, sentinel flag; Gone: destroyed entries leave at the next look; Reads: 600 reads and a spawn make no look, the first read looks once; Rate: interval clamp, timer at the cvar's interval, a tuned value taken at the next look; Gate: the label sleep rule).
- Guards on code I changed: `-Filter Looter.UI.WorldCard` (gun spawned a moment before is offered by `GatherInteractions` through the index: the test that catches a stale index), `-Filter Looter.Bosses.Abel` (enemy shots through `GatherVictims`/`MakeWorldQuery`), `-Filter Looter.UI.Minimap`, `-Filter Looter.World.GroundIgnoreCache`.
- Not tested, by hand: the enemy shot passing a PCG volume's trees (a test level's traces do not see new bodies); the label sleep on a live subsystem (the subsystem is Game/PIE only).

## PIE check for the user
Nothing visible should change: minimap dots and diamonds, loot labels stacking apart at a chest, the gun card when you look at a gun, E/hold pickup all as before; shoot a dropped-ammo box open and its dot is on the map within a tenth of a second. Dredger/Abel pellets still stop on trees.

## Left alone (seen, not touched; other lanes' or throttled)
`Creatures/CreatureBaseSpacing.cpp` already uses `UEncounterSubsystem::GetTrackedCreatures` (its scan is the fallback); `Audio/MusicDirectorSenses.cpp` (4 Hz), `Combat/FightState.cpp` (throttled by `CombatLookSeconds`), `Embers/Powers/*`, `Elements/ElementStatusComponent.cpp`, `Creatures/CreaturePackComponent.cpp`, `Creatures/SpitterGlob.cpp` (volumes, areas, characters) still walk the world per use: candidates for index lists later; how often each runs was not checked. 188 `TActorIterator` uses outside tests/dev in all.
