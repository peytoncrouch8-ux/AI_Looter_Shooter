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

## Mains 4-5

Lane MAIN45 (Main 4 "A Coal Under Ash", Main 5 "Choke Point"; MAINS_2_7.md §6-7, §14). Not generated: the user makes and
approves each (spend only when approved). Each has one swap path; until it's imported the stand-in shows. Already in the
game and reused: the Company tender (no. 14, `/Game/Art/Tripo/SM_CompanyTender`, flown by `ACompanyTenderDrop`), the dredge
hose and intake (no. 13), the loudspeaker post (no. 18), the hose-cart (M23-3).

- **M45-1. Saint Ada's Reliquary, whole** (§14 no. 28). A granite chest 1.2 x 0.75 m, 0.6 m tall (2.2 m with the plinth if
  made together: make the chest alone, the chapel's plinth is its own), banded in gold, a shallow round recess on the lid
  where her ember lies, worn carving on the front. Unbroken. Front +X, pivot at the bottom middle; sockets `SOCKET_Ember`
  (the recess's middle, +Z up) and `SOCKET_Interact` (the front's middle). Swap: `/Game/Art/Tripo/SM_ReliquaryWhole`
  (`build_area_chapel.RELIQUARY_MESHES`); stand-in: `AChapelReliquary`'s granite chest with a brass band and a dark recess.
- **M45-2. Junie's salt charges** (§14 no. 26): a canvas satchel, 0.6 m, flap open, three charges showing (swap
  `/Game/Art/Tripo/SM_SaltSatchel`: `build_area_main5.SATCHEL_MODELS` and Main 5's item mesh; stand-in SM_FlourSacks at
  0.45); and one charge, 40 cm: three salt jars wrapped in wire with a fuse and a round magnet clamp on its back, the clamp's
  face -X (it goes on the collar) (swap `/Game/Art/Tripo/SM_SaltCharge`, `AIntakeCharge::ChargeModelPath`; stand-in: three
  white cylinders and an iron disc).
- **M45-3. Company air-shaft head** (§14 no. 35). A timbered shaft head for the Providence on Larkspur High: a square
  timber collar 3 x 3 m, a pulley frame over it 4 m tall, a riveted iron cowl where the barge's hose goes in. Pivot at the
  ground middle, the cowl's opening up. Swap: `/Game/Art/Tripo/SM_AirShaftHead` (`build_area_main5.SHAFT_MODELS`); stand-in:
  a 3.2 x 3.2 x 1 m block in MI_LanternWood.
- Not Tripo (drawn or built in code): the soot handprint on the chapel's door (a decal texture to draw; stand-in
  MI_Decal_ImpactScar), the window's hose and its whip, the intake's clamp collar, the ropes, the smog pool, the second hose.

## Mains 6-7

Lane MAIN67 (Main 6 "The Pilot Light", Main 7 "Given to the Wind"; MAINS_2_7.md §10-11, §14; lanes/MAIN67.md). Not
generated: the user makes and approves each (spend only when approved). Each has one swap path (first one that exists
wins); until it's imported the stand-in shows. All Company iron in the barge's rust and VC paint, our stylized realism, real
sizes, pivot at the bottom middle, front +X unless said. Already in and reused: the dredge barge (no. 12,
`SM_DredgeBarge`: the walkable barge's hull and its wreck's stand-in), the tender (no. 14), the hose (no. 13), the
loudspeaker post (no. 18), Abel's body (TRIPO_PEOPLE).

- **M67-1. Barge deck plate and rail** (§14 no. 30): a riveted iron deck plate 10 x 10 m (and one with a hatch), a rail
  section 10 m. Swap `SM_BargeDeckPlate`, `SM_BargeRail` (`World/BargeLooks.h`; declared, not wired yet: the kit's shapes
  show). Stand-in: engine cubes in MI_Landfall_BargeIron.
- **M67-2. Smokestack, jib crane, hose drum, furnace vent** (§14 no. 32): stack 3 m across, 14 m tall, a ladder up one side
  (`SM_BargeStack`); a 12 m deck crane (`SM_BargeCrane`); the hose drum on its gantry, drum 6 m across, a jam lever on its
  side (`SM_BargeHoseDrum`); a 2 x 2 m vent grille with glowing slots (`SM_FurnaceVent`). Stand-ins:
  engine cylinders and cubes.
- **M67-3. Soul coal** (§14 no. 31): a 30 cm lump of black faceted coal with glowing cyan veins (emissive veins, no
  translucency); and a burlap sack stencilled VC. Swap `SM_SoulCoal` (`ASoulCoalLump::ModelPath`, `build_area_main7`,
  BargeFall's coal rain); stand-in SM_Rock_B tinted dark with a cyan crust.
- **M67-4. The soul furnace** (§14 no. 33): a 6 m iron furnace with pipes, a glass-windowed soul cage at its heart (the
  glass an emissive slot), an intact and a burst version. Swap `SM_SoulFurnace`, `SM_SoulFurnaceBurst` (`ABargeFurnace`);
  stand-in: shapes and a glowing core.
- **M67-5. Furnace hall** (§14 no. 34): a round riveted shell 22 m across (the hull's beam holds no more than that, so not
  the design's 26) and 9 m high, a catwalk ring at 4 m, two stokehold doors; a floor grate wedge (12 make the ring,
  `SM_FurnaceGrate`); a draft valve, a wheel on a pipe run, 1.5 m (`SM_DraftValve`); the stoker's office set (a desk, an iron
  safe, a radio set); the manifest: a thick ledger with brass VC corners (`SM_BargeManifest`, `build_area_main6`
  MANIFEST_MODELS; stand-in SM_PayrollBook). Shell and office: no swap path yet (the kit's shapes).
- **M67-6. Dredger chief stoker** (§14 no. 29): the hose-man in a furnace-proof leather hood and long gauntlets, two tanks on
  his back (the Chief Stoker; stand-in: a Hallowed hose-man, gold tag). No swap path yet (DREDGER's body per role).
- **M67-7. The barge wreck** (§14 no. 37): the barge broken in two and burning, for the canyon floor (seen from afar, LODs
  low). Swap `SM_BargeWreck` (`build_area_main6` WRECK_MODELS); stand-in: SM_DredgeBarge pitched into the canyon.
- **M67-8. Grapple launcher** (§14 no. 37): the tender's launcher as its own model for the Lamplighter's starboard rail
  (a short iron barrel on a swivel, a crank, the grapple seated in its mouth). No swap path yet (`ALamplighterLauncher`'s
  shapes).
- **M67-9. Abel's furnace lantern** (§14 no. 27): a keeper's lantern of black iron, its glass soot-black with a cyan glint
  inside. Stand-in: his ghost lantern (SM_AbelLantern) blackened by material. No swap path yet.
- (optional) **M67-10. Company hopper car** (§14 no. 38) for the depot's siding: not used yet.
- Not Tripo (built in code): the tender's gas-bag weak spot, the chains, the gangplank, the grate's heat, the vents' fire
  columns, the smog tendril and drink lines, the soul followers and the river of lights, the light wave, the fog surge.
