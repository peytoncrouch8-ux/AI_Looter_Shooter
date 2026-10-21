# Pipeline cleanup plan

From the audit of 2026-09-29 (report: https://claude.ai/artifact/HYnmdXhMQuVJiF7BQivQQb), approved by the user the same day.
Finished phases' checklists and narratives: `Docs/Archive/Plan_history.md`. Where the work stands: `Docs/Handoff.md`.

- Project: `C:\Dev\AI_Looter_Shooter`, Git with LFS, private GitHub remote (github.com/peytoncrouch8-ux/AI_Looter_Shooter) as the
  off-site backup; every commit is pushed.
- The in-game Build Mode is retired; levels are built in the Unreal Editor.
- Models come from Blender, made by hand and by script, plus our generators baked into assets. No game code builds meshes at
  runtime.
- No unity builds (file-private names can't collide).
- Target 120 fps at 1080p, Medium preset, RX 580; Lumen on High and Epic only (`Docs/Performance.md`).

## Phase 0: Foundation

Done: project copied out of OneDrive (the old copy stays until the user deletes it), Git and LFS, clean non-unity build, `Tools/`
scripts in the repo, `CLAUDE.md`, the private GitHub remote, the performance baseline in `Docs/Performance.md`.

## Phase 1: Reorganize (no behavior change)

Done: files moved into the area folders below; a C++ game mode, player controller and character replace the template Blueprints'
logic (move/look/jump input in C++; the character Blueprint is `/Game/Player/BP_LooterCharacter`, data only); Build Mode retired;
copied UI helpers merged into the style kit and the long files split; dead code and template leftovers deleted; `CODEMAP.md` has
one line per file.

Layout of `Source/AI_Looter_Shooter` (organize code by game area):
- `Core/`: game mode, player controller, character (the module file and log category stay at the module root)
- `Player/`: movement (locomotion, stance), camera and view, player animation, input binding
- `Combat/`: health, damage types, critical hits, bullets, impacts, player vitals, target dummy
- `Weapons/`: weapon actor, definitions, rolls, firing, recoil, reload, muzzle flash, models or parts
- `Inventory/`: slots, backpack, ammo pools, equipping and holstering, pickup focus
- `Affixes/`: rarity and roll rules now; affixes and parts later
- `Loot/`: loot tables, drops, pickups, toss physics
- `Creatures/`: mob base and AI, spider, spawning
- `Progression/`: experience, levels and the curve, the player's progress save, level-up rewards later
- `World/`: minimap, fall recovery, level runtime pieces
- `UI/Style`, `UI/HUD`, `UI/Menus`, `UI/Inventory`, `UI/World`: the UI kit and every screen
- `Settings/`: graphics and key-binding settings, quality presets
- `Tests/`: automation tests, one file per area

## Phase 2: Assets and levels

Done:
- Editor-only module `LooterEditor` with a prop baker: generators become static mesh assets with Nanite, a few variants per shape.
  The generator stays in the editor as `StylizedProp`; `Looter.BakeLevelProps` bakes a level's props.
- Blender export and Unreal import scripts with fixed settings (scale, axes, names, collision): `Tools/models.ps1`,
  `Tools/Blender/`, `Looter.ImportModels`; rules in `Art/README.md`. First scripted model: `Art/Models/Props/LanternPost.py`. The
  stylized materials allow Nanite and instancing, so instances share the parents' shaders.
- Lvl_Skyreach converted to placed static meshes (418 actors; load time 5.1 s to 0.9 s). The runtime layout, palette and prop
  code are retired; the minimap reads only actor tags.
- PCG scatter for grass and flowers: the `Meadow` volume runs `/Game/Environment/PCG/PCG_Meadow` (built by
  `Tools/Unreal/build_meadow.py`); 604 instances for about 1 ms more GPU time; every instance scales with `foliage.DensityScale`.

## Phase 3: Rendering budget

Done: Low, Medium, High and Epic presets in the settings menu; rules and measured costs are in `Docs/Performance.md`. Hardware
ray tracing off, Substrate off, the outline needs no optimizing.

## Phase 4: Creatures, weapons, affixes

Done:
- The spider is a rigged skeletal mesh: `SK_Spider` from `Art/Models/Creatures/Spider.py`, 31 bones and a convex hit zone around
  every part (`PA_Spider`). `ASpiderCreature` keeps its gait (two groups take turns) and procedural leg IK, posing the bones
  through `USpiderAnimInstance`; crits are by bone. Rigged models go through the Blender pipeline like any other
  (`Art/README.md`).
- Weapon parts system: Blender part meshes (`Art/Models/Weapons`); each definition lists part slots and paints, `WeaponParts::Pick`
  chooses by the roll's seed (rarity only adds rarity parts), `UWeaponModelComponent` assembles, paints and animates them.
- No game code builds meshes at runtime: ammo boxes are baked too, the mesh kit moved to the editor module. FBX imports always use
  the classic importer (Interchange re-imported models with its own settings).
- Affix design (the user chose Borderlands-style): parts carry the stats (each trades one strength for another) and name the gun
  ("Epic Scoped Assault Rifle"); rarity unlocks the better parts; stats only for now (elements later); legendaries are the top
  tier of the same system.

## Phase 5: Tutorial island in the new style

The user chose textured "stylized realism" (`Docs/Art/StyleTarget_Outpost.png`) for the whole game. Skyreach became the tutorial
island, a grassy meadow in that style. `Docs/TutorialIsland.md` holds the art bible, asset list, island layout and performance
plan. Done: textured-model pipeline (master materials in `Tools/Unreal/build_world_materials.py`, `Looter.Editor.TexturedImport`);
texture library and house trim sheet (`Art/Textures`, `Tools/Blender/looter_textures.py`, 18 sets); buildings, vegetation, rocks,
cliffs, props (`Art/Models/Buildings`, `Vegetation`, `Rocks`, `Props`; shared Blender code `Tools/Blender/looter_buildings.py`,
`looter_plants.py`, `looter_props.py`); the island terrain from `Art/Levels/TutorialIsland/layout.json`; the level placed by
`build_tutorial_island.py` and scattered by `build_island_scatter.py`; Lvl_TutorialIsland is the game's map. Round 2 (waterfall,
chimney smoke, reeds and pond plants, meadow trees, gun rack's weapon, tutorial) was built 2026-10-01; the tutorial has since moved
into Ransom's Rest's opening.

## Phases 6-11: from the cloud session

These phases came from the user's cloud session of 2026-10-02 to 2026-10-05 (handoff: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`,
which has the full spec of 7-11; its files were brought in on 2026-10-07). Phase 6 was renewed on 2026-10-08, Phase 8 is built;
the others are not started.

## Phase 6: The art style

Closed. The user asked on 2026-10-08 for ten very different art styles previewed in the game's scene (the Style Lab,
`Tools/StyleLab/`; <https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr>; catalog with Unreal recipes: `Docs/Art/StyleLab/Catalog.md`),
and picked 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp and 6 Sunbleached (all ten kept in the catalog). Trials on the tutorial
island only, reversible:
- Style 3 (`build_island_painted.py apply|undo`): the user: "I dont like style 3, lets try the sunbleached style instead". Undone
  and removed from the project (recipe stays in the catalog).
- Style 6 Sunbleached (`build_island_style.py`): tried, undone; kept as the tool for any later style.
- Style 4 Inkslinger: asked for, then stopped before any code. The user: "I don't want to change the style." The island is on
  today's look; the style trials are over.
- Only if the user asks again: another style through `build_island_style.py`; then its spec and transition plan in `Docs/Art/`,
  built as post-process, materials, light and sky, every asset in the new style, tour and performance recorded.

## Phase 7: Crossroads Town

The user found the tutorial island bland and lonely and, from four 3D layout concepts (`Docs/TutorialIslandConcepts.md`), chose
**Crossroads Town** on 2026-10-03: the crossroads grows into a small town round a cobbled square, the slimes get a bog and the
spiders a webbed hollow, the roads get brick kerbs, lamps and cover, with far more trees, hedges, flowers and life. Every placement
is in `Art/Levels/TutorialIsland/crossroads_town.json`; the plan and the concept's mistakes to fix are in the cloud handoff.
- [x] Round 1 (2026-10-08): the ten buildings, the farm and range roads, the Wallow's pools, 680 instanced props, fences, trees,
      hedges and Web Hollow's webs (`make_town.py` -> `town.json`); six spiders in Web Hollow and five slimes in the Wallow that
      never respawn (the user's call); the scatter and five new tour views; Medium 160-226 fps at every view (heaviest, above the
      town, 6.3 ms). Kerbs and the cobbled square wait for their models (round 2).
- [ ] New models in the chosen art style: stalls, sheds, coop, haystacks, scarecrows, picket fences, hedges, barricades, sandbags,
      targets, the dock, garden crops, kerbs and cobbles, and the creature grounds' dressing.
- [ ] Sheep, hens and townsfolk: models, actors and bestiary pages.
- [ ] Tour and performance recorded at every viewpoint.

## Phase 8: HUD upgrade

Done 2026-10-07 (731e0df, 5b5b36a); full spec in the cloud handoff. The user approved the mockup on 2026-10-03
(<https://claude.ai/artifact/MjHmSiZunHXm2MqxCqhmJ1>, sources `Docs/HudMockup/`) after four rounds of changes: a smaller ammo
cartridge holding the ammo counts, upright beside the weapon slots (now a column), bigger gun icons, a smaller player frame with no
name, the tutorial prompt moved off the top of the screen. Built:
- the player frame and portrait (`Art/Icons/HudPortrait.svg` as vector data; blinks, flinches, squints at low health, flares on a
  level-up) with the health bar and number, level gem and experience bar;
- the level-up banner; the experience bar's logic moved into the frame;
- the mission tracker in place of the tutorial prompt (ranger's star, mission name over a bar of its steps, one short objective, a
  key hint);
- the weapon slots as a column with the upright cartridge; the minimap and boss bar in the same metalwork.
Measured on Medium, tests passing, `CLAUDE.md`'s UI rules updated.

## Phase 9: Gun ideas

On 2026-10-04 the user picked three ideas from a brainstorm (full spec in the cloud handoff). Built 2026-10-08 (4f6abb1):
- Notches: each gun counts its kills (`FWeaponInstanceData`), cut into its stock; wakes at 50, 250 and 1,000 with small bonuses and
  a nickname.
- Cursed irons: 6% of Rare-or-better drops; a strong perk with a real drawback; lifted at 100 notches or with Tilly's grave salt
  (the salt waits for Tilly's shop and grave gold).
- Part swapping at Ozias's bench: scrap a gun to keep one part, fit parts onto guns of the same kind. The user's call: a plain
  gunsmith's bench now, in Skyreach's village and at Ransom Farm; Ozias takes it over after the *Lily*.
- Tests for all three, `CODEMAP.md` updated.
- [ ] The user plays them; numbers tuned (notch bonuses, curse rates and effects).

## Phase 10: Ember powers

On 2026-10-04 the user played a demo of six ember powers (<https://claude.ai/artifact/MeXAwqdJdX4SYGDzhEy4xR>, sources in
`Docs/EmberDemo/`) and kept four, for the main session to revise: Lucky Streak (ricochets and critical hits), Dust Devil (a dash
that flings creatures aside), Slag Bomb (a burning pool), Spyglass (marks creatures through walls). With the heroes (Phase 11) they
became four of the heroes' fifteen powers; the outlaws' embers no longer give powers. Nothing in the code has changed yet.
- [ ] The ember component, definitions, keys and saving.
- [ ] The creatures' knocked, dizzy, burning and marked states.
- [ ] The fifteen powers and their effects, in the chosen art style, the four from the demo first.
- [ ] The HUD's ember sockets and the inventory's ember page.
- [ ] Tests, measured on Medium, `CODEMAP.md` updated.

## Phase 11: Playable heroes

On 2026-10-05 the user approved five playable heroes to choose from, as in Borderlands, each with three ember powers
(<https://claude.ai/artifact/5NVdW7yTzReFkP3wvaxbyg>, sources in `Docs/HeroSelect/`): Ellis Ransom the Revenant, Odessa Lark the
Cardsharp, Hollis Crane the Unpaid, Gauge the Iron Hand and Wendell Pike the Surveyor. Their models are Blender scripts in
`Art/Backlog/Characters/`, rigged on the mannequin's skeleton. Nothing in the game has changed yet.
- [ ] The five brought in and retargeted from the mannequin, materials merged, checked in first and third person.
- [ ] Hero definitions, the hero select screen, and the session's hero and its saving.
- [ ] Four new portraits for the player frame.
- [ ] `Docs/Story.md` updated for the five; tests; `CODEMAP.md` updated.

## Phase 12: Core loop (the freeze, 2026-10-21)

The user's call (Handoff "Direction"): nothing new until Ransom's Rest's core is fun. Order (Main's, the user agreed):
gunplay, enemies, loot, pacing and story; art alongside. Ends when the user and their friend both want to replay RR with
another hero.

### 12.1 Gunplay (plan written 2026-10-21, waiting on the user's OK)

The user's picks: enemy hit reactions, gun sound and kick, guns feel alike (hit markers/numbers are fine).
What's there now (why it feels weak):
- `FHitReaction`: only a crit, a kill or a burst worth 20% of health inside 80 ms does anything (crit hit-stop 45 ms, kill
  65/80; stagger 0.32 s Basic, 1.1 s cooldown). A plain body hit gets the 0.09 s hit flash and nothing else, so most
  rifle and revolver hits show no reaction.
- One `Weapon.<Kind>.Fire` cue per gun, played attached: no layers (mechanism, punch, low end, tail), no tail by
  surroundings, same sound near and far, nothing for the last rounds in the magazine.
- Recoil (`FWeaponRecoilProfile`) and view kicks (`ViewKicks::ForShot`) differ per kind but are small (rifle pitch
  0.32 deg). No controller rumble anywhere.
- The three guns differ in numbers but play the same way: aim, click, the target flashes.

Steps (each tuned live by `Looter.Gunfeel.*` cvars so the user tunes by feel in PIE, then the numbers go into code):
0. **Baseline:** package a test build of today's game (Pipeline "Packaging a test build") for the user's friend, before
   anything changes; their notes are the yardstick.
1. **Every hit reacts** (Combat/HitReaction, Creatures/*Pose/*React):
   - A flinch on every hit: 60-150 ms, size by the hit's share of health, away from the shot, at the hit spot: a
     spider's struck leg or body jerks, the slime dents and wobbles there, the Unpaid jolts and its coal flares, a miner
     twists. Procedural, on each kind's existing pose code. Bosses flinch smaller (never interrupt their moves).
   - Staggers come more often: heavy share 20% -> ~12%, cooldown 1.1 -> ~0.7 s, Basic 0.32 -> ~0.4 s (to tune).
   - Kills by gun: a shotgun kill throws the body (bigger corpse knock), a revolver crit kill bursts the crit spot, a
     rifle kill crumples. Bigger creature-hit spurts (blood, gel, ash) and an exit spray behind the target.
2. **Gun sound and kick** (Art/Sounds, Weapons/WeaponBaseFiring, ViewKick, WeaponRecoil):
   - Sound: a licensed royalty-free pack (asked before downloading: which pack, its size and licence). Each shot in
     layers: mechanism (near, first person), punch, low thump, a tail chosen by surroundings (open valley, town between
     walls, inside the mine), a distant version for shots far away; a rising click in the last quarter of the magazine.
     Processed in the script pipeline; licence file kept with the pack.
   - Kick: stronger per kind (shotgun shove and FOV punch, revolver snap, rifle climb you pull down), first-person gun
     motion bigger, muzzle flash bigger in first person.
   - Controller rumble per shot and per hit taken (new; scaled by a setting next to camera shake).
3. **Each gun its own job:**
   - Bullpup rifle, pressure: sustained hits build a stagger (e.g. 5 hits within 1 s), recoil climbs over a burst, spread
     blooms on the hip.
   - Ranchhand shotgun, power: close blasts shove and can knock small creatures back or down (`UCreatureKnockComponent`),
     sharp falloff past its range, the pump's rhythm heard and felt.
   - Drover revolver, precision: its own crit multiplier above the 1.5x rule (needs the user's OK; the per-gun
     multiplier already exists in `LooterCombat::HitDamage`), longer crit hit-stop, a ring on a crit.
   - Handling: aim speed, move speed while aiming, swap speed per kind.
4. **Tune with the user:** they play with the cvars, Main bakes the numbers; then the friend plays a new build.

Checks: tests for the new rules (`Looter.Gunfeel.*`), all tests pass; perf at RR spawn with 8 spiders on Medium before
and after (target 8.3 ms); no capture runs (the user's 2026-10-20 call).
Agents: GUNFEEL-REACT (Opus: Combat, Creatures), GUNFEEL-KICK (Sonnet: Weapons recoil/kick, rumble, Settings),
GUNFEEL-SOUND (Opus: Art/Sounds, after the pack is approved); one shared contract (cvar names, events) from Main first.
Open questions for the user, asked at their step: the sound pack; the revolver's crit multiplier.
