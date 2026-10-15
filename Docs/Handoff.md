# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Ellis portraits", 2026-10-15, cloud; branch `claude/ellis-portraits`)

**The request:** the heroes' models "suck"; first paint portraits like the user's reference (richly framed painted busts),
pick one, then build the 3D model from it. Ellis only for now, three portraits.
**Built:** `Art/Portraits/` (Pipeline, "Character portraits"): Ellis A The Revenant, B Came Home at the Last, C Dressed for
the Grave, rendered in Blender from MakeHuman's CC0 base, painted and framed; the frame generator; `sheet.js`.
**The user's decisions (keep):** Blender's `bpy` may be installed in the cloud box; MakeHuman's CC0 assets may be used
(`Art/ThirdParty/MakeHuman`); the code-only SDF busts were passed over for Blender.
**Waiting on the user:** which Ellis (A, B, C or a mix) becomes the model.
**Next:** build Ellis's in-game model from the pick: the MakeHuman base shaped as in `EllisBlender.py`'s `FACE`, the
clothes as real meshes (cloth sim or sculpt where the drape is too smooth), retopology, rig on the UE mannequin skeleton
(`Tools/Blender/looter_heroes.py`), materials; then the other four heroes' portraits the same way.

## Before that (end of session "Polish: make it alive", 2026-10-14)

All committed and pushed (HEAD 8a005cb). Tests 687/687 at 57264b8; after the last hands fix (d0d9d2e) the Player and
Weapons groups pass. This session's full log: `Docs/Archive/Handoff_history.md` (last section). How each piece works:
`Docs/Pipeline.md`; perf numbers: `Docs/Performance.md`; release-only work: `Docs/Release.md`.

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

1. The user's playtest notes first (hands, guns, swaps, water, spiders).
2. Small leftovers: the revolver reads tipped up at the hip (perspective; tilting it up looked worse); the shotgun's
   sleeve end just touches the frame's bottom; `WeaponManagerComponent.h` is 543 lines (split it); an ensure at load on
   Hiram's figure (DeadFriend's `WearModel` sets SkinnedAsset in the constructor without NotifyIfSkinnedAssetChanged).
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
