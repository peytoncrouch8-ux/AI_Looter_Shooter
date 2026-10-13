# Progression: experience and pacing

How the player earns experience, and where Ransom's Rest's story leaves them. The user's call (2026-10-08): the story
should end about **level 6-7**, not 9-10; no experience for a mission's steps; missions are **turned in**, not finished
the moment their last objective is done.

## Where experience comes from

Only two places, nothing else (checked by `Looter.Missions.TurnIn.Rewards`):

- **Kills.** `FLevelRules::KillXP`: the creature's `XPReward` (10, times its rank's multiplier: Restless ×2,
  Gravebound ×4, Soulfed ×10, Boss ×20) × the **kill scale** (0.37; 0.4 until 2026-10-12) × 1.08 per level of the
  creature above 1, less 15 points a level when it's below the player (never under 10%). A level 1 Basic kill gives 4;
  one at level 5 gives 5. Skyreach (practice) gives none. The creature's level is where it stands, never the player's
  (below), so a player who comes back strong gets little from the early ground: a bluff spider is worth 4 to a level 1
  player and 1 to a level 7 one.
- **Missions turned in.** Each story mission has a fixed amount (`FMissionRewards::Experience`), given only when it's
  turned in to its giver, never for its objectives or steps. Skyreach's tutorial and skiff give none.

The knobs: `KillXPScale`, `KillXPGrowth`, the falloff (Project Settings > Game > Progression, `UProgressionSettings`),
each mission's `experience` in `Tools/Unreal/create_mission_assets.py` / `create_side_mission_assets.py`, and the
creatures' levels (below). The curve stays the user's: 100 XP for level 2, then 12% more a level. **Level 6 starts at
634 XP in all, 7 at 810, 8 at 1007.**

## Creature levels

The user's call (2026-10-12): "Monsters shouldn't scale with the player, creature levels are determined based on
areas", and Abel, the second boss, above the first. Before, a creature's level was the player's, one either side, kept
inside its area's band (1-10 on Ransom's Rest), and a boss's the player's plus its rank's: Abel showed level 2 to a
level 1 player. Now a creature's level is **where it stands**:

- its kind's **set level** in the area, if it has one (the bosses and Legendary monsters, never rolled);
- else the levels of the **zone** holding its spot (the first listed that does), rolled evenly between them: a level of
  variety; outside every zone the band's floor or one above;
- a spawner group's or a boss wave's own level, a creature placed at a level other than its class's, and the Bell-Ringer's
  raised dead (the fallen's level) keep theirs;
- its rank's levels on top, as before (Restless and Gravebound +1, Soulfed +2, a boss +1).

The player's own melee and grenade still grow with the player's level (`FLevelRules::EnemyScale` of theirs): that's the
player's side. Chests' guns still follow the player inside the band (`AChest::GetLootLevel`).

The data is the area's layout, `gameplay.levels` (`band`, `zones`, `kinds`), written into `DA_Area_<Area>` by
`Tools/Unreal/create_area_assets.py` (`UAreaDefinition::LevelZones`, `KindLevels`). `UAreaRulesSubsystem::RollLevelFor`
gives a creature its level as it begins play (its spot) and each time it comes back (its home). The station board shows
the band. Tests: `Looter.Areas.LevelBand` (the rules), `Looter.Areas.Bands` (the assets match the layouts),
`Looter.Creatures.AreaLevels` (every fight's and encounter's levels, the bosses).

**Skyreach:** level 1 everywhere (practice). **Ransom's Rest:** levels 1-6 before ranks, rising along the story:

| Zone | Where (layout) | What's fought there | Levels | The player there, on the story's pace |
|---|---|---|---:|---:|
| bluff | `ransomsPoint` | Main 2's spider nest | 1-2 | 1 |
| valley | everywhere else | Main 3's town gate, the orchard's edge camp, the farm road, Main Street | 2-3 | 2 |
| chapel | `chapel` | Main 4's chapel yard, the churchyard's risen dead | 3-4 | 2-4 |
| bootHill, northRoad | `bootHill`; 25 m round the hanging tree's road | the roaming Unpaid after Main 4 | 3-4 | 4 |
| whitlock | `whitlock` | Side 2's hired hands, Mill Creek's slimes | 3-4 | 3 |
| west | the west road to the keeper's grave | the west road's walkers, the sheep fold's camp | 4-5 | 4-5 |
| sink | `sink` | Main 5's floor spiders and egg sacs | 4-5 | 4-5 |
| webwood, quarry | `webwood`; 15 m round the quarry track | the optional drop and patrol: ground to come back to | 5-6 | 3-5 |
| deck | `gravewindPoint` | Main 6: Abel's adds | 5-6 | 5-6 |
| **The Gravemother** (set) | the Sink's den | Side 3, after Main 5: 4, Soulfed +2 | **6** | 5 |
| **Abel** (set) | the deck | Main 6's boss: 6, Boss +1 | **7** | 6 |

Why those bosses: the Gravemother opens after Main 5 with the player about level 5, so 6 puts her a level over them and
the Sink round her; Abel ends the story with the player about 6, so 7 makes him its hardest fight, over his adds and
over her (no Restless creature anywhere is above him). His health, 40 Unpaid of his level, is about 9,500 at 7.

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
and the sheep fold's camp on the way to the deck. Kills are averaged over the levels where each fight stands (its zone's,
each as likely; Abel at his set 7) and the area's promotions (8% Restless, 2% Gravebound), as the game rolls them.
`Looter.Progression.Pacing` recomputes this table from the mission assets, the creatures' own numbers, the layout's
levels and encounters, and fails if the end leaves level 6.

Since 2026-10-12 (creature levels by place, the kill scale 0.4 to 0.37, the Bell-Ringer and Spitter in):

| After | Kills on the way (their levels) | Kill XP | Turned in | XP in all | Level |
|---|---|---:|---:|---:|---:|
| Main 1, Seven Days | none | 0 | 20 | 20 | 1.20 |
| Main 2 | the bluff nest (1-2): 4 spiders, 1 Restless | 25 | 30 | 75 | 1.75 |
| Main 3 | the town gate (2-3): 3 Unpaid, 1 Restless | 21 | 30 | 126 | 2.23 |
| Side 1 | none | 0 | 35 | 161 | 2.54 |
| Main 4 | the chapel yard (3-4): 12 Unpaid, 1 Restless | 64 | 40 | 264 | 3.42 |
| Side 2 | Amos's hands (3-4): 5 Unpaid, 1 Restless | 32 | 40 | 336 | 3.99 |
| (roaming) | the west road's walkers (4-5): 3 Unpaid; boot hill and the north road (3-4): 6 Unpaid | 49 | | 385 | 4.34 |
| Main 5 | the churchyard's risen dead (3-4): 3 Unpaid, 1 Restless, a Bell-Ringer; the Sink (4-5): 4 floor spiders, 6 from the egg sacs | 83 | 40 | 507 | 5.19 |
| Main 6 | the sheep fold's camp (4-5): 2 Unpaid, 1 Restless; about 12 of Abel's adds (5-6), and Abel (7, about 117) | 197 | 45 | 749 | 6.65 |
| Main 7, The Lantern Leans | none | 0 | 45 | **794** | **6.91** |

**In all: 68 kills give 469 XP, ten missions 325: 794 XP, level 6.9**, the same end as before the change (at 0.4 the
new levels gave 826, level 7.08: on the story's pace the creatures now stand at or a level over the player, so fewer
kills lose the below-the-player share). Without the roaming kills: 745 (6.6). Each further visit's roaming kills (they
come back on every level load) add about 45 XP, less as the player outgrows them. An explorer who also clears every
optional encounter once (the orchard's edge, Mill Creek's slimes, the quarry track's spiders and Spitter, the Webwood's
drop: 18 kills) ends at about 885 XP, level 7.4 (the test holds it under 7.75). The Gravemother (Side 3, optional, level
6: about 54) and her brood (4: about 5 each), with Side 3's 40, add about 130 more: past level 7. Before 2026-10-08's
change the same play (without the encounters between the fights) ended at 1,587 XP, level 10.4 (kills at full
`XPReward` and 30% / 20% of a level per mission); with that change and no encounters between the fights, 721 XP (6.49).
The Bell-Ringer's raised dead (half experience) aren't counted: a few XP more.

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

If the user wants the end nearer 6.5, either lowers it about 0.2 of a level: `KillXPScale` 0.37 to 0.35 (762 XP, 6.73;
an explorer 7.24), or Main 6's and Main 7's experience 45 to 30 (about 764 XP, 6.75). Neither is made: the user asked
for 6-7.

The kill budget's sources: `Tools/Unreal/build_area_story.py` (BluffNest, TownGate), `build_area_chapel.py` (ChapelYard,
BootHill, NorthRoad), `build_area_whitlock.py` (WhitlockHands), `build_area_sink.py` (SinkFloor, the egg sacs' two
spiders each), `Bosses/AbelRules` (phase 1's two Unpaid every 25 s, phase 2's two waves of four; about 12 in a 3-4
minute fight), and the layout's encounters (`build_area_camps.py`), which the test reads itself. The design's Mill Creek
slimes are now the optional MillCreek camp.

## Changing the numbers

1. Change a mission's `experience` in the asset scripts (or `KillXPScale` in the settings), an encounter's creatures
   or budget in `layout.json` gameplay.encounters, or a zone's or boss's level in gameplay.levels (a new encounter needs
   its levels planned in `Tests/CreatureAreaLevelTests.cpp` too).
2. Run the scripts (`create_area_assets.py` for levels), then `Tools\runtests.ps1 -Filter Looter.Progression.Pacing`:
   its log prints this table again, and the explorer's figure; `Looter.Areas.Bands` and `Looter.Creatures.AreaLevels`
   check the levels.
3. Update the tables above (the test warns when an asset's experience differs from it).

## Economy

The money is **silver** (2026-10-09, Docs/Polish/BorderlandsComparison.md item 13): the living's coin on the frontier.
Grave gold stays the story's (the old fare laid on dead men's eyes, Sexton's pay to the gang) and is never money. The
HUD shows the silver carried at the top of the weapon column (an Inked silver coin and the amount, "+N" on a find); the
inventory's ammo line ends with it. Code: `Economy/` (`LooterEconomy` in `EconomyRules` holds every number below),
`UI/Vendor/` (the vendor's screen), `Session` (`ULooterSessionSave::Silver`, `VendorOffer`, `FSavedMapWorld::SilverPickups`).

**Growth.** Every amount below is level 1's and grows as enemies do: x(1 + 0.08 x (level - 1)), so x1.24 at level 4 and
x1.48 at level 7. Drops grow with the creature's or the area's level, prices with the player's (a gun's with its own).

### Where silver comes from

Silver pops out as a small pile of coins (`ASilverPickup`) that flies to a living player within 4 m once it has dropped
(0.55 s), with a coin chime. It drops where guns do: not on Skyreach once the player has left it (practice never farms).

| Source | Chance | Amount (level 1) | Average |
|---|---:|---:|---:|
| An Unpaid's kill: Basic / Restless / Gravebound / Soulfed / Boss | 55% / 85% / 100% / 100% / 100% | 3-7 / 9-15 / 18-30 / 50-80 / 100-140 | 2.75 / 10.2 / 24 / 65 / 120 |
| A beast's kill (spiders, slimes): the same ranks | 20% / 50% / 80% / 100% / 100% | 2-4 / 5-9 / 10-18 / 40-60 / 80-120 | 0.6 / 3.5 / 11.2 / 50 / 100 |
| Ranger cache, camp crate (Supply Crate) | 90% | 10-20 | 13.5 |
| The gang's Strongbox | 100% | 40-70 | 55 |
| Coffin / grave | 70% / 75% | 6-14 / 6-16 | 7 / 8.25 |
| Mailbox / footlocker | 80% / 90% | 4-10 / 8-18 | 5.6 / 11.7 |
| Crate or barrel broken | 25% | 2-6 | 1 |

Rolled apart from the loot tables (their own random streams), so the guns', ammo's, grenades' and motes' odds don't move.

**Mission purses** (`MissionRewards::SilverFor`, fixed, paid at the turn-in and shown on the MISSION COMPLETE banner):

| Mission | Giver | Silver |
|---|---|---:|
| Main 1, Seven Days | Delia (Grandpa's egg money under the door) | 20 |
| Main 3, Cold Welcome | Tilly (the unclaimed dead's pockets) | 40 |
| Side 1, Wanted: Already Dead | Tilly (the posters' reward money) | 50 |
| Main 4, Hallowed Ground | Father Aldana (the poor box) | 40 |
| Side 2, Unfinished Business | Amos (a season's wages) | 60 |
| Main 5, The Keeper's Lantern | Father Aldana | 50 |
| Main 7, The Lantern Leans | Tilly (her father's fare money) | 60 |
| Main 2, Main 6, Side 3; Skyreach's postings | Sexton pays in grave gold and not to Ellis; the boss and the Gravemother's loot is the reward; practice | 0 |

In all 320.

### The vendors

- **Pruitt's General Store**, Main Street, Ransom's Rest: the Pruitts (heard, never seen) trade through the hatch in their
  door once Main Street is safe, after Main 3. **Crossroads Mercantile**, the store front on Skyreach's square: a practice
  store with an honesty box, always open. Both are `AVendor`s at the store's door, placed by
  `Tools/Unreal/build_area_vendors.py`; Interact opens the vendor's screen.
- **Buy:** a box of each ammo the game's guns use: 36 rounds (24 shells, 18 sniper rounds: never more than half the most
  carried) for **15**, what fits when the player is nearly full for its share (rounded up); a jar of grave salt for
  **25**, up to the three carried.
- **The gun of the day:** one gun at the player's level from the default loot table, Uncommon 70% / Rare 24% / Epic 5% /
  Legendary 1%, never cursed, the same at both vendors. A level-up or an hour of play (a day's trade) brings the next;
  bought, it's sold out until then. The session keeps it, so loading never rerolls it. Asks first.
- **Sell:** backpack guns only (never one in hand), never a named gun (Heirloom is Pa's). Rare and up ask first. J (or a
  right-click) marks a gun junk, here or in the inventory; "Sell all junk" (R) sells every marked one at once, asking
  first when one is Rare or better. The junk mark rides with the gun (`FWeaponInstanceData::bJunk`).

| Rarity | Sells for | Gun of the day costs |
|---|---:|---:|
| Common | 8 | (never stocked) |
| Uncommon | 18 | 150 |
| Rare | 40 | 320 |
| Epic | 90 | 700 |
| Legendary | 200 | 1,500 |

A cursed iron sells for three quarters. Buying costs 7-8 times what selling pays, so a sold gun never buys itself back.

### What a playthrough of Ransom's Rest earns (`Looter.Economy.Balance`)

The normal play counted above (every main fight, both sides, the roaming walkers), at level 4 on average, plus about twelve
containers opened, fifteen crates broken and twenty guns sold (twelve Common, six Uncommon, two Rare):

| Source | Silver |
|---|---:|
| Kills (46 Basic and 5 Restless Unpaid, 15 spiders, Abel) | about 385 |
| Containers (twelve, the Strongbox among them) | about 200 |
| Crates and barrels (fifteen) | about 20 |
| Mission purses | 320 |
| Twenty guns sold | about 350 |
| **In all** | **about 1,270** |

Steady supplies (ten boxes of ammo, six jars of salt) cost about 375; what's left buys **about five Uncommon or two Rare
guns of the day** (186 and 397 at level 4), a Legendary (2,100 at level 6) costs more than the whole playthrough earns, and
a Basic kill is worth under a box of ammo: drops stay about the guns. An explorer (every container, every optional camp)
earns about 1,700-1,900. The test holds the normal play between 1,000 and 1,800 and the guns of the day to "a few".
