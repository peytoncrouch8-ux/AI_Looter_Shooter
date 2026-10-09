# Handoff

Where the work stands, for the next session and every agent. Update it as each request finishes (newest first). Rules are
in `CLAUDE.md` ("How we work"); how things are made and fixed is in `Docs/Pipeline.md`.

## Now (2026-10-08 night, the local session after "Game Enchancements")

- **The user's asks:** (1) change the shooting, task-completion and reload sounds; no crit sound; creatures should sound
  like what they are (slimes squishy); (2) start building Crossroads Town on Skyreach. Then, going to sleep: **a polish
  loop until they say stop** (below). They said **"The new sounds are good."**
- **Done, pushed:**
  - Sounds (d89c720, 68c764e): guns rebuilt twice (heavy and punchy, then less synthetic after Borderlands: a modelled
    pressure blast through an overloaded mic, ground bounce, loud action, uneven echo tails), weightier reloads, short
    reward-ping mission sounds, crits play the hit tick 12% higher, per-creature bullet hits, slimes all jelly. Listening
    page: <https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa>.
  - Crossroads Town round 1 (this commit): see Plan.md Phase 7 and the pipeline's "Crossroads Town". Six spiders in
    Web Hollow, slimes in the Wallow, none respawn on Skyreach; the tutorial says "Follow the road into town".
- **Calls made while the user slept (review):** the town's hedges are passable (bushes have no hulls); the notice
  board moved in front of the saloon; Web Hollow's tor moved 10 m north; the memorial's benches moved off the roads.
- **Round 2 of the town (not started):** the cobbled square and kerbs, stalls, scarecrows, coop, haystacks, sandbags,
  targets, the dock, mushrooms, burrows, cabbages, flowers in pink, red and blue, the Wallow's darker sward and slime
  trails, townsfolk, sheep and hens (`town.json` `deferred`). The Wallow's pools hide in tall grass.

## The polish loop (the user, 2026-10-08 night: "make the game better and increase the quality on loop until I tell you to stop")

Compare with Borderlands' mechanics and playstyle; surf the web as needed; no new levels or areas (new assets are fine).
Don't ask the user questions while they sleep (until 5 AM EST): make the calls and list them here. Their examples:
- [ ] The tutorial feels rushed and unnecessary.
- [ ] Boss fights lack excitement (the Gravemother, Abel).
- [ ] Creature and character models clip or animate weirdly.
- [x] Random rocks hang out of cliff walls: cliff faces squashed into slabs near the ramps' heads are left out
      (11be5fe); a probe of every rock and cliff piece in both levels finds none floating.
- [ ] Ransom's Rest's intro scene: the map is flat brown terrain (trees alone don't fix it).
- [ ] The end of Ransom's Rest's story should leave the player at about level 6-7, not 9-10.
- [ ] No experience for mission steps; missions are turned in, not finished by their last task.
- [ ] The inventory is confusing and boring, with too much information at once.
- [ ] And more: whatever makes it feel empty or wonky. The research and its prioritized list:
      `Docs/Polish/BorderlandsComparison.md` (no ambience or music, no healing, no camera shake or hit-stop, no mantle).

**State at 01:40 (2026-10-09): waves 1 and 2 built, placed, all tests passing, committed.** In the game now: boss
shows (entrances, phases, staggers, loot shower), turn-in missions with a banner and no step XP, the clearer inventory,
hit feedback (camera kicks, hit-stop, stagger, death bursts), mantle and vault, wounds that close and soul-motes, ambience
and music, the reworked tutorial (find a gun, read Skyreach's notice board, its practice postings, control hints), ambient
life (crows, sparrows, swallows, hawks, insects, tumbleweeds, dust devils, washing), the melee strike (V or the right
stick; the gamepad view toggle moved to D-pad up), encounters between the fights in Ransom's Rest (3 camps, 2 patrols, 2
ambushes; flanking, retreat, the rank sting and its flash), creature steering round fences (wall-following, unstick),
Ransom's Rest's late-summer land (gold and olive mosaic, hayfields, banded ridges, grass to 40-50 m), animation fixes
(spider bite legs, death curl, Unpaid hat bank, player climb pose and third-person melee jab), and sounds for every new
cue (160 cues). Story end: about 786 XP = level 6.87; an explorer about 7.44.
**Calls made (review):** the story ends at 6.87 (inside 6-7; `KillXPScale` 0.36 would give 6.67); the churchyard
ambush springs again on every load; camp crates reuse Ruth's Supply Crate; a melee kill notches the gun in hand; the
Unpaid's shriek still presses its right armpit about 2.5 cm into its waist (invisible through the ghost body; the test
now allows 3 cm after two fix passes); the spider's death curl is slower (0.8 s) and ends knees up and out.
**Open:** the "rain" in the HUD photos was the test boss's fog-wall curtain (thin pale dashes that read as rain, Abel's
gate too): being restyled to read as a wall; the photo scene now removes the test boss after its shot. Skyreach's notice
board model still reads "RIM RANGERS" (a NOTICES variant is being made). Wave 3 (running): a lootable world (breakables, graves, coffins), a map page and fast travel between graves, Unpaid
barks and signs of life in town, a new gun family, a grenade.

**State at 23:30 (2026-10-08):** finished by agents, not yet built or committed (Main is integrating: build errors
being fixed one by one): bosses (show, phases, staggers, loot shower; `UBossComponent` show), missions (turn-ins,
`FMissionTurnIn`, fixed XP, kill XP x0.4, the mission-complete banner, `Docs/Progression.md`: about 721 XP = level 6.5
at the story's end), inventory (list + docked card + gun showcase, `Looter.MenuShots`), feedback (one
`UCameraShakeModifier`, shot kicks, per-creature hit-stop and stagger, death bursts, loot fanfare, damage arcs, a camera
shake setting), creature animation (Unpaid arms, spider feet, slime tilt, pack spacing, `Looter.CastShots`), traversal
(mantle, vault, coyote time, jump buffer, unstick), recovery (wounds that close after 6 s, soul-motes), and the sounds
for all their new cues (`recipes/bosses.py` etc.). Still running: terrain look, ambience and music, ambient fauna,
the tutorial rework, encounter pacing. After the build: create_mission_assets.py and create_side_mission_assets.py,
sounds into the bank, tests, MenuShots/CastShots/hudshots, tours, perf, then commits per feature.

**Wave 1 agents (started 2026-10-08 night; each owns its files, Main builds, checks and commits):**
missions (turn-ins, no step XP, level 6-7 pacing, a mission-complete banner); bosses (entrances, phases, adds, weak
spots, loot shower); Ransom's Rest's terrain look (macro mosaic, ridges, distance grass, the ring); the inventory
(clearer, Borderlands-like card and list, plus `Looter.MenuShots`/`Tools\menushots.ps1`); ambience and music
(script-made beds, emitters, a music director); feedback (camera kick, hit-stop, stagger, death bursts, loot fanfare,
damage direction); traversal (mantle, vault, coyote time, jump buffer); creature and character animation and clipping
(plus `Looter.CastShots`/`Tools\castshots.ps1`). Next after the missions agent: the tutorial rework. Later: the
recovery loop (out-of-combat regen), encounter pacing, ambient life.

## Earlier on 2026-10-08 (the local session "Game Enchancements")

- **The user's asks:** slower creature chases (not the slimes); Skyreach keeps its name and starts the game; fix narrow
  areas; the gun ideas (notches, cursed irons, part swapping); a smooth slide with dust; start on sounds.
- **Done, all pushed, 254 tests passing:**
  - Chases: the Unpaid ~400 and Abel ~365, scaled to the player like the spiders (54b1408). Skyreach was already the
    island's name everywhere in the game.
  - Narrow areas: Ransom's Rest's ledge faces sink under the bluff path and the Sink ramp (982127f); Skyreach's roads
    keep big stones off (a boulder stood in the forest road, a3228f6). Path probes clear in both levels.
  - Gun ideas, slide and sounds (4f6abb1, dfa6204; Plan Phase 9): see the commit and `CODEMAP.md` (Audio, Weapons,
    UI/Bench). Benches in Skyreach's village (by the gun rack) and against Ransom Farm's barn (d63bd03).
  - The sounds: 65 cues, 225 takes, synthesized from scratch (the user's call: script-made only). The listening page:
    <https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa> (private to the user). Least sure: the creature voices, the
    gunshots' balance, the hit marker, the mix levels.
- **Waiting on the user:** to listen and say which sounds to keep or redo (by their names on the page); to play the
  slide, the bench and the cursed irons.
- **Choices the agents made that the user may want to change:** notches count only kills that give experience, so none
  on Skyreach (a practice area); Hungry charges at a reload's end; a cursed gun's beam gutters even once lifted; the
  last gun in the slots can't be scrapped; the settings menu is now one scrolling list.
- **Small, open:** the HUD's weapon-name line has no cracked coin for a cursed gun yet (`MakeGunNameLine` or
  `CrackedCoinBrush` in `PlayerHUDWidget.cpp`); the cold open's `ColdOpenSet` sound fields could play cues; roads carved
  into terrain sound like grass (tag meshes `Surface.<Name>`); `PlayerViewComponent.cpp` is 566 lines (split it).

## Earlier on 2026-10-08 (the local session "UI and HUD changes")

- **The user's three asks (2026-10-08):** the new HUD in the game (it is: built 2026-10-07, 731e0df and 5b5b36a); the
  old art-style preview and the style it showed deleted from every file; and ten very different art styles, previewed
  in the game, to choose from. The user found the current look and every earlier style exploration disappointing:
  "I need something phenomenal".
- **Done on the user's PC (the local session, 2026-10-08):** the sandbox preview deleted (the user confirmed), the cloud's
  gem change built and checked at 1080p. The gun's name now ends at the cluster's right edge: it was set in a scale box
  that shrank and centred it, so it's measured and set smaller only when it's too long (`FitWeaponName`).
- **The ten styles are ready for the user to pick** in the Style Lab, <https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr>
  (private to the user): Ransom's Rest rebuilt in three.js from the game's own models, textures, terrain and editor-build
  placements, under the in-game HUD; walk, shoot spiders, number keys switch styles, `I` shows each style's card
  (references, the Unreal recipe, its cost on Medium). Six fixed shots per style: `Saved/StyleLab/shots/styles/` in the
  cloud container (rebuild with `Tools/StyleLab/test/shots.mjs`). The ten, cartoon to real: 1 Clay Frontier, 2 Skyward
  Anime, 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp, 6 Sunbleached, 7 Neon Frontier, 8 Ember Gothic, 9 Celluloid
  West, 10 Golden Hour; 0 is today's look. The art direction briefs and the research behind them: `Docs/Art/StyleLab/`.
- **The user's call (2026-10-08):** the best are 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp and 6 Sunbleached.
  The ten are catalogued (done 2026-10-08, the local session): `Docs/Art/StyleLab/Catalog.md`, contact sheets in
  `Docs/Art/StyleLab/Sheets/`.
- **The art style stays as it is (the user, 2026-10-08: "I don't want to change the style").** Style 3 was tried
  and removed; style 6, Sunbleached, was tried and taken off the island; style 4 was stopped before any code. Every level,
  the tutorial island included, wears today's look. `Tools/Unreal/build_island_style.py <style> apply|undo|status`
  and Sunbleached's files stay in the project in case the user asks again (its switch on the masters is off, so it
  costs nothing). Don't start style work unless the user asks.
- **A test build exists (the local session, 2026-10-08):** `Tools\package.ps1` ("Packaging a test build" in the
  pipeline). The first: `Saved\Packaging61008-163145\AI_Looter_Shooter_Test_20261008-163145.zip`, 605 MB, with
  today's look (made before the painted trial). It runs the menu, sessions, saves and the HUD; it was made before the
  sounds. Sharing it needs the user's PC (or a cloud-drive tool the user agrees to).
- **The Gilded Lily stays on hold**, and the cloud handoff's other decisions (Crossroads Town, ember powers, heroes)
  are not started; the gun ideas are built (above).

## Finished in this session (2026-10-07)

- **The HUD upgrade** (spec: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, "The HUD upgrade in brief"):
  - the kit draws painted pictures (`FPaintedIcon`, `PaintedIconBrush`) and glows; gunmetal, gem and health colours
    in the palette;
  - the player frame (`UHudPlayerFrameWidget`) with the portrait (`UHudPortraitWidget`, from
    `Art/Icons/HudPortrait.py`), the level-up banner and red screen edges replace the health ring and the XP bar;
  - the mission tracker (`UHudMissionTrackerWidget`) replaces the tutorial prompt and follows any tracked mission;
    objectives carry a short line and a key hint;
  - the weapon slots stand in a column beside an upright cartridge; the crosshair kicks on each shot;
  - the minimap's gunmetal bezel and area name; the boss bar built like the health bar;
  - the old whole-screen red flash on hits is gone (the user's call).
- **Tools:** `Looter.HudShots` / `Tools\hudshots.ps1` photographs the HUD at 1080p through twelve states.
- **Rules (the user's):** at most 8 agents; the orchestrator (Opus 5.5) picks each agent's model; parallel agents work
  from one shared contract; `CLAUDE.md` condensed, its details moved to `Docs/Pipeline.md`.

## Finished in the session before (2026-10-07 to 10-08)

- **The user's asks:**
  - The player 15% smaller and slower.
  - A look-sensitivity slider (Settings > Controls).
  - Space stands up from a crouch, and the next press jumps.
  - Sprint plus crouch is a slide at 1.1x speed, which ends in a sprint if forward is held.
  - Spiders and the Gravemother slowed to match the player.
  - Commits 8fba9e6, 73aa50f, 6b03dff.
- **The user's play-test bugs:**
  - The spider boss stuck in her den's wall (f3e3579).
  - A rock blocking the tutorial island's ramp to the lookout (bd96147, 8e0e50c).
- **Ransom's Rest art round**, all of the art session's notes closed:
  - the Sink's pit wall in new cliff panels, with talus at their feet (de5a8fe, e2e826a);
  - the den re-dressed;
  - the slopes' scrub in patches, and far-tree groves with pines (7c305cc, c685d66);
  - fences standing plumb and stepped, and the churchyard's graves facing the valley (c49039f, 473a338);
  - the burial deck clear of the rim's rock (732b43d);
  - the windmill in galvanized steel, and Abel's lantern glass warmer.
- **New build tools:** ramp walkways cleared (`build_area_walkways.py`), cliffs under platforms
  (`build_area_platforms.py`), pit panel runs (`build_area_panels.py`), talus (`build_area_talus.py`), path probes
  (`Tools/Unreal/path_probe.py`), and Main's helpers moved into `Tools` (`commitlib.py`, `stage_import.ps1`,
  `winshot.ps1`, `open_clean.py`, `pie_check.py`).

## Open, small (the user hasn't asked for these)

- The slide is still a code-drawn pose (now smooth, with dust and sounds); a real slide animation would need an art pass.
- The cloud session's handoff (`Docs/Handoffs/`): the HUD upgrade and the gun ideas are built; Crossroads Town, the
  ember powers and the heroes are not.

## Sessions and helpers

- **AILS Artwork** (a local Claude session) models in Blender and never commits; Main imports and commits. It has
  closed every Ransom's Rest note and was told the *Lily* is on hold.
- The cloud sessions (Cloud, AILS Storyline) are idle.

## Starting a new session

1. Read this file, then `CLAUDE.md`'s rules, then `Docs/Pipeline.md`.
2. Run `git status` (the tree should be clean) and `git log --oneline -15`.
3. Before any editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`, and look for a running
   `UnrealEditor.exe ... -game`.
4. Ask the user what to polish first, with tap-to-answer choices.
