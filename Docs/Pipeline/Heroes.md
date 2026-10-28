# Pipeline: Hero portraits and the hero pick (select screen, third-person hold)

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

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
  `stage`, material ids 14 glass / 15 wood); a hero's cyan rim light that also lit the
  hair went light-linked past it (Odessa's `rim_sparing`); eyes with no glow use the skin id (the aux pass blooms every
  id-2 pixel). **The merge (2026-10-20, no visible change):** the helpers the hero files carried each moved to
  `Art/Portraits/bustkit.py` (listed in `CODEMAP.md`: garment parts and props, shader shorthands, bmesh objects, wires,
  chains, hair tufts, the stoop, the raised arm via `ARM` + `use_arm_pose()`, Gauge's primitives); `bust.render_aux` reads a
  material's `looter_glow` / `looter_aux_skip` (Gauge's own pass is gone), `bust.area_light` places the stage's lights (and
  Odessa's rim); `bust.ID` has glass 14 and wood 15. Crane's instanced curve links (`link_chain`) and Pike's one-mesh links
  (`mesh_chain`) are different builds, so both stay; the heroes' tears differ in outline only and share `edit_faces`.
  Proof: all 15 scenes (5 heroes x A/B/C) saved with `--blend` at 96 px / 1 sample before and after and compared object by
  object (names, parents, transforms, every RNA property, vertex positions to 1e-5, polygons as sorted records because a
  bmesh primitive orders its faces differently each run, UVs, weights, materials' nodes and links, curve points, lights,
  world), the aux PNGs at 384 px pixel for pixel, and the Ellis, Crane, Gauge and Odessa model exports with
  `Saved/BakeCheck/fix/compare_fbx.py`: all identical (the rebake the new digest forced wrote the same texture bytes; only
  the `bake.digest` stamps changed). Any model script that bakes from the portraits lists `bustkit.py` in its `BAKE_SOURCES`.

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
  LongShot`) have their code since 2026-10-20 (Ember powers, "The twelve" below); each entry names its class and icon,
  with its numbers and why in a comment. The portrait, mesh and retargeter fields are set by `SetLookPaths` (Python
  sets a soft pointer only from a loaded object) only once each asset exists: a soft pointer to a missing package failed
  the editor's asset validation on save (AssetReferenceRestrictions, 2026-10-19). Meanwhile the game finds each by the
  naming rule (`GetFigureMeshPath` and the rest), so a hero lights up as soon as the model lands; rerun the script after
  each to fill the fields.
- **Playable** (`IsPlayable`): the mesh and retargeter load and every power has a class. Every power has its class now, so a
  hero is playable once SK_<Id> and RTG_MannyTo<Id> are in; until then "In the works". `Looter.Hero.Unlock 1` (console
  variable; never in shipping) lets one be picked anyway: Ellis's body stands in, their powers work.
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
  the inventory's pages): header top left (bar, SESSION N · NEW GAME, CHOOSE YOUR HERO, a line wrapped at 360), the live
  stand in the middle (below) with a 1080 px glow in the hero's colour behind it (alpha 0.5, breathing) and a soft shadow
  under the plinth, the roster under it (five 176 x 140 cards: eye band, number key, colour strip, name, role; greyed and
  stamped IN THE WORKS when not playable), the panel on the right (496 x 820: the painted face 76 px beside role, name and
  colour rule, story, EMBER POWERS with three rows: medallion with the Inked icon or the power's glow, name, nature chip,
  words, cooldown; status line; Back and "Play as <name>", disabled "In the works"). A hero whose body isn't in the game
  shows their whole painted portrait (500 px, centred at 560, 424) where the stand is. Greyed = the image tinted 0.42 grey
  under a 42% IconShade wash (Slate has no desaturation without a material).
- **The live stand** (2026-10-20, `AHeroSelectStage`, `UI/Menus/HeroSelectStage` + `HeroSelectStand.cpp`): the chosen hero's
  game model on a plinth, rendered into the screen's middle (picture 512 x 640 at 600, 372 on the page: right of the
  roster's middle, as Crane's hat crossed the title's end at 560; from a 720 x 900 capture through the loadout's cut-out
  material `M_UI_LoadoutStage`).
  - Built like the other off-screen stands (`StageStudio`, stand index 4) and like the player's body: a hidden
    SKM_Manny_Simple plays ABP_Unarmed with the stance layer's weary idle, and `UBodyFigureComponent` wears the hero's
    FigureMesh through their Retargeter (`SetShownHero`: a stand's hero instead of the hero played). The stand is an
    `APawn`, never possessed: the anim instance needs a pawn owner standing still to ease the weary idle in, and reads
    which way the body faces from it, so the whole actor turns (the camera and lights are absolute).
  - Framing: camera 600 cm out at 110 cm, looking at 96, 20.5 degrees across (the tallest hero, Crane, and the plinth's
    front foot both fit with ~1 degree to spare). Turn: a three-quarter rest (12 degrees) swaying 16 either side over 14 s;
    a drag (0.45 degrees a pixel; dragging right turns the front right) or the right stick (120 degrees a second, dead
    zone 0.2; the main menu hands RightX on) turns them, the sway resting 2.5 s then easing back over 1.5 s from where the
    player left them (its share is folded into the player's turn, so nothing jumps).
  - Lights (candelas, live: `Looter.HeroStand.Key` 160 warm spot with the one shadow, `.Fill` 20 cool, `.Rim` 420 in the
    hero's colour from behind, `.Kicker` 200 in their first power's colour from the other side, `.Exposure` 1); colours
    normalized to full strength so a dark hero colour still lights. The rim's colour at full strength lights at most
    luminance 0.6 (`RimShareFor`, 2026-10-20): Crane's pale sage (0.84 at full strength) blew his pale grey-green face out
    in green-white patches (`final_crane.png`; with the rims off it read clean), now x0.71; Pike's teal x0.85; Ellis,
    Odessa and Gauge whole. (His skin's red-and-white blotches then were the Subsurface Profile's; gone with the
    Preintegrated Skin, see Bugs.) A 1.5x capture (1080 x 1350) smooths edges for +0.7 ms GPU (not made).
    The hero is on channel 1, the plinth on channel 2
    (key and fill only: the rims grazing its top turned it into a disc of the hero's colour). The sky light is off in the
    capture (it flattened everything blue), and AO, contact and capsule shadows, translucency, eye adaptation and lens
    flares.
  - The plinth: the engine's cylinder three times (top r 60 MetalDeep, band r 61.5 glowing in the hero's colour through
    M_StylizedSurface's Glow 6, foot r 70 Ink).
  - A switch: the picture fades in from nothing over 0.28 s (the swap's first frame never shows) while the hero turns in
    from 70 degrees off over 0.6 s (ease-out cubic), the rim flaring (x2, fading over 0.7 s) and the band brightening. The
    confirm: the picture lifts 3%, rim x1.9, kicker x1.8, key x1.35, band x2.5 (`SetFlare`). A hero in the works with a
    body stands as a silhouette (exposure 0.03, the bestiary's unmet creatures) with the IN THE WORKS stamp.
  - Cost: spawned the first time the screen opens, kept, inactive (no tick, no capture, lights off) whenever it's closed
    (`UHeroSelectWidget::Close`, from the main menu's CloseHeroSelect and ShowMainButtons), destroyed with the widget.
    Measured in PIE on Medium (2026-10-20, `Looter.HeroStand.Run 0` holds it for comparisons): +1.4 ms GPU (6.1 against
    4.7 for the whole frame), +2.1 ms render thread, +0.2 ms game thread; the main menu without the hero select pays nothing.
- **Keys:** one roster row then Back/Play (`FMenuNavigator`, wrapping). Left/right move and the choice follows; 1-5 and
  the bumpers choose; A or Enter on a card or Play confirms; B or Esc back to the picker (focus on that New Game). Mouse:
  click a card to choose, Play to confirm; a card under the mouse lights. Sounds: UI.Hover as the choice moves, UI.Click
  for a click, UI.Equip + Ember.Ready on confirm, UI.Denied (and the reason on the panel) for a hero in the works,
  UI.Back. Confirm plays a 0.85 s beat (the stand's picture lifts 3%, its lights flare, the glow swells) before
  `OnConfirmed`; keys are ignored until the level opens or `CancelConfirm` (StartNewGame refused). The hints add "Drag
  Turn" (keyboard) or "RS Turn" (pad) while the stand shows.
- **Session cards** name their hero: the face cut in a 72 px square with a hairline in their colour, the name in their
  colour under the level; the delete popup names them too.
- **The HUD portrait** (2026-10-20): each hero's HUD portrait is an SVG in Ellis's structure (groups glass, colour offset
  1.3 x 0.9, rim, ink, eyes-open/hurt/flare with gradient ellipses as glows): `Art/Icons/HudPortrait[_<Hero>].svg` ->
  `UI/Style/HudPortraitData[_<Hero>].inl` by `Art/Icons/HudPortrait.py [Hero|all]` (default Ellis; the generator also reads
  plain `<circle>` and `<ellipse>` and refuses a gradient ellipse outside the eye groups); previews
  `Saved/ArtPreviews/Icons/HudPortrait_<Hero>_<State>[_Hud].png`. `UHudPortraitWidget` picks the set by hero id
  (`PortraitSetFor`, anonymous in the .cpp; any other id shows Ellis's) and names each hero's painted textures apart
  (`HudPortrait<Hero>Bust`; Ellis's keep `HudPortraitBust`): painted icons are cached by name for the session. It asks
  `FindPlayedHero` as it builds (the same answer the body and powers get; none in a test world: Ellis); `SetHero(Id)`
  redraws a built window (`BuildPortrait`), which `Looter.Hero <Id>` calls through `WearHeroNow`. Each hero's glow lists
  take any count and colour (Crane's one eye: 2 glows, a warm glint on his coin in the flare; Gauge's lamp: warm, blink
  centre 89.0). Gauge's squint also flickers its glows like a failing lamp (`FPortraitSet::bHurtFlickers`; two beating
  sines on `BlinkClock`, never below 0.45). Four .inl (50-70 KB) compile in the one .cpp; only the played hero's
  textures are drawn. Test: `Looter.HUD.PortraitSets` (`Tests/HudPortraitTests.cpp`: a whole set per roster id, texture
  names apart, unknown id = Ellis's, the built widget's image count follows the hero).
- **The third-person hold** (2026-10-20): the guns hang from the hidden mannequin's `HandGrip_R` (the left hand solved
  onto the foregrip) and every hero's hands are pinned to its hands (the retargeter's Blend to Source), so a gun sat where
  the mannequin's shoulders and eyes are, whoever wore it. Measured aiming the Bullpup (world cm at the player's 0.85, from
  the grip; Ellis = the mannequin within 2.5): Crane's head and shoulders 14 higher (every gun at his chest), Pike's head
  6.6 lower (the sight over his eyes), Odessa's 3-4, Gauge's head 3 lower and his shoulder joints 5.5 out and 6.5 back in
  the pauldrons (the rifle ran through his helm). `UHeroDefinition::ThirdPersonHold` (cm on the full-size body: along the
  aim, right, up; create_ember_assets.py `THIRD_PERSON_HOLD`: Ellis 0, Odessa (0, 0, -4.5), Crane (0, 0, 16.5), Gauge
  (5, 3, -8), Pike (0, 0, -6.5)) -> `UBodyFigureComponent::GetHoldOffset` (the body worn's; zero when Ellis's stands in)
  -> `FLooterStanceInput::HoldOffset` -> `LooterStancePoseDetail::ApplyHoldOffset`: both mannequin hands moved by it (arm
  IK, turn kept), pitched as the torso pitches with the aim, before the six-gun's reach; the carry's chest and side
  spots move by it too (`ApplyCarry`). Gauge's stocks still meet his right pauldron (expected: shoulders 12 cm wider);
  lowered, the rifle runs under his helm. Test `Looter.Heroes.Hold` (`Tests/HeroHoldTests.cpp`).
  - Looking: `Tools\heroguns.ps1 [-Setup] [-Tag] [-Heroes] [-Slots] [-States] [-Views]` (`Tools/Unreal/hero_guns.py`):
    one contact sheet per hero, `Saved\Screenshots\HeroGuns\<Hero>_<Tag>.png` (rows gun x state, columns views; the
    2026-10-20 `_before` and `_after`). Lessons: aiming works only in third person (not the front view), so the capture
    moves the third-person camera onto each view every frame (relative to the boom's socket); `take_high_res_screenshot`
    photographs the PIE viewport without the HUD; console.ps1 retypes a command whose echo it can't find in the log, and
    a long one ran 8 times at once (hero_guns.py reads its arguments from a file and stops a run before it); a reload
    check calls the weapon's StartFire/StopFire/Reload after `play.py ammo` (a fresh play has no reserve). Twice
    (2026-10-20) the editor crashed in python311 (access violation reading 0x00007465736c6f18, the bytes of "olset" as
    in a toolset's name) at the first MCP call 25-35 s after a run of Python tick-callback scripts had finished
    (hero_guns, a lamp and Fleet check, a first-person check); cause not found: after a long capture session, restart
    the editor before other work.
  - The left hand's grip (2026-10-20): `LooterStancePoseGrip.cpp`. The hidden mannequin's left hand is turned onto the
    gun by its hold kind (`GripOf`: Handguard, Pump for a shotgun, SixGun for `HoldReach > 0` or a revolver; gun-space
    Handle/Palm/Turn tables in `MakeHandguard`/`MakePump`/`MakeSixGun`, started from first person's `FGunHands`) and its
    fingers and thumb curled from straight (rest bends read off the reference skeleton, as the first-person rig does):
    handguard 67/63/30 (at 55/55 the fingers stood off the shell), pump 55/70/45, six-gun 45/60/40 (knuckle/middle/tip
    degrees); thumbs 15/10/5, 15/10/0, 18/5/5. All of it blends by `ForegripHold x GripShare`, so it lets go in a reload, a
    one-handed carry, a throw or a climb. The palm point stays on the foregrip; `ApplyCarry` estimates the left arm's
    reach with the gripping hand (`LeftHandOnGun`), else the pump falls short in a sprint. Heroes copy it through their
    retargeters. Tune live with `Looter.Grip.*` (`.On 0` shows the old open hand), bake with `Looter.Grip.Dump`. Lessons:
    the rifle animation's and the mannequin's left fingers already rest bent 36-64 degrees, so tests check where the
    fingertips end up (round the gun), not closing against the rest pose; the mannequin's smaller hand closes its outer
    fingers inside the Bullpup's shell, the heroes' lie on its far side. Test `Looter.Anim.Grip.*` (`Tests/StanceGripTests.cpp`).
  - The camera per hero (2026-10-20): the over-the-shoulder camera was tuned to Ellis, so Crane's hat and Gauge's stacks
    filled the left of the aimed view. `UHeroDefinition::ThirdPersonCamera` (cm on the full-size body: X extra arm length,
    + back; Y right; Z up; create_ember_assets.py `THIRD_PERSON_CAMERA`: Ellis 0, Odessa (-5, 0, -3), Crane (20, 8, 16),
    Gauge (45, 12, 4), Pike (-10, -4, -8)) -> `UBodyFigureComponent::GetCameraOffset` -> `UPlayerViewComponent::
    UpdateHeroCamera`: added to the arm and shoulder offset over the shoulder only, on top of the `Looter.ThirdPerson.*`
    overrides, eased at 7/s on a `Looter.Hero` swap (snapped in first person and the front view). Tuned in PIE with a
    Bullpup, hip and aimed: the hat or stacks at Ellis's height on screen, the head bone within ~0.01 of his; a wall behind
    and crouch frame as Ellis's. Not checked: the Ranchhand and Drover, Gauge and Pike at 4x. Tests
    `Looter.Heroes.CameraData`, `.CameraBoom` (`Tests/HeroCameraTests.cpp`).
  - Left as they were, Ellis's too: the Bullpup's reload tips its stock up across the face; the Ranchhand's reload swings
    the left arm out at the side; a flat white card by the Ranchhand's receiver in right-side views (seen, not looked into).
- **Stances and the third-person aim** (the user, 2026-10-22; CharArt sheet 2, `Docs/Work/2026-10-22-CharArt/PLAN.md`
  items 2-3): `UHeroDefinition::Idle` (`FHeroIdle`) and `::Aim` (`FHeroAim`, `Player/Animation/HeroPoseTypes.h`),
  create_ember_assets.py `HERO_IDLE`/`HERO_AIM` (fields that differ from the defaults; the defaults are Ellis's weary idle)
  -> `UBodyFigureComponent::GetIdle/GetAim` (the worn hero's; defaults when Ellis's body stands in) -> `HeroPoseTuning`
  (the `Looter.Pose.*` cvars) -> `FLooterStanceInput::Idle/Aim` -> the hidden mannequin's stance layer; heroes copy it.
  Their hands are pinned to the mannequin's, so each rest has a per-hero offset (ahead, out, up; ~ the THIRD_PERSON_HOLD
  heights).
  - Stances: Ellis keeps the weary revenant (thumb in the belt in spells, left hand loose). Crane stooped (lean 8), head low
    and forward, left hand closed on the toll chain at his chest (`ChainHand`). Odessa hip cocked (`HipCock` 1.8 shifts),
    chin up, right hand on the hip (`HipHand`), left raised with a card walking her knuckles (`CardHand`: a knuckle each
    0.8 s, a half turn each, passed back under; the card is two engine planes back to back, ivory face and her red back on
    M_StylizedSurface MIDs, set on the hero mesh's `*_01` knuckle bones each tick, owned by the body's actor so the select
    stand's ShowOnly capture shows it). Gauge wide (feet 7 out each, hips settling to reach), hands hung 14 out clear of the
    boiler, a 1.5 cm piston sway over 5.5 s, slow breath, steam from both stack mouths every 3rd breath out
    (`SteamVent` (-14, 19.7, 191) from gauge_parts.py; FWeaponFX wisps, cue `Hero.Gauge.Vent`; in play only: the stand's
    capture has translucency off), `HeadLift` 6 in every stance (the user: "looking down all the time"; the layer runs for
    it alone). Pike upright, weight back (hips 3 back, lean -3), thumbs in both pack straps (`StrapThumb`), head 28 to his
    right and a little up, few glances.
  - Every idle hand rests curled (`LooterStancePoseIdleHands.cpp`, PLAN item 2): absolute bends from straight read off the
    reference skeleton (`LooterStancePoseHands.cpp`, the grip's way for both hands): loose 35/45/30 with the thumb in
    20/15/10; a rest's own hold (a fist on the chain 60/70/40, fingers over a strap 40/50/30); flexes on top; the card's
    finger straightens under it. `RestCurl` scales it (`Looter.Pose.Curl`; `Looter.Pose.On 0` shows the old shared idle).
  - Aim down the sights in third person (the user: "no aim-in animation in 3rd person"; `LooterStancePoseAim.cpp`): by
    `UPlayerViewComponent::GetAimAlpha` (eased, so each kind's `Looter.Gunfeel.Aim.Time.*`), after the hold offset and a
    six-gun's reach: the head 7 down (40% neck) and canted 6 to the gun (a six-gun 30% of it), then both hands moved (arm
    IK, the elbows' hint down 20 -> 35 and out 10 -> 2) until the gun's sight line (`AWeaponBase::GetAimPoint` from the
    grip) runs through the eye with the grip `GripAhead(Kind)` ahead of it (rifle 14, shotgun 15, six-gun 40, SMG 13,
    sniper 14, LMG 14, launcher 12; `Looter.Pose.Aim.Grip.<Kind>`). The eye = the head bone + `EyeFromHead` (9 ahead, 3.5
    right, 10 up; `Looter.Pose.Aim.Eye*`) in the body's frame pitched with the aim + the hero's hold + `FHeroAim::EyeOffset`
    (Gauge (0, 6, 2): his sight beside the helm). Off in a reload, a carry (sprint, swap), a climb and a throw's dip.
  - Tune live: `Looter.Pose.Set <Hero> <Idle|Aim>.<Field> <Value>` (in memory), bake `Looter.Pose.Dump` lines into the
    script. Tests `Looter.Pose.Idle.Clock`, `.Curl`, `.Heroes`, `.HeadLift` (`Tests/PlayerIdleTests.cpp`),
    `Looter.Pose.Aim.Raise`, `.Data` (`Tests/PoseAimTests.cpp`).
  - Gauge's revolver aim (2026-10-22): the two-hand aim splayed his arms over the pauldrons (5.5 cm wider than the mannequin's shoulders), so HERO_AIM Gauge has `elbow_in` 1.4 (elbows down 40 not 35, out 2) and `grip_ahead` 3 (arms straighter). Judged in PIE from behind-right (`Looter.ThirdPerson.ShoulderRight 70, ArmLength 110, ShoulderUp 15`; reset them to -1): hands and forearms a little lower and closer in, the arms under the pauldron cap; the difference is small from any camera, no front view aims. Not tuned further without the user's eye.
- Tests: `Looter.Heroes.Definitions`, `.Playable`, `.Save`, `.Screen`, `.Menu`, `.Character` (`Tests/HeroTests.cpp`);
  `Looter.Heroes.Stand` (`Tests/HeroStandTests.cpp`: the stand builds for each of the five with their own body and
  retargeter, turns, stops; the screen puts the chosen hero on it, the right stick turns them, a hero with no body gets
  their painting, Close stops it).
  `.Playable` checks every power has code and a hero is playable exactly when body and retargeter load; `.Character`
  expects Toll Chain's own Refused on a bare actor. `.Screen` still says "Odessa is in the works": update it when SK_Odessa lands.

## Melee tools (playtest round 3, 2026-10-27, note 5: "melee is jank, we want each character to have a specific tool they hit with the right hand")
- 2026-10-28: the overnight Crane/Odessa grabs predate f2423b2d (which fixed both). `Looter.Player.Hands.Grip.Tool` poses the
  rig on each held tool (the gaff: four fingers on the pole); `Melee.Tool.Hand` keeps Odessa's glove 38 cm+ from the eye in view.
  Crane's yank draws the butt cap ~19 cm from the eye (`ThroughReach` 0.75, Gaff arc): try 0.85 if it reads too big.

The user picked one tool per hero from Main's concept sheets (`Art/Concepts/MeleeTools/`: three options A/B/C per hero, README
lists them all, `node sheets.js [Hero...]` redraws the PNGs; the unpicked options stay there as ideas). Modeled in
`Art/Models/Weapons/Melee/MeleeTools.py` (`Assets.md` "Melee tools"), wired in `UHeroDefinition::MeleeToolFor` (`Game.md` "Melee"
has the swing's clock and fallbacks). A set `DA_Hero_<Id>` `MeleeTool.Mesh` overrides the table (`SetMeleeTool` is callable from a Python script; no script sets it yet).

| Hero | Pick | Mesh | Style, heft, grip | Idea (the sheet's) |
|---|---|---|---|---|
| Ellis | A Grave Spade | `SM_Melee_GraveSpade` (95 cm) | Swat, Steady, held | the Sexton's spare spade that filled their grave; flat-blade backhand swat, high right to low left; black iron blade worn bright, ash shaft and D-grip, a cyan ember crack |
| Odessa | A Razor Fan | `SM_Melee_RazorFan` (5 cards, 7 x 9.4 cm) | Flick, Quick, held | her steel-edged trick cards (the deck Fifty-Two Pickup throws); backhand flick, low left to high right, the fan snaps open mid-cut; gold rims, ace of spades on top, oxblood backs |
| Crane | A Ferry Gaff | `SM_Melee_FerryGaff` (120 cm) | Gaff (chop, then a yank back), Heavy, held | a ferryman's boat hook for the man owed his fare (rhymes with the toll chain); grey ash pole, black iron spike and hook, a fare coin nailed on |
| Gauge | C Piston Fist | `SM_Melee_PistonFist` (a forearm attachment, no grip) | Jab, Quick, Forearm | the right forearm's piston (the one behind Rail Spike) drives a knuckle plate; the piston kicks at the blow (`Looter.Melee.PistonPush`); three firebox-hot knuckle caps, a steam vent |
| Pike | A Rock Pick | `SM_Melee_RockPick` (35 cm) | Peck, Quick, held | the surveyor's field hammer; short overhand peck, pick first, down and left; steel head, hickory handle bound in brass wire, a teal band |

- **Mesh convention** (`PlayerMeleeToolTypes.h`, `FHeroMeleeTool`): `SOCKET_Grip`: the grip point, X up the handle toward the
  head, Z the striking side, Y the way the right palm faces onto the handle. Gauge's `SOCKET_Forearm` on the forearm's axis at the
  front clamp ring, X toward the knuckles (the punch way), Z out of the back of the hand. Other sockets: `SOCKET_Tip` (striking
  end; its distance from the Grip is `ToolTipLength`, which aims a long tool's arc), `SOCKET_Hook` (the gaff's hook point),
  `SOCKET_Vent` (Gauge's steam vent: the steam on a hit comes from it). No socket:
  the mesh's origin and axes stand in. Wrong socket axes hold the tool turned: check in play and tune with `Looter.Melee.*`.
- The hands: the right hand leaves the gun's grip for the tool (`FirstPersonHandsActions.cpp` `RightOnTool`, `GetToolHold` gives the
  grip point and `HandleRadius`); Gauge's fist stays bare inside the clamp. Round 4: `ClosedRound` (`FirstPersonHandsCast.cpp`) shuts all four fingers round a slim handle (the gaff's pole 1.75 cm radius, the pick's 1.45, the spade's 1.55, the fan's pivot 1.2: `FHeroMeleeTool::HandleRadius`); round 3's looser curl left Crane's fingers open beside the gaff.
- Swing styles by feel tag: `Heft` scales the arc (`Looter.Melee.Heft.Quick` 0.8, `.Heavy` 1.25). Same damage, clock and hit-stop for
  every tool, so a tool is look and feel, not a stat.
- Round 4, melee polish (HANDS, the user's pick; a play-check fixed the rest):
  - Sounds: each tool has its own swing (2D, replaces the shared `Player.Melee.Swing` for that hero) and hit (3D Near, on top of the
    shared `Player.Melee.Hit` and the body layer; on walls at `MeleeMotion::WallVolume`): `Player.Melee.<Spade|Fan|Gaff|Piston|Pick>
    .Swing/.Hit`, 4 takes each (`melee_tools.py`; named by key: `MeleeFX::SwingCue/HitCue`, `FHeroMeleeTool::Kind`). A tool set on
    `DA_Hero_<Id>` keeps its pick's `Kind` unless it names its own. Ellis a ringing clang, Odessa a slicing snick, Crane the hook's
    bite, Gauge a hiss-thump, Pike a bright chink.
  - Effects (`Weapons/WeaponFXMelee`, `FWeaponFX`'s code-drawn effects, no assets; `Looter.Melee.FX` strength, `.Sparks`, `.Steam`):
    sparks (`MeleeFX::Sparks`, world hits only, never on a body): spade, pick and gaff off stone, gravel, metal and sheet metal, the
    fan off iron alone (weaker), the piston none (it meets a surface with steam). Spade: a flick of the cyan ember on a body; fan:
    four edge glints, gold and white; gaff: grit and splinters dragged back toward Crane, a pale iron glint on a body; piston: a
    ring of the kick and a burst of steam off the plate, and steam from the cylinder's vent (`FindToolSocket` finds where
    `SOCKET_Vent` shows); pick: stone chips off rock, a teal glint on a body. The play-check made the sparks fuller.
  - Third person: the tool rides the body's right hand (the gun's socket `HandGrip_R`: handle up, striking side forward, palm from the
    right; a forearm tool with the punch forward) and shows only through the swing; the gun in hand is hidden for it
    (`HideGunForSwing`, restored only if it is still the gun in hand) through the body's jab, whose wrist lays the tool back with the
    coil and snaps it over at the blow (`LooterStancePose::MeleeWristDegrees`). Tune `Looter.Melee.ThirdPerson.Pitch/Yaw/Roll/
    WristBack/LayBack/SnapOver`. No tool on the belt or back at rest; the basic-shape stand-in has no third-person tool.
  - First person (play-check fixes): every tool part is born hidden, hidden at rest and on a hero change (Gauge's fist stayed in view
    after `Looter.Hero Ellis`); a long tool's end crosses the view's middle at the blow, 4 cm under the crosshair, not its handle
    (`AimAtTip`, for tools whose tip is 30 cm or more from the hand: the spade and the gaff; `Looter.Melee.AimTipFrom/AimTipDrop`);
    the spade is rolled to show its blade, not its edge (`FaceView` 0.75 on its arc, `Looter.Melee.FaceView` scales all);
    `Looter.Melee.WristBack` 9 -> 15 (the piston fist's clamp sits 6-7 cm behind the wrist, its front 19 cm ahead of it).
  - Tests `Looter.Melee.Tool.Feel` (each tool's key, cues, and which surfaces throw sparks) beside Picks, Arcs, Hand.
  - Not done: the arcs, cast spots and third-person turn are guesses until the user plays (the cvars tune them); the third-person tool's
    turn depends on `HandGrip_R`'s axes.
- Round 5, swing feel (HANDS, 2026-10-27, the user's pick from a video: "swing smear + hit feel"; test build `2026-10-27_1818`):
  - The gun is out of the way: it drops out of the frame over `Looter.Melee.GunLowerSeconds` 0.08, is hidden while all the way down
    (`MeleeMotion::GunAwayShare`/`IsGunAway`, `HideGunForSwing`) and returns over the swing's last `.GunRaiseSeconds` 0.08. Before this
    the gun stayed in the left hand and the tool/arm left the view.
  - The tool stays in view through the arc, with the right arm: the arcs had carried the hand off-screen for all but the first 0.1 s
    of the swing (`Bugs.md`). `Looter.Melee.Tool.Shown` checks tool and arms visible mid-swing and hidden at rest.
  - Smear: a code-drawn glowing band behind the tool's tip, laid from late in the wind-up through the follow-through, tinted per tool
    (`MeleeFX::SmearColor`), fading in `Looter.Melee.Smear.Seconds` 0.12 (`.Inner`, `.Glow`, `.Width`); on `FWeaponFX`'s streaks, no
    assets (`PlayerMeleeSmear.cpp`). Test `Looter.Melee.Tool.Smear`.
  - Hit-stop on the attacker: `Looter.Melee.HitStop` 0.065 s (0.05 went by unseen), `.HitStop.Kill` 0.085 s, both held to
    `FMeleeRules::AttackerHitStop` 0.09 so the swing still ends before the cooldown; a wall hit is 0.6 of the first. The struck creature's
    own stagger/flinch is the existing `FHitReaction::MeleeHitStop` 0.06 and sparks per material are round 4's.
  - The hit's view kick and shake (`MeleeMotion::Impact`, `KillShake`) are round 3's. Checked in PIE; `Looter.Melee` passes (13 tests).
