# Models to make in Tripo (for Landfall)

The game uses stand-ins until these arrive; each replaces its stand-in. Sizes are real-world. People: A-pose, arms out
about 45°, feet apart, empty hands, nothing held (props are separate models). Ships: make the deck flat and open where it
says walkable (Main adds the collision).

People (1-6) are imported onto the **SK_Mannequin skeleton** (lane CREW's rule: any other skeleton warns and the stand-in
stays), so in Tripo make them **humanoid, A-pose, for auto-rigging** to the mannequin. The mesh needs a `HandGrip_R`
socket if SKM_Manny_Simple has it as a mesh socket rather than on the skeleton (Delia's gun hangs there; check).

## People

1. **Delia Ransom.** Abel's mother, in her seventies, small and tough: grey hair in a bun, a dark shawl over a work dress,
   an apron, boots. 1.6 m.
2. **Captain Mercy Holloway.** The Lamplighter's captain, a woman in her forties: a long weathered coat, a captain's hat
   with a small brass lantern badge, a scar across one cheek, gloves. 1.75 m.
3. **Brother Tobiah Finch.** The lantern-keeper, a thin older man: a plain hooded robe over trousers and boots, round
   spectacles, a belt of small brass tools and wicks. 1.8 m.
4. **Junie Calloway.** The engineer, a young woman: work overalls with rolled sleeves, goggles pushed up on her head, a
   kerchief, heavy gloves, oil stains. 1.65 m.
5. **Dredger, rifleman.** Vane's company man: a black rubber coat to the knees, a gas mask with two round glass lenses and
   a short filter snout, a flat company cap, heavy boots, a belt with ammo pouches, a small brass company badge "VC" on
   the chest. 1.8 m.
6. **Dredger, hose-man.** Same company man, bulkier: a big brass and iron tank on his back with gauges and a valve, a
   heavy leather apron over the rubber coat. 1.85 m.

## Props held or used

7. **Delia's shotgun.** An old double-barrel shotgun, worn wood stock.
8. **Company rifle.** Vane's issue rifle: blued steel, a brass "VC" plate on the stock, a short scope.
9. **Soul-hose nozzle.** A brass nozzle with a pistol grip, on a short ribbed black hose (the long hose is drawn by the
   game).
10. **The Great Lantern.** A big brass lantern (1.5 m tall) on a swivel mount with glass panes and a cage, for the ship's
    prow.

## Ships

11. **The Lamplighter.** The Wardens' airship, about 30 m long: a long gas-bag above, a wooden hull with brass trim and
    dark teal paint, a **walkable open deck** about 24 x 7 m with rails, a small wheelhouse at the back with a ship's
    wheel, an engine housing with two propellers, the Great Lantern's mount at the prow. Lantern motifs, worn and lived in.
12. **Vane's dredge barge.** A huge, ugly industrial airship, 70-80 m: riveted rusty iron hull, smokestacks, furnace vents
    with orange glow, cranes, a big hose drum underneath, company paint with "VANE CONSOLIDATED". Seen mostly from afar.
13. **Dredge hose.** A segmented black hose, about 1.5 m across, one 10 m straight section with iron collars (the game
    chains sections down into the Sink), and an iron intake mouth for its end.
14. **Company tender.** Vane's small boarding skiff, about 12 m: an iron hull, a small gas-bag, two harpoon-grapple
    launchers on the bow.
15. **Grapple hook.** A big iron three-pronged grapple with a chain eye, about 1 m.

## World pieces

16. **Anchor winch.** A ground post with an iron winch drum and a crank, for mooring the ship at the ledge. 1.2 m.
17. **Porch barricade.** A few pieces: an overturned kitchen table, a stack of boards nailed across a gap, a flour sack
    pile. Each its own model.
18. **Company loudspeaker post.** A wooden pole with two big brass horn speakers and a "VC" plate, for Vane's broadcasts.
    4 m.
19. **Abel's hat.** A weathered wide-brim keeper's hat: worn felt, a lantern-soot band, dropped on the bluff path and
    later hung on Delia's rocking chair. Stand-in: SM_SheriffHat (`story_texts.READABLE_MODELS['AbelsHat']`, one name).

Later rounds (not needed for Landfall): Cornelius Vane, Lucky Ned, the Deacon, the rest of the gang.
20. **Lantern post.** The Wardens' save and respawn post: a weathered wooden post about 3.2 m tall on a cross foot, a
    braced arm reaching about 1 m out with an iron lantern (glass panes, a cage, a cap) hung from its end, a small iron
    bell hook under the arm, a Wardens' lantern badge burnt into the post at eye height. Arm toward +X, pivot at the foot,
    the glass as its own material (LanternGlow). Replaces the respawn graves and Hob's roosts.

## Mains 2-3

Lane MAIN23 (Main 2 "The Ferryman's Price", Main 3 "Under New Management"; MAINS_2_7.md §4-5, §14). Not generated: the
user makes and approves each (spend only when approved). Each has one swap path; until it's imported the stand-in shows.
Lettering: Tripo paints text badly, so make boards and the banner **blank** (the game draws the words) unless a test shows
it can letter cleanly.

- **M23-1. Sexton's ferry** (§14 no. 21). A long black wooden ferry, 9 m long, 1.6 m beam: a high curved prow with an
  iron lantern pole and arm, rotted planks, a raised stern thwart (a seat about 0.9 m over the floor: Sexton's seated model
  sits on it), one long sweep oar off the stern, chain eyes at bow and stern (it hangs on chains in fog). Length along +Y,
  pivot at the hull's middle on its gunwale line. Swap: `/Game/Art/Tripo/SM_SextonsFerry` (`ASextonsFerry::FerryModelPath`);
  the stand-in is engine cubes in MI_Charcoal.
- **M23-2. Shrouded dead** (§14 no. 22). A body wrapped in a pale linen sheet tied with rope at chest, knees and ankles,
  lying on its back, 1.8 m; and a small one, 1.1 m. Length along +X, pivot at the bottom middle. Swap:
  `/Game/Art/Tripo/SM_ShroudedDead` (`build_area_main2.SHROUD_MODELS`, first path); stand-in SM_Cocoon_Lying in MI_ClothLinen.
- **M23-3. Company hose-cart** (§14 no. 23). A two-wheeled iron pump cart, 2.2 m long: a riveted tank lying along it with
  two gauges, a brass valve wheel on top at the front, a hand-crank pump, a reel of coiled black hose at the back, a brass
  "VC" plate, short handles forward. +X toward the handles, pivot on the ground under the axle. Swap:
  `/Game/Art/Tripo/SM_CompanyHoseCart` (`ACompanyHoseCart::ModelPath`; the valve's hold spot stays the stand-in's: wheel
  about 0.4 m forward of the axle, 1.8 m up).
- **M23-4. Company checkpoint** (§14 no. 24): a sandbag wall section, 2 m long, waist high (swap
  `/Game/Art/Tripo/SM_CompanySandbags`, `build_area_main3.SANDBAG_MODELS`; stand-in SM_FlourSacks); and a blank black
  checkpoint board, 3 x 1.3 m on two posts, its face +X (swap `/Game/Art/Tripo/SM_CompanyCheckpointSign`,
  `ACompanySign::BoardModelPath`). The striped barrier pole stays the game's own (a long ACompanySign board).
- **M23-5. Company banner** (§14 no. 25). A 12 m canvas banner, 1.4 m tall, blank (the game letters UNDER NEW MANAGEMENT),
  rope eyes at each end, a slight sag. Length along +Y, pivot at its middle. Swap: `/Game/Art/Tripo/SM_CompanyBanner`
  (`ACompanyBanner::ModelPath`).
- (M23-6, already listed: the Company loudspeaker post, no. 18 above: `ACompanyLoudspeaker::ModelPath`
  `/Game/Art/Tripo/SM_CompanyLoudspeaker`; Main 2-3 make it shootable, so its horns are best near the top, facing +X.)
- Not needed: the ferry's landing and rope ladder (built in code, timber and rope), Lucky Ned's skiff (the built
  SM_Skiff_A_Gang), the ASSESSED chalk (drawn).
