# COLLISION lane (2026-11-04): Tripo hulls that fit the looks, the ships' decks

Source: `Docs/Work/2026-11-03-ArtAudit/AUDIT.md` finding 6, plan rows 6-7; CONTRACT.md "Round 3, more lanes" (COLLISION).
Only UCX_ hulls and the ships' C++ collision changed; no visible mesh. Not imported, not built, not committed.

## How it was measured
- Hulls: the audit's metric (its collide3.py): 6000 samples of the hulls' surface, those not 2 cm inside another hull,
  distance to the visible mesh: share over 10 / 25 / 50 cm. Second column: the same without hull faces resting on the
  ground (facing down within 10 cm of z 0: under the ground, never seen or hit). Visible surface (12000 samples) outside
  every hull by over 3 / 15 cm.
- Decks: rays down every 10 cm over the deck; the walk area = rays whose visible hit lies within 15 cm of the fitted floor
  (furniture and rails left out), the same rays for old and new: share within 3 / 10 cm of the collision's top.
- Before: buildings = the exports that were imported (`Intermediate/ArtExport_TripoLitWindows` farmhouse and cottage,
  `ArtExport_TripoBuildings` barn and windmill, 2026-11-01); props and tender = `looter_tripo_wardens.build`'s SPECS boxes
  rebuilt in Blender (they give the audit's numbers exactly). After: `Intermediate/ArtExport_Collision`.
- Scripts (this session's scratchpad, `coll/`): `cm.py` (metric, capsule walker), `final.py` (this table), `measure.py`,
  `bldwalk.py` + `walks_*.json`, `deckmap.py` / `deckfit.py` / `deckprof.py` (the Lamplighter fit), `lampcheck.py` (rebuilds
  the deck from LamplighterShipParts.cpp's tables and walks it), `tenderdeck.py`, `Tripo/WardensCollision.py` (export test
  of the SPECS hooks).

## Before and after
| Model | Hulls | Off the look >10/25/50 cm | Ground faces out >10/25 | Visible outside >3/15 cm |
|---|---|---|---|---|
| SM_Farmhouse_Ransom | 13 -> 67 | 71/48/8 -> 25/10/6 | 70/44 -> 13/1 | 12/7 -> 5/0 |
| SM_Barn_Ransom | 2 -> 45 | 61/30/9 -> 9/1/0 | 68/39 -> 10/2 | 23/2 -> 7/0 |
| SM_Cottage_Ransom | 9 -> 23 | 31/14/1 -> 7/1/0 | 36/17 -> 7/1 | 20/2 -> 2/0 |
| SM_RR_Windmill | 4 -> 32 | 87/66/31 -> 44/13/1 | 89/70 -> 44/12 | 12/9 -> 2/0 |
| SM_BarricadeTable | 1 -> 8 | 88/36/4 -> 8/0/0 | 89/34 -> 8/0 | 0/0 -> 0/0 |
| SM_HoseIntake | 1 -> 3 | 90/70/52 -> 25/0/0 | 95/86 -> 6/0 | 0/0 -> 0/0 |
| SM_AnchorWinch | 1 -> 4 | 63/35/7 -> 3/0/0 | 74/41 -> 4/0 | 0/0 -> 0/0 |
| SM_LanternPost | 2 -> 11 | 67/49/24 -> 6/0/0 | 67/50 -> 7/0 | 61/40 -> 0/0 |
| SM_BarricadeBoards | 1 -> 2 | 58/4/0 -> 8/0/0 | 60/4 -> 7/0 | 0/0 -> 0/0 |
| SM_BarricadeSacks | 1 -> 1 | 57/18/0 -> 6/0/0 | 65/21 -> 6/0 | 0/0 -> 0/0 |
| SM_CompanyTender | 2 -> 19 | 83/66/42 -> 21/2/0 | 82/65 -> 21/2 | 15/7 -> 8/3 (wheels left out) |
The farmhouse's and the intake's remaining >10 cm are mostly their undersides on the ground (the intake's open mouth). The
windmill is a lattice tower: its hulls round the legs' frustum and platform, air between the legs inside them.

Lamplighter deck (walk area, share off the collision top by over 3 cm / 10 cm):
| Area (her frame, cm) | Old flat boxes | New pieces |
|---|---|---|
| Audit rectangle x -791..1055, abs y <= 360 | 78% / 36% (visible over the top >10 cm 25%, under 11%) | 10% / 0% |
| Main deck x -250..820, abs y <= 330 | 70% / 16% | 14% / 0% |
| Stern x -1140..-250, abs y <= 300 | 89% / 62% (median 12.5 cm over the box) | 6% / 0% |
| Forecastle x 1060..1280, abs y <= 200 | 75% / 24% | 0% / 0% |
Measured deck: crowned, +10 cm over DeckTop 422 on the centreline falling 4.6 deg to -18 at the bulwarks; aft of x -250 it
rises to +50 at the wheel (the old SternDeck was flat +32, the main box flat 0 under it); the forecastle +98 at x 1000
rising 6 deg to +129 at 1300 (old block flat +118), its aft edge an arc (x 1050 centreline, 990 at abs y 200, 960 at 240).
Bulwark inner face (50 cm up): abs y 392 amidships (the old rail walls stood at 360: 30 cm of invisible wall inside it),
curving to 233 at x 950 and 215 at the stern (the old walls let the player 1.4 m into the bulwark there).

Company tender (riflemen stand where the C++ puts them; they ride its hull collision):
- Fore spot was (260, 0) at 298: the crown there 305-309 (feet 7-11 cm in the iron). Now (300, 0, 305): hull 306.5, mesh 304.
- Aft spot was (-450, 0) at 298 on the old box: the crown there 232-250 (48 cm in the air). Now (-360, 0, 272): hull 274,
  mesh 270. The old aft spot is on the stern's taper (4.5 cm fall in 10 cm); -360 is the first walkable crown aft of the
  bag's hose.
- The audit's rectangle (x abs <= 450, y abs <= 150): visible vs collision top median -24 -> -10 cm, within 10 cm 21% -> 45%
  (it takes in the bag's underside and the hull's flanks; nobody walks there).

## Capsule sweeps
Player capsule: BP_LooterCharacter overrides radius 34, half-height 96 (read from the .uasset; the code's ACharacter default
is 34/88), scaled by LooterPlayerSize::Scale 0.85: radius 28.9, height 163.2. Swept at that and at the task's 34/176; step
45 (MaxStepHeight). Walker: feet on the highest hull top under the capsule, a rise over 45 cm is a wall, the capsule (2 cm
up) must clear every hull. The Tripo houses are not enterable (the scripted design: a solid body; the farmhouse's hall is a
lamplit recess behind the door), so a door sweep means reaching the door face.
- Farmhouse: yard up the steps onto the porch: passes (closest 2.2 cm). Along the porch between the posts and the wall:
  passes. Off the porch between two posts: passes. Round the house: passes. To the front door: stops 25 cm from the wall
  face (the house's belt trim at head height, 2.32-2.53 m, stands 23 cm proud). Porch deck: collision 2 cm over the boards
  (57.2-58.2 vs 55.2-56.1; it was 60.1 by the door). Steps 40 / 20 exact.
- Cottage: up the step onto the stoop to the door: passes, stops at the wall. Stoop collision 47.0 vs the boards' 46.9
  (old 37.5: feet 9 cm in the boards), step 32.7 vs 30.6 (old 30.9). The stoop is 54 cm deep and its posts stand 41 cm from the wall: the 58 cm capsule gets
  onto it between the posts, not along it. Round the cottage: passes.
- Barn: up to the big doors: stops 17 cm before the wall, at the door leaf hanging 11-30 cm proud (real). Round: passes.
- Windmill: round the windpump: passes (closest 5 cm).
- Lamplighter (the new C++ tables, rebuilt in Blender): landing to the prow marker, round the console to the wheel, rail
  to rail across the waist, down the port side to the stern, to the gangway with its span open: all pass at the player's
  size. Into the bow and stern bulwarks: stopped at their faces. Port walkway beside the chest is 66 cm (the full-size
  capsule doesn't fit); the stern beside the wheel house about 60 cm.

## Files changed
- `Tools/Blender/looter_tripo_collision.py` (new): `fit_prop` (props: recursive volume-minimising convex split, worst
  piece first, hulls cut to 48 corners), per-model hooks (`barricade_table`, `barricade_boards`, `barricade_sacks`,
  `hose_intake`, `anchor_winch`, `lantern_post`, `company_tender`), `refit_building` (walls fitted as planes to ray
  readings, banded where a wall steps or bends; `BUILDINGS`: the farmhouse's stovepipe and back cross gable, the windmill
  as parts) and `cover_leftovers` (hulls for whatever stands 10 cm or more outside: rakes, fascias, stones).
- `Tools/Blender/looter_tripo_buildings.py`: `refit_hulls` calls refit_building; `swap` fits after the trim boards and lit
  windows are placed (their faces are what the player sees at the edges), then cover_leftovers; `old_hulls()`:
  LOOTER_TRIPO_HULLS=kdop keeps the old refit for comparing.
- `Source/AI_Looter_Shooter/World/LamplighterShipParts.cpp`: 18 tilted floor pieces (DeckPieces; Deck, SternDeck, ProwDeck
  three of them), 5 deck blocks (console, its engine, locker, chest, wheel house: BlockAll, WorldDynamic), side rails along
  the measured bulwark line (20 stretches a side, tagged by side and span; ShowGangway opens the gangway's span by tag),
  spots on the measured deck (LandingLocal z 10, Holloway 50, Finch 120, engine barrels 8).
- `Source/AI_Looter_Shooter/World/CompanyTender.cpp`: DeckTop 298 replaced by HingeTop 300 (the plank's hinge on the bag
  cradle's rim) and per-spot rifleman heights (above). Needs the tender's SPECS hook: with the old box (top 298) the aft
  rifleman would spawn inside it and stand 28 cm over the hull.

## SPECS hooks in looter_tripo_wardens.py (added 2026-11-04 after REWORK; real Wardens.py export of the seven in
`Intermediate/ArtExport_CollisionWardens`, hulls identical to the numbers above)
- Import: `import looter_tripo_collision as tc`.
- In `build()`, after the `post_box` block at the end of the collision section:
  `if spec.get('collision'): spec['collision'](obj, report)` (the hook clears that model's box hulls itself).
- SPECS: CompanyTender `boxes=[], collision=tc.company_tender`; HoseIntake `boxes=[], collision=tc.hose_intake`; AnchorWinch
  `boxes=[], collision=tc.anchor_winch`; LanternPost `boxes=[], collision=tc.lantern_post` (and drop `post_box=0.075`);
  BarricadeTable `boxes=[], collision=tc.barricade_table`; BarricadeBoards `boxes=[], collision=tc.barricade_boards`;
  BarricadeSacks `boxes=[], collision=tc.barricade_sacks`. Never `boxes=[]` without the hook (no hulls = mesh collision).

## Not done, or for Main
- Import from `Intermediate/ArtExport_Collision`: SM_Farmhouse_Ransom, SM_Barn_Ransom, SM_Cottage_Ransom, SM_RR_Windmill and
  the 7 Wardens models from `Intermediate/ArtExport_CollisionWardens` (the real Wardens.py export). The other folder's
  other models (SM_Farmhouse, SM_Barn, SM_Cottage, the RR homes, SM_RR_WindmillFan) are the same scripts' unchanged ones.
- Hull counts rose (farmhouse 67, barn 45): most are small leftover hulls on trim and stones. Worth a perf row if a fight
  happens round them.
- The Lamplighter's own UCX boxes (SPECS) stay swollen (95% >10 cm) but are never used: her Hull has NoCollision.
- Not given collision on her deck: the forecastle's stair (x 1000-1050, abs y < 60: the forecastle stays not climbed),
  the 42 cm ring forward of the console (x 680-840), the overhead spars aft (2.7 m up).
- Elsewhere: Junie's launchers (PilotSortieFight.cpp, DeckPoint(..., RailOut - 40, 0)) stand where the deck is 17 cm under
  DeckTop; the tender's LauncherSpots (z 330) float 30-130 cm over its hull (dressing). The gangway hinge at the rail is 18
  cm over the deck's edge there.
- LamplighterShipParts.cpp is 556 lines (over ~500: the deck tables could move to their own file); the new Python module
  888. CODEMAP has its line.
- Tests after the build: Looter.World.Landfall*, Looter.World.PilotSortie*, the roost tests, Looter.World.Barge.* (the berth
  reads the ship's numbers; DeckTop and RailOut are unchanged).
