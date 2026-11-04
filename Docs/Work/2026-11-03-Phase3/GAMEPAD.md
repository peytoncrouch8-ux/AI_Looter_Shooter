# GAMEPAD lane (change 7): a gamepad that works

Built unbuilt, untested, uncommitted (lane rules). Plan: `Docs/Work/2026-11-03-Audit/AUDIT.md` finding 6, plan row 7.

## What changed
- The right stick turns the view in degrees a second x the frame's real time (`FApp::GetDeltaTime`, not the world's, so hit-stop
  does not slow the pad and the mouse stays as fast as the pad), through a round dead zone and a power curve, x the Look
  sensitivity setting, x the zoomed sight's 1/AimZoom slowing (the mouse's existing rule).
- Invert Y (Settings > Controls, saved in `ULooterControlsSave`) flips Y for pad AND mouse.
- Aim assist, pad only: friction (look x 0.6 while the crosshair is within a creature's capsule + 20 cm, creature within 40 m)
  and magnetism (a pull of at most 2 deg/s toward the creature the crosshair is nearest, inside a 2 deg cone, only while the stick
  moves). Never a dead, passive, hidden (`IsHidden()`) or collision-less creature, nor one behind geometry (one
  `ECC_Visibility` line trace from the camera to its middle, pawn and what it holds ignored, at most two per frame, only
  while the stick moves). Settings > Controls "Aim assist" On/Off (default On).
- `DefaultInput.ini`: `bEnableMouseSmoothing=False`, `bEnableFOVScaling=False`.

## How the pad is told from the mouse (differs from the brief)
Not by `GunfeelKick::IsGamepadLastInput()` (the Slate watch). `SetupPlayerInputComponent` now binds `LookAction` (pad,
IMC_Default Gamepad_Right2D) to a lambda -> file-local `LookByStick`, and `MouseLookAction` (IMC_MouseLook Mouse2D) stays on
`Look()`. The action says which device turned the view; the watch only sees Slate events and, if it missed mouse moves in a
captured game viewport, would have treated the mouse as a stick (the dead zone and the time scaling would have wrecked mouse
look). `ALooterCharacter.h` is untouched (the lambda calls a free function that uses only public API).

## Legacy input scales (found while checking the maths)
`DefaultInput.ini` has `bEnableLegacyInputScales=True` and `BaseGame.ini` sets `InputYawScale=2.5`, `InputPitchScale=-2.5`:
every `AddController*Input` is multiplied by them. So the old pad turned 2.5 deg per frame at a full push (about 150 deg/s at
60 fps, 300 at 120). `AimAssist::LegacyInputScales` reads the controller's scales (1,1 with the setting off) and
`DegreesToInput` divides their size out, so 220 really is 220 degrees. The magnet's pull is a rotation (yaw right, pitch up) and
goes through `RotationToInput` (divided by the signed scales). The stick keeps the IMC's own signs (the IMC negates Y for the -2.5).
The mouse path is not touched by this.

## Files
Changed:
- `Core/LooterCharacter.cpp`: `LookByStick` (anonymous namespace), the pad binding, `Look()` (mouse) now with Invert Y.
- `Settings/ControlSettingsSubsystem.h/.cpp` (+ `ULooterControlsSave`): `bInvertY` (false), `bAimAssist` (true);
  `DefaultInvertY`, `DefaultAimAssist`, `InvertYOf/AimAssistOf(save)`, `IsInvertYFor/IsAimAssistFor(controller)` (false without a
  local player), `IsInvertY/SetInvertY`, `IsAimAssistOn/SetAimAssistOn`. Older saves load at the defaults (plain UPROPERTYs).
- `UI/Menus/SettingsMenuHelp.h/.cpp`: `ESettingRow::InvertY`, `ESettingRow::AimAssist` (after ControllerRumble) with their words;
  the Controls page's words mention aim assist.
- `UI/Menus/SettingsMenuPages.cpp`: two On/Off switch rows after Controller rumble, before the key list.
- `Config/DefaultInput.ini`: the two flags.
- `Tests/SettingsTests.cpp`: `Looter.Settings.GamepadControls`, `Looter.Settings.GamepadRows`.

New (CODEMAP text):
- `Player/AimAssist` (.h/.cpp): the gamepad's look as plain math (stick dead zone + power curve, degrees a second x frame time,
  Invert Y, the controller's legacy input scales divided out) and the aim assist (friction, magnetism, the creature list found at
  most 10x a second, one sight trace); `AimAssist::ForPlayer`; cvars `Looter.Look.Pad*`, `Looter.AimAssist*`.
- `Tests/AimAssistTests.cpp`: `Looter.AimAssist.*` (stick curve, turn at any fps, Invert Y and scales, friction, magnetism, refresh
  rate, tuning/cvars).
- Also for CODEMAP's existing lines: `Settings/ControlSettingsSubsystem` adds "Invert Y (mouse and stick), Aim assist (gamepad)";
  `UI/Menus/SettingsMenuWidget` Controls page now has two switches.

## Numbers, before -> after
| | before | after |
|---|---|---|
| Pad turn, full push | 2.5 deg/frame (150 deg/s at 60 fps, 300 at 120) | 220 deg/s round, 160 up/down, any fps (x sensitivity) |
| 360 degrees at a full push | 2.4 s at 60 fps, 1.2 s at 120 | 1.64 s at any rate |
| Stick response | linear, the IMC's own dead zone only | round dead zone 0.18, power 1.6 (half way past it = 33% speed) |
| Hitch frame | spun (per-frame input) | the step is capped at 0.1 s (22 deg) |
| Friction | none | look x 0.6 on a creature (capsule + 20 cm, <= 40 m) |
| Magnetism | none | <= 2 deg/s inside 2 deg, eased to 0 inside 0.25 deg |
| Invert Y | none | setting, default off, pad and mouse |
| Mouse smoothing / FOV scaling | True / True | False / False |

Mouse note: with FOV scaling off the engine no longer scales mouse deltas by the camera FOV (the player's 70-110 FOV setting,
sprint +6, ADS). The sight's slowing is now only the game's own 1/AimZoom (before, the engine's FOV ratio came on top). Expect
the ADS mouse to feel a little faster than before if it was double-slowed; there is no cvar for this, it is the ini.

## Cvars (read live; defaults in `AimAssist.h` `Default::`)
- `Looter.Look.PadDegreesPerSecond` [220] yaw; **0 = the old per-frame stick back** (the live switch for the whole pad change).
- `Looter.Look.PadPitchDegreesPerSecond` [160], `Looter.Look.PadDeadZone` [0.18], `Looter.Look.PadCurve` [1.6] (1 = linear).
- `Looter.AimAssist` [1] (0 = off for everyone), `.Friction` [0.6] (1 = none), `.Magnetism` [2 deg/s, 0 = none], `.Cone` [2 deg],
  `.Margin` [20 cm], `.Range` [4000 cm]. All held to sane ranges (a rate past 1000 or a friction of 0 cannot freeze or spin the view).
- Fixed in code (not cvars): the creature list refresh 0.1 s, the hitch cap 0.1 s, the magnet's 0.25 deg ease-out.

## Settings rows (Controls page: Look sensitivity, Camera shake, Controller rumble, **Invert Y, Aim assist**, Reset all keys, keys)
Both are On/Off switches. The lane brief's contract table also lists a "Pad sensitivity" row: not built (the Look sensitivity
slider already scales the stick; a second slider would need `UFUNCTION` handlers in `SettingsMenuWidget.h`).

## For Main: three things outside the lane's files
1. **`Tests/SettingsMenuTabsTests.cpp` lines 191-196 now fail** (not mine to edit): the Controls page has two more stops before
   Reset all keys. Change the loop to `Step < 5` and the message to "Five down: Reset all keys, after the page's sliders and
   switches". `Looter.Settings.GamepadRows` already covers the new order.
2. **The switches take their own click in `SettingsMenuPages.cpp`**, not in `HandleButton` (`SettingsMenuInput.cpp`),
   because `SettingsMenuWidget.h`, `SettingsMenuParts.h`, `SettingsMenuInput.cpp` and `SettingsMenuWidget.cpp` are outside the
   lane. `MakeArtButton` wires every button to `HandleButton`; the page rebinds `OnButtonClicked` of the four new buttons to a
   lambda (sets the control, saves, status line, lights the side, `StopListening`, `RefreshKeyLabels`, `SetKeyboardFocus`).
   Their action names (`InvertYOn/Off`, `AimAssistOn/Off`) are file-local. If you prefer the usual pattern (member buttons,
   `HandleButton` cases, a `RefreshControlSettings` line), it is a mechanical move later; behaviour is the same. The state is read
   when the page is built (settings only change in this menu).
3. **Stacked dead zones.** `IMC_Default` has an `InputModifierDeadZone` class in it (default thresholds 0.2 lower / 1 upper,
   radial, rescaled to 0-1) and the project's `Gamepad_RightX/Y` axis config says 0.25 (Enhanced Input does not apply that one).
   If the IMC's modifier is on the right stick, a push must pass ~0.2 raw and then 0.18 of what is left (about 0.34 raw) before the
   view moves. If the stick feels numb in the PIE check, set `Looter.Look.PadDeadZone 0.05` (a cvar; no asset edited here) and tell me
   the number to make the default.
- Also: `PERF`'s `ActorIndexSubsystem` will hold a creature list; `AimAssist.cpp` keeps its own (10 Hz `TActorIterator`, one
  file-level cache) so the lanes stay disjoint. Switch it to the index when that lands. Scene free look (`SceneSubsystemKeys.cpp`)
  does not invert Y (out of lane).

## Tests (run after the one build)
- `Tools\runtests.ps1 -Filter Looter.AimAssist` (7 tests: StickCurve, StickTurn, InvertY, Friction, Magnetism, Refresh, Tuning)
- `Tools\runtests.ps1 -Filter Looter.Settings` (adds GamepadControls: defaults, save round trip, row words; GamepadRows: the page's
  stops in order, the switch presses)
- `Tools\runtests.ps1 -Filter Looter.UI.SettingsMenu` once item 1 above is fixed (Help checks the two new rows' sentences).
Not covered by tests (needs a world with a pad and creatures): `AimAssist::ForPlayer`'s world part (list, capsule, sight trace).

## PIE check for the user
- On the pad, a full right-stick push turns 360 degrees in about 1.6 s, and the same at `t.MaxFPS 60` and `t.MaxFPS 120`; a small
  push aims finely (a third of the speed half way past the dead zone). If it feels numb at the start of the push, see item 3.
- Sweep the stick across a spider or an Unpaid 10 m away: the crosshair slows on its body and eases onto it if you let the stick
  drift (2 deg/s only); behind a wall or on a dead one nothing slows. `Looter.AimAssist 0` (or Settings > Controls > Aim assist
  Off) crosses at one speed; the mouse never changes.
- Settings > Controls > Invert Y On: stick up and mouse up both look down. `Looter.Look.PadDegreesPerSecond 0` is the old stick.
