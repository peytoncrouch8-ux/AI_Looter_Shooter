# BARGE_MEASURE (MEASURE lane, 2026-11-04 06:07-06:15, editor open on Lvl_RansomsRest)

Source: the SOURCE triangles of `/Game/Art/Tripo/SM_DredgeBarge` (37318 vertices, 75775 triangles) copied through GeometryScript
(`barge_dump.py`), vertical rays on a 50 cm grid (`barge_deck.js`), maps (`barge_map.js`) and my extra cuts (zones, levels, rail, holes,
beam, fit, hall, spots, rect). Barge frame = mesh frame (the Hull component is at identity: relative loc 0, scale 1, world rot = the
actor's). cm, +X bow, -Y the berth (port) side, Z up from the keel pivot. Read-only: nothing saved, no PIE, no build.
All raw files in `C:\Users\Peyton\AppData\Local\Temp\claude\C--Users-Peyton--claude-spares-spare-rh8i84\2d5408c8-7e6b-4466-8b5a-fd3da713cff6\scratchpad\`
(= SP below): `barge_facts.json`, `barge_tris.txt`, `barge_tris_grid.json`, `barge_deck_out.txt`, `barge_map_out.txt` (class map, deck-height
map, walkable y-runs per metre of x, half beam), `barge_zones_out.txt` + `barge_tris_classes.txt` (S/L/B/H class map), `barge_levels_out.txt`,
`barge_rail_out.txt`, `barge_holes_out.txt`, `barge_beam_out.txt`, `barge_fit_out.txt`, `barge_beam_envelope_out.txt`, `barge_hall_envelope_out.txt`, `barge_spots_out.txt`,
`barge_rect_out.txt`. Scripts beside them (`barge_*.js`). The editor had Lvl_TutorialIsland open: opened Ransom's Rest with
`Tools/Unreal/open_clean.py` (dirty maps after: none, nothing saved). One script fix: `barge_dump.py` read `StaticMesh.sockets`, a protected
property in 5.8 (exception); wrapped in try/except, the facts say "unreadable". I found no way to list the mesh's sockets from Python in 5.8
(only `find_socket`, by name); none are known, so unmeasured.

## 1. Hull facts
- Bounds x -3750..+3750 (75.00 m), y -1237.4..+1237.4 (24.75 m), z 0..4849.3 (48.49 m). Pivot: bottom centre (x, y symmetric, z min 0).
- Nanite off. Material `/Game/Art/Materials/MI_TripoDredgeBarge`, one slot, NOT two-sided (back faces cull: inside the hull you see through).
- Collision: `Hull` component `NO_COLLISION`; the mesh's body setup has `CTF_USE_DEFAULT` with ONE convex element (useless to walk on).
- Winding: the stored winding gives downward normals on the topmost surface (vote -554), so the script flipped it (outward = up on top).
- The shell is a single skin: from above a column shows only [bottom v, top ^] plus a few thin plates. The belly (lowest surface at y 0) is z 725-1060
  from x -3000 to +2000, rising to 1230 at x 2500, 1540 at x 3000, 2200 at x 3500 (the bow); stern 1230 at x -3500. Nothing inside is modelled.
  Below 500 there are only 554 columns (138 m2), x -1750..-50, y -300..+1000: thin stepped plates (x -1000 y 800: 28v 108^ 432v 464^ 734v 793^ then the
  hull from 1253), a ladder/cradle hanging under the belly, not hull. The thin plates at |y| 1150-1237 (x -400..-200 z 2170-2266, x 700-800) are the bounds' 1237.
- `Barge_Pilot` (class BargeBoarding): location (7492.13, -2517.81, 3250.70), yaw -62.757, pitch 0, roll 0, scale 1. `kit_lift` = 0.0.
  `hull` mesh = SM_DredgeBarge, relative (0,0,0), scale (1,1,1); the Hull component's world location equals the actor's.
  So the Tripo deck at 2250 is at world z 5500.7 (keel 3250.7).
- Attached actors (world): Item_BargeManifest (7272.4, -2091.1, 4135.7) = 885 over the keel; Barge_Horn_1..4 (CompanyLoudspeaker) at z 4650.7
  = 1400 over the keel (DeckTop): they stand 8.5 m inside the hull until DeckTop moves.

## 2. The deck (walkable = up-facing, normal z > 0.85, surface in 2100..2500, the next surface over it >= 220 cm or none)
- Up-facing area by height (50 cm columns, the fullest 10 cm bins): 2250: 820 columns (205 m2); 2290-2370: 780 columns (195 m2); the rest of
  the 2100-2500 band is ramps and rims. Higher up-facing tops: 2760-2850 (aft block roof), 3220 (163 m2), 3760-3920, 4600-4800 (see 3).
- Walkable 493 m2 (S, headroom ok) + 67 m2 covered under 220 cm (L) in the whole footprint. Deck height 5/25/50/75/95%: 2250/2252/2304/2350/2398.
  Within the kit's rectangle (x -3450..3350, y +-1150, 1610 m2): standable 444 m2 (28%), deck with low headroom 54 (3%), hull without a
  deck in the band 109 (7%), BLOCKED by superstructure 800 (50%), no Tripo surface at all 203 (13%, the tapered ends and outside the beam).
  A ~1 m wide pawn fits (all 8 neighbours at 50 cm standable, deck within 30 cm) on only 123 m2: foredeck 91, mid rim 21, aft platform 11,
  cross walk 10. The 50 cm grid cannot resolve strips under ~1 m; a capsule is narrower (see `barge_fit_out.txt` map, "+" = narrow).
- It is NOT one flat deck. Terraces (connected, steps <= 15 cm; area, x, y, z min/med/max):
  - foredeck A: 64 m2, x 1400..2350, y -800..850, z 2249/2251/2255
  - foredeck B: 79 m2, x 2300..3250, y -600..600, z 2366/2371/2375 (a raised platform, +1.2 m over A)
  - foredeck C (bow): 64 m2, x 2300..3700, y -800..850, z 2249/2295/2305; bow rim at x 3350..3700 z ~2300 (half beam 600-650)
  - foredeck side steps: x 1400..2750, y 650..950, z 2328-2353 (13 m2) and x 1650..2750, y -900..-650, z 2331-2351 (11 m2)
  - cross walk + inner strips: 71 m2, x -650..900, y -900..950, z 2250/2252/2256; the cross walk itself x -650..-550 (1.5 m wide) spans
    y -900..+1050 between the aft block and the midship tower
  - aft platform: x -3500..-2550, y -850..-350 (17 m2) and y 400..900 (16 m2), z 2250/2252/2256; steps at x -3600..-3250, |y| 150..350, z ~2349
  - port side catwalk: x -3500..-1100, y -1000..-550, z 2311/2315/2340 (20 m2); stbd: x -3400..-2050, y 650..1050, z 2313-2340 (13 m2) and
    x -1900..600, y 1000..1050, z 2307-2325 (18 m2); more at x -500..750, y -1050..-950, z 2314-2334
- Standable y-runs per metre of x (full list in `barge_map_out.txt`, "along x"): stern x -3650: -150..150; x -3450: -600..-200, 200..500;
  x -3350: -750..-200, 200..850; x -3150..-2350: only the rims, port -1000..-650 and stbd 600..1050 (1-3 m wide); x -2250..-1650: rims only,
  ~0.5-1 m wide (y -1000 and +1000..1050); x -1550..-650: port -1050..-900, stbd 1000..1050; x -650..-550: the cross walk, -900..1050 (full);
  x -450..-250: port -1150..-700 and stbd 750..1200 (3-4.5 m wide each); x -150..1100: port -1050..-800, stbd 850..1100 (1-2.5 m);
  x 1150..1850: port ~-950..-700, stbd 750..1000; x 1950..2250: -900..850 (full width); x 2350..3350 broken by L/H bits but y -700..800 mostly open;
  x 3450..3550: -650..650; x 3650: -600..600 in pieces. Headroom >= 220 almost everywhere standable except x -3600..-3200 (cover 160 over a
  deck at 2341-2490) and x 3350..3400 (cover 158).

## 3. The superstructure (columns with no deck in the band, topmost surface above 2500; grouped, 4-neighbour, tops within 150 cm)
- A. Aft block ("house/engine casing"): x -3250..-700, y -900..900, 340 m2, tops 2503..3020 (median 2794, flat roof plateaus 2820-2850).
  5.5 m over the 22.5 deck; fills the beam except the 1-3 m rims. Smaller bits on it: x -1950..-1750 y -350..-50 and 100..400 (tops 3016-3090,
  8 + 7 m2), x -2100..-1350 y -50..100 (top 3300, 9 m2).
- B. The stacks (47 m): x -2600..-2150, y -200..200 (17 m2, top 4600-4799) and x -1300..-900, y -200..200 (16 m2, top 4607-4804).
- C. Midship, four steps stepping up toward the bow, x -500..1900: x -500..50 (42 m2, top 3223-3316, ~32 m, "the tank"/low house); x -300..350
  (53 m2, top 3802-3998, ~39 m); x 100..950, y -800..800 (120 m2, top 4365-4840, median 4616, 46 m: the tall tower); x 450..1900,
  y -750..800 (141 m2, top 3738-4073, median 3875, 38.8 m: the forward house/tank). Side canopies at ~31.4-32.2 m over the rims at x 0..1000.
- D. Clear aft of the foredeck: x 1950..3700 has nothing above 2500 except bits (x 3350..3400 cover 158 at 2300-2360; x 2400-2700 rail bars).
- Blocked area by top height (m2 per 100 cm bin): 2500:34 2600:58 2700:93 2800:151 2900:18 3000:17 3200:60 3700:37 3800:152 3900:16 4300:2 4400:17
  4500:34 4600:61 4700:38. Class map: `barge_tris_classes.txt` (S L B H .; 50 cm; rows y -1200..+1200 bottom up, columns x -3750..3750).
- The kit's rectangle has NO Tripo deck under: the middle (x -3250..-700 and -500..1900, |y| < ~800) = bodies A, C; the corners past the hull's taper
  (203 m2 with no surface: the stern x < -3350 and bow x > 3500 beyond the half beam, and |y| > hull edge).

## 4. Hull half beam (cm). By VOLUME: z lies between a down-facing surface and the next up-facing one over it, the pair >= 100 cm thick (thin plates, rails, treads do not count). `barge_beam_out.txt`
```
   z  -3500 -3000 -2500 -2000 -1500 -1000  -500     0   500  1000  1500  2000  2500  3000  3500   max
1200      0   250   550   650   650   700   650   650   650   600   550   400     0     0     0   700
1300    100   500   700   750   800   850   800   750   750   700   650   550   300     0     0   850
1400    100   650   800   800   850   900   900   850   850   800   750   650   450     0     0   900
1500     50   750   900   900   950   950   950   900   900   850   800   700   550     0     0   950
1600      0   850   950   950   950  1000   950   950   950   900   850   750   600   200     0  1000
1700    250   850   950  1000  1000  1000  1000  1000  1000   950   900   750   650   400     0  1000
1800    350   900  1000  1000  1000  1050  1050  1050  1000   950   900   800   650   450     0  1050
1900    450   900  1000  1000  1000  1050  1050  1050  1050  1000   900   850   700   500     0  1050
2000    600   950  1000  1000  1000  1050  1050  1050  1050  1000   950   850   750   600     0  1050
2100    600   950  1000  1000  1050  1050  1050  1050  1050  1000   950   850   750   600     0  1050
2200    650  1000  1050  1050  1050  1050  1100  1100  1050  1050   950   900   750   600     0  1100
2300    650   950  1000  1050  1050  1050  1050  1050  1050  1000   950   900   750   600     0  1050
2400    550   700   850   900   900   900   650   750   800   850   750     0   600   450     0   900
```
- Rows 600-800 hold a leg-like body at x -1500..-500 (850/750/700, z 500-1000) under the belly; it is not the skin. The earlier envelope version (any surface,
  thin plates counted; `barge_beam_envelope_out.txt`) reads 50-100 wider at the deck rims and 1000 at x -1500 low down; use this table.
- Widest at the deck: 1100 (22 m) at x -1000..0 (z 2200); not the bounds' 1237. V-shaped below: 650-900 at z 1400. The kit's DeckHalfBeam 1150 overhangs the hull's
  outline at every x (700 at x -3500, 650 at x 3500 at z 2200). Bow end: no hull volume at x 3500 above 1200 except the 650 lip (envelope), by volume 0.
- HallSkinHalfBeam PLUG (the narrowest over the hall, floor to ceiling), floor 1250 / walls to 2150, per x (-3500..3500 step 500): 0/300/650/700/750/750/750/700/700/650/600/500/150/0/0;
  floor 1350 / walls to 2250: 0/600/750/800/800/850/850/800/800/750/700/600/350/0/0. Largest round hall (85 cm skin, walls 900): R 650 at x -1300 (floor 1250); R 750 at x -1200
  (floor 1350); at HallCenter x -1800: R 600 either way. The kit's HallRadius 1100 (and 800-1000) fits nowhere with a floor 1000..1350 (`barge_beam_out.txt` end).

## 5. Openings in the Tripo deck
- None. Inside the 2200 plan outline minus 1 m, x -3400..3300: 0 columns with no surface and 0 columns whose topmost surface is below 2100
  (`barge_holes_out.txt`). The mesh has no hatch or hole to align the kit's hatch/hall to; the hull is a closed skin (belly z 725-1060 under the middle).
- Plan outline at z 2200 (half beam): -3500:700 -3000:1000 -2500:1050 -2000:1050 -1500:1050 -1000:1100 -500:1100 0:1100 500:1050 1000:1100 1500:1000
  2000:950 2500:850 3000:700 3500:650.

## 6. The rail line
- Thin plates (a down-face then an up-face within 30 cm, 50..130 cm over a deck surface) exist at: stbd y +1000 and port y -950, x -2000..-1100
  (top 2388-2401, ~87 cm over a deck at ~2312), port y -900 at x -2400..-2300 (top 2399-2412), stbd y 1000 / port -950 continuing to x -1100/-1000;
  also x 1200 port (-950..-900, 2437), and bits on the foredeck at 2420-2520 (x 2500-2700 y -600..-200 and 250..650; x 3300-3700 y +-600-650, top 2426-2434,
  ~110 over the 2300 deck). Rail top over the deck: 76/87/121 cm (10/50/90%). Only 101 columns found, so it is a thin bar (the 50 cm grid hits
  about half of it): rail runs along the side catwalks x -2400..-1000 (|y| 950-1000) and in pieces at the bow; elsewhere none (midship rims
  have no rail; the 31-32 m canopy edge is above). Output: `barge_rail_out.txt`.
- Berth side: at BerthX 2000 the hull's edge at the deck is y -950 (half beam 950 at 2200); the foredeck there stands at 2250 (y -900..850)
  and side step 2331-2351 at y -900..-650 (x 1650..2750). No rail bar was found at x 1900-2100 on the port side (nothing for the gangplank to pass through there).

## 7. The kit's current spots against the Tripo columns (barge frame, `barge_spots_out.txt`)
- 9 of 38 spots stand on a standable Tripo deck column: Cover (2500,300) 2371, (1500,800) 2254, (-500,-700) 2251; Vent (2700,480) 2371; Horn (-1500,-980)
  2312; Fight (1400,900) 2330, (-300,-900) 2252, (-800,950) 2255, (-3200,700) 2252. The rest sit in bodies A/C (tops 2.5-4.8 m over the deck) or
  past the hull (Horn (2650,-950) no surface; Berth point (2000,-950) no surface).
- Drum (-889,323) in A (top 2820); Stacks (-1107,21)/(-2373,19) in the Tripo stacks (top ~4800), (-300,380) in C (3231); Crane (900,700) in C (4365);
  HallCentre (-1800,0) under A (top 3318).

## Reading (mine, from the numbers)
- A kit deck at 2250 would put half of the kit's rectangle inside superstructure; the Tripo "open deck" is the foredeck (x 1400..3700), a cross walk, side rims and the aft
  platform. The walkable layout has to follow the S map, not a rectangle; or the superstructure is a nav/collision decision for Main (hide, cut, or place the fight on the foredeck).
- DeckTop candidates: 2250 (the main level, 205 m2 of the walkable) with the raised bits at +60..+120 (2300-2371) as steps; no single height is flat over the deck.
- The hall at R 1100 does not fit the hull; the biggest round one is R 650 (floor 1250, x -1300) to R 750 (floor 1350, x -1200); R 600 at the kit's x -1800.
