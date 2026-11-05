# Dredge barge layout (measured)

SM_DredgeBarge after the round-5 rework (Tools/Blender/looter_tripo_barge.py), measured with rays by Tools/Blender/looter_tripo_barge_check.py. UE frame, cm: pivot bottom-centre, bow +X, berth side -Y, Z up.
Bounds: x -3750..5590, y -1237..1237, z 0..4809 (9340 x 2475 x 4809). Triangles 100553.

## Well deck

- Deck z 2250..2252 (rays down on a 100 cm grid, |y| <= 650); one level.
- Clear box: x 2153 (the house's fore face, its foremost point at z +100..+180) .. 3958 (the stairs' foot), |y| <= 707 (the bulwark's inner face, nearest of 18 stations); 1806 x 1415.
- Bulwark inner face |y|: +Y 738..781, -Y 707..9000; its rim top z 2339..2342.
- Things standing on it (ray hits over z +8): none.

## Forecastle and its stairs

- Aft face x 4121 at y 0 (rays forward at z +40 across |y| <= 650: y -600 x 4121, y -450 x 3958, y -300 x 4032, y -150 x 3959, y 0 x 4121, y 150 x 3958, y 300 x 4032, y 450 x 3959, y 600 x 4120).
- Floor (the scoop's, up-facing faces at its level): x 4125..5500, z 2373 (median), between the scoop's walls at |y| 353 (x 4125), 529 (x 4425), 433 (x 4725), 352 (x 5025).
- The scoop's frame over it: 30 collision chunks (40 cm, fitted to the tubes) up to head height; the lowest tube 29 over the floor.
- Clear of the frame to 200 over the floor (rays up on a 25 cm grid; per x, the |y| runs): x 4125: -353..347; x 4225: -494..481; x 4325: -203..247; x 4425: -529..-379, -329..-204, -79..121, 221..371, 421..546; x 4525: -481..-356, -306..344, 369..519; x 4625: -450..500; x 4725: -423..452; x 4825: -389..436; x 4925: -389..411; x 5025: -340..385; x 5125: none.
- Stairs |y| 150..450 on +Y (rays down at y 300): from x 3901 z 2251, from x 3971 z 2273, from x 4001 z 2294, from x 4031 z 2312, from x 4061 z 2331, from x 4091 z 2352, from x 4111 z 2357, from x 4121 z 2371.
- Stairs |y| 150..450 on -Y (rays down at y -300): from x 3901 z 2251, from x 3971 z 2274, from x 4001 z 2294, from x 4031 z 2312, from x 4061 z 2331, from x 4091 z 2352, from x 4121 z 2371, from x 4131 z 2367.

## Side walks (casing and house)

- +Y casing walk, x -3325..-675 (100 stations): floor z 2253..2265, from the wall's foot |y| 427..782 to the bulwark's inner face |y| 710..980; clear width (z +20..+180) 146 at least (x -3075; by height +20: 146, +100: 152, +180: 152), median 196; headroom 228 at least, under 220 at 0 stations. Nowhere under 120.
- +Y house walk, x -100..2125 (90 stations): floor z 2253..2266, from the wall's foot |y| 587..744 to the bulwark's inner face |y| 815..976; clear width (z +20..+180) 167 at least (x 1000; by height +20: 167, +100: 223, +180: 227), median 251; headroom 360 at least, under 220 at 0 stations. Nowhere under 150.
- -Y casing walk, x -3325..-675 (106 stations): floor z 2252..2268, from the wall's foot |y| 351..894 to the bulwark's inner face |y| 708..941; clear width (z +20..+180) 124 at least (x -2225; by height +20: 126, +100: 144, +180: 124), median 184; headroom 228 at least, under 220 at 0 stations. Nowhere under 120.
- -Y house walk, x -100..2125 (90 stations): floor z 2253..2262, from the wall's foot |y| 558..718 to the bulwark's inner face |y| 792..940; clear width (z +20..+180) 170 at least (x 1375; by height +20: 170, +100: 224, +180: 223), median 244; headroom 348 at least, under 220 at 0 stations. Nowhere under 150.

## Cross walk and hatch

- Cross walk: x -671 (the casing's fore face at z +120, y 0) .. -294 (the house's aft face), deck z 2251..2256.
- Hatch hole (rays down on a 5 cm grid fall past z 2150): x -611..-436, y -220..225 (3240 rays); centre (-525, 0). Coaming top z 2301..2309 on its -Y, aft and fore sides; its +Y edge open (the deck beside it z 2251..2254: where the kit puts its stair); through the hole the first thing below is at z 942. Collision in the hole from z 1880 up: none.

## Rail line and the berth gap

- The gap in the -Y bulwark (rays inboard at z +40 pass): x 3005..3300 (295 wide); its sill z 2262, the rim beside it z 2339; the hull's side outboard there |y| 836.
- The -Y rail's collision at z 2295 (points across y -845..-645, every 5 cm over x 2900..3400): open over x 2900..2910, 3000..3300, held elsewhere.

## The hall's cavity

- Cylinder centre (-1185, 0), R 750, floor z 1300, ceiling z 2220.
- Nearest mesh from the axis (rays out in 24 directions at 6 heights): 827 at z 1320, 270 deg, 838 at z 1320, 255 deg, 858 at z 1320, 90 deg, 866 at z 1320, 240 deg.
- Deck underside over it (rays up from the floor + 50): z 2253..4771; the hull's bottom under it (rays down from the ceiling - 50): z 733..1229.
- Collision inside it: none (sample points inside a UCX hull).
- Over it the deck is not closed everywhere (Tripo made no deck inside the casing: rays up reach the stack): the hall's own ceiling closes it.

## Material slots

- TripoDredgeBarge_Hull 27652, TripoDredgeBarge_Deck 6724, TripoDredgeBarge_Super 46282, TripoDredgeBarge_Walls 19895.
- well deck: TripoDredgeBarge_Deck
- forecastle floor: TripoDredgeBarge_Deck
- cross walk: TripoDredgeBarge_Deck
- hull below the strake: TripoDredgeBarge_Hull
- house wall: TripoDredgeBarge_Walls
- casing wall: TripoDredgeBarge_Walls
- stack: TripoDredgeBarge_Super
- the well deck bulwark: TripoDredgeBarge_Super

## Deck zones (the floor boxes: x, y, top z)

- well deck 0: x 2245..2545, y -782..782, top z 2254
- well deck 1: x 2545..2845, y -758..758, top z 2254
- well deck 2: x 2845..3145, y -758..758, top z 2254
- well deck 3: x 3145..3445, y -758..758, top z 2254
- well deck 4: x 3445..3745, y -758..758, top z 2254
- well deck 5: x 3745..4045, y -758..758, top z 2254
- well deck 6: x 4045..4120, y -729..729, top z 2254
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
- casing 0 low: x -3349..-2461, y -709..736, z 2268..2700
- casing 1 cabin: x -2458..-1562, y -341..371, z 3202..3759
- casing 1 high: x -2459..-1564, y -627..644, z 2700..3200
- casing 1 low: x -2459..-1560, y -896..784, z 2261..2698
- casing 2 cabin: x -1556..-843, y -236..264, z 3204..3412
- casing 2 high: x -1557..-671, y -616..638, z 2700..3198
- casing 2 low: x -1560..-669, y -782..784, z 2266..2700
- cradle: x -1788..-10, y -524..1032, z 0..950
- house 0 high: x -358..506, y -852..876, z 2700..3199
- house 0 low: x -315..505, y -715..748, z 2257..2699
- house 1 high: x 508..1371, y -858..884, z 2701..3198
- house 1 low: x 508..1364, y -682..734, z 2261..2700
- house 2 high: x 1382..2238, y -826..853, z 2701..3200
- house 2 low: x 1374..2160, y -680..708, z 2259..2698
- scoop bow: x 5090..5520, y -668..700, z 2373..3129
- scoop wall +Y 0: x 4155..4420, y 566..664, z 2377..2564
- scoop wall +Y 1: x 4424..4720, y 516..682, z 2377..2780
- scoop wall +Y 2: x 4721..5019, y 434..673, z 2381..3016
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
- stairs +Y: x 3970..4120, y 150..450, z 2254..2371
- stairs -Y: x 3970..4120, y -450..-150, z 2254..2371
- scoop frame (tubes and posts over the forecastle, 30 chunks of 40 cm): x 4099..4518, y -455..558, z 2376..2558

## Collision (UCX_), 165 hulls

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
- bulwark +Y 21: centre (2462, 847, 2286), size 296 x 182 x 114
- bulwark +Y 22: centre (2747, 842, 2286), size 321 x 173 x 114
- bulwark +Y 23: centre (3061, 842, 2286), size 308 x 173 x 114
- bulwark +Y 24: centre (3345, 842, 2286), size 308 x 173 x 114
- bulwark +Y 25: centre (3647, 842, 2286), size 298 x 173 x 114
- bulwark +Y 26: centre (3954, 833, 2285), size 316 x 192 x 119
- bulwark +Y 3: centre (-2943, 957, 2284), size 290 x 150 x 85
- bulwark +Y 4: centre (-2654, 986, 2282), size 318 x 147 x 97
- bulwark +Y 5: centre (-2341, 1003, 2332), size 307 x 145 x 212
- bulwark +Y 6: centre (-2057, 1005, 2336), size 316 x 136 x 221
- bulwark +Y 7: centre (-1744, 1007, 2339), size 310 x 132 x 227
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
- bulwark -Y 21: centre (2446, -809, 2284), size 318 x 198 x 118
- bulwark -Y 22: centre (2634, -805, 2284), size 93 x 186 x 117
- bulwark -Y 22 aft of the gap: centre (2810, -804, 2284), size 206 x 188 x 118
- bulwark -Y 23 aft of the gap: centre (2947, -802, 2284), size 106 x 185 x 118
- bulwark -Y 23 gap: centre (3058, -805, 2244), size 100 x 187 x 36
- bulwark -Y 24 fore of the gap: centre (3406, -804, 2284), size 212 x 188 x 118
- bulwark -Y 24 gap: centre (3259, -803, 2244), size 78 x 184 x 37
- bulwark -Y 25: centre (3703, -804, 2284), size 196 x 188 x 118
- bulwark -Y 25 fore of the gap: centre (3544, -805, 2284), size 102 x 186 x 115
- bulwark -Y 26: centre (3956, -799, 2285), size 310 x 198 x 120
- bulwark -Y 3: centre (-2943, -923, 2305), size 315 x 156 x 45
- bulwark -Y 4: centre (-2649, -958, 2298), size 323 x 138 x 70
- bulwark -Y 5: centre (-2348, -966, 2355), size 325 x 147 x 210
- bulwark -Y 6: centre (-2052, -970, 2347), size 326 x 139 x 195
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
- casing 1 low: centre (-2010, -56, 2479), size 899 x 1680 x 437
- casing 2 cabin: centre (-1199, 14, 3308), size 713 x 500 x 208
- casing 2 high: centre (-1114, 11, 2949), size 886 x 1254 x 498
- casing 2 low: centre (-1114, 1, 2483), size 891 x 1566 x 434
- cradle: centre (-899, 254, 475), size 1778 x 1556 x 950
- house 0 high: centre (74, 12, 2950), size 864 x 1728 x 499
- house 0 low: centre (95, 17, 2478), size 820 x 1464 x 441
- house 1 high: centre (940, 13, 2950), size 863 x 1742 x 497
- house 1 low: centre (936, 26, 2480), size 856 x 1416 x 439
- house 2 high: centre (1810, 14, 2950), size 856 x 1679 x 498
- house 2 low: centre (1767, 14, 2478), size 786 x 1388 x 439
- hull 0: centre (-3576, 17, 1686), size 349 x 1670 x 1077
- hull 1: centre (-3100, 15, 1617), size 599 x 2171 x 1216
- hull 10: centre (2300, 23, 1563), size 599 x 1936 x 1324
- hull 11: centre (2900, 16, 1562), size 599 x 1835 x 1318
- hull 12: centre (3500, 16, 1610), size 598 x 1835 x 1221
- hull 13: centre (4101, 16, 1612), size 596 x 1835 x 1224
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
- scoop wall +Y 1: centre (4572, 599, 2579), size 296 x 166 x 402
- scoop wall +Y 2: centre (4870, 553, 2698), size 298 x 239 x 635
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
- well deck 1: centre (2695, 0, 2204), size 300 x 1515 x 100
- well deck 2: centre (2995, 0, 2204), size 300 x 1515 x 100
- well deck 3: centre (3295, 0, 2204), size 300 x 1515 x 100
- well deck 4: centre (3595, 0, 2204), size 300 x 1515 x 100
- well deck 5: centre (3895, 0, 2204), size 300 x 1515 x 100
- well deck 6: centre (4083, 0, 2204), size 75 x 1458 x 100
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
- stairs +Y: centre (4045, 300, 2312), size 150 x 300 x 117
- stairs -Y: centre (4045, -300, 2312), size 150 x 300 x 117
- scoop frame: 30 hulls of 40 cm chunks (bounds above)

## Sockets

- Gangway: (3150, -848, 2262), yaw -90
- Hatch: (-525, -0, 2250), yaw -0
- HoseDrum: (-889, 323, 0), yaw -0
- Stack_1: (-1107, 17, 4808), yaw -0
- Stack_2: (-2373, 16, 4809), yaw -0
