# The four heroes' game models and the hero select screen: the build contract (2026-10-19)

Why: the hero pick (`Docs/Story.md` "The heroes"): five playable heroes chosen before a new game, Borderlands' way. Ellis
has a game model (SK_Ellis) and three powers. The user picked the other four's looks from painted portraits on
2026-10-19 and asked for **all four models at once** and **the select screen alongside** (built now with the painted
portraits; models drop in as they land). Rules: `CLAUDE.md` (read it); no copyrighted likenesses, names or text.

The picks (the spec of each model; `Art/Portraits/<Hero>.py` `OPTIONS[<key>]`, `FACE`, `MACRO` and its builders; the
painted sheet `Saved/ArtPreviews/Portraits/<Hero>/Sheet_<Hero>.png`, the bust `<Hero><key>.png` beside it):
- **Hollis Crane, the Unpaid: A The Marshal.** 1.95 m, gaunt (a stoop only as a shape: head and shoulders carried
  forward in the mesh; the posture itself comes from the mannequin's animations). Grey-green skin, a tarnished coin over
  the LEFT eye, cyan light in the RIGHT socket, drooping white mustache, long white hair under a battered slouch hat; the
  ember glowing through a tear in his shirt; a long tattered slate coat and a ragged shoulder cape; the tin star; the
  toll chain from shoulder to hip with its hook.
- **Wendell Pike, the Surveyor: A The Surveyor.** 1.68 m, stocky. A brown bowler with brass goggles pushed up on it,
  round spectacles over cyan eyes, grey mutton chops joined to a big mustache, a teal neckerchief, a khaki field coat
  full of pockets under a cartridge belt, puttees and boots; the pack with the folded tripod (brass-shod legs), map tube
  and bedroll; a brass spyglass at the hip. **No rifle on the pack** (the game's guns are his guns).
- **Gauge, the Iron Hand: C Company Iron.** 1.92 m, massive; the railroad's clockwork track-layer the day it left the
  works: black enamel and polished brass, an enamelled crest on the chest, a brass nameplate GAUGE on the brow, a tidy
  headlamp with warm white light (#ffe2a8, not cyan), no soot; boiler chest with the pressure gauge for a heart, the
  firebox glowing in the belly, two stacks, domed pauldrons, one headlamp eye in a squared head, a grille mouth, a steam
  whistle at the shoulder, three-fingered riveted hands (thumb + two). No MakeHuman body: hard-surface parts.
- **Odessa Lark, the Cardsharp: A The Dealer.** 1.72 m, slight. The flat-crowned black gambler hat with a playing card in
  its band (a plain pip, no real deck's art), cyan eyes, a half smile; the wine-red tailcoat cut away at the waist with
  long tails, gold brocade waistcoat with a watch chain, white high collar and black cravat, black gloves, gold
  earrings, tall heeled riding boots, a card case. **The card in her hand stays in the portrait** (her hands hold guns).
- The lower halves aren't in the busts: design them from the looks above (`Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`
  "Five heroes") and the old concept models `Art/Backlog/Characters/<Hero>.py` (proportions, legs, boots; the user
  disliked their look, not their layout). Stay in the portraits' look: Ellis's "less smooth" standard of real folds,
  creases, seams and wear.

## Shared rules for every agent

- **Never commit, push, or run `git` commands that change the index or tree** (Main commits). Touch only your own files
  (below). If shared code lacks something, write it in your own file and name it in your report.
- **Every Blender run goes through `Tools\artrun.ps1`** (`-Script <py> -Threads 6 [-ScriptArgs a,b]`, `-Export <Hero>
  -Family <Hero>`): at most two Blender runs go at once on this PC (16 GB, 6 cores) and it queues the rest. Never call
  blender.exe or `Tools\models.ps1` directly. Run anything over ~2 minutes in the background.
- Don't open, close or build the Unreal Editor unless your role says so (SELECT does); never send keys to it.
- Look at your own renders (Read the PNG) and iterate. Report in under 40 lines: what's done, every decision you made
  (as a list), what's left, what you'd want Main to merge into shared code.

## The models (CRANE, PIKE, GAUGE, ODESSA): one agent each

Read first, whole: `Art/Models/Characters/Ellis.py` (the worked example: build, cloth simulation, game mesh, bake,
rig, swing chains, hit zones, facing), `Docs/Pipeline.md` "Character models" (every lesson: read the bugs met), and
your portrait script. Then `Tools/Blender/looter_human.py`, `looter_garments.py`, `looter_looks.py`, `looter_bake.py`,
`looter_audit.py`, `ellis_hair.py`, `ellis_hat.py` as you need them.

**Your files (nobody else's):** `Art/Models/Characters/<Hero>.py`, its cache `<Hero>.drape.json`, any
`Tools/Blender/<hero>_*.py` (lowercase: `crane_hair.py`, `gauge_parts.py`), `Art/Textures/<Hero>*/`,
`Saved/ArtPreviews/Characters/<Hero>/`, `Saved/Concepts/<Hero>*/`, `Intermediate/ArtExport_<Hero>/`.
**Read-only:** every other file, above all `Tools/Blender/*` (the bake digest fingerprints scripts: an edit there
re-bakes Ellis), `Art/Portraits/*` (import your hero's portrait module the way Ellis.py imports `EllisBlender`; its
builders take MakeHuman space, see `Figure.to_portrait`), `Ellis.py` (copy and adapt from it; don't import it).
Keep each file under ~1500 lines: split by topic into your `<hero>_*.py` modules.

What the game needs (it reads these by name; anything else needs C++ and goes in your report):
- **One rigged model:** an armature with one skinned mesh named `<Hero>` -> `/Game/Art/Characters/SK_<Hero>` (+
  `SK_<Hero>_Skeleton`, `PA_<Hero>`). Built facing -Y, turned by a `face_like_mannequin` step last (Ellis.py's), so it
  faces +Y in Unreal like SKM_Manny_Simple. Nanite off; LODs 50 % and 25 %.
- **Up to 35,000 triangles** (the user's budget for Ellis); `--stats` prints them per part. Spend on silhouette,
  face, hands, hat brim, coat edges.
- **Bones:** the UE5 mannequin's 63 deform bones by name and hierarchy on the body's joints (`looter_human.skeleton()`;
  Gauge places them on its own joints at human proportions), `lowerarm_twist_01_l/r` and `lowerarm_twist_02_l/r` with
  the forearm weights shared as Ellis's `twist_weights`; arms long enough that the hands reach where the mannequin's do
  (the guns hang from the hidden mannequin's hands; the retargeter puts this body's hands there). Gauge's three fingers:
  thumb on `thumb_*`, the two fingers on `index_*` and `middle_*` (ring and pinky bones present, unweighted).
- **Swing chains**, only with the prefixes the game knows (`Player/Animation/BodySwing.cpp`): `skirt_` (a coat's skirt
  or tails; names from the ring `skirt_fl, sl, bl, bli, bm, bri, br, sr, fr`, any subset: Odessa's tails e.g.
  `skirt_bl, skirt_bm, skirt_br`), `cape_` (`cape_{f,a,s,b}{l,r}`, `cape_bm`, with `cape_arm_l/r` as Ellis's), `bandana_`
  (a neckcloth's point: Pike's neckerchief), `hair_` (long back hair: Crane), `lantern_` (a pendulum: Crane's hook; a
  hanging spyglass or watch if it should swing). `_01.._03` per chain, each ending in an unweighted `<chain>_end` leaf, as
  Ellis.py's `swing_layout`. Gauge: none needed.
- **`hat`** bone on `head` for a hat (Crane, Pike, Odessa), as Ellis's `hat_bone`: the game tips the hat back while
  aiming (up to 20 degrees) so the brim clears a scope; check the tipped brim clears hair and collar.
- **Hit zones** as Ellis's `hit_zones` (head, trunk, hips/skirt, each limb, hands, feet): every visible part inside one.
- **Texture sets** at 2048, at most five, named `<Hero><Set>` (`CraneSkin`, `CraneCoat`, ...) in
  `Art/Textures/<Hero><Set>/T_<Hero><Set>_BC|N|ORM.png`, baked as Ellis's (`looter_bake`), with its digest file.
- **Material slots** `<Hero><Slot>` on the character masters that exist (`CharacterSkin`, `CharacterCloth`, `Character`,
  `CharacterHair`; glowing parts as Ellis's `GLOWS`: eyes `#5AD8FF` for Crane's socket, Pike's and Odessa's eyes; Crane's
  ember; Gauge's lamp `#FFE2A8` and firebox) via a `game_materials` step like Ellis's. No new masters.
- A holstered gun, if a hero wears one, is its own slot `<Hero>HolsterGun` (the game hides it while a revolver is drawn).
- Script flags as Ellis.py's where they apply: `--stats`, `--shape` (clay), `--preview` (the baked look, arms down:
  front, three-quarter, side, back, and a face close-up, into `Saved/ArtPreviews/Characters/<Hero>/`), `--rig-check`,
  `--audit <tag>` (`looter_audit.POSES`: idle, aim, stride, crouch, grips; every layer against the one over it; into
  `Saved/Concepts/<Hero>Audit/<tag>/`).

**Done means:** `artrun.ps1 -Script Art\Models\Characters\<Hero>.py -Threads 6 -ScriptArgs --preview` builds, bakes and
renders without error; the previews read as the picked portrait at a glance (you looked: face, hat, silhouette front and
back); `--audit` shows no part through another in idle, aim, stride and crouch (or each one left is listed);
`artrun.ps1 -Export <Hero> -Family <Hero>` writes the FBX without error; the report lists the bone count, triangles,
texture sets, slots, chains and every decision. Not this round: first-person arms, the spin viewer, the Unreal import
and retargeter (Main).

## The select screen (SELECT): one agent, C++ in the game module

Read first: `CLAUDE.md` UI rules, `Docs/Pipeline.md` (UI kit, menus, sessions), `UI/Style/LooterUIStyle.h`,
`UI/Menus/MainMenuWidget.*` + `MainMenuSessions.cpp` + `MenuNavigator.*`, `Session/SessionSubsystem*`, `SessionSave.h`,
`Embers/HeroDefinition.h`, `EmberDefinition.h`, `EmberComponent.*`, `Player/Animation/BodyFigureComponent.*`,
`Tools/Unreal/create_ember_assets.py`, and the cloud's page `Docs/HeroSelect/ui.html` (the layout idea: a roster of the
five, the chosen one large, a panel with name, role, story and powers).

**Your files:** new `UI/Menus/HeroSelect*.{h,cpp}`, new `Tests/HeroTests.cpp`, new `Tools/Unreal/import_hero_portraits.py`,
`Art/Portraits/Picks/<Hero>.png` (copied in), and edits to `HeroDefinition.h`, `EmberComponent.*`,
`BodyFigureComponent.*`, `MainMenuSessions.cpp`/`MainMenuWidget.*`/`MainMenuNav.cpp`, `Session/SessionSubsystem*`,
`SessionSave.h`, `create_ember_assets.py`, a `Dev/` command file, `CODEMAP.md`, `Docs/Pipeline.md` (a new section).
You may build (`Tools\launch.ps1 -Build`), run the editor, MCP and `Tools\runtests.ps1`. First check nobody is playing
(`Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`). Blender agents run beside you at below-normal priority.

The interface the rest of the work relies on (exact names):
- `UHeroDefinition` gains `FText Role` ("the Revenant", "the Cardsharp", "the Unpaid", "the Iron Hand", "the
  Surveyor"), `FText Story` (the concept doc's one-liners; Ellis is "they" in all text), `FLinearColor Color` (Ellis
  `#5AD8FF`, Odessa `#D04A68`, Crane `#B9D98C`, Gauge `#FFB347`, Pike `#3FBFAA`, as `create_ember_assets.py`'s
  `linear_color`), `TSoftObjectPtr<UTexture2D> Portrait` (`/Game/UI/Portraits/T_Portrait_<Id>`),
  `TSoftObjectPtr<USkeletalMesh> FigureMesh` (`/Game/Art/Characters/SK_<Id>`), `TSoftObjectPtr<UIKRetargeter>
  Retargeter` (`/Game/Player/Retarget/RTG_MannyTo<Id>`), and `bool IsPlayable() const` (mesh and retargeter load and
  every power has a power class). Ids: `Ellis`, `Odessa`, `Crane`, `Gauge`, `Pike` (never renamed, like part keys).
- `DA_Hero_<Id>` for all five (`/Game/Data/Embers/`), from `create_ember_assets.py`; the twelve new powers as data
  `DA_Ember_<Id>` (`LuckyStreak, FiftyTwoPickup, DoubleOrNothing, TollChain, GraveWard, LastRites, SlagBomb, IronHide,
  RailSpike, Spyglass, Tripwire, LongShot`; words, nature, colour and numbers from the concept doc's "Five heroes" and
  "Ember powers"; no power class yet, no icon yet). Only Ellis is playable today; the rest light up as Main lands each
  model, retargeter and power.
- The session keeps its hero: `ULooterSessionSave::HeroId` (new within version 3: a save without it reads as Ellis);
  `USessionSubsystem::StartNewGame(Index, HeroId)`; the hero for a play with no session (`Looter.PlayLevel`) is Ellis
  unless `Looter.Hero <Id>` set it. A function the character asks for its hero's definition.
- The character takes its hero's powers (`UEmberComponent`) and body (`UBodyFigureComponent`: the hero's `FigureMesh`
  and `Retargeter`, Ellis's when they don't load). The holster slot: any material slot ending in `HolsterGun`.
- Flow: an empty slot's New Game opens the hero select; confirm starts that session with the hero; back returns to the
  picker. A session's card names its hero. Heroes not playable show on the roster, their portrait greyed and the line
  "In the works", and can't be confirmed (dev: `Looter.Hero.Unlock 1` lets them, with Ellis's body and no powers).
- Portraits: copy `Saved/ArtPreviews/Portraits/<Hero>/<Hero><key>.png` (Ellis A, Crane A, Pike A, Gauge C, Odessa A)
  to `Art/Portraits/Picks/<Hero>.png` and import them as UI textures (`T_Portrait_<Id>`, no mips, UI group). These are
  paintings, not mockup vector art: textures are right for them.
- Keyboard, mouse and gamepad as the other menus (`FMenuNavigator`); sounds `UI.Hover` and the confirm/equip cues;
  every painted background through `LooterUI::MarkBackground`; colours from `LooterUI::Color` (a hero's own colour is
  data, like an ember's). Leave room where the portrait shows for a live 3D stand later.
- Tests `Looter.Heroes.*`: the five definitions complete (ids unique, words, colours, three powers, portrait loads),
  the hero survives a save and an old save reads as Ellis, only Ellis playable, the screen builds and confirm starts the
  chosen hero. Keep every `Looter.*` test passing.

**Done means:** built, all tests passing, the flow checked in PIE from the main menu with screenshots (`Tools\grab.ps1`)
that you looked at (roster, a locked hero, Ellis confirmed into Ransom's Rest playing as Ellis with Dust Devil on Q), a
Pipeline section and CODEMAP lines written, and the report. Not yours: line variants, the HUD's portrait for the other
heroes, the twelve powers' code.

## Main

Writes this contract, reviews each agent's work against it, imports each model (`Looter.ImportModels`), builds its
retargeter (from `Tools/Unreal/build_ellis_retarget.py`), checks it in PIE, commits each finished piece, keeps the
docs, and asks the user's calls.
