# Lane UNPAID (audit change 11): one teleport gate, one flee rule, a hunched drift

Not built, not run (lane rules). Every file is in `Saved/Phase3Staging/UNPAID/Source/AI_Looter_Shooter/` (same paths as `Source/`);
Main copies the tree over `Source/`, builds, runs the tests below. Staged from HEAD f0ff713d plus the working tree: the staged
`Creatures/UnpaidFightTuning.h` (FrenzyTokens 4 -> 7) and `Tests/UnpaidFightTests.cpp` (Kit.Director's PackSize) already carry the
uncommitted change-4 edits that were in those two files; if they change again before the copy, merge by hand.

## Files
Changed: `Creatures/UnpaidCreature.h` (comments only, 530 -> 529 lines), `UnpaidCreaturePhase.cpp`, `UnpaidCreatureSight.cpp`,
`UnpaidCreatureMoves.cpp`, `UnpaidCreatureVariety.cpp` (496), `UnpaidCreatureFadeBack.cpp`, `UnpaidCreatureGait.cpp`,
`UnpaidCreatureArms.cpp`, `UnpaidCreatureRig.cpp`, `UnpaidFightRules.h/.cpp`, `UnpaidFightTuning.h/.cpp`, `UnpaidMoveRules.h`
(a comment), `UnpaidGaitRules.h/.cpp`, `UnpaidLifeTuning.h/.cpp`, `UnpaidPackSubsystem.h/.cpp`, `CreatureTraitComponentPhase.cpp`
(the gate call only); tests `UnpaidKitSightTests.cpp`, `UnpaidFightTests.cpp`, `UnpaidFadeBackTests.cpp`, `UnpaidMoveTests.cpp`,
`UnpaidGaitTests.cpp`. Not changed: `UnpaidCreatureVarietyMoves.cpp` (its moves pose over the hunch as they are),
`CreatureTraitPlayTests.cpp` (its Phasing test is a spider's: the gate is the Unpaid's; the Unpaid's Phasing is in the new test),
`PackRules.*` (nothing to cut: see Flee).

New (CODEMAP lines):
- `Creatures/UnpaidPackSubsystemBlinks.cpp`: `UUnpaidPackSubsystem`'s one blink gate: every Unpaid blink at a player asks `MayBlink`
  (budget, gap, an owed press step first), lands only where `MayLandAt` allows, is counted by `NoteBlink`; `Looter.Unpaid.Blinks`.
- `Tests/UnpaidBlinkGateTests.cpp`: `Looter.Creatures.Unpaid.Blink.*`: the gate's budget, gap and owed step (Gate), the landing rule
  and the flank's arc (Landing), every blink path through it in a level: phase-step, phase-strike, fade-back, Phasing trait (Paths).
CODEMAP edits for existing lines: `UnpaidFightRules` gains `FUnpaidBlinkGate` (was `FUnpaidStepGate`, kept as an alias),
`FUnpaidBlinkLanding`, `UnpaidBlinkRules` (`AngleOffView`, `MayLand`); `UnpaidGaitRules` gains `FHunch`, `HunchTrunk`, `HunchArm`,
`HuntArmShare`, `ArmHunch`; `UnpaidPackSubsystem` gains the gate (`MayBlink`, `NoteBlink`, `BlinksInMinute`, `MayLandAt`).

## 1. One teleport gate (layers merged: seven permissions -> one)
Each path keeps its own reasons to blink; only the permission is shared: `AUnpaidCreature::MayPackStep` -> `UUnpaidPackSubsystem::
MayBlink` (was the fight director's `MayStep`), counted in `StartStepTo` -> `NoteBlink` (was `NoteStep`).
| Path | Asks the gate | Lands by |
|---|---|---|
| phase-step, press step, step to sight (`TickPhaseStep`) | yes (as before, new gate) | `CanStandAt`: the behind rule (refusal 6, "behind them" in `Looter.Unpaid.TryStep`) |
| flank (`TryFlank`) | yes | `FlankBearings` in the arc + `MayLandAt(bFlank)` + `CanStandAt` |
| phase-strike (`BlinkToStrike`, `Choose`'s `bMayBlink`) | yes | same as the flank, at 1.7 m: only 60-90 degrees pass the behind rule |
| fade-back (`TryFadeBack`) | yes | `CanStandAt` |
| Phasing trait (`CreatureTraitComponentPhase.cpp`) | **new** (it asked none): an Unpaid's, not a boss's, at its hitter | `FindBlinkSpot` + `MayLandAt` |
A rise (`CanStandAt` with `For`) is no blink: not gated, not landing-ruled. Bosses (Abel) keep their own steps. The Bell-Ringer's and
the miners' phase-steps go through the same gate (same pack). Every ask/count is plain data: `FUnpaidBlinkGate` (sliding minute + gap
+ the owed hold, OwedHold 0.25 s as before), `UnpaidBlinkRules::MayLand` (0.5 degree slack at the arc's edges and at 90).

| Number | Before | After |
|---|---|---|
| gap between blinks at one player | `Looter.Unpaid.PackStepGap` 1.2 s, phase-steps/flanks/strikes/fade-backs (not the trait) | `Looter.Unpaid.BlinkGap` 2.0 s, all seven |
| blinks a minute at one player | up to ~50 (60/1.2) + the trait's | `Looter.Unpaid.BlinksPerMinute` 15 (any 60 s window) |
| landing behind the player's back (> 90 degrees off their view) | anywhere (flanks 3.3 m at up to 180; strikes 1.7 m at 105-180; sight steps 1.5 m) | never nearer than `Looter.Unpaid.BlinkNearestBehind` 250 cm |
| flank / phase-strike bearings off the player's view | 105, 145, 180, 75 | `Looter.Unpaid.FlankArcMin` 60 .. `FlankArcMax` 120: 90, 75, 105, 60, 120 (own side first) |
| `Looter.Unpaid.PackStepGap` | cvar | removed (BlinkGap) |
A/B: `Looter.Unpaid.BlinksPerMinute 0`, `Looter.Unpaid.BlinkGap 1.2`, `Looter.Unpaid.BlinkNearestBehind 0`, `FlankArcMax 180`
approximates the old (the old 145/180 bearings are gone). `Looter.Unpaid.Blinks` prints the blinks at the player in the last minute
against the budget; `Looter.Unpaid.Moves` gained `Blink` (every blink since its reset).
Leftover for Main (FIGHTS' file, not mine): `UFightDirectorSubsystem::MayStep`/`NoteStep` and `FTargetFight::Steps` have no Unpaid
caller now; delete them and the `FUnpaidStepGate` alias + `FUnpaidBlinkGate::MayStep` shim in `UnpaidFightRules.h` together.

## 2. One flee rule
Kept: the leader-down falter (`OnPackLeaderDown`: falter, scream/tell called off, coal gutters) and the shriek.
- Fade-back: `Looter.Unpaid.FadeBack` 0.5 -> **0** (off; 0.5 = the old for the A/B). `TryFadeBack` now refuses at 0 even with its
  roll skipped (the tests' `bRoll false`).
- Break-off (PackRules 35% / 0.5) and the leader-down break-off (0.35): they never reached the Unpaid (`UEncounterSettings::RetreatKinds`
  = spider, slime; no ini override), so nothing to cut in `PackRules`; a test pins it (`MayRetreat(AUnpaidCreature)` false).
Four flee rules on paper -> for the Unpaid one live (the falter), one behind a cvar.

## 3. The hunched drift (the plain Unpaid only; miners, Bell-Ringer, bosses as before)
`FUnpaidGaitWork` lays it (`UnpaidCreatureGait.cpp`: `Step` eases `Kit.Gait.Hunch` at 3/s, its first frame starts there; `LayBones`
the trunk; `ArmPose` the arms); `PoseArms` keeps only a share of the hunt's arm raise; the rig adds the shroud's trail to its lift.
Given up: by the channels as they come (a shriek's Rear, a lunge's Reach, a crumple; an arm's to Spread and its own Rake), and its
goal 0 in the moves that pose the whole body (failed-lunge shriek, rally, low swipe), rising, dead.
| `Looter.Unpaid.Idle.*` | Default | What |
|---|---|---|
| `Hunched` | 0 (was 1; off until seen in PIE, Enemies.md says why) | master; 1 = the hunched drift, 0 = the arms-out float as before |
| `Spine` / `Chest` | 3 / 6 deg | waist and chest curl forward. Kept small: `Looter.Creatures.Unpaid.Coal` (not mine) holds the coal's facing within ~18 deg at rest: lean 4 + 9 + breath/sway up to ~4 = ~17 |
| `HuntCurl` | 8 deg | the chest curls this much more hunting (chase: lean 6-22 + 9 + 8) |
| `Neck` / `HeadUp` | 16 / 18 deg | head carried low and forward on the neck, face lifted back toward its prey (net ~7 deg down) |
| `ArmHang` | 0.85 | arms swing back this share of the trunk's curl: plumb under the shoulders |
| `ArmOut` / `Elbow` / `Curl` | 3 / 24 / 12 deg | arms a little out, elbows bent (12 -> 36), fingers half shut: hands low at the hips |
| `HuntArms` / `HuntElbow` | 0.3 / 8 deg | hunting keeps 30% of the old raise (arms forward 24-42 -> 7-13 deg) and bends the elbows 8 more: hands low, a little forward |
| `ShroudTrail` | 10 deg | the shroud trails back off its hang (on top of a slope's lift) |
Paper check (rig lengths): hunting hand ~11 cm lower than the old pose, ~20 cm further forward than the hunched idle (the not-mine
`Looter.Creatures.Anim.UnpaidArms` wants > 8 cm forward); idle hand drift kept (Calm unchanged at rest).

## Tests (`Tools\runtests.ps1 -Filter ...`)
- New/changed, mine: `Looter.Creatures.Unpaid.Blink` (Gate, Landing, Paths), `Looter.Creatures.Unpaid.Kit` (Rules: the arc's bearings;
  PackStep: the subsystem's gate; FadeBack: off by default, on for its play part, the Unpaid never a retreat kind; Sight),
  `Looter.Creatures.Unpaid.Moves` (Mix through the gate's gap and budget), `Looter.Creatures.Unpaid.Gait` (Rules: the hunch's rules;
  new Hunch: head forward, coal down but still within 0.95 of ahead, hunting hands lower than the old pose).
- Touched by the change, not mine, run them: `Looter.Creatures.Anim.Unpaid` (UnpaidArms: arms out of the torso, hands forward
  hunting, no jumps; UnpaidBank; UnpaidShroudSlope with the trail), `Looter.Creatures.Unpaid.Coal` (the coal facing at rest),
  `Looter.Creatures.Unpaid.PhaseStep`, `Looter.Creatures.Unpaid.Rise`, `Looter.Creatures.Traits.Phasing`, `Looter.Creatures.BellRinger`,
  `Looter.Creatures.Hits.Unpaid`. One filter for most: `Looter.Creatures.Unpaid`, then `Looter.Creatures.Anim.Unpaid`,
  `Looter.Creatures.Traits`, `Looter.Creatures.BellRinger`.
- Risks to watch: `Anim.UnpaidArms` armpit/arm depth with the new hang (the keep-out should clear it; if not, lower `Idle.Elbow` or
  raise `Idle.ArmOut`); `Unpaid.Coal`'s 0.95 if a later change adds chest pitch at rest.

## PIE check for the user
- Ransom's Rest town gate (or `Looter.Unpaid.Rise 6 1`): the Unpaid arrive in front of you or at the edge of the screen, never behind
  your back close; `Looter.Unpaid.Moves reset`, fight a minute, `Looter.Unpaid.Blinks` (15 or fewer) and `Looter.Unpaid.Moves` (Blink).
- `Looter.Unpaid.Idle.Hunched 1`: they drift hunched, hands low, heads forward (off by default; live, eases over).
- A hurt one no longer slips away to the edge; `Looter.Unpaid.FadeBack 0.5` brings the old fade-back back to compare.

## Cost
Per Unpaid a frame: ~13 more cvar reads (the idle numbers) and a few float ops; per blink search candidate one angle/distance check
(before the traces: it saves traces); the gate's minute list holds at most a budget's worth of doubles per player. Fewer blinks = fewer
spot searches. No new actors, ticks or traces.
