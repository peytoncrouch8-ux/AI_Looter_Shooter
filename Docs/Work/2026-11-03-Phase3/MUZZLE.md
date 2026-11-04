# MUZZLE (Phase 3 lane, change 8): the gun lights the hands and the ground

Not built, not run (rules: no build/editor/PIE/commit). Code written against the files read; Main builds once and runs the tests below.

## Files
Edited (all in the lane's list):
- `Weapons/WeaponBase.cpp` ctor: `MuzzleLight` Movable (each shot moves it), radius 320 -> 500 cm, indirect lighting 0 (Lumen would pay for a 40 ms flash). Still unshadowed.
- `Weapons/WeaponBaseMuzzle.cpp` (332 lines): `FlashLightCandelas` 24 deleted; `HangShotLight` (per shot: kind's size, `SetWorldLocation` at `GetVisibleMuzzleLocation()` + `LightPush` along the barrel, radius) and `FadeShotLight` (per tick: kind's peak x flash fade over the light's own life x `FlashStrength`, hidden when spent) as anonymous-namespace functions (WeaponBase.h not in the lane: no new members).
- `Weapons/VfxTuning.h/.cpp`: new fields/cvars below; `MuzzleAim` 0.55 -> 0.8; `Looter.Vfx.Muzzle.Light` changed meaning (was a x1 multiplier of 24 cd, now candelas, 400).
- `Weapons/WeaponFXImpacts.cpp` (500 lines): one call in `SpawnSurfaceImpact`, `UImpactFlashSubsystem::Flash(Owner.Get(), Vfx, Point, Up, Material, Weight, bHasCamera ? &LastCamera : nullptr)`, after `Weight` is known.
- `Weapons/VfxRulesMuzzle.cpp`: not touched (the new rules live in the new file below: `VfxRules.h` is not in the lane, so the new header declares into `namespace VfxRules`).

New (CODEMAP one-liners):
- `Weapons/VfxLightRules.h/.cpp`: the gun lights the hands and the ground: plain rules for the shot light (per kind, life, place) and the impact flash (surface, eye rule, heft), and `FImpactFlashPool`, the four-slot bookkeeping.
- `Weapons/ImpactFlashSubsystem.h/.cpp`: `UImpactFlashSubsystem`, four pooled unshadowed point lights on one transient actor for the flash a round throws on stone, metal and wood; ticks only while one is lit.
- `Tests/VfxLightTests.cpp`: the shot light's and impact flash's rules, the pool's cap, the eye rule (`Looter.Vfx.Light.*`).

Deviations from the brief (all because of the lane's file list): the pool is a new world subsystem, not a member of `UBulletSubsystem`/`FWeaponFX` (their headers are not in the lane; same build as `UUnpaidPackSubsystem`'s three lights); the tests are a new file because `Tests/VfxTests.cpp` is at 499 lines (group `Looter.Vfx.Light`, so `-Filter Looter.Vfx` runs them with the old ones).

## Numbers (before -> after)
Shot light (rifle-sized; `Muzzle.Light` cd x kind share; radius `Muzzle.LightRadius` x sqrt(share); so every kind's glow edge is alike):
| | before | after |
|---|---|---|
| peak | 24 cd x `MuzzleFlashScale` (0.85..2.4 by definition) | 400 cd x kind share; `MuzzleFlashScale` no longer multiplies it |
| reach | 320 cm | 500 cm rifle; Shotgun 612, Revolver 559, Sniper 570, LMG 524, Launcher 632, SMG 387 |
| kind share | none | Rifle 1, Shotgun 1.5 (600 cd), Revolver 1.25 (500), Sniper 1.3 (520), LMG 1.1 (440), Launcher 1.6 (640), SMG 0.6 (240) |
| life | the flash's (30-60 ms by kind) | `LightSeconds` 40 ms, never longer than the flash (the gun ticks only while the flash shows): all kinds 40 ms but the SMG's 30 |
| place | gun-local +10 cm along X, real muzzle | each shot: where the muzzle SHOWS (first person: the render-space point) + 10 cm along the barrel; world cm, whatever the gun's scale |
| aimed | flash x0.6 | same x0.6 (it is in `FlashStrength`): 240 cd |
| shadows | off | off; indirect 0 |
Aimed flash size 0.55 -> 0.8 (`Looter.Vfx.Muzzle.Aim`); AimGlow stays 0.6; `MuzzleAimClear` 2 deg stays, so the clearance still shrinks it off the crosshair.
Impact flash (none before): 150 cd for a rifle round on stone/metal x surface (Stone 1, Metal 1, SheetMetal 0.8, Wood 0.55; everything else 0: dirt, grass, sand, gravel, water, cloth, hay, glass, bone, unknown) x heft (hit weight / 1.1, clamped 0.5..1.5: sniper 225, revolver ~204, rifle 150, pellet ~104, SMG ~98) x eye share; 300 cm reach; 60 ms, fade (1 - t)^2; hung 30 cm off the surface along its normal; colour warm gold (stone/metal) or orange (wood). Eye rule: judged at the light's place: 0 at <= 100 cm, easing to full at 200 cm; no light past 4000 cm; no eye known yet = no limit. Pool 4: a hit within 150 cm of a live flash takes that light over (restarts full) unless that is still brighter; all lit: a brighter hit takes the weakest, an equal or dimmer one is dropped. Not shadowed, indirect 0, no SpawnActor per hit (the actor and lights are made once, at the first flash in a played world).

## Cvars (all read live)
- Muzzle: `Looter.Vfx.Muzzle.Light` [400 cd, 0 off], `.LightRadius` [500 cm], `.LightSeconds` [0.04], `.LightPush` [10 cm], `.Light1P` [1: the player's own view's share; first lever if the hands wash white], `.Light.Rifle` [1] `.Shotgun` [1.5] `.Revolver` [1.25] `.SMG` [0.6] `.Sniper` [1.3] `.LMG` [1.1] `.Launcher` [1.6], `.Aim` [0.8].
- Impact: `Looter.Vfx.Impact.Light` [150 cd, 0 off], `.LightRadius` [300], `.LightSeconds` [0.06], `.LightLift` [30 cm], `.LightEye` [100 cm, 0 no rule], `.LightFar` [4000, 0 no limit], `.LightMax` [4, 0..4], `.LightMerge` [150 cm].
- Constants in `VfxLightRules.h` (shaping, not feel): `ImpactLightWeightRef` 1.1, heft 0.5..1.5, fade power 2; surface shares and colours in `VfxLightRules.cpp`.

## Tests
`Tools\runtests.ps1 -Filter Looter.Vfx.Light` (5: `.Muzzle` per-kind numbers/switches/life/place/aimed 0.8, `.MuzzleComponent` the gun's CDO light: unshadowed, movable, candelas, 500 cm, hidden, `.Impact` surfaces/eye rule/far/heft/switches, `.ImpactPool` the cap of 4, Max, merge, frame guard, fade curve, 500-hit stream, `.ImpactWorld` a test world lights nothing and sleeps). Then `-Filter Looter.Vfx` (old muzzle/effects tests, untouched) and `-Filter Looter.Weapons.FX` (SpawnSurfaceImpact callers) once.

## Perf check for Main (the real risk)
Margin at the heaviest view is 0.07 ms (8.26 vs 8.33). A first-person shot light sits ~35 cm from the eye with a 5 m reach: for its 40 ms (~43% of the time at 650 rpm) it is a near full-screen unshadowed deferred light. Expect a few tenths of a ms GPU at worst, not proven. A/B, editor closed, same row twice (the PERF lane's `Looter.Perf.Fire` after it lands):
`Tools\perf.ps1 -Label "fire, lights ON" -Exec "Looter.Quality Medium,Looter.Perf.Horde Spider 10 Basic 0 -1400 90,Looter.Perf.Fire LMG 10"` then the same with `Looter.Vfx.Muzzle.Light 0,Looter.Vfx.Impact.Light 0,` prepended. Over budget: `Looter.Vfx.Muzzle.LightRadius 350`, `Looter.Vfx.Impact.LightMax 2`, `Looter.Vfx.Muzzle.Light.SMG 0.4`/`.LMG 0.8` (the fast guns), then `Looter.Vfx.Muzzle.Light 250`. Lights make no draw calls and no shadow work; the cost is lighting pixels only.

## PIE check for the user
Fire the shotgun at dusk by the farmhouse: the hands, the gun and the porch light up warm for a frame or two. Shoot a stone wall and a tin roof from 3+ m: a flash on the rock/tin for a beat; dirt and grass don't flash. Hold the LMG into a wall: it pulses, a few lights at most, never a white screen. Aim down sights: the flash is a bit bigger and still off the crosshair.

## Watch (Main reads it, the user judges it)
- 10-30 lesson (Enemies.md, the Unpaid shriek lights): 70/150 cd 35 cm out washed a pale body flat white. The first-person shot light is 400 cd ~35 cm from the eye: if the hands go flat white or the screen strobes at 650 rpm (11 Hz), lower `Looter.Vfx.Muzzle.Light1P` (0.4) first, then `Looter.Vfx.Muzzle.Light` (250); aimed is already x0.6.
- Non-gun callers of `SpawnSurfaceImpact` flash too (a rocket's ground burst, the Gravemother's rubble on stone); the pool's 4 and the merge hold them. Say if rubble flashing reads wrong.
- Not verified here: that Movable is what `UPointLightComponent` defaults to (set explicitly now), and how first-person primitives take the light (the light is hung at the render-space muzzle so the hands and the near ground are lit from where the gun shows).
