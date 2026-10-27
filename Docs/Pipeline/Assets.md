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

## Sounds (script-made, 2026-10-08)

- The user's rule: sounds are synthesized from scratch, like the scripted models: no recordings, libraries, AI generators or new
  programs. Blender 4.4's bundled Python has numpy (no system Python on this PC).
- Exception (the user, 2026-10-21): royalty-free recordings for **guns and bullet impacts only**, always processed through the
  recipes. Source: Sonniss GDC bundles 2016/2017 (games OK, edits OK, no attribution; no resale/redistribution as sounds, no AI
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

Sources and the unconfirmed parts: `Docs/Work/2026-10-27-Playtest/RESEARCH_ANIMATION.md`. None is installed or used.
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
