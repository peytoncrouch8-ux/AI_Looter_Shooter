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

**World polish done, 620/620 tests, pushed** (ff25b07..80c68ad): concept C (lamps flicker, window life, ambient air,
gusts), the cliff and rock review (steep terrain wears the kit's sandstone, no floating fragments or mirrored repeats,
outcrops reworked: look sheet 45), cliff life (look sheet 43: 273 pieces, 70 stains), the town (look sheet 42 + the
village kit: 655 instances, 12 dusk lamps), the regenerated terrain (band fix, ore bin bay) and a full rebuild + scatter;
path_probe clean. Look sheets 42-45 approved (44 animals: "slightly more realistic", no need to show).

**Combat, animals, roosts, rewards and the user's list done, 666/666 tests, pushed** (baabab1..fc7363a): two crow
stations a map (the Depot, Calder's Post; a death wakes at the last one found; the other nine removed); D creatures
react, E guns that feel physical, small animals (hens + rooster in two yards, a barn cat, cliff swallows at their
nests), F rewards (level-up burst, XP popups, footfall dust, pickups pull), their 99 sounds (334 cues). The user's seven
items (2026-10-14): tracker under the minimap with check circles; "+N XP" by the kill and by the XP bar; the gun's
notches before its name; no crosshair in ADS (third person keeps a centre dot); crouched ADS aligned; the wheel up =
previous slot; Subtitles On/Off (Interface; spoken lines hide, notices stay); health and XP bars 25% longer and thicker.
The town's small clutter culls at ~30 m. Perf (editor closed, 46 views): heaviest 8.1 ms (PointOverValley_FarShadow),
Aerial 8.05, Farm 8.0; render-thread bound at ~1,450 draws: thin headroom.

**Waiting on the user:** a playtest of all of the above (they were away; told it's ready 2026-10-14).

## Next (in order)

1. The user's playtest notes first.
2. Draw-call headroom in Ransom's Rest (Aerial, Farm, PointOverValley ~8.0-8.1 ms of 8.3): `Looter.Perf.HideTag` shares.
3. Keep polishing (the user's "until I tell you to stop"): nothing major, concepts before art.
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
