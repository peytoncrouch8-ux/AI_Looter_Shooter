# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Small leftovers", 2026-10-15, local)

Done this session (the user picked "small leftovers" first): `WeaponManagerComponent.h` split (its events and small
types in `Inventory/WeaponManagerTypes.h`, 491 lines); the editor-exit ensure on Hiram's figure fixed in all six
poseable-mesh constructors (Pipeline, Bugs); the shotgun reworked with the user watching: held 20 cm nearer, the left hand
cupping the pump with a new `EHandGrip::Pump`, the thumb along the pump (the user approved the close-up); live switches
for the support hand's finger and thumb curl; `play.py equip <n>`. Many other files are still over ~500 lines
(`CreatureBase.h` 562, `BossShrine.cpp` 568, `LooterUIStyle.cpp` 765, `LooterSoundCues.h` 847, several tests).

**Ellis is the player's body (the user's ask mid-session, done: 116316b, fdd3894):** the cloud branch merged (ffb56c2),
SK_Ellis built and baked here, imported; the mannequin stays the animated body, hidden, and Ellis copies it through
RTG_MannyToEllis (Pipeline, "Character models", "Ellis as the player's body"). The user's calls: MakeHuman's base.obj
fetched here; "retarget live" (not every clip retargeted onto him). Checked in PIE: third person from behind, standing
and running; front view aiming the rifle and the revolver (hands on the guns); first person (he's invisible to the
player). Tests 687/687; RR spawn 7.5 ms on Medium (was 7.7). The user saw the first two shots; not yet played.

**Ellis quality loop (the user, 2026-10-15: "it looks plastic, stiff and unlike a real fps character. Improve it until
I tell you stop and run all decisions by me"):** the user's picks: holster empty only while the revolver is drawn;
first-person gloves and sleeves match him; all four rounds (materials, coat and hair that move, pose and hands, remodel
the silhouette); budget raised to ~35k triangles. Done: round 1 materials (approved; 3512eba), the holster, FP arms matched
(17e2348), round 2 (39683fe, b26dbf0): swing bones + springs, the hat tipping back when aiming plus a 12% smaller brim
(the user's "both"), hands 15% thicker, the coat and cape looser, the bandana off his neck, facing +Y (the fingers now
auto aligned: no claws). Round 3 (an audit in Blender, `Saved/Concepts/EllisAudit/defects.md`): flipped faces (the
waistcoat's holes), the cape's wings and fin, gauntlets, collar, lantern hook, holster; hands kept out of the coat
(f9be3bf). Round 4, the user's picks: **"Apply C in full"** (squarer shoulders, pointed cape back, deeper hat crease, harder
brim curl, the coat to the ankle), **proper boots** (stovepipe on a real last), **face kept dark**, **the waistcoat as
cloth** (smooth black wool, brass buttons), **the coat's back hanging straight** (the user: "Back of the cape is deforming
in between his legs. It should be straight": the cloth simulated over a filler behind the legs, the vent closed). 34.2k
triangles. The springs now keep the bone between two points off the limbs (a stride's back boot came through the coat)
and keep the points over the ground (crouched, the hem sank 11 cm). Checked in PIE: back standing, walking, crouched;
front.
**Next in the loop (ask first, tap-to-answer):** the pose round (idle weight shift, stance). Still open: the hair reads
as ribbons up close; the crouch's holster and lantern touch the thigh; the collar's turned-up corner reads like a horn
from below; FBX import warns "Not valid bind pose" (recreated fine); the spin viewer artifact predates the bake here.

## Before that (session "Ellis 3D model", 2026-10-15, cloud; branch `claude/ellis-3d-character-model-tvwr9z`, merged)

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
gloves show, level (no forward tilt); Ellis's own third-person body LATER (the user asked for it on 2026-10-15: Now).

**Waiting on the user:** a playtest of the hands after d0d9d2e. Not seen in PIE: the support hand during reloads (tests
cover the paths) and the hands while aiming down sights.

## Next (in order)

1. The user's playtest notes first (Ellis in third person, the shotgun's new hold, hands, guns, swaps, water, spiders).
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
