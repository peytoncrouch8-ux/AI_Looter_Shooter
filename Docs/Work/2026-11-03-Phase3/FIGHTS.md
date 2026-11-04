# Lane FIGHTS (audit change 4): packs press together with smaller hits; enemies hear gunfire

Not built, not run (lane rules). Main builds, runs the tests below, then `Looter.Fight.Sim` to refit the paper uptimes.

## Numbers (all `FFightTuning` defaults = the live `Looter.Fight.*` cvars; one place: `Creatures/FightTuning.h`)
| What | Before | After |
|---|---|---|
| `Looter.Fight.MeleeTokens` | 3 | 5 |
| `Looter.Fight.RangedTokens` | 2 | 4 |
| `Looter.Fight.DamageScale` | 1.8 | 1.2 (spider bite L1 19.8 -> 13.2; Unpaid hit 14.4 -> 9.6) |
| `Looter.Fight.RingRadius` | 480 | 300 (spider waits at 300 = reach 220 + margin 80; slime 530 unchanged; Unpaid 480 -> 460; asks for a turn within 1.6 rings: spider 768 -> 480 cm) |
| `Looter.Fight.FeintChance` | 0.15 | 0.08 |
| `Looter.Fight.HearGunfire` (new) | none | 3000 cm; 0 = off |
| Spider `PackAlertRadius` (`SpiderCreature.cpp`) | 0 | 1500 (Spitter and spiderlings inherit it; the Gravemother's rank call 3000 is wider anyway) |
| Slime 2500, Dredger 3000, Unpaid 1500, placeholders 1500 | unchanged (already call; slime and Dredger farther than 15 m, left so) |
| `CreatureBase.h` PackAlertRadius default | 0 | 0 kept on purpose: every regular kind now sets its own; 0 left for the Sentinel and bosses |
| `FittedUptime` spider / slime / Unpaid | 0.75 / 0.9 / 0.3 | 0.65 / 0.9 / 0.55 |
| `StandingDeathBands` | all target 8-12, guard 6-16; spider guard 2-16 | same; spider guard 3.5-16 |

## Hearing (new)
- Rule, plain data: new `Creatures/GunfireHearing.h/.cpp` (`FGunfireListener`, `GunfireHearing::MayHear` cheap half, `::Hears` full).
  Hears = within radius (>0), state Idle or Wander, no target, not passive, brain not held (buried/perched/climbing out), not hidden,
  not a boss (`KeepsBossRules`), no scene holding the player, shooter `CheckHunt == Huntable` (on its ground, out of safe zones), not spurned.
  Off-ground shooters are NOT heard (a pack on another level of ground stays put; it would only walk to its edge).
- Listener: `static ACreatureBase::HearGunfire(APawn* Shooter, const FVector& Where)` in `CreatureBaseHunting.cpp`. Only the player's
  pawn (IsPlayerControlled, or `UEncounterSubsystem::GetPlayer()` = a test stand-in) is heard. Walks `GetTrackedCreatures()` (no actor
  iterator), collects hearers, then each: `BeginHunt` + `BeginNotice` + `RecordEncounter` (= a sight). Verbose log "heard ... shot".
- Broadcast: `UBulletSubsystem::Fire` end, once per volley (`bNewVolley`; a shotgun blast = one), `EffectGeneration == 0` only, `Where = Shot.Start`.
  (BulletSubsystem.h untouched: a direct static call, no delegate.)

## Pack call change (CreatureBaseHurt.cpp)
- The hurt pack call now skips a creature in its entrance (`BrainHold > 0` and no target: lurker on its perch, buried Unpaid, one climbing
  out). Why: with Basic spiders calling 15 m, a shot lurker would drop every perched mate at once and past `MateReach`, breaking the
  lurk's 0.35-1.2 s mate stagger (`AAmbushSpawner::NoteLurkerSprang`). Side effect: a Gravebound's 30 m call no longer springs lurkers either.

## Paper model (Looter.Fight.Pace.Bands; 6 of a kind, standing, levels 1/3/5)
| Kind | Before (3 turns, x1.8) | After (5, x1.2, refit) | Band |
|---|---|---|---|
| Spider | 2.7 / 3.0 / 3.2 s | 4.7 / 5.3 / 5.6 s | target 8-12 (warns), guard 3.5-16 |
| Slime | 7.5 / 8.4 / 8.9 s | 10.4 / 11.6 / 12.3 s | L5 warns, guard 6-16 |
| Unpaid | 11.5 / 13.0 / 13.7 s | 9.4 / 10.6 / 11.2 s | in target |
- Uptime working: Unpaid from DEMO's 10-29 run (6 Basic L3 still, dead 10.4 s, 175 taken, 3 turns held) = 0.43 old, x1.3 for 5 turns
  (~1.9 starts/s vs 1.5) = 0.55. Spider: 0.75 fitted 10-28, before the 10-31 cunning (circle 65% of turns 0.55-1.15 s, hit-and-run 35%):
  own cycle 2.1 -> ~3.0 s, 6 bite ~2/s against the model's 2.86 = 0.65-0.7. Slime: 0.9 kept (section 0's "about 9 s"); WARNING the
  10-29 Restless+leader run implies ~0.45 (with 5 turns ~0.6 = 15-18 s still, over the guard). Sim decides.
- Spider can't reach 8-12 with this lane's levers (six bite at own pace). Main/user pick (not in this lane): spider bite 11 -> 6 in
  Project Settings > Creature Kinds (`SpiderCreature AttackDamage=6`): 8.6 / 9.7 / 10.2 s; or a spider melee gap 0.75 s
  (`ASpiderCreature::GetMeleeGap`, like the slime's 0.45): 10.1 / 11.3 / 12.0 s. Then drop the spider's guard exception.

## Files
- `Creatures/FightTuning.h/.cpp`: defaults above, `HearGunfire` field + cvar + `LiveHearGunfire()`.
- `Creatures/GunfireHearing.h/.cpp` (new). CODEMAP: "Hearing a player's gunfire: FGunfireListener and GunfireHearing::MayHear/Hears,
  the plain rule ACreatureBase::HearGunfire acts on (Looter.Fight.HearGunfire)."
- `Creatures/CreatureBase.h`: `static void HearGunfire(APawn*, const FVector&)` only. `CreatureBaseHunting.cpp`: its body (CODEMAP: add
  "and what it hears: a player's gunfire, HearGunfire"). `CreatureBaseHurt.cpp`: the entrance skip. `SpiderCreature.cpp`: PackAlertRadius line.
- `Combat/BulletSubsystem.cpp`: the call (CODEMAP: add "tells the creatures each player shot: ACreatureBase::HearGunfire").
- `Creatures/FightPaceRules.cpp`: `FittedUptime`, `StandingDeathBands` spider. `FightDirectorSubsystem.cpp`: a stale comment only.
- Tests: `FightTests.cpp` (Tuning defaults + HearGunfire, Ring at 300, Scales 1.2, Director sized by live MeleeTokens + 2 at 4.5 m),
  `FightPressureTests.cpp` (ease test with MeleeTokens spiders so the ease shows), `FightPaceTests.cpp` (spider warning text),
  `CreaturePursuitTests.cpp` (new `Looter.Creatures.Pursuit.Hearing`: plain rule + test level).

## Tests (editor open)
- `Tools\runtests.ps1 -Filter Looter.Fight`
- `Tools\runtests.ps1 -Filter Looter.Creatures.Pursuit`
- After Main's fixes below: `-Filter Looter.Creatures.Unpaid.Kit`, `-Filter Looter.Creatures.Gravemother.PackCalls`,
  `-Filter Looter.Creatures.Spider.Ambush`, `-Filter Looter.Bestiary` (reads LiveDamageScale; should pass).
- Sim (Main): `Looter.Fight.Sim Unpaid 6 Basic`, `Spider 6 Basic`, `Slime 6 Basic`, standing; refit `FittedUptime` from them.

## Breaks outside this lane (Main must fix; files not mine)
1. CRASH RISK `Tests/UnpaidFightTests.cpp` `Looter.Creatures.Unpaid.Kit.Director` (~l.497-524): spawns 5 Unpaid, then indexes
   `Pack[Usual]` with Usual = MeleeTokens = 5: out of range (TArray check). Spawn `FFightTuning::Live().MeleeTokens + 1` (angle 360/N).
2. `UnpaidFightTests.cpp:129` "A frenzy lets more in at once" (`FrenzyTokens 4 > MeleeTokens 5`) fails, and in play the frenzy now adds
   no turns. Raise `Looter.Unpaid.FrenzyTokens` 4 -> 7 (`UnpaidFightTuning.h:179`). (l.134 flank range: 1104 > 1100, still passes, barely.)
3. `Tests/GravemotherTests.cpp` `Looter.Creatures.Gravemother.PackCalls` l.413-415: "A Basic spider ... calls nobody" (Neighbour 5 m
   off) now answers. Make it "answers within 15 m" and add one 20 m off that doesn't.
4. Stale text: `FightPaceRules.h:143` (spiders "the known exception, leave them"), its `FFightNumbers` defaults 3/1.8 (Model test's own
   numbers, fine to keep); `SpiderCreatureLurk.cpp:10-11` ("its pack's call still reach it": not while perched now); `SpitterTests.cpp:413`
   "(1.8 ...)"; `UnpaidCreatureVariety.cpp:468` no-director fallback 480; Pipeline `Enemies.md` "Fight: AI" (tokens, ring, feints,
   DamageScale [1.5] there), "Fight: pack brains" (spider exemption), add hearing + spider call.
5. Watch in play: `FeintDepth` 0.55 of a 300 ring brings a waiting spider's feint to 165 cm (inside its 220 bite reach, a bluff bite);
   was 264. If it reads as a missed bite, `Looter.Fight.FeintDepth 0.75`.
6. Another lane left a stray `Source/AI_Looter_Shooter/Dev/sed0kuJEJ` (sed temp file) in the tree: delete before committing.

## PIE check for the user
- Ransom's Rest, after Main 4: from ~25 m, fire one shot at the ground near Camp SheepFold (the sheep fold, west field): the whole camp
  turns and comes, not just the ones that saw you. Stand among them: up to five swing at once, each hit about 10 instead of 14.
- The Quarry Track spider patrol: shoot one spider: every spider within 15 m comes at once; they crowd in to 3 m and bite ~13 (was ~20).
- Switches: `Looter.Fight.HearGunfire 0` (no hearing), `Looter.Fight.MeleeTokens 3`, `Looter.Fight.DamageScale 1.8`, `Looter.Fight.RingRadius 480` (old feel).
