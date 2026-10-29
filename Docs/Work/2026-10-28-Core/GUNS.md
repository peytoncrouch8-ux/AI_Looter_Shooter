# GUNS: every gun kind feels its own (planner, 2026-10-28 late)

The user's complaint (Handoff "Direction", 2026-10-21): "guns feel alike" (gun sound and kick; not hit confirmation).
Their pick tonight: "Guns feel different". Their answers (CONTRACT.md): overnight build on Main's calls (log each in Handoff
"Decided unattended"); gun sounds script-made now, a royalty-free pack shown in the morning before any download.
Enemy hit reactions and impacts are HITS.md's (impact weight by kind exists: Gunplay.md "Gun feel pass"). Read with
`Docs/Pipeline/Gunplay.md`. Every new number is a live cvar `Looter.Gunfeel.*`; defaults in brackets.

## Kinds in the game (7; no pistol besides the revolver)
Rifle = Bullpup AR (DA_AssaultRifle), Shotgun = Ranchhand pump (DA_Shotgun), Revolver = Drover (52 dmg, 140 rpm, 6),
SMG = Rustler (11/900/40-60), Sniper = Longhorn bolt (150/45/5-8), LMG = Thresher (16/650/80-150, spin-up, heat),
Launcher = Powderkeg (320 blast/40/1-3). Rifle and shotgun stats/recoil live only in their assets: step 0 dumps them.

## Why they still feel alike (read from the code)
1. **Heavy guns hit the same caps.** Hip view kick = `ViewKicks::ForShot` base x `Kick.<Kind>` x `ShotRecoil` (stat x
   `Recoil.More` 1.4 x calm), clamped by `FViewKickStack::MaxTurn` 3 deg / `MaxFieldOfView` 0.05. Today at a base gun:
   shotgun 1.15x1.9x1.4 = 3.06 deg, sniper 3.64, launcher 3.92: all three clip to 3.0 deg; FOV 0.058/0.056/0.084 all clip
   to 5%. Same peak, only the spring speed differs. Also `ForShot` clamps its RecoilStat to 0.6-1.5, so a base gun (1.4)
   sits at the clamp and the accuracy stat's "wild" end (up to 1.96) is cut off.
2. **The gun in hand weighs the same for every kind.** `PlayerLocomotionViewModel.cpp` look lag (TurnScale 0.012, clamp
   4/3 deg, interp 9), walk bob (depth 0.5-1.3, side 0.7-2, roll 0.6-3), idle breath (0.15 cm / 0.25 deg at 1.6 rad/s),
   jump lag (-18) and landing kick (fall/12) have no kind in them: an LMG swings like an SMG.
3. **Automatics share one sustain cap** (`Kick.SustainCap` 1.2 deg) and one aimed view share (`ForShot` halves the turn,
   0.6 the FOV, for all). Aimed model shares (`Recoil.AimedFlip/Roll/Back` 0.45/0.4/1.2) are one set for all kinds.
4. **ADS is one motion.** Only its time differs (`Aim.Time.<Kind>`); the curve (`GetAimAlpha`: `InterpEaseInOut` exp 2),
   the arrival (nothing happens when the sights land) and the sound (one `Weapon.AimIn` at -20.5 dB) are shared.
5. **Single-shot kinds have no shape.** Revolver/shotgun/sniper/launcher kick a random roll/yaw and recover at their
   asset's `AimRecovery`; nothing says "six-gun" vs "bolt gun" in how the kick comes back.
6. **Sound layers are scaled the same for every kind** (`Sound.Mech/Low/Tail` 1/1/1 global); no body "punch" layer;
   no loudness ladder (cues.json Core: SMG -1.5 dB is the loudest Core volume in the game; rifle -3.5, sniper -3.0).
7. **Brass:** SMG, sniper and LMG throw the rifle's case at the rifle's speed (`FWeaponBrass::KindFor`; `SizeOf` 4.5x0.95).
8. **Reloads have no felt beats:** only the shotgun's rack nudges the view (`GunfeelKick::RackBeat`); mag seats, bolt
   slams and cylinder snaps are sound only.
9. **One crosshair shape** (`PlayerHUDWidgetLayout.cpp` `MakeTicks` 9/4 px; kick 1.45x over 0.16 s for every gun).
Already distinct and kept: muzzle looks (`VfxRules::MuzzleLook`), tracers (`TracerLook`), impacts by weight, reload
rhythms (tactical/empty, shell-by-shell, rack, bolt cycle), LMG spin/heat, SMG helix glow, sniper breath/scope, maker
sound flavour, rumble per kind, aim/swap times per kind, recorded shot layers for all seven kinds.

## Master switch
`Looter.Gunfeel.Identity 1|0` [1]: 0 returns every new rule below to today's numbers (A/B for the Friday playtest and
for DEMO). Each lane reads it through `GunfeelKick::IdentityOn()` (Lane A defines it).

## Per kind: identity and changes (hip numbers = peak at a base Common, ShotRecoil 1.4)
View table rule (Lane A): `ForShot`'s table holds these final targets / 1.4 and `Kick.<Kind>` defaults all become 1.0
(one table to read); the RecoilStat clamp becomes 0.4-2.0 (the accuracy calm reaches both ends). New `FViewKick` fields:
`Push` (cm, camera back along the view; stack max `MaxPush` 4) and `Delay` (s; the spring starts late). Heft weight W
(Lane B), aimed shares back/flip/roll, view aimed turn share, recovery x (on the asset's `AimRecovery`).

**SMG (Rustler): "a hornet in the hand: a light buzz, quick to the eye, hose it and the cone opens, not the climb."**
- View: 0.2 deg, FOV +0.3%, 18 Hz, damp 0.65, roll +-0.12 random. Sustain cap 0.5. Aimed turn 0.6.
- Aim kick: climb 0.03 kept; `Recoil.SMGShimmy` 0.03 -> 0.05; `Recoil.SideRandom.SMG` 0.8 (rifle/LMG keep 0.4); bloom
  `Spread.SMGBloom` 0.09 -> 0.12; recovery x1.2. Aimed shares 1.0/0.35/0.3.
- Cadence jitter +-5% (`Cadence.SMG` 0.05). ADS curve 1.6, settle x0.5. Heft W 0.7. Shake none.
- Sound: Mech x1.2, Low x0.75, Tail x0.85, no Punch; ladder -3 LU under the rifle. Aim-in: light polymer tick.
- Brass: new `ECasingKind::Small` 2.0x0.95 cm, 300-420 cm/s, spin 30-48, landing pitch 1.25. Crosshair: short X ticks.
- Reload beats: HelixIn -0.08 deg roll 0.1, rumble 0.15/0.04 s; Charge (Bolt step) +0.1, 0.2/0.04.

**Rifle (Bullpup): "the workhorse: a crisp crack, a climb you can learn and hold, tight in bursts."**
- View: 0.55 deg, FOV +0.6%, 14 Hz, damp 0.6. Sustain cap 1.0. Aimed turn 0.5. Aimed shares 1.2/0.45/0.4 (today).
- Pattern: straight up 6 shots, right `RifleDrift` 0.1/shot to shot 15, then back left `Recoil.RifleDriftBack` -0.06/shot
  from `RifleDriftBackFrom` 16 (an S you learn). Climb 0.06 to 1.8x kept. Recovery x1.0.
- Cadence +-2.5%. ADS curve 2.0, settle x1.0. Heft W 1.0. Shake none.
- Sound: 1/1/1; Punch every 3rd shot at -4 dB; ladder 0 (reference). Aim-in: cloth + rail tap.
- Brass unchanged. Crosshair: today's 4 ticks. Beats: MagIn -0.12 deg FOV -0.3% 0.2/0.05; handle slap (empty) +0.18
  roll 0.2 0.3/0.06.

**LMG (Thresher): "a mill wheel: slow to wake, digs in and steadies as it spins, rolls the shoulders, never stops."**
- View: 0.6 deg, FOV +0.8%, 10 Hz, damp 0.55, roll 0.3 following the sway's side. Sustain cap 1.6. Aimed turn 0.55.
- New dig-in: every kick (aim, view, model) x lerp(`LMG.DigInStart` 1.25 -> `LMG.DigInEnd` 0.75) over spin 0.6 -> 1
  (`GetSpinAt`), so the first rounds bully you and a held belt settles. Sway pattern 0.15 kept. Recovery x0.9.
  Aimed shares 1.0/0.4/0.35.
- Cadence +-3% once spun up. ADS curve 2.6, settle x1.6. Heft W 1.6.
- Shake: `Shake.LMG` 0.035 for 0.15 s a shot, held under `Shake.LMGMax` 0.12 (reads `UCameraShakeModifier` strength).
- Sound: Mech x1.1, Low x1.2, Tail x1.0; Punch every 2nd; ladder +1. Aim-in: heavy shoulder thump + belt jingle.
- Brass: new `ECasingKind::Belt` 5.1x1.05 thrown down-right 160-240 + a link per round (engine Cube 1.2x0.9x0.25 cm,
  dark brass, own ISM, last 40), landing pitch 0.95. Crosshair: thick corner brackets, tinted toward `Color::Accent` from
  heat 0.6. Beats: BoxOut -0.1; BoxIn -0.25 FOV -0.5% 0.45/0.08; BeltLay -0.06.

**Revolver (Drover): "the gunslinger's iron: a hard snap that rolls the wrist and drops back on target before the next pull."**
- View: 1.9 deg, FOV +1.2%, 11 Hz, damp 0.7, roll +0.6 always clockwise (the wrist; not random), yaw +0.1 right.
- Recovery x1.25 (9 -> 11.25): the sight is back within 0.35 s (the pull is 0.43 s). Aimed shares 1.1/0.7/0.6 (the flip
  is its character through the irons). Aimed turn 0.55.
- No cadence change (the pull's rhythm is the identity). ADS curve 1.6, settle x0.6. Heft W 0.6. Shake none.
- Sound: Mech x0.9, Low x1.1, Tail x1.2; Punch every shot (70 Hz, 120 ms, with a crack); ladder +2. Aim-in: the hammer
  cocked as the sights come up (two clicks): the six-gun's tell.
- Brass unchanged (empties at Eject). Crosshair: 3 ticks (left, right, under) + dot. Beats: CylinderOut roll -0.5;
  Eject -0.15; RoundsIn -0.05; CylinderIn roll +0.6 pitch -0.1, 0.3/0.05.

**Shotgun (Ranchhand): "a porch cannon: one shove through the shoulders, the pump racked, the world kicked back."**
- View: 2.5 deg, FOV +3.6%, 6.5 Hz, damp 0.5, roll +-0.9 lean, Push 2 cm. Aimed turn 0.6. Aimed shares 1.5/0.5/0.45.
- Recovery x0.8; `ShotgunSettle` 0.8 kept. Rack beat kept. ADS curve 2.2, settle x1.3. Heft W 1.25.
- Shake: `Shake.Shotgun` 0.10 for 0.18 s.
- Sound: Mech x1.0, Low x1.25, Tail x1.1; Punch every shot (50 Hz, 220 ms); ladder +3. Aim-in: stock thump + wood creak.
- Brass: hull kept. Crosshair: a ring sized to the pellet cone with 4 notches. Beats: ShellIn -0.05 each 0.1/0.03;
  Pump (end of reload) +0.15 0.25/0.05.

**Sniper (Longhorn): "one thunderclap down the valley: the kick holds while you work the bolt, then the world settles."**
- View: 2.0 deg, FOV +1.6%, 8 Hz, damp 0.6, roll 0.25, Push 1.2 cm. Aimed turn 0.4, aimed FOV share 0.4 (scope readable).
- New hold: aim recovery paused while the bolt cycles (`FWeaponRecoil::HoldRecovery(Seconds)` = bolt timing, at most
  `BoltMaxSeconds` 0.85; `Recoil.SniperHold` [1]), then recovers at its 5; a bolt-home beat +0.08 deg 0.2/0.04 at
  `UpdateBoltCycle`'s close. Aimed shares 1.3/0.45/0.4 (model hidden from scope 0.5 anyway).
- ADS curve 2.6, settle x1.2. Heft W 1.3. Shake: `Shake.Sniper` 0.12 for 0.3 s.
- Sound: Mech x0.9, Low x1.2, Tail x1.35 (recipe: 2.5 s valley roll, slapback 0.35-0.5 s); Punch every shot (45 Hz,
  250 ms); ladder +4. Aim-in: cheek-weld cloth + scope ring.
- Brass: new `ECasingKind::Magnum` 6.9x1.45, lazy 110-170 cm/s on the bolt draw, spin 8-14, landing pitch 0.8, volume x1.2.
  Crosshair (hip): 4 long thin ticks, wide gap (says "hip is a bad idea"). Beats: BoltOpen +0.06; MagIn -0.12; BoltClose
  -0.2 roll 0.15 0.35/0.06.

**Launcher (Powderkeg): "a keg on the shoulder: a whoomph that heaves you, the tube sags back, then the ground answers."**
- View: 1.4 deg up, FOV +4.4%, 4.5 Hz, damp 0.45, Push 3.5 cm, then a heave: a second kick -0.5 deg, `Delay` 0.16 s,
  6 Hz (the tube's weight settling). Aimed turn 0.6. Aimed shares 1.4/0.55/0.5. Recovery x0.8.
- ADS curve 2.4, settle x1.5. Heft W 1.5. Shake: `Shake.Launcher` 0.3 for 0.45 s at the shot (the blast's own jolt kept).
- Sound: Low x1.2; Punch every shot (40 Hz, 300 ms whoomph); ladder +3. Aim-in: tube clank.
- Brass none (back-blast kept). Crosshair: wide ring + dot. Beats: RocketIn -0.3 FOV -0.6% 0.5/0.1.

Cross-kind (Lane A): `Aim.Curve.<Kind>` replaces exp 2 in `GetAimAlpha`; `ViewKicks::ForAimSettle(Kind)` once when the
raw AimAlpha reaches 1 after being under 0.5: pitch -0.12 deg, FOV -0.3%, 9 Hz, damp 0.6, x `Aim.Settle.<Kind>`.
Cadence jitter (`Cadence.<Kind>`): symmetric, deterministic per shot (hash of a per-gun shot count, so the two
`GetShotInterval` calls a shot makes agree), mean interval kept within 0.5% over 1000 shots: DPS unchanged.
Heft rule (Lane B, one weight W per kind, `Heft.<Kind>`, master `Heft.Scale` [1]): look lag x W (clamps x W), lag
interp 9 / W^0.8, bob depth/side/roll x (1 + 0.35 (W-1)), idle breath x (1 + 0.4 (W-1)), jump lag and landing kick x W,
a sight-arrival forward push 0.3 cm x W (eases out over 0.15 s). Aimed, all of it still settles by `AimSteadiness` and the
sight lock (`Lock`) still holds the aim point on the line.

## Lanes (disjoint files; no builds, editor, commits; Main builds once)
**A. KICK (Opus).** `Player/ViewKick.h/.cpp` (table, Push, Delay, ForAimSettle), `Player/CameraShakeModifier.h/.cpp`
(apply Push; strength read), `Player/PlayerViewKicks.cpp` (shakes, heave), `Player/PlayerViewAim.cpp` (curve, settle,
calls `Weapon->PlayAimOut()`), `Player/PlayerViewComponent.h`, `Weapons/WeaponRecoil.h/.cpp` (HoldRecovery, per-kind
SideRandom), `Weapons/WeaponBaseKick.cpp`, `Weapons/WeaponBaseKinds.cpp` (cadence in `GetShotInterval`, sniper hold +
bolt-home beat, LMG dig-in), `Weapons/WeaponBase.h` (only lane; shot counter, `PlayAimOut` declared), `GunfeelKickTuning.h/.cpp`,
`GunfeelKickPattern.cpp`, new `Weapons/GunfeelKickKinds.cpp` (per-kind tables, `IdentityOn`). Tests: update
`GunfeelKickTests.cpp` (Kick.Identity, .ViewKick), `GunfeelKickPatternTests.cpp`, `FeedbackTests.cpp` (ViewKicks),
`GunfeelSteadyTests.cpp` (Profile); new `Tests/GunfeelIdentityTests.cpp`: `Looter.Gunfeel.Identity.Caps` (no kind's hip
peak clips: pitch <= 2.9, FOV <= 0.048, push <= 3.8), `.Distinct` (every pair differs >= 20% in two of pitch/FOV/
frequency/push), `.SustainCaps`, `.Aimed`, `.Recovery` (revolver back within 0.35 s; sniper held through the bolt),
`.Heave`, `.DigIn`, `.Cadence`, `.AimCurve`, `.Settle`, `.Shakes`, `.Switch` (Identity 0 = today's numbers).
PIE: each kind ADS + hip at a 15 m dummy; revolver 6 aimed pulls back on target; rifle 30 rounds shows the S; LMG first
rounds rough then calm; sniper view held until the bolt closes; launcher heave; `Looter.CameraShake 0` stops all of it.

**B. HEFT (Sonnet).** `Player/PlayerLocomotionViewModel.cpp`, `Player/PlayerLocomotionComponent.h` (view-model members
only), new `Weapons/GunHeftTuning.h/.cpp` (`FGunHeft`, `GunHeft::For(Kind)`, the rule above as plain functions), new
`Tests/GunHeftTests.cpp`: `Looter.Gunfeel.Heft.Table` (revolver < SMG < rifle < shotgun < sniper < launcher < LMG),
`.Lag`, `.Bob`, `.Switch`; run `Looter.Weapons.Aim.CrouchAligned` (the lock). PIE: flick the mouse with the Rustler then
the Thresher (the LMG trails visibly), walk/sprint bob, jump/land; ADS while strafing: sight stays on the crosshair.

**C. SOUND-CODE (Sonnet).** `Weapons/GunshotSound.h/.cpp`, `Weapons/GunfeelSoundTuning.h/.cpp` (`Sound.<Kind>.Mech/.Low/
.Tail` multipliers x maker flavour x the global ones; `Sound.Punch` [1], `Sound.PunchEvery.<Kind>` Rifle 3 SMG 0 LMG 2
others 1, Rifle at -4 dB; Punch 2D, own gun only), `Weapons/WeaponSounds.h` (`AimIn(Kind)` -> `Weapon.<Kind>.AimIn`,
falls back to `Weapon.AimIn`; `AimOut()`; `CasingPitch(ECasingKind)` table for Lane E), `Audio/LooterSoundCues.h`.
Tests `GunfeelSoundTests.cpp`: `Looter.Gunfeel.Sound.KindLayers`, `.Punch` (every N, own gun only, none for SMG),
`.AimIn` (fallback), `.Switch`. PIE: `au.Debug.Sounds 1` shows Punch on the right shots; no voice cut in a 3 s LMG burst.

**D. SOUND-ART (Opus).** `Art/Sounds/recipes/guns_*.py`, `Art/Sounds/cues.json`, `Art/Sounds/check.py` (FREQUENT gains
`Weapon.*.Core/.Mech/.Tail*/.Punch`), `Art/Sounds/README.md`. New cues: `Weapon.<7 kinds>.Punch` (rifle 80 Hz 60 ms;
LMG 60/90; revolver 70/120 + crack; shotgun 50/220; sniper 45/250; launcher 40/300; transients cut from the repo's pack
excerpts, sub synthesized), `Weapon.<7 kinds>.AimIn`, `Weapon.AimOut`; sniper TailOpen 2.5 s with slapback; SMG tails
0.6 s. Loudness ladder on the own-gun sum (short-term max LUFS vs rifle): SMG -3, rifle 0, LMG +1, revolver +2, shotgun +3,
launcher +3, sniper +4; every own-gun sum incl. Punch and the worst maker (Roundhouse Low 1.3, Saint Maud Tail 1.35)
peaks <= -1 dBFS (the revolver indoors was -0.1: fix it here). Renders with `Tools\sounds.ps1 Weapon.*`, 0 check flags,
presence 2-5 kHz under 8; prints the ladder table. Only the pack excerpts already in `Art/Sounds/packs/Sonniss`.
Main imports with `Tools/Unreal/build_sound_bank.py` (no PIE). Test `Looter.Audio`.

**E. BRASS + RELOAD BEATS (Sonnet).** `Weapons/WeaponBrass.h/.cpp` (`ECasingKind` gains Small, Magnum, Belt appended;
`SizeOf`, `KindFor`, link ISM, landing pitch via `WeaponSounds::CasingPitch`), `Weapons/WeaponBaseBrass.cpp` (speeds,
spin, LMG down-right throw + link), new `Weapons/ReloadBeats.h/.cpp` (`FReloadBeat {FViewKick Kick; float Rumble,
RumbleSeconds}`, `ReloadBeats::For(EWeaponKind, EReloadStep)`, `Play(const AWeaponBase&, EReloadStep)`; uses only
FViewKick's Pitch/Roll/FOV/Frequency/Damping), `Weapons/WeaponBaseReload.cpp` (one call where a step's sound plays).
Tests: `WeaponFXTests.cpp` (KindFor per kind), `NewGunsTests.cpp` lines 378-379 (SMG/LMG are no longer Rifle cases),
`GunfeelReloadTests.cpp` + `Looter.Gunfeel.Reload.Beats` (table, aimed none, Identity 0 = none). PIE: SMG small fast
cases, LMG links on the ground, sniper fat case lazily on the bolt; every reload's seat felt; `Looter.Gunfeel.Reload.Debug 1`.

**F. CROSSHAIR (Sonnet).** New `UI/HUD/HudCrosshairShapes.h/.cpp` (shape per kind through the LooterUI kit, outlined,
colours from `LooterUI::Color` only; gap = spread as today; kick scale/time per kind: SMG 1.1/0.08, rifle 1.3/0.12, LMG
1.2/0.1, revolver 1.5/0.16, shotgun 1.55/0.2, sniper 1.6/0.22, launcher 1.6/0.25), `UI/HUD/PlayerHUDWidgetCrosshair.cpp`,
`UI/HUD/PlayerHUDWidgetLayout.cpp` (the crosshair block only), `UI/HUD/PlayerHUDWidget.h` (crosshair members only). HITS
lanes touching the HUD widget files go after this one (Main sequences). Opacity/ADS-fade rules unchanged. First a 7-up
mockup PNG (Main's call overnight, shown to the user in the morning). Test new `Tests/HudCrosshairTests.cpp`:
`Looter.UI.HUD.Crosshair.Kinds`, `.Kick`. PIE + `Tools\hudshots.ps1` once with each kind in hand.

**Main's hooks (shared files):** `AWeaponBase::PlayAimIn` (WeaponBaseFiring.cpp) -> `WeaponSounds::AimIn(Kind)`, new
`PlayAimOut` beside it; CODEMAP lines (new files: GunfeelKickKinds, GunHeftTuning, ReloadBeats, HudCrosshairShapes, 4
tests); Gunplay.md "Gun identity (2026-10-28)" section; Handoff "Decided unattended". Model choice: A and D Opus (spring
math, mix); B, C, E, F Sonnet (specified).

## Order (one editor lane)
0. Main: read-only probe (sandboxed `Tools\runscript.ps1`) dumping all 7 definitions' stats, recoil, FireMode, BulletSpeed
   into this file; `Tools\perf.ps1 -Label "guns before" -Exec "Looter.Quality Medium"` + RR town horde
   (`Looter.Perf.Horde Spider 8 Basic 0 -1400 90`). The probe's rifle/shotgun `AimKick`/`AimRecovery` may move Lane A's
   aim-kick numbers; the view table above stands.
1. Lanes A-F in parallel. D first into the bank (old code ignores new cues), commit.
2. One `Tools\launch.ps1 -Build`; tests once per filter: `Looter.Gunfeel.Identity`, `.Heft`, `.Kick`, `.Steady`, `.Sound`,
   `.Reload`, `Looter.Feedback.ViewKicks`, `Looter.Weapons.FX`, `Looter.Weapons.NewGuns`, `Looter.Weapons.Aim`,
   `Looter.UI.HUD.Crosshair`, `Looter.Audio`.
3. DEMO play-check (below), fixes, perf after, commit per lane, push.

## DEMO play-check (RR, `Looter.GiveWeapon <Kind> Epic 5`, 3 slots: restart PIE for more; no capture batches)
Every kind: 10 hip + 10 ADS shots at a 15 m dummy, one full magazine, one empty reload, one tactical, a mouse flick, a
walk and a sprint; then `Looter.Gunfeel.Identity 0` and the same with the Bullpup and Ranchhand. Verdict per kind in one
line ("reads as its identity / still like X"), under 150 words. Check: no clip of the view at a shotgun/sniper/launcher
shot; ADS lock holds under heft; sounds `au.Debug` + by ear if the PC's audio is on.

## Perf (120 fps, 8.3 ms, 1080p Medium, RX 580; RR Medium now 7.6 ms, p95 7.9)
All new work is per-shot math (springs, tables), one more ISM component (LMG links, last 40, one draw), at most one more
voice per heavy shot (Punch; LMG 5.5/s at concurrency 3), a crosshair of at most 12 painted lines (kit brushes, no
textures), camera shakes as numbers. Expected under 0.05 ms game thread, nothing on the GPU. Measure before/after with
the same two runs; reject if p95 rises over +0.2 ms.

## Risks
- Motion sickness: heavier kicks, Push and shakes. Caps untouched (3 deg, 5%, push 4 cm), all under the Camera shake
  setting; `Identity.Caps` test keeps every kind under them.
- The sight lock: heft adds lag; it must stay inside `Offset *= Settle` and the `Lock` correction (test + PIE strafe).
- Tests that pin today's numbers break by design (Kick.Identity, Feedback.ViewKicks, NewGuns 378-379, Steady.Profile):
  each lane updates its own; none edited outside its lane.
- Clipping: Punch + maker Low on a Roundhouse revolver indoors; D's worst-case table must show <= -1 dBFS.
- `GetAimAlpha`'s curve feeds spread, sensitivity and the crosshair fade: the curve only reshapes 0..1, end points kept.
- HITS.md overlap: `ViewKicks::ForKill` and the hit marker. Lane A owns ViewKick.cpp; HITS asks Main for kill-punch
  changes; Lane F owns only the crosshair pieces of the HUD widget files.
- Cadence jitter could beat with the LMG spin-up sound (cut to 0.35 s): jitter starts only at full spin.
- Many 10-22 numbers were never play-checked by the user: the master switch lets them compare in one console line.

## Questions (Main decides overnight; the user's answer in the morning overrides)
1. Heavy guns' view kick (shotgun/sniper/launcher all clip at 3 deg / 5% today):
   A) keep the caps, separate by shape (pitch, FOV, push, heave, springs) [recommended]; B) raise the caps to 4 deg / 7%
   for those three; C) lower all kicks and carry the weight in sound and the gun model only.
2. Hip move speed by kind (a balance change, nothing like it today):
   A) small, +5% SMG/revolver, -5% LMG/launcher, -3% sniper (`Move.<Kind>`, Lane B) [recommended]; B) none, weight only
   in the hands and camera; C) bigger, +-10%.
3. The royalty-free pack to show in the morning (gaps: SMG/LMG action rattle, sniper bolt, aim-in foley, valley echoes):
   A) another year's Sonniss GDC Game Audio Bundle (free, same licence as the 2016/2017 files already in the repo:
   commercial games, no attribution) [recommended]; B) freesound.org files under CC0 only (licence per file, each URL
   kept); C) ZapSplat's standard licence (free plan needs a credit line). Main names file, source and size before any
   download either way.
