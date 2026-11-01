# Lane MAIN23: Main 2 "The Ferryman's Price" and Main 3 "Under New Management" (2026-11-01)

Built from `MAINS_2_7.md` §4-5 as revised to the user's calls (`DECISIONS.md` "Mains 2-7 calls"). Nothing built, run in
Unreal or committed. Python compiled with UE's python (`py_compile`) and checked for undefined names (an ast pass); C++
written against the headers it calls (read, not compiled: risks below). Ids kept: `Main2`, `Main3`; Main 2's Log step stays
3 (`Ledger::HandedOverStep`); `TownGate`'s id and count (7), `BluffNest`'s id and numbers kept; `LanternBail` handed at
Main 3's turn-in as before. No Tripo calls.

## What the player gets

**Main 2, The Ferryman's Price** (Hob's offer on the move, 550 XP). 0: the keeper's path to Gravewind Point (25 m round
the Keeper's Gate), the Three Widows' spider nest on the way (moved off the Point: Landfall climbs it), Hob muttering on
Keeper's Bridge (optional). 1: a Company crew pumping the week's six shrouded dead off the burial boards into a hose-cart:
3 riflemen in the biers' cover and a hose-man at the cart (level 2), at work as the player comes up the cairns; Hob on the
gate rock says why and how (shut the valve, or burst the tank; the horn is Vane's mouth). Stopping it either way spills
four half-dredged dead rising round the cart (Hob: "That's what the hose leaves..."), then Vane on the Keeper's Gate
loudspeaker ("That cart is Company property." / "I'm adding it to your bill."); shoot the horn out mid-line and he's cut at
"your bi—" (optional). 2: down the rope ladder (a 2.6 s climb scene) to the Ferryman's railed landing hung under the deck's
west end; his black ferry sways on chains beside it in its own fog, prow lantern lit; Sexton in the stern sells the plan
(starve the barge; a keeper's soul goes in his own lantern) and hands up the Log. 3: open the Warden's Log. Turn-in (card,
then): his price, the barge's manifest; "Come back when you've silver." He stays at his ferry.

**Main 3, Under New Management** (Delia met first, 1,000 XP, Uncommon gun, 40 silver). 0: the farm road; as it starts
(into a quiet moment) Lucky Ned's dark skiff casts off from the barge and flies south over the Hogback out of the valley,
Vane and Ned on the radio, Hob and Sully after (once a campaign). 1: the Company's checkpoint at the town gate: sandbags,
a barrier pole, a VANE CONSOLIDATED / ASSESSMENT IN PROGRESS / NO ENTRY. NO EXIT. board; 3 riflemen behind the sandbags
and a hose-man guarding from the road, then 2 Unpaid and a Restless one drawn by the smog; Vane on the gate's horn as it
begins. 2: Main Street (its set piece, gaining 2 riflemen: layout edit E7c), ASSESSED / n SOULS / V.C. chalked on the doors,
a hose-cart assessing the Gilt Spur (burstable), Vane on Main Street's horn once the street is won; optional: shoot both
ropes of the UNDER NEW MANAGEMENT banner and it falls into the street and stays down. Turn-in to Tilly Bright (card, then
her three lines; the bail and the box). After: "Every coffin in my cellar's salted, Warden." The barrier goes after Main 3.

## Files

New C++ (all `Source/AI_Looter_Shooter/`):
| File | What |
|---|---|
| `World/CompanyLoudspeaker.h/.cpp` | `ACompanyLoudspeaker`, `FLoudspeakerCue`: Vane's horn post. `Broadcast(Lines)` (UVaneBroadcast), `Cues` (once a visit, condition + earshot 60 m, waits ≤6 s for quiet), horns shootable (UHealthComponent 60, a Weapon-only capsule; the pole ignores bullets), `Silence` (sparks, crackle, cuts Vane mid-word: `CutOff`), `bShowPost` off = a voice only. Tags `CompanyLoudspeaker`, `Obstacle`. Soft path `/Game/Art/Tripo/SM_CompanyLoudspeaker`. |
| `World/CompanyHoseCart.h/.cpp`, `World/CompanyHoseCartSpill.cpp` | `ACompanyHoseCart`, `EHoseCartState`: pump cart, hose pulsing into what it drinks; `ShownWhen` / `PumpingWhen` (stoppable only then) / `StoppedWhen`; valve hold 1.5 s or tank burst (150 on `TankHit`, soul-cyan blast, 35% to creatures in 3.5 m); stop → mission event `HoseCart.Stopped`, `SpillLines`, crew spawner's next wave at `SpillSpots`, then Vane's `StopBroadcast` from `LoudspeakerTag` once captions are quiet. Soft path `/Game/Art/Tripo/SM_CompanyHoseCart`. |
| `World/CompanySign.h/.cpp` | `ACompanySign`, `ECompanySignLook {Board, Chalk}`: drawn words (UTextRenderComponent) on a black board on posts, or chalk on a wall; ShownWhen/HiddenWhen. Soft path `/Game/Art/Tripo/SM_CompanyCheckpointSign`. |
| `World/CompanyBanner.h/.cpp` | `ACompanyBanner`: 12 m canvas, words both faces, two shootable ropes (cut → sag, both → falls flat into the street, dust), `DownEvent` to missions, `DownFlag` keeps it down. Soft path `/Game/Art/Tripo/SM_CompanyBanner`. |
| `World/SextonsFerry.h/.cpp` | `ASextonsFerry`: the landing (4 x 6 m planks, rails, pawn-only blockers 1.9 m, chains to the deck) 3.8 m under the boards, the ferry (stand-in cubes, MI_Charcoal) swaying on chains with a prow lantern and point light, a fog bank; `Passenger` (Sexton) snapped to its stern `Seat`. Soft path `/Game/Art/Tripo/SM_SextonsFerry`. |
| `World/FerryLadder.h/.cpp` | `AFerryLadder`: stanchions, ropes, 9 rungs over the deck's edge; tap Interact at either end: "Climb down to the ferry" / "Climb up"; `Climb` (the scene, or straight there with scenes off); `GetPerch` (Hob). Tag `FerryLadder`. |
| `Scenes/FerryLadderClimb.h/.cpp` | `FerryLadderClimb`: the climb as a scene (`FerryLadder`, 2.6 s, skippable): body stood at the far end first, the view turns to the ladder, rung by rung, steps off; `RungAlpha`, `EyeAt` (plain math). |
| `World/GangSkiffFlyBy.h/.cpp` | `AGangSkiffFlyBy`: Ned's skiff (SM_Skiff_A_Gang) flown along `Course` with `FColdOpenCourse` (the cold open's flight), `PlayWhen`, `RadioLines` via UVaneBroadcast at 2.5 s, `AfterLines` queued once the radio is off, `DoneFlag` (`Main3.NedLeft`) so once a campaign. |
| `Dev/Main23DevCommands.cpp` | `Looter.HoseCart.Stop [burst]`, `Looter.Loudspeaker.Silence`, `Looter.Loudspeaker.Say`, `Looter.FlyBy.Ned`, `Looter.Ferry.Climb`, `Looter.Banner.Cut`. |
| `Tests/CompanyPropsTests.cpp` (new) | `Looter.World.CompanyLoudspeaker.CutOff/.Silence`, `Looter.World.HoseCart.Stop/.Story`, `Looter.World.CompanyBanner.Ropes`, `Looter.World.CompanySign.Look`, `Looter.World.GangSkiffFlyBy.Flight`. |
| `Tests/SextonsFerryTests.cpp` (new) | `Looter.World.SextonsFerry.Landing/.Ladder`, `Looter.Scenes.FerryLadder.Climb`. |
| `Tests/UnderNewManagementTests.cpp` (new) | `Looter.Story.UnderNewManagement.Mission/.Lines/.Placed`. |
| `Tests/TalkBusinessTests.cpp` (rewritten: Main 2's own test file) | now `Looter.Story.FerrymansPrice.Mission/.Flow/.Lines/.Placed` (the old `Looter.Story.TalkBusiness.*` are gone). Suggest `git mv` to `FerrymansPriceTests.cpp`. |
| `Tests/ColdWelcomeTests.cpp` (edited: Main 3's own) | Tilly's new words (constants, fallback lines, "she hands over the box"), the Placed gate checks (3 riflemen, 1 hose-man, 2+1 Unpaid, from step 0, not hunting); includes DredgerRifleman/HoseMan. Names unchanged (`Looter.Story.ColdWelcome.*`). |

Python (`Tools/Unreal/`):
- `create_mission_assets.py`: docstring entries, constants (SULLY, SEXTON_TAG, BOARDS_CART, BOARDS_CREW, CART_STOPPED,
  GATE_LOUDSPEAKER, TOWN_GATE, TILLY_TAG, BANNER_TAG, BANNER_DOWN), `main2_steps`, `main3_steps`, the Main2 and Main3 specs.
- `create_story_lines.py`: `NED = 'Lucky Ned'`; Main 2 sets HobMain2Blue (reworded), HobMain2Bridge, HobMain2Cart,
  HobMain2Spill, Main2VaneCart, HobMain2Ladder, SextonWaiting/Deal/Ledger/After (reworded); Main 3 sets HobMain3Road/Gate/
  Tilly (reworded), Main3Radio, Main3NedAfter, Main3VaneGate, Main3VaneStreet, TillyClosed/Main3/AfterMain3 (reworded),
  DeliaMain3. No set retired (all old names kept or reworded: no asset deletes).
- `story_texts.py`: LookoutStains marked retired; TOPICS `Speaker_Delia` + `(during Main3) DeliaMain3`; new
  `Speaker_Sexton`; ENCOUNTERS `BluffNest` (during Main2 before step 2), `BoardsCrew` (same), `TownGate` (during Main3
  before step 2); new `MAIN23_PERCHES`, `MAIN23_AFTER_PERCHES`, `MUTTER_POINTS`, `LOUDSPEAKER_CUES`, `NED_LEFT_FLAG`,
  `BANNER_DOWN_FLAG`; docstring.
- `build_area_main2.py` (new): the Widows' nest, Keeper's Bridge's mutter point, the Keeper's Gate loudspeaker, six
  shrouded dead (AStoryProps on biers 1,4,5,6,7,8 from Main 2 until Main 7), the cart in the aisle, `BoardsCrew`
  (AAmbushSpawner, Rise, level 2, 2 waves, wave 2 only on the cart's word, ground = zone gravewindPoint), the ferry +
  landing (with a clearance check that warns), Sexton (`MisterSexton_Ferry`, shown after Main1, topics from TOPICS), the
  ladder; removes stale `MisterSexton`, `Encounter_BluffNest` first. Returns `{'widows', 'ladderTop'}`.
- `build_area_main3.py` (new): the checkpoint (sandbags, barrier sign hidden after Main 3, the board), `TownGate`
  (AAmbushSpawner Rise: 3 riflemen, 1 hose-man, 2 Unpaid + 1 Restless leader, max 4, from step 0, not hunting; removes the
  stale one), the gate loudspeaker + Main Street's voice (on WORLD's middle post marker if found), the banner (ropes tied
  where traces meet the facades), ASSESSED chalk on ten buildings, the Gilt Spur cart, Ned's skiff.
Docs: `Docs/Areas/RansomsRest_Expansion.md` (flow rows M2/M3, the Main 2 and Main 3 sections rewritten),
`Docs/Pipeline/Story.md` (new section "Story missions as world pieces"), `Docs/Pipeline.md` (its line),
`Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md` ("Mains 2-3").

## CODEMAP lines (Main writes)

- `World/CompanyLoudspeaker`: `ACompanyLoudspeaker`, `FLoudspeakerCue`: a Company horn post (Vane's voice by story cue or on request; horns shot out cut him mid-word; a voice only without its post). `World/CompanyHoseCart` (+`CompanyHoseCartSpill.cpp`): `ACompanyHoseCart`, `EHoseCartState`: the pump cart drinking the dead (valve hold or tank burst; the spill, the crew's next wave, Vane's complaint).
- `World/CompanySign`: `ACompanySign`, `ECompanySignLook`: drawn Company words on a board or chalked. `World/CompanyBanner`: `ACompanyBanner`: Main Street's banner, two shootable ropes, falls into the street.
- `World/SextonsFerry`: `ASextonsFerry`: the Ferryman's landing and swaying ferry under Gravewind Point, Sexton in its stern. `World/FerryLadder`: `AFerryLadder`: the rope ladder down to it (a tap at either end). `Scenes/FerryLadderClimb`: the climb as a scene.
- `World/GangSkiffFlyBy`: `AGangSkiffFlyBy`: Lucky Ned's skiff leaving the barge over the Hogback (Main 3, once).
- `Dev/Main23DevCommands.cpp`: `Looter.HoseCart.Stop`, `Looter.Loudspeaker.Silence/.Say`, `Looter.FlyBy.Ned`, `Looter.Ferry.Climb`, `Looter.Banner.Cut`.
- Tests: `CompanyPropsTests.cpp` (`Looter.World.CompanyLoudspeaker.*`, `.HoseCart.*`, `.CompanyBanner.Ropes`, `.CompanySign.Look`, `.GangSkiffFlyBy.Flight`), `SextonsFerryTests.cpp` (`Looter.World.SextonsFerry.*`, `Looter.Scenes.FerryLadder.Climb`), `UnderNewManagementTests.cpp` (`Looter.Story.UnderNewManagement.*`), `TalkBusinessTests.cpp` now `Looter.Story.FerrymansPrice.*` (was TalkBusiness; rename the file to `FerrymansPriceTests.cpp`).

## Exact edits for Main (in others' files)

**E1 (required) `Tools/Unreal/build_area_story.py` `place()`: the hooks.** Old:
```python
    landfall_spots = importlib.reload(importlib.import_module('build_area_landfall')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots)
```
New:
```python
    landfall_spots = importlib.reload(importlib.import_module('build_area_landfall')).place(build)
    # Main 2, The Ferryman's Price (its own module, reloaded as the chapel's is; after the deck and Landfall): the Widows'
    # nest, the boards' cart, crew and dead, the gate's loudspeaker, the Ferryman's landing, ferry and ladder; Hob's spots.
    main2_spots = importlib.reload(importlib.import_module('build_area_main2')).place(build)
    # Main 3, Under New Management (its own module): the checkpoint and its fight, the loudspeakers, the banner, the chalk,
    # the Gilt Spur's cart, Lucky Ned's skiff.
    importlib.reload(importlib.import_module('build_area_main3')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots)
```

**E2 (required) `place_hob`.** (a) signature old `              landfall_spots=None):` new `              landfall_spots=None, main2_spots=None):`.
(b) in `spots = {...}`, after `        'abelBoard': board,` add:
```python
        # Mains 2-3's (story_texts.MAIN23_PERCHES names them): the middle Widow's top and the ferry ladder's crossbar,
        # measured by build_area_main2.place.
        'widows': (main2_spots or {}).get('widows'),
        'ladderTop': (main2_spots or {}).get('ladderTop'),
```
(c) old:
```python
        # Main 2: beside Sexton for the deal and the Ledger; on the rock for the fight; at the path's foot for the climb.
        perch(spots['rail'], condition(during=MAIN2, from_step=2)),
        perch(spots['rock'], condition(during=MAIN2, from_step=1), 'DA_Lines_HobMain2Blue'),
        perch(spots['path'], condition(during=MAIN2)),
        # Main 3: on the sign once the gate is won; on the gate's post for the fight (no flight: the same spot) and the road.
        perch(spots['sign'], condition(during=MAIN3, from_step=2), 'DA_Lines_HobMain3Tilly'),
        perch(spots['gate'], condition(during=MAIN3, from_step=1), 'DA_Lines_HobMain3Gate'),
        perch(spots['gate'], condition(during=MAIN3), 'DA_Lines_HobMain3Road'),
```
new:
```python
        # Mains 2 and 3 (story_texts.MAIN23_PERCHES, the Wardens story's: each on the first of its spots the build has).
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN23_PERCHES],
```
(d) old:
```python
        perch(spots['sign'], condition(after=[MAIN3])),
        perch(spots['rail'], condition(after=[MAIN2])),
```
new:
```python
        # After Mains 3 and 2 (story_texts.MAIN23_AFTER_PERCHES): on the undertaker's sign; on the ladder by the Ferryman.
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN23_AFTER_PERCHES],
```

**E3 (cleanup; harmless if skipped: build_area_main2/3 remove what these place) `place()`:** old
`    place_sexton(build, lookout, seat)` → delete (and `def place_sexton` with it). `place_bluff_top`: delete from
`    spider = actor_class('SpiderCreature')` through its `    return nx, ny, nz`, put `    return None` (the Point's marker
stays: Landfall climbs to it; `ring` and the NEST_* constants then go unused). `place_town_gate`: delete from
`    unpaid = actor_class('UnpaidCreature')` through the spawner's `build.log(...)`, keep `    return gx, gy, gz`. Docstring:
the Main 2 bullets (Sexton on the rail, the nest) and Main 3's gate-fight bullet now point at build_area_main2.py /
build_area_main3.py.

**E4 `Tools/Unreal/build_area_missionprops_valley.py` `place_lookout_stains`** (the Main 2 optional about Ellis's death is
gone): old
```python
    readable(build, meshes, 'LookoutStains', deck if deck is not None else spot, turn.yaw,
             shown_when=story.condition(during=MAIN2))
```
new
```python
    # (The LookoutStains readable went with Shall We Talk Business?, 2026-11-01: only the old stain's decal stays.)
```

**E5 `Art/Levels/RansomsRest/layout.json`.** (a) `gameplay.encounters.storyFights`: old
`        { "id": "BluffNest", "zone": "ransomsPoint", "during": "Main2", "by": "build_area_story.py" },` new
```json
        { "id": "BluffNest", "polygon": [[-1590,-9620],[1300,-9620],[1300,-6000],[-1590,-6000]], "during": "Main2", "by": "build_area_main2.py (the Three Widows on the keeper's path, since 2026-11-01)" },
        { "id": "BoardsCrew", "zone": "gravewindPoint", "during": "Main2", "by": "build_area_main2.py (the Company's crew on the burial boards, level 2)" },
```
and in the TownGate entry `"by": "build_area_story.py"` → `"by": "build_area_main3.py"`.
(b) `gameplay.levels.zones`: insert before the `west` entry (it must come before `west`, 4-5, which the nest's ground
touches):
```json
        {
          "id": "widows",
          "polygon": [[-1590,-9620],[1300,-9620],[1300,-6000],[-1590,-6000]],
          "levels": [1,2],
          "note": "Main 2's spider nest at the Three Widows on the keeper's path (the Wardens story, 2026-11-01)"
        },
```
then `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"`.
(c) `gameplay.encounters.fights` MainStreet `groups`: after the Rare Unpaid line add
`            { "class": "DredgerRifleman", "count": 2, "rank": "Basic", "firstWave": 2, "lastWave": 2 },` (and its
`"where"`/`"note"`: "...the second push from the alleys gains two Company riflemen (Under New Management, 2026-11-01)").
`gameplay` is build-only: no terrain staleness.

**E6 `Source/AI_Looter_Shooter/Tests/PacingPaths.h`.** `MainTwoStop` new body:
```cpp
		// The Ferryman's Price (2026-11-01, build_area_main2.py): the Three Widows' nest, four spiders and a Restless one (the
		// user's "Soften early only" numbers, moved off the Point); the boards' crew at level 2 whatever the deck's zone says
		// (three riflemen, a hose-man, then the cart's spill: four Unpaid), on Gravewind Point's deck (layout (1200, -15000)).
		FPathStop Stop = { TEXT("Main 2, The Ferryman's Price"), TEXT("Main2"),
			{ MakeKills(Foes.Spider, EPathRank::Basic, 4, TEXT("BluffNest")), MakeKills(Foes.Spider, EPathRank::Restless, 1, TEXT("BluffNest")),
			  KeptKills(Foes.Unpaid, EPathRank::Basic, 4, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2) }, 550 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2));
		return Stop;
```
`MainThreeStop` new body:
```cpp
		// Under New Management (2026-11-01, build_area_main3.py): the Company's checkpoint (TownGate: three riflemen, a
		// hose-man, two Unpaid and a Restless one); then Main Street's set piece (layout gameplay.encounters.fights MainStreet):
		// six Unpaid, a Restless one, the balcony's Bell-Ringer and, with its second push, two riflemen, at the valley's 2-3.
		FPathStop Stop = { TEXT("Main 3, Under New Management"), TEXT("Main3"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 2, TEXT("TownGate")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("TownGate")),
			  PlannedKills(Foes.Unpaid, EPathRank::Basic, 6, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("MainStreet"), 0.0, 3400.0) }, 1000 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("TownGate")));
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("TownGate")));
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerRifleman, EPathRank::Basic, 2, TEXT("MainStreet"), 0.0, 3400.0));
		return Stop;
```
**E7 `Tests/CreatureAreaLevelTests.cpp` line 85** (words only): `TEXT("Main 2's spider nest on the bluff")` →
`TEXT("Main 2's spider nest at the Three Widows")`; line ~229's message "the bluff's spiders" stays true (1-2 by E5b).
**E8 `Docs/Progression.md`**: M2/M3 titles; Main 2's kills (+8 at level 2 on the boards), Main 3's (gate 4 Dredgers + 3
Unpaid, was 7 Unpaid; Main Street +2 riflemen). Re-copy the Pacing test's sums once it runs.
**E9 CODEMAP** (above), **`git mv` TalkBusinessTests.cpp FerrymansPriceTests.cpp**, CODEMAP's old TalkBusiness line.

## Run order (Main)

1. E1-E7. 2. Build (`Tools\launch.ps1 -Build`). 3. `create_story_lines.py`, `create_mission_assets.py` (then
`create_side_mission_assets.py` as usual). 4. `create_area_assets.py` (E5b's zone). 5. `build_area.py RansomsRest gameplay`
(story → ... → landfall → main2 → main3 → Hob → roosts); read the log for `the Ferryman's landing: something ... stands in
its room` (move `LANDING_OUT`), `no Place_KeepersBridge`, `no Three Widows`, `no Landfall_Barge`, `Encounter BoardsCrew`,
the counts of shrouds (6) and chalk (about 10). 6. Tests below. 7. PIE walkthrough. 8. Perf (`Tools\perf.ps1`) on the deck
(cart, crew, ferry's point light) and Main Street (banner, chalk text renders, cart). No new sounds (existing cues only),
no new materials, no assets to delete.

## Tests (run once at commit, `Tools\runtests.ps1 -Filter ...`)

New: `Looter.Story.FerrymansPrice`, `Looter.Story.UnderNewManagement`, `Looter.World.CompanyLoudspeaker`,
`Looter.World.HoseCart`, `Looter.World.CompanyBanner`, `Looter.World.CompanySign`, `Looter.World.GangSkiffFlyBy`,
`Looter.World.SextonsFerry`, `Looter.Scenes.FerryLadder`. Touched: `Looter.Story.ColdWelcome`,
`Looter.Progression.Pacing` (after E6), `Looter.Creatures.AreaLevels` (after E5), `Looter.Missions.Definitions`,
`Looter.Bestiary.Ledger`, `Looter.Lines`, `Looter.Story.Landfall` (Delia's topics list grew by one, last).

## PIE walkthrough (step by step)

1. `Looter.Mission.Start Main2` (or finish Landfall): Hob's three offer lines on the move; tracker "Follow the keeper's
   path to Gravewind Point"; Hob on the middle Widow.
2. Walk the keeper's path from the farm's salt-line gate: at the Widows the nest springs (2, then 2 + the blue one, 2 out
   at most); last down: Hob's blue line. Cross Keeper's Bridge: Hob's Tuesday line (optional ticks, +10 silver).
3. Up the cairns: the deck's crew are visible at work (3 riflemen by the biers, the hose-man at the cart, the hose
   pulsing into a shrouded body, the pump's drain loop). Within 25 m of the gate: step 1; Hob on the gate's north rock
   (cart and horn lines). Check the loudspeaker post stands beside the south rock, horns toward the cairns.
4. Shoot the horn out while nobody's talking (optional ticks); then a second run: let Vane talk and shoot mid-line: his
   caption should cut to "...your bi—". Shoot the cart's tank (about a magazine): cyan blast, the hose-man staggered/hurt,
   the tank gone, black spill spreading, Hob's spill line, the four dead rising round the cart, then Vane's complaint.
   Another run (`Looter.Mission.Start Main2 2`): hold Interact on the valve (wheel spins, crank loop), same spill.
5. Clear all (count hidden on the tracker); step 2; Hob on the ladder's crossbar. Tap Interact on the stanchions: the
   climb (view turns to the rungs, steps down, steps off facing the ferry); Sexton in the stern, swaying a little with the
   ferry and the creak; walk the landing: can't step or jump off any side; no fall-recovery flash.
6. Talk: the deal's five lines; step 3; open the inventory's Log tab (story pages open). Talk again: the card, then his
   price. Climb up (tap the ladder's foot). Reload: Sexton still there after Main 2.
7. Delia: the meet-first talk starts Main 3; Hob's road line; then (quiet) Ned's skiff off the barge's side, south over
   the town and the Hogback (watch it from the farm road), Vane and Ned on the radio, then Hob/Sully. Reload in step 0: it
   doesn't fly again.
8. The town gate: the checkpoint's board, barrier, sandbags; the men standing guard as you come up the orchard; step 1
   at the gate; Hob's checkpoint line, then Vane's gate lines. Fight: the dead rise as the men fall (4 out at most).
9. Main Street: chalk on the doors (readable, facing the street, not mirrored), the Gilt Spur's cart pumping into the
   door (burst it), the banner over the street by the saloon: shoot one rope (sags), the other (falls flat, words up,
   dust; optional ticks). The set piece's second push brings two riflemen. Street won: Vane's "Residents!" line.
10. Tilly: card, then her three lines; the bail in Carried; the gun drops. After: her salted-coffins line. Reload after
    Main 3: the barrier gone, sandbags and board still there, the banner down if it was cut.

## Tripo models listed (TRIPO_MODELS.md "Mains 2-3")

M23-1 Sexton's ferry, M23-2 shrouded dead (two sizes), M23-3 Company hose-cart, M23-4 checkpoint sandbags + blank board,
M23-5 Company banner (blank); the loudspeaker post is no. 18 already. Each has its one swap path (listed there).

## Risks I couldn't check

- Compile: written against the headers, not built. Watch: `UTextRenderComponent` enum names (`EHTA_Center`,
  `EVRTA_TextCenter`), `ULocalLightComponent::SetIntensityUnits`, implicit float/double spots, `FColdOpenCourse` used
  outside the cold open. Shadowing checked by hand.
- TextRender facing: assumed it faces +X; if the chalk/board/banner words read from behind or mirrored, turn `Words` 180.
  The default text material may look self-lit at dusk.
- The landing under the deck: assumed open air 1-3 m out past the deck's front (it's built 3 m out over the canyon);
  `check_landing` warns if anything stands in its room. The ferry and the fog bank (SM_CanyonFog_A at 0.3) sizes are
  guesses: look in PIE. The ladder's foot is about 4 m from Sexton: the player takes a step to talk (reach 4 m).
- `BoardsCrew` wave 2 spots: if the spill's spots are refused (too near the player at the valve), the spawner falls back to
  spots round its middle in the aisle.
- Dredgers spawned at 45 m on the deck while the player is below the gate could be seen popping in from the last cairn.
- The town gate's spawner now stands from Main 3's step 0 at 70 m: the men appear while the player is in the orchard
  (trees should hide it); if seen, lower `GATE_ACTIVATION`.
- `MissionKillNamedObjective` for the horn counts a kill of its health component; a Dredger's stray bullet could silence
  it (no credit, no harm).
- Hob's Main 2/3 perches need E2; until then he uses the old spots (rail/rock/path) with the reworded lines.
- Pacing: Main 2 gains 8 level-2 kills, Main 3 swaps 4 Unpaid for 4 Dredgers and gains 2 riflemen; if the bands fail, trim
  mission XP (not kills), MAINS_2_7 §15.
- Sexton's secrets (§12) aren't built: his turn-in's "Come back when you've silver..." promises them; his after-topic says
  his price instead. A follow-up lane (with Paid on Delivery).
- WORLD's instanced Main Street posts aren't shootable; only the town gate's and the Keeper's Gate's are. Converting WORLD's
  three to `ACompanyLoudspeaker` (build_area_landfall.py, locked to me) would make every horn shootable.

## Numbers to tune

Cart: valve hold 1.5 s, tank 150, burst 3.5 m / 35%, pulse 0.8/s. Crew: level 2, max 6 alive, activation 45 m. Nest:
unchanged (4+1, 2 out, 3 s breath, 30 m). Gate: activation 70 m, max 4. Loudspeaker: horns 60, earshot 60 m, quiet wait
6 s. Banner: 5.2 m up, sag 0.6 s, fall 0.75 s. Ferry: drop 3.8 m, sway 1.2°/0.6°/3 cm, lantern 10 cd r 9 m. Skiff: 22 s,
radio at 2.5 s, ≥45 m over ground at the Hogback. Climb: 0.4 + 1.8 + 0.4 s.
