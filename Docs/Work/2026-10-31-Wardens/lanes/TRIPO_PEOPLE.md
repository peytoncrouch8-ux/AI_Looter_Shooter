# Lane TRIPO-PEOPLE: the six story people from Tripo, on their script rigs

The user's OK (2026-11-01): Tripo credits for "the story people", bodies only (no props). Done here: six Tripo models
made, each fitted onto its existing script rig (recipe 2 of `TRIPO_REDO_INVENTORY.md`, the Tripo Unpaid's), exported
without importing. Not done (lane rules): no Unreal, no import, no build, no commit, no tests run.

## Credits (cap 220; Tools/tripo.py counts per task in Art/Tripo/<Name>/task.json)

| Model | Task | Credits |
|---|---|---|
| Abel | text-to-model | 30 |
| Foreman | text-to-model | 30 |
| Amos | text-to-model | 30 |
| Hiram | text-to-model | 30 |
| Thaddeus | text-to-model | 30 |
| DeadMiner | text-to-model | 30 |
| **Total** | no retries, no repaints | **180** (40 unspent) |

Specs: `Art/Tripo/specs/story_people.json` (person false: fitted to our bones, not rigged by Tripo). Run:
`python Tools/tripo.py batch 180 story_people` (stopped before DeadMiner by its 40-credit reserve check), then
`python Tools/tripo.py make DeadMiner --no-rig`. Raw sheet: `Saved/ArtPreviews/TripoPeople/sheet_raw.png`.

What Tripo made (judged against the in-game Unpaid and Wardens Tripo models and `Saved/Screenshots/HudShots/01_calm.png`):
all six in the same painted "stylized realism" as the Tripo Unpaid and the Wardens crew; no grey-clay texture. Abel, Foreman,
Amos, Hiram came in a T-pose, Thaddeus and the Dead Miner in an A-pose. Everyone but the Dead Miner has legs and bare feet
(the prompt's "no legs, a shroud" was ignored): cut off, the script's shroud kept instead. All are bald (the hats cover it);
Amos, Hiram and the Dead Miner have long pointed ears (Tripo's ghost read; they show under the hats). Too-loud colours
(Amos's and the miner's blue skin, the miner's royal-blue neckerchief, Thaddeus's orange apron) were muted in the paint
itself (see Style), so no repaint was bought.

## How (Art/Models/Creatures/Tripo/parts_people*.py; models.ps1 skips parts_*.py)

Each person's script builds as before (rig, props, pose table and its checks, hit zones); a 10-line hook at its end, just
before the previews, calls `parts_people.swap('<Name>', globals())` (LOOTER_PEOPLE_TRIPO=0 keeps the script body).
- `parts_people_cfg.py`: per person: the GLB turned to face -Y, feet on z 0 ("raw"); joints measured by eye from ortho
  views (`Docs/Work/2026-10-31-Wardens/tripo_people/glb_views.py` -> `Saved/ArtPreviews/TripoPeople/<Name>_front/_side*.png`);
  the cut height under the hems; triangle budget; fists; grips; lower-body rule; zone prototypes; paint grade.
- `parts_people_src.py`: raw -> "upright" (shoulders to 1.43 m, hips to 0.91 m: the scripts' upright frame); the legs cut;
  **Tripo's own surface collapsed** to the budget (not voxel + QuadriFlow: QuadriFlow ran out of memory on the shared PC with
  the blown-up face, and a voxel surface's two close cloth sheets stopped the collapse at 75-90k triangles).
- `parts_people_rig.py`: a skeleton on the model's own pose (spine straight up with the rig's lengths, measured neck, head,
  jaw, arms, hands), bone heat for the arms (cleaned as the Unpaid's), the torso by height, head and jaw rules, the lower body
  by rule (Abel/Foreman: Abel.py's skirt_weights; Amos/Hiram/Thaddeus: Amos.py's thigh weights; Dead Miner: pelvis), Hiram's
  thighs laid standing so the map seats him; each rig bone's map from there to its rest (head scaled to the script head's
  chin-to-crown, hands scaled to the rig's wrist-to-knuckles); open hands' fingers onto the rig's finger bones after the warp;
  the lips parted with a pocket, Unpaid-style teeth on head and jaw.
- `parts_people.py` (swap): the head shrunk a little at a time until the script hat's crown covers it (8 tries, at most 8%);
  the fist moved onto its prop bone's head (Abel lantern and gun, Foreman pick, Amos fork, miner pick; 5-7 cm); the coal's
  hole pressed into the chest round the script's own coal; kept from the script body with their own colours and UVs: the
  coal (GhostCoal slot), the shroud (strip UVs, fade), Thaddeus's tape and its case; hit zones rebuilt by each script's own
  `hit_zones()` on the new skin (hulls padded 5-10 mm: bmesh hulls of a dense skin left points 4-6 mm out); the Foreman's
  satchel and strap widened x1.2/1.25 to sit on Tripo's fuller coat.
- `parts_people_look.py`: tint zones per face (bare skin by bones and colour, the rest nearest of three cfg colours); the
  paint: **Tripo's own colour map** (graded) packed into a 2048 atlas's lower left, swatches in the upper right (teeth,
  shroud, tape, case, and one per zone for props that share the body's slot); each face's UV 1 ('Paint') from where its
  corners lay on Tripo's surface (a face over a Tripo seam takes the island under its middle). Written to
  `Art/Textures/<Name>Paint/T_<Name>Paint_BC.png`.

## Materials (the masters / instances convention, as the Tripo Unpaid)

Every body keeps its script slot (GhostAbel, GhostForeman, GhostAmos, GhostHiram, GhostThaddeus, DeadMiner_A; and
GhostCoal) on **M_Ghost**; the importer sets the MI from the Blender material's properties: TextureSet `<Name>Paint`,
PaintAmount 1, PaintZone1-4 and Zone1-4Color = each zone's mean paint colour, and parts_unpaid_look.PAINT_LOOK (rim 0.8,
glow 0.03, top light 0.9-1.06, cloth weave/folds/stains 0.06/0.12/0.15, ColdRim 0.5) - the same light response as SK_Unpaid.
Props that share the body slot got a 'Paint' UV onto their zone's swatch so they draw as before: SM_AbelHat,
SM_ForemanBowler, SM_DeadMinerHat, SM_DeadMinerCap.

## Style (the user's 2026-11-01 note: everything from Tripo in the game's look)

- Prompts in the game's palette and finish: "stylized realistic, hand-painted look, matte cloth, muted warm frontier 1880s
  palette", colours named for every garment, negatives for glossy/plastic/toy/cartoon/photoreal/grey clay.
- Judged side by side with 01_calm.png (Ransom's Rest in game), the Tripo Unpaid pictures
  (Saved/ArtPreviews/RansomsRest/Unpaid/UnpaidTripo_*.png) and the Wardens sheet (Art/Tripo/sheet_batch1.png).
- The paint graded before it goes in the atlas (`parts_people_look.grade`): saturation over 0.45 compressed to 60%, and hue
  bands pulled further (Amos and the miner's blue skin to 45-50% saturation toward ashen grey, Thaddeus's orange apron to
  70%). No style_image repaint was needed (Tools/tripo.py has `paint_style` now if one is wanted later).
- Through M_Ghost with PAINT_LOOK, as SK_Unpaid: the same rim, glow and top light.

## Results (rest pose, the props on; Saved/ArtPreviews/TripoPeople/<Name>_rest/_face/_hand/_poses.png)

Side by side with the game and the Tripo Unpaid: `Saved/ArtPreviews/TripoPeople/people_sheet.png` (people_sheet.py).
Re-render one: `Toolsrtrun.ps1 -Script Docs\Work6-10-31-Wardens	ripo_people\people_check.py -ScriptArgs <Name>`
(`,script` for the old script body); the built scene is saved to `Intermediate/TripoPeople/<Name>_check.blend`.

| Body | Triangles | Vertices | Bones | Slots | UV | Hit zones | Head scale | Paint |
|---|---|---|---|---|---|---|---|---|
| SK_Abel | 20,475 (script 17,252) | 10,446 | 39 (same) | GhostAbel, GhostCoal | UVMap, Paint | 19 hulls + coal (+ lantern) | 0.958 | AbelPaint |
| SK_Foreman | 19,300 (script 11,915) | 9,965 | 38 (same) | GhostForeman, GhostCoal | UVMap, Paint | 18 + coal | ~0.81 | ForemanPaint |
| SK_Amos | 16,643 (script 13,391) | 8,548 | 36 (same) | GhostAmos, GhostCoal | UVMap, Paint | 16 + coal | 0.734 | AmosPaint |
| SK_Hiram | 16,592 (script 11,664) | 8,481 | 35 (same) | GhostHiram, GhostCoal | UVMap, Paint | 16 + coal | 0.702 | HiramPaint |
| SK_Thaddeus | 16,131 (script 10,750) | 8,361 | 35 (same) | GhostThaddeus, GhostCoal | UVMap, Paint | 16 + coal | 1.046 | ThaddeusPaint |
| SK_DeadMiner | 13,235 (script 8,306) | 6,923 | 34 (same) | DeadMiner_A, GhostCoal | UVMap, Paint | 14 + coal | 0.807 | DeadMinerPaint |

Bones, their rest places, the pose tables (Intermediate/<Name>Model/*_poses.json), sockets and props are the scripts'
own, untouched (the swap runs after them): no C++ change is needed. Poses checked in the renders: Abel's flare, shot, lunge,
sunset turn, kneel and sit; Amos's lean, sit and talk; Hiram's tube both sides and talk; Thaddeus's together, mark, knock,
look; the Foreman and miner by the Unpaid rig's test turns (arm up, arm out, jaw, chest bow).

## For Main

- Export (done by this lane): `Intermediate/ArtExport_TripoPeople` (`Tools\artrun.ps1 -Export Abel,Amos,Hiram,Thaddeus,
  Foreman,DeadMiner -Family TripoPeople`). Import with the editor open: `Looter.ImportModels C:/Dev/AI_Looter_Shooter/
  Intermediate/ArtExport_TripoPeople` (brings the six SK_ bodies, their props as before, the six `<Name>Paint` textures, the
  ghost MIs with PaintAmount 1).
- M_Ghost must already have PaintAmount / PaintZone1-4 (the Unpaid's 2026-10-31 patch; it is in).
- **MI_DeadMiner_B and _C** (the miners' other two outfits, also HiramCoyle's fallback tint) are separate M_Ghost instances
  without the paint: on the new body they draw flat zone colours. Make them paint-aware the way `build_creature_materials.py`
  `paint_clothes()` makes MI_Ghost_B/_C from MI_Ghost_A (A = DeadMiner_A, CLOTHES = DeadMiner.py's B/C), or the miners in
  B/C will not match A.
- Tests to run after the import: `Looter.Bosses.Abel`, `Looter.Bosses.AttackRules`, `Looter.Creatures.Foreman`,
  `Looter.Creatures.DeadMiner`, `Looter.Story.Amos`, `Looter.Story.DeadFriends`, `Looter.Story.Gravewind`,
  `Looter.Creatures.Unpaid.Model`, `Looter.Bestiary.Entries`; a PIE look at each in daylight and at night, and
  `Tools\perf.ps1` with a miner shift (the miners went 8.3k -> 13.2k triangles; LODs 50/25 kept).
- Commit list: `Art/Tripo/specs/story_people.json`, `Art/Tripo/{Abel,Foreman,Amos,Hiram,Thaddeus,DeadMiner}/`,
  `Art/Models/Creatures/Tripo/parts_people{,_cfg,_src,_rig,_look}.py`, the hooks in `Art/Models/Creatures/{Abel,Amos,Hiram,
  Thaddeus,Foreman,DeadMiner}.py`, `Art/Textures/{Abel,Foreman,Amos,Hiram,Thaddeus,DeadMiner}Paint/`, this file and
  `Docs/Work/2026-10-31-Wardens/tripo_people/` (check scripts: glb_views.py, people_check.py, dump_rig.py). CODEMAP has no
  Art lines for parts files; Docs/Pipeline/Enemies.md "Spider and Unpaid models" or CharacterModels.md wants a short
  "Story people from Tripo" paragraph (the How above, condensed).

## Risks (unchecked in Unreal)

- The script previews (`-Preview` on these scripts) still assume the script body's colours (RAW_COLORS is None after the
  swap): run them with LOOTER_PEOPLE_TRIPO=0, or use people_check.py.
- Hands: fists are moved onto their prop grips (5-7 cm) and look right at rest; in fast poses a haft may show through a
  finger. The Dead Miner's and Hiram's hands are Tripo's open claws (the code's finger curls drive them).
- Head scale: the heads were shrunk to fit the script hats (Amos 0.73, Hiram 0.70 of the chin-to-crown fit): a hatless moment
  (the Foreman's hat-off pose) shows a slightly small head.
- Amos's straw hat: a little of his forehead shows through the band in front.
- Shroud joins: the script shroud starts inside Tripo's hems; Amos's tube shows between the knees as before.
- Pointed ears (Amos, Hiram, miner) are Tripo's.
- Paint atlas: Tripo's map at 1987 px, seams handled per face; a few faces across a seam may stretch a texel run (not
  seen in the renders). LOD1/2 may smear small islands at range.
- Bone heat once failed on a dense mesh: an envelope fallback is in, unused in the final runs.
- Foreman's satchel widened: it is rigid on spine_02 as before; check it in his swings.
