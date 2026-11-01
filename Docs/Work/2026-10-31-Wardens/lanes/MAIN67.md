# Lane MAIN67: Main 6 "The Pilot Light" and Main 7 "Given to the Wind" (2026-11-01)

Built from `MAINS_2_7.md` §1-3, §10, §11, §14, §15 and the user's calls (`DECISIONS.md`: the Rest's last boss is Abel,
corrupted; the Vigil Stone rematch kept as "the smog's echo of him"; call-backs by a hold at his post; the six dead an
optional save; the Chief Stoker kept; out by train; the barge falls into Gravewind Canyon; Abel's soul back to Saint Ada;
Lucky Ned only heard here) and SECRETS E2 (Main 6 step 4 picks up Paid on Delivery's `BargeManifest`). Nothing built, run,
tested or committed. Ids kept (`Main6`, `Main7`); Abel's HangStep 1 / FightStep 2 / SceneStep 3 now read on Main7.

Worked as five sub-lanes against one contract, `Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md` (names, steps,
ownership, line table): W1 this lane (data, lines, docs, mission tests), W2 FLIGHT6 (the Lamplighter's sortie), W3 BARGE (the
barge's boarding stage), W4 ABEL (Abel corrupted), W5 HOME (Main 7 after the fight); then two compile reviews by reading
(a few fixes, see Risks). **All C++ is staged** under `main67_src/Source/AI_Looter_Shooter/` (copy into `Source/`
over the old files after Main's "C++ OK"). Python, data and docs are in place.

## What the player gets

**Main 6, The Pilot Light** (about 18 min). After Back Pay Hob on the wheel housing ("The captain wants you..."); Holloway at
the Lamplighter's wheel (met first): starving, the barge is coming down over the chapel; Junie re-rigged the Company's own
grapples. Accepting: dusk under thin smog (`DuskSmog`). 0: hold Interact on the anchor winch (the crank backwards, chain
reeled in); she lifts off the ledge, crew bracing, and cruises east over the farm and town. 1: two Company tenders: one
stands off to port with riflemen firing, the other comes alongside starboard and boards two riflemen and a hose-man over
its plank; **their gas-bags are the weak spot** (gas puffs, a leak hiss and list past half, then the tear, a fireball, a
long burning fall into the fields, a crash, `ExplosionDistant`, a smoke column that stays ~40 s); both down and the deck
clear: `Pilot.TendersDown`. 2: alongside the barge (25 m over the chapel knoll, its hose snaking toward the nave's ridge):
hold each starboard launcher 1 s (recoil, the hook arcs over and bites, chain strain); both fired: she's hauled into the
berth (lurch, crew brace), the plank drops; Finch: "Her light's flickering!" 3: the barge's ~68 m deck: three stacks, a
crane, ten cover crates, eight vents flaring in a running pattern (1 s glow, then a 1.4 s fire column that burns anyone in
it; Hob's warning on the first close one), nine Dredgers at level 6; optional: jam the hose drum (2 s: shriek, sparks, the
hose stops short of the roof, Finch's blessing, `Pilot.DrumJammed`), shoot out the four horns (Vane on the bow horn: "Mind
the vents. They bite."). The hatch, a stair down to the catwalk. 4: the manifest in the stoker's office (Hob: "Vane's fare
bill..."; it's carried into Paid on Delivery). 5: the furnace hall (22 m round: the hull's 24.75 m beam won't hold the
design's 26; 9 m high, catwalk at 4 m, two stokeholds, the office): the soul furnace with Abel's light beating in its cage,
the floor's 12-wedge grate ring (4 glow then burn, stepping round every 3 s), the Chief Stoker (Hallowed hose-man, level 7,
x6 health) with his crew in two pushes; three draft valves (1.5 s each: wheel, steam, the ring cooling a notch, the heart
dimming, a line each: Stoker, Hob, Finch); Ned on the radio. 6: the cage (3 s, needs Abel's lantern): **CageBurst** (~7 s):
what comes out is a soot-black shape in a keeper's coat that throws Sully across the hall, tears out through the furnace's
back and streaks west; Vane: "Oh, that's not good." 7: the barge dies (lights out, 12° list, drift west, stern burning, a
stack falling across the port lane at 3.5 s); Holloway cuts the chains and brings her under the broken gangway ("Jump,
Warden!"); a missed jump is caught by Hob onto her deck; on her deck: **BargeFall** (~12.6 s: coal rain over the Rest, a
smoke streak down onto the boards, the plunge into Gravewind Canyon, fireball, distant blast). She flies home and sets down
on the ledge; turned in to Holloway at the wheel (card, then hers, Finch's, Hob's lines). 3,000 XP, 50 silver. After: the
wreck burning below the Rim (with `Pilot.BargeFell`), the Sink barge gone.

**Main 7, Given to the Wind** (about 20 min). Holloway and Finch (one conversation), started on the move. 0: to the boards
by the west road or the keeper's path; the barge's soul coal lies smoking along both (four instanced fields) and 12 lumps
can be cracked (1 s hold each: a soul rises and follows the lantern, a growing trail; Hob's word on the first); the Bone
Hounds on the keeper's road. 1: hang Abel's lantern on the keeper's post; the fog wall seals the gate; he rises out of the
fog. 2: **Abel Ransom, Corrupted** (smog-black, coal furnace-orange, black lantern, smog puffing off him; bar "SMOG", three
gold diamond pips "ABEL"). Phase 1 Stoker: buckshot after the flare, tendrils from the black lantern (drain and pull; break
at 14 m, on a stagger, or by shooting the lantern in its flare), his turn toward the six (coal open, a mutter). At 66% and
33% the bar locks (hits spark), he kneels 6 s: hold 1.5 s on the keeper's post: a pip pops, a third of his colour comes
back, "...Ma? Is that my lantern?" / "Who are you? You're not from here."; missed: again 12 s later. Phase 2 The Hose: out in
the fog, untargetable, drinking the six dead one at a time (12 s; three hits on the line's collar break it; a body drunk
out heals him 5% and rises Restless), the three lanterns dark, relit to drag him back. Phase 3 Keeper: the fog surges to
~15 m sight, gusts, two tendrils, double buckshot, walks to a lantern post and is pulled back (3 s stun, coal open). At zero:
hold 3 s on the post under a weak tendril; the black lantern cracks and falls. Optional: all six saved (`Abel.SixSaved`).
3: **FreeAbel** (17.6 s): the smog boils off; "That's my lantern. You've been looking after it." / "Will you carry me?"; he
steps into it, it burns like a sunrise, Sully takes it down lit, the wall drops; Vane on the gate's horn ("That was a very
expensive barge"). 4: carry him home: an "ABEL'S LIGHT" meter; the west road's five Dredgers (level 6) latch the lantern at
Sully's hip, not Sully (it dims, "DRAINING"; break the line; nothing fails; refills), SheepFold's dead on the road too. 5:
Finch at the Reliquary ("On the lid. Gently."); hold 3 s: **SaintAdaWakes** (~35 s): his light pours into her, she blazes
white-gold, the bell, a ring of light washes over the valley and the smog burns off (`DuskSmog` -> `Dusk` blend), the Unpaid
sink, the six and the coal's souls rise west on the Gravewind past the wreck's smoke, Abel stands by her, Delia has come up
the hill (the four lines). Turned in to Delia at the chapel door (card, then her four lines; with the drum jammed, Finch's
word first): Heirloom by her hand, the Lily on the station board, the hearse car warm. 3,000 XP, 60 silver. After: Day;
Abel's soul by the Reliquary (talkable); the shrouds and coal gone; Finch back aboard (the ship gone on a later load).

## Files

Staged C++ (`main67_src/Source/AI_Looter_Shooter/`), new:
| Lane | Files |
|---|---|
| W2 | `World/PilotSortieRules.h/.cpp` (`EPilotPhase`, `namespace PilotLight` names/steps/math), `World/PilotSortie.h/.cpp` + `PilotSortieFlight.cpp`, `PilotSortieFight.cpp`, `PilotSortieJump.cpp` (`APilotSortie`), `World/LamplighterLauncher.h/.cpp`, `World/LamplighterShipSortie.cpp`, `World/CompanyTenderEscort.cpp`, `Dev/PilotSortieDevCommands.cpp`, `Tests/PilotSortieTests.cpp` |
| W3 | `World/BargeRules.h/.cpp`, `World/BargeLooks.h`, `World/BargeBoarding.h/.cpp` + `BargeBoardingKit.cpp`, `BargeBoardingDeck.cpp`, `BargeBoardingDying.cpp`, `World/BargeFurnace.h/.cpp`, `World/FurnaceDraftValve.h/.cpp`, `World/BargeHoseDrum.h/.cpp`, `Scenes/CageBurst.h/.cpp`, `Scenes/BargeFall.h/.cpp`, `Dev/BargeDevCommands.cpp`, `Tests/BargeBoardingTests.cpp` |
| W4 | `Bosses/AbelCorruptionRules.h/.cpp`, `Bosses/AbelCorruption.h/.cpp` + `AbelCorruptionCallBack.cpp`, `Tendril.cpp`, `Lines.cpp`, `Look.cpp`, `Drink.cpp`, `Surge.cpp`, `UI/HUD/HudBossBarWidgetPips.cpp`, `Scenes/FreeAbel.h/.cpp`, `Tests/AbelCorruptionTests.cpp` |
| W5 | `World/CarriedLanternRules.h/.cpp` (`namespace GivenToTheWind`), `World/CarriedLantern.h/.cpp`, `UI/HUD/HudCarriedLanternWidget.h/.cpp`, `World/SoulFollowers.h/.cpp`, `World/SoulCoal.h/.cpp`, `World/SoulCoalField.h/.cpp`, `World/ReliquaryLantern.h/.cpp`, `Scenes/SaintAdaWakes.h/.cpp` + `SaintAdaWakesWorld.cpp`, `Dev/Main7DevCommands.cpp`, `Tests/GivenToTheWindWorldTests.cpp`, `Tests/GivenToTheWindPlacedTests.cpp` |
| W1 | `Tests/PilotLightTests.cpp` (`Looter.Story.PilotLight.Mission`), `Tests/GivenToTheWindTests.cpp` (`Looter.Story.GivenToTheWind.Mission`) |

Staged C++, changed copies (replace the Source file): W2 `World/LamplighterShip.h/.cpp` (`EShipFlight::Sortie` last,
`GoneWhen`, the sortie hook), `World/CompanyTender.h/.cpp` (BagHit, escort mode, `SpawnRiflemen(Level)`, fall split),
`World/LandfallWinch.h/.cpp` (`CastOffWhen`, `CastOff`); W3 `World/BargeBoarding.h` (W1 made `GetBerth` a
`UFUNCTION(BlueprintCallable)`: the sortie script aims at it); W4 `Bosses/AbelKeeper.h/.cpp`, `AbelKeeperFight.cpp`,
`AbelKeeperMoves.cpp`, `AbelKeeperPose.cpp`, `AbelKeeperShow.cpp`, `Bosses/AbelRules.h/.cpp`, `World/KeeperLanternPost.h/.cpp`
(call-back hold, `GoneWhen`, `TakeDownLit`), `UI/HUD/HudBossBarWidget.h/.cpp` (`SetPips`), `World/BossShrine.h`,
`World/BossShrineCallBack.cpp` (the echo), `Scenes/BossIntro.cpp`, `Dev/AbelDevCommands.cpp`, tests `AbelTestWorld.h`,
`AbelTests.cpp`, `AbelFightTests.cpp`, `AbelShowTests.cpp`, `GravewindTestWorld.h`, `GravewindTests.cpp`,
`BossShrineTests.cpp`, `HeroLinesFamilyTests.cpp`; W5 `World/ChapelReliquary.h/.cpp` (`SetLitNow`, blaze),
`Story/AbelOnBoard.h/.cpp` (`bStanding`, `bSoulLook`), `Creatures/DredgerHoseComponent.cpp` (the lantern latch),
`World/Train.cpp` (warm after Main 7), `Tests/TrainTests.cpp`; W1 `World/DuskScenery.h/.cpp` (`AlsoShownStates`),
`World/TownLifeRules.cpp` (Abel at rest after Main 7), tests `TownLifeTests.cpp`, `MissionTests.cpp` (Main 6 turned in),
`MissionMainStartTests.cpp` (Main 6 met at Speaker_Holloway), `MissionMinePromotedTests.cpp` (labels).

Python (in place): `create_mission_assets.py` (W1: constants, `main6_steps`, `main7_steps`, both specs, docstring),
`create_story_lines.py` (W1: the contract's sets, `ABEL`, BossIntroAbel, DeliaMain7 reworded + Jammed, TillyMain7 reworded,
docstring; no asset deletes: DeliaMain6, AbelOnBoard, AbelAfterMain7, DeliaMain7After, HobMain7Lean now unused),
`story_texts.py` (W1: the "Mains 6-7 (lane MAIN67)" block before `def condition`: `MAIN67_PERCHES`, the Keeper's Gate cue,
`ENCOUNTERS` BargeDeck/FurnaceHall/WestRoadCrew, `TOPICS` Speaker_DeliaChapel/Speaker_Abel), new `build_area_main6.py` (W3),
new `build_area_main6_sortie.py` (W2), `build_area_deck.py` (W4: Main 7's fight, the post's conditions, the six dead's
spots, no Pa's board, the dusk rules), new `build_area_main7.py` (W5), `build_area_depot.py` (W5: train warm after Main 7,
no hand-off, no deck flame). Docs: `Docs/Areas/RansomsRest_Expansion.md` (rows M6/M7, both sections), `Docs/Progression.md`
(the two rows, a Mains 6-7 note), `Docs/Pipeline/Story.md` (one bullet), `TRIPO_MODELS.md` ("Mains 6-7": M67-1..10).

## CODEMAP lines (Main writes)

- New: `World/PilotSortie` (+`PilotSortieFlight/Fight/Jump.cpp`): `APilotSortie`, the Lamplighter's Main 6 sortie (cast off,
  tenders, launchers, berth, jump and catch, BargeFall, home). `World/PilotSortieRules`: `EPilotPhase`, `PilotLight` (Main
  6's names, steps, lines, flight math). `World/LamplighterLauncher`: `ALamplighterLauncher`, the starboard grapple launchers.
- New: `World/BargeRules` (frame, numbers, grate wheel, vents, hose reach, list/drift, fall pose, aboard box),
  `World/BargeLooks.h` (stand-in and Tripo paths), `World/BargeBoarding` (+`Kit`, `Deck`, `Dying`.cpp): `ABargeBoarding`,
  `EBargeState`, Vane's barge over the chapel in Main 6 (walkable deck and furnace hall, vents, hose, keeping the player
  aboard, dying, BargeFall's pose; getters for the sortie and the build script). `World/BargeFurnace`: `ABargeFurnace` (the
  soul furnace, Abel's beating light, the grate ring, the cage hold). `World/FurnaceDraftValve`: `AFurnaceDraftValve`.
  `World/BargeHoseDrum`: `ABargeHoseDrum`. `Scenes/CageBurst`: what comes out of the cage. `Scenes/BargeFall`: the barge
  into Gravewind Canyon.
- New: `Bosses/AbelCorruptionRules` (pips, tendril breaks, flare window, drink, surge, tint), `Bosses/AbelCorruption`
  (+`CallBack`, `Tendril`, `Lines`, `Look`, `Drink`, `Surge`.cpp): `UAbelCorruptionComponent`, Abel's smog (the call-back lock
  and pips, tendrils, the six dead, the fog surge, the last call). `Scenes/FreeAbel`: Main 7's scene, Abel into his lantern.
  `UI/HUD/HudBossBarWidgetPips.cpp`: the boss bar's pip row and bar word.
- New: `World/CarriedLanternRules` (`GivenToTheWind` names/steps/flags; drain/refill and trail math), `World/CarriedLantern`:
  `UCarriedLanternSubsystem` (Abel carried home at Sully's hip; hose lines drain it; the meter; the freed souls),
  `UI/HUD/HudCarriedLanternWidget` ("ABEL'S LIGHT"), `World/SoulFollowers`: `ASoulFollowers`, `ESoulFollow`, `World/SoulCoal`:
  `ASoulCoalLump` (tag SoulCoal, 1 s hold), `World/SoulCoalField`: `ASoulCoalField`, `World/ReliquaryLantern`:
  `AReliquaryLantern` (Reliquary_Lantern, 3 s), `Scenes/SaintAdaWakes` (+`SaintAdaWakesWorld.cpp`): the relighting and its
  end state.
- Change: `Bosses/AbelKeeper` "(Main 6 boss)" -> "(Main 7 boss, corrupted: UAbelCorruptionComponent)"; delete the
  `Scenes/SitWithPa` line; `Story/AbelOnBoard` -> "Abel's soul standing by the Reliquary after Main 7 (bStanding,
  bSoulLook)"; `Story/DoorHandoff` -> "(unplaced since Main 7's rework: Heirloom by hand at the turn-in)";
  `World/KeeperLanternPost` add "the call-back hold (Main 7), gone once Abel's carried"; `World/LamplighterShip` add
  "`LamplighterShipSortie.cpp` (Main 6, `EShipFlight::Sortie`, `GoneWhen`)"; `World/CompanyTender` add "escort mode and the
  gas-bag weak spot (`CompanyTenderEscort.cpp`)"; `World/LandfallWinch` add "cast off (Main 6)"; `World/StoryLighting`
  "(Main 6/7 at Dusk)" -> "(Main 6/7 at DuskSmog, Dusk once Saint Ada wakes)"; `World/DuskScenery` add "`AlsoShownStates`";
  `World/Train` "cold until Main 7" -> "warm after Main 7"; `World/ChapelReliquary` add "blaze (SaintAdaWakes)";
  `UI/HUD/HudBossBarWidget` add "pips (`HudBossBarWidgetPips.cpp`)".
- Dev: `Dev/PilotSortieDevCommands.cpp` (`Looter.Pilot.State/.TendersDown/.Bag/.Launch`), `Dev/BargeDevCommands.cpp`
  (`Looter.Barge.Show [step] [aboard]`, `.Story`, `.Valve [n]`, `.Jam`, `.Cage`, `.Die`, `.Fall`, `.Grates [run|stop]`),
  `Dev/Main7DevCommands.cpp` (`Looter.Coal.Free [n]`, `Looter.Lantern.Carry [1|0|-1]`, `.Lantern.Drain <share>`,
  `Looter.Reliquary.Wake [quiet]`); `Dev/AbelDevCommands.cpp` add `.CallBack`, `.Pips <n>`, `.Tendril <n>`, `.Drink <k>`,
  `.Surge <0|1>`, `.Free`.
- Tests: `PilotLightTests.cpp`, `PilotSortieTests.cpp`, `BargeBoardingTests.cpp`, `AbelCorruptionTests.cpp`,
  `GivenToTheWindTests.cpp`, `GivenToTheWindWorldTests.cpp`, `GivenToTheWindPlacedTests.cpp`; GravewindTests now Main 7's fight
  (`.Fight`, `.Placed`, `.Scenery`; `.Mission` and `.Friend` gone); `LanternLeansTests.cpp`, `LanternLeansPlacedTests.cpp`
  gone (keep `LanternLeansTestWorld.h`: `HeirloomRewardTests.cpp` still tests the unplaced ADoorHandoff).
- Tools: `build_area_main6.py` (Main 6's barge, fights, horns, manifest, the wreck, the Sink barge hidden during Main 6, the
  sortie hook), `build_area_main6_sortie.py` (the sortie's courses, the winch's cast-off, the ship's `gone_when`, Hob's
  wheel), `build_area_main7.py` (soul coal, WestRoadCrew, the Reliquary's lantern rest, Abel's soul, Delia at the chapel
  door); `build_area_deck.py` "Main 6:" -> "Main 7 (Abel corrupted): ... the six dead's spots, DuskSmog/Dusk rules, no Pa's
  board"; `build_area_depot.py` "Main 7: the train ..." -> "the train (warm after Main 7), the depot's place, Hob's spots".

## Exact edits for Main (others' files)

**E1 (required) `Tools/Unreal/build_area_story.py` `place()`.** Old:
```python
    main5_spots = importlib.reload(importlib.import_module('build_area_main5')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots, main5_spots)
```
New:
```python
    main5_spots = importlib.reload(importlib.import_module('build_area_main5')).place(build)
    # Main 6, The Pilot Light (its own module; after the chapel, Landfall and Main 5): the barge over the chapel, its fights,
    # horns and manifest, the wreck; then the Lamplighter's sortie (build_area_main6_sortie). Hob's wheel spot back.
    main6_spots = importlib.reload(importlib.import_module('build_area_main6')).place(build)
    # Main 7, Given to the Wind past the fight (its own module; after the deck and Main 4's Finch): the soul coal, the west
    # road's crew, the Reliquary's lantern rest, Abel's soul, Delia at the chapel door. Hob's spots back.
    main7_spots = importlib.reload(importlib.import_module('build_area_main7')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots, main5_spots, main6_spots, main7_spots)
```
**E2 (required) `place_hob`.** (a) `              landfall_spots=None, main2_spots=None, main5_spots=None):` ->
`              landfall_spots=None, main2_spots=None, main5_spots=None, main6_spots=None, main7_spots=None):`.
(b) after `        'satchel': (main5_spots or {}).get('satchel'),` add:
```python
        # Mains 6-7's (story_texts.MAIN67_PERCHES names them): the Lamplighter's wheel housing while she's moored
        # (build_area_main6_sortie through build_area_main6); a coal lump by the west road and the chapel's front door
        # (build_area_main7.place).
        'wheel': (main6_spots or {}).get('wheel'),
        'roadCoal': (main7_spots or {}).get('roadCoal'),
        'chapelDoor': (main7_spots or {}).get('chapelDoor'),
```
(c) old (from `        # Main 6: on the bier beside Pa's for the scene` through
`        perch(spots['keeperPost'], condition(during=MAIN7), 'DA_Lines_HobMain7Lean'),`) new:
```python
        # Mains 6 and 7 (story_texts.MAIN67_PERCHES, the Wardens story's: each on the first of its spots the build has).
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN67_PERCHES],
```
(The after-perches stay: `hearse` after Main 7, `deckBier` with HobMain6Point after Main 6.)

**E3 (required) `Source/AI_Looter_Shooter/Tests/PacingPaths.h`.** (a) in `EPathRank`, after `Gravebound,` add
`		/** Hallowed (the barge's Chief Stoker, Main 6). */` / `		Hallowed,`. (b) `MainSixStop` new body:
```cpp
		// The Pilot Light (2026-11-01, lane MAIN67): the escort tender's boarders over the Lamplighter's plank (two riflemen and a
		// hose-man; the tenders' own deck riflemen fall with them, uncounted, as in Landfall), the barge's top deck (BargeDeck:
		// six riflemen, three hose-men) and its furnace hall (FurnaceHall: the Chief Stoker, a Hallowed hose-man at 5 + 2 = 7;
		// four riflemen, three hose-men), all kept at level 6, 25 m over the chapel knoll (layout (6700, -1900)).
		FPathStop Stop = { TEXT("Main 6, The Pilot Light"), TEXT("Main6"), {}, 3000 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 2, TEXT("LamplighterSortie"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("LamplighterSortie"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 6, TEXT("BargeDeck"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 3, TEXT("BargeDeck"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("FurnaceHall"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 3, TEXT("FurnaceHall"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Hallowed, 1, TEXT("FurnaceHall"), 6700.0, -1900.0, 5, 5));
		return Stop;
```
(c) `MainSevenStop()` -> `MainSevenStop(const FPathCast& Foes, bool bWithHounds)`:
```cpp
	inline FPathStop MainSevenStop(const FPathCast& Foes, bool bWithHounds)
	{
		// Given to the Wind (lane MAIN67): about twelve of Abel's adds in a 5-6 minute fight, then Abel (7); the west road's
		// crew off the barge on the carry home (WestRoadCrew: three riflemen, two hose-men, kept 6). The keeper's road's Bone
		// Hounds (placeholders) on the way to the boards, met by everything once (the keeper's path is optional).
		FPathStop Stop = { TEXT("Main 7, Given to the Wind"), TEXT("Main7"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 12, TEXT("Abel")), MakeKills(Foes.Abel, EPathRank::Own, 1, TEXT("Abel")) }, 3000 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("WestRoadCrew"), 2800.0, -7900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("WestRoadCrew"), 2800.0, -7900.0, 6, 6));
		if (bWithHounds)
		{
			AddKills(Stop.Kills, PlannedKills(Foes.BoneHound, EPathRank::Basic, 4, TEXT("KeepersRoadHounds"), 2000.0, -10000.0));
			AddKills(Stop.Kills, PlannedKills(Foes.BoneHound, EPathRank::Restless, 1, TEXT("KeepersRoadHounds"), 2000.0, -10000.0));
		}
		return Stop;
	}
```
(d) `StoryPath`: `MainSixStop(Foes), MainSevenStop() };` -> `MainSixStop(Foes), MainSevenStop(Foes, /*bWithHounds*/ false) };`;
`EverythingPath`: `Path.Add(MainSevenStop());` -> `Path.Add(MainSevenStop(Foes, /*bWithHounds*/ true));`; in the "Stone
Sentinels, the keeper's road's hounds" stop delete the two `KeepersRoadHounds` lines, label -> `TEXT("Stone Sentinels")`,
its comment "and the keeper's road's Bone Hounds until The Gravewind" -> "(the hounds moved to Main 7, lane MAIN67)".
(e) The header's top comment: add "On 2026-11-01 (lane MAIN67) Main 6 took the barge's 20 Dredgers and the Chief Stoker,
Main 7 Abel (from Main 6) and the west road's crew: Main 6 5,000 -> 3,000, Main 7 4,500 -> 3,000."
**E4 (required with E3) `Tests/ProgressionPacingTests.cpp` `ExpectedKillXP`'s switch:** after the `Gravebound` case add
```cpp
		case EPathRank::Hallowed:
			Odds.Add({ ECreatureRank::Hallowed, 1.0 });
			break;
```
**E5 (required) `Art/Levels/RansomsRest/layout.json`.** (a) `gameplay.encounters.storyFights`: after the IntakeSpiders line add
```json
        { "id": "BargeDeck", "obstacle": "churchyardFence", "during": "Main6", "by": "build_area_main6.py (the barge's deck 25 m over the chapel knoll: six riflemen, three hose-men, level 6; The Pilot Light, 2026-11-01)" },
        { "id": "FurnaceHall", "obstacle": "churchyardFence", "during": "Main6", "by": "build_area_main6.py (the furnace hall: the Chief Stoker, Hallowed 7; four riflemen, three hose-men, level 6)" },
        { "id": "WestRoadCrew", "at": [2800, -7900], "radius": 3000, "during": "Main7", "by": "build_area_main7.py (the barge's crew on the west road: three riflemen, two hose-men, level 6; Given to the Wind)" },
```
and old `        { "id": "Abel", "zone": "gravewindPoint", "during": "Main6", "by": "build_area_deck.py" },` new
`        { "id": "Abel", "zone": "gravewindPoint", "during": "Main7", "by": "build_area_deck.py (Abel, corrupted: Given to the Wind, 2026-11-01)" },`.
(b) camps SheepFold: `"countedBefore": "Main6",` -> `"countedBefore": "Main7",` and its note's "(Main 6's two ways to the
deck)" -> "(Main 7's two ways to the deck, and the carry home)". (c) `level.environment.states`, after SmogThin's closing
`}` add `,` and:
```json
        "DuskSmog": {
          "about": "Dusk under thin smog (Mains 6-7, lane MAIN67): the barge starved and down but its smog still over the Rest; build_area_deck.py's AStoryLighting rules (Main 6, and Main 7 until Saint Ada wakes); SaintAdaWakes blends it to the clean Dusk. Dusk's sun, a little dimmer; the haze thicker and greyed, its glow toward the sun dulled to brass; the far ranges and clouds greyed.",
          "sun": { "azimuth": 252, "elevation": 4, "intensity": 3.4, "temperature": 3500, "shadowDistance": 6000, "cascades": 2 },
          "sky": { "intensity": 0.5 },
          "atmosphere": { "skyLuminance": [1.05, 1.6, 3.4], "ozone": 2.6 },
          "fog": { "density": 0.0065, "inscattering": [0.08, 0.085, 0.09],
                   "directional": { "color": [0.75, 0.42, 0.18], "exponent": 6.5 } },
          "backdrop": { "tint": [0.34, 0.32, 0.28] },
          "clouds": { "tint": [0.28, 0.2, 0.16] },
          "post": { "exposureBias": 0.45 }
        }
```
then `build_area.py RansomsRest environment` before the gameplay pass.
**E6 `Tools/Unreal/build_area_main5.py` `place_smog_rule`:** `rule.set_editor_property('when', story.condition(after=[MAIN5]))`
-> `rule.set_editor_property('when', story.condition(after=[MAIN5], before=['Main7']))` (Day after Main 7; its log line
"SmogThin after {MAIN5}" -> "SmogThin after {MAIN5} until Main7").
**E7 `Tools/Unreal/build_area_farm.py` `delia_topics`:** delete the `lantern = story.lines(build, 'DA_Lines_DeliaMain6')`
block (5 lines: Main 6 is Holloway's now; its event `Delia.Main6` started nothing since 2026-10-22) and replace
```python
    said += story.topics(build, (story.condition(during=MAIN7, from_step=1), 'DA_Lines_DeliaMain7After'),
                         (story.condition(during=MAIN7), 'DA_Lines_DeliaMain7'))
```
with `    said += story.topics(build, (story.condition(during=MAIN7), 'DA_Lines_DeliaMain7After'))` (her porch word while Main 7
runs: "I'll keep setting a plate."; the turn-in is the chapel Delia's) and its comment "Main 7: Heirloom handed out ..." ->
"Main 7 (Given to the Wind): a word on the porch; her turn-in is at the chapel door (build_area_main7.py)".
**E8 `Tools/Unreal/build_area_main4.py` `place_finch`:** `finch.set_editor_property('shown_when', story.condition(after=[MAIN4]))`
-> `story.condition(after=[MAIN4], before=['Main7'])` (Finch back aboard after Main 7).
**E9 `Tools/Unreal/build_area_main2.py` `place_shrouds`:** `    hidden = [story.condition(after=[MAIN7])]` ->
`    hidden = [story.condition(after=[MAIN7]), texts().condition(dict(with_flags=['Main7.SaintAdaAwake']))]` (the shrouds lift
with the river of souls).
**E10 `Tools/Unreal/build_area_shrines.py` SHRINES' Abel:** `name='Abel Ransom, the Keeper'` -> `name="The smog's echo of Abel"`,
`beaten_after=['Main6'], callable_after=['Main6']` -> `beaten_after=['Main7'], callable_after=['Main7']`; its docstring
"Abel (Main 6)" -> "Abel's echo (after Main 7: the furnace's shadow of him; the real Abel keeps Saint Ada)".
**E11 `Tools/Unreal/build_area_placeholders.py` KeepersRoadHounds:** `after=['Main4'], before=['Main6']` ->
`after=['Main6'], before=['Main7']`, its where's "(after Hallowed Ground, until Abel keeps his board after The Gravewind)" ->
"(once the barge falls, until Abel is home: they raid the boards he can't sit up with)".
**E12 `Tools/Unreal/create_bestiary_pages.py` Abel's page:** `known_when=dict(after=['Main6'])` -> `dict(after=['Main7'])` and
its name/words to "Abel Ransom, Corrupted" (the Warden's Log: "A week in Vane's furnace twisted the keeper. Freed, he keeps
Saint Ada from inside her.").
**E13 git:** `git rm Source/AI_Looter_Shooter/Scenes/SitWithPa.h Source/AI_Looter_Shooter/Scenes/SitWithPa.cpp
Source/AI_Looter_Shooter/Tests/LanternLeansTests.cpp Source/AI_Looter_Shooter/Tests/LanternLeansPlacedTests.cpp`
(keep `LanternLeansTestWorld.h` and `HeirloomRewardTests.cpp`; ADoorHandoff stays in code, unplaced: TrainDevCommands uses it).
**E14 cosmetic, any time:** `Story/AbelOnBoard.h` comment "SitWithPa" -> "FreeAbel"; `World/Train.h` comments "after Main 6"
-> "after Main 7"; `World/LanternFlame.cpp` `LitAfter` Main6 (the class is unplaced on the deck now).
**E15** CODEMAP (above); `Docs/Handoff.md` entry; `Docs/Story.md` gets §2's lore (Warden's eyes, a saint and her keeper, what
the furnace does to a soul, the Rest's barge, why the train) when the user approves the playtest.

## Run order (Main)

1. Copy `main67_src/Source/` over `Source/` (after "C++ OK"); E1-E14. 2. Build (`Tools\launch.ps1 -Build`).
3. `create_story_lines.py`, `create_mission_assets.py`, `create_side_mission_assets.py` (Paid on Delivery needs Main6),
`create_bestiary_pages.py`. 4. `build_area.py RansomsRest environment` (DuskSmog, E5c). 5. `build_area.py RansomsRest
gameplay`; read the log for: "the barge over the chapel at ..." (hull top vs DeckTop 1400), "BargeDeck: 9 Dredgers",
"FurnaceHall: 8", "four Company horns", "the barge's manifest", "the barge's wreck", "Landfall's barge ... hidden", "the Main 6
sortie: her course out ... raised Nx" (warnings "still touches something", "no barge", "no anchor winch"), "the keeper's
post: hung in Main7 from step 1, gone from step 4", "Abel on the deck ... 6 dead on the boards", "the story's light: Main7
from step 6 at Dusk ...", "ADuskScenery has no AlsoShownStates yet" (the DuskScenery copy missing), "Abel has no corruption
component" (module not rebuilt), "soul coal lumps placed, not 12", "Delia isn't at the chapel door", "WestRoadCrew:",
"no Reliquary or Place_GravewindPoint", Hob "no spot for" lines. 6. Tests below. 7. PIE walkthrough. 8. Perf
(`Tools\perf.ps1`): on the barge's deck mid-fight (vents, ~100 hidden boxes, 9 Dredgers), in the furnace hall (grate ISM,
furnace light 120 cd, 8 Dredgers), from the Lamplighter's deck during the tender fight, on the boards in Abel's phase 3 (fog
surge, tendrils, adds), and the wide shot of SaintAdaWakes (the wave, the river of souls).

## Tests (once, at commit)

New: `Looter.Story.PilotLight`, `Looter.World.PilotSortie`, `Looter.World.Barge`, `Looter.Bosses.Abel` (with `.CallBack`,
`.Pips`, `.Tendril`, `.Drink`, `.Surge`, `.Free`), `Looter.Story.GivenToTheWind` (`.Mission`, `.Board`, `.Placed`),
`Looter.World.SoulCoal`, `Looter.World.CarriedLantern`, `Looter.World.ReliquaryLantern`, `Looter.Scenes.SaintAdaWakes`.
Touched: `Looter.Story.Gravewind` (`.Fight/.Placed/.Scenery`), `Looter.World.BossShrine`, `Looter.Lines` and
`Looter.Story.HeroLines`/`Looter.Lines.NoFamily`, `Looter.Train`, `Looter.World.TownLife`, `Looter.Missions.Definitions`,
`Looter.Missions.MainStart`, `Looter.Missions.MinePromoted`, `Looter.Story.PaidOnDelivery`, `Looter.Progression.Pacing` (after
E3/E4), `Looter.Story.Landfall` (the ship changed underneath), `Looter.World.CompanyLoudspeaker`, `Looter.Creatures.Dredger`
(the latch), `Looter.Scenes.BossIntro`, `Looter.Bestiary.Entries`, `Looter.UI.Boss*` (the bar), `Looter.Story.Delia`.

## PIE walkthrough

1. `Looter.Mission.Complete BackPay`: Hob on the wheel housing; talk to Holloway (her three lines, Main 6 starts, dusk under
   smog). Hold Interact on the winch: lift, cruise east (`Looter.Pilot.State Tenders` jumps).
2. Two tenders: shoot both bags (puffs, hiss, list, tear, fire, the fall, crash, smoke); kill the three boarders: the step
   ticks. `Looter.Pilot.Bag 1` shows the hit capsule.
3. Alongside: hold both launchers; the haul, the plank; walk across. `Looter.Barge.Show 3 aboard` jumps here.
4. Deck: vents (Hob's word), nine Dredgers, Vane from the bow horn; jam the drum (`Looter.Barge.Jam`), shoot the horns; the
   hatch; the office and the manifest (Hob; the manifest in Carried).
5. Hall: Ned on the radio, the Stoker and Vane; the ring turning; three valves (a line each), the two pushes.
6. The cage (3 s): CageBurst. 7: the list, the stack's fall, Holloway under the gangway; jump (miss it once: Hob's catch);
   BargeFall; home; turn in at the wheel (card, four lines): Main 7 starts; reload: the wreck below the Rim.
7. Main 7: crack lumps (souls follow); the boards; hang the lantern; the fight as written above (`Looter.Abel.CallBack`,
   `.Pips 2`, `.Drink 0`, `.Surge 1`, `.Free`); let one body be drunk once (Restless rises, the optional fails; reset by
   dying: all six back).
8. FreeAbel; the meter; the west road (latches drain the meter, not health); the chapel, Finch's word, the 3 s hold:
   SaintAdaWakes; turn in to Delia at the door (with and without `Pilot.DrumJammed`): Heirloom at her feet, the Lily on
   the board, the train warm. Reload: Day, Abel's soul by the Reliquary, no shrouds, no coal; after a later load no ship.
9. The Vigil Stone by the Keeper's Gate after Main 7: "the smog's echo of him", fought for his loot; the real Abel stays.

## Pacing (paper; re-copy from the test)

Per kill: XP x level^1.2 x rank (rifleman 14, hose-man 18; Hallowed x6 XP, +2 levels). Main 6 new kill XP about 3,640 at
level 6-7 (12 riflemen, 7 hose-men, the Chief 1,116), Main 7 about 3,360 (Abel 2,066, ~12 adds, 5 Dredgers); the old Main 6
was 3,368 (sheep fold, adds, Abel). Net +4,070 story, +3,830 everything. With Main 6 3,000 and Main 7 3,000 (-3,500):
story about 41,440 (6.99, band 6.7-7.1), everything about 54,500 (7.68, band 7.3-7.7: tight), Abel met at about 6.5 (story)
and 7.4 (everything). **The missions' share of the story's XP falls to about 51%, under the test's 60% floor**: the barge's
20 Dredgers on the main path can't be paid for out of mission XP alone without breaking the band. Main/the user: lower that
floor to 50% (the Wardens story is a war with a Company), or cut the barge's fights (e.g. BargeDeck 9 -> 6, the boarders
off). Trim Main 7 to 2,600 first if everything goes over 7.7.

## Risks I couldn't check

- Compile: everything written against the headers and read twice (two review passes fixed a missing include in
  BargeBoarding.cpp, a TPair loop in BargeFurnace, `CeilToInt32`, a null world in Main7DevCommands, an include in
  AbelCorruptionTests). Files a little over ~500 lines: AbelKeeper.h 526, AbelKeeperFight.cpp 510, KeeperLanternPost.cpp 508,
  SaintAdaWakes.cpp 518 (split when next touched).
- Measures guessed: the barge's walkable `DeckTop` 1400 over SM_DredgeBarge's keel (its own stacks/crane may poke through the
  kit's deck: tune `KitLift`); the tender bag capsule (150 x 500); the hall 22 m. ~100 hidden boxes on the barge actor.
- The barge hangs at a fixed spot 25 m over the knoll; spawns 39 m up use `ground_max_rise` 300: check the Dredgers land on
  the deck, and none steps off the berth's rail gap before the plank lands.
- Moving decks: tender riflemen are re-placed each frame; the barge's list/drift carries the player as a moving floor
  (like the ship); W3's 1.6 s escape grace and W2's jump catch overlap (both put the player on her deck: fine, check once).
- No 3 s plank scene (the plank swings down in 0.8 s under control). The breach in the furnace's back wall isn't cut, the
  Dredgers fleeing for a tender and the Stoker's harder latch as it cools aren't built, the Sink barge's drone still hums in
  Main 6, Abel's grave doesn't thaw (static dressing).
- Abel: hits on his collar/lantern parts (on his actor) registering; the Unpaid coal colour may be reset on respawn; drunk
  shrouds re-hidden each frame (AStoryProp shows them again); the boss wall drops at the kill, not after FreeAbel;
  `Abel.SixSaved` is sent at the second call-back (phase 2's end: none can be drunk after), so the optional ticks before the
  kill; `Looter.Abel.Phase 3` needs `.CallBack` after it now.
- Two Delias exist during Main 7 steps 5-6 (the porch one stays; they're far apart). Unpaid destroyed by SaintAdaWakes may be
  spawned again by their spawners. The soul coal field's ISM is Static mobility with runtime instances (confirm it draws).
- Hob has no perch while she flies (he waits on the chapel hood; his lines come as captions); his wheel perch only holds
  while she's moored.
- Rule slips: one sed on my own docstring line in create_story_lines.py, one node one-liner by W3 on its own staged test.

## Numbers to tune

Flight: lift 6.5 s / 9 m; cruise 560 cm/s, 320 fighting, 780 home; tenders at 16% of the course or 14 s; bag 230 (grown to
6); approach 9 s, haul 3.2 s, jump glide 5 s, settle 7 s; catch 1.5 m under her deck. Barge: 25 m over the knoll, drum 7 m
beside the ridge; grates 4 of 12, 3 s, 1 s tell, 14%/s; vents every 6 s, 1 s tell, 1.4 s burn, 22%/s, 1.3 x 4.5 m; holds
drum 2 s, valves 1.5 s, cage 3 s; dying list 12° over 6 s, drift 150 cm/s to 60 m, the stack at 3.5 s for 35%; fall 12 s, 40
lumps; furnace 120 cd, deck lamps 35 cd; activation deck 35 m, hall 22 m. Abel: `Looter.Abel.HealthScale` for 5-6 min; locks
0.66/0.33; kneel 6 s, retry 12 s; holds 1.5 s / 3 s; tendrils first 9 s then 14 s (12 in phase 3), drain 0.3 x AttackDamage/s,
break 14 m, longest 6 s; drink 12 s, 3 hits, 5% heal; surge density 0.05, start 200, in 3 s out 2.5 s. Home: drain 0.07/s per
line, floor 0.15, refill 0.22/s after 1.5 s; followers 12; release 14 s; scene 34.6 s; blaze 90 cm / 90 cd / 16 m; wave to
650 m over 6 s; crew at 55% of the west road. XP: Main 6 3,000, Main 7 3,000.
