# Next builds contract (Main, 2026-10-22 noon)

The user's picks (2026-10-22): a **player shield**, **gun manufacturers**, a **bounty board**, **playtest builds** (all four),
plus Main's in-scope work from the user's earlier asks: **per-place ground detail** (the world reads samey up close) and
small leftovers. Read CLAUDE.md (rules: C++ first, LooterUI kit for every UI, colours only from LooterUI::Color, sounds
script-made, no copyrighted names, CODEMAP, Docs/Work rule 12, 3-sentence reports to the user), Docs/Handoff.md (state),
Docs/Pipeline/ (the topic file for your area), CODEMAP.md.

## Rules for every agent
- Phase 1: write C++ into a holding tree `Saved/<Job>Code/Source/AI_Looter_Shooter/...` (copy each existing file first;
  `HASHES.txt` with each original's git blob hash); Python/CSV/Art/recipes may be edited in place if no other job in this
  file owns them. No build, no editor, no commits. Report when ready; Main copies the tree (the user approved Main doing
  the copies) and gives you the editor lane for phase 2 (build, scripts, tests, PIE with single `Tools\grab.ps1` shots,
  never Python capture batches).
- Every feel number a cvar (`Looter.<Area>.*`), plain rules with tests (`Looter.<Area>.*`), compile-clean, files under
  ~500 lines, comments say why. Perf: Medium 8.3 ms at RR's heaviest views; no new lights on Medium.
- Names: original, in the Revenant world (planet Teropa, the Reaches, western-supernatural, frontier-tech guns).
- Each job owns only its files below; anything else goes in your report for Main.

## SHIELD (Opus): the player's shield
A recharging shield over health (Borderlands' core loop). Owns: new `Shields/` folder (definition data asset, instance
data with rarity and rolled stats, the player's shield component, rules, tuning), `Combat/HealthComponent.*` (route
damage through the existing `FHealthShield` hook the creature wards use: shield first, then health), `Combat/PlayerVitals*`
(recharge delay/rate interplay with health regen: health regen stays as tuned for 12.2, the shield recharges first),
`UI/HUD/` player frame (a slim slanted shield bar over the health bar in the kit's cyan, break flash, recharge shimmer;
`Tools\hudshots.ps1` rules), `UI/Inventory/Loadout*` (a shield slot with a card), `Inventory/` save of the equipped and
backpack shields, `Loot/` (shields drop like guns: weapon-drop share split, rarity order), the art of a shield item (a
belt/hip "ward box" device, Blender `Art/Models/Shields/WardBox.py`, parts or paints by rarity, Inked icon via
`Art/Icons/InkedIcons.py`), sounds (`Shield.Break`, `.RechargeStart`, `.Full`, `.Hit`: script-made recipes), tests.
Stats: capacity, recharge delay, recharge rate; traits at Rare+ (e.g. a nova when it breaks dealing an element, spikes
that hurt melee attackers, absorbing a share of bullets as ammo, faster recharge after a kill): pick 4-6, original names.
Creature damage x1.5 (12.2) assumed a health-only player: retune so a big fight still drains most of shield+health
(note your numbers).

## MANUFACTURERS (Opus): gun makers
3-4 in-world makers with a handling identity and a look (original names and a one-line fiction each, fitting Teropa:
e.g. a frontier railroad works, a saint-blessed mission armory, a salvage outfit, a precision instrument maker). Each
gun rolls a maker (by rarity weights); the maker adds a handling quirk (e.g. reload/handling vs damage, heat for power,
accuracy for fire rate, elements more often) and a look (paint scheme and decals on the existing parts through material
params, a maker mark on the body, the name prefix). Owns: `Affixes/` (rolls, maker table), `Weapons/WeaponParts*`,
`Weapons/WeaponTypes.h` (instance field, saved by name), `UI/Style/WeaponText*` + labels/cards (maker name and mark),
`Tools/Unreal/setup_gun_parts.py` and a new `Tools/Unreal/create_manufacturers.py`, the material params on the gun
master/MIs (no model changes), Inked maker marks, tests. **Before code**: make a one-page concept sheet (HTML/SVG ->
PNG with node + playwright, as tonight's gun concepts) of the 3-4 makers (name, fiction line, quirk, paint and mark on a
Bullpup and a Drover), save `Saved/ArtPreviews/Weapons/Concepts/Makers.png`, and report it so Main can show the user.
Old saved guns get a default maker.

## BOUNTY (Opus): the bounty board
A board in Ransom's Rest town (by the sheriff's office or the notice board: find the place in Docs/Areas/RansomsRest.md)
offering a rotating hard fight with a chest, using the set-piece machinery (`ASetPieceSpawner`, pushes, beats, loot
shower): 3 bounties at a time drawn from a pool (target creature kind and rank/tier, a twist like Fleet or Warded
traits, a place in RR from a list of arenas), refreshed on a real-time timer (e.g. every few hours played or each day
of the session) and after completion; the reward a Strongbox-luck chest plus silver and XP by level. A board UI in the
LooterUI kit (Concept C) with the three cards, accept, track on the mission tracker and minimap. Owns: new `Bounties/`
folder, the board actor and widget, `Tools/Unreal/build_area_bounties.py` (placement, hooked in build_area.py like the
secrets), save fields, sounds via cue names (`Bounty.*`, script-made), tests. Pacing: bounty XP must not break
`Looter.Progression.Pacing` (optional content: give it the side-content share).

## PLAYTEST (Sonnet): packaged builds
A script `Tools\package.ps1` that packages a playable Development build of the game for Windows (Docs/Pipeline "Packaging
a test build" and Docs/Release.md: cook the directories listed in DefaultGame.ini; the game starts at the menu) into
`Builds\Playtest\<date>\` (git-ignored), plus a `Play Latest Build.bat` at the repo root that starts the newest one.
Check the packaged game starts, reaches the menu and loads Ransom's Rest (log only; a single grab of the window if
possible). Runs last in the lane (it needs the editor closed and lots of CPU). Report size and time.

## GROUND-DETAIL (Opus): places that read up close
The bolder macro tones read only from above (`Saved/World/compare_tones.png`); up close roads, boardwalks and the
ground's detail textures dominate. Give each place its own ground at eye level: per-place detail layers in M_Terrain
(town red-ochre dust and boot ruts, the Sink's ash and cinders, the quarry's pale grit and chips, the mine's coal dust,
the chapel's green with yarrow), path decals/edges where roads meet them, and clutter. Owns: `Tools/Unreal/build_world_
materials.py` (M_Terrain only), `Tools/Blender/looter_textures.py` (new sets), `Art/Levels/RansomsRest/` (layout
macro/detail blocks), `Tools/Unreal/build_area_dressing.py` and `build_island_scatter.py`, the ground preview tool.
Phase 2 imports and checks at eye level with before/after grabs per place; perf before/after.

## SMALL-FIXES (Sonnet)
(1) Gauge's revolver aim splays his arms over his pauldrons: per-hero aim values (`Looter.Pose.*`, the FHeroAim table in
`Tools/Unreal/create_ember_assets.py`); (2) the Rustler's orange round-count glow drowns its Soul-light part (Saint40):
the helix glow takes the part's element colour when an element part is fitted (code: the GunAmmoGlow param) or the art
tones it; (3) the Ranchhand's element tube parts read small in first person (the pump covers them): make the element
dress show on the pump/receiver side seen from first person (art). Owns those files only.

## PERF (Sonnet, first)
A quiet re-measure: create `Saved\ArtPause.flag` (Blender jobs wait on it), close the editor, run `Tools\perf.ps1` at RR
spawn (8 spiders in town, 8 Unpaid in town, Slag Bomb repeat) and `Tools\tour.ps1` for every view on Medium; compare to
Docs/Performance.md rows from 2026-10-21/22; list any view over 8.3 ms with its likely cause; delete the flag; reopen the
editor. Report under 150 words.
