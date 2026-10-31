# ART: the spider's and the Unpaid's models (2026-10-31)

Skeletons untouched (bone names, hierarchy, rest pose): the parts and weights changed only. Export test passes:
`Tools\artrun.ps1 -Export Spider,Unpaid -Family Creatures1031` (SK_Spider 22,816 tris, 31 bones, 23 hit zones;
SK_Unpaid 33 bones, 15 hit zones; SM_UnpaidHat UV UVMap + Paint). Nothing built, imported or committed.

## Causes found
- **Spider colours**: the Tripo build (`parts_spider_build.py`) kept the old scripted atlas (`BROWN` in Spider.py:
  near-black with pale bands). Tripo's own paint (warm tan, chevroned abdomen, rayed carapace, speckled legs with dark
  joint rings) was never used. In game: grey-black, flat.
- **Spider "spikes"**: no skinning stretch (every part is 100% on its bone). Tripo's legs are 114-159 cm, the rig's
  230-238 cm: they were stretched 1.5-2.6x along but only sqrt of that (max 1.4) across, and Tripo's bends between
  joints flattened by the stretch, so a leg read as a long thin straight pole. The rest is animation: the gait's IK lets a
  leg reach 0.999 of full length (femur and tibia in one line) and the bite holds the front legs straight up
  (`SpiderCreature.cpp` Raised (Hip.X+80, Hip.Y*1.5, 50)). For SPIDER/Main: cap a planted foot's stretch near 0.9 of
  reach (step sooner) and bend the raised front legs; the art can't hide a fully straight leg.
- **Unpaid washed out**: M_Ghost drew four flat pale zones (skin and robe #BAC4C6, shirt #93A5B2) with a near-white rim
  (RimStrength 2.2), its own glow 0.12 and painted top light up to 1.18; Tripo's paint only chose the zones. Tripo's
  model is blue-grey skin, cream shirt, dark brown waistcoat and tie, brown robe and hat.
- **Unpaid deforming** (measured, every edge posed vs rest, game poses + extremes): hands and fingers stretched 3-6 cm
  along themselves when the arm moved (bone heat gave a hand hanging by the hip a share of pelvis/spine_01, and fingertips
  were half hand, half finger); the jaw's weight stepped 0 to 1 in 7 mm past the mouth's corners. Shoulders, elbows,
  neck, robe: fine (shoulder top compresses at 100 deg out, normal for linear skinning).

## What changed (files I own; edited in place)
- `Art/Models/Creatures/Tripo/parts_spider_look.py` (new): Tripo's colour, roughness and normal map carried texel by
  texel onto the same atlas layout (templates of each part on Tripo's surface; nearest point; normal re-framed into our
  tangent space, DirectX green; colour relief bump 0.35; roughness capped 0.88). One leg (REP_LEG 2) fills the shared
  leg regions; left palp and chelicera theirs. Writes `Art/Textures/SpiderTripo/T_SpiderTripo_BC/_N/_ORM.png`.
- `parts_spider_build.py`: templates for the look; `material()`; legs thicker (`ACROSS` 1.0-1.65,
  `ACROSS_POWER` 0.75), Tripo's bends kept (`BEND_KEEP` 0.8), coxa keeps Tripo's girth at the body (death-curl check
  stays 1.0 cm < 3). `LOOTER_SPIDER_LOOK=0` reuses the maps (check runs).
- `parts_spider_shape.py` (Source keeps `maps`), `parts_spider_shells.py` (abdomen returns `shift`),
  `parts_spider_limbs.py` (docstring). `Spider.py`: Tripo build's MATERIAL = SpiderBody slot on set SpiderTripo. The old
  SpiderBody set is still painted (Gravemother's pale hide and the Spitter fallback use it).
- `parts_unpaid_paint.py` (new): body + hat unwrapped into one 2048 atlas on a second UV channel `Paint` (UV 1; UV 0
  unchanged for M_Ghost's grain and strip fade), Tripo's colour cast onto it (short cast 6/16 mm; misses and the robe's
  torn-hole darks refilled from neighbours), teeth/pocket/coal on a solid corner. Writes
  `Art/Textures/UnpaidPaint/T_UnpaidPaint_BC.png`.
- `parts_unpaid_look.py`: `paint_look()` sets Ghost_A: TextureSet UnpaidPaint, PaintAmount 1, Zone<N>Color =
  PaintZone<N> = the paint's zone means (#453F39 skin+robe, #8D8577 shirt, #383028 waistcoat+hat, #342D26 tie+band),
  RimStrength 0.8, GlowStrength 0.03, TopLight 0.9-1.06, cloth weave/folds/stains 0.06/0.12/0.15, ColdRim 0.5.
- `parts_unpaid_rig.py`: forearm/hand/fingers wholly the arm's; each finger 100% its bone from 1.2 cm past its knuckle
  (`FINGER_HOLD`); shoulder upper-arm weight smoothed (`SHOULDER_BLEND`); jaw eased over the cheeks past the corners and
  smoothed (`JAW_CORNER`, `JAW_CHEEK`). Measured after: hand/finger stretch gone (rake 59 -> 4 edges over 2 cm, reach
  197 -> 31, rest armpit/neck skin). Jaw edges still over 2 cm are inside the mouth's pocket (looks right: shriek face).
- `parts_unpaid.py`: calls the paint after the hat; docstrings.

## For Main, in order (editor open, nothing else needs C++)
1. `git apply Docs/Archive/Work/2026-10-31-Creatures/ART_build_creature_materials.patch` (M_Ghost: PaintAmount, PaintZone1-4,
   BaseColorMap on UV 1 when painted; `paint_clothes()` recolours MI_Ghost_B/C from A's paint means). Shared file, so a
   patch, not an edit. Other ghosts: PaintAmount 0 = unchanged.
2. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_creature_materials.py M_Ghost"` (new params on the
   master, before the import sets them; else the importer warns and skips them).
3. `Tools\models.ps1 -Only Spider,Unpaid` (~3.5 min; bakes both sets, imports SK_Spider, SK_Unpaid, SM_UnpaidHat, the
   new textures, MI_SpiderBody now on SpiderTripo, MI_Ghost_A with the paint).
4. Step 2 again (MI_Ghost_B and _C copy the new MI_Ghost_A).
5. Tests: `Tools\runtests.ps1 -Filter Looter.Creatures.Spider` and `-Filter Looter.Creatures.Anim` (hit zones, death
   curl, Unpaid arms/shroud); PIE `Looter.SpawnCreature Spider`, `Looter.SpawnCreature Unpaid` in daylight and at night,
   one Unpaid in each clothes set (BellRinger wears B), lunge/shriek up close.
- Docs: Enemies.md "Placeholder creatures"/Stone Sentinel section lines on the Unpaid look and the spider model are stale
  (paint now drawn); add the Tripo look notes from this README. CODEMAP: two new Blender modules
  (`Art/Models/Creatures/Tripo/parts_spider_look.py`, `parts_unpaid_paint.py`) if CODEMAP lists art scripts.

## Previews (Saved/ArtPreviews/Creatures/)
- Tripo's own look: `Fix1031/glb_spider_*.png`, `Fix1031/glb_unpaid_*.png`.
- Spider: `SpiderTripo/*.png` (official views tool, final) and `Fix1031/spider/` extremes: `x_reach_*` (legs at 0.97
  reach), `x_fold_q`, `x_run_*` (70 cm stride), `x_player_run.png` (eye height, wide lens, like the game shot).
- Unpaid (M_Ghost approximated: paint x cavity, thin cold rim, dithered fade): `Fix1031/unpaid/rest_*`, `lunge_*`,
  `shriek_*` (incl. `_face`), `rake_*`, `x_spread_*`, `x_reach_*`, `x_back_*` (incl. `_shoulder`, `_hand_l/_r`).
- Check scripts: `Docs/Archive/Work/2026-10-31-Creatures/art_checks/` (`spider_views.py`, `unpaid_views.py` [`--stats` prints
  edge stretch per pose, `--clay`], `render_glb.py <glb> <out prefix>`), run with `Tools\artrun.ps1 -Script ...`.

## Risks
- Step order matters: importing before the M_Ghost patch shows the paint as a box-projected grain and very dark zones.
- MI_Ghost_B/C on meshes without UV 1 would sample garbage paint; only SK_Unpaid and SM_UnpaidHat get B/C today
  (Unpaid, BellRinger, Thaddeus fallback) and both have UV 1.
- The Unpaid paint atlas has many small islands (smart projection on the remesh): LOD1/LOD2 reduction may smear a few
  seams; check at mid range. The robe keeps a few dark Tripo tear marks (read as rips).
- Spider legs are thicker (front legs 1.65x Tripo across): hit hulls grow with them (shots test passes).
- Not seen in the game's lighting yet: tune Ghost_A's RimStrength/TopLight or the spider's ROUGH_MAX after a PIE look.
- A stray empty folder `C:\Saved\ArtPreviews\Creatures\Fix1031` was created by a first render (relative path); its PNGs
  were moved into the repo. The removal was blocked by a safety check: delete it by hand.
