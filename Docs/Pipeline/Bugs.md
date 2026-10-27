# Pipeline: Bugs and lessons

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Bugs and how they were fixed

Each line: bug (date). Cause. Fix.

- A modal "build lighting" message blocked every MCP call (2026-10-27, building Lvl_MenuTeropa). Cause: the menu-map script
  (`build_menu_teropa.py`) set the World Settings' `force_no_precomputed_lighting` (bForceNoPrecomputedLighting); the editor
  answered with a message box that waits for OK. Fix: find the window with `Tools\editorwindows.ps1` and click OK
  (`Tools\dialogclick.ps1`; a one-line Message's OK is 535 127). The line is still in the script (try-guarded, around line 269; the
  map has no lights and its materials are unlit): on a rerun watch for the dialog, or drop the line. Lesson: a script that sets a
  World Settings flag can raise a modal; check the editor's windows when an MCP call hangs (`CLAUDE.md` Safety).
- Moving the default map to the menu's own scene would have turned the session fallbacks into the menu (2026-10-27, caught before
  the build). Cause: `USessionSubsystem::NewGameMap()` returned `GameDefaultMap`, and a session whose level is gone (Continue,
  a legacy save) and `OpenMainMenu` both used it. Fix: `MenuMap()` is the default map (the menu, and Save & Quit), `NewGameMap()` is
  a fixed `/Game/Maps/Lvl_TutorialIsland`. Lesson: a setting that names "the default map" is read by more than the menu.
- `Looter.Encounters.Camps.Placement` failed on the OrchardEdge camp (2026-10-27): its crate spots tucked at Coffin Rock's foot
  (the chest-spot move, `Levels.md` "Roads and chest spots") were too near the rock (the test judges the camp against Coffin Rock). Fix: the layout's crate
  spots moved out to 4.4 m off the rock (`layout.json`, a `world_2` patch). Lesson: after moving a camp's chests run
  `Looter.Encounters.Camps`.
- The new game's intro would have played where the cold-open test expects the cold open (2026-10-27). Cause: `BeginIfDue` now
  starts `NewGameIntro::PlayRestIfDue` first. Fix: `FColdOpenFirstArrivalTest` sets the `Intro.Seen` flag
  (`NewGameIntro::SeenFlag()`) on its campaign record. Any new test of a new game's first arrival does the same.
- Old saves already in Main 6 without the mine (2026-10-27, a known risk of promoting Dead Lift and Back Pay): Main 6 keeps running
  and "Talk to Hiram Coyle" is listed too; talking to Hiram switches the campaign's active main mission (Main 6 then restarts
  after Back Pay). Recommend a new game; no migration was written.

- A dead player could still fire, reload and aim (2026-10-27): death only blocked moving and looking (a trigger held or sent while
  dying landed shots 2 s later). Fixed 2026-10-27: the weapon and the aim check the holder's health; back at respawn (Game.md).

- A gameplay pass run right after the level opened placed things with no ground (2026-10-27): 4 of 7 bounty arenas, Calder's
  Post roost, Abel's vigil stone, caches, a bench left out, camps at z 0, and the level saved and committed that way. Cause:
  the terrain tiles' collision wasn't there yet (open_clean.py, then the pass at once). Fix: `AreaBuild.require_ground` stops
  the pass when no tile answers a trace; still read the pass's warning count (0 is right) before committing.
- Playtest 2026-10-27, round 1 (the user's notes, `Docs/Work/2026-10-27-Playtest/NOTES.md`):
  - Knocking Below's cellar dead spawned on the workshop floor (note 20). Cause: `SnapToGround` from a middle inside a stair's
    hull or low ceiling saw no roof (the look up starts inside it), so the look down started over the floor upstairs. Fix: settling
    never lifts a creature more than a step (`SettleMostRise`); past that it looks again from just over its own feet
    (`CreatureBaseSteering.cpp`). Cellar replayed 3x, all on its floor.
  - Vigil stones readied together after a Save & Quit (note 36). Cause: `Evaluate` and `BringBackAfterLoad` readied every stone
    whose death was in an earlier load (the global `SessionLoads`). Fix: each stone counts 5 minutes of play from its own
    `DefeatedAt` (play time is saved); the load rule and its fields removed. Reverses the 2026-10-12 "at once after a Save & Quit".
  - Apples floated round the trees (note 11). Cause: `hang_apples` placed them outside the leaf cards, on nothing. Fix:
    `Trees.py` keeps each limb and side branch (`plant.branch_limbs`) and hangs each apple on a 3-sided bark twig from the nearest
    branch with a 12 cm stem into its top, on the underside of the leaf shell (+18 tris an apple; same names and counts).
    Not previewed in Blender yet: `Trees.py -- --preview Apple_A Apple_B Apple_C`, then `Tools\models.ps1 -Only Trees`.
  - The mountain pond vanished from below, no underwater look (note 30). Cause: `M_Water` was one-sided. Fix: two-sided; from
    below the colours go x `UnderDark` 0.55 with no sky tint or foam plus an `UnderGlow` 0.6 emissive so the back face is not
    black (`build_world_materials.py M_Water`). Underside brightness is a guess. The mountain farm house's floating fences (same note) were not in this round.
  - The game stuttered when a legendary dropped (note 17). Cause: the 0.3x world slow, and on the landing frame the column and
    name pop widget were built and `AWeaponBase` LoadObject'd the muzzle material and plane on every gun spawn (a chest legendary
    before the first shot also initialised `FWeaponFX` there). Fix: no slow; pooled column and name pop built at play start;
    muzzle lookups cached; a burst in its place (Gunplay.md).
- Playtest 2026-10-27, round 4 (the play-check of melee, boss intros, swimming and grenade kinds, and the round 3 nits):
  - A pawn in a test level takes no damage from `ApplyPointDamage`, and a test actor's health never fills (`Looter.Grenade.Kinds.
    Cinder` failed on the first build). Cause: a test level has no game mode, and `APawn::ShouldTakeDamage` refuses
    damage without one; a health component on a spawned actor binds to the actor's damage and fills its health only in its
    `BeginPlay`, which a test level never runs. Fix: the test shows who the bed catches with spiders and where the burn lands with a
    block that is not a pawn (`SpawnBlock` as `ECC_WorldDynamic`, since the burn's search looks for pawns and dynamic things, never the
    world's static shapes) carrying a `UHealthComponent` (`AddInstanceComponent`, `RegisterComponent`, `Post->DispatchBeginPlay()`,
    then `SetMaxHealth` and `ResetHealth`), and runs the burn inside an `FEditorScriptExecutionGuard` (the actors' own events need
    letting through before play). Lesson: in a test world, damage to a pawn proves nothing; use a non-pawn with health, and call
    `DispatchBeginPlay` on it (`SwimTests.cpp` does the same for its water).
  - Gauge's piston fist stayed in view after `Looter.Hero Ellis` (melee HANDS). Cause: a tool part made while its root was already
    hidden showed at rest, and a root's flag does not hide its children. Fix: every part born hidden and set hidden at rest and on a
    hero change.
  - Ellis's spade read like a spear edge-on, and its shaft rather than its head crossed the view at the blow; Crane's fingers stood
    open beside the gaff; Odessa's cards popped out of sight a second after a kill (round 3 nits). Fix: `FaceView`, `AimAtTip`,
    `ClosedRound`, and a card on a dying body falls, lies and shrinks away over `Looter.Embers.FiftyTwo.Fall` 2 s (`Heroes.md`,
    `EmberPowers.md`).
  - Boss intros: Abel's first line played twice (the scene cleared the captions and repeated the fight's own line); his buckshot landed
    0.5 s after the hand back; the low "Lantern" eye went under the deck; the Foreman's shot at -4 EV was black; the card's epithet
    differed from the boss bar's. Fixes in `Enemies.md` "Boss intros".
  - The new game intro ignored Space in Ransom's Rest (the menu part said HOLD SPACE) and ended on white with no title. Fix:
    `UIntroWidget::SetPollsKeys`; the intro ends on REVENANT (`Game.md` new game intro).
  - Swimming started at 135 full-size cm of water (115 on the player): the island's 1.2 m pond swam. Fix: `Looter.Swim.StartDepth` 160
    (136 on the player), and `SWIM_DEPTH` in `build_area_swimwater.py` kept equal. The `Player.Underwater.*` cues had not been rendered
    into the bank (`Workflow.md` "Sound recipes").

- Flat bright tan patches on level ground (Ransom's Rest's street by the boards, Whitlock Fields' path; 2026-10-22).
  Cause: M_Terrain's SIDE_NORMAL_CODE normalized 0 where the vertex normals pack to straight up (Facing 0): NaN, and the
  steep-relief lerp's weight 0 keeps a NaN (WorldNormal buffer black there; shadows, local lights, places, Nanite all
  ruled out). Fix: it returns the face's own normal when the blend is ~0 (build_world_materials.py M_Terrain rebuilt).
- Giver ? and ! marks never drew (2026-10-22). Cause: AGiverMarkActor checked AActor::Owner (never set). Fix: its Giver.
- Sexton couldn't wear the objective highlight (Looter.Missions.Highlight.Figure, 2026-10-22). Cause: SextonGlint
  (CHAR-MAT) on additive M_StylizedGlow; any see-through slot rules a mesh out. Fix: on M_CharacterGlow (CHARACTER_GLOWS).
- Looter.Story.UnfinishedBusiness.Placed counted only SM_FenceRail; the barn yard's rails are SM_FenceRail_B/_C since
  World variety 12.5 (2026-10-22). Fix: the test counts the look-alikes (walls too).

- Coffin Rock's secret couldn't be climbed: the player slid off the stepping boulders after four steps (2026-10-22). Cause:
  scaled SM_Boulder_A tops are rounded. Fix: a fallen slab (Boulder_D in three overlapping pieces, the butte's MI_RockCliff)
  leaning up its north edge at 29 degrees, and a hidden collision plank (the engine cube, invisible) on the ramp's line
  over it: the bare pieces had 80-96 cm dips at their joins (`build_area_secrets.py` RAMP_*). Walked in PIE: ground to top
  in 4 s. Lesson: a stretched rock is no floor; walkways get a plain hidden collider.
- `play.py press Move` walked sideways (2026-10-22). Cause: IA_Move's x is right, y forward (the docstring said the reverse).
  Fix: the docstring; walk ahead with `press Move <s> 0 1`.

- A `Looter.Hero` swap mid-play (Odessa to Crane) tripped "SK_Crane has incorrect BoneVisibilityStates" (93 against 145)
  and the engine's bone-count check (2026-10-20). Cause: `UBodyFigureComponent::Wear` showed every material section
  (which refreshes the bones) before `InitAnim` had sized the bone arrays for the new body. Fix: show them after InitAnim.

- Red-and-green/white blotches on the heroes' bare skin (Pike's hand, Odessa's cheek, Crane's face; 2026-10-20). Cause:
  M_CharacterSkin's Subsurface Profile goes through the Separable SSS filter on Medium/High (BaseScalability's
  r.SSS.HalfRes 1 + r.SSS.HalfRes.ForceSeparable 1; only Epic runs Burley); its fitted kernel (~2.7 cm) smeared red into
  blotches and left green, at any r.SSS.Scale, full res too. Not the textures, lights or SubsurfaceAmount. Fix:
  Preintegrated Skin shading (Materials above). Also: build_world_materials.py run during PIE fails (EditorAssetLibrary
  refuses in play mode, does_asset_exist says False, create_asset raises an "Overwrite Existing Object" dialog that blocks
  the editor): cancel it (nothing changes) and build with PIE stopped. take_high_res_screenshot in PIE writes the file
  twice ~0.35 s apart, and a background editor draws late: focus it and wait 1 s+ after the file appears.

- The Bestiary's habitat line ("RANSOM'S REST: RANSOM'S POINT AT THE TOP OF THE BLUFF PATH, ...") ran past the details card's
  right edge (2026-10-20). Cause: an unwrapped kit label. Fix: it wraps (BestiaryWidgetDetails.cpp), as the name above it.

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
      B back out; Page Up and Down or the bumpers turn the sections (the settings menu's four tabs since 2026-10-27); a slider held steps faster.
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
