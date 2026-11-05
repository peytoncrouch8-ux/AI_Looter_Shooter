# Dredge barge layout (measured)

SM_DredgeBarge after the round-5 rework (Tools/Blender/looter_tripo_barge.py), measured with rays by Tools/Blender/looter_tripo_barge_check.py. UE frame, cm: pivot bottom-centre, bow +X, berth side -Y, Z up.
Bounds: x -3750..5590, y -1237..1237, z 0..4809 (9340 x 2475 x 4809). Triangles 161973.

## Well deck

- Deck z 2250..2252 (rays down on a 100 cm grid, |y| <= 650); one level.
- Clear box: x 2153 (the house's fore face, its foremost point at z +100..+180) .. 3958 (the stairs' foot), |y| <= 707 (the bulwark's inner face, nearest of 18 stations); 1805 x 1414.
- Bulwark inner face |y|: +Y 738..781, -Y 707..9000; its rim top z 2339..2342.
- Things standing on it (ray hits over z +8): none.

## Forecastle and its stairs

- Aft face x 4121 at y 0 (rays forward at z +40 across |y| <= 650: y -600 x 4121, y -450 x 3958, y -300 x 4001, y -150 x 3958, y 0 x 4121, y 150 x 3958, y 300 x 4001, y 450 x 3958, y 600 x 4121).
- Floor (the scoop's, up-facing faces at its level): x 4125..5500, z 2373 (median), between the scoop's walls at |y| 440 (x 4125), 529 (x 4425), 433 (x 4725), 352 (x 5025).
- The scoop's frame over it: 32 collision chunks (40 cm, fitted to the tubes) up to head height; the lowest tube 29 over the floor.
- Clear of the frame to 200 over the floor (rays up on a 25 cm grid; per x, the |y| runs): x 4125: -450..450; x 4225: -494..481; x 4325: -203..247; x 4425: -529..-379, -329..-204, -79..121, 221..371, 421..546; x 4525: -481..-356, -306..344, 369..519; x 4625: -450..500; x 4725: -423..452; x 4825: -389..436; x 4925: -389..411; x 5025: -340..385; x 5125: none.
- Stairs |y| 150..450 on +Y (rays down at y 300): from x 3901 z 2250, from x 3971 z 2274, from x 4001 z 2294, from x 4031 z 2313, from x 4061 z 2333, from x 4091 z 2352, from x 4111 z 2355, from x 4121 z 2372, from x 4131 z 2369.
- Stairs |y| 150..450 on -Y (rays down at y -300): from x 3901 z 2250, from x 3971 z 2274, from x 4001 z 2294, from x 4031 z 2313, from x 4061 z 2333, from x 4091 z 2352, from x 4121 z 2372, from x 4131 z 2367.

## Side walks (casing and house)

- +Y casing walk, x -3325..-675 (107 stations): floor z 2253..2265, from the wall's foot |y| 427..929 to the bulwark's inner face |y| 752..980; clear width (z +20..+180) 157 at least (x -675; by height +20: 157, +100: 284, +180: 290), median 203; headroom 222 at least, under 220 at 0 stations. Nowhere under 120. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- +Y house walk, x -100..2125 (90 stations): floor z 2253..2264, from the wall's foot |y| 592..756 to the bulwark's inner face |y| 815..976; clear width (z +20..+180) 217 at least (x 2125; by height +20: 217, +100: 258, +180: 263), median 257; headroom 389 at least, under 220 at 0 stations. Nowhere under 150. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- -Y casing walk, x -3325..-675 (107 stations): floor z 2254..2264, from the wall's foot |y| 351..840 to the bulwark's inner face |y| 708..941; clear width (z +20..+180) 160 at least (x -675; by height +20: 160, +100: 190, +180: 284), median 194; headroom 219 at least, under 220 at 2 stations: x -1875 (219), x -1850 (219). Nowhere under 120. Measured on their neighbours' floor (none found under a low cut stub): x -2250. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.
- -Y house walk, x -100..2125 (90 stations): floor z 2252..2261, from the wall's foot |y| 560..731 to the bulwark's inner face |y| 788..945; clear width (z +20..+180) 223 at least (x 2050; by height +20: 223, +100: 240, +180: 244), median 251; headroom 883 at least, under 220 at 0 stations. Nowhere under 150. A capsule (radius 34, half-height 88) along its middle: no collision hull in its way.

## Collision convex

- 196 UCX, every one convex.

## The stern push's way (as Looter.World.Barge.Fight walks it: looter_tripo_barge_route)

- -Y: open to the well deck.
- +Y: open to the well deck.

## Cross walk and hatch

- Cross walk: x -671 (the casing's fore face at z +120, y 0) .. -294 (the house's aft face), deck z 2251..2256.
- Hatch hole (rays down on a 5 cm grid fall past z 2150): x -611..-436, y -220..225 (3240 rays); centre (-525, 0). Coaming top z 2307..2310 on its -Y, aft and fore sides; its +Y edge open (the deck beside it z 2251..2254: where the kit puts its stair); through the hole the first thing below is at z 942. Collision in the hole from z 1880 up: none.

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

- TripoDredgeBarge_Hull 48637, TripoDredgeBarge_Deck 18872, TripoDredgeBarge_Super 73839, TripoDredgeBarge_Walls 20535.
- well deck: TripoDredgeBarge_Deck
- forecastle floor: TripoDredgeBarge_Deck
- cross walk: TripoDredgeBarge_Deck
- hull below the strake: TripoDredgeBarge_Hull
- house wall: TripoDredgeBarge_Walls
- casing wall: TripoDredgeBarge_Walls
- house walk plates: TripoDredgeBarge_Walls
- casing walk plates: TripoDredgeBarge_Walls
- stack: TripoDredgeBarge_Super
- the well deck bulwark: TripoDredgeBarge_Super

## Deck zones (the floor boxes: x, y, top z)

- well deck 0: x 2245..2545, y -782..782, top z 2254
- well deck 1: x 2545..2845, y -737..737, top z 2254
- well deck 2: x 2845..3145, y -737..737, top z 2254
- well deck 3: x 3145..3445, y -737..737, top z 2254
- well deck 4: x 3445..3745, y -737..737, top z 2254
- well deck 5: x 3745..4045, y -737..737, top z 2254
- well deck 6: x 4045..4119, y -729..729, top z 2254
- forecastle 0: x 4119..4419, y -591..591, top z 2373
- forecastle 1: x 4419..4719, y -570..570, top z 2373
- forecastle 2: x 4719..5019, y -485..485, top z 2373
- forecastle 3: x 5019..5090, y -431..431, top z 2373
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
- stairs +Y: x 3969..4119, y 150..450, top z 2371
- stairs -Y: x 3969..4119, y -450..-150, top z 2371

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
- casing 1 low: x -3208..-3061, y -475..518, z 2268..2695
- casing 10 low: x -1854..-1711, y -729..751, z 2258..2694
- casing 11 low: x -1708..-1561, y -728..751, z 2307..2680
- casing 12 low: x -1560..-1424, y -725..751, z 2310..2689
- casing 13 low: x -1402..-1262, y -725..755, z 2309..2678
- casing 14 low: x -1247..-1110, y -728..754, z 2271..2697
- casing 15 low: x -1110..-963, y -729..751, z 2306..2690
- casing 16 low: x -951..-810, y -729..751, z 2260..2696
- casing 17 low: x -808..-669, y -753..762, z 2262..2700
- casing 2 cabin: x -1556..-843, y -236..264, z 3204..3412
- casing 2 high: x -1557..-671, y -616..638, z 2700..3198
- casing 2 low: x -3059..-2911, y -622..650, z 2264..2699
- casing 3 low: x -2910..-2770, y -649..678, z 2268..2700
- casing 4 low: x -2758..-2611, y -680..714, z 2274..2697
- casing 5 low: x -2609..-2461, y -711..739, z 2273..2700
- casing 6 low: x -2459..-2312, y -727..744, z 2292..2678
- casing 7 low: x -2309..-2162, y -757..750, z 2259..2698
- casing 8 low: x -2159..-2011, y -730..761, z 2279..2676
- casing 9 low: x -2003..-1860, y -730..783, z 2257..2697
- cradle: x -1788..-10, y -524..1032, z 0..950
- house 0 high: x -358..506, y -852..876, z 2700..3199
- house 0 low: x -315..-219, y -555..581, z 2257..2695
- house 1 high: x 508..1371, y -858..884, z 2701..3198
- house 1 low: x -183..-70, y -587..614, z 2259..2682
- house 10 low: x 1141..1289, y -732..692, z 2262..2675
- house 11 low: x 1290..1438, y -663..689, z 2265..2695
- house 12 low: x 1447..1590, y -636..668, z 2257..2634
- house 13 low: x 1590..1739, y -624..652, z 2259..2700
- house 14 low: x 1748..1882, y -612..637, z 2261..2677
- house 15 low: x 1891..2025, y -595..625, z 2264..2698
- house 16 low: x 2040..2160, y -588..614, z 2265..2696
- house 2 high: x 1382..2238, y -826..853, z 2701..3200
- house 2 low: x -57..89, y -617..644, z 2261..2698
- house 3 low: x 93..239, y -634..668, z 2346..2699
- house 4 low: x 247..388, y -643..675, z 2264..2603
- house 5 low: x 394..529, y -665..701, z 2264..2634
- house 6 low: x 540..690, y -675..709, z 2258..2700
- house 7 low: x 697..840, y -676..701, z 2263..2694
- house 8 low: x 841..986, y -676..706, z 2262..2692
- house 9 low: x 992..1137, y -677..713, z 2259..2692
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
- stairs +Y: x 3969..4119, y 150..450, z 2254..2371
- stairs -Y: x 3969..4119, y -450..-150, z 2254..2371
- scoop frame (tubes and posts over the forecastle, 32 chunks of 40 cm): x 4090..4518, y -456..558, z 2376..2558

## Collision (UCX_), 196 hulls

- bulwark +Y 1: centre (-3489, 669, 2334), size 192 x 302 x 190
- bulwark +Y 10: centre (-851, 1053, 2355), size 306 x 187 x 204
- bulwark +Y 11: centre (-544, 1062, 2340), size 312 x 168 x 219
- bulwark +Y 12: centre (-255, 1084, 2292), size 314 x 217 x 126
- bulwark +Y 13: centre (50, 1104, 2275), size 325 x 267 x 96
- bulwark +Y 14: centre (349, 1044, 2339), size 326 x 157 x 176
- bulwark +Y 15: centre (651, 1028, 2291), size 325 x 143 x 81
- bulwark +Y 16: centre (951, 1006, 2338), size 328 x 139 x 223
- bulwark +Y 17: centre (1250, 1010, 2301), size 328 x 218 x 151
- bulwark +Y 18: centre (1549, 996, 2338), size 327 x 248 x 226
- bulwark +Y 19: centre (1849, 919, 2332), size 326 x 156 x 209
- bulwark +Y 2: centre (-3251, 834, 2328), size 327 x 269 x 202
- bulwark +Y 20: centre (2157, 867, 2284), size 314 x 216 x 116
- bulwark +Y 21: centre (2464, 837, 2285), size 301 x 202 x 116
- bulwark +Y 22: centre (2750, 830, 2285), size 330 x 188 x 116
- bulwark +Y 23: centre (3050, 830, 2285), size 330 x 188 x 116
- bulwark +Y 24: centre (3350, 830, 2285), size 329 x 188 x 116
- bulwark +Y 25: centre (3649, 830, 2285), size 328 x 188 x 116
- bulwark +Y 26: centre (3949, 830, 2285), size 326 x 188 x 119
- bulwark +Y 3: centre (-2945, 954, 2284), size 315 x 155 x 85
- bulwark +Y 4: centre (-2650, 987, 2282), size 325 x 146 x 97
- bulwark +Y 5: centre (-2350, 1005, 2273), size 325 x 142 x 94
- bulwark +Y 6: centre (-2049, 1004, 2273), size 324 x 138 x 94
- bulwark +Y 7: centre (-1750, 1008, 2273), size 328 x 131 x 94
- bulwark +Y 8: centre (-1456, 1012, 2282), size 287 x 138 x 67
- bulwark +Y 9: centre (-1150, 1018, 2283), size 329 x 140 x 74
- bulwark -Y 1: centre (-3470, -658, 2359), size 168 x 264 x 197
- bulwark -Y 10: centre (-852, -1012, 2354), size 325 x 196 x 211
- bulwark -Y 11: centre (-548, -1027, 2337), size 314 x 172 x 220
- bulwark -Y 12: centre (-249, -1075, 2290), size 322 x 273 x 130
- bulwark -Y 13: centre (50, -1087, 2275), size 325 x 301 x 99
- bulwark -Y 14: centre (349, -1017, 2340), size 327 x 163 x 172
- bulwark -Y 15: centre (650, -997, 2291), size 325 x 145 x 74
- bulwark -Y 16: centre (952, -973, 2338), size 316 x 131 x 221
- bulwark -Y 17: centre (1252, -983, 2337), size 317 x 208 x 222
- bulwark -Y 18: centre (1549, -962, 2340), size 328 x 250 x 226
- bulwark -Y 19: centre (1846, -884, 2333), size 320 x 152 x 205
- bulwark -Y 2: centre (-3256, -802, 2336), size 316 x 256 x 206
- bulwark -Y 20: centre (2152, -828, 2284), size 324 x 197 x 115
- bulwark -Y 21: centre (2450, -805, 2284), size 329 x 206 x 118
- bulwark -Y 22: centre (2643, -795, 2288), size 114 x 186 x 111
- bulwark -Y 22 aft of the gap: centre (2807, -798, 2284), size 213 x 193 x 118
- bulwark -Y 23 aft of the gap: centre (2943, -800, 2284), size 115 x 190 x 118
- bulwark -Y 23 gap: centre (3056, -725, 2247), size 110 x 326 x 30
- bulwark -Y 24 fore of the gap: centre (3407, -798, 2284), size 215 x 193 x 118
- bulwark -Y 24 gap: centre (3258, -800, 2244), size 81 x 190 x 37
- bulwark -Y 25: centre (3706, -798, 2284), size 212 x 193 x 118
- bulwark -Y 25 fore of the gap: centre (3543, -795, 2288), size 114 x 186 x 111
- bulwark -Y 26: centre (3948, -798, 2285), size 326 x 195 x 120
- bulwark -Y 3: centre (-2943, -918, 2291), size 315 x 165 x 73
- bulwark -Y 4: centre (-2649, -952, 2294), size 323 x 150 x 77
- bulwark -Y 5: centre (-2348, -967, 2339), size 325 x 145 x 178
- bulwark -Y 6: centre (-2052, -970, 2327), size 326 x 140 x 155
- bulwark -Y 7: centre (-1750, -969, 2347), size 306 x 136 x 190
- bulwark -Y 8: centre (-1450, -977, 2326), size 320 x 138 x 142
- bulwark -Y 9: centre (-1147, -986, 2339), size 321 x 144 x 198
- cabin 0: centre (290, 14, 3606), size 1295 x 1740 x 811
- cabin 1: centre (1588, 17, 3608), size 1292 x 1727 x 815
- casing -1 high: centre (-3390, 16, 2862), size 49 x 520 x 296
- casing -1 low: centre (-3386, 6, 2475), size 50 x 687 x 436
- casing 0 cabin: centre (-2558, 16, 3347), size 193 x 918 x 277
- casing 0 high: centre (-2907, 12, 2949), size 894 x 1326 x 497
- casing 0 low: centre (-3280, 10, 2495), size 139 x 651 x 410
- casing 1 cabin: centre (-2010, 15, 3481), size 896 x 712 x 557
- casing 1 high: centre (-2012, 8, 2950), size 895 x 1271 x 500
- casing 1 low: centre (-3134, 21, 2482), size 147 x 993 x 427
- casing 10 low: centre (-1783, 11, 2476), size 142 x 1480 x 436
- casing 11 low: centre (-1634, 11, 2494), size 147 x 1479 x 373
- casing 12 low: centre (-1492, 13, 2499), size 136 x 1476 x 379
- casing 13 low: centre (-1332, 15, 2494), size 140 x 1480 x 369
- casing 14 low: centre (-1179, 13, 2484), size 136 x 1482 x 426
- casing 15 low: centre (-1036, 11, 2498), size 147 x 1480 x 383
- casing 16 low: centre (-880, 11, 2478), size 141 x 1480 x 436
- casing 17 low: centre (-738, 5, 2481), size 139 x 1515 x 438
- casing 2 cabin: centre (-1199, 14, 3308), size 713 x 500 x 208
- casing 2 high: centre (-1114, 11, 2949), size 886 x 1254 x 498
- casing 2 low: centre (-2985, 14, 2482), size 148 x 1272 x 436
- casing 3 low: centre (-2840, 14, 2484), size 140 x 1327 x 432
- casing 4 low: centre (-2684, 17, 2486), size 147 x 1394 x 423
- casing 5 low: centre (-2535, 14, 2487), size 149 x 1450 x 427
- casing 6 low: centre (-2386, 9, 2484), size 147 x 1471 x 386
- casing 7 low: centre (-2236, -4, 2478), size 147 x 1506 x 439
- casing 8 low: centre (-2085, 16, 2478), size 148 x 1491 x 397
- casing 9 low: centre (-1932, 27, 2477), size 143 x 1512 x 440
- cradle: centre (-899, 254, 475), size 1778 x 1556 x 950
- house 0 high: centre (74, 12, 2950), size 864 x 1728 x 499
- house 0 low: centre (-267, 13, 2476), size 96 x 1135 x 438
- house 1 high: centre (940, 13, 2950), size 863 x 1742 x 497
- house 1 low: centre (-127, 14, 2470), size 113 x 1201 x 423
- house 10 low: centre (1215, -20, 2468), size 148 x 1424 x 413
- house 11 low: centre (1364, 13, 2480), size 148 x 1351 x 430
- house 12 low: centre (1519, 16, 2446), size 143 x 1305 x 377
- house 13 low: centre (1664, 14, 2479), size 149 x 1276 x 441
- house 14 low: centre (1815, 13, 2469), size 134 x 1249 x 416
- house 15 low: centre (1958, 15, 2481), size 135 x 1220 x 434
- house 16 low: centre (2100, 13, 2481), size 119 x 1202 x 431
- house 2 high: centre (1810, 14, 2950), size 856 x 1679 x 498
- house 2 low: centre (16, 13, 2480), size 146 x 1261 x 437
- house 3 low: centre (166, 17, 2522), size 145 x 1302 x 353
- house 4 low: centre (318, 16, 2433), size 141 x 1318 x 340
- house 5 low: centre (461, 18, 2449), size 135 x 1366 x 370
- house 6 low: centre (615, 17, 2479), size 149 x 1384 x 442
- house 7 low: centre (768, 13, 2478), size 143 x 1376 x 431
- house 8 low: centre (913, 15, 2477), size 146 x 1382 x 431
- house 9 low: centre (1065, 18, 2476), size 145 x 1390 x 433
- hull 0: centre (-3576, 17, 1686), size 349 x 1670 x 1077
- hull 1: centre (-3100, 15, 1617), size 599 x 2171 x 1216
- hull 10: centre (2300, 23, 1563), size 599 x 1936 x 1324
- hull 11: centre (2900, 16, 1564), size 599 x 1833 x 1322
- hull 12: centre (3500, 16, 1632), size 599 x 1833 x 1187
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
- well deck 0: centre (2395, 0, 2204), size 300 x 1564 x 100
- well deck 1: centre (2695, 0, 2204), size 300 x 1474 x 100
- well deck 2: centre (2995, 0, 2204), size 300 x 1474 x 100
- well deck 3: centre (3295, 0, 2204), size 300 x 1474 x 100
- well deck 4: centre (3595, 0, 2204), size 300 x 1474 x 100
- well deck 5: centre (3895, 0, 2204), size 300 x 1474 x 100
- well deck 6: centre (4082, 0, 2204), size 74 x 1458 x 100
- forecastle 0: centre (4269, 0, 2314), size 300 x 1183 x 119
- forecastle 1: centre (4569, 0, 2314), size 300 x 1140 x 119
- forecastle 2: centre (4869, 0, 2314), size 300 x 970 x 119
- forecastle 3: centre (5055, 0, 2314), size 71 x 863 x 119
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
- scoop frame: 32 hulls of 40 cm chunks (bounds above)

## Sockets

- Gangway: (3150, -840, 2262), yaw -90
- Hatch: (-525, -0, 2250), yaw -0
- HoseDrum: (-889, 323, 0), yaw -0
- Stack_1: (-1107, 17, 4808), yaw -0
- Stack_2: (-2373, 16, 4809), yaw -0
