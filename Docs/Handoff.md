# Handoff

Where the work stands, for the next session and every agent. Update it as each request finishes (newest first). Rules are
in `CLAUDE.md` ("How we work"); how things are made and fixed is in `Docs/Pipeline.md`.

## Now (2026-10-08)

- **Ransom's Rest is done.** The user played it through and approved it ("it all looks good"); steps 18-27 are recorded
  as approved in `Docs/Areas/RansomsRest.md`. All 226 `Looter.*` tests pass, the terrain rebuilds identically, and every
  view runs at 149-216 fps on Medium.
- **The next level, the *Gilded Lily*, is on hold.** The user wants to polish other things first. Build nothing of it
  until they say so. Airship or riverboat is still open: Main leaned airship (the canyon and fog work carries over; the
  cost is moving gondolas and a Sky Jumper enemy). The trade-offs are in `Docs/Story.md`, decision 12.
- **Next:** whatever the user picks to polish. Ask them which first.

## Finished in the last session (2026-10-07 to 10-08)

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
- The cloud session's handoff (`Docs/Handoffs/`: Screen Print Wash, Crossroads Town, the HUD upgrade and more) is not
  built: the user said "not yet" and will decide which look wins.

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
