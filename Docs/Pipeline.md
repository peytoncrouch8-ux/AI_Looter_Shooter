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
  at the side one-handed; the upper body keeps ~44% of the jog's bounce. The mannequin is animated hidden; Ellis wears
  its pose (2026-10-15, "Character models", "Ellis as the player's body").
- The idle (2026-10-15, the user's picks: "Weary revenant" with breathing, glances around and hand fidgets; the armed
  stance kept; `LooterStancePoseIdle.cpp`, on the mannequin before the crouch, so Ellis copies it): standing still
  (under 10 cm/s, grounded, not crouched, sprinting, sliding or climbing) and unarmed it eases in over 1.2 s, out over
  0.25 s. Its own clock (`DescribeIdle`, the same moment always the same idle): the weight from leg to leg (18 s round,
  held near each end; the hips 2.5 cm over the standing leg, the free hip 3 degrees down, the chest 2 back, the legs
  solved back onto the planted feet), weary (chest 4 degrees forward, shoulders rounded 5, neck 7 down, head 2 back up),
  a breath every 4.2 s (chest 0.8, shoulders 1.3), a glance aside in about three of four 10 s spells (18-35 degrees,
  neck 40%), a finger flex now and then (22 degrees a joint toward the palm), the right thumb in the belt for 5-9 s in
  about two of three 24 s spells (the wrist 15 out, 12 ahead, 8 up from the hips; fingers down, palm to the body).
  First person is unaffected (the arms ride the capsule). Tests `Looter.PlayerAnim.Idle.*`.
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
- On this PC (the user, 2026-10-16: "Install Node and Playwright on this PC"): Blender 4.4's own binary runs the bust
  scripts (no bpy module needed); Node 22 LTS is the official zip from nodejs.org unpacked to `%LOCALAPPDATA%\Programs\node`
  (no admin; on the user PATH; a new shell sees it, an old one needs the folder prepended), Playwright comes from the
  root `package.json` (`npm install`, `node_modules/` ignored) and its browser from `npx playwright install chromium`
  (`%LOCALAPPDATA%\ms-playwright`). Ellis A at 600 px: render 22 s, paint 5 s.
- Code (2026-10-16): `Art/Portraits/bust.py` is what every hero shares (body, eyes and lids, materials, hair curves and
  `clump`, drape, shells, strips, props, stage, render, the runner). A hero script `Art/Portraits/<Hero>.py` (Ellis's is
  `EllisBlender.py`; the other four `Odessa.py`, `Crane.py`, `Gauge.py`, `Pike.py`; the contract of their first round in
  `Saved/Portraits/CONTRACT.md`, not committed) holds only
  `FACE`, `MACRO`, `OPTIONS`, its `# PAINT <X>` lines and its costume builders, then `build(opt)` (starts with
  `build_human(opt, FACE, MACRO, "<Hero>")`, returns `(h, eyes)`) and `bust.run(OPTIONS, "<Hero>", build)`; `EllisBlender.py`
  is the example, bust.py's docstring lists the option keys the machinery reads (yaw, pitch, lids, skin, eyes, glow, world,
  cam, lights). Blender 4.4 runs them unchanged.
- Run: `"C:\Program Files\Blender Foundation\Blender 4.4\blender.exe" -b --factory-startup --python-exit-code 1 --python
  Art/Portraits/EllisBlender.py -- <A|B|C> [--size 1024] [--samples 80] [--out <dir>] [--no-aux] [--blend]` (the script's
  arguments after `--`; with the bpy module: `python Art/Portraits/EllisBlender.py <A|B|C> ...`) writes `Ellis<X>_color.png`
  (Cycles, transparent) and `_aux.png` (R glow mask, B material id / 16); `node Art/Portraits/render.js Ellis<X> --passes <dir>
  [--script <Hero>Blender.py]` (default EllisBlender.py, where it reads the `# PAINT` line)
  paints and frames it into `Ellis<X>.png`; `node Art/Portraits/sheet.js <dir>` lays the three on `Sheet.png` (labels in Chakra
  Petch: `git lfs pull --include="Content/UI/Fonts/ChakraPetch-*.ttf"` first in a fresh clone). `<dir>` defaults to
  `Saved/ArtPreviews/Portraits`. 1024 px at 80 samples: ~2.5 min a bust on 4 CPU cores; 600 px at 32 samples ~22 s for looks;
  400 px at 8 samples ~8 s on the local PC (Blender 4.4). Not bit-exact: two runs differ by 1/255 in a few dozen pixels (seed 0
  fixed; bmesh's uv sphere orders its faces differently each run) and a build's float noise depends on the thread count
  (`-t`). To check a refactor, save `--blend` both ways and compare the scenes, not the pixels.
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
  under `Frame.svg`. Per-option backgrounds: the `# PAINT <X> {...}` lines in the hero script (EllisBlender.py).
- The frame: `node Art/Portraits/frame/gen.js` (with `lib.js`) writes `Frame.svg` and `Window.svg`, deterministic. Window: the
  square 96..928 with radius-28 notched corners; ornaments reach into it up to 50 px at top and bottom centre, 30 elsewhere.
  Motifs: Saint Ada's sun-ring, crow feathers, bandolier segments, Pa's lantern, Sexton's ledger, the headboard "ELLIS RANSOM"
  over crossed revolvers, barbed wire.
- What didn't work: busts sculpted as signed-distance shapes and marched in WebGL (the first try; a clay, CG look the user
  passed on for Blender). Removed; the code is in commit 7ed66a0 (`Art/Portraits/*.glsl`).

- **The four heroes' round (2026-10-16, the user: "Install Node and Playwright on this PC and paint the four heroes'
  portraits"):** one agent per hero wrote `Odessa.py`, `Crane.py`, `Gauge.py`, `Pike.py` (three options each, A the
  signature look, B the death or the ember's mark, C as they were in life; contract `Saved/Portraits/CONTRACT.md`), a
  FRAME agent grew `Frame_<Hero>.svg` (`frame/gen.js <Hero>`; kits in `frame/heroes.js`, shared drawing in
  `frame/common.js`; the saga's sun-ring, feathers and ledger stay, the personal motifs and the name plate change) and
  made `sheet.js --hero <Hero>` read each option's `label` and `line` from the hero script (`Sheet_<Hero>.png`).
  `Tools\portraits.ps1 <Hero>` runs a hero end to end (1024 px, 80 samples: about 100 s a bust here, paint 10 s).
  Lessons: the aux pass must save with the **Raw** view transform (Standard's sRGB curve turned id 1 into 4 and 6 into
  10, so paint.js tightened the wrong materials; every portrait before 2026-10-16 was painted that way);
  `--aux-only` / `-AuxOnly` redo just that pass (3-10 s a bust; a lone string splatted into a native call goes letter
  by letter, so `portraits.ps1` wraps its extra arguments in `@(...)`); `render.js --script` and `--frame` resolve beside render.js (pass
  `Odessa.py`, not `Art/Portraits/Odessa.py`); Gauge has no MakeHuman body (primitives, a stand-in head Empty for
  `stage`, its own `render_aux_glow` reading each material's `looter_glow`); a hero's cyan rim light that also lit the
  hair went light-linked past it (Odessa's `rim_sparing`); eyes with no glow use the skin id (the aux pass blooms every
  id-2 pixel). Each hero file carries 100-330 lines of helpers bust.py should absorb (primitives, chains, tubes, shader
  shorthands, posed arms, `turn`, a stoop, hair tufts, glass/wood ids 14/15): the merge is the next clean-up, renders
  compared before and after as the split was.

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
  dense fine crumple network read as cracked stone or marble, not cloth (cloth creases: few, broad, long, soft). The
  waistcoat's brocade (Ellis's alone) is smooth black wool with a faint woven figure (0.12 of the colour, bump 0.04) and
  broad shallow creases (9/m, bump 0.12): with crinkles it read as leather (the user, 2026-10-15: "as cloth").
- `Tools/Blender/looter_bake.py`: `pack()` (one atlas per set: islands averaged to one density, packed); `bake_set()`: the
  occlusion on the game parts themselves (Cycles AO bake at a quarter of the set's size, 32 samples, 0.1 m, scaled up into
  the ORM; the detailed parts hidden, or lying on their game parts they shade them black), then colour, roughness and
  metal as emissions at one sample and the normal (tangent, OpenGL; written green-flipped as DirectX), cast from a joined
  copy of the set's game parts onto the detailed parts (`sources`: selected to active, cage 1.5 cm, reach 4 cm); writes
  `Art/Textures/<Set>/T_<Set>_BC|N|ORM.png`. `load_set()` reads a set back; `digest()` fingerprints the scripts. ~5 min for
  four 2048 sets on the cloud box's 4 cores (it was ~15 min at 1024 with ambient-occlusion nodes live in the looks; grime is
  Pointiness now). Any edit to the scripts changes the digest and bakes again.

The duster's cloth simulation (`settle()`, `looter_garments.simulate()`): the skirt is built fuller than it hangs (flare 0.13)
and Blender's cloth lets it fall onto the body, trousers, waistcoat and belt, pinned above the hips (smoothstep 0.92-1.08 m),
90 frames, stiffness tension/compression 40, shear 15, bending 12, mass 0.3, self-collision on (~50 s; ~28 s on the PC). Kept in
`Art/Models/Characters/Ellis.drape.json` under a digest of the starting cloth, pins, colliders and settings, so an export
simulates again only when one of them changes (as MisterSexton.drape.json).
- Bugs met: the cloth exploded (points metres away) under self-collision because the start mesh crossed itself: `clear()`
  against the waistcoat's armhole edge had folded the coat under the arms (now it clears against the belt only; the drape
  already rests on the waistcoat). Check with `--selfx` (prints crossing face pairs). The lantern and holster are not
  colliders (they started inside the coat's panel).
- The back hangs straight (the user, 2026-10-15: "Back of the cape is deforming in between his legs. It should be
  straight"): the boots are colliders too (the coat reaches the ankle), and `leg_fill()` adds one only the simulation sees:
  per height (every 3 cm, the hem - 4 cm to 0.90 m) the convex hull of both legs' slice, 4 mm out, and behind the legs
  narrowing at most `LEG_FILL_DROP` 0.03 m/m going down (the back falls near plumb from the seat). Its rows must run top
  down (`gm.grid`'s way) so its faces face out: Blender's cloth collides one-sided by the collider's normals, and inward
  they pulled the coat through the fill and round each leg like trousers.
- The vent (closed and straight, the user): simulated, its two edges start together and the cloth falls with them welded
  (`welded()`: a copy with each pair one vertex, the strip between them dropped), then `lap_vent()` laps them as the built
  coat's: the left flap's edge 4 mm out and 0.025 rad round over the right's, the right's 0.02 rad under it (`ob['vent']`
  holds the pairs). Started apart, the flaps fell into each other and tangled, one swept across the other and kicked out.
- The cape is not simulated: on the shoulders it had nowhere to fall but into itself and crumpled. It stays procedural, with
  flutes of uneven depth and spacing. Bending 4 crumpled the skirt and the vent like linen; 12 gives a duster's broad folds.
- Looser (the user, 2026-10-15: "hugging his body too much"): `COAT_EASE` 1.6 cm off the body from under the collar to the
  chest, 1 cm more from the hips (the collar and the cape's top, fitted to the coat's top, keep their size); flare 0.10 ->
  0.13 (0.15 read as a tent); the cape `CAPE_EASE` 1 cm off the shoulders and back, hem flare 2.4 -> 4 cm. Clay before and
  after: `Saved/Concepts/EllisSwing/coat_loose_{before,after}_{front,side,back}.png`.
- The vent never opens in the mesh: `gm.grid` joins its two columns with a strip of narrow faces down to the hem (the weld
  only collapses them above VENT_TOP). Anything that moves its two edges apart stretches that strip into a web.

Ellis.py:
- Order: body, trousers (knee and boot-top creases), boots (below), waistcoat and shirt,
  belt (before the duster, which drapes over it), duster, sleeves (bunched at the cuff, crescents at the elbow), cape
  (draped over the arms let down to the game's idle, `IDLE_ARM` 10 degrees off the body: draped over the bind pose's A,
  its sides stood out like wings and fins once the arms came down; its front corners flare less; each point at least
  8 mm outside what's under it, unsmoothed (the drape's smoothing let the sleeves' tops through by the collar's foot);
  its top carried up into the bind pose by its share of the upper arm (below), cleared off the coat and the sleeves
  as the idle has them (`clear_cape`, `cape_idle`; round 6)),
  holster, lantern (hung from a `BeltHook` piece out from the belt; clear of the thigh from fount to cap, 4.3 cm and 3.3 more:
  with 1.7 a crouch's level thigh, under the hook it rides, pressed 1.5 cm into it), the
  simulation (the belt, cartridges and bullets as colliders), gloves (`thicken_hand`: `GLOVE_GIRTH` 1.15, each point pushed off its
  skinning bones square to them, blended by its weights: fuller palm and fingers, no longer; the user: thin, clawed
  hands, 2026-10-15; the glove runs `GLOVE_CUFF` 6 cm up the forearm inside the gauntlet; the gauntlet's mouth sits at
  the wrist, each ring the smallest ellipse round the forearm's axis holding the glove's slice + `CUFF_ROOM` 4 mm,
  flaring `CUFF_FLARE` 1.8 cm, 3 read as a bell; the sleeve stops `SLEEVE_END` 3 cm up inside it, gathered in under it to
  `SLEEVE_TUCK` 1.2 cm off the arm. The user, 2026-10-15: "his wrists are either invisible or don't match the gloves":
  the game mesh has no wrist, so between the glove's edge and a mouth held open by the loose sleeve's end you saw
  through him, and the coat's colour; fitted angle by angle the mouth stood out in a lobe at the thumb's root; its lip
  rolled out `CUFF_LIP` 3 mm: cut square, a turned and bent hand met it in a sharp notch), hat
  (EllisBlender.build_hat moved into the figure's space; the brim's reach past the crown `BRIM_REACH` 0.88, each spoke
  shortened toward its root so the curl keeps its shape),
  bandana (inside the collar: draped over skin, waistcoat and shirt; its point down the chest), collar and pin (built
  after the bandana: `COLLAR_GAP` 1.2 cm off the bandana, its knot and the neck, its wall no nearer the neck than the
  cape's top edge at the idle + 1.2 cm (its most over a few degrees, smoothed: over the square shoulders that edge is
  18-19 cm out, past the old 16 cm foot, and the sleeves' tops showed between); its top edge as round 5's, the brim's
  clearance; the front ends roll out half as much (they turned out flat like tabs); the throat's points splay half as
  far: more and the flap with the pin jutted like a box; its lining, `collar_lining` (the game's one-sided materials:
  looking down between the neck and the collar you saw past the skin, which stops under the collar, into the neck,
  and a jumble of the cape's, coat's and sleeves' tops): a band `COLLAR_BAND` 2.5 mm off the body round the back
  (`COLLAR_LINING` 85-275 degrees, where the bandana's band is cut) from 1.2 cm over the skin's lower edge down to a floor
  at neck_01's head + `COLLAR_FLOOR` 3 mm, out to the wall, on to the collar's ends from the bandana out; its 'Lining'
  points turn with the skin's weights near the neck, cleared off the reduced skin (`clear_collar`); round 6 tried the
  foot tucked into the cape's top flaring out to the same top edge: it read as a frilled ruff on a stalk), hair (cards,
  below), then
  'Seams' on the sewn pieces; the game mesh (below), its UVs, the bake from the detailed parts (then deleted), thickness,
  weights, one joined mesh `Ellis`.
- Hair cards (round 6, `Tools/Blender/ellis_hair.py`; the user, 2026-10-15: "hair looks like mud"). What failed before:
  flat ribbons read as slabs; 80 narrow ribbons as ribbons; closed lens-shaped tube locks in two layers as dark ropes
  and dreadlocks (mud: no shine, no strands; the detailed locks baked onto the low ones smeared the strands). Now layered
  card strips UV-mapped onto a script-made strand atlas, not baked (the cards are the game's hair as built; `HAIR` and
  `HAIR_LOW` build the same): from the scalp out a sheet (41 columns round the back from temple to temple: the
  underlayer, so no skin shows), main (30 cards, 4 cm, dense strips), outer (36, 2.9 cm, shorter and flicked out at the
  tips: the layered, shaggy cut), wisps (24), fly-aways (12) and the lock across the left brow (3); each card leans round
  the head by a random sweep (all plumb read as a curtain), arched to the head's curve, 8 rows; custom normals out of
  the hair's volume (they survive the join and the export's transforms). 2748 triangles (round 5's locks: 5.8k).
  Roots 1.2 cm up under the hat band (`ellis_hat.band_ring`, the sweatband's inner face), pressed within its room off
  the scalp (7 mm sides and back, 2 mm forehead; logged: points past it, 0), fuller from a little below the band.
  Tips: the jaw by the temples, into the turned-up collar round the back (2.2 cm under its rim: collar length). Grown
  over skin and bandana only; the collar and cape are walls the hair is kept 5 mm inside (as colliders, one-sided, they
  pushed cards out through the collar). The hat tipped 5-20 degrees on its pivot: card points its brim would sweep are
  moved in under the crown (rig check: 0 of 2357). The back's swing chain `hair_01..02` on the cards' median depth
  (`chain_joints`; the outermost card bent it 10 cm out).
- The hair atlas (`ellis_hair.make_textures`; `Art/Textures/EllisHair/T_EllisHair_BC|N|ORM.png`, 2048): top quarter a
  sheet tiling across (the underlayer: solid to 70 % of its length, then strands); below, 8 strips 256 px wide, root at
  the top: 2 dense, 3 clumped, 2 wisps, 1 fly-aways. Strands drawn one by one (numpy, no library art) in clumps and
  sub-clumps (forked, uneven tips), body strands behind for mass; sRGB root 0.08/0.06/0.047, mid 0.16/0.118/0.09, tip
  0.185/0.138/0.104 with a little dust (lighter tips read as dyed). BC alpha = coverage (soft-edged: the game dithers it);
  a strand fades where it reaches its strip's side (clamped there, strands drew the card's edge as a line); N the strands'
  round profile (DirectX); ORM R occlusion (root, deeper strands), G roughness per strand (~0.41), B each strand's random
  id, NOT metal (M_CharacterHair tips the tangent by it). Ellis.py: `MADE_SETS` (not baked; made if missing), slot
  `EllisHair` on `M_CharacterHair`; `--look <tag>` renders the head (Eevee, dithered alpha; `--aniso` Cycles) into
  `Saved/Concepts/EllisRound6/hair/`.
- The coat: option C, the user's pick (2026-10-15), is the build: squarer shoulders and a pointed back on the cape, a
  deeper crown crease and harder brim curl on the hat, the duster to the ankle (`HEM` 0.24; it hangs 0.20-0.23 m).
- Boots (the user, 2026-10-15: "proper boots"; MakeHuman's foot reshaped into a toe box read as a bare foot on a sandal,
  then a last with the calf's shell standing in it as a slipper with a sock): `boot_shaft()` a closed thick-walled tube,
  round each 1 cm slice of the leg (cut through its faces: the shin's vertices lie in rings 4-5 cm apart) its convex
  hull + `BOOT_EASE` 1.2 cm, a quarter rounder, narrowing at most `BOOT_TAPER` 0.08 m/m going down (a stovepipe, not a
  sock), straight on down from 3 cm over the ankle to the insole, the top at `BOOT_TOP` dipping 3 cm front and back, a
  `BOOT_WALL` 5 mm wall round a well down to 0.34 m for the trousers; `boot_last()` 46 rounded-rectangle sections (closer
  at the ends) from the heel to 2.5 cm past the toes, over the foot with 9-10 mm room, a roomy toe box (insole + 4.6 cm),
  ahead of the ball narrowing at most 0.15 m/m (to the toes' own tips it came to a beak), the toe a quarter ellipsoid on
  the sole, a little square in plan. Both fused by a voxel remesh (`BOOT_VOXEL` 1.8 mm), the crease where they meet
  rounded (`fillet()`: vertices within 2.2 cm of both eased toward their neighbours, 60 passes), reduced to `BOOT_TRIS`
  7000 each; the soles' outline is the fused boot's footprint + 4 mm welt. No UVs (smart-projected for the bake); the look
  is object space: the toe cap stitched at y -0.19 (5.5 cm behind the tip), the welt at 2.15 cm. The body's feet aren't
  in the game mesh (its skin is the head and neck).
- Texture sets (2048): EllisSkin (face and scalp), EllisDuster (coat, sleeves, cape, collar), EllisGarb (hat, bandana,
  waistcoat, shirt, trousers), EllisHair (script-made, above), EllisGear (boots, soles, gloves, belt, holster, revolver, lantern, brass, lead, iron). The eyes
  and the lantern's globe and flame are Glow materials (EllisEyeGlow, EllisLanternGlow).
- Skin weights by part (`weights()`): the body's own; the duster above the hips from the torso, its skirt pelvis to the
  thighs (side by x) as looter_heroes' skirts; the cape's top what the clothes under it ride (`cape_weights`: the nearest
  skin's at the idle, its collarbones and upper arms, the rest spine_05, kept as 'CapeW_<bone>'), handing over to its
  chains from the shoulder line to halfway down ('CapeChain'), its top carried up into the bind pose by its share of
  the upper arm (`cape_idle`) so the idle puts it back as draped, nothing riding the upper arm below (round 5's
  'ArmFollow' carried the whole cape up by a share of the arm: blended halfway between the chest and an arm raised to
  aim, the cloth sank into the arm, the clipping the user saw; round 6); collar spine_05 to neck_01 by height, its
  lining's band and floor near the neck the skin's own; bandana the skin's own weights where it lies (head over nose, mouth and chin; head,
  neck_02, neck_01 down the neck; the knot too; the user saw it cut through the neck when the head turned, 2026-10-15), its
  bib onto its chain; hair, eyes head; hat crown, brim and band 100% `hat`; belt pelvis (+spine_01); the holster's skirt
  pelvis over the belt to thigh_l down the pouch, the pouch and its loops all thigh_l, the gun rigid at `HOLSTER_GUN` 60 %
  thigh_l (round 5: blended from the belt down the pouch, the crouch's thigh sank 3.6 cm into it; the gun all thigh, a
  stride swung its grip, in front of the hip joint, 4.5 cm into his belly; all hips, a crouch's thigh rose 5 cm into it;
  at 60 % 0.9 and 1.1 cm);
  `BeltHook` pelvis to thigh_r along it; lantern all on lantern_01 (it stays on thigh_r: on pelvis a crouch's thigh rose
  5 cm into it);
  gauntlets lowerarm to hand; gloves from the hand's faces; then (`twist_weights`) on the sleeves, gauntlets, conchos and
  the glove's cuff the lowerarm and hand weight shared by how far down the forearm a point is: the hand's share coming in
  over `WRIST_BEND` 0.78-1.25 of the forearm (it brings the bend; over its last tenth a turned, bent hand creased the
  gauntlet's lip), the rest over lowerarm, lowerarm_twist_01 (half way down) and lowerarm_twist_02 (90%) (the game turns
  each twist bone by its share of the hand's twist: a gun's grip turns the left hand ~100 degrees, the right ~32;
  unturned, the glove tore off the gauntlet); past the wrist the forearm's share is the hand's (the wrist's own blend there
  candy-wrapped the turned glove). Hit zones:
  head, trunk, skirt round the hips, each limb, hands, feet. LODs 50 % and 25 %.
- Swing bones (`swing_layout`, placed from the built game mesh; the game's springs, Player/Animation/BodySwing, move them):
  skirt_{fl,sl,bl,fr,sr,br}_01..03 on pelvis (each side's f and s chains at 1/6 and 1/2 of its arc from the coat's front
  edge to the back's middle, b straight behind the knee: at 5/6, by the vent, a stride's back leg slid it aside and the
  knee came out through the ankle-length coat; joints at the belt line, mid-thigh 74 cm, the knee 53 cm, the hem 21 cm)
  and skirt_bm_01..03 down the middle of the back over the vent (the same heights; without it a stride's back shin came
  through between the two b chains), skirt_bli and skirt_bri (`INNER_AT` a fifth of the way from each b to bm: there
  the stride's back shin carries them with b; halfway 35 points of it came through, a third 31, a fifth none),
  cape_{f,a,s,b}{l,r}_01..02 and cape_bm_01..02 (`CAPE_CHAINS`, round 6; round 5's two a side, f and b, from the shoulder
  line, let a raised arm come up through the cloth between them): f the front panel (0.09 of the arc from the front
  edge, from the cloth at the shoulder line 143 cm on spine_05), a the front of the shoulder (0.24), s straight out
  over the shoulder joint, b behind it (0.64), these three from the shoulder joint itself on the collarbone (their
  cloth turns about the joint the arm turns about: the springs' share of the arm's turn, no cloth blended halfway);
  bm down the middle of the back from the shoulder line on spine_05; ring_weights reads a joint-rooted chain's place
  round the cape from its cloth points ('ring'); `cape_arm_l/r` (unweighted, under the collarbones: the upper arm as it
  hangs at the idle, `add_cape_arm_bones`) for the springs to take the cape from; bandana_01..02 on neck_02 (under the chin to the
  point), hair_01..02 on head (from 168 cm, a quarter of the way from the skull's bulge to the nape, which is inside the
  collar, down the back at the cards' median depth to where most of the back hair ends, on the nape at ~156 cm),
  lantern_01 on thigh_r (from the bail's top on the hook's end, right above the lantern, to the
  fount: from the hook's belt end the springs' gravity swung it 6 cm into the thigh). 50 bones in 21 chains; each one's
  head at its joint, its Z out of the cloth; and each chain's `<chain>_end`, an unweighted leaf (deforming, so the FBX
  keeps it; 2 cm on along the last bone) with its head where the cloth ends: an FBX bone has no tail, and BodySwing
  guessed the last tip as long as the bone before (the skirt's at mid-shin, the hem 11 cm past it, which then went
  through the ground in a crouch). Weights: today's down to 0.91 m on the skirt (the shoulder line on the cape), all chains from
  mid-thigh (halfway on the cape): past there a vertex already shares two chains' two bones (4 influences); round the body
  between the two nearest chains at the vertex's height; across the back each side's b, bi and then skirt_bm (the cape:
  its two b's to each other; the vent never opens: the cloth simulation's notes); down a chain
  each bone hands over to the next along that one's length by a smoothstep. `soft_top4`: the fifth-largest weight comes
  off the top four (plain top four drops a bone at once: a kink). The bandana's chain was on head (the contract): it swung
  2 cm out through the collar when the head turned or rolled onto the stock; on neck_02 the bib stays inside the
  collar's opening while the mask turns with the face.
- `hat` (on head): from the back of the band at its middle height to the front of the band; the game tips the hat on it
  while aiming (brim front up, up to `HAT_TIP` 20 degrees) so the brim clears the rifle's scope. Tipped 20 degrees the
  brim reaches no hair and no collar (it first touches the hair at ~50 degrees).
- Facing: built facing -Y; `face_like_mannequin`, the last step before the export, turns the armature's bones, the mesh and
  the hit zones a quarter about Z (into the data) to face -X, so SK_Ellis faces +Y in Unreal with his left at +X, as
  SKM_Manny_Simple (facing +X, the retargeter couldn't line his fingers and arms up: the clawed hands). An exported FBX
  read back gives the file's own axes; Unreal flips Y, so Unreal = (x, -y, z) of those.
- The game mesh, 34.9k triangles (the user's call, 2026-10-15: up to 35k; at 20.6k it read coarse): the detailed build
  (~108k) is only the bake source. `make_lows()` copies each part and decimates it by its `LOW` share (Blender's Decimate,
  collapse), spending where the silhouette and close-ups need it (face, hands, collar, brim, coat edges, boots; small hard
  parts whole: reduced, the gun, holster and lantern shredded into shards); the face spares the eyes and the cloth
  (`EDGE_KEEP`: duster, cape, collar, bandana, brim) a band along its open edges (a vertex group, inverted: Decimate
  collapses only what its group weights); `unflip()` turns back the faces Decimate flipped (against the detailed part and
  their neighbours) and dissolves its slivers: Unreal culls a face's back, so each was a hole (the waistcoat's "black
  holes", 116 in the old boots' toes, the duster's back); the shirt keeps only what shows in the waistcoat's V (bent, the rest
  came through); the bandana's back, hidden under collar and hair, goes; the hair cards are built again as they are
  (`HAIR_LOW`; each card's sideways direction carried along it: a card running straight out twisted into a hook); reduced layers are pushed back outside the reduced
  layers under them (a reduced sleeve cut through its reduced gauntlet; the coat over the cartridges and bullets too);
  below the chin the bandana stands 1 cm off the neck's skin (the mask above, 3 mm). The copies take each part's
  transform (the eyes, buttons, pin and conchos are placed by their object, not their mesh: copies without it fell to the
  floor). Biggest shares (round 5; round 6's hair cards 2.7k): hair 5.8k, duster 3.4k, face 2.6k, boots 2.5k (LOW 0.18 of the fused 14k), gloves 2.3k, cape
  2.0k, bandana 2.0k (round 5 paid for the hair's locks with the waistcoat's buttons at 0.35, the bullets 0.25 and the
  lantern's wire 0.4). `--stats`
  prints the triangles per part (no bake). Leather looks keep their grain at 280/m (900-1100 aliased at the bake's ~1000
  px/m into a fuzz that read as suede and fur).
- `--audit <tag>` (`Tools/Blender/ellis_audit.py` on `looter_audit.py`; neither is in the bake's digest): the baked look in
  Cycles (sun ~40 degrees and sky; one dusk shot) as an 8-angle turntable at the game's idle and close-ups (face, under
  the brim, hands, belt, boots front and side, waistcoat, back, collar, hair), the game's poses (`looter_audit.POSES`:
  idle, aim, stride, crouch; grip_a and grip_b, the aiming arms with the hands turned on the forearms as the guns turn
  them, 100 and 32 degrees each way round, the twist bones by their shares: `('twist', side, degrees, bend)`) in clay with the swing chains where BodySwing's springs would rest them (`settle_swing`: its
  constants, limbs, bone push (top 35% left alone, at most half the bone a pass), nothing over its head, and floor
  copied, keep in step; its points the game's: an FBX skeleton has no tails, so a chain's last tip is
  its head plus the bone before, a lone bone's 18 cm down; the pose reached in 12 stages, each point moving at most 1 cm a
  pass and kept out of the limbs on the way, as the springs follow frame by frame: set straight into a pose, a limb
  already past a point pushed it out the far side), and every layer checked against the one over it in each pose (`PAIRS`: a point looks out along its normal, turned
  away from the nearest bone so a thickened part's inner shell counts, and back in; `SOLIDS`: a part inside another; `GROUNDED`: under the ground), into
  `Saved/Concepts/EllisAudit/<tag>/` with `findings.json`. The 2026-10-15 audit: `EllisAudit/defects.md`.
- The cape's and collar's audit (round 6): `Tools\artrun.ps1 -Script Tools\Blender\ellis_cape_audit.py -ScriptArgs
  --audit,<tag>` builds and rigs Ellis.py WITHOUT the bake (any edit changes the bake's digest; a `--audit` run of
  Ellis.py itself bakes and writes the texture sets), then 16 poses (`looter_audit.POSES`, round 6 added carry,
  aim_high, aim_low, crouch_aim, swing, look_l/r/up/down, look_up_tip; `('tip', bone, degrees)` tips the hat as the game
  does while aiming): crossing faces (BVH overlap) of the cape and collar with every layer, counted apart where out of
  the collar's ring; points through (PAIRS); cape points inside the sleeves' tubes; the brim's nearest to the collar;
  close clay views (shoulders, arms, hands, collar; backs culled as Unreal's one-sided materials) and views down into
  the collar against a magenta sky (magenta between the neck and the collar is a hole), into
  `Saved/Concepts/EllisRound6/cape/<tag>/`. Env: `CAPE_POSES` (a+b), `CAPE_SHOW` (parts to draw), `CAPE_RENDERS=0`,
  `CAPE_SWEEP` ('f=0.2,0.3;bulk=4,6': tries the springs' shares and the cape's bulk), `CAPE_SRC` (another version of
  Ellis.py, e.g. `git show HEAD:...`), `CAPE_MIRROR=r5` (BodySwing as round 5 shipped it).
- `--option A|B` (`Tools/Blender/ellis_options.py`, off unless asked; the user picked C, now the build, 2026-10-15, and
  not these): A gear and story (a bandolier over the coat, a satchel at the right hip, spurs, a watch chain, the lantern
  a third bigger; +2.9k triangles), B the coat itself (lapels, pocket flaps, buckled sleeve straps, a second cape tier, a
  frayed and patched hem; +3.3k). An option's cloth is cached apart (`Saved/Concepts/EllisOptions/
  Ellis.drape.<option>.json`); clay views in `Saved/Concepts/EllisOptions/<option>_<view>.png`.
- Bake cache: `Art/Textures/EllisSkin/bake.digest` holds the digest of the scripts, the size and the Blender version the sets
  were baked from; a run with the same digest reads the PNGs back instead of baking (`--rebake` forces it).
- The cloud box can't push Git LFS: its PNGs are not committed (`.git/info/exclude` there). The PC baked them on
  2026-10-15 with Blender 4.4 (the cloud wrote the scripts against 5.2's module: they ran unchanged; ~25 min for build,
  simulation and four 2048 sets beside the editor) and they're committed. MakeHuman's base.obj (1.75 MB, gitignored) is
  fetched when missing: `Art/ThirdParty/MakeHuman/fetch.py` (the PC has no system Python: run it with Blender's,
  `C:\Program Files\Blender Foundation\Blender 4.4\4.4\python\bin\python.exe -I`). Export beside the editor:
  `Tools\artrun.ps1 -Export Ellis -Family Ellis`, then `Looter.ImportModels <that folder>`: `/Game/Art/Characters/SK_Ellis`
  (142 bones with the root: 63 mannequin, 4 forearm twist, 50 swing, 21 `_end` leaves, 2 `cape_arm`, hat; 3 LODs, Nanite off for the eye glow, 185.5 cm to the hat's top),
  `MI_Ellis*`, `PA_Ellis`.
- Run: `blender -b --factory-startup --python Art/Models/Characters/Ellis.py -- [--size 1024] [--glb <path>] [--preview]
  [--shots full+upper+hips+feet] [--shape] [--sim-only] [--selfx] [--no-sim] [--rebake] [--rig-check] [--audit <tag>]
  [--option A|B]`. `--rig-check`
  (no bake, ~15 s): Workbench clay into `Saved/Concepts/EllisSwing/`: the swing bones drawn at rest, `shares_*` (each
  vertex coloured by its chains), `SWING_TESTS` bends, the hat tipped (`hat_tip_*`), `HEAD_TESTS` (the head alone turned
  35 degrees each way, rolled 20, pitched 20: `bandana_*`); and logs at most 4 influences, the steepest weight steps, the
  tipped brim's reach into hair and collar, and per head pose the bandana's clearance from the neck's skin and anything
  past the collar (at rest 1.7 cm; turned 1.1-1.5; pitched 0.6; nothing through; rolled, 2 points 0.3 cm past the
  collar). `--shape` renders the untextured
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
  his hands missed the rifle). Lessons: SK_Ellis faced +X, the mannequin +Y: the retarget pose turned his root 90 degrees
  and nothing else, as the engine's auto align compared them unturned and tipped his head ~25 degrees sideways (both
  rest poses are close enough: arms down, the neck a little forward). Since 2026-10-15 he's exported facing +Y like the
  mannequin (Ellis.py `face_like_mannequin`): the script reads his facing (toes) and auto aligns every chain when he faces
  +Y (the fingers line up: the clawed hands), the root turn only for a +X mesh. The script rebuilds the assets in place: deleting
  and recreating the retargeter nulled the character's pointer and dirtied BP_LooterCharacter (a Save prompt blocked
  `launch.ps1 -Build`), and the component then also loads the assets again at play if its pointers are empty.
- Checking in PIE: `play.py view [n]` cycles first/third/front views (F5); `play.py inventory [close]`; toggling the
  mannequin back on (its `set_hidden_in_game`) compares poses. Keys from `input.ps1` (Tab, number keys) didn't reach the
  game on 2026-10-15, so both have play.py steps.
- Materials (the user: "plastic, stiff and unlike a real fps character"; round 1 approved 2026-10-15): Ellis.py's
  GAME_MATERIALS gives each kind of surface its own slot on a master made for it, several slots sharing one baked set:
  EllisSkin on M_CharacterSkin (Subsurface Profile, SP_Skin: the engine's skin defaults), EllisDuster and EllisGarbCloth
  on M_CharacterCloth (the engine's Cloth shading: a fibre sheen at grazing angles), EllisGarbHair (the hat band only
  since round 6), EllisGear and EllisHolsterGun on M_Character, EllisHair on M_CharacterHair (masked, two-sided, the
  engine's Hair shading along the cards' V; opacity = the atlas's alpha raised per mip level (MipBoost 0.35) through
  DitherTemporalAA, clip 0.5; Scatter 0.2 on the Metallic pin, Specular 0.45, TangentJitter 0.5 from the ORM's blue;
  Backlit, Custom Data 0, unwired: 1; `HAIR_SHADING = MSM_DEFAULT_LIT` is its fallback, lit by the strands' normals). The masters (build_world_materials.py's characters) share M_World's maps plus
  RoughnessScale/Offset/Breakup (a fine noise: no smooth sheen across a part), NormalStrength, DiffuseAO 0.7. Cloth's sheen
  is its fuzz colour times FuzzStrength (the Cloth amount pin, Custom Data 0, can't be wired from Python; unwired it's 1):
  at 1.2 everything went chalk white; tuned in game to 0.2 (oilskin) and 0.3 (wool, linen, felt). Leather and metal stay
  on default lit.
- The holster: the holstered revolver has its own looks (GunIron, GunWalnut) and slot (EllisHolsterGun); while a revolver
  is the gun in hand, UBodyFigureComponent hides that slot on every LOD (ShowMaterialSection), the user's "empty only while
  drawn".
- Swing (the user's "coat and hair that move", 2026-10-15; `FBodySwing`, after the retarget in UBodyRetargetAnimInstance):
  each swing chain (Ellis.py's 40 swing bones, 16 chains) is a string of points in world space, so they lag and swing as he moves: a
  spring toward the animated pose (an acceleration with its stiffness as a natural frequency, damped; Verlet), a small share
  of gravity (the pose already hangs; the lantern full, a pendulum), each point its length from the one before, out of the
  limbs. Limb clearance needs no radii: a point keeps 95% of its rest distance from each limb segment (thighs, calves,
  pelvis, spine, neck, upper arms, forearms, hands), at most the limb's own (14/10/18/15/9/9/7/5 cm). The bone between two
  points keeps clear too (`BoneRadius`: its nearest rest distance, same 95% and caps; `PushBoneOut` turns the tip out by
  the overlap over how far down the bone it is, at most half the bone's length a step, ignored in the bone's top 35%: at
  20% and uncapped a crouch's thighs swung a coat flap out level with the hip, once up to the shoulder): with points
  alone a full stride's back shin slid between two of them and the boot showed through the ankle-length coat's lower
  back (Ellis.py's audit, 2026-10-15). Nothing rises over what it hangs from (`KeepUnder`: level at most). A floor: no
  point under the component's origin + 2 cm (the mannequin's root stays on the ground, crouched too); a point under it
  swings up about the one above onto it, keeping its lean (`KeepAbove`), so a crouch lays the hem on the ground (it sank
  ~11 cm); a grounded point rests (no speed: kept, it flipped a gathered hem over its head). A thick thing keeps more
  room (`FBodySwingSettings::Bulk`, added to the clearance caps): the lantern 2.5 (its 4.3 cm body sank 1.5 cm into the
  crouched thigh while its line kept clear). Hand and forearm limbs: at
  idle the hands sank 2.5 cm into the coat's sides; the boots' feet (foot to ball, 8 cm): the hem went through a toe.
  The ring (`BodySwingRing.cpp`; the user, 2026-10-15: "cape still clips through the boots and shins"): the cloth
  between neighbouring skirt chains (fl-sl-bl-bm-br-sr-fr; the open front not joined) keeps off the limbs too, at the
  midpoint of each level's two points and the middle of the patch down to them (rest distance x 0.95, capped), on the
  shown points only: the push is level, shared by nearness (the nearer point takes more; both taking all of it folded a
  corner another push had already moved right over), at most 12 cm a point. Audit: the stride's shin through the coat
  34 -> 28 points (what's left: the inner back of the right shin between bm and br with the leg far back). Checked in
  PIE 2026-10-15: back standing, walking, sprinting; crouched front and back (drapes round the knees).
  `play.py press Crouch` toggles crouch (the game's setting); `Looter.ThirdPerson.FrontYaw <deg>` turns the front view
  round him (90 his left side, 270 his right) for inspection. A video for the user (2026-10-15): screen-grab the
  editor's game viewport at ~15 fps while held inputs run (System.Drawing CopyFromScreen in a loop; the held `press
  Sprint`/`press Move` sent before the loop), then encode the PNGs with Blender's bundled FFmpeg (`bpy` sequencer,
  H264 MPEG4, fps/fps_base set to the measured rate); scripts kept in the session scratchpad (`record_run.ps1`,
  `encode_video.py`), not the repo. Lessons: console commands from a second process don't reach the game while the
  capture loop holds the window (plan one camera per take); the front view at the default arm length puts the camera
  inside his head (set `FrontArmLength` ~320 first); start where 40 m of ground lies ahead (a take ran into the barn).
  Check a frame from each half before sending. Measured
  in Ellis.py's audit (its copy in step, round4b -> round4c): square to both bone and limb the push went sideways when
  they shared a plane (the stride's back panel slid round the shin's side), so it goes the chain's way out of the body
  (`Outward`), square to the limb and the bone; the extrapolated last tip let the hem 11 cm past it through the ground,
  so each chain ends in a `<chain>_end`, and a last point gathers (at most a third of its length out on the floor, its
  bone shortened along its length axis); the stride's boot through the coat 48 -> 34 points, the crouch's hem under the
  ground 278 points 15.0 cm -> 57 points 4.9 cm (the panel between two gathered chains).
  Tuning table: `FBodySwing::SettingsFor`
  (skirt 7 rad/s, damping 3.5; cape 12, 5; bandana and hair 9, 3.5; lantern 2, 1.2). `Looter.Body.Swing 0` holds the
  animated pose. Lessons (each found by Looter.Player.BodySwing.* and a Python copy of the step): a nudge of the position
  toward the target each step is a stiffer spring the shorter the step (30 vs 120 fps ended 7.5 cm apart); chains with
  length constraints swing differently at different step sizes (1/60 vs 1/240 s steps: 20 cm), so it always takes fixed
  1/120 s steps, the leftover carried, and shows the blend of the last two; a push out of a limb must not become speed (it
  flung a chain right over the thigh); three loose links are a chaotic triple pendulum (test frame rates on the skirt).
- The cape hangs from spine_05 through its chains, so it kept the arms-out shape it was modelled over (wings in front):
  its corners' targets also ride the upper arms (front 60%, back 40%: `CapeFollowFront/Back`), so it drapes as the arms
  hang and lifts as a gun comes up. The arm they ride is eased on the cape's own bone (`FollowEaseSeconds` 0.35): riding
  it directly, the corners flapped up at the shoulders with a sprint's arm swing (the user, 2026-10-15); a held raise
  comes through whole, a run's swing (about 1.5 a second) at about a third. Round 6 (the user: "Cape clippings"; the
  CAPE agent's model notes above): nine cape chains, `cape_<key><side>` with the key's follow share (`CapeFollowShare`:
  f 0.2 the front, a 1.0 over the arm, s 1.0 the shoulder's side, b 0.5 the back; `cape_bm` follows nothing); the a, s
  and b chains hang from the shoulder joint on the collarbone, so they turn about the arm's pivot. The bind pose's arms
  are raised 31 degrees off the idle, so a chain's rest on the arm (`TipInFollow`) and the upper arm's clearance are
  measured against Ellis.py's unweighted `cape_arm_<side>`, the arm as it hangs at the idle. A chain riding an arm may
  rise to its target's height (`bRisesToTarget`: `KeepUnder` takes a ceiling; held level with its anchor the cloth
  couldn't follow an arm raised to aim and the arm came up through it). Cape `Bulk` 6 cm. Audit: cape cut by the
  sleeves 331 -> 27 face pairs over 16 poses, 0 in every aiming pose.
  Round 6's model (nine cape chains, `cape_arm_l/r`) needs, in Bind and the step (looter_audit mirrors it, marked
  PROPOSED; the user saw the cape cut by the arms aiming, carrying, crouching): each cape chain's share of its upper
  arm's turn by its key, `cape_<key><side>` (the side the name's last letter): f 0.2, a 1.0, s 1.0, b 0.5, cape_bm none;
  `TipInFollow` from `cape_arm_<side>`'s rest (the arm as it hangs at the idle; the cape is draped over arms hanging
  so: from the bind pose's raised arm the idle pulled it 31 degrees into the arms), and the upper arm limb's rest
  segment for every clearance from there too (shoulder joint to the elbow turned with it); a chain point riding an arm
  may rise as high as its target (KeepUnder held the cape level with its anchor: an arm raised to aim came up through
  it); the cape's `Bulk` 6 cm (its chains keep their room from the forearms and hands: a gun's hands in front of the
  chest came through the front panels). Audit (ellis_cape_audit, 16 poses): the cape cut by the sleeves out from
  under the collar 410 face pairs -> 27, by the gauntlets 12 -> 0.
- The forearms (the user, 2026-10-15: "His wrists are either invisible or don't match the gloves"): the retarget rolls
  his hand about the forearm (left about 100 degrees onto a foregrip or the revolver, right about 32; the bend only
  10-20) and nothing rolled the forearm, so the gauntlet stayed put while the glove turned and the wrist tore open (no
  wrist geometry under them). `FBodyForearmTwist` turns each `lowerarm_twist*` bone under a lowerarm by its place down
  the forearm (0 elbow, 1 wrist) times the hand's twist from rest (swing-twist about the forearm axis), before the swing.
  Ellis.py: twist bones and weights spread along the forearm, the glove's cuff run up into the gauntlet.
- The hat (the user, 2026-10-15: in third-person aiming the brim covered the scope; also the brim 12% smaller in Ellis.py):
  `FBodyHatTip` turns the `hat` bone brim-up about the head's left-right axis as a sight comes up (UPlayerViewComponent's
  aim alpha, eased; 18 degrees at full aim), about the back of the band.
- The hat seated, the head closed (round 6, the user: "can see through the head/hat, hat doesn't seat on his head
  properly"; `Tools/Blender/ellis_hat.py`): three causes. The game head was a "crystal": `make_lows` spared everything
  within 3 cm of the eyes and decimated the rest of the head to 0.30, so the skull's back and top collapsed into ~12
  triangles with holes (clay renders draw both sides of a face and never showed it; Unreal culls backs). Now
  `decimate_skin` goes by region (eyelids whole, skull 0.25, skin under the bandana 0.10, visible skin 0.20; eyelids at
  0.75 tore the slits). The crown was one-sided (culled from below): its felt grows 4 mm inward and a leather sweatband
  lines the foot. The hat stood 2-3.6 cm off the skull and 4.4 cm too high at the back (tipped 9 degrees forward about
  the band's middle): `fit` seats the band's foot on a plane from brow + 2 cm to the occipital bump + 1.2 cm (a 4 degree
  rake; level it lost its menace), 2 mm off the forehead, 7 mm at the sides and back (pressed hair), the crown 8-14%
  narrower; the pivot 4 cm in and 3 cm down from the band's back (tipped 20 degrees the band lifts at most 1 cm; before
  3). `band_ring(fig)` gives the hair its line. Check: `artrun -Script Tools/Blender/ellis_hat_check.py -ScriptArgs
  --rig-check,--tag,<tag>` renders with back faces culled on magenta (`Saved/Concepts/EllisHat/<tag>/`). Lessons: check
  one-sided holes with back faces culled; sparing a region from one global Decimate ratio starves the rest.
- The hair's engine side (round 6, the user: "hair looks like mud"; the cards and their script-made atlas: the HAIR notes
  above, `Tools/Blender/ellis_hair.py`): `M_CharacterHair` (`build_world_materials.py M_CharacterHair`; built before the
  import, or MI_EllisHair fails) is masked (clip 0.5), two-sided, the engine's Hair shading; base colour from the atlas
  with a DiffuseAO lerp; coverage from the BC's alpha raised by the mip level sampled (`HAIR_COVERAGE`: a coarser mip
  averages strands with their gaps, so the mask thinned with distance and the hair went bald a few metres off), through
  the engine's DitherTemporalAA material function (`/Engine/Functions/Engine_MaterialFunctions02/Utility/
  DitherTemporalAA`; there is no expression class of that name in Python: a MaterialFunctionCall with its
  `AlphaThreshold` pin); Scatter on the Metallic pin, a strand tangent jittered by the ORM's blue (each strand's random
  id, not metal). Without Lumen, if the Hair shading reads wrong, `HAIR_SHADING = MSM_DEFAULT_LIT` lights the cards with
  the strands' normal map (both sides). After an import check `T_EllisHair_BC` kept its alpha (BC3) and the 50% LOD
  hasn't collapsed the cards. In the golden backlight the Hair shading glowed the hair light brown: Tint 0.55 (set in
  Ellis.py's `EllisHair` slot, which the importer writes to MI_EllisHair; a value set only on the MI is lost on
  re-import). An import that reparents a bone (round 6: cape chains onto the collarbones) logs "Discarding existing
  LODs" and rebuilds them: check `get_lod_count` is 3 afterwards.
- Looking at him: `Looter.ThirdPerson.FrontArmLength <cm>` brings the front view close (110-120); `play.py aim [0]`.

## Ember powers

The heroes' active powers (Docs/Story.md, "The heroes"; the user's calls 2026-10-15: three owned from the start, one
equipped, Q, the HUD socket by the weapon column, the powers page as the inventory's second tab). The contract the first
round was built to: the shared headers in `Source/AI_Looter_Shooter/Embers/` (EmberTypes, EmberDefinition, HeroDefinition,
EmberPower, EmberComponent); ids, colours, cues and file ownership in the round's `Saved/Embers/CONTRACT.md` (not
committed; its content is in this section once the round lands). The agents' notes follow.

- **Ids (never change: the session saves the equipped one by it):** `DustDevil` (Wind, #e8f6ff), `Sundown` (Dusk,
  #ffb24d), `BorrowedTime` (Debt, #c9b4ff); hero `Ellis`. Cues: `Ember.DustDevil.Use`, `Ember.Sundown.Start`/`.End`,
  `Ember.BorrowedTime.Start`/`.Settle`, `Ember.Ready` (off cooldown), `Ember.Denied` (a press that did nothing).
- **Making a power:** (1) a `UEmberPower` subclass in `Embers/Powers/` (CanActivate, Activate, Tick while it lasts, End
  with bInterrupted; it leaves the world as it found it, and documents which definition fields it reads); (2) its
  definition `/Game/Data/Embers/DA_Ember_<Id>` written by `Tools/Unreal/create_ember_assets.py` (POWERS list: id,
  words, colour as sRGB hex, Inked icon name, cooldown, duration, Damage/Range/Radius/Strength, Extras by name,
  `power_class` = `/Script/AI_Looter_Shooter.<Class without U>`); (3) its place in the hero's list (HEROES: the first
  is equipped in a new game). Run the script after the C++ is built (`Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/
  Tools/Unreal/create_ember_assets.py"`): it prints `EMBERS ... made/updated` and `EMBERS done`, rewrites every field
  it names, leaves a missing class empty with a warning (rerun once built).
- **The component** (`UEmberComponent` "Embers" on ALooterCharacter, like the grenade's "Throw"): the hero is the
  Blueprint's `HeroDefinition` or `DA_Hero_Ellis`; one power object per owned power, made at BeginPlay (lazily on first
  use too, so a restore before BeginPlay works). Clocks in its own tick (game time; off while Ready): `CooldownLeft`
  from the use (through the duration), `ActiveLeft` for the duration; an instant power (duration 0) ends inside its use.
  The gate's order: NoPower (none equipped, or no class), Dead, Busy (scene, pause, menu, mantle/vault, gun swap, melee
  strike, grenade throw), Active, Cooldown, then the power's CanActivate (a power answering Cooldown/Active reads as
  Refused). A denied press plays `Ember.Denied` (not for Dead or Busy) and fires OnDenied. Death or a scene interrupts a
  running power at the next tick; EndPlay interrupts too. A swap mid-cooldown keeps the wait (held to the new power's
  cooldown); no swap while one runs.
- **Damage:** `UEmberDamageType`, applied with the player's controller as instigator and NO causer: AwardKill gives XP
  through the instigator, `AWeaponBase::FindKillWeapon` finds no gun without a causer (no notch).
- **The key:** binding `Ember`, "Ember power" under Combat, Q, the left bumper (fixed on pads), in the character context
  (`UKeyBindingSubsystem::AddEmberAction`, `GetEmberAction`); bound on Started.
- **The save:** `ULooterSessionSave::bHasEmbers` + `Embers` (`FEmberSave`: equipped id, cooldown left), captured and
  restored beside the grenades (`SessionSubsystemWorld.cpp`). An older save reads as none captured: the first power,
  ready; an id the hero no longer owns gives the first; a cooldown is held to the power's own.
- **Console:** `Looter.Embers.Use [id]` (equips the named one, ends a running one, fires past the cooldown; the body's
  gate still applies), `Looter.Embers.Equip <id>`, `Looter.Embers.State` (hero, owned with numbers and class, state,
  clocks, what blocks a press). Tests: `Looter.Embers.Component` (a bare actor in a test world, a probe power
  `Tests/EmberProbePower.h`, clocks ticked by hand), `Looter.Embers.Keys`.
- **The HUD socket** (`UHudEmberSocketWidget`, bottom-right LEFT of the weapon slots, its bottom level with the last
  slot's, 34 px off them: the slots' 12 px gap plus the 22 px the slot in hand reaches left as it lifts). It is NOT in
  the weapon cluster (which hides with no gun in hand; Q works unarmed): a row of its own on the HUD canvas at the
  cluster's anchor, the socket then a spacer as wide as the Arms row (slot row 122 + 14 + cartridge 51), lifted over the
  cluster's two lines (52 px); it takes the cluster's fade while the cluster shows. A change to the slots' or the
  cartridge's widths must change `ArmsWidth` in PlayerHUDWidgetLayout.cpp too. The slots' 64 px ring
  (disc a background, rim, gunmetal, hairline; `HudEmberSocketWidgetPictures.cpp`, the slots' radii), the power's Inked
  icon in a 36 px box, the Ember key's tab under it (grenade-style plate; wider per extra letter, `TabWidthFor`), the
  power's name (10 px) under that. Ready: the orange ring and a glow in the power's colour beating 0.18-0.6 over 2.4 s.
  Running: an arc in its colour from 12 o'clock round the gunmetal, shrinking with `GetActiveFraction`, the name in its
  colour. Cooling: a dark pie (`Color::Track` 0.8) covering the share still to wait, drained clockwise, the seconds
  (rounded up, never 0) in white 20 px, the icon greyed. The pie is one `MakeLines` stroke as wide as the radius (less
  1.5 px at the middle, so its inner edge never folds); pie, arc and seconds are drawn in `NativePaint` with the style's
  opacity, so they fade with the cluster (a `ULoadoutPaintLayer` would not: its painter gets no widget style). Denied
  Cooldown/Active: a 0.4 s shake (4 px); the rest a 0.35 s dim; no sound (the component plays `Ember.Denied`). Back to
  Ready (OnStateChanged): a 0.35 s pop (scale 1.18, the ring flashing white). `PlayerHUDWidgetEmbers.cpp` binds the
  pawn's component each frame like the grenade's; a denied press or a state change brings the column forward.
- **The powers page** (`UEmbersWidget`, `EInventoryPage::Embers`, the inventory's second tab "Embers"; the Ledger is
  third): three 440 x 560 cards side by side (nature chip in the power's colour, a 132 px medallion ringed in its colour
  with the 92 px icon, name, description, cooldown and duration via `SecondsWords`, the state line), the equipped one
  with an orange EQUIPPED tag and its colour's glow; the chosen card's edge orange. A / D, arrows, D-pad (W / S too)
  choose; E, Enter, Space, A, the Interact key or a click equips (`EquipIndex`); a refusal while a power runs says
  "<power> is running: it finishes before you can swap" with `UI.Denied`; an equip plays the loadout's `UI.Equip`; the
  choice moving plays `UI.Hover` (the cards' own sounds are off). Pages: 1-5 (`LoadoutParts::PageForNumberKey`), the
  shoulders to the neighbours (loadout's LB stays Move); the inventory key closes, as on every page.
  `LoadoutParts::NumPages`/`PageTabName` hold the tabs' order and words; the control hints now say "Then 3: the Ledger",
  "Then 4: your missions". `Looter.MenuShots` has an `embers` step, `Looter.HudShots` an `ember` step
  (`Looter.Embers.Use`: Dust Devil dashes the player in the picture).
- **Icons** (`Art/Icons/InkedIcons.py`: `dust_devil`, `sundown`, `borrowed_time`, in the ammo icons' shared box and ink
  width; `EMBER_ICONS` makes them square): Dust Devil three tapered spiral streaks (`offset_band`) round a ringed eye;
  Sundown a half sun on a horizon bar, four tapered rays, two bands cut across the sun; Borrowed Time an hourglass
  between end plates, the lower bulb shaded with a heap and a stream, a slanted ledger rule through the waist with its
  edges cut so it reads over the glass (posts and a top sand line were dropped: the posts thickened the ink at 30 px,
  the line made a face). The generator also writes `InkedIconData::Find` (every icon by name), behind
  `LooterUI::FindInkedIcon`, so a definition's `IconName` finds its icon. Tests: `Looter.Embers.Pages`, `.Socket`.
- **Dust Devil** (`UEmberDustDevil`; reads Damage, Range, Radius, Extras FlingMin/FlingMax/Dizzy/FovKick, the duration
  as the dash's time): the dash is laid out first (`PlanDash`, 30 cm steps): each step's ground found by a world-static
  trace (`LooterWorld::StaticGeometryParams`) within a step's rise plus a 45-degree slope of the step, else it stops at
  the edge; a capsule sweep (the player's own responses, pawns, physics bodies and destructibles ignored; lifted half a
  step, radius 0.9) stops it short of walls, the playable area's included; `APlayableArea::Contains` too. Under 1 m:
  refused. A start high in the air keeps its height and ends falling. Run on the component's Tick (the player's clock):
  MOVE_None while the capsule is teleported along the path (an even pace blended 0.4 with an ease-out: fastest as it
  leaves, 0.6 of the mean speed as it arrives), Velocity set to the dash's for the animation, bInvulnerable on (only if
  it was off), then walking on at 700 cm/s. End(false) runs the rest at once; End(true) stops where it is. Creatures
  within Radius (flat) and 220 cm of height of the stretch just run, in sight of it (world-static trace), are struck
  once: damage, then `UCreatureKnockComponent::Knock` along their side of the path (+0.2 of the dash's way), 400-500
  cm, arc top 110 cm. Felt: `UPlayerViewComponent::SetPowerFieldOfView` (up over 0.07 s, back over 0.45 s after), 80
  speed lines a second (world streaks 200-650 cm ahead, 70-190 cm round the eye, never low, 0.16-0.24 s), a 16-puff dust
  ring with grit and five spinning wisps at the start, 9 at the end, a dust puff every 110 cm along the path, per
  creature a dust burst, grit, `Player.Melee.Hit` and a small view jolt; a lean kick at the start, a dip at the end. What
  lingers after (the settle, the lines) runs on a next-frame timer chain of its own.
- **Knocked and dizzy** (`Creatures/CreatureKnockComponent`, a default subobject "Knock" on every `ACreatureBase`;
  `TickBrain` waits while `HoldsBrain`, as for a stagger): `Knock(Away, Distance, Height, Dizzy)` launches like a hop
  (`LaunchVelocity`: up for the arc's top, across for the distance in that air time; its speed replaced), the distance
  times the rank's share (Rare 0.85, Epic 0.7, Legendary 0.45, Boss none) over the square root of its size; the landing
  must have ground within 110 cm over / 150 cm under its feet (sizes scale it) and inside the playable area, else 0.75,
  0.5, 0.3 of it, else only dazed. The body cartwheels once over the planned flight (the mesh turned round the
  capsule's middle from its captured rest transform), lands (LandedDelegate, the floor found, or 2.5 s) with a dust ring,
  grit and a dull `Player.Melee.Hit`, then reels for the daze (at least 0.2 s): the tumble settles upright over 0.15 s,
  a 9-degree sway at 1.6 Hz, three gold sparks (FWeaponFX streaks) circling 24 cm out over the head, drawn only within
  50 m of a view. Death, EndPlay or the end of the daze put the mesh back. Bosses and the dead are never knocked or dazed.
- **Sundown** (`UEmberSundown`; Strength = the world's speed, Extras Pitch 0.75): `SetGlobalTimeDilation(Strength)`;
  the player's actors get CustomTimeDilation = 1 / the world's effective dilation: the pawn, everything attached (each
  Tick, for a gun swapped in), the controller (the look), the camera manager (the kicks) and the HUD actor.
  `UBulletSubsystem::Tick` advances each bullet by the shooter's CustomTimeDilation, so the player's shots fly at full
  speed. The component ticks on the pawn, so its 6 s are the player's. `SetGlobalPitchModulation(Pitch, 0.35 s)`, back to
  1 at the end. Refused while time is already bent (a boss's slow-motion death). End undoes only what is still its own
  (the world's dilation if still the one it set; each actor's if still the compensation); another slow taken meanwhile is
  followed (the compensation recomputed each Tick). `AWeaponBase::GetOwnerTimeScale` (the holder's CustomTimeDilation)
  divides the fire-rate and reload timers' seconds, so a gun in the player's hands fires and reloads at full speed; the
  melee and throw cooldowns still read world time (short, left as they are).
- **Borrowed Time** (`UEmberBorrowedTime`; Strength = the share a kill strikes off, Extras PayBackSeconds 3): binds the
  player's `UHealthComponent::OnDamaged` and `OnHealthChanged` (the last known health tells what a hurt really took) and
  the progression's `OnKillXP` while it runs. A hurt is healed back inside OnDamaged, before the health component counts
  a death, so a killing blow is borrowed too; the debt grows by the damage. `NoteKill` strikes the share off (a violet
  soul-light flare at the one killed). End: death forgives it; otherwise (a debt of 0.5 or more) it's drained evenly
  over PayBackSeconds with `Drain` (never the last point), on a next-frame timer chain at the player's clock, with
  `Ember.BorrowedTime.Settle`, a heavy view dip and a 0.25 shake for the pay-back. Violet motes rise round the feet
  while it runs; a violet puff where each hurt is taken back. The HUD reads `GetDebt`, `GetDebtFraction` (of the most
  health), `IsSettling`; after End the object is `UEmberComponent::GetPowerFor("BorrowedTime")`.
- **The powers' sounds** (`Art/Sounds/recipes/embers.py`, group `embers`; all 2D): Dust Devil a two-ear gust, whump,
  grit and a receding whirl; Sundown a tape-stop chord sinking an octave, a slowed bronze chime, a drone and a heartbeat
  thud (End: the chord wound back up, a shimmer); Borrowed Time a ledger's cover, a nib, a clock tick and a cold minor
  chord (Settle: the ledger slammed, three slowing ticks, the chord sinking flat); Ready a breath of flame and a chime;
  Denied a fizzle and a muted click. Tests: `Looter.Embers.Power.DustDevil`, `.Sundown`, `.BorrowedTime`
  (`Tests/EmberPowerTests.cpp`: definitions made in the test; the dash stepped by hand; a test level has no game mode,
  so the damage itself isn't checked there; hurts are broadcast as `AActor::TakeDamage` hands them out).

## The hero pick (the select screen, built 2026-10-19)

Before a new game the player picks one of five (Docs/Story.md "The heroes"; contract `Saved/Heroes/CONTRACT.md`, not
committed: its names are here). The session keeps the pick; the character takes the hero's body and powers.
- **Ids (never change, like part keys):** `Ellis`, `Odessa`, `Crane`, `Gauge`, `Pike`, the roster's order
  (`UHeroDefinition::RosterIds`). Roles: the Revenant, the Cardsharp, the Unpaid, the Iron Hand, the Surveyor. Colours
  (sRGB): Ellis #5AD8FF, Odessa #D04A68, Crane #B9D98C, Gauge #FFB347, Pike #3FBFAA (data, like an ember's).
- **Data:** `DA_Hero_<Id>` in `/Game/Data/Embers` (name, role, story, colour, `Portrait` `/Game/UI/Portraits/T_Portrait_<Id>`,
  `FigureMesh` `/Game/Art/Characters/SK_<Id>`, `Retargeter` `/Game/Player/Retarget/RTG_MannyTo<Id>`, three powers) and the
  fifteen `DA_Ember_<Id>`, all from `Tools/Unreal/create_ember_assets.py`. The twelve new powers (`LuckyStreak,
  FiftyTwoPickup, DoubleOrNothing, TollChain, GraveWard, LastRites, SlagBomb, IronHide, RailSpike, Spyglass, Tripwire,
  LongShot`) are data only: words, nature, colour and starting numbers in the script (the demo's four from the cloud
  handoff, the rest decided 2026-10-19 as starting points), no class, no icon (`power_class` None leaves the field as it is;
  name the class there when its code lands). The portrait, mesh and retargeter fields are set by `SetLookPaths` (Python
  sets a soft pointer only from a loaded object) only once each asset exists: a soft pointer to a missing package failed
  the editor's asset validation on save (AssetReferenceRestrictions, 2026-10-19). Meanwhile the game finds each by the
  naming rule (`GetFigureMeshPath` and the rest), so a hero lights up as soon as the model lands; rerun the script after
  each to fill the fields.
- **Playable** (`IsPlayable`): the mesh and retargeter load and every power has a class. Only Ellis today; the others show
  "In the works". `Looter.Hero.Unlock 1` (console variable; never in shipping) lets one be picked anyway: Ellis's body
  stands in, their powers are owned but can't fire (NoPower).
- **Portraits:** the user's picks copied to `Art/Portraits/Picks/<Hero>.png` (Ellis A, Crane A, Pike A, Gauge C, Odessa A)
  and imported by `Tools/Unreal/import_hero_portraits.py` as UI textures (BC7, UI group, no mips, never streamed, clamped).
  Paintings, so textures (not kit vector art). The screen cuts them by UV region (`HeroSelectParts::ECrop`: whole, face
  square, eye band), so one texture serves every size. Run order: import, then create_ember_assets.
- **The save:** `ULooterSessionSave::HeroId` (new within version 3; none reads as Ellis: `GetHeroId`). `StartNewGame(Index,
  HeroId)` writes it first and refuses a hero that can't be picked (`ApplyNewGameHero`). `PlaySession` on an empty slot (the
  console's `Looter.Session.Play`) starts Ellis. `FSessionSummary::HeroId` names it on the card.
- **The hero played:** `USessionSubsystem::GetPlayedHeroId` = the session's, else (no session: Looter.PlayLevel, a level
  from the editor) `GetNoSessionHeroId`, Ellis unless `Looter.Hero <Id>` set another (kept for the editor run).
  `FindPlayedHero(context)` is what the character asks: `UEmberComponent` binds it on an `ALooterCharacter` (else the
  Blueprint's `HeroDefinition`, else Ellis); `UBodyFigureComponent` wears its mesh and retargeter when both load, else
  Ellis's, and restarts its animation on a change. A holstered gun is any material slot ending in `HolsterGun`.
  `Looter.Hero <Id>` mid-play (no session only) rebinds the powers and re-wears every body figure in the world;
  `Looter.Hero` alone lists the roster and what each hero still lacks.
- **The screen** (`UHeroSelectWidget`, `UI/Menus/HeroSelect*`): an empty slot's New Game opens it over the island, the
  main menu's column hidden; the main menu hands it every key and its clock. Laid out at 1600 x 900, scaled to fit (like
  the inventory's pages): header top left (bar, SESSION N · NEW GAME, CHOOSE YOUR HERO, one line), the chosen portrait
  500 px on the stage (top clear of the header: the frame's fittings touched its line at 520) with a 1080 px glow in the
  hero's colour (alpha 0.5, breathing; the space is meant for a live 3D stand later), the roster under it (five 176 x 140 cards: eye band, number key, colour strip, name, role; greyed and stamped IN THE
  WORKS when not playable), the panel on the right (496 x 820: role, name, colour rule, story, EMBER POWERS with three
  rows: medallion with the Inked icon or the power's glow, name, nature chip, words, cooldown; status line; Back and
  "Play as <name>", disabled "In the works"). Greyed = the image tinted 0.42 grey under a 42% IconShade wash (Slate has no
  desaturation without a material).
- **Keys:** one roster row then Back/Play (`FMenuNavigator`, wrapping). Left/right move and the choice follows; 1-5 and
  the bumpers choose; A or Enter on a card or Play confirms; B or Esc back to the picker (focus on that New Game). Mouse:
  click a card to choose, Play to confirm; a card under the mouse lights. Sounds: UI.Hover as the choice moves, UI.Click
  for a click, UI.Equip + Ember.Ready on confirm, UI.Denied (and the reason on the panel) for a hero in the works,
  UI.Back. Confirm plays a 0.85 s beat (portrait lifts 3.5%, the glow swells) before `OnConfirmed`; keys are ignored
  until the level opens or `CancelConfirm` (StartNewGame refused).
- **Session cards** name their hero: the face cut in a 72 px square with a hairline in their colour, the name in their
  colour under the level; the delete popup names them too.
- Tests: `Looter.Heroes.Definitions`, `.Playable`, `.Save`, `.Screen`, `.Menu`, `.Character` (`Tests/HeroTests.cpp`).
  `.Playable` expects only Ellis while the twelve powers have no code: update it as they land.

## Seeds in Ransom's Rest (the saga's first clues, built 2026-10-16 overnight)

The list and the words' facts: `Docs/Story.md` "Seeds in Ransom's Rest" (the seven towns' saints in its Areas table).
Three agents built it to a contract (`Saved/Seeds/CONTRACT.md`, not committed; what it fixed is here). Everything placed
lives in the outliner folder `RansomsRest/Gameplay/Seeds`, from `Tools/Unreal/build_area_seeds.py` (hooked into
`build_area.py gameplay()` after the valley script; alone: `build_area_seeds.py RansomsRest` clears and refills only its
folder). The pieces:
- **The hooks (C++):** `AReadableProp.SetsFlag` (Python `sets_flag`; set after the `Readable.<Id>` event; reading again
  is harmless), `FSpeakerTopic.SetsFlag` (set only when that topic's own lines were said: not on a turn-in, never by a
  mutter; `without_flags` on the topic makes a one-time talk), `AFarSignalLight` `bSteady` (no Morse; a 0.5 Hz breath of
  ±8%) and `ShownState` (a lighting-state gate like `ADuskScenery`'s; None = any state; its constructor still defaults
  `ShownWhen` to The Last Watch's, so a new light overrides it), `AWantedPoster` variant `Seven` (tag `SevenPoster`, not
  `WantedPoster`: Side 1 never counts it; never torn, no hitbox; `CrossedFlags` = `Ember.Paid.Deacon/Ned/Ira/Constance/
  Barrels/Lena/Tobias`: each face gets a small `SevenCross` decal, sort order +1, once its flag holds, redrawn on
  `UMissionRunner::OnChanged`; the faces' cells `SevenFaces` in `FWantedPosterAtlas`, overridable by `Atlas=` in
  DefaultGame.ini), the cold open's spark (`AColdOpenCastGlows.cpp`: `FlareEmber()` at the `DeaconFires` moment: x3 in
  0.15 s, down to 0.35 over 1.2 s, back to 1 over 2 s; glow and light both; not while skipping). Tests `Looter.Seeds.*`.
- **The Ledger's loose pages:** six `AReadableProp`s with `SM_LedgerPage` (ids `LedgerPage_<Town>`, flag
  `Ledger.Page.<Town>`), their words generated from `story_texts.LEDGER_TOWNS` (one dict a town: saint, carrier, habitat,
  where found, the page's text, the lines after reading). Each opens a Ledger town page: `DA_Bestiary_Town_<Town>`
  (`create_bestiary_pages.py`; STORY_CHARACTER pages in the NPC section, "???" until found, `found_when with_flags`;
  not LEDGER_NAME: `Looter.Ledger` wants exactly the seven names there). The seventh (Stoneweight Quarry) has no page
  on the Rest. Where: the lockup floor by the bunk (The Needles), a trunk in the depot's baggage room (Highwater
  Landing), the vestry door's step (Ransom's Rest: entered a month before the robbery, Ellis's name twice), Ruth's table
  at Calder's Post (Dustwater Mesa), the Providence's tag board under the IN hooks (Furnace Hollow), the mooring ledge
  (Saint Agnes Mire).
- **The Deacon's letters:** `SM_DeaconLetter` (folded, sealed; one model for both). `DeaconLetter_Cleft` wedged by the
  mooring post, never sent (flag `Letter.Deacon.Cleft`); `DeaconLetter_Saloon` standing in slot 3 of `SM_PostRack` on
  the Gilt Spur's front, right of the doors, 1.4 m up (the rack's sockets `Slot_1..8` sit where a standing letter's
  bottom middle rests, leaning back 8°; the flat model is stood nose down (pitch -90 in its own frame), then given the
  slot's lean, then lifted half its length, or it sinks into the shelf). Postmarked Halfway Junction, 9 August, to Mrs.
  D. Ransom, unopened: his warning to get Abel and the lantern off the Rest.
- **The map of the Reaches:** `SM_ReachesMap` (90 x 60 cm, `MI_ReachesMap` on M_World; texture painted by
  `Art/Textures/ReachesMap/make_reaches_map.py`, numpy + Rye, BC/N/ORM, 2048 x 1024 stored from a 2048 x 1366 sheet) on
  the ticket office's north wall over `SOCKET_AgentDesk`, a readable (`ReachesMap`). Halfway Junction at the heart, the
  Rim and "UNSURVEYED" west, Highwater Landing "END OF THE LINE", "W. PIKE, SURVEYOR" (Wendell Pike, a nugget ART
  added). Sloane's window (`DA_Lines_AgentWindowLily`, and the valley script's `LINES` fallback) names the Junction
  and Highwater Landing once the line is open.
- **The Seven on the wall:** the poster in the Rangers' board's free strip (layout.json `gameplay.posters`, offset
  [0, -12, 0] between Ellis's poster and Calder's note; 25 x 29.7 cm). Art: `Tools/Blender/looter_posters.py` cells
  `Seven` (U 0.75-1, V 0.703-1: "THE SEVEN / OF THE DUNNE GANG", seven inked busts in ovals told apart by hat and
  outline, names, "DEAD OR ALIVE", Calder's signature) and `SevenCross` (U 0.25-0.359, V 0.891-1: an ink X on alpha).
  After rerendering the atlas run `build_decal_materials.py`.
- **The road at dusk:** the Gravewind's wisps were already the river of lights at Dusk; added the second light: a
  steady `AFarSignalLight` due west of `Place_GravewindPoint`, 9 km out, 30 m up, size 5500,
  strength 30, pale cold white, `shown_state Dusk`, `shown_when after Main6`; and Hob's `DA_Lines_HobMain6Point` on his
  after-Main6 bier perch (Ellis sees the light, then *"Don't look at that one, sunshine."*). Not gated on Grave Sight
  (there is still no vision mode: only the Reliquary's flash). `Looter.FarLight 1` shows a far light for a check.
- **What Ada gave up:** Aldana's topic `(after Main4, without_flags Ada.Told) -> DA_Lines_AldanaAda`, `sets_flag
  Ada.Told`, placed ABOVE the after-Main topics (first match wins); the `ReliquaryLid` readable (no mesh, at the
  Reliquary's `SOCKET_Interact`, shown after Main4, "Look": the old toll mark scratched out under the lid, which the
  model shows prised up at one edge, not turned over).
- **Hob's feathers:** `DA_Lines_HobIdle` (the three lines, in order) as Hob's speaker point's `Mutters`
  (`story_texts.MUTTERS['Speaker_Hob']`, after Main1; `place_hob` sets them). They ride the town-life mutter clock
  (one line per mutter, 75 s rest, within 7 m of his perch) and play the town-life voice cue like any muttering point.
- **The spark:** see the hooks. **Fenn's fare:** untouched.
- Lessons: a `.ps1` argument like `/Game/Maps/X` run through Git Bash becomes `C:/Program Files/Git/Game/...` (run
  perf and anything taking a `/Game` path from PowerShell); `stage_import.ps1` wants its names as ONE quoted
  comma-separated string from PowerShell (a bare comma list is split into an array and joined with spaces);
  `Looter.ImportModels` resolves a relative folder against the editor's working directory, so pass the absolute path.
  Look sheets for the user: `Saved/Concepts/Seeds/` (`Tools/Blender/seeds_sheet.py`).

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
    Run it from PowerShell, never through Git Bash: bash rewrites `/Game/Maps/...` into `C:/Program Files/Git/Game/...`
    and the game waits 15 min on a map it can't load (every `/Game/...` argument to a .ps1 has the same trap).
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
