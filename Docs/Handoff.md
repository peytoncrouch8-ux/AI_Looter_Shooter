# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Polish: make it alive", 2026-10-14)

**The user's ask:** finish the handoff, then polish until they say stop: "the game feels dead and devoid of character...
minor details around houses, cliffs, better mob animations, better projectiles, better visualizations"; plus "observe the
terrain (mainly the cliffs and rocks) around the map, fix any discrepancies or odd shapes". Nothing major, no key details
changed; art only after they approve concepts. Their answers: guns never vanished for them; Lost Property shelving dearer
(Main doubled it: 300/600/1000/1600/2400); third person keeps its own FOV; endgame scaling decided later.

**Handoff items done, 607/607 tests, pushed** (0b54bdb..d0815dc): the vanished guns were play.py's teleport (a
set_editor_property in PIE rebuilt the pawn's Blueprint components; the game now warns loudly); Sexton's highlight is a
rim (`Figure` look); a searching creature no longer blocks fast travel or holds combat music; CreatureBase.h split (526
lines); board note focus, board turn-in lines, the Haskett lift mutter, Ellis's headboard readable, save v3 step
migration; porch plate in three pieces; the lookout stain decal (readable moved onto the model's stained boards); the
5 flagged sounds fixed and imported; the generator's onFloor band fix, the ore bin's loading bay (`oreBinBay`), no oak on
the Bench; the mine's mouth no longer shows sky through the hidden rooms (adit drawn from outside + a veil); tour views
inside the interiors (`top`), `runtests.ps1` reopens clean and closes the Message Log (`-Restart`). RangerCabin's tangent
warning was the reimport's first build, not the model. Perf (editor closed, 37 views): 128-206 fps, heaviest 7.8 ms,
render-thread bound at ~1,400 draws (`Docs/Performance.md`).

**In flight when this was written:**
- The Ransom's Rest generator run (`--macro --computed`) for the band fix and the ore bin bay; then `models.ps1 -Only
  RansomsRest`, a FULL `build_area.py RansomsRest` (it also places the mine's veil, the stain decal, the headboard
  readable, the note's card), `build_island_scatter.py RansomsRest`, probes (`Saved/bench_oaks.py`,
  `Saved/missions_c_probe.py`, `cliff_in_buildings.py`, `path_probe.py`), the H agent's `computed_diff.py` (scratchpad)
  against HEAD's layout_computed.json, save, commit the level and the new terrain.
- Polish, world first (concept boards A-F approved, PNGs in `Saved/Concepts/Polish/`; memory `polish-alive`): look sheets
  42 Frontier clutter, 43 Cliff life, 44 Small animals (hens, rooster, cat, swallows), 45 Rocks reworked (outcrops) go to
  the user for approval before import; code agents on lamp flicker + window life (curtains, passing shadows) and ambient
  air + gusts (`UGustSubsystem`, `USwingComponent`); the town dressing (`Art/Levels/RansomsRest/make_town.py` -> town.json,
  existing kit now, new kit deferred until approved); the cliff review (steep terrain reads as sandstone, gaps filled, no
  floating fragments, varied repeats). Contract: the session's scratchpad `contract_polish.md`.
- The cliff review's evidence: `Tools/cliff_views.py` writes a view per cliff point (play side only) for `tour.ps1`.

## Next (in order)

1. Finish the in-flight work above; show the user each look sheet; build what they approve; re-shoot the cliff views.
2. Then combat polish (approved): D creatures that react (notice beat, own blood + hit flash, weightier deaths, rising
   spawns, idle life, footfall dust), E guns (brass, smoke, chips, ricochets, near-miss whiz, tracers), F rewards
   (level-up burst, XP popups, footstep dust, pickups pull).
3. Ask the user to play when a wave lands; perf after each wave (new dressing instanced).
4. **Older open items:** Story.md's later areas' level bands predate the new curve (retune when the next area is
   designed); the user hasn't heard the newest sounds (republish the listening page); the slide is a code-drawn pose;
   from the cloud handoff, the ember powers and heroes aren't built (major: only if the user asks); `area_faces.fields`
   gives the East Shoulder's face ground ~748 m^2 inside the boundary where the ridge doesn't stand (Stage Gap's depot
   side; changes the macro maps: needs the user's eye).

**On hold (the user's calls):** the Gilded Lily; Skyreach's own house kit; no style work unless asked.

## Sessions

- Main's agents model in Blender this session (the AILS Artwork session isn't running); Main imports and commits.
- A peer local session named "Handoff" was open and idle on 2026-10-14. The cloud sessions are idle.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
