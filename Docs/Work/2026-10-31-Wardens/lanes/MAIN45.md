# Lane MAIN45: Main 4 "A Coal Under Ash" and Main 5 "Choke Point" (2026-11-01)

Built from `MAINS_2_7.md` §1-3, §6, §7, §14, §15 as revised to the user's calls (`DECISIONS.md`). Nothing built, run in
Unreal, tested or committed. Python compiled with UE's python (ast + compile, an undefined-name pass); C++ written against
the headers it calls (read, not compiled: risks below). Ids kept: `Main4`, `Main5`, `ChapelYard`, `Speaker_Aldana`,
`Bell_Chapel`, `Reliquary_Chapel`, `GraveSight.Reliquary` (Main 4's Reliquary look stays step 4), `KeepersRoll`,
`EggSac.Burst`, `Lantern_Keeper` (Main 5's lantern stays step 2: `AKeepersLantern::TakeStep`), `SheriffHat`, `SinkFloor`,
`Place_SinkFloor`/`Place_SinkRim`. XP kept (1,300 / 3,000). No Tripo calls. C++ was drafted during the package freeze and
copied into `Source/` after Main's "C++ OK" (a plain file copy of my own files).

## What the player gets

**Main 4, A Coal Under Ash** (Tilly's two offer lines on the move after Main 3's turn-in). 0: up the north road ("If her
light still burns, Father Aldana will know"). 1: Saint Ada's Yard behind its fog gate: eight dead the crew spilled; then
the crew's two hose-men at the side window, five dead and the Gravebound "Leftover" (was the Sexton's Debt; the Bell-Ringer
left the yard: the bell is Sully's now). Meanwhile a Company hose-cart on the west lane runs a hose over the sill of the
west window nearest the apse, across the sanctuary onto the Reliquary: pulses of her orange light run out along it toward
the window, and her ember is dimmer while it drinks. Hob on the door hood says why and how. Hold Interact 2 s at the sill:
the hose whips back through the window like a snake, out over the sill and slaps onto the yard, coughing black smog
(snap, wood knock, sparks, a view shake); or shut the cart's valve / burst its tank: the hose bucks out on its own. Either
way the cart spills and Vane complains from the lychgate's horn ("You're making a habit of this."). 2: the knock (his
three design lines). 3: the bell. 4: Hob ("Something answered, sunshine."), then Vane on the lychgate's horn; a Company
tender swings off the barge, flies in on an arc over 9 s and hangs 15 m over the yard, bobbing, engine droning; four ropes
drop, and four riflemen (level 3) and two hose-men (level 4) slide down them (two men a rope, one after another), land in
dust with a bark and come for Sully. Inside, the whole Reliquary's ember (a coal under ash, guttering) flares once through
the Grave Sight grey when looked at. Optional: the keepers' roll (SILAS, HANNAH, JONAS, ABEL RANSOM; Hob's "Ninety
years..."). Once the six are down the tender lingers 8 s and flies home. Turn-in at the vestry door: card, then his four
lines. From then Brother Finch stands at the Reliquary keeping her lit; drag furrows in the yard and a soot mark on the
front door stay.

**Main 5, Choke Point** (Aldana, Junie on the tube and Hob's grumble, straight after Main 4's turn-in). 0: Hob on the rim
by his canvas satchel ("Junie's salt, sunshine..."): take Junie's charges, go down (Vane on the Sink fence's horn). 1: the
egg sacs, as built. 2: Abel's lantern (Hob's kept line, then "Now the hose..."). 3, the Intake: within 10 m the mouth pulls
Sully toward it (a share of their walk, rising to the mouth; smog sucked in round it; the screen's edge frosting), inside
3 m it drains 5 % health a second. As the step starts (player within 25 m) two riflemen and a hose-man slide down ropes
beside the hose from 20 m up; the first charge (a 1.5 s hold on a clamp spot: clank, magnet thunk, sparks) wakes three
spiders down the webbed ramp and gets Vane on the barge's horn ("that's my supper"); the second brings the second drop;
the third lights the fuses (sparks, red light, hiss; Hob: "Fuse is lit! Get clear...") and the draw doubles for 5 s. The
blast: salt-white soul burst and shock ring, the big shake, the barge groaning overhead, a black cloud from the mouth and
the two dying stacks coughing their last; Landfall's hose is swapped for the intake's own in the same curve, which bucks
for 0.8 s, swings across the floor (everything in its arc knocked down and hurt: creatures 35 %, Sully 15 %) and settles
limp along the floor; two of three plumes are gone. Vane: "...Coughing. My barge is coughing." / "Stoker! Find me another
mouth." The smog pool at the den's mouth drains over 6 s, then something huge growls in the den (Hob's line). 4: climb out
(the hat optional). At the rim the barge lets a second hose down over 4 s onto a new air-shaft head on Larkspur High (Hob:
"She's found the mine."). Turn-in to Brother Finch at the Reliquary (card, then Finch, Junie, Hob). SmogThin from then.

## Files

New C++ (`Source/AI_Looter_Shooter/`):
| File | What |
|---|---|
| `World/ChapelWindowHose.h/.cpp`, `World/ChapelWindowHoseLine.cpp` | `AChapelWindowHose` (`HoseTag` Hose_ChapelWindow, `PulledEvent` ChapelHose.Pulled): the window hose: sections on one ISM along `InsideCurve` (sill → floor → `InsideEnd`, aimed at the tagged Reliquary's ember), her light's pulses, `Pull` (hold `HoldSeconds` 2, `PullableWhen`), whip (`WhipSeconds` 0.9) to `PulledRest`, smog cough; stops its cart (`CartTag`); a cart stopped first frees it and it sends `FMissionEvent::Interaction(this, true)`; `ShownWhen`, `PulledWhen`; sets the Reliquary's `SetDrinking`. |
| `World/ChapelReliquary.h/.cpp` (rewritten) | whole stand-in (lid shut, brass band, dark recess); her ember always guttering (`GutterAt`), lower while drunk (`SetDrinking`), bright while `LitWhen` (Main 7's lane sets it); the flash flares it (`EmberRise` 24) instead of lifting it off; ticks only with the player within 40 m. API kept: `Look`, `CanLook`, `SightEvent`, `GetEmberStart/Location/Glow`, `Advance`. Component subobject name "EmberGlow" kept. |
| `World/CompanyTenderDrop.h/.cpp` | `ACompanyTenderDrop`, `ETenderDropState`: flies `ComeFrom` → `HoverAt` (`RunAt` arc, `FlySeconds` 9) when `ComeWhen` begins to hold, hovers (bob, roll), `RopeAnchors`/`GetRopeAnchor`, leaves `LeaveAfter` 8 s after `DropSpawnerId`'s men are down or when its story ends. Hull: Tripo SM_CompanyTender (in), else the gang skiff. |
| `Creatures/RappelSpawner.h/.cpp` | `ARappelSpawner : AEncounterSpawner`: `AnchorTag` (a tender; first wave waits for its hover) or `RopeTops`; each spawned creature lifted to a rope top, passive, flying, slid down (`SlideAlpha`, `SlideSeconds`), a drop's men taking the ropes in turn; lands (dust, bark, `AlertTo`); ropes on one ISM (MI_RopeHemp); `IsSliding`, `AdvanceRopes`. |
| `World/SinkIntake.h/.cpp`, `World/SinkIntakeDraw.cpp`, `World/SinkIntakeHose.cpp` | `ASinkIntake`, `EIntakeState` (`IntakeTag` Intake_Sink, `ChokedEvent` Intake.Choked): the draw (`DrawWhen`, `DrawRadius` 10 m, `DrainRadius` 3 m, `DrainShare` 0.05), `NoteChargeSet` (1: spiders + Vane, 2: second drop, 3: fuse), `FuseSeconds` 5 / `FuseDrawScale` 2, `Choke` (blast, Landfall hose hidden by `RestHoseTag`, `DyingStacks` cough, Vane), buck/swing/limp hose (`SwingSeconds` 4, knocks within `KnockRadius` 2.4 m), the pool drain and growl (`PoolAt`, `DenAt`, `GrowlLines`), the second hose (`SecondHoseFrom/To/When/GoneWhen`, `SecondDropSeconds` 4), `ChokedWhen`, `GoneWhen`. |
| `World/SinkIntakeRules.h/.cpp` | `SinkIntakeRules` (`DrawInput`, `RestShape`, `LimpShape`, `BuckShape`, `SwingAngle`, `SwingShape`, `SecondShape`), `FIntakeHoseShape`. |
| `World/IntakeCharge.h/.cpp` | `AIntakeCharge` (`ChargeTag` Intake_Charge, `ChargeModelPath` /Game/Art/Tripo/SM_SaltCharge): a clamp spot; `SetCharge` (hold 1.5 s, `SetWhen`), tells its intake (`IntakeTag`); `LightFuse` (sparks, light, Foreman fuse hiss), `Blow`. |
| `Dev/Main45DevCommands.cpp` | `Looter.ChapelHose.Pull`, `Looter.Tender.Come [hover]`, `Looter.Intake.Charge [n]`, `Looter.Intake.Choke`, `Looter.Intake.SecondHose`. |
| `Tests/Main45WorldTests.cpp` (new) | `Looter.World.ChapelHose.Pull`, `Looter.World.TenderDrop.Flight`, `Looter.Creatures.Rappel.Slide`, `Looter.World.SinkIntake.Draw`, `Looter.World.SinkIntake.Choke`. |
| `Tests/HallowedGroundTests.cpp` (rewritten: Main 4's own) | now `Looter.Story.CoalUnderAsh.Mission/.Roaming/.Placed` (the old HallowedGround.Mission/.Roaming/.Placed are gone). `git mv` to `CoalUnderAshTests.cpp`. |
| `Tests/KeepersLanternTests.cpp` (edited: Main 5's own) | now `Looter.Story.ChokePoint.Mission/.Placed`. `git mv` to `ChokePointTests.cpp`. |
| `Tests/HallowedGroundTestWorld.h`, `Tests/KeepersLanternTestWorld.h`, `Tests/ChapelTests.cpp` (edited) | the 5-step Mains, `BellStep`/`ReliquaryStep`/`IntakeStep`/`OutStep`, Aldana's new words and topics, the flare. ChapelTests' names unchanged (`Looter.Story.HallowedGround.Bell/.GraveSight/.Aldana`). |

Python (`Tools/Unreal/`):
- `create_mission_assets.py`: docstring entries; constants (ALDANA_TAG, CHAPEL_HOSE, YARD, BELL_ANSWERED, RELIQUARY_TAG,
  RELIQUARY_SIGHT, FINCH_CHAPEL, SALT_CHARGES, INTAKE_CHARGE, INTAKE_TAG, JUNIE, FINCH); `main4_steps`, `main5_steps`; the
  Main4 and Main5 specs (titles, offers, briefings, turn-ins; Main 5's item SaltCharges, turn-in to Finch with `why`).
- `create_story_lines.py`: Main 4 sets HobMain4Yard/Reliquary (reworded), AldanaBarred/Waiting/Bell (reworded),
  AldanaAnswer (new), AldanaMain4 (the design's turn-in words), AldanaAfterMain4, Main4VaneBell, Main4VaneHose,
  FinchReliquary, FinchAfterMain5 (new); Main 5 HobMain5Satchel, HobMain5Fuse, HobMain5Growl, HobMain5Mine,
  Main5VaneSink, Main5VaneCharge, Main5VaneChoked (new), HobMain5Out and AldanaAfterMain5 (reworded); HobMain5Way kept,
  unused. No asset deletes.
- `story_texts.py` (my blocks only, all above the SECRETS block): READABLES KeepersRoll and SheriffHat reworded, ReliquaryLid
  marked retired, AldanaPage's Ellis line now Sully's; ITEMS SaltCharges; TOPICS Speaker_Aldana (+ AldanaAnswer) and
  Speaker_FinchChapel; a "Mains 4-5 (lane MAIN45)" block before `def condition`: `MAIN45_PERCHES`, `LOUDSPEAKER_CUES.update`
  (Loudspeaker_Lychgate, Loudspeaker_SinkFence, Loudspeaker_Barge), `ENCOUNTERS.update` (BellAnswered, IntakeDrop,
  IntakeSpiders); docstring.
- `build_area_chapel.py`: YARD_PUSHES two pushes with hose-men, YARD_HOSE_SPOTS, beats reworded, no Bell-Ringer, the
  Reliquary whole (RELIQUARY_MESHES = the Tripo whole one only), docstring.
- `build_area_sink.py`: INTAKE_STEP/CLIMB_STEP constants, docstring (behaviour unchanged).
- `build_area_main4.py` (new): the cart and window hose (sill found by traces), drag furrows + door soot decals, the
  lychgate loudspeaker, the tender's hover (searched: every rope foot inside the fence, off the chapel, clear of the
  dressing) and BellAnswered, Finch at the Reliquary. Returns `{'lychgate'}`.
- `build_area_main5.py` (new): the satchel (AMissionItem), the intake (Landfall's hose curve re-read with
  `build_area_landfall.plan_barge`/`barge_parts`, mirrored from `hose_parts`) and its three clamp spots, IntakeDrop
  (ropes beside the hose 20 m up), IntakeSpiders (on the ramp's lower half), the fence and barge loudspeakers, the air-shaft
  head + Place_AirShaft, Landfall's hose and plumes 2+ `hidden_when` from the choke, the SmogThin rule before Smog. Returns
  `{'satchel'}`.
Docs: `Docs/Areas/RansomsRest_Expansion.md` (rows M4/M5, both sections rewritten), `Docs/Pipeline/Story.md` (two bullets),
`TRIPO_MODELS.md` ("Mains 4-5": M45-1 Reliquary whole, M45-2 salt satchel + charge, M45-3 air-shaft head).

## CODEMAP lines (Main writes)

- `World/ChapelReliquary` (replace its line): `AChapelReliquary`, Saint Ada's whole Reliquary (Main 4, tag Reliquary_Chapel): her ember guttering (lower while a hose drinks her, bright while `LitWhen`), a tap plays Grave Sight's flash and her ember's flare, tells missions `GraveSight.Reliquary`.
- `World/ChapelWindowHose` (+`ChapelWindowHoseLine.cpp`): `AChapelWindowHose`: the Company's hose over the chapel's side window drinking her ember (Main 4); pulled with a hold or freed by its cart's stop, it whips out.
- `World/CompanyTenderDrop`: `ACompanyTenderDrop`, `ETenderDropState`: a tender flown off the barge to hang over a drop with its ropes (Main 4's answer), then home.
- `Creatures/RappelSpawner`: `ARappelSpawner`: an encounter whose creatures slide down ropes (a tender's, or beside the barge's hose).
- `World/SinkIntake` (+`SinkIntakeDraw.cpp`, `SinkIntakeHose.cpp`): `ASinkIntake`, `EIntakeState`: the barge's intake in the Sink (Main 5): its draw, three charges, fuse, blast, swinging and limp hose, the pool and growl, the second hose to Larkspur High. `World/SinkIntakeRules`: `SinkIntakeRules`, `FIntakeHoseShape`: its draw and hose shapes as math. `World/IntakeCharge`: `AIntakeCharge`: a clamp spot on its collar.
- `Dev/Main45DevCommands.cpp`: `Looter.ChapelHose.Pull`, `Looter.Tender.Come`, `Looter.Intake.Charge/.Choke/.SecondHose`.
- Tests line 701: `HallowedGroundTests.cpp (Main 4)` → `CoalUnderAshTests.cpp (Main 4, Looter.Story.CoalUnderAsh.*)`; `KeepersLanternTests.cpp (with KeepersLanternTestWorld.h; Main 5)` → `ChokePointTests.cpp (with KeepersLanternTestWorld.h; Main 5, Looter.Story.ChokePoint.*)`; add `Main45WorldTests.cpp` (`Looter.World.ChapelHose.Pull`, `.TenderDrop.Flight`, `.SinkIntake.*`, `Looter.Creatures.Rappel.Slide`).
- Tools: `build_area_chapel.py` line: "yard fight (two waves of six...)" → "Saint Ada's Yard set piece (eight Unpaid; two hose-men, five and the Leftover)". Add `build_area_main4.py` (Main 4's window hose and cart, the tender's drop, the lychgate horn, Finch at the Reliquary, the marks) and `build_area_main5.py` (Main 5's satchel, intake and charges, drops, ramp spiders, horns, air shaft, SmogThin, Landfall's hose/plumes hidden after the choke).

## Exact edits for Main (in others' files)

**E1 (required) `Tools/Unreal/build_area_story.py` `place()`.** Old:
```python
    importlib.reload(importlib.import_module('build_area_main3')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots)
```
New:
```python
    importlib.reload(importlib.import_module('build_area_main3')).place(build)
    # Main 4, A Coal Under Ash (its own module; after the chapel and Landfall): the window's hose and its cart, the
    # lychgate's horn, the tender's drop, Finch at the Reliquary, the marks.
    importlib.reload(importlib.import_module('build_area_main4')).place(build)
    # Main 5, Choke Point (its own module; after the Sink and Landfall): the satchel, the intake and its charges, the
    # drops, the ramp's spiders, the horns, the air shaft, SmogThin; Hob's satchel spot.
    main5_spots = importlib.reload(importlib.import_module('build_area_main5')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots, main5_spots)
```

**E2 (required) `place_hob`.** (a) old `              landfall_spots=None, main2_spots=None):` new
`              landfall_spots=None, main2_spots=None, main5_spots=None):`. (b) after `        'ladderTop': (main2_spots or {}).get('ladderTop'),` add:
```python
        # Main 5's (story_texts.MAIN45_PERCHES names it): beside Hob's satchel on the Sink's rim (build_area_main5.place).
        'satchel': (main5_spots or {}).get('satchel'),
```
(c) old (the whole block from `        # Main 4: on the door hood for the Reliquary and the bell; on the vestry lantern's bracket for the knock at` through
`        perch(spots['denRock'], condition(during=MAIN5), 'DA_Lines_HobMain5Way'),`) new:
```python
        # Mains 4 and 5 (story_texts.MAIN45_PERCHES, the Wardens story's: each on the first of its spots the build has).
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN45_PERCHES],
```
(The after-perches `roadTree` after Main 5 and `lantern` after Main 4 stay.)

**E3 (required, together) the lantern bail moves to Main 5** (MAINS_2_7 §7/§15: it goes on the lantern there).
(a) `create_mission_assets.py` Main 6's `items=[dict(item_id='LanternBail', ...)],` block (the comment over it and the
four lines) → cut, and paste the same `dict(item_id='LanternBail', ...)` into Main 5's `items=[...]` list after the
SaltCharges dict. (b) Main 6's step 0: old
```python
                       short="Take the keeper's path", place=place(tag='Place_KeepersBridge', radius=600.0), optional=True),
             collect_items(asset, "Bring Pa's lantern bail. Tilly handed it over.", ['LanternBail'],
                           short="Bring Pa's lantern bail")),
```
new `                       short="Take the keeper's path", place=place(tag='Place_KeepersBridge', radius=600.0), optional=True)),`
(the Main 6/7 lane rewrites Main 6 anyway). (c) `story_texts.py` ITEMS `'LanternBail': dict(mission='Main6', ...` →
`mission='Main5'` and its comment "Main 6's item" → "Main 5's item (it goes on Abel's lantern there)".

**E4 (required) `Art/Levels/RansomsRest/layout.json`.** (a) `gameplay.encounters.storyFights`: old
`        { "id": "ChapelYard", "obstacle": "churchyardFence", "during": "Main4", "by": "build_area_chapel.py" },` new
```json
        { "id": "ChapelYard", "obstacle": "churchyardFence", "during": "Main4", "by": "build_area_chapel.py (Saint Ada's Yard: two pushes, the crew's two hose-men; A Coal Under Ash, 2026-11-01)" },
        { "id": "BellAnswered", "obstacle": "churchyardFence", "during": "Main4", "by": "build_area_main4.py (the tender's drop on ropes: four riflemen level 3, two hose-men level 4)" },
        { "id": "IntakeDrop", "zone": "sink", "during": "Main5", "by": "build_area_main5.py (two drops down the barge's hose, the Sink's 4-5)" },
        { "id": "IntakeSpiders", "zone": "sink", "during": "Main5", "by": "build_area_main5.py (three spiders down the webbed ramp)" },
```
(b) `level.environment.states`: old
```json
          "clouds": { "tint": [0.6, 0.62, 0.52] },
          "post": { "exposureBias": 0.2 }
        }
```
new
```json
          "clouds": { "tint": [0.6, 0.62, 0.52] },
          "post": { "exposureBias": 0.2 }
        },
        "SmogThin": {
          "about": "The smog thinned (Choke Point's turn-in on, lane MAIN45: one hose choked, the barge down to one stack; after Main 6 it stays thin with no source; build_area_main5.py's AStoryLighting rule, before the Smog rule): halfway back from Smog to the clean golden hour.",
          "sun": { "azimuth": 247.5, "elevation": 15, "intensity": 5.5, "temperature": 4050 },
          "sky": { "intensity": 0.88 },
          "atmosphere": { "skyLuminance": [0.95, 1.22, 1.65] },
          "fog": { "density": 0.007, "inscattering": [0.23, 0.25, 0.24],
                   "directional": { "color": [1.1, 0.72, 0.25], "exponent": 7.5 } },
          "backdrop": { "tint": [0.78, 0.8, 0.75] },
          "clouds": { "tint": [0.8, 0.81, 0.76] },
          "post": { "exposureBias": 0.3 }
        }
```
then `build_area.py RansomsRest environment` (the state) before the gameplay pass. `gameplay` is build-only.

**E5 `Tools/Unreal/build_area_missionprops_valley.py`** (its own READABLES place the roll): (a) the KeepersRoll entry's
text: old `"KEEPERS OF THE LANTERN\nSAINT ADA'S, RANSOM'S REST\n\nSILAS RANSOM\nHANNAH RANSOM\nJONAS RANSOM\n"` +
`"ABEL RANSOM\n\nUnder Abel's name a line has been painted out in white. It still shows through: ELLIS.",` and
`say((HOB, "Heir to the family business, sunshine. Funny how that goes.")), "Read", ('KeepersRoll',)),` new
```python
                    "KEEPERS OF THE LANTERN\nSAINT ADA'S, RANSOM'S REST\n\nSILAS RANSOM\nHANNAH RANSOM\nJONAS RANSOM\n"
                    "ABEL RANSOM\n\nThe last name is fresh paint. There is room under it, and nobody to put there.",
                    say((HOB, "Ninety years of Ransoms kept her lit. Not one of them got a hose.")), "Read",
                    ('KeepersRoll_Bare', 'KeepersRoll')),
```
(b) old `OTHER_MESHES = {'KeepersRoll': 'KeepersRoll_Bare'}` new `OTHER_MESHES = {}` (Sully is a Warden: no ELLIS under ABEL
for anyone; the comment over it says so). (c) SheriffHat's heading stays; its words already match.

**E6 `Tools/Unreal/build_area_seeds.py` `place()`:** old `    place_reliquary_lid(build)` new
`    # (The Reliquary's lid went with A Coal Under Ash, 2026-11-01: the Reliquary is whole, never prised.)` and the
docstring bullet "The Reliquary's lid ..." → "(retired 2026-11-01)".

**E7 `Source/AI_Looter_Shooter/Tests/PacingPaths.h`.** `MainFourStop` new body:
```cpp
		// A Coal Under Ash (2026-11-01, build_area_chapel.py YARD_PUSHES, build_area_main4.py): Saint Ada's Yard in two pushes
		// (eight Unpaid; then the crew's two hose-men, five Unpaid and the Gravebound Leftover), then the barge's answer down the
		// tender's ropes (BellAnswered: four riflemen at level 3, two hose-men at 4) round the churchyard (layout (6700, -1900)).
		FPathStop Stop = { TEXT("Main 4, A Coal Under Ash"), TEXT("Main4"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 13, TEXT("ChapelYard")), MakeKills(Foes.Unpaid, EPathRank::Gravebound, 1, TEXT("ChapelYard")) }, 1300 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("ChapelYard")));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("BellAnswered"), 6700.0, -1900.0, 3, 3));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("BellAnswered"), 6700.0, -1900.0, 4, 4));
		return Stop;
```
`MainFiveStop` new body:
```cpp
		// Choke Point (2026-11-01, build_area_sink.py, build_area_main5.py): the floor's six spiders and the egg sacs' six; the
		// Intake's set piece: two drops down the hose (four riflemen, two hose-men) and three spiders down the ramp, the Sink's 4-5.
		FPathStop Stop = { TEXT("Main 5, Choke Point"), TEXT("Main5"),
			{ MakeKills(Foes.Spider, EPathRank::AreaRolled, 6, TEXT("sink")), MakeKills(Foes.Spider, EPathRank::Basic, 6, TEXT("sink")),
			  MakeKills(Foes.Spider, EPathRank::AreaRolled, 3, TEXT("sink")) }, 3000 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("sink")));
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("sink")));
		return Stop;
```
(Main 4 loses the Bell-Ringer and one Basic, gains 8 Dredgers; Main 5 gains 9 kills. If the Pacing bands fail, trim mission
XP, not kills: Main 4 1,300 → 1,100, Main 5 3,000 → 2,600 first; MAINS_2_7 §15.)

**E8 `Source/AI_Looter_Shooter/World/LamplighterShipParts.cpp` (FLIGHT's) `SpawnPieces`:** after `			Member->Crew = Station.Crew;` add
```cpp
			// Brother Finch goes ashore at Main 4's turn-in to keep Saint Ada lit by hand (A Coal Under Ash; build_area_main4.py
			// stands him at the Reliquary from then): the ship's Finch is aboard until it.
			if (Station.Crew == EWardenCrew::Keeper)
			{
				Member->ShownWhen.BeforeMissions = { FName(TEXT("Main4")) };
			}
```
**E9** CODEMAP (above); `git mv Tests/HallowedGroundTests.cpp Tests/CoalUnderAshTests.cpp` and
`git mv Tests/KeepersLanternTests.cpp Tests/ChokePointTests.cpp`; `Docs/Progression.md`: M4/M5 titles and kills (above),
re-copy the Pacing sums once it runs; `Docs/Handoff.md` entry.

## Run order (Main)

1. E1-E8. 2. Build (`Tools\launch.ps1 -Build`). 3. `create_story_lines.py`, `create_mission_assets.py` (then
`create_side_mission_assets.py`, the SECRETS lane's scripts). 4. `build_area.py RansomsRest environment` (SmogThin, E4b).
5. `build_area.py RansomsRest gameplay`; read the log for: `the chapel's west window: no opening found` (fix WINDOW /
SILL_PROBE), `the tender: no hover spot` (ROPE_ANCHORS / DROP_CLEAR), `the dressing stands within 1.5 m of the chapel yard's
spots` (the hose-men's spots), `Landfall's barge plan failed`, `no Landfall_BargeHose placed`, `the air shaft's ground falls`,
`IntakeDrop: two ropes`, `SmogThin ... before the Smog rule`, `Brother Finch at the Reliquary`. 6. Tests below. 7. PIE. 8. Perf
(`Tools\perf.ps1`) at the chapel yard during step 4 (tender, ropes, 6 Dredgers, the Reliquary's light) and on the Sink's
floor during the swing (two hose ISMs, smoke puffs), and once from the farm toward Larkspur High with the second hose up.
No new sounds (existing cues only), no new materials, no assets to delete.

## Tests (once, at commit: `Tools\runtests.ps1 -Filter ...`)

New: `Looter.Story.CoalUnderAsh`, `Looter.Story.ChokePoint`, `Looter.World.ChapelHose`, `Looter.World.TenderDrop`,
`Looter.Creatures.Rappel`, `Looter.World.SinkIntake`. Touched: `Looter.Story.HallowedGround` (Bell, GraveSight, Aldana),
`Looter.Story.EggSac` (its test world changed underneath it), `Looter.Progression.Pacing` (after E7),
`Looter.Missions.Definitions`, `Looter.Lines`, `Looter.Lines.NoFamily` (AldanaPage's line moved to SULLY),
`Looter.Story.WardenCrew` (after E8), `Looter.Bestiary.Entries` (no new creature kinds).

## PIE walkthrough

1. `Looter.Mission.Start Main4` (or turn in Main 3): Tilly's two lines on the move; tracker "Go up to the Chapel of Saint
   Ada"; Hob on the door hood.
2. At the chapel (step 1): the fog gate shuts; eight rise; Hob's yard line. Walk the west lane: the cart on it, its hose
   over the window sill; inside, the hose across the sanctuary onto the Reliquary, orange pulses running out along it, her
   ember dim. Hold Interact at the sill (2 s): the whip out, smog, snap; Vane on the lychgate horn. Second run
   (`Looter.Mission.Start Main4 1`): shoot the cart's tank: the hose bucks out on its own; the objective ticks.
3. Push 2: two hose-men by the cart first, then five and the Leftover (its name over it). Yard clear: fog lifts.
4. Knock (his three lines, Sully's "Warden. Tilly sent me."), the bell (held), then Hob's "Something answered", Vane's two
   lines, the tender in from the barge's side over ~9 s, hanging clear of the spire; four ropes; men sliding two to a rope,
   landing in dust, barking, hunting; look at the Reliquary: the flare. Six down: the tender lingers 8 s, flies home.
   Optional roll. Turn in: card, his four lines; Main 5's offer follows (Aldana, Junie, Hob). Finch now at the Reliquary
   (talk: "I'll sit with her."); the ship's Finch gone (E8).
5. Main 5: Hob by the satchel on the rim (his line); Vane on the fence horn; take the charges (Carried), go down; sacs;
   lantern (Hob's two lines).
6. Step 3: walk to the intake: the pull (walk away slower, stand still and drift in, frost at the edges, smog swirling in;
   inside 3 m the drain). Two riflemen and a hose-man slide down beside the hose. Set charge 1 (clank, sparks; spiders down
   the ramp; Vane's "supper"), charge 2 (the second drop), charge 3 (fuse sparks and hiss, Hob's warning, the pull doubled:
   still escapable). Blast at 5 s: shake, white burst, black cloud, the hose bucks and swings (stand in its arc once: knocked
   down, 15 %), lies limp; two plumes gone; Vane's choke lines; the pool shrinks, the growl and Hob's line.
   `Looter.Intake.Charge 3` and `Looter.Intake.Choke` replay it.
7. Climb out: at the rim the second hose lets down to Larkspur High (watch from the rim), Hob's "found the mine". Turn in
   to Finch at the Reliquary (tracker "Brother Finch keeps Saint Ada lit at the Reliquary: turn in to Brother Finch").
   After: the light thinner (SmogThin); reload: the hose limp, the pool gone, the second hose up.

## Tripo models listed (TRIPO_MODELS.md "Mains 4-5")

M45-1 Saint Ada's Reliquary whole (/Game/Art/Tripo/SM_ReliquaryWhole), M45-2 the salt satchel (SM_SaltSatchel) and one
charge (SM_SaltCharge), M45-3 the Company air-shaft head (SM_AirShaftHead). Reused, already in: the tender, the hose and
intake, the loudspeaker post, the hose-cart. Not Tripo: the soot handprint (a decal to draw; stand-in MI_Decal_ImpactScar).

## Risks I couldn't check

- Compile: written against the headers, not built. Watch: `FMath::Lerp` on FVector with a float alpha (SinkIntakeRules),
  `UInstancedStaticMeshComponent::AddInstances/BatchUpdateInstancesTransforms` signatures, `TSet<TWeakObjectPtr<AActor>>`,
  `case IntakeStep:` (a constexpr in a header namespace), implicit double/float spots, `LoadObject` in a static local
  (IntakeCharge::Blow), `UMissionInteractObjective::Count`.
- `ACreatureBase::HoldBrain` is protected: the slide holds a man with `SetPassive(true)` + MOVE_Flying and sets his place
  each tick after movement (TG_PostUpdateWork). A passive creature may try to "walk home"; if it fights the slide, add a
  public `HoldBrain` wrapper. The body plays idle on the rope (no rope pose yet: polish). The spawner's straggler recall
  could pull a man off a rope in a long slide (2.2-2.6 s; it shouldn't).
- `bHuntOnSpawn`'s `AlertTo` right after `OnCreatureSpawned`: assumed refused while passive; if not, the man shoots from the
  rope (the slide still moves him).
- The yard set piece's entrance is Rise: the two hose-men "rise" like the dead (an Unpaid rise sound, no dirt). Their spots
  are behind the chapel's west side, mostly out of sight; if seen, give FEncounterGroup an entrance of its own or place
  them (`setPiece.placed`, but placed ones don't count toward ChapelYard's 16).
- The window: WINDOW (-440, 320) and SILL_UP 175 are read from Chapel.py's numbers (LEFT_BAYS 1, sills 1.35 m); the sill
  trace looks for the opening. The cart at (-440, 950) is checked against yard spots, not the graves (only the spawn spots
  are): look in PIE; the inside hose's curve may clip a pew (1 m from the wall?).
- The tender's Tripo hull size and pivot are assumed ~12 m, pivot mid; rope anchors (±300, ±230, -60) may hang off the
  hull's sides oddly: tune ROPE_ANCHORS. Hover 15 m over the yard clears the 17.9 m spire only because its hull box keeps
  off SPIRE_BOX: check it doesn't cut the cross.
- The intake's hose mirrors `build_area_landfall.hose_parts`' curve (I can't import the curve itself): if WORLD changes
  HOSE_SAG or the drum socket, update `build_area_main5.Hose`. The swap at the choke: Landfall's baked hose hidden that
  frame; if its baked mesh's sections differ in count, a one-frame difference shows. AStoryProp plume pops are covered by
  the cough puffs; the barge can't lurch (a baked static prop: a groan sound and the shake stand in).
- The second hose (~225 m) is a fixed sag curve: it may pass through Larkspur Ridge or the town's roofs between: trace it
  in the editor (looking along it from the rim), raise SecondShape's sag or SHAFT_AT. Its shaft head stand-in is a block.
- The smog pool is a black splat on the floor (no 0.5 m dip: the terrain is WORLD's, a rebuild costs); it may float or sink
  on uneven ground at the den's mouth.
- Finch: `ApplyCrew` adds Speaker_Finch to the chapel Finch too (two actors carry it after E8 hides the ship's): anything
  that looks up Speaker_Finch after Main 4 may find him at the Reliquary (fine: he's Finch). His height/hands at the
  Reliquary (Keeper station's lantern pose) are a guess: HandsShift may need tuning.
- Hob's Main 4/5 perches need E2; without it he keeps the old spots with the reworded arrival lines.
- Pacing: +8 Dredgers in Main 4 and +9 kills in Main 5 at 3-5: the bands may fail (trim XP as E7 says).
- The yard's cart is stoppable only in step 1; shot from the approach it holds (as Main 2's).

## Numbers to tune

Hose: hold 2 s, whip 0.9 s, girth 16 cm, 14 sections, pulses 5 at 0.32/s, PULLED_REST (260, -120, -170). Reliquary: gutter
size 11 / strength 7 / 2.5 cd, flare 26 / 34 / 16 cd, lit 30 / 40 / 22 cd, drinking ×0.4, rise 24 cm, tick within 40 m.
Tender: 15 m up, fly 9 s, rise 18 m, bob 28 cm / 3.4 s, leave 8 s after. Ropes: slide 2.2 s (chapel) / 2.6 s (hose), second
man on a rope after 0.7 of a slide, 20 m up the hose, ±2.2 m beside it. Drop: riflemen level 3, hose-men 4, activation 60 m.
Intake: draw 10 m (0.22 → 0.55 of a walk, fused ×2 capped 0.85), drain 3 m at 5 %/s, fuse 5 s, buck 0.8 s, swing 4 s
(kick 72°, period 2.3 s, decay 1.7 s), knock 2.4 m (creatures 35 %, Sully 15 %, thrown 9 m/s + 4.2 up), pool 2.6 m drained
over 6 s from 2 s, growl 1 s after, second hose 4 s. Yard: 8; then 2 hose-men + 5 + the Leftover (Epic), max 9 alive.
