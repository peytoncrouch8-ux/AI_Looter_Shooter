# Handoff

Where the work stands, for the next session and every agent. Update it as each request finishes (newest first). Rules are
in `CLAUDE.md` ("How we work"); how things are made and fixed is in `Docs/Pipeline.md`.

## Now (2026-10-08, the cloud session)

- **The user's three asks (2026-10-08):** the new HUD in the game (it is: built 2026-10-07, 731e0df and 5b5b36a); the
  old art-style preview and the style it showed deleted from every file; and ten very different art styles, previewed
  in the game, to choose from. The user found the current look and every earlier style exploration disappointing:
  "I need something phenomenal".
- **Done on the user's PC (the local session, 2026-10-08):** the sandbox preview deleted (the user confirmed), the cloud's
  gem change built and checked at 1080p. The gun's name now ends at the cluster's right edge: it was set in a scale box
  that shrank and centred it, so it's measured and set smaller only when it's too long (`FitWeaponName`).
- **The Style Lab** (`Tools/StyleLab/`, being built in the cloud session) previews the ten styles in a real-time page
  of Ransom's Rest made from the game's own models, textures and terrain, under the new HUD.
- **The Gilded Lily stays on hold**, and the cloud handoff's other decisions (Crossroads Town, gun ideas, ember powers,
  heroes) are not started.

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

- The slide is drawn in code (a low pose, a lean, a view roll). A real slide animation, dust and a sound would need an
  art and sound pass.
- Creature chase speeds other than the spiders' weren't scaled with the player. The Unpaid chase at 470 against the
  510 walk (92%, was 78%); Abel at 430.
- The tutorial island is called "Skyreach" in the game (the menu, the tutorial, the station board). Renaming it is the
  user's call.
- Ledge paths: the outer face pieces of the bluff path and the Sink ramp reach into the walkway's middle band; a 2 m+
  clear lane remains. The cliffs pass logs these as warnings.
- The cloud session's handoff (`Docs/Handoffs/`): the HUD upgrade is built; Crossroads Town and the rest are not.

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
