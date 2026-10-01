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
