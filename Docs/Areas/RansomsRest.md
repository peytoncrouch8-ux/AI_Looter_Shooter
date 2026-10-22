# Ransom's Rest: the first area

First area of the Revenant campaign and the game's hub after it. Story canon: `Docs/Story.md`.
- Status: built; approved by the user 2026-10-08 after a play-through (“it all looks good”).
- **The expansion** (2026-10-13, `Docs/Areas/RansomsRest_Expansion.md`: the mountain, mission givers, Hob's roosts, the
  opening inside Ransom's Rest) overrides this doc where they differ, above all its §1 "The decisions" and §8.
- Build history and superseded text: `Docs/Archive/RansomsRest_history.md` (not loaded by default).

**Coordinates** are metres: x east, y north, origin at the centre of the play space (about 11 m east of the town gate).
`layout.json` uses centimetres with X north and Y east: X = y × 100, Y = x × 100. Heights are from the ground a thing
stands on; a negative height is a drop.

## The fantasy

- Come home dead. Ellis ran from Ransom's Rest at sixteen and helped rob it ten years later.
- A high valley in the Reaches, the frontier at the far west of Teropa's great continent. Valley and town share the name;
  locals call it “the Rest”.
- Golden late-summer farmland, a one-street town along Main Street, a white chapel on a knoll, a sinkhole full of spiders,
  and burial boards on a point of the Rim facing the sunset over Gravewind Canyon.
- Nobody left on Ransom's Rest wants to see Ellis. With the saint dark, years of the Rest's dead have drifted home to their
  headboards and stand in the chapel yard and the hayfields with nowhere to go; only Ellis sees them clearly.
- A homecoming in reverse: every place is a memory, and the last person Ellis finds is Pa.

## At a glance

- Map: `/Game/Maps/Lvl_RansomsRest`, built from `Art/Levels/RansomsRest/layout.json`. Area asset `DA_Area_RansomsRest`.
- Setting: a high valley in the Reaches, on Teropa. Ridges close it north, east and south. West it ends at the Rim, a 70 m
  drop into Gravewind Canyon, with plains running to the sunset beyond.
- Size (the valley): about 309 × 205 m with Gravewind Point and Stage Gap, about 42,800 m², about 1.5× the tutorial island
  (about 28,500 m²; both measured on their outlines). The mountain up the ore lift adds about +50%: expansion §6.
- Missions, play time, XP: expansion §2 (7 main, 9 side; main path about 74 minutes, everything about 3 h 15) and §10
  (story alone about level 5.5, a full clear about 6.8; `Docs/Progression.md`).
- Enemy levels: by where they stand, never the player's level (2026-10-12): 1–6 before ranks, rising along the story from
  the bluff (1–2) to the deck (5–6); the Gravemother set at 6, Abel 7 (`layout.json` `gameplay.levels`;
  `Docs/Progression.md`, "Creature levels"). The mountain: zones 5–6, the Foreman a set 7 (expansion §10).
- Boss: Abel Ransom, the Keeper (one of the Unpaid).
- Legendary monsters: the Gravemother, in the Sink's den; the Foreman, in the Providence mine (expansion §6).
- New enemy family: the Unpaid.
- Light: golden late afternoon, sun low in the west-southwest over the canyon; a scripted dusk for the cold open and the boss.
- Arrival: a new game plays the cold open, then the grave (expansion §1, "The opening"; Skyreach's tutorial is retired).
  Later trips arrive on the depot's platform by train.
- Exit: the depot at the east end of town, by the undertaker's yard, where the line comes in through Stage Gap. Its
  station board lists the *Gilded Lily* after Main 7, when Tilly's hearse car waits at the platform.

## Size and shape

Not an island: a high valley on Teropa. Valley floor about 245 m east–west, 205 m north–south. Gravewind Point reaches
45 m west off the Rim; Stage Gap cuts 18 m into the east ridge.

**What closes it in:**
- **West, the Rim:** a 70 m drop into Gravewind Canyon. Past it: the canyon floor in blue haze with a thin river, its lower
  far wall about 300 m off, and the plains of Teropa running to the sunset with a few far mesas. The Rimrock stands on the
  lip beside the farm; the lip is open on Gravewind Point and at Ransom's Point. The Gravewind: locals' name for the cold
  wind that pours off the Rim at dusk and out over the canyon.
- **North, Larkspur Ridge:** steep pines over an 8–12 m cliff band, crest 40–60 m up. The Old Quarry is cut into its foot
  at (20, 95). Higher forested ridges and a far blue range behind. The mountain on top (Larkspur Bench, the ore lift):
  expansion §6; the quarry's enlargement: §7.
- **East, the East Ridge:** about 35 m high, rock band at its foot. Stage Gap, x 128–146, y −5 to 11: a saddle 16–20 m wide
  between 12 m rock walls, the railroad's cut through the ridge. The line comes up out of the wooded valley, runs west through
  the cut along its south side to the depot (track, platform and train: *Zones*, the Undertaker's Yard). Past the gap the
  ground falls to the wooded valley; the line runs down into it on a short timber trestle. A rockslide buried the old stage
  road down that side, and its broken slabs bank the cut on both sides of the locomotive, so nobody walks out east.
- **South, the Hogback:** a grass ridge about 30 m high, a 6–10 m rock band at its foot, dead trees along its crest.
  Ransom's Point is its west end. Mill Creek leaves through Mill Gorge at (75, −90).

**The boundary:**
- Nothing past it can be walked: cliff bands, slopes too steep to climb, the canyon drop (Hob catches a fall), and in Stage
  Gap the parked train and the rockslide. Invisible walls behind the ridge feet are only a backstop. The mountain's neck and
  boundary: expansion §6, "The way back down, and the boundary".
- Past it, cheap generated meshes: the ridges' outer slopes, the canyon wall below the Rim's cliff kit, the canyon floor and
  its far wall. The plains and far ranges are unlit silhouettes. The line down past the gap is a few track pieces with no
  collision, run on only in the train's arrival and departure shots. Nothing past the Rim has collision. Together they
  replace the backdrop islands here (Skyreach keeps its own).
- Only the valley's own terrain is tagged `Ground`, so the minimap covers the valley and the ridge feet, dims everything past
  the boundary and never shows the canyon.

**Features and heights:**
- **Ransom's Point** (+20 m): a bluff in the south-west corner, where the Hogback meets the Rim. Top about 40 m across,
  centred at (−92, −80).
  - West face is the Rim; north and east faces stand 16 m over the farm terrace.
  - South face drops 20 m to **the Mooring Ledge** (−102, −101), a rock shelf over the Mooring Notch, a cleft of the canyon.
    The gang's skiff came up the canyon and tied up there, out of sight of town.
  - The lookout tower stands on top facing the sunset, with stairs down the cliff to the ledge.
- **The bluff path:** a 64 m ramp climbing about 16 m at about 25% (14°). Starts at the family plot's west gate (−108, −46),
  runs east along the bluff's north face, bends south along its east face, comes onto the top from the east. Two wide bends,
  no hairpins.
- **The farm terrace** (+4 m), south-west: from the Rim east to about x = −40 and north to about y = −6; the orchard runs down
  its east edge. **The town** is on the flat valley floor.
- **The chapel knoll** (+10 m) in the north; **boot hill** (+5 m) behind Main Street.
- **The Sink** (−12 m): a collapsed sinkhole 36 m across at (65, 46), with a 45 m ramp curving down its south wall. The
  Gravemother's den is a cave mouth on the east wall, under the overhang of Den Rock.
- **Mill Creek** runs from a spring at (84, −16) south through the fields to **Mill Falls** (75, −88), where the existing
  waterfall drops 14 m into Mill Gorge.
- **The Dry Wash:** a dry creek bed 8 m wide and 3 m deep, from the saddle between boot hill and the knoll (−28, 30) west to
  the Pour-off (−115, −6), where it spills over the Rim. The keeper's path crosses it on Keeper's Bridge (−95, 2).
- **Gravewind Point:** a promontory of the Rim reaching 45 m west, 70 m above the canyon floor.
  - The burial boards deck (25 × 18 m, centred at (−150, 12)) covers its flat tip; its west end is built 3 m out over the drop.
  - Two rocks at its neck, **the Keeper's Gate** (−128, 13), frame the road. The boss fight's fog wall seals the gap between them.

**What breaks up the open ground.** Outside the zones no gap is wider than about 19 m, except the crossroads at the keeper's
grave, kept clear on purpose. All from kits, instanced (adds instances, not draws): the cliff kit, a freestanding outcrop kit
(rocks seen from every side), boulders, pines, oaks, dead trees, fences, stone walls, the house trim sheet. Exact outlines
are in `layout.json`.
- *Farm side:*
  - The Rimrock (6–7 m): three pieces on the lip from (−117, −58) to (−114, −12), with two gaps onto the sunset.
  - The orchard, an avenue on both sides of the farm road.
  - Delia's salt line, the farm's fence, from the Rimrock round the orchard to the Tumble.
  - The Three Widows: three 7 m slabs at x −85 to −69, y 1 to 4.
  - The Tumble: rockfall below the bluff's east face, closing off the Mooring Ledge.
  - On the bluff top, a waist-high nest ring and two 3 m rocks south of the path.
- *West road:*
  - The Dry Wash, with 5 m willows on its lower half.
  - The sheep fold (−79, 21) and the fallen glebe wall, both low cover.
  - Widow's Pines (13 m) filling the north-west, at least 10 m off the road.
  - The Burnt Homestead (−72, 49) and the old lantern house (−102, 35), roofless ruins by the road.
  - Knee-high cairns from the keeper's grave to the Keeper's Gate.
- *Middle:*
  - The Gate Stones (on the gate's north-west side, clear of the farm fence), the old pound wall and two burnt wagons around
    the town gate.
  - The stock pens and the burnt livery behind Main Street's south side.
  - The hanging tree (20, 46) by the north road.
  - The Spoil Bank (5 m) of quarry rubble at x 16 to 42, y 56 to 76, and the quarry track up to the Old Quarry.
- *South:*
  - Coffin Rock, an 8 m flat-topped butte at (−15, −60).
  - The Nose, a spur of the Hogback at x 2 to 8, y −97 to −80.
  - The Hogback talus and the Scatter, boulders between them.
- *East:*
  - The hayfields, split by Amos's fence, the barn yard fence and a stone field wall.
  - Mill Creek's bottom, with 10 m willows along its east bank and a springhouse at the spring.
  - The Stone Teeth: 9 m rock fins at x 98 to 106, y −26 to −58.
  - The Sink road cottonwoods behind the east end of Main Street.
  - Hearse Rock, a spur walling the undertaker's yard on the north.
  - Den Rock over the den.
  - The Webwood: dead, webbed trees north of the Sink.
  - The Sink's broken rim fence, and collapsed blocks and coffins on its floor.

**Open ground:** the widest empty circle outside the zones per stretch. Fences, low walls and cairns count as breaks: they are
cover, though a player sees over them.
- The west field (between the farm, the west road and the chapel) 19 m; south of the orchard 17 m; west of the town gate
  19 m; between the chapel and the Sink 18 m; between the farm road and the town 15 m; east of Ransom's Point 17 m; the root
  of Gravewind Point 24 m; between Mill Creek and the East Ridge 15 m; north of the undertaker's yard 16 m; the north-west
  corner 8 m; the south edge west of the barn 14 m; north of the Sink 15 m.
- The two 19 m gaps are one gap, at (−57, 5): the grave's line to the bell tower, kept open on purpose. The 24 m one is the
  crossroads at the keeper's grave.
- None of it changes a fight's ground: inside every fight's area it is cover standing on that ground, and the wash, the
  gorge, Coffin Rock and the Nose are outside all of them (see *Encounters stay on one level of ground*).

**The route,** a loop of about 1 km on foot through town in the middle and at the end:
1. The grave and the farm (south-west).
2. Up the bluff path to Ransom's Point.
3. Along the farm road through the orchard into town (centre).
4. North to the chapel, then north-east into the Sink.
5. The west road to Gravewind Point, between the wash and the pines.
6. Home to the farm by the keeper's path over Keeper's Bridge.
7. East through town to the depot, where the line comes in through Stage Gap.

**Landmarks guide it.** From the grave: the bluff above (where Ellis died) and the chapel's bell tower over the orchard. The
Three Widows mark the keeper's path; Coffin Rock marks the south. At the end of the west road the Keeper's Gate frames the
deck. Down Main Street, the water tower in Stage Gap marks the depot.

**Sightlines are blocked on purpose.**
- The orchard and the farm terrace hide the town from the grave. The orchard and Coffin Rock hide Whitlock Fields from the farm.
- The chapel knoll and boot hill hide the north-east.
- The Sink is seen only from its rim; the cottonwoods behind Main Street keep it hidden from the street.
- The creek willows hide Mill Falls until you reach the fields.
- Only Ransom's Point sees the whole valley, and nothing spoils that:
  - Tall rock and trees stand where their shadow from the tower falls on nothing that matters.
  - Whatever stands on the tower's lines stays low: the nest ring under 1.2 m, the springhouse 1.5 m, the burnt livery and
    the Scatter 3 m, the gate's south rock 4 m, the wash willows 5 m.
  - The tower sees every landmark, all of Main Street, the deck and 90% of the west road.
  - It is the planned heaviest view, with the canyon and the horizon in it.
- The grave keeps its view of the bell tower: the willows stop short of that line, and the Three Widows stand to one side of
  it, well under it.

**The palette** (apart from Skyreach's green midday): golden grass, ochre soil, an amber orchard, blue larkspur along the
fences, dark pines on Larkspur Ridge, the canyon's blue haze to the west and a low warm sun.

- One level, no streaming.
- Terrain: see *How the terrain generator changes*. The den can't come from a heightfield: it is a separate mesh set into the
  Sink's wall under Den Rock. The minimap's top-down bake won't show under it (fine for a cave).

## Zones

- **Ransom Farm, Orchard and Family Plot** (spawn), (−70, −38), +4 m. Used by Main 1, 6, 7.
  - The existing Farmhouse (−58, −40) with black crepe on the door, plus the Barn, Well and Outhouse.
  - The family plot (−86, −36) under a dead oak, inside a picket fence: 8 old headboards, Ellis's fresh grave and Abel's
    frosted one.
  - The amber orchard runs down the terrace's east edge as an avenue on both sides of the farm road.
  - Delia's salt line runs along the farm fence from the Rimrock round the orchard to the Tumble and keeps the Unpaid off the
    farm: a safe zone; the town gate's Unpaid give up the chase at it.
  - The Rimrock on the Rim's lip to the west (two gaps onto the sunset); the Three Widows mark the keeper's path to the north.
- **Ransom's Point**, (−92, −80), +20 m (shape: *Size and shape*). Used by the cold open, Main 2, the new-moon payments later.
  - The lookout tower, intact, with a railing deck facing the sunset; the boards are still stained.
  - Cliff stairs down the south face to the Mooring Ledge, where the gang's mooring post still holds a frayed rope.
  - Spiders nest among the rocks at the top, inside a waist-high ring of broken rock, with two 3 m rocks south of the path.
  - The Tumble, a rockfall, fills the corner under the east face.
- **Main Street**, (28, 0). Used by Main 3, Side 1. Safe after Main 3.
  - An 80 m street from the town gate (−11, 0) east to (70, 0), with boardwalks and false fronts:
    - Bright & Daughter, Undertakers (Tilly).
    - Pruitt's General Store, a vendor: the Pruitts, heard and never seen, trade in silver through the hatch in their door
      once Main Street is safe, after Main 3 (`AVendor`, `build_area_vendors.py`).
    - The Gilt Spur saloon, boarded up (“CLOSED FOR MOURNING”).
    - The empty sheriff's office with the Rim Rangers' notice board.
    - The town memorial: two hats on a post with a wreath.
  - The existing log cabins and cottages fill the side lots (Ransom's Rest's own house kit since 2026-10-13: expansion §9).
  - Empty stock pens and the burnt livery behind the south side; cottonwoods behind the east end hide the Sink.
  - The Gate Stones, the old pound wall and two burnt wagons around the gate. Shutters slam as Ellis passes.
- **Boot Hill**, (0, 32), +5 m. Used by roaming Unpaid after Main 4.
  - A small hillside cemetery behind the street. The hanging tree by the north road; north of it the Spoil Bank, the quarry's
    rubble heap, with the quarry track up to the Old Quarry under Larkspur Ridge.
- **Undertaker's Yard and the Depot**, (100, 2). Used by Main 7, then travel. Every trip arrives and leaves here.
  - From the back of Bright & Daughter east to the depot and into Stage Gap (the rail cut between 12 m rock walls). A coffin
    shed.
  - The depot (114, 9): a timber station house with the station board by its door (rebuilt larger with rooms inside:
    expansion §7).
  - The track comes in through the gap along its south side and ends at a buffer stop at (110, −1). A low timber platform runs
    beside it from the depot to the water tower (140, 7) in the gap; a signal stands at the gap's mouth.
  - The train at the platform: the locomotive at the gap's mouth facing out, then a passenger car, then Tilly's black hearse
    car by the depot. Until Main 7 cold and shut; from Main 7 the locomotive has steam up and the hearse car's door stands open.
  - Hearse Rock walls the yard on the north; a roofless springhouse covers Mill Creek's spring behind it.
- **Chapel of Saint Ada**, (−19, 60), +10 m. Used by Main 4.
  - A white clapboard chapel with a louvered belfry and a tall steeple, the tallest building on Ransom's Rest.
  - A churchyard of about 50 headboards behind an iron fence, with dead trees.
  - Inside, the smashed Reliquary, dark and empty.
  - The fallen glebe wall runs down the knoll's west side.
- **The Sink**, (65, 46), −12 m (shape: *Size and shape*). Used by Main 5, Side 3.
  - Ringed by the cliff kit and a broken warning fence (the ramp head is its gap).
  - The webbed ramp leads down to a floor of old coffins, collapsed blocks, egg sacs and webbing.
  - Den Rock, over the den, is a dome of fractured rock on the east rim.
  - The Webwood's dead, webbed trees line the north side.
- **Whitlock Fields and Mill Creek**, (60, −58). Used by Side 2.
  - Hayfields split by a stone field wall, with hay bales; Amos's fence along the north; the Whitlock barn and the existing
    windmill inside a yard fence.
  - The creek runs south in a bottom 2 m deep to Mill Falls, where the existing waterfall drops 14 m into Mill Gorge through
    the Hogback. Willows on its east bank hide the falls until you reach the fields; the Stone Teeth, a row of rock fins,
    close the east.
  - Meadow slimes in the creek bottom, as plain wildlife.
- **Gravewind Point**, (−150, 13) (shape and deck: *Size and shape*). Used by Main 6; Abel afterwards.
  - On the timber deck: 8 burial boards on posts, 3 keeper's lantern posts, and fog rising out of the canyon at dusk.
  - The Keeper's Gate frames the deck. Knee-high cairns line the road to it from the keeper's grave at its root (−112, 17),
    near the roofless old lantern house.
- **The west road and the keeper's path** (between zones). Used by Main 6, Main 7.
  - The west road: from the chapel (−19, 54) to the deck, between the Dry Wash and Widow's Pines, past the sheep fold, the
    fallen glebe wall and the Burnt Homestead's standing chimney.
  - The keeper's path: from the farm (−70, −30) through the salt line's gate, past the Three Widows, over the wash on Keeper's
    Bridge (−95, 2).
  - Both meet at the keeper's grave.
- Save, respawn and fast-travel points are Hob's roosts, not graves: expansion §8 (ids, places, when each opens).

## Encounters stay on one level of ground

- Creatures steer without a navmesh. They won't step off a drop of more than about 4 m and only turn so far around obstacles.
  So every group lives and fights on one level of ground, and its ground's edge is where its chase stops.
- At the edge (2026-10-12, `FCreaturePursuit`) a creature doesn't turn back at once: it waits, pacing and watching, as near to
  the player as its ground lets it, and walks home when its patience runs out (10 s a hunt; a hit, or reaching the player,
  fills it again). A player on a rock or roof above its reach gets the same wait. Bosses keep their fights' own rules.
- Only a real reason ends a hunt sooner: the player gone (a fast travel), in a scene or sheltered inside Delia's salt line, or
  unseen and far for 13 s (30 s anywhere). Out of sight but near, it tracks to the last spot it knew them at and sniffs round
  it first.

Each group: where it lives; where it waits and watches.
- Bluff spiders (Main 2): the top of Ransom's Point, round the nest ring and the two 3 m rocks; the top's edge (none on the ramp).
- Town gate Unpaid (Main 3): the flat ground at the gate, the fight centred a little north-east of it so it keeps off the farm
  fence; the Gate Stones, the old pound wall and the burnt wagons give cover. Waits 30 m from the gate or at Delia's salt line,
  whichever is nearer, so no chase crosses into the farm (the wash's head is 34 m out); the salt line, a safe zone, ends a
  hunt at once.
- Chapel yard Unpaid (Main 4): inside the iron fence; the fence.
- Roaming Unpaid (after Main 4): boot hill and the north road, round the hanging tree; their own stretch.
- Sink spiders and egg-sac spiders (Main 5): the Sink floor, among the collapsed blocks and coffins; the foot of the ramp
  (nothing lives on the rim).
- The Gravemother and spiderlings: the den and the Sink floor; the foot of the ramp.
- Whitlock hands (Side 2): the barn yard; the barn yard fence.
- Slimes: Mill Creek's bottom; the banks.
- Abel's adds: spawned on the deck; the deck.
- Camp SheepFold (after Main 4): the west field round the sheep fold, stopping at the Dry Wash's north bank; its own ground.
- Camp OrchardEdge (after Main 3): south of the orchard, outside the salt line, under Coffin Rock; its own ground.
- Camp MillCreek (after Main 3): the creek bottom above Mill Falls, with the 2 m bank tops; its own ground.
- Patrol WestRoad (after Main 4): the west road below the chapel knoll, 20 m there and back; the road's corridor.
- Patrol QuarryTrack (after Main 3): the quarry track to the Old Quarry, 14 m; the track's corridor.
- Ambush Churchyard (after Main 4): the lanes between the west grave rows, rising when the player walks back in; the fence.
- Ambush Webwood (after Main 4, a set piece since 2026-10-21): round the player in the webbed band, dropping 3.5 m; its own ground.
- Main Street's set piece (Main 3, after the gate): the street between the boardwalks round the Gilt Spur, its Bell-Ringer on
  the balcony a storey up (in reach of the street's ground, 6 m band); the town's zone.
- The Providence's cave-in (Back Pay, on the way out): the upper gallery and the office, as the gallery's shift.
- The mountain's and the expansion's new fights: expansion §4 and §6, "Encounters".

- The camps, patrols and ambushes (2026-10-09, `layout.json` `gameplay.encounters`, `build_area_camps.py`) fill the walks
  between the story's fights: 14 more kills on the main path since the bigger fights (7 before), about 45 more for an explorer. Packs flank (the ends swing out up
  to 35°), a hurt Basic spider or slime may break off once with its pack gone, and a Restless or better creature's first turn
  on the player plays the rank sting.
- Obstacles keep to this rule: inside every fight's ground they are cover standing on it (boulders, low walls, wagons, a rock
  ring, blocks on the Sink floor). Every change of level (the Dry Wash, Mill Gorge, Coffin Rock, the Nose) is outside all
  fights, and nothing stands within 5 m of the bluff path.
- Check: spawn each group with `Looter.SpawnCreature` and chase it. A navmesh with cliff and ramp links comes only if this
  proves too limiting.

## Fights and set pieces

The user's calls (2026-10-21, Saved/Fight/CONTRACT.md): 6-10 enemies a normal fight (camps and patrols 5-8, the chapel yard
and the mine at the top), plus a few hand-made set pieces an area, each announced and rewarded; no plain waves, no new mixed
packs; each fight keeps its area's kinds and ranks. The fights escalate with the story. Caps: 20 near the player, 16 of the
Unpaid family. XP held by trimming the missions' (`Docs/Progression.md`). Code: `Docs/Pipeline.md` "Fight: Layout".

| Fight | When | Now (was) | Where it's set |
|---|---|---|---|
| Bluff nest | Main 2 | 5 spiders, 1 Restless (4+1) | `build_area_story.py` NEST_BASIC |
| Town gate | Main 3 step 1 | 6 Unpaid, 1 Restless, from the street's mouth and its south side (3+1) | GATE_BASIC, GATE_SPOTS |
| **Main Street** (set piece) | Main 3 from step 2 | 4 Unpaid + 1 Restless, then 5; a Bell-Ringer on the saloon balcony (new) | layout `fights` MainStreet |
| Chapel yard | Main 4 step 1 | 8 Unpaid, then 7 and a Restless (6, then 6+1) | `build_area_chapel.py` YARD_WAVE/YARD_SECOND |
| Boot hill, north road | after Main 4 | 5 each, the area's ranks (3) | ROAMING_COUNT |
| Whitlock's hands | Side 2 | 7 Unpaid, 1 Restless (5+1) | `build_area_whitlock.py` HANDS_BASIC |
| Sink floor | Main 5 | 6 spiders, area ranks (4); the sacs' 6 unchanged | `build_area_sink.py` FLOOR_COUNT |
| Depot waiting room | The Old Fare | 4 Unpaid, 1 Restless, a Bell-Ringer (3+1+1) | `build_area_missionprops_valley.py` DEPOT_BASIC |
| Tilly's cellar | Knocking Below | 5 Unpaid, 1 Restless (3+1) | CELLAR_BASIC |
| Camp SheepFold | after Main 4 | 5 Unpaid, 1 Restless (2+1) | layout `camps` |
| Camp OrchardEdge | after Main 3 | 5 Unpaid, 1 Restless, a Bell-Ringer (3+1+1) | layout `camps` |
| Camp MillCreek | after Main 3 | 5 slimes, 1 Restless (3+1) | layout `camps` |
| Patrol WestRoad | after Main 4 | 5 Unpaid, area ranks (3) | layout `patrols` |
| Patrol QuarryTrack | after Main 3 | 5 spiders, a Spitter, area ranks (3+1) | layout `patrols` |
| Ambush Churchyard | after Main 4 | 6 Unpaid, 1 Restless, a Bell-Ringer (3+1+1) | layout `ambushes` |
| **Webwood** (set piece) | after Main 4 | 4 spiders + 1 Restless, then 3; 2 Spitters on the high ground (3+1+1) | layout `ambushes` Webwood |
| Quarry crew + Abe | Dead Lift | 5 miners + Abe (4+1) | layout `fights` |
| Raising steam | Dead Lift | two waves of 2 spiders and a Spitter, 2 miners with the second (unchanged: 8) | layout `fights` |
| Crest patrol | after Dead Lift | 5 spiders, area ranks (3) | layout `fights` |
| Calder's Post / the watch | The Last Watch | 5 miners + 1 Restless (4+1) / 6 (3) | layout `fights` |
| Dinner Pail camp | after Dead Lift | 4 miners, 1 Restless, a Bell-Ringer (3+1+1) | layout `fights` |
| Spoil heap | Back Pay's way | 6 miners (3) | layout `fights` |
| Upper gallery / lower drift | Back Pay | 7 miners + 1 Restless (4+1) / 7 + 1 Restless + a Bell-Ringer (4+1+1) | layout `fights` |
| **Providence cave-in** (set piece) | Back Pay from step 6 | 4 miners + 1 Restless ahead, then 5 behind (new) | layout `fights` CaveIn |
| Abel, the Gravemother, the Foreman | | unchanged (bosses keep their own fights) | |

The three set pieces (`ASetPieceSpawner`, once a session: each sets its story flag when won, `SetPiece.MainStreet`,
`SetPiece.Webwood`, `SetPiece.CaveIn`, kept in the campaign record; its condition holds only without it). Each push has its
beat (Hob's caption line, a sting, a camera shake; the cave-in's dust from the roof); the next push comes once 3 (the
Webwood's 2) are left, or 30 s after the last. Won: Hob's line and a loot shower where it ended (2 guns at a Strongbox's luck,
2 chest-full ammo pickups and a bonus box; no silver).
- **Main Street ambush** (Main 3, after the gate, on the way to Tilly): sprung walking into the town's middle (X -1300 to 1300,
  Y 1950 to 5000) from Main 3's step 2 on. The first push rises from the street's edges by the boardwalks ahead of the player
  round the Gilt Spur, with a Bell-Ringer on the saloon's balcony deck tolling over them; the second from the alleys and
  boardwalks behind, back toward the gate. Hob: "Under the boardwalks, sunshine! The whole street's been waiting on you."
  **Safe zone:** Main Street turns safe only after Main 3 is turned in, and Tilly keeps her window shut (`Speaker_Tilly`
  hushed) from the first push until the street is won, so the turn-in, and with it the safe zone, can only come after the
  fight. A player who stood in the middle as the gate was won has to walk out and back in (the ambush rule).
- **Webwood drop** (optional, after Main 4): sprung on the Webwood's ground; the spiders drop round where the player stands
  (13 m out), five then three, and two Venom Spitters stand on the highest ground near (7150, 6400) and (6600, 8300). Hob:
  "Look up, sunshine. LOOK UP." The shower bursts where it sprang.
- **Providence cave-in** (Back Pay, from the payroll step, the Foreman down): the upper gallery, where the decline and the
  manway both come out. It springs within 13 m of the gallery's middle on the way out (not while the gallery's or the drift's
  shift or the Foreman's lair is fighting): the view shakes, dust sifts from the roof, four dead miners and a Restless one dig
  out of the walls ahead by the adit, then five behind. Hob: "The walls, sunshine! They're digging out of the walls!"

## Missions

**Superseded (2026-10-13, the user's call):** the mission list, givers, steps, rewards and XP are in
`Docs/Areas/RansomsRest_Expansion.md` §2 (the flow table), §3 (the seven mains and three old sides, reworked) and §4 (the new
sides). Every mission but Main 1 is picked up from a giver under a ? and turned in under a ! (expansion §1). XP:
`Docs/Progression.md`. The old list: the archive.

Still true here (the expansion keeps these where it says "today's"):
- Sexton's deal comes early, about 15 minutes in, on the lookout where Ellis died (decided at step 1), so the seven names are
  the hook of the first quarter hour.
- The cold open (Main 1): the gang's skiff glides out of the evening cloud in the west, low over the plains, and drops into
  Gravewind Canyon toward the Mooring Ledge under Ransom's Point; then dusk on Ransom's Point. How it now begins: expansion
  §1 and §11 item 3.
- The grave wake-up (Main 1, step 2): Ellis wakes in the family plot beside Pa's frosted grave, Hob there, and claws out of
  the grave (press Jump three times).
- Main 4: the bell won't call the Unpaid to rest, because the saint's Reliquary lies smashed and dark. Ring the chapel bell;
  looking at the Reliquary plays a two-second Grave Sight flash of the ember lifting off the lid.
- Main 5: the keepers walk the dead to the boards by lantern light. Three egg sacs, each lets out 2 spiders on the floor. Take
  the Keeper's Lantern from the webbing: it is dark. The Gravemother's den is visible from the floor.
- Main 6: starting the mission fades the Rest to dusk. The lantern goes to Gravewind Point by the west road or the keeper's
  path; both end at the keeper's grave, and the cairns lead on to the Keeper's Gate. Hang it on the keeper's post; defeat
  Abel; sit with Pa (scene): he lights the lantern from the ghost light he carries.
- Main 7: the flame leans north-east, over the ridges; Ned's page in the Ledger fills in. Tilly's hearse car, her late
  father's funeral car coupled to the train, carries Ellis between areas.
- Side 1: Ellis's wanted poster hangs all over Ransom's Rest, someone has written ALREADY under DEAD OR ALIVE. One is on the
  Rim Rangers' notice board, signed “Capt. R. Calder”, beside her note about three Ranger caches (seeds Ruth).
- Side 2: Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint went dark, and
  he drifted home to find it rotting in the field. He isn't angry yet. Only Ellis can see him. At the end he sits on his fence
  to wait for the saint to come back.
- Side 3, "Find where the Gravemother hungers": done as you go down into the Sink: anywhere on its floor, or on its ramp from
  about 2 m below the rim (the rim and the ramp's head don't count; the den needn't be entered). The arrow points at the ramp
  head on the rim. XP the first time; the Legendary loot table on every kill. She comes back on an arrival after at least 20
  minutes of play since her last death (an arrival: coming in at the station by train, or loading a session). Her fight is
  Barrow (*Enemies by rank*, The Gravemother).

Today's lines (word for word):
- Main 1, Delia (she won't open the screen door to a corpse, but sets a plate on the porch):
  *“I was to lay you on the boards tonight. Your Pa got up Wednesday night, came up through the dirt like it was fog. A keeper doesn't lie still while his saint is dark. He walks the boards at dusk.”*
- Main 2, Hob: *“Someone's waiting on you. Up top, where it happened.”* Spiders nest at the top of the bluff, and one glows
  blue. Hob: *“See the blue on that one? Fed longer on the dark. Hits harder, and its iron's better.”*
- Main 2, the deal: at the lookout, on boards still stained with Ellis's blood, Mister Sexton sits on the railing and offers
  seven embers at the new moon, *“and your father crosses.”* Ellis: *“They scattered. Where do I even start?”*
  Sexton: *“Ask a keeper. Their lanterns lean toward a saint's light.”* *“The keeper's dead.”* *“So are you, friend.”*
- Main 3: the living shutter their windows, and the Unpaid come for the corpse at the gate. Tilly Bright talks through her
  shop window: she laid out both Ransoms, and Abel's lantern wasn't on him. *“You've ruined my collar, by the way.”*
- Main 4, Father Aldana through the vestry door:
  *“A keeper's lantern can find an ember. In every town they robbed, the gang shot the keeper first and smashed his lantern. Your father's fell in the dark, whole. Spiders hoard anything a saint has touched. Look in the Sink.”*
- Main 6, Delia through the door: *“Take him the lantern. Show him the way, even if he can't go.”*
- Main 7, Hob: *“That's Purcell. The Lily's moored out that way.”* Delia hands Heirloom out through the door:
  *“A keeper's buried with his lantern, not his iron. His lantern wasn't on him, so I kept this back. He'd want you to have it. Hold the door.”*
- Abel's scene: see *The boss*.

## Enemies by rank

- **The Unpaid** (Basic), new: `AUnpaidCreature`, its own small rig. Town gate, chapel yard, boot hill, north road, Whitlock
  barn, boss adds. 160 HP and 8 damage at level 1. Floats, no legs: no walk cycle, no navmesh. Its dull red coal is the crit
  spot, tested by how close the shot line passes, like the slime's core. Lunge and shriek. Stuck or far behind, it
  phase-steps: fades out and reappears 3–5 m closer. Three clothing tints. Ledger section: Enemies.
- **Restless Unpaid** (Rare), rank variant: one in most scripted fights, plus 8% of spawns. Blue tag and coal, ×2 health
  (the tiers, 2026-10-20: `UCreatureRankSettings`; Hallowed ×3.5, Ethereal ×4), faster lunge.
- **Gravebound Unpaid** (Epic), rank variant: 2% of spawns. Purple tag and coal, ×3 health. Trait: its shriek slows the player
  for 2 s (a ring effect).
- **Meadow Wolf spider** (Basic), existing (`ASpiderCreature`): the bluff top, the chapel yard's edge, the Sink floor. 300 HP
  at level 1, head crit.
- **Restless or Gravebound Meadow Wolf** (Rare or Epic), rank variants: a placed Restless on the bluff top in Main 2;
  promotions. Epic trait: calls every spider within 30 m (the pack call, made wider and matched by a pack tag, not the exact
  class).
- **Spiderling** (Basic): a configuration of the spider, not a new class: 0.45× size, 20% health (60 HP at level 1). From the
  Gravemother's calls. Shares the Meadow Wolf's Ledger page. Uses the Basic table, so it can drop a legendary.
- **Meadow slime** (Basic), existing (`ASlimeCreature`): Mill Creek's bottom. Wildlife, not part of the story. 120 HP,
  6 damage at level 1.
- **Venom Spitter** (Basic), spider variant (`ASpitterSpider`, 2026-10-12): thinner legs, olive hide, a glowing venom sac. The
  QuarryTrack patrol and the Webwood ambush. The ranged kind: 200 HP, 10 bite at level 1; keeps its distance and lobs a glob
  of venom every 3 s (a clear tell first, then a puddle that burns). Head and sac crit; a shot to the sac in its tell chokes
  the spit. Restless spits sooner, Gravebound throws three.
- **The Bell-Ringer** (Basic), Unpaid subclass (`ABellRingerCreature`, 2026-10-12): the chapel's old ringer with a bronze
  hand bell. The OrchardEdge camp and the Churchyard ambush. The support kind, kill it first: 110 HP, 6 damage at level 1.
  Hangs back behind its pack and tolls every 7 s (a clear tell): the toll raises one Unpaid fallen lately near it, or
  steadies its pack. The bell is a crit spot; a shot to it in the tell loses the toll. Gravebound's cracked bell slows the player.
- **The Gravemother** (Legendary): spider subclass at 1.8× size (SK_Gravemother, Spider.py's rig in a mud-grey hide, once
  imported; until then SK_Spider in the pale hide); the den is its own mesh. The Sink's den. Orange tag, ×12 health (about
  5,000 at her set level 6). No climbing, no leaps. Its own Ledger page. Her fight is **Barrow** (the user's pick
  2026-10-15, built 2026-10-20; sheets `Saved/Concepts/Gravemother/Barrow_*.png`):
  - **Headstone** (the user's pick over plain armour): a granite headstone root-bound to her abdomen, its own health (20% of
    hers), intact / cracked at 66% of it / shattered at 33%; shots on it hurt the stone, not her (sparks, chips). Broken it
    cracks off (her long stagger, 2.5 s, not her last third); at her 33% it cracks off by itself (no stagger). A thin granite
    bar under her boss bar while it stands.
  - **Crits:** her head always; her abdomen's soft rear (under the stone's back edge) while the stone stands; all of her
    abdomen once it's off.
  - **Furrow** (was the charge): 5-16 m, a 2 s tell (head low, front legs in the floor; the ground heaves and splits along
    her line, all of it by 1.8 s, dust jetting), then she ploughs the line head-low (twice her bite, run down once) and a
    berm rises each side behind her (cover 20 s, then they crumble). Never off a drop or through a wall, berm or rubble.
    Missed, 2 s down (the window).
  - **Grave Clod** (was the spit; in her fight, 7-22 m, the lob's way clear): front legs dig in, the clod torn up at 0.4 s and
    overhead by 0.8 s, lobbed at 1.2 s at where the player stands (1.5 s to 20 m, a landing shadow growing there); it bursts
    (1.5× bite at its middle, half at 2 m) and leaves rubble (cover 10 s, then it settles). Shot in her grip it bursts on her:
    stunned 2 s ("Stunned" on her bar, stars), no damage.
  - **Unearthed** (was the brood): at 66% and 33% her roar's slam raises 4 mounds round her (a call she can't roar for comes
    up 2.5 s later anyway); they bulge 1 s, then spiderlings dig out (the spider at 0.45×, 20% health). A mound shot while it
    bulges caves in: no spiderling, the shooter gets its kill XP.
  - Kept: her bite, the roar, the Gravequake (from 66%), staggers from crits, the bar, phases, loot shower, walking home.
  - **Last third** (her 33%): +25% chase and plough speed. Tells never shorten. Cooldowns: furrow 8/7/5.5 s, clod 7/6/4.5 s,
    quake 9/7 s by phase.
- **Abel Ransom, the Keeper** (Boss), on the Unpaid rig: Gravewind Point. See *The boss*.
- The mountain's dead miners and the Foreman: expansion §6.
- The egg sacs are shootable props modeled on `ATargetDummy`, not enemies. Everything a spawner, an egg sac or a boss creates
  is gone for good when killed; only placed creatures come back. Every enemy that can die uses at least the Basic loot table.

## NPCs

- **Grandma Delia Ransom:** the farmhouse screen door. A talking door: a lamp behind the screen and captions. She won't open it
  to a corpse. Main 1, 6, 7.
- **Tilly Bright:** Bright & Daughter's window. A talking window. Her shop sign reads “Back after the funeral” until vendors
  exist. Main 3; vendor later; her hearse car (Main 7, then travel).
- **Father Moses Aldana:** the chapel's vestry door. A talking door. Main 4.
- **Hob:** everywhere in the story; never on Skyreach. A small crow rig. Perches near the next objective, makes dry captions
  only Ellis hears, comments on ranks, respawns and falls. All missions.
- **Mister Sexton:** the railing on Ransom's Point. A seated model, the game's first human: undertaker's coat, stovepipe hat,
  face in shadow, a ledger on his knee. A posed idle only. Main 2, the cold open.
- **Abel Ransom:** Gravewind Point. The Unpaid rig at 1.3× in a keeper's coat and hat, with a ghost lantern and a spectral
  Ranchhand pump (Heirloom's silhouette). After the fight he sits on his board for the rest of the game, with new lines after
  each ember. Main 6.
- **Amos Whitlock:** his fence in Whitlock Fields, by the gate where the fields path passes. The Unpaid rig in work clothes,
  his coal banked low. Side 2.
- **Ranger Ruth Calder:** not in person here: her signature and note on the notice board. Side 1.
- **Townsfolk:** heard, not seen. Doors bolt and shutters slam. Half of Ransom's Rest left when the saint went dark, which keeps
  the cast small.
- Who gives which mission, and the expansion's new characters (Thaddeus Bright, Hiram Coyle, Mr. Sloane, Josiah Fenn,
  Mrs. Pruitt, Cyrus Teague the Foreman): expansion §2–4 and §11 item 14.
- Every new creature and character gets a Ledger page, checked by `Looter.Bestiary.Entries`. Delia, Tilly, Aldana, Sexton and
  Ruth get story-character pages that need no actor. Abel has one page: Enemies until “The Gravewind” is done, then Friends.

## The boss: Abel Ransom, the Keeper

His look: *NPCs*; he holds his ghost lantern in one hand and the spectral twin of his Ranchhand pump in the other.

**Arena.** The burial boards deck on Gravewind Point at dusk (*Size and shape*), flat, its 8 burial boards on posts for cover,
with the 3 keeper's lantern posts. The sun sets behind him. A fog wall seals the Keeper's Gate while the fight runs.

**Numbers.**
- Level: set at 7 (a 6 and the Boss rank's +1), whatever the player's level (2026-10-12). The story's hardest fight, over his
  adds (5–6) and the Gravemother (6).
- Health: about 40× an Unpaid of his level, about 9,500 at 7. Tuned to a 3–4 minute fight with same-level Uncommon guns.
- Crit spot: his coal heart, gold like every boss's (the Boss rank's color, clear of the legendary orange), at the standard
  1.5×. His lantern arm covers it while he fights; open only when he grieves.

**Phase 1, “You brought them here” (100–60%).**
- Floats along the boards and fires spectral buckshot: slow, visible pellets in a cone after a one-second lantern flare.
- Up close he lunges with the shotgun stock.
- Every 25 s two Unpaid rise through the boards (at most 4).
- Every 12 s he stops and turns toward the sunset for 3 s, and the coal is open.

**Phase 2, “The bell” (60–25%).**
- He drifts out into the fog over the canyon and can't be targeted.
- The chapel bell tolls across the valley, and 8 Unpaid rise through the deck in two waves.
- The three keeper's lanterns go dark. Relighting them (hold Interact for 1.5 s each, under pressure) drags him back.

**Phase 3, “Let me go” (25–0%).**
- The Gravewind pours off the point toward the sunset: wisps and mild gusts toward the deck's open end. A fall off the deck is
  caught by Hob, as everywhere in the story: fall recovery puts the player back on the deck within a second.
- Abel tries to walk off into the wind, and the dark saint pulls him back. Each pull stuns him for 3 s with the coal open.
  Between pulls he fights faster.

**At zero** he kneels, the coal sinks to an ember, and the scene plays.
- *“...El? You came home.”*
- He tells it straight: Ned shot him, the Deacon wept, and a tall fella sat on the lookout rail and never lifted a finger.
- *“Bring Saint Ada home, El. I'll wait.”*
- He lights the Keeper's Lantern from the ghost light he carries, and its flame leans north-east, over the ridges, toward Ned.
- He sits on his board facing the sunset.
- Hob: *“Well. I've seen worse reunions.”*

**Why it lands.** The player fights the cost of what Ellis helped do. The best damage comes while Pa mourns, and winning
doesn't free him. His last act as Keeper is to light the way for the whole campaign.

**Death and reset.** Ellis wakes at the nearest found roost (the Keeper's Crossroads roost by the keeper's grave, expansion
§8). Abel, his adds and the lanterns reset, and the fog wall drops.

**Loot.** The Boss table once: 3 guns at Luck 1.3, a 30% chance of a legendary. In the story Abel is fought once; afterwards
he is a friend (a separate, non-hostile actor). His adds use the Basic table.

**The Vigil Stone** (2026-10-12, the user's call: a pedestal with a menu beside each boss arena, to fight the boss again).
- Once a boss is beaten, a stone with grave candles and an iron bell rises out of the ground beside its arena the first time
  the player comes near: the Gravemother's at the Sink's ramp head, Abel's outside the Keeper's Gate (`build_area_shrines.py`).
- A tap of Interact opens its menu: the boss's name and level, when it was last laid to rest, how many times it was beaten,
  what it drops.
- “Call it back” works five minutes of play after the boss's last death, or at once after a Save & Quit; until then the menu
  shows the countdown.
- The fight starts as the first did (seal, intro, phases) but with no scene, no mission and no story reward. Its loot shower
  rolls exactly as the first kill's (the same table and luck, the user's call 2026-10-13).
- Abel's call waits until Main 6 is finished. While he walks the boards again Pa leaves his board, and he is back on it
  before “Sit with Pa” could replay.

## Loot and chests

- **Kills** use the rank tables (*Enemy ranks and legendary drops*, `Docs/Story.md`). Guns drop at the enemy's level. Ammo
  drops on every kill, leaning 2× to the kill gun's class, with more boxes from higher ranks.
- The gun pool is the Bullpup and the Ranchhand. No unique legendary on Ransom's Rest: the first, Dead Man's Hand, needs a
  pistol base and belongs to the *Lily*.
- **Mission rewards:** expansion §2. Fixed loot that stands:
  - Abel (Main 6): the Boss table, 3 guns at Luck 1.3 (30% chance of a legendary).
  - Heirloom (Main 7): Abel's Ranchhand, a named Epic with fixed parts and the line *Hold the door.* Its special effect waits
    for unique legendaries.
  - The Gravemother: the Legendary table on every kill, 2 guns at Luck 1.3 (21% chance of a legendary); her return: *Missions*,
    Side 3.
- Most legendaries come from Abel and the Gravemother. Common guns stay the most common at every rank.
- **Chests** (built, step 26; the Calder's Caches mission: expansion §3):
  - Three Supply Crates as Ruth's Ranger caches (under the windmill, on the Sink's rim, on the bluff path): 1 gun each at
    Luck 0.5 (3.7% legendary).
  - The gang's Strongbox in the sheriff's office: 2 guns at Luck 1.0 (15.5% legendary).
- The smashed Reliquary in the chapel is a story prop, dark and empty, from the backlog Reliquary design (step 18).

## The exit and the unlock

Finishing Main 7, The Lantern Leans:
- records the mission in the session's campaign record;
- fills in the Ledger's first page: Lucky Ned Purcell, the *Gilded Lily*;
- lists the *Gilded Lily* on the depot's station board. Tilly's hearse car waits at the platform from then on. Until that
  level exists, choosing the *Lily* says “The line to the *Lily* isn't open yet” and the player stays on Ransom's Rest.

**The station board** is a LooterUI panel at every area's station. It lists every opened area the Keeper's Lantern has found,
plus one blank line naming the mission that opens the next (for example “Finish ‘The Lantern Leans’”). Later areas aren't
shown; every opened area always is, so travel can't strand the player.

Ransom's Rest stays the hub; every later station's board lists it. Its creatures keep their place's levels (1–6, and Abel's 7)
however strong the player comes back, so it is easy and low-XP to revisit. What the endgame does with the bands is still open
(`Docs/Story.md`, “Endgame”).

**Skyreach** is no longer a practice island (2026-10-13, the user's call): it is area 3, reached from the *Lily* by a skylift
(expansion §1). No board lists it; `Looter.Travel Skyreach` still goes there for testing. The old practice-trip design: the archive.

## The hub afterwards

- Every trip starts and ends at **the depot** by Tilly's yard.
- **Save, respawn and fast travel:** Hob's roosts (expansion §8), in place of the respawn graves.
- **Delia's door** is a mission giver (expansion §2), and a plate is always set on the porch.
- **Ransom's Point** is where each ember is paid at the new moon.
- **Abel** sits on his board at Gravewind Point, with new lines after each ember.
- **Tilly's parlor** becomes the grave-goods vendor once currency exists.
- **The hub changes cheaply:**
  - A little color returns to the orchard with each ember paid (material swaps).
  - Ozias Penhallow arrives after the *Lily*, and Ruth's tent after Dustwater.
  - A new-moon night version serves the end of Act II.
  - Hanging Moon nights come in the endgame.
- **Revisits:** the Gravemother and the placed creatures come back; promotions reroll after 20 minutes of play. Abel isn't
  fought again; he's a friend now (the Vigil Stone's rematch aside).

## Art needs

- **Terrain, macro color maps, scatter mask** (new terrain, existing method, new grounded mode): `Art/Levels/RansomsRest/layout.json`
  through the terrain scripts for any area. Golden grass, ochre soil. The core: at most 150k triangles in about 20 tiles,
  full Nanite fallbacks, a 4096 px macro map (split 2×2 if it looks soft). The ring past the boundary has its own 2048 px
  macro map.
- **The ridges, the canyon and the horizon** (new, generated), past the boundary:
  - the ridges' outer slopes in the surround ring (6–8 sectors, at most 40k triangles, collision on the ridge sectors);
  - the canyon wall below the Rim's cliff kit (4 sectors, made like the tutorial island's underside wall, no collision);
  - the canyon floor with its thin river painted in, and the lower far wall about 300 m off;
  - unlit silhouettes of the plains, far mesas and far ranges at about 3, 6 and 12 km (3 layers × 4 sectors, at most 6k
    triangles), tinted by the light state. They replace the backdrop islands here.
- **Existing, reused as they are:** Farmhouse, Barn, Well, Outhouse, LogCabin, Cottage, Windmill, Bridge, Fences, LanternPost,
  FarmProps (hay bales), VillageProps, Containers, Rocks, Cliffs, Trees, Pines, DeadTree, Undergrowth, GroundCover, Waterfall,
  chimney smoke. Bridge, the footbridge, is Keeper's Bridge. (Homes and fronts now come from Ransom's Rest's own house kit:
  expansion §9.)
- **Amber orchard:** existing apple trees, a new material instance (leaf tint).
- **Blue larkspur accent:** existing GroundCover, a new variant, along fences.
- **Freestanding outcrop kit** (new): `Art/Models/Rocks/Outcrops.py`: four tors of 4–10 m and a 12 m rock spine, finished on
  all sides, 1.5–3k fallback triangles. For rocks seen from every side: the Three Widows, the Stone Teeth, Hearse Rock, Den
  Rock, the Keeper's Gate and the Rimrock.
- **Stone walls:** `Fences.py`'s dry-stone wall makes the pound wall, the glebe wall, the field wall and the sheep fold. Broken,
  fallen and corner pieces are new, in `Art/Models/Props/Ruins.py`.
- **Far trees and rocks** (new): opaque low-poly trees and rocks for the ridges past the boundary: instanced, no collision, no
  shadows, no masked leaves. Trees inside the boundary get a last LOD card for long views.
- **Ruins and dressing for the obstacles** (existing kits, new arrangements): the Burnt Homestead's standing chimney, the
  roofless old lantern house and springhouse, the burnt livery, the fallen quarry derrick, the hanging tree's rope, cairns and
  the Gap rockslide, from the house trim sheet, Rocks, DeadTree, Fences and VillageProps. The new pieces (the chimney, the
  roofless huts, the livery's footing and posts, the derrick, two burnt freight wagons, the cairns and the rope) are in
  `Art/Models/Props/Ruins.py`.
- **Lookout tower, intact, with cliff stairs:** `Art/Models/Buildings/Lookout.py`, a sound version of the ruined
  `LookoutTower.py`. For Ransom's Point: a railing deck facing the sunset, stairs down the south face to the Mooring Ledge, the
  gang's mooring post with a frayed rope.
- **False-front kit:** `Art/Models/Buildings/FalseFronts.py` and `Art/Models/Props/Boardwalk.py`: facades, boardwalk, awnings,
  hitch rails, shutters that can slam (separate, hinged) and sign boards lettered in Rye (`Art/Fonts`, OFL), on the house trim
  sheet. Makes Bright & Daughter, Pruitt's store, the Gilt Spur and the sheriff's office, with the notice board and the town
  memorial.
- **Chapel of Saint Ada:** `Art/Models/Buildings/Chapel.py`: white clapboard, louvered belfry, steeple (chosen 2026-10-05), the
  bell its own model so it can swing; a simple interior (pews, the altar, the Reliquary's niche); 5–8k fallback triangles.
- **Graves kit:** `Art/Models/Props/Graves.py`: old and fresh headboards, a frosted mound, crosses, iron fence and gate, the
  family plot's picket fence, coffins, a mourning wreath, black crepe. Instanced.
- **Smashed Reliquary:** the Reliquary from `Art/Backlog/Loot/Chests.py` with a cracked lid and no crystal.
- **Burial boards deck:** `Art/Models/Props/BurialDeck.py`: a timber deck on Gravewind Point's tip, west end cantilevered 3 m
  over the canyon, 8 biers on posts, 3 keeper's lantern posts.
- **Keeper's Lantern:** a held prop, in `BurialDeck.py` beside the lantern posts.
- **The Sink:** a pit feature plus the cliff kit. Web cards (masked, at most 20 per view), egg sacs, old coffins.
- **The Gravemother's den:** a cave mouth and overhang mesh set into the Sink's east wall, under Den Rock.
- **Skiff:** `Art/Models/Vehicles/Skiff.py`: the gang's skiff in dark paint for the cold open. (The packet-paint tutorial skiff
  and Skyreach's jetty went with the tutorial: the archive.)
- **The train** (one kit): `Art/Models/Vehicles/Train.py`: a small tank locomotive (about 9 m), a passenger car (about 12 m)
  and Tilly's hearse car (about 10 m; black and brass, with a coffin rack and black crepe).
  - Built from shared parts (wheel sets, bogies, couplers, roofs, doors, windows, lamps) sharing materials: the house trim
    sheet for timber, window glass and painted trim, and the existing metal and paint sets, tinted. No new texture set.
  - LOD0 at most 12k triangles for the locomotive and 8k per car; three LODs down to under 1k for the view from Ransom's Point.
  - Static at the platform; with steam up, its smoke is the existing chimney smoke.
- **Rail track kit, signal and station board:** `Art/Models/Props/Railway.py`: track straights and curves on one grid, a buffer
  stop, a semaphore signal and the station board prop, each with LODs. Track and signal use the existing metal and wood sets;
  the board uses the house trim sheet. The track is instanced.
- **Depot and water tower:** `Art/Models/Buildings/Depot.py`: the depot (a timber station house with a platform canopy), its low
  timber platform, a water tower with its spout. House trim sheet, merged per material with LODs like the other buildings.
- **Backdrop islands:** a few cheap, unlit distant islands in the sky around Skyreach only.
- **Wanted posters:** decals.
- **The Unpaid:** `Art/Models/Creatures/Unpaid.py`: spine, head, two arms and shroud-tail bones, posed by code like the spider.
  Three clothing tints; the coal's color comes from the rank (dull red for Basic).
- **Abel,** on the Unpaid rig: keeper's coat and hat, ghost lantern, spectral Ranchhand pump.
- **Amos,** on the Unpaid rig: work clothes.
- **Hob:** a small rig: body, head, wings and legs, flapped by code.
- **Mister Sexton:** a seated static mesh, the first human.
- **Cold open silhouettes:** the UE mannequin, posed, flat black against the sun.
- **Gravemother and spiderlings:** the spider mesh and rig at 1.8× with a pale-hide material instance; 0.45×.
- **Cloud bank:** a card for the cold open's cloud and the evening cloud in the west that the gang's skiff comes out of.
- **Effects:** ghost fade and phase-step, the coal glow, spectral buckshot pellets, the shriek ring, Gravewind wisps, unlit wisp
  cards along the Rim, fog rising out of the canyon at the deck, a grave-dirt burst, frost on Abel's grave.
- **Lighting:** new data on the existing setup: golden late afternoon and dusk, sun low in the west-southwest over the canyon.

## Tech needs

- **Terrain scripts for any area** (generalized from the tutorial island's, about 3,400 lines): one shared generator that takes
  a layout path, with an island setting and a grounded one: feature lists by type, pits, mesas, gullies, cliff groups per
  feature, stacked cliff courses above 12 m, the playable boundary, the surround ring, the canyon wall and the backdrop.
  `build_area.py <Area>` replaces `build_tutorial_island.py`; the scatter builder takes the area. The tutorial island
  regenerates identically. See *How the terrain generator changes*.
- **Playable area:** `APlayableArea` (World/) holds the boundary polygon from `layout.json`.
  - Edges flagged open where a drop is part of play: the Rim's lip (running on along the Mooring Ledge's south edge over the
    Mooring Notch to about (−84, −101)), the deck, Mill Falls.
  - Past the closed edges the ridges rise steeper than the walkable angle (about 45°) for at least 4 m, dressed with cliffs
    and outcrops; invisible walls (box components about 50 m tall) stand behind them as a backstop. At Stage Gap's mouth the
    closed edge crosses the track behind the parked locomotive and the slide's banks.
  - The `PlayableBounds` collision profile in `DefaultEngine.ini` blocks only Pawn, so bullets, the camera, the minimap and
    PCG traces pass through.
  - Answers `Contains()`, draws itself with `Looter.World.Bounds`; `LooterWorld::StaticGeometryParams` skips it. A level
    without one (the tutorial island) is unchanged.
- **Fall recovery on the ground** (`UFallRecoverySubsystem`): drops inside the boundary are at most about 20 m and nothing past
  the Rim has collision, so the old rule (more than 30 m below the last safe spot) would fire only deep in the canyon. With an
  `APlayableArea` in the level:
  - safe spots are sampled only inside the polygon;
  - a player who left over an open edge and dropped 5 m below their last safe spot (off the Rim, the Mooring Ledge, the deck or
    Mill Falls) is recovered at once;
  - the 30 m rule stays as a backstop;
  - an `OnRecovered` event feeds Hob's lines about falls and the boss's Gravewind phase.
  - `KillZ` is about 100 m below the canyon floor, so stray creatures and drops get cleaned up.
- **Minimap bounds** (`UMinimapSubsystem`): keeps its traced bake and tags. Only the core terrain tiles are tagged `Ground`
  (the valley and the ridge feet; not the ring, the canyon or the backdrop). Nothing past the Rim has collision, so it traces as
  void, and the coast lines draw the Rim with no new code. With an `APlayableArea`: texels outside the polygon are dimmed, the
  closed edges are drawn as a boundary line, and the height tint uses only heights inside the polygon (the ridges would squash
  it). The resolution is a member sized for about 1 m per texel: 256 on the tutorial island, about 384 on Ransom's Rest.
- **Perf by tag:** `Looter.Perf.HideTag <tag> 0|1`. The build script tags everything past the boundary `Beyond` and each zone
  `Zone_<id>`; hiding a tag lets tour and perf runs measure that group's draws and milliseconds by difference.
- **Creature size:** `BodyScale` on `ACreatureBase`, applied after `BeginPlay` resets every creature to scale 1. Scales spider
  leg IK, the capsule, attack range, steering and ledge probes and the health bar height (all fixed centimetres before). Tested
  by walking and attacking with a 0.45× and a 1.8× spider.
- **Ranks** (on `ACreatureBase`, `ULootTable`, `ULootDropComponent`, `UCreatureHealthBarWidget`): `ECreatureRank`, a rank
  settings asset, four rank tables (Basic stays `DA_LootTable_Default`, which `LootLibrary` and the DefaultTable test name),
  tag colors and words, coal colors. The creature's level is pushed into `ULootDropComponent::Level` whenever it is set.
  Commands `Looter.SpawnCreature <kind> <rank> [count] [chase]` and `Looter.Loot.SimulateDrops`; test `Looter.Loot.RankOdds`.
- **Level bands and XP:** `UAreaDefinition` (name, map, band, promotion chances): `DA_Area_RansomsRest`. Skyreach's is marked
  practice (0 XP; ammo-only drops once the first cast-off is recorded), though nothing offers it since 2026-10-13. The level
  roll is by where a creature stands (the layout's `gameplay.levels`, since 2026-10-12), with linear health and damage. Kill XP
  and its falloff for low-level enemies: `Docs/Progression.md`; `Looter.XP.Table` prints it.
- **Respawn rules** (`ACreatureBase`; by default a creature respawns about 34 s after death): off for Legendary monsters,
  bosses and everything spawners and egg sacs create. A promoted creature comes back as Basic. Promotions are rolled on arrival
  (coming in at the station by train, or loading a session), with the 20-minute cooldown saved per map.
- **Pack calls** (`ACreatureBase`): matched by a pack tag, not the exact class, so the Gravemother's and Epic spiders' calls
  reach every spider.
- **Session save version 2** (`ULooterSessionSave`):
  - world state per map, so each map keeps its own world;
  - the campaign record: completed missions, active mission and step, opened areas, first boss defeats, the first cast-off
    from Skyreach, cold open seen;
  - the arrival point (a station's platform, or Skyreach's jetty), and per-map promotion and Legendary-monster times.
  - Version 1 saves are upgraded inside the load, before anything can save them again, their world filed under the tutorial
    map: the old sessions carry over, tutorial done or not, without losing guns. Tested on a copy of a real version 1 save
    before merging.
- **Travel** (uses the save's `Map` field): `UAreaDefinition` gets its station and opening mission. Travel sets `Map` to the
  destination, clears the saved player spot, writes the arrival tag and saves, then opens the level with `?Session=N`. The save
  on level teardown is skipped for that trip (it would write the old level's world over the new one). Autosave and save-soon
  pause during the train's shots, the fades and scenes. Arrival points are tagged by station: on Ransom's Rest, the platform by
  the hearse car's door. The session picker shows area names (Skyreach, Ransom's Rest), not the map file's name.
  `Looter.Travel <area>` for testing.
- **Leaving Skyreach:** superseded with the tutorial (2026-10-13, expansion §1); the old design is in the archive.
- **Station board:** `UStationBoardWidget` (UI/World/), a LooterUI panel that calls `MarkBackground`. Its lines: *The exit and
  the unlock*. Choosing a line asks to confirm, then travels.
- **Stations and the train:** `ATrainStation` (World/), one per area, placed by `gameplay()` from `layout.json`; on Ransom's
  Rest, the depot.
  - Hold Interact at the station board to open it. The arrival point is on the platform by the hearse car's door.
  - The train is static meshes with LODs parked at the platform. It moves only in two short shots (the default; a fade alone is
    the other choice, step 9). Departure: a fixed camera on the platform watches it pull out east through Stage Gap along a
    spline for about 4 s, then the screen fades. Arrival: the screen fades in on it pulling into the platform, and control
    returns when it stops. Both skippable. Its wheels turn by code, nothing in Sequencer. Cold until Main 7: *Zones*.
- **Track and train collision** (the models' `UCX_` hulls and `ATrainStation`):
  - No per-rail collision: each track piece's hull is one low box over its ties and rails with sloped sides, so a player steps
    across without snagging.
  - The platform is about 40 cm high, under the character's step height, so it can be stepped onto anywhere.
  - Each car's hull is one simple box, and the gaps between cars are closed: nobody climbs onto the train, squeezes between
    cars or gets past the locomotive at the gap's mouth.
  - The train, the depot and the water tower are tagged `Obstacle` for the minimap; the track is not. The track past the gap
    belongs to the surround and has no collision.
- **Interaction:** one component owns the Interact key, tap and hold, generalizing the weapon manager's loot focus: loot, the
  station board, doors and windows, headboards, the bell, lantern posts, hay bales and posters. Loot keeps its tap and hold
  rules and tests.
- **Missions as data** (on `UMissionSubsystem`, committed at 45260c4): `UMissionDefinition` data assets list objectives; their
  types are C++: reach, interact or hold, talk at a speaker point, kill N by class, tag or zone, kill a named actor, collect,
  defend for a time, play a scene. Rewards: an XP share, a gun with a rarity floor, a named gun, unlocks. A runner per level
  feeds titles, objectives and waypoints to `UMissionSubsystem` (the minimap arrow). A Missions page in the inventory
  (LooterUI). `Looter.Mission.Start/Complete <id>`. Givers, offers and the newer objective kinds: expansion §3 and §12.
- **Captions and speaker points:** captions as floating outlined text with no panel, like the HUD. Speaker points on doors and
  windows. Lines as data (speaker, text, seconds). Hob's lines on events, never on Skyreach.
- **Story characters:** `AStoryCharacter`: non-hostile, posed by code, carries a speaker point, shown or hidden by story state.
  Sexton, Hob, Amos and Abel after the fight. Abel the boss is a separate class (`ACreatureBase` is hostile only).
- **The Ledger** (the bestiary): a story-character page type with no actor (the Entries test updated for it), pages that show
  the area's band instead of class-default level and XP, a page field that moves Abel's page from Enemies to Friends, and the
  seven-names page. The UI stays Concept C.
- **Spawners:** `AEncounterSpawner`: groups and waves switched by mission state, alive caps, rank rolls, boss adds, safe zones
  (the farm, and the town after Main 3). Nothing they spawn respawns. Set pieces (`ASetPieceSpawner`, 2026-10-21): *Fights
  and set pieces*.
- **The Unpaid:** `AUnpaidCreature`, a child of `ACreatureBase`: floating, coal crit by shot-line distance, lunge, shriek,
  phase-step.
- **Lighting states:** golden afternoon and dusk, on a sky made for the ground. `Looter.Light Day|Dusk`.
  - The painted cloud dome grows from 1 km to past the last backdrop layer (it is translucent and depth-tested, so at 1 km it
    would paint clouds over the far ranges).
  - SkyAtmosphere's planet top moves down to the plains, with a ground albedo in their colors.
  - The height fog (once tuned to fill the void under the island: blue, max opacity 0.85) is warm for golden hour, with
    directional inscattering toward the sun and a start distance near 60 m, kept thin so the backdrop's layers fade by distance
    under the atmosphere's stretched haze rather than melting into one band.
  - The backdrop's tint and the inscattering come from a material parameter collection.
  - Dusk is the same sun rotated, with fog color changed and the sky light recaptured behind a fade.
  - The atmosphere's sky luminance factor cancels most of the warm sun's tint, so the afternoon sky stays blue overhead and
    warms only toward the sun. Dusk raises the ozone (3.5 times Earth's) so the sky away from the sun turns blue-violet, and
    the height fog ends before the sky and the cloud dome (cutoff 13.5 km).
  - The painted clouds are lit from the sun's side: warm where they face it, blue-grey in shade. Volumetric clouds and fog
    stay off.
- **Scenes** (C++): the cold open (a code camera path, silhouettes, captions), the grave wake-up camera, Sexton's scene, Abel's
  ending, the train's departure and arrival shots. All skippable (`Looter.Scene.Skip`), no logic in Sequencer. Scenes stay off
  in tour and perf runs.
- **Boss framework:** phases by health, untargetable states, add waves, the fog-wall seal, a boss bar (a slim slanted bar per the
  HUD rules), reset on the player's death. Built on a test spider first.
- **Enemy projectiles:** the Pawn profile ignores the Weapon channel, so a bullet fired the way guns fire would pass through the
  player. Enemy shots get their own channel (or a sphere sweep against the player's capsule) and a slow pellet visual (an
  emissive sphere, instanced). Tested: a pellet hurts the player and never the shooter's adds.
- **Respawn points:** `ARespawnMarker` (with `UPlayerVitalsSubsystem`): wake at the nearest activated one. They are Hob's roosts
  now (expansion §8).
- **Named weapons** (on `FWeaponInstanceData`): fixed parts, a name and a flavor line. Heirloom now, unique legendaries later.
- **Egg sacs:** modeled on `ATargetDummy`; shootable; each lets out 2 spiders.
- **Tour and perf:** `Looter.Tour` and `views.json` take an optional `exec` per view, run before the view is measured, for
  example `Looter.Light Dusk; Looter.SpawnCreature Spider Basic 12 chase`. Horizon and boundary views.
- **Tests:** missions and objectives, the save upgrade, travel, the station board's lines (opened areas, the blank line),
  arrival on the platform, rank odds and rarity order, loot level, creature size, Unpaid hit zones, enemy pellets, boss phases
  and reset, story-character pages, the generator's two fixtures. Also:
  - `Looter.World.PlayableArea`: `Contains()`; a capsule sweep is stopped by a closed edge and not by an open one.
  - `Looter.World.FallRecovery`: a player off an open edge is back on the last safe spot within a second; no safe spot is ever
    recorded outside.
  - `Looter.World.Minimap.Bounds`: outside texels dimmed, the boundary drawn, the canyon traced as void.
  - `Looter.World.Station`: a capsule walked across the track never snags, and none gets past the locomotive or between the cars.
  - `Looter.World.RansomsRest.InsideBoundary`: every respawn point, player start, encounter spot and tour view lies inside the
    boundary.

### How the terrain generator changes

- **A grounded setting.** `layout.json` has `"setting": "island"` or `"grounded"`.
  - Grounded skips everything that assumed a floating rim: `_rim()`'s roll-over, `underside()` and its spires, `rimDrop` and
    `undersideDepth`, the waterfall's `dropTo` measured to the underside, `rim_points()` cliff dressing, the rim band in the
    macro map (`_rock`) and in the scatter mask (rim pebbles, a land mask cut from the outline), and `terrain_ao()`'s padding
    that treats everything past the rim as open sky (it pads with the real surround heights instead).
  - Everything keyed to `edge` (distance inside the rim) uses distance inside the playable boundary: `_relax`, quadtree
    clearance, orchard rows and waterline filtering.
  - The island setting keeps its own code path untouched, so the tutorial island regenerates identically.
- **One height field, three levels of detail.** A regional height field (large-scale noise, ridges as polylines with profiles,
  the Rim's escarpment drop and the canyon floor) is defined everywhere and sampled at three resolutions:
  1. the playable core, a square of about 400 m at 12.5–15 cm cells (512 m since the expansion: §6, Geography), with the feature
     pipeline on top;
  2. a surround ring out to about 500–600 m from the centre, on a 1 m raster with 2–10 m vertex spacing and a full fallback;
  3. backdrop silhouettes at about 3, 6 and 12 km.

  The core's outer 25 m is a seam band that fades every core feature and the micro-noise into the regional field, so the
  core's edge heights match the ring's. The ring's inner loop reuses the core's edge vertices exactly (no T-junctions), and both
  meshes take the seam normals from the field's gradient: no crack, no light crease. A feature that enters the seam band stops
  the generator with an error. The core's edge is the square on the ridge sides and the Rim's lip on the west.
- **The west edge.** Ransom's Point and Gravewind Point stand on the Rim; the gang moored on the Mooring Ledge under the Point;
  Mill Creek falls into Mill Gorge; fog rises out of the canyon at the deck. Below the Rim's cliff kit the canyon wall is
  generated as an open strip in 4 sectors reusing `underside()`'s rim-wall code (`_zipper`, level-strata UVs, baked AO). The
  CliffFace kit goes only on the top course (12–20 m, stacked) and where players stand close (the lookout, the deck). The ring
  holds the canyon floor, with the river painted into its macro map, and the lower far wall. Past that, the plains and far
  mesas are silhouettes.
- **The map square is set per area.** `MAP_HALF`, the 2048 shape raster and every raster size once tuned to 204.8 m (curvature
  1024, AO 1024, scatter 512, macro 4096) are cell sizes in metres per layout, with caps. The tutorial computes the same sizes,
  and a 400 m core keeps its detail. The core's macro map is 4096 px, about 9.8 cm/px against the tutorial's 5 cm; if it looks
  soft it splits into 2×2 maps, one per tile quadrant, with no extra draws (the tiles are separate meshes already). The ring's
  2048 px macro map (about 0.6 m/px) is painted from the same palette functions, and the core's blends into it across the seam
  band. `layout_computed.json` records the squares each map covers; the build and scatter scripts read them, not the old
  hard-coded `HALF = 10240`. The core's 150k triangles are weighted toward the playable area, with a coarser tolerance past
  the boundary.
- **Feature types:** a valley with raised terrain outside the boundary (no rim drop or underside); ridges with cliff bands cut
  by a saddle (Stage Gap, its floor graded for the track) and a gorge (Mill Creek); an escarpment edge down to a backdrop
  canyon on the west; the playable boundary as a polygon with its open edges marked; a scarp (a one-sided step of 2–6 m along a
  polyline); a ridge or spine (a raised line with cliffs on one or both sides); an outcrop knob (a small footprint that seats a
  freestanding rock); a pit (the Sink: a plateau turned upside down, with an inward-facing cliff ring and a ramp); a mesa
  (Coffin Rock: a plateau with a cliff ring and no ramp); hills (the chapel knoll, boot hill); any number of plateaus with ramps
  (Ransom's Point with its ramp, the farm terrace); dry gullies as creeks without water (the Dry Wash); a creek that ends in a
  falls into a gorge; a rail bed (a strip graded flat along a polyline, for the track through Stage Gap); and stacked cliff
  courses above 12 m, since the tallest cliff piece is 12 m (the 20 m bluff, the Rim's lip, the ridges' bands). Each feature
  writes its own cliff group to `layout_computed.json`. Outcrops, boulder fields, tree lines, walls and ruins are placements
  and scatter areas; they don't change the ground level, so they are the only obstacles used inside an encounter's ground.
- **An open-ground metric.** For every walkable metre inside the boundary, the generator measures the distance to the nearest
  break: a building or wall line, a fence, an outcrop or boulder group, a tree stand, a cliff, or a 1.5 m rise within 6 m.
  Fences, low walls and knee-high cairns count as breaks. It writes the largest distance and the worst spots to
  `layout_computed.json` and paints a red overlay on the plan preview. Target: nothing more than about 10 m from a break (empty
  circles of about 20 m across at most), outside ground marked open on purpose: the hayfields, Main Street, the crossroads at
  the keeper's grave, and the grave's line to the bell tower.
- **Scatter.** The ray lift and length come from the computed height range (the ridge feet stand higher than anything on the
  tutorial island). Grass, flowers and pebbles stop 10 m past the boundary and stay off the rail bed. The ring gets one cheap
  layer of opaque low-poly far trees and rocks from its own mask: no collision, no shadows, no masked leaves.
- **The build script.** Outcrops, graves, fences and walls are instanced components, one per mesh, tagged `Obstacle` for the
  minimap and the scatter; cliff pieces stay actors (see the 2026-10-05 lesson under *Performance plan*). Track pieces are
  instanced the same way, untagged, since the player walks over them. Static building clusters are merged per material, with
  LODs. The script places the playable area, `KillZ`, and one `CullDistanceVolume` with a size-to-distance table read from
  `layout.json`. Everything past the boundary gets a `Beyond` tag and each zone a `Zone_<id>` tag.
- **Scripts and names.** The generator lives in a shared module that takes a layout path (its old `Island` class is `Area`),
  moved out of `Art/Levels/TutorialIsland`. `Art/Models/Terrain/TutorialIsland.py` and `RansomsRest.py` are thin wrappers
  around it. `build_tutorial_island.py` became `build_area.py <Area>`, with the level, mesh prefixes, spider zones and gameplay
  read from the layout. Meshes: `SM_<Area>_Tile_i_j` (tagged `Ground`), `_Ring_n`, `_CanyonWall_n`, `_Backdrop_n` and
  `_Water`. Two fixtures guard the generator: the tutorial island (island setting) and a small `TerrainTest` layout
  (grounded); their computed outputs are checked in and compared after every generator change. `views.json` and
  `island_views.py` take the area; `CODEMAP.md` lists every file.
- **Identity check:** `Tools\terrain_identity.ps1` rebuilds Ransom's Rest, Lvl_TerrainTest and the tutorial island byte for
  byte as committed. The first check's bar (step 3a): `layout_computed.json` within 0.1 cm; tile, underside and water meshes
  with identical vertex and triangle counts and hashes; macro and scatter PNGs pixel-identical; `Tools\tour.ps1 -Quality
  Medium` within 3% at every view; `Tools\runtests.ps1` green.

## Performance plan

**Target.** 8.3 ms (120 fps) at 1080p on Medium on the RX 580, at every tour viewpoint: GPU at most 7.5 ms, game and render
threads at most 7 ms each, about 700 draws. Lumen and Nanite on High and Epic only; every mesh draws its Nanite fallback on
Medium.

**Baseline** (the tutorial island, same kit): 151–201 fps on Medium. Its lookout sees the whole island for 4.2 ms GPU, 422
draws, 0.74M triangles. Its worst view, the forest grove (5.5 ms GPU), costs from masked leaves, not distance. Its game thread
was 4–5 ms with 13 mostly idle creatures, 3.0 ms once distant creatures updated less often (2026-10-01). Ransom's Rest is about
1.5× the area plus a town, a chapel, about 50 graves, many more cliff pieces and the obstacles, and Ransom's Point sees all of
it plus everything past the boundary.

**Draw budget per zone, as seen from Ransom's Point** (700 in all): terrain tiles, cliffs and outcrops 110; past the boundary
30; farm and orchard 100; Main Street, the yard and the depot 160; chapel, churchyard and boot hill 90; the Sink and the fields
80; Gravewind Point and the deck 40; creatures, characters, effects and HUD 90. Everything past the boundary: at most 30 draws
and 0.6 ms GPU at the heaviest view, inside the same totals.

**Rules.**
- Terrain: the core at most 150k triangles in about 20 tiles, full Nanite fallbacks on walkable meshes.
- Past the boundary, a handful of merged meshes in sectors: the ring in 6–8 sectors (at most 40k triangles), the canyon wall
  in 4, the backdrop in 3 layers (one mesh per layer: see the lessons; unlit, untextured, at most 6k triangles). Any one view
  draws 3–5 ring and canyon pieces. The ridges to the north, east and south hide the land behind them, so the ring there is
  only their outer slopes. On the west the canyon floor and far wall add about 4–6 draws plus their pixels to the sun-side
  view. Only the ring casts shadows, and only inside the 100 m shadow range.
- No masked foliage past the boundary: ridge trees are opaque low-poly far trees (instanced, no collision, no shadow). Grass and
  flowers stop 10 m past the boundary; the macro maps paint their color beyond. Inside the boundary trees get a last LOD card:
  the orchard and town trees seen from Ransom's Point are the real overdraw risk.
- Buildings share the house trim sheet; static building clusters are merged per material with LODs (World Partition HLODs show
  only for unloaded cells, so they do nothing in one unstreamed level). Headboards, crosses, fences, walls, web cards and
  outcrops are instanced, so the obstacles add instances, not draws.
- Cull distances follow object size through one `CullDistanceVolume` (for example: up to 0.5 m culled at 40 m, up to 2 m at
  90 m, up to 6 m at 200 m, larger never), measured with Medium's own view-distance scaling.
- Shadows: Medium keeps two 1536 cascades over 100 m (approved 2026-10-07). From Ransom's Point the town is 120–180 m away and
  gets no dynamic shadows. The tall ridges stay on the side away from the sun: a 50 m ridge under a 15° sun throws a shadow
  about 190 m long.
- The train is static except in its two shots; the depot is built like the town. The locomotive and both cars share their
  materials and have three LODs each; the track is instanced. At the depot view the train, depot, platform, water tower, signal
  and track take at most 15 draws; from Ransom's Point, about 240 m away, at most 8, inside Main Street's 160.
- Ghosts use a masked, dithered material with an emissive rim and coal, never translucency.
- Live creatures: at most 16 Unpaid at once, at most 10 adds plus Abel in the boss fight, at most 20 creatures of any kind
  within 80 m (12 and 16 until the bigger fights of 2026-10-21). What Ransom's Point can see is covered by the existing rules: creatures beyond 60 m tick less often, spiders stop
  posing bones off screen, health tags show only nearby. View distance doesn't change the game thread.
- No volumetric fog or clouds: height fog plus a few unlit wisp cards along the Rim. The fog stays thin (0.003) and the
  atmosphere's aerial perspective is stretched 3 times, so each backdrop layer fades more than the one before it (darker near,
  paler far) and the last is almost all haze; the cloud dome grows past the backdrop.
- Lanterns are emissive meshes, with at most 3 shadowless point lights in any view.
- Dusk is the same sun rotated, with the sky light recaptured behind a fade. It has its own viewpoints, a capped shadow
  distance and 2 cascades.
- The level loads behind a fade; its load time is measured.
- A tour after every import; anything over budget is fixed before the next step. The render thread's own cost comes from
  `perf.ps1 -GpuStats` captures (the tour's Render column follows the frame); at 700 draws, merging and instancing are the
  main levers. Fights are measured with real, chasing creatures (`Looter.Perf.Horde`, below), not pictures.

**Lessons measured** (Medium, 1080p):
- 2026-10-05, test level `/Game/Maps/Dev/Lvl_TerrainTest`, by the tour's cost-by-difference views:
  - Beyond in four sectors per layer cost 0.40 ms GPU and 33 draws looking west over the canyon, 43 from the air: over 30
    draws. Fix: one backdrop mesh per layer.
  - The 116 cliff pieces cost 6–22 draws and up to 0.2 ms GPU: Unreal's dynamic instancing already merges identical pieces
    into one draw. Instancing by hand would save little and cost the scatter its per-obstacle boxes (it keeps grass and trees
    out of each actor tagged `Obstacle` by its bounds). So cliff pieces stay actors unless Ransom's Rest's own tour says otherwise.
- 2026-10-06, `Lvl_RansomsRest` (step 5a's level before the greybox, after the boundary rework):
  - The render thread is the limit, not the GPU: render 4.1–5.9 ms against GPU 3.3–4.9 ms; a `-GpuStats` capture puts
    1.7–3.3 ms of it in its visibility wait.
  - Beyond, one backdrop mesh per layer: 13–20 draws, at most 0.22 ms (13 west from Ransom's Point, 16 from the deck, 17 at
    the ridge foot, 20 from the air). Four sectors per layer had cost 23–38.
  - One far cascade to 400 m drawn by buildings and cliffs (`Looter.Perf.FarShadow 1 400`) costs 0.95 ms at Ransom's Point over
    the valley and 0.3 ms over town (GPU +0.7 and +0.25, 45–58 more draws), and the town beyond 100 m hardly changes
    (`Saved/Screenshots/Review/RR_FarShadow.jpg`). Kept: two 100 m cascades.
  - Dusk costs about what the day does, −0.2 to +0.5 ms by view (its shadows reach 60 m): the grave 5.6 ms against 5.2,
    Ransom's Point west 4.8 against 4.3, Main Street 5.1 against 5.3.
  - The boundary: steep rock (50.8–65.7 degrees, starting 0.0–2.1 m out) rises past every closed edge but Stage Gap's mouth,
    and the walls stand 2.5 m behind the line at its foot (`level.wallSetback`). Walking into all 25 closed edges (55 runs),
    every run stops at the wall, at most 2.16 m past the line; none gets past or falls. Only Stage Gap's mouth (the train's
    place) has walkable ground past the line.
  - In play: stepping off the Gravewind deck is recovered (5 m down, edge 19), and the minimap covers the valley.
- Twelve Unpaid chasing at the town gate (step 16), the player looking down Main Street: 5.9 ms (169 fps), p95 6.3, game thread
  4.3, GPU 4.9, 672 draws. With one Unpaid: 5.5 ms, game 2.8, GPU 4.5, 465 draws. So each Unpaid costs about 0.14 ms of game
  thread, 0.04 ms of GPU and 19 draws.
  - How: `perf.ps1 -Exec "Looter.Quality Medium,Looter.Perf.Horde Unpaid 12 Basic 0 -1400 90"` puts the player at the gate,
    unhurtable, with the fight coming at them. A tour view can't measure a fight: the tour looks through its own camera while
    the player stays at the spawn, and creatures far from the player slow down.

**Latest** (2026-10-08, editor closed, after the last art round; `Docs/Performance.md`): all 28 tour views at 149–216 fps; the
heaviest are ChapelOverTown with the far shadow cascade on (6.7 ms, a comparison view), the Farm and the aerial at dusk
(6.6 ms). The spawn: 6.3 ms, p95 6.9 ms. A per-pass GPU capture shows no single art asset as a real cost: the heaviest views
spend 1.1–1.2 ms in the base pass, 0.5–0.7 ms in the depth prepass, at most 0.5 ms on shadows, 0.24 ms on translucency at the
deck. The expansion's views: expansion §6, "Performance".

**Tour viewpoints** (`Art/Levels/RansomsRest/views.json`, with `exec` where a view needs setup), as planned:
1. The grave.
2. The farm porch toward town.
3. Ransom's Point over the valley (expected heaviest).
4. Ransom's Point west over Gravewind Canyon, into the low sun.
5. Ransom's Point at dusk (`Looter.Light Dusk`).
6. The town gate down Main Street with 12 Unpaid chasing (a fight: measured with `Looter.Perf.Horde`, above).
7. The depot from the undertaker's yard, the train at the platform and Stage Gap behind it.
8. The chapel yard.
9. Inside the chapel.
10. The Sink floor.
11. The Gravemother's den.
12. Whitlock Fields.
13. Gravewind Point at dusk, with Abel and 10 adds.
14. The north boundary, looking out over Larkspur Ridge.
15. The ridge foot, where the core meets the ring.

## Scope cuts, in order

The first pass's defaults; later items are cut only if needed.
1. **Talking doors.** Delia, Tilly and Aldana speak through doors and windows; only Sexton and Hob get models in this area. Full
   townsfolk models come after Sexton's concept sets the human look, and only if the user wants them.
2. **Captions only,** no voice acting.
3. **A cheap cold open:** a code camera, captions, sound, a muzzle flash, flat-black posed mannequins, the gang's skiff in dark
   paint, and Sexton's seated model on the far rail. No costumes, no Sequencer logic.
4. **Then, if needed:** Hob's perching (a plain bird on fixed perches, or none), the Gravebound Unpaid's slow, the Ranger caches,
   the dusk fade (the boss at golden hour), the train's moving shots (a fade alone) and the Gravemother side mission.

Concept renders come before any human costume. Sexton first: he sets the bar for every human after him.

## Risks

Each risk, then what answers it. Risks answered by a rule stated elsewhere are not repeated: the game thread and ghosts on
Medium, long views, far shadows, the low sun and the render thread (*Performance plan*); steering without a navmesh and
obstacles splitting encounters (*Encounters stay on one level of ground*); the seam and breaking the tutorial island (*How the
terrain generator changes*); invisible walls, the track as a way out and snagging on rails (*The boundary*, *Playable area*,
*Track and train collision*); the save upgrade (*Session save version 2*).
- **The first human:** Sexton sets the look of every human after him (the gang, the townsfolk, the *Lily*'s card sharps), and
  the user rejected faceted low-poly. Concepts first, a seated pose, the face in shadow, no facial animation.
- **A father as the first boss** is heavy for hour one: Hob's humor and the grief mechanic carry it.
- **The Rim may read as an island's edge:** the canyon's floor and far wall, the plains running on to the sunset and the
  ridges on the other three sides show solid ground all round.
- **Fall recovery could fire by mistake** if the boundary is drawn too tight along the Rim: safe spots are recorded only
  inside, the outside rule needs a 5 m drop, and the boundary sits a step past the walkable edge.
- **Generator cost:** the 400 m core has about 2.4× the tutorial island's raster cells (15 cm cells are the fallback). The
  single 4096 px macro map has about half the tutorial island's texel density; if soft, four 4096 maps (2×2).
- **The backdrop at dusk:** unlit silhouettes don't react to the sun, so they take their color from the light state (else
  bright ridges at sunset).
- **Legendary feel:** at 0.3% per basic kill, most legendaries come from ranked enemies and bosses, so the 8% and 2%
  promotions must show up often enough.
- **Talking doors** read as cheap beyond Ransom's Rest: the *Lily* needs human NPCs and human enemies anyway, so plan that
  pipeline.
- **Thin gun variety:** two gun bases are thin for an hour of loot. Plan a revolver base before the *Lily*; Dead Man's Hand
  needs a pistol anyway.
- **Story wording on a grounded world:** `Docs/Story.md` names Teropa; never call a story place an island but Skyreach.

## Decisions for you, by step

The user's decisions, by build step. Superseded ones are in the archive, marked.
- **Step 1 (2026-10-01):**
  - The place is **Ransom's Rest**, and so is its town; locals call it “the Rest”. The town's street is Main Street. Level
    `/Game/Maps/Lvl_RansomsRest`, layout folder `Art/Levels/RansomsRest`, area asset `DA_Area_RansomsRest`.
  - Sexton's deal comes early, about 15 minutes in, on the lookout where Ellis died.
  - The first boss is Abel Ransom, Ellis's father. The Gravemother stays the Legendary-monster side mission.
  - Superseded 2026-10-13 (expansion §1): the tutorial exit by cloud match cut, Skyreach as a practice island, “Skip the
    tutorial”, and the tutorial's guns as the guns Ellis was buried with. Now a new game opens on the cold open, and Ellis is
    buried with a Common Bullpup.
- **Step 2 (2026-10-01, with the user's changes):** Ransom's Rest is grounded on Teropa, the earth-like planet where the story
  takes place, not an island like the tutorial island. The layout stands as drawn: the size (about 1.5× the tutorial island),
  the zones, the route and the bluff path. Cliffs and obstacles added to break up the open ground.
- **Step 2b and the story proposals (2026-10-01):**
  - Approved: the valley ending at the Rim (the revised map: the ground; the obstacle pass, leaving no gap wider than about
    19 m outside the zones except the crossroads at the keeper's grave; the boundary and what lies past each side). This
    document is written to it.
  - **Travel between areas is by train,** not by skiff: *“I think fast travel between regions should be via train.”* Every
    area has a station with a station board. On Ransom's Rest the exit in Stage Gap became the depot, its platform, a water
    tower and the track through the gap, in the same places, and the rockslide over the old stage road stays (*Size and
    shape*). The gang's skiff stays for the cold open.
  - Approved: the dead go west down the Sundown Road to the Toll Gate.
  - Approved: bodies go back to their graves at dawn, after the night on the burial board.
  - Approved: keep the Reaches and the Rim Rangers.
  - Deferred until Ransom's Rest was complete: whether the *Gilded Lily* stays an airship. Ransom's Rest is complete
    (2026-10-08); the *Lily* is on hold while the user polishes other things; the question is still open. Map order
    (2026-10-13): the *Lily* is area 2, reached by Tilly's train (expansion §1).
  - Build Ransom's Rest with several agents, to a professional standard: buildings, obstacles, the environment and every asset
    working properly; interesting and interactive; fully optimized, with small files and good graphics; fleshed out.
- **Still open, a small choice:** the lookout tower stays at (−100, −86), where the bluff's edge hides it from the grave, unless
  the user wants it moved to the top's north-west corner, at about (−106, −66), where the grave sees it.
- **Step 5a (approved 2026-10-07, as recommended):** today's two 100 m cascades stay; the far cascade cost 0.2–0.4 ms GPU at
  the long views for nothing visible. Approved with it: the horizon and the boundary in the game (with art notes 2 and 3: the
  blue sky, the blue-violet dusk and the layered far band) and the timings (Medium, the heaviest view 6.5 ms of the 8.3 ms
  budget).
- **Step 6:** the legendary odds per rank: 0.3%, 2.2%, 11.8%, 21.4% and 30.3%. Ranked enemies drop guns more often than the
  user's 20–40% rule (Rare 60%; Epic and up on every kill); Basic keeps 30%. Rank words and colors: Restless blue, Gravebound
  purple, Soulfed orange; ranked creatures a little bigger.
- **Step 7:** promotions of 8% and 2% on Ransom's Rest, with Legendaries hand-placed; +8% max health per level as the first
  level-up reward. Enemy levels are fixed by place (2026-10-12) and kill XP follows `Docs/Progression.md`; the step's other
  options are superseded (the archive).
- **Step 8:** the 20-minute cooldown on rerolled promotions and the Legendary monster.
- **Step 9 (2026-10-02):**
  - The train (the locomotive, the passenger car, Tilly's hearse car), the depot and the water tower: approved from the look
    sheet, with **locomotive B** (the balloon stack, bell and cowcatcher). Approved the same day: the ruins, the lookout with
    its cliff stairs, the burial deck and the Keeper's Lantern.
  - The skiff: **option A**, the cigar gas-bag in a rope net, in the packet paint and the gang's dark paint, with Skyreach's
    jetty, bell post and slate (the tutorial's skiff and jetty retired with the tutorial, 2026-10-13). Skyreach's backdrop
    islands: approved as an addition, then again after a polish pass for more interest.
  - Ahead of step 17, the same day: Main Street's look from its look sheet: the four false fronts, the boardwalk and awning
    kit, the shutters, the notice board and the town memorial.
  - Ahead of step 18 (2026-10-05): the Chapel of Saint Ada, shown half-timbered (plaster and oak) and as **white clapboard
    with a louvered belfry and a tall steeple**; the clapboard one chosen. The graves kit (Ellis's grave, Abel's frosted grave,
    the keeper's grave, headboards, crosses, fences, coffins, wreath and crepe) shown with it.
  - The train arrives and departs by **a short shot, then a fade** (the default).
- **Step 10:** superseded with the tutorial (2026-10-13; the archive).
- **Step 12:** talking doors and captions only for this area.
- **Step 14 (2026-10-06):** Hob's look, chosen from three concepts: **the Revenant crow**, lean and brooding, with a dull ember
  deep in the empty socket and wingtips and tail feathers fading to ash (masked and dithered, like the Unpaid). His game model
  approved from its look sheet the same day (“Hob's sheet looks good”): `SK_Hob`, 4,416 triangles, 24 bones, perched with wings
  folded, snapping onto `SOCKET_Perch_*`.
- **Step 15 (2026-10-06):** Sexton's look, which sets every human after him, chosen from three concepts: **the Gentleman**,
  very tall and gaunt in a black frock coat buttoned to the throat, an exaggerated stovepipe, coat tails over the rail, legs
  crossed, long pale fingers on the ledger and a dip pen; his face in shadow but for the point of a pale chin, and a tarnished
  coin weighting the ledger's ribbon as the only hint of the ferryman. Then his seated game model (“Sexton is good”):
  `SM_MisterSexton` (11,974 triangles, LODs at 50% and 20%, no Nanite, one hull) sits on the lookout's `SOCKET_Sit`, with
  `SM_SextonLedger` on his `SOCKET_Ledger` and his captions from `SOCKET_Speaker`. His face above the shadow line never lights.
- **Step 16 (2026-10-06):** the Unpaid's look, chosen from three concepts: **the clothes they died in** (a homesteader in hat
  and vest, the coal burning through his chest, fading into shroud strips), made gaunter, with the hungry dead's face and lunge;
  the render of that mix confirmed the same day; the rig and mesh built from it.
- **Step 17 (2026-10-06):** Ellis's wanted poster, chosen from three: **B, a woodcut of a masked rider**, so the face stays
  hidden and Ellis's gender open, with ALREADY written under DEAD OR ALIVE. The posters are decals (one atlas: the poster, its
  torn remnant, Calder's note and a scrap).
- **Steps 14, 15 and 17 (2026-10-07, as recommended):** the first-draft lines for Mains 1 to 3 stay as written, and the Ledger
  shows on Skyreach too once Sexton has handed it over: it's the book Ellis carries.
- **Step 18:** the backlog Reliquary design for the smashed one.
- **Step 21 (approved 2026-10-05):** the boss bar and the test fight.
- **Step 22:** Abel's look, chosen 2026-10-06 from three concepts: **A, the Sunday keeper**. His difficulty and the ending's
  wording approved 2026-10-08 with the play-through (“it all looks good”; the drafted lines stand).
- **Step 23 (2026-10-07, as recommended):** Heirloom is a named Epic with fixed parts and the line *Hold the door.*, not a
  Legendary; its special effect waits for unique legendaries. Its parts are the user's pick of the art session's candidates
  (B, 2026-10-07): the Ranchhand's Heritage body, Trap barrel (56 cm, Abel's length), Crown muzzle, Tube6 magazine, Flip sight,
  Field stock and Walnut pump; SM_AbelPump is remodeled to match, so the gun Delia hands over is the one Abel fought with.
- **Step 26 (approved 2026-10-08):** the Supply Crate and Strongbox out of the backlog.
- **Step 27:** whether the *Gilded Lily* stays an airship (deferred 2026-10-01 until Ransom's Rest was complete; on 2026-10-08,
  with Ransom's Rest approved, the next level was put on hold while the user polishes other things).
- **When the *Lily* starts:** a Hollow's signature legendary guaranteed on the first defeat, or always 15%.

## Build steps

All 27 steps are done. Ransom's Rest was approved on 2026-10-08 after the user's play-through from the start to the station
board (“it all looks good”). Each step's full text: the archive. The expansion's steps: expansion §12. Per step, what stands:
- 1. The revised story: `Docs/Story.md` is canon; this doc sits beside it.
- 2, 2b. The map (approved 2026-10-01, with changes): see *Decisions*.
- 3a–3c. The terrain generator for any area, its feature types and the grounded setting, on `Art/Levels/TerrainTest/layout.json`
  (a 150 m square: a pit and its ramp, a 20 m cliff in two stacked courses, a scarp, a ridge, two outcrop knobs, two plateaus
  with ramps, a mesa, a dry gully, a creek that ends in a falls into a gorge, a rail bed through a saddle), the open-ground
  metric with its plan overlay, and a layout checker (ramp grades and widths, every cliff course at most 12 m, features clear
  of the seam band). The grounded build prints: a seam gap of 0 mm and a normal difference under 1°; triangles per piece within
  budget (core 150k, ring 40k, canyon wall 20k, backdrop 6k).
- 3d. Bounds, fall recovery and the minimap on `/Game/Maps/Dev/Lvl_TerrainTest`.
- 4. The greybox layout, `Art/Levels/RansomsRest/layout.json`.
- 5a. Terrain, bounds and sky in the game (approved 2026-10-07).
- 5b. Overtaken by the real buildings and train; its last checks (Stage Gap past the parked train, the zones' draws) folded
  into step 27.
- 6–8. Ranks; level bands and XP; sessions that travel.
- 9. The train, depot and skiff (look approved 2026-10-02).
- 10. Leaving Skyreach (superseded 2026-10-13).
- 11–13. Missions as data; captions, doors and graves; day and dusk.
- 14–17. Main 1; Sexton and Main 2 (approved 2026-10-06); the Unpaid; Main Street, Main 3 and Side 1.
- 18–20, 22, 24, 25. The chapel and Main 4; the Sink and Main 5; the Gravemother; Abel and Main 6; travel by train and Main 7
  (the shots from the console: `Looter.Train.Depart`, `Looter.Train.Arrive`); Side 2. Built 2026-10-07, approved 2026-10-08
  with the play-through (“it all looks good”); the drafted lines stand.
- 21. The boss framework (approved 2026-10-05). 23. Heirloom (built 2026-10-07).
- 26. Ranger caches: `AChest`; the windmill's crate under the tower, the Sink rim's, the bluff path's, and the gang's Strongbox
  in the sheriff's office (approved and built 2026-10-08; approved with the play-through).
- 27. Finished (2026-10-08): every `Looter.*` test green, a Medium tour under 8.3 ms at every view, a Ledger page for every new
  creature and character (the Unpaid, the Gravemother, Hob, Sexton, Delia, Aldana, Tilly, Ruth, Abel, Amos, and the seven
  names), and `Docs/Performance.md`. Also built: the layout's fences, walls, grave rows, cairns and yard props
  (`build_area_dressing.py`; every fence plumb and stepped down slopes); the ridge faces broken up with slope scrub in patches,
  crease pines and far-tree groves; the den dressed; the Sink's pit wall in the art's cliff panels with talus at their feet;
  the ramps' walkways clear of cliff pieces; the burial deck's boards clear of the Rim's rock.
