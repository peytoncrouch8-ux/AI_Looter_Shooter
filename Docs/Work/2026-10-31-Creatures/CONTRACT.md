# Overnight creature jobs (2026-10-31, Main's contract)

## The user's words and answers (bedtime, 2026-10-31)
- "The spider models didn't import right and neither did the Unpaid models, their animations need improved upon as well. Their
  AI is lackluster."
  - What looked wrong (the user's picks): **textures/colours** AND **rig/deforming**.
  - What AI should improve most (the user's pick): **ambush**: spiders drop from walls/ledges, Unpaid rise from the ground
    near the player.
- "Stone sentinel animations are weak, they aren't a threat either, they need to be harder to kill and have some other attack
  than just a melee."
  - The user's pick: **both** a hurled boulder at range and a ground-slam shockwave up close. Main's default: about twice
    its health, tuned so it's a real fight at its level.
- Quality bar: CLAUDE.md rule 1 (big-studio quality) and rule 7 (every action rewarding, high-quality visual and sound
  effects). The spider's fight balance had a standing "untouched" rule. The user now asks for better AI, so changes to how it
  hunts are allowed. Keep its numbers (health, bite) unless a change is part of the ask.

## Pictures of today (in-game, 2026-10-31)
- `Saved/Screenshots/Tools/cr_spider3.png`: the spider walking. Its near legs stretch into long straight spikes; the
  colours are flat.
- `Saved/Screenshots/Tools/cr_unpaid2.png`: the Unpaid. Washed out pale/white, the clothes near white.
- Earlier shots: `Saved/Screenshots/Tools/show_*.png` (Unpaid, Spider, Spitter, Gravemother, Stone Sentinel).

## Jobs and ownership (two agents never own one file)
- **ART: the spider's and the Unpaid's models.** Textures and colours true to the user's Tripo models
  (`Art/Models/Creatures/Tripo/MeadowWolf.glb`, `Unpaid.glb`), and skin weights so nothing stretches or twists.
  - Owns `Art/Models/Creatures/Spider.py`, `Unpaid.py`, `Tripo/parts_spider*.py`, `Tripo/parts_unpaid*.py`, their texture
    sets in `Art/Textures`, and any import/material script lines for them (write the edit for Main if the script is shared).
  - **Must keep each skeleton exactly as it is** (bone names, hierarchy, rest pose), because the C++ poses bones by name.
- **SPIDER: the spider's animation and AI.**
  - Gait, idle, turn, attack, hit reactions and death in code.
  - The ambush: spiders wait on cliff faces, ledges, rock walls and trees, climb down or leap down onto the player who
    passes under them, and lurk out of sight before they spring.
  - Owns `Creatures/SpiderCreature*`, the spider's rules/tuning files, `Creatures/AmbushSpawner.*`, `Creatures/Encounter*`
    (spawner code) and new spider/ambush files.
  - Places new ambushes through layout patches (`Art/Levels/RansomsRest/patches/<name>.json`, see `Docs/Pipeline/Workflow.md`
    "Layout patches"), never by editing layout.json.
- **UNPAID: the Unpaid's animation and AI.**
  - Gait, idle, attacks, hit reactions, death.
  - The ambush: Unpaid lie buried and rise out of the ground AROUND the player (surrounding, staggered, with dirt bursting
    and a sound), in Ransom's Rest's graveyards, fields and roads at night or near graves. Uses `EAmbushEntrance::Rise` /
    `AUnpaidCreature::RiseIn` and improves them.
  - Owns `Creatures/Unpaid*` and new Unpaid files. Spawner code changes it needs: the exact edit in its README for Main.
    New placements: layout patches.
- **SENTINEL: the Stone Sentinel.**
  - Much better animation: weighty, stone-grinding, readable windups.
  - About twice the health, made a real threat.
  - A hurled boulder at range: tears a rock from the ground, throws it in an arc, splash damage, dodgeable, with a shadow or
    marker where it lands.
  - A ground-slam shockwave up close: a ring of rock spikes travelling out; jump or step aside.
  - Effects and script-made sounds for each (`Art/Sounds/recipes/<new file>.py`; render only your own cues with
    `Tools\sounds.ps1`).
  - Owns `Creatures/StoneSentinel*`, its bestiary data lines (write them for Main), new projectile/shockwave files and its
    sound recipe.
- **MAIN:** builds, imports, runs scripts and layout patches, PIE checks, CODEMAP, docs, git.

## Rules for every agent
- Read CLAUDE.md, Docs/Pipeline.md, Docs/Pipeline/Enemies.md (each creature's section), Workflow.md and CODEMAP.md first.
- C++ goes in a holding tree, `Saved/CreatureCode/<AGENT>/Source/AI_Looter_Shooter/...`: copies of changed files with full
  contents, plus new files, with the originals' `git hash-object` in `HASHES.txt`. Python, art and sound files you own:
  edit in place.
- Never build, open Unreal or touch git. Blender only through `Tools\artrun.ps1`.
- Other agents are rewriting `UI/Menus/PauseMenu*`, `SettingsMenu*`, `UI/HUD/LooterHUDPause.cpp`, the loadout and ward
  code, and the weapon manager: never touch those.
- README in your tree:
  - files and CODEMAP lines;
  - hooks for Main (exact edits) and what Main must run (imports, sound bank, layout patches, area builds);
  - tests: add or update them (`Looter.Creatures.<Name>.*`), plain rules testable without a world;
  - how Main checks it in PIE: `Looter.SpawnCreature <Kind>`, slomo, play.py;
  - risks.
- One class per file, under ~500 lines a file. Every .cpp includes what it uses. Comments say why in plain words. Check
  every API you call against its header so it compiles first time. Performance target 120 fps on an RX 580: keep effects
  cheap.
