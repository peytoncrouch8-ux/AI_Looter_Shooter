# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Ellis 3D model", 2026-10-15, cloud; branch `claude/ellis-3d-character-model-tvwr9z`)

**The request:** turn Ellis portrait A into the game's 3D model, realistic enough for the game, textured from boots to head,
and something the user can spin round. Midway the user: "I'm liking the look so far, but his texturing looks too smooth ...
More realistic modeling and less smooth looking." (done: cloth-simulated duster, creases, seams, stitching, grain, wear).
**Built:** `Art/Models/Characters/Ellis.py` -> `SK_Ellis` (Pipeline, "Character models"): MakeHuman's body shaped as the
portrait; a cloth-simulated duster to mid-calf (back vent, seams, topstitched hems), fluted cape, rolled turned-up collar
with the sun-ring pin; waistcoat, work shirt, trousers tucked into stovepipe boots with stacked soles and cuban heels; roper
gloves with gauntlets and conchos; cartridge belt, Mexican-loop cross-draw holster with the revolver, Pa's lantern burning
at the right hip; hat, hair, pointed bandana, glowing slit eyes. 63 mannequin bones, four 2048 texture sets baked from
procedural looks, hit zones, LODs 50/25; ~20.6k triangles (the user asked for Borderlands' 10-20k: the ~108k detailed build
is now only the bake source, its folds and creases baked into the game mesh's maps). Helpers `looter_human`, `looter_garments`, `looter_looks`,
`looter_bake`. Merged `claude/ellis-portraits` (portraits, MakeHuman assets) into this branch. The spin viewer: an artifact
page (three.js; Spin, Look at Head/Hands/Belt/Boots, Dusk/Day light) with the posed, textured glTF: https://claude.ai/artifact/HJCgTaXnSRbEb5k7SXjdDa (source `Tools/CharacterViewer/`).
**Not done (needs the PC):** the textures aren't committed (LFS): `Tools\models.ps1 -Only Ellis` bakes them (minutes), then
commit `Art/Textures/Ellis*`; the script was run only with Blender 5.2's Python module here, not the PC's Blender (watch
for API differences on the first run); the IK retargeter from the mannequin; SK_Ellis on the player in place of the white
mannequin (third person); the first-person arms against it (SK_FP_Arms' sleeves are #8f7052, lighter than the model's
dust-brown oilskin, and its gloves #a47c56 lighter than the model's: the user's eye decides which wins).
**The user's decisions (keep):** as the portraits session's; Ellis A is the spec; less smooth (folds, creases, seams, wear);
a game budget of about 20k triangles (picked from ~20k / ~35k / keep 108k, 2026-10-15).
**Next:** the user's notes on the viewer; then on the PC: import, retarget, wire SK_Ellis into third person, check the hands;
then the other four heroes' portraits and models the same way.

## Before that (session "Ellis portraits", 2026-10-15, cloud; branch `claude/ellis-portraits`, merged into the above)

**The request:** the heroes' models "suck"; first paint portraits like the user's reference (richly framed painted busts),
pick one, then build the 3D model from it. Ellis only for now, three portraits.
**Built:** `Art/Portraits/` (Pipeline, "Character portraits"): Ellis A The Revenant, B Came Home at the Last, C Dressed for
the Grave, rendered in Blender from MakeHuman's CC0 base, painted and framed; the frame generator; `sheet.js`. The
images aren't committed (PNGs go to Git LFS, which the cloud box can't push): remake them with the Pipeline's commands
(~8 min for all three).
**The user's decisions (keep):** Blender's `bpy` may be installed in the cloud box; MakeHuman's CC0 assets may be used
(`Art/ThirdParty/MakeHuman`); the code-only SDF busts were passed over for Blender; **Ellis is option A, The Revenant**
(picked 2026-10-15): cattleman hat low, red bandana up, cyan eyes burning as slits in the brim's shadow, collar-length dark
hair, a dust-brown duster with a fluted shoulder cape and the collar turned up, Saint Ada's sun-ring pin on the collar, a
black waistcoat with brass buttons.
**Next then:** build Ellis A as the in-game model (done in the session above).

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
