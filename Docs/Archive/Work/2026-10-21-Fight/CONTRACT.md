# Fight contract (Phase 12.2 enemies; Main, 2026-10-21 night)

The user's calls: difficulty = Borderlands normal (real damage, keep moving, a careless player dies now and then, a big
fight drains most of your health); smarter AI = packs surround and take turns + ranged enemies use their range; bigger
fights = 6-10 enemies in a normal fight + a few hand-made set-piece fights. Not wanted: dodging, waves, new mixed packs.
Gunplay just changed (Docs/Pipeline.md "Gunfeel: ..."): every hit flinches and briefly slows (55% for 0.12 s), staggers
are frequent, the shotgun knocks small creatures back. Read CLAUDE.md, this file, Docs/Handoff.md "Direction".

Today (audit 2026-10-21, numbers with sources in Saved/Fight/AUDIT.md): no attack coordination at all (each creature
attacks whenever in reach and off cooldown; flanking only bends the approach); every regular kind chases slower than the
player walks (spider 413 x0.765 player-size factor vs walk 459, sprint 711), so nothing catches a walking player; spider
12 dmg per 2.1 s; player 100-148 HP, regen 12%/s after 6 s unhurt, soul-motes heal 15%; biggest fight 7 at once; only
the Spitter is ranged (keeps 1000-1600, strafes, no group logic).

## Rules for every agent
Same as Saved/Gunfeel/CONTRACT.md "Rules for every agent" (read it): cvars for every feel number (`Looter.Fight.*`),
plain rules testable without a world, own files only, **no building/editor/tests/commits** (Main batches), compile-clean
code, bosses' moves never interrupted (bosses and Legendary monsters keep their own fights: no tokens, no ring),
perf: RR 8 spiders on Medium is 7.6 ms today, target under 8.3 with 10 fighting; CODEMAP lines; Pipeline section
"Fight: <yours>"; report under 300 words.

## FIGHT-AI (Opus): surround and take turns, ranged use range, difficulty numbers
Owns: new `Creatures/FightDirectorSubsystem.h/.cpp` (+ topic .cpp splits), new `Creatures/FightRules.h/.cpp`, new
`Creatures/FightTuning.h/.cpp` (cvars); `Creatures/CreatureBase*.cpp/.h` (attack gate, chase goal, speeds; NOT
CreatureBaseHurt.cpp's gunfeel lines), `Creatures/CreaturePackComponent.*`, `Creatures/PackRules.*`,
`Creatures/SpitterSpider*.cpp/.h`, `Creatures/SpitterRules.*`, `Creatures/UnpaidCreature*.cpp` (lunge gate only),
`Creatures/SlimeCreature*.cpp` (leap gate only), `Creatures/DeadMinerCreature*.cpp` (swing gate only),
`Creatures/BellRingerCreature*` (keep-back with the ring), `Combat/RecoverySettings.*`, `Combat/PlayerVitalsSubsystem.*`
(regen numbers only), new `Tests/FightTests.cpp`, and the existing tests of what it changes (list groups).
1. **Fight director** (world subsystem, no tick of its own unless needed; one per world): per target (the player),
   attack tokens: `Fight.MeleeTokens` 3 creatures may be in an attack (wind-up to recovery) at once, `Fight.RangedTokens`
   2 may be in a spit/shot tell. A creature asks before starting an attack (`TryTakeToken(Creature, Target, EFightRole)`),
   returns it when the attack's recovery ends, dies, is staggered/knocked or leaves reach (`ReleaseToken`); a token held
   past `Fight.TokenMaxSeconds` 3 is taken back. Fairness: the longest-waiting creature in reach gets the next token;
   a creature that just attacked waits `Fight.TokenRest` 0.6 s before asking again.
2. **Surround ring**: creatures hunting the same target without a token hold a spot on a ring round it
   (`Fight.RingRadius` 480 cm, outside melee reach), spread by bearing (each takes the free slot nearest its approach,
   slots `Fight.RingSlots` 8, re-evaluated every `Fight.RingRefresh` 0.4 s), drifting round the ring slowly
   (`Fight.RingDrift` 25 deg/s) so the circle feels alive, feinting in now and then (`Fight.FeintChance` 0.15 per
   refresh: a short step in and back) so waiting ones read as threatening, not queued. Prefer slots beside or behind the
   player's view over in front (`Fight.BehindBias` 0.35) so the player must turn. With a token, go in by the shortest
   way and attack. Ring spots must be walkable (the existing steering, ground and unstick rules); a spot that can't be
   reached is skipped. Melee packs with fewer than 3 hunting skip the ring (they just come in). Today's flank fan stays
   for the approach until the ring is reached.
3. **Ranged use range** (Spitter, any ranged kind): keep its band but choose a spot with a clear lob at the target,
   off the melee crowd's line (an angle `Fight.RangedOffAngle` 50 deg from where the melee pack stands), move to a new
   spot after each spit (`Fight.RangedRelocate` 0.6 chance), back off when the player closes inside 900, fire only with a
   ranged token, and stagger shots between spitters (the token does it). Lead stays 50%.
4. **Difficulty (Borderlands normal)**: every regular kind chases at `Fight.ChaseScale` 1.3 x today (spider ~537 > walk
   459 < sprint 711: a walking player gets caught, a sprinting one escapes); creature damage x `Fight.DamageScale` 1.5;
   player regen waits `Fight.RegenDelay` 7 s (was 6) and heals `Fight.RegenRate` 0.07 of max a second (was 0.12); soul
   motes unchanged. Bosses' own damage unchanged (their fights are tuned). Keep the attacks' tells readable (no shorter
   wind-ups).
Tests `Looter.Fight.*`: tokens (count, release on death/stagger, fairness, timeout), ring slot choice and spread, ranged
spot choice, the scales.

## FIGHT-LAYOUT (Opus): bigger fights and set pieces
Owns: `Art/Levels/RansomsRest/layout.json` (fights, camps, patrols, ambushes sections only), `Tools/Unreal/build_area_*.py`
(the spawner placements: story, chapel, camps, mine, mission props), `Creatures/EncounterSettings.*` (caps),
`Creatures/AmbushSpawner.*`, `Creatures/EncounterSpawner*.cpp/.h`, `Creatures/PatrolSpawner.*`, new
`Creatures/SetPiece*.h/.cpp` if a set piece needs code, `Docs/Areas/RansomsRest.md` (fight tables, the stale rank
numbers: Restless x2 health, Gravebound x3, Hallowed x3.5, Ethereal x4), `Docs/Progression.md` (pacing), the progression
and encounter tests (`Tests/EncounterCampTests.cpp`, `Tests/ProgressionPacingTests.cpp`, encounter tests).
1. **6-10 in a normal fight**: raise every main-path and side fight to 6-10 creatures (camps and patrols 5-8, the
   chapel yard and mine fights at the top), keeping each area's kinds and ranks; caps: near-player 16 -> 20, the Unpaid
   family 12 -> 16 (`EncounterSettings`). Spread spawns so a fight comes from 2+ directions where the ground allows.
2. **Set pieces**, three in Ransom's Rest, made from the existing spawners/entrances (`AAmbushSpawner` appear/rise/drop,
   waves only inside a set piece), each with a beat that announces it (a sound sting through an existing or new cue name,
   a camera shake via `UCameraShakeModifier`, a caption line through the existing caption system) and a reward at its
   end (a Strongbox chest where it ends, or a loot shower like a boss's but smaller): (a) **Main Street ambush** after
   the town gate fight (Main 3): the Unpaid rise from the boardwalk's shadows and alleys, 10 in two pushes with a
   Bell-Ringer on the saloon balcony; (b) **Webwood drop**: spiders drop from the trees round the player (8) with two
   Spitters on the high ground; (c) **Providence cave-in** in the lower mine: the gallery shakes, dust falls, dead
   miners dig out of the walls (10) behind and ahead. Each runs once per session (saved like the other one-time fights).
   Respect safe zones (Main Street is safe only after Main 3: the ambush happens during/at the end of Main 3 before the
   zone turns on; say how).
3. **Pacing**: more kills give more XP. Keep the user's curve and kill XP rule (Docs/Progression.md) and keep the
   story-alone level 5.3-5.8 and the everything-once target by trimming mission XP where needed; update the pacing
   test's kill bounds with the reason. Check `Looter.Economy.Balance`'s silver too and say how it moved.
Main runs the area build (`build_area.py RansomsRest gameplay` etc.) and checks in PIE; write the run order in your
report.
