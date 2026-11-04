# Lane CAMERA (audit change 6): the camera reacts to movement. Built 2026-11-03, NOT compiled, NOT run (lane rules: no build/editor/PIE)

Before: `HandleLanded` only pushed the gun's kick spring; `OnTraversalLanded` had no listener; the only camera motion from movement was
sprint FOV +6 deg and the slide's 5 deg roll; only the gun bobbed. Now a landing, a jump, each footfall, a strafe and a mantle/vault landing
move the view (and a landing lowers the eye). Master `Looter.Camera.Motion` [1]; 0 = the old dead camera.

## Files
New:
- `Player/PlayerCameraMotion.h/.cpp` (CODEMAP): `CameraMotion` namespace (`Default` numbers, `FTuning`, `Live()` = the cvars read live, the rules as plain
  functions), `FCameraMotion` (eye dips, lean ease, footfall finder over the gun bob's stride clock), `FCameraMotionPose`. Plain math, no world.
  One-line CODEMAP: `Player/PlayerCameraMotion`: `CameraMotion::` rules, `FCameraMotion`, `FCameraMotionPose`: the camera answering the body's movement as plain math (landing
  dip 0.4-1.6 deg + eye drop 1-4 cm by fall speed, nothing under 300 cm/s; jump kick 0.3 up; footfall nod 0.15-0.35 + sprint roll 0.2 found in the gun bob's own
  stride clock; strafe lean 0.6; mantle/vault landing dip); `Looter.Camera.Motion` [1] master, `Looter.Camera.Motion.*` every number.
- `Player/PlayerLocomotionCameraMotion.cpp` (CODEMAP): `UPlayerLocomotionComponent`'s half: `UpdateCameraMotion` (per frame: eye dip + strafe lean; binds
  `OnTraversalLanded` on its first run), `CameraMotionLanded/Jumped/Stepped`, `HandleTraversalLanded`. Kicks go to `UCameraShakeModifier::Kick`
  (local player only; scaled by the Camera shake setting there); the eye dip and lean are scaled by the same setting here. Add to the
  `PlayerLocomotionComponent` CODEMAP line's "Split:" list.
- `Tests/CameraMotionTests.cpp`: `Looter.Camera.Motion.*`.
Edited:
- `Player/PlayerLocomotionViewModel.cpp`: `HandleLanded` calls `CameraMotionLanded(FallSpeed)`; the jump branch of `UpdateViewModel` calls `CameraMotionJumped()`;
  `CameraMotionStepped(Dt)` right after the stride clock (`StepPhase`) moves; `UpdateCamera` asks `UpdateCameraMotion(Dt)`, lowers the rig by `Motion.EyeDrop` more and
  adds `Motion.Roll` to the slide's roll on the camera's additive offset (`AppliedViewTilt` = slide + lean; the gun turns with it as it did with the slide;
  `AppliedViewRoll`/`GetViewRoll()` stay the slide's alone). 455 lines.
- `Player/PlayerLocomotionComponent.h`: includes `PlayerCameraMotion.h`; `HandleTraversalLanded`, `UpdateCameraMotion`, `CameraMotionLanded/Jumped/Stepped`;
  members `AppliedViewTilt`, `ViewMotion` (`FCameraMotion`), `TraversalLandedHandle`; one doc bullet. Now 550 lines (was 529: already over, a class declaration
  can't split; Main may trim).
Not touched: `ViewKick.*`, `PlayerViewKicks.cpp`, `PlayerViewAim.cpp` (RECOIL), `CameraShakeModifier.*`, `PlayerLocomotionTraversal.cpp` (the broadcast
already carries kind and height; no fall speed needed), `PlayerLocomotionComponent.cpp` (not in the lane's list, so the `OnTraversalLanded` listener is bound lazily
in `UpdateCameraMotion`; Main may move that `AddUObject` to `BeginPlay` and drop `TraversalLandedHandle`).

## Numbers (before -> after; all cvars `Looter.Camera.Motion.<Name>`, read live)
| What | Before | After (default) | Cvars |
|---|---|---|---|
| Landing, view | gun spring only | dip (pitch down) 0.4 deg at 300 cm/s fall speed .. 1.6 deg at 1000+, linear; nothing under 300 (= `UPlayerFootfallComponent::SoftLandSpeed`, a curb's hop); spring 4.5 Hz, damping 0.55 (down in ~42 ms, settled ~0.34 s, comes back up past level by ~13%) | `LandMinSpeed` 300, `LandFullSpeed` 1000, `LandPitchMin` 0.4, `LandPitchMax` 1.6, `LandHertz` 4.5, `LandDamping` 0.55 |
| Landing, eye | none | rig (arms, gun, camera) lowered 1 cm .. 4 cm by the same scale; curve u^2 e^(2(1-u)): starts at rest, deepest at 60 ms, gone in ~0.48 s, never lifts above rest; added on top of the eye's own curve (the rest height is still measured with the dip taken off) | `LandDropMin` 1, `LandDropMax` 4, `LandDropSeconds` 0.06 |
| Jump | gun lag only | view kicks up 0.3 deg, 6 Hz, damping 0.55 | `JumpPitch` 0.3, `JumpHertz` 6, `JumpDamping` 0.55 |
| Footfall | none | one nod per step down 0.15 deg walking .. 0.35 sprinting (x the view model's `MoveWeight` held to 1), 5.5 Hz, damping 0.6 (settles 0.26 s, steps are 0.3-0.35 s apart) | `StepBobWalk` 0.15, `StepBobSprint` 0.35, `StepHertz` 5.5, `StepDamping` 0.6 |
| Sprint sway | none | the same kick rolls 0.2 deg x SprintAlpha toward the stepping foot's side (+ right) | `SprintSway` 0.2 |
| Strafe | none | held lean 0.6 deg at walk speed lateral (into the strafe, + right; negative cvar leans out), eased at 8/s, off in a slide and in the air | `StrafeRoll` 0.6, `StrafeRate` 8 |
| Mantle / vault landing | gun spring only | view dips 0.5 / 0.8 deg (the eye already gives through a move) | `MantleDip` 0.5, `VaultDip` 0.8 |
| Through the sights | n/a | all of it x 0.25 (the gun bob keeps 15%): the reticle stays on its target; no pivot, so a landing mid-aim is up to 0.4 deg off for ~0.1 s | `AimedShare` 0.25 |
Footfalls use the gun bob's own clock: `StepPhase` (one pi per step, the gun at its lowest dip pi/2 into each step, swaying right while sin > 0). The kick is given
`PeakSeconds` (spring peak, ~37 ms) early by the clock's current pace, so the nod's peak lands on the gun's dip; side = the gun's sway side at that moment, so the
camera and gun roll the same way. All of it x the Camera shake setting (setting 0 = nothing; `Looter.CameraShake [0-1]`). Local player controller only: a
character with no local controller (the test worlds) moves nothing, so `Looter.Locomotion.*` view-pop tests are unaffected.

## Tests
`Tools\runtests.ps1 -Filter Looter.Camera.Motion` (9: Variables, Shipped, Landing, EyeDip, StepKick, StridePhase, Kicks, Lean, Master): the cvars exist and are read live,
the shipped numbers are the audit's, nothing under a step / 0.4-1.6 deg and 1-4 cm / monotonic / the kick's spring (dip reached in <= 80 ms, back-swing <= 20%,
settled <= 0.5 s), the eye dip never snaps (first frame < half, never negative, gone by 0.5 s, pile held at 6 cm), the footfall nod peaks within a frame of the gun's
lowest dip and on its sway side at walk/sprint, 60/120/144 fps, wrapped clocks, one per step, feet alternate, none when standing or after a clock jump,
strafe 0.6 and signs, aimed share, the lean ease, master 0 = every rule empty. Also worth one run after the build: `-Filter Looter.Locomotion` (`UpdateCamera` changed;
the tests have no controller so the motion is inert there).

## PIE check for the user
Sprint and jump off the porch: the view dips on the landing (harder from higher) and the arms/gun drop a few cm for a moment; sprinting, the view nods and rolls a touch at each
stride, in step with the gun's bob; strafing leans the view into the move; a jump kicks it up a hair. `Looter.Camera.Motion 0` to compare with the old camera; tune with
`Looter.Camera.Motion.LandPitchMax 3` etc.; the Camera shake setting scales or removes it.

## Notes for Main
- Not compiled. Likely first-build friction: none known; names checked for shadowing (`LandingSpeed` not `FallSpeed`), no unity build.
- Lessons (pipeline Game/Movement): a view kick spring snaps to its peak in 1-2 frames, which is right for a thump but would have been a one-frame 4 cm teleport as the eye's dip,
  so the eye has its own at-rest-start curve; held lean can't be a kick, so it rides the camera's additive offset with the slide's roll; camera motion is gated on a local
  player controller so bare-character test worlds stay unchanged.
- If the landing feels too much while the gun's own `LandKick` spring also sinks: lower `LandDropMax`, or set `LandHertz` 3.
