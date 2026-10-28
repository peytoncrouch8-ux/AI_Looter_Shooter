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

**Set 2: Roundhouse** (redrawn 2026-10-27 as new designs: the user rejected recoloured set-1 shapes). Heavy and
industrial: oxblood paint, black iron, big brass rivets, cream stencil stripes. New silhouettes; only the interfaces are
set 1's (barrel-nut face at (51.9, 0) r 1.25, butt-plate boss at (0, -3.3), magazine well u 10.4-17.6 at v -5.6 and 2.4
wide, rail top v 6.15 over u 14-32 with the sight origin at 23, grip hand at (22.5, -10)). Drawn by `render.js --upload` (the rh* functions).

| Folder | Key | Size | Design |
|---|---|---|---|
| `Body_Armored` | Armored | 52.2 x 22.6 x 7.4 | Blocky oxblood slab receiver, riveted iron armour plate, iron rail beam on gussets the length of the gun, squared iron handguard with ember slots and a brass front band, chunky grip with an oxblood panel, heavy bar trigger guard, brass T-knob charging handle (left), hinged ejection hatch (right), cream chevrons, sling bar |
| `Barrel_Heavy` | Heavy | 18 x 3.8 x 3.8 | Thick squared iron shroud with ember in five slots and brass rivets, oxblood end cap, round barrel out of it, brass collar at the nut |
| `Muzzle_BoxBrake` | BoxBrake | 9.9 x 6.4 x 5.2 | Oversized iron box brake, three ports with ember blast, oxblood baffle fins standing proud, bolted brass face plate, knurled threaded collar |
| `Magazine_Drum` | Drum | 12.6 x 10 x 13.3 | Pan drum hanging upright under a riveted feed neck: oxblood can, iron lid, hoop and base, cartridge window, cream band, brass crank (right) |
| `Sight_Holo` | Holo | 11.6 x 7 x 3.9 | Riveted oxblood holo box inside an iron roll cage, heavy clamp with hex bolts, cream stripe, red dot in the glass |
| `Stock_Monopod` | Monopod | 15.3 x 21.6 x 5.2 | Heavy brace stock: iron beams round an oxblood web with two lightening holes, thick ribbed pad, folding monopod (oxblood sleeve, brass collar, steel spike) |

**Set 3: Windfall** (sea-teal paint over rust, bare steel, tape, copper; rattly, scrap-built).

| Folder | Key | Size | Look |
|---|---|---|---|
| `Body_Salvager` | Salvager | 52.2 x 22.4 x 5.9 | Chipped teal shell, welded rust and steel panels, hazard tag, taped grip, pipe guard, copper line, ember coolant line (right) |
| `Barrel_Shrouded` | Shrouded | 16.5 x 4.1 x 4.1 | Rusty bare-steel ported shroud, ember core in the holes, tape wrap, copper wire, steel collar |
| `Muzzle_Prong` | Prong | 6.2 x 2.1 x 2.1 | Bare-steel three-prong hider, welded roots, copper ring |
| `Magazine_Coupled` | Coupled | 9.6 x 19.4 x 5.6 | A polymer 30 and a rusty steel 30 clamped side by side with copper clamps |
| `Sight_Reflex` | Reflex | 7 x 5.2 x 2.6 | Chipped teal hood welded to a steel base, tape patch, glass window |
| `Stock_Skeleton` | Skeleton | 10.3 x 11.8 x 5.2 | Rust frame with old teal paint, taped top bar, welded joint, cracked black pad |

**Set 4: Meridian** (clean and precise; satin-white and graphite polymer, machined aluminium, fine brass, a cyan accent
line). Redrawn 2026-10-27 as six new designs (the user rejected set 1's outlines recoloured): own silhouettes and
construction, only the interfaces kept (barrel-nut face (51.9, 0) r 1.25 with a 3.2 cm barrel collar on it, butt plate
-8.4..1.8 with the boss at (0, -3.3), magazine top 10.4..17.6 at v -5.6, rail top 6.15 over u 14-32, grip centre
(22.5, -10)). Script: `node Art/Concepts/Weapons/AR/render_set4.js [Folder ...]` (upload images only; render.js still
holds the older navy set 4 code, unused).

| Folder | Key | Size | Look |
|---|---|---|---|
| `Body_Marksman` | Marksman | 52.2 x 22.3 x 5.5 | Long streamlined shell: satin-white upper, graphite lower split by one flowing parting line with a cyan inlay; raised comb over the butt, flat spine with a flush low graphite rail and faired ends, long tapering snout to a brass-ringed nut, six ember heat slits in a recessed strip on the nose cap; flush brushed-aluminium side panel (ejection cover in it on the right), hand-stop lip, separate raked stippled grip with an aluminium cap, slim swept guard, flat aluminium trigger, charging slot and tab on the left, brass pins and selector, aluminium mag-well flare and QD cups |
| `Barrel_Marksman` | Marksman | 24 x 3.2 x 3.2 | Thin graphite barrel, three visible flutes each side of a low aluminium gas block, aluminium seating collar with wrench flats, fine brass ring, bare-steel muzzle threads; back face keeps the threaded boss and brass key |
| `Muzzle_Compensator` | Compensator | 5.0 x 2.1 x 2.1 | Slim graphite body, four raked side ports (ember inside), milled top flat, bright machined front chamfer, brass ring, sleeve with wrench flats |
| `Magazine_20` | 20 | 9.2 x 11.3 x 2.9 | Short graphite 20, slight forward curve, molded border, smoked witness window showing the brass stack with index ticks (top one cyan), grip grooves, aluminium feed lips, machined base plate with a toe, brass pin and cyan inlay |
| `Sight_RedDot` | RedDot | 6.9 x 5.4 x 4.0 | Compact enclosed dot: graphite housing with a raked hood and side pocket, aluminium window bezels, coated front lens, red dot centred in the rear window (SOCKET_Aim (20.2, 8.95)), aluminium elevation cap with brass ring, brass windage cap and battery tray (right), +/- buttons (left), brushed aluminium clamp with a knurled thumb nut (right), cyan accent |
| `Stock_Cheek` | Cheek | 10.2 x 14.7 x 5.7 | Adjustable: aluminium seat plate (boss and guide slots as set 1), two aluminium rods and a marked graphite lock bar for length, a swept two-tone block with a cyan line, satin-white cheek riser on brass posts, lock knob (right) or index plate (left), aluminium spacer, curved black rubber pad, QD cup |

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
