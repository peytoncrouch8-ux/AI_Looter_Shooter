# Pipeline

How things get made and fixed here. Update when work finishes (a new way, a bug and its fix, what didn't work). Rules:
`CLAUDE.md`. State of the work: `Docs/Handoff.md`. Old detail, not loaded by default: `Docs/Archive/Pipeline_history.md`.
The notes live in `Docs/Pipeline/<Topic>.md`; cite them as `Docs/Pipeline/<Topic>.md "<section>"` (section names are the
`##` headings and bold lead-ins inside the files). Add new notes to the matching file; a new topic gets a file and a line here.

- `Docs/Pipeline/Game.md` ("The game"): what the game is and how every runtime system works (player, HUD, weapons, creatures, loot, saves, UI kit, audio; round 3: startup logos, main menu, Teropa menu map, new game intro, melee, Lost Loot; round 4: swimming and the underwater look, grenade kinds and the carry rule, boss intros).
- `Docs/Pipeline/Workflow.md`: "Who does what" (agents and contracts), "The everyday loop", "Play checks" (PIE), "Packaging a test build", "Performance" (measuring), "Tests".
- `Docs/Pipeline/Levels.md`: "Making a level (grounded areas)" (terrain, scatter, ground fit, world variety), "The tutorial island and the older levels", "Island styles", "The Style Lab"; roads, signposts and chest spots are a bullet under "Making a level".
- `Docs/Pipeline/Assets.md`: "Making an asset" (Blender to Unreal, materials, textures; grenade jars, swim waters and the underwater assets) and "Sounds (script-made)" (incl. round 4's recipe groups).
- `Docs/Pipeline/Heroes.md`: "Character portraits" (painted busts) and "The hero pick" (select screen, third-person hold, per-hero camera), "Melee tools" (each hero's tool, round 3; its sounds, effects and third-person hand, round 4).
- `Docs/Pipeline/CharacterModels.md`: "Character models" (MakeHuman bodies, clothes, hair cards, rig, swing bones, the idle, the forearms, Ellis as the player's body, bake/export rules).
- `Docs/Pipeline/EmberPowers.md`: "Ember powers" (the heroes' powers, their code and assets; "Round 3": cast pose, Odessa's cards).
- `Docs/Pipeline/Story.md`: "The heroes' words", "Seeds in Ransom's Rest", "The story's talk cut, and things to find".
- `Docs/Pipeline/Enemies.md`: "Enemy tiers", "The Gravemother's Barrow fight", "Fight: AI", "Fight: Layout", "Boss intros" (under the Gravemother fight; round 4), "Placeholder creatures", "Bounties".
- `Docs/Pipeline/Gunplay.md`: "Gunfeel: kick and handling", "Gunfeel: hit reactions", "Gunfeel: gun sounds", "Grenades" (the grave-salt jar; "Grenade kinds", round 4), "Loot: the four new guns", "Iron sights", "Loot: elements, part effects, drop moments", "VFX pass".
- `Docs/Pipeline/Bugs.md`: "Bugs and how they were fixed" (incl. test-world traps) and "What worked, and what didn't".
