# Pipeline cleanup plan

This plan comes from the audit of 2026-09-29 (report: https://claude.ai/artifact/HYnmdXhMQuVJiF7BQivQQb). The user
approved it the same day.
- The project lives in `C:\Dev\AI_Looter_Shooter` under Git with LFS, with a private GitHub remote as the off-site backup.
- The in-game Build Mode is retired; levels are built in the Unreal Editor.
- Models come from Blender, made both by hand and by script, plus our generators baked into assets.
- Minimum spec: 60 fps at 1080p, Medium preset, RX 580.
- Lumen is used on the High and Epic presets only.
- The work is done by one agent, in order.

## Phase 0: Foundation
- [x] Copy the project out of OneDrive to `C:\Dev\AI_Looter_Shooter`. The old copy stays until the user deletes it.
- [x] Git and LFS, with an ignore list, and a snapshot commit of the untouched project.
- [x] Make the clean build work. Unity builds had hidden duplicate private names.
- [x] `Tools/` scripts in the repo; `CLAUDE.md`.
- [x] Private GitHub remote (github.com/peytoncrouch8-ux/AI_Looter_Shooter); every commit is pushed.
- [x] Performance baseline in `Docs/Performance.md`.

## Phase 1: Reorganize (no behavior change)
- [x] Move files into area folders, listed below.
- [x] A C++ game mode, player controller and character replace the template Blueprints' logic. Move/look/jump input goes
      to C++, and the Blueprint becomes data only. The character Blueprint is now `/Game/Player/BP_LooterCharacter`; the
      template game mode, controller, camera manager, touch controls and the empty `BP_WeaponBase` are gone.
- [x] Build without unity, so file-private names can't collide between merged files.
- [x] Retire Build Mode: the editor component, camera pawn, widget, hotkey and its HUD hooks.
- [x] Merge copied UI helpers into the style kit; split the long files (loadout, HUD minimap, weapon manager, weapon).
- [x] Delete dead code and template leftovers: the rifle locomotion clips, Quinn, touch controls, prototype assets,
      the StateTree plugin and config leftovers.
- [x] `CODEMAP.md`: one line per file.

Target layout of `Source/AI_Looter_Shooter`:

| Folder | Owns |
|---|---|
| `Core/` | Game mode, player controller, character (the module file and log category stay at the module root) |
| `Player/` | Movement (locomotion, stance), camera and view, player animation, input binding |
| `Combat/` | Health, damage types, critical hits, bullets, impacts, player vitals, target dummy |
| `Weapons/` | Weapon actor, definitions, rolls, firing, recoil, reload, muzzle flash, models or parts |
| `Inventory/` | Slots, backpack, ammo pools, equipping and holstering, pickup focus |
| `Affixes/` | Rarity and roll rules now; affixes and parts later |
| `Loot/` | Loot tables, drops, pickups, toss physics |
| `Creatures/` | Mob base and AI, spider, spawning |
| `Progression/` | Experience, levels and the curve, the player's progress save, level-up rewards later |
| `World/` | Minimap, fall recovery, level runtime pieces |
| `UI/Style`, `UI/HUD`, `UI/Menus`, `UI/Inventory`, `UI/World` | The UI kit and every screen |
| `Settings/` | Graphics and key-binding settings, quality presets |
| `Tests/` | Automation tests, one file per area |

## Phase 2: Assets and levels
- [x] Editor-only module `LooterEditor` with a prop baker: generators become static mesh assets with Nanite, a few
      variants per shape. The generator stays in the editor as `StylizedProp`, and `Looter.BakeLevelProps` bakes a
      level's props.
- [x] Blender export and Unreal import scripts with fixed settings (scale, axes, names, collision): `Tools/models.ps1`,
      `Tools/Blender/`, `Looter.ImportModels`; rules in `Art/README.md`. The first scripted model is
      `Art/Models/Props/LanternPost.py`. The stylized materials now allow Nanite and instancing, so their instances share
      the parents' shaders.
- [x] Convert Lvl_Skyreach to placed static meshes (418 actors). Load time fell from 5.1 s to 0.9 s. Retire the runtime
      layout, palette and prop code; the minimap reads only actor tags.
- [x] PCG scatter for grass and flowers (instanced, in place of the placed patches). The `Meadow` volume runs
      `/Game/Environment/PCG/PCG_Meadow` (built by `Tools/Unreal/build_meadow.py`). It covers the island with 604
      instances (the 199 placed patches are gone) for about 1 ms more GPU time; every instance scales with
      `foliage.DensityScale`, for the presets.

## Phase 3: Rendering budget
- [x] Low, Medium, High and Epic presets in the settings menu. They cover GI (Lumen only on High and Epic),
      shadows, anti-aliasing (TSR on Epic, TAA otherwise), outline quality, clouds and resolution scale.
      Each preset is the engine's scalability level plus Lumen, the anti-aliasing method and Nanite (off below
      High; it costs the RX 580 about 2.5 ms). All render at full resolution; the engine's own Medium renders at
      71%. The outline (0.18 ms) and the clouds (0.4 ms) are cheap enough to keep on every preset. A fresh install
      starts on Medium.
- [x] Hardware ray tracing off; decide on Substrate; optimize the outline post material. Substrate is off: the
      same frame time, and fewer shaders to compile. The outline needs no optimizing.
- [x] Verify 60 fps at 1080p Medium on the RX 580 with `Tools/perf.ps1`: 12.1-12.5 ms (80-82 fps). Low runs 7.4 ms,
      High 22.0 and Epic 38.1.

## Phase 4: Creatures, weapons, affixes
- [x] The spider becomes a rigged skeletal mesh from Blender, with physics-asset hit zones and our procedural leg IK.
      `SK_Spider` comes from `Art/Models/Creatures/Spider.py` (a port of the old code-built body): 31 bones and a convex
      hit zone around every part (`PA_Spider`). `ASpiderCreature` keeps its gait and IK and poses the bones through
      `USpiderAnimInstance`; crits are by bone. The gait's two groups now take turns (one used to starve the other).
      Rigged models go through the Blender pipeline like any other (`Art/README.md`). Medium: 11.9 ms, was 12.5; 487
      draw calls, was 1729.
- [x] A weapon parts system: baked or Blender parts, chosen by the roll's seed. Guns are Blender part meshes
      (`Art/Models/Weapons`); each definition lists part slots and paints, `WeaponParts::Pick` chooses by the seed (rarity
      only adds rarity parts) and `UWeaponModelComponent` assembles, paints and animates them.
- [x] No game code builds meshes at runtime: the ammo boxes are baked too, and the mesh kit moved to the editor module,
      which still builds the level props with it. FBX imports always use the classic importer (Interchange re-imported
      models with its own settings).
- [x] Affix design on top of parts and rarity. The user chose Borderlands-style: parts carry the stats (each trades one
      strength for another) and name the gun ("Epic Scoped Assault Rifle"), rarity unlocks the better parts, stats only
      for now (elements later), and legendaries are the top tier of the same system.

## Phase 5: Tutorial island in the new style
The user chose textured "stylized realism" (`Docs/Art/StyleTarget_Outpost.png`) for the whole game. Skyreach becomes
the tutorial island, a grassy meadow rebuilt in that style; other areas come later. `Docs/TutorialIsland.md` holds the
art bible, the asset list, the island layout and the performance plan. The target is 120 fps at 1080p Medium.
- [x] The pipeline takes textured models: master materials (`Tools/Unreal/build_world_materials.py`), texture sets
      imported by suffix, LODs and Nanite fallback shares per model, models without collision
      (`Looter.Editor.TexturedImport`).
- [x] Texture library and the house trim sheet (`Art/Textures`, `Tools/Blender/looter_textures.py`): 18 sets.
- [x] Buildings, vegetation, rocks, cliffs and props (`Art/Models/Buildings`, `Vegetation`, `Rocks`, `Props`): 10
      buildings and the windmill's fan, 23 plants, 12 rocks and cliff pieces, 20 props. Shared Blender code in
      `Tools/Blender/looter_buildings.py`, `looter_plants.py`, `looter_props.py`.
- [x] The island terrain from `Art/Levels/TutorialIsland/layout.json`: 16 tiles (133k triangles), the underside, roads
      painted and carved, pond and creek, the macro color map and the scatter mask.
- [x] Place the level from the layout (`build_tutorial_island.py`), the PCG scatter (`build_island_scatter.py`: 25k
      grass patches, 1.5k flower drifts, trees, bushes, rocks), afternoon light, painted sky-dome clouds, no outlines.
      Lvl_TutorialIsland is the game's map.
- [x] Medium at 8.3 ms or less from every viewpoint: 5.5 to 7.2 ms (139 to 182 fps) on `Tools/tour.ps1`. Volumetric
      clouds were the big cost (9.5 ms with the engine's layer, 2 ms thinned); the dome costs almost nothing.
- [ ] Next: a waterfall and chimney smoke, reeds at the pond, more trees in the meadows, the gun rack's weapon, and the
      tutorial itself (prompts along the road).

## Phases 6-11: from the cloud session
These phases came from the user's cloud session of 2026-10-02 to 2026-10-05 (its handoff is
`Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`; its files were brought in on 2026-10-07). Phase 6 was renewed on
2026-10-08, and Phase 8 is built; the others are not started.

## Phase 6: The art style
The user finds the current look (stylized realism) and the earlier style explorations short of what the game needs,
and on 2026-10-08 asked for ten very different art styles, previewed in the game's own scene under the new HUD, to
choose from: the Style Lab (`Tools/StyleLab/`). The chosen style then goes onto every asset.
- [x] The ten styles built in the Style Lab (2026-10-08): <https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr>.
- [x] The user's favourites: 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp and 6 Sunbleached (2026-10-08). All ten
      are kept in the art catalog, since the look may change later.
- [x] The ten catalogued with their Unreal recipes: `Docs/Art/StyleLab/Catalog.md` (2026-10-08).
- [x] Style 3, Painted Frontier, built on the tutorial island only (2026-10-08, `build_island_painted.py apply|undo`):
      first pass, measured (heaviest view 7.5 ms on Medium, was 6.7) and toured.
- [x] The user's verdict (2026-10-08): "I dont like style 3, lets try the sunbleached style instead". Style 3 undone
      and removed from the project (its recipe stays in the catalog).
- [ ] Style 6, Sunbleached, on the tutorial island only, reversible; the user judges it.
- [ ] Its spec and transition plan written down (`Docs/Art/`), then built: post-process, materials, light and sky.
- [ ] Every asset in the new style; tour and performance recorded.

## Phase 7: Crossroads Town
The user found the tutorial island bland and lonely and, from four 3D layout concepts
(`Docs/TutorialIslandConcepts.md`), chose **Crossroads Town** on 2026-10-03. The crossroads grows into a small town
round a cobbled square, the slimes get a bog and the spiders a webbed hollow, and the roads get brick kerbs, lamps and
cover, with far more trees, hedges, flowers and life. Every placement is in
`Art/Levels/TutorialIsland/crossroads_town.json`; `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md` has the plan and the concept's mistakes to fix. Nothing
in the level has changed yet.
- [ ] The town with the models that exist: placements, kerbed roads and the cobbled square, the creature groups, the
      scatter and new tour views; measured on Medium.
- [ ] New models in the chosen art style: stalls, sheds, coop, haystacks, scarecrows, picket fences, hedges, barricades,
      sandbags, targets, the dock, garden crops, kerbs and cobbles, and the creature grounds' dressing.
- [ ] Sheep, hens and townsfolk: models, actors and bestiary pages.
- [ ] Tour and performance recorded at every viewpoint.

## Phase 8: HUD upgrade
The user found the gameplay HUD simple and bland and asked for a more personable one that wows, after an RPG unit
frame as the reference. On 2026-10-03 they approved the mockup (<https://claude.ai/artifact/MjHmSiZunHXm2MqxCqhmJ1>,
sources in `Docs/HudMockup/`) after four rounds of changes: a smaller ammo cartridge that holds the ammo counts and
stands upright beside the weapon slots, now a column, bigger gun icons, a smaller player frame with no name, and the
tutorial prompt moved off the top of the screen. A player frame replaces the health ring and the bottom experience
bar: an inked portrait of the player character in a gunmetal diamond that blinks, flinches, squints at low health and
flares on a level-up, with a thick health bar with its number, a level gem and the experience bar. A mission tracker
on the left replaces the tutorial prompt: a ranger's star, the tracked mission's name over a bar of its steps, one
short objective and a key hint. The weapons, minimap and boss bar get the same metalwork, and a banner announces each
level. `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md` has the full spec. Built on 2026-10-07 (731e0df, 5b5b36a).
- [x] The player frame and the portrait (`Art/Icons/HudPortrait.svg` as vector data), with its reactions.
- [x] The level-up banner; the experience bar's logic moved into the frame.
- [x] The mission tracker in place of the tutorial prompt, with the tutorial's short lines and key hints.
- [x] The weapon slots as a column with the upright cartridge, then the minimap and boss bar, in the same metalwork.
- [x] Measured on Medium, tests passing, `CLAUDE.md`'s UI rules updated.

## Phase 9: Gun ideas
On 2026-10-04 the user picked three ideas for the loot from a brainstorm: notches (each gun counts its kills, cut into
its stock, and wakes at 50, 250 and 1,000 with small bonuses and a nickname), part swapping at Ozias's bench (scrap a
gun to keep one part, fit parts onto guns of the same kind) and cursed irons (6% of Rare-or-better drops: a strong perk
with a real drawback, lifted at 100 notches or with Tilly's grave salt). `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md` has the full spec. Nothing in the
code has changed yet.
- [ ] Notches: the kill count on `FWeaponInstanceData`, the tally on the stock, the milestones and their messages.
- [ ] Cursed irons: the curse table, the roll, the effects, how they show, and lifting.
- [ ] Part swapping: the bench, scrapping into the parts box, fitting by the rules, the bench screen.
- [ ] Tests for all three, `CODEMAP.md` updated.

## Phase 10: Ember powers
On 2026-10-04 the user played a demo of six ember powers (<https://claude.ai/artifact/MeXAwqdJdX4SYGDzhEy4xR>, sources
in `Docs/EmberDemo/`). They kept four, for the main session to revise:
- Lucky Streak: ricochets and critical hits.
- Dust Devil: a dash that flings creatures aside.
- Slag Bomb: a burning pool.
- Spyglass: marks creatures through walls.

With the heroes (Phase 11) they became four of the heroes' fifteen powers, and the outlaws' embers no longer give
powers. `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md` has the full spec. Nothing in the code has changed yet.
- [ ] The ember component, definitions, keys and saving.
- [ ] The creatures' knocked, dizzy, burning and marked states.
- [ ] The fifteen powers and their effects, in the chosen art style, the four from the demo first.
- [ ] The HUD's ember sockets and the inventory's ember page.
- [ ] Tests, measured on Medium, `CODEMAP.md` updated.

## Phase 11: Playable heroes
On 2026-10-05 the user approved five playable heroes to choose from, as in Borderlands, each with three ember powers
(<https://claude.ai/artifact/5NVdW7yTzReFkP3wvaxbyg>, sources in `Docs/HeroSelect/`): Ellis Ransom the Revenant,
Odessa Lark the Cardsharp, Hollis Crane the Unpaid, Gauge the Iron Hand and Wendell Pike the Surveyor. Their models are
Blender scripts in `Art/Backlog/Characters/`, rigged on the mannequin's skeleton. `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md` has the full spec.
Nothing in the game has changed yet.
- [ ] The five brought in and retargeted from the mannequin, materials merged, checked in first and third person.
- [ ] Hero definitions, the hero select screen, and the session's hero and its saving.
- [ ] Four new portraits for the player frame.
- [ ] `Docs/Story.md` updated for the five; tests; `CODEMAP.md` updated.
