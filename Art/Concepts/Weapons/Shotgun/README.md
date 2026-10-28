# Ranchhand (shotgun) parts: concept views for Tripo (2026-10-28)

The user models gun parts in Tripo (multi-view image to 3D) from part-only concept views, as for the AR
(`Art/Concepts/Weapons/AR/README.md`). Four sets, one maker each. **Each set is a new design (own silhouette and
construction), never another set recoloured** (the user rejected recolours on the AR). Only the interfaces are shared
(`Art/Models/Weapons/Ranchhand.py`: sockets, joint sizes, overall size per key), so any set's part fits any other's.
Part keys are saved with dropped guns: these are existing keys (`Ranchhand.parts.csv`), none renamed or new.

| Slot | Set 1 Saint Maud | Set 2 Roundhouse | Set 3 Windfall | Set 4 Meridian |
|---|---|---|---|---|
| Body | Standard | Armored | Salvaged | Tactical |
| Barrel | Field | Shielded | Short | Slug |
| Muzzle | Crown | Brake | Duckbill | Choke |
| Magazine | Tube6 | Drum20 | Box8 | Tube6Window |
| Sight | Flip | Prism | GhostRing | Variable |
| Stock | Skeleton | Mule | Folding | Thumbhole |

Why these keys: set 1 = the default parts (as the AR's set 1), on their in-game shapes with richer sculpted detail.
Roundhouse (magazine +30%, recoil -25%, handling -15%): armored, riot heat shield, recoil brake, 20-shell drum, chunky
prism, heavy recoil-soaking Mule. Windfall (scrap, accuracy -20%, reload -20%): salvaged, sawn-off, wide duckbill,
quick-reload box, scrap ghost ring, folding pipe stock. Meridian (accuracy +30%, range +20%, handling +10%): tactical,
rifled slug barrel, extended choke, windowed tube, 1-4x scope (a surveyor's transit), steady thumbhole.
The pump (`Pump_Walnut`, the only pump key, on every gun) has no views yet.

Looks: 1 ivory enamel, cast iron, gilt brass, a little ember (plain, careful). 2 oxblood, black iron, big brass rivets,
cream stencils; boxy riveted railway slabs (cab and louvred hood, boarding-step guard, driving-wheel drum, signal-lamp
prism, buffer-beam butt). 3 rusted steel, galvanized tin, teal and sky-blue paint, friction and tan tape, brass wire,
hose clamps, welds. 4 satin white, graphite, machined aluminium, fine brass, a cyan line, graduations, a level vial.
No maker marks drawn (the game projects them).

Folders: `<Slot>_<Key>/upload/{front,back,left,right}.png` (transparent, the part alone filling ~84%, one scale across
its four views) and `upload_white/` (the same on #ffffff). Views: front = from the muzzle end, back = from the butt end,
left = the gun's left side (muzzle to the image's left), right = its right side (muzzle to the right). The ejection port
is on the right. `compare_bodies.png` and `compare_sets.png`: left views side by side, a column per set.

## Interfaces every set keeps (gun space, cm: u along the gun from the receiver's back, v up from the bore)
- Body u 0..20: stock seat on the back face (u 0, v -3.0..1.6); barrel nut (20, 0), barrel r 1.15; magazine/pump seat
  (20, -2.5), tube r 1.05; flat bottom at v -3.0 over u 13.4..19.6 (box and drum wells); picatinny rail base v 2.2, top
  3.15, width 2.1, over u 1..19.5; trigger group u -0.6..8.2 (moved 5 cm back on 2026-10-28 so the first-person finger reaches the trigger, face at u 3.4; the guard's rear meets the grips' fronts); the left flank flat over u 5.8..14, v -2.2..1.3 (element
  plate). The stock brings the pistol grip.
- Barrel: from the nut, r 1.15, lengths Field 46, Shielded 46, Short 38, Slug 46 (SOCKET_Muzzle at the tip, the last cm
  plain); the underside plain (the pump rides under it, the tube clamp wraps it at 32.5..34.5 / 42.5..44.5).
- Muzzle: back face on the barrel tip (bore r 1.15); lengths Crown 1.0, Brake 5.6, Duckbill 6.6 (flat, wide sideways),
  Choke 4.6.
- Magazine, origin (20, -2.5): tubes r 1.05 from u 20 (<= r 1.1 to u 47.2 where the pump rides), cap 54..56, clamp
  52.5..54.5 up to the barrel; Box8 / Drum20: a well u 13.4..19.6 plus a pump guide tube r 0.8 to u 47..48.
- Sight, origin (10.25, 3.15) on the rail top, mounts within u 3..17.5. SOCKET_Aim: Flip (4.9, 5.1), GhostRing
  (5.65, 5.35), Prism (5.15, 5.75), Variable (2.0, 6.75). Through-sights stay see-through. (Meridian's Variable has a
  longer narrow middle than the script's so both rings sit on it; eyepiece, objective and aim are unchanged.)
- Stock: front face at u 0 (v -3.0..1.8); butt pad's back face at Skeleton -40.2, Mule -41.6, Thumbhole -40.2,
  Folding -33.0.

## Scripts (Node + Playwright, no other tools)
`node Art/Concepts/Weapons/Shotgun/render.js [Folder... | set1..set4]` renders upload/ and upload_white/;
`render.js --sheet` the two compare sheets. Parts: `parts_set1.js` (Saint Maud) .. `parts_set4.js` (Meridian); the
drawing kit `lib.js` (the AR scripts' helpers as a module). Note: a straight horizontal or vertical `rod()` vanishes (its
ao filter has a zero-size box); draw those as bars.

## Tripo steps (per part, one at a time)
1. Multi-view (image to 3D): front slot = `upload/front.png`, back = `upload/back.png`, left = `upload/left.png`,
   right = `upload/right.png` (use `upload_white/` only if Tripo wants no alpha). If a long part (barrel, tube, stock)
   comes back thin or warped, try `left.png` as the front image (then right = `front.png`, back = `right.png`,
   left = `back.png`).
2. Generate with Smart Mesh on (about 10k quads), then **Smart UV**, then **Texture** (PBR if offered).
3. Export **GLB** and save it in the part's `upload/` folder (e.g. `Body_Armored/upload/Body Armored.glb`). Main's
   pipeline (`Tools/Blender/looter_tripo.py`) scales, orients and fits it to the scripted part, copies the sockets and
   splits the glow and paint slots (Docs/Pipeline/Assets.md, "Tripo parts").
Watch for: thin parts (barrels, tubes) and tiny ones (Crown 1 cm) are drawn large, the size table fixes their scale;
sight glass comes back opaque (cut it, a dot on SOCKET_Aim); glow (ember, cyan, the rarity line) needs its own emissive
slots again; the Flip and GhostRing sights are two pieces on one base strap or rail clamps, Tripo may bridge them.
