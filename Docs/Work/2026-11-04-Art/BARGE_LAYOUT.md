# Dredge barge layout (measured)

SM_DredgeBarge after the round-5 rework (Tools/Blender/looter_tripo_barge.py), measured with rays by Tools/Blender/looter_tripo_barge_check.py. UE frame, cm: pivot bottom-centre, bow +X, berth side -Y, Z up.
Bounds: x -3750..5590, y -1237..1237, z 0..4809 (9340 x 2475 x 4809). Triangles 143989.

## Well deck

- Deck z 2250..2252 (rays down on a 100 cm grid, |y| <= 650); one level.
- Clear box: x 2153 (the house's fore face, its foremost point at z +100..+180) .. 3958 (the stairs' foot), |y| <= 707 (the bulwark's inner face, nearest of 18 stations); 1805 x 1414.
- Bulwark inner face |y|: +Y 738..781, -Y 707..9000; its rim top z 2339..2342.
- Things standing on it (ray hits over z +8): none.

## Forecastle and its stairs

- Aft face x 4121 at y 0 (rays forward at z +40 across |y| <= 650: y -600 x 4121, y -450 x 3958, y -300 x 4031, y -150 x 3958, y 0 x 4121, y 150 x 3958, y 300 x 4031, y 450 x 3958, y 600 x 4121).
- Floor (the scoop's, up-facing faces at its level): x 4125..5500, z 2373 (median), between the scoop's walls at |y| 353 (x 4125), 529 (x 4425), 433 (x 4725), 352 (x 5025).
- The scoop's frame over it: 28 collision chunks (40 cm, fitted to the tubes) up to head height; the lowest tube 28 over the floor.
- Clear of the frame to 200 over the floor (rays up on a 25 cm grid; per x, the |y| runs): x 4125: -353..347; x 4225: -494..481; x 4325: -203..247; x 4425: -529..-379, -329..-204, -79..121, 221..371, 421..546; x 4525: -481..-356, -306..344, 369..519; x 4625: -450..500; x 4725: -423..452; x 4825: -389..436; x 4925: -389..411; x 5025: -340..385; x 5125: none.
- Stairs |y| 150..450 on +Y (rays down at y 300): from x 3901 z 2250, from x 3971 z 2274, from x 4001 z 2294, from x 4031 z 2313, from x 4061 z 2333, from x 4091 z 2353, from x 4111 z 2358, from x 4121 z 2372, from x 4131 z 2369.
- Stairs |y| 150..450 on -Y (rays down at y -300): from x 3901 z 2250, from x 3971 z 2275, from x 4001 z 2294, from x 4031 z 2313, from x 4061 z 2333, from x 4091 z 2353, from x 4121 z 2372, from x 4131 z 2367.

## Side walks (casing and house)

- +Y casing walk, x -3325..-675 (106 stations): floor z 2253..2265, from the wall's foot |y| 427..783 to the bulwark's inner face |y| 710..980; clear width (z +20..+180) 146 at least (x -3075; by height +20: 146, +100: 152, +180: 152), median 195; headroom 222 at least, under 220 at 0 stations. Nowhere under 120. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- +Y house walk, x -100..2125 (90 stations): floor z 2253..2266, from the wall's foot |y| 587..744 to the bulwark's inner face |y| 815..976; clear width (z +20..+180) 167 at least (x 1000; by height +20: 167, +100: 223, +180: 227), median 251; headroom 360 at least, under 220 at 0 stations. Nowhere under 150. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- -Y casing walk, x -3325..-675 (106 stations): floor z 2252..2268, from the wall's foot |y| 351..826 to the bulwark's inner face |y| 708..941; clear width (z +20..+180) 126 at least (x -725; by height +20: 126, +100: 144, +180: 144), median 184; headroom 222 at least, under 220 at 0 stations. Nowhere under 120. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- -Y house walk, x -100..2125 (90 stations): floor z 2253..2262, from the wall's foot |y| 558..718 to the bulwark's inner face |y| 792..940; clear width (z +20..+180) 170 at least (x 1375; by height +20: 170, +100: 224, +180: 223), median 243; headroom 348 at least, under 220 at 0 stations. Nowhere under 150. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.

## Cross walk and hatch

- Cross walk: x -671 (the casing's fore face at z +120, y 0) .. -294 (the house's aft face), deck z 2251..2256.
- Hatch hole (rays down on a 5 cm grid fall past z 2150): x -611..-436, y -220..225 (3240 rays); centre (-525, 0). Coaming top z 2305..2310 on its -Y, aft and fore sides; its +Y edge open (the deck beside it z 2251..2254: where the kit puts its stair); through the hole the first thing below is at z 942. Collision in the hole from z 1880 up: none.

## Rail line and the berth gap

- The gap in the -Y bulwark (rays inboard at z +40 pass): x 3005..3300 (295 wide); its sill z 2262, the rim beside it z 2339; the hull's side outboard there |y| 834.
- The -Y rail's collision at z 2295 (points across y -845..-645, every 5 cm over x 2900..3400): open over x 3000..3300, held elsewhere.

## The hall's cavity

- Cylinder centre (-1185, 0), R 750, floor z 1300, ceiling z 2220.
- Nearest mesh from the axis (rays out in 24 directions at 6 heights): 827 at z 1320, 270 deg, 838 at z 1320, 255 deg, 858 at z 1320, 90 deg, 866 at z 1320, 240 deg.
- Deck underside over it (rays up from the floor + 50): z 2253..4771; the hull's bottom under it (rays down from the ceiling - 50): z 733..1229.
- Collision inside it: none (sample points inside a UCX hull).
- Over it the deck is not closed everywhere (Tripo made no deck inside the casing: rays up reach the stack): the hall's own ceiling closes it.

## Material slots

- TripoDredgeBarge_Hull 46428, TripoDredgeBarge_Deck 20476, TripoDredgeBarge_Super 57149, TripoDredgeBarge_Walls 19936.
- well deck: TripoDredgeBarge_Deck
- forecastle floor: TripoDredgeBarge_Deck
- cross walk: TripoDredgeBarge_Deck
- hull below the strake: TripoDredgeBarge_Hull
- house wall: TripoDredgeBarge_Walls
- casing wall: TripoDredgeBarge_Walls
- stack: TripoDredgeBarge_Super
- the well deck bulwark: TripoDredgeBarge_Super

## Deck zones (the floor boxes: x, y, top z)

- well deck 0: x 2245..2545, y -782..782, top z 2253
- well deck 1: x 2545..2845, y -736..736, top z 2253
- well deck 2: x 2845..3145, y -736..736, top z 2253
- well deck 3: x 3145..3445, y -736..736, top z 2253
- well deck 4: x 3445..3745, y -736..736, top z 2253
- well deck 5: x 3745..4045, y -736..736, top z 2253
- well deck 6: x 4045..4120, y -729..729, top z 2253
- forecastle 0: x 4120..4420, y -591..591, top z 2373
- forecastle 1: x 4420..4720, y -570..570, top z 2373
- forecastle 2: x 4720..5020, y -485..485, top z 2373
- forecastle 3: x 5020..5090, y -431..431, top z 2373
- walk -Y: x -3360..2245, y -1057..-351, top z 2257
- walk +Y: x -3360..2245, y 398..980, top z 2258
- cross walk aft of the hatch: x -700..-615, y -1057..1057, top z 2260
- cross walk fore of the hatch: x -435..-300, y -1057..1057, top z 2260
- cross walk -Y of the hatch: x -615..-435, y -1057..-225, top z 2260
- cross walk +Y of the hatch: x -615..-435, y 225..1057, top z 2260
- stern platform: x -3650..-3360, y -700..700, top z 2252
- coaming aft: x -631..-611, y -240..245, top z 2310
- coaming fore: x -436..-416, y -240..245, top z 2310
- coaming -Y: x -631..-416, y -240..-220, top z 2310
- stairs +Y: x 3970..4120, y 150..450, top z 2371
- stairs -Y: x 3970..4120, y -450..-150, top z 2371

## Footprints to keep clear of (the collision hulls' bounds)

- cabin 0: x -357..938, y -856..884, z 3200..4011
- cabin 1: x 942..2234, y -846..881, z 3200..4015
- casing -1 high: x -3414..-3365, y -244..276, z 2714..3010
- casing -1 low: x -3411..-3361, y -338..349, z 2257..2693
- casing 0 cabin: x -2654..-2461, y -443..475, z 3208..3486
- casing 0 high: x -3354..-2460, y -651..675, z 2700..3197
- casing 0 low: x -3349..-3211, y -316..335, z 2290..2700
- casing 1 cabin: x -2458..-1562, y -341..371, z 3202..3759
- casing 1 high: x -2459..-1564, y -627..644, z 2700..3200
- casing 1 low: x -3208..-3061, y -475..504, z 2268..2695
- casing 10 low: x -1854..-1711, y -729..758, z 2261..2694
- casing 11 low: x -1708..-1560, y -725..753, z 2294..2680
- casing 12 low: x -1560..-1424, y -716..752, z 2309..2689
- casing 13 low: x -1402..-1267, y -726..752, z 2279..2678
- casing 14 low: x -1223..-1110, y -727..751, z 2314..2697
- casing 15 low: x -1110..-967, y -726..751, z 2317..2690
- casing 16 low: x -942..-810, y -726..754, z 2327..2696
- casing 17 low: x -808..-669, y -782..784, z 2266..2700
- casing 2 cabin: x -1556..-843, y -236..264, z 3204..3412
- casing 2 high: x -1557..-671, y -616..638, z 2700..3198
- casing 2 low: x -3059..-2911, y -603..629, z 2305..2699
- casing 3 low: x -2910..-2769, y -660..698, z 2300..2700
- casing 4 low: x -2758..-2611, y -694..717, z 2282..2697
- casing 5 low: x -2608..-2461, y -709..736, z 2326..2700
- casing 6 low: x -2459..-2312, y -775..747, z 2344..2677
- casing 7 low: x -2308..-2178, y -777..756, z 2261..2698
- casing 9 low: x -2009..-1860, y -727..784, z 2264..2697
- cradle: x -1788..-10, y -524..1032, z 0..950
- house 0 high: x -358..506, y -852..876, z 2700..3199
- house 0 low: x -315..-219, y -555..581, z 2257..2695
- house 1 high: x 508..1371, y -858..884, z 2701..3198
- house 1 low: x -183..-76, y -587..607, z 2261..2682
- house 11 low: x 1290..1436, y -682..708, z 2265..2695
- house 12 low: x 1442..1583, y -635..663, z 2259..2631
- house 13 low: x 1605..1734, y -610..595, z 2260..2319
- house 14 low: x 1743..1857, y -611..642, z 2260..2666
- house 15 low: x 1908..2032, y -594..622, z 2261..2698
- house 16 low: x 2076..2160, y -578..598, z 2265..2696
- house 2 high: x 1382..2238, y -826..853, z 2701..3200
- house 2 low: x -57..90, y -681..709, z 2261..2692
- house 3 low: x 90..240, y -715..742, z 2300..2699
- house 4 low: x 243..387, y -671..749, z 2290..2671
- house 5 low: x 409..538, y -674..700, z 2316..2634
- house 6 low: x 540..687, y -679..704, z 2261..2700
- house 7 low: x 693..836, y -682..706, z 2261..2699
- house 8 low: x 841..986, y -679..704, z 2333..2692
- house 9 low: x 990..1134, y -681..734, z 2386..2689
- scoop bow: x 5090..5520, y -668..700, z 2373..3129
- scoop wall +Y 0: x 4155..4416, y 566..664, z 2377..2564
- scoop wall +Y 1: x 4420..4718, y 516..682, z 2377..2780
- scoop wall +Y 2: x 4720..5019, y 434..673, z 2381..3016
- scoop wall +Y 3: x 5023..5090, y 432..668, z 2383..3123
- scoop wall -Y 0: x 4153..4418, y -633..-508, z 2377..2569
- scoop wall -Y 1: x 4438..4719, y -640..-462, z 2379..2781
- scoop wall -Y 2: x 4720..5018, y -635..-370, z 2386..3043
- scoop wall -Y 3: x 5021..5088, y -634..-415, z 2376..3123
- stack 1 head: x -1328..-874, y -215..241, z 4462..4808
- stack 1 shaft: x -1365..-852, y -242..274, z 2900..4448
- stack 2 head: x -2604..-2143, y -216..240, z 4450..4809
- stack 2 shaft: x -2631..-2116, y -243..271, z 2900..4447
- tank +Y: x 376..1247, y 0..713, z 3853..4691
- tank -Y: x 372..1209, y -688..-0, z 3851..4696
- coaming aft: x -631..-611, y -240..245, z 2260..2310
- coaming fore: x -436..-416, y -240..245, z 2260..2310
- coaming -Y: x -631..-416, y -240..-220, z 2260..2310
- stairs +Y: x 3970..4120, y 150..450, z 2254..2371
- stairs -Y: x 3970..4120, y -450..-150, z 2254..2371
- scoop frame (tubes and posts over the forecastle, 28 chunks of 40 cm): x 4090..4518, y -455..558, z 2374..2558

## Collision (UCX_), 190 hulls

- bulwark +Y 1: centre (-3489, 669, 2334), size 192 x 302 x 190
- bulwark +Y 10: centre (-856, 1050, 2356), size 317 x 191 x 204
- bulwark +Y 11: centre (-543, 1062, 2340), size 309 x 168 x 219
- bulwark +Y 12: centre (-256, 1086, 2292), size 312 x 214 x 126
- bulwark +Y 13: centre (41, 1104, 2275), size 307 x 266 x 96
- bulwark +Y 14: centre (349, 1045, 2339), size 326 x 156 x 175
- bulwark +Y 15: centre (651, 1028, 2292), size 325 x 143 x 79
- bulwark +Y 16: centre (953, 1014, 2338), size 324 x 124 x 223
- bulwark +Y 17: centre (1250, 1010, 2339), size 328 x 218 x 225
- bulwark +Y 18: centre (1543, 999, 2338), size 314 x 241 x 226
- bulwark +Y 19: centre (1846, 918, 2332), size 320 x 150 x 209
- bulwark +Y 2: centre (-3251, 834, 2328), size 327 x 269 x 202
- bulwark +Y 20: centre (2158, 867, 2284), size 313 x 216 x 116
- bulwark +Y 21: centre (2464, 836, 2285), size 301 x 203 x 117
- bulwark +Y 22: centre (2751, 830, 2285), size 328 x 190 x 117
- bulwark +Y 23: centre (3050, 830, 2285), size 329 x 190 x 117
- bulwark +Y 24: centre (3350, 830, 2285), size 329 x 190 x 117
- bulwark +Y 25: centre (3650, 830, 2285), size 328 x 190 x 117
- bulwark +Y 26: centre (3949, 830, 2285), size 326 x 190 x 119
- bulwark +Y 3: centre (-2943, 957, 2284), size 290 x 150 x 85
- bulwark +Y 4: centre (-2654, 986, 2282), size 318 x 147 x 97
- bulwark +Y 5: centre (-2341, 1003, 2334), size 307 x 145 x 216
- bulwark +Y 6: centre (-2057, 1005, 2336), size 316 x 136 x 221
- bulwark +Y 7: centre (-1744, 1006, 2339), size 310 x 136 x 227
- bulwark +Y 8: centre (-1454, 1014, 2326), size 292 x 134 x 155
- bulwark +Y 9: centre (-1149, 1020, 2342), size 317 x 137 x 192
- bulwark -Y 1: centre (-3470, -658, 2359), size 168 x 264 x 197
- bulwark -Y 10: centre (-852, -1012, 2354), size 325 x 196 x 211
- bulwark -Y 11: centre (-543, -1027, 2337), size 304 x 172 x 220
- bulwark -Y 12: centre (-249, -1076, 2290), size 321 x 272 x 130
- bulwark -Y 13: centre (49, -1084, 2275), size 305 x 307 x 99
- bulwark -Y 14: centre (345, -1014, 2339), size 319 x 167 x 174
- bulwark -Y 15: centre (654, -997, 2291), size 313 x 146 x 74
- bulwark -Y 16: centre (952, -974, 2338), size 313 x 129 x 221
- bulwark -Y 17: centre (1249, -983, 2337), size 322 x 207 x 222
- bulwark -Y 18: centre (1548, -969, 2340), size 327 x 236 x 226
- bulwark -Y 19: centre (1845, -888, 2333), size 317 x 135 x 205
- bulwark -Y 2: centre (-3257, -802, 2336), size 313 x 256 x 206
- bulwark -Y 20: centre (2152, -828, 2284), size 324 x 197 x 115
- bulwark -Y 21: centre (2448, -806, 2284), size 324 x 203 x 118
- bulwark -Y 22: centre (2643, -797, 2288), size 113 x 182 x 111
- bulwark -Y 22 aft of the gap: centre (2808, -800, 2284), size 211 x 190 x 118
- bulwark -Y 23 aft of the gap: centre (2943, -800, 2284), size 113 x 190 x 118
- bulwark -Y 23 gap: centre (3056, -724, 2247), size 108 x 327 x 30
- bulwark -Y 24 fore of the gap: centre (3407, -800, 2284), size 215 x 190 x 118
- bulwark -Y 24 gap: centre (3257, -800, 2244), size 82 x 190 x 37
- bulwark -Y 25: centre (3703, -800, 2284), size 205 x 190 x 118
- bulwark -Y 25 fore of the gap: centre (3545, -797, 2288), size 111 x 182 x 111
- bulwark -Y 26: centre (3948, -798, 2285), size 326 x 195 x 120
- bulwark -Y 3: centre (-2943, -923, 2305), size 315 x 156 x 45
- bulwark -Y 4: centre (-2649, -958, 2298), size 323 x 138 x 70
- bulwark -Y 5: centre (-2348, -966, 2355), size 325 x 147 x 210
- bulwark -Y 6: centre (-2052, -970, 2347), size 326 x 140 x 195
- bulwark -Y 7: centre (-1748, -971, 2351), size 302 x 131 x 181
- bulwark -Y 8: centre (-1453, -979, 2326), size 313 x 134 x 142
- bulwark -Y 9: centre (-1142, -986, 2339), size 308 x 144 x 198
- cabin 0: centre (290, 14, 3606), size 1295 x 1740 x 811
- cabin 1: centre (1588, 17, 3608), size 1292 x 1727 x 815
- casing -1 high: centre (-3390, 16, 2862), size 49 x 520 x 296
- casing -1 low: centre (-3386, 6, 2475), size 50 x 687 x 436
- casing 0 cabin: centre (-2558, 16, 3347), size 193 x 918 x 277
- casing 0 high: centre (-2907, 12, 2949), size 894 x 1326 x 497
- casing 0 low: centre (-3280, 10, 2495), size 139 x 651 x 410
- casing 1 cabin: centre (-2010, 15, 3481), size 896 x 712 x 557
- casing 1 high: centre (-2012, 8, 2950), size 895 x 1271 x 500
- casing 1 low: centre (-3134, 14, 2482), size 147 x 979 x 427
- casing 10 low: centre (-1783, 14, 2477), size 142 x 1487 x 433
- casing 11 low: centre (-1634, 14, 2487), size 147 x 1478 x 386
- casing 12 low: centre (-1492, 18, 2499), size 136 x 1468 x 380
- casing 13 low: centre (-1335, 13, 2479), size 135 x 1478 x 399
- casing 14 low: centre (-1167, 12, 2506), size 113 x 1479 x 383
- casing 15 low: centre (-1038, 13, 2503), size 143 x 1478 x 373
- casing 16 low: centre (-876, 14, 2512), size 132 x 1480 x 369
- casing 17 low: centre (-738, 1, 2483), size 139 x 1566 x 434
- casing 2 cabin: centre (-1199, 14, 3308), size 713 x 500 x 208
- casing 2 high: centre (-1114, 11, 2949), size 886 x 1254 x 498
- casing 2 low: centre (-2985, 13, 2502), size 148 x 1232 x 394
- casing 3 low: centre (-2840, 19, 2500), size 141 x 1358 x 400
- casing 4 low: centre (-2684, 12, 2489), size 147 x 1411 x 416
- casing 5 low: centre (-2534, 14, 2513), size 148 x 1446 x 374
- casing 6 low: centre (-2386, -14, 2511), size 147 x 1522 x 334
- casing 7 low: centre (-2243, -10, 2480), size 130 x 1533 x 436
- casing 9 low: centre (-1935, 28, 2480), size 149 x 1511 x 433
- cradle: centre (-899, 254, 475), size 1778 x 1556 x 950
- house 0 high: centre (74, 12, 2950), size 864 x 1728 x 499
- house 0 low: centre (-267, 13, 2476), size 96 x 1135 x 438
- house 1 high: centre (940, 13, 2950), size 863 x 1742 x 497
- house 1 low: centre (-129, 10, 2471), size 107 x 1194 x 421
- house 11 low: centre (1363, 13, 2480), size 145 x 1390 x 430
- house 12 low: centre (1513, 14, 2445), size 140 x 1297 x 372
- house 13 low: centre (1670, -8, 2290), size 130 x 1205 x 60
- house 14 low: centre (1800, 15, 2463), size 114 x 1252 x 405
- house 15 low: centre (1970, 14, 2479), size 123 x 1216 x 437
- house 16 low: centre (2118, 10, 2481), size 84 x 1177 x 432
- house 2 high: centre (1810, 14, 2950), size 856 x 1679 x 498
- house 2 low: centre (16, 14, 2477), size 147 x 1390 x 431
- house 3 low: centre (165, 13, 2499), size 149 x 1457 x 399
- house 4 low: centre (315, 39, 2480), size 144 x 1419 x 382
- house 5 low: centre (474, 13, 2475), size 129 x 1374 x 317
- house 6 low: centre (613, 12, 2480), size 146 x 1383 x 439
- house 7 low: centre (764, 12, 2480), size 143 x 1388 x 438
- house 8 low: centre (913, 13, 2513), size 145 x 1383 x 359
- house 9 low: centre (1062, 26, 2538), size 144 x 1415 x 303
- hull 0: centre (-3576, 17, 1686), size 349 x 1670 x 1077
- hull 1: centre (-3100, 15, 1617), size 599 x 2171 x 1216
- hull 10: centre (2300, 23, 1563), size 599 x 1936 x 1324
- hull 11: centre (2900, 16, 1564), size 599 x 1833 x 1322
- hull 12: centre (3501, 16, 1612), size 599 x 1833 x 1225
- hull 13: centre (4099, 16, 1612), size 599 x 1833 x 1225
- hull 14: centre (4700, 20, 1739), size 599 x 1682 x 971
- hull 15: centre (5229, 14, 2007), size 456 x 1489 x 435
- hull 2: centre (-2500, 15, 1597), size 599 x 2165 x 1254
- hull 3: centre (-2087, 15, 1592), size 222 x 2164 x 1261
- hull 3 beside the hall -Y: centre (-1788, -962, 1652), size 371 x 360 x 1144
- hull 3 beside the hall +Y: centre (-1791, 944, 1629), size 368 x 327 x 1190
- hull 4 beside the hall -Y: centre (-1307, -942, 1761), size 585 x 286 x 924
- hull 4 beside the hall +Y: centre (-1298, 948, 1587), size 597 x 336 x 1274
- hull 5 beside the hall -Y: centre (-689, -943, 1745), size 579 x 299 x 939
- hull 5 beside the hall +Y: centre (-700, 955, 1727), size 598 x 341 x 978
- hull 6: centre (-97, 14, 1587), size 593 x 2330 x 1277
- hull 7: centre (500, 15, 1578), size 598 x 2231 x 1256
- hull 8: centre (1100, 16, 1595), size 599 x 2340 x 1258
- hull 9: centre (1702, 19, 1612), size 594 x 2202 x 1227
- scoop bow: centre (5305, 16, 2751), size 430 x 1368 x 756
- scoop wall +Y 0: centre (4286, 615, 2471), size 261 x 99 x 188
- scoop wall +Y 1: centre (4569, 599, 2579), size 298 x 166 x 402
- scoop wall +Y 2: centre (4869, 553, 2698), size 299 x 239 x 635
- scoop wall +Y 3: centre (5056, 550, 2753), size 66 x 236 x 740
- scoop wall -Y 0: centre (4286, -571, 2473), size 264 x 125 x 192
- scoop wall -Y 1: centre (4578, -551, 2580), size 281 x 178 x 402
- scoop wall -Y 2: centre (4869, -503, 2714), size 298 x 264 x 657
- scoop wall -Y 3: centre (5055, -524, 2749), size 67 x 220 x 748
- stack 1 head: centre (-1101, 13, 4635), size 454 x 456 x 347
- stack 1 shaft: centre (-1108, 16, 3674), size 513 x 516 x 1547
- stack 2 head: centre (-2374, 12, 4630), size 460 x 455 x 359
- stack 2 shaft: centre (-2374, 14, 3674), size 515 x 515 x 1547
- tank +Y: centre (812, 357, 4272), size 871 x 713 x 839
- tank -Y: centre (791, -344, 4274), size 837 x 688 x 846
- keel under the hall: centre (-1185, 0, 1145), size 1580 x 800 x 290
- well deck 0: centre (2395, 0, 2203), size 300 x 1564 x 100
- well deck 1: centre (2695, 0, 2203), size 300 x 1473 x 100
- well deck 2: centre (2995, 0, 2203), size 300 x 1473 x 100
- well deck 3: centre (3295, 0, 2203), size 300 x 1473 x 100
- well deck 4: centre (3595, 0, 2203), size 300 x 1473 x 100
- well deck 5: centre (3895, 0, 2203), size 300 x 1473 x 100
- well deck 6: centre (4082, 0, 2203), size 74 x 1458 x 100
- forecastle 0: centre (4270, 0, 2314), size 300 x 1183 x 120
- forecastle 1: centre (4570, 0, 2314), size 300 x 1140 x 120
- forecastle 2: centre (4870, 0, 2314), size 300 x 970 x 120
- forecastle 3: centre (5055, 0, 2314), size 70 x 863 x 120
- walk -Y: centre (-558, -704, 2239), size 5605 x 707 x 37
- walk +Y: centre (-558, 689, 2239), size 5605 x 582 x 38
- cross walk aft of the hatch: centre (-658, 0, 2240), size 85 x 2115 x 40
- cross walk fore of the hatch: centre (-368, 0, 2240), size 135 x 2115 x 40
- cross walk -Y of the hatch: centre (-525, -641, 2240), size 180 x 832 x 40
- cross walk +Y of the hatch: centre (-525, 641, 2240), size 180 x 832 x 40
- stern platform: centre (-3505, 0, 2202), size 290 x 1400 x 100
- coaming aft: centre (-621, 2, 2285), size 20 x 485 x 50
- coaming fore: centre (-426, 2, 2285), size 20 x 485 x 50
- coaming -Y: centre (-524, -230, 2285), size 215 x 20 x 50
- stairs +Y: centre (4044, 300, 2312), size 150 x 300 x 117
- stairs -Y: centre (4044, -300, 2312), size 150 x 300 x 117
- scoop frame: 28 hulls of 40 cm chunks (bounds above)

## Sockets

- Gangway: (3150, -845, 2262), yaw -90
- Hatch: (-525, -0, 2250), yaw -0
- HoseDrum: (-889, 323, 0), yaw -0
- Stack_1: (-1107, 17, 4808), yaw -0
- Stack_2: (-2373, 16, 4809), yaw -0
