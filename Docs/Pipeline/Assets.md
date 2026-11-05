# Pipeline: Making assets and sounds

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

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
   - Places (2026-10-22, `PLACE_WEIGHTS`; off on the master, `PlaceAmount` 0): `GroundPlaces` (looter_textures, 1024 px over
     3.6 m) packs four patterns: `T_GroundPlaces_M` each place's ramp position (R dust, G ash, B grit, A coal), `_NA`/`_NB`
     their normals' xy; `T_GroundPlaces_stats.json` their means (`PlaceMeans`). An area's PlaceMap (by world position)
     picks the place; its pattern runs through `Place<Name>Dark`..`Light` (Dark.w: own colour vs the macro's), roughness
     `PlaceRough`..`PlaceGloss` (coal lumps glint); `PlaceCues` x edge fray, y old relief at the edge, z tile m, w old
     stones/tussocks kept inside. Reads inside branches with `DDX`/`DDY` grads (`Texture2DSampleGrad`; compiles, 815
     instructions, 12 samplers). Every 8-bit value at least 1 (PNG infill under zero alpha).
   - Judge a change without Unreal first: `Tools\artrun.ps1 -Script Tools\Blender\ground_preview.py` renders the layers from tour
     viewpoints at eye height (before above, after below) into `Saved/ArtPreviews/Terrain/`. It is a numpy port of the master's code:
     keep it in step with `GROUND_WEIGHTS` and `GROUND_DEFAULTS`. It has no shadows, grass or hollows' damp, so look in the editor too.
   - The detail layer (2026-11-04, `M_World`, the art audit's plan row 4; parameters in the script's docstring, numbers
     `DETAIL_DEFAULTS`, `DETAIL_PLATE_DEFAULTS`): static switch `DetailOn`, off on the master. Off is the old graph exactly
     (M_World and its instances still 317 VS / 229 PS instructions, 5 samplers, 0 interpolators). Static `DetailWorld`: world
     metres in the piece's own axes (scaled engine cubes tile true; rides with a listing barge, which pure world space
     would not). `DetailPlates` draws riveted plates (seams, rivets, each plate's tone; none of our sets has plate seams).
     `DetailGrainUp` 1 turns the sides' picture a quarter (grain up posts and upright boards). `DetailTopTint` lightens the
     frame's up faces over the rest at every distance (treads over risers: a step reads in shade). The barge's `_Deck`
     (round 5): plates 0.85/0.9, MetalWorn 0.3 at 0.7 m (its 1 m blotches read as dirt), Tint 1.8 (Tripo's deck paint
     averages 0.045 linear luma), TopTint 1.25. `DetailMaskAlpha` 1: the base colour map's alpha gates the set's light and
     dark (a paint set's chips on painted zones, none on rust; grain, roughness and plates stay); `MI_TripoDredgeBarge_Walls`
     (PaintWorn 0.65 at 1.6 m, plates 2.0 x 1.0 m) uses it once REBAKE writes the chip weight into the walls' BC alpha.
   - Town looks (2026-11-04, `M_World` static `TrimRecolor`, on `MI_HouseTrim` only, set by the DetailInstances step):
     custom primitive data 0-2 roof tint (shake strip E), 3 H1/H2 trim swap, 4-6 bare siding stain (strip A), 7 paint
     cover, 8-10 paint colour (linear; zero = cream 0xe6dac2), 11 roof grey (desaturate before the tint). Paint keeps half
     the boards' grain (luminance over strip A's mean 0.161) and leaves the dark gaps bare. Table
     `Art/Levels/<Area>/town_looks.json` (sRGB paint, neighbour rule checked), written by
     `Tools/Unreal/build_area_town_looks.py <Area> [check|clear]` (re-run after any rebuild of the level's buildings).
     Round 1 (the plan's multipliers only: x0.45-1.15, a cream lerp) read as nothing; round 2's painted frontier palette
     (chalk, sage, grey-blue, mustard, brick, silvered/tarred bare; silver, honey, tar-paper, moss roofs) reads at 30 m.
     The false fronts' street faces are mostly other materials (WoodPlanks, PaintCream, signs): side walls and roofs carry
     most of it. Cost: MI_HouseTrim 229 -> 269 PS, 5 -> 6 samplers; every other instance unchanged. Lessons: a level keeps
     only the component's Custom Primitive Data Defaults (`set_default_custom_primitive_data_float`;
     `set_custom_primitive_data_*` is run-time only and is lost on save); a mesh with no data reads zeros, never the
     parameter's default (that is the material editor's preview), so the code takes zeros as no change.
   - M_Terrain's Medium path (2026-11-04, `TERRAIN_NEAR` and the code beside it): Quality Switches send material quality
     Medium (r.MaterialQualityLevel 2, EffectsQuality@1) and Low to a cheaper graph; High and Epic take Default (stats
     unchanged: 160 VS / 972 PS / 13 samplers / 14 graph samples). Medium: no side-laid rock normals; thatch, stony and the
     places' patterns read only within 30-40 m (their mean mip past it); the side-laid rock and strata noise only where a
     face can be steep (983 PS, 13 samplers, 6 graph samples). Editor view at 8x filtering, the Aerial: terrain 1.20 -> 0.75 ms
     of base pass; the valley -0.1 ms. The custom nodes' own reads use `View.MaterialTextureBilinearWrapedSampler` (each
     texture object's own sampler took a slot: 17). `FarGround` (static, round 7): the Beyond ring drawn from the macro map
     alone at the layers' far brightness with the steep faces' tint and banding (260 PS, 3 samplers, 1 read; reads the
     same at High at range, so every quality). It is on `MI_RansomsRestRingMacro_Steep` (the ring's actors wear it through
     level.swaps; build_area never clears it, the ring's re-import clears only its parent); `FAR_GROUND_INSTANCES`, set by
     every M_Terrain build and by `build_world_materials.py TerrainFar` (run that if the _Steep instance is ever remade).
     One Custom node does it all (`Leaned` an additional output). On, object space: 442 PS, 8 samplers, 6 interpolator
     scalars; DetailWorld: 340 VS, 450 PS, 9 interpolators (the frame worked out per vertex: per pixel it cost ~0.1 ms
     more). Editor viewport (Epic, 1885x866), BasePass: barge filling the view +0.09-0.12 ms (round 1); a kit wall filling
     ~85% of it 0.64 -> 0.79 ms (plates ~0.04 of it, DetailWorld ~0); the deck plates +0.05 ms over the plain detail.
     Instances: `DETAIL_INSTANCES` (the barge's one slot `MI_TripoDredgeBarge`; its split slots `MI_TripoDredgeBarge_Deck`
     plates 2.4 x 1.2 m, `_Hull` MetalRust 1.5 m, `_Super` MetalWorn light; `MI_BargeKitIron` dark iron in world metres with
     plates 1.8 x 1.2 m, child `MI_BargeKitIronDark`; round 4: the Lamplighter, the tender, RR's Tripo buildings and the
     big props at 0.35-0.5, `detail_on()`; wood = ToolWood (grain, no plank seams, so it never fights Tripo's boards);
     `DETAIL_SPLITS`: one slot over two materials, and no set for burlap or cloth: sandbags, sacks, banner left without).
     Their cost (round 4, same view): the Lamplighter's deck +0.15 ms, her bag and deck filling the view +0.16, the barn's
     wall +0.12 (Nanite, Epic). A set's means: `DETAIL_SETS`. A model re-import clears its instances
     (FModelImporter's `ClearParameterValuesEditorOnly`, static switches too) and parents them to the master: run
     `build_world_materials.py DetailInstances` after one. Slots by name: a Blender material `X` is the slot `X` and
     `/Game/Art/Materials/MI_X` (looter_export `collect_materials`). `FLAT_NORMAL` (the masters' default NormalMap) is
     `/Engine/EngineMaterials/FlatNormal` since 2026-11-04; it was `DefaultNormal`, a 1024 px brick pattern that six
     instances showed as faint blocks (MI_SextonTrousers, MI_SextonWool, four Bullpup part instances; M_World, M_Gun,
     M_CharacterCloth rebuilt; the other 302 instances' stats unchanged). Never use `DefaultNormal` as "flat".
     Grabs: `Saved/ArtPreviews/BargeDetail/` (before/after, r3before/r3after, compare_*; round 4 in `r4/`).
   - Gun patterns and element stains (2026-10-27, `M_Gun`; numbers `GUN_PATTERN` group GunPattern, `ELEMENT_FINISH` group
     ElementFinish; built by Main's MAT agent): custom primitive data 29-35 (kind, scale cm, seed, tint rgb, amount) -> `PATTERN_CODE` between the paint
     and the wear (the scuffs then wear it through). All in the part's own space (PreSkinnedPosition, cm), triplanar on its own
     normal: Tripo atlases never chop it. Painted vs bare per pixel: not metal (ORM blue < 0.25), tint luma > 0.022 (rubber's
     0x262626 = 0.019 is bare), texture luma > 0.025 (Tripo atlases' black grips); bare takes only grime and worn edges. Glow
     and glass slots are other masters (`M_StylizedSurface`, `M_Glass`), so never patterned. `PART_EDGES_CODE`: outside edges
     and creases from ddx/ddy of the local normal over the local position (1/cm; only smooth-shaded bevels bend, hard edges
     stay flat; creases also from the baked vertex occlusion).
   - T_MacroNoise is evenly spread 0-1 (histogram-equalized, ~8 blotches per UV repeat): thresholds are area shares (0.6 =
     40% of the surface), no contrast stretch needed. Pattern noise = Scale x 0.15 repeats per cm so a blotch is about a Scale.
   - Element stains: every part of an element gun gets the element kind at ~0.2 amount (`ElementLook.Spread` 0.25); the
     master shows the whole finish only past amount 0.35-0.6 (`ElementStainMix`), the stains at amount x 4.
   - Checking without the game: a board of transient StaticMeshActors in the editor level (spawn_actor_from_class(...,
     transient=True), set_custom_primitive_data_float/vector4, the level viewport camera set at them), grabbed with
     grab.ps1; remove them before the editor closes. Destroy them with EditorActorSubsystem.destroy_actor (Actor.destroy_actor
     does nothing in the editor world). A rebuilt master shows its old shader in the viewport until the compile finishes
     (a minute or more): grab again before judging. The board script used 2026-10-27 was a scratch file: rewrite it from this.
   - The stencil's flip: the side facing -Y reads the part's X backwards (seen on the board: unflipped it read mirrored).
     MARK_CODE flips on -Y like the stencil (fixed 2026-10-28: it flipped +Y, so every mark read mirrored on both sides).
     T_GunMakerMarks never streams (sampled only from a custom node: the streamer can't see it used). Third person never
     shows a mark: the over-the-shoulder camera sees the gun end-on, ~7 deg onto its right flank, so a 4-5 cm mark is ~2 px
     (same parts and data as first person: one AWeaponBase re-attached; accepted 2026-10-22). Check it with the front view
     (F5 twice) and `Looter.ThirdPerson.FrontYaw 270`.
   - Looks (checked on the board, AR Tripo body): grime = dark streaks down the sides and in creases; worn edges = small
     tint-coloured chips plus paler rubbed edges (burnished on bare metal); stripes = 1-3 bands round the part; chevrons =
     forward-pointing V bands; camo = light (1.22x) and dark (`PatternCamo.z` 0.72x) blotches of the gun's own paint about a Scale big, never the tint (2026-10-28: red on cream read as cow hide; Amount x1.6); pale enamel (tint luma 0.4-0.62, `PatternPale`, any kind or none: Saint Maud's ivory) 25% darker with outside edges rubbed in chips to dark iron; stencil = letter+two figures in
     column-bar glyphs ("K06"); clean bold = darker belly under a raked split with a bone (or near-black) pinstripe. Stains:
     Ember thin soot streaks + edge scorch, Soul-light fine gold crackle veins + edge gilt (faint glow), Grave-salt rime in
     creases and edges; the element part's whole finish is unchanged.
   - Cost: M_Gun pixel shader 497 -> ~800 instructions (all branches counted; each branch is per part, so a pixel runs one
     kind), samplers unchanged (6), +3 noise taps for a pattern; the element masks' 9 taps now run on every part of an
     element gun.
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
   - Iron sights follow one sight-picture rule and the Drover's `SIGHT_LINE` is 3.0 (round 3): `Gunplay.md` "Iron sights".
8. Throwables: `Art/Models/Throwables/<Name>.py` (the grave-salt grenade, `SaltGrenade.py`) becomes `/Game/Art/Throwables/SM_<Name>`
   through `Tools\models.ps1 -Only <Name>`. Code loads it by path (`AGraveSaltGrenade`, `AGrenadePickup`), and a path is not a
   reference: `/Game/Art/Throwables` is listed under `DirectoriesToAlwaysCook` in `Config/DefaultGame.ini`, so a packaged game has
   it. A new category folder needs the same line.
   - `Art/Models/Throwables/GrenadeJars.py` (round 4, the grenade kinds): three tins in one script, each its own asset
     (`SM_PitchJar`, `SM_ScatterJar`, `SM_CinderJar`; `Tools\models.ps1 -Only GrenadeJars`; the export was tested with `-NoImport`,
     the preview `Saved/ArtPreviews/Throwables/GrenadeJars.png` via `--preview`). Each is the salt grenade's tin underneath, built at its
     first size and scaled by `SCALE` 2 after the UVs (15 cm across, 24 cm to the fuse's top: `FGraveSaltRules` `JarMiddle` 10,
     `JarRadius` 7.5, `JarFuseTop` 24), pivot at the middle of the foot, `SOCKET_Fuse` at the fuse tip, so they sit in the hand, fly and
     lie exactly as the salt jar does. Pitch: dun tin, glossy pitch run down its shoulder in drips, tarred sacking tied round its foot,
     the stopper sealed in pitch. Scatter: verdigris tin, two iron bands riveted round the waist between four small copper capsules
     (the brood) with corks. Cinder: rusted iron tin, a vent band with slots glowing ember-orange, iron bands, soot-black wax.
     Materials are the textured sets the game loads (`PitchTin`, `PitchCoat`, `ScatterTin`, `CinderGlow` flat Glow, ...); the salt
     grenade's slots are shared by name. No collision, no Nanite, no LODs. Until a tin is imported its kind wears the salt tin
     (`AGraveSaltGrenade::FindJarMesh`).

Rules that saved time:
9. Melee tools (round 3, 2026-10-27; the user's picks of `Art/Concepts/MeleeTools/`): `Art/Models/Weapons/Melee/MeleeTools.py`
   builds all five in one script, each its own asset (`Melee_<Tool>` -> `SM_Melee_<Tool>`: `GraveSpade`, `RazorFan`, `FerryGaff`,
   `PistonFist`, `RockPick`; models.ps1 names the category from the folder, so they import to `/Game/Art/Melee`, which is always
   cooked because code loads them by path). Hero wiring and the per-hero table: `Heroes.md` "Melee tools".
   - Space: centimeters in tool space `(u, v, w)` = (+X up the handle to the head, +Z the striking side, +Y the way the right palm
     faces = Blender -X); the origin is the grip point (the middle of the fist). Sockets `SOCKET_Grip` (the grip point, the hand's
     frame), `SOCKET_Tip` (striking end), `SOCKET_Hook` (the gaff's hook point). The Piston Fist's origin is where it clamps on:
     `SOCKET_Forearm` on the forearm's axis at the front clamp ring (X toward the knuckles, Z out of the back of the hand), its
     two rings bored 4.7 cm to fit Gauge's first-person casings (radius 3.9-4.45 cm; `Tools/Blender/fp_arms_gauge.py`), the knuckle
     plate's back face 20 cm ahead of the clamp, `SOCKET_Vent` the steam vent's mouth. No collision, no Nanite, no LODs (small, held
     close); 1024 px/m texel density as the guns.
   - Materials (tinted texture sets on the World master): `MI_MeleeIronBlack`, `MeleeSteelBright`, `MeleeSteelHead`, `MeleeGold`,
     `MeleeCoin` (MetalWorn), `MeleeAsh`, `MeleeAshGrey`, `MeleeHickory` (ToolWood), `MeleeCardFace`/`Back`/`Ink`/`Red` (Polymer),
     `MeleeTealPaint` (PaintWorn), brass shared (`BrassWorn`), and `MeleeEmberCyan`, `MeleeEmberOrange`: opaque emissive surfaces
     (Glow; Nanite can't draw the additive kind anyway).
   - `ToolWood` (`Art/Textures/ToolWood/make_tool_wood.py`, run with `Tools\artrun.ps1 -Script ...`): pale straight-grained
     handle wood made to be tinted: `T_ToolWood_BC`/`_N`/`_ORM`, imported to `/Game/Art/Textures/ToolWood`.
   - Preview without Unreal: `Tools\artrun.ps1 -Script Art\Models\Weapons\Melee\MeleeTools.py -Preview` renders each to
     `Saved/TOOLS3/<Model>_A.png` and `_B.png`. Concept sheets are HTML drawn to PNG by Node + Playwright (`node
     Art/Concepts/MeleeTools/sheets.js`, as `Tools\portraits.ps1`).
10. The menu planet's maps are script-made too: `Art/Textures/MenuTeropa/make_teropa_maps.py` (six equirectangular PNGs, ~8 min
   for the 4096 x 2048 ones; `Tools\artrun.ps1 -Script ...`); `Tools/Unreal/build_menu_materials.py` imports them and builds the
   materials; `Game.md` "Main menu" has the scene.
11. Swim waters and the underwater look (round 4; behaviour: `Game.md` round 4 "Swimming"). Both are scripts run in the open editor
   (`Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/<script>"`), in this order:
   - `build_underwater_assets.py`: `M_UnderwaterPost` (`/Game/Art/Materials/Masters`; a post-process material after the tonemapper, so
     its tints read in any exposure; its graph is rebuilt in place, an asset of another kind at the path stops it) and the sound mix
     `SMX_Underwater` (`/Game/Audio/Mix`; `LOW_PASS` 900 Hz on SC_Effects and SC_Ambience at volume 0.8, SC_Music 2600 Hz, the interface
     clear; fades 0.12 s in, 0.3 s out). Needs `build_vfx_materials.py` (the bubbles' `M_FX_Shape`) and `build_sound_bank.py` (the
     `SC_` classes the mix names) run first; needs no C++. The numbers to tune are its `DEFAULTS`. Prints `UNDERWATER` lines.
   - `build_area_swimwater.py <RansomsRest|TutorialIsland>`: one `ASwimWater` per layout `pond` feature, replacing only the waters a
     build placed before (folder `Water`, tag `SwimWater`), then saves the level. Needs the C++ built and the terrain in the level (the
     bed is measured by traces: `DEEP_RAYS` 24 rays from the middle, `DEEP_STEP` 25 cm). Check its `SWIMWATER` log: Ransom's Rest's
     larkspurTarn should get a deep zone (1.7 m deep); the island's pond (1.2 m) and the Wallow's pools none. `SWIM_DEPTH` 160 x 0.85 is
     `Looter.Swim.StartDepth` at the player's size, kept equal by hand. A full `build_area.py` build clears the waters, so it calls the
     script's `place` at the end of its gameplay part and in its `water` mode.
12. The adjusted Tripo barge (2026-11-04, round 3; why, the user's calls and the numbers: `Game.md` "The barge: the reworked Tripo hull and the kit on it"). A Tripo model the player walks on is reshaped by script on the fitted mesh, never replaced:
   round 1's scripted hull and round 2's boxed house were rejected (`Workflow.md`).
   - Hooks: `SPECS['DredgeBarge']` in `Tools/Blender/looter_tripo_wardens.py` has `rework=barge.rework`, `slots=barge.slots`, `boxes=[]` (the rework writes its own `UCX_` hulls). `build()` runs `rework` after the turn, scale and pivot and before the
     maps, sockets and LODs, and `slots` after the maps, so every export reproduces it. A reworked spec can't carry glow faces (their indices move). The build logs `reworked tris` (75,775 in, 135,393 out).
   - Modules, all in `Tools/Blender/`, in the Unreal frame (cm; `B()`/`U()` turn a point into Blender metres and back, front -Y): `looter_tripo_barge.py` the rework (its docstring lists the steps in order: 0 walks cleared, 1 the squeeze, 2 the fore
     jumbo, 3 the midship jumbo, 4 the berth gap, 5 the forecastle's stairs, 6 the hatch, 7 slots, 8 collision and sockets; its constants are the contract's numbers, which `BargeRules.h` shares: change both together; `final_x()` maps a Tripo x to the
     final one), `looter_tripo_barge_jumbo.py` (the fore run's slabs, warp, end morph, zip), `_clear.py` (what crosses the walks, the stub posts, a copy's rail post), `_floors.py` (the walk floors and the bulwark's top per 25 cm station), `_collision.py` (the
     `UCX_` hulls and floor boxes fitted to the final pieces), `_check.py` (below); the toolkit `looter_tripo_rework.py` (`bisect`, `spanning`, `copy`, `patch`, `trim`, `cap`, `separate`, `tidy`, `ucx`, `hull`, `close_slot`) and its paint
     `looter_tripo_paint.py` (`Nearest`, `paint_like`, `paint_group`, `repaint`). `looter_tripo_barge_house.py` (round 2) is deleted.
   - The rule: new faces COPY real Tripo faces with their UVs (`copy`/`patch`, cut with `bisect` where a piece moves) and a cap or fill takes the paint of the Tripo triangle nearest it or of the face beside it; never a flat atlas patch or a stretched
     strip. A face left without UV area gets a small atlas patch (Unreal needs a tangent frame on every corner); `tidy` keeps the tangent check clean. The jumbo lays unmirrored copies of three different slabs in a searched run: mirrored copies made
     butterfly seams, turned ones read as tiles.
   - Slots: a Blender material `X` is the slot `X` and `/Game/Art/Materials/MI_X` (`TripoDredgeBarge_Hull/_Deck/_Super`, one atlas). `slots()` drops the unused first slot; `materials.pop()` moves the face indices itself (round 3's first export moved
     them by hand: every face one slot off).
   - The check: `Tools\artrun.ps1 -Script Tools\Blender\looter_tripo_barge_check.py` builds the barge, measures it with rays (every number a hit or a vertex) and writes `Docs/Work/2026-11-04-Art/BARGE_LAYOUT.md`, the kit's contract: deck zones as
     boxes with z, the walks' clear width and headroom, the rail line and the gap, the hatch hole, the hall's cavity, footprints to keep clear of, the hulls, the sockets. It raises when a slot is wrong (a ray per region must hit its slot). Re-run it
     after any change to the model and plug the numbers into `BargeRules.h` / `BargeHallRules.h`.
   - Sockets (`lm.socket`; an Unreal yaw turns the other way about Blender's Z): `Gangway` and `Hatch` from the rework; `HoseDrum`, `Stack_1`, `Stack_2` from the spec (Landfall's build reads them).
   - Preview (Tripo's angle, the spyglass side, bow, stern, top, the well deck at eye height both ways, the berth, the hatch, two close-ups, into `Saved/ArtPreviews/DredgeBargeRework3/`): `$env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\artrun.ps1
     -Script Art\Models\Tripo\Wardens.py -Preview`; Main judges it against `Art/Tripo/DredgeBarge/preview.png`. Export test:
     `$env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\artrun.ps1 -Export Wardens -Family DredgeBarge`, then `Tools\artrun.ps1 -Script Tools\Blender\tangentcheck.py -ScriptArgs Intermediate\ArtExport_DredgeBarge,--where`. Without `LOOTER_TRIPO_ONLY` a run
     builds all the Wardens' models.
   - Export folder rule: `Tools\models.ps1` clears the `.fbx` and `.json` files of its export folder first, so each Blender lane exports to a folder of its own (`artrun.ps1 -Export ... -Family <Lane>` -> `Intermediate\ArtExport_<Lane>`): the Wardens
     family run `ArtExport_Tripo` (the contract's), a barge-only run `ArtExport_DredgeBarge`, BargeKit `ArtExport_Props`, the collision lane `ArtExport_Collision` and `ArtExport_CollisionWardens`. Never share one.
   - Import (editor open): `$env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\models.ps1 -Only Wardens` overwrites `/Game/Art/Tripo/SM_DredgeBarge`, `MI_TripoDredgeBarge` and its maps. **After any re-import run** `Tools\console.ps1 "py
     C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_world_materials.py DetailInstances"`: the importer clears the instances' parameters, the `DetailOn` static switch too, and the barge would fall back to Tripo's soft atlas.
     Then rebuild the level if the deck or the sockets moved (`Levels.md` "The barge in the level").
13. The barge kit's models (2026-11-04, the PROPS lane): `Art/Models/Landfall/BargeKit.py` makes 16 scripted models in our sets (the furnace and its burst, a cage shard, a grate wedge, the draft valve and its wheel, a pipe section, the deck vent, the
   furnace vent, a fire tongue, the falling stack and its foot, the hose drum, the drum gantry, the jam lever, the radio) that the code drew as engine shapes (the audit's plan row 3). Sizes, pivots, sockets, slots, collision and how the code scales
   each: `Docs/Work/2026-11-04-Art/BARGE_PROPS.md`; the code's paths: `World/BargeLooks.h`.
   - The script's folder names the Unreal folder (`models.ps1`): they land at `/Game/Art/Landfall/SM_Barge*`; `Config/DefaultGame.ini` always cooks `/Game/Art/Landfall` (the code finds them by path, some outside a constructor).
   - Materials by slot name (`MI_<slot>`): shared `IronBlack`, `BrassWorn`, `PaintOxide`, `PaintCream`, `PaintBlack`, `CinderGlow`; new `MI_BargeGunmetal` (MetalRust tinted `#6A6560`; it was `#8A98A8` and read pale beside Tripo's brown iron) and
     `MI_HoseRubber` (Polymer `#36322E`). 1024 px/m, BargeGunmetal 512 (MetalRust's rust patches are painted for 320 px/m). A re-import overwrites the gunmetal tint.
   - `CinderGlow` is additive, so every model is Nanite off: cold means the code overrides the `CinderGlow` slot with `MI_IronBlack`, found with `GetMaterialIndex(TEXT("CinderGlow"))` (works on ISMs); never `SetMaterial(0, ...)` (slot 0 is not the
     glow). LODs 50/25% on most; vertex occlusion and soot baked in alpha; seeded RNG, repeatable exports.
   - Preview: `Tools\artrun.ps1 -Script Art\Models\Landfall\BargeKit.py -Preview` (`'--only=Name,Name'` builds some) into `Saved/ArtPreviews/BargeKit/` (each model, the lineup, the pieces laid as the code lays them). Export test: `Tools\models.ps1
     -Only BargeKit -Out Intermediate\ArtExport_Props -NoImport`. Import: `Tools\models.ps1 -Only BargeKit` (editor open).
14. Collision that fits the Tripo looks (2026-11-04, the COLLISION lane; the art audit's finding 6, plan rows 6-7; the user's complaint: "the collisions are messed up"). Only `UCX_` hulls change, never a visible mesh; before and after, with the
   metric: `Docs/Work/2026-11-04-Art/COLLISION.md`. `Tools/Blender/looter_tripo_collision.py`:
   - Props and ships: `SPECS[...]['collision']=tc.<hook>` with `boxes=[]` (never `boxes=[]` without the hook: no hulls means mesh collision), called by `build()` after the `post_box` block. `fit_prop` splits the mesh's vertices into convex pieces
     (the cut that leaves the two smallest hulls, worst piece first, at most `max_hulls` 8, each cut to 48 corners along fixed directions in its own frame). Hooks: `barricade_table`, `barricade_boards`, `barricade_sacks`, `hose_intake`,
     `anchor_winch`, `lantern_post` (its `post_box` dropped), `company_tender` (the hull in slices along its crown).
   - Buildings: `looter_tripo_buildings.refit_hulls` calls `refit_building` in `swap`, after the trim boards and lit windows are placed: each scripted hull's box is a region of the shell, its walls planes fitted to the faces rays meet from outside
     (the outermost wall-wide cluster per row, so trim, sills and posts don't push a wall out: the old refit stood walls 25-50 cm proud), banded where a wall steps or bends; deck and porch roofs fitted from readings above and below; `BUILDINGS` holds
     the farmhouse's stovepipe and back gable and the windmill as parts. Then `cover_leftovers` hulls whatever stands 10 cm or more outside (rakes, fascias, stones). `LOOTER_TRIPO_HULLS=kdop` keeps the old k-DOP refit to compare.
   - The capsule sweep rule: after any refit, walk the player's capsule (BP_LooterCharacter radius 34, half-height 96, times `LooterPlayerSize::Scale` 0.85: radius 28.9, height 163.2; also the 34/176 case; step 45 = `MaxStepHeight`; feet on the
     highest hull top under it, a rise over 45 is a wall) up every door's steps and porch, between the posts, round the building and along the ships' decks: it must pass wherever a player can reach and stop at the walls. The Tripo houses are not
     enterable, so a door sweep ends at the door face (the farmhouse stops 25 cm short: its belt trim stands 23 cm proud). The sweep script (`cm.py`) lived in the lane's scratch folder and was not kept.
   - Ships in C++: `World/LamplighterShipParts.cpp` (18 tilted deck pieces fitted to her measured crowned deck, 5 deck blocks, side rails on the measured bulwark line in tagged stretches so `ShowGangway` opens the gangway's span, spots on the
     measured deck) and `World/CompanyTender.cpp` (`HingeTop` 300 replaces `DeckTop` 298; per-spot rifleman heights).
   - Result (hulls, share off the look by over 10 cm, old -> new): farmhouse 13 -> 67 hulls, 71 -> 25%; barn 2 -> 45, 61 -> 9; cottage 9 -> 23, 31 -> 7; windmill 4 -> 32, 87 -> 44; the props 57-90 -> 3-25; the tender 83 -> 21; the Lamplighter's deck
     off the collision by over 3 cm 78 -> 10%. Hull counts rose: a perf row if a fight happens round the farmhouse or barn.
   - Left: her own `UCX_` boxes stay swollen but are unused (her Hull has NoCollision); no collision on her forecastle stair, the ring forward of the console, the overhead spars; Junie's launchers (`PilotSortieFight.cpp`) stand where the deck is 17
     cm under `DeckTop`; the tender's `LauncherSpots` float 30-130 cm over its hull; `LamplighterShipParts.cpp` is 556 lines and `looter_tripo_collision.py` 888 (over ~500).

15. Tripo atlases repacked (2026-11-04, the REPACK lane; the audit's plan row 5; numbers and what each model gained:
   `Docs/Work/2026-11-04-Art/REPACK.md`). `Tools/Blender/looter_tripo_repack.py` re-places a fitted model's UV islands (Tripo's
   own, seams kept) to fill more of the square and re-cuts its BC, ORM and N from the GLB's 4096 maps onto them; the visible
   mesh is untouched. Rules learnt:
   - Tripo's atlases are already packed by a raster packer as good as ours (their 36-66% use is island shapes and gaps):
     space is won only where the game mesh drops faces from the atlas (cuts, faces moved to other slots). So the step keeps
     Tripo's layout unless the new one holds 1.05x its UV area (`MIN_GAIN`), and never leaves an island less dense than it was
     (the floor: evened out freely, 28% of the farmhouse came out softer). Blender's own packer lost to Tripo's (0.35-0.47 vs
     0.58 on Abel's hat).
   - Re-cut by an exact UV-to-UV box filter (k x k point taps over each texel's footprint): bilinear taps came out softer than
     Tripo's old 2:1 shrink. Normals re-encoded in the new MikkTSpace frames (a scratch mesh of the loop triangles: n-gons OK).
   - A soft source can't be sharpened by a re-cut: check the survey's "lost at map size" first
     (`Toolsrtrun.ps1 -Script Tools\Blender	ripo_repack_check.py -ScriptArgs survey`). Lamplighter 0.8%, Unpaid 0.2%, tender 1.0%
     at 2K: rebake those in our sets instead.
   - Hooks: `repack=True` (Wardens SPECS), `repack_after=True` (m23/m47 BASE: after their fit), `swap(repack=True)` (buildings:
     after the retexture), the build's `sets` (own_set: faces onto a texture set of their own). `LOOTER_TRIPO_REPACK=0` gives the
     old layout (before/after). Repacked today: SM_Farmhouse_Ransom, SM_Cottage_Ransom, SM_Barn_Ransom, SM_CompanyCheckpointSign,
     SM_CompanySandbags. The pack is cached in `Intermediate/TripoRepack`; the re-cut adds ~1-2 min a model to an export.
   - Before/after: `tripo_repack_check.py -ScriptArgs model,<kind>:<Name>` (kinds wardens, m23, m47, building) renders the same
     camera at 1, 3, 10 m with `LOOTER_TRIPO_REPACK=0` and on into `Saved/ArtPreviews/Repack/`.
   - `lt.write_png` and `looter_tripo._save_colour` retry Windows' transient OSError 22/13 (`lt.retry_write`): a PNG rewritten a
     second after it was written (write_maps, write_set + grade) is often still mapped by another process, likely the virus
     scanner; 5 of 13 Tripo builds failed so before the retry.

- Exports must be repeatable. Never iterate BMesh sets (their order changes between runs). Export twice and compare; only FBX IDs
  and time stamps may differ.
- Faces without UV area give tangent warnings; `Tools/Blender/tangentcheck.py` predicts them.
- Terrain meshes keep 100% Nanite fallback. Medium draws the fallback and collision is cooked from it.
- A Nanite mesh with UCX hulls may cut its fallback (Medium has no LOD chain on Nanite: the fallback at every distance), but
  complex traces (bullets, spiders climbing) still use it. The cliff faces: 12% (was 25%, 2026-11-04; `Cliffs.py` FALLBACK,
  checked with `Tools/Blender/cliff_fallback_sheet.py`: Decimate stands in for Nanite's cut, its UVs projected again or it
  smears the texture where Nanite wouldn't).
- A variant of a model (Farmhouse_Ransom) goes in the same script, with the original exported byte-identical.
- A prop whose pieces the game hides is several models on one pivot in the same script (the porch plate: plate, jars,
  biscuits).
- The kit's `_triangulate` collapses triangles under 1 mm^2: build tiny details (ink, cards) as single faces of at least
  1 mm^2 a triangle. Map round, flat parts with `Tile(set, along=None)` (the default turns each face's grain to its long side:
  a star of seams on a lathe's floor).
- Tangent warnings logged by a reimport's first build come from Unreal's default fallback, not the model
  (SM_RR_RangerCabin, 2026-10-13); `tangentcheck --log` labels them.

## Sounds (script-made, 2026-10-08)

- The user's rule: sounds are synthesized from scratch, like the scripted models: no recordings, libraries, AI generators or new
  programs. Blender 4.4's bundled Python has numpy (no system Python on this PC).
- Exception (the user, 2026-10-21): royalty-free recordings for **guns and bullet impacts only**, always processed through the
  recipes. Source: Sonniss GDC bundles 2016/2017 (+2026, the user's 2026-10-29 yes: no gunshots in it, only cloth for
  aim-in/out, lock/latch clicks for the sniper bolt, clacks and a spring for SMG/LMG mech; 2018 and 2021-23 have guns but the
  user said no more downloads) (games OK, edits OK, no attribution; no resale/redistribution as sounds, no AI
  training). Raw packs in `Saved/SoundPacks/Sonniss` (outside git); the excerpts used in `Art/Sounds/packs/Sonniss/` (LFS, cut
  by `packs/make_pack.py`; sources and licence in `packs/README.md`). Never publish `packs/`.
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
- Round 3 (2026-10-27) recipes: `Art/Sounds/recipes/opening.py` (`UI.Startup.Studio`, `.Engine`, `UI.Menu.Reveal`, `.Select`: 4 cues,
  5 takes) and `intro.py` (`Music.Intro.Planet` 28 s one-shot, `.Rest` 50.5 s and a ringing tail, `.Card`, `.Whiteout`), both added to
  `recipes/__init__.py` `GROUPS`. Render with `Tools\sounds.ps1 UI.Startup.* UI.Menu.* Music.Intro.*`, then `build_sound_bank.py`. The
  intro music's lengths are tied to the script's constants (`PlanetSeconds`, `RestSeconds` in `NewGameIntro.h`): change both together.
- Round 4 (2026-10-27) recipes, all appended to `recipes/__init__.py` `GROUPS`:
  - `melee_tools.py`: `Player.Melee.<Spade|Fan|Gaff|Piston|Pick>.Swing` (2D, the player's own, crossing the ears the way the tool swings)
    and `.Hit` (3D Near), 4 takes each, over the shared `Player.Melee.*` blow, so they carry what the tool is made of, not the blow's
    weight. Named by key in code (`MeleeFX::SwingCue/HitCue`), not in `LooterSoundCues.h`.
  - `bossintro.py`: `Music.BossIntro.Gravemother` (~7.8 s one-shot; drum hits at the script's five bursts 3.0-6.2 s, a riser into 6.6),
    `.Foreman` (~6.8 s: drips, three slow knocks at 1.2/2.4/3.6 s, a riser at 5.6), `.Abel` (~6.8 s: the chapel bell, strings in D minor,
    the Rest's reed theme slowed, a riser at 5.6), `.Reveal` (2 takes, ~3 s), and the 3D `Boss.Gravemother.Burrow` / `.Emerge`. The beds'
    hits are tied to `BossIntro.cpp`'s times: change both together. Names in `LooterSoundCues.h`'s boss block and `BossIntroCue`.
  - `throwables_kinds.py`: `Throwable.PitchJar.Stick` / `.Burst`, `.ScatterJar.Burst` / `.Pop`, `.CinderJar.Burst` / `.Bed` (a loop),
    `Throwable.Jar.Swap` (2D); the bursts are LAYERS over the salt jar's (`throwables.py`), not whole explosions. Names: `GrenadeKindCue`.
  - `swim.py` (9 cues): `Player.Swim.In`, `.Out`, `.Stroke`, `.StrokeUnder` (2D), `.Gasp` (2D), `Player.Underwater.Loop`, `.Enter`, `.Exit`,
    `.Bubbles` (2D, made dark and close already: the mix low-passes the world). Names: `SwimCue` in `Player/UnderwaterView.h`.
  - Render with `Tools\sounds.ps1 <patterns>` then `build_sound_bank.py`. Those cue names live in headers other than `LooterSoundCues.h`.
- Landfall and the Dredgers (2026-10-31; recipes `landfall.py`, `landfall_story.py`, `dredger.py`, all in `GROUPS`; 30 cues, flat constants at the
  end of `LooterSoundCue` in `LooterSoundCues.h`, `VaneRadioIn` ... `TurnInStamp`; group notes and two lessons in `Art/Sounds/README.md`):
  synthesized except the two guns, which reuse recordings already in `packs/` (the Dredger rifle: L85 and Tavor slowed 0.82-0.93x plus a
  synthesized crack, barrel ring and spark flare; Delia's shotgun: the Ranchhand's blast slowed 0.86-0.88x). Rendered, not yet imported or heard:
  `Tools\sounds.ps1 Ship.*,Story.Vane.*,Story.Spyglass.*,Story.Lantern.*,Story.Delia.*,Ambience.Smog,UI.TurnIn.*,Creature.Dredger.*` (the
  "all shifted -0.8 dB" line is the standing value), then `build_sound_bank.py` and `... check`. Loops (`Vane.RadioHiss`, `Ship.Lamplighter.Engine|Creak`,
  `Ship.Grapple.Strain`, `Ship.Anchor.Crank`, `Ambience.Smog`, `Creature.Dredger.Hose.Drain`) go through `LooterSound::Start`/`Stop`; `Grapple.Strain`,
  `Hose.Drain` and the ship's loops are 3D mono 'Creature' (35 m); the Vane hiss and the smog bed are 2D. `Ship.Lamplighter.HardLanding` and
  `Ship.Tender.Fall` carry like a gunshot ('Gun', ~60 m): play them at the ship/tender, shake timed to the hit at 0 s and the skip at ~0.4 s.
  `Creature.Dredger.Bark` is one cue with 6 takes (random pick; rate-limit it). Most likely to need the user's ear: the masked voices (could sound
  too narrow or steady), the rifle's boom, the crash's debris balance, the lantern's tone, the tender's fall.

## Niagara by script (round 5, 2026-10-27; the user's pick from a video: "try Niagara effects")

First Niagara effect: the cinder jar's fire. Script `Tools/Unreal/build_niagara_fx.py` makes `NS_CinderBed` (looping, CPU, world space: heat
glows, flames, velocity-aligned tongues, curl-noise embers, smoke; ~85 live sprites), `NS_CinderBurst` (one shot: heat flash, 70 flung
embers, fire puffs, a soot plume) and their materials (`M_NFX_Fire`, `MI_NFX_Tongue`, `MI_NFX_Smoke`, `M_NFX_Ember`, `M_NFX_Heat`,
`MI_NFX_HeatFlash`) in `/Game/Art/Effects/Niagara` (already cooked: DefaultGame.ini cooks `/Game/Art/Effects`). Spawned by
`Weapons/NiagaraFX` (`SpawnAt`), used by `Combat/GraveSaltCinderBed` and `GraveSaltBurstEffects.cpp`. `Looter.Fx.Niagara 1|0`
(0 = the old code-drawn look everywhere, so both can be compared), `Looter.Fx.Niagara.Scale`. A system that isn't built = the code look.
Test `Looter.Fx.Niagara`.
- **How Python authors Niagara:** NiagaraEditor exposes no authoring functions to Python (only baker/upgrade helpers; the empty system
  from `NiagaraSystemFactoryNew` has no settable emitter list). The one scripting API that adds emitters, modules, inputs and renderers
  is the **Cascade To Niagara Converter** plugin's `unreal.FXConverterUtilitiesLibrary` (`create_system_conversion_context(system)` ->
  `add_empty_emitter`, `find_or_add_module_script`, `set_parameter`, `set_parameter_directly`, `add_renderer`, `set_sim_target`,
  `finalize`). Engine plugin, Editor module only (ships nothing), Beta, off by default: **enabled 2026-10-27 with the user's OK**
  (`AI_Looter_Shooter.uproject`: `CascadeToNiagaraConverter`, `TargetAllowList` Editor). The script uses only its C++ library; every
  module is the engine's `/Niagara/...`.
- **What it can't do:** user parameters (so no per-spawn strength or radius parameters: the code scales the component and
  deactivates it instead; a bed's last second, `EndPlay`), and no preview check from a script (eyes in PIE; if a system shows nothing,
  open it once in the editor: a compile error lists there).
- **Running it** (editor open, not in PIE: a material script can't save during PIE): `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_niagara_fx.py check"`
  (plugin and engine modules present; builds nothing), then without `check` (or `materials`, `bed`, `burst`); look for `NIAGARAFX done`.
  A wrong module input name shows as a conversion-context log line, not an exception. It refuses non-matching assets at its paths and
  rebuilds a system by delete + create. Every look number is in its `BED`/`BURST` tables.
- **Unverified input names** (taken from the converter's scripts and the module assets' strings): `curl` frequency 50 is a guess (drop
  `curl` in `BED.embers` if the embers swirl wrong); velocity-aligned sprites assume SpriteSize.y is the length along velocity (swap
  the tuples if streaks lie flat). Heat glows are flat quads like the code's ring: on bumpy ground parts hide under it or float.
- **Cost estimate** (Main measures with `Tools\perf.ps1`, Niagara 1 vs 0, ArtPause on): code look per bed ~0.02-0.04 ms game thread,
  0.05-0.15 ms GPU; Niagara bed ~0.04-0.08 ms game/worker thread, ~0.02 ms render thread, 0.15-0.35 ms GPU near it (smoke to ~3 m and
  five overlapping 5.6 m additive glows are the overdraw); burst +0.1-0.2 ms GPU for ~2 s. Knobs: smoke `grow`/`rate`, heat `rate`.
  The first spawn may hitch once (new shaders/PSOs). Nothing measured yet. Cinder test: `Looter.Grenade.Kinds.FromLevel 1`, `.Share 1`,
  `.PitchWeight 0`, `.ScatterWeight 0` make found tins cinder (no give command).

## Tools we looked at (round 5 research, 2026-10-27)

Sources and the unconfirmed parts: `Docs/Archive/Work/2026-10-27-Playtest/RESEARCH_ANIMATION.md`. None is installed or used.
- **Tripo** (AI model maker): a paid plan is needed for models that ship (the free plan's rights are contested). Concepts only so far.
- **Suno** (AI music): free plan = non-commercial only, no downloads, Suno keeps ownership: not usable in a Steam game. Pro ($8/mo)
  and Premier give commercial rights to songs made and downloaded while subscribed (perpetual; none retroactive for free-plan songs).
  Voice Models only of your own voice; no NPC speech product. "Suno Sounds" effects: terms unconfirmed. Tension with rule 1 (original
  content; sounds are script-made). Not used; the user decides.
- **NVIDIA Kimodo** (text/pose to motion, 30-joint SOMA humanoid, 30 fps, 10 s max): code Apache-2.0, SOMA weights under the NVIDIA Open
  Model License (commercial use ok, NVIDIA claims no rights in outputs; the SMPLX checkpoint is research-only). Needs an NVIDIA CUDA GPU
  (~17 GB VRAM, under 3 GB with the text encoder on CPU) and a gated Llama 3 8B encoder: **not runnable on the RX 580**. Paths if wanted:
  a rented NVIDIA GPU or the unofficial Replicate wrapper, BVH out, then Unreal's IK Retargeter. Humanoids only (not spiders or slimes).
- **MediaPipe** Pose Landmarker (Google): Apache 2.0 code, `pip install mediapipe`, runs on Windows on the CPU, 33 body points (3D in
  metres), no finger or spine detail; we'd write the landmarks-to-bone-rotations solver ourselves. The model bundle's own licence is
  unconfirmed (we would ship only the animation, so low risk).

- **Tripo parts (2026-10-27, the user's call to try it):** the user models a part in Tripo from Art/Concepts/Weapons/<Gun>/<Slot>_<Key>/upload/*.png (transparent, part-only, 4 views; a grey backdrop got modeled as a piece) and saves a TEXTURED GLB there. Tools/Blender/looter_tripo.py loads, cleans, fits (scale/orient to the part's frame, sockets copied from the scripted part), splits glow/paint slots; the gun's .py switches per part (Bullpup.py TRIPO_PARTS; LOOTER_BULLPUP_TRIPO=0 = scripted). Maps max 2048 into Art/Textures/Tripo<Gun><Slot><Key>/ through the Gun master; LODs 50%/20%; a painted shell is its own slot named <paint>_<part> (UWeaponModelComponent paints by prefix). Import: Looter.ImportModels <ABSOLUTE path>. Licence: shipped parts must be made on a paid Tripo plan (free-plan rights contested). AR: all four sets in the game (2026-10-27): set 1 Saint Maud, 2 Roundhouse, 3 Windfall, 4 Meridian. Each set lives in Art/Models/Weapons/Tripo/parts_set<N>.py (PARTS = {key: dict spec or fn(bp, m)}), merged by Bullpup.py over the scripted parts; LOOTER_BULLPUP_TRIPO=0 scripted only, =1,2 only those sets; models.ps1 skips parts_*.py. Checks every set needed: Tripo paints sight glass opaque (cut it, add a dot on SOCKET_Aim), parts off the bore by a few mm (re-centre), no rarity accent (add strips), glow too faint to read per element (make glow bands/cells big and bright; compare the three elements side by side). Chests (2026-10-28): `Art/Models/Loot/ChestTiers.py` fits the five closed chest GLBs (`Art/Concepts/Chests`) whole, not as gun parts: Tripo maps on the Gun master (its worn edges and grime), colour map unsharp-masked; planarize only vertices inside a panel (snapping a panel's edge vertices folded latches flat); decimate by edge collapse only (planar dissolve smeared the atlas into spikes); bmesh holes_fill misses ragged holes (fan-fill the rest); tier 3's GLB was a hollow shell (inside walls dropped by ray visibility); measure the hollow on the dense mesh (decimation leaves inside-out faces); join after `view_layer.update()` (matrix_world is stale after moving an origin). Glows are crisp overlays over Tripo's paint. Round 2 (Main's review): linings are a script-made quilted set (ChestLining, tinted) with cushions; a cut piece's hole is flattened onto its convex outline (a clean seat; moves over 1.2 cm skipped: a far move bent tier 2's case); seam caps scanfilled (a centroid fan laid wedges across concave notches); sparing faces in a collapse (vertex group) only on tier 1 (it bent tier 2); tier 4's trim light is a ribbon ray-cast onto the front (strips copied from gold-painted faces were blotches).
- **Tripo melee tools (2026-10-28):** the five hero tools (SM_Melee_<Tool>) are the user's Tripo remodels: GLBs in Art/Models/Weapons/Melee/Tripo/<Tool>.glb (copied from Art/Concepts/MeleeTools/<Tool>/), swapped in by Art/Models/Weapons/Melee/parts_tripo.py (MeleeTools.py builds the scripted tool first: its box is the fit, its sockets are copied; LOOTER_MELEE_TRIPO=0 scripted, =GraveSpade,RockPick only those). The user's quality bar (2026-10-28): crisp textures with worn metal and edge wear like the gun parts, clean shapes (no lumps, straight edges, no holes, sharp silhouettes), our style, nothing reads as AI-made. So Tripo's maps are not used: Tools/Blender/looter_rebake.py bakes the scripted model's look (our sets, tints, crisp borders) onto Tripo's atlas by a Cycles selected-to-active cast, adds edge wear and crease grime from pointiness (blurred 3 px; tighter thresholds, or subdivided handles streak) and a normal map from the new colour's relief only (Tripo's normal map no longer fits a rounded mesh: it drew streaks and lumps). Where Tripo's shape strays from the scripted one the cast picks up the wrong part, so painted layers (parts_tripo *_PAINT rules on each texel's baked place and normal, a material's look baked from a copy of the mesh) draw those borders on Tripo's own shape: the spade's longer socket and ferrule, the gaff's pole, the fist's lower, fatter cylinder (bands found from its profile), the pick's handle (the scripted handle's per-facet box UVs streak; wood is mapped with its grain wrapped round the u axis). Painting per face first gave jagged triangle borders: paint per texel. Shapes: Tripo builds turned parts as octagons (faceted up close). One Catmull-Clark level with edges over 75 deg creased, collapse-decimated, shading by 70 deg; rounding before the decimation is undone by it. The pick's handle is then made round in place (turn_true: subdivide, push onto the ellipse of each 0.3 cm slice, slices median-smoothed: per-slice radii rippled into streaks); the spade's shaft and the gaff's pole are rebuilt as clean 40-sided lathes on Tripo's measured profile (relathe: bands found where 0.08 cm proud, squared off; ends tucked 0.96 slim into the next part), since rounding Tripo's thin long triangles still left dents; the fist's cylinder is the scripted one scaled 1.27 onto Tripo's lower axis. 11-13.6k triangles, LODs 50/20. Fixes: the spade's blade (a fin off both faces, a gouged crack, white streaks on the edge from our own upload texture) is the scripted blade, crack, tread and frog, socket cut at u 46.9 and capped under a scripted collar; the fist's yoke came out a hollow wire frame and its brass rear ring a washer with a 2.8 cm bore (both cut, the scripted ones in), the front ring's bore pushed out to 4.7 cm; the fan's card faces pressed flat (w smoothed, 40 passes; snapping its edges onto the scripted outlines tore the fused cards: don't); the gaff's pole re-centred on the axis and the scripted coin wrapped on it. Check renders: Saved/MeleeFit/<Tool>_<tag>_side/_q/_fp.png.
