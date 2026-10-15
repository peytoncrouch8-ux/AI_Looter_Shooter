# Pipeline

How things get made and fixed here. Update when work finishes (a new way, a bug and its fix, what didn't work). Rules:
`CLAUDE.md`. State of the work: `Docs/Handoff.md`. Old detail, not loaded by default: `Docs/Archive/Pipeline_history.md`.

## The game

- Story: *Revenant* (`Docs/Story.md`), weird west on the planet Teropa. One area per Hollow, each designed in `Docs/Areas/`.
- Areas, map order since 2026-10-13:
  - 1 Ransom's Rest: a new game starts on its cold open; its first missions teach the controls by hints. Mountain expansion:
    `Docs/Areas/RansomsRest_Expansion.md`.
  - 2 the *Gilded Lily*: behind the north mountain, by train; not built.
  - 3 Skyreach (the old tutorial island): reached later from the Lily by skylift; off the station boards for now, test with
    `Looter.Travel Skyreach`.
- Missions: all but Main 1 come from a giver (`EMissionStart::Offered`, `FMissionOffer`): a speaker point, a notice board or an
  artifact (`AMissionArtifact`).
  - ? = pick up, ! = turn in (the user's call, 2026-10-13: the reverse of Borderlands). Gold = story, white = sides. Shown on the
    map, the minimap and over the giver (`UGiverMarkSubsystem`).
  - Offer card `UMissionOfferWidget`: Accept / Not now. The Missions tab lists only missions taken up.
    `Looter.Mission.Offer <id>` opens a card.
  - Features: optional objectives (`bOptional`: never hold a step, pay `OptionalSilver` at the turn-in); mission items
    (`AMissionItem`, carried, dropped by a creature); a turn-in with a choice of two (held to choose; story flags in
    `FCampaignRecord::StoryFlags`, read by `FStoryCondition::WithFlags`); readable props; the porch plate; an UNDERGROUND minimap
    plate (actors tagged `Underground`); any speaker point or a vendor's hatch as a turn-in.
- Hob's roosts (the user's pick, 2026-10-13): the ONE save / respawn / fast-travel point on every map: the Crow's Mast
  (`SM_HobRoost`, bell `SM_HobRoostBell`), beacon in Hob's Ember (#D8321C, core #FF8A52; `URoostSettings`, in Project Settings
  under Game > Hob's Roosts; `LooterUI::Color::Roost`).
  - Walk within 9 m (clear line) to find one: column shoots up, bell rings, crows lift, three circle the beacon, ROOST FOUND,
    early autosave. Continue wakes the player at the area's last found roost (`FCampaignRecord::LastGraves`).
  - Fast travel and deaths: Hob's crows carry the player across the screen (`URoostCarrySubsystem`, `UHudCrowVeilWidget`); Hob
    speaks at every death.
  - The actor is still `ARespawnMarker` (save ids unchanged). Placed from `layout.json` `gameplay.roosts` by
    `Tools/Unreal/build_area_roosts.py` (run alone it replaces every roost in the level). `Looter.Grave.List`,
    `Looter.Grave.Mark <id> | forget <id|all>`.
  - TWO a map at most (the user, 2026-10-14; `MOST_PER_MAP`): Ransom's Rest's are the Depot (train station) and Calder's Post
    (mountain); the other nine were removed, not kept for saves.
  - A death wakes at the roost last found on the map (never the nearest), before any at the level's own start (RR: the
    family plot's PlayerStart), Hob's crows carrying Ellis either way on a map with roosts. Arriving by fast travel finds
    the roost too (`MarkOnArrival`). Continue wakes at the same roost, before any where saved. One lookup:
    `ARespawnMarker::FindLastFound` (`LastGraves`, else the newest `MarkedGraves` still standing: an old save naming a gone
    roost loads onto one that stands; no migration).
- Impacts and bullet marks: a hit's surface (13: dirt, grass, gravel, sand, stone, wood, metal, sheet metal, glass, water, cloth,
  hay, bone) comes from tags, then the trim sheet's strip under the hit (collision UVs), the terrain's baked surface map,
  material-name words; no physical surface types. Each has its sound, puff and mark; glancing hits ricochet.
  - Guns' after-shot (2026-10-14): brass (`FWeaponBrass`: last 60, 20 s + 1.2 s sink, SOCKET_Eject or a per-kind port on the
    visible gun; the revolver tips its empties at the reload's Eject step; `Weapon.Casing.*` by surface); a hot barrel's wisp
    (no shimmer: a distortion pass costs too much); tracers with a hot head and a smoke thread; visible ricochets (at most 2 a
    volley); impact chunks land on the hit's floor; each creature splashes its own (`ECreatureBlood`); enemy shots passing within
    1.5 m of the player's head whiz or hiss once (`NearMiss`).
  - Spider deaths (the user, 2026-10-14: more dramatic, legs curled UNDER the body): `SpiderCreatureDeath.cpp` throe
    (rear, kicks) 0-0.4 s, drop on the cry's knock with a bounce + `Creature.Spider.Thud` + dust, legs folded under back to
    front by 1.15 s, shudder before the sink; timing scales by sqrt(size). Watch any death on cue: `Looter.SpawnCreature`,
    `slomo 0.25`, `Looter.KillNearest`, grab frames, `slomo 1`.
  - Creatures react (2026-10-14, `UCreatureReactComponent`): a notice beat once a hunt (0.42-0.62 s by seed; none when hurt,
    a boss, or called by the pack), the M_FX_HitFlash overlay on a hit, deaths that linger (spiders 10 s with twitches, slimes
    splat, the Unpaid crumble to ash), rising entrances when seen within 60 m, idle dice, footfall dust (level budget 40/s).
  - Marks are pooled DBuffer decals (`Looter.BulletMarks.Max/.Hold/.Fade`, 64, 20 s + 15 s fade); atlas script-made
    (`Art/Textures/BulletMarks/make_bullet_marks.py`).
  - Re-run `Tools/Unreal/build_impact_surfaces.py` after a macro map, scatter map or new trim-sheet model changes.
- Objective highlight: within 13 m, what the tracked mission's current objective is about (optional ones too, the giver once it
  waits; never a fight's creatures) glows orange as an overlay on its meshes (a spot of a building round a door).
  `M_ObjectiveHighlight` from `build_highlight_material.py`; `Looter.Highlight.List`. A character (a story character or a
  skinned figure, never a gun; `IsFigure`) wears the figure look: rim only, body all but clear (`Figure` 1; tune the
  script's `Figure*` defaults). A thing's 20% fill turned Sexton's near-black coat solid orange (fixed 2026-10-14).
- Holds: every hold-to-use shows the kit's round ring (`SLooterProgressRing`, `LooterUI::MakeHoldKeyCap`).
- HUD (the user's list, 2026-10-14): the mission tracker sits under the minimap, right-aligned (`TopFor`, `RightInset` 36,
  `MaxWidth` 540: clear of the boss bar's 380 px either side of the middle); steps and the objective are circles, checked
  once done; a line without a count takes the count's room before wrapping. A kill's XP floats by the kill (`XpPopup*`) and
  as "+N XP" after the XP bar's numbers. The gun's notches (inked tally in its tier's colour, count) sit before its name.
  Health and XP bars 25% longer and thicker (`HudFrameShapes::BarScale`). Subtitles On/Off in Interface
  (`Looter.Subtitles`): off hides lines with a speaker (barks too); notices stay. The game has no voice acting, so off
  means NPC talk isn't shown at all.
- Lost Loot (the user's call, 2026-10-13, so ground guns don't pile up and cost frame time): a gun on the ground lasts 5 minutes
  (not while the player stands within 6 m), then goes to the depot's LOST PROPERTY window. Leaving a map or quitting sends every
  ground gun there; a load never puts one back. 30 spaces, +10 per upgrade (five, bought with silver); the oldest unnamed gun goes
  out when full. `Looter.LostLoot.*`.
- The ore lift: `AOreLift` rides as a skippable look-only scene; the body is parked on the far landing at the start (a save,
  death or load mid-ride finds it there). `Looter.Lift.*`.
- Look: stylized realism; the user rejected faceted low-poly. Golden-hour palettes on Ransom's Rest (ochre, olive, sandstone),
  late summer. Loot colours (rarity orange, teal, gold) stay off the world.
- Third person (2026-10-14, `LooterStancePoseCarry`): low-ready after 0.8 s moving without aim/fire (~19 deg down), aim or
  fire snaps back in 0.12 s; sprint = chest carry, both hands turning with the gun (long foregrips drawn in), revolver low
  at the side one-handed; the upper body keeps ~44% of the jog's bounce. The player's body is still the template
  mannequin (Ellis's own body: later, the user's call).
- First-person hands (2026-10-14, `UFirstPersonHandsComponent`, the user's pick: full, like Borderlands): arms on the
  camera posed after the gun each frame; per-kind hold table in `FirstPersonHandsGrips` (where tuning happens); the support
  hand follows reload parts and carries the jar. SK_FP_Arms (approved 2026-10-14; `Art/Models/Characters/FPArms.py`,
  sheet `Saved/Concepts/Hands/fp_arms_sheet.png`) at `ArmsScale` 0.88 (the user: hands looked big). The rig reads bone
  positions only (the import turns bone axes). Hip holds (`Weapons/WeaponHipHold.cpp`; the revolver's in
  `create_revolver.py`): grip cm ahead/right/up, rifle 50, 23, -23.5; shotgun 30, 22.5, -23; revolver 50, 22, -21 (the
  user's Borderlands framing: low right, both gloves in view; `Looter.Player.Hands.OnGun` keeps the grip points inside
  0.85 of the half-frame, but for the shotgun's trigger hand). Long guns level (no muzzle-down tilt: the user, "the AR is
  tilted forward"). Support hand palm up under the fore-end, forearms from below so the sleeve's cut end (SK_FP_Arms
  ends at the elbow in a capped cut) stays under the frame (`ForeForearm` in `FirstPersonHandsGrips.cpp`).
- The shotgun (the user, 2026-10-15): 20 cm nearer than the rifle ("too far forward making the arm look silly"): the
  stock leaves the frame past the bottom right and the left arm's reach is short. Every full stock hid the trigger glove
  at the old hold (only the stockless Raider grip showed it), so it sits under the frame now. The left hand cups the pump
  from the lower left, rolled 20 degrees and turned 45 in its own plane, forearm in line (straight down bent the wrist
  square: "angles abnormally"), the pump through the grip's middle, `EHandGrip::Pump` curls (fingers closed round it,
  the thumb straight along its near side: curled, it read as "a nub").
- Tune hands live in PIE: `Looter.HipHold.Kind/Ahead/Right/Up/Pitch/Yaw/Roll`, `Looter.Hands.Kind`,
  `Looter.Hands.Support*/Trigger*` (palm roll/pitch/yaw, forearm yaw/pitch, hold), `Looter.Hands.SupportFingerCurl` and
  `SupportThumbCurl` (every joint, degrees), `Looter.Hands.Scale`; `Looter.Hands.Dump` prints paste-ready table lines.
  The forearm switches barely move a far hand: past the upper arm's reach the shoulder comes to the elbow (90 cm at
  most), so move the hold instead. `Looter.GiveWeapon <gun> <rarity> <level> Slot=Key` picks parts; `play.py equip <n>`
  takes a slot in hand (number keys don't reach the game through input.ps1).
- Gun sizes (the user, 2026-10-14): drawn bigger than modelled, everywhere: rifle and shotgun 1.1, revolver 1.2
  (`UWeaponModelComponent::ScaleFor`; sockets scale with it). The pistol shows as "Revolver" (DA_Revolver's display name;
  Drover stays its parts' and files' name).
- Swaps (2026-10-14, `Player/WeaponSwapMotion`, `Inventory/WeaponManagerSwap.cpp`): the gun in hand drops and tips out
  of view (~0.19-0.24 s by handling, `Weapon.Holster` / `Weapon.Revolver.Holster`), the guns change out of sight, the
  next comes up with an overshoot and settle; a second press mid-holster retargets; no sights while swapping;
  `Looter.Swap.Motion 0` turns it off. Third person: the right arm lowers the gun, the left lets go.
- Water (2026-10-14, the user's concept, clear teal): water meshes carry vertex R shore, G depth, B/A flow (layout x, y);
  `M_Water` reads them (opaque, lit, 3 taps; defaults tuned: RippleScale 0.2, RippleStrength 0.3, Roughness 0.12, SkyTint
  0.35, DepthPower 0.6, shallow #43918A, deep #184A54); `M_Waterfall` streaks in world metres; `M_WaterMist` (Opacity 0.35,
  Brightness 1.6). Rebuild only the water: `$env:LOOTER_TERRAIN_ONLY='water'; Tools\models.ps1 -Only <Areas>`, then
  `build_area.py <Area> water` (re-places the falls in `Effects/Falls`). terrain_identity differs only in `<Area>_Water`
  until committed. Splash rings and wake: `FWaterRings`.
- Player numbers (2026-10-13): 0.85 of the mannequin's size and 0.9 pace on top (the user: "pace of play is super fast"): walk
  459, sprint 711, crouch 230, slide 783 cm/s; eye about 140 cm; jump 103.5 cm. A slide comes out of sprint plus crouch. Chasing
  creatures scale with `LooterPlayerSize::MoveScale` (size and pace); slimes don't.

## Who does what

- Main (orchestrator, Opus 5.5): owns Unreal, builds, asset scripts, tests, play checks, git, docs; splits work among at most 8
  agents and checks everything they hand back.
- Models (the orchestrator's call): Opus for design and tricky code (new systems, rasterizers, widgets with timing and layout
  logic); Sonnet for well-specified code against a contract, reviews, doc edits, mechanical changes; Haiku for simple searches and
  listings.
- Shared contract for parallel agents (first used for the HUD upgrade, 2026-10-07). Before launching agents that touch each
  other's code, Main writes one contract file (in its scratchpad) every brief points to: each new class's public API with exact
  signatures, new palette entries with values, where each widget is placed and by whom, which agent owns which files. Agents write
  disjoint files at the same time; Main builds them together once.
- Agents never build, open Unreal or touch git. They report files changed, what Main must run, CODEMAP lines, tests affected,
  risks they couldn't check. For editor numbers they write a read-only probe (`Saved\<short>.py`) that Main runs.
- Layout patches (2026-10-13): while one agent owns an area's `layout.json` (the terrain agent), the others write
  `Art/Levels/<Area>/patches/<name>.json` (path, op set/merge/append, where {id}, unlessKind, comment; entries marked `file` are
  area_shape.py tables, edited by hand). Main applies them with `Tools/layout_patch.py <Area>` (keeps the file's formatting;
  `--check` first), deletes the patch files, and regenerates the terrain if they touch the ground.
- The art session (a separate session) models in Blender (`Art/Models/**`), never commits, never opens Unreal. It exports into
  `Intermediate\ArtExport_<Family>`, then messages Main with files, triangle counts, pivots, sockets. Main imports, commits, and
  sends back in-game shots to check.
- Main's helpers (in `Tools`):
  - `Tools\commitlib.py`: `commit(files, codemap_edits, title, body)` stages files plus CODEMAP edits applied to HEAD's copy, so
    other uncommitted CODEMAP lines stay out. Edits an agent already wrote into the working copy are skipped there.
  - `Tools\stage_import.ps1 <export folder> <model,model>`: copies chosen models and their materials into `<folder>_Import`, for
    `Looter.ImportModels`.
  - `Tools/Unreal/open_clean.py [map]`: collects garbage, then loads a map from disk.
  - `Tools\winshot.ps1 <handle>`: captures a window with PrintWindow; use it on a Save Content dialog.
  - `Tools\dialogclick.ps1 <handle> <x> <y>`: clicks a point in that dialog, in winshot's pixels, only if the dialog really came to
    the front (Save Content's Don't Save is 493 502; a one-line Message's OK is 535 127).
  - `Tools\menushots.ps1 [-Label]`: photographs the inventory through its states with a realistic loadout.
  - `Tools\castshots.ps1 [-Tag]`: every creature and character in its states from three angles, with `numbers.csv` (feet in the
    ground, limbs through bodies, sliding feet, pops as a change of world velocity per 60 fps frame) and a `past_mark` column. Dead
    bodies and falling slimes aren't sampled; a limb's 30% nearest its joint is excused against the part it hangs from. Still read
    the pictures before trusting a number.
  - `Tools/Unreal/pie_check.py`, `path_probe.py`, `width_probe.py`: below.
- Briefs that worked: give the agent the user's exact words, the files it owns, the files it must not touch, the run order Main
  will use, and what to report. Say "edit with the Edit/Write tools only" (a PowerShell `-replace` once corrupted a script). A usage
  limit can stop agents mid-step: resume each with SendMessage, saying what landed on disk.

## The everyday loop

1. Before any editor work, check the user isn't playing: `Tools/Unreal/pie_check.py` (a play session in the editor);
   `Get-CimInstance Win32_Process` for `UnrealEditor.exe ... -game` (the launcher's game). While they play, run no console commands
   and no builds. Saves fail quietly on files the game holds.
2. Build: `Tools\launch.ps1 -Build`. If it says "EDITOR DID NOT CLOSE", a "Save Content" dialog is open: find it with
   `Tools\editorwindows.ps1`, capture it with PrintWindow, read its list, choose Don't Save unless those assets were meant to change.
3. Scripts: run the asset scripts the change needs through `Tools\console.ps1 "py <absolute path>"`. Keep the command under about
   200 characters, or it's dropped. Each `build_area.py` mode saves the level itself.
4. Tests: `Tools\runtests.ps1`, all green; `-Filter Looter.<Group>` runs a subset. Tests leave the level dirty: the runner reopens it
   clean afterwards (`Tools/Unreal/open_clean.py`; closing the editor once saved the tests' leftovers into the level) and closes
   the Message Log. `-Restart` also restarts the editor: after two full runs PIE crawled at 1-3 fps (2026-10-14).
5. Look: `Tools\tour.ps1 -Map <map> -Views <json>` (an interior's view gives `top`, a world height under its ceiling where the
   ground search starts; `Tools/cliff_views.py <Area>` writes a view per cliff point). Scratch view lists go in `Saved/TourViews/`; screenshots land in
   `Saved\Screenshots\Tour`. Read every screenshot before calling a visual change done.
6. Commit and push each working step on its own. The message says why and ends with the co-author line. To commit part of a shared
   file (CODEMAP), stage edits on HEAD's copy (`Tools\commitlib.py`).

## Play checks (playing a level through in PIE, 2026-10-14)

- Start: `Tools\pie.ps1 -Map RansomsRest` (Looter.PlayLevel: a new game, no session, nothing saved; Main 1 starts at step 3
  with no gun: `Looter.GiveWeapon Rifle Common 1`, then `play.py ammo`; `God` keeps the player up).
- `Tools\playstep.ps1` runs one step: `-Py "<play.py command>"`, `-Cmd "<console>"`, `-Keys` (input.ps1 steps),
  `-Button ACCEPT|CLOSE|...` (a game menu's button: found in the Slate tree, clicked with the real mouse), `-Focus`
  (what Interact would use), `-List` (open missions and the guns), `-Shot <name>` (half-size grab).
- `Tools/Unreal/play.py`: `goto <tag|label> [cm] [yaw]` (stands in front of it on the floor, looking at its meshes),
  `at x y z [yaw]`, `find <word>`, `where`, `look`, `lookat <tag>`, `what [yaw] [pitch]` (what the view's middle meets),
  `press <Interact|Fire|Jump|Reload|Move> [seconds] [x] [y]` (feeds the input action itself: no focus needed; Move's y is
  forward), `fight [s] [m]` (aims at creatures in sight, pulses Fire, keeps ammo full), `kill [m]`, `ammo`, `guns`,
  `equip <slot>`, `view [n]` (F5's cycle), `inventory [close]`.
- Lessons: the console box takes keyboard focus, and only a left click gives the game viewport focus back (it fires a held
  gun), so use `press` for game actions and real keys only for menus (Tab, number keys). Offer and turn-in cards and the
  notice boards take mouse clicks, not injected actions. Teleports skip a roost's find (walk the last 15 m) and give hits
  time to knock the view off a target: check `-Focus` before pressing.
- Never `set_editor_property` on a play session's objects, and never edit the PIE pawn's components in the Details
  panel: a changed value's notice reruns the actor's construction script (`ConsolidatedPostEditChange` ->
  `RerunConstructionScripts`; no PIE guard in 5.8). On a moving player that rebuilt BP_LooterCharacter's Blueprint
  components and the weapon manager came back empty (2026-10-14, twice: the play-through's vanished guns). play.py stops
  the player with `stop_movement_immediately`/`clear_accumulated_forces`; the game warns `Guns: ... lost ...`.
- Probes for walkability: `Tools/Unreal/reach_probe.py` misreads floors inside thick hulls (stairs, the watchtower);
  walking it with `press Move` legs and `where` is the reliable check.

## Making a level (grounded areas)

- The next new level (the Gilded Lily) starts as a World Partition map (the user, 2026-10-14), so the engine's HLOD
  (simplified stand-ins for far groups: the town, cliff belts) comes with it. Ransom's Rest stays a plain map: converting it
  means one file per actor and rechecking every build script, the minimap, tours and tests (1-2 days, risky), and HLOD would
  win only ~0.3-0.6 ms in its wide views (hiding every building saved 0.1-0.2 ms, every cliff 0.4-0.5). Check the build
  scripts (`build_area*.py`), `open_clean.py`, the minimap bake and `Looter.Perf.HideFolder` against World Partition first.
- Data: `Art/Levels/<Area>/layout.json` (hand-written: features, roads, zones, obstacles, placements, the `level` block, gameplay).
  `views.json` holds the tour viewpoints.
- Generator (Blender, headless): the `Art/Levels/area_*.py` modules build the terrain tiles, ring, canyon wall and backdrop, and
  write the macro, scatter and scrub masks and `layout_computed.json` (cliff points, far trees, scrub points, boundary).
  - `Tools\terrain_identity.ps1 -Area <Area>` checks the generator still rebuilds an area byte for byte. Run it after any generator
    change, once the change is committed (it compares with HEAD).
  - `Tools\terrain_compare.ps1 -Area <Area> [-Except <json>]` proves a grown or edited area kept its old ground: heights, AO and
    curvature inside HEAD's boundary, outside the `-Except` areas meant to change (Ransom's Rest's mountain: max 0.82 cm).
  - Run: `blender -b --factory-startup --python Art/Models/Terrain/<Area>.py -- --macro --computed` (~35 min at 512 m, 6.8 GB;
    `--preview` adds the look-sheet renders), then `Tools\models.ps1 -Only <Area>`.
  - Fitting terrain to art (the mountain, 2026-10-13): `pad` with `exact` (hard edge and flat top where a model's floor must meet
    the ground); `mesh.holes` (open the ground over a stair or an adit's mouth); `boundary.interiors` (splice an interior's tongue
    into the play area); closed-boundary stretches where no rock dressing may stand; `map.grownFrom` (grow the square without
    changing the old ground); `map.macroSplit` (four 4096 macro windows).
  - After re-gridding the core (a grown square): delete the old tile assets the new export doesn't overwrite (`SM_<Area>_Tile_*`)
    and their actors; `build_area.py` places every tile mesh it finds.
- In the editor: `Tools/Unreal/build_area.py <Area> [mode]`; no mode = full build. Modes:
  - `gameplay`: story pieces and creatures (`build_area_story.py` and its `build_area_<place>.py` scripts);
  - `environment`: light, sky, fog;
  - `beyond`: ring, backdrop, far trees;
  - `cliffs`: cliff faces and outcrops;
  - `dressing`: fences, walls, graves, props (`build_area_dressing.py`).
- Cliffs (`build_area.py` cliffs()). The `level.cliffs` settings:
  - `varied` groups are mirrored, sunk and turned. `lean` groups lean with the wall; `leanTop` and `leanProud` can be set per group.
    The lip ceiling keeps tops under a rounded rim.
  - `abut` stretches a run's end into a rock, at most 2x its width.
  - `under` caps cliff tops under a platform (`build_area_platforms.py`; the burial deck).
  - `panels` is a run of narrow panels along a pit's curve (`build_area_panels.py`), with `talus` at their feet
    (`build_area_talus.py`).
  - `walled` (groups, or true): a point gets no faces unless the terrain's wall stands within 2.5 m behind it (2026-10-14:
    an `onFloor` ridge's band ran on past its floor, and 31 East Shoulder slabs stood on the valley floor, through the
    depot and across the rail line). `keepOut` (placement keys): no cliff piece, panel or outcrop stands inside those
    buildings (`build_area_keepout.py`: moved back into its wall, cut under a floor, slid along its run, else left out;
    each logged). Check with `Tools/Unreal/cliff_in_buildings.py <Area>` (read-only, `CLIFFIN` lines). Since 2026-10-14
    the generator writes an onFloor ridge's band only where its floors stand under its top (`Region.band_height` x
    `floor_weight`: eastShoulder 32 -> 1 point); `walled` stays as a guard.
  - Every ramp's walkway is cleared of cliff pieces (`build_area_walkways.py`): a player capsule is tested down its middle. A ramp's
    own wall moves back into its cut; a ledge's outer face (under the path) sinks until it clears, at most 1.5 m (2026-10-08: nine
    faces on the bluff path and the Sink ramp sank 25-100 cm, no gaps under the lip).
  - The review's rules (2026-10-14, the user: "fix any discrepancies or odd shapes"): no gap isolates a piece and no piece stands
    alone (`LONE_REACH` 15 m: its neighbours all left out, it goes too); no piece beside its own mirror (`VARY_SWAP`: the second of
    two identical takes another fitting kit piece); `level.cliffs.overlap` (cm, all or per group) widens a run's pieces;
    `level.cliffs.outcrops` {knob: {piece, height, yaw}} gives a knob another outcrop (benchGraniteH wears the narrow Slab: the
    reworked TorD stood on the Bench trail). Probe: `Saved/cliff_cover.py` (bare and buried shares per run). Photograph runs with
    `Tools/cliff_views.py` + `tour.ps1`.
  - The steep terrain reads as the kit's sandstone (M_Terrain's `SteepRock`, `SteepBeds`, `SteepAOLift`, `SteepSharp/Breakup/Pull`,
    `StrataSmooth/Flat`, all off by default; Ransom's Rest's four core `*_Steep` MIs turn them on; keep `SteepRockTint` equal to
    MI_RockCliff_Ransom's Tint). Preview without Unreal: `ground_preview.py walls`.
  - Cliff life (`build_area_cliff_life.py`, in the full build after gameplay and in the `cliffs` mode; `level.cliffLife`): drapes on
    lips, ledge plants, crack junipers, scree fans and fallen slabs at feet, the swallows' nests (SM_SwallowNests) under overhangs,
    stain decals (`MI_CliffStain`, the cell picked by the decal colour's R) on faces seen from the play area; caps per kind (RR: 285
    instances, 70 stains). `footprints()` keeps the scatter's grass off the fans and slabs.
- Scatter (PCG): `Tools/Unreal/build_island_scatter.py <Area>`.
  - Re-imports changed masks by MD5, then generates. Wait for the instance count to settle, then save.
  - Zones of kinds alpine, outpost, mine and crest, and `benchPines*` stands, grow pines only (PCG Intersection: one `In` pin,
    density MINIMUM or MULTIPLY; there is no BINARY), stands and lone meadow trees alike; the boxes reach 5 m past the zones
    (`PINE_ZONE_MARGIN`, over the mask's 4 m forest fade: a lone oak stood on the Bench until 2026-10-14).
  - Keeps off the dressing's footprints and wears the area's material swaps. Run `dressing` before the scatter when fences or graves
    move.
- Materials per area: `level.materials` makes area instances of shared MIs; `level.swaps` puts them on everything the build placed,
  and on the scatter; `level.swapsOn` swaps on named actors only (the windmill's steel).
- Path checks: `Tools/Unreal/path_probe.py <Area>` walks a player capsule down every road and ramp; `Tools/Unreal/width_probe.py`
  probes across one. Run them after cliff, dressing or scatter changes.
  - Both areas use `"scatter": {"roadside": "pebbles"}`, so no rock or boulder lands on a road (the tutorial island had a boulder in
    its forest road until 2026-10-08).
  - The bridge's one flagged spot on the keeper's path is the step onto its deck.
- Gameplay pieces from the layout's `gameplay` block need no generator run (read straight from `layout.json`): jetty, landings,
  posters, the gunsmith's benches (`gameplay.benches`, `build_area_benches.py`). `build_area.py <Area> gameplay` places them again.
  Also from it (2026-10-09):
  - encounters between the story's fights (`gameplay.encounters`, `build_area_camps.py`): camps with a Supply Crate
    (`CampCache_<id>`), patrols on a road, ambushes that rise or drop when the player walks in. Each waits for a story step and keeps
    off the safe zones and every scripted fight's ground;
  - Skyreach's notice board (`build_area_board.py`): the practice postings' board on the town's NoticeBoard piece, and the lookout's
    marker.

  From 2026-10-12, also in the gameplay pass:
  - Vendors (`build_area_vendors.py`): an `AVendor` at a shop's door, derived from the built level, so the store must stand first (it
    comes from the full build's models; the script runs after the town life). It stands 0.95 m out from the store's lamp socket,
    facing out, in `Gameplay/Vendors`. Ransom's Rest gets Pruitt's General Store (`Vendor_Pruitt`, open after Main 3), Skyreach the
    practice store on the square (`Vendor_Crossroads`, always open); any other layout gets none. A new shop is a new entry in the
    script, with its `OpenWhen`. On its own: `Tools\console.ps1 "py
    C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_vendors.py RansomsRest"`. It places nothing, and says so, when the game
    module's `AVendor` isn't built yet.
  - Vigil Stones (`build_area_shrines.py`): a stone beside each boss arena (the Gravemother's at the Sink's ramp head, Abel's outside
    the Keeper's Gate), after the story's pieces and before the camps: on level walkable ground clear of the layout's roads, ramps,
    obstacles and rocks, off the boss's ground, facing where the player comes out. Each choice is logged; a boss whose arena or actor
    isn't in the level gets a warning and no stone.
  - Creature levels (`gameplay.levels` in the layout: `band`, `zones`, `kinds`). A creature's level is where it stands, never the
    player's: the first zone holding its spot gives it a level rolled between its two (`zones`: a layout `zone`, an `obstacle`, its
    own `polygon`, or a `radius` round a point `at` or `between` two obstacles); a `kinds` entry sets a class's level (the bosses).
    - After editing, run `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"` in the open editor: it
      writes band, zones and kinds into `DA_Area_<Area>` (`UAreaDefinition`). `Looter.Areas.Bands` checks the assets against the
      layouts. An area with no layout yet (the Lily) keeps a plain `band` in the script.
    - Tests: `Looter.Areas.LevelBand`, `Looter.Areas.Bands`, `Looter.Creatures.AreaLevels` (every fight's and encounter's levels, the
      bosses'). Never read the player's level for a creature. Table: `Docs/Progression.md`, "Creature levels".
  - Creature pursuit (`FCreaturePursuit`, `CreatureBasePursuit.cpp`): a hunt goes on until there is a real reason to stop; a creature
    that can't get to the player waits at its ground's edge (or under them), watching, then walks home when its patience runs out.
    - Tunables: Project Settings > Game > Encounters > Pursuit (`UEncounterSettings`): sense radius, grace out of sight, sniff time
      and radius, lost-after, unseen limit, patience (`ProwlSeconds`), stuck time, spurn.
    - A group that hangs back (the Spitter, the Bell-Ringer) tracks no nearer than its hold distance. Bosses keep their fights' own
      rules. Tests: `Looter.Creatures.Pursuit.*`.
- Ambient life (`build_area_fauna.py`, at the end of a full build or on its own): bird perches traced on fence rails, roofs, graves
  and dead limbs (crows 15 m clear of Hob), insect zones from flowers, orchards and water, tumbleweed lanes, dust devil spots,
  washing on laundry lines. Derived from the built level: re-run after dressing changes. `UFaunaSubsystem` updates it (nothing ticks
  on its own); `Looter.Fauna 0/1`, `Looter.Fauna.Stats`.
- The small animals (`build_area_animals.py`, after the fauna in the full build, or on its own; look sheet 44): chicken yards and the
  porch cat from town.json's `spots` (pens from the picket yard and the coop's run, the cat's exit and hide point traced under its
  porch, or `hide`/`exit` on the spot), cliff swallows at the placed SM_SwallowNests (one flock per cluster group). Code-posed from
  pose tables (`Tools/animal_poses.py` writes `World/*PoseData.inl` from `Intermediate/AnimalsModel/*_poses.json`, which the
  models' scripts write: rerun both after a pose change). They're fauna: `Looter.Fauna 0/1`, `.Scare`.
- Air, light and life with nothing placed (concept C, 2026-10-14): lights tagged `Flicker` breathe near the player
  (`ULightFlickerSubsystem`; AHouseLights' lamps by default, the mine's and keeper's lanterns, the town's lamps); lit windows
  (homes' `SOCKET_Window_<n>`) play a passing shadow or a curtain now and then (`UWindowLifeSubsystem`, never a face); motes, pollen,
  leaves and moths round the camera (`UAmbientAirSubsystem`: trees, lamps and underground boxes scanned at start and at each
  lighting switch; pollen follows the terrain's surface map); a gust every 40-120 s (`UGustSubsystem`, MPC_Wind read by
  M_WorldFoliage and M_Smoke) bows plants, leans smoke, flaps washing, spins the windmill, rolls dust. Hung things swing on
  `USwingComponent` (put it at the pivot, attach the mesh). `Looter.Flicker`, `Looter.WindowLife`, `Looter.Air`, `Looter.Gust`.
- A town's dressing on Ransom's Rest: `Art/Levels/RansomsRest/make_town.py` writes town.json (vignettes per building, in its
  frame; `APPROVED` names the new clutter that goes in; `Saved/rr_town_probe.py` first, for real ground and spots), placed by
  the dressing like Crossroads Town's; the town's lamps light at dusk (`place_town_lights`). New town meshes need PIECES and
  FOOTPRINT entries in `build_area_dressing.py` (make_town.py prints them).
- Ambient sound emitters (`build_area_sound.py`, in the full build): creeks, ponds, falls, open edges, Main Street.
- The lootable world (`build_area_loot.py`, 2026-10-09): runs in the full build and the `dressing` mode, after the dressing and
  gameplay pieces stand and before the fauna; on its own, `Tools\console.ps1 "py
  C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_loot.py RansomsRest"`.
  - Turns the dressing's wooden crates and barrels (`Crate_A`, `Crate_B`, `Barrel_A`; never the steel drum) into `ABreakableProp`s
    where they stood, and stands breakables, graves to dig, coffins to pry, mailboxes and footlockers about the area at spots it
    finds (level, clear, off roads, off the story's and encounters' spots, never near a respawn grave or the family plot). Everything
    sits in the area's `Loot` folder, so a rebuild replaces only that.
  - Reads the built level: run it after any dressing or story change. Idempotent (a run gives the dressing's instances back first).
    Ids come from where each piece stands, so the session keeps the same one broken or opened; moving a piece forgets its state.
  - Breakables are pooled and nothing ticks idle: pieces are thrown from one pool of components (`UBreakableDebrisSubsystem`, at most
    40 out); a prop ticks only while a hit rocks it.
  - Models: `Art/Models/Props/Lootables.py` (pieces modeled where they sat, origin the prop's pivot); sounds: the `props` group.
    Check with `Looter.Breakable.Break/Mend/List` and `Looter.Chest.Open/Reset/List`.
- Town life (`build_area_townlife.py`, 2026-10-09): runs in `build_area.py`'s gameplay pass, after the story's pieces (the shutters
  must stand; the house lights come from the effects pass), or on its own like the loot script. Ransom's Rest only (a layout with
  Ellis's grave). Places `ATownLifeDoor`s outside the unnamed townsfolk's doors (folder `Gameplay/TownLife`) and gives the lit
  houses and the store's shutters a household, which is all `UTownLifeSubsystem` needs; nobody is seen, only heard. Households, their
  sounds and the mutters' lines are `TownLifeRules`. Places nothing, and says so, when the game module's classes aren't built yet.
- The map page and fast travel (the inventory's fourth page; M, down on the D-pad): the map is the minimap's runtime bake, so a
  level needs its `Ground` tags and nothing else. Kept by hand:
  - the places' names (`UI/Inventory/MapPlaces.cpp`, copied from the layout's `preview.labels`: change both together);
  - each grave's name (`ARespawnMarker::DisplayName` on the marker, else the map shows its id in words).
  - Pins come from the level (open respawn graves, stations, benches, chests found or opened, turn-ins, the objective), so a new
    kind of pin is a `MapPins` change.
  - Check with `Looter.Map` (opens the page), `Looter.Map.Graves` (each grave and whether travel could go there),
    `Looter.Map.Travel <grave id>`; tests `Looter.UI.Map.*`.

## The tutorial island and the older levels

- Tutorial island (Skyreach): built by scripts from `Art/Levels/TutorialIsland/layout_computed.json` (the terrain model writes it).
  - `Tools/Unreal/build_tutorial_island.py` places the terrain, cliffs, buildings, lighting and gameplay actors; rebuilding replaces
    only what it placed; its `gameplay` mode places just the spawn, dummies, spiders and slimes again.
  - `Tools/Unreal/build_island_scatter.py` scatters grass, flowers, trees and rocks with PCG from the scatter mask.
  - `Tools/Unreal/review_stage.py` photographs new models under the island's lighting.
- Crossroads Town (round 1, 2026-10-08): buildings, roads, the Wallow's pools (small `pond` features), zones and creature groups are
  in `layout.json`; the rest comes from `Art/Levels/TutorialIsland/make_town.py`, which turns the concept's drawing
  (`crossroads_town.json`) into `town.json`: pieces, fence lines, Web Hollow's webs, the no-tree areas, and the concept pieces still
  waiting for a model (`deferred`).
  - It applies the concept's fixes, maps its kit to the game's models, and keeps every piece off buildings and porches, roads,
    gameplay spots, each other and the rim (3 m), printing every nudge and drop.
  - `layout.json` `level.town` names `town.json`; `build_area_dressing.py` places it instanced (one `AInstancedProps` per mesh and
    solidity: reeds, lily pads, lone bushes, webs and cocoons are passable), its fences through the kits, its strands tied into the
    dead trees' trunks. `no_tree_boxes()` keeps the scatter's trees off the gardens, creature grounds and the town trees' crowns. The
    scatter grows no trees on the island any more: the concept's own woods are town pieces.
  - Order: edit `layout.json` -> terrain (`TutorialIsland.py -- --macro --computed`, then `models.ps1 -Only TutorialIsland`) ->
    `make_town.py` -> full `build_area.py TutorialIsland` (the `dressing` mode doesn't redo the no-tree boxes) ->
    `build_island_scatter.py TutorialIsland`, wait for the count to settle, save -> `path_probe.py`.
- Older levels are built in the editor. Procedural props are `StylizedProp` actors (shape, seed, two colors). Before committing such
  a level, run `Looter.BakeLevelProps`: it swaps them for static mesh actors and saves their meshes and materials under
  `/Game/Environment/Props`.
- Seating props: after placing props or changing terrain, run `Tools/Unreal/conform_hills.py` (terrain changes only: it fits the
  hills' rims under the ground), then `Looter.SettleProps` (`selected` for the selection), and save. It seats every prop so no edge
  hovers, leaning low, wide ones with the slope.
  - Not after a full `build_area.py` run (2026-10-13): the build already stands everything on the new ground, and SettleProps
    re-seats every mesh within 6 m of the ground: it tipped the mine portal 15°, dropped Keeper's Bridge 2.8 m into the wash and
    lifted the lift head 1.5 m. `TRANSACTION UNDO` in the console took it back. Use it only on a selection, for props placed by hand.
- Lvl_Skyreach's meadow: grass and flowers come from the `Meadow` PCG volume (`/Game/Environment/PCG/PCG_Meadow`), which raycasts
  onto `Ground` actors and avoids `Obstacle` ones. After changing terrain, select it, press Generate and save.
  - Ground cover never collides (a placed static mesh actor takes its mesh's collision unless `bUseDefaultCollision` is off).
  - Patches are about 3.5 m and lie on the slope; `World/PCGGroundFitFilter` drops the ones that would hang off an edge;
    `Looter.BakeGroundCover` bakes their meshes again.

## Making an asset

1. The art session scripts the model in Blender (`Art/Models/<Category>/<File>.py`), with shared material names and texture sets from
   `Art/Textures/<Set>`.
2. It export-tests: `Tools\artrun.ps1` or `Tools\models.ps1 -NoImport -Out Intermediate\ArtExport_<Family>`.
3. Main stages only the new models (`Tools\stage_import.ps1 <folder> <names>`), runs `Looter.ImportModels <folder>_Import`, checks the
   import log (bounds, sockets, collision), and commits the `.py` with the `.uasset`s.
4. A colour-only change in the manifest can be set on the MI directly instead of reimporting a skeletal mesh (the lantern glow).
5. Tools: `Tools\artrun.ps1` runs a model script (`-Preview` renders it), any Blender script, or an export test, at below-normal
   priority, waiting while `Saved\ArtPause.flag` exists. `Tools/Blender/tangentcheck.py` checks an exported FBX's tangents as
   Unreal's import will; with `--log` it sorts the editor log's tangent warnings into the model's own and Unreal's reduced builds'.
6. Materials: the textured masters (`M_World`, `M_Gun` with per-gun wear, `M_WorldFoliage`, `M_Terrain`, `M_Water`) are built by
   `Tools/Unreal/build_world_materials.py`. Older surfaces use the flat stylized materials (`M_StylizedSurface`,
   `M_StylizedFoliage`, `M_StylizedGlow`).
   - The ground's layers (2026-10-12, `M_Terrain`): under the macro map's colour sit three detail layers and damp soil. The macro map
     picks them: grass on green ground; matted thatch (`GroundThatch`) on gold and dry ground; stony ground (`GroundStony`: crust,
     gravel, stones, and leaf litter under trees) on bare soil, roads and scree; darker, glossier damp soil in the hollows (the
     terrain's baked occlusion). Each layer is normalized by its own mean (`T_<Set>_stats.json`, written with the textures), so a
     texture change moves the pattern, never the ground's colour from afar.
   - Textures come from `Tools/Blender/looter_textures.py` (sets `GroundThatch`, `GroundStony`, `GroundNoise`; the thatch's height and
     the stony ground's leaf litter ride in the colour map's alpha). The master is rebuilt by `build_world_materials.py M_Terrain`;
     its numbers are `GROUND_DEFAULTS` (repeats, how far the borders wander, the damp, where the contrast fades out), and an area's
     instance may change any of them.
   - Judge a change without Unreal first: `Tools\artrun.ps1 -Script Tools\Blender\ground_preview.py` renders the layers from tour
     viewpoints at eye height (before above, after below) into `Saved/ArtPreviews/Terrain/`. It is a numpy port of the master's code:
     keep it in step with `GROUND_WEIGHTS` and `GROUND_DEFAULTS`. It has no shadows, grass or hollows' damp, so look in the editor too.
7. Guns from parts: `Art/Models/Weapons/<Gun>.py` models the parts (sockets chain them; sights carry `SOCKET_Aim` for aiming down
   sights). `<Gun>.parts.csv` lists each part's key, name, name word, rarity and stat ranges in percent (capped per stat,
   `Weapons/WeaponParts.h`). After importing, run `Tools/Unreal/setup_gun_parts.py` in the editor to fill the gun's definition from
   the spreadsheet.
   - The Drover (2026-10-09), the third family (Bullpup rifle, Ranchhand shotgun, Drover revolver on pistol ammo):
     `Art/Models/Weapons/Drover.py` and `Drover.parts.csv`; parts by key as the others (frame, barrel, cylinder, sight, grip; never
     rename or reuse a key).
   - A new kind needs the C++ first (`EWeaponKind::Revolver`, `EWeaponReloadPart::Cylinder`), then in the editor, in this order:
     `create_revolver.py` (makes `DA_Revolver`, puts it in the default loot table), `setup_gun_parts.py` (the parts from the CSV),
     `create_rank_assets.py` (so the ranks' loot tables take the new gun).
   - The Drover's cylinder swings out on SOCKET_Crane; its chambers are the CSV's Magazine for that cylinder (rarity never changes
     the count). A short gun is held out by its definition's `HoldReach`.
8. Throwables: `Art/Models/Throwables/<Name>.py` (the grave-salt grenade, `SaltGrenade.py`) becomes `/Game/Art/Throwables/SM_<Name>`
   through `Tools\models.ps1 -Only <Name>`. Code loads it by path (`AGraveSaltGrenade`, `AGrenadePickup`), and a path is not a
   reference: `/Game/Art/Throwables` is listed under `DirectoriesToAlwaysCook` in `Config/DefaultGame.ini`, so a packaged game has
   it. A new category folder needs the same line.

Rules that saved time:
- Exports must be repeatable. Never iterate BMesh sets (their order changes between runs). Export twice and compare; only FBX IDs
  and time stamps may differ.
- Faces without UV area give tangent warnings; `Tools/Blender/tangentcheck.py` predicts them.
- Terrain meshes keep 100% Nanite fallback. Medium draws the fallback and collision is cooked from it.
- A variant of a model (Farmhouse_Ransom) goes in the same script, with the original exported byte-identical.
- A prop whose pieces the game hides is several models on one pivot in the same script (the porch plate: plate, jars,
  biscuits).
- The kit's `_triangulate` collapses triangles under 1 mm^2: build tiny details (ink, cards) as single faces of at least
  1 mm^2 a triangle. Map round, flat parts with `Tile(set, along=None)` (the default turns each face's grain to its long side:
  a star of seams on a lathe's floor).
- Tangent warnings logged by a reimport's first build come from Unreal's default fallback, not the model
  (SM_RR_RangerCabin, 2026-10-13); `tangentcheck --log` labels them.

## Character portraits (painted busts that pick a look before the model, 2026-10-15)

Why: the hero models in `Art/Backlog/Characters/` don't satisfy the user. The new order: a painted bust portrait per option
(the user's reference: richly framed, painterly character portraits), the user picks, then the 3D model is built from it.
First: Ellis, options A The Revenant, B Came Home at the Last, C Dressed for the Grave (`Art/Portraits/EllisBlender.py`
docstring and `OPTIONS`).
- Tools the user OK'd (2026-10-15): Blender's Python module `bpy` 5.2.2 from PyPI in the cloud box (`python3 -m venv`, `pip
  install bpy==5.2.2 numpy`; download.blender.org is blocked by the proxy, PyPI isn't), and MakeHuman's assets: base mesh,
  targets, default rig and weights, CC0 1.0 (`Art/ThirdParty/MakeHuman/LICENSE.ASSETS.md`), fetched by
  `Art/ThirdParty/MakeHuman/fetch.py` (run it first in a fresh clone: `base.obj` is ignored, an `.obj` would go to Git LFS and
  the cloud box can't push LFS objects). Never MakeHuman's code (AGPL). Node + Playwright (Chromium) paint and frame.
- Run: `python Art/Portraits/EllisBlender.py <A|B|C> [--size 1024] [--samples 80] [--out <dir>]` writes `Ellis<X>_color.png`
  (Cycles, transparent) and `_aux.png` (R glow mask, B material id / 16); `node Art/Portraits/render.js Ellis<X> --passes <dir>`
  paints and frames it into `Ellis<X>.png`; `node Art/Portraits/sheet.js <dir>` lays the three on `Sheet.png` (labels in Chakra
  Petch: `git lfs pull --include="Content/UI/Fonts/ChakraPetch-*.ttf"` first in a fresh clone). `<dir>` defaults to
  `Saved/ArtPreviews/Portraits`. 1024 px at 80 samples: ~2.5 min a bust on 4 CPU cores; 600 px at 32 samples ~22 s for looks.
- `Art/Portraits/mh.py` (numpy only): the base mesh shaped by macro sliders (gender 0.5 = neither; Ellis is "they") and face
  modifiers (`FACE` in EllisBlender.py), posed by linear blend skinning with MakeHuman's weights (arms down, head yaw and pitch;
  `glob[bone]` is each bone's posed 4x4). MakeHuman space: decimetres, +y up, +z out of the face; `to_blender()` converts.
- Eyes: the eye helpers carry no weights, so they're taken at rest and moved by `glob["head"]`. The helper runs deep into the
  skull: the ball (radius 1.18 cm) is sized from the helper's front, `EYE_BACK` behind it. Narrowed eyes: `close_lids` slides
  the lid skin over the ball in the head's frame (the lid bones pulled the lids into the socket instead).
- Cloth is draped, not simulated (`drape()`): columns round a vertical axis, each ring offset outside the widest point at or
  above it, so a bandana tents off the nose and a cape hangs off the shoulders. Drape over the clothes underneath too (over the
  body alone the waistcoat pokes through); the down bandana only over points within 9.8 cm of the neck. Lapels, straps and
  ties are strips projected onto the clothes from the front (`strip_on`, `bvh_of` takes several objects).
- Hair and brows are Blender hair curves (`curves_object`): strands drape down the head (`grow`, `grow_combed`) and are pulled
  together toward the tips in clumps, which reads painterly.
- Light: per option in `OPTIONS["lights"]`; the key is an area light close to the head, so its falloff leaves the face lit and
  the shoulders in dusk. Area-light spread keeps the power in a narrower cone (it blew the image out): don't use it.
- `paint.js` turns the render into a painting (Hertzmann's curved brush strokes in four sizes along the picture's edges, the
  face and details given part of the crisp render back, bloom from the glow mask, embers, grain), clipped by `Window.svg`
  under `Frame.svg`. Per-option backgrounds: the `# PAINT <X> {...}` lines in EllisBlender.py.
- The frame: `node Art/Portraits/frame/gen.js` (with `lib.js`) writes `Frame.svg` and `Window.svg`, deterministic. Window: the
  square 96..928 with radius-28 notched corners; ornaments reach into it up to 50 px at top and bottom centre, 30 elsewhere.
  Motifs: Saint Ada's sun-ring, crow feathers, bandolier segments, Pa's lantern, Sexton's ledger, the headboard "ELLIS RANSOM"
  over crossed revolvers, barbed wire.
- What didn't work: busts sculpted as signed-distance shapes and marched in WebGL (the first try; a clay, CG look the user
  passed on for Blender). Removed; the code is in commit 7ed66a0 (`Art/Portraits/*.glsl`).

## Character models (MakeHuman bodies, simulated and draped clothes, baked looks; Ellis, 2026-10-15)

Why: the portraits pick the look (Ellis A, The Revenant); the game model is built the same way, from MakeHuman's CC0 base,
as a scripted model: `Art/Models/Characters/Ellis.py` -> `SK_Ellis`. The user wants to spin it (the spin viewer is a glTF
export of it) and asked for it "less smooth": real folds, creases, seams and wear, not clean shapes. Helpers, for the
other heroes too:
- `Tools/Blender/looter_human.py`: `Body` (MakeHuman shaped by macro and face sliders, sized to a height, standing on the
  origin facing -Y; `lift` raises it onto boot heels). `skeleton()`: the UE5 mannequin's 63 deform bones on MakeHuman's
  joints (pelvis at the hips' middle; spine_01..05 up MakeHuman's spine, the chest's bone halved; clavicle to the shoulder
  joint; finger1 is the thumb, metacarpal1..4 index..pinky). `vertex_weights()`: MakeHuman's own skin weights summed onto
  those bones (spine and hip weights pooled and shared over pelvis..spine_05 by height; every face, eye, jaw and tongue
  bone to `head`), top four. `weights_at(points, faces)`: a garment point takes the weights of the nearest point on the
  body (or on chosen faces). Bind pose: MakeHuman's rest (arms about 49 degrees below level, elbows a little bent).
- `Tools/Blender/looter_garments.py`: `drape()` (cloth round a vertical axis, each column outside the widest point at or above
  each height; `hb`, the wedge a column looks across: 7 degrees bridges the spine's groove and a row of cartridges, 2 left a
  dark seam down the coat's back and bullet tips through it); `shell()` (body faces pushed out along their normals,
  Taubin-smoothed, MakeHuman's UVs kept); `clear()` (pushes points back outside the layers under them); `grid()`, `tube()`,
  `strip()` (UVs in metres; `tube()` faces outward); `solidify()` (plain offset: Blender's even thickness shot vertices metres
  away from sharp corners, the soles' edges and the coat's vent); `simulate()` (Blender cloth, cached, below).
- `Tools/Blender/looter_looks.py`: procedural Cycles looks ending in OUT_COLOR/OUT_ROUGH/OUT_METAL/OUT_NORMAL reroutes:
  oilskin, felt (a sweat stain from the band), leather (creases that crack, scuffs, pores; boots: a stitched toe cap, the
  welt's stitches, scalloped rows on the shaft), wool, brocade, linen, bandana, metal (scratches, dents), walnut (checkered),
  skin (pores, fine wrinkles, veins), scalp, hair, stacked sole, glow. Patterns in object space (metres). Shared touches:
  `ridged()` crease networks (stretched down the body on hanging cloth), `construction()` (hems, topstitching, seams from
  the model's 'Seams' attribute: R closeness to an open edge, G to a sewn seam), `dashes()` (stitches along any line from
  three sines), `stains()`, `splatter()`, dust by height and facing, grime from occlusion, wear from Pointiness (no rays).
  What didn't work: a 380/m twill and 700/m weave aliased into moire at the bake's ~1000 px/m (keep patterns under ~300/m); a
  dense fine crumple network read as cracked stone or marble, not cloth (cloth creases: few, broad, long, soft).
- `Tools/Blender/looter_bake.py`: `pack()` (one atlas per set: islands averaged to one density, packed); `bake_set()`: the
  occlusion on the game parts themselves (Cycles AO bake at a quarter of the set's size, 32 samples, 0.1 m, scaled up into
  the ORM; the detailed parts hidden, or lying on their game parts they shade them black), then colour, roughness and
  metal as emissions at one sample and the normal (tangent, OpenGL; written green-flipped as DirectX), cast from a joined
  copy of the set's game parts onto the detailed parts (`sources`: selected to active, cage 1.5 cm, reach 4 cm); writes
  `Art/Textures/<Set>/T_<Set>_BC|N|ORM.png`. `load_set()` reads a set back; `digest()` fingerprints the scripts. ~5 min for
  four 2048 sets on the cloud box's 4 cores (it was ~15 min at 1024 with ambient-occlusion nodes live in the looks; grime is
  Pointiness now). Any edit to the scripts changes the digest and bakes again.

The duster's cloth simulation (`settle()`, `looter_garments.simulate()`): the skirt is built fuller than it hangs (flare 0.10)
and Blender's cloth lets it fall onto the body, trousers, waistcoat and belt, pinned above the hips (smoothstep 0.92-1.08 m),
90 frames, stiffness tension/compression 40, shear 15, bending 12, mass 0.3, self-collision on (~50 s). Kept in
`Art/Models/Characters/Ellis.drape.json` under a digest of the starting cloth, pins, colliders and settings, so an export
simulates again only when one of them changes (as MisterSexton.drape.json).
- Bugs met: the cloth exploded (points metres away) under self-collision because the start mesh crossed itself: `clear()`
  against the waistcoat's armhole edge had folded the coat under the arms (now it clears against the belt only; the drape
  already rests on the waistcoat). Check with `--selfx` (prints crossing face pairs). The vent's two edges start apart when
  simulated (built, they overlap), and the lantern and holster are not colliders (they started inside the coat's panel).
- The cape is not simulated: on the shoulders it had nowhere to fall but into itself and crumpled. It stays procedural, with
  flutes of uneven depth and spacing. Bending 4 crumpled the skirt and the vent like linen; 12 gives a duster's broad folds.

Ellis.py:
- Order: body, trousers (knee and boot-top creases), boots (toe box, sole and cuban heel, ankle creases), waistcoat and shirt,
  belt (before the duster, which drapes over it), duster, sleeves (bunched at the cuff, crescents at the elbow), cape,
  holster, lantern, the simulation, collar and pin, gloves, hat (EllisBlender.build_hat moved into the figure's space),
  bandana (inside the collar: draped over skin, waistcoat and shirt; its point down the chest), hair (two layers of locks with
  rounded backs round the back of the head, `EllisBlender.grow` over head, collar, cape and bandana; a hair-coloured scalp
  under them), then 'Seams' on the sewn pieces; the game mesh (below), its UVs, the bake from the detailed parts (then
  deleted), thickness, weights, one joined mesh `Ellis`.
- Texture sets (2048): EllisSkin (face and scalp), EllisDuster (coat, sleeves, cape, collar), EllisGarb (hat, hair, bandana,
  waistcoat, shirt, trousers), EllisGear (boots, soles, gloves, belt, holster, revolver, lantern, brass, lead, iron). The eyes
  and the lantern's globe and flame are Glow materials (EllisEyeGlow, EllisLanternGlow).
- Skin weights by part (`weights()`): the body's own; the duster above the hips from the torso, its skirt pelvis to the
  thighs (side by x) as looter_heroes' skirts; the cape half its nearest body point, half spine_05 and the clavicles; collar
  spine_05 to neck_01 by height; bandana head, its point toward neck and chest; hat, hair, eyes head; belt pelvis
  (+spine_01); holster pelvis + thigh_l, lantern pelvis + thigh_r (rigid); gauntlets lowerarm to hand; gloves from the
  hand's faces only. Hit zones: head, trunk, skirt round the hips, each limb, hands, feet. LODs 50 % and 25 %.
- The game mesh, ~20.6k triangles (the user's call, 2026-10-15: Borderlands' 10-20k budget): the detailed build (~108k)
  is only the bake source. `make_lows()` copies each part and decimates it by its `LOW` share (Blender's Decimate,
  collapse); the face spares the eyes (a vertex group, inverted: Decimate collapses only what its group weights); the
  bandana's back, hidden under collar and hair, goes; the hair is built again as fewer, wider locks (`HAIR_LOW`); reduced
  layers are pushed back outside the reduced layers under them (a reduced sleeve cut through its reduced gauntlet). The
  copies take each part's transform (the eyes, buttons, pin and conchos are placed by their object, not their mesh: copies
  without it fell to the floor). Biggest shares: face 2.6k, duster 1.8k, hair 1.8k, gloves 1.1k, cape 1.1k, boots 1.1k.
  `--stats` prints the triangles per part (no bake).
- Bake cache: `Art/Textures/EllisSkin/bake.digest` holds the digest of the scripts, the size and the Blender version the sets
  were baked from; a run with the same digest reads the PNGs back instead of baking (`--rebake` forces it).
- The cloud box can't push Git LFS: its PNGs are not committed (`.git/info/exclude` there). The PC baked them on
  2026-10-15 with Blender 4.4 (the cloud wrote the scripts against 5.2's module: they ran unchanged; ~25 min for build,
  simulation and four 2048 sets beside the editor) and they're committed. MakeHuman's base.obj (1.75 MB, gitignored) is
  fetched when missing: `Art/ThirdParty/MakeHuman/fetch.py` (the PC has no system Python: run it with Blender's,
  `C:\Program Files\Blender Foundation\Blender 4.4\4.4\python\bin\python.exe -I`). Export beside the editor:
  `Tools\artrun.ps1 -Export Ellis -Family Ellis`, then `Looter.ImportModels <that folder>`: `/Game/Art/Characters/SK_Ellis`
  (64 bones with the root, 3 LODs, Nanite off for the eye glow, 185.5 cm to the hat's top), `MI_Ellis*`, `PA_Ellis`.
- Run: `blender -b --factory-startup --python Art/Models/Characters/Ellis.py -- [--size 1024] [--glb <path>] [--preview]
  [--shots full+upper+hips+feet] [--shape] [--sim-only] [--selfx] [--no-sim] [--rebake]`. `--shape` renders the untextured
  build in clay (the looks unbaked are minutes a frame); `--preview` renders the baked look in the viewer's arms-down pose
  to `Saved/ArtPreviews/Characters/Ellis/`; `--glb` writes the posed, textured figure (WebP maps) for the spin viewer.
- The spin viewer (`Tools/CharacterViewer/index.html`, three.js 0.160 from jsDelivr, the game's LooterUI look): published as
  an artifact (https://claude.ai/artifact/HJCgTaXnSRbEb5k7SXjdDa). Artifacts serve no .glb (nor model/gltf-binary): convert with
  `Tools/CharacterViewer/glb_to_json.py <glb> <dir> ellis` and publish index.html with ellis.json and the ellis_N.webp maps.
  The page reads the `LanternAnchor` node for its lantern light. Check it locally with Playwright's Chromium
  (`--use-gl=swiftshader`), the import map pointed at a local three.js.

Ellis as the player's body (the user, 2026-10-15; their pick "retarget live" over retargeting every clip onto him):
- The mannequin (`GetMesh()`, SKM_Manny_Simple with ABP_Unarmed/ABP_Rifle and the C++ stance layer) stays the animated
  body, hidden: guns, hand sockets, the hands' IK, the jar, footsteps and every test still use it. `UBodyFigureComponent`
  (`Figure` on ALooterCharacter, attached to the mesh) wears SK_Ellis and copies its pose each frame through
  `RTG_MannyToEllis` with `UBodyRetargetAnimInstance`: the engine's Retarget Pose From Mesh node as a native anim
  instance's root (`GetCustomRootNode`; its game-thread `PreUpdate` called by the proxy, since only a Blueprint graph calls
  it for you; the node value-initialized, its counters have no initializers). Ellis ticks after the mannequin (tick
  prerequisite) so the hands don't lag the guns, and mirrors its first/third-person drawing (`bOwnerNoSee`,
  `FirstPersonPrimitiveType`) each tick: invisible to the player in first person, his shadow cast. Without SK_Ellis or the
  retargeter the mannequin shows as before. `ALoadoutStage` wears him too (unused since the redesign).
- `Tools/Unreal/build_ellis_retarget.py` makes `/Game/Player/Retarget/IK_Manny`, `IK_Ellis` (the engine's auto chains: the
  same 29 on both, fingers too) and the retargeter: default ops (Pelvis Motion, FK Chains, Run IK Rig, Root Motion, Remap
  Curves) plus Blend to Source on LeftArm and RightArm (hands go to the mannequin's, the arms reach them by IK: before it
  his hands missed the rifle). Lessons: SK_Ellis faces +X, the mannequin +Y: the retarget pose turns his root 90 degrees
  and nothing else, as the engine's auto align compared them unturned and tipped his head ~25 degrees sideways (both
  rest poses are close enough: arms down, the neck a little forward). The script rebuilds the assets in place: deleting
  and recreating the retargeter nulled the character's pointer and dirtied BP_LooterCharacter (a Save prompt blocked
  `launch.ps1 -Build`), and the component then also loads the assets again at play if its pointers are empty.
- Checking in PIE: `play.py view [n]` cycles first/third/front views (F5); `play.py inventory [close]`; toggling the
  mannequin back on (its `set_hidden_in_game`) compares poses. Keys from `input.ps1` (Tab, number keys) didn't reach the
  game on 2026-10-15, so both have play.py steps.
- Not done: his holster keeps its revolver while the revolver is in hand (one mesh: hiding it needs its own material slot
  or section); the first-person arms (SK_FP_Arms) are lighter than his duster and gloves (the user's eye decides).

## Island styles (Style Lab looks on the tutorial island, 2026-10-08)

- The user's call (2026-10-08): "I don't want to change the style." The look stays as it is; no style work unless the user asks.
  Style 3 (Painted Frontier) was rejected and removed, style 6 (Sunbleached) undone, style 4 (Inkslinger) stopped before any code.
  The island wears today's look. Trials skip tests and screenshots (the user's call); full checks only when a style goes into the
  game.
- Tools that exist, if asked again: `Tools/Unreal/build_island_style.py`, `style_common.py` (shared helpers), `style_sunbleached.py`
  (its numbers), `Art/Levels/TutorialIsland/style_sunbleached.json` (the island's settings),
  `/Game/Art/Materials/IslandStyle/Sunbleached/` (its island-only MIs). A style = a static switch on the masters, off by default (so
  Sunbleached costs nothing now), plus any post material (`build_world_materials.py`; `styled()` chains one style's switch inside
  another's).
- Apply: `build_world_materials.py M_World M_WorldFoliage M_Terrain M_SkyClouds M_SunbleachedPost`, then
  `build_island_style.py sunbleached apply`: island-only instances for every slot on those masters, light filters, and a
  `<Style>Post` volume (tagged IslandStyle) above `IslandPost`; recorded in `style_applied.json` (commit it with the level). `undo`
  restores everything; `status` reports what's styled. One style at a time (the script refuses otherwise). After an island rebuild
  or a new scatter, run `apply` again. To drop a style, delete its `IslandStyle/<Style>` folder once the island no longer wears it.
- Lighting: the island's Day must stay the default (`Looter.World.Lighting.Levels`). A style's own sun waits in its json's `state`
  block until `LightingTests.cpp` skips the default-Day check on a level with an `IslandStyle` actor (one line).
- Lessons from style 3: rim terms on alpha-tested leaves speckle unless they read the crown's vertex normal; a screen-space edge pass
  cost the most (+0.8-1.0 ms in all).

## Sounds (script-made, 2026-10-08)

- The user's rule: sounds are synthesized from scratch, like the scripted models: no recordings, libraries, AI generators or new
  programs. Blender 4.4's bundled Python has numpy (no system Python on this PC).
- Make: `Art/Sounds/lib` (synthesis library), `Art/Sounds/recipes/<group>.py` (one recipe per cue, seeded takes). `Tools\sounds.ps1
  [cue patterns]` renders `Art/Sounds/Out/<Cue>_NN.wav` and `cues.json` (each cue's mix) and runs `check.py` (peaks, DC, clicks, loop
  seams, harsh bands; spectrograms and a listening page in `Saved/SoundCheck/`). Agents can't hear: they judge by numbers and
  pictures; the user listens.
- Into the game: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_sound_bank.py [check]"` imports the WAVs to
  `/Game/Audio`, makes the classes (under `SC_Master`), the mix and attenuations, and fills `DA_SoundBank`.
- In code: `LooterSound::PlayAt/PlayAttached/Play2D/Start/Stop` by cue name (`Audio/LooterSoundCues.h`). A cue without sounds plays
  nothing, so code can call new cues first. Add a cue to the header and to `cues.json` together.
- Groups (each a recipe file listed in `recipes/__init__.py`'s `GROUPS`; an unlisted file is never rendered). Cue name namespaces:
  `LooterSoundCue::Lootables`, `::Throwable`, `::Voice`, `::TownLife`, `::Revolver`, `::Economy`, `::BellRinger`, `::Shrine`,
  `::Spitter`; `WeaponSounds.h` picks a gun's cues by its kind.
  - 2026-10-09: `props` (the lootable world), `throwables` (the grave-salt grenade), `voices` (the Unpaid's murmured barks, one
    syllable a cue; the spiders' and slimes' idle calls; the living heard through walls on Ransom's Rest). The Drover's shots and
    reload steps are in `guns`, the fast-travel whoosh in `ui`.
  - 2026-10-12: `economy` (silver found, 2D and the player's own: `Loot.Silver.Pickup`; vendor: `Vendor.Buy`, `Vendor.Sell`,
    `Vendor.NotEnough`), `bells` (the Bell-Ringer's bronze hand bell), `shrines` (the Vigil Stones; the iron bell is `bells.py`'s hand
    bell made duller, so `shrines.py` imports from it). The Venom Spitter's cues are in `creatures`. The pickup chime keeps the
    2-5 kHz band cut, since a fight's worth of pickups sounds it over and over.
  - 2026-10-13 (69 cues): `givers`, `graves` (a grave marked as the save point), `holds` (the hold ring's completion, kept in the
    frequent-cue 2-5 kHz check), `lostloot`, `missions`, `orelift`, `watchfire`, `miners` (the dead miners; Hiram's and Thaddeus's
    sounds), `foreman`. Stamps on the cards land 0.11 s in, on the cards' slam.
  - 2026-10-14 (334 cues in all): `animals` (`::Animals`), `gusts` (`::Gust`), `windows` (`::Window`), `reactions`
    (`::React`: notice, emerge, twitch, bubble, swell, ash), `brass` (`::Casing` by surface, `::FlyBy` whiz and venom hiss);
    `player` adds `Player.Land.Hard`, `Player.Splash` (`::Footfall`).
  - Lessons (`Art/Sounds/README.md`): start soft cues on a small physical touch (the check flags a late start); put a loop's seam in a
    lull between events; mix loop layers by loudness, not peak. CLICK compares spikes with the local level 30 dB under the
    cue's peak: a crackle thinning to a few grains a second flags in its tail (keep its rate floor high: Spitter Sizzle 1500/s).
- For the user: the listening page is an Artifact with the WAVs as its files (<https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa>);
  republish it from the session's scratchpad copy after a render.
- Lesson: setting a USoundClass's `child_classes` all at once parents only the first new child; add them one at a time
  (`build_sound_bank.py` does).

## Packaging a test build

- Run: close the editor (`Tools\close.ps1`; no game running either), then start `Tools\package.ps1` in the background.
  - Options: `-Clean` (full rebuild and recook), `-NoZip`, `-DebugInfo` (keep .pdb files), `-OutDir <folder>`.
  - The first run is long (every shader compiles for D3D12 SM6 and D3D11 SM5); later ones reuse the derived data cache.
  - Progress: `Saved\Logs\Package.log` (UAT's whole log). A failure prints the first errors.
- It makes `Saved\Packaging\<date-time>\Windows\` (run `AI_Looter_Shooter.exe`) and beside it
  `AI_Looter_Shooter_Test_<date-time>.zip`; the script prints the zip's path, size and time taken. A Windows Development build: logs
  and the console (`~`, the `Looter.*` commands) stay in. `-prereqs` adds the Visual C++ runtime installer.
- Cooked (`Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`): `Lvl_TutorialIsland` (also the menu's map) and
  `Lvl_RansomsRest` with everything they refer to, plus the `DirectoriesToAlwaysCook`. Left out: `Lvl_Skyreach`, `Maps/Dev`,
  `Developers`, the Skyreach props, editor content. `LooterEditor` is an Editor module; the MCP plugins (`ModelContextProtocol`,
  `AllToolsets`) are Editor-only in the .uproject.
- Rule: a path in C++ is not a reference. The cooker follows references only. A `LoadObject`, `FindIfMade`,
  `TSoftObjectPtr(FSoftObjectPath(...))` or `TryLoadClass` on an asset nothing else refers to finds nothing in a package, and the
  `FindIfMade` ones quietly draw a cube instead. Keep such assets under a directory in `DirectoriesToAlwaysCook` (or add its
  directory). After a new package, look for stand-in shapes: the jetty, depot, train, egg sacs, lanterns, chests, fonts.
- Share: a Drive, OneDrive or Dropbox link to the zip (no new upload tool without the user's OK). The friend unzips somewhere
  writable (not Program Files) and runs `AI_Looter_Shooter.exe`; the first launch compiles pipeline states while the level loads. A
  GPU without D3D12 SM6 starts it with `-dx11`.
- Saves and logs sit beside the game, not in AppData: a packaged Development build isn't an "installed" one, so they go to
  `<unzipped>\AI_Looter_Shooter\Saved\` (SaveGames, Logs, Crashes). Ask a tester for the log after a bug.

## The Style Lab (art styles previewed in the game's scene, without Unreal)

- `Tools/StyleLab/` (README there): a three.js rebuild of a piece of Ransom's Rest from the game's own sources with the in-game HUD
  over it, so art styles can be judged "in game" from a cloud session with no Unreal. Built 2026-10-08: ten styles, the user's picks
  3, 4, 5, 6 (3 and 6 were tried on the island). Style 0 is today's look.
- `Docs/Art/StyleLab/Catalog.md`: all ten styles with Unreal-terms numbers, recipe, cost, refs; contact sheets in
  `Docs/Art/StyleLab/Sheets/`; briefs and research beside it.
- Export: `Tools/StyleLab/export/export_all.sh` (Blender's `bpy` 4.5 module under Python 3.11); `assemble.py` builds
  `Saved/StyleLab/site/`. The artifact host serves no `.glb` or `.bin`, so models and heights go as base64 JSON (`--b64-glb`; packs
  under about 9.5 MB so each JSON stays under 16 MB); a publish over 64 MB goes in two calls.
- Cloud sessions commit text only (see the bug list: LFS uploads are refused); binaries live under `Saved/StyleLab/`.

## Performance

- Target, rules and logged numbers: `Docs/Performance.md`. Per-pass budgets: `Docs/TutorialIsland.md`. Lumen and Nanite are switched
  by `UGraphicsSettingsSubsystem::QualitySettings`.
- `perf.ps1 -GpuStats` records each pass; `Tools\perfdiff.ps1` compares two captures; `-Map` measures another level.
- Measure with the editor closed and Blender paused (create `Saved\ArtPause.flag`, delete it after):
  - `Tools\tour.ps1` for every view;
  - `Tools\perf.ps1 -Label ... -Exec "Looter.Quality Medium" -Map <map>`, which appends a row to `Docs/Performance.md`.
- Per pass: `perf.ps1 -GpuStats -Exec "Looter.Quality Medium,Looter.Tour <views.json> noshots" -Frames 16000 -Skip 0 -NoRecord`, then
  `Tools\perfviews.ps1 <csv>`. Pass the views file: without it, `Looter.Tour` walks the tutorial island's list. `-Pattern
  'Exclusive/RenderThread'` lists the render thread's work.
- What a group costs: a view file of its own under `Saved/` (any path under the project) with each view twice, once with
  `"exec": ["Looter.Perf.HideFolder RansomsRest/<folder> 1"]`, `"after": [... 0]`; `Looter.Perf.DrawShares <view>` in a view's
  `after` lists what it drew by outliner folder and mesh (`Saved/Tour/DrawShares.csv`); `ProfileGPU` in `after` dumps one
  frame's passes to the log (the frame as the tour leaves: can miss spikes; use the CSV capture for those);
  `r.LumenScene.DumpStats 2` dumps Lumen's scene (cards, objects, surface cache).
- A view measured right after a jump includes the renderer's catch-up: list a view twice (`<name>_Stay`, same place) to time
  it standing still.
- Lesson (2026-10-14): `Looter.Quality` in a tour or perf run saves the preset to the project's `Saved/SaveGames`, the same file
  the user's game reads: end every batch of runs on Medium (or the user's own preset).

## Tests

- `Tools\runtests.ps1` runs one batch per test group, with `obj gc` between batches (all in one batch let test worlds' GPU buffers
  pile up until the driver removed the device).
- Test worlds are editor worlds. Traps:
  - A movement component gets no capsule there: call `SetUpdatedComponent` and set the movement mode yourself.
  - Blueprint native events are dropped: open an `FEditorScriptExecutionGuard`. See `Tests/LocomotionTestWorld.h` and
    `BossTestWorld.h`.
  - Damage through `UGameplayStatics::Apply*Damage` is dropped before play begins: broadcast the actor's damage events the way
    `BossTestWorld::Hurt` does.
- A test's name must not be the start of another test's name (`Looter.Economy.Offer` beside `Looter.Economy.Offer.Odds`;
  `Looter.Locomotion.Pace` beside `.Pace.Chases`, fixed by naming it `.Pace.Speeds`): the runner takes it for a group and reports it
  "Not Run" (happened twice on 2026-10-13). Name every test as a leaf.

## Bugs and how they were fixed

Each line: bug (date). Cause. Fix.

- Game crashed at start in Enhanced Input ("Unhandled tracking mode", 2026-10-09) though the build and every test passed. Cause: an
  agent added members to `KeyBindingSubsystem.h` mid-build, so files compiled against the old header were never rebuilt and read
  `CharacterContext` at the old offset (an input action). Fix: touch every changed header before the integration build; no builds
  while agents edit headers; after each integration build start the game once (`menushots.ps1`/`hudshots.ps1`): tests don't add
  input contexts.
- The tutorial island's dressing build put 38 of Ransom's Rest's pieces on it (graves, dead trees, bales, fallen pines). Cause:
  `build_area_dressing`'s `EXTRA_GRAVE_ROWS` and `SPOTS` applied to every layout. Fix: only on Ransom's Rest (`EXTRAS_AREA`).
- Skyreach's scattered trees all vanished with the town. Cause: Web Hollow's no-tree clearing sits on the forest rise's middle, where
  the scatter's woods grew densest. Fix: the concept's woods round the hollow placed as town pieces (`make_town.py`).
- A bench blocked the forest road at the town square. Cause: the concept's two memorial benches were exempt from the road check (the
  plaza core, 6.5 m). Fix: only the memorial itself is exempt (3 m); the path probe found it.
- The view jerked at a slide's start and end (the user, 2026-10-08). Cause: the engine's crouch sets the body mesh's relative Z
  without moving it, so the camera (on the body) sat ~24 cm off for a frame after a stand outside the movement tick; in the air a
  crouch resizes the capsule about its middle; two added smoothsteps of different lengths; alphas that reversed speed in one frame.
  Fix: `RefreshBodyTransform` each tick and after our own stand; the eye tracks its height above the feet on one minimum-jerk curve
  (`FViewEase`, re-planned from speed and acceleration), as do the roll and stance blends; `Looter.Locomotion.Slide.View` checks
  every frame of seven endings.
- The slide stopped the player abruptly. Cause: a straight 0.2 s ease to crouch speed. Fix: a smooth ease to the next speed; forward
  held goes back to the sprint (73aa50f).
- The cloud session's commit with PNGs and a font would not push. Cause: the cloud refuses Git LFS uploads (`lfs.github.com` verify:
  Forbidden). Fix: from the cloud, commit text only: inline small assets as data URIs (`Tools/StyleLab/web/hud/assets.js`), keep
  generated binaries in `Saved/`.
- The Style Lab's trees had black fringes, the valley's grass turned yellow-red. Cause: Pillow's `resize()` premultiplies RGBA, so
  colour under alpha 0 was lost; lossy WebP drops it too. Fix: resize RGB and alpha separately, save cut-outs lossless with
  `exact=True`, write macro maps as RGB with the alpha in its own file.
- The Style Lab's aerial views had horizontal stripes on far terrain. Cause: GTAO read the depth buffer's coarse far steps as
  terraces, and a fixed shadow bias drew acne in the wide far cascade. Fix: AO allows for the depth step and samples wider far away
  (near plane 0.12 m); shadow lookups offset along the surface normal, more on slopes turned from the sun.
- The editor crashed in the boss bar test (HUD upgrade). Cause: `Outline.Add(Outline[0])`; TArray asserts when it adds a reference
  to its own element (the add may reallocate). Fix: add a copy, `Outline.Add(FVector2D(Outline[0]))`. Grep new code for `X.Add(X[`
  before building, recursively: Windows PowerShell's `Select-String -Path Source\**\*.cpp` is not recursive and missed the cracked
  coin's `Points.Add(Points[0])` (2026-10-08). Use `Get-ChildItem Source -Recurse -Include *.cpp,*.h |
  Select-String '(\w+)\.(Add|Emplace)\(\s*\1\['`.
- HUD text came out a third bigger than the mockup. Cause: Slate sets a font's Size in points at 96 DPI (Size 18 draws 24 px); the
  mockups use CSS px. Kept: the user chose the larger text (2026-10-07), so mockup px stay the Slate Size; px × 0.75 would match.
- Keycap corners drew twice their size; the boss hatch leaned 26°. Cause: Slate sizes a Box brush's ends and repeats a tiled brush in
  the texture's own texels, not by `ImageSize`. Fix: make such textures at 1 texel per slate unit (`PaintedIconBrush` at
  PixelsPerUnit 1).
- The first test build failed to compile (11 errors). Cause: `ADirectionalLight::GetComponent()` and
  `UStaticMesh::IsNaniteEnabled()` exist only in editor builds; the lighting code read the sun through the first, so a packaged game
  would have broken. Fix: `Cast<UDirectionalLightComponent>(Sun->GetLightComponent())`; the test's Nanite check under `WITH_EDITOR`.
  Editor builds never catch these: package to find them.
- Editor automation plugins would have shipped in the game. Cause: `ModelContextProtocol` and `AllToolsets` had no `TargetAllowList`
  (one is marked NoRedist). Fix: `"TargetAllowList": ["Editor"]` in the .uproject.
- The first cook stopped: "Content is missing from cook". Cause: the engine's Landmass plugin loads startup materials that refer to
  `/Engine/EditorMaterials`, which `-SkipCookingEditorContent` marks never-cook. Fix: the flag dropped from `package.ps1` and the ini
  (`bSkipEditorContent=False`).
- `Tools\close.ps1` said "EDITOR DID NOT CLOSE" with no dialog. Cause: a floating Content Browser held the process's main window, so
  closing it only closed the panel. Fix: `close.ps1` asks the editor to `QUIT_EDITOR` when the main window isn't the editor.
- A hit tinted the whole screen red. Cause: the old hurt camera fade (`PlayerVitalsSubsystem`, up to 55%) on top of the new red
  edges. Fix: the fade on hits removed; the HUD shows hits (edges, portrait, chip); death and respawn fades kept.
- The gun's name stopped short of the right edge, its gem far from it. Cause: a scale box fitted the name to the 24 px line's height
  (shrinking every name) and centred the rest. Fix: no scale box; `FitWeaponName` measures a long name and sets it smaller.
- The Gravemother spawned stuck in her den's wall. Cause: the den's floor is part of Den Rock, tagged Obstacle, so its spots were
  refused; the fallback spot was checked with a man-sized capsule. Fix: room for the largest body; the floor inside a lair's own rock
  counts as ground (`FEncounterGroundProbe`), f3e3579.
- A giant rock blocked the tutorial island's lookout ramp. Cause: a cliff run's end piece, stretched to meet its one neighbour,
  reached across the ramp. Fix: walkways cleared with a capsule test (8e0e50c, bd96147).
- A lump came through the burial deck. Cause: the Rim's top cliff course stood under the deck's open end. Fix: `level.cliffs.under`
  cuts pieces 40 cm under the boards (732b43d).
- The Sink's walls became floating rock fragments. Cause: faces brought 50 cm proud through a noisy, curving wall. Fix: old placement
  for theSink; then the art's narrow panels (de5a8fe).
- The egg sacs couldn't be shot. Cause: a new sampling constant reused the name `SAC_STEP` (the mission step). Fix: renamed
  `SAC_PROBE_STEP`; the Keeper's Lantern test caught it.
- Hillside scrub read as even dots. Cause: thick patches asked for more plants than the candidate spacing held, so it drew a grid;
  the 10 m patches averaged out at range. Fix: 1.5 m spacing, 15-40 m patches, varied sizes (7c305cc).
- Spiders caught a walking player. Cause: the player got 15% slower and spider chase wasn't scaled with it. Fix: creature speeds
  times `SpeedScale` (6b03dff).
- Fence panels leaned on slopes. Cause: every section rolled to the slope. Fix: plumb level sections stepped down slopes (c49039f).
- The scatter script failed in a fresh editor. Cause: it imported a neighbour module, relying on `build_area.py` having added its
  folder to the path. Fix: it adds its own folder now.
- Gravewind blades drew black. Cause: the material lacked the instanced-mesh usage flag. Fix: flag set (3a04a48).
- Saves failed quietly. Cause: the user's standalone game held the content files. Fix: check for a running game before editor work.
- After Save & Quit then Continue, no damage numbers, creature tags or loot labels (the user, 2026-10-13). Cause: they draw on the
  engine's `WidgetComponentScreenLayer`, which the game viewport keeps per player for the whole run; the pause menu collapsed it,
  Save & Quit left from there, and the next level's HUD remembered "shown" for itself (`bWorldLabelsVisible`) so never showed it
  again. A trip from the map page or a station board did the same. Fix: `ALooterHUD::ShowWorldLabels` sets it from what the layer
  shows now, every frame, and at BeginPlay/EndPlay (`Looter.LevelReload.WorldLabels`). Rule: anything kept by the viewport, local
  player or game instance (layers, input mode, cursor, mixes) outlives the level; set it from its real state, never a per-level copy.
- After Save & Quit, hostiles stopped showing on the minimap (2026-10-13). Cause: its 32 markers were handed out in drawing order
  (ammo, loot, hostiles), and every Continue gave every ammo and silver pickup a fresh 5 minutes, so a player saving every few
  minutes kept every drop round the fights (a log: 59 to 260 ammo pickups over six loads). Fix: hostiles get markers first
  (`SplitMarkers`, `Looter.UI.Minimap.MarkerPool`); the session keeps each pickup's time left (`LifeLeft`,
  `Looter.LevelReload.LootLife`).

- Tilly's cellar's dead spawned on the yard over it (2026-10-14 play-through). Cause: an encounter's home was looked for
  from 3 m over the spawner (over the vault's collision) and its spots from a step and a metre over home. Fix: both from
  under any roof over the spawner (`FEncounterGroundProbe::FindRoofOver`), wander goals too; `Looter.Encounters.Cellar`.
- 32 cliff slabs stood in a line on the valley floor, through the depot and over the rail line (2026-10-14). Cause: an
  `onFloor` ridge's band ran on past its floor. Fix: `level.cliffs.walled` and `keepOut` (above); generator side done
  the same day (`Region.band_height` x `floor_weight`).
- Dead Lift's crank couldn't be reached (2026-10-14). Cause: the sheriff's cell was one solid collision box. Fix: hulls
  for the cell's floor, walls, ceiling and bunk. Lesson: a model's hulls must leave every room a mission sends the
  player into open; walk it in PIE.
- Abel's beaten boss bar stayed on screen for good (2026-10-14). Cause: its 3 s hide counted down in the boss
  component's tick, which seating him on his board turns off. Fix: a tick turned off hides the bar; a bar steps aside
  100 m from its fight.
- Every notice board posting read NO REWARD: IT'S PRACTICE (2026-10-14). Cause: the line only knew gun rewards. Fix:
  the mission's own reward summary.
- The player's guns vanished twice in the play-through (2026-10-14). Cause: play.py's teleport (see Play checks). Fix: the
  tool; the game warns whenever guns go from some to none outside the player's own drop, sale, clear or load
  (`Inventory/WeaponManagerWatch.cpp`, `Looter.Inventory.GunsLostAloud`).
- From the Diggings' yard the mine's mouth showed daylight and floating trees (2026-10-14). Cause: outside, every mine
  piece was hidden, so the mouth looked through the terrain's hole. Fix: the adit's first 29 m stay drawn from outside and
  a black card across its far end (`AInteriorVisibility::VeilTag`) shows only while the view is outside.
- Ruth Calder's note's card lost the Interact key to the paper under it (2026-10-14). Cause: `InteractionFocus::ScoreOf`
  scores distance as a share of reach; both 250, the paper 0.7 cm nearer. Fix: artifact reach 3 m, the card 2 cm wider,
  tagged CalderNote so Side 1 counts through it; the board offers a job pinned on its face ("pinned to this board").
- Board turn-ins said nothing; Ellis's headboard was a speaker point (Main 1's optional never counted); the valley props
  script skipped lasting-interact steps (2026-10-14). Fixes: `ANoticeBoard::TurnIn` plays the turn-in lines; the headboard
  is an `AReadableProp`; the step finder counts `MissionLastingInteractObjective`.
- Save version 3 (2026-10-14): a step-list change under live saves needs an entry in `CampaignStepMigration`'s
  `MovedSteps` and a cutoff time (Main 4: old 0-4 -> 0,1,3,4,5; sides that had started by themselves count as taken).
- A searching creature blocked fast travel and held the combat music (2026-10-14). Cause: both asked whether its target
  was the player. Fix: `ACreatureBase::IsFightingWith` (Chase or Attack); a lair Legendary's boss mood still follows
  'after the player'.
- The ore bin's trestle end stood 3.1 m over the Diggings yard (2026-10-13). Cause: its model wants a 3.6 m step. Fix:
  `oreBinBay`, an exact pad at +54 cut into the yard's south edge. Lesson: a model with a built-in step gets the step as a
  pad in the terrain, not a placement Z (sinking it buries the model).
- High and Epic "absolutely tanked" (the user, 2026-10-14): 1-22 fps with 1.3 s frames on the RX 580. Cause: Lumen's
  surface cache kept recapturing the ~66k small scatter instances' cards (`LumenSceneUpdate`). Fix: `ULumenScatterSubsystem`
  leaves instanced meshes under 1.5 m out of Lumen and the distance fields; High drops Lumen (the user's pick: a middle step
  on their card). See `Docs/Performance.md`.
- The mouse wheel changed guns the wrong way (the user, 2026-10-14). Cause: wheel up was the next slot (2 -> 3), but the
  HUD's column puts slot 1 on top, so up went down the column. Fix: `KeyBindingSubsystem` defaults: wheel up = previous.
- The crosshair showed while aiming down sights (2026-10-14). Fix: it fades out while aiming in every view
  (`PlayerHUDWidgetCrosshair.cpp`); third person keeps a centre dot (no sight to look through).
- Crouched, the sight sat off the line of fire (2026-10-14). Cause: the crouch's stance pose only settled to 15% under the
  sight. Fix: stance poses fade out with the aim and the sight's aim point is held on the line of sight
  (`Looter.Weapons.Aim.CrouchAligned`; its test needed `SpawnPlayerCameraManager`: a test level skips
  `PostInitializeComponents`, where a controller makes its camera manager).
- An ensure at editor exit on Hiram's figure ("SkinnedAsset property overwritten ... without a call to
  NotifyIfSkinnedAssetChanged", 2026-10-14). Cause: six constructors (the dead friends, Amos, Abel on board, Hob, the barn
  cat, the hens) set a poseable mesh's asset with the bare `SetSkinnedAsset`, which skips the editor's notice; an ensure
  fires once per session, so only the first showed. Fix: `SetSkinnedAssetAndUpdate(Model, false)` (`SetSkeletalMeshAsset` on
  a skeletal mesh): it notices, and does nothing more before the component registers. Never the bare setter.

## What worked, and what didn't

- Worked:
  - Probes before fixes. A read-only script that logs real numbers (gap depths, overlaps, coverage) settled every look problem
    faster than guessing.
  - Dry runs in plain Python (a mocked `unreal` and the generator's heights) catch logic errors before the editor. They miss
    everything that needs real geometry, so always tour after.
  - Data-driven rules in `layout.json`, so another area can reuse them.
  - One commit per step, and partial CODEMAP staging.
  - Asking the art session for its call on looks, with in-game shots.
  - Settings that preview live under the pause menu (2026-10-12): a paused world doesn't tick the player controller's camera
    updates, so a camera setting set from the menu shows only once the game runs again.
    - The field of view slider calls `UPlayerViewComponent::PreviewFieldOfView`, which sets the camera and locks
      `APlayerCameraManager::SetFOV` to it until the first frame that runs.
    - The menu's dimming is its own layer so the slider can fade it and ghost the panel (`SettingsMenuPreview.cpp`).
    - The HUD's minimap is collapsed under the menu, so its zoom preview is a second `UHudMinimapWidget` (`SetPreview`) in the same
      corner.
    - The previews count a controller's capture and a held key's repeated moves as holding, so a slider stepped from the keyboard or
      a gamepad shows its preview while it is held.
  - Menus for a controller and the keyboard (2026-10-12), the main menu's and the settings menu's alike:
    - `FMenuNavigator` (`UI/Menus/MenuNavigator.h`) keeps the focus over a menu's stops (a button, a slider, a row of choices) in
      rows, which the menu hands it with `SetRows` each time it rebuilds.
    - `MenuNav` has the keys as plain rules: arrows, W A S D, the D-pad and the left stick move; Enter, Space, E and A pick; Esc and
      B back out; Page Up and Down or the bumpers turn the sections; a slider held steps faster.
    - `SettingsMenuNav.cpp` and `MainMenuNav.cpp` wire each menu to it. A new menu gets the same by building its stops and calling
      the navigator, not by handling keys itself.
    - The focus look (`LooterUIFocus.h`: an orange outline doubled inside, a lit edge on the left, a faint wash on a row) is drawn
      only while the last input in a menu was a key or the gamepad (`LooterUI::IsMenuFocusShown`); under the mouse the buttons keep
      their hover look. The prompts along the bottom name the gamepad's buttons or the keyboard's keys by `IsMenuGamepad`. Give a new
      row its look with `LooterUI::AddFocusFrame` while building.
    - Tests: `MenuNavTests.cpp`.
- Didn't work:
  - Pushing generic cliff faces proud of a noisy pit wall: they poked through as fragments.
  - Level-wide material swaps for one actor (use `swapsOn`).
  - Judging scrub in stand-in renders at Blender culls; preview at the game's Medium cull distances instead.
  - Line-by-line `perl -pi` or regex edits with `\n` in the pattern on CRLF source files: they match nothing and fail silently. Use
    the Edit tool, or allow `\r?` before the newline, and check the result.
  - A file-scope name in a test (`const FName Extra`) that an engine header also uses: C4459 fails the build.
  - A foreground build or test run longer than the tool's 10 minutes is killed with no log: run them in the background.
  - Chaining `git add -u && ... ; git commit`: when `git add` failed on the transient "Permission denied", the commit still ran with
    whatever was staged. Stage, check, then commit. Commit scripts (`Tools/commitlib.py`) run in a retry loop: `git reset -q`,
    wait 5 s, run again (up to 5 tries). They run on Blender's Python (`C:/Program Files/Blender Foundation/Blender
    4.4/4.4/python/bin/python.exe`); `python` here is the Microsoft Store stub.
  - Starting a build without checking whether the user was playing (step 1 of the loop).
  - Keys sent with input.ps1 right after console.ps1 typed into the editor's console box (its autocomplete list stayed
    open over the view): click into the game view first (`clickat 0.5 0.45`; it fires one shot) before keys.
  - Every console.ps1 command leaves the editor's console box focused: click into the game (`clickat`) before each
    burst of keys, or the keys type into the box. Watch short motions (swaps, reloads, deaths) at `slomo 0.2`.
  - A floating Content Browser (opened by a script's import) takes the keyboard from PIE: close it with
    `Tools\closemsglog.ps1 -Title "Content Browser"` before input.
