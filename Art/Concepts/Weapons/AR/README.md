# AR (Bullpup) default parts: richer concept views for Tripo (2026-10-27)

The user finds the gun models bland and will try Tripo (multi-view image-to-3D). These are the six default parts (the plain,
first option of each slot in `Art/Models/Weapons/Bullpup.parts.csv`), redrawn with more detail (bevels, panel seams,
screws and rivets, edge wear, material breakup, a generic proof stamp). Silhouettes and attach points come from
`Art/Models/Weapons/Bullpup.py`'s numbers. Original designs; the maker marks are not drawn on, because the game projects
them (`T_GunMakerMarks`).

Rebuild: `node Art/Concepts/Weapons/AR/render.js [Body_Standard ...]` (Node + Playwright, as for `Tools\portraits.ps1`).
Each folder holds `upload/` and `upload_white/` (the Tripo images; below), plus `front.png`, `back.png`, `left.png`, `right.png` (1024 x 1024, orthographic, one scale across a
part's four views, grey #d9d9d9, soft even light, no text, no ground shadow) and `sheet.png` (the four views labelled,
size, numbered attach points, materials; for the user).

Views: **front** = from the muzzle end looking back; **back** = from the butt end; **left** = the gun's left side
(muzzle at the image's left); **right** = its right side (muzzle at the right).

| Folder | Mesh / key | Size L x H x W (cm) | Attach points (gun space u along the gun, v up) |
|---|---|---|---|
| `Body_Standard` | BullpupBody_Standard / Standard | 52.2 x 21.4 x 5.7 | Barrel: barrel-nut face (51.9, 0). Stock: butt plate boss (0, -3.3). Magazine: well (14, -5.6). Sight: rail top middle (23, 6.15). Also Grip (22.5, -10), Foregrip (44, -4.5) |
| `Barrel_Carbine` | BullpupBarrel_Carbine / Carbine | 12 x 3.3 x 3.3 | Origin: its back face on the body's barrel nut. SOCKET_Muzzle at the tip, 12 cm |
| `Muzzle_Cap` | BullpupMuzzle_Cap / Cap | 1.8 x 1.8 x 1.8 | Origin: its back face on the barrel's tip. SOCKET_Muzzle at its tip, 1.8 cm |
| `Magazine_30` | BullpupMagazine_30 / 30 | 9.7 x 19.2 x 2.8 | Origin: its top, where it seats in the well (14, -5.6) |
| `Sight_Iron` | BullpupSight_Iron / Iron | 16.4 x 3.9 x 2.9 | Origin: rail top, between the clamps (23, 6.15). SOCKET_Aim: rear aperture centre (16.8, 8.75) |
| `Stock_Pad` | BullpupStock_Pad / Pad | 1.6 x 10.8 x 5.2 | Origin: front face's boss on the butt plate (0, -3.3) |

## Four sets, one maker each (the user, 2026-10-27)

Every set keeps the in-game outlines and the same attach points (table above; each part's origin and sockets as in
`Bullpup.py`), so any body takes any barrel, muzzle, magazine, sight and stock. Makers' looks:
`Docs/Work/2026-10-22-Next/MAKERS.md`. Sets 2-4 have `upload/` and `upload_white/` only (no grey views, no sheets);
`node Art/Concepts/Weapons/AR/render.js --upload <Folder ...>` rebuilds them. Part sizes are L x H x W in cm.

**Set 1: Saint Maud** (the closest of the four: ivory-sand shell, dark iron, brass trim; it is the plainest set, not
ornate). Folders and sizes in the table above.

**Set 2: Roundhouse** (oxblood paint, black iron, brass trim, cream; heavy, industrial, big magazine).

| Folder | Key | Size | Look |
|---|---|---|---|
| `Body_Armored` | Armored | 52.2 x 21.4 x 6.7 | Oxblood shell, black iron armour plates with brass rivets and edge, vents through the nose plate, brass handle and nut ring, cream pinstripe |
| `Barrel_Heavy` | Heavy | 17 x 2.6 x 2.6 | Black bull barrel with cooling grooves, cream paint band, brass collar, three ember heat rings |
| `Muzzle_BoxBrake` | BoxBrake | 7.4 x 3.4 x 3.8 | Black iron box, ember blast through three ports, oxblood band, bolted brass face plate |
| `Magazine_Drum` | Drum | 13.6 x 17.6 x 6.3 | Oxblood drum, iron rim with rivets, stamped spokes, cream ring, ember ring, brass caps with winding keys |
| `Sight_Holo` | Holo | 11 x 6.8 x 3.4 | Oxblood hood with ribs and brass rivets, black base, cream cap with a cyan bar, red dot in the glass |
| `Stock_Monopod` | Monopod | 4 x 20.4 x 5.2 | Black pad on a black bracket, oxblood leg sleeve, brass collar, steel leg, rubber foot |

**Set 3: Windfall** (sea-teal paint over rust, bare steel, tape, copper; rattly, scrap-built).

| Folder | Key | Size | Look |
|---|---|---|---|
| `Body_Salvager` | Salvager | 52.2 x 22.4 x 5.9 | Chipped teal shell, welded rust and steel panels, hazard tag, taped grip, pipe guard, copper line, ember coolant line (right) |
| `Barrel_Shrouded` | Shrouded | 16.5 x 4.1 x 4.1 | Rusty bare-steel ported shroud, ember core in the holes, tape wrap, copper wire, steel collar |
| `Muzzle_Prong` | Prong | 6.2 x 2.1 x 2.1 | Bare-steel three-prong hider, welded roots, copper ring |
| `Magazine_Coupled` | Coupled | 9.6 x 19.4 x 5.6 | A polymer 30 and a rusty steel 30 clamped side by side with copper clamps |
| `Sight_Reflex` | Reflex | 7 x 5.2 x 2.6 | Chipped teal hood welded to a steel base, tape patch, glass window |
| `Stock_Skeleton` | Skeleton | 10.3 x 11.8 x 5.2 | Rust frame with old teal paint, taped top bar, welded joint, cracked black pad |

**Set 4: Meridian** (deep navy, nickel, walnut-dark, brass; clean and precise, small magazine).

| Folder | Key | Size | Look |
|---|---|---|---|
| `Body_Marksman` | Marksman | 52.2 x 21.4 x 5.7 | Navy shell, nickel plates with brass rivets, walnut cheek rest, nickel rails (top and under the nose), thumb rest, brass nut ring |
| `Barrel_Marksman` | Marksman | 24 x 5.2 x 2 | Long navy barrel with etched rings, nickel gas block and front-sight block, brass bands and crown, post with a cyan dot |
| `Muzzle_Compensator` | Compensator | 5.4 x 2.3 x 2.3 | Nickel body, ember in three top ports, navy band, brass ring |
| `Magazine_20` | 20 | 8.2 x 11.2 x 2.8 | Navy steel with witness holes (brass behind), engraved border, nickel lips and base plate |
| `Sight_RedDot` | RedDot | 9.2 x 4.4 x 3.7 | Navy tube, nickel clamp and rings, brass-lipped sunshade, cyan LED knob (right), red dot in the eyepiece |
| `Stock_Cheek` | Cheek | 12.7 x 14.2 x 5.2 | Black pad on a nickel plate, walnut cheek rest with checkering on brass posts |

What changed from the in-game parts in set 1 (all inside the old silhouettes):
- **Body:** butt-cap and nose-cap seams, seven hex screws, ribbed gunmetal plates with a border groove, a ported bolt on
  the right only (as the model), a brass selector over the grip, a serial plate (ticks, no text) and a star proof stamp,
  pocketed optic mount with cross bolts, dovetail rail, knurled barrel nut, stippled grip back strap, heat tint round the
  ember vents, a pale accent inlay (the rarity glow), scuffed edges. Butt plate: two bolts, guide rails, a locating boss.
- **Barrel:** knurled grip band and seating flange on the shroud, ember in the four slots (the fourth runs under the
  brass collar, as in the model), two screws, a heat-tinted barrel and muzzle threads; rear face with a threaded boss and
  an alignment key.
- **Muzzle cap:** diamond knurl, groove ring, chamfered closed face with a struck proof ring, internal threads.
- **Magazine:** lighter textured polymer, steel feed lips, a raised side panel with a cartridge emblem, the four ribs,
  a front-spine rib and latch notch, base plate with a steel strip and pin.
- **Sight:** clamp screws, sand hinge barrels with brass pins, a brass spring at the rear leaf, a sand lip round the
  aperture, a white index line; the back view shows the sight picture (cyan dot centred in the aperture).
- **Stock pad:** rubber with grip ribs, a molded eight-point star roundel, screw wells; a gunmetal backing plate.

## Uploading to Tripo
**Upload the files from `<Part>/upload/` (transparent background).** The grey views beside `sheet.png` made Tripo build
a backdrop slab (the user, 2026-10-27). `upload/` holds the part alone on full transparency: no markers, labels,
scale bars, contact shadows or glow spill, edge wear clipped inside the outline, the part filling ~84% of the frame at
one scale across its four views. `upload_white/` is the same on pure #ffffff, in case Tripo wants no alpha. Rebuild
both with `node Art/Concepts/Weapons/AR/render.js --upload` (leaves the grey views and sheets alone).
1. Multi-view (image to 3D): one part at a time. Front slot = `upload/front.png`, back = `upload/back.png`, left =
   `upload/left.png`, right = `upload/right.png`. If a long part (body, barrel, sight) comes back thin or warped, try again with `left.png` as the front
   image (then right = `front.png`, back = `right.png`, left = `back.png`): some generators read the most detailed view
   best as the front.
2. Settings (Main's brief): Smart Mesh on, about 10k quads, Smart UV, texture on (PBR if offered).
3. Export FBX or GLB. Before it goes in the game (Main's pipeline): scale to the sizes above (cm), turn so the muzzle
   faces the gun's +u (Blender -Y, as every model), put the origin on the listed attach point, add the sockets
   (SOCKET_Muzzle, SOCKET_Aim and the body's six) and a simple UCX_ collision if the part needs one.

Watch for in the game, because Tripo bakes one texture:
- Makers repaint parts by material slot (GunPolymerSand shell, GunPolymerGrey fittings, GunBlackMetal / GunSteelBlued
  metal, GunBrass trim; `Docs/Work/2026-10-22-Next/MAKERS.md`). A one-texture Tripo mesh loses that unless it is split
  back into those slots (or tinted by a mask).
- The rarity accent line (GunAccentGlow), the ember vents and slots (GunEmberGlow) and the sight's cyan dot are emissive
  materials in game; on a baked texture they need their own small emissive slots again.
- The iron sight is two separate pieces 14 cm apart on one mesh; Tripo may bridge the gap. If it does, make the rear
  leaf and the front post as two runs and join them.
- Small parts (the 1.8 cm muzzle cap) are drawn large; only the size table fixes their scale.
