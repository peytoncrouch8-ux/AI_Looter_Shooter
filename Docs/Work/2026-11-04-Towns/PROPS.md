# PROPS lane result (2026-11-04 22:50; the TOWN PROPS lane's report, kept for DRESSING)

48 new models, export `Intermediate\ArtExport_TownProps` (70 FBX: the 48 + the originals the variant scripts re-export; repeatable,
0 tangent warnings; originals byte-identical). Main imports them in art round 6. Previews: `Saved/ArtPreviews/TownProps/` (one PNG per
model, `Lineup_*.png`, `Variants_*.png`, `Atlas_Handbills.png`, `*.report.json`).
Files: new `Art/Models/Props/TownClutter.py`, `TownGround.py`, `FarmYard.py`, `Art/Textures/Handbills/` (T_Handbills_*); appended to
`Boardwalk.py`, `VillageProps.py`, `Containers.py`; `Tools/Blender/looter_clutter.py` (additions only: tube/rod density/set_name,
set_scale, Dense, bar, batten, nail_heads, matrix_of, merge = the vignette baker); `Tools/Blender/looter_posters.py --handbills`.
New MIs: MI_Handbills, MI_DrumBurnt (MetalRust 0x6e5e52), MI_Sandstone (RockCliff 0xd9c9a6), MI_SteelWorn (MetalWorn 0x8e8c88),
MI_WoodHoney (WoodPlanks 0xf2d6a4); the rest reuse HouseTrim, Hay, WoodPlanks, GroundDirt, WoodEndGrain, BarkOak, Charcoal,
IronBlack, MailboxTin, Canvas, FoliagePalette, LanternGlow.

Pivots: "wall" = pivot on the wall face at its foot, the piece stands toward -Y; else the ground under the middle, front -Y.
Small props Nanite off, LODs 50/25 from 600 tris.

## TownClutter.py (W x D x H cm, tris, slots, collision)
CrateStack_A 102x83x149 2412 Trim+Hay 3 hulls (ORRIN BROS. stencil); CrateStack_B 201x91x104 3345 Trim 4 hulls; Crate_C 167x56x48
1515 Trim+Hay 1 hull; BarrelStack_A 200x95x135 3132 Trim 2 hulls (3+2 barrels on chocks); Keg_Trestle 140x78x82 2300 1 hull;
ToolBarrel 118x106x175 2092 1 hull; SackPile_B 146x127x28 1238 Trim+Hay 1 hull; LumberPile_Small 190x119x34 316 Trim+Planks 1 hull;
Sawhorse_Log 176x75x129 522 Trim+EndGrain 1 hull; BarrelFire 60x63x102 1150 DrumBurnt+Charcoal 1 hull, SOCKET_Fire (0,0,0.91);
ToolsLeaning_A (wall) 86x59x151 570 1 hull; Ladder_A (wall) 55x39x361 434 1 hull (feet 0.36 m out); Handbills_A (wall) 110x4x77 954
and Handbills_B 105x5x67 956, no collision, pivot = the cluster's middle, hang ~1.45 m up; SignBoard_Rooms/_Smith/_Feed/_Baths 87x48
688-786 no collision, hang at SignBracket's SOCKET_Sign; WagonFreight 192x521x201 5195 5 hulls Nanite full fallback (box middle
+1.06 m Y from pivot); WagonFreight_Canvas 192x530x262 5796 +Canvas; WagonFreight_Broken 192x610x202 3992; WaterBarrel_Cart
148x222x146 3128 2 hulls.
Vignettes (one merged mesh each, its pieces' hulls kept, AO over the cluster): Vignette_ShopFront_A (wall) 227x126x124 5380
Trim+Hay; _ShopFront_B (wall) 267x135x93 3563; Vignette_PorchLived_A (wall) 206x119x124 3298 Trim+Tin+Hay; Vignette_PorchEmpty_A
(wall) 277x113x140 2252 +Foliage (wind 0); Vignette_Alley_A (wall) 225x129x181 2580 +Planks; Vignette_BackYard_A (free) 191x188x115
1946 Trim+EndGrain+BarkOak.

## TownGround.py (no collision)
WeedFringe_A 209x56x49 1003 Foliage (wind in vertex colour; pivot = middle of the wall edge); WeedFringe_B 212x78x82 1272;
WeedFringe_C 56x60x50 208 (post ring, pivot = post centre); LOD screens 0.14/0.075. PlankCrossing_A 147x263x6 536 Planks+Dirt (five
20 cm planks); PlankCrossing_B 129x325x8 440 (four, one snapped); StrawLitter_A 169x132x4 366 Hay; FallenBoards_A 205x99x13 232.

## FarmYard.py
Plough 88x235x109 1014 Trim+SteelWorn 2 hulls; DragHarrow 164x184x29 1582 1 low hull; HayRake 284x218x130 3268 3 hulls Nanite full
fallback; FeedTrough 243x93x55 446 Trim+Hay 1 hull; ButterChurn 40x40x107 668 1 hull; WashKettle 113x110x141 1188 2 hulls, UNLIT
(the user's D3), SOCKET_Fire kept; Grindstone 124x59x101 776 Trim+Sandstone 1 hull.

## Variants (chain/stand like the original)
Boardwalk_4m_Worn 744 (+WoodHoney; Boardwalk_4m is 312); Boardwalk_4m_Patched 954; Boardwalk_Steps_Worn 342; LampPost_B 384 (corner
bracket, SOCKET_Light, hull on the lantern only); LampPost_C 488 (two arms, SOCKET_Light_1/_2); Barrel_C 996 (rain barrel, +MailboxTin).

## The lane's read
Strongest: wagons, crates, barrels, signs, handbills, wall vignettes, lamps. Weaker: weeds sparse/thin up close (fine at street
distance); sacks keep the flat-pillow look; tumbleweed loose; firewood end grain fresh-cut orange; the worn walk's cupping subtle.
Over the ~2.5k tri guide (Nanite off): CrateStack_B 3.3k, BarrelStack 3.1k, WaterBarrel_Cart 3.1k, vignettes up to 5.4k.
48 mesh types vs the plan's 24-type cap for the town: DRESSING chooses; vignettes stand in for many single pieces.
Copy is invented (Vane Consolidated, Orrin Bros., Pruitt's, Gravesend, Saint Ada); Rye is the only typeface.
Decisions: wood on HouseTrim at 320 px/m (the town's), iron/tin/canvas/steel tiled at 1024 px/m, burnt drum and sandstone 512-640.
