# Progression: experience and pacing

How the player earns experience, and where Ransom's Rest's story leaves them. The user's call (2026-10-08): the story
should end about **level 6-7**, not 9-10; no experience for a mission's steps; missions are **turned in**, not finished
the moment their last objective is done.

## Where experience comes from

Only two places, nothing else (checked by `Looter.Missions.TurnIn.Rewards`):

- **Kills.** `FLevelRules::KillXP`: the creature's `XPReward` (10, times its rank's multiplier: Restless ×2,
  Gravebound ×4, Soulfed ×10, Boss ×20) × the **kill scale** (0.4) × 1.08 per level of the creature above 1, less 15
  points a level when it's below the player (never under 10%). A level 1 Basic kill gives 4; one at level 5 gives 5.
  Skyreach (practice) gives none.
- **Missions turned in.** Each story mission has a fixed amount (`FMissionRewards::Experience`), given only when it's
  turned in to its giver, never for its objectives or steps. Skyreach's tutorial and skiff give none.

The knobs: `KillXPScale`, `KillXPGrowth`, the falloff (Project Settings > Game > Progression, `UProgressionSettings`),
and each mission's `experience` in `Tools/Unreal/create_mission_assets.py` / `create_side_mission_assets.py`. The curve
stays the user's: 100 XP for level 2, then 12% more a level. **Level 6 starts at 634 XP in all, 7 at 810, 8 at 1007.**

## Turn-ins (Borderlands' way)

A mission whose objectives are all done is **ready to turn in**: the tracker says "Turn in to <giver>" under a full step
bar, its arrow and the minimap's waypoint on the giver; the Missions page says "Ready to turn in: talk to <giver>". The
giver's Interact prompt says "Turn in". Talking to them finishes it: the experience, the reward gun dropped in front of
the player, the areas opened, the HUD's MISSION COMPLETE banner (its fanfare, then a level-up banner if one came).
The state is saved (`FCampaignRecord::ReadyMissions`).

| Mission | Turned in to | How |
|---|---|---|
| Main 1, Seven Days | Delia, at her screen door | her Main 1 lines (that talk was its last step) |
| Main 2, Shall We Talk Business? | Mister Sexton, on the rail | after the Ledger is read; a line of its own |
| Main 3, Cold Welcome | Tilly, at her window | her Main 3 lines (that talk was its last step); the Uncommon gun |
| Main 4, Hallowed Ground | Father Aldana, at the vestry door | his Main 4 lines (that talk was its last step) |
| Main 5, The Keeper's Lantern | Father Aldana | he sent Ellis to the Sink; his line sends Ellis home to Delia, whose word starts Main 6 |
| Main 6, The Gravewind | nobody: finishes as Pa sits down | its scene is the hand-in; Pa's board, the lit lantern and the train's steam follow it finished |
| Main 7, The Lantern Leans | Tilly, at her window | on the way to the depot (her father's car); the Lily opens on the board as it's turned in |
| Side 1, Wanted: Already Dead | Tilly, at her window | a line of its own; the Rare gun |
| Side 2, Unfinished Business | Amos, at his fence | his thanks (that talk was its last step); the Epic gun |
| Side 3, The Gravemother | nobody: finishes as she dies | a hunt nobody asked for |

Where the giver's talk used to be the last objective, the steps end one earlier and that talk is the turn-in; the
ready state's step is the old talk's step, so the story's "from step N" conditions (the giver's lines, Hob's perch)
are unchanged. What waits for "turned in": the next mission (prerequisites), rewards, respawn graves, safe zones,
shutters, Amos, the Ledger's pages, and everything else that reads "after Mission N".

## Ransom's Rest's story, counted

A normal play: every fight on the main path, both side missions, and a few roaming creatures (boot hill's and the north
road's six Unpaid and the west road's three walkers, once). Since 2026-10-08 the main path also meets two of the
encounters between the fights (below): the churchyard's dead rising on the way back to Father Aldana with the lantern,
and the sheep fold's camp on the way to the deck. Kills are averaged over the area's level roll (the player's level, one
either side) and its promotions (8% Restless, 2% Gravebound), as the game rolls them. `Looter.Progression.Pacing`
recomputes this table from the mission assets, the creatures' own numbers and the layout's encounters, and fails if the
end leaves level 6.

| After | Kills on the way | Kill XP | Turned in | XP in all | Level |
|---|---|---:|---:|---:|---:|
| Main 1, Seven Days | none | 0 | 20 | 20 | 1.20 |
| Main 2 | the bluff nest: 4 spiders, 1 Restless | 25 | 30 | 75 | 1.75 |
| Main 3 | the town gate: 3 Unpaid, 1 Restless | 21 | 30 | 126 | 2.23 |
| Side 1 | none | 0 | 35 | 161 | 2.54 |
| Main 4 | the chapel yard: 12 Unpaid, 1 Restless | 57 | 40 | 258 | 3.37 |
| Side 2 | Amos's hands: 5 Unpaid, 1 Restless | 33 | 40 | 332 | 3.96 |
| (roaming) | the west road's walkers: 3 Unpaid; boot hill and the north road: 6 Unpaid | 50 | | 381 | 4.32 |
| Main 5 | the churchyard's risen dead: 3 Unpaid, 1 Restless; the Sink: 4 floor spiders, 6 from the egg sacs | 75 | 40 | 496 | 5.12 |
| Main 6 | the sheep fold's camp: 2 Unpaid, 1 Restless; about 12 of Abel's adds, and Abel (about 118) | 200 | 45 | 741 | 6.61 |
| Main 7, The Lantern Leans | none | 0 | 45 | **786** | **6.87** |

**In all: 67 kills give 461 XP, ten missions 325: 786 XP, level 6.9.** Without the roaming kills: 737 (6.6). Each
further visit's roaming kills (they come back on every level load) add about 55 XP. An explorer who also clears every
optional encounter once (the orchard's edge, Mill Creek's slimes, the quarry track's spiders, the Webwood's drop: 15
kills) ends at about 898 XP, level 7.4 (the test holds it under 7.75). The Gravemother (Side 3, optional) and her brood
add about 140 more: past level 7. Before 2026-10-08's change the same play (without the encounters between the fights)
ended at 1,587 XP, level 10.4 (kills at full `XPReward` and 30% / 20% of a level per mission); with that change and no
encounters between the fights, 721 XP (6.49).

**The encounters between the fights** (`Art/Levels/RansomsRest/layout.json` gameplay.encounters, placed by
`Tools/Unreal/build_area_camps.py`; the user's budget, 2026-10-08: about 15 more kills on the main path at most, which
the test checks). Each entry says its `budget`: **main** (met on the story's way, counted before `countedBefore`'s own
fights), **roaming** (with the few roaming kills) or **optional** (the explorer's figure). Today: main 7 (the
churchyard's 4, the sheep fold's 3), roaming 3 (the west road's walkers), optional 15.

| Encounter | Where | On after | Creatures | Budget |
|---|---|---|---|---|
| Camp SheepFold | the west field, round the sheep fold between the Dry Wash and the west road | Main 4 | 2 Unpaid, 1 Restless; a crate | main, before Main 6 |
| Camp OrchardEdge | south of the orchard, just outside the salt line, under Coffin Rock | Main 3 | 3 Unpaid, 1 Restless; a crate | optional |
| Camp MillCreek | the creek bottom above Mill Falls, below the east hayfield | Main 3 | 3 slimes, 1 Restless; a crate | optional |
| Patrol WestRoad | the west road below the chapel knoll | Main 4 | 3 Unpaid (the area's ranks) | roaming |
| Patrol QuarryTrack | the quarry track to the Old Quarry | Main 3 | 3 spiders (the area's ranks) | optional |
| Ambush Churchyard | the churchyard's west grave rows: the dead rise | Main 4 | 3 Unpaid, 1 Restless | main, before Main 5 |
| Ambush Webwood | the Webwood's crowned trees: spiders drop | Main 4 | 3 spiders, 1 Restless | optional |

If the user wants the end back nearer 6.5 with the encounters in, either lowers it about 0.2 of a level: `KillXPScale`
0.4 to 0.36 (752 XP, 6.67; an explorer 7.2), or Main 6's and Main 7's experience 45 to 30 (756 XP, 6.69; an explorer
7.34). Neither is made: the user asked for 6-7.

The kill budget's sources: `Tools/Unreal/build_area_story.py` (BluffNest, TownGate), `build_area_chapel.py` (ChapelYard,
BootHill, NorthRoad), `build_area_whitlock.py` (WhitlockHands), `build_area_sink.py` (SinkFloor, the egg sacs' two
spiders each), `Bosses/AbelRules` (phase 1's two Unpaid every 25 s, phase 2's two waves of four; about 12 in a 3-4
minute fight), and the layout's encounters (`build_area_camps.py`), which the test reads itself. The design's Mill Creek
slimes are now the optional MillCreek camp.

## Changing the numbers

1. Change a mission's `experience` in the asset scripts (or `KillXPScale` in the settings), or an encounter's creatures
   or budget in `layout.json` gameplay.encounters.
2. Run the scripts, then `Tools\runtests.ps1 -Filter Looter.Progression.Pacing`: its log prints this table again, and
   the explorer's figure.
3. Update the tables above (the test warns when an asset's experience differs from it).
