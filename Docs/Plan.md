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
- [ ] Private GitHub remote (the user creates the repository; then push).
- [x] Performance baseline in `Docs/Performance.md`.

## Phase 1: Reorganize (no behavior change)
- [x] Move files into area folders, listed below.
- [x] A C++ game mode, player controller and character replace the template Blueprints' logic. Move/look/jump input goes
      to C++, and the Blueprint becomes data only. The character Blueprint is now `/Game/Player/BP_LooterCharacter`; the
      template game mode, controller, camera manager, touch controls and the empty `BP_WeaponBase` are gone.
- [x] Build without unity, so file-private names can't collide between merged files.
- [x] Retire Build Mode: the editor component, camera pawn, widget, hotkey and its HUD hooks.
- [ ] Merge copied UI helpers into the style kit; split the long files (loadout, HUD minimap, weapon manager, weapon).
- [x] Delete dead code and template leftovers: the rifle locomotion clips, Quinn, touch controls, prototype assets,
      the StateTree plugin and config leftovers.
- [ ] `CODEMAP.md`: one line per file.

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
- [ ] Editor-only module `LooterEditor` with a prop baker: generators become static mesh assets with Nanite, a few
      variants per shape.
- [ ] Blender export and Unreal import scripts with fixed settings (scale, axes, names, collision).
- [ ] Convert Lvl_Skyreach: placed and instanced meshes, plus PCG scatter for grass and flowers. Retire the runtime
      layout, palette and prop code.

## Phase 3: Rendering budget
- [ ] Low, Medium, High and Epic presets in the settings menu. They cover GI (Lumen only on High and Epic),
      shadows, anti-aliasing (TSR on Epic, TAA otherwise), outline quality, clouds and resolution scale.
- [ ] Hardware ray tracing off; decide on Substrate; optimize the outline post material.
- [ ] Verify 60 fps at 1080p Medium on the RX 580 with `Tools/perf.ps1`.

## Phase 4: Creatures, weapons, affixes
- [ ] The spider becomes a rigged skeletal mesh from Blender, with physics-asset hit zones and our procedural leg IK.
- [ ] A weapon parts system: baked or Blender parts, chosen by the roll's seed.
- [ ] Affix design on top of parts and rarity.
