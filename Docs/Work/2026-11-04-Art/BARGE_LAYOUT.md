# Dredge barge layout (measured)

SM_DredgeBarge after the round-4 rework (Tools/Blender/looter_tripo_barge.py), measured with rays by Tools/Blender/looter_tripo_barge_check.py. UE frame, cm: pivot bottom-centre, bow +X, berth side -Y, Z up.
Bounds: x -3750..5590, y -1237..1237, z 0..4809 (9340 x 2475 x 4809). Triangles 135393.

## Well deck

- Deck z 2249..2252 (rays down on a 100 cm grid, |y| <= 650); one level.
- Clear box: x 2153 (the house's fore face, its foremost point at z +100..+180) .. 4031 (the stairs' foot), |y| <= 701 (the bulwark's inner face, nearest of 18 stations); 1878 x 1402.
- Bulwark inner face |y|: +Y 732..781, -Y 701..9000; its rim top z 2339..2342.
- Things standing on it (ray hits over z +8): none.

## Forecastle and its stairs

- Aft face x 4169 at y 0 (rays forward at z +40 across |y| <= 650: y -600 x 4193, y -450 x 4031, y -300 x 4031, y -150 x 4031, y 0 x 4169, y 150 x 4031, y 300 x 4031, y 450 x 4031, y 600 x 4188).
- Floor (the scoop's, up-facing faces at its level): x 4125..5500, z 2373 (median), between the scoop's walls at |y| 353 (x 4125), 529 (x 4425), 433 (x 4725), 352 (x 5025).
- The scoop's frame over it: 17 collision chunks (40 cm, fitted to the tubes) up to head height; the lowest tube 29 over the floor.
- Clear of the frame to 200 over the floor (rays up on a 25 cm grid; per x, the |y| runs): x 4125: -353..347; x 4225: -494..481; x 4325: -203..247; x 4425: -529..-379, -329..-204, -79..121, 221..371, 421..546; x 4525: -481..-356, -306..344, 369..519; x 4625: -450..500; x 4725: -423..452; x 4825: -389..436; x 4925: -389..411; x 5025: -340..385; x 5125: none.
- Stairs |y| 150..450 on +Y (rays down at y 300): from x 3949 z 2251, from x 3979 z 2274, from x 4009 z 2293, from x 4039 z 2312, from x 4069 z 2332, from x 4099 z 2351, from x 4119 z 2366, from x 4139 z 2369.
- Stairs |y| 150..450 on -Y (rays down at y -300): from x 3949 z 2251, from x 3979 z 2274, from x 4009 z 2293, from x 4039 z 2312, from x 4069 z 2332, from x 4099 z 2351, from x 4119 z 2365, from x 4139 z 2369.

## Side walks (casing and house)

- +Y casing walk, x -3325..-675 (106 stations): floor z 2252..2265, from the wall's foot |y| 427..779 to the bulwark's inner face |y| 754..980; clear width (z +20..+180) 146 at least (x -3075; by height +20: 146, +100: 197, +180: 239), median 194; headroom 46 at least, under 200 at 15 stations: x -3325 (67), x -2775 (48), x -2750 (49), x -2725 (50), x -2700 (46), x -2400 (144), x -2275 (111), x -2225 (171), x -1975 (58), x -1950 (59). Under 150 wide at 3 stations: x -3075 (146), x -3050 (146), x -2050 (150)
- +Y house walk, x -100..2125 (90 stations): floor z 2253..2266, from the wall's foot |y| 587..744 to the bulwark's inner face |y| 815..976; clear width (z +20..+180) 130 at least (x 1375; by height +20: 130, +100: 218, +180: 229), median 246; headroom 29 at least, under 200 at 1 stations: x 700 (29). Under 150 wide at 1 stations: x 1375 (130)
- -Y casing walk, x -3325..-675 (106 stations): floor z 2252..2268, from the wall's foot |y| 351..773 to the bulwark's inner face |y| 708..941; clear width (z +20..+180) 82 at least (x -2325; by height +20: 82, +100: 188, +180: 153), median 183; headroom 26 at least, under 200 at 19 stations: x -2825 (29), x -2800 (29), x -2775 (26), x -2750 (27), x -2400 (137), x -2350 (70), x -2300 (147), x -2275 (143), x -2225 (151), x -2200 (87). Under 150 wide at 12 stations: x -2850 (133), x -2825 (108), x -2800 (109), x -2775 (134), x -2750 (139), x -2725 (129), x -2700 (129), x -2400 (149)
- -Y house walk, x -100..2125 (90 stations): floor z 2254..2262, from the wall's foot |y| 558..718 to the bulwark's inner face |y| 792..940; clear width (z +20..+180) 158 at least (x 700; by height +20: 158, +100: 224, +180: 223), median 242; headroom 144 at least, under 200 at 1 stations: x 1500 (144). Nowhere under 150.

## Cross walk and hatch

- Cross walk: x -671 (the casing's fore face at z +120, y 0) .. -294 (the house's aft face), deck z 2251..2256.
- Hatch hole (rays down on a 5 cm grid fall past z 2150): x -611..-436, y -220..225 (3240 rays); centre (-525, 0). Coaming top z 2301..2308 on its -Y, aft and fore sides; its +Y edge open (the deck beside it z 2251..2254: where the kit puts its stair); through the hole the first thing below is at z 942. Collision in the hole from z 1880 up: none.

## Rail line and the berth gap

- The gap in the -Y bulwark (rays inboard at z +40 pass): x 3005..3300 (295 wide); its sill z 2262, the rim beside it z 2337; the hull's side outboard there |y| 830.
- The -Y rail's collision at z 2295 (points across y -845..-645, every 5 cm over x 2900..3400): open over x 3000..3300, held elsewhere.

## The hall's cavity

- Cylinder centre (-1185, 0), R 750, floor z 1300, ceiling z 2220.
- Nearest mesh from the axis (rays out in 24 directions at 6 heights): 827 at z 1320, 270 deg, 838 at z 1320, 255 deg, 858 at z 1320, 90 deg, 866 at z 1320, 240 deg.
- Deck underside over it (rays up from the floor + 50): z 2253..4771; the hull's bottom under it (rays down from the ceiling - 50): z 733..1229.
- Collision inside it: none (sample points inside a UCX hull).
- Over it the deck is not closed everywhere (Tripo made no deck inside the casing: rays up reach the stack): the hall's own ceiling closes it.

## Material slots

- TripoDredgeBarge_Hull 51167, TripoDredgeBarge_Deck 12885, TripoDredgeBarge_Super 71341.
- well deck: TripoDredgeBarge_Deck
- forecastle floor: TripoDredgeBarge_Deck
- cross walk: TripoDredgeBarge_Deck
- hull below the strake: TripoDredgeBarge_Hull
- house: TripoDredgeBarge_Super
- casing: TripoDredgeBarge_Super
- stack: TripoDredgeBarge_Super
- the well deck bulwark: TripoDredgeBarge_Super

## Deck zones (the floor boxes: x, y, top z)

- well deck 0: x 2245..2545, y -782..782, top z 2254
- well deck 1: x 2545..2845, y -746..746, top z 2254
- well deck 2: x 2845..3145, y -746..746, top z 2254
- well deck 3: x 3145..3445, y -746..746, top z 2254
- well deck 4: x 3445..3745, y -746..746, top z 2254
- well deck 5: x 3745..4045, y -746..746, top z 2254
- well deck 6: x 4045..4121, y -729..729, top z 2254
- forecastle 0: x 4121..4421, y -591..591, top z 2373
- forecastle 1: x 4421..4721, y -570..570, top z 2373
- forecastle 2: x 4721..5021, y -485..485, top z 2373
- forecastle 3: x 5021..5090, y -431..431, top z 2373
- walk -Y: x -3360..2245, y -1057..-351, top z 2257
- walk +Y: x -3360..2245, y 418..980, top z 2258
- cross walk aft of the hatch: x -700..-615, y -1057..1057, top z 2260
- cross walk fore of the hatch: x -435..-300, y -1057..1057, top z 2260
- cross walk -Y of the hatch: x -615..-435, y -1057..-225, top z 2260
- cross walk +Y of the hatch: x -615..-435, y 225..1057, top z 2260
- stern platform: x -3650..-3360, y -700..700, top z 2252
- coaming aft: x -631..-611, y -240..245, top z 2310
- coaming fore: x -436..-416, y -240..245, top z 2310
- coaming -Y: x -631..-416, y -240..-220, top z 2310
- stairs +Y: x 3971..4121, y 150..450, top z 2371
- stairs -Y: x 3971..4121, y -450..-150, top z 2371

## Footprints to keep clear of (the collision hulls' bounds)

- cabin 0: x -357..938, y -856..884, z 3200..4011
- cabin 1: x 942..2234, y -846..881, z 3200..4015
- casing -1 high: x -3414..-3365, y -244..276, z 2714..3010
- casing -1 low: x -3411..-3361, y -338..349, z 2257..2693
- casing 0 cabin: x -2654..-2461, y -443..475, z 3208..3486
- casing 0 high: x -3354..-2460, y -651..675, z 2700..3197
- casing 0 low: x -3349..-2461, y -709..736, z 2268..2700
- casing 1 cabin: x -2458..-1562, y -341..371, z 3202..3759
- casing 1 high: x -2459..-1564, y -627..644, z 2700..3200
- casing 1 low: x -2459..-1560, y -729..757, z 2261..2698
- casing 2 cabin: x -1556..-843, y -236..264, z 3204..3412
- casing 2 high: x -1557..-671, y -616..638, z 2700..3198
- casing 2 low: x -1560..-669, y -753..780, z 2279..2700
- cradle: x -1788..-10, y -524..1032, z 0..950
- house 0 high: x -358..506, y -852..876, z 2700..3199
- house 0 low: x -315..505, y -715..748, z 2257..2699
- house 1 high: x 508..1371, y -858..884, z 2701..3198
- house 1 low: x 508..1365, y -682..734, z 2261..2700
- house 2 high: x 1382..2238, y -826..853, z 2701..3200
- house 2 low: x 1378..2160, y -680..708, z 2260..2698
- scoop bow: x 5090..5520, y -668..700, z 2373..3129
- scoop wall +Y 0: x 4155..4420, y 566..664, z 2377..2564
- scoop wall +Y 1: x 4424..4721, y 495..682, z 2377..2780
- scoop wall +Y 2: x 4722..5019, y 434..673, z 2381..3016
- scoop wall +Y 3: x 5023..5090, y 432..668, z 2383..3123
- scoop wall -Y 0: x 4153..4418, y -633..-508, z 2377..2569
- scoop wall -Y 1: x 4438..4720, y -640..-462, z 2379..2781
- scoop wall -Y 2: x 4724..5018, y -635..-370, z 2386..3043
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
- stairs +Y: x 3971..4121, y 150..450, z 2254..2371
- stairs -Y: x 3971..4121, y -450..-150, z 2254..2371
- scoop frame (tubes and posts over the forecastle, 17 chunks of 40 cm): x 4320..4518, y -395..558, z 2403..2558

## Collision (UCX_), 152 hulls

- bulwark +Y 1: centre (-3489, 669, 2334), size 192 x 302 x 190
- bulwark +Y 10: centre (-870, 1050, 2356), size 289 x 191 x 204
- bulwark +Y 11: centre (-534, 1062, 2340), size 292 x 168 x 219
- bulwark +Y 12: centre (-249, 1086, 2292), size 298 x 214 x 126
- bulwark +Y 13: centre (41, 1104, 2275), size 307 x 266 x 96
- bulwark +Y 14: centre (349, 1045, 2339), size 326 x 156 x 175
- bulwark +Y 15: centre (651, 1030, 2294), size 325 x 140 x 76
- bulwark +Y 16: centre (953, 1014, 2338), size 324 x 124 x 223
- bulwark +Y 17: centre (1250, 1018, 2339), size 328 x 203 x 225
- bulwark +Y 18: centre (1543, 1002, 2338), size 314 x 233 x 226
- bulwark +Y 19: centre (1846, 918, 2332), size 320 x 150 x 209
- bulwark +Y 2: centre (-3251, 837, 2328), size 327 x 264 x 202
- bulwark +Y 20: centre (2158, 868, 2284), size 313 x 215 x 116
- bulwark +Y 21: centre (2462, 837, 2286), size 296 x 201 x 116
- bulwark +Y 22: centre (2741, 828, 2286), size 312 x 185 x 116
- bulwark +Y 23: centre (3049, 828, 2286), size 327 x 185 x 116
- bulwark +Y 24: centre (3354, 829, 2286), size 313 x 183 x 116
- bulwark +Y 25: centre (3649, 828, 2286), size 328 x 185 x 116
- bulwark +Y 26: centre (3955, 828, 2285), size 314 x 185 x 119
- bulwark +Y 3: centre (-2945, 957, 2284), size 285 x 150 x 85
- bulwark +Y 4: centre (-2649, 986, 2282), size 310 x 147 x 97
- bulwark +Y 5: centre (-2341, 1003, 2332), size 307 x 145 x 212
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
- bulwark -Y 15: centre (654, -997, 2291), size 313 x 145 x 74
- bulwark -Y 16: centre (952, -974, 2338), size 313 x 129 x 221
- bulwark -Y 17: centre (1249, -983, 2337), size 322 x 207 x 222
- bulwark -Y 18: centre (1548, -969, 2340), size 327 x 236 x 226
- bulwark -Y 19: centre (1845, -888, 2333), size 317 x 135 x 205
- bulwark -Y 2: centre (-3257, -802, 2336), size 313 x 256 x 206
- bulwark -Y 20: centre (2153, -828, 2284), size 324 x 197 x 115
- bulwark -Y 21: centre (2448, -805, 2284), size 324 x 204 x 119
- bulwark -Y 22: centre (2638, -798, 2285), size 106 x 189 x 118
- bulwark -Y 22 aft of the gap: centre (2805, -798, 2284), size 201 x 189 x 119
- bulwark -Y 23 aft of the gap: centre (2944, -798, 2285), size 114 x 188 x 118
- bulwark -Y 23 gap: centre (3057, -797, 2244), size 100 x 190 x 37
- bulwark -Y 24 fore of the gap: centre (3407, -798, 2284), size 213 x 189 x 119
- bulwark -Y 24 gap: centre (3253, -799, 2244), size 67 x 186 x 37
- bulwark -Y 25: centre (3706, -798, 2284), size 211 x 189 x 119
- bulwark -Y 25 fore of the gap: centre (3538, -798, 2285), size 106 x 189 x 118
- bulwark -Y 26: centre (3952, -796, 2285), size 318 x 193 x 119
- bulwark -Y 3: centre (-2943, -923, 2305), size 315 x 156 x 45
- bulwark -Y 4: centre (-2649, -958, 2298), size 323 x 136 x 70
- bulwark -Y 5: centre (-2348, -967, 2355), size 325 x 145 x 210
- bulwark -Y 6: centre (-2052, -970, 2347), size 326 x 138 x 195
- bulwark -Y 7: centre (-1748, -971, 2351), size 302 x 131 x 181
- bulwark -Y 8: centre (-1453, -979, 2326), size 313 x 134 x 142
- bulwark -Y 9: centre (-1142, -986, 2339), size 308 x 144 x 198
- cabin 0: centre (290, 14, 3606), size 1295 x 1740 x 811
- cabin 1: centre (1588, 17, 3608), size 1292 x 1727 x 815
- casing -1 high: centre (-3390, 16, 2862), size 49 x 520 x 296
- casing -1 low: centre (-3386, 6, 2475), size 50 x 687 x 436
- casing 0 cabin: centre (-2558, 16, 3347), size 193 x 918 x 277
- casing 0 high: centre (-2907, 12, 2949), size 894 x 1326 x 497
- casing 0 low: centre (-2905, 14, 2484), size 889 x 1446 x 432
- casing 1 cabin: centre (-2010, 15, 3481), size 896 x 712 x 557
- casing 1 high: centre (-2012, 8, 2950), size 895 x 1271 x 500
- casing 1 low: centre (-2010, 14, 2479), size 899 x 1486 x 437
- casing 2 cabin: centre (-1199, 14, 3308), size 713 x 500 x 208
- casing 2 high: centre (-1114, 11, 2949), size 886 x 1254 x 498
- casing 2 low: centre (-1114, 13, 2490), size 891 x 1533 x 421
- cradle: centre (-899, 254, 475), size 1778 x 1556 x 950
- house 0 high: centre (74, 12, 2950), size 864 x 1728 x 499
- house 0 low: centre (95, 17, 2478), size 820 x 1464 x 441
- house 1 high: centre (940, 13, 2950), size 863 x 1742 x 497
- house 1 low: centre (937, 26, 2480), size 857 x 1416 x 439
- house 2 high: centre (1810, 14, 2950), size 856 x 1679 x 498
- house 2 low: centre (1769, 14, 2479), size 782 x 1388 x 438
- hull 0: centre (-3576, 17, 1686), size 349 x 1670 x 1077
- hull 1: centre (-3100, 15, 1617), size 599 x 2171 x 1216
- hull 10: centre (2300, 23, 1563), size 599 x 1936 x 1324
- hull 11: centre (2899, 16, 1564), size 597 x 1825 x 1322
- hull 12: centre (3499, 16, 1644), size 598 x 1825 x 1163
- hull 13: centre (4099, 16, 1612), size 599 x 1825 x 1225
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
- scoop wall +Y 0: centre (4287, 615, 2471), size 265 x 99 x 188
- scoop wall +Y 1: centre (4572, 588, 2579), size 296 x 187 x 402
- scoop wall +Y 2: centre (4870, 553, 2698), size 297 x 239 x 635
- scoop wall +Y 3: centre (5056, 550, 2753), size 66 x 236 x 740
- scoop wall -Y 0: centre (4286, -571, 2473), size 264 x 125 x 192
- scoop wall -Y 1: centre (4579, -551, 2580), size 282 x 178 x 402
- scoop wall -Y 2: centre (4871, -503, 2714), size 294 x 264 x 657
- scoop wall -Y 3: centre (5055, -524, 2749), size 67 x 220 x 748
- stack 1 head: centre (-1101, 13, 4635), size 454 x 456 x 347
- stack 1 shaft: centre (-1108, 16, 3674), size 513 x 516 x 1547
- stack 2 head: centre (-2374, 12, 4630), size 460 x 455 x 359
- stack 2 shaft: centre (-2374, 14, 3674), size 515 x 515 x 1547
- tank +Y: centre (812, 357, 4272), size 871 x 713 x 839
- tank -Y: centre (791, -344, 4274), size 837 x 688 x 846
- keel under the hall: centre (-1185, 0, 1145), size 1580 x 800 x 290
- well deck 0: centre (2395, 0, 2204), size 300 x 1564 x 100
- well deck 1: centre (2695, 0, 2204), size 300 x 1492 x 100
- well deck 2: centre (2995, 0, 2204), size 300 x 1492 x 100
- well deck 3: centre (3295, 0, 2204), size 300 x 1492 x 100
- well deck 4: centre (3595, 0, 2204), size 300 x 1492 x 100
- well deck 5: centre (3895, 0, 2204), size 300 x 1492 x 100
- well deck 6: centre (4083, 0, 2204), size 76 x 1458 x 100
- forecastle 0: centre (4271, 0, 2314), size 300 x 1183 x 119
- forecastle 1: centre (4571, 0, 2314), size 300 x 1140 x 119
- forecastle 2: centre (4871, 0, 2314), size 300 x 970 x 119
- forecastle 3: centre (5056, 0, 2314), size 69 x 863 x 119
- walk -Y: centre (-558, -704, 2239), size 5605 x 707 x 37
- walk +Y: centre (-558, 699, 2239), size 5605 x 562 x 38
- cross walk aft of the hatch: centre (-658, 0, 2240), size 85 x 2115 x 40
- cross walk fore of the hatch: centre (-368, 0, 2240), size 135 x 2115 x 40
- cross walk -Y of the hatch: centre (-525, -641, 2240), size 180 x 832 x 40
- cross walk +Y of the hatch: centre (-525, 641, 2240), size 180 x 832 x 40
- stern platform: centre (-3505, 0, 2202), size 290 x 1400 x 100
- coaming aft: centre (-621, 2, 2285), size 20 x 485 x 50
- coaming fore: centre (-426, 2, 2285), size 20 x 485 x 50
- coaming -Y: centre (-524, -230, 2285), size 215 x 20 x 50
- stairs +Y: centre (4046, 300, 2312), size 150 x 300 x 117
- stairs -Y: centre (4046, -300, 2312), size 150 x 300 x 117
- scoop frame: 17 hulls of 40 cm chunks (bounds above)

## Sockets

- Gangway: (3150, -840, 2262), yaw -90
- Hatch: (-525, -0, 2250), yaw -0
- HoseDrum: (-889, 323, 0), yaw -0
- Stack_1: (-1107, 17, 4808), yaw -0
- Stack_2: (-2373, 16, 4809), yaw -0
