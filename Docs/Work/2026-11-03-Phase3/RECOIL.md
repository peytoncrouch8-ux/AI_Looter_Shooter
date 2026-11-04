# Lane RECOIL (audit change 3): a rifle burst that climbs; one kick layer. Built 2026-11-03, NOT compiled, NOT run (lane rules)

Before (verified in the code): Common rifle aim kick 0.35 (DA_AssaultRifle keeps the struct default; read off the .uasset) x
`Recoil.More` 1.4 = 0.49 deg; aim recovered at 35% while held, and the 0.09 s full-recovery delay sat under the 92 ms round, so
every interval also got one frame of full recovery: a held 30 rounds plateaued ~2.3-3.5 deg, sideways within +-0.35-0.55. Camera crack
14 Hz / 0.6: gone (zero crossing 45 ms) before the next round, so `Kick.SustainCap` never trimmed anything. More was divided out of the
view (table at 1.4) but multiplied the aim and, on top of `Recoil.<Kind>`, the model (rifle model 1.96x asset; revolver aimed flip
8 x 1.96 x 0.7 = 11 deg; shotgun aimed push 7 x 1.5 x 1.4 x 1.5 = 22 cm = the 22 cm eye-to-sight gap). AccuracyCalm 1.5 [0.3-1.4]:
Epic plain 0.72, Epic +40% 0.37 (0.3 at the floor with a good variance roll). Identity 0/1 kept two kick tables and gates.

## Files
Edited: `Weapons/WeaponRecoil.h/.cpp`, `Weapons/GunfeelKickTuning.h/.cpp`, `Weapons/GunfeelKickKinds.cpp`, `Weapons/GunfeelKickPattern.cpp`,
`Weapons/WeaponBaseKick.cpp`, `Player/ViewKick.h/.cpp`, `Player/PlayerViewKicks.cpp`, `Player/PlayerViewAim.cpp`; tests `GunfeelKickTests.cpp`
(RevolverCrit untouched), `GunfeelKickPatternTests.cpp`, `GunfeelIdentityTests.cpp`, `GunfeelIdentityFeelTests.cpp`, `GunfeelSteadyTests.cpp`.
New (CODEMAP lines):
- `Weapons/GunfeelKickKinds.h`: `GunfeelKick::FKindFeel` + `KindFeelDefault` (each kind's identity: sight shares, aim kick, recovery, cadence, ADS curve, settle,
  sustain cap, shake), `IsAutomaticKind`, the identity rules, the shot-profile chain (`FProfileNumbers`, `ShippedProfileNumbers`/`LiveProfileNumbers`,
  `ShotProfile`), `SightGap`/`AimedBackMost`; defaults `RecoveryWhileFiring`, `RecoveryDelay`, `AimedBackGap`, `LiftKick`. Included by GunfeelKickTuning.h
  (split out: that header was 486 lines).
- `Weapons/GunFeelSettings.h/.cpp`: `UGunFeelSettings` (UDeveloperSettings, Project Settings > Game > Gun Feel, `Config=Game, DefaultConfig` ->
  `[/Script/AI_Looter_Shooter.GunFeelSettings]` when saved) + `FGunKindFeelSettings`: each kind's FKindFeel, starting at `KindFeelDefault`; read by `GunfeelKick::KindFeel`.
- `Tests/GunfeelBurstTests.cpp`: `Looter.Gunfeel.Burst.*` (RifleClimb, Tap, Kinds, Model, MoreOnce): the trigger held frame by frame through the real chain.
CODEMAP updates for edited files: `ShotProfile`/`ShippedProfileNumbers`/`LiveProfileNumbers` live in WeaponBaseKick.cpp; `SightGap`/`AimedBackMost` in
PlayerViewKicks.cpp; `ViewKicks::ForLift` replaced `ForShotToday` in ViewKick; GunfeelKickKinds.cpp reads the settings (no per-kind cvars).
DefaultGame.ini: no entry needed (class defaults = KindFeelDefault). Settings were small (~190 lines with the struct) so the table moved; the per-kind
cvars are gone.

## Numbers (before -> after)
| What | Before | After | Switch |
|---|---|---|---|
| Aim recovery while held | 0.35 of rate | 0.1 | `Looter.Gunfeel.Recoil.WhileFiring` [0.1] |
| Full recovery starts after | 0.09 s (under the 92 ms round) | 0.12 s | `Looter.Gunfeel.Recoil.RecoveryDelay` [0.12] |
| Common rifle aim kick a round | 0.35 x 1.4 = 0.49 | 0.35 x 2.3 = 0.805 | Gun Feel > Rifle > AimKick 2.3 (others 1.4 = as before) |
| Rifle aim recovery | 7 x 1 = 7/s | 7 x 2.3 = 16.1/s (tap settles) | Gun Feel > Rifle > Recovery 2.3 |
| SMG recovery | 8 x 1.2 | 8 x 2.0 (hosed mag climbs ~3.8, half the rifle) | Gun Feel > SMG > Recovery |
| Burst build (all automatics' max) | 0.06/shot to 1.8x | 0.02 to 1.2x | `Recoil.RifleClimb`, `.RifleClimbMax` |
| Rifle drift | 0.1 x1.4 from shot 6; back -0.06 x1.4 from 16 | 0.3 from shot 5; back -0.04 from 22 | `Recoil.RifleDrift/From/Back/BackFrom` |
| Rifle random side | 0.12 x 1.4 x 0.4 = 0.067 | 0.12 x 2.3 x 0.4 = 0.11 | (scales with AimKick) |
| Camera crack (every kind) | table, 14 Hz rifle | unchanged; never trimmed, not counted under the cap | `Kick.<Kind>` |
| Camera lift (new, automatics) | none | rifle 0.5 deg 2.2 Hz, SMG 0.25 3 Hz, LMG 0.6 1.8 Hz, damping 0.8; stacks, held by the kind's cap (rifle 1.0, SMG 0.5, LMG 1.6, unchanged numbers) | `Looter.Gunfeel.Kick.Lift` [1], Gun Feel > SustainCap |
| `Recoil.More` | divided out of view, x on aim, x again on model | 1.4 kept, applied once: every layer x KickShare = ShotRecoil / 1.4 | `Recoil.More` |
| Model kick (all kinds) | asset x 1.4 x Recoil.<Kind> | asset x Recoil.<Kind> (rifle hip flip 4.9 -> 3.5 deg, shotgun hip back 14.7 -> 10.5 cm) | `Recoil.<Kind>` |
| Revolver aimed flip | 11.0 deg | 5.0 (8 x 1.4 x 0.45) | Gun Feel > Revolver > AimedFlip 0.7 -> 0.45 |
| Shotgun aimed push (profile) | 22 cm | 15.75, limited to 0.6 x eye-to-sight gap (13.2 at 22 cm) | `Looter.Gunfeel.Recoil.AimedBackGap` [0.6] |
| AccuracyCalm | ^-1.5 in [0.3, 1.4] | ^-0.8 in [0.6, 1.3]: +30% 0.81, plain Epic 0.84, Epic +40% 0.64 | `Recoil.AccuracyCalm`, `...Least`, `...Most` |
| FViewKickStack::MaxKicks | 12 | 16 (a held LMG keeps ~7 lifts alive) | code |
Result (the test's own chain; JS port with UE's FRandomStream, 120/60/30 fps agree): 30 Common rifle rounds climb 7.3 deg (aim 6.6 + lift 0.8;
4.2 after 10 rounds), first 5 rounds within 0.15 sideways, then 2.0 deg right; back to 0.01 deg 0.5 s after release; 3-round tap 2.0 deg,
4% left at 0.3 s; LMG rides its 2.5 deg cap (both ways of sway); single shots' aim kicks unchanged (shotgun 3.08, sniper 2.24, revolver 1.4).

## Deleted (one kick layer)
`ViewKicks::ForShotToday`; `Default::Today` (except `KickShotgun`/`KickLauncher`: NewGunsTests.cpp:391 still reads them, delete with that line);
shared `Looter.Gunfeel.Kick.SustainCap` and `Recoil.AimedFlip/Roll/Back` (+ the live `AimedModelKick(alpha, out)`); per-kind cvars
`Kick.AimedTurn/AimedFov.<Kind>`, `Recoil.AimedBack/Flip/Roll/Recovery.<Kind>`, `Cadence.<Kind>`, `Aim.Curve/Settle.<Kind>`, `Kick.SustainCap.<Kind>`,
`Shake.<Kind>[Seconds]` (now Gun Feel settings); every Identity gate in the kick (cadence, dig-in, sniper hold, shakes, heave, bolt-home, sight settle,
SMG bloom/shimmy/side random). `Looter.Gunfeel.Identity` stays for sound, heft, brass, reload beats (other files ask it).

## Contract deviation: "~8 Hz" for the automatics' camera kick
At 8 Hz no damping stacks: damping 0.6 crosses zero at 78 ms (before the 92 ms round), 0.95 keeps ~10% by then. `Looter.Feedback.ViewKicks`
(not this lane's) also requires the rifle's crack faster than the shotgun's 6.5 Hz and all but gone at 0.1 s. So the crack stays and a slow
lift (2.2 Hz, ~95% left at the next round) carries the stack; the cap now trims (rifle: free 1.0 -> held 0.83).

## Tests (`Tools\runtests.ps1 -Filter ...`, editor open)
`Looter.Gunfeel.Burst`, `Looter.Gunfeel.Kick`, `Looter.Gunfeel.Identity`, `Looter.Gunfeel.Steady`. Worth one run (others' files, untouched, read
changed code): `Looter.Feedback.ViewKicks`, `Looter.Weapons.Recoil`, `Looter.Weapons.Revolver`, `Looter.Weapons.NewGuns`, `Looter.GunIdeas`
(Restless ratio), `Looter.Weapons.Aim` (the settle's pivot now via `GunfeelKick::SightGap`). Burst.RifleClimb loads DA_AssaultRifle to check its numbers.

## For Main
- Stale comments outside the lane: `Weapons/WeaponBase.h:253` (dig-in "1 with Identity 0"), `Player/PlayerViewComponent.h:302` (AimCurve cvar name).
- Pipeline Gunplay.md: kick sections' cvar names above; "Gun identity" line "`Looter.Gunfeel.Identity 0` gives the feel before it" is now sound/heft/brass/beats only.
- Shots read the camera's kicked view (PlayerController::GetPlayerViewPoint), so the lift is real climb that springs back on its own.
- What the eye sees of the aimed push is ~15% of the profile's (PlayerLocomotionViewModel AimSteadiness 0.85); the gap limit guards sloppy/Restless rolls.

## PIE check for the user
Hold the Common rifle at a wall from the hip: the reticle climbs most of a hand's width (~7 deg) over the magazine, straight up for the first rounds,
then drifting right; pull down to hold it on target. Let go and it is back in ~0.3 s; tap 3 rounds and it settles before the next tap.
A/B: `Looter.Gunfeel.Recoil.WhileFiring 0.35` (old plateau), `Looter.Gunfeel.Kick.Lift 0` (no view lift).
