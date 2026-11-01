# Lane FLIGHT: the Lamplighter, the tender, the landing, the opening flow

Not built, not run (the lane rules). Every edit is in FLIGHT's own files. Main applies the hooks below.

## Files

New (S = `Source/AI_Looter_Shooter/`):
- `S/World/LamplighterShip.h` + `.cpp` (life, clock, sailing, state jumps, keeping the player aboard, sounds),
  `LamplighterShipParts.cpp` (hull, deck boxes, rail walls, Great Lantern + flame + light, wheel, engine, gangway, prow
  marker, crew), `LamplighterShipBoarding.cpp` (Vane on the horn, tender, grapples, lurch, waves, deck clear, cuts, break),
  `LamplighterShipLanding.cpp` (breaking, HardLanding start, touchdown, moored groan/slide, anchor, winch),
  `LamplighterShipStory.cpp` (BeginLandfall from the campaign, player on deck, save hold, lines on Main 1 steps).
- `S/World/LamplighterCourse.h/.cpp`: `FLamplighterCourse` (Catmull-Rom by distance, speed profile, bank/pitch/bob).
- `S/World/LamplighterRules.h/.cpp`: `LandfallRules` (names, Main 1 step indices, `ELandfallStart`, `StartFor`,
  `ApplyStart`, line sets, `Say`).
- `S/World/LamplighterEffects.h/.cpp`: `LamplighterEffects` (FWeaponFX dust, sparks, gas, smoke, fireball; shakes, kicks).
- `S/World/CompanyTender.h/.cpp`: `ACompanyTender`.
- `S/World/LandfallGrapple.h/.cpp`: `ALandfallGrapple`, `ELandfallGrappleState`.
- `S/World/LandfallWinch.h/.cpp`: `ALandfallWinch`.
- `S/World/LandfallSpyglass.h/.cpp`: `ALandfallSpyglass`.
- `S/World/LandfallMarker.h/.cpp`: `ALandfallMarker`.
- `S/Scenes/HardLanding.h/.cpp`, `S/Scenes/SmogReveal.h/.cpp`, `S/Story/VaneBroadcast.h/.cpp`.
- `S/Dev/LandfallDevCommands.cpp`, `S/Tests/LandfallFlightTests.cpp`.

Edited: `S/Scenes/ColdOpenSubsystem.h/.cpp` (BeginIfDue hands a level with the ship to `BeginLandfall`), `S/Areas/
StationBoard.cpp` (`FirstArrivalLanding()` = `Landing_Lamplighter`), `S/Session/SessionSubsystemNewGame.cpp` (comments only:
the flow works through FirstArrivalLanding), `S/Tests/ColdOpenTests.cpp` (new `Looter.Scenes.ColdOpen.Landfall`; old tests
kept: the old opening still plays in a level without the ship). Not needed: SessionSubsystemTravel.cpp,
AreaTravelSubsystem.cpp (PlaceAtLanding finds the ship by its actor tag and stands the player on its tagged `LandingSpot`).

Header names used from other lanes (all confirmed present): `Creatures/DredgerCreature.h` (`ADredgerCreature::ClassFor`,
`EDredgerRole` in DredgerTypes.h, `SetHoldsGround(true)` for the tender's two), `Story/WardenCrewMember.h` (`AWardenCrewMember`,
`EWardenCrew`, `Crew`, `Brace`), `Audio/LooterSoundCues.h` (every contract cue, plus the old `ImpactWood` for plank thuds).

## How it works (one paragraph each)

- Begin: `UAreaTravelSubsystem::HandleLevelBegun` -> `UColdOpenSubsystem::BeginIfDue` -> ship found -> `BeginLandfall`
  (takes the menu intro's handoff, marks NewGameIntro/ColdOpen/GraveWake played without telling missions) ->
  `ALamplighterShip::BeginLandfall` reads `LandfallRules::StartFor(campaign)`: Main1 done -> Moored+anchored; Main1 running
  without flag `Landfall.Begun` -> old save: moored, anchored, Main1 step 5, its old optional keys cleared; Main1 running
  with the flag at step <= 3 (Save & Quit mid-air) -> flies again, step back to 0; step >= 4 -> moored (anchored if flag
  `Landfall.Anchored` or step > 4); otherwise (new game, Looter.PlayLevel) -> flight. `ApplyStart` always sets
  `bColdOpenSeen`, `Intro.Seen`, `Landfall.Begun`. Scenes off (tour/perf) or no course -> moored, anchored, record untouched.
  BeginIfDue returns false, so the arrival reveals the white with REVENANT (ArrivalTitle of the first-arrival landing).
  The ship also begins itself 1 s into play if nothing called it.
- Flight: cruise 9 m/s along the course, bob/bank, the player placed at `LandingSpot` and based on the deck
  (`SetBase`); she brakes to a 1.6 m/s creep at GrappleStop and holds at most 30 m past it. Autosaves held
  (`HoldSaves("Landfall")`) until moored. Off the deck while flying for 0.6 s (a death's wake at a roost, a fall) -> back on
  deck out of a short black. Deck lines at 4.5 s.
- Prow reached (Main1 step > 0, or with no Main1 running, within 350 cm of the marker) -> `UVaneBroadcast::Say(VaneHorn)`
  -> tender spawned astern 2 s later, alongside 9 s after -> grapples fire when alongside, she's within 4 m of the stop and
  Vane is done (max 30 s) -> two grapples fly 0.4 s, bite (clang, sparks, shake) -> pinned: lurch 1.4 s, then not one
  transform change until the lines part. +1 s tender riflemen (hold ground), +2.2 s gangplank, +3 s wave 1 (2 riflemen,
  0.7 s apart), wave 2 (hose-man + rifleman) 1.5 s after wave 1 is down (or 25 s in with one left); all four down ->
  `NotifyEvent("Landfall.DeckClear")` + Grapples lines; only then are the hooks usable (hold 1.2 s). Both cut -> tender
  falls burning (fireball, smoke, riflemen frozen and carried, gone after 8 s), bag tear, lurch, Landing lines.
- Landing: 2 s breaking (player free, gas jetting), then scene `HardLanding` (look-only, carried, 5.8 s, skippable): drop,
  list to the torn side, flare, Touch at 6 s (crash cue, big shake + kick, dust skirt, keel sparks), 0.8 s skid of sparks,
  settle exactly at `Moored`. Moored: gangway opens on `GangwaySide` at `GangwayAt`, engine + creak run, she groans and
  shifts every 3.5-7 s (slides up to 80 cm along `CanyonSide` if set) until the winch is wound (hold 2.5 s, crank turns,
  AnchorCrank loop, AnchorSet, chain shown, engine and creak fade, flag `Landfall.Anchored`).
- Spyglass: tap -> `SmogReveal`: dip into the glass, barge (FOV 18->10), down the hose to the traced ground, up 35 m of
  smog; Vane's valley broadcast at 1 s; waits for him (max 45 s); dip out; Trail lines + LanternLean cue. First look only
  (flag `Landfall.Revealed`); later looks are the quiet 9.8 s version.

## Hooks for Main (exact edits in other lanes' / shared files)

1. `S/Tests/NewGameTests.cpp` (breaks on the new landing):
   old `	TestEqual(TEXT("...its first arrival: Ellis's grave in the family plot"), StationBoard::FirstArrivalLanding(), FName(TEXT("Landing_FamilyPlot")));`
   new `	TestEqual(TEXT("...its first arrival: the Lamplighter's deck"), StationBoard::FirstArrivalLanding(), FName(TEXT("Landing_Lamplighter")));`
   and in the comment above it `arriving at the family plot with the title` -> `arriving on the Lamplighter's deck with the title`.
2. `S/Tests/StationTests.cpp` line 95:
   old `		TestTrue(TEXT("It arrives at the family plot"), Leave.Landing == FName(TEXT("Landing_FamilyPlot")));`
   new `		TestTrue(TEXT("It arrives on the Lamplighter"), Leave.Landing == FName(TEXT("Landing_Lamplighter")));`
   lines 241-242:
   old `	TestTrue(TEXT("It arrives at the family plot, the story begun, Skyreach's world kept"), Arriving`
   new `	TestTrue(TEXT("It arrives on the Lamplighter, the story begun, Skyreach's world kept"), Arriving`
   old `		&& Arriving->GetArrivalOn(TEXT("/Game/Maps/Lvl_RansomsRest")) == FName(TEXT("Landing_FamilyPlot")) && Arriving->Campaign.bFirstCastOff`
   new `		&& Arriving->GetArrivalOn(TEXT("/Game/Maps/Lvl_RansomsRest")) == FName(TEXT("Landing_Lamplighter")) && Arriving->Campaign.bFirstCastOff`
3. `S/Areas/StationBoard.h` line 95 (comment):
   old `	/** Where the story's first arrival puts the player: the family plot, by Ellis's grave (the cold open plays from there). */`
   new `	/** Where the story's first arrival puts the player: the Lamplighter's deck (Landfall; the ship carries the tag). */`
4. `S/Areas/AreaTravelSubsystem.h` comment lines 18-22 and 28-30 still describe the cold open/family plot: replace with "the
   story's first arrival: the Lamplighter's deck on Ransom's Rest (Landing_Lamplighter); UColdOpenSubsystem::BeginIfDue
   hands the opening to ALamplighterShip" (wording free; comments only).
5. WORLD's `build_area_landfall.py` `SHIP` dict (its guess) -> the stand-in's real numbers: `length=3240.0` (the bag; hull
   2430), `beam=1130.0` (the bag; hull 700), `keel_below_pivot=0.0`, `top_over_pivot=980.0`, `deck_over_pivot=56.0`,
   `forward_yaw=0.0`. And WORLD sets the extra properties below (tender_side, gangway_side, gangway_at, canyon_side).
6. STORY / mission data: the line sets are played by FLIGHT's code at these moments, so do NOT also attach them to Main 1's
   step talks: `DA_Lines_LandfallDeck` 4.5 s into the flight (queued); `DA_Lines_LandfallVaneHorn` at the prow (Vane's radio,
   interrupts; an empty speaker becomes "Cornelius Vane"); `DA_Lines_LandfallBoarders` as the grapples pin her;
   `DA_Lines_LandfallGrapples` at deck clear (should tell the player to cut the lines); `DA_Lines_LandfallLanding` as she
   breaks free (interrupts; ~8 s until she's down); `DA_Lines_LandfallAnchor` when she's down, if not anchored (drop the
   anchor or she slides; the dead climbing); `DA_Lines_LandfallPoint` as Main 1 enters step 5 (sends Sully up to the Point;
   to play it on arrival instead, set `LandfallRules::PointLinesStep` to `StepSpyglass`); `DA_Lines_LandfallVaneValley` 1 s
   into the reveal (radio); `DA_Lines_LandfallTrail` right after the reveal (queued).
7. Main 1's objectives as the code expects (STORY's `create_mission_assets.py`): step 0 Reach tag `Lamplighter_Prow` radius
   350 (height ignored, the default); step 1 Event `Landfall.DeckClear` count 1; step 2 Interact tag `Landfall_Grapple`
   count 2, `bHold` true; step 3 Scene `HardLanding`; step 4 Interact tag `Landfall_AnchorWinch`, `bHold` true (+ WORLD's
   LedgeClimbers clear); step 6 Interact tag `Landfall_Spyglass`, `bHold` FALSE (a tap).
8. CODEMAP lines (add under World / Scenes / Story / Dev / Tests; update the three existing ones):
   - `World/LamplighterShip`: `ALamplighterShip`, `EShipFlight` (Sailing, Grappled, Breaking, Landing, Moored): the Lantern Wardens' airship, Landfall's stage (Main 1): sails in along `Course` with the player on her deck (a moving floor), Vane on the horn at the prow, the tender's grapples pin her dead still, boarders in two waves (`Landfall.DeckClear`), the cuts, breaking free, `HardLanding`, moored at `Moored` (groans until the anchor's down); begins from the campaign (`BeginLandfall`); saves held while she flies; keeps the player aboard. `LamplighterShipParts.cpp` hull, deck boxes, rail walls, Great Lantern, crew, gangway; `LamplighterShipBoarding.cpp` tender, grapples, waves, cuts; `LamplighterShipLanding.cpp` breaking, landing, moored, anchor, winch; `LamplighterShipStory.cpp` campaign start, player on deck, lines on Main 1's steps.
   - `World/LamplighterCourse`: `FLamplighterCourse`, her course as math (curve by distance, cruise/creep/hold speeds, bank, pitch, bob).
   - `World/LamplighterRules`: `LandfallRules`, `ELandfallStart`: Landfall's names, Main 1's step indices, `StartFor`/`ApplyStart` (new game flies, mid-air save flies again, old save moored at step 5), line sets, `Say`.
   - `World/LamplighterEffects`: `LamplighterEffects`, Landfall's dust, sparks, gas, smoke, fire (FWeaponFX) and view shakes/kicks.
   - `World/CompanyTender`: `ACompanyTender`, Vane's boarding tender: run in, station alongside, launchers, gangplank, two holding riflemen, burning fall.
   - `World/LandfallGrapple`: `ALandfallGrapple`, `ELandfallGrappleState`: a grapple (tag `Landfall_Grapple`): flight, bite, line, hold-to-cut, fall.
   - `World/LandfallWinch`: `ALandfallWinch`, the anchor winch (tag `Landfall_AnchorWinch`, hold 2.5 s, crank, chain).
   - `World/LandfallSpyglass`: `ALandfallSpyglass`, the lookout's spyglass (tag `Landfall_Spyglass`): Interact plays `SmogReveal`.
   - `World/LandfallMarker`: `ALandfallMarker`, a tagged point a Reach objective goes to, carried by what it marks (`Lamplighter_Prow`).
   - `Scenes/HardLanding`: `HardLanding`, the hard landing on the Mooring Ledge: her path as math of time (`PoseAt`), the scene (look-only, skippable).
   - `Scenes/SmogReveal`: `SmogReveal`, the look through the spyglass at the barge, its hose and smog, Vane on the loudspeakers, then Finch's trail lines.
   - `Story/VaneBroadcast`: `UVaneBroadcast`, Cornelius Vane on the radio (`Say`, `Stop`, `IsSpeaking`): crackle, captions in his name under a hiss, click.
   - `Dev/LandfallDevCommands.cpp`: `Looter.Landfall [0-8]`, `Looter.Landfall.Ship <Sailing|Grappled|Breaking|Landing|Moored|Anchored>`, `Looter.Landfall.ClearDeck`.
   - `Tests/LandfallFlightTests.cpp`: `Looter.Story.Landfall.Flight.*` (Course, OldSave, LandingPath, Sequence, Skip).
   - Update `Scenes/ColdOpenSubsystem` (a level with the Lamplighter hands the opening to her; the old opening console-only), `Areas/StationBoard` (`FirstArrivalLanding` = `Landing_Lamplighter`), `Session/SessionSubsystemNewGame.cpp` (new game lands on the Lamplighter's deck).

## Placement for WORLD (build script, Python property names)

- `unreal.LamplighterShip` (`/Script/AI_Looter_Shooter.LamplighterShip`). Pivot = her keel's middle; bow +X; deck top 56 cm
  over the pivot; rails 3.15 m out; bag top ~980 over the pivot, ~11 m wide, ~32 m long. Tags must keep
  `Landing_Lamplighter` (the script's tags replace the C++ default).
  - `course` (list of unreal.Vector, world cm, >= 2): where her keel goes, from ~350 m west, through `grapple_stop`; may run
    on toward the ledge (she creeps at most 30 m past the stop; the landing glides from wherever she's pinned to `moored`).
  - `grapple_stop` (int): the course index ~120 m short of the ledge over open air.
  - `moored` (unreal.Transform): keel's middle on the ledge ground, yaw = bow. Identity = where the actor is placed.
  - `tender_side` (float +1/-1): the open-air side at the stop; the tender runs in from 65 m astern / 32 m out / 18 m up
    to 8.5 m out, 1.1 m up, and falls away on that side.
  - `gangway_side` (+1/-1, default -1 port): the side facing ledge ground when moored; `gangway_at` (cm along her from
    midships, -1150..750, default 475 = the bow third): where ground is under that side (the rail's span there opens and a
    3.9 m plank drops to the ground).
  - `canyon_side` (unreal.Vector, default zero): toward the drop (e.g. her stern's way if the stern hangs over it): she
    slides up to 80 cm that way until anchored. Zero: she only groans.
  - `winch_offset`: only used when no LandfallWinch is placed. Leave `hull_scale`, `lantern_target` (Speaker_Delia),
    `spawn_crew`, `spawn_boarders` at their defaults.
- `unreal.LandfallWinch`: pivot at its foot on the ground; drum 1 m up along its Y; tags must include `Landfall_AnchorWinch`.
  Finds the ship itself; its chain runs to her hull 7 m fore of midships on `gangway_side`.
- `unreal.LandfallSpyglass`: pivot at its feet (eye 1.4 m up); tags must include `Landfall_Spyglass`; `barge_tag` default
  `Landfall_Barge`; it aims itself at the barge in play.
- The barge: tag `Landfall_Barge`; the reveal looks at its actor location (put the pivot mid-hull) and traces straight down
  for the hose's foot.

## Run order

Main's contract order. Nothing extra: no FLIGHT assets or scripts. After the build, before tests: STORY's
`create_story_lines.py` (else each missing set warns once: tests that treat warnings as errors would fail), then WORLD's
`build_area.py RansomsRest gameplay`.

## Tests (run once by filter, editor open)

- `Tools\runtests.ps1 -Filter Looter.Story.Landfall.Flight` : `.Course`, `.OldSave`, `.LandingPath`, `.Sequence`, `.Skip`.
- `Tools\runtests.ps1 -Filter Looter.Scenes.ColdOpen` : the old three plus `.Landfall`.
- After hooks 1-2: `-Filter Looter.Session.NewGame` / the station tests' group (whatever NewGameTests/StationTests register).

## PIE check (step by step)

1. `Tools\pie.ps1 -Map RansomsRest`: log `Landfall: the Lamplighter sails in`; the player on deck facing the prow, crew at
   wheel/lantern/engine, the lantern flame leaning toward the farm, engine and creak heard; walk around while she flies: no
   jitter, no sliding off, jumping lands back on her.
2. Walk to the Great Lantern: `Vane on the air`, his captions; the tender comes up astern (~2 s), alongside (~9 s); she slows
   at the stop; grapples fire, bite, lurch; `grappled ... dead still`.
3. Riflemen on the tender fire across; the plank drops; two riflemen board, then a hose-man and a rifleman; kill all:
   `the deck is clear`, the tracker moves to the lines; the hook prompt shows only now.
4. Hold E on each hook (riflemen still firing): crack and sparks; after the second: tender burns and falls, bag tears, 2 s
   later the landing: drop, list, crash, dust, skid, settle on the ledge. Skip it once (hold E): moored all the same.
5. Moored: she groans/shifts, Anchor lines, LedgeClimbers rise; hold E on the winch 2.5 s: crank, clank, chain, quiet.
   Off the gangway; Point lines; climb; spyglass: the reveal and Vane; trail lines.
6. Dev: `Looter.Landfall 1`, `2`, `3`, `4`, `6`; `Looter.Landfall.Ship Sailing`; `Looter.Landfall.ClearDeck`.
7. A session: Save & Quit mid-flight, load: she flies again from the start at step 0. Die on deck: woken back on deck.
   An old session with Main 1 running: moored, anchored, Main 1 at "Climb to Ransom's Point".

## Risks I couldn't check

- The character on a fast moving, rolling base (9 m/s, bank up to 6 degrees, bob): if the view jitters or the player
  slides, lower `MaxBankDegrees`/bob in `LamplighterCourse.h` or the cruise; tick order relies on the CMC's based-movement
  prerequisite (set when the character is based; `PutPlayerOnDeck` calls `SetBase`).
- Dredgers on her deck: they rely on world-static ground traces (her deck boxes are WorldStatic, the rail walls WorldDynamic
  and pawn-only) and DREDGER's ledge avoidance; the tender's two stand on its scaled skiff hulls (complex traces).
- The stand-in hull is NoCollision (its scaled side slabs reach far above the painted gunwale): shots pass through hull and
  bag. The Tripo ship needs her own deck/rail numbers (`LamplighterShipParts.cpp` constants, `IsPlayerAboard` bounds).
- Crew hands vs the stand-in wheel/lantern/barrels: tune `AWardenCrewMember::HandsShift` or the station spots
  (`Stations[]` in LamplighterShipParts.cpp).
- `ApplyStart` writes the record on every Ransom's Rest load (flags only; harmless).
- If the level has another scene playing when she breaks free, she hangs at the 2 s pose until it ends.

## Numbers to tune

Cruise 900 cm/s, creep 160, accel 120, creep limit 30 m, bank 6 / pitch 4 degrees, bob 14 cm (LamplighterCourse.h);
tender lead 2 s, approach 9 s, stop tolerance 4 m, Vane wait 30 s, lurch 1.4 s / roll 4 / pitch 2.5, riflemen 1 s, plank
2.2 s, wave 1 at 3 s, wave gap 1.5 s, timeout 25 s (Boarding.cpp, Ship.cpp); break 2 s, touch 6 s, skid 0.8, settle 1, list
9, nose 6, flare 3, squash 25 (HardLanding.h); groan 3.5-7 s, shift 12 cm, slide max 80 (Landing.cpp); hold times 1.2 s
(grapple) / 2.5 s (winch); hull scale (2.7, 2.9, 1.3); lantern scale 4.2, 40 cd.
