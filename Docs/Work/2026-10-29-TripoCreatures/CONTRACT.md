# Tripo creatures + effect polish + playtest package (2026-10-29 night)

The user: "Both models are in, they need rigged, animated and some of the textures aren't right so they need refined and
polished. Quads should be polished and refined as well. Package once complete." Asked: "quads" = BOTH the two models' mesh
topology AND the effect sprites (the flat particle quads); package = a Development playtest build.

Sources (the user's Tripo exports, paid plan): `Art/Concepts/Creatures/StoneSentinel/Stone Sentinel.glb` (29 MB),
`Art/Concepts/Creatures/Unpaid/Unpaid.glb` (17.5 MB). Earlier Tripo lessons: Docs/Pipeline/Assets.md "Tripo parts" and
"Tripo melee tools" (the user's quality bar: crisp textures, worn edges, clean shapes, no lumps/holes, our style, nothing
reads as AI-made; Tripo's maps re-baked with Tools/Blender/looter_rebake.py; turned parts faceted -> rebuilt; decimate by
edge collapse; tangent checks).

## Lane S: the Stone Sentinel (Opus)
- Swap the stand-in for the Tripo model inside `Art/Models/Creatures/StoneSentinel.py` (an env switch like
  LOOTER_BULLPUP_TRIPO: LOOTER_SENTINEL_TRIPO, default on; 0 = the script stand-in), keeping EVERYTHING lane D's code
  relies on: asset names SK_StoneSentinel + SM_StoneSentinelPlinth (plinth unchanged), the 17 bone names + the
  SOCKET_Head/FistL/FistR bones, the rest pose = the dormant statue pose (hands clasped, head bowed: pose the rigid pieces
  into it before export, since Creatures/StoneSentinelCreaturePose.cpp builds poses as offsets from the rest), ~2.37 m tall,
  hit hulls per bone, LODs 50%/25%, no Nanite.
- Mesh: clean it (holes, lumps, inside faces), retopologize to clean, even quad-dominant topology (Blender's built-in
  QuadriFlow or careful decimation: no new programs), ~15-25k triangles; cut into RIGID pieces at the joints (stone moves
  stiffly), each piece weighted 100% to its bone, cut faces capped.
- Look: our stylized stone (granite set through MI_StoneSentinel / MossMI as the stand-in did, or Tripo's map re-baked into
  our style with looter_rebake.py: crisp, painted, moss in folds, cracks, iron staples readable), no AI-looking smears.
- Check: the preview sheet + one render per pose (dormant, waking, walk, slam wind-up, slam) by posing the bones as the C++
  pose does (read StoneSentinelCreaturePose.cpp); Read your renders and fix. Export only (Tools\artrun.ps1 -Export
  StoneSentinel -Family StoneSentinel); Main imports.

## Lane U: the Unpaid (Opus)
- Swap the Unpaid's body for the Tripo model without changing what other creatures inherit: `Art/Models/Creatures/
  Unpaid.py` is a LIBRARY for DeadMiner, Foreman, Hiram, Thaddeus, Abel, UnpaidBell, Amos (looter_ghosts / looter_miners /
  looter_friends run it up to its build line). Add the Tripo path only for SK_Unpaid's own build (LOOTER_UNPAID_TRIPO,
  default on), never changing the library functions' outputs.
- Keep everything the C++ relies on: SK_Unpaid's bone names (UnpaidRigBones, Rig in UnpaidCreatureRig.cpp,
  DefaultGame.ini), the rest pose (the game idle), the hat as its own SM_UnpaidHat (cut the Tripo hat off cleanly onto its own
  mesh, attach point as today), the coal weak spot (its bone/socket and material slot: the coal glows from code: read
  UnpaidCreatureLook.cpp), material slots M_Ghost expects (the ghost fade, rank look via custom primitive data, clothes
  slots), PA_Unpaid's bodies and the arm guard points (UnpaidCreatureArmGuard.cpp), size within a few % of today.
- Mesh: clean, retopologize to quad-dominant deforming topology (edge loops at elbows, wrists, fingers, neck, waist; the
  shroud tongues with enough loops to bend), ~15-25k triangles; skin to the rig with smooth weights (automatic then fixed:
  shoulders, fingers, shroud chain bones), no candy-wrapping at the wrists; the face simple and stylized.
- Look: our stylized look (rebake into the Unpaid's material scheme; pale skin, faded clothes; the coal's cracks clear); fix
  any Tripo texture faults (smears, baked lighting, seams).
- Check: render in rest pose + the code's poses (read UnpaidCreatureRig.cpp / UnpaidCreatureMoods.cpp for the shriek, lunge,
  rake), side by side with the old model; Read and fix. Export only (artrun -Export Unpaid ...); Main imports.

## Lane F: effect sprite polish (Opus)
- The pooled FWeaponFX particles (Weapons/WeaponFX*.cpp, WeaponFXDraw.cpp, the M_FX_* masters built by
  Tools/Unreal/build_vfx_materials.py, M_FX_Shape stand-ins): make them read less flat: soft edges, depth fade where
  supported, velocity stretch for sparks, random rotation and per-particle shape variety for flashes/puffs/embers, better
  falloff on glows, crisper smoke shapes. Keep the budget (no more particles; cheap material ops; 120 fps target on the RX 580
  at Medium, Docs/Performance.md). The lessons in Docs/Pipeline/Gunplay.md "VFX pass" apply (additive washes white past ~2-3,
  Nanite can't draw additive). Tests: keep every Looter.Vfx* / Weapons.* FX test passing.

## Rules for every lane
Never build the game, open/close the editor, commit, or edit CODEMAP.md/Docs. Blender only via Tools\artrun.ps1. Only your
own files. Files under ~500 lines. Comments say why. No new programs or downloads. Report: files changed/added, export
command + output paths, render paths, numbers (triangles, bones, slots), anything unsure.

## Lanes W, V, G: the three spiders (added 2026-10-29 night; the user: "all 3 spider types are in, they need the same
treatment ... rigged, animated and clean quads. Keep in mind, these spiders have different leg amounts and sizes and they
generated with a tilt so fix this in play")
Sources: `Art/Concepts/Creatures/Meadow Spider/Meadow Wolf.glb`, `Art/Concepts/Creatures/Spitter Spider/Spitter.glb`,
`Art/Concepts/Creatures/Gravemother/Gravemother.glb`.
- Same treatment as lane S/U: clean, quad-dominant topology (~12-25k tris; the Gravemother up to ~35k), our stylized painted
  look (fix Tripo's texture faults), LODs, hit hulls per bone, export only (Main imports).
- The rig the game drives (ASpiderCreature's gait, two-bone leg IK, attack rear, death curl) needs EXACTLY 8 legs on the
  spider skeleton: bones body, head, palp_l/r, fang_l/r, abdomen, femur_/tibia_/foot_<0-3>_<l|r> (Spitter adds sac). If a
  model came out with more or fewer legs, fix the mesh: remove extra legs cleanly (cap and repaint the hip), or rebuild a
  missing one from its mirror. Report each model's original leg count.
- The tilt: level each model (body level, legs symmetric, front along Blender -Y) BEFORE rigging, and stand it in the
  skeleton's rest pose solved by the game's IK (knee_pole), the origin on the ground under the thorax, so it stands level
  in play. Verify by a side render against the ground plane.
- Sizes: each kind keeps its in-game size (SK_Spider as today; SK_Spitter as today; SK_Gravemother modelled at the spider's
  size, drawn 1.8x by the game): scale to match today's leg reach and ride height (read the bones' lengths from the current
  scripts) so the IK and hit zones keep working.
- Library rule: Spider.py's parts code (above its "# --- The rig ---" line) is run by Spitter.py, Gravemother.py,
  GravemotherStone.py and looter_barrow: never change what those read. Each swap behind its own env switch
  (LOOTER_SPIDER_TRIPO / LOOTER_SPITTER_TRIPO / LOOTER_GRAVEMOTHER_TRIPO, default on).
- Gravemother: if her Tripo model carries the headstone, cut the stone off as its own mesh (it cracks off in her fight:
  GravemotherStone.py / the game's stone actor; match its slot names and attach), cap and paint the bald root-scarred patch
  under it; keep her slots (GravemotherAbdomen etc.) and the soft-rear sphere on the abdomen.
- Spitter: the sac on its own bone `sac` with the Gel-master slot (MI_SpitterSac) as today, so the game swells and lights it.
- Lanes run in parallel: W = Meadow Wolf (Spider.py), V = Spitter (Spitter.py), G = Gravemother (Gravemother.py,
  GravemotherStone.py). Never edit another lane's script.
