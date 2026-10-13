# Pipeline history (archived 2026-10-13; not loaded by default)

Text moved out of `Docs/Pipeline.md` when the docs were condensed for the AI (the user's call), word for word. What still
applies is kept in `Docs/Pipeline.md`. Read this only when you need the old detail.

## Island styles (Style Lab looks on the tutorial island, 2026-10-08)

Superseded in part: the user ended the style trials on 2026-10-08 ("I don't want to change the style."). Style 3 was
removed from the project, the island wears today's look, and `Docs/Pipeline.md` keeps only the tools that exist and the rule.

- **What:** the user judges a Style Lab look in game on the tutorial island alone; it can be undone and no other level
  changes (the menu shows it, since the menu is the island). Style 3 was tried and rejected; style 6, Sunbleached, is on
  trial. Trials skip tests and screenshots (the user's call); the full checks come when a style goes into the game.
- **Parts of a style:** a static switch named for it on the masters, off by default, plus any post material
  (`build_world_materials.py`; `styled()` chains one style's switch inside another's); its numbers in
  `Tools/Unreal/style_<name>.py` (shared helpers in `style_common.py`); the island's settings in
  `Art/Levels/TutorialIsland/style_<name>.json`.
- **Apply:** `build_world_materials.py M_World M_WorldFoliage M_Terrain M_SkyClouds M_SunbleachedPost`, then
  `build_island_style.py sunbleached apply`: every slot on those masters gets an island-only instance
  (`/Game/Art/Materials/IslandStyle/<Style>/`: placed actors, dressing, sky dome, scatter), light filters, and a
  `<Style>Post` volume (tagged IslandStyle) above `IslandPost`; everything is recorded in `style_applied.json` (commit
  it with the level).
- **One style at a time:** undo the one on before applying another (the script refuses otherwise). After an island
  rebuild or a new scatter, run `apply` again. `undo` restores everything; `status` reports what's styled. To drop a
  style, delete its `IslandStyle/<Style>` folder once the island no longer wears it.
- **Lighting states:** the island's Day must stay the default (`Looter.World.Lighting.Levels`). A style's own sun waits
  in its json's `state` block until that test allows a styled island its own Day (the change is one line in
  `LightingTests.cpp`: skip the default-Day check on a level with an `IslandStyle` actor).
- **Lessons from style 3:** rim terms on alpha-tested leaves speckle unless they read the crown's vertex normal; a
  screen-space edge pass cost the most (+0.8-1.0 ms in all).

## The Style Lab (art styles previewed in the game's scene, without Unreal)

Finished work (2026-10-08). `Docs/Pipeline.md` keeps what the lab is, where its files are and how to export and publish.

`Tools/StyleLab/` (README there) rebuilds a piece of Ransom's Rest in three.js from the game's own sources and lays the
in-game HUD over it, so art styles can be judged "in game" from a cloud session that has no Unreal (2026-10-08: the ten
styles the user chooses from).

- **Catalog:** `Docs/Art/StyleLab/Catalog.md` has all ten styles (the user's picks: 3, 4, 5, 6; 3 is on the tutorial
  island) with each look's numbers in Unreal terms, recipe, cost and refs, plus the contact sheets in
  `Docs/Art/StyleLab/Sheets/`. Briefs and research are beside it.
- **Export** (`Tools/StyleLab/export/export_all.sh`, Blender's `bpy` 4.5 module under Python 3.11): the scripted models
  (GLB packs), the texture sets (webp), the terrain from the area generator, and every placement the editor build makes:
  `Tools/Unreal/build_area.py RansomsRest` runs unchanged under `mock_unreal.py`, with traces hitting the exported
  terrain. PCG scatter goes over as rules plus masks. Output in `Saved/StyleLab/export/`.
- **Engine** (`web/engine/`): sun with soft cascades, sky dome (clouds, stars, planets, moons), height fog, HDR with
  MSAA, a post chain, material builders (`pbr`, `toon`, `flat`, terrain) with a GLSL hook, walk-and-shoot play with
  spiders, and six fixed shots rendered as repeatable stills (`test/shots.mjs`, software WebGL in headless Chromium).
- **Styles** are one module each (`web/styles/NN_<id>.js`): light, sky, fog, materials, post and effect colours, plus a
  card with references, an Unreal recipe and a cost estimate. Style 0 is today's look.
- **HUD** (`web/hud/`): the approved mockup with the in-game differences; its icons and font are inlined (`assets.js`).
- **Publishing:** `assemble.py` builds `Saved/StyleLab/site/`. The artifact host serves no `.glb` or `.bin`, so models and
  heights go as base64 JSON (`--b64-glb`; packs under about 9.5 MB so each JSON stays under 16 MB), and a publish over
  64 MB goes in two calls.

## Crossroads Town: Web Hollow's woods (from "The tutorial island and the older levels")

- Web Hollow sits on the forest rise's middle, where the scatter's woods grew densest, so its clearing took the
  forest: the concept's own woods round it are pieces now (the scatter grows no trees on the island any more).
  (The bug and fix are one line in the live bug list.)

## Performance, Ransom's Rest, 2026-10-08

Moved to `Docs/Performance.md` ("Current reference numbers"); kept there, word for word:

- all 28 views at 149-216 fps (6.7 ms at the heaviest); spawn 6.3 ms;
- base pass 1.1-1.2 ms, prepass 0.5-0.7, shadows at most 0.5;
- no single art asset is a real cost.

## Cut from "The game" (the rules are in `CLAUDE.md`; the story is in `Docs/Story.md`)

- **Story:** *Revenant* (`Docs/Story.md`). A weird-west campaign on the planet Teropa. Ellis Ransom is shot dead at
  Ransom's Point by the gang that stole the saints' embers, wakes in the family plot, and makes a deal with Mister Sexton:
  seven embers at the new moon, and Pa crosses. One area per Hollow; each area has its own design in `Docs/Areas/`.
- **Look:** stylized realism: real lighting and textured materials, still a little stylized. The user rejected faceted
  low-poly. (The live doc keeps the rejection and the palette.)
- **UI:** "Concept C" (`UI/Style/LooterUIStyle.h`): dark glass panels, orange accents, cyan lines, Chakra Petch. The HUD
  has no panels. Icons are Inked icons drawn in code.
- **Feel:** every action should reward the player: tracers, impacts, muzzle flash, spring recoil, hit flash, sound.
- **Selling:** nothing copyrighted. All models are scripted in Blender here, textures are made here, names are original.

## Cut from "Sounds": what each group holds (the recipes and `cues.json` list the cues)

- **Groups added 2026-10-09:** `props` (the lootable world: crates and barrels bursting, a grave dug, a coffin pried, a mailbox,
  a footlocker), `throwables` (the grave-salt grenade: throw, bounce, fuse loop, burst, sear, pickup) and `voices` (the
  Unpaid's murmured barks, one syllable a cue; the spiders' and slimes' idle calls; the living heard through walls on
  Ransom's Rest). The Drover's shots and reload steps are in `guns`, the fast-travel whoosh in `ui`. The cues' names are in
  `LooterSoundCue::Lootables`, `::Throwable`, `::Voice`, `::TownLife` and `::Revolver`; `WeaponSounds.h` picks a gun's
  cues by its kind.
- **Groups added 2026-10-12:** `economy` (silver found, 2D and the player's own: `Loot.Silver.Pickup`; the vendor's counter:
  `Vendor.Buy`, `Vendor.Sell`, `Vendor.NotEnough`; names in `LooterSoundCue::Economy`), `bells` (the Bell-Ringer's bronze hand
  bell: the toll, the cracked toll of a Gravebound ringer, the lift before it, a bullet's clang, the raising at a grave, the
  swipe; `::BellRinger`) and `shrines` (the Vigil Stones: rising, kneeling, the refused call, the iron bell's summons;
  `::Shrine`; the iron bell is `bells.py`'s hand bell made duller, so `shrines.py` imports from it). The Venom Spitter's cues
  (tell, spit, splat, puddle sizzle and burn, hurt, death; `::Spitter`) are in `creatures`.
- **Groups added 2026-10-13** (69 cues): `givers` (the offer card, accept, decline, a new offer nearby, an artifact read),
  `graves` (a grave marked as the save point, marked again), `holds` (the hold ring's completion), `lostloot` (the Lost
  Property window, claim, no room, more shelving, guns sent there), `missions` (optional objectives, mission items, the choice
  card, readables, the porch plate, the UNDERGROUND plate), `orelift` (bells, whistle, winch and cable loops, sheave clanks, the
  cage), `watchfire`, `miners` (the dead miners and Hiram's and Thaddeus's sounds) and `foreman` (his fight).

## Typo fixed in the live doc

- The bug row "`Toolsose.ps1` said "EDITOR DID NOT CLOSE" with no dialog" was `Tools\close.ps1` (a lost `\cl`); the live
  list says `Tools\close.ps1`.
