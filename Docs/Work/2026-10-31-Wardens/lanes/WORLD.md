# Lane WORLD: Landfall's world pieces (build scripts)

Nothing built, nothing run in Unreal, no git. Python syntax and names checked with UE's bundled python (ast, symtable);
the ledge search, course spline and trail routing ran offline against a synthetic ledge (stub `unreal`): a 9 m deep
ledge takes the hull only "alongside" (see Moored below), course 375 m, 24 points, grapple stop index 16.

## Files

| File | What |
|---|---|
| `Tools/Unreal/build_area_landfall.py` (new) | Everything Landfall places, measured with traces at build time (`make_plan`, read-only) then placed (`place`), in `Gameplay/Landfall`. One table of other lanes' names (`NAMES`, `SHIP`), one of stand-in paths (`STAND_INS`, `MATERIALS`). Standalone: `py .../build_area_landfall.py RansomsRest` (clears its folder only; Hob and Delia need the full gameplay pass). |
| `Tools/Unreal/build_landfall_materials.py` (new) | 5 MIs in `/Game/Art/Landfall/Materials`: `MI_Landfall_AbelTrail` (M_GravewindWisp, gold-white), `MI_Landfall_SoulSmog` (M_Smoke, black-violet), `MI_Landfall_SoulSmogGlint` (M_Smoke, faint cyan), `MI_Landfall_BargeIron` (MI_IronBlack), `MI_Landfall_HoseRubber` (MI_PaintBlack). No new master. |
| `Tools/Unreal/build_area_story.py` | Hook: `build_area_landfall.place(build)` in `place()` after the lift (so after the deck's StoryLighting), before Hob. Ellis's grave actor destroyed in `place_family_plot` (layout placement `graveEllis` KEPT: ~12 scripts gate "is Ransom's Rest" on it); Ellis's readable headboard + `hero_headboards` removed; Abel's headboard stays. Farm safe zone now `after Main1` (was always: the porch fight is inside it and safe ground keeps creatures off). Hob: placed on Abel's headboard; Main 1 perches = winch (anchor step), lookout rail (point step), Abel's headboard (trail step), porch beam end (porch step); away for steps 0-3; after Main 1 on Abel's headboard. |
| `Tools/Unreal/build_area_farm.py` | `ADeliaRansom` (class `DeliaRansom`) on the porch floor 1.1 m out from SOCKET_Speaker, facing out, tag `Speaker_Delia`, name "Delia Ransom"; falls back to the old screen-door `ASpeakerPoint` (same tag/name) until CREW's class exists. Topics: `story_texts.TOPICS['Speaker_Delia']` if STORY adds it, else the old post-Main 1 ones (DeliaMine, DeliaMain6 + event, Main 7). Porch barricade (6 solid stand-ins tagged `Obstacle`,`Barricade`, each ground/road/overlap checked). |
| `Tools/Unreal/build_area_missionprops_valley.py` | Porch plate (`porch_plate_when`): STORY hasn't decided, so **a free pickup after Main 1** (unless Main 1's data asks for `PorchPlate.Taken`/Interact `PorchPlate`, or `story_texts.PORCH_PLATE.take_when` names something other than Main 1). Words from `story_texts.PORCH_PLATE` when given; `DELIA` = "Delia Ransom"; the eat line no longer says "Ellis". |
| `Art/Levels/RansomsRest/layout.json` | Only `level.environment.states.Smog` (build-only block: layoutSha1 unaffected). |
| `Saved/landfall_probe.py` (new, read-only) | Prints `LANDFALL` lines + writes `Saved/landfall_probe.txt`: classes/properties, stand-ins' bounds, merge API, an ASCII map of the ledge with hull/winch/climbers, the whole plan, course clearance per segment, Beyond meshes' collision, lookout deck heights, farmhouse sockets. |

CODEMAP: no lines (scripts aren't in it).

## What goes where (all measured, UE cm, X north)

- **Lamplighter** (`LamplighterShip`, tags `Landing_Lamplighter`, `Lamplighter`: set here because a script's tags replace C++ defaults). Placed **at Moored** in the editor (the tours see her on the ledge); FLIGHT puts her at `Course[0]` for a new game. Sets `course` (TArray<FVector>, pivots), `grapple_stop` (int, index into it), `moored` (FTransform).
  - Moored: a traced 50 cm height grid round the mooring post (X -1100..+900, Y -2000..+2300 off it); cells ground / rough / drop / rock. Tiers tried in order: `on` (bow third 90% on ground, ≥40% of the hull, stern over the drop, rolled 2.5°/pitched 1.5°), `relaxed` (60%, 30%), `alongside` (hull wholly over the drop beside the ledge edge, deck flush with the ledge, level; one side ≤1.5 m from ground along ≥35% of its length). Keep-outs: post 4.2 m (covers the seeds' page, letter and `Secret_MooringLocker`, all ≤2.7 m from it), stairs' foot 3 m. Prefers heading east and the hull's middle near the stairs. The real ledge (boundary corners 71-74) is ~9 m deep: expect `alongside` over the Mooring Notch.
  - Course: start ~350 m west over the plains (40 m over whatever's under it), down into the canyon (25 m under the ledge level, 190 m out), the grapple stop 120 m back along her approach 8 m over the ledge (approach = moored heading if open air there, else due east), then two landing points (40 m out 12 m up; 3 m over Moored) FLIGHT may use or ignore. Every segment before the stop is swept (keel −6 m, bag top +17 m, both sides) and raised 5 m at a time (≤14 tries) where it touches.
- **Winch** (`LandfallWinch`, tag `Landfall_AnchorWinch`): ground cell beside the bow on the ledge side, 2.6 m off the hull, facing the hull's middle.
- **LedgeClimbers** (`AmbushSpawner`, id `LedgeClimbers`, entrance Rise, no ambush corners so it springs on approach, 35 m): 4 Basic Unpaid at ledge cells next to the drop, spread apart, ≥2.5 m off the hull and keep-outs; during Main 1 anchor step only; hunt on spawn; give up 30 m; tag `Unpaid_LedgeClimbers`.
- **Spyglass** (`LandfallSpyglass`, tag `Landfall_Spyglass`): lookout deck point (commonest height 5.6-7.6 m over the pivot) with a clear eye-height line to the barge, farthest toward it, ≥1.3 m from Sexton's seat and the ranger cache's two spots; yaw faces the barge.
- **Barge** (one `AStoryProp`, label/tag `Landfall_Barge`, its pivot = the keel's middle): SM_Skiff_A_Gang ×7 (63 m) all slots MI_Landfall_BargeIron, keel 45 m over the Sink's highest rim vertex, broadside to the lookout; 3 stacks (cylinders, through the bag +6 m), 12 glowing vents (MI_CinderGlow), a hose drum under the keel. **Hose** (`Landfall_BargeHose`): ≥6 sections 1.5 m across with collars, a 6 m sideways sag, to an iron intake 4.5 m off the floor's middle toward the lookout. Both baked into one mesh each (`/Game/Art/Landfall/Baked/SM_Landfall_<Piece>_<hash>`, reused while the parts don't change; else placed one by one, capped). **Smog**: 3 plumes ×4 (32 m) on the stacks, a cyan glint ×2.8 in the middle one, ×2.2 at the intake. **Drone**: `AAmbientEmitter` loop `Ambience.Smog` along the hull (SOUND's cue; always on, can't follow the story). All shown `before Main6` (`BARGE_UNTIL`).
- **Smog rule**: appended to the deck's `StoryLighting` (after its Dusk rules for Main 6/7): `Smog` while `before [Main6]`. State values in layout.json (sun 4.5, sky 0.75, fog density 0.011 grey-green, glow brass, backdrop/clouds greyed, exposure 0.2 vs Day's 0.4).
- **Abel's trail** (`Landfall_AbelTrail`, one baked AStoryProp, `during Main1 from trail step`): ~45 wisps (SM_GravewindWisp_B/C/A ×0.55-0.75, lifted 1.1 m, pitched 22° so the ribbons lie over the ground, flowing toward the farm) every 3.4 m: the lookout's foot → bluff path (ramp reversed) → bluff approach → past Abel's grave → family plot path → round the farmhouse's north corners → the porch steps. The merge must keep vertex colours and UV1 (checked; else placed one by one, 30 at most, culled at 60 m).
- **Abel's hat** (`ReadableProp`, tags `Landfall_AbelsHat`, `Readable`, `Readable_Landfall_AbelsHat`, SM_AbelHat): the bluff path's middle bend (-7000,-6600), 1.1 m toward the higher (bluff) side; shown `without_flags [Landfall.AbelsHat]`; reading sets `Landfall.AbelsHat`. Words: `story_texts.READABLES['Landfall_AbelsHat']` if STORY adds it, else fallback words in the script. Then an AStoryProp hat on Delia's rocking chair `with_flags [Landfall.AbelsHat]`.
- **FarmPorch** (`EncounterSpawner`, id `FarmPorch`, at the porch's front): wave 1 two riflemen (farm road) + three Unpaid (orchard); wave 2 one rifleman + two hose-men (road), wait for clear + 2.5 s, 5 alive max, all Basic; spots 15-23 m from the porch; on approach 45 m; `during Main1 from trail step before porch step + 1`; hunt on spawn; give up 60 m; tag `Landfall_FarmPorch`.
- **Loudspeakers**: posts (SM_FencePost stretched to 4.2 m, solid), two brass cone horns, a black plate; 3 `InstancedProps` (culled 150 m); one on Ransom's Point 6.5-11 m from the lookout toward the bluff path's top, horns toward town; up to three on Main Street at X ±700 (Y 900, 2600, 4300), horns across the street; each spot off roads, on Ground, nothing solid there. A `TargetPoint` tagged `Landfall_Loudspeaker` at each post's horns.

## Exact edits Main must make in others' files

1. `Source/AI_Looter_Shooter/Tests/ColdWelcomeTests.cpp` line 595 (Looter.Story.ColdWelcome.Placed breaks: the farm zone isn't always on now):
   - old: `TestTrue(TEXT("Delia's salt line keeps the farm safe from the start"), Farm && Farm->ActiveWhen.IsEmpty() && Farm->Corners.Num() >= 3);`
   - new: `TestTrue(TEXT("Delia's salt line keeps the farm safe after Landfall (its porch fight is fought inside it)"), Farm && Farm->ActiveWhen.AfterMissions.Contains(FName(TEXT("Main1"))) && Farm->Corners.Num() >= 3);`
2. `Art/Levels/RansomsRest/layout.json` `gameplay.levels.zones` (contract: levels 1-2 here; today the ledge and farm fall in `valley` 2-3). Insert before the `west` entry:
   ```json
   {"id":"landfallLedge","polygon":[[-7000,-50000],[-7000,-11000],[-9590,-11000],[-9590,-8300],[-11800,-8300],[-11800,-50000]],"levels":[1,2],"note":"Landfall (Main 1): the Lamplighter's course, its boarders and the Mooring Ledge's climbers"},
   {"id":"landfallPorch","at":[-4000,-5280],"radius":2600,"levels":[1,2],"note":"Landfall's porch fight at Delia's (FarmPorch spawns 15-23 m out); kept clear of the OrchardEdge camp's spots at Coffin Rock (29 m+)"},
   ```
   then `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"`; tests `Looter.Areas.Bands`, `Looter.Creatures.AreaLevels`.
3. `Art/Levels/RansomsRest/views.json` (new tour views, append before `DeckFight_Dusk`, which must stay last):
   ```json
   {"name": "LedgeMoored", "x": -9300, "y": -9000, "height": 900, "yaw": -132, "pitch": -35, "fov": 80},
   {"name": "LedgeFromStairs", "x": -9700, "y": -10150, "height": 170, "yaw": -95, "pitch": -4, "fov": 80},
   {"name": "SpyglassBarge", "x": -8600, "y": -10000, "height": 170, "yaw": 51, "pitch": 4, "fov": 60},
   {"name": "FarmBarge", "x": -3900, "y": -5100, "height": 170, "yaw": 60, "pitch": 12, "fov": 80},
   {"name": "SinkUnderBarge", "x": 4523, "y": 4301, "height": 170, "yaw": 88, "pitch": 25, "fov": 80},
   {"name": "TrailFromPoint", "x": -7800, "y": -7400, "height": 170, "yaw": 35, "pitch": -10, "fov": 80,
    "exec": ["Looter.Landfall 7"], "after": ["Looter.Landfall 0"]},
   ```
   (`Looter.Landfall <step>` is FLIGHT's dev command; it shows the trail. Note: `Looter.Landfall 7` moves the player/ship, so
   put TrailFromPoint last but one, before `DeckFight_Dusk`, or check FLIGHT's command doesn't move the tour's camera.)
4. STORY (`Tools/Unreal/story_texts.py`, `create_mission_assets.py`, `create_story_lines.py`), what this script reads:
   - Main 1's data: step 4 = Interact `Landfall_AnchorWinch` + Clear `LedgeClimbers`; step 5 Reach `Place_RansomsPoint`; step 6 Interact `Landfall_Spyglass`; step 7 Reach `Speaker_Delia` (+ optional Interact `Landfall_AbelsHat`); step 8 Clear `FarmPorch`. The script finds these steps in `DA_Mission_Main1` by those tags, else uses 4-8.
   - `READABLES['Landfall_AbelsHat']`: title, heading, text, prompt, lines, `sets_flag='Landfall.AbelsHat'`, no `shown_when` (the script sets it). Delia's best line: her turn-in variant `with_flags ['Landfall.AbelsHat']`.
   - Optional `TOPICS['Speaker_Delia']` (`default`, `topics`): replaces the farm script's built-in list (which has no Main 1 topic: Landfall's words are the turn-in's). Her name on the point: "Delia Ransom".
   - Optional `PERCHES` keys `landfallLedge`, `landfallPoint`, `landfallTrail`, `landfallPorch` = (condition dict, arrival line set); else the script uses `during Main1 from_step` 4/5/7/8 and line sets `DA_Lines_HobLandfallLedge|Point|Trail|Porch` if they exist.
   - `PORCH_PLATE`: its `take_when` still says `during Main1 from_step 4`: the script ignores that (no Landfall step asks for the plate) and makes it a free pickup after Main 1. Fix its `eat_lines` speaker ("Delia Ransom") and drop "Ellis".
   - The old Main 1 optional "Read your own headboard" (`HeadboardEllis`) can't be done any more: the readable is gone.
5. FLIGHT: confirm `NAMES`/`SHIP` at the top of `build_area_landfall.py`: Python names `course`, `grapple_stop`, `moored`; pivot at the keel's middle, bow +X, 30 x 8 m hull, deck 1.45 m and bag top 17 m over the pivot. Moored may be **alongside** (hull over the drop, deck flush with the ledge, level): HardLanding should scrape her along the edge and settle, the winch's line holds her. **Fall recovery**: the player rides the deck outside the playable boundary over the canyon (open edges) the whole way in, and stands on the stern over the drop once moored: `UFallRecoverySubsystem` must not recover a player based on the ship. Hob has no perch during steps 0-3 (away). `UVaneBroadcast` can play its hiss at the `Landfall_Loudspeaker` points.
6. CREW: `ADeliaRansom`'s speaker component Python name assumed `speaker_point` (`build_area_farm.DELIA_SPEAKER`); she is placed on the porch floor, facing out; the barricade pieces (tag `Barricade`) are her cover. FarmPorch creatures carry `Landfall_FarmPorch`.
7. DREDGER: class names `DredgerRifleman`, `DredgerHoseMan` (EncounterGroup classes); cover in the yard: barricade, crates, the cart (tag `Obstacle`).

## Run order (Main's step 5)

0. **Probe first** (before the C++ build is fine; again after it for the class checks): `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Saved/landfall_probe.py" -Until "LANDFALL done"`, read `Saved/landfall_probe.txt`.
1. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_landfall_materials.py"` (warns per missing parameter).
2. `build_area.py RansomsRest environment` (adds Smog to `ALightingStates`).
3. `build_area.py RansomsRest gameplay` (story → farm → … → landfall → Hob → roosts; then the other gameplay scripts).
4. `build_area_loot.py RansomsRest`, `build_area_fauna.py RansomsRest` (they read the built level), optionally `build_island_scatter.py RansomsRest` (grass back where Ellis's grave stood; kept off the barricade).
5. Delete unused `/Game/Art/Landfall/Baked/SM_Landfall_*` from earlier runs (a changed level makes a new hash).

## Numbers to check in the log / probe

`landfall plan:` lines: the tier (`on`/`relaxed`/`alongside`/`design` = nothing fit, check), share on the ledge, winch not None, 4 climbers, course raised count (0-3 fine; 14 = still touching), `open air under the stop: True`, barge keel ≈ rim + 4500, spyglass not None and its yaw ≈ 51, trail ~40-50 wisps, hat not None, porch dredgers 5 / dead 3. Warnings to read: "placed one by one" (merge failed), "no ... class", any barricade piece "left out".

## PIE check (after Landfall plays through)

Ship on the ledge (or alongside) not clipping the cliff, stairs reachable from the deck, winch reachable and the post/page/letter/locker untouched; the dead rise at the edges during the anchor step; spyglass faces the barge from the rail; the barge reads as a silhouette with black smog through the haze from the Point and the farm; Smog state on from the start, Dusk still at Main 6; trail visible from the Point and leads to the steps without crossing the house; hat on the bend, gone once read, on the chair after; FarmPorch comes from the road and orchard, Delia's barricade gives cover, the steps stay clear; Hob on winch → rail → headboard → porch beam.

## Tour views to photograph

New (item 3): LedgeMoored, LedgeFromStairs, SpyglassBarge, FarmBarge, SinkUnderBarge, TrailFromPoint. Existing heaviest, before/after: PointOverValley, PointOverValley_FarShadow, Farm, Aerial, Aerial_Dusk, PointWest (sees the ship), MainStreet (speakers, plumes overhead), SinkRim.

## Perf risks (margin 0.2-0.5 ms)

- Valley views gain the barge (1 baked mesh, ~2-3 sections, casts a shadow: only inside the shadow distance, i.e. from town and the Sink), the hose (1-2 sections), 5 translucent plumes 18-32 m tall (overdraw when looked up at from town/the Sink), the drone. If over budget: `cast_shadow` off on the barge, drop the glint plume, `STACK_PLUME` 3.
- The Farm view (7.8 ms, a heaviest one) gains 6 barricade draws + Delia (CREW) + the trail (only steps 7-8). If over: bake the barricade (merge with `merge_physics_data` True) or cut the yard pieces.
- If the merge fails, the barge falls back to 5 props, the hose to 8, the trail to 30 (culled at 60 m).
- perf.ps1's "RR spawn" moves: a new game now starts on the ship over the canyon (FLIGHT), so its numbers aren't comparable with the old spawn's; compare tour views.
- The Smog state itself costs nothing (fog density and tints only), but it changes the look of every perf/tour shot taken in a new game.

## Risks I couldn't check

- The merge API in 5.8 Python (`StaticMeshEditorSubsystem.merge_static_mesh_actors`, `MergeStaticMeshActorsOptions`, the setting names) and that the merged pivot is the first part's place (the barge's actor location is what FLIGHT's SmogReveal camera aims at). Fallbacks exist; the probe reports.
- Whether the canyon/ring meshes (tagged Beyond) collide: if not, the course sweep can't see them and she may sail through the far wall's look (probe prints their collision).
- AReadableProp's `shown_when` re-read when a flag is set (the hat should vanish on reading).
- The skiff's bag shape: stacks may poke oddly through it; vents assume the 1.2 m gunwale half-breadth.
- Barricade stand-ins' orientations (bench and lumber stack long axes unknown); each is overlap-checked but may need yaw tweaks in `BARRICADE`.
- Soul smog read: M_Smoke multiplies by MPC CloudTint (dimmed in Smog: 0.6), so it gets darker still; fine for black smog, but the glint may vanish (raise its SmokeColor).
- `Ambience.Smog` plays always (emitters have no story condition), also after Main 6.

## Stand-ins (TRIPO_MODELS.md numbers)

Barge [12] SM_Skiff_A_Gang ×7 + engine shapes; hose [13] engine cylinders/cone; loudspeaker post [18] SM_FencePost + cones + cube; barricade [17] SM_Bench, SM_LumberStack, SM_FlourSacks ×2, SM_Crate_B, SM_Cart; the hat is the real SM_AbelHat. Paths: `STAND_INS` (landfall), `BARRICADE` (farm). The ship, winch [16], spyglass, grapples, Delia and the crew are FLIGHT's/CREW's.
