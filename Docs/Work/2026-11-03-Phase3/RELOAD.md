# Lane RELOAD (audit change 12): spring slaps, the seat's jolt, the thrown magazine

Staged in `Saved/Phase3Staging/RELOAD/Source/AI_Looter_Shooter/` (same relative paths; copy over Source, build, run the tests below).
No .uasset, CODEMAP, Pipeline, ini or Blender changes; no new sound cue.

## Files
Changed: `Weapons/ReloadMotion.h`, `ReloadMotion.cpp`, `ReloadMotionPlan.cpp`, `ReloadTuning.h`, `ReloadTuning.cpp`,
`WeaponBaseReload.cpp`, `Player/FirstPersonHandsReload.cpp`, `Tests/FirstPersonHandsTests.cpp` (.Reloads runs with Snap 0 and 1).
Not touched (nothing needed): `ReloadBeats.*`, `WeaponBasePump.cpp`, `FirstPersonHandsMotion.h`, `Tests/GunfeelReloadTests.cpp` (532 lines already).

New, with CODEMAP text:
- `Weapons/ReloadMotionSnap.cpp` (under ReloadMotion.h): `ReloadCurves` (the reload's spring curves as plain math: `Snap` a move with
  wind-up, overshoot and a critically damped settle; `SnapInto` into a hard stop at speed; `SnapBeat` a knock out and back; `Jolt` the gun
  recoil's spring knocked from rest; the progress forms `Rise`/`Window`/`Into`/`Beat` the choreography and the hands call, the eased
  curves with `Looter.Gunfeel.Reload.Snap 0`) and `LooterReload::JoltPose` (the first-person gun's knock at each seat and slam).
- `Weapons/WeaponMagazineDrop`: `UWeaponMagazineDrop` (tickable world subsystem): old magazines thrown by a reload, pooled 8, one
  instanced mesh per mesh and paint, arc traced ahead in chords, one bounce (Loot.Land thump + Weapon.Casing.<surface> rattle at pitch
  0.62), 20 s rest, 1.2 s sink; ticks only while one is out. `FMagazineDropLaunch`.
- `Weapons/WeaponMagazineDropRules.cpp` (under WeaponMagazineDrop.h): `MagazineDrop` rules (`Drops`, `DropAt`, `Hides`, `Arc`, `Bounce`,
  `PickSlot`, `SinkShare`, `RestRotation`, `RestLift`, `FindReloadPart`, `Throws`, `LaunchFrom`: the part's mesh, paint and wear, where it
  shows in first person, its slide speed, the holder's run, a flick off the gun's left, a tumble).
- `Tests/ReloadSnapTests.cpp`: `Looter.Gunfeel.Reload.Snap.Curves`, `.Switch`, `.Jolt`, `.Hands`.
- `Tests/ReloadDropTests.cpp`: `Looter.Gunfeel.Reload.DropMag.Rules`, `.Pool`, `.Gun`.

CODEMAP edits for Main: ReloadMotion's line gains "spring curves `ReloadCurves` and `JoltPose` in ReloadMotionSnap.cpp; `JoltsFor`,
`RawProgressOf` in ReloadMotionPlan.cpp; `MagazineClearAt` 0.28, `FreshMagazineAt` 0.46, `BoltHomeAt` 0.88"; ReloadTuning's gains the
`Snap*`/`DropMag*` cvars (`FSnapShape`, `FJoltRules`); WeaponBaseReload's gains "throws the old magazine, the seat's jolt
(ApplyCyclePose)"; tests list gains the two new files.

## What changed (numbers before -> after)
Shape (all `Looter.Gunfeel.Reload.Snap.*`): Anticipate 0.25 (the move goes 25% the other way first), Overshoot 0.12, Lead 0.2 (the wind-up's
share of the window), Settle 0.4 (the critically damped tail's share). Total reload times, ready times, steps (sounds) and commit points:
unchanged (no ReloadTuning time changed; `Snap.Switch` checks the steps are identical with the switch either way).
- The gun's knocks (`ReloadCurves::Beat`; rifle/SMG/LMG mag out and seat, sniper box seat, bolt yank and home, rocket seat, revolver flick,
  punch, rounds in, cylinder slapped home, every shell, the old four-shell pump): a symmetric SmoothStep bump peaking at the step ->
  a spring knock whose hit is the step: wound up -25% over the 20% of its window before the hit, kicked to the peak, rebounding to -12%,
  settled by the window's end (window width unchanged, moved so the hit lands on the step). Tactical rifle (1.89 s) seat: wind-up 34 ms
  before the seat, peak +28 ms, rebound -12% at +69 ms, rest at +138 ms. Sniper yank/home knocks now on the bolt's stops (0.16 / 0.88; were 0.13 / 0.84).
- Worked parts and the hand's short moves (`Rise`/`Window`): rifle charging handle, SMG knob, LMG belt lay (the gun's offset and the
  hand's yank): SmoothStep -> Snap (hand cocks forward 25% of the 4 cm pull, yanked to 112%, settles; let run home past rest by 12%).
- Into a stop (`Into`/`SnapInto`): fresh magazine/helix/box `(1-T)^2` easing in -> cocked down 25% (26 -> 32.5 cm under the well) then
  rammed home accelerating (`v^2`), hitting the seat at ~3x the move's average speed; rocket `(1-T)^2` -> `v^2` (no wind-up, so never
  past its reach); pump (reload and rack), sniper bolt, shell thumbed into the port: SmoothStep -> `v^2` into each stop, no wind-up.
- Rack between shots: fore-end home at 0.95 of the rack -> 0.80 (snapped), its shove knock from the home over 0.76-0.96 (was a bump
  0.86-0.98). Back point (hull, rack beat) still 0.45. Reload pump from empty gets the same shove knock at its home.
- The seat's and slam's jolt (new, `LooterReload::JoltPose` through `ApplyCyclePose`, first person only): seats (MagIn, HelixIn, BoxIn,
  RocketIn, CylinderIn) 1.8 cm, slams (handle home at HandleTo 0.82 from empty, LMG belt cover, sniper bolt home 0.88, pump home from empty)
  2.2 cm, back 0.8 / down 0.6, muzzle dip 0.6 deg per cm; spring 9 Hz x the gun's recoil Snappiness, damping 0.55 (FWeaponRecoil's): peak
  at 21 ms, rebound 12.6% at 87 ms, under 5% from ~123 ms, cut at 175 ms. The hands ride the model, so the whole rig moves.
- The old magazine (rifle magazine, SMG helix, LMG box, and the sniper's box too): went invisible / fell 60 cm along the well out of view
  -> thrown at `MagazineClearAt` 0.28 (sniper 0.34) as a falling copy of the part (its mesh, paint copied, wear CPD), its own part hidden
  from that very frame until the fresh one shows at 0.46; arc with gravity from its slide speed (~90 cm/s at 1.9 s) + the holder's run +
  25-55 cm/s off the gun's left, tumbling 3-6 rad/s; one bounce (restitution 0.28 hard / 0.1 soft), lies on its flattest side, 20 s, sinks
  1.2 s. First person: it starts where the part shows at its drawn size and grows to its own (/ FPHandsFraming::GunScale) over 0.25 s.
  The support hand lets go at 0.28 (was 0.30). Pool 8 shared by every holder (an AI's gun too); past it the oldest goes.

## Switches
- `Looter.Gunfeel.Reload.Snap` [1]: 0 = every curve exactly the SmoothStep before and no jolt.
- `.Snap.Anticipate` [0.25], `.Snap.Overshoot` [0.12], `.Snap.Lead` [0.2], `.Snap.Settle` [0.4] (clamped to shapes that start/end at rest).
- `.Snap.Jolt` [1] (scale, 0 none), `.Snap.JoltSeat` [1.8 cm], `.Snap.JoltSlam` [2.2 cm], `.Snap.JoltPitch` [0.6 deg/cm], `.Snap.JoltHz` [9],
  `.Snap.JoltDamping` [0.55].
- `Looter.Gunfeel.Reload.DropMag` [1] (0 = the part vanishes and falls out of view as before, hand lets go at 0.30), `.DropMag.Rest` [20 s].
- Snap 0 + DropMag 0 = the reload before this lane. Defaults in `ReloadTuning::Default`.

## Tests (editor open)
`Tools\runtests.ps1 -Filter Looter.Gunfeel.Reload` (the new Snap.* and DropMag.* plus Plan/Steps/Commit/Interrupt/Rack/Guns/Beats, which
must still pass with the snap on), `-Filter Looter.Player.Hands.Reloads`. Regression worth one run (they read the curves I changed):
`Looter.Weapons.ReloadMotion`, `Looter.Weapons.NewGuns.Reload`, `Looter.Weapons.Revolver.Reload`, `Looter.PlayerAnim.RevolverReload`,
`Looter.Weapons.FX.RevolverEmpties`.

## PIE check for the user
Reload the rifle half empty: the hand cocks the fresh magazine down then slaps it home with a visible overshoot, the gun jolts as it
seats, and the old magazine tumbles to the ground and lies there 20 s; empty, the handle snaps back past its pull and the gun jolts as it
slams. `Looter.Gunfeel.Reload.Snap 0` and `Looter.Gunfeel.Reload.DropMag 0` to compare.

## For Main
- FWeaponRecoil lives private in `UPlayerViewComponent` (RECOIL's `PlayerViewAim.cpp`) with no hook, and `AddShot` would also kick the aim,
  so the jolt is the same spring in closed form applied through `AWeaponBase::ApplyCyclePose` (the bolt's and the pump's own path; free
  while a reload holds the gun). First person only: the third-person body doesn't jolt. If wanted later: a
  `UPlayerViewComponent::AddGunJolt(Back, Pitch)` -> an `FWeaponRecoil` velocity-only impulse (no aim kick), and drop JoltPose.
- `WeaponBaseReload.cpp` resets the cycle pose in FinishReload/CancelReload when a jolt is still ringing; Reload() already clears it.
- Landing sounds reuse `Loot.Land` (0.8 vol, pitch 1.15) + `Weapon.Casing.<surface>` (pitch 0.62); a dedicated magazine-drop recipe
  would be a sound-lane job (cues.json is off limits here).
- Not verified: no build or PIE here. Watch in PIE: the thrown magazine's first frame in first person (size/place hand-off), and that the
  hand never shows a gap at the let-go.
