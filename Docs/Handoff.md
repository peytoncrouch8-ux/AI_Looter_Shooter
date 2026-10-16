# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Small leftovers", 2026-10-15, local)

**The saga (the user, 2026-10-15 night: a long-running Borderlands-like story; "Why is Ellis here? Where does the story
go? What's the goal? Who's the final boss? What is the world lore?"):** the campaign already existed (Revenant); the user
chose to deepen and scale it (15–18 regions) and seed it into Ransom's Rest. Written: the bible page for the user
(https://claude.ai/artifact/Gr66uUTXWzetBTaHvE9j9Z) and the same canon in `Docs/Story.md`, marked *(saga)*: the
saints as souls who refused to cross, the spark of Ada in Ellis, Sexton's false lighthouse (the Act II turn), the Well on
Skyreach, three new regions (Halfway Junction, Hoarfrost Pass, the Sundown Road), the cloud heroes as companions, and
the list of seeds to build in Ransom's Rest (Story.md, "Seeds in Ransom's Rest"). The user's calls (Decisions 15):
the *Lily* an airship, the false lighthouse, all three new regions, and **five playable heroes picked before the story
starts, like BL2**; a non-Ellis pick **takes Ellis's place** (one story for everyone; heroes not picked don't appear;
Story.md "The heroes"); the pick is built **after the Rest's seeds, before the *Lily***. All calls in. **Next, in
order (the user, revised 2026-10-15 night):** the ember-power system with Ellis's three (Dust Devil, Sundown, Borrowed
Time; one equipped at a time, all three owned from the start; Story.md "The heroes"), then the hero pick (select
screen, four game models, their twelve powers, line variants), the Rest's seeds alongside (Story.md "Seeds in Ransom's
Rest"), then the *Lily* as an airship.
**Overnight run (2026-10-15 night; the user: "Start building. I am going to bed... don't ask anymore until I prompt
again"):** the user's last calls: power key **Q** (rebindable), the HUD socket **beside the weapon column,
bottom-right**, the swap UI **its own inventory page as the second tab** (the Ledger third), and after the powers
**plant the seeds in Ransom's Rest** (looks that need approval are left as look sheets). The powers session is running
unattended and records its decisions in this file under "Decided unattended"; Main starts the seeds session when it
ends, also unattended. No questions to the user until they prompt.

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
triangles, 113 bones (each swing chain ends in an unweighted `<chain>_end`; a 7th skirt chain `skirt_bm` down the vent).
The springs: the bone between two points pushed out the body's side of a limb, the points kept over the ground with the
hem gathering there, nothing rising over what it hangs from (Pipeline, "Swing"). Audit: the stride's boot through the
coat 53 -> 34 points, the crouch's hem under the ground 11.7 cm -> 4.9 cm. Checked in PIE: back standing, walking,
sprinting; crouched front and back. Shots sent to the user (round 4 before/after, in game).
**Next in the loop (ask first, tap-to-answer):** the pose round (idle weight shift, stance). Still open: a full stride
still shows the shin below the knee through the coat's back (34 points) and the left hand sits 4.9 cm into the coat's
front in it; the cape's corners lift with the arms when sprinting (CapeFollowFront/Back); the hair reads as ribbons up
close; the crouch's holster and lantern touch the thigh; the collar's turned-up corner reads like a horn from below; FBX
import warns "Not valid bind pose" (recreated fine); the spin viewer artifact predates the bake here.

**After round 4 (the user's notes, 2026-10-15):** "When crouching should his knee be touching the ground?" answered:
no, the crouch is a squat on both feet (code pose, LooterStancePose); the idle stance's right foot is back so that knee
comes down to about 20 cm (the left 34). "cape still clips through the boots and shins": foot limbs and the ring
(Pipeline, "Swing"). "His wrists are either invisible or don't match the gloves": forearm twist bones + FBodyForearmTwist
(Pipeline, "The forearms"), the glove's cuff up into the gauntlet, the sleeve stopped inside it (round 4g; 117 bones,
34.3k triangles); checked in PIE, before/after sent. Left: a small crease at the right gauntlet's lip by the thumb when
the hand is turned and bent.

**The Gravemother fight concepts (the user, mid-session 2026-10-15):** "generate a sprite sheet for the grave mother
boss ... projectiles, the spiders ground charge, the spider itself, attack particles, etc. ... give it some character and
feel like a real boss fight. Just generate 3 to 5 concept arts for now, we will turn them into game assets once I choose
the ones I like." Four sheets made and sent (`Art/Backlog/Creatures/GravemotherFightConcepts.py`, sheets in `Saved/Concepts/Gravemother/`
by `GravemotherFightSheets.ps1`): A Bone Mother (brood riding her, milk spit, grave plough), B Vigil (chapel candles as
her phase meter, corpse-lights, a procession of ghost-flames), C Shroud (ivory and oxblood, burial silk, net shot,
drag-lines, egg sacs), D Barrow (the bold one: a headstone shell, grave clods, a furrow charge that leaves cover,
unearthed spiderlings). **The user picked D, Barrow (2026-10-15)**, to build after the Ellis work they asked for first.
The user's review ask ("Make it better"): the real sprite sheets are made and sent (`Art/Backlog/Creatures/
GravemotherBarrowSheet.py` + `GravemotherBarrowSheets.ps1`; `Saved/Concepts/Gravemother/Barrow_{Moves,FX,Hero,
Spiderling}.png`; renders and `BW.json` in `Saved/ArtPreviews/RansomsRest/Concepts/GravemotherBarrow/`): every move
as 6 timed frames, the FX pieces isolated with alpha and sizes in cm (clod x3 sizes, dust burst cards at 0.1/0.4/1.0 s,
debris x5, rubble, the heave crack strip + end cap, dust jets, berm segment, mound closed/burst, stone chips, sparks,
stun stars; plus a landing-shadow decal, a furrow decal, fresh-earth dust #806B55 vs pale floor dust), her turnaround,
the spiderling (135 x 132 x 57 cm). The headstone is grey granite (#8E8B86), not sandstone (it vanished against the
Sink's wall). Not numbered yet: her faster run after 33%. Design calls made by Main for it (the user
hasn't confirmed them; ask if they object): the berms stay 20 s then crumble; the headstone is armour that deflects
(sparks, chips; no health), cracking off at 33% to bare the Bone abdomen; crits head + the abdomen's soft rear, the
whole abdomen after 33%; Grave Clod 1.2 s tell, shot in her grip = burst on her + 2 s stun, a 1.5 s lob to 20 m, a
rubble pile 10 s; Furrow 2 s heave tell then the plough; 4 spiderlings at 66% and 33% from mounds that bulge 1 s
(shootable).

**The user's next picks (2026-10-15, tap answers):** "Both 1 and 2": Ellis's leftovers (all four: the shin through the
coat's back, the cape lifting when sprinting, the hair reading as ribbons, the crouch's holster and lantern) and the pose
and idle round. Idle: **"Weary revenant"** with **breathing, glances around, hand fidget** (not the hat touch); the armed
stance **kept as it is**. Built: the idle (LooterStancePoseIdle.cpp; Pipeline, "The idle"), the cape's corners following
an eased arm (BodySwing, `FollowEaseSeconds`), the ring order for two new back chains, the lantern's `Bulk` (2.5 cm
more room off the limbs); round 5 in Ellis.py: `skirt_bli`/`skirt_bri` (a fifth of the way from each back chain to bm;
the stride's shin through the coat 28 points -> 0), hair as closed lens-shaped locks in two layers (5.8k triangles; still a
little chunky up close from the back), the holster pouch on the left thigh (crouch contact 3.6 -> 2.2 cm; the gun now
grazes the trousers 0.9 cm in a stride), the gauntlet lip rolled out; 125 bones, 34.9k triangles. Checked in PIE: idle,
sprint from behind, hair, crouch front and right side. Shots sent. Tests 698/698.
**Round 6 (the user on round 5's shots, 2026-10-15: "Cape clippings, hair looks like mud, can see through the
head/hat, hat doesn't seat on his head properly, don't stop until you get it right"; then "Run multiple agents if you
need to"):** three Blender agents on one contract (`Saved/Concepts/EllisRound6/CONTRACT.md`): HAT (seat the band on
the skull, close the crown's inside and the scalp), HAIR (cards with a script-made strand texture, masked; maybe a new
M_CharacterHair master), CAPE (cape and collar clipping in every pose). Found: every Ellis material is one-sided, so
a single surface seen from behind vanishes (the crown's inside). In-game evidence: `Saved/Screenshots/Tools/aud_*.png`.
**Round 6 done (2026-10-15 night):** HAT seated the hat and closed the head (`ellis_hat.py`); HAIR made cards on a
script-made strand atlas with a new master `M_CharacterHair` (`ellis_hair.py`, `Art/Textures/EllisHair`); CAPE rebuilt
the cape's weights and chains (9, hung from the shoulder joint, `cape_arm_<side>` idle arms) and lined the collar
(`ellis_cape_audit.py`), with BodySwing's follow shares, idle-arm rests, rising chains and cape bulk 6 to match
(Pipeline, "Swing"). 142 bones, 33.2k triangles. Checked in PIE from 20 head/body angles plus carry, aim (5 angles) and
sprint: no see-through, the hat on the head, hair reads as hair, no cape in the sleeves. The hair glowed light brown
backlit: MI_EllisHair Tint 0.55 (also in Ellis.py's EllisHair slot so a re-import keeps it). Shots sent. Tests
698/698; perf at the RR spawn 7.6 ms on Medium (as round 2; game thread 3.7 -> 3.9 ms for the bones and chains, GPU
unchanged). Committed 0ad1801. Cleanup later: `Saved/Concepts/EllisRound6/cape/wip` (1.7 GB of the agent's trials).
Main imports, checks in PIE, commits. Then Gravemother. Found on the way (HAT): the game skin was a "crystal":
`make_lows` spared everything within 3 cm of the eyes, so the skull's back and top collapsed into ~12 triangles (the
holes); now decimated by region. Leftover for later: BandanaKnot sits inside the neck (mid-neck, not at its back) with
36 points of the bandana's lower side edge: invisible, wasted triangles; move the knot to the nape or drop it.
**Next:** the user's notes on round 6; then the Gravemother, Barrow (D) as the real fight (concepts first done: model,
projectile, charge, FX, sounds; ask the user the open calls with tap-to-answer choices).

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
