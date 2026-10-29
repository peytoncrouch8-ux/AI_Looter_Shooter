# FIGHTS: smarter, bigger fights in Ransom's Rest (plan; Opus planning agent, 2026-10-28; read-only, nothing built)

The user's complaint (grill 2026-10-21; 10-27 notes 23/39 "terrible AI", "not frightening enough"): enemies too easy, dumb
AI, fights too small (variety NOT the problem). Pick 2026-10-28: "Smarter, bigger fights". Rules kept: levels from the
area's zones (RR band 1-7, zones 1-2 bluff, 2-4 town/fields, 4-5 Sink, 5-6 Webwood/quarry/deck/mountain), never the
player's; no new creature types (slimelets below are a configuration like spiderlings: BodyScale + HealthScale); no plain
waves, no new mixed packs, set pieces announced + rewarded + once a session (10-21); the first three fights stay soft
(10-23 "Soften early only"); spiders: the user said "leave them" on 10-28 (balance) -> Q1.

## 0. What exists (read from code; numbers are defaults)

- Director `Creatures/FightDirectorSubsystem` (+`FightDirectorPlaces.cpp`), rules `FightRules`, cvars `FightTuning`
  (`Looter.Fight.*`): MeleeTokens 3, RangedTokens 2, TokenRest 0.6, TokenMaxSeconds 3, TurnKeep 0.5, RangedGap 0.35, ring
  RingRadius 480 / RingMargin 80 / RingSlots 8 / RingRefresh 0.4 / RingDrift 25 / FeintChance 0.15 / BehindBias 0.35 /
  RingMinPack 3 / RingApproach 1.6; ranged RangedOffAngle 50, RangedRelocate 0.6, RangedBackOff 900; difficulty ChaseScale
  1.3, **DamageScale 1.8** (Pipeline/Enemies.md still says 1.5: fix the doc), RegenDelay 7, RegenRate 0.07.
- Chase/turn code `CreatureBaseFight.cpp` `TickChaseFight`; attack frame `CreatureBaseAttack.cpp` `TickAttack`/`Strike`
  (`IsStrikeClear`, dot 0.4 front check); damage `HitWithAttack` = AttackDamage x Strength x `GetFightDamageScale`.
- Packs `CreaturePackComponent` + `PackRules`: flank fan (FlankStepDegrees 20, max 35, release 450), break-off only for a
  hurt (<=35%) Basic spider/slime whose pack is GONE (chance 0.5, 2.5 s, 700 cm). No leader concept anywhere.
- Ranks `CreatureRankSettings.cpp`: Restless health x2, dmg x1.15, +1 level, Defense 0.1, 1 trait; Gravebound x3 / x1.3 /
  0.2; Hallowed x3.5/x1.45/0.28 (2 traits); Ethereal x4/x1.6/0.35. Traits incl. **Rallying** (8 m: x0.85 taken, x1.1 move,
  ring + tethers + 6 s pulse) = a ready-made "leader" look.
- Spawners: `AEncounterSpawner` (`NumWaves`, `WaveInterval`, `bWaitForClear`, `MaxAlive`, `MaxTotal`), `FEncounterGroup`
  (Count, RankRoll Area/Chances/Fixed, Level 0 = zone, BodyScale, HealthScale, First/LastWave, MaxAliveOfClass),
  `ASetPieceSpawner` (pushes `FSetPieceBeat`, `pushWhenLeft`/`pushAfter`, placed spots, hush, loot shower, DoneFlag),
  `AAmbushSpawner`, `APatrolSpawner`. Caps `UEncounterSettings`: 20 within 80 m, Unpaid family 16.
- Kinds (level 1 class numbers; growth x(1+0.08(L-1)) on health and damage, `FLevelRules::EnemyScale`):
  - Spider 230 HP, bite 11, range 220, windup 0.55 / recovery 0.45 / cooldown 1.1, chase 540 x MoveScale x1.3. Bite only.
  - Spitter 200 HP, bite 10, lob every ~3 s with a tell, puddle; Gravebound throws 3. Ranged role, keeps a band.
  - Slime 120 HP, 6 dmg, leap range 450, windup 0.5 / recovery 1.4 / cooldown 1.6, pack aggro 2500.
  - Unpaid 160 HP, 8 dmg, range 380, windup 0.6 / recovery 0.7 / cooldown 1.3, full kit (`UnpaidFightTuning`, 47
    `Looter.Unpaid.*`): lead lunge (start 520, flies 470, hit radius 100), rake (260, 70%, 2 x 0.5), flank (0.4, cd 7),
    cinder (gap 2 s/target, 0.4x), rally -> 4 s frenzy at 4 tokens, howl, press past edge, sight rules, PackStepGap 1.2,
    ArrivalHold 0.6. Bell-Ringer (support, 110 HP, toll every 7 s), dead miners (no kit).
- Player: 100 HP x(1+0.08(L-1)) + ward (lvl 1 ~15, lvl 3 ~41). Common rifle `FWeaponStats` 20 dmg, 600 rpm, mag 30, reload
  2 s: 200 DPS paper, ~120 sustained; x1.08/level. Crit x1.5 (`LooterCombat::CriticalHitMultiplier`).
- Measured (overnight playtest + 10-28 DEMO): spider pack kills a still lvl-3 player (157 EHP) in 3.2 s (3 bites/s at
  ~23); Unpaid yard: still player dead 12-20 s, nave floor 49 s, dais 23 s; kiting both yard waves costs ~half health.
- Melee pressure on the player is set by the turn throughput: **DPS_in ~= min(tokens, n) x hit x land% / (windup +
  recovery)** (+ TokenRest only if no waiter). Tokens, not pack size, decide lethality; size decides how long it lasts.

## 1. Difficulty targets (level-appropriate player: level = zone level, Uncommon zone-level rifle, 60% on target)

| Measure | Target | Now |
|---|---|---|
| TTK Basic (body, 60%) | spider 2.0-2.5 s, Unpaid 1.6-2.2, slime 1.0-1.5, Spitter 2.0-2.5 | spider ~1.9, Unpaid ~1.3 (L3) |
| TTK Restless / Gravebound | 4-6 s / 7-10 s | ~4.2 / ~7 (spider) |
| Still player, open ground, full pack (6+), dies in | 8-12 s, every kind | spiders 3-4, Unpaid 12-20 (nave 49) |
| Moving + shooting, normal fight (6-10) | ends at 30-70% EHP, 40-90 s | not measured per kind |
| Set piece (12-18 over pushes) | lowest point 10-35% EHP once; <=1 death in 3 tries (DEMO bot) | Main Street only checked |
| Attacks landed on a strafing player | 25-40% (melee), 20-35% (ranged) | Unpaid 25%, spiders ~90% |
- If Unpaid TTK stays under 1.5 s after the kit work: `AUnpaidCreature` MaxHealth 160 -> 190 (Abel is safe: his
  `UnpaidHealth` is a constant in AbelKeeper.cpp; the Ledger reads the class).
- Spider lethality is reached by a per-target bite gap (lane 2) not by lowering the bite (Q1).

## 2. Shared fight-director work (lane DIRECTOR)

New cvars in `FightTuning` (defaults in brackets):
- **Melee gap** `Looter.Fight.MeleeGap` [0 = off]; per kind `ACreatureBase::GetMeleeGap()` virtual (spider 0.9 s, slime
  0.45, Unpaid 0, placeholders 0): melee turns of one kind family at one target begin at least that far apart (like
  RangedGap; `FFightTokens` gains a per-family last-start). Spider still-player TTD at L3: 157/(23 x 0.95 / 0.9) ~ 6.5 s
  -> with the pounce misses and skitters ~8 s.
- **Player read** `FFightPlayerRead` (new `FightPressureRules.h`), refreshed each RingRefresh per target: `StillFor` (moved
  <150 cm flat in the last `StillSeconds` [2.5]; 8-sample ring buffer of positions), `HiddenFor` (no fighter has a clear
  eye line for `HiddenSeconds` [2]; reuse the Unpaid's `SeesTarget` cache, else 1 `StaticGeometryParams` trace per fighter
  per refresh, max 12), `HealthShare` (health + ward). `UFightDirectorSubsystem::GetPlayerRead(const APawn&)`.
  - `IsCamping()` = StillFor >= 2.5 or HiddenFor >= 2 while 3+ fight them. Kinds answer it (lanes 3-5).
  - **Close call ease**: EHP < 25% -> melee and ranged tokens -1 (min 1) for `EaseSeconds` [4], once per `EaseGap` [20] s
    (`Looter.Fight.Ease` [1]). Invisible; makes near-deaths instead of deaths (player enjoyment first).
- **Pack leader** (the "elite per fight"): `FEncounterGroup::bLeader` (Phase 0) marks the group's first creature;
  `UCreaturePackComponent::SetLeader/IsLeader`; a leader always wears **Rallying** (added to its rolled traits via
  `UCreatureTraitComponent::SetTraits`; its tethers show who leads) and its pack tag counts as led.
  - Leader down (`PackLeaderRules::OnLeaderDown`, called from the leader's death): packmates within `LeaderReach` [2000]
    falter `LeaderFalter` [1.0 s] (`HoldBrain`, turns given back), Basic spiders/slimes roll break-off at 0.35 (existing
    `StartRetreat`, made reachable through a public `BreakOff()`), Unpaid lose rally/frenzy for 16 s; cue
    `Fight.LeaderDown` (2D sting) and the falter; no slow-mo (hit-stop is HITS').
- Events for the log: `EFightEvent` (Phase 0 header `Creatures/FightEvent.h`: TurnTaken, AttackStarted, AttackLanded,
  Pounce, Skitter, Flush, LeaderDown, Split, Dodge, FadeBack, Ease) via `UFightDirectorSubsystem::NoteEvent(const
  ACreatureBase&, EFightEvent, float Value = 0)` and `FOnFightEvent OnFightEvent` (native multicast, no cost unbound).

## 3. Per kind: new behaviours (each a cvar, kit-gated, needs the director; `0` = today's)

### Spiders (Meadow Wolf; lane BEASTS) - from "a ring of biters" to a hunting pack
- **Pounce** `Looter.Spider.Pounce` [1]: holding a melee turn at 450-800 cm with a clear line (`IsStrikeClear` + landing
  ground within 60 cm of the target's floor, same hunting ground), chance 0.3 a turn (0.6 while `IsCamping`), cooldown 6 s
  per spider, one pounce per target per 1.5 s (director gate, generic `MayLeap`/`NoteLeap` like `MayStep`). Tell 0.6 s
  (crouch 15 cm, front legs up, fangs flared, `Creature.Spider.Pounce` rising hiss); leap 0.45 s ballistic, apex 120 cm,
  aimed at the lead point (LeadShare 0.2, max 0.15 s, fixed at the tell's end); lands: hit if target within 110 cm ->
  1.2x bite + shove; miss -> 0.9 s sprawl (no turn, crit on head x1.5 as always: the window). Never off a drop > 150 cm.
- **Hit-and-run** `Looter.Spider.Skitter` [0.7 chance, Restless+ 1.0]: after its bite's recovery it skitters 300 cm back
  (1.6x chase, 0.5 s max, facing the player, `Creature.Spider.Skitter` tick) before its ring slot: no face-hugging.
- **Behind bias** `Looter.Spider.BehindBias` [0.6] (ring slots off the player's view; Fight's 0.35 stays for others).
- Bite numbers unchanged (11, 0.55/0.45/1.1) unless Q1 says otherwise. Melee gap 0.9 s (section 2).
### Venom Spitter (lane BEASTS)
- **Flush lob** `Looter.Spitter.Flush` [1]: target `HiddenFor` >= 2 s or `StillFor` >= 2.5 s within its band: its next
  ranged turn lobs at the last seen spot +150 cm on the far side from it, high arc (apex +300 cm; `SpitterRules::LobVelocity`
  with a height arg, path checked in 3 segments), puddle as now. Tell unchanged (the sac shot still chokes it). Cooldown 8 s.
- **Height** `Looter.Spitter.HighBonus` [0.5]: ranged spots score -0.5 cost per 150 cm above the target (cap 450),
  6 tries (was 4) in `PlaceRanged`/`IsRangedSpotUsable` (+2 traces a refresh for a spitter only).
### Meadow slime (lane SLIME) - wildlife, light touch
- **Split** `Looter.Slime.Split` [1]: a Restless+ slime dies into 2 slimelets (BodyScale 0.55, HealthScale 0.3, damage
  x0.5, XP x0.25, Basic, the Basic loot table like spiderlings), thrown 150 cm apart with a gel burst (`Creature.Slime.Split`);
  only under the caps (`KeepUnderCaps`), else it just dies. Slimelets never split.
- **Drumbeat** melee gap 0.45 s (section 2): the pack's leaps land one after another, a readable rhythm.
- **Dodge hop** `Looter.Slime.Dodge` [0.5]: 3+ hits in 0.8 s while waiting on the ring -> a 250 cm side hop (not mid-attack,
  cooldown 4 s, `NoteEvent Dodge`).
### The Unpaid (lane UNPAID) - the kit exists; make it read as a pack with a head
- **Leader's rally**: in a led pack only the leader screams the rally (`RallyCooldown` 16 -> `LeaderRallyCooldown` 12);
  leaderless packs as now. Leader down: frenzy ends at once, no rally 16 s, packmates' coals gutter (TellFlare 0.2 for 1 s).
- **Fade-back** `Looter.Unpaid.FadeBack` [0.5]: a Basic under 35% after its turn, once a life: phase-steps 500-650 cm AWAY
  (`ChoosePhaseSpot` with the away bearing; counts against `PackStepGap`), waits 2 s at the ring's outer edge, returns.
- **Punish camping** (`IsCamping`): lunge lead off (a still target needs none), `LungeHitRadius` +20, `CinderGap` x0.6,
  `FlankChance` 0.4 -> 0.6. Moving players see today's numbers.
- **Dais fix** (open from 10-28): `ArrivalHold` 0.8 s for landings within 150 cm (was the idea; 0.6 elsewhere).
- Split `UnpaidCreature.h` (544 lines) first: the kit's members into `UnpaidKitMembers.h` (`FUnpaidKitMembers` struct
  held by value; no UPROPERTYs move). Bell-Ringer, dead miners, Abel/boss adds: leader rules only, no kit change.

## 4. Encounters by place (lane ENCOUNTERS; levels are the zones'; at-once <= 9 + placed <= 1)

| Fight | When / lvl | Now | New |
|---|---|---|---|
| Bluff nest | Main 2 / 1-2 | 4+1R, 2 out, health 0.7/0.6 | unchanged (soft); the Restless is leader (teaches the tethers) and the only one that pounces (teaches the tell) |
| Town gate | Main 3.1 / 2 | 6+1R, 4 out | unchanged counts; Restless = leader; Hob's caption once at its death ("Their loud one's down. Watch 'em falter.") |
| Main Street (set piece) | Main 3.2 / 2-3 | 4+BR, then 2+1R | unchanged (soft); 2nd push's Restless = leader |
| **Chapel yard -> set piece "Saint Ada's Yard"** | Main 4.1 / 3-4 | waves 8, then 7+1R | ARENA (Q2): 3 pushes 6 / 5+1R leader / 4+1 Gravebound leader (DisplayName "the Sexton's Debt"), Bell-Ringer placed on the vestry steps from push 2; 17 + 1; max alive 9; pushWhenLeft 3, pushAfter 35 s; DoneFlag `SetPiece.ChapelYard`; shower 2 guns. ~150 s |
| Boot hill / north road | after Main 4 / 3-4 | 5 each (area) | 6 each: 5 + 1R leader |
| **Whitlock's hands -> set piece "Barn Doors"** | Side 2 / 3-4 | 7+1R | push 1: 5 + 1R leader in the yard; push 2 (3 left or 30 s): 4 out of the barn doors + a Bell-Ringer placed in the hay-loft door (height, 4 m up); 11; DoneFlag `SetPiece.BarnDoors`; arena optional (Q2) |
| Sink floor | Main 5 / 4-5 | 6 spiders (area) + 6 sacs | 7 + 1R leader + 2 Spitters placed on the collapsed blocks (height); sacs unchanged. Same family as QuarryTrack/Webwood: not a new mix |
| Webwood (set piece) | after Main 4 / 5-6 | 4+1R, then 3; 2 Spitters high | 3rd push: 3 + a Gravebound leader dropping from the canopy (Hob: "Big one in the branches, sunshine!"); 13 |
| Camp SheepFold | after Main 4 | 5+1R | 6 + 1R leader |
| Camp OrchardEdge (+Dusk) | after Main 3 | 5+1R+BR | unchanged; Restless = leader |
| Camp MillCreek (slimes) | after Main 3 / 2-3 | 5+1R | 6 + 1R leader (splits: 2 slimelets) |
| Patrol WestRoad / QuarryTrack | after Main 4 / 3 | 5 / 5+Sp | WestRoad 6 (5 + 1R leader, the patrol's head); QuarryTrack unchanged |
| Churchyard ambush, depot, cellar | | | unchanged counts (rooms are tight); leaders on their Restless |
| Mountain (quarry, Calder's Post, Dinner Pail, mine, cave-in) | 5-6 | 5-10 | counts unchanged (already the biggest); leaders on every Restless; Gravebound leader on MineLower |
- Main path gains ~+11 kills (yard +2, boot/north +2, Sink +3, gate/MainStreet 0, Webwood +3 optional, barn +3 side).
  XP: run `Looter.Progression.Pacing` (story ~6.9, full clear ~7.5) and `Looter.Economy.Balance` (1,000-1,800); trim mission
  XP in `create_mission_assets.py` / `create_side_mission_assets.py`, never kill XP rules.
- Where: `build_area_chapel.py` (YARD_* -> a set piece via `build_area_fights_mountain.place_one`-style call or
  `build_area_setpieces.py` keys), `build_area_whitlock.py` (HANDS_*), `build_area_sink.py` (FLOOR_COUNT 7 + spitter spots),
  `build_area_camps.py` + `layout.json` gameplay.encounters (camps, patrols, Webwood's 3rd push, `leader` key per group)
  through a patch `Art/Levels/RansomsRest/patches/fights.json` (FINDS may own layout.json at the same time; Workflow.md).
- **Arena** (`ASetPieceSpawner`): new `ArenaGates` (TArray of from/to pairs) -> one `ABossSeal` Gate shape each across the
  yard's openings, raised at the first push, dropped when done, on the player's death or the set piece's reset; the press
  rule is off while sealed (`NotePressed` ignored: they can't stand past the edge). An ammo crate (T1) stands inside.

## 5. Measuring (lane MEASURE goes first: baseline before any change)

- `Dev/FightLog.h/.cpp` `UFightLogSubsystem` (world subsystem; dev only, `Looter.Fight.Log 0|1|report|reset` [0]): binds the
  player's health/ward damage (by causer class and `EFightEvent`), `OnFightEvent`, creature deaths. Report (log + CSV
  `Saved/FightLog/<time>.csv`): fight length (first notice -> last death), damage in by kind/move, DPS-in peak (5 s window),
  lowest EHP share, deaths, attacks started/landed by move, % landed, TTK median by kind+rank (first hit -> death), peak
  alive, peak tokens, counts of pounce/flush/leader/split/fade/ease.
- `Dev/FightSimDevCommands.cpp`: `Looter.Fight.Sim <Kind> <Count> [Rank] [Level] [Leader 0|1] [Spitters n]`: sets the
  player to Level (progression subsystem, health reapplied), gives an Uncommon rifle of Level (`Looter.GiveWeapon` path),
  fills ammo, God off, spawns the pack in town at (0, -1400, 90) at Level (not the farm: safe zone), resets + starts the log.
  `Looter.Fight.Sim.Place <SpawnerId>` = `Looter.Encounter.Wave <id> force` with the log.
- `Tools/Unreal/play.py fight <s> <range m> [aim 0-1] [still|strafe|kite]`: aim misses by an offset cone at (1-aim); strafe
  A/D 1.5 s; kite backpedal + strafe. DEMO's matrix: spider/Unpaid/slime/Spitter+spiders x L1/L3/L5 x still/strafe at 0.6,
  plus each changed place in PIE (Main 2-5, Side 2, Webwood). Results into this folder's `RESULTS.md`.
- `Creatures/FightPaceRules.h/.cpp` + `Tests/FightPaceTests.cpp` (`Looter.Fight.Pace`): the throughput model of section 0
  over each kind's class defaults and cvars; asserts still-player TTD 6-14 s for 6 of a kind at levels 1/3/5 and Basic TTK
  bands vs `FWeaponStats` defaults: catches anyone (GUNS too) moving the numbers out of band.

## 6. Lanes (disjoint files; Main does Phase 0, builds once per round, runs only touched tests, commits)

Phase 0 (Main, the contract commit): `Creatures/FightEvent.h` (enum); `FightDirectorSubsystem.h` declarations (`GetPlayerRead`,
`NoteEvent`, `OnFightEvent`, `MayLeap`/`NoteLeap`, melee gap) with stub bodies; `ACreatureBase` virtuals in CreatureBase.h:
`GetMeleeGap()`, `StartsRingMove(const APawn&, float Distance)` (called in `TickChaseFight` when it holds a turn, before the
plain attack), `OnTurnPlayedOut()` (from `TickAttack`'s end, before `ResumeHunt`), `OnPackLeaderDown(const ACreatureBase&)`;
`FEncounterGroup::bLeader`. After Phase 0 no lane edits CreatureBase*.cpp/h.

| Lane | Model | Files (only these) | Tests |
|---|---|---|---|
| 1 MEASURE | Sonnet | Dev/FightLog.*, Dev/FightSimDevCommands.cpp, Creatures/FightPaceRules.*, Tests/FightPaceTests.cpp, Tools/Unreal/play.py | `Looter.Fight.Pace` |
| 2 DIRECTOR | Opus | FightDirectorSubsystem.cpp, new FightDirectorPressure.cpp, FightRules.* (gap in FFightTokens), FightTuning.*, new FightPressureRules.*, new PackLeaderRules.*, CreaturePackComponent.*, Tests/FightPressureTests.cpp | `Looter.Fight.Pressure`, `.Leader`, `.Gap`; rerun `Looter.Fight` |
| 3 BEASTS | Opus | SpiderCreature.h, new SpiderCreaturePounce.cpp, new SpiderFightRules.* (+`Looter.Spider.*`), SpitterSpider.h, SpitterSpiderKeepAway.cpp, SpitterSpiderMoves.cpp, SpitterRules.*, FightDirectorPlaces.cpp (height score only), Tests/SpiderFightTests.cpp, Tests/SpitterFlushTests.cpp | `Looter.Creatures.Spider.Fight`, `Looter.Creatures.Spitter.Flush` |
| 4 SLIME | Sonnet | SlimeCreature.h/.cpp, new SlimeCreatureSplit.cpp, new SlimeFightRules.*, Tests/SlimeFightTests.cpp | `Looter.Creatures.Slime.Fight` |
| 5 UNPAID | Opus | UnpaidCreature.h (split first), new UnpaidKitMembers.h, UnpaidCreatureMoves/Ring/Phase.cpp, new UnpaidCreatureFadeBack.cpp, UnpaidFightRules.*, UnpaidFightTuning.*, Tests/UnpaidFadeBackTests.cpp | `Looter.Creatures.Unpaid.Kit` (all) |
| 6 ENCOUNTERS | Sonnet | SetPieceSpawner.h, new SetPieceSpawnerArena.cpp, SetPieceRules.*, EncounterSpawnerWaves.cpp (leader mark), the build_area_*.py above, build_area_setpieces.py, patches/fights.json, Tests/SetPieceArenaTests.cpp | `Looter.SetPiece`, `Looter.Encounter`, `Looter.Progression.Pacing`, `Looter.Economy.Balance` |
| 7 SOUNDS | Sonnet | new Art/Sounds/recipes/fight_kit.py (`Creature.Spider.Pounce`, `.Skitter`, `Creature.Slime.Split`, `Fight.LeaderDown`, `Creature.Spitter.Flush` whoosh); Main renders only these (`Tools\sounds.ps1 fight_kit`) + `build_sound_bank.py` | cue check by hand vs cues.json |
| 8 DEMO | Sonnet | none (RESULTS.md only); owns the editor while it plays (SendMessage Main first) | baseline, then the matrix + PIE per place |

- Order: Phase 0 -> lane 1 -> DEMO baseline -> lanes 2-7 in parallel -> one build -> lane tests -> DEMO matrix -> cvar
  tuning by DEMO (defaults only into the tuning files, Main edits) -> Main runs the area build passes -> DEMO walks Main 2-5,
  Side 2, Webwood -> perf -> docs (Pipeline/Enemies.md new section "Fight: pack brains", RansomsRest.md fights table,
  Progression.md, CODEMAP lines, Handoff).
- Hands off HITS' files (`CreatureHitReactionComponent`, `CreatureKnockComponent`, hit-stop) and GUNS' (Weapons/, Combat/
  BulletSubsystem); FIGHTS only reads `IsHeldByHit`. Rerun `Looter.Fight.Pace` after GUNS lands (DPS moves TTK).
- PIE checks per lane (DEMO, God off, aim 0.6): spider pounce tell readable at 8 m and dodgeable by a strafe; skitter seen;
  Spitter puddle lands behind the chapel wall when the player hides; slime split + drumbeat at Mill Creek; Unpaid fade-back
  and leader-only rally in the yard; arena walls rise/drop (and drop on death); leader tethers + falter at the gate.

## 7. Performance (120 fps 1080p Medium, RX 580; target 8.3 ms)

- Today: 10 spiders fighting in town 7.5-7.7 ms (game thread 6.4-6.9 incl. ~1 ms waiting), 8 Unpaid 7.7 (p95 9.4), RR Medium
  7.6 / p95 7.9; director < 0.05 ms, ring facing 0.3 ms for 10. Margin ~0.6 ms, mostly game thread.
- Rule: at most 10 fighting at once (MaxAlive 9 + 1 placed); bigger fights come from pushes, never more bodies at once.
  Slimelets count toward MaxAlive and the caps (a denied split just dies). Caps 20 / Unpaid 16 unchanged.
- New per-creature costs must stay <= 0.01 ms avg: pounce search 3 traces once a turn; player read <= 12 traces per 0.4 s
  per target (reuse cached sight); spitter +2 traces a refresh; no new ticking components; sounds pooled.
- Seal: `ABossSeal` instanced fog cards (translucent): measure in the yard.
- Runs (editor closed): `Tools\perf.ps1 -Label "Fights before" -Exec "Looter.Quality Medium"` with `Looter.Perf.Horde Unpaid 10
  Basic 0 -1400 90` and `Looter.Fight.Sim Spider 8 Basic 4 1 2`; same after; plus the yard set piece in PIE (`stat unit`).
  Pass: avg <= 8.3 ms, p95 <= 9.5; if over, MaxAlive 8 in the yard.

## 8. Risks

- Spiders: the user's 10-28 "leave the spiders" (Q1); their 3-4 s kills are the opposite of "too easy": measure first.
- The "too easy" note predates DamageScale 1.8, the director and the Unpaid kit (none played by the user since): the DEMO
  baseline decides how much to change; don't stack difficulty on top blindly.
- Arenas feel like boss fights and can trap a player low on ammo (crate inside, shower's ammo, drop rules); a death drops
  the walls and the set piece resets (once a session still holds: not done until won).
- XP/silver pacing tests will fail with more kills (fix in mission XP); bestiary pages: none new (slimelets share the slime's).
- Tests that tick a spider/Unpaid 4.5-8 m from a player may now see a pounce or fade-back: new moves need the director, which
  test levels lack unless `bAllowInTestWorlds`; cvars `0` restore today.
- Unpaid header split is mechanical but touches every Unpaid .cpp include: lane 5 alone.
- layout.json shared with FINDS: patches only. `Looter.Fight.MeleeGap` changes every regular kind's pressure: the Pace test
  pins it.

## 9. Questions for the user (tap choices; recommendation first)

1. Spiders (you said "leave the spiders" on 10-28; today a pack kills a standing player in 3-4 s):
   a) New moves (pounce, hit-and-run) and one bite at a time, so a pack kills a standing player in ~8 s (recommended)
   b) New moves, keep today's 3-4 s lethality  c) Leave spiders exactly as they are
2. Arena fights (grave-fog walls close the way out until it's won):
   a) Chapel yard only, as the big Main 4 set piece (recommended)  b) Chapel yard and Whitlock's barn  c) No walls anywhere
3. How hard (a level-matched player):
   a) Borderlands-normal: standing still kills you in ~8-12 s, a moving player ends fights at 30-70% health (recommended)
   b) Harder: standing still kills in ~5-7 s, set pieces usually cost a death the first time  c) Measure first, then decide
