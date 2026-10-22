# BOUNTY: the bounty board (design, phase 1, 2026-10-22)

The user's pick (2026-10-22): a bounty board so Ransom's Rest has replayable hard fights now, before new regions.
Contract: `CONTRACT.md` "BOUNTY". Code in `Saved/BountyCode/` (holding tree, `HASHES.txt`); script, recipes in place.

## The loop

- **The board** (`ABountyBoard`, `BountyBoard_RansomsRest`, board id `RansomsRest`) stands 2.8 m to the right of the Rim
  Rangers' board in front of the sheriff's office on Main Street (layout `gameplay.boards[0]`, X 660 Y 1680, yaw 180),
  facing the street. Model `SM_NoticeBoard_Town` (the same board with NOTICES on its header, so it isn't a twin of the
  RIM RANGERS one). Usable after Main 3 (`OpenWhen`: Main Street is safe then). Three paper sheets on its face, one a
  posting, each with a wax seal in the mark's tier colour (code stand-ins: engine plane and cylinder in
  `M_StylizedSurface`); a taken or claimed posting's sheet is gone from the board (the player carries it).
- Its own floating mark (an `AGiverMarkActor` it owns): **?** while a posting is up that the player hasn't read on the
  board yet; **!** while a won bounty waits to be paid. ! over ?, as the givers' marks.
- **Tap Interact** opens the board screen (`UBountyBoardWidget`, LooterUI Concept C): three cards side by side, A/D (or
  the mouse, the d-pad) choose, E takes or collects, R twice gives a taken one up, Esc closes. Reading the board counts
  every posting up as seen (the ? goes).
- **One bounty taken at a time.** Taken: tracked on the mission tracker and the minimap's arrow (a `UMissionSubsystem`
  entry, tracked at once), its fight waits at its arena. Given up: back on the board, its fight gone.
- **Won** the moment its mark dies (saved). The posse's stragglers fight on; when the fight is over a Strongbox drops
  where it ended (or at once if the player left: the fight goes). The tracker then says "Collect the bounty at the
  board" (and "Open the strongbox" as an optional line while it's shut). **Paid at the board:** experience and silver,
  the mission-complete banner (as a side job's, title "Bounty: <mark>"), `Bounty.Paid`. The slot fills again later.

## A posting

Drawn from a seed (the board's id and the posting's serial), saved whole, so a load never rerolls it:
- **Arena**: one whose story condition holds, never one another slot or the taken bounty uses. Fewer open arenas than
  slots: fewer postings (the empty card says more come as the Reaches open up).
- **Kind** (the mark's): one the arena lists. Kinds (`BountyRules::Kinds`, C++):

| Kind | Mark | Posse | Support (from push 2) | Entrance |
|---|---|---|---|---|
| Unpaid | `AUnpaidCreature` | Unpaid | Bell-Ringer | rise |
| Spider | `ASpiderCreature` | spiders | Venom Spitter | the arena's (drop in the Webwood, else their own) |
| Spitter | `ASpitterSpider` | spiders | Venom Spitter | as spiders |
| DeadMiner | `ADeadMinerCreature` | dead miners | Bell-Ringer | rise |

- **Tier** (weights among the open ones 50 / 35 / 15):

| Tier | Mark's rank | Opens | Pushes (posse; the mark comes with the last) | Mark twists | Chest | Purse (level 1) | XP share |
|---|---|---|---|---|---|---:|---:|
| Wanted | Gravebound | the board (after Main 3) | 4 Basic; then the mark, 1 Restless, 2 Basic | 1 | 2 guns | 35 | 0.05 |
| Dangerous | Hallowed | after Main 5 | 4 Basic + 1 Restless; 3 Basic + support; the mark + 2 Restless | 1, and a pack twist 35% | 2 guns | 55 | 0.07 |
| Deadly | Ethereal | after Main 6 | 4 Basic + 1 Restless; 3 Basic + 1 Restless + support; the mark + 2 Restless | 1, and a second (half the time a pack twist) | 3 guns | 80 | 0.09 |

  Ranks are Fixed (never promoted, never stepped down by caps). Levels from the arena's zone (the user's rule, never the
  player's), the rank's levels on top. 8 / 12 / 13 creatures in all, at most about 9 up at once (a push comes when 2
  are left, 30 s at the latest). Caps hold: 20 near, the Unpaid family's 16.
- **Twists**: on the mark Fleet, Warded, Mending, Rallying, Vengeful, Grave-burst (each only where
  `CreatureTraitRules::Fits` its kind; never Phasing: the Ethereal's own, and the Deadly mark has it already). Pack
  twists: **Big posse** (+1 Basic in every push), **Swift posse** (the posse runs Fleet; not for the Spitter's posse
  rule: every spider fits). The mark's traits are exactly as posted: its twists, then its rank's `TraitCount` more
  rolled from the posting's seed (`CreatureTraitRules::Roll`), set on it as it spawns.
- **Name**: from its kind's list (8 each, original; none of the story's names), never one already up. Its tag reads
  "<rank word> <name>" (`ACreatureBase::DisplayName`). A flavour line from its kind's list, by serial.

## The timer (session's time played, so Save & Quit never rerolls)

- `Looter.Bounty.RotateMinutes` [120]: every two hours of play the untaken postings come down and new ones go up (the
  taken one stays).
- `Looter.Bounty.RefillMinutes` [10]: a slot paid out fills again ten minutes of play later.
- An empty slot (no arena was free) fills as soon as an arena opens (the story moved on).
- The screen shows "New postings in 1 h 42 min" and each empty card "Next posting in 7 min".

## Rewards and pacing

- **Strongbox** (`ABountyChest`, an `AChest` of kind Strongbox, id `Bounty_<board>_<serial>`): 2 / 2 / 3 guns at Luck
  1.0 (a Strongbox's: 15.5% legendary each), 2 ammo pickups, its kind's own silver roll (40-70 at level 1). It drops
  from 2.6 m with a thud, dust and `Bounty.Chest.Land`. One not opened is owed (saved) and stands again at its arena on
  the next visit.
- **Purse** 35 / 55 / 80 grown with the arena's top level (`LooterEconomy::AtLevel`: x1.16 at 3, x1.4 at 6).
- **Experience**: the tier's share of the step at the arena's top level (`FXPCurve::XPToNextLevel`), times the kills'
  falloff for a player above it (`FLevelRules::KillXPShare`): the Webwood at 6 gives 733 / 1,027 / 1,320; the Scatter
  at 3 gives 242 for a Wanted. That is the **side-content share** (a side mission's turn-in is 0.06-0.10 of the level
  where it's met); the fight's kills add about as much again. Bounties repeat, so they stay **out of
  `Looter.Progression.Pacing`'s paths** (as the vigil stones' rematches are: "a time" in Docs/Progression.md);
  `Looter.Bounty.Pacing` holds every tier's turn-in inside 0.04-0.10 of the level at its arena and the falloff past it.
  At RR's 5-6 one Deadly bounty is about a quarter of a level all told; past level 7 the falloff takes more each level.

## Arenas (`Tools/Unreal/build_area_bounties.py` ARENAS; layout cm, X north, Y east)

| Id | Name | Where | Opens | Kinds | Levels |
|---|---|---|---|---|---|
| Scatter | the Scatter | round the Scatter's boulders (-5160, 840), radius 1300 | after Main 3 | Unpaid | 2-3 |
| WhitlockYard | Whitlock's barn yard | inside the barn yard fence (-6850, 3500) | after Side 2 | Unpaid | 3-4 |
| Webwood | the Webwood | the webwood band, round the player, dropping (the set piece's ground) | after Main 4 and `SetPiece.Webwood` | Spider, Spitter | 5-6 |
| OldQuarry | the Old Quarry | the quarry floor (9725, 2050) | after Dead Lift | Spider, Spitter, DeadMiner | 5-6 |
| GraniteMeadow | the Bench's granite meadow | between the Bench's granite D and E (20900, 700) | after Dead Lift | DeadMiner, Spider | 5-6 |

Each is an `ABountyArena` in `Gameplay/Bounties` (label `BountyArena_<Id>`): its ground (radius or polygon, height band),
three spot lists (push 1 ahead, push 2 the flank, the mark's push: the mark's spot first), its reward spot, spring radius
2200 (or walking onto its ground for the Webwood), the spawners it waits on (`WaitForSpawners`: only story fights
(Dead Lift's and Amos's hands'), never a camp or patrol: they hold it whenever the player is near). Chosen clear of every camp, patrol, ambush, story fight and safe zone while
open; the Sink floor was left out (the Gravemother's lair shares it), the gate (Main Street's safe zone after Main 3)
and the keeper's road (the sheep fold's ground) too. **To check in phase 2:** the meadow's ground (placed from the layout
only), the yard's spots round the barn, the quarry's against the patrol's corridor.

## The fight (`ABountyFight`, an `ASetPieceSpawner`)

Spawned at the arena when a bounty is taken, and on every level start while it's taken (never saved: encounters
aren't). Its beats: push 1 Hob's line by kind and `Creature.RankSting`; the mark's push `Bounty.Mark.Arrive`, a 0.2
shake and "There's <mark>..."; the mark's death `Bounty.Mark.Down` and Hob's line; done: Hob's line, the Strongbox.
`bReward` off (the chest is the reward), no `DoneFlag` (it repeats). Destroyed when paid, given up, or won and the player
gone past its despawn radius (the chest is put down then).

## Save (`FCampaignRecord::Bounties`, by board id: `FBountyBoardRecord`)

Slots (three `FBountyPosting`s: serial, kind, tier, arena, levels, name, twists, the mark's traits, state Posted /
Taken / Won, when posted), each slot's refill time, the next serial, the rotation and when, the highest serial read,
bounties paid, the strongboxes owed. New within session version 4 (an older save reads as a fresh board: no version
bump). Enums saved as numbers: new tiers, twists, states only ever go at the end.

## Sounds (`Art/Sounds/recipes/bounties.py`, group `bounties`; `LooterSoundCue::Bounty`)

`Bounty.Board.Open` (3D near: sheets rustling, a tack loosened), `Bounty.Board.Posted` (2D, quiet: a tack tapped home
twice, a sheet's flap; a new posting while the player is within 40 m), `Bounty.Take` (2D: a sheet torn off its tack,
folded), `Bounty.Abandon` (2D: paper crumpled), `Bounty.Mark.Arrive` (2D sting: a struck iron bar and a low drum, a
rattle rising), `Bounty.Mark.Down` (2D sting: an iron clang falling away, a shimmer of coin), `Bounty.Chest.Land` (3D:
an iron-bound box into dirt, its hasp rattling, dirt pattering), `Bounty.Paid` (2D: a purse dropped on a counter, a
stamp, a small till bell).

## Feel numbers (cvars `Looter.Bounty.*`, defaults in brackets; `FBountyTuning`)

`Enabled` [1], `RotateMinutes` [120], `RefillMinutes` [10], `PushWhenLeft` [2], `SpringRadius` [2200],
`RewardLuck` [1], `RewardGunsWanted` / `Dangerous` / `Deadly` [2 / 2 / 3], `RewardAmmo` [2], `PurseWanted` /
`Dangerous` / `Deadly` [35 / 55 / 80], `XPShareWanted` / `Dangerous` / `Deadly` [0.05 / 0.07 / 0.09], `ChestDrop` [260
cm], `PostedChimeRange` [4000 cm].

## Dev commands

`Looter.Bounty.List`, `.Post` (new postings now), `.Take <1-3>`, `.Abandon`, `.Win` (the mark dies), `.Collect`,
`.Make <slot> <Kind> <Tier> <Arena> [twist...]`, `.Forget` (the board's record), `.Board` (opens the screen, by the board), `.Arenas [off]` (draws the arenas), `.Clock
<minutes>` (moves the board's clock on).

## Tests

`Looter.Bounty.Rules` (kinds, tiers, twists fit, the draw's determinism and distinct arenas and names, rotation and
refill), `.Rewards` (purse and XP sums, the chest's numbers), `.Pacing` (the side-content band, the falloff, the pacing
paths untouched), `.Play` (a board, an arena and a test campaign: posted, read, taken, the fight springs, its pushes,
the mark named with its traits, won, the chest owed and dropped, paid: XP, silver, the slot refilling later; given up),
`.Save` (the record through a campaign copy; an old campaign reads as a fresh board), `.View` (the cards' words).

## For Main (files not BOUNTY's)

1. `Tools/Unreal/build_area.py` gameplay(), after the placeholders:
   `importlib.reload(importlib.import_module('build_area_bounties')).place(self)`
2. `UI/HUD/LooterHUD.cpp` `IsMenuOpen`: `|| UBountyBoardWidget::GetOpen() != nullptr` (include
   `UI/World/BountyBoardWidget.h`); and the same in `LooterHUDMissionOffer.cpp`'s two refusals.
3. CODEMAP: the lines in "CODEMAP lines" below.
4. Docs after phase 2: Pipeline/Enemies.md (a "Bounties" part), Areas/RansomsRest.md (board, arenas), Progression.md
   (bounty XP and silver beside the vigil stones), Handoff.
5. Shared edits in the holding tree (merge with `git merge-file`): `Session/CampaignRecord.h` (one include, one field),
   `Audio/LooterSoundCues.h` (the `Bounty` namespace at the end); `Art/Sounds/recipes/__init__.py` GROUPS gets
   `'bounties'` (edited in place).
6. Later, an ART ask: a real bounty sheet (poster cell in `looter_posters.py`, the mark's ink portrait by kind) and a
   BOUNTIES header for the board; and a "BOUNTY PAID" variant of the mission banner.

## CODEMAP lines

- `## Bounties` (new section after Missions): the bounty board (Docs/Work/2026-10-22-Next/BOUNTY.md until moved).
- `Bounties/BountyTypes.h`: `EBountyTier`, `EBountyTwist`, `EBountyState`, `FBountyPosting`, `FBountyChestOwed`, `FBountyBoardRecord` (saved in `FCampaignRecord::Bounties`).
- `Bounties/BountyRules`: `BountyRules`: kinds (`FBountyKindInfo`), tiers (`FBountyTierInfo`), twists, the draw (`Draw`), the fight's groups (`MakeGroups`); `BountyRulesPay.cpp` rotation, purse and XP, names in code; `BountyRulesWords.cpp` names, flavour, Hob's lines, card words (`FBountyCardView`).
- `Bounties/BountyTuning`: `FBountyTuning`, `Looter.Bounty.*` cvars.
- `Bounties/BountyArena`: `ABountyArena`, a bounty's ground, spots and reward spot, its story condition and kinds (placed by `build_area_bounties.py`).
- `Bounties/BountyFight`: `ABountyFight`, an `ASetPieceSpawner` made for a taken bounty: its pushes and beats, the named mark and its traits, `OnMarkDown`, `OnDone`.
- `Bounties/BountyChest`: `ABountyChest`, the bounty's Strongbox (an `AChest`): drops in, tells the board when looted.
- `Bounties/BountyBoard`: `ABountyBoard`, the board in town: its sheets and mark, Interact opens the screen.
- `Bounties/BountyBoardSubsystem`: `UBountyBoardSubsystem`, the board's record and clock: posting, rotating, taking, giving up, winning, paying; `BountyBoardSubsystemActions.cpp` taking, giving up, paying, the cards; `BountyBoardSubsystemFight.cpp` the fight, the chest, the tracker, the banner.
- `Bounties/BountyCommands.cpp`: `Looter.Bounty.*` console commands.
- `UI/World/BountyBoardWidget`: `UBountyBoardWidget`, the board screen (three cards, take, collect, give up); `BountyBoardWidgetCards.cpp` the cards.
- Tests: `BountyTests.cpp` (`Looter.Bounty.Rules`, `.Rewards`, `.Pacing`, `.View`), `BountyPlayTests.cpp` (`.Play`, `.Save`).
- `Tools/Unreal/build_area_bounties.py`: the board and the arenas on Ransom's Rest (`Gameplay/Bounties`; gameplay pass after the placeholders; runs alone too).
- `Art/Sounds/recipes/bounties.py`: the bounty board's cues (`LooterSoundCue::Bounty`).
