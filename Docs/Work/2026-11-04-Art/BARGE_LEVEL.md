# Barge level lane (2026-11-04): what the level saves about the barge's deck, and how to place it again

LEVEL lane of Art round 1, step 2 (`CONTRACT.md`). Python only: `Tools/Unreal/build_area_main6.py`,
`build_area_main6_sortie.py`. Nothing was run (editor closed for Main's measurement, C++ not built). The barge's C++
(`World/Barge*`) is the BARGE lane's; read only here.

## Why a rebuild is needed

Every piece the build script places "on the deck" is saved in `Lvl_RansomsRest` as a world position the barge's getters
gave at build time (`ToWorld` = actor transform + `KitLift`, with `BargeRules` numbers). The scripts already read those
getters (no deck number was copied, bar the hose drum's x,y), so a rebuild after the C++ build follows the code by
itself. Until then the saved spawners, horns, manifest and course end stand where the old kit was (about 8.5 m low,
inside the hull). The C++-spawned pieces (hatch marker, furnace, valves, hose drum, deck, hall, stairs, rails) are made
at BeginPlay/OnConstruction from the same rules: they move with the build, no level rebuild.

## 1. Every placement that depends on the deck's height, rectangle or berth

All in the barge's frame through `ToWorld`; "live" = read from the getter when the script runs.

| # | File : function | Placement | Read from | Status |
|---|---|---|---|---|
| 1 | main6 : `place_barge` | the barge itself: keel `chapel foot z + KEEL_OVER` (25 m) | design number, the keel, not the deck | unchanged (see "For Main") |
| 2 | main6 : `place_barge` | its x,y so the hose drum stands `DRUM_BESIDE` (7 m) off the chapel ridge | was `DRUM_LOCAL (-889, 323)` = a copy of `BargeRules::DrumUnderKeel`; now `get_hose_top()` | CHANGED |
| 3 | main6 : `place_barge` log | "the kit's deck is DeckTop 1400" | was a string; now `get_hatch_spot().z - keel` (+ `kit_lift` property) | CHANGED |
| 4 | main6 : `place_spawner` (BargeDeck) | spawner at `get_deck_middle()` + 50 cm; 9 spawn points = `get_fight_spots('BargeDeck')`, stored relative to the spawner | live (`DeckFightSpots`, `DeckAft/Fore`, `DeckTop`) | follows; log now prints heights over the keel |
| 5 | main6 : `place_spawner` (FurnaceHall) | spawner at `get_hall_middle()` + 50; 8 points = `get_fight_spots('FurnaceHall')` | live (`HallCenter`, `HallFloor`, stokeholds) | follows |
| 6 | main6 : `place_spawner` | `ground_max_rise` 300, `activation_radius` 3500 / 2200 | tolerances measured from the spawner itself, so height-independent | unchanged; stale comment "39 m over the knoll" fixed |
| 7 | main6 : `place_horns` | 4 `ACompanyLoudspeaker` posts at `get_horn_spots()` (position and yaw), attached | live (`HornSpots`, `DeckTop`) | follows |
| 8 | main6 : `place_manifest` | `AMissionItem` at `get_office_spot()` + 3 cm, attached | live (`DeskTop()`, hall frame) | follows |
| 9 | main6 : `place_wreck` | `fall_target` = canyon floor + 300 (the keel's landing), `boards_point`, the wreck prop on the floor | ground traces / layout, the keel | no deck dependence |
| 10 | main6 : `place_fights` etc. | `attach(.., barge)` KEEP_WORLD | positions as placed | follows (placed after the barge) |
| 11 | sortie : `plan` | course end: 65 m (or 0.6 x the line) short of the berth on the line from the town, z = `max(berth.z + 600, ground + 30 m)` | was `barge.call_method('GetBerth')` with a SILENT fallback to the keel middle; now `get_berth()` and a warning if it fails | CHANGED |
| 12 | sortie : `place` | `PilotSortie` actor at course[0] (over the ledge); `barge` property = the actor; the C++ reads `GetBerth/GetJumpBerth/GetGrappleAnchor/GetBoardingPoint` live at run time | live | follows; label now removed first (`remove_stale`), log prints the course end, the berth and its height over the keel |
| 13 | main6 : `hide_sink_barge` | edits Landfall's Sink barge and the Sink intake | nothing of the Main 6 barge | no deck dependence |

Not in the level and not deck data: the hatch Reach objective (`create_mission_assets.py`, tag `Barge_FurnaceHatch`,
radius 300, height counted) and Main 6's other objectives find actors by tag; `create_story_lines.py`, `story_texts.py`
(`BargeDeck`/`FurnaceHall` conditions) hold no positions. Landfall's barge (`build_area_landfall.py`, `Landfall_Barge`;
mains 3, 4, 5, 7) is the Sink's stand-in, a different actor: its "DeckTop" is the Lamplighter's. No tour view
(`views.json`) looks at `Barge_Pilot`. `layout.json` line 3212 (`BargeDeck` encounter note) says "the barge's deck 25 m
over the chapel knoll": text only, the keel is 25 m, the deck will be about 47.5 m (not edited, not a script).

The Python's contract with the C++ (renaming any of these breaks the rebuild, so BARGE must keep them): UFUNCTIONs
`GetBerth GetFightSpots GetOfficeSpot GetHornSpots GetHatchSpot GetDeckMiddle GetHallMiddle GetHoseTop`; properties
`hull` (component, `static_mesh`), `hose_target`, `fall_target`, `boards_point`, optionally `kit_lift` (read for the log,
guarded). `GetHatchSpot().z` must stay the deck's walking surface (the log's "deck over keel" reads it).
`GetBoardingPoint`, `GetJumpBerth`, `GetGrappleAnchor`, `GetPlayerStart` are not UFUNCTIONs, Python can't call them.

## 2. What was changed

- `build_area_main6.py`
  - `place_barge`: no `DRUM_LOCAL`. The barge is placed over the ridge, turned, then moved by `get_hose_top() - actor
    location` so its drum lands `DRUM_BESIDE` off the ridge (same position as before, from the C++ drum).
  - `place_barge` log: hull mesh top (the mesh's bounding box, stacks included, as before), the kit's deck over her keel
    from `get_hatch_spot().z - keel`, `kit_lift` as the actor has it, and the difference. The "BargeRules::DeckTop 1400:
    set kit_lift" claim is gone (the deck follows the hull by design, in BargeRules).
  - `place_spawner` log: the spots' heights over her keel (`low..high`) and the spawner's; comment no longer says 39 m.
  - `tag_conditions` adds a Main 6 condition only if not already on the actor (the module can now run alone).
  - New `run(name)` and `__main__` (as `build_area_landfall.py`): `open_level('Gameplay/Main6')` clears only that folder,
    `place(build)`, saves.
- `build_area_main6_sortie.py`: `barge_berth` (was `barge_berth_guess`) calls `get_berth()`, warns loudly if it can't
  (twice: the reason, and that the course end is then aimed at the keel middle); `plan` takes the berth; `remove_stale`
  for `PilotSortie`; the log line says where the course ends, how far from the berth, the berth and its height over the
  keel; the `END_OVER_KEEL` comment says "berth", not "keel".
- Syntax-checked only (`ast.parse`); not run.

## 3. Rebuild (after `Tools\launch.ps1 -Build` and the editor back, MCP up; not during PIE)

The getters answer from the compiled class, so build first or the pieces land where they were.

Preferred, Main 6 alone (seconds to a minute; the sortie's sweeps trace the world):

    Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_main6.py RansomsRest" -Until "Main 6's pieces placed and saved"

`RansomsRest` is the area folder under `Art/Levels` (build_area.py's usage). It opens `/Game/Maps/Lvl_RansomsRest` if
another map is open (refuses with unsaved changes elsewhere), clears and places `Gameplay/Main6`, saves the level (the
`.umap`, an LFS binary, for Main to commit).

Fallback, the whole gameplay pass (minutes; re-places every gameplay actor, so after it follow `Pipeline/Levels.md`:
`build_area_loot.py`, `build_area_fauna.py` again):

    Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest gameplay" -Until "gameplay actors placed and saved"

Neither needs `swap_materials` for the barge (RansomsRest's `level.swaps` are rock, leaf and terrain materials only).

## 4. What is removed and placed again (all in outliner folder `RansomsRest/Gameplay/Main6`, tag `RansomsRestBuild`)

Standalone: `open_level` destroys every build-tagged actor in that folder, so all of these, then each is placed again
(each also has its own `remove_stale` by label, which matters if `place()` is called some other way):

| Label | Class | Notes |
|---|---|---|
| `Barge_Pilot` | `ABargeBoarding` | tags `Barge_Pilot`, `Obstacle`, `Floor`; `hose_target` set |
| `Encounter_BargeDeck`, `Encounter_FurnaceHall` | `AEncounterSpawner` | attached to the barge |
| `Barge_Horn_1` .. `_4` | `ACompanyLoudspeaker` | tags + `Barge_Horn`; `_1` carries Vane's deck cue |
| `Item_BargeManifest` | `AMissionItem` | attached |
| `Main6_BargeWreck`, `Main6_WreckSmoke_1` .. `_3` | `AStoryProp` | wreck in Gravewind Canyon; not deck data, re-placed anyway |
| `PilotSortie` | `APilotSortie` | `course`, `return_course`, `ship`, `barge` set |

13 actors in all. Not removed, set again: the Lamplighter's `gone_when`, the anchor winch's `cast_off_when` (overwritten),
`hidden_when` of every actor labelled `Landfall_Barge*` / `Landfall_Smog*` and `gone_when` of every `Intake_Sink` actor
(a Main 6 condition added only if missing). Hob is not touched (the wheel spot doesn't move).

## 5. Check in the editor afterwards

Let K = the barge's keel z (chapel foot + 2500, unchanged) and D = the new `BargeRules::DeckTop` (old 1400, now about
2250: everything below about 850 cm higher than before).

- Log (`Saved/Logs/AI_Looter_Shooter.log`, lines `RansomsRest: ...`): "the barge over the chapel ... the kit's deck N cm over
  her keel (kit_lift 0)": N = D. "BargeDeck: 9 Dredgers ... (9 spots L..H cm over her keel, spawner S cm over it": L = H = D + 20,
  S = D + 70. "FurnaceHall: 8 Dredgers ... spots ...": with the hall riding the deck (BARGE's in-progress
  `HallFloor = DeckTop - HallUnderDeck` 1000) L = H = HallFloor + 20 = D - 980, spawner D - 930 (old: 420 and 470); else as
  BARGE leaves the hall. "the Main 6 sortie ... berth ... N cm over her keel":
  N = D - 572 (D - `BerthUnderDeck` 150 - ship deck 422); the course end z = berth z + 600 unless the ground rule wins.
- No warning lines: "can't be called from Python", "aimed at the barge's keel middle", "no ... class", "left out".
- Outliner, folder `RansomsRest/Gameplay/Main6`: 13 actors. Actor z (world): `Encounter_BargeDeck` K + D + 70;
  `Barge_Horn_N` K + D (feet on the deck); `Item_BargeManifest` the desk (K + HallFloor + 482 + 3); `PilotSortie` unchanged at the
  ledge, its last `course` point about 850 cm higher than before. Anything in the folder lower than K + D - 1000 is stale.
- Visual: viewport on `Barge_Pilot`: spawner and horn gizmos on the Tripo deck, none in the hull; the hose from the drum
  under her keel to the ridge as before (the barge's x,y is what `DRUM_LOCAL` gave while `GetHoseTop` stays (-889, 323)).
- The script saves the level itself; Main runs `Looter.World.Barge.*` and the PIE grabs after.

## For Main and the BARGE lane

- `KitLift`: the script spawns a fresh actor and sets only `hose_target`, so a raise stored as a property on the placed
  actor would be lost at the next rebuild. BARGE's header keeps the deck in `DeckTop` (C++) and `KitLift` at 0: right. If
  that ever changes, `KitLift` needs a C++ default, not a Python line. (`ToWorld` adds `KitLift` to everything, hall included.)
- `KEEL_OVER` stays 25 m: the deck moves from 39 m to about 47.5 m over the chapel knoll, the mesh top (stacks) from about
  73 m. A design call, not a defect; lowering the keel is not an option (the spire's cross is ~16 m, the drum hangs under it).
- The sortie's course is swept for the ship against collision only: the build-time sweep sees the hull only if the hull
  mesh has collision (the Tripo one had none). The last glide (C++, from the course end to the standoff) is not swept at
  all: if the town lies across the barge from her berth side the glide crosses the hull and its superstructure. Worth one
  look in PIE (`Dev/PilotSortieDevCommands.cpp`).
- Counts: the deck fight is 9 creatures (`DECK_GROUPS`) on 9 spots and the mission's `count=9` (`create_mission_assets.py`),
  the hall 8 and `count=8`; changing `DeckFightSpots`' or the creatures' number needs the other changed too.
- `Docs/Pipeline/Story.md` (Mains 6-7 paragraph) could note that `build_area_main6.py` now runs alone like Landfall's.

## 6. The reworked Tripo hull: Landfall's barge over the Sink and the wreck

The hull comes back at the same path, `/Game/Art/Tripo/SM_DredgeBarge`, in a new shape (flat deck at 2250 over x -400..3350,
the aft house to x -400, the stacks; sockets `HoseDrum` (-889, 323, 0), `Stack_1`, `Stack_2` kept, `Door` and `Gangway`
new). For a few hours the scripts pointed at a scripted `/Game/Art/Vehicles/SM_DredgeBarge`; the user dropped it and that
is fully undone (no `SCRIPTED_BARGE`, no second path anywhere under `Tools`).

Edits (Python only; `ast.parse` checked, not run), net against the files as they were before the hull swap:
- `build_area_landfall.py:1273-1279` (`barge_parts`): after `tripo_barge(plan, hull)` a warning if the mesh lacks a
  `Stack_1` or `HoseDrum` socket (no plumes on the stacks; the hose then leaves from the hull's bottom middle). The only
  change left in Landfall's script. `STAND_INS['barge_hull']` is the Tripo path, so `is_tripo` sends it down the Tripo branch
  (the mesh as is, its own materials, plumes at `Stack_n`, hose at `HoseDrum`; the new `Door`/`Gangway` sockets are ignored).
- `build_area_main6.py:74` `WRECK_MODELS`: Tripo `SM_DredgeBarge` second again (the wreck's fallback hull, worn in
  `MI_GunScorched`; `SM_BargeWreck` stays first).
- Nothing else under `Tools` names the hull (`tripo.py`, `Blender/looter_tripo_wardens.py`, `import_tripo.py` list
  `DredgeBarge` as a Tripo model to make or import). Main 6's own barge takes its hull from C++ (`ABargeBoarding::HullPath`,
  the BARGE lane's), not from these scripts.

Precondition: the reworked mesh imported at that path (re-import over the old asset; sockets come with it). The C++ built
for Main 6. The meshes are read live, so there is no stale copy to clear: Landfall's bake is off (`BAKE = False` since
2026-11-01; the barge is placed as one prop `Landfall_Barge_01`, its log line still says "baked" for a single part).

Who reads the hull: Landfall's `place_barge` (the Sink barge, the stack plumes at `Stack_n`, the drum from `HoseDrum`, the
drone path from the hull's length); Main 5's `Hose` (re-plans with `lf.barge_parts`: the intake's `dying_stacks` are the stack
sockets other than `Stack_1`, the hose top is the `HoseDrum` socket); Main 6's wreck. Main 4 (the tender, keel only) and
Main 3 (Ned's skiff, the `Landfall_Barge_01` actor's location = the keel) do not depend on the mesh. The hose's start does
not move while `HoseDrum` stays (-889, 323, 0).

### Commands

Landfall alone (seconds to a few minutes; it traces the ledge grid and sweeps the Lamplighter's course; refuses with unsaved
edits in another map):

    Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_landfall.py RansomsRest" -Until "Landfall's pieces placed and saved"

It clears `RansomsRest/Gameplay/Landfall` (tag `RansomsRestBuild`) and places again: `Lamplighter`, `Landfall_AnchorWinch`,
`Encounter_<ledge spawner>` (LedgeClimbers) and `Encounter_<porch spawner>` (FarmPorch), `Landfall_Spyglass`,
`Landfall_Barge_01`, `Landfall_BargeHose_01`.. (the sampled hose sections), `Landfall_Smog_1`.._N (one per `Stack_n` socket),
`Landfall_SmogGlint`, `Landfall_SmogIntake`, `Landfall_SmogDrone`, `Landfall_AbelTrail*`, `Landfall_AbelsHat`,
`Landfall_Loudspeaker` target points, `Landfall_SpeakerPosts/Horns/Plates`. It rewrites the Smog rule of the deck's
`StoryLighting` (that actor lives in `Gameplay`, not cleared). Hob and Delia are not placed (they need the whole pass, and
do not move).

What a Landfall-alone run breaks: it re-creates actors that Main 5 and Main 6 patch.
- Main 5 added `hidden_when` (from the choke) to the `Landfall_BargeHose*` props and to the plumes other than
  `Landfall_Smog_1`, and tagged the hose props `Landfall_BargeHose`; its intake's `dying_stacks` come from the sockets. Main 5
  has no standalone entry. A Landfall-alone run loses the hose and plume patches (they stay shown after the choke).
- Main 6 added `hidden_when` to the same pieces, `gone_when` to the `Intake_Sink` actors and the ship, `cast_off_when` to
  the winch, and the `PilotSortie`'s `ship` reference: all lost with the re-created ship and winch. Main 6 alone repairs these.

### Run order

1. Re-import the hull; `Tools\launch.ps1 -Build` (Main); editor back, MCP up.
2. Complete and consistent (recommended): the whole gameplay pass. It runs the story's modules in their fixed order (the
   deck, then Landfall, Mains 2-5, Main 6 after them, Main 7), so Main 5's patches and Main 6's (barge, sortie, hide) land
   on the new Landfall pieces:

       Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest gameplay" -Until "gameplay actors placed and saved"

   Nothing is needed after it for the barge. `build_area_loot.py` and `build_area_fauna.py` do not need a rerun: they read
   ground-level things (crates, graves, perches) and the gameplay pass places the same ground-level pieces at the same
   spots (the plan is traced and deterministic; the barge is 45 m up and no loot or perch stands on it). They are idempotent
   if wanted (`py .../build_area_loot.py RansomsRest`, `py .../build_area_fauna.py RansomsRest`), as `Pipeline/Levels.md`
   says after any gameplay rebuild. Creature levels in a gameplay pass are rolled again inside their bands.
3. Quick, barge only (leaves Main 5's patches on Landfall's hose and plumes missing until the next gameplay pass): Landfall
   alone, then Main 6 alone, in that order (Main 6 patches what Landfall just re-created; the other way round, Landfall
   wipes Main 6's patches and its sortie's refs):

       Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_landfall.py RansomsRest" -Until "Landfall's pieces placed and saved"
       Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_main6.py RansomsRest" -Until "Main 6's pieces placed and saved"

4. If only Main 6 is wanted (the Sink barge's props already show the reworked mesh, since the mesh is read live; only the
   stack plumes and Main 5's `dying_stacks` sit at the old socket places until Landfall and Main 5 run): section 3's Main 6
   command alone.

Check after: Landfall's log "Vane's barge over the Sink (75 m, ...), its hose (...), N stacks smoking" (N = the `Stack_n`
sockets, 2); no "has no socket" warning; Main 5's "Landfall's hose (N prop(s)) and N-1 plume(s) hidden" (gameplay pass) and
"1 stack(s) dying at the choke"; Main 6's wreck line "the barge's wreck ... (/Game/Art/Tripo/SM_DredgeBarge)". In the
viewport: a plume over each stack mouth, the hose leaving from under the keel.
