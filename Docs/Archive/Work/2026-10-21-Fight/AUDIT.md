# Enemy combat audit (2026-10-21, read-only survey; paths under Source/AI_Looter_Shooter unless noted)

## Stats
- Health/damage = base x (1 + 0.08(L-1)) x rank (Creatures/CreatureBaseRank.cpp:139-159, Progression/LevelRules.cpp:12).
  L1-7 scale 1.00/1.08/1.16/1.24/1.32/1.40/1.48. Hits roll +-10% (Combat/CombatRules.h:15). A creature hit launches the
  player 450 back, 180 up (Creatures/CreatureBaseAttack.cpp:99-101).
- Ranks (Creatures/CreatureRankSettings.cpp:66-74): health x1/2/3/3.5/4 (Regular, Restless, Gravebound, Hallowed,
  Ethereal), damage x1/1.15/1.3/1.45/1.6, defense 0/10/20/28/35%, level +0/+1/+1/+2/+2. RR upper-rank shares
  8/2/0.4/0.08% (:79-82). Docs/Areas/RansomsRest.md:348-352 (x2.5, x5) is stale.

| Kind (HP/dmg L1) | windup/recovery/cooldown s | reach | chase cm/s | aggro/pack call | source |
|---|---|---|---|---|---|
| Spider 300/12 | 0.55/0.45/1.1 (CreatureBase.h:318-324) | starts <=220, holds 165, strike 290 | 413 | 3120/none (Basic) | SpiderCreature.cpp:63-76 |
| Venom Spitter 200/10 | bite 0.5/0.45 cd 1.2; spit tell 0.9+0.5 | spit 300-2200, keeps 1000-1600 | 367 (back 294, strafe 147) | 3120 | SpitterSpider.cpp:61-81, .h:108-184 |
| Slime 120/6 | 0.5/1.4/1.6 | leap starts <=450, 500, hit r 140 | 220 (hops 200-260) | 2160/2500 | SlimeCreature.cpp:35-39, 85-101 |
| Unpaid 160/8 | 0.6 shriek/0.7/1.3 | lunge <=380, 320 at 1400 cm/s, r 130 | 360 | 3120/1500 | UnpaidCreature.cpp:82-102, .h:165-193 |
| Dead miner 180/9 | 0.8/0.9/1.2 | swing 250 | 329 | 3120/Unpaid pack | DeadMinerCreature.cpp:123-139 |
| Bell-Ringer 110/6 | swipe 0.55/0.7/1.0 only run down <=260 | holds 1100 | 306 (back 245) | 3120/Unpaid | BellRingerCreature.cpp:96-108, BellRingerRules.h:46-90 |

- Restless+ Unpaid wind up x0.75, lunge x1.35 (UnpaidRules.cpp:22-33); Gravebound+ shriek ring r 900 slows 50% 2 s,
  once per 5 s. Chase speeds x0.765 player-size factor (PlayerSize.h:30); player walk 459, sprint 711
  (PlayerLocomotionComponent.h:168, Core/LooterCharacter.cpp:50): every regular kind is slower than the walk.
- Aggro needs sight (CreatureBase.cpp:361-363), hears within 700 (EncounterSettings.h:110), hunts out to 8000; notice
  beat 0.42-0.62 s (CreatureReactRules.h:20-22). Bell toll every 7 s, tell 1.3, range 2400: raises <=2 or speeds x1.3 6 s.
- Cycle (windup+recovery+cooldown): spider 2.1 s, Unpaid 2.6, miner 2.9, slime 3.5.

## Coordination: none
- No attack tokens/slots/queue/orbit. Chase -> Attack when in start range and own cooldown clear (CreatureBase.cpp:267).
- Flanking = approach fan only: 20 deg per member, max 35, straight in within 450, pack radius 3000
  (EncounterSettings.h:65-76); re-ranked every 0.4 s (CreaturePackComponent.cpp:18, 124-178), offset by bearing
  (PackRules.cpp:27-70), bends the chase goal only (CreaturePackComponent.cpp:235-256); bosses/Legendaries excluded (:39).
- At the player: stop at 0.75 x attack range, face, attack when off cooldown; footprint spacing only stops overlap
  (CreatureBaseSpacing.cpp:81-145).
- Break-off: Basic spider/slime with pack gone, <=35% health, 50% chance, 2.5 s over 700 (EncounterSettings.h:87-97).
- Hurt calls pack: Unpaid 1500, slime 2500, Gravebound+ spiders 3000 (CreatureBaseHurt.cpp, CreatureRankSettings.cpp:85-88).

## Ranged
- Only the Spitter (regular). Starts a spit hunting if ready and target 300-2200 with a clear lob (6 arc pieces to 85%
  of flight, static only, cached 0.25 s: SpitterSpiderMoves.cpp:114-148); bites within 260 or cornered (0.4 s); spits
  every 3.0 s (2.4 Restless+), first 1.2 s after turning on, +-0.25 jitter (:30, :173).
- Glob: flight 0.55 + dist/2800 (0.7-1.45 s), gravity 1800 (SpitterRules.cpp:14-17); aims at the feet, leads 50%
  (:187-188); direct = bite x1, splash x0.5 r 160, puddle x0.2 per 0.5 s for 5 s r 130 (:196-210); Gravebound+ 3 globs
  260 apart; a sac crit during the tell chokes the spit.
- Keep-away (SpitterSpiderKeepAway.cpp:25-99): back off under 1000, strafe bursts 1-2.2 s (pauses 0.5-1.1) in 1000-1600;
  no cover, no group logic. Abel's buckshot via EnemyProjectileSubsystem (speed 1000).

## Encounters
- layout.json (Art/Levels/RansomsRest): camps :3140, patrols :3191, ambushes :3221, fights :3258. Group Count clamped
  1-16 (Creatures/EncounterGroup.h).
- Caps: 16 within 8000 of the player at spawn (EncounterSettings.h:41-45, EncounterSpawnerWaves.cpp:197-203); 12 alive
  Unpaid family incl. miners and ringers (EncounterSettings.cpp:11-14); spiders uncapped.
- Spawned creatures never respawn (CreatureBaseRank.cpp:276). Spawners despawn past 9000, activate at 4500, spawn >=800
  from the player (EncounterSpawnerWaves.cpp:339-379). Safe zones: Farm always, Main Street after Main 3
  (Tools/Unreal/build_area_story.py:575-590).

| Fight (levels) | Count |
|---|---|
| Bluff nest, Main 2 (1-2) | 4 + 1 Restless spider (build_area_story.py:468) |
| Town gate, Main 3 (2-3) | 3 + 1R Unpaid (:504) |
| Depot waiting room (2-3) | 3 + 1R Unpaid + Bell-Ringer (build_area_missionprops_valley.py:613) |
| Chapel yard, Main 4 (3-4) | wave 6, then 6 + 1R, waits for clear (build_area_chapel.py:71, 137): 13, 7 at once |
| Sink floor, Main 5 (4-5) | 4 spiders |
| Abel (5-6) | adds 2 per 25 s, max 4, then 2 waves of 4 (AbelRules.h:130-137) |
| Mountain (5-6) | QuarryCrew 4 + Abe; RaisingSteam 3 then 5 (22 s apart); CalderPost 4+1R; Watch 3; SpoilAmbush 3; MineUpper 4+1R; MineLower 4+1R+Bell; Foreman |
| Side: Whitlock hands | 5 + 1R Unpaid |
| Side: Tilly's cellar | 3 + 1R Unpaid |
| Camps (3-5) | SheepFold 3, OrchardEdge 5, MillCreek 4 slimes |
| Patrols | WestRoad 3 Unpaid, QuarryTrack 3 spiders + Spitter, CrestPatrol 3 |
| Ambushes | Churchyard 5, Webwood 5 (4 spiders + Spitter) |
| Roaming | Boot Hill 3, North Road 3, DinnerPail 5 |

## Player
- Health 100 x (1 + 0.08(L-1)): 100/108/116/124/132/140/148 at L1-7 (Progression/PlayerProgressionSubsystem.cpp:173-178).
- No shield (the hook serves creature wards only). Damage taken unscaled except ember powers (EmberIronHide.cpp:60).
- Regen after 6 s unhurt, ramps 1.5 s to 12% max/s (Combat/RecoverySettings.h:30-41). Soul-motes drop 12% Regular,
  25% Restless, 50% Gravebound+ per kill, heal 15% each (:50-71).
- Death: 2.5 s, then full health at the last roost or level start (PlayerVitalsSubsystem.cpp:133-179).

## Performance
- Update rate: <2500 every frame, to 6000 20 Hz, to 12000 10 Hz, beyond 5 Hz (CreatureUpdateRate.h:25-60); hunting,
  attacking, searching, targeted or hurt within 8 s = every frame (CreatureBaseUpdateRate.cpp:22-26).
- Grows with N: spacing O(N^2) every 0.1 s (CreatureBaseSpacing.cpp:97); pack look every 0.4 s; sight trace per hunter
  every 0.2 s beyond 700 (CreatureBasePursuit.cpp:168); steering sweeps every 0.1 s; Spitter 6 lob traces per 0.25 s;
  a health widget per hunter. ~0.14 ms game thread per Unpaid (Docs/Areas/RansomsRest.md:901). 8 fighting: 7.6-7.7 ms.
- `Looter.Perf.Horde <kind> <1-40> [rank] [x y yaw]` (Dev/CreatureDevCommands.cpp:352).

## Tests and settings
- Pinning: Looter.Creatures.Unpaid.Stats, DeadMiner.Stats, BellRinger.Stats, Spitter.Rules, Looter.Creatures.Ranks,
  Looter.Encounters.Caps (16, 8000, 12), Looter.Encounters.Packs.Flank (20/35), Looter.Progression.LevelHealth,
  Looter.Recovery.*, Looter.Creatures.UpdateRate, Looter.Creatures.AreaLevels.
- Will break as fights grow: Looter.Encounters.Camps.Placement (3-5 per camp, Tests/EncounterCampTests.cpp:67);
  Looter.Progression.Pacing (<=15 added main-path kills, story-alone 5.3-5.8: Tests/ProgressionPacingTests.cpp:429-457).
- Config/DefaultGame.ini overrides only the progression curve (:20-23). Only cvar: Looter.Creatures.UpdateRates.
  Settings: UEncounterSettings, UCreatureRankSettings, UProgressionSettings, FRecoverySettings, per-creature UPROPERTYs.
  Commands: Looter.SpawnCreature, KillNearest, CreatureHealth, Encounter.List/.Wave/.Zones/.Test.
