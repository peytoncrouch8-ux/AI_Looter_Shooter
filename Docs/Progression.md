# Progression: experience and pacing

How the player earns experience, and where Ransom's Rest leaves them. The old curve: `Docs/Archive/Progression_history.md`.

The user's calls:
- Missions are **turned in**, not finished the moment their last objective is done, and give no experience for their
  steps (2026-10-08).
- Since the mountain and the givers (2026-10-13, `Docs/Areas/RansomsRest_Expansion.md` section 13): **the story alone
  ends about level 5.5, a full clear of Ransom's Rest about level 6.8**, so a player reaches the *Gilded Lily* about
  level 6-7 whichever way they played.
- (2026-10-13) **Levelling works like Borderlands': it takes more experience, the top level is 65, and kills give real
  numbers again.** In the user's words: "Just make it take more XP to level up. Find something similar to the way
  Borderlands handles their level up system and do that. We plan on making a bunch of maps or areas with a max level of
  65 so we don't want to get to too high a level too soon."

## How Borderlands does it (the research, 2026-10-13)

- **The curve.** Borderlands 2's experience to reach level L, in all, is about **60 × (L^2.8 − 1)** (worked out by its
  players: [the Borderlands wiki's formula thread](https://borderlands.fandom.com/wiki/Forum:Experience_Points_Formula%3F),
  [its talk page](https://borderlands.fandom.com/wiki/Talk:Experience_Points)); it matches the game's own totals, about
  9.5 million at level 72 and 12.8 million at 80 ([Steam](https://steamcommunity.com/app/49520/discussions/0/612823460257469706/)).
  358 for level 2, 1,240 for 3, about 270,000 for one level in the 60s: each level a little more than the last.
- **Missions** give a share of a level that falls as the levels go up: My First Gun 71 XP and Blindsided 53 at level 1,
  Cleaning Up the Berg 321 at level 3 ([Game Guides](https://www.gamerguides.com/borderlands-2/guide/borderlands-2/main-story-missions/cleaning-up-the-berg)),
  about 15-20% of a level; players report about 9,800 for a level 50 mission, 5% of that level. The first levels come
  fast, the last ones slowly.
- **Kills** give experience by the enemy's level against the player's: more from one above, less and less from one
  below, until a player far over it gets next to nothing; mission experience falls the same way when the player is over
  the mission's level. The exact kill formula was never published. (Players' reports on Steam,
  [1](https://steamcommunity.com/app/49520/discussions/0/43099721372790397),
  [2](https://steamcommunity.com/app/49520/discussions/0/1697175413669456789), and
  [GameFAQs](https://gamefaqs.gamespot.com/boards/638786-borderlands-2/64424551).)

## Our curve and kills (since 2026-10-13)

- **The curve** (`FXPCurve`): reaching level L takes **180 × (L^2.8 − 1)** in all, rounded: Borderlands 2's shape at
  three times its experience. **The top level is 65** (about 21.4 million in all).
- **Kills** (`FLevelRules::KillXP`): the creature's `XPReward` (10; times its rank's multiplier: Restless ×2,
  Gravebound ×4, Soulfed ×10, Boss ×20) × the kill scale (**1**) × **its level^1.2**, less 15 points a level when it's
  below the player (never under 10%), never under 1. A Basic creature gives **10** at level 1, 23 at 2, 69 at 5, 103 at
  7, 364 at 20 and 1,361 at 60; a Restless one at level 3 gives 75, a level 5-6 miner 76-94, the Gravemother 859, the
  Foreman 1,136, Abel 2,066. The creature's level is where it stands, never the player's (*Creature levels*).
- **Missions turned in.** Each mission has a fixed amount (`FMissionRewards::Experience`), given only when it's turned in
  to its giver, never for its objectives or steps. Skyreach's postings give none. Sized as a share of the level where
  it's met: on Ransom's Rest a main mission about a third to two thirds of a level, a side mission an eighth to a fifth
  (a short errand less), generous as a first area should be (as Borderlands' first hours are).

Only those two give experience (checked by `Looter.Missions.TurnIn.Rewards`).

| Level | To the next | In all, to reach it | A Basic kill at it | Basic kills a level |
|---:|---:|---:|---:|---:|
| 1 | 1,074 | 0 | 10 | 107 |
| 2 | 2,647 | 1,074 | 23 | 115 |
| 3 | 4,830 | 3,721 | 37 | 131 |
| 4 | 7,577 | 8,551 | 53 | 143 |
| 5 | 10,862 | 16,128 | 69 | 157 |
| 6 | 14,666 | 26,990 | 86 | 171 |
| 7 | 18,967 | 41,656 | 103 | 184 |
| 8 | 23,754 | 60,623 | 121 | 196 |
| 10 | 34,739 | 113,392 | 158 | 220 |
| 20 | 115,784 | 790,784 | 364 | 318 |
| 30 | 236,700 | 2,461,389 | 592 | 400 |
| 40 | 394,336 | 5,508,410 | 837 | 471 |
| 50 | 586,631 | 10,289,184 | 1,093 | 537 |
| 60 | 812,078 | 17,143,182 | 1,361 | 597 |
| 64 | 911,264 | 20,538,685 | 1,470 | 620 |
| 65 | (the top) | 21,449,949 | | |

**Why the kills grow slower than the levels.** A level's experience grows about as level^1.8, a kill's as level^1.2, so
the kills a level takes climb from about 110 to 600 over the campaign and the turn-ins carry more of each level, as in
Borderlands. **For the areas to come:** an area of Ransom's Rest's size (about 150 kills, 7 main missions, 9 side) gives
about 2 levels at the campaign's middle when its main missions are about 15% of a level at its band and its side
missions about 6%, so some 30 areas reach 65 near the end. Ransom's Rest's bigger shares are the first area's (levels
1-7 come quickly, as in Borderlands' opening). A mission given far below the player's level still pays its fixed amount
(Borderlands would take some off; not needed while the areas' bands follow the story).

**Saves from before the change keep their level** (`FXPCurve::Clamp`, as for any change to these settings): the level
stays and the XP into it carries over as it is, now a smaller part of a bigger bar; a save above 65 comes down to 65.
Keeping the total experience instead would have dropped every player to about level 1 or 2 (a full Ransom's Rest was
about 800 XP on the old curve), and with it their health and their place against the creatures' levels.

The knobs (Project Settings > Game > Progression, `UProgressionSettings`, saved to `Config/DefaultGame.ini`; defaults in
`Progression/XPCurve.h` and `Progression/LevelRules.h`): `MaxLevel`, `CurveScale`, `CurvePower`, `KillXPScale`,
`KillXPPower`, the falloff; each mission's `experience` in `Tools/Unreal/create_mission_assets.py` and
`create_side_mission_assets.py`; the creatures' levels (below). `Looter.XP.Table` prints the curve and the kills.

## Creature levels

The user's call (2026-10-12): "Monsters shouldn't scale with the player, creature levels are determined based on
areas". A creature's level is **where it stands**:

- its kind's **set level** in the area, if it has one (the bosses and Legendary monsters, never rolled);
- else the levels of the **zone** holding its spot (the first listed that does), rolled evenly between them; outside
  every zone the band's floor or one above;
- a spawner group's or a boss wave's own level, a creature placed at a level other than its class's, and the
  Bell-Ringer's raised dead (the fallen's level) keep theirs;
- its rank's levels on top (Restless and Gravebound +1, Soulfed +2, a boss +1).

The player's own melee and grenade still grow with the player's level (`FLevelRules::EnemyScale` of theirs). Chests'
guns still follow the player inside the band (`AChest::GetLootLevel`).

Data: the area's layout, `gameplay.levels` (`band`, `zones`, `kinds`), written into `DA_Area_<Area>` by
`Tools/Unreal/create_area_assets.py` (`UAreaDefinition::LevelZones`, `KindLevels`). `UAreaRulesSubsystem::RollLevelFor`
gives a creature its level as it begins play and each time it comes back. Tests: `Looter.Areas.LevelBand` (the rules),
`Looter.Areas.Bands` (the assets match the layouts), `Looter.Creatures.AreaLevels` (every fight's and encounter's levels,
the bosses). The mountain's zones and the Foreman came in `Art/Levels/RansomsRest/patches/missions.json` (2026-10-13).

**Skyreach:** level 1 everywhere (practice). **Ransom's Rest:** band 1-7 (the station board's); zones 1-6 before ranks,
rising along the story, the mountain 5-6 (the user's call):

| Zone | Where (layout) | What's fought there | Levels | The player there, story alone / everything |
|---|---|---|---:|---:|
| bluff | `ransomsPoint` | Main 2's spider nest | 1-2 | 1 / 1 |
| valley | everywhere else | Main 3's town gate, the orchard's edge camp, Main Street, the depot's waiting room (The Old Fare) | 2-3 | 2 / 2-3 |
| chapel | `chapel` | Main 4's chapel yard, the churchyard's risen dead | 3-4 | 2-3 / 3 |
| bootHill, northRoad | `bootHill`; 25 m round the north road | the roaming Unpaid after Main 4 | 3-4 | 3 / 4 |
| whitlock | `whitlock` | Side 2's hired hands, Mill Creek's slimes | 3-4 | — / 3 |
| (Tilly's cellar) | its spawner's own | Knocking Below's four | 3-4 | — / 4 |
| west | the west road to the keeper's grave | the west road's walkers, the sheep fold's camp | 4-5 | 3-4 / 4-6 |
| sink | `sink` | Main 5's floor spiders and egg sacs | 4-5 | 3 / 4 |
| webwood, quarry | `webwood`; 15 m round the quarry track | the optional drop and patrol | 5-6 | — / 3-4 |
| quarryBowl, crest | `oldQuarry`, `crest` | Dead Lift's lift crew, Abe Haskett, the raising steam; the crest's spiders | 5-6 | — / 5 |
| calderPost, bench | `calderPost`, `larkspurBench` | The Last Watch's yard and watch; the Dinner Pail camp | 5-6 | — / 5 |
| diggings, providence | `diggings`, `providence` | Back Pay: the spoil heap, the upper gallery, the lower drift, the stope's shift | 5-6 | — / 5 |
| deck | `gravewindPoint` | Main 6: Abel's adds | 5-6 | 4 / 6 |
| **The Gravemother** (set) | the Sink's den | Side 3, after Main 5: 4, Soulfed +2 | **6** | — / 4-5 |
| **Abel** (set) | the deck | Main 6's boss: 6, Boss +1 | **7** | 4 / 6 |
| **The Foreman** (set) | the Providence's stope | Back Pay's Legendary monster: 5, Soulfed +2 | **7** | — / 5 |

Abel stays the story's hardest fight, about three levels over a player who only follows the story; the Foreman is level
with him, a Legendary over the mine as the Gravemother is over the Sink. No Restless creature anywhere is above them.

## Givers and turn-ins (Borderlands' way)

Every mission but Main 1 is **picked up from a giver** (the user's call, 2026-10-13): a ? over them once the mission
before it is turned in; talked to, their offer's lines and the offer card (or the card alone from a board or a found
artifact). A mission whose objectives are all done is **ready to turn in**: the tracker says "Turn in to <giver>", a !
over them; talking to them finishes it (the experience, the purse, the reward gun at the player's feet, MISSION COMPLETE)
and any mission they offer next is offered straight after, as one conversation. The state is saved
(`FCampaignRecord::ReadyMissions`). Steps, givers and words: `Tools/Unreal/create_mission_assets.py` and
`create_side_mission_assets.py`'s docstrings; what's said where: `Tools/Unreal/story_texts.py`.

| Mission | Picked up from | After | Turned in to | XP | Purse | Gun |
|---|---|---|---|---:|---:|---|
| Main 1, Seven Days | (a new game) | — | Delia, at her screen door | 450 | 20 | — |
| Main 2, Shall We Talk Business? | Hob | Main 1 | Mister Sexton, on the rail | 550 | — | (the Ledger) |
| Main 3, Cold Welcome | Delia | Main 2 | Tilly, at her window | 1,200 | 40 | Uncommon+ |
| Side 1, Wanted: Already Dead | the poster on the town gate | Main 3 | Tilly | 400 | 50 | Rare |
| Calder's Caches | Ruth Calder's note | Main 3 | the Rim Rangers' board | 250 | 30 | — |
| The Old Fare | Mr. Sloane, through his door | Main 3 | Josiah Fenn, in his coffin | 600 | 30 | Rare Drover |
| Main 4, Hallowed Ground | Tilly | Main 3 | Father Aldana, at the vestry door | 1,400 | 40 | — |
| Side 2, Unfinished Business | Amos, at his fence | Main 4 | Amos | 800 | 60 | Epic |
| Knocking Below | Tilly | Main 4 | Tilly | 800 | 40 | Rare |
| Main 5, The Keeper's Lantern | Father Aldana | Main 4 | Father Aldana | 2,500 | 50 | — |
| Side 3, The Gravemother | the sheriff's bounty on the Rangers' board | Main 5 | Mrs. Pruitt, at the store's hatch | 1,200 | 80 | (her Legendary table) |
| Dead Lift | Hiram Coyle, in the winch house | Main 5 | Hiram, up the speaking tube | 1,500 | 40 | Rare rifle |
| The Last Watch | the board at Calder's Post | Dead Lift | the post's board | 1,300 | 60 | (Calder's fourth cache) |
| Back Pay | No. 7's brass tag | Dead Lift | the tag board: pay the eleven, or keep the payroll | 2,000 | 0 / 400 | Rare / — |
| Main 6, The Gravewind | Delia | Main 5 | nobody: finishes as Pa sits down | 5,000 | — | (Abel's loot) |
| Main 7, The Lantern Leans | Pa, on his board | Main 6 | Tilly | 4,600 | 60 | (Heirloom, by hand) |

Main missions **15,700 XP** in all, side missions **8,850**. Each optional objective done adds 10 silver at the turn-in
(at level 1, grown with the economy) and no experience, so the sums below hold.

## Ransom's Rest, counted

`Looter.Progression.Pacing` plays two paths on paper from the same data the game uses (`Tests/PacingPaths.h`): the
mission assets' rewards, the creatures' own numbers, the layout's levels and encounters. Kills one at a time at the level
the ones before left the player on, each averaged over the levels where it stands and the area's promotions (8%
Restless, 2% Gravebound) as the game rolls them; then the turn-in. It fails outside **5.3-5.8** for the story alone and
**6.6-7.0** for everything, or if the missions give under two thirds of the story's experience.

### The story alone

Every main mission, the fights on its way (the layout's "main" encounters: the churchyard's risen dead before Main 5, the
sheep fold's camp before Main 6), and the roaming dead once (boot hill's, the north road's, the west road's walkers).

| After | Kills on the way (their levels) | Kill XP | Turned in | XP in all | Level |
|---|---|---:|---:|---:|---:|
| Main 1, Seven Days | none | 0 | 450 | 450 | 1.42 |
| Main 2 | the bluff nest (1-2): 4 spiders, 1 Restless | 126 | 550 | 1,126 | 2.02 |
| Main 3 | the town gate (2-3): 3 Unpaid, 1 Restless | 180 | 1,200 | 2,507 | 2.54 |
| Main 4 | the chapel yard (3-4): 12 Unpaid, 1 Restless | 662 | 1,400 | 4,569 | 3.18 |
| (roaming) | boot hill, the north road (3-4): 6 Unpaid; the west road (4-5): 3 Unpaid | 551 | | 5,120 | 3.29 |
| Main 5 | the churchyard (3-4): 3 Unpaid, 1 Restless, a Bell-Ringer; the Sink (4-5): 10 spiders | 984 | 2,500 | 8,605 | 4.01 |
| Main 6 | the sheep fold (4-5): 3; about 12 of Abel's adds (5-6) and Abel (7: 2,066) | 3,273 | 5,000 | 16,878 | 5.07 |
| Main 7, The Lantern Leans | none | 0 | 4,600 | **21,478** | **5.49** |

**62 kills give about 5,800 XP, seven missions 15,700 (73%): about 21,500 XP, level 5.5** (5.44 without the roaming
dead). Each level-up lands on a turn-in: 2 at Main 2's, 3 at Main 4's, 4 at Main 5's, 5 at Main 6's. Each further
visit's roaming kills add about 550.

### Everything once

Every mission as soon as it opens (the valley's side missions before Main 5, the Gravemother and the mountain before Main
6), their fights, and every optional camp, patrol and ambush once.

| After | Kills on the way | Kill XP | Turned in | XP in all | Level |
|---|---|---:|---:|---:|---:|
| Mains 1-3 | as above | 306 | 2,200 | 2,507 | 2.54 |
| Side 1 | the optional camps and patrol counted before it: the orchard's edge (2-3), Mill Creek's slimes (3-4), the quarry track (5-6) | 852 | 400 | 3,759 | 3.01 |
| Calder's Caches | none | 0 | 250 | 4,009 | 3.06 |
| The Old Fare | the depot's waiting room (2-3): 3 Unpaid, 1 Restless, a Bell-Ringer | 218 | 600 | 4,828 | 3.23 |
| Main 4 | the chapel yard | 662 | 1,400 | 6,890 | 3.66 |
| Side 2; Knocking Below | Amos's hands (3-4): 6; Tilly's cellar (3-4): 4 | 604 | 1,600 | 9,094 | 4.07 |
| (roaming) | as above | 538 | | 9,632 | 4.14 |
| Main 5 | the Webwood's drop (5-6, optional), the churchyard, the Sink | 1,472 | 2,500 | 13,604 | 4.67 |
| Side 3 | the Gravemother (6: 859) and about 6 of her brood (4) | 1,177 | 1,200 | 15,981 | 4.98 |
| Dead Lift | the lift crew and Abe Haskett, the raising steam, the crest's spiders (5-6): 13 | 1,228 | 1,500 | 18,709 | 5.24 |
| The Last Watch | Calder's Post's yard and the watch (5-6): 8; the Dinner Pail camp: 5 | 1,382 | 1,300 | 21,391 | 5.48 |
| Back Pay | the spoil heap, the two shifts (5-6): 14; the Foreman (7: 1,136) and his 6 | 3,113 | 2,000 | 26,504 | 5.96 |
| Main 6 | the sheep fold, Abel's adds and Abel | 3,220 | 5,000 | 34,724 | 6.53 |
| Main 7 | none | 0 | 4,600 | **39,324** | **6.84** |

**About 149 kills give about 14,800 XP, sixteen missions 24,550: about 39,300 XP, level 6.8.** A side mission is worth
about an eighth to a fifth of a level where it's met, Borderlands' share (Calder's Caches, a short errand, a twentieth).
A player who does the mountain after Main 6 instead meets Abel at about 5 and ends about the same (the falloff takes a
little more of the mountain's kills). The Gravemother's and the Foreman's Vigil Stones (a kill each again) add about 860
and 1,140 a time.

These are the paper model's; the test's own sums, with the layout's levels, print in its log and may differ by a few
hundredths. The mountain's zones come from `patches/missions.json`; without them its fights would count at the valley's
2-3 and a full clear end about 6.7.

**The encounters between the fights** (`Art/Levels/RansomsRest/layout.json` gameplay.encounters, placed by
`Tools/Unreal/build_area_camps.py`; the user's budget, 2026-10-08: about 15 more kills on the story's way at most, which
the test checks). Each entry says its `budget`: **main** (met on the way, counted before `countedBefore`'s own fights),
**roaming** (with the few roaming kills) or **optional** (the explorer's). The missions' own fights (the depot's, the
cellar's, the mountain's) are the paths' own, counted once even when the layout lists them.

| Encounter | Where | On after | Creatures | Budget |
|---|---|---|---|---|
| Camp SheepFold | the west field, round the sheep fold | Main 4 | 2 Unpaid, 1 Restless; a crate | main, before Main 6 |
| Camp OrchardEdge | south of the orchard, under Coffin Rock | Main 3 | 3 Unpaid, 1 Restless, a Bell-Ringer; a crate | optional |
| Camp MillCreek | the creek bottom above Mill Falls | Main 3 | 3 slimes, 1 Restless; a crate | optional |
| Patrol WestRoad | the west road below the chapel knoll | Main 4 | 3 Unpaid (the area's ranks) | roaming |
| Patrol QuarryTrack | the quarry track to the Old Quarry | Main 3 | 3 spiders, a Spitter (the area's ranks) | optional |
| Ambush Churchyard | the churchyard's west grave rows | Main 4 | 3 Unpaid, 1 Restless, a Bell-Ringer | main, before Main 5 |
| Ambush Webwood | the Webwood's crowned trees | Main 4 | 3 spiders, 1 Restless, a Spitter | optional |
| (the mountain's) | the crest, the Bench, the Diggings | Dead Lift | CrestPatrol 3 spiders; DinnerPail 3 miners, 1 Restless, a Bell-Ringer; SpoilAmbush 3 miners | optional / Back Pay's way |

## Changing the numbers

1. Change a mission's `experience` in the asset scripts (or the curve or the kill rules in the settings), an encounter's
   creatures or budget in `layout.json` gameplay.encounters, or a zone's or a set kind's level in gameplay.levels (a new
   encounter needs its levels planned in `Tests/CreatureAreaLevelTests.cpp` too; a new mission fight its kills in
   `Tests/PacingPaths.h`).
2. Run the scripts (`create_area_assets.py` for levels), then `Tools\runtests.ps1 -Filter Looter.Progression.Pacing`:
   its log prints both tables again; `Looter.Areas.Bands` and `Looter.Creatures.AreaLevels` check the levels.
3. Update the tables above (the test warns when an asset's experience differs from `PacingPaths.h`'s planned amount).

## Economy

The money is **silver** (2026-10-09, Docs/Polish/BorderlandsComparison.md item 13): the living's coin on the frontier.
Grave gold stays the story's (the old fare laid on dead men's eyes, Sexton's pay to the gang) and is never money: Fenn's
two coins in The Old Fare are a mission item, never silver. The HUD shows the silver carried at the top of the weapon
column (an Inked silver coin and the amount, "+N" on a find); the inventory's ammo line ends with it. Code: `Economy/`
(`LooterEconomy` in `EconomyRules` holds every number below), `UI/Vendor/` (the vendor's screen), `Session`
(`ULooterSessionSave::Silver`, `VendorOffer`, `FSavedMapWorld::SilverPickups`).

**Growth.** Every amount below is level 1's and grows as enemies do: x(1 + 0.08 x (level - 1)), so x1.24 at level 4 and
x1.48 at level 7. Drops grow with the creature's or the area's level, prices with the player's (a gun's with its own).

### Where silver comes from

Silver pops out as a small pile of coins (`ASilverPickup`) that flies to a living player within 4 m once it has dropped
(0.55 s), with a coin chime. It drops where guns do.

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

**Mission purses** (fixed, paid at the turn-in, shown on the MISSION COMPLETE banner): the older missions' are kept by id
(`MissionRewards::SilverFor`), the newer ones' are their own (`FMissionRewards::Silver`, the mission scripts';
`MissionRewards::PurseOf` takes the mission's own first). Each optional objective done adds 10 more (at level 1).

| Mission | Giver | Silver |
|---|---|---:|
| Main 1, Seven Days | Delia (Grandpa's egg money under the door) | 20 |
| Main 3, Cold Welcome | Tilly (the unclaimed dead's pockets) | 40 |
| Side 1, Wanted: Already Dead | Tilly (the posters' reward money) | 50 |
| Calder's Caches | the Rangers' board (the gang's banknotes, the Deacon's "kid's share") | 30 |
| The Old Fare | Josiah Fenn (what was under him with the pistol) | 30 |
| Main 4, Hallowed Ground | Father Aldana (the poor box) | 40 |
| Side 2, Unfinished Business | Amos (a season's wages) | 60 |
| Knocking Below | Tilly (her father's box under the counter) | 40 |
| Main 5, The Keeper's Lantern | Father Aldana | 50 |
| Side 3, The Gravemother | Mrs. Pruitt (the town's purse, the sheriff's bounty) | 80 |
| Dead Lift | Hiram (the company's tool box at the lift head) | 40 |
| The Last Watch | the post's board | 60 |
| Back Pay | the payroll kept: 400; paid to the eleven: none (a Rare gun) | 0 / 400 |
| Main 7, The Lantern Leans | Tilly (her father's fare money) | 60 |
| Main 2, Main 6 | Sexton pays in grave gold and not to Ellis; Abel's loot is the reward | 0 |

The valley's purses come to 420, the mountain's and the bounty to 180 more (580 with a kept payroll).

### The vendors

- **Pruitt's General Store**, Main Street, Ransom's Rest: the Pruitts (heard, never seen) trade through the hatch in their
  door once Main Street is safe, after Main 3; Mrs. Pruitt, the late sheriff's sister, takes Side 3's bounty there.
  **Crossroads Mercantile**, the store front on Skyreach's square, always open. Both are `AVendor`s at the store's door,
  placed by `Tools/Unreal/build_area_vendors.py`; Interact opens the vendor's screen.
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

A normal play: every main fight, the valley's five side missions (the posters, Calder's caches, the old fare, Amos's hay,
Tilly's cellar), the roaming walkers, at level 4 on average, plus about twelve containers opened, fifteen crates broken
and twenty guns sold (twelve Common, six Uncommon, two Rare):

| Source | Silver |
|---|---:|
| Kills (53 Basic and 7 Restless Unpaid, 15 spiders, Abel) | about 435 |
| Containers (twelve, the Strongbox among them) | about 200 |
| Crates and barrels (fifteen) | about 20 |
| Mission purses (the valley's) | 420 |
| Twenty guns sold | about 350 |
| **In all** | **about 1,420** |

Steady supplies (ten boxes of ammo, six jars of salt) cost about 375; what's left buys **about five or six Uncommon or two
Rare guns of the day** (186 and 397 at level 4). A Legendary (2,100 at level 6) costs more than the whole playthrough
earns, and a Basic kill is worth under a box of ammo: drops stay about the guns. An explorer (the mountain, the bounty,
every container and camp) earns about 2,000, and 400 more with the payroll kept. The test holds the normal play between
1,000 and 1,800 and the guns of the day to "a few"; Lost Property's shelving upgrades (150 to 1,200) are what the
explorer's silver is for.
