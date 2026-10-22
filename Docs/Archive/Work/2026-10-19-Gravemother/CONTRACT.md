# The Gravemother's Barrow fight: the build contract (2026-10-19)

Why: the user picked concept D "Barrow" for the Gravemother (Ransom's Rest's Legendary monster, Side 3, in the Sink's
den) on 2026-10-15 and asked (2026-10-19) to build it alongside the hero work that's running. The concept sheets are the
spec: `Saved/Concepts/Gravemother/Barrow_Hero.png` (her look, crits), `Barrow_Moves.png` (every move in six timed
frames), `Barrow_FX.png` (every effect piece with sizes), `Barrow_Spiderling.png`; their source
`Art/Backlog/Creatures/GravemotherBarrowSheet.py` (docstring: the fight) and `GravemotherFightConcepts.py` (`barrow()`:
her dressing); renders and `BW.json` in `Saved/ArtPreviews/RansomsRest/Concepts/GravemotherBarrow/`. Today's fight:
`Source/AI_Looter_Shooter/Creatures/GravemotherCreature*.{h,cpp}`, `GravemotherFight.*`, `Docs/Areas/RansomsRest.md`
(Side 3, "Enemies by rank"). Rules: `CLAUDE.md` (read it).

## The user's calls (2026-10-15 and 2026-10-19; keep every one)

- **Upgrade in place:** her charge becomes the **Furrow**, her brood becomes **Unearthed**, her venom spit becomes the
  **Grave Clod**. The Gravequake (for one who hugs her), her roar as she rears, the staggers (crits close together), the
  boss bar, phases, loot shower and walking home stay.
- **Grave Clod:** front legs dig in and lift a clod overhead (the tell, **1.2 s**); she lobs it (about **1.5 s** to a
  player at 20 m; a landing shadow shows where); it bursts into dust and leaves a **rubble pile** (cover, **10 s**, then
  it settles away). **Shooting the clod while she holds it** bursts it on her and stuns her **2 s**.
- **Furrow:** a **2 s** tell: the ground heaves and splits in a line toward the player, dust jetting from the crack; then
  she ploughs the line head-low and throws up a **berm each side** (cover, **20 s**, then they crumble). Never off a drop
  or through a wall (as today's charge).
- **Unearthed:** at **66% and 33%** she slams the floor and **4 spiderlings** (the game's spider at 0.45x, as today's
  brood) dig out of **mounds that bulge for 1 s** first; **a mound shot while it bulges kills the spiderling in it**.
- **The headstone is breakable** (the user's pick over plain armour): it has its own health; shots on it hurt the stone,
  not her (sparks and chips fly). Broken, it cracks off and her pale Bone abdomen is bare. It also cracks off by itself
  at her 33% if it still stands.
- **Crits:** her head always; the abdomen's soft rear (under the stone's back edge) while the stone is on; the whole
  abdomen once it's off.
- **The last third (her 33%)** she runs **+25%** faster.
- Main's calls (the numbers the user didn't set): the stone's health is **20% of her maximum health**; it shows three
  states (intact, cracked at 66% of its health, shattered at 33%); breaking it early staggers her (the long stagger) but
  doesn't bring her last third early; her speed-up comes with the last third only.

## The look (the sheet's colour key)

Mud-grey hide #8C8070; headstone granite #8E8B86 (grey, not sandstone: sandstone vanished against the Sink's wall);
bare Bone abdomen #BAB2A1; grave mud #C9B392 caked on her legs; roots #5B4330 binding the stone on; fresh grave earth
#6E5C49 (clods, rubble, berms, mounds: darker than the sun-baked floor so they read against floor and wall); fresh-earth
dust #806B55; footfalls kick up the paler floor dust. Everything drawn must work at 1080p Medium without Lumen: meshes,
soft-alpha cards, decals, emissive surfaces; no volumes. **She stays Spider.py's mesh and rig** (SK_Spider's bones and
the game's spider code): the hide, mud, roots and stone are her own dressing.

## Who does what (disjoint files; nobody commits or changes git state: Main does)

**BARROW-ART (Blender + texture scripts; never opens Unreal).** Every Blender run through `Tools\artrun.ps1` (`-Threads
6`; it queues two runs at a time with four hero agents' runs). Your files:
- `Art/Models/Creatures/Gravemother.py` -> `/Game/Art/Creatures/SK_Gravemother`: Spider.py's mesh and rig (import or
  build it the way Spider.py does; never edit Spider.py), the same bone names, in her mud-grey hide with grave mud
  caked on the legs, root stubs on the abdomen and the pale Bone abdomen under where the stone sits. Her own texture
  set(s) `Art/Textures/Gravemother*/`. Hit zones as SK_Spider's, with the abdomen's soft rear its own zone (`Bone` of the
  abdomen; name it so code can tell it: `USP_AbdomenRear` or as Spider.py names its zones + `Rear`; say which).
- The headstone, as static meshes in `Art/Models/Creatures/GravemotherStone.py` (or one file per group, your call):
  `SM_GravemotherStone_0` intact, `_1` cracked, `_2` shattered, each with its roots binding it, the pivot where it meets
  the abdomen; `SM_GravemotherStoneShard_1..5` (the pieces that fall when it cracks off); `SM_StoneChip_1..3` (tiny, for
  hits). Write the stone's place on the abdomen bone (Unreal cm and degrees, in that bone's space) into
  `Intermediate/ArtExport_Gravemother/stone_attach.json` and your report.
- The fight's earth, `Art/Models/Props/GraveFight.py`: `SM_GraveClod_S/M/L`, `SM_GraveRubble` (a pile, ~1.1 m high, UCX
  hulls: cover), `SM_GraveBerm` (a segment ~2 m long, ~0.9 m high, UCX: cover; laid end to end along the furrow),
  `SM_GraveMound` (closed), `SM_GraveMoundBurst` (dug out), `SM_GraveDebris_1..5` (chunks thrown by bursts and the
  plough). Sizes from `Barrow_FX.png`. Nanite off on small moving pieces.
- Textures, script-made (no library art): `Art/Textures/GraveFight/make_grave_fight.py` -> `T_GraveFight_*.png`: a dust
  puff atlas 2x2 (fresh earth) and one for pale floor dust, a dust jet card, a spark streak, a flash; decal textures: the
  heave crack (tiling strip + end cap), the ploughed furrow strip, the impact scar, the landing shadow (soft disc), the
  mound's cracks. Then `Tools/Unreal/build_grave_fight_materials.py` (Main runs it): imports them to
  `/Game/Art/Effects/GraveFight/` and makes `MI_Decal_HeaveCrack`, `MI_Decal_HeaveCrackEnd`, `MI_Decal_Furrow`,
  `MI_Decal_ImpactScar`, `MI_Decal_LandingShadow`, `MI_Decal_MoundCracks`, `MI_FX_GraveDust`, `MI_FX_FloorDust`,
  `MI_FX_GraveDustJet`, `MI_FX_StoneSpark`, `MI_FX_GraveFlash`, the way the game's decal and FX materials are made
  (`build_decal_materials.py`, `build_stain_decal.py`, the `M_FX_*` the weapons use: find them and follow them).
- Previews of every piece (`--preview`-style renders into `Saved/ArtPreviews/Creatures/Gravemother/`) and export tests
  (`artrun.ps1 -Export Gravemother,GravemotherStone,GraveFight -Family Gravemother`).

**BARROW-SOUND (script-made sounds; never opens Unreal).** Read `Art/Sounds/README.md` (the format, "Add a cue", how the
sounds are built, what's learned) and the Gravemother's recipes today. Your files: `Art/Sounds/recipes/` (a new
`gravemother_barrow.py` or the boss recipe file the Gravemother's cues are in), `Art/Sounds/cues.json` (your cues only),
renders in `Art/Sounds/Out/`. Render with `Tools\sounds.ps1 Boss.Gravemother.* -Jobs 2` (other agents share the CPU);
listen through the check's pictures and numbers (`Saved\SoundCheck`). New cues (exact names; 3D at her or the piece
unless said):
- `Boss.Gravemother.ClodDig` (the tell: legs tearing into the floor, the clod wrenched up, 1.2 s),
  `Boss.Gravemother.ClodThrow` (a heave and the clod's whoosh), `Boss.Gravemother.ClodBurst` (it lands: a deep thud,
  a dirt burst, rubble rattling), `Boss.Gravemother.ClodShot` (shot in her grip: the burst on her and her pained
  shriek),
- `Boss.Gravemother.StoneHit` (a bullet on granite: a hard crack and chips; short, several variants: it plays a lot),
  `Boss.Gravemother.StoneCrack` (the stone reaching a cracked state), `Boss.Gravemother.StoneBreak` (roots snapping,
  the slab splitting and falling: the fight's big moment),
- `Boss.Gravemother.Heave` (the furrow's 2 s tell: the ground heaving and splitting, rising, dust hissing from the
  crack), `Boss.Gravemother.Plough` (the charge down the line: earth rushing, legs drumming), `Boss.Gravemother.BermCrumble`,
- `Boss.Gravemother.MoundBulge` (earth stirring, scratching under it, 1 s), `Boss.Gravemother.MoundBurst` (a
  spiderling digs out), `Boss.Gravemother.MoundPop` (a bulging mound shot: a muffled burst, the earth caving).
Her Roar, Slam, Quake, Death and CrackBurn stay; Spit and Gurgle go unused once the clod replaces the spit (leave them;
Main removes them later). Main imports the bank (`build_sound_bank.py`).

**BARROW-CODE (C++; the game module).** Phase 1, now: read the Gravemother's code, the spider's (`SpiderCreature*`),
`UBossComponent`, `AGroundCrack`, the creature hit/crit path, the FX systems the game already uses (dust, impacts,
decals, chunks) and the sheets; write the design in `Saved/Gravemother/PLAN.md` (classes and files, what changes in
which file, how each move runs tick by tick, the stone's health and states, hit zones read, the asset paths and cue names
above, tests); **do not edit Source in phase 1** (the SELECT agent is building the module now; a half-written file
breaks its build). Report and stop. Phase 2, when Main says the editor is free: implement it, build
(`Tools\launch.ps1 -Build`), add the cue constants to `Audio/LooterSoundCues.h` (the Gravemother block), tests
`Looter.Gravemother.*`, check the fight in PIE (`Looter.SpawnCreature Gravemother`, `slomo`, screenshots you look at),
`CODEMAP.md` lines, `Docs/Areas/RansomsRest.md` (Side 3 and the boss) and a Pipeline note. Assets that aren't imported
yet load by path and are skipped when missing (the fight must run on today's SK_Spider until SK_Gravemother lands).

## Done means

ART: every mesh and texture above built, previewed (looked at), exported without error, the attach json written.
SOUND: every cue rendered, mixed and checked, nothing clipping, each cue reads as its words. CODE: built, every
`Looter.*` test passing, each move seen in PIE (clod lob and its counter, furrow and berms, unearthed mounds and a mound
shot, the stone's three states and its crack-off, the last third's speed). Each agent reports in under 40 lines: what's
done, files, every decision made, what's left, what Main must run.
