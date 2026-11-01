# Lane TRIPO-PISTOLS: the Drover's four Tripo part sets (done, fitted, exported; not imported, not committed)

The user approved Tripo credits for "the gun part sets for pistols" (2026-11-01). The Drover has five slots (Body, Barrel,
Cylinder, Sight, Grip; no muzzle, no stock), so the AR's "6 slots x 4 sets" is **5 x 4 = 20 parts** here. Cap 750 credits;
**640 used** (110 left).

## The sets (existing keys only: none new, none renamed; the AR precedent replaces the meshes of existing keys)
| Slot | Set 1 Saint Maud | Set 2 Roundhouse | Set 3 Windfall | Set 4 Meridian |
|---|---|---|---|---|
| Body | Blued (default) | Ironclad | Scrapper | Ranger |
| Barrel | Service (default) | Lugged | Comped | Vented |
| Cylinder | Fluted6 (default) | Bigbore5 | Plain6 | Moon6 |
| Sight | Notch (default) | Glass (2x scope) | Peep | Pip (dot) |
| Grip | Plowhandle (default) | Coffin | Wrapped | Target |

Why: set 1 = the defaults (as the AR's and Ranchhand's set 1); the others pick keys whose scripted shapes fit the maker
(Roundhouse heavy, Windfall scrap and quick, Meridian precise), so no set is another recoloured. Heritage parts stay
scripted (Nickel, Homestead, Hollow, Engraved; Snub, Octagon, Long, Gilded; Seven, Eight, the element cylinders; Blade,
Longeye; Birdshead, Stag, Bone, Pearl). CSV model reuse: Thunder6/Rot6 wear Fluted6, TwinBore6 Plain6, Blasting5 Bigbore5,
TrickShot Service, ThunderHammer Ironclad: they get the Tripo meshes too (same as the scripted reuse). Drover.parts.csv is
unchanged.

## How it was made (the user's style ask: "everything generated in Tripo matches our current art style")
1. **Views** (`Tools/Blender/drover_views.py`): the game's own scripted parts (Drover.py, our Gun-master materials,
   textures and baked occlusion), recoloured in each maker's palette (MAKERS.md, as the AR/Ranchhand Tripo sets: 1 ivory
   enamel/cast iron/gilt; 2 oxblood/black iron/brass/cream; 3 sea teal over rust/galvanized/copper/black tape; 4 satin
   white/graphite/aluminium/fine brass/cyan) and **dressed with maker detail for the views only**
   (`Tools/Blender/drover_dressing.py`: gilt pinstripes, screws and checkering; brass rivets and cream chevrons; friction
   tape, hose clamp, copper wire, hazard tag; aluminium rings, graduation ticks, cyan line, stippling), because the AR's
   concept views were drawn richer than its scripted parts and a bare render came back as plain as the script (first
   Body_Blued, 40 credits, kept as `Art/Tripo/Drover/Body_Blued/Body_Blued_plainviews.glb`). Four orthographic Cycles
   views, 1024 px, part filling 84%, one scale per part, transparent `upload/` and `upload_white/`, in
   `Art/Concepts/Weapons/Drover/<Slot>_<Key>/`. The cylinder's views show the drum alone (rod and crane stay scripted).
2. **Tripo** (`Tools/tripo_image.py`, new; imports tripo.py's key/call/wait/download, tripo.py untouched): multiview-to-model
   (`/generation/multiview-to-model`, model v3.1-20260211, PBR, `texture_quality` standard, `texture_alignment`
   original_image, `orientation` align_image, face limits body 12k, cylinder/grip 8k, barrel 6k, sights 3-6k). Style: the
   input views themselves are the style reference (our materials and palette; texture aligned to the image), so no
   style_image repaint was needed: no grey clay, nothing glossy or toy-like (checked on the sheets). Records:
   `Art/Tripo/Drover/<Part>/task.json` (task ids, credits, earlier attempts), `<Part>.glb`, `preview.png`. Per-lane cap in
   `CAPS` (Drover 750), counted from the family's own task.json files.
3. **Fit** (`Tools/Blender/looter_tripo_gun.py`, new generic version of Bullpup.py's tripo_build for any `looter_guns.Gun`;
   Drover rules in `Tools/Blender/looter_tripo_drover.py`; specs in `Art/Models/Weapons/Tripo/Drover/parts_set1-4.py`;
   GLBs there as `Drover<Slot>_<Key>.glb`): Drover.py builds the scripted parts, then `tripo_gun.override` swaps each
   spec'd part, `LOOTER_DROVER_TRIPO=0` scripted only, `=1,3` only those sets. Per part: Tripo's axes (muzzle +X, left +Y),
   stretched onto the scripted part's bounds (the cylinder: the drum's), maps as texture set `TripoDrover<Slot><Key>` on
   the Gun master (2048 px max), the shell as a paintable slot `GunPolymerSand_TripoDrover...` (grips Wrapped/Target:
   `GunPolymerGrey_...`, the fittings paint) by face, tinted the set's own colour (the maker's paint recolours it in game,
   as the AR), **every scripted socket kept** (87 sockets, names and places identical to the scripted export, checked),
   LODs 50/20% with switch sizes scaled by part size against a 0.28 m gun.
   - Body: rarity accent strips (GunAccentGlow) and ember cores kept from the script; the cylinder window, rod channel and
     lug vents (Blued, Scrapper) or the armour slits (Ironclad) cut through again so the cylinder turns/swings and the
     ember shows. Ranger: GunCyanGlow lines laid on the skin where the views drew the cyan line.
   - Barrel: the scripted front blade and its cyan dot kept (on SIGHT_LINE); Tripo's blade trimmed. Vented: cyan line.
   - Cylinder: centred on its axis, turned so a chamber is on top (measured from the copper noses on the front face:
     all four came out within 0.3 deg), outer wall trued round; the scripted ejector rod, sleeve and crane kept, so the
     turn per shot and the crane swing work as before (`Set*_Reload.png`).
   - Sights: notch, peep aperture, Pip window and the Glass scope's bore cut open; scripted lens and reticle kept. The
     sight pictures match the scripted ones (`Sheet_SightPictures.png`).
   - Grip: SOCKET_Grip/Foregrip kept. **No hand markers added** (deviation): no Drover grip has markers yet (GripSpots
     lane D never marked the Drover), and markers on 4 of 8 grips would split the revolver's hands between marks and the
     kind's table. Do them for all eight grips in one pass with `fp_hands_parts_sheet.py`.
4. **Export**: `Tools\artrun.ps1 -Export Drover -Family TripoDrover` -> `Intermediate/ArtExport_TripoDrover` (39 models).

## Credits (multiview standard = 30, detailed = 40)
Body_Blued 40 (plain views, detailed) + 30 (redo from dressed views); every other part 30: Barrel_Service, Cylinder_Fluted6,
Sight_Notch, Grip_Plowhandle, Body_Ironclad, Barrel_Lugged, Cylinder_Bigbore5, Sight_Glass, Grip_Coffin, Body_Scrapper,
Barrel_Comped, Cylinder_Plain6, Sight_Peep, Grip_Wrapped, Body_Ranger, Barrel_Vented, Cylinder_Moon6, Sight_Pip,
Grip_Target. Total **640** (`python Tools/tripo_image.py spent Drover`). No failed tasks.

## Triangles (per part, LOD0)
Bodies 9.1-9.9k, barrels 5.2-5.6k, cylinders 7.5-8.0k, sights 2.8-5.0k, grips 6.6-7.4k: about 33-35k a revolver (the AR's
Tripo sets about 60k a rifle; scripted Drover parts are a few hundred to 1.8k each).

## Pictures (`Saved/ArtPreviews/TripoDrover/`)
`Sheet_Sets.png` (each set and a mixed gun: three-quarter, right, first person, reload swing), `Sheet_Parts.png` (all 20
fitted parts), `Sheet_SightPictures.png` (scripted vs Tripo, eye 22 cm behind SOCKET_Aim), `Sheet_AR_vs_Drover.png` (the
AR's Tripo sets beside the Drover's at one view), `Sheet_UploadViews.png` (the views Tripo got); singles `Set<N>_<Maker>_*.png`.
Rebuild: `Tools\artrun.ps1 -Script Tools\Blender\drover_tripo_sheet.py [-ScriptArgs '--parts']` (`--ads` sight pictures
only; `--tag x` suffix; `$env:LOOTER_DROVER_TRIPO='0'` for the scripted ones).

Honest judgment: shapes are faithful to the scripted silhouettes with Tripo's softer bevels, wear and painted detail
(rivets, pinstripes, tape, ticks read well); the sets are clearly four makers and sit with the AR's Tripo sets (same
palette, matte worn finish). They are less busy than the AR's sets (whose views were new, richer designs). Weakest: the
Notch and Peep (tiny, little to gain), Windfall's Plain6 (rust reads brown and muddy), the frames' cut window walls (Tripo's
texture stretched on the new walls, mostly hidden by the cylinder).

## Main's steps
1. Look at the sheets (user's OK on the look if wanted).
2. Import (editor open): `Tools\console.ps1 "Looter.ImportModels C:\Dev\AI_Looter_Shooter\Intermediate\ArtExport_TripoDrover"`
   (brings the 33 texture sets `Art/Textures/TripoDrover*` too; 20 Tripo + 13 paint copies). Then rerun
   `Tools/Unreal/setup_gun_parts.py` only if the importer needs it for the rank assets (keys and CSV unchanged).
3. Tests: `Tools\runtests.ps1 -Filter Looter.Weapons.Parts`, `Looter.Weapons.ModelScale`, `Looter.Weapons.Makers`,
   `Looter.Weapons.NewGuns`, `Looter.Weapons.ElementLook`, `Looter.Player.Hands` (revolver holds), plus any revolver
   cylinder/reload test (`-Filter Looter.Weapons.Revolver` if it exists).
4. PIE check: a Drover of each set and a mixed one: cylinder turns a chamber a shot (no wobble), swings out on reload,
   notch/peep/Pip/Glass sight pictures, maker paint on the shells, rarity accent, ember vents.
5. Commit (new: `Tools/tripo_image.py`, `Tools/Blender/{drover_views,drover_dressing,looter_tripo_gun,looter_tripo_drover,
   tripo_part_views,drover_tripo_sheet}.py`, `Art/Models/Weapons/Tripo/Drover/` (4 specs, 20 GLBs), `Art/Tripo/Drover/`,
   `Art/Concepts/Weapons/Drover/`, `Art/Textures/TripoDrover*`; changed: `Art/Models/Weapons/Drover.py` (docstring and the
   Tripo override before `g.build`)). CODEMAP lines for the six new Tools/Blender files and tripo_image.py;
   Docs/Pipeline/Assets.md "Tripo parts" gains the Drover line (the API path: multiview from rendered, dressed views).

## Risks
- Sizes: textures 154 MB, the views 80 MB, GLBs 42 MB (+51 MB duplicate originals in `Art/Tripo/Drover`; drop one copy if
  LFS space matters).
- Blender crashed once (UNKNOWN EXCEPTION) building all 20 beside other lanes' Blender runs; the rerun was clean.
- Tripo drums aren't exactly N-fold symmetric: small detail shifts may show as the cylinder turns (outline is trued round).
- The Comped's Tripo ports are only painted (its ember strip is kept inside); Ranger's ember pinstripe is the scripted pipe
  half sunk in Tripo's strap.
- The paint slots are found by colour; a few dark/edge faces of shells stay in the textured slot (show Tripo's colour
  under a different maker paint).
- Hand markers not added (above).
