# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Small leftovers", 2026-10-15)

Done this session (the user picked "small leftovers" first): `WeaponManagerComponent.h` split (its events and small
types in `Inventory/WeaponManagerTypes.h`, 491 lines); the editor-exit ensure on Hiram's figure fixed in all six
poseable-mesh constructors (Pipeline, Bugs); the shotgun reworked with the user watching: held 20 cm nearer, the left hand
cupping the pump with a new `EHandGrip::Pump`, the thumb along the pump (the user approved the close-up); live switches
for the support hand's finger and thumb curl; `play.py equip <n>`. Many other files are still over ~500 lines
(`CreatureBase.h` 562, `BossShrine.cpp` 568, `LooterUIStyle.cpp` 765, `LooterSoundCues.h` 847, several tests).

## Before (end of session "Polish: make it alive", 2026-10-14)

All committed and pushed (HEAD 8a005cb). Tests 687/687 at 57264b8. That session's full log:
`Docs/Archive/Handoff_history.md` (last section). How each piece works: `Docs/Pipeline.md`; perf numbers:
`Docs/Performance.md`; release-only work: `Docs/Release.md`.

**Built this session:** the old handoff items; world life (lamps flicker, window life, ambient air, gusts), the cliff and
rock review, cliff life, the town's dressing; two crow stations a map; creatures react; guns that feel physical; small
animals; rewards you can see; the user's seven HUD/control items; High/Epic fixed (small scatter out of Lumen, High
without Lumen); spider deaths (legs curl under); clear teal water and waterfalls; the third-person carry; first-person
hands (SK_FP_Arms, Ellis's gloves and sleeves) with raised, level holds and hands at 0.88; guns drawn bigger (rifle and
shotgun 1.1, revolver 1.2); the pistol named "Revolver"; swap animations with holster sounds.

**The user's decisions this session (keep):** Lost Property shelves 300/600/1000/1600/2400; third person keeps its own
FOV; endgame level scaling decided later; two crow stations per map (RR: the Depot and Calder's Post), a death wakes at
the last one activated; animals small only (hens, cat, swallows; no mule); Medium stays as it is (no grass trade-off),
new additions save their own cost; no optimization work now (smoothness waits in `Docs/Release.md`); the Gilded Lily
starts as a World Partition map; water clear teal; first-person hands the full Borderlands way, guns raised so both
gloves show, level (no forward tilt); Ellis's own third-person body LATER (the player is still the white mannequin).

**Waiting on the user:** a playtest of the hands after d0d9d2e. Not seen in PIE: the support hand during reloads (tests
cover the paths) and the hands while aiming down sights.

## Next (in order)

1. The user's playtest notes first (hands, guns, swaps, water, spiders; the shotgun's new hold).
2. Small leftovers: the revolver reads tipped up at the hip (perspective; tilting it up looked worse); the rifle's and
   revolver's support hands weren't looked at again after the shotgun's rework (same kind of check if the user asks).
3. Keep polishing until the user says stop: nothing major, concepts before art, ask with tap-to-answer choices.
4. **Older open items:** Story.md's later areas' level bands predate the new curve (retune when the next area is
   designed); the user hasn't heard the newest sounds (republish the listening page); the slide is a code-drawn pose;
   from the cloud handoff, the ember powers and heroes aren't built (major: only if the user asks); `area_faces.fields`
   gives the East Shoulder's face ground ~748 m^2 inside the boundary where the ridge doesn't stand (Stage Gap's depot
   side; changes the macro maps: needs the user's eye).

**On hold (the user's calls):** the Gilded Lily (when it starts: a World Partition map, Pipeline "Making a level");
Skyreach's own house kit; no style work unless asked.

## Sessions

- Main's agents model in Blender this session (the AILS Artwork session isn't running); Main imports and commits.
- The session "Polish: make it alive" ended 2026-10-14 at the user's call; the next session picks up here.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
