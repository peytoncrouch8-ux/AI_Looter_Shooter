# CHAR-ART: characters audit and plan (2026-10-22, phase 1: audit only, nothing changed in the game)

Why: after playing Ransom's Rest the user named "characters" one of the weak art areas (Handoff "Direction"). Freeze
rules hold: no new characters, fix what's there. Look stays textured "stylized realism" (the user hated faceted
low-poly; style trials ended 2026-10-08: no outlines, no new style). Portrait picks are the user's: Ellis A, Crane A,
Pike A, Gauge C, Odessa A.

Concept sheets (paint-overs of in-game grabs; scripts in the session scratchpad `charart/`, node + Playwright, not kept):
- `Saved/ArtPreviews/Characters/Concepts/CharArt_1_Unpaid_PaintedGhost.png` (item 1)
- `Saved/ArtPreviews/Characters/Concepts/CharArt_2_HeroIdles.png` (items 2, 3)
- `Saved/ArtPreviews/Characters/Concepts/CharArt_3_Gauge_Enamel.png` (item 4)

## The bar (principles only, nothing copied)

Big-studio stylized shooters' characters read because: (1) a silhouette you can name at 30 m (hat, shoulders, prop,
stance); (2) three value groups, face and hands the lightest near the darkest, so the face is the focal point; (3)
light painted into the textures (top-light gradient, edge highlights, cavity dark, wear), so they read under any
lighting, Medium included; (4) a stance and idle per character (posture tells who they are); (5) hands that rest
curled, never flat; (6) glossy parts with something to reflect.

## What exists, character by character (image it's seen in)

Heroes (MakeHuman-based, baked looks, retargeted from the mannequin; Gauge hard-surface):
- All five: one shared idle, Ellis's weary revenant (`LooterStancePoseIdle.cpp`), so all stand alike; idle hands open
  with straight, splayed fingers (`Saved/Screenshots/Tools/hero_front_*.png`, Crane and Pike worst). Armed hands are fine
  since the 2026-10-20 grip work (`HeroGuns/*_grip.png`).
- Ellis: strongest. Duster, waistcoat and trousers all near black: at range one dark mass, the red bandana the only
  read (`hero_front_Ellis.png`). Gauntlets read as plain blocks.
- Crane: face a flat green with no value structure (cheek, brow, nose tip); mustache thin vs the portrait's big white
  one; hair cards end in flat slab tips; mustache cards stiff spikes (`Characters/Crane/Crane_face_three.png`); navy
  coat + dark trousers merge (`hero_front_Crane.png`); daylight turns his skin yellow-green (`HeroGuns/Crane_grip.png`).
- Odessa: face smooth and mannequin-like (soft planes, no painted shadow); glow eyes flat white ovals
  (`Characters/Odessa/Odessa_face_three.png`); otherwise reads well (brocade, coat).
- Gauge: picked as black enamel + polished brass (portrait C); in game the enamel is matte black (nothing to reflect
  on Medium, no Lumen), brass reads as yellow lines and dots, the lamp a flat white (`hero_front_Gauge.png`); limbs read
  as plain cylinders and boxes, toy-like (`Characters/Gauge/Gauge_full_three.png`).
- Pike: best match to his portrait. Glow behind the spectacles flat white ovals (`Pike_face_three.png`). The black
  pocket patch and waistcoat holes in `hero_front_Pike.png` (05:19) predate the 05:20 bake fix; gone in
  `HeroGuns/Pike_after.png`. Beard tips pick up the neckerchief's teal (`hero_front_Pike.png`): check in PIE.
NPCs and the dead (older pipeline: vertex-colour zone tints on M_Ghost, no textures, posed in code):
- The Unpaid (+ ranks), Abel, Amos, the Foreman, dead miners, Thaddeus, Hiram, the Bell-Ringer: flat tints, no fold,
  weave or top light, read as plastic; shroud ends cut into hard floating shards; smooth rubber hands; rectangle patch
  decals (`CastShots/after/232_unpaid-*`, `373_abel-*`, `361_amos-level_lean_front.png`, `Tools/foreman3.png`). The
  biggest gap next to the heroes, and the most-seen enemy.
- Mister Sexton: reads as a flat black cut-out, face lost (`CastShots/after/355_sexton-*`).
- Hob: reads well (`370_hob-*`). Tilly and Delia aren't seen as figures in the grabs (window, door): nothing to fix.
Creatures:
- Spider family (spider, ranks, spiderling, Spitter, Gravemother): leg segments are cylinders with flat cut caps
  showing at every joint (`CastShots/after/001_spider-*`, `Creatures/Gravemother/Gravemother_three.png`); body reads
  well. Gravemother in the cast grab reads as a grey re-tint of the spider (stone not visible in `133_gravemother-*`:
  check its attach).
- Slime: gel fine; the core inside is faceted low-poly (`166_slime-*`).
- Placeholders (clay domes, `APlaceholderCreature`): by design (the user's call); not in scope.

## Ranked fixes (biggest gain per cost first)

Cost: S under half a day, M about a day, L several days. **USER OK** = changes a look the user approved; FREE = no
change to an approved look (a fix, or brings a model back to its pick).

1. **Painted ghost material** (Unpaid family, 8+ characters at once). M_Ghost gets a script-made cloth set
   (`T_GhostCloth`, 1024: weave, seams, folds, stains; numpy like `ellis_hair.make_textures`) on the box UVs every
   ghost has, multiplying the zone tints (colours kept); a top-light gradient by body height; a cyan Fresnel rim; inner
   glow by distance to the coal in the rank colour; cavity (G) pushed harder; fade (A) broken by rising streaks,
   dithered, so no hard shard ends. Where: `Tools/Unreal/build_creature_materials.py` (M_Ghost), new
   `Art/Textures/GhostCloth/` script, `Unpaid.py` Ghost_A slot (the others reuse it). No mesh change; one texture
   sample more. Cost M. **USER OK** (Unpaid option D, 2026-10-06; shapes and colours kept). Sheet 1.
2. **Idle hands rest curl** for every hero: a base curl (~35/45/30 degrees, thumb in) under the idle's finger flexes
   in `LooterStancePoseIdle.cpp`; the retarget copies it. Cost S. FREE. Sheet 2.
3. **A stance per hero** (Crane stooped with a hand on the toll chain, Odessa hip cocked with a card, Gauge wide and
   planted with steam, Pike thumbs in his straps looking off; Ellis keeps the weary revenant): an `FHeroIdle` table on
   `UHeroDefinition` (`create_ember_assets.py`) read by `LooterStancePoseIdle.cpp` (lean, hip cock, head tilt, arm rest
   targets by the belt hook's two-bone IK, prop hand). Shows on the select stand and in third person. Tests: extend
   `PlayerIdleTests`. Cost M-L (1-2 days with a PIE look per hero). **USER OK** for the four new stances. Sheet 2.
4. **Glossy parts read on Medium + Gauge back to pick C**: a painted reflection term in M_Character for glossy slots
   (reflection vector up/down picks a sky / horizon / ground gradient, x Fresnel x (1 - roughness); no capture, no
   Lumen), Gauge's looks (`Tools/Blender/gauge_looks.py`) bake a faint top-light gradient and edge wear into the enamel,
   GaugeLamp tinted to the spec's #FFE2A8 and toned down. Others gain (Ellis's revolver and buckles, Pike's goggles,
   Odessa's buttons). Cost M (material + Gauge rebake ~25 min). FREE (restores the pick); check the other four still
   read as approved. Sheet 3.
5. **Hero value pass** (three value groups, the face the focal point): Ellis's trousers and waistcoat a step lighter
   than the duster, the duster's edge wear and seams lighter; Crane's coat lifted off his trousers, his skin given
   painted structure (cheek and brow shadow, warmer nose and lips, the daylight yellow cut), mustache and brows fuller as
   in portrait A; Odessa's face sharper planes (cheekbone, jaw) and painted shadow. Where: `Tools/Blender/looter_looks.py`
   recipes, each hero's `GAME_MATERIALS`, rebake (`--rebake`), reimport. Cost M per hero (L for all). **USER OK**
   (approved models and materials): show a before/after per hero first.
6. **Crane's hair and mustache cards**: port Ellis's round-6 card method (`Tools/Blender/ellis_hair.py`: layered cards
   on a script-made strand atlas, tapered tips) to Crane's hair, mustache and brows; same for Pike's beard if his tips
   still show teal. Cost M. **USER OK** (moves toward the portrait, but changes an approved model).
7. **Spider leg joints**: knuckle balls and overlapping sleeves at each joint, segments tapered, a few hair tufts on
   the silhouette (`Art/Models/Creatures/Spider.py`; Spitter and Gravemother build on it). Cost S-M. FREE (same design,
   a modelling fix).
8. **Gravemother reads as the boss**: check the stone attaches in game, then a bigger, paler body (bone and moss), the
   stone higher, a lower, wider stance (`Gravemother.py`, `GravemotherStone.py`). Cost M. **USER OK**.
9. **Mister Sexton's value**: a lit face (paler skin, eye catch-light), a grey-green sheen on the black wool (fuzz up),
   brighter collar and cuffs (`MisterSexton.py` looks, M_CharacterCloth). Cost S-M. **USER OK** (the user reviewed his
   wool closeups v1-v4).
10. **Ghost details** with item 1: rubber hands get knuckle and nail cavity in G, patch decals cut ragged, hem shards
    joined into strips (`Unpaid.py`, `Abel.py`, `Amos.py`, `Foreman.py`). Cost S each. Rides on item 1's OK.
11. **Glow eyes**: a hot core, iris ring and soft falloff instead of flat white ovals (each hero's eye glow material;
    Pike's behind the lenses dimmer). Cost S. FREE (the glow eyes stay).
12. **Slime core** smoothed (subdivided, no facets) with a painted look (`Slime.py`). Cost S. FREE.
13. **Face life**: a blink (the glow dims, the lids close where rigged: the portraits' `close_lids` shows the lid skin
    moves) and a slow glow pulse, driven from C++ like the HUD portrait's blink clock. Cost S-M. FREE.

Not here: first-person arms (the FP-HANDS work), placeholder creatures (by design), the cold-open figures (not seen in
the grabs; look at them when item 1 lands, they use M_Ghost if they're Unpaid).

## First steps

1. Show the user the three sheets; ask OK for items 1, 3 and 5 (tap-to-answer).
2. Meanwhile, FREE items in one batch: 2 (hand curl), 4 (reflection term + Gauge), 7 (spider joints), 11, 12. One
   build, a PIE look on the select stand and in RR, tests for the changed code at commit, perf once before and after
   (item 4 touches every character's shader).
3. Then item 1 once OK'd (and 10 with it), then 3, then 5/6 hero by hero, each with a before/after for the user.
Agents: material and C++ work (1, 3, 4, 13) on Opus; model scripts (6, 7, 12) on Sonnet against a contract; Main checks
each in PIE.

## CHAR-MAT (materials; items 1, 4, 5's materials, 8/9 as looks, 11) — written, editor steps wait for Main's GO

Notes: `Docs/Pipeline/CharacterModels.md` "Materials pass". Renders: `Saved/ArtPreviews/Characters/CharMat_*.png`.
At GO, in order (editor open): (1) `build_world_materials.py M_Character M_CharacterGlow`; (2)
`build_creature_materials.py M_Ghost`; (3) `Looter.ImportModels` of `Intermediate/ArtExport_CharMat_Gauge`, `_Ellis`,
`_FPArms`, `_Gravemother`, `_Sexton`; (4) `build_character_glows.py` (Pike's, Crane's and Odessa's eyes). Then PIE
(the select stand, RR in daylight and in the mine) and a perf run (M_Character's change reaches every hero).
Left: Crane's and Odessa's skins bake with their next export, after CHAR-FACE (crane_looks.dead_skin,
odessa_looks.painted_light). Not done here: item 10 (ghost hands/patches/hem: model scripts), item 12 (the slime core is
faceted geometry: Slime.py's 14x9 ellipsoid): Main queues a small Blender job for both. Sexton's crows: dropped (his Toll
Gate form, not this model; Main, 2026-10-22). Editor lane order: LOOT-POLISH -> CHAR-POSE -> tones -> element parts ->
CHAR-MAT -> CHAR-FACE.
