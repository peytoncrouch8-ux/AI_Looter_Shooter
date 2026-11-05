# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed; the 2026-10-27 playtest rounds 1-5 are there).

## START HERE (2026-11-05 08:20: TWO playtest builds ready; the user hasn't played either)
**Builds:** build 2 `Builds\Playtest6-11-05_0800\Windows\AI_Looter_Shooter.exe` (commit dffbb14a, smoke PASS, full cook
5573) = build 1 (`2026-11-05_0158`, 9052a9dc) + round 7: the town dressed by rules (338 pieces; town_rules.py), the smithy, bathhouse
and Home L placed, PERF2's fight fixes (hit zones once a frame; Abel's lines held), the tour's warm pass fixed. Page (v3):
https://claude.ai/artifact/CCLjmC72XB8wg7wGfDE41k (scratchpad `changes_page/`). Build 2's clean warm tour: ALL 53 views <= 8.3 ms
(Aerial_Dusk 8.16, Farm 8.15, Aerial 8.13; town views 7.8-7.9; Abel deck fight 7.15; horde 7.9; deck 7.9).
**Ask the user (two failed fixes, their call):** the barge's starboard walk (Looter.World.Barge.Fight; the cause is open:
Bugs.md "OPEN: the barge's starboard walk"); the 13 of 16 new buildings without a site (LEVEL: the town core is full; new ground
such as a back street on a pad, or sites outside the core).
**After build 2 (committed, not in a package):** the dredge hose under Landfall's barge was 8 of 13 sections since 11-01 (BAKE off;
fallback_most=8): now all 13 (`build_area_landfall.py`; checked in a shot). Next round: the bathhouse's north-wall handbill 2 cm into
the wall (DEMO3), the boiler fire unseen, the Unpaid's crit is the CHEST COAL (MORNING.md corrected; the page too). Lessons:
Workflow.md (big test filters crash the RX 580's driver at ~196 tests; watchdog every chain's tests), Bugs.md.

## Earlier: the first playtest build (2026-11-05 02:40)
**For the user (morning):** the build `Builds\Playtest6-11-05_0158\Windows\AI_Looter_Shooter.exe` (commit 9052a9dc, smoke PASS,
1.79 GB, full cook) and the what-changed page https://claude.ai/artifact/CCLjmC72XB8wg7wGfDE41k (scratchpad `changes_page/`: Phase 3's
12 feel changes, before/after sliders from `Saved/ArtPreviews/Step1/before` vs clean shots of 02:32, the frame table, the honest list).
Ask them: the 12 of 16 new buildings with no site (LEVEL: the town core is full; they need new ground, e.g. a back street on a pad).
**Round 6 results:** f3bd38c7 (6a) and 9052a9dc (6b). Warm 53-view tour 01:24 measured in the WRONG state (the warm pass ran
BargeDeck_Landfall's exec, nothing undid it: Day light, Main 6 state, a second barge in view): Farm 8.50, PointOverValley 8.33 over,
51 of 53 under. PERF2's quiet traces in a new game's Smog state: every view under (Aerial 8.1, Farm 8.1, POV 8.0, MainStreet 7.7);
the round-5 game-thread rise was machine contention (a repack + a stray `find /` from the LIGHTS lane), not code. Deck row 7.9,
horde 7.9 (this morning 7.4). Barge 11/12: Fight's starboard walk blocked at (1590, 750) in Hull.
**Stashed for round 7 (`git stash list`: "round 7 source"; also `Docs/Work/2026-11-04-Art/round7_source.patch`):** PERF2's
`CreatureBase.cpp` bDeferKinematicBoneUpdate (-0.3..-0.4 ms in fights), `LamplighterRules` LoadLineSet, `AbelCorruption` preloads his
line sets (a 16 ms hitch), and the tour fix (`ViewTour.cpp`: the warm pass skips views with exec but no after). The package was
restarted once: PERF2 edited AbelCorruption.h mid-compile (lesson: no source edits while a package or build runs).
**Round 7 inputs:** `town_next.json` (DRESSING step 4: 299 pieces, 18 new types; `make_town.py --out`), LEVEL's
`towns_buildings.patch.json` (smithyTown, bathhouse, homeL, privyB + fires; heights unknown until a probe; `_O` swaps L for O),
`Intermediate\ArtExport_Buildings2` (16 models, BUILDINGS.md), REWORK's fixed hull (FOOT_MEDIAN, route check) rebuilt with the walls
set by REPACK into `ArtExport_BargeWalls`. Second package only if every view <= 8.3 and the tests pass (the user's rule).

## Earlier: art round 6 (2026-11-04 23:00)
**The user's calls at bedtime (23:15):** a view a bit over 8.3 ms does NOT hold the playtest build: package anyway, the page lists
which views are over and by how much, perf fixes continue after; after the package, KEEP GOING overnight on the towns (DRESSING,
BUILDINGS; a second build only if it measures under 8.3 ms and passes tests; the first build stays untouched); Home B goes BY THE
GATE (2.8 m from the Unpaid's rise spot in the TownGate fight: `towns_homeB.patch.json` re-applied 23:14; check that fight in PIE);
the Lamplighter keeps our textures (the Unpaid's rebake stays off). Chains: `build_round6a.ps1` (running 23:10) then
`build_round6b.ps1` (the round-5 hull `ArtExport_TripoR5`? and walls `ArtExport_BargeWalls`: set `$Imports` from the lanes'
notes; the perf window and PERF2's traces), then package.
**Round 6 (Main's chain `build_round6.ps1` in the scratchpad: imports, DetailInstances, close, build, modes environment/gameplay/
loot/fauna/dressing/lights, `build_area_town_looks.py`, missions, tests, then the warm 42-view tour WITH shots (`tour.ps1 -Warm`,
new: every view once unmeasured first; shots to `Saved/ArtPreviews/Round6/after/` vs this morning's `Step1/before/`), the deck and
horde rows).** In it: KIT round 18 (the barge's front half was outside the boundary: `IPlayableAloft`, `APlayableArea::AddAloft`;
the tower push could never spawn; camps test reads `aloft`/`maxRise`), LIGHTS round 2 (gloom lamps reach 400, drawn to 25 m;
HouseLights test renamed `.Lamp`), Smog/SmogThin sun `shadowDistance` 6000 (layout.json; `environment` mode), MATERIAL's TrimRecolor
+ `town_looks.json` (built in the editor; round 2 pushing stronger tints; zero custom data = no tint; looks saved as default custom
primitive data), imports `ArtExport_Repack` (5 re-cut Tripo models), `ArtExport_Rebake2_Lamp` (the Lamplighter in our sets; the
Unpaid's rebake is OFF: worse up close), `ArtExport_TownProps` (48 town models, `Docs/Work/2026-11-04-Towns/PROPS.md`). Waiting on:
REWORK round 5 hull (stairs, walks, berth, `_Walls` slot), REPACK's walls set, LEVEL (RR_Home_B/I sites, WestRoadCrew moved to the
chapel end), PERF2 (the game thread rose ~1.3 ms on every view in round 5, DeckFight_Dusk 7.2 -> 11.4 ms; notes `Docs/Work/2026-11-04-Art/PERF2.md`).
New lanes: DRESSING (PLAN step 4: town_rules.py, clutter by rules) and BUILDINGS (step 5: home kits, RR_Home_K..P, outbuildings).
After round 6: package (`Tools\package.ps1 -CloseEditor -SmokeTest`) + the one-page what-changed artifact (before JPGs in the
scratchpad `changes_page/img/`; Phase 3's 12 changes from `Docs/Work/2026-11-03-Phase3/MORNING.md` + today's art).

## Earlier this evening (art round 4 on CONTRACT.md "Round 4"; barge round 3 committed in 0c0750bf)
**The user, 21:30:** their subscription ends on the 7th, and they will not renew unless these changes make the game better.
Main's answer: judge by the game, not by reports: after the round 5 chain, a smoke-tested playtest build plus a one-page list of
what changed and where to look (before/after shots), so a few minutes of play shows it; anything that plays worse is fixed or
rolled back first. Nothing of Phase 3 or the art rounds has been played yet.
**The user's calls this evening:** next "Sharper Tripo models" (REPACK lane); barge leftovers "Berth repetition, Plain stairs,
Blurry house walls" (REWORK r5, REBAKE); perf "Alongside the next work" (PERF lane: the heavy views wait on last frame's
occlusion queries, GPU + 1.2-1.4 ms; `r.NumBufferedOcclusionQueries 2` on test). From a grill round: playtest "Later this week"
(keep building); 120 fps "Hard line, every view" (CLAUDE.md); the deck fight "Bigger" (three pushes, droppers from the house roof
and the tower, a push from the stern: KIT round 14); Mains 4-7 set pieces "Script-made only" (CLAUDE.md). The full PIE walk
(`Saved/ArtPreviews/BargeRound3Final/`) found: no boarding from the Lamplighter (her sortie opens `Rail_Port_4`, a stale index
since the COLLISION lane's 20 rails a side, and her `Locker` block sits on the plank's hinge: COLLISION fixing), knife-edge lanes
to the walks, the manifest 2.1 m up on the bench's tool wall, the desk blocking the catwalk ring, no inner handrail (a 5.8 m
fall), dark forecastle, weak lamps (KIT round 13); the hall walls' blotch tiling (`MI_BargeKitIron` at full strength).

## Round 3 of the barge (2026-11-04 ~11:00; committed 0c0750bf; a81c52ab had the boxed house the user REJECTED)
The user on a81c52ab's barge: "why the fuck did you put a big box on the Tripo model. This does not look good in slightest. Use
the Tripo model and adjust it, don't just throw a structure on the ship and call it a day." Then: pieces may be moved, resized,
retextured ("act like a senior game designer and developer"); the hall "Below deck, smaller". Round 3 (`CONTRACT.md` "Round
3": the look bar, the jumbo design, BARGE_LAYOUT.md owned by the model) runs on lanes REWORK (the model), MATERIAL (per-slot
detail, world-space iron for the kit), REUSE (props), KIT (round 6: zones, the hall below deck). Lesson: Main judges every
preview against the bought model's own preview before passing it; a contract that forces geometry onto a bought model is wrong.

## Before round 3 (2026-11-04 day: Art round 1 built and committed, a81c52ab)
The user's calls today, in order: "Art: steps 1-2" (not play-testing Phase 3 yet); "more agents for faster delivery"; after the
measurement, the barge on a scripted hull in our sets; after seeing it in PIE: **"remove the hull we just made, and use the Tripo
model but rework the model to fit our needs and expectations"**, then **"Reshape for the kit"** and **"Tripo paint + our tiled
detail"**; step 1 kept as **filtering + full mips** (creature LOD bias back to the engine's Medium). Job notes:
`Docs/Work/2026-11-04-Art/` (CONTRACT.md: every shared number and decision, "Round 2" is the one in the game; BARGE.md the kit's
design, round 4-5 current; BARGE_MEASURE.md why the Tripo hull failed as it was; BARGE_LEVEL.md the build scripts and the rebuild
order). The scripted hull (`DredgeBarge.py`, `looter_barge.py`, its assets) is deleted, never committed.
- **Step 1** (`Settings/GraphicsSettingsSubsystem.cpp` QualityVariables, `Tests/SettingsTests.cpp`, the preset help text):
  Medium keeps full-size textures (r.Streaming.MipBias 0), 8x anisotropy, a 1000 MB streaming pool; r.SkeletalMeshLODBias stays
  1. Measured in `Docs/Performance.md` (rows "Art step 1", "isolate", "Art round 1 final"; the 42-view tour CSVs and shots in
  `Saved/ArtPreviews/Step1/`, close-ups in `closeup/`): the filtering is free and the only visible gain, the mips 0.2 ms.
- **Step 2, the barge:** the Tripo hull as bought has no deck to stand on (28% of the kit's rectangle standable, no opening, a V
  hull), so `Tools/Blender/looter_tripo_barge.py` (+ `looter_tripo_rework.py`, `looter_tripo_barge_house.py`, hooked as
  `rework=` in `looter_tripo_wardens.py`) reshapes it on import: the midship superstructure cut, the deck filled flat at 2250
  over x -400..3350, the house x -3300..-400 full beam to 3300 with a 240 x 300 door at (-400, 0), the tower moved onto its
  roof, the bucket moved forward onto a seat at the bow, ribs, portholes, a catwalk ring, a door frame and steps, a bulwark with
  the gap at BerthX 2000, 47 UCX hulls, sockets Door/Gangway/HoseDrum/Stack_1/Stack_2, new faces on Tripo atlas patches;
  `MI_TripoDredgeBarge` wears M_World's new opt-in object-space triplanar detail layer (MetalWorn, ~0.1 ms; the importer clears
  the switch: after any re-import run `build_world_materials.py DetailInstances`). The kit (`World/BargeRules.h`, rounds 4-5):
  DeckTop 2250, the arena x -300..3300 x +-1100, the house hollow, door -> vestibule -> office -> the furnace hall AT DECK LEVEL
  (centre (-2050, 0), R 1100, catwalk 4 m up with two stairs through openings in the ring), no crane, vents flush; new files
  `BargeBoardingHall.cpp`, `BargeBoardingSpots.cpp`, `BargeBoardingBeatsTests.cpp`, `BargeTestWorld.h`. The level rebuilt
  (`build_area.py RansomsRest gameplay`, Landfall before Main 6, both barges on `/Game/Art/Tripo/SM_DredgeBarge`). Tests Barge
  9/9 (`.Hull` traces the imported collision), PilotSortie 5/5, Settings 13/13. PIE walked by the DEMO lane (feet on the deck,
  the bulwark stops you, door/vestibule/office/hall, no sky; the stairs' re-lay checked last, see the Game.md line; grabs in
  `Saved/ArtPreviews/BargeRework/`). Pipeline: Game.md "The barge: the reworked Tripo hull and the kit on it", Assets.md item
  12 and the detail-layer bullet, Levels.md "The barge in the level", Workflow.md (measure a Tripo model's walkable surface
  before fitting gameplay to it; rework the bought model, don't replace it; the lane split).
- **Faults seen, not fixed (the audit's plan row 3, the kit made real, is next):** the kit's engine-cube rooms (vestibule,
  office, hall) carry stretched cube UVs and have no light of their own; the office floor and desk are flat `MI_LanternWood`;
  the vents are plain slabs with bare cone fires; the kit's falling stack is a bare cylinder; the house walls are flat Tripo
  atlas patches (the detail layer gives grain, not plates); the deck's 3 m tiles are plain; the Lamplighter's plank and the 5 m
  jump were not walked; the old `AttachTo` warning (static `EncounterSpawner` under the movable `BargeBoarding`) stands.
- Lanes today, all on CONTRACT.md with one editor lane (Main builds, imports, rebuilds, tests): MEASURE, KIT (5 rounds), HULL
  (dropped), REWORK (2 rounds), MATERIAL, LEVEL (3), REVIEW, GRAB, DEMO (3), DOCS (2). Lesson: a review lane between a code
  round and the build paid for itself (10 real fixes); a demo walk after every build found what tests did not (the stairs).
Suggestions (not done, the user's call): plan row 3 first (interior and deck pieces as scripted models or world-aligned iron,
lights in the vestibule, office and hall); the Phase 3 suggestions below. Job notes: `Docs/Work/2026-11-04-Art/` (CONTRACT.md: every shared number and decision; BARGE.md the kit's
design, rounds 1-3; BARGE_MEASURE.md why the Tripo hull failed; BARGE_LEVEL.md the build scripts and the rebuild order).
- **Step 1** (`Settings/GraphicsSettingsSubsystem.cpp` QualityVariables, `Tests/SettingsTests.cpp`, the preset help text):
  Medium keeps full-size textures (r.Streaming.MipBias 0), 8x anisotropy, a 1000 MB streaming pool; r.SkeletalMeshLODBias stays
  1. Measured in `Docs/Performance.md` (rows "Art step 1", "isolate", "Art round 1 final"; the 42-view tour CSVs and shots in
  `Saved/ArtPreviews/Step1/`, close-ups in `closeup/`): the filtering is free and the only visible gain, the mips 0.2 ms.
- **Step 2, the barge:** the Tripo hull has no deck to stand on (28% of the kit's rectangle standable, no opening, a V hull: the
  hall fits nowhere), so `Art/Models/Vehicles/DredgeBarge.py` (toolkit `Tools/Blender/looter_barge.py`) builds the hull in our
  sets -> `/Game/Art/Vehicles/SM_DredgeBarge` (+ `MI_BargeIron`, `MI_BargeRust`; 41k triangles, 65 convex hulls, sockets
  HoseDrum/Door/Stack_1/Stack_2/Gangway); the kit refitted (`World/BargeRules.h`: DeckTop 2250, the arena x -300..3300 x
  +-1100, the house x -3300..-400 with its door at (-400, 0), a vestibule, a straight 32° stair, the office, the hall at (-1900, 0)
  floor 1250, the berth/plank/jump recomputed against the bulwark; new files `BargeBoardingHall.cpp`, `BargeBoardingSpots.cpp`,
  `BargeBoardingBeatsTests.cpp`, `BargeTestWorld.h`); the level rebuilt (`build_area.py RansomsRest gameplay`, Landfall before
  Main 6, both barges on the new asset). Tests: Barge 9/9 (the new `.Hull` traces the imported collision), PilotSortie 5/5. PIE
  walked by the DEMO lane (feet on the hull's plates, the bulwark stops you, door/stair/office/hall, no sky leaks; grabs in
  `Saved/ArtPreviews/Barge/`). Pipeline: Game.md "The barge: scripted hull and the kit on it", Assets.md item 12, Levels.md "The
  barge in the level", Workflow.md (measure a Tripo model's walkable surface before fitting gameplay to it; the lane split).
- **Faults seen, not fixed (the next step = the audit's plan row 3, the kit made real):** the kit's engine-cube rooms
  (vestibule, office, hall) carry stretched cube UVs and have no light; the vents are plain slabs with bare cone fires; the
  kit's falling stack is a bare cylinder beside the model's two; at range the hull reads as a flat-ended box with a gridded rust
  set (less shaped than the Tripo's: a sheer/flare/scoop pass is the user's call); the deck's MetalWorn shows its repeat (plan
  row 4, the detail layer); the Lamplighter's berth, plank and the 5 m jump were not walked; the old `AttachTo` warning (static
  `EncounterSpawner_6/7` under the movable `BargeBoarding_0`) stands.
- Lanes today, all on CONTRACT.md with one editor lane (Main builds, imports, rebuilds, tests): MEASURE, KIT (3 rounds), HULL,
  LEVEL (2), REVIEW, GRAB, DEMO, DOCS. Lesson: a review lane between a code round and the build paid for itself (10 real fixes).
Suggestions (not done, the user's call): plan row 3 first (interior and deck pieces as scripted models or world-aligned iron,
lights in the vestibule, office and hall); then row 4 (the detail layer) and a hull shape pass; the Phase 3 suggestions below.

## Before today (session end 2026-11-04 morning: Phase 3 of the core-loop audit is BUILT and packaged; nobody has PLAYED it)
The user plays first, one change at a time, with the editor focused: `Docs/Work/2026-11-03-Phase3/MORNING.md` (the per-change
PIE checklist: what to feel for, the switch for the old feel, commit, tests) and the packaged build
`Builds\Playtest\2026-11-03_2355\Windows\AI_Looter_Shooter.exe` (smoke test: menu and Ransom's Rest PASS; 1.77 GB; 26 min to
make; saves in `%LOCALAPPDATA%\AI_Looter_Shooter\Saved`). Audit: `Docs/Work/2026-11-03-Audit/AUDIT.md` (Phase 1 findings, Phase 2 plan); lanes: `CONTRACT.md` and
HITS/RECOIL/FIGHTS/LOOT/CAMERA/GAMEPAD/MUZZLE/PERF/UNPAID/RELOAD.md beside the morning note. Commits: 1 6e64f232; 2 and 9
c6255e84 (also change 1's test fixes: `git commit` sweeps everything staged); 8 5a67f6b0; 10 7317ee9c; 7 fbceab1e; 5 f0ff713d;
4 c5fcb0d2 + retune ea3e46be; 6 f02daeb6; 3 0a7d7835; docs c665a1ba; 11 284bd823; 12 ce9543bc; test fix 70059aa3. Pipeline notes:
Gunplay.md (crits, fire cadence, hits, recoil, muzzle light, reloads with snap), Enemies.md (kill speed, fights press and the
sim retune, the Unpaid one gate), Game.md (loot, camera, gamepad, actor index).
The user's calls this round: start with kill speed; spiders may change; lower enemy health, never raise guns; run agents in
parallel; go on to 11 and 12 overnight; package at the end; Main may retune to the 8-12 s band; their worst moment is the ART
(Tripo models blurry, don't read as intended, collisions wrong; the barge ships, terrain): read-only audit
`Docs/Work/2026-11-03-ArtAudit/AUDIT.md` + sheet `Saved/ArtPreviews/ArtAudit/compare.png` (ranked findings, protect list, the one
change first, a plan; nothing changed, no credits spent). Not approved yet: ask before any art work.
Decided unattended (night of 11-03): fights retuned to the band from the sims (DamageScale 1.4, spider bite 8, slime bite 10;
six spiders kill a standing player in 11.7 s, Unpaid 10.3 s, slimes 18.7 s: loosest, left); the Unpaid's hunched drift is OFF
by default (`Looter.Unpaid.Idle.Hunched 1` shows it: on, the lunge pressed an upper arm 5.3 cm into the trunk and the hunting
hands came only 3 cm forward); the Unpaid arms test's armpit limit 4.5 -> 5.5 cm (change 4's ring at 300 deepens the failed
lunge's shriek fold to 5.3 cm, hidden under the skin by the test's own notes; 480 passes; the user decides whether to chase it);
the melee jab test reads the rules' seconds (it had read old numbers since dbf6cfa4).
Perf (Medium 1080p, Ransom's Rest town, 10 Basic spiders; rows of 2026-11-04 in `Docs/Performance.md`): the horde idle 7.4 ms /
135 fps (p95 8.6; game 7.3, render 5.0, GPU 6.2); 30 s of fire into it 8.3-8.5 ms / 118-120 fps with every gun (p95 9.8-10.0; game
8.2-8.5 vs GPU 6.6-6.8: game-thread bound, the fire costs ~1 ms of game thread); the muzzle and impact lights off change nothing
(8.5 vs 8.4). The "before" at the audit's code (e3390778, same scene) read 9.4 ms / 107 fps (game 7.4, render 7.8) but ran cold
(first run after its build, load 14.7 s vs 6 s): the game thread is equal, the render-thread gain is not trusted. The fire rows sit
a hair over the 8.3 ms line: the next perf work is the game thread under fire (bullets, hits, the fight), not the lights.
Suggestions (not done, the user's call): Ranchhand/Thresher/Powderkeg have no Rare effect parts (LOOT.md); the slimes' band
(18.7 s; their leap uptime); the art audit's one change first (Medium's r.Streaming.MipBias 0, r.MaxAnisotropy 8, r.SkeletalMeshLODBias 0 in `GraphicsSettingsSubsystem.cpp` QualityVariables: three lines, measure before and after), then its plan 2, the barge deck;
delete dead code: `UFightDirectorSubsystem::MayStep/NoteStep`, `FTargetFight::Steps`, `FUnpaidStepGate`,
`FUnpaidBlinkGate::MayStep` (no callers); the two Chakra Petch font faces (`Content/UI/Fonts/ChakraPetch-*.uasset`) still name
their .ttf by the old OneDrive project path (seen in a 2026-11-04 game log): check their loading policy is Inline (the bytes in the
asset) and re-point the source to `Content/UI/Fonts`, or a buyer's PC has no font.
Lessons: agents stage C++ in `Saved/Phase3Staging/<LANE>/` while Main builds and Main copies it in; the fight sim on a new game
needs `Looter.Landfall 8` and `Looter.Mission.Complete Main1` first (the flying ship pulls a teleported player back); a perf
"before" by commit and the tests to rerun after a tuning change: `Docs/Pipeline/Workflow.md` (Performance, Tests).
Still open from before Phase 3: the 2026-11-01 playtest's report (Missions 1-7, Sexton, Paid on Delivery) and items 1-7 of the
section below.

## Before Phase 3 (session end 2026-11-01 night): what was left then
2026-11-03: the user asked for a blunt core-loop audit (Phase 1 done, nothing in the game changed): `Docs/Work/2026-11-03-Audit/AUDIT.md`
(ranked problems, what to protect, the one change first, the Phase 2 plan). Phase 3 (one change at a time, the user plays each)
waits on the user approving or reordering that plan. The editor was closed and MCP down during the audit.
Evening: the user approved the order (start with kill speed; spiders may change; lower health, not raise guns) and asked for
parallel agents. Change 1 BUILT and committed (6e64f232: `UCreatureKindSettings`, crit 2.0, Unpaid head crit, bands; Enemies.md
"Kill speed"); its tests fixed after (see the next commit). Lanes 2-10 running on `Docs/Work/2026-11-03-Phase3/CONTRACT.md`
(HITS, RECOIL, FIGHTS, LOOT, CAMERA, GAMEPAD, MUZZLE, PERF; Main did 9, the fire timer). The user's worst moment is the ART
(Tripo models blurry, don't read, collisions wrong; the barge ships, terrain): an art audit (read-only) runs overnight once a
lane frees; its note `Docs/Work/2026-11-03-ArtAudit/`. **The user's overnight calls (went to bed ~20:45):** go on to changes
11 (Unpaid: one teleport gate, blink cap, one flee rule, hunched idle) and 12 (reload slaps with overshoot, a thrown magazine)
after 2-10 land; package a smoke-tested playtest build at the end; Main may retune tokens/DamageScale to the 8-12 s standing
band after the fight sim; art audit only, no model changes, no Tripo credits. Morning: three sentences + the per-change PIE
checklist (each lane's note has its line); nobody has PLAYED any of it: the user plays first, one change at a time.
State: everything the user asked for this session is built, tested, committed and pushed (last commit cb4a704e). Latest
playtest (Missions 1-7, Sexton's shop, Paid on Delivery, Tripo Reliquary + soul coal): Builds\Playtest\2026-11-01_1753\
Windows\AI_Looter_Shooter.exe (smoke test passed). The user has not reported on it yet: ask what they found first.
What's left (none started; ask the user before spending Tripo credits or starting big work):
1. Tripo models for the rest of Mains 4-7 (TRIPO_MODELS.md M45-*, M67-*): stand-ins in the game. Tripo balance 25 credits
   (a model costs 30): the user must add credits and approve. Template lanes: TRIPO_M23.md, TRIPO_M47.md.
2. PIE checks not done: BargeFall (the barge falls into Gravewind Canyon) and CageBurst scenes, the secret keyholes on the
   full map page (minimap keyhole works). Their tests pass.
3. Perf on the new set pieces not measured (MAIN67.md run order step 8): the barge deck mid-fight, the furnace hall, the
   Lamplighter's deck in the tender fight, Abel's phase 3, the SaintAdaWakes wide shot. Last numbers (before Mains 4-7):
   default view 171 fps, Ransom's Rest tour heaviest 8.26 ms (PointOverValley_FarShadow), farm 7.50 ms.
4. Side missions still tell the old Revenant premise: MAINS_2_7.md section 13 rewrites them for the Wardens story (Dead
   Preferred (was Wanted: Already Dead), Calder's Caches, The Old Fare, Knocking Below, The Last Watch, Unfinished Business,
   The Gravemother). No lane has done it.
5. Docs/Story.md: the Wardens lore for Mains 6-7 (MAINS_2_7.md section 2) not written in.
6. Small art: dark Tripo knee-brace bits under the farmhouse porch roof (seen from below), the farmhouse dormer roof slightly
   wavy. Tripo Depot, Chapel, Workshop Bright, Whitlock barn, Smithy, Watchtower made but not swapped in (need doorways cut
   round the scripted interiors); Assay and Barber fronts fitted but not hooked (they'd lose the lettered signs); Hotel
   not tried. Drover grips have no hand markers (do all 8 grips in one pass). Abel's black lantern has no Tripo swap path.
7. Later, by the user's earlier calls: voices (captions only for now), Lucky Ned's fight (area 4 or 5), the next area (on
   hold until the user says).
Known, harmless: the Mooring Ledge warning (the ship moors at the design spot), the flour-sack barricade piece left off a
road, no Encounter_Tollman yet.
How to work (lessons this session): run Creatures tests by subgroup (one 204-test batch crashes the GPU driver);
launch.ps1 -Build reopens the level clean and waits for a closing editor; perf.ps1 answers the Zen prompt; a bare test
world needs FEditorScriptExecutionGuard for an actor's own delegate handlers; agents stage C++ outside Source/ while Main
builds (main67_src pattern) or hold C++ edits during packaging; after open_clean.py, reload build_area_camps (fixed now).

## 2026-11-01 evening: Mains 4-7 + Sexton's secrets + Paid on Delivery BUILT, tests pass; PIE-checked, packaged
All of the Wardens story's Ransom's Rest mains are in (lanes MAIN45.md, MAIN67.md + CONTRACT67.md, SECRETS.md): Main 4 "A Coal
Under Ash" (chapel window hose, tender drop on ropes), Main 5 "Choke Point" (Sink intake, salt charges, turned in to Finch),
Main 6 "The Pilot Light" (the Lamplighter's sortie, boarding the barge, furnace hall, cage burst, the barge falls into
Gravewind Canyon), Main 7 "Given to the Wind" (corrupted Abel: call-backs at his post, the six dead an optional save, the
fog's three phases; FreeAbel; carry his light home; SaintAdaWakes). Sexton sells 17 secrets for silver (map keyholes);
Paid on Delivery after Main 6. Mission XP trimmed for the Pacing band (Main 2 400, 3 800, 4 1,000, 5 1,400, 6 3,000, 7
3,000, Paid on Delivery 150); the missions' share floor is now 50% (the user's pick). Tripo M23 models in (ferry, shrouded
dead, hose-cart, sandbags, board, banner; 230 credits, balance 85); farmhouse/cottage windows glow again (lit panes).
Tests: every group passes (Story, World, Bosses 39, Scenes, Missions 64, Lines, Train, Progression, Creatures in 32
subgroups (one 204-test batch crashes the GPU driver: run Creatures by subgroup), Bestiary, UI 78). Perf before Mains 4-7:
default view 171 fps; tour of Ransom's Rest heaviest PointOverValley_FarShadow 8.26 ms, farm 7.50 ms. Tools: perf.ps1
answers the Zen prompt and stops a game hung on exit after its CSV; launch.ps1 waits for a closing editor before building.
PIE-checked (demo agent): Mains 4-7, the shop and Paid on Delivery play to their turn-in cards; fixed after it: barge deck
Floor tag, sortie lift-off over the lookout tower, Finch beside the Reliquary, tender fall trace (StaticGeometryParams),
boss bar hint below the ABEL pips, interact focus ignores passing bodies (call-back hold under adds), FreeAbel camera
(SceneTalkView), Hob's hose line after ChapelHose.Pulled. Unchecked in PIE: BargeFall and CageBurst scenes, the map page's
keyholes. Builds: launch.ps1 -Build reopens the level clean first (no Save Content stop). Packaged (2026-11-01_1753);
perf on the new set pieces still to do. Known: the Mains 4-7 Tripo
models (TRIPO_MODELS.md M45-*, M67-*) not made except SM_ReliquaryWhole and SM_SoulCoal (lane TRIPO_M47, 60 credits; Tripo balance 25); Mooring Ledge warning (works).

## 2026-11-01: Wardens round 1 + Mains 2-3 BUILT, Tripo models in (Docs/Work/2026-10-31-Wardens/)
The user restarted the story (DECISIONS.md: living Lantern Wardens on the airship the Lamplighter vs Cornelius Vane, soul
coal and smog; hero Sully Graves; no grave start) and asked for a turn-in card on every mission (card, then talk).
Committed (this commit): the turn-in card; Landfall (Main 1: ship, tender, boarders, grapples, hard landing, winch,
spyglass reveal, Abel's trail and hat, Delia's porch fight; played end to end); Vane's Dredgers; Delia and the crew;
renames (skills, the Warden's Log, Wickcutter, lantern posts); creature inside-out fix (looter_creatures face_out);
Main 2 "The Ferryman's Price" and Main 3 "Under New Management" (lanes/MAIN23.md; PIE played start to turn-in, fixes:
deck Floor tag, barrier height, banner flip, spill colour, ferry fog, gate riflemen posted 6 m (PostRadius/SetPost),
radio waits for quiet, bestiary only for creatures with a page (HasPageFor)); mission XP Main 2 400, Main 3 800 (Pacing
band: everything once 7.70, story 6.99; Docs/Progression.md). Tripo models imported: 17 Landfall statics + crew/Delia/
Dredger people (IMPORT), farmhouse/barn/cottage/windmill (TRIPO_BUILDINGS: farmhouse posts, roof, fascia, deck rim on
trim boards; dusk window glow lost on farmhouse/cottage fronts), Drover 4 sets (TRIPO_PISTOLS, 20 parts, keys kept),
Gravemother in Tripo's paint with new smooth eye domes (the user: "weird bumpy eyes"). Tests: every changed group
passes (Makers.Bias and Hands.OnGun fixed: outdated assumptions). Tools: grab.ps1 takes the editor frame by title;
build_area_camps tile cache refreshes after open_clean; Hob's Keeper's Bridge word placed after the bridge marker.
Running: TRIPO-PEOPLE (Abel, Amos, Hiram, Thaddeus, Foreman, DeadMiner bodies; 180 of 220 credits; its files left out
of this commit), TRIPO-M23 (ferry, shrouded dead, hose-cart, sandbags/board, banner; cap 250, the user: "only if credits
remain"; ~315 balance before it). Tripo balance after both: about 25-65.
Next: merge those two (import, gameplay pass, tests), perf before/after (farm view +17k tris, Drover 33-35k), package
the playtest (the user said yes), then Mains 4-7 (the user: keep going overnight; MAINS_2_7.md).
Known: Mooring Ledge warning (ship moors at the design spot, works); farmhouse knee-brace bits under the porch roof, dormer roof slightly wavy;
Assay/Barber Tripo fronts fitted but not hooked (lose lettered signs); Depot/Chapel/Workshop/Smithy/Watchtower Tripo
models kept for later (need cut openings).

## 2026-10-31 night: pause menu size BUILT
The user: "ESC menu ui needs a gui option to decrease its size". Built: Settings > Interface "Pause menu size" 60-100%
(default 100%) shrinks the hub and, over the game, the settings page (the user: "Settings page too"; Pipeline Game.md pause menu "Size"). Tests PauseHub.Size, MenuNav.SettingsMenu, PauseHub, SettingsMenu, Settings pass; seen at 70% in uishots.

## 2026-10-31 night: mission tracker top-left BUILT
The user: missions and their tasks on the left, a little under the top corner; no "3/4", just the next objective; the
objective's circle bigger, the task on its right; then "lower the tracker some, make the optional objective the same size".
Built: tracker top-left (Left 36, Top 150), left-aligned, step row gone, ring 28 / check 32 px, optional rows the same size,
control hint moved to 420. Tests TrackerTopLeft, ObjectiveChecks, LootCardClear, Missions.Tracker
pass; seen in hudshots (Ransom's Rest and the island). Pipeline Game.md "HUD".

## 2026-10-31 evening: Sink dens BUILT (contract Docs/Archive/Work/2026-10-31-SinkDens/CONTRACT.md)
The user: the Sink's spiders (Main 5) crawl out of dens in the walls instead of being there; picks: every time they spawn, when
the player reaches the floor. Built: EAmbushEntrance::Den (spider crawls out, tips onto the face, climbs down, hunts), four burrows
SM_SpiderBurrow_A/_B (SinkBurrows.py, MI_BurrowThroat new), Creature.Spider.Den, the SinkFloor spawner an ambush on the floor,
hidden creatures off the minimap. Tests Spider.Den 4, Encounters.Ambush pass; PIE by log only (the PC was locked: no screenshots).
Open: see it by eye (grope, tip, grit; the skirt's texture seam on the wall; the throat colour at the lip); SpiderCreature.h is
588 lines (split when next touched).

## 2026-10-31 afternoon: orchestral score round 1 BUILT (contract Docs/Archive/Work/2026-10-31-Score/CONTRACT.md)
The user: "We need soundtracks, different music for the menu screen, introductory scenes, ransom rest will have different music
than the gilded lily". Their picks: keep music script-made (no Suno, no libraries, no composer); round 1 = main menu theme +
boss fights; style "mix by moment" (ambient/folk exploring, rock in fights, orchestral for bosses and the menu).
Built: recipes/orchestra.py (instruments), Music.Menu.Theme, the four boss themes each with a .Rage last-phase stem (Abel,
Cyrus Teague, Gravemother, Stone Sentinel), Music.Sting.BossVictory; the director plays them (Art/Sounds/README "The orchestral
score"). Imported; Looter.Audio.Director 5 pass; checked in PIE (menu theme plays; Sentinel + Rage forced, Rage starts in step).
Previews sent to the user (Saved/MusicPreview). Waiting on: the user's ear on all of it. Next rounds (not started): intro
scenes (Music.Intro.*, Music.BossIntro.*), Ransom's Rest by area, combat in rock, the Lily (on hold). Open: orchestra.py is
1.6k lines (split by section when next touched); the choir's realism is the least sure part.

## 2026-10-29 day: UI round 3 BUILT (contract Docs/Archive/Work/2026-10-29-UI3Build/CONTRACT.md)
The user picked from six concepts: pause/settings = mix of 1 Ledger & Brass + 3 Halo Reticle, main menu = 3 with a new logo
(two logo rounds; picked 6 Spur Rowel), level-up = 5 Signal Glass. All built, photographed with the new `Tools\uishots.ps1`
and compared with the concepts; notes Pipeline/Game.md "UI round 3". The user's two asks mid-job: the rifle's first-person
hold 3 cm nearer (WeaponHipHold 50 -> 47 cm ahead), the E / HOLD E caps centred (and hollow again: drawn filled since the
tag card). Tests Looter.UI 64, Settings 11, FPHands 6, Player.Hands 13, Inventory 10, Menu 3 pass.
- Gun card after (the user's asks): the maker badge bigger (15 px words), no "Maker · Kind" line under the name, the
  element as the stat list's last row ("Normal" without one; was a gem by the badge), no Zoom row. Tests UI.GunCard 3, WorldCard 2, Inventory 10 pass.
  Then Borderlands' element look: element rows in its colour ("Burn dmg/sec", "Chance to burn"; salt, smite alike), its
  bullet line coloured and named, its glyph and the gun's kind (rarity colour) by the maker badge. Tests UI.GunCard 3, WorldCard 2 pass.
  Then: no maker trait line (only special traits on the card), a gap before the element rows, the maker at the top over
  the name with no box. Tests UI.GunCard 3, WorldCard 2, Inventory 10 pass.
  Then element glyphs picked from four concepts each: Ember Hearth, Soul-light Sunburst, Grave-salt Salt Cluster, Normal
  Empty Ring (Docs/Archive/Work/2026-10-29-Embers). Tests UI.GunCard 3 pass.
- Elements renamed for the player (the user's picks from options): Cinder (Ember), Hallow (Soul-light), Wardbreak (Grave-salt);
  code names unchanged (Pipeline Gunplay.md "Names").
- Two new elements BUILT (the user's picks: Thunder = a jolt that staggers and arcs to the living, Rot = a growing rot that
  spreads from the dying; Pipeline Gunplay.md "Thunder and Rot"): rules, statuses, damage types, cvars, sounds, CSV parts,
  card rows and glyphs (Bolt, Skull). Wardbreak recoloured teal with a Cracked Ward glyph, Thunder deep blue (the user's
  picks). Looks (FX, finishes) by lane F: see the commit. Not yet: the ward nova's element list (ShieldRules::NovaElementFor)
  still rolls only the first three; element finishes reuse frosted (Thunder) and scorched (Rot) patterns.
- Card: a plain gun's row reads just "Normal". Rifle hip hold 43 cm ahead (the user: 4 cm back). The AR's left hand: the
  user keeps it cupped under the trigger housing (fit the fingers onto the loop, lift the gun so it shows), and asked
  whether a different hands approach would stop per-gun tuning (answered with options; see the next entry when built).
- Hands rework IN PROGRESS (the user's pick: grip spots on the parts): contract Docs/Work/2026-10-29-GripSpots/CONTRACT.md
  (marker sockets SOCKET_<Hold|Rack|Reload>_<L|R>_<Pose>, a pose library, the game reaching the arms to them, a Blender
  preview sheet; the old tables stay as the fallback until every part is marked). Lanes A (Blender), B (poses), C (C++)
  started 2026-10-29 evening; then Bullpup markers (AR cup under the trigger housing, visible), then the other guns.
- Main's calls (undo any): from the main menu the reticle row says BACK and the footer Back chip is hidden (the row was an
  empty band); the RESUME wash runs to the right bracket; slider end labels 5 px lower; arrowheads point out (c5's picture).
- Not seen by the user in game yet. Next: the user's look, then the rest of round 2's list (HUD mission popup, bestiary
  rows, vendor, bench, boards, Lost Loot) in this language; playtest Friday 2026-10-30 (don't package yet).

## 2026-10-29 morning (the user awake briefly; contract Docs/Work/2026-10-29-Morning/CONTRACT.md)
The user's answers: per-kind crosshairs yes, colour picked in Settings; the Sonniss GDC 2026 bundle yes (downloaded, 14 GB
in Saved/SoundPacks/Sonniss/2026: no gunshots in it; the user then said no to 2018 / 2021-23, which have guns); next = the
open issues first, then UI round 2. Built and pushed (bba515e8 + the gold fix): crosshair per kind (seen in PIE: SMG,
shotgun ring, sniper hairs; colour row in Settings > Interface), LMG climb cap + aimed sway fix, find crows low with one
perched, under-water finds glow, capsule hits take the nearest bone's zone, aim-in/out cloth + sniper bolt latch + SMG/LMG
mech clacks from the 2026 bundle. Not checked by hand: the LMG burst, the crows from the farm yard, the tarn glow, by-ear sounds.
- Next: UI round 2 (Slate & Salt to the main and pause menus, HUD popups, bestiary rows, then vendor, bench, boards, Lost
  Loot); the user playtests Friday 2026-10-30 (don't package yet).

## 2026-10-29 night: core gameplay job BUILT (the user picked all four: guns feel different, enemy hit reactions,
smarter bigger fights, things to find off the path; asleep, answers in Docs/Work/2026-10-28-Core/CONTRACT.md)
All pushed (122556fd round 1, 58e7faa2 round 2, 597d0d6c fixes). Notes: Pipeline/Gunplay.md "Gun identity and hit feel",
Enemies.md "Fight: pack brains", Levels.md "Finds". Play-check: Docs/Work/2026-10-28-Core/RESULTS.md. Nobody has played it yet.
- Guns: per-kind kick, view shakes, heft and move speed (+-5%), reload beats, brass, punch and aim-in sounds (script-made).
  `Looter.Gunfeel.Identity 0` = the old feel.
- Hits: zones, hit weight, side, poise staggers, overkill; spider leg buckle/limp (looks only), Unpaid whiplash, arm flings,
  hats knocked off; slime air knock and pops; kill styles; ten hit sounds. Staggers need a creature in a fight (an idle one
  only flinches; the demo's "never staggers" was that). `Looter.Hits.Test Body 30` on a chasing Unpaid logs "heavy (..%)".
- Fights: fight director (melee gaps by kind, player read, close-call ease, camping punished), pack leaders and leader-down
  falter, slime split and dodge, Unpaid leader-only rally, fade-back, dais hold; Saint Ada's Yard arena with a fog gate,
  Barn Doors, leaders in most fights. Sim: 6 Unpaid kill a still player in ~10 s; strafing ends at 63% lowest health.
  `Looter.Fight.Sim` / `Looter.Fight.Log`.
- Finds: 15 off the path with crows and glints, FOUND banner, map counts (total and per region), Pike's cache, the tarn
  lockbox, the bolt-hole, the Ranger Post, the miser's grave, the Tollman; named guns Still Water, Bench Mark, Ferryman's Due.
- Perf after (tutorial island default, Medium): 6.1 ms p95 6.5 (before 6.2 / 7.6); 8 Unpaid horde 7.0 p95 8.1 (before 7.8 / 11.4).
- Decided unattended (undo any): CONTRACT.md "Main's calls overnight" (heavy-gun kick caps kept; spiders' fight balance
  untouched, the user's 10-28 "leave them"; arena at the chapel yard only; Borderlands-normal targets; crows+glint tells; the
  Tollman a find); Regular Unpaid silver chance 55% -> 40% (Progression.md 10-21 said 40%); FOUND banner moved to 78% down
  (the card over loot covered it); the SMG has no punch sound.
- Morning asks answered (see 2026-10-29 morning above).
- Open (not built or not checked): launcher not tried; Hob's caption at the town-gate leader's death; Sink/Webwood spider additions; news when a find is found;
  E6 grenade tins; Pike's footlocker body (needs a C++ flag); art: laid rope, card+knife, bigger cache tag; FightPace
  uptimes need fitting to play. Files over 500 lines to split: CreatureBase.h 694, GunfeelSoundTests 636, WeaponBase.h 616,
  WeaponBrass.cpp ~565, GunfeelSoundTuning.cpp 511, SpiderCreatureHurt.cpp 508, UnpaidCreature.h 506.

## 2026-10-28 evening: UI character, round 1 (the user at work; playtest Friday 2026-10-30; "don't package yet")
Committed and pushed. Contract and mockups: Docs/Work/2026-10-28-UICharacter/ (kept open for round 2). Notes: Pipeline/Game.md
"The gun card". The user picked the look "Wide Sweep" (mockup/wide.png) after four rounds; saw it in game only as screenshots.
- Round 2 built (later the same evening, after ~10 mockup rounds the user steered): the tag card (Borderlands-like list,
  Slate & Salt, level gem, keys under it, like-with-like compare). The user's pick: Slate & Salt for the whole UI (next rounds).
- Built: the gun card kit (UI/Cards), the card over a looked-at gun with hollow keys, the in-hand card on the HUD's right, the
  four makers' card looks and open sounds (UI.Card.*), the inventory back to two columns (backpack right), card on screen clamp.
- Not heard by ear: the card sounds. Bars use the all-kinds rating, so shotgun fire rate/magazine bars sit low (maybe per-kind later).
- Next (round 2, the user's list): the same look and motion on the main and pause menus, the HUD popups (level-up, mission,
  pickups), the bestiary rows; then the other screens (vendor, bench, boards, Lost Loot) still in the old boxy panels.
- The Gilded Lily: design only, H2 The Spine picked; not to build until the user says.

## 2026-10-28 afternoon (the user at work; their picks: Unpaid balance, maker sounds, packaged fixes, cleanups)
All pushed (last: 683405f9 + this handoff). Nothing played by the user yet. Contract: Docs/Work/2026-10-28-Polish2/CONTRACT.md.
- Unpaid (play-checked by DEMO each round, numbers in Pipeline/Enemies.md):
  - LungeHitRadius 85 -> 100 (at 85 no lunge ever landed on a still player); FlankChance 0.6 -> 0.4.
  - Sight: no turn or lunge through walls; a blind Unpaid lets its turn go and steps to a spot that sees the player
    (reaches the chapel dais now); one blink per player per 1.2 s across the pack, step clock jittered 0-1.5 s;
    no attack 0.6 s after a blink lands (ArrivalHold). Standing still on the nave floor: dead in 49 s (was 12).
  - Open: the dais still hardest (landings at 1.5 m; a still player dies in 23 s). Idea: 0.8 s hold for 1.5 m landings.
    UnpaidCreature.h is 544 lines (over ~500): split when next touched.
- Gun maker sounds: MakerSoundFlavor (Roundhouse heavy, Saint Maud ring, Windfall rattle, Meridian clean); heard in PIE
  via au.Debug only, not by ear. Watch for clipping on a Roundhouse/Saint Maud revolver indoors.
- Paper marker: decal/sheet mission items (the gate poster) get an orange ring-diamond while lit (PASS in PIE).
- Maker marks were mirrored on both sides: fixed (MARK_CODE); T_GunMakerMarks never streams. Third person can't show a mark
  (camera sees the gun end-on; accepted since 10-22).
- Shotgun trigger: Grip.Report 2.23 cm (test now 3 cm, indexed). The shotgun's support hand reads a bit off (cup box sits on
  the housing front, fingers not wrapping): the user to look.
- Packaged game: r.PSOPrecache.GlobalShaders=0 (22 compute PSO errors gone on the old build with -dpcvars); not yet in a
  packaged build.
- Part swapping waits for Ozias after the Lily (the user): Ransom Farm's and the tutorial island's benches removed.
- Plan.md refreshed (phases 9-11 built); the user: stay on Ransom's Rest, no tutorial-island work.
- The Gilded Lily DESIGN only (the user: "I don't want to make the gilded lily yet"): Docs/Areas/GildedLily.md. A
  Titanic-like liner anchored at sea (not an airship), bigger than RR, town first and ship last, Ned's rigged card game,
  slots, Ozias, hostile boarding. Concept maps A-H2 in Art/Concepts/Areas/GildedLily/; **the user picked H2 The Spine**
  (Hulltown in Crescent Bay: a beached wreck, the Sovereign, across the bay). Still not to build until the user says.
- Element answer given to the user: Ember, Soul-light, Grave-salt; any maker; Saint Maud element parts x3.

## 2026-10-28 day (the user at work, no playtest from them)
The user's picks: "Fix the unpaid not hitting but leave the spiders where they are. Fix Sniper framing" (a193ab75), then all four
of gallery fix, gun looks, melee hand bugs, small cleanups (76c191db). All pushed; nothing seen in play by the user yet.
- Unpaid lunge commits its last 0.3 s through the hit slow; the Unpaid take that slow at most once per 0.5 s. Spiders untouched.
- Sniper hip: HipUp -1.5, ArmsScale 0.9 (like the LMG).
- Unpaid gallery: shot from within 8 m past their ground's edge, the pack fights there and the one hit blinks in (Pipeline/Enemies.md).
- No rocket ammo unless a launcher is carried or in the drop.
- Gun looks: camo in two close tones of the gun's own paint; pale enamel (Saint Maud) darker with dark chipped edges (M_Gun 822
  instructions, was 800); AR Reflex dot a 4 mm disc + 1.1 cm ring (the Tripo set 3 sight is the one in the game).
- Melee hands: the overnight Crane/Odessa grabs predate f2423b2d, which fixed both; new tests only.
- Split for ~500 lines: UnpaidCreatureLook.cpp, UnpaidCreatureArmGuard.cpp, UnpaidBodyParts.h.
- Open, the user's call: the shotgun trigger finger can't reach (geometry: the Ranchhand's trigger is ~13.4 cm ahead of
  SOCKET_Grip, ~20 hand-cm drawn vs ~10.6 reach). Fix = Ranchhand.py `trigger_group` ~5 cm back, best before the shotgun's
  Tripo modelling (its views show today's trigger). Side note: grip tests check guns at 1.1, not the drawn 1.32.
- Not built: spider balance (the user: leave them), the old builds still unplayed.
- Play-checks (Demo agent, PIE, God, aimbot): sniper hip PASS (Saved/Screenshots/Tools/sniper_hip2.png), Saint Maud and camo
  PASS (maud_shotgun2.png, camo1-3.png), Reflex dot PASS (ads_reflex.png). Unpaid hits landing: ~25% of attacks started
  (overnight 3.4%), rough.
- **Unpaid gallery: the user said "Dig in" after two failed play-checks. Found and fixed (Pipeline/Enemies.md): every
  phase-step and flank had been refused game-wide by the scatter's PCG volume answering the landing spot's sight trace.**
  Main's PIE check after: past the west fence the yard's pack blinked over (19 steps in ~10 s). The Unpaid are now much more
  aggressive than any playtest saw (blinks and flanks work for the first time): the user should feel it out. The press path
  itself (a hit from past the edge) is covered by `.Kit.Press`; no shot of Main's landed in that PIE check.
- Shotgun trigger moved back at the user's go-ahead (Pipeline/Game.md); body views redrawn; not seen in play.
- History of the two failures:
  - Run 1: the press never blinked anyone in. Cause: a fight begun in the yard pressed the whole pack, so a later shot from
    past the fence owed nothing, and an Unpaid waiting its turn may never step. Fixed in the last commit (hit from past its own
    edge always owes the blink; `WaitGoesNowhere`, `Looter.Unpaid.BlindStep`).
  - Run 2: still 0 phase-steps. "is pressed past its edge" fired 202 times (every hit: log spam, Verbose only), the pack
    lined up 0.5-1.8 m inside the west fence 7-8 m from the player and never stepped; 2-4 walked out through gaps. Likely
    `FindPhaseSpot` failing silently (retries every 0.75 s, no log): suspects the landing spot's sight trace, the Obstacle-tag
    rule, or Search state. Next: a log line on its failure.
  - The chapel nave is INSIDE the yard's ground (fence polygon x 5600-7800, y -3600..-200 encloses it). Standing in the nave,
    Unpaid walked in through the doors in ones and twos, the rest along the outside walls 3.4-5.5 m off (too near to step:
    MinStep 3 + NearestToTarget 2.5). No line at the back door this time.

## State at the session swap (2026-10-28 morning)
All committed and pushed (last: the handoff commit). Test build `Builds\Playtest\2026-10-28_0250` (1.45 GB, smoke PASS;
`Play Latest Build.bat`) has everything below. **The user hasn't played it yet: ask first.** Older builds (10-27_1006,
_1711, _1818, 10-23_0658) were never reported played either.

Done 2026-10-27 evening to 10-28 overnight (details in git; the user's orders and calls kept here):
- First person (the user's playtest asks):
  - GunScale 1.2 in first person only (Looter.FPHands.GunScale, per kind .<Kind>.GunScale).
  - Guns pushed forward; forearms steeper (72/78 degrees, ForearmBack 0.5).
  - The support hand cups palm-up under each gun's middle (FirstPersonHandsCup.cpp, the user's rule: "cup beneath the trigger housing... on the magazine of the LMG... keep this in mind for all weapons").
  - Cup fitted at runtime to whatever parts the gun has (FirstPersonHandsCupFit.cpp: rays on the parts' collision, Looter.Hands.CupFit.*).
  - LMG: SupportAhead 3, HipAhead -2, HipUp -3, ArmsScale 0.9 (new per-kind Looter.FPHands.<Kind>.ArmsScale). The user tuned it by eye; the LMG's support hand now sits below the frame, which is accepted.
- Tripo (the user models on a PAID plan from Main's views; Main checks, fixes and fits; method in Docs/Pipeline/Assets.md "Tripo parts"; memory tripo-gun-parts):
  - AR: all 4 maker sets in the game (Art/Models/Weapons/Tripo/parts_set1-4.py merged by Bullpup.py; LOOTER_BULLPUP_TRIPO=0 scripted).
  - Melee tools: all 5 in the game (Art/Models/Weapons/Melee/parts_tripo.py; LOOTER_MELEE_TRIPO=0).
  - Chest tiers: 5 in the game (Art/Models/Loot/ChestTiers.py).
  - Shotgun views READY for the user to model: Art/Concepts/Weapons/Shotgun/<Slot>_<Key>/upload (24 parts, 4 sets: Saint Maud Standard/Field/Crown/Tube6/Flip/Skeleton, Roundhouse Armored/Shielded/Brake/Drum20/Prism/Mule, Windfall Salvaged/Short/Duckbill/Box8/GhostRing/Folding, Meridian Tactical/Slug/Choke/Tube6Window/Variable/Thumbhole; the pump has no views).
  - The user's rule: each set is a genuinely new design, never a recolour.
  - Heroes: Tripo DROPPED by the user (clipping, z-fighting, unfilled parts): "expand on what you already have and get it perfect". Views stay in Art/Concepts/Heroes (Tools/Blender/hero_views.py).
  - The Tripo API trial was dropped (no API page on the user's account); the user models by hand.
- Gun pattern variety (the user's call: rarer = fancier, subtle and worn-in):
  - WeaponModelPattern.cpp rolls from the seed, rarity and maker into custom data 29-35.
  - M_Gun's pattern layer: 1 grime, 2 worn edges, 3 stripes, 4 chevrons, 5 camo, 6 stencil, 7 clean bold.
  - Element stains on every part (Looter.Weapons.ElementLook.Spread 0.25). M_Gun is 800 instructions, up from 497.
  - Force a pattern with Looter.Weapons.Pattern.Force.
- Loot chest tiers 1-5 (the user: "the better the chest the better the loot"; "steady climb"):
  - T1 ammo + 35% one gun; T2 one gun; T3 1-2 guns uncommon+; T4 two rare+; T5 three epic+ past the early cap (~70% legendary).
  - The "escalating show" (ChestShow.cpp): T1-2 latches pop; T3 the lock spins green; T4 orb pulse and gold trim sweep; T5 crest unfold, burst and rising tone, then a loot fountain thrown away from the opener. Eight Loot.Chest.* cues.
  - Ransom's Rest's 13 chests tiered by Tools/Unreal/chest_tiers.py + set_chest_tiers.py. Ruth's bluff cache is T2 (the swap lesson).
  - The Supply Crate and Strongbox models are replaced by the tiers; Coffin, Grave, Mailbox and Footlocker are unchanged.
- Heroes "perfect", the user's ask:
  - Fit pass on all five (Tools/Blender/fit_audit.py, looter_fit.py).
  - Cloth springs kept off the limbs (BodySwing: back boot, hems, hands).
  - TurnCarry: cloth turns with the body.
  - The hero select orbits its camera and lights round a still hero, a turntable (the user: "cloth acts unnaturally" when panned).
- Melee:
  - Crane's hand wraps the gaff (bRoundHandle).
  - Odessa's fan winds up 45 cm from the eye.
- Other: sniper scope glass (GunScopeLens). Tools/zenwait.ps1: launch.ps1 presses Yes on the editor's Zen "keep waiting" box (the user saw it).
- Perf: RR Medium 7.6 ms (132 fps), p95 7.9 (target 8.3).
- Overnight playtest: Docs/Work/2026-10-28-Overnight/PLAYTEST.md.

Open (ask the user first; tap choices):
1. Feel calls from the overnight playtest: b-f BUILT 2026-10-28 (above); a) spiders left as they are by the user's call.
   - a) Spider packs kill in 3-4 s while the Unpaid land almost nothing. Suggested: spider melee tokens 1 or bite cadence 1.6 s while 2+ bite; the Unpaid lunge commits its last 0.3 s, hit-slow at most every 0.5 s.
   - b) The sniper at the hip hides a third of the screen (per-kind framing like the LMG).
   - c) The Unpaid wait at their ground's edge, a shooting gallery (phase-step in, or cinders).
   - d) Saint Maud's clean finish reads flat, and camo on red/cream reads as cow hide.
   - e) The AR reflex dot is tiny (2x, a ring, or cyan).
   - f) No rocket ammo while no launcher is carried.
2. The user models the shotgun's 24 parts in Tripo; then fit them like the AR (one agent per set, the cup fit covers the hands).
3. Still open from the 10-27 notes:
   - Note 15 (stairs invisible), note 10 (terrain), note 30 (floating fences): the user's screenshots are needed, asked 10-27 with no answer.
   - The Unreal Engine logo download: the user asked Main to do it. The built-in browser's policy check failed 3x. Retry, naming the file, source and size first.
   - Not yet checked by eye in play: the Niagara Cinder fire, the boss attack rules.
4. Older, small (the 10-23 list, archived): the sniper ground ring (ask: drop it), Tilly's silhouette, gunfeel cvar tuning by the user, decal mission items can't wear the highlight, bounty variety after Main 5, ~66 D3D12 PSO errors in the packaged game, the shotgun trigger finger, third person never shows a maker mark.

How the work ran 10-27/28 (keep doing it):
- Main orchestrates up to 8 agents on disjoint files.
- Blender agents run one job each (16 GB RAM): more RAM was suggested to the user as the biggest speed-up.
- One editor lane: agents SendMessage Main before a build or long PIE use, and Main hands the editor over.
- Main imports (Looter.ImportModels <ABSOLUTE path>), runs only the touched tests, commits and pushes after each step.
Lessons:
- A "Save Content" dialog blocks close.ps1. Read it with Tools\winshot.ps1, then answer with Tools\dialogclick.ps1 (Don't Save at 493 502). Reload the level clean first: Tools/Unreal/open_clean.py.
- A build compiles every agent's in-progress files: warn them first.
- git "Permission denied" on .git/objects passes: retry after a few seconds.
- Check what exists before an art job: render existing models, don't redraw them (the user caught a redraw; memory reuse-existing-assets-first).
- An editor test run stops another agent's PIE: tell it.

The user's calls this stretch:
- Reports in 3 sentences.
- "Run multiple agents as needed."
- "Don't worry about checking or taking screenshots" (for small framing tweaks the user watches themself).
- Fast mode isn't worth it here.
- The user may be on Remote Control: put questions in the closing message too.

## Direction (the user's answers, grill session 2026-10-21) - overrides "next" below

- **Core-loop freeze:** no new regions, heroes or systems (the *Lily* stays on hold) until shooting, fights, loot and
  pacing are fun in Ransom's Rest. The user played it through after the heroes: "something's off" in gunplay, pacing,
  enemies, loot and art.
  - Gunplay: enemy hit reactions, gun sound and kick, guns feel alike (not hit confirmation).
  - Enemies: too easy, dumb AI, fights too small (variety not picked as the problem).
  - Loot: all four wanted: more gun kinds, part effects that change behaviour, elements, bigger drop moments.
  - Art: characters, samey world, weak VFX (not lighting).
  - Pacing: story slows it down, nothing to find off the path, fetch-quest missions (walking not picked).
  - Story delivery: cut dialogue roughly in half; most lines as short captions while walking or fighting
    (Borderlands-radio style); long scenes only at big beats. Still no voice acting.
- **Freeze ends when** the user and their one playtester friend each play Ransom's Rest start to finish on a packaged
  build and both want to replay it with another hero.
- **Sounds:** paid/free royalty-free libraries licensed for games are now allowed for guns and impacts (e.g. Sonniss
  GDC bundle), processed through the script pipeline; script-made stays the rule elsewhere.
- **Release plan:** single-player at launch (co-op not planned). 1.0 keeps 15-18 regions. A free Steam demo once the
  game is 60-70% built, within a year (~Oct 2027). Store page ("Coming Soon") goes up once the core is fun, with a
  Ransom's Rest trailer. Steam's AI-content survey answered plainly (made with AI tools, nothing generated live).
- Playtesters: the user and one friend.

## 2026-10-29 night: elements matter BUILT, AR grip spots IN
- Elements matter (Pipeline Gunplay.md "Elements matter", Enemies.md "The Stone Sentinel"): all seven lanes built, tests
  pass (Elements 15, Creatures Touched/Sentinel/StatusPose/TierLook, UI StatusIcons/Tier/HitMarker/DamageNumbers, Shield 11,
  Loot.TouchedLean, Vfx.Element 3, BulletMarks 4, Weapons.ElementLook 2, Bestiary 5). Imported: sounds (38), the Sentinel
  stand-in, the 8x8 mark atlas + M_BulletMark; 4 Sentinels placed. Not yet looked at in play: reaction poses, trails, marks,
  typed ward colours. Files over 500 lines to split later: CreatureBase.h, CreatureDevCommands.cpp, WeaponFX.h,
  FirstPersonHandsComponent.cpp, FirstPersonHandsReload.cpp, fp_hands_parts_sheet.py.
- AR grip spots: every Bullpup body/magazine marked (Art/Models/Weapons/Bullpup.py spot tables); left hand cupped at the
  bottom of the trigger housing (the user's pick), hold 43, 23, -27.5 (the user: back 4, down 4); in PIE palm gap 0.15,
  all fingers touching. The other guns get spots with their new parts.
- Tripo remakes in progress by the user: Stone Sentinel and the Unpaid (prompts in this session; Enemies.md).

## Overnight 2026-10-29/30 (the user went to bed; their answers)
- In flight: Tripo remakes (Sentinel, Unpaid, Meadow Wolf, Spitter, Gravemother: contract Docs/Archive/Work/2026-10-29-TripoCreatures),
  effect sprite polish (built? see commits), file splits, the tuning pass (the user: element bullets x1.75/x0.5 with status
  x2/x0.33, ammo by class, compounding +9% a level, bosses ignore body resists keeping their story immunity, plus the
  analyst's fixes), three boss legendaries at 10% per kill (Gravesilk, Company Scrip, Lamplighter), the ESC pause hub
  (Resume top, Inventory, Missions, Bestiary, Embers, Settings, Save & Quit bottom; Save & Quit removed from Settings),
  thicker player ward bar (as the health bar) and enemy bars (10 px).
- The user's overnight calls: a Tripo model that can't reach the bar goes in ANYWAY (no fallback to the old one); start the
  Unpaid attack variety + failed-lunge shriek (the user's prompt, in this session's transcript: diagnose the lunge-only
  fights, a weighted move mix with tells, the AAA shriek with rings/wisps/embers/frost/HUD feedback, perf with 3 at once)
  once the new Unpaid is in; before packaging run perf (perf.ps1) and FIX anything under 120 fps; then package a
  Development playtest build (Tools\package.ps1 -CloseEditor -SmokeTest) and commit/push as each piece lands.
- Done overnight: d6e5e04f (pause hub, bars, legendaries, tuning, sprite polish, splits, Tripo Unpaid + Meadow Wolf);
  0f02842a (Unpaid fights: move mix Lunge/Rake/PhaseStrike/Feint/LowSwipe picked by range, the cause of lunge-only fights was
  the 480 cm ring sitting inside lunge range; the failed-lunge shriek; Tripo Spitter (6 legs rebuilt to 8) and Gravemother
  (6 legs, middle pair doubled) imported; the ten stale tests fixed). Test `Looter.Creatures.Unpaid.Shriek` renamed
  `.Shriek.SlowRing` (its name was also a group, so it never ran): rerun after the next build.
- Then (2026-10-30 early): play-check passed (Variety 1 mix Rake 29 / Lunge 26 / LowSwipe 20 / PhaseStrike 13 / Feint 12 %,
  Variety 0 was 93% lunges; shriek light 70/150 -> 25/50 cd and its cone stops short of the camera, f2bf413c; chest ring
  dropped, e0a73e5e; pause hub, bars, models level with 8 legs: pass). Perf (Performance.md rows 10-30): base 6.2 ms, horde
  6.4, horde + shrieks 6.5, Gravemother 6.4, tour 37 views all <= 8.2 ms. Shriek file split (29b31a90). Tripo Stone Sentinel
  in game (6cd8be48; fists at the sides, slam poses cut). Playtest package `Builds/Playtest/2026-10-30_0215` (1.47 GB, smoke menu + Ransom's Rest pass; `Play Latest Build.bat` runs it).
- Open, for the user: the Unpaid's hat reads near-white (darker?); the Spitter's sac looks plain; enemy nameplates overlap
  the boss bar's title near a boss; Rake tally counts both swipes; the Gravemother spawn hitch was an editor first-use (none
  in the standalone capture). Tell them: Map row added to the hub; named guns can't be sold; player health compounds +9% a
  level too; spider leg rebuilds (Spitter 6 -> 8, Gravemother middle pair doubled); grass doesn't bend in the shriek.
- TripoCreatures contract archived; GripSpots stays in Docs/Work (code points at it; the other guns use it later).

## 2026-10-30 day (the user awake; "fix the issues and ask questions to continue")
- Fixed (9502f1db): grass blows flat in the Unpaid's shriek (`UFoliageBlastSubsystem`, MPC_Wind Blast*, M_WorldFoliage; M_Smoke
  resaved), enemy name tags fade under a shown boss bar, the Rake tally counts a landed rake once.
- The user's calls (574c3b0d): player max health +6% a level (enemies/guns/wards stay +9%); boss legendaries (signature) sell
  behind a one-of-a-kind confirm, story heirlooms (Heirloom, Bench Mark, Ferryman's Due, Still Water) never; Sentinel keeps
  fists at its sides; next = the loot and pacing pass. Art: Unpaid hat worn felt brown, Spitter venom sac (veins, glossy
  membrane); both imported. Screens of the five Tripo creatures sent (Saved/Screenshots/Tools/show_*.png).
- Loot and pacing pass (analysis: the Planning agent's model of RR 1-7; the user: boss uniques STAY 10%, early legendary cap
  STAYS 1/300, Gravemother stays hard, Ferryman's Due stays an heirloom, "raise the difficulty of Abel"): Abel's hits x1.7
  (was 1.25), the Foreman 380 base health (was 190), the quarry track patrol after Main 4 at 4-5 (was Main 3 at 5-6; level
  actor and DA_Area_RansomsRest updated), gun of the day every 30 min of play (was 60), `Looter.Economy.Balance` rewritten to
  today's drop counts. Not done (analysis, needs the user): more offer odds change (.45/.45/.10).
  DestroyAll <class> in PIE wipes far more than the class (the grass went): never use it; Looter.SpawnCreature + slomo 0.02 +
  play.py goto <name> <cm> [yaw] frames a creature for a grab.
- The user's playtest notes, one at a time ("so the quality gets better with each iteration"): 1. riding the lift down put
  the player back at the top: fixed (fall recovery forgets a held player's safe spot; Pipeline Bugs.md), played down and up in PIE.
  2. the cage clipped the mountain near the top: the lift head's footprint circle levelled the end of the lift's cutting; footprint
  dropped (area_shape.py), the cutting 0.5 m deeper and 11 m wide (layout liftCut, liftHeadSlot); terrain rebuilt. 3. regen slower and
  only out of combat, a way to heal in a fight: built (Pipeline Game.md "Healing"): regen 3.5%/s after the fight, boss fights'
  kills leave motes half the time, the soul flask (H; 3 charges, refilled by kills and respawn). Played in PIE 1P/3P; the user
  hasn't tried it.
- Playtest package `Builds/Playtest/2026-10-30_0828` (1.47 GB, everything above; smoke menu + Ransom's Rest pass; `Play Latest
  Build.bat` runs it). Not seen in play yet: the grass blast, the tag fade, the harder Abel and Foreman.

## Overnight 2026-10-31 (the user asleep; their asks and answers, in their words where given)
In flight at bedtime: the pause menu E2 "Big Dial" hub + the full-page Settings (contract Docs/Archive/Work/2026-10-30-PauseMenu/CONTRACT.md),
wards like guns (ward card in the gun card's style in the world and the loadout, ward rows like gun rows, spare wards in the
backpack sharing the 12 slots with guns; worn ward stays left), Ranger Post Three removed (chest + stone steps by the wake-up grave;
not committed yet: layout, build_area_finds.py docstring, FindLevelTests 14 finds, the level). Then, the user's overnight asks:
1. Spider and Unpaid models "didn't import right": textures/colours wrong AND rig deforming wrong (the user's picks); their
   animations need improving.
2. Spider and Unpaid AI "lackluster": the user's pick is AMBUSH (spiders drop from walls/ledges, Unpaid rise from the ground
   near the player).
3. Stone Sentinel: animations weak, not a threat: harder to kill, and BOTH a hurled boulder at range and a ground-slam
   shockwave up close (the user's pick), besides its melee.
4. Gun slots: 4 equipped. "2 slots available at the start of the game, 3rd slot available at level 3, and fourth slot
   available after finishing all of Ransom's Rest's main missions."
5. Package a playtest build at the end (as every overnight), smoke-tested; handoff and doc index current.
STATUS (morning 2026-10-31): ALL BUILT, tested, checked in PIE, committed. Wards (930a7070). Pause menu E2 hub + full-page
Settings (both modes; uishots pausehub2), Stone Sentinel (1080 health at L1, boulder + shockwave + open window; impact dust kept
3 m off the camera), spider (Tripo look, legs keep their bends, gait capped at 0.9 reach, 4-stage bite, Lurk ambush: GoatSteps,
QuarryWalls, WebwoodTrees), Unpaid (Tripo paint, weights, lurch gait, RiseAround surround rise: Churchyard, MiserPlot, new
FurrowDead; punishes a reload), gun slots (2 / L3 / all 9 RR mains; banner, key 4, locked rows). Docs: Game.md, Enemies.md,
Progression.md, CODEMAP. Not seen in play by the user: all of it. Open: the Sentinel XP stays 45 for twice the health (agent
suggests ~70; the user's call); UI.GunSlotUnlocked cue unwritten (banner uses UI.NotchMilestone); no time of day, so the
Unpaid ambushes are active day and night; a safe zone rejects every rise spot (by design); the editor sat at ~10 GB with
memory-pressure warnings after many builds (restart it every few builds). PIE fps reads 12-19 when the editor is unfocused
(its background throttle), not a game cost: focus it before reading fps.
Playtest package Builds/Playtest/2026-10-31_0318 (1.48 GB; smoke menu + Ransom's Rest pass; marked installed: saves and settings
in %LOCALAPPDATA%AI_Looter_ShooterSaved, the user's Session1 and settings carried over). `Play Latest Build.bat` runs it.

## Queue (the user's picks, 2026-10-29 night)
1. The AR's grip spots (in progress), 2. Make elements matter (Docs/Archive/Work/2026-10-29-NewElements/ELEMENT_DESIGN.md:
typed wards, bodies, immunities, Touched variants, Stone Sentinel, x2 / x0.33; plus element feedback: firing looks,
status reactions, status icons + callouts, hit marks + bigger deaths, all four the user's picks), 3. The other six guns' grip spots: placed as each gun's NEW parts are fitted (the user is modelling new parts; marking
today's parts would be done twice), the old tables stay as their fallback until then, 4. Loot and pacing pass (play 1-7, drops, XP, money, difficulty, unique legendaries).
Offered and not picked: real models/moves for the placeholder enemies; a performance pass (70-88 fps in the editor).

Terrain, the user's call (2026-10-30), built the NEXT time terrain is touched (before that job's own change): (a) rebuild only
what changed: the generator rebuilds and exports only the tiles a layout edit reaches and repaints only that patch of the
macro/scatter maps (a small edit went ~45 min: 6 min shape, ~40 min full macro repaint, 10 min import of all 46 meshes);
(b) an erosion pass in area_shape.py (our own numpy: hydraulic + thermal after the features, sparing pads/roads/exact
features; flow and sediment maps to feed the macro colours). No new tool (Gaea, World Machine, Houdini were offered;
not picked). Until then a shape-only tweak can run --computed without --macro.

## Suggestions for the user (Main's; none built without a go-ahead)
- A decal-friendly objective highlight or a small world marker for flat mission items (posters, papers).
- More bounty variety before Main 5: open the Webwood arena after Main 4 and add a spider/Spitter arena.
- A ward-hit tint in the HUD portrait was built; next could be a short "shield down" voice bark per hero.
- Gun maker sound flavour (Roundhouse heavy, Saint Maud ringing, Windfall rattly, Meridian clean) as Mech pitch/rattle.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
