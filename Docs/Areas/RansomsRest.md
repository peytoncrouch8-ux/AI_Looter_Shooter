# Ransom's Rest: the first area

This is the first area of the Revenant campaign, and the game's hub from then on. It is the plan to approve one step at a time; nothing is built yet. It lives in the repo as `Docs/Areas/RansomsRest.md`, beside the story, `Docs/Story.md`. The six step 1 questions were decided on 2026-10-01 (see *Decisions for you, by step*). Step 2, the map, was approved the same day with your changes: Ransom's Rest is grounded on Teropa, the earth-like world where the story takes place, instead of being an island, and cliffs and obstacles are to be added to break up the open ground. Later that day you approved the valley ending at the Rim (step 2b) and four of the six story proposals, decided that travel between areas is by train, and deferred the *Gilded Lily*'s question until Ransom's Rest is complete. Building starts next, at step 3: the main session builds Ransom's Rest with several agents, to a professional standard.

**Coordinates** are in metres: x east, y north, with the origin at the centre of the play space (the same point as before, about 11 m east of the town gate). `layout.json` uses centimetres with X north and Y east, so X = y × 100 and Y = x × 100.

## The fantasy

Come home dead. Ransom's Rest is the place Ellis ran from at sixteen and helped rob ten years later. It is a high valley in the Reaches, the frontier at the far west of Teropa's great continent. The valley and its town share the name, and locals call it “the Rest”. It is golden late-summer farmland with a one-street town along Main Street, a white chapel on a knoll, a sinkhole full of spiders, and burial boards on a point of the Rim, facing the sunset over Gravewind Canyon. Nobody left on Ransom's Rest wants to see Ellis. With the saint dark, years of the Rest's dead have drifted home to their headboards and stand in the chapel yard and the hayfields with nowhere to go, and only Ellis sees them clearly. It is a homecoming in reverse: every place is a memory, and the last person Ellis finds is Pa.

## At a glance

| | |
|---|---|
| Map | `/Game/Maps/Lvl_RansomsRest` (new), built from `Art/Levels/RansomsRest/layout.json`. Its area asset is `DA_Area_RansomsRest` |
| Setting | A high valley in the Reaches, on Teropa. Ridges close it on the north, east and south. On the west it ends at the Rim, a 70 m drop into Gravewind Canyon, with plains running to the sunset beyond |
| Size | About 309 × 205 m with Gravewind Point and Stage Gap: about 42,800 m², roughly 1.5× the tutorial island (about 28,500 m², both measured on their outlines). The valley floor is about 245 × 205 m |
| Play time | Main path about an hour; 75–90 minutes with the side missions |
| Enemy levels | 1–10 until the campaign is finished. A main-path player leaves at about level 9 |
| Missions | 7 main, 3 side (one is the Gravemother), and optional Ranger caches |
| Boss | Abel Ransom, the Keeper (one of the Unpaid) |
| Legendary monster | The Gravemother, in the Sink's den |
| New enemy family | The Unpaid |
| Light | Golden late afternoon, with the sun low in the west-southwest over the canyon; a scripted dusk for the cold open and the boss |
| Arrival | The first cast-off from Skyreach: the skiff sails into a cloud, and it parts on the gang's skiff gliding out of the evening cloud in the west seven days ago, with the player as Ellis (the cold open). “Skip the tutorial” goes straight to the cold open. Every later trip here arrives on the depot's platform: by train, or back from a practice trip to Skyreach |
| Exit | The depot at the east end of town, by the undertaker's yard, where the line comes in through Stage Gap. Its station board lists “Skyreach (practice)” after the first cast-off, and the *Gilded Lily* after Main 7, when Tilly's hearse car waits at the platform |

## Size and shape

Ransom's Rest is not an island. It is a high valley on Teropa, the earth-like world where the story takes place. Ridges close the basin on the north, east and south. On the west it ends at the Rim, a 70 m escarpment over Gravewind Canyon, with the sunset beyond. The valley floor is about 245 m east to west and 205 m north to south. Gravewind Point reaches 45 m west off the Rim, and Stage Gap cuts 18 m into the east ridge. In all it is about 309 × 205 m and 42,800 m², roughly 1.5× the tutorial island. Coordinates are unchanged. Heights are measured from the ground a thing stands on, and a negative height is a drop.

**What closes it in:**
- **West, the Rim:** a 70 m drop into Gravewind Canyon. Past it the player sees the canyon floor in blue haze with a thin river, its lower far wall about 300 m off, and the plains of Teropa running to the sunset with a few far mesas. The Rimrock stands on the lip beside the farm, and the lip is open on Gravewind Point and at Ransom's Point. Locals call the cold wind that pours off the Rim at dusk and out over the canyon the Gravewind.
- **North, Larkspur Ridge:** steep pines over an 8–12 m cliff band, with the crest 40–60 m up. The Old Quarry is cut into its foot at (20, 95). Higher forested ridges and a far blue range stand behind it.
- **East, the East Ridge:** about 35 m high, with a rock band at its foot. Stage Gap, at x 128–146 and y −5 to 11, is a saddle 16–20 m wide between 12 m rock walls, and the railroad's cut through the ridge. The line comes up out of the wooded valley past the gap, runs west through the cut along its south side and ends at a buffer stop by the depot, at the east end of town. The platform runs beside the track from the depot to a water tower in the gap. The train stands at the platform with its locomotive at the gap's mouth, facing out. Past the gap the ground falls away to the wooded valley, and the line runs down into it on a short timber trestle. A rockslide buried the old stage road down that side, and its broken slabs bank the cut on both sides of the locomotive, so nobody walks out east.
- **South, the Hogback:** a grass ridge about 30 m high with a 6–10 m rock band at its foot and dead trees along its crest. Ransom's Point is its west end. Mill Creek leaves through Mill Gorge at (75, −90).

Nothing past the boundary can be walked: cliff bands, slopes too steep to climb, the canyon drop (Hob catches a fall), and in Stage Gap the parked train and the rockslide. Invisible walls behind the ridge feet are only a backstop. The ridges' outer slopes, the canyon wall below the Rim's cliff kit, the canyon floor and its far wall are cheap generated meshes. The plains and far ranges are unlit silhouettes. The line down the far side of the gap is a few track pieces with no collision, which the train runs on only in its arrival and departure shots. Nothing past the Rim has collision. Together they replace the backdrop islands here (Skyreach keeps its own). Only the valley's own terrain is tagged `Ground`, so the minimap covers the valley and the ridge feet, dims everything past the boundary and never shows the canyon.

**Features and heights:**
- **Ransom's Point** (+20 m): a bluff in the south-west corner, where the Hogback meets the Rim. Its top is about 40 m across, centred at (−92, −80).
  - Its west face is the Rim, and its north and east faces stand 16 m over the farm terrace.
  - Its south face drops 20 m to **the Mooring Ledge** (−102, −101), a rock shelf over the Mooring Notch, a cleft of the canyon. The gang's skiff came up the canyon and tied up there, out of sight of town.
  - The lookout tower stands on the top facing the sunset, with stairs down the cliff to the ledge.
- **The bluff path** (unchanged): a 64 m ramp climbing about 16 m at about 25% (14°). It starts at the family plot's west gate (−108, −46), runs east along the bluff's north face, bends south along its east face and comes onto the top from the east. It has two wide bends and no hairpins.
- **The farm terrace** (+4 m) in the south-west reaches from the Rim east to about x = −40 and north to about y = −6, and the orchard runs down its east edge. **The town** is on the flat valley floor.
- **The chapel knoll** (+10 m) in the north, and **boot hill** (+5 m) behind Main Street.
- **The Sink** (−12 m): a collapsed sinkhole 36 m across at (65, 46), with a 45 m ramp curving down its south wall. The Gravemother's den is a cave mouth on the east wall, under the overhang of Den Rock.
- **Mill Creek** runs from a spring at (84, −16) south through the fields to **Mill Falls** (75, −88), where the existing waterfall drops 14 m into Mill Gorge.
- **The Dry Wash:** a dry creek bed 8 m wide and 3 m deep. It runs from the saddle between boot hill and the knoll (−28, 30) west to the Pour-off (−115, −6), where it spills over the Rim. The keeper's path crosses it on Keeper's Bridge (−95, 2).
- **Gravewind Point:** a promontory of the Rim reaching 45 m west, 70 m above the canyon floor.
  - The burial boards deck (25 × 18 m, centred at (−150, 12)) covers its flat tip, and its west end is built 3 m out over the drop.
  - Two rocks at its neck, **the Keeper's Gate** (−128, 13), frame the road. The boss fight's fog wall seals the gap between them.

**What breaks up the open ground.** Every stretch that was blank on the step 2 map now has rock, trees, a wall or a ruin in it. Outside the zones no gap is wider than about 19 m, except the crossroads at the keeper's grave, which stays clear on purpose. Everything comes from kits, instanced: the cliff kit, a new freestanding outcrop kit for rocks seen from every side, boulders, pines, oaks and dead trees, fences, new stone walls, and the house trim sheet. It adds instances rather than draws. Exact outlines go into `layout.json` at step 4.
- *The farm side:*
  - The Rimrock (6–7 m): three pieces on the lip from (−117, −58) to (−114, −12), with two gaps onto the sunset.
  - The orchard, an avenue on both sides of the farm road.
  - Delia's salt line, the farm's fence, from the Rimrock round the orchard to the Tumble.
  - The Three Widows: three 7 m slabs at x −85 to −69, y 1 to 4.
  - The Tumble: rockfall below the bluff's east face, which closes off the Mooring Ledge.
  - On the bluff top, a waist-high nest ring and two 3 m rocks south of the path.
- *The west road:*
  - The Dry Wash, with 5 m willows on its lower half.
  - The sheep fold (−79, 21) and the fallen glebe wall, both low cover.
  - Widow's Pines (13 m) filling the north-west, at least 10 m off the road.
  - The Burnt Homestead (−72, 49) and the old lantern house (−102, 35), roofless ruins by the road.
  - Knee-high cairns from the keeper's grave to the Keeper's Gate.
- *The middle:*
  - The Gate Stones, the old pound wall and two burnt wagons around the town gate. The stones stand on the gate's north-west side, clear of the farm fence.
  - The stock pens and the burnt livery behind Main Street's south side.
  - The hanging tree (20, 46) by the north road.
  - The Spoil Bank (5 m) of quarry rubble at x 16 to 42, y 56 to 76, and the quarry track up to the Old Quarry.
- *The south:*
  - Coffin Rock, an 8 m flat-topped butte at (−15, −60).
  - The Nose, a spur of the Hogback at x 2 to 8, y −97 to −80.
  - The Hogback talus and the Scatter, boulders between them.
- *The east:*
  - The hayfields, split by Amos's fence, the barn yard fence and a stone field wall.
  - Mill Creek's bottom, with 10 m willows along its east bank and a springhouse at the spring.
  - The Stone Teeth: 9 m rock fins at x 98 to 106, y −26 to −58.
  - The Sink road cottonwoods behind the east end of Main Street.
  - Hearse Rock, a spur walling the undertaker's yard on the north.
  - Den Rock over the den.
  - The Webwood: dead, webbed trees north of the Sink.
  - The Sink's broken rim fence, and collapsed blocks and coffins on its floor.

**Open ground, before and after:** the widest empty circle outside the zones in each open stretch of the step 2 map. Fences, low walls and cairns count as breaks: they are cover, even though a player sees over them.

| Stretch | Before | Now |
|---|---|---|
| The west field, between the farm, the west road and the chapel | 76 m | 19 m |
| South of the orchard | 70 m | 17 m |
| West of the town gate | 55 m | 19 m |
| Between the chapel and the Sink | 45 m | 18 m |
| Between the farm road and the town | 40 m | 15 m |
| East of Ransom's Point | 39 m | 17 m |
| The root of Gravewind Point | 38 m | 24 m |
| Between Mill Creek and the East Ridge | 35 m | 15 m |
| North of the undertaker's yard | 34 m | 16 m |
| The north-west corner | 34 m | 8 m |
| The south edge west of the barn | 30 m | 14 m |
| North of the Sink | 29 m | 15 m |

The two 19 m gaps are one gap, at (−57, 5): the grave's line to the bell tower, kept open on purpose. The 24 m one is the crossroads at the keeper's grave.

None of this changes a fight's ground. Inside every fight's area it is cover standing on that ground, and the wash, the gorge, Coffin Rock and the Nose are outside all of them (see *Encounters stay on one level of ground*).

**The route** is a loop of about 1 km on foot that passes through town in the middle and at the end:
1. The grave and the farm (south-west).
2. Up the bluff path to Ransom's Point.
3. Along the farm road through the orchard into town (centre).
4. North to the chapel, then north-east into the Sink.
5. The west road to Gravewind Point, between the wash and the pines.
6. Home to the farm by the keeper's path over Keeper's Bridge.
7. East through town to the depot, where the line comes in through Stage Gap.

**Landmarks guide it.** From the grave the player sees the bluff above, where Ellis died, and the chapel's bell tower over the orchard. The Three Widows mark the keeper's path and Coffin Rock marks the south. At the end of the west road the Keeper's Gate frames the deck. Down Main Street, the water tower in Stage Gap marks the depot.

**Sightlines are blocked on purpose.**
- The orchard and the farm terrace hide the town from the grave. The orchard and Coffin Rock hide Whitlock Fields from the farm.
- The chapel knoll and boot hill hide the north-east.
- The Sink is a hole you see only from its rim, and the cottonwoods behind Main Street keep it that way from the street.
- The creek willows hide Mill Falls until you reach the fields.
- Only Ransom's Point sees the whole valley, and nothing new spoils that.
  - Tall rock and trees stand where their shadow from the tower falls on nothing that matters.
  - Whatever stands on the tower's lines stays low: the nest ring under 1.2 m, the springhouse 1.5 m, the burnt livery and the Scatter 3 m, the gate's south rock 4 m, the wash willows 5 m.
  - The tower sees every landmark, all of Main Street, the deck and 90% of the west road.
  - It is the planned heaviest view, now with the canyon and the horizon in it as well, and it is measured on the greybox at steps 5a and 5b, before any art.
- The grave keeps its view of the bell tower. The willows stop short of that line, and the Three Widows stand to one side of it, well under it.

**The palette** sets it apart from Skyreach's green midday: golden grass, ochre soil, an amber orchard, blue larkspur along the fences, dark pines on Larkspur Ridge, the canyon's blue haze to the west and a low warm sun.

It is one level, with no streaming.

**What the terrain generator needs** (steps 3a–3c; how it changes is under *Tech needs*). Today it handles one island: an outline with a rim drop and an underside, exactly one plateau with one ramp, one pond and one creek, each found by a fixed id, and three fixed cliff groups. Ransom's Rest needs:
- a valley instead of an island, with raised terrain outside the boundary in place of the rim drop and the underside:
  - ridges with cliff bands on the north, east and south, cut by a saddle for Stage Gap (its floor graded for the track) and a gorge for Mill Creek;
  - on the west, an escarpment edge down to a backdrop canyon;
- the playable boundary as a polygon, with its open edges marked;
- features as lists by type: two plateaus (Ransom's Point with its ramp, the farm terrace), two hills (the chapel knoll and boot hill) and a mesa (Coffin Rock: a plateau with a cliff ring and no ramp);
- a new pit type for the Sink: the plateau turned upside down, with an inward-facing cliff ring and a ramp;
- dry gullies (the Dry Wash) as creeks without water, and a creek that ends in a falls into a gorge;
- cliff groups per feature, and stacked cliff courses for the 20 m bluff, the Rim's lip and the ridges' bands, since the tallest cliff piece is 12 m;
- outcrops, boulder fields, tree lines, walls and ruins as placements and scatter areas.

The den can't come from a heightfield, so it is a separate mesh set into the Sink's wall under Den Rock. The minimap's top-down bake won't show under it, which is fine for a cave.

## Zones

| Zone | Where | What's there | Used by |
|---|---|---|---|
| Ransom Farm, Orchard and Family Plot (spawn) | (−70, −38), +4 m | The existing Farmhouse (−58, −40) with black crepe on the door, plus the Barn, Well and Outhouse. The family plot (−86, −36) under a dead oak, inside a picket fence: 8 old headboards, Ellis's fresh grave and Abel's frosted one. The amber orchard runs down the terrace's east edge as an avenue on both sides of the farm road. Delia's salt line runs along the farm fence from the Rimrock round the orchard to the Tumble and keeps the Unpaid off the farm, so it is a safe zone: the town gate's Unpaid give up the chase at it. The Rimrock stands on the Rim's lip to the west, with two gaps onto the sunset, and the Three Widows mark the keeper's path to the north | Main 1, 6, 7 |
| Ransom's Point | (−92, −80), +20 m | A bluff where the Hogback meets the Rim; its west face drops to the canyon. The lookout tower, intact, with a railing deck facing the sunset; the boards are still stained. Cliff stairs lead down the south face to the Mooring Ledge, where the gang's mooring post still holds a frayed rope. Spiders nest among the rocks at the top, inside a waist-high ring of broken rock, with two 3 m rocks south of the path. The Tumble, a rockfall, fills the corner under the east face | Cold open, Main 2, the new-moon payments later |
| Main Street | (28, 0) | An 80 m street from the town gate (−11, 0) east to (70, 0), with boardwalks and false fronts: Bright & Daughter, Undertakers (Tilly); Pruitt's General Store; the Gilt Spur saloon, boarded up (“CLOSED FOR MOURNING”); the empty sheriff's office with the Rim Rangers' notice board; the town memorial (two hats on a post with a wreath). The existing log cabins and cottages fill the side lots. Empty stock pens and the burnt livery stand behind the south side, and cottonwoods behind the east end hide the Sink. The Gate Stones, the old pound wall and two burnt wagons stand around the gate. Shutters slam as Ellis passes. Safe after Main 3 | Main 3, Side 1 |
| Boot Hill | (0, 32), +5 m | A small hillside cemetery behind the street, with a respawn grave. The hanging tree stands by the north road, and north of it lies the Spoil Bank, the quarry's rubble heap, with the quarry track up to the Old Quarry under Larkspur Ridge | Roaming Unpaid after Main 4 |
| Undertaker's Yard and the Depot | (100, 2) | From the back of Bright & Daughter east to the depot and into Stage Gap, the railroad's cut through a saddle between 12 m rock walls. A coffin shed, and the depot (114, 9): a timber station house with the station board by its door. The track comes in through the gap along its south side and ends at a buffer stop at (110, −1). A low timber platform runs beside it from the depot to the water tower (140, 7) in the gap, and a signal stands at the gap's mouth. The train stands at the platform: the locomotive at the gap's mouth, facing out, then a passenger car, then Tilly's black hearse car by the depot. Until Main 7 the train is cold and shut; from Main 7 the locomotive has steam up and the hearse car's door stands open. The line runs on down into the wooded valley past the gap, and a rockslide buries the old stage road down that side; its slabs bank the cut on both sides of the locomotive. Hearse Rock walls the yard on the north, and a roofless springhouse covers Mill Creek's spring behind it. Every trip arrives and leaves here, including practice trips to Skyreach | Main 7, then travel |
| Chapel of Saint Ada | (−19, 60), +10 m | A white clapboard chapel with a louvered belfry and a tall steeple, the tallest building on Ransom's Rest. A churchyard of about 50 headboards behind an iron fence, with dead trees. Inside, the smashed Reliquary, dark and empty. A respawn grave. The fallen glebe wall runs down the knoll's west side | Main 4 |
| The Sink | (65, 46), −12 m | A collapsed sinkhole 36 m across, ringed by the cliff kit and a broken warning fence (the ramp head is its gap). A webbed ramp curves down the south wall to a floor of old coffins, collapsed blocks, egg sacs and webbing. The Gravemother's den opens under the overhang of Den Rock, a dome of fractured rock on the east rim. The Webwood's dead, webbed trees line the north side | Main 5, Side 3 |
| Whitlock Fields and Mill Creek | (60, −58) | Hayfields split by a stone field wall, with hay bales, Amos's fence along the north, and the Whitlock barn and the existing windmill inside a yard fence. The creek runs south in a bottom 2 m deep to Mill Falls, where the existing waterfall drops 14 m into Mill Gorge through the Hogback. Willows on its east bank hide the falls until you reach the fields, and the Stone Teeth, a row of rock fins, close the east. Meadow slimes in the creek bottom, as plain wildlife | Side 2 |
| Gravewind Point | (−150, 13) | A promontory of the Rim reaching 45 m west, 70 m above the canyon floor. A 25 × 18 m timber deck covers its tip, with its west end built 3 m out over the drop: 8 burial boards on posts, 3 keeper's lantern posts, and fog rising out of the canyon at dusk. The Keeper's Gate, two rocks at its neck, frames the deck. Knee-high cairns line the road to it from the keeper's grave (respawn) at its root (−112, 17), near the roofless old lantern house | Main 6; Abel afterwards |
| The west road and the keeper's path (between zones) | The west road from the chapel (−19, 54) to the deck; the keeper's path from the farm (−70, −30) to the keeper's grave | The west road runs between the Dry Wash and Widow's Pines, past the sheep fold, the fallen glebe wall and the Burnt Homestead's standing chimney. The keeper's path leaves the farm through the salt line's gate, passes the Three Widows and crosses the wash on Keeper's Bridge (−95, 2). Both meet at the keeper's grave | Main 6, Main 7 |

## Encounters stay on one level of ground

Creatures steer without a navmesh. They won't step off a drop of more than about 4 m, and they only turn so far around obstacles. So every group lives, fights and gives up the chase on one level of ground:

| Group | Lives on | Gives up at |
|---|---|---|
| Bluff spiders (Main 2) | The top of Ransom's Point, round the nest ring and the two 3 m rocks | The top's edge; none on the ramp |
| Town gate Unpaid (Main 3) | The flat ground at the gate, with the fight centred a little north-east of it so it keeps off the farm fence. The Gate Stones (on the gate's north-west side, clear of the farm fence), the old pound wall and the burnt wagons give cover | 30 m from the gate or at Delia's salt line, whichever is nearer, so no chase crosses into the farm (the wash's head is 34 m out) |
| Chapel yard Unpaid (Main 4) | Inside the iron fence | The fence |
| Roaming Unpaid (after Main 4) | Boot hill and the north road, round the hanging tree | Their own stretch |
| Sink spiders and egg-sac spiders (Main 5) | The Sink floor, among the collapsed blocks and coffins | The foot of the ramp; nothing lives on the rim |
| The Gravemother and spiderlings | The den and the Sink floor | The foot of the ramp |
| Whitlock hands (Side 2) | The barn yard | The barn yard fence |
| Slimes | Mill Creek's bottom | The banks |
| Abel's adds | Spawned on the deck | The deck |

The obstacles keep to this rule. Inside every fight's ground they are cover standing on it: boulders, low walls, wagons, a rock ring, blocks on the Sink floor. Every new change of level (the Dry Wash, Mill Gorge, Coffin Rock, the Nose) is outside all fights, and nothing new stands within 5 m of the bluff path.

This is checked on the step 5b greybox by spawning each group with `Looter.SpawnCreature` and chasing it. If it is too limiting, a navmesh with cliff and ramp links comes forward to that step instead of waiting for the *Lily*.

## Missions

Sexton's deal comes early, about 15 minutes in, on the lookout where Ellis died (decided at step 1). Main missions give 30% of the current level's XP and side missions 20%, on top of kills.

### Main 1: Seven Days (about 8 minutes)
The skiff ride and the cold open, then Ellis wakes in the family plot beside Pa's frosted grave. Hob is there. Grandma Delia won't open the screen door to a corpse, but she sets a plate on the porch: *“I was to lay you on the boards tonight. Your Pa got up Wednesday night, came up through the dirt like it was fog. A keeper doesn't lie still while his saint is dark. He walks the boards at dusk.”* Ellis comes out of the grave with the guns they were buried with: whatever the player carried in the tutorial, or a Common Bullpup in the coffin after “Skip the tutorial”.
1. The cold open plays on the first arrival only. The player's skiff from Skyreach sails into a cloud, and it parts on the gang's skiff seven days ago, with the player as Ellis: it glides out of the evening cloud in the west, low over the plains, and drops into Gravewind Canyon toward the Mooring Ledge under Ransom's Point. Then dusk on Ransom's Point. “Skip the tutorial” starts here.
2. Claw out of the grave (press Jump three times).
3. Read the headboard beside yours.
4. Go up to the farmhouse.
5. Talk to Grandma Delia at the screen door.

**Reward:** XP. The family plot becomes a respawn grave.

### Main 2: Shall We Talk Business? (about 8 minutes)
Hob: *“Someone's waiting on you. Up top, where it happened.”* Spiders nest at the top of the bluff, and one glows blue. Hob: *“See the blue on that one? Fed longer on the dark. Hits harder, and its iron's better.”* At the lookout, on boards still stained with Ellis's blood, Mister Sexton sits on the railing. He offers the deal: seven embers at the new moon, *“and your father crosses.”* Ellis: *“They scattered. Where do I even start?”* Sexton: *“Ask a keeper. Their lanterns lean toward a saint's light.”* *“The keeper's dead.”* *“So are you, friend.”* The deal lands about 15 minutes in, so the seven names are the hook of the first quarter hour.
1. Climb the bluff path to Ransom's Point.
2. Clear the spiders nesting at the top (4, and one Restless Meadow Wolf).
3. Talk to Mister Sexton.
4. Open the Ledger (the bestiary, now in Sexton's voice: seven names, whereabouts blank).

**Reward:** XP and the Ledger.

### Main 3: Cold Welcome (about 8 minutes)
Into town. The living shutter their windows, and the Unpaid come for the corpse at the gate. Tilly Bright talks through her shop window: she laid out both Ransoms, and Abel's lantern wasn't on him. *“You've ruined my collar, by the way.”*
1. Follow the farm road through the orchard into town.
2. Fight off the Unpaid at the town gate (4, one of them Restless).
3. Find Bright & Daughter, Undertakers.
4. Talk to Tilly at the window.

**Reward:** XP and a guaranteed Uncommon gun from the undertaker's unclaimed effects. Main Street is safe from now on, and Side 1 opens.

### Main 4: Hallowed Ground (about 10 minutes)
The Unpaid hold the chapel yard. The bell won't call them to rest, because the saint's Reliquary lies smashed and dark. Father Aldana talks through the vestry door: *“A keeper's lantern can find an ember. In every town they robbed, the gang shot the keeper first and smashed his lantern. Your father's fell in the dark, whole. Spiders hoard anything a saint has touched. Look in the Sink.”*
1. Go up to the Chapel of Saint Ada.
2. Clear the chapel yard (12 Unpaid in two waves, and one Restless).
3. Ring the chapel bell.
4. Look at the Reliquary: a two-second Grave Sight flash of the ember lifting off the lid.
5. Talk to Father Aldana at the vestry door.

**Reward:** XP and the chapel yard's respawn grave. From now on the Unpaid walk the north road and boot hill. Side 2 opens.

### Main 5: The Keeper's Lantern (about 8 minutes)
The keepers walk the dead to the boards by lantern light, and Abel's lantern is in the Sink.
1. Climb down into the Sink.
2. Shoot down the three egg sacs (each lets out 2 spiders on the floor).
3. Take the Keeper's Lantern from the webbing. It is dark.
4. Climb out. The Gravemother's den is visible from the floor.

**Reward:** XP. Side 3 opens.

### Main 6: The Gravewind (boss, about 10 minutes)
Delia, through the door: *“Take him the lantern. Show him the way, even if he can't go.”* Starting the mission fades the Rest to dusk.
1. Carry the lantern to Gravewind Point, by the west road or the keeper's path. Both end at the keeper's grave, and the cairns lead on to the Keeper's Gate.
2. Hang it on the keeper's post.
3. Defeat Abel Ransom, the Keeper.
4. Sit with Pa (scene). He lights the lantern from the ghost light he carries.

**Reward:** XP and Abel's boss loot.

### Main 7: The Lantern Leans (about 5 minutes)
The flame leans north-east, over the ridges. Hob: *“That's Purcell. The Lily's moored out that way.”* Ned's page in the Ledger fills in.
1. Go home to Delia. She hands Heirloom out through the door: *“A keeper's buried with his lantern, not his iron. His lantern wasn't on him, so I kept this back. He'd want you to have it. Hold the door.”*
2. Go to the depot at the east end of town, by the undertaker's yard. Tilly's hearse car waits at the platform: her late father's funeral car, coupled to the train. Ellis rides in it between areas.
3. Read the station board.

**Reward:** XP and Heirloom. The *Gilded Lily* opens on the station board.

### Side 1: Wanted: Already Dead (after Main 3)
Ellis's wanted poster hangs all over Ransom's Rest, and someone has written ALREADY under DEAD OR ALIVE.
1. Tear down 6 wanted posters around town and the farms.
2. The last one is on the Rim Rangers' notice board, signed “Capt. R. Calder”, beside her note about three Ranger caches. This seeds Ruth.

**Reward:** XP and a guaranteed Rare gun.

### Side 2: Unfinished Business (after Main 4)
Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint went dark, and he drifted home to find it rotting in the field. He isn't angry yet.
1. Talk to Amos at his fence (only Ellis can see him).
2. Load 6 hay bales into his barn (hold Interact).
3. Drive off his old hired hands, Unpaid from the fever winter (6, one of them Restless).
4. Talk to Amos. He sits on his fence to wait for the saint to come back.

**Reward:** XP and a guaranteed Epic gun, the one Amos was buried with and never needed.

### Side 3: The Gravemother (after Main 5; Legendary monster)
Something big lives in the Sink's den, and it eats what the Unpaid leave behind.
1. Enter the den.
2. Kill the Gravemother.

**Reward:** XP the first time, and the Legendary loot table on every kill. She comes back on an arrival after at least 20 minutes of play since her last death. An arrival is coming in at the station, by train or back from Skyreach, or loading a session. Until the *Lily* exists, that means loading the session or coming back from Skyreach.

### Optional: Ranger Caches (only if you OK bringing the backlog chests in)
Find Ruth's three Supply Crates, one each under the windmill, on the Sink's rim and on the bluff path, plus the gang's Strongbox in the sheriff's office.

## Enemies by rank

| Enemy | Rank | Existing or new | Where | Notes |
|---|---|---|---|---|
| The Unpaid | Basic | New: `AUnpaidCreature`, its own small rig | Town gate, chapel yard, boot hill, north road, Whitlock barn, boss adds | 160 HP and 8 damage at level 1. Floats, with no legs, so no walk cycle and no navmesh. Its dull red coal is the crit spot, tested by how close the shot line passes, like the slime's core. Lunge and shriek. When stuck or far behind, it phase-steps: fades out and reappears 3–5 m closer. Three clothing tints. Ledger section: Enemies |
| Restless Unpaid | Rare | New rank variant | One in most scripted fights, plus 8% of spawns | Blue tag and a blue coal, ×2.5 health, faster lunge |
| Gravebound Unpaid | Epic | New rank variant | 2% of spawns | Purple tag and coal, ×5 health. Trait: its shriek slows the player for 2 s (a ring effect) |
| Meadow Wolf spider | Basic | Existing (`ASpiderCreature`) | The bluff top, the chapel yard's edge, the Sink floor | 300 HP at level 1, head crit |
| Restless or Gravebound Meadow Wolf | Rare or Epic | New rank variants | A placed Restless on the bluff top in Main 2; promotions | Epic trait: calls every spider within 30 m (the existing pack call, made wider and matched by a pack tag instead of the exact class) |
| Spiderling | Basic | A configuration of the existing spider, not a new class: 0.45× size, 20% health (60 HP at level 1) | The Gravemother's calls | Shares the Meadow Wolf's Ledger page. Uses the Basic table, so it can drop a legendary |
| Meadow slime | Basic | Existing (`ASlimeCreature`) | Mill Creek's bottom | Wildlife, not part of the story. 120 HP, 6 damage at level 1 |
| The Gravemother | Legendary | New subclass of the spider at 1.8× size, with a pale-hide material instance; the den is a new mesh | The Sink's den | Orange tag, ×12 health (about 5,600 at level 8). Crits on the existing head and abdomen hit hulls (no new bones). A charge with a long telegraph and a ground crack. Calls 4 spiderlings at 66% and 33%. No climbing and no leaps. Its own Ledger page |
| Abel Ransom, the Keeper | Boss | New, on the Unpaid rig | Gravewind Point | See *The boss* |

The egg sacs are shootable props modeled on the existing `ATargetDummy`, not enemies. Everything a spawner, an egg sac or a boss creates is gone for good when killed; only placed creatures come back. Every enemy that can die uses at least the Basic loot table.

## NPCs

| Character | Where | How they appear in this area | Missions |
|---|---|---|---|
| Grandma Delia Ransom | The farmhouse screen door | A talking door: a lamp behind the screen and captions. She won't open it to a corpse | Main 1, 6, 7 |
| Tilly Bright | Bright & Daughter's window | A talking window. Her shop sign reads “Back after the funeral” until vendors exist | Main 3; vendor later; her hearse car (Main 7, then travel) |
| Father Moses Aldana | The chapel's vestry door | A talking door | Main 4 |
| Hob | Everywhere in the story; never on Skyreach | A new small crow rig. He perches near the next objective, makes dry captions only Ellis hears, and comments on ranks, respawns and falls. He doesn't come along on a practice trip to Skyreach and says nothing there | All |
| Mister Sexton | The railing on Ransom's Point | A new seated model, the game's first human: undertaker's coat, stovepipe hat, face in shadow, a ledger on his knee. A posed idle only. Concepts come first | Main 2, the cold open |
| Abel Ransom | Gravewind Point | The Unpaid rig at 1.3× in a keeper's coat and hat, with a ghost lantern and a spectral Ranchhand pump (Heirloom's silhouette). After the fight he sits on his board for the rest of the game, with new lines after each ember | Main 6 |
| Amos Whitlock | His fence in Whitlock Fields, by the gate where the fields path passes | The Unpaid rig in work clothes, his coal banked low | Side 2 |
| Ranger Ruth Calder | None in person | Her signature and note on the notice board | Side 1 |
| Townsfolk | Heard, not seen | Doors bolt and shutters slam. Half of Ransom's Rest left when the saint went dark, which keeps the cast small | None |

Every new creature and character gets a Ledger page, checked by `Looter.Bestiary.Entries`. Delia, Tilly, Aldana, Sexton and Ruth get story-character pages that need no actor (new page type). Abel has one page: Enemies until “The Gravewind” is done, then Friends.

## The boss: Abel Ransom, the Keeper

The Unpaid rig at 1.3× in a keeper's coat, with his ghost lantern in one hand and the spectral twin of his Ranchhand pump in the other.

**Arena.** The burial boards deck on Gravewind Point at dusk: a flat 25 × 18 m timber deck on the point's tip, its west end built out over Gravewind Canyon, with 8 burial boards on posts for cover and 3 keeper's lantern posts. The sun sets behind him. A fog wall seals the Keeper's Gate, the gap between the two rocks at the point's neck, while the fight runs.

**Numbers.**
- Level: the player's level kept inside 1–10, plus 1 (bosses skip the ±1 roll). About 9 for a player at level 8.
- Health: about 40× an Unpaid of his level, about 10,500 at level 9. Tuned to a 3–4 minute fight with same-level Uncommon guns.
- Crit spot: his coal heart, gold like every boss's (the Boss rank's color, which stays clear of the legendary orange), at the standard 1.5×. His lantern arm covers it while he fights; it is open only when he grieves.

**Phase 1, “You brought them here” (100–60%).**
- He floats along the boards and fires spectral buckshot: slow, visible pellets in a cone after a one-second lantern flare.
- Up close he lunges with the shotgun stock.
- Every 25 s two Unpaid rise through the boards (at most 4).
- Every 12 s he stops and turns toward the sunset for 3 s, and the coal is open.

**Phase 2, “The bell” (60–25%).**
- He drifts out into the fog over the canyon and can't be targeted.
- The chapel bell tolls across the valley, and 8 Unpaid rise through the deck in two waves.
- The three keeper's lanterns go dark. Relighting them (hold Interact for 1.5 s each, under pressure) drags him back.

**Phase 3, “Let me go” (25–0%).**
- The Gravewind pours off the point toward the sunset: wisps and mild gusts toward the deck's open end. A fall off the deck is caught by Hob, as everywhere in the story: fall recovery puts the player back on the deck within a second.
- Abel tries to walk off into the wind, and the dark saint pulls him back. Each pull stuns him for 3 s with the coal open. Between pulls he fights faster.

**At zero** he kneels, the coal sinks to an ember, and the scene plays.
- *“...El? You came home.”*
- He tells it straight: Ned shot him, the Deacon wept, and a tall fella sat on the lookout rail and never lifted a finger.
- *“Bring Saint Ada home, El. I'll wait.”*
- He lights the Keeper's Lantern from the ghost light he carries, and its flame leans north-east, over the ridges, toward Ned.
- He sits on his board facing the sunset.
- Hob: *“Well. I've seen worse reunions.”*

**Why it lands.** The player fights the cost of what Ellis helped do. The best damage comes while Pa mourns, and winning doesn't free him. His last act as Keeper is to light the way for the whole campaign.

**Death and reset.** Ellis wakes at the keeper's grave at the root of Gravewind Point. Abel, his adds and the lanterns reset, and the fog wall drops.

**Loot.** The Boss table once: 3 guns at Luck 1.3, a 30% chance of a legendary. Abel is fought once; afterwards he is a friend (a separate, non-hostile actor). His adds use the Basic table.

## Loot and chests

**Kills** use the rank tables (see *Enemy ranks and legendary drops* in `Docs/Story.md`). Guns drop at the enemy's level (new: today every creature drop is level 1). Ammo drops on every kill, leaning 2× to the kill gun's class, with more boxes from higher ranks. The gun pool is the existing Bullpup and Ranchhand. There is no unique legendary on Ransom's Rest: the first, Dead Man's Hand, needs a pistol base and belongs to the *Lily*.

**The reward ladder:**

| When | Reward |
|---|---|
| Main 1, Seven Days | The guns Ellis was buried with: whatever the player carried in the tutorial, or a Common Bullpup after “Skip the tutorial” |
| Main 3, Cold Welcome | A guaranteed Uncommon gun |
| Side 1, Wanted: Already Dead | A guaranteed Rare gun |
| Side 2, Unfinished Business | A guaranteed Epic gun |
| Main 6, The Gravewind | Abel's Boss table: 3 guns at Luck 1.3 (30% chance of a legendary) |
| Main 7, The Lantern Leans | Heirloom, Abel's Ranchhand: a named Epic with fixed parts and the line *Hold the door.* Its special effect waits for unique legendaries |
| Side 3, The Gravemother | The Legendary table on every kill: 2 guns at Luck 1.3 (21% chance of a legendary). She comes back after 20 minutes of play |

**A first full clear** gives about 40 guns. It includes at least one legendary about 7 times in 10, or a little over half the time on the main path alone; most come from Abel and the Gravemother. Common guns stay the most common at every rank.

**Chests (optional, step 26, only with your OK).** The backlog Supply Crate and Strongbox:
- Three Supply Crates as Ruth's Ranger caches: 1 gun each at Luck 0.5 (3.7% legendary).
- The gang's Strongbox in the sheriff's office: 2 guns at Luck 1.0 (15.5% legendary).

The smashed Reliquary in the chapel is a story prop, dark and empty. It uses the backlog Reliquary design, which also needs your OK (step 18).

## The exit and the unlock

Finishing Main 7, The Lantern Leans:
- records the mission in the session's campaign record;
- fills in the Ledger's first page: Lucky Ned Purcell, the *Gilded Lily*;
- lists the *Gilded Lily* on the station board at the depot, beside “Skyreach (practice)”, which the board lists from the first day. Tilly's hearse car waits at the platform from then on. Until that level exists, choosing the *Lily* says “The line to the *Lily* isn't open yet” and the player stays on Ransom's Rest.

**The station board** is a LooterUI panel at every area's station. It lists every opened area the Keeper's Lantern has found, plus one blank line naming the mission that opens the next (for example “Finish ‘The Lantern Leans’”), and “Skyreach (practice)” after the first cast-off. Later areas aren't shown, and it always lists every opened area, so travel can't strand the player.

Ransom's Rest then stays the hub. Every later station's board lists it. Its band stays 1–10 until the campaign is finished, so it is easy and low-XP to revisit; after that every band follows the player, and Hanging Moon nights use the endgame band.

**Skyreach stays reachable as a practice island** (decided at step 1). It stays outside the story.
- The first cast-off from Skyreach plays the cloud match cut and the cold open. Its confirm reads: *“Leave Skyreach? Your story begins. You can come back to practice any time.”* “Skip the tutorial” counts as that first cast-off.
- After that, every station board (the depot here, and every later station) lists “Skyreach (practice)” at any time. Choosing it is a plain transition, a fade with no cutscene, to Skyreach's jetty. This is a default you can change (step 10).
- Skyreach's jetty board lists every opened area and returns the player to its station the same way. A trip from Skyreach to Ransom's Rest arrives on the depot's platform.
- No story character, line or plot appears there. Hob doesn't come along and says nothing there. The story never names Skyreach; only the station boards, Skyreach's jetty board and the session picker do.
- Its creatures give 0 XP. By default they drop only ammo on return visits, never guns, so it stays practice and not a loot farm. This is a default you can change (step 10).
- The session save keeps each map's world state, so Skyreach and Ransom's Rest each keep their own world between trips.

## The hub afterwards

- On Ransom's Rest, every trip starts and ends at **the depot** by Tilly's yard, including a practice trip to Skyreach.
- **Respawn graves:** the family plot, the chapel yard, boot hill and the keeper's grave.
- **Delia's door** is where story missions start, and a plate is always set on the porch.
- **Ransom's Point** is where each ember is paid at the new moon.
- **Abel** sits on his board at Gravewind Point, with new lines after each ember.
- **Tilly's parlor** becomes the grave-goods vendor once currency exists.
- **The hub changes cheaply:**
  - A little color returns to the orchard with each ember paid (material swaps).
  - Ozias Penhallow arrives after the *Lily*, and Ruth's tent after Dustwater.
  - A new-moon night version serves the end of Act II.
  - Hanging Moon nights come in the endgame.
- **Revisits:** the Gravemother and the placed creatures come back; promotions reroll after 20 minutes of play. Abel isn't fought again; he's a friend now.

## Art needs

| Item | Existing or new | Notes |
|---|---|---|
| Terrain, macro color maps, scatter mask | New terrain, existing method, with a new grounded mode | `Art/Levels/RansomsRest/layout.json` through the terrain scripts for any area. Golden grass and ochre soil. The core: at most 150k triangles in about 20 tiles, with full Nanite fallbacks, and a 4096 px macro map (split 2×2 if it looks soft). The ring past the boundary gets its own 2048 px macro map |
| The ridges, the canyon and the horizon | New, generated | Past the boundary: the ridges' outer slopes in the surround ring (6–8 sectors, at most 40k triangles, collision on the ridge sectors); the canyon wall below the Rim's cliff kit (4 sectors, made like the tutorial island's underside wall, no collision); the canyon floor, with its thin river painted in, and the lower far wall about 300 m off; and unlit silhouettes of the plains, far mesas and far ranges at about 3, 6 and 12 km (3 layers × 4 sectors, at most 6k triangles), tinted by the light state. They replace the backdrop islands here |
| Farmhouse, Barn, Well, Outhouse, LogCabin, Cottage, Windmill, Bridge, Fences, LanternPost, FarmProps (hay bales), VillageProps, Containers, Rocks, Cliffs, Trees, Pines, DeadTree, Undergrowth, GroundCover, Waterfall, chimney smoke | Existing | Reused as they are. Bridge, the footbridge, is Keeper's Bridge |
| Amber orchard | Existing apple trees, new material instance | Leaf tint |
| Blue larkspur accent | Existing GroundCover, new variant | Along fences |
| Freestanding outcrop kit | New | `Art/Models/Rocks/Outcrops.py`: four tors of 4–10 m and a 12 m rock spine, finished on all sides, 1.5–3k fallback triangles. For rocks seen from every side: the Three Widows, the Stone Teeth, Hearse Rock, Den Rock, the Keeper's Gate and the Rimrock. The greybox stands cliff faces up in their place |
| Stone walls | Existing, plus new pieces | `Fences.py`'s dry-stone wall makes the pound wall, the glebe wall, the field wall and the sheep fold. Broken, fallen and corner pieces are new, in `Art/Models/Props/Ruins.py` |
| Far trees and rocks | New | Opaque low-poly trees and rocks for the ridges past the boundary: instanced, with no collision, no shadows and no masked leaves. Trees inside the boundary get a last LOD card for long views |
| Ruins and dressing for the obstacles | Existing kits, new arrangements | The Burnt Homestead's standing chimney, the roofless old lantern house and springhouse, the burnt livery, the fallen quarry derrick, the hanging tree's rope, cairns and the Gap rockslide, from the house trim sheet, Rocks, DeadTree, Fences and VillageProps. The new pieces (the chimney, the roofless huts, the livery's footing and posts, the derrick, two burnt freight wagons, the cairns and the rope) are in `Art/Models/Props/Ruins.py` |
| Lookout tower, intact, with cliff stairs | New: `Art/Models/Buildings/Lookout.py`, a sound version of the ruined `LookoutTower.py` | For Ransom's Point: a railing deck facing the sunset, stairs down the south face to the Mooring Ledge, and the gang's mooring post with a frayed rope |
| False-front kit | New | `Art/Models/Buildings/FalseFronts.py` and `Art/Models/Props/Boardwalk.py`: facades, boardwalk, awnings, hitch rails, shutters that can slam (separate, hinged) and sign boards lettered in Rye (`Art/Fonts`, OFL), on the house trim sheet. Makes Bright & Daughter, Pruitt's store, the Gilt Spur and the sheriff's office, with the notice board and the town memorial |
| Chapel of Saint Ada | New | `Art/Models/Buildings/Chapel.py`: white clapboard with a louvered belfry and a steeple (chosen over plaster and timber on 2026-10-05), and the bell as its own model so it can swing; a simple interior (pews, the altar, the Reliquary's niche) for the view inside; 5–8k fallback triangles |
| Graves kit | New | `Art/Models/Props/Graves.py`: old and fresh headboards, a frosted mound, crosses, iron fence and gate, the family plot's picket fence, coffins, a mourning wreath, black crepe. Instanced |
| Smashed Reliquary | Backlog model variant (needs your OK) | The Reliquary from `Art/Backlog/Loot/Chests.py` with a cracked lid and no crystal |
| Burial boards deck | New | `Art/Models/Props/BurialDeck.py`: a timber deck on Gravewind Point's tip with its west end cantilevered 3 m over the canyon, 8 biers on posts, 3 keeper's lantern posts |
| Keeper's Lantern | New | A held prop, in `BurialDeck.py` beside the lantern posts |
| The Sink | New pit feature plus the existing cliff kit | Web cards (masked, at most 20 per view), egg sacs, old coffins |
| The Gravemother's den | New | A cave mouth and overhang mesh set into the Sink's east wall, under Den Rock |
| Skiff and jetty | New, for the tutorial exit and the cold open | `Art/Models/Vehicles/Skiff.py`: packet paint (cream and teal) for the tutorial's skiff, and dark paint for the gang's skiff in the cold open. Skyreach keeps a timber jetty over its drop, with its bell post and slate |
| The train | New, one kit | `Art/Models/Vehicles/Train.py`: a small tank locomotive (about 9 m), a passenger car (about 12 m) and Tilly's hearse car (about 10 m; black and brass, with a coffin rack and black crepe). All three are built from shared parts (wheel sets, bogies, couplers, roofs, doors, windows and lamps) and share their materials: the house trim sheet for timber, window glass and painted trim, and the existing metal and paint sets, tinted. No new texture set, so the files stay small. LOD0 at most 12k triangles for the locomotive and 8k for each car, with three LODs down to under 1k for the view from Ransom's Point. Static at the platform; once it has steam up, its smoke is the existing chimney smoke |
| Rail track kit, signal and station board | New | `Art/Models/Props/Railway.py`: track straights and curves on one grid, a buffer stop, a semaphore signal and the station board prop, each with LODs. The track and the signal use the existing metal and wood sets; the board uses the house trim sheet. The track is instanced |
| Depot and water tower | New | `Art/Models/Buildings/Depot.py`: the depot (a timber station house with a platform canopy) and its low timber platform, and a water tower with its spout. On the house trim sheet, merged per material with LODs like the other buildings |
| Backdrop islands | New | A few cheap, unlit distant islands in the sky around Skyreach only. The horizon of Ransom's Rest is the ridges, the canyon and the silhouettes above |
| Wanted posters | New | Decals |
| The Unpaid | New rig and mesh | `Art/Models/Creatures/Unpaid.py`: spine, head, two arms and shroud-tail bones, posed by code like the spider. Three clothing tints; the coal's color comes from the rank (dull red for Basic) |
| Abel | New, on the Unpaid rig | Keeper's coat and hat, ghost lantern, spectral Ranchhand pump |
| Amos | New, on the Unpaid rig | Work clothes |
| Hob | New small rig | Body, head, wings and legs, flapped by code |
| Mister Sexton | New seated static mesh | The first human; concepts first |
| Cold open silhouettes | Existing UE mannequin, posed | Flat black against the sun |
| Gravemother and spiderlings | Existing spider mesh and rig | 1.8× with a pale-hide material instance; 0.45× |
| Cloud bank | New | A card for the tutorial skiff's ride into the cloud, the cold open's match cut, and the evening cloud in the west that the gang's skiff comes out of |
| Effects | New | Ghost fade and phase-step, the coal glow, spectral buckshot pellets, the shriek ring, Gravewind wisps, unlit wisp cards along the Rim, fog rising out of the canyon at the deck, a grave-dirt burst, frost on Abel's grave |
| Lighting | New data on the existing setup | Golden late afternoon and dusk, with the sun low in the west-southwest over the canyon |

## Tech needs

| Piece | Existing or new | What it does |
|---|---|---|
| Terrain scripts for any area | Generalized from the tutorial island's (about 3,400 lines) | A shared generator that takes a layout path, with an island setting and a new grounded one: feature lists by type, pits, mesas, gullies, cliff groups per feature, stacked cliff courses above 12 m, the playable boundary, the surround ring, the canyon wall and the backdrop. `build_area.py <Area>` replaces `build_tutorial_island.py`, and the scatter builder takes the area. The tutorial island is regenerated first: `layout_computed.json`, its meshes and its maps must come out identical, and its tour and screenshots must match. See *How the terrain generator changes* |
| Playable area | New: `APlayableArea` (World/) | Holds the playable boundary polygon from `layout.json`, with edges flagged open where a drop is part of play: the Rim's lip, which runs on along the Mooring Ledge's south edge over the Mooring Notch to about (−84, −101); the deck; and Mill Falls. Past the closed edges the ridges rise steeper than the walkable angle (about 45°) for at least 4 m, dressed with cliffs and outcrops, and invisible walls (box components about 50 m tall) stand behind them as a backstop. At Stage Gap's mouth the closed edge crosses the track behind the parked locomotive and the slide's banks. A new `PlayableBounds` collision profile in `DefaultEngine.ini` blocks only Pawn, so bullets, the camera, the minimap and PCG traces pass through. It answers `Contains()` and draws itself with `Looter.World.Bounds`, and `LooterWorld::StaticGeometryParams` skips it. Without one in the level (the tutorial island), nothing changes |
| Fall recovery on the ground | Changes `UFallRecoverySubsystem` | Drops inside the boundary are at most about 20 m, and nothing past the Rim has collision, so today's rule (more than 30 m below the last safe spot) would fire only deep in the canyon. With an `APlayableArea` in the level it samples safe spots only inside the polygon, recovers a player at once when they have left it over an open edge and dropped 5 m below their last safe spot (off the Rim, the Mooring Ledge, the deck or Mill Falls), keeps the 30 m rule as a backstop, and gets an `OnRecovered` event that feeds Hob's lines about falls and the boss's Gravewind phase. `KillZ` goes about 100 m below the canyon floor, so stray creatures and drops get cleaned up |
| Minimap bounds | Changes `UMinimapSubsystem` | Keeps its traced bake and today's tags. Only the core terrain tiles are tagged `Ground`, so the map covers the valley and the ridge feet, not the ring, the canyon or the backdrop. Nothing past the Rim has collision, so it traces as void, and today's coast lines draw the Rim with no new code. New when an `APlayableArea` is present: texels outside the polygon are drawn dimmed, the closed edges are drawn as a boundary line, and the height tint uses only heights inside the polygon (the ridges would otherwise squash it). The resolution becomes a member sized for about 1 m per texel: 256 stays on the tutorial island, and Ransom's Rest gets about 384 |
| Perf by tag | New: `Looter.Perf.HideTag <tag> 0\|1` | The build script tags everything past the boundary `Beyond` and each zone `Zone_<id>`. Hiding a tag lets tour and perf runs measure that group's draws and milliseconds by difference |
| Creature size | New, on `ACreatureBase` | Today `BeginPlay` resets every creature to scale 1, and spider leg IK, the capsule, attack range, steering and ledge probes and the health bar height are fixed centimetres. A `BodyScale` applied after the reset scales all of them. Tested by walking and attacking with a 0.45× and a 1.8× spider |
| Ranks | New, on the existing `ACreatureBase`, `ULootTable`, `ULootDropComponent` and `UCreatureHealthBarWidget` | `ECreatureRank`, a rank settings asset, four new rank tables (Basic stays `DA_LootTable_Default`, which `LootLibrary` and the DefaultTable test name), tag colors and words, coal colors. The creature's level is pushed into its `ULootDropComponent::Level` whenever it is set (new; today drops are level 1). Commands `Looter.SpawnCreature <kind> <rank> [count] [chase]` and `Looter.Loot.SimulateDrops`; test `Looter.Loot.RankOdds` |
| Level bands and XP | New | `UAreaDefinition` (first with just name, map, band and promotion chances): `DA_Area_RansomsRest`, and one for Skyreach marked as practice (0 XP, and ammo-only drops after the first cast-off by default). The level roll, linear health and damage, the kill XP formula with the falloff for low-level enemies, `Looter.XP.Table` to print it |
| Respawn rules | Changes `ACreatureBase` (today every creature respawns about 34 s after death) | Off for Legendary monsters, bosses and everything spawners and egg sacs create. A promoted creature comes back as Basic. Promotions rolled on arrival (coming in at the station, by train or back from Skyreach, or loading a session), with the 20-minute cooldown saved per map |
| Pack calls | Changes `ACreatureBase` (today they reach only the exact same class) | Matched by a pack tag, so the Gravemother and Epic spiders' calls reach every spider |
| Session save version 2 | Extends `ULooterSessionSave` (version 1 today) | World state per map (today one world), the campaign record (completed missions, active mission and step, opened areas, first boss defeats, the first cast-off from Skyreach, cold open seen), the arrival point (a station's platform, or Skyreach's jetty), and per-map promotion and Legendary-monster times. Each map keeps its own world, so practice trips to Skyreach and back leave both intact. Version 1 saves are upgraded inside the load, before anything can save them again, with their world filed under the tutorial map. Tested on a copy of a real version 1 save before merging |
| Travel | New; uses the save's existing `Map` field | `UAreaDefinition` gets its station and opening mission. Travel sets `Map` to the destination, clears the saved player spot, writes the arrival tag and saves, then opens the level with `?Session=N`. The save on level teardown is skipped for that trip (today it would write the old level's world over the new one), and autosave and save-soon pause during the first cast-off's ride, the train's shots, the fades and scenes. Arrival points are tagged by station: on the platform by the hearse car's door on Ransom's Rest, and at the jetty on Skyreach. A trip to Skyreach after the first cast-off is ordinary travel to Skyreach's jetty: a plain fade with no cutscene. The session picker shows area names (Skyreach, Ransom's Rest) instead of the map file's name. `Looter.Travel <area>` for testing |
| Leaving Skyreach | Extends `ATutorialDirector` and `USessionSubsystem` | The tutorial's done flag keeps meaning steps 1–6 and lowers the gangplank. Once the first cast-off is recorded, the gangplank is always down, and “Skip the tutorial” also sets the tutorial's done flag, so a practice visit can never strand a player who skipped. A new campaign flag records the first cast-off, which plays the match cut and the cold open; once it is set, the station board on Skyreach's jetty lists every opened area, and leaving is a plain fade to that area's station, with no ride. The first cast-off's confirm says the player can come back to practice any time. “Board the skiff” is its own `UMissionSubsystem` mission with a waypoint, shown whenever the tutorial is done and the player hasn't made the first cast-off, which covers skipping and older sessions. Skyreach's creatures give 0 XP, and on return visits they drop only ammo by default. “Skip the tutorial” stays in the new-session flow: it counts as the first cast-off and puts a Common Bullpup in the coffin. The jetty is a `layout.json` placement; `ASkiffJetty` (hold Interact at the gangplank to open the station board; on the first cast-off it shows one destination and the confirm, then the 12 s ride and the cloud whiteout), the bell and the slate are placed by `gameplay()` and checked by `Looter.World.TutorialIslandGameplay` |
| Station board | New: `UStationBoardWidget` (UI/World/) | A LooterUI panel that calls `MarkBackground`. It lists every opened area the Keeper's Lantern has found, plus one blank line naming the mission that opens the next, and “Skyreach (practice)” after the first cast-off. Choosing a line asks to confirm, then travels. Skyreach's jetty uses the same panel: on the first cast-off it shows one destination and its confirm, and after that every opened area. Only the first cast-off from Skyreach plays the match cut and the cold open |
| Stations and the train | New: `ATrainStation` (World/) | Every area has one, placed by `gameplay()` from `layout.json`; on Ransom's Rest it is the depot. Hold Interact at the station board to open it. The arrival point is on the platform by the hearse car's door. The train is static meshes with LODs, parked at the platform, and it moves only in two short shots (the default; a fade alone is the other choice, at step 9). On departure a fixed camera on the platform watches it pull out east through Stage Gap along a spline for about 4 s, then the screen fades. On arrival the screen fades in on it pulling into the platform, and control returns there when it stops. Both shots can be skipped. Its wheels turn by code, with nothing in Sequencer. Until Main 7 it is cold and shut; from Main 7 the locomotive has steam up and the hearse car's door stands open |
| Track and train collision | New, in the models' `UCX_` hulls and `ATrainStation` | The track has no per-rail collision: each piece's hull is one low box over its ties and rails with sloped sides, so a player steps across without snagging. The platform is about 40 cm high, under the character's step height, so it can be stepped onto anywhere. Each car's hull is one simple box, and the gaps between cars are closed, so nobody climbs onto the train, squeezes between cars or gets past the locomotive at the gap's mouth. The train, the depot and the water tower are tagged `Obstacle` for the minimap; the track is not. The track past the gap belongs to the surround and has no collision |
| Interaction | New, generalizing the weapon manager's loot focus | One component owns the Interact key, tap and hold, for loot, the skiff's gangplank on Skyreach, the station board, doors and windows, headboards, the bell, lantern posts, hay bales and posters. Loot keeps its tap and hold rules and tests |
| Missions as data | New, on the existing `UMissionSubsystem` (committed at 45260c4) | `UMissionDefinition` data assets list objectives; their types are C++: reach, interact or hold, talk at a speaker point, kill N by class, tag or zone, kill a named actor, collect, defend for a time, play a scene, board (the tutorial skiff). Rewards: an XP share, a gun with a rarity floor, a named gun, unlocks. A runner per level feeds titles, objectives and waypoints to `UMissionSubsystem` (the minimap arrow exists). A Missions page in the inventory (LooterUI). `Looter.Mission.Start/Complete <id>` |
| Captions and speaker points | New | Captions as floating outlined text with no panel, like the HUD. Speaker points on doors and windows. Lines as data (speaker, text, seconds). Hob's lines on events, never on Skyreach |
| Story characters | New: `AStoryCharacter` | Non-hostile, posed by code, carries a speaker point, shown or hidden by story state. Sexton, Hob, Amos and Abel after the fight. Abel the boss is a separate class (today `ACreatureBase` is hostile only) |
| The Ledger | Changes the existing bestiary | A story-character page type with no actor (and the Entries test updated for it), pages that show the area's band instead of class-default level and XP, a page field that moves Abel's page from Enemies to Friends, and the seven-names page. The UI stays Concept C |
| Spawners | New: `AEncounterSpawner` | Groups and waves switched by mission state, alive caps, rank rolls, boss adds, safe zones (the farm, and the town after Main 3). Nothing they spawn respawns |
| The Unpaid | New: `AUnpaidCreature`, a child of `ACreatureBase` | Floating, coal crit by shot-line distance, lunge, shriek, phase-step |
| Lighting states | New | Golden afternoon and dusk, on a sky made for the ground. The painted cloud dome grows from 1 km to past the last backdrop layer (it is translucent and depth-tested, so at 1 km it would paint clouds over the far ranges). SkyAtmosphere's planet top moves down to the plains, with a ground albedo in their colors. The height fog, tuned today to fill the void under the island (blue, max opacity 0.85), becomes warm for golden hour, with directional inscattering toward the sun, a start distance near 60 m, kept thin so the backdrop's layers fade by distance under the atmosphere's stretched haze rather than melting into one band. The sun sits low in the west-southwest, over the canyon. The backdrop's tint and the inscattering come from a material parameter collection. Dusk is the same sun rotated, with fog color changed and the sky light recaptured behind a fade. The sky's own color (the atmosphere's sky luminance factor) cancels most of the warm sun's tint, so the afternoon sky stays blue overhead and warms only toward the sun; dusk raises the ozone (3.5 times Earth's) so the sky away from the sun turns blue-violet, and the height fog ends before the sky and the cloud dome (cutoff 13.5 km). The painted clouds are lit from the sun's side, warm where they face it and blue-grey in shade. Volumetric clouds and fog stay off. `Looter.Light Day\|Dusk` |
| Scenes | New, in C++ | The cold open (a code camera path, silhouettes, captions), the grave wake-up camera, Sexton's scene, Abel's ending and the train's departure and arrival shots. All skippable (`Looter.Scene.Skip`), with no logic in Sequencer. Scenes stay off in tour and perf runs |
| Boss framework | New | Phases by health, untargetable states, add waves, the fog-wall seal, a boss bar (a slim slanted bar per the HUD rules), reset on the player's death. Built on a test spider first |
| Enemy projectiles | New | Today the Pawn profile ignores the Weapon channel, so a bullet fired the way guns fire would pass through the player. Enemy shots get their own channel (or a sphere sweep against the player's capsule) and a slow pellet visual (an emissive sphere, instanced). Tested: a pellet hurts the player and never the shooter's adds |
| Respawn graves | Extends `UPlayerVitalsSubsystem` | `ARespawnMarker`: wake at the nearest activated one |
| Named weapons | New, on `FWeaponInstanceData` | Fixed parts, a name and a flavor line. Heirloom now, unique legendaries later |
| Egg sacs | New, modeled on the existing `ATargetDummy` | Shootable; each lets out 2 spiders |
| Tour and perf | Changes `Looter.Tour` and `views.json` | An optional `exec` per view (for example `Looter.Light Dusk; Looter.SpawnCreature Spider Basic 12 chase`), run before the view is measured. New horizon and boundary views |
| Tests | New | Missions and objectives, the save upgrade, travel, the skiff mission and the skip, the station board's lines (opened areas, the blank line, “Skyreach (practice)” after the first cast-off), arrival on the platform, the practice round trip (no cutscene, 0 XP, ammo-only drops, both worlds kept), rank odds and rarity order, loot level, creature size, Unpaid hit zones, enemy pellets, boss phases and reset, story-character pages. `Looter.World.PlayableArea` (`Contains()`; a capsule sweep is stopped by a closed edge and not by an open one), `Looter.World.FallRecovery` (a player off an open edge is back on the last safe spot within a second; no safe spot is ever recorded outside), `Looter.World.Minimap.Bounds` (outside texels dimmed, the boundary drawn, the canyon traced as void), `Looter.World.Station` (a capsule walked across the track never snags, and none gets past the locomotive or between the cars). The generator's two fixtures |

### How the terrain generator changes

- **A grounded setting.** `layout.json` gets `"setting": "island"` or `"grounded"`. Grounded skips everything that assumed a floating rim: `_rim()`'s roll-over, `underside()` and its spires, `rimDrop` and `undersideDepth`, the waterfall's `dropTo` measured to the underside, `rim_points()` cliff dressing, the rim band in the macro map (`_rock`) and in the scatter mask (rim pebbles, a land mask cut from the outline), and `terrain_ao()`'s padding that treats everything past the rim as open sky (it pads with the real surround heights instead). Everything keyed to `edge` (distance inside the rim) switches to distance inside the playable boundary: `_relax`, quadtree clearance, orchard rows and waterline filtering. The island setting keeps today's code path untouched, so the tutorial island still regenerates identically.
- **One height field, three levels of detail.** A regional height field (large-scale noise, ridges as polylines with profiles, the Rim's escarpment drop and the canyon floor) is defined everywhere and sampled at three resolutions:
  1. the playable core, a square of about 400 m at 12.5–15 cm cells, with today's feature pipeline on top;
  2. a surround ring out to about 500–600 m from the centre, on a 1 m raster with 2–10 m vertex spacing and a full fallback;
  3. backdrop silhouettes at about 3, 6 and 12 km.

  The core's outer 25 m is a seam band that fades every core feature and the micro-noise into the regional field, so the core's edge heights match the ring's. The ring's inner loop reuses the core's edge vertices exactly (no T-junctions), and both meshes take the seam normals from the field's gradient, so there is no crack and no light crease. A feature that enters the seam band stops the generator with an error. The core's edge is the square on the ridge sides and the Rim's lip on the west.
- **The west edge.** Every edge beat of the approved layout keeps its place: Ransom's Point and Gravewind Point stand on the Rim, the gang moored on the Mooring Ledge under the Point, Mill Creek falls into Mill Gorge, and fog rises out of the canyon at the deck. Below the Rim's cliff kit, the canyon wall is generated as an open strip in 4 sectors that reuses `underside()`'s rim-wall code (`_zipper`, level-strata UVs, baked AO). The CliffFace kit goes only on the top course (12–20 m, stacked) and where players stand close (the lookout, the deck). The ring holds the canyon floor, with the river painted into its macro map, and the lower far wall. Past that, the plains and far mesas are silhouettes.
- **The map square is set per area.** `MAP_HALF`, the 2048 shape raster and every raster size tuned to 204.8 m (curvature 1024, AO 1024, scatter 512, macro 4096) become cell sizes in metres per layout, with caps. The tutorial still computes the same sizes, and a 400 m core keeps its detail. The core's macro map is 4096 px, about 9.8 cm/px against the tutorial's 5 cm. If step 5a's screenshots look soft it splits into 2×2 maps, one per tile quadrant, with no extra draws because the tiles are separate meshes already. The ring's 2048 px macro map (about 0.6 m/px) is painted from the same palette functions, and the core's blends into it across the seam band. `layout_computed.json` records the squares each map covers, and the build and scatter scripts read them instead of the hard-coded `HALF = 10240`. The core's 150k triangles are weighted toward the playable area, with a coarser tolerance past the boundary.
- **New feature types.** A scarp (a one-sided step of 2–6 m along a polyline), a ridge or spine (a raised line with cliffs on one or both sides), an outcrop knob (a small footprint that seats a freestanding rock), a pit (the Sink), a mesa (Coffin Rock), dry gullies (the Dry Wash), a creek that ends in a falls into a gorge, a rail bed (a strip graded flat along a polyline, for the track through Stage Gap), any number of plateaus with ramps, and stacked cliff courses above 12 m. Each feature writes its own cliff group to `layout_computed.json`. Outcrops, boulders, walls, tree lines and ruins don't change the ground level, so they are the only obstacles used inside an encounter's ground.
- **An open-ground metric.** For every walkable metre inside the boundary, the generator measures the distance to the nearest break: a building or wall line, a fence, an outcrop or boulder group, a tree stand, a cliff, or a 1.5 m rise within 6 m. Fences, low walls and knee-high cairns count as breaks: they are cover, even though a player sees over them. It writes the largest distance and the worst spots to `layout_computed.json`, and paints a red overlay on the plan preview. Target: nothing more than about 10 m from a break (empty circles of about 20 m across at most, as on the step 2b picture), outside ground marked open on purpose: the hayfields, Main Street, the crossroads at the keeper's grave, and the grave's line to the bell tower.
- **Scatter.** The ray lift and length come from the computed height range, because the ridge feet stand higher than anything on the tutorial island. Grass, flowers and pebbles stop 10 m past the boundary and stay off the rail bed. The ring gets one cheap layer of opaque low-poly far trees and rocks from its own mask, with no collision, no shadows and no masked leaves.
- **The build script.** Cliff pieces, outcrops, graves, fences and walls become instanced components, one per mesh, tagged `Obstacle` for the minimap and the scatter (today each cliff piece is its own actor). Track pieces are instanced the same way, untagged, since the player walks over them. Static building clusters are merged per material, with LODs. The script places the playable area, `KillZ`, and one `CullDistanceVolume` with a size-to-distance table read from `layout.json`. Everything past the boundary gets a `Beyond` tag and each zone a `Zone_<id>` tag.
- **Scripts and names.** The generator moves out of `Art/Levels/TutorialIsland` into a shared module that takes a layout path (its `Island` class becomes `Area`). `Art/Models/Terrain/TutorialIsland.py` and a new `RansomsRest.py` are thin wrappers around it. `build_tutorial_island.py` becomes `build_area.py <Area>`, with the level, mesh prefixes, spider zones and gameplay read from the layout. Meshes are `SM_<Area>_Tile_i_j` (tagged `Ground`), `_Ring_n`, `_CanyonWall_n`, `_Backdrop_n` and `_Water`. Two fixtures guard the generator: the tutorial island (island setting) and a small `TerrainTest` layout (grounded). Their computed outputs are checked in and compared after every generator change. `views.json` and `island_views.py` take the area, and `CODEMAP.md` lists every new file.

## Performance plan

**Target.** 8.3 ms (120 fps) at 1080p on the Medium preset on the RX 580, at every tour viewpoint: GPU at most 7.5 ms, game and render threads at most 7 ms each, about 700 draws. Lumen and Nanite run on High and Epic only, and every mesh draws its Nanite fallback on Medium.

**Where we start.** The tutorial island, built from the same kit, runs 151–201 fps on Medium. Its lookout sees the whole island for 4.2 ms of GPU, 422 draws and 0.74M triangles. Its worst view is the forest grove (5.5 ms GPU), and the cost there comes from masked leaves, not distance. Its game thread took about 4–5 ms with 13 mostly idle creatures, and 3.0 ms since distant creatures update less often (2026-10-01). Ransom's Rest is about 1.5× the area and adds a town, a chapel, about 50 graves, many more cliff pieces and the obstacles. Ransom's Point sees all of it, plus everything past the boundary: the ridges, the canyon and the horizon.

**Draw budget per zone, as seen from Ransom's Point:**

| Zone | Draws |
|---|---|
| Terrain tiles, cliffs and outcrops | 110 |
| Past the boundary | 30 |
| Farm and orchard | 100 |
| Main Street, the yard and the depot | 160 |
| Chapel, churchyard and boot hill | 90 |
| The Sink and the fields | 80 |
| Gravewind Point and the deck | 40 |
| Creatures, characters, effects and HUD | 90 |
| **Total** | **700** |

Everything past the boundary gets at most 30 draws and 0.6 ms of GPU at the heaviest view, inside the same totals. The 30 come from trimming terrain and cliffs from 120 to 110 (cliffs instanced), the farm from 110 to 100 and Main Street from 170 to 160.

**Rules.**
- Terrain: the core at most 150k triangles in about 20 tiles, with full Nanite fallbacks on walkable meshes.
- Past the boundary, a handful of merged meshes in sectors: the ring in 6–8 sectors (at most 40k triangles), the canyon wall in 4, and the backdrop in 3 layers × 4 (unlit, untextured, at most 6k triangles). Any one view draws 3–5 pieces of each. The ridges to the north, east and south hide the land behind them, so the ring there is only their outer slopes. On the west the canyon floor and far wall add about 4–6 draws plus their pixels to the sun-side view. Only the ring casts shadows, and only inside the 100 m shadow range.
- No masked foliage past the boundary. Ridge trees are opaque low-poly far trees (instanced, no collision, no shadow). Grass and flowers stop 10 m past the boundary, and the macro maps paint their color beyond that. Inside the boundary, trees get a last LOD card, because the orchard and town trees seen from Ransom's Point are the real overdraw risk.
- Buildings share the house trim sheet, and static building clusters are merged per material with LODs: World Partition HLODs show only for unloaded cells, so they do nothing in one unstreamed level. Headboards, crosses, fences, walls, web cards, outcrops and cliff pieces are instanced (today each cliff piece is its own actor), so the obstacles add instances, not draws.
- Cull distances follow object size through one `CullDistanceVolume` (for example: up to 0.5 m culled at 40 m, up to 2 m at 90 m, up to 6 m at 200 m, larger never), measured with Medium's own view-distance scaling.
- Shadows: Medium keeps two 1536 cascades over 100 m. From Ransom's Point the town is 120–180 m away and gets no dynamic shadows. Step 5a measures one far cascade, drawn only by buildings and cliffs (`bCastFarShadow`), against that and picks by screenshot and milliseconds. The tall ridges stay on the side away from the sun, because a 50 m ridge under a 15° sun throws a shadow about 190 m long.
- The train is static except in its two shots, and the depot is built like the town. The locomotive and both cars share their materials and have three LODs each, and the track is instanced. At the depot view the train, the depot, the platform, the water tower, the signal and the track take at most 15 draws; from Ransom's Point, about 240 m away, at most 8, inside Main Street's 160.
- Ghosts use a masked, dithered material with an emissive rim and coal, never translucency.
- Live creatures: at most 12 Unpaid at once, at most 10 adds plus Abel in the boss fight, and at most 16 creatures of any kind within 80 m. The existing rules cover what Ransom's Point can see: creatures beyond 60 m tick less often, spiders stop posing bones off screen, and health tags show only nearby. View distance doesn't change the game thread.
- No volumetric fog or clouds: height fog plus a few unlit wisp cards along the Rim. The fog stays thin (0.003) and the atmosphere's aerial perspective is stretched 3 times, so each backdrop layer fades more than the one before it (darker near, paler far) and the last is almost all haze; the cloud dome grows past the backdrop.
- Lanterns are emissive meshes, with at most 3 shadowless point lights in any view.
- Dusk is the same sun rotated, with the sky light recaptured behind a fade. It has its own viewpoints, a capped shadow distance and 2 cascades.
- The level loads behind the cloud whiteout on the first cast-off, and behind the fade on every other trip; its load time is measured.

**Order of work.**
1. Step 5a, on the greybox terrain before any buildings: everything tagged `Beyond`, measured by difference with `Looter.Perf.HideTag Beyond` at the heaviest view (at most 30 draws and 0.6 ms); shadows at Ransom's Point and on the deck, two cascades against one far cascade; and the render thread from a `perf.ps1 -GpuStats` capture, since the tour's Render column follows the frame.
2. Step 5b, before any art: each zone's draws from Ransom's Point against the table, by zone tag; 12–20 real spiders chasing the camera for the game thread; 12 dithered ghost stand-ins for the GPU.
3. A tour after every import. Ransom's Point, the orchard, the horizon views and the `Beyond` budget are re-measured after steps 17, 18 and 19.
4. Anything over budget is fixed before the next step.

**Measured on the test level** (`/Game/Maps/Dev/Lvl_TerrainTest`, 2026-10-05, Medium at 1080p, by the tour's
cost-by-difference views):
- Beyond (the ring, canyon wall and backdrop) costs 0.40 ms GPU and 33 draws looking west over the canyon, and 43
  draws from the air. The time fits the 0.6 ms budget, but the draws are over 30. On Ransom's Rest, try one backdrop mesh
  per layer instead of four sectors each first.
- The 116 cliff pieces cost 6-22 draws and up to 0.2 ms GPU: Unreal's dynamic instancing already merges identical
  pieces into one draw. Instancing them by hand would save little. It would also cost the scatter its per-obstacle boxes,
  since the scatter keeps grass and trees out of each actor tagged `Obstacle` by its bounds. So cliff pieces stay actors
  unless Ransom's Rest's own tour says otherwise.

**Measured on Ransom's Rest** (`/Game/Maps/Lvl_RansomsRest`, step 5a's level before the greybox, 2026-10-06, Medium at
1080p, after the boundary rework):
- All 22 tour viewpoints hold the budget: 4.1-6.0 ms (168-244 fps). The heaviest are the chapel over town with the far
  cascade (6.0 ms, 507 draws), the aerial (5.8 ms, 434 draws) and the chapel over town (5.7 ms). `perf.ps1` from the
  grave (before the rework): 5.5 ms, p95 6.0, 489 draws, 0.97M triangles, a 1.05 s load.
- The render thread is the limit, not the GPU: render 4.1-5.9 ms against GPU 3.3-4.9 ms. A `-GpuStats` capture puts 1.7-
  3.3 ms of the render thread in its visibility wait.
- Beyond, now one backdrop mesh per layer: 13-20 draws and at most 0.22 ms (13 west from Ransom's Point, 16 from the
  deck, 17 at the ridge foot, 20 from the air), inside its 30 draws and 0.6 ms. Four sectors per layer had cost 23-38.
- Shadows: one far cascade to 400 m, drawn by buildings and cliffs (`Looter.Perf.FarShadow 1 400`), costs 0.95 ms at
  Ransom's Point over the valley and 0.3 ms over town (GPU +0.7 and +0.25, 45-58 more draws), and the town beyond 100 m
  hardly changes (`Saved/Screenshots/Review/RR_FarShadow.jpg`). The recommendation is to keep today's two 100 m cascades
  on Medium.
- Dusk costs about what the day does, -0.2 to +0.5 ms by view (its shadows reach 60 m): the grave 5.6 ms against 5.2,
  Ransom's Point west 4.8 against 4.3, Main Street 5.1 against 5.3.
- The boundary: steep rock (50.8-65.7 degrees, starting 0.0-2.1 m out) rises past every closed edge but Stage Gap's
  mouth, and the walls stand 2.5 m behind the line at its foot (`level.wallSetback`). Walking into all 25 closed edges
  (55 runs), every run stops at the wall, at most 2.16 m past the line, and none gets past it or falls. Only Stage Gap's
  mouth, which waits for the train, has walkable ground past the line.
- In play: stepping off the Gravewind deck is recovered (5 m down, edge 19), and the minimap covers the valley.
- Twelve Unpaid chasing at the town gate (step 16), the player looking down Main Street:
  - 5.9 ms (169 fps), p95 6.3, game thread 4.3, GPU 4.9, 672 draws.
  - With one Unpaid there: 5.5 ms, game 2.8, GPU 4.5, 465 draws.
  - So each Unpaid costs about 0.14 ms of game thread, 0.04 ms of GPU and 19 draws.
  - Measured with `perf.ps1 -Exec "Looter.Quality Medium,Looter.Perf.Horde Unpaid 12 Basic 0 -1400 90"`, which puts the
    player at the gate, unhurtable, with the fight coming at them. A tour view can't measure a fight: the tour looks
    through a camera of its own while the player stays at the spawn, and creatures far from the player slow down.

**Tour viewpoints** (`Art/Levels/RansomsRest/views.json`, with `exec` where a view needs setup):
1. The grave.
2. The farm porch toward town.
3. Ransom's Point over the valley (expected heaviest).
4. Ransom's Point west over Gravewind Canyon, into the low sun.
5. Ransom's Point at dusk (`Looter.Light Dusk`).
6. The town gate, looking down Main Street, with 12 Unpaid chasing (a fight: measured with `Looter.Perf.Horde`, above).
7. The depot from the undertaker's yard, with the train at the platform and Stage Gap behind it.
8. The chapel yard.
9. Inside the chapel.
10. The Sink floor.
11. The Gravemother's den.
12. Whitlock Fields.
13. Gravewind Point at dusk, with Abel and 10 adds.
14. The north boundary, looking out over Larkspur Ridge.
15. The ridge foot, where the core meets the ring.

## Scope cuts, in order

These are the defaults for the first pass of Ransom's Rest. Later items are cut only if needed.
1. **Talking doors.** Delia, Tilly and Aldana speak through doors and windows, and only Sexton and Hob get models in this area. Full townsfolk models come after Sexton's concept sets the human look, and only if you want them.
2. **Captions only**, with no voice acting.
3. **A cheap cold open:** a code camera, captions, sound, a muzzle flash, flat-black posed mannequins, the gang's skiff in dark paint, and Sexton's seated model on the far rail. No costumes and no Sequencer logic.
4. **Then, if needed:** Hob's perching (a plain bird on fixed perches, or none), the Gravebound Unpaid's slow, the Ranger caches, the dusk fade (the boss at golden hour), the train's moving shots (a fade alone) and the Gravemother side mission.

Concept renders come before any human costume. Sexton is first, because he sets the bar for every human after him.

## Risks

- **The first human.** Sexton sets the look of every human after him (the gang, the townsfolk, the *Lily*'s card sharps), and you rejected faceted low-poly. Mitigation: concepts first, a seated pose, the face in shadow, no facial animation.
- **Scope.** Missions, save version 2, travel, ranks, spawners, a ghost enemy, captions, a boss framework, scenes and a dusk state. Each is its own step, and the greybox makes the chapter playable before the art.
- **A father as the first boss** is heavy for hour one. Hob's humor and the grief mechanic carry it, and his difficulty is tuned at step 22.
- **The game thread.** The tutorial's game thread was 4–5 ms with 13 idle creatures before distant creatures updated less often. The fights on Ransom's Rest must be measured with real, chasing creatures at step 5b, not just pictures.
- **Ghosts on Medium.** Masked, dithered ghosts have to look good, and 12 at once plus fog must stay in budget. They are measured at step 5b with stand-ins and at step 16 for real.
- **Steering without a navmesh.** The Unpaid float and phase-step, every group lives on one level of ground, and the boss deck is flat. A navmesh comes forward only if the step 5b checks fail.
- **Obstacles can split encounters.** Creatures don't step off drops of more than about 4 m and have no navmesh, so a scarp or a knob inside an encounter's ground would split it. Inside encounter grounds only obstacles that leave the ground level alone are used (outcrops, walls, boulders), and every change of level is outside all fights. Checked at step 5b.
- **The Rim may read as an island's edge.** The canyon's floor and far wall, the plains running on to the sunset and the ridges on the other three sides show solid ground all round. The step 2b picture and the step 4 horizon views check it.
- **The seam** where the core meets the ring could show a crack, a light crease or a color step at the ridge foot. Shared vertices, normals taken from one field and a macro blend across the seam band prevent it. Step 3c prints the gap and the normal difference, and steps 4 and 5a photograph the ridge foot.
- **Long views and masked leaves.** The tutorial island's worst view is its forest grove (5.5 ms GPU), and the cost comes from leaves, not distance. Ransom's Point sees the orchard, the town's trees and the ridges, so nothing past the boundary has masked leaves and trees inside get a last LOD card. Measured at steps 5a and 5b.
- **No dynamic shadows past 100 m on Medium.** From Ransom's Point the town is 120–180 m away and may look flat. A far cascade for buildings and cliffs costs extra shadow draws; step 5a decides with screenshots and milliseconds.
- **A low golden sun makes long shadows:** a 50 m ridge under a 15° sun throws one about 190 m long. The ridges stay on the side away from the sun, and the step 4 renders check the bluff's own shadow over the farm.
- **Invisible walls can feel cheap,** and players may try to jump up the ridges. The boundary stands at the foot of rock steeper than the walkable angle, dressed with cliffs and outcrops, so the walls are only a backstop. Step 5a walks the whole boundary.
- **Fall recovery could fire by mistake.** A boundary drawn too tight along the Rim could pull back players who are only standing at the edge. Safe spots are recorded only inside, the outside rule needs a 5 m drop, and the boundary sits a step past the walkable edge. Tested at step 3d.
- **Generator cost.** The 400 m core has about 2.4× the tutorial island's raster cells, so step 3a measures memory and build time (15 cm cells are the fallback). The single 4096 px macro map has about half the tutorial island's texel density; if step 5a looks soft, it becomes four 4096 maps (2×2).
- **The terrain generator.** Generalizing the scripts can break the tutorial island. The island setting keeps its own code path, and its identity check runs at steps 3a, 3b and 3c.
- **The backdrop at dusk.** Unlit silhouettes don't react to the sun, so a dusk state that forgets their tint shows bright ridges at sunset. They take their color from the light state, and step 13 checks it.
- **The render thread at 700 draws.** The tour's Render column follows the frame time, so the render thread's own cost comes from `perf.ps1 -GpuStats` captures at steps 5a and 5b. Merging and instancing are the main levers if it runs over.
- **The save upgrade.** Today's three sessions live on the tutorial island. Version 2 must carry them over, tutorial done or not, without losing guns. It is tested on a copy of a real save first.
- **Travel and saving.** Today a level change saves the old level's world into the session, and the next level would restore it in the wrong place. Travel is built and tested (step 8) before the tutorial skiff's ride and the station board use it (step 10). Practice trips make round trips common, so each map keeps its own world, and step 8 tests a trip to Skyreach and back.
- **The track is a way out.** The line leaves the valley through Stage Gap, so a player may try to walk out along it. The parked train fills the cut at the gap's mouth, the slide's banks close in on both sides of the locomotive, and the boundary's wall stands behind them as a backstop. It is checked on the step 5b greybox, with a grey-box train in the cut, and by `Looter.World.Station`.
- **Snagging on rails and the train.** Rails, ties and the gaps between cars catch a walking capsule. The track has no per-rail collision, only one low, sloped box per piece; the platform is under the step height; and each car is one box, with the gaps between cars closed.
- **Skyreach as a farm.** A reachable practice island could be farmed for XP and guns. Its creatures give 0 XP and, by default, drop only ammo on return visits.
- **The tutorial itself.** Its steps 2–6 have never been played end to end in a live run. Step 10 starts by doing that.
- **XP pacing.** With your level 1 numbers (100 XP for the first level, 10 XP a kill), about 80 main-path kills plus missions put a player near level 9 at the end. Spawn counts and mission shares are tuned at step 27.
- **Legendary feel.** At 0.3% per basic kill, most legendaries come from ranked enemies and bosses, so the 8% and 2% promotions have to show up often enough.
- **Repeating the tutorial.** The farm, spiders and slimes are reused. The palette, Main Street, the chapel and graves, the Unpaid, the ranks and the boss set Ransom's Rest apart, and there's no gun rack beat and no slime-shooting mission.
- **Talking doors** read as cheap if they last beyond Ransom's Rest. The *Lily* needs human NPCs and human enemies anyway, so plan that pipeline while Ransom's Rest is built.
- **Thin gun variety.** Two gun bases are thin for an hour of loot. Plan a revolver base before the *Lily*; Dead Man's Hand needs a pistol anyway.
- **Story consistency.** The stories branch still says Skyreach. `Docs/Story.md` becomes the single canon at step 1, and this document becomes `Docs/Areas/RansomsRest.md` beside it.
- **Story wording on a grounded world.** `Docs/Story.md` was reworded for a grounded world; it names Teropa, and Skyreach is the only island, outside the story and never explained.

## Decisions for you, by step

- **Step 1 (decided on 2026-10-01):**
  - The place is **Ransom's Rest**, and so is its town; locals call it “the Rest”. The town's street is Main Street. The level is `/Game/Maps/Lvl_RansomsRest`, the layout folder `Art/Levels/RansomsRest` and the area asset `DA_Area_RansomsRest`.
  - Sexton's deal comes early, about 15 minutes in, on the lookout where Ellis died.
  - The first boss is Abel Ransom, Ellis's father. The Gravemother stays the Legendary-monster side mission.
  - The tutorial exit is the cloud match cut: the player's skiff sails into a cloud, and it parts on the gang's skiff arriving at Ransom's Rest seven days ago, with the player as Ellis. The story never names Skyreach.
  - Skyreach stays reachable as a practice island, outside the story (see *The exit and the unlock*). New sessions still offer “Skip the tutorial”.
  - The tutorial's guns come along as the guns Ellis was buried with. After “Skip the tutorial”, Ellis starts with a Common Bullpup in the coffin.
- **Step 2 (decided on 2026-10-01, with your changes):**
  - Ransom's Rest is grounded on Teropa, the earth-like planet where the story takes place, not an island like the tutorial island.
  - The layout stands as drawn: the size (about 1.5× the tutorial island), the zones, the route and the bluff path.
  - Cliffs and obstacles are to be added to break up the open ground.
- **Step 2b and the story proposals (decided on 2026-10-01):**
  - The revised map picture showed:
    - The ground: a high valley in the Reaches, closed by ridges on the north, east and south, that ends on the west at the Rim, a 70 m drop into Gravewind Canyon. The beats that hung over the island's rim stand on the Rim and in the ridges.
    - The obstacle pass: the cliffs and obstacles listed under *Size and shape*. Outside the zones they leave no gap wider than about 19 m, except the crossroads at the keeper's grave.
    - The boundary and what lies past each side.
  - **Approved: the valley ending at the Rim.** This document is written to it.
  - **Travel between areas is by train**, not by skiff: *“I think fast travel between regions should be via train.”* Every area has a station with a station board. On Ransom's Rest the line comes up from the wooded valley east of the East Ridge, through a rail cut in Stage Gap, to the depot at the east end of town by the undertaker's yard. The exit in Stage Gap became the depot, its platform, a water tower and the track through the gap, in the same places, and the rockslide over the old stage road stays. Skiffs stay only for the tutorial's skiff and jetty on Skyreach, outside the story, and for the gang's skiff in the cold open. The cloud match cut is unchanged.
  - **Approved:** the dead go west down the Sundown Road to the Toll Gate.
  - **Approved:** bodies go back to their graves at dawn, after the night on the burial board.
  - **Approved:** keep the Reaches and the Rim Rangers.
  - **Deferred until Ransom's Rest is complete:** whether the *Gilded Lily* stays an airship. Ransom's Rest is complete
    (2026-10-08); the *Lily* is on hold while you polish other things, the question still open.
  - **Build Ransom's Rest with several agents, to a professional standard:** buildings, obstacles, the environment and every asset working properly; interesting and interactive; fully optimized, with small files and good graphics; and fleshed out. Building starts at step 3.
- **Still open, a small choice:** the lookout tower stays at (−100, −86), where the bluff's edge hides it from the grave, unless you want it moved to the top's north-west corner, at about (−106, −66), where the grave sees it.
- **Step 5a (approved on 2026-10-07, as recommended):** today's two 100 m cascades stay; the far cascade cost 0.2–0.4 ms GPU at the long views for nothing you could see. The horizon and the boundary in the game (with art notes 2 and 3: the blue sky, the blue-violet dusk and the layered far band) and the timings (Medium, the heaviest view 6.5 ms of the 8.3 ms budget) were approved with it.
- **Step 6:**
  - The legendary odds per rank: 0.3%, 2.2%, 11.8%, 21.4% and 30.3%.
  - Ranked enemies dropping guns more often than your 20–40% rule (Rare 60%; Epic and up on every kill). Basic keeps 30%.
  - The rank words and colors (Restless blue, Gravebound purple, Soulfed orange), and ranked creatures being a little bigger.
- **Step 7:**
  - Enemy levels following the player inside each band, or fixed per area.
  - Kill XP growing 8% a level (recommended). The alternative, 12%, keeps 10 kills per level forever and runs players past every band.
  - Promotions of 8% and 2% on Ransom's Rest, with Legendaries hand-placed.
  - +8% max health per level as the first level-up reward.
- **Step 8:** the 20-minute cooldown on rerolled promotions and the Legendary monster.
- **Step 9 (decided on 2026-10-02):**
  - The train's look (the locomotive, the passenger car and Tilly's hearse car), the depot's and the water tower's: approved from the look sheet, with **locomotive B** (the balloon stack, bell and cowcatcher). The ruins, the lookout with its cliff stairs, the burial deck and the Keeper's Lantern were approved the same day.
  - The tutorial skiff: **option A**, the cigar gas-bag in a rope net, in the packet paint and the gang's dark paint, with Skyreach's jetty, bell post and slate. Skyreach's backdrop islands: approved as an addition, then again after a polish pass for more interest.
  - Ahead of step 17, Main Street's look was approved from its look sheet the same day: the four false fronts, the boardwalk and awning kit, the shutters, the notice board and the town memorial.
  - Ahead of step 18 (on 2026-10-05), the Chapel of Saint Ada: shown half-timbered (plaster and oak) and as **white clapboard with a louvered belfry and a tall steeple**; the clapboard one was chosen. The graves kit (Ellis's grave, Abel's frosted grave, the keeper's grave, headboards, crosses, fences, coffins, wreath and crepe) was shown with it.
  - How the train arrives and departs: **a short shot, then a fade** (the default).
- **Step 10:** the first cast-off's ride and its length (12 s); practice trips as plain fades with no cutscene (the default); and whether Skyreach's creatures keep dropping only ammo on return visits (the default).
- **Step 12:** talking doors and captions only for this area.
- **Step 14:** Hob's look. Chosen on 2026-10-06 from three concepts: **the Revenant crow**, lean and brooding, with a dull ember deep in the empty socket and wingtips and tail feathers fading to ash (masked and dithered, like the Unpaid). His game model was approved from its look sheet the same day (“Hob's sheet looks good”): `SK_Hob`, 4,416 triangles, 24 bones, perched with wings folded, snapping onto `SOCKET_Perch_*`.
- **Step 15:** Sexton's look. It sets every human after him. Chosen on 2026-10-06 from three concepts: **the Gentleman**, very tall and gaunt in a black frock coat buttoned to the throat, an exaggerated stovepipe, coat tails over the rail, legs crossed, long pale fingers on the ledger and a dip pen; his face in shadow but for the point of a pale chin, and a tarnished coin weighting the ledger's ribbon as the only hint of the ferryman.
- **Step 16:** the Unpaid's look. Chosen on 2026-10-06 from three concepts: **the clothes they died in** (a homesteader in hat and vest, the coal burning through his chest, fading into shroud strips), made gaunter, with the hungry dead's face and lunge; the render of that mix was confirmed the same day, and the rig and mesh are built from it.
- **Step 17:** Ellis's wanted poster. Chosen on 2026-10-06 from three: **B, a woodcut of a masked rider**, so the face
  stays hidden and Ellis's gender open, with ALREADY written under DEAD OR ALIVE. The posters are decals (one atlas: the
  poster, its torn remnant, Calder's note and a scrap).
- **Steps 14, 15 and 17 (decided on 2026-10-07, as recommended):** the first-draft lines for Mains 1 to 3 stay as written, and the Ledger shows on Skyreach too once Sexton has handed it over: it's the book Ellis carries.
- **Step 18:** using the backlog Reliquary design for the smashed one.
- **Step 21 (approved on 2026-10-05):** the boss bar and the test fight.
- **Step 22:** Abel's look, chosen on 2026-10-06 from three concepts: **A, the Sunday keeper**. Still to come: his
  difficulty and the ending's wording.
- **Step 23 (decided on 2026-10-07, as recommended):** Heirloom is a named Epic with fixed parts and the line *Hold the door.*, not a Legendary; its special effect waits for unique legendaries. Its parts are the user's pick of the art session's candidates (B, 2026-10-07): the Ranchhand's Heritage body, Trap barrel (56 cm, Abel's length), Crown muzzle, Tube6 magazine, Flip sight, Field stock and Walnut pump; SM_AbelPump is remodeled to match, so the gun Delia hands over is the one Abel fought with.
- **Step 26:** bringing the Supply Crate and Strongbox out of the backlog.
- **Step 27:** whether the *Gilded Lily* stays an airship (deferred on 2026-10-01 until Ransom's Rest is complete; on
  2026-10-08, with Ransom's Rest approved, the next level was put on hold while you polish other things).
- **When the *Lily* starts:** a Hollow's signature legendary guaranteed on the first defeat, or always 15%.

## Build steps

Each step ends with something you can look at or play. Each one names the console command that reaches what it shows. Building starts at step 3: the main session builds Ransom's Rest with several agents, as you asked on 2026-10-01, so steps that don't depend on each other run side by side, and a step that builds on another starts only after you approve that one. The art session can draw concepts (steps 9, 14, 15, 16) ahead of time; you see them at their step.

**1. The revised story** (done).
- You get: the Revenant document as `Docs/Story.md` on main, with the one-screen “what changed” first, and this document as `Docs/Areas/RansomsRest.md`. `CLAUDE.md` points to them, and the stories README's Skyreach rule is retired for Revenant. No code.
- Decided on 2026-10-01: the six story questions, with the answers listed under *Decisions for you, by step*. Later that day: four of the six story proposals approved, travel by train, and the *Gilded Lily*'s question deferred (also listed there).

**2. The Ransom's Rest map** (done; approved on 2026-10-01, with changes).
- You got: one phone-sized picture drawn to scale beside the tutorial island's outline. It showed the zones, the main route numbered, the side missions, the boss deck, the lookout, the bluff path with its grade, the respawn graves and the exit in Stage Gap, with an inset of Skyreach's jetty, where the first cast-off leaves and practice trips arrive. No code.
- Approved with your changes: Ransom's Rest is grounded on Teropa instead of being an island, and more cliffs and obstacles break up the open ground. The layout stands.

**2b. The revised map** (done; approved on 2026-10-01).
- You got: the step 2 map redrawn on Teropa as one phone picture, showing:
  1. the playable boundary;
  2. what lies past each side: the Rim and Gravewind Canyon on the west, with the plains beyond; Larkspur Ridge and the Old Quarry on the north; the East Ridge and Stage Gap on the east; the Hogback and Mill Gorge on the south;
  3. the edge beats on the ground: the deck over the canyon, Mill Falls into Mill Gorge, the gang's mooring on the Mooring Ledge under Ransom's Point, and the exit in Stage Gap;
  4. the cliffs and obstacles, numbered;
  5. the open ground before and after;
  6. a sketch of the horizon from Ransom's Point.
- Approved on 2026-10-01: the valley ending at the Rim. The same day you chose travel by train, so the exit in Stage Gap is now the depot, its platform, a water tower and the track through the gap, in the same places. One small choice on the picture is still open: the lookout tower stays at (−100, −86), where the bluff's edge hides it from the grave, or moves to the top's north-west corner, at about (−106, −66), where the grave sees it.

**3a. Terrain scripts for any area, with the tutorial island unchanged** (built).
- You get: the generator in a shared module that takes a layout path; the map square and cell sizes set per layout; features as lists by type, with cliff groups per feature; and the covered squares written to `layout_computed.json` and read by `build_area.py <Area>` and the scatter builder (no more hard-coded `10240`).
- Check: the tutorial island regenerated, with a compare script printing each result. `layout_computed.json` must match to 0.1 cm; the tile, underside and water meshes must have identical vertex and triangle counts and hashes; the macro and scatter PNGs must be pixel-identical. Then the level is rebuilt: `Tools\tour.ps1 -Quality Medium` within 3% of the last run at every view, with screenshots side by side, and `Tools\runtests.ps1` green. Also the generator's peak memory and build time on a dry run at the square size of Ransom's Rest.
- You approve: the tutorial island looks and runs the same.

**3b. New feature types, on a test layout** (built).
- You get: `Art/Levels/TerrainTest/layout.json`, a 150 m square with a pit and its ramp, a 20 m cliff in two stacked courses, a scarp, a ridge, two outcrop knobs, two plateaus with ramps, a mesa, a dry gully, a creek that ends in a falls into a gorge, and a rail bed through a saddle. Also the open-ground metric with its plan overlay, and a layout checker (ramp grades and widths, every cliff course at most 12 m, features clear of the seam band).
- Check: the checker passes, and the tutorial island's identity check still passes.
- You approve: Blender clay renders of each feature, and the plan with its overlay.

**3c. Grounded terrain, on the test layout** (built).
- You get: the grounded setting on `TerrainTest`: no rim or underside; the regional height field and the seam band; the surround ring in sectors, with collision on its ridge sectors; an escarpment edge with its generated wall and a canyon floor below; three backdrop silhouette layers; the ring's macro map; and the boundary polygon with its open edges in `layout_computed.json`.
- Check (printed by the script): a seam gap of 0 mm and a normal difference under 1°; triangles per piece within budget (core 150k, ring 40k, canyon wall 20k, backdrop 6k); the tutorial island's identity check still passes.
- You approve: renders from inside the boundary toward each side, from the highest point, and along the escarpment edge.

**3d. Bounds, fall recovery and the minimap in the game, on a test level** (built).
- You get: `APlayableArea` with its walls and the `PlayableBounds` profile; the fall recovery and minimap changes; the `Looter.World.Bounds` and `Looter.Perf.HideTag` commands; `build_area.py` placing the ring, canyon wall, backdrop, playable area, `KillZ` and a cull distance volume; and `/Game/Maps/Dev/Lvl_TerrainTest` built from the test layout. New tests: `Looter.World.PlayableArea`, `Looter.World.FallRecovery` and `Looter.World.Minimap.Bounds` (see *Tech needs*). The tutorial island's tour is rerun and unchanged.
- You approve: a short play of walking into the boundary, jumping off the edge and reading the minimap, or screenshots of it if you'd rather.

**4. Ransom's Rest greybox, pictures (no Unreal)** (built).
- You get: `Art/Levels/RansomsRest/layout.json`, grounded, with the approved boundary, edges and obstacles, plus Blender renders:
  1. the plan with zones, boundary and the open-ground overlay, against the target of nothing more than about 10 m from a break (fences and low walls count), outside ground marked open on purpose;
  2. clay views of the bluff path, the Sink ramp and Main Street;
  3. two horizon views: Ransom's Point over the valley toward the ridges, and the Gravewind deck looking west over the canyon into the low sun;
  4. the ridge foot where the core meets the ring, at the planned sun angle, and the bluff's shadow over the farm.

  Printed numbers: triangles per piece, ramp grades, openness.
- You approve: the shape, heights, sightlines, horizon and obstacles.

**5a. Ransom's Rest in the game: terrain, bounds and sky** (done; approved on 2026-10-07).
- You get: `Lvl_RansomsRest` with the terrain tiles, ring, canyon wall and backdrop; cliffs and outcrops instanced; the playable area; the golden afternoon (a low sun in the west-southwest over the canyon, warm fog that fully hides the backdrop's ends, the cloud dome grown past the backdrop); `views.json` with per-view `exec`; and a Medium tour baseline that includes the new views (Ransom's Point west over the canyon into the low sun, the north boundary looking out, the ridge foot).
- Measured: (1) everything tagged `Beyond` stays at or under 30 draws and 0.6 ms GPU at the heaviest view, by difference with `Looter.Perf.HideTag Beyond`; (2) shadows at Ransom's Point and on the deck, today's two 100 m cascades against one far cascade drawn by buildings and cliffs, with screenshots and milliseconds side by side; (3) the render thread, from a `perf.ps1 -GpuStats` capture.
- Checks: a walk along the whole boundary finds no climbable gap (Stage Gap's cut waits for the grey-box train at step 5b), a jump off the deck is recovered, and the minimap reads correctly.
- You approve: the horizon and the boundary in the game, the shadow choice, and the timings.

**5b. Ransom's Rest greybox: buildings, load and encounters** (overtaken by the real buildings and train; its remaining checks, Stage Gap past the parked train and the zones' draws, fold into step 27).
- You get: grey boxes for the new buildings (the depot and the water tower among them), a grey-box train parked at the platform with the track through Stage Gap, and existing models where they fit; a walk through Stage Gap that finds no way past the parked train; zone tags, with each zone's draws measured from Ransom's Point against the zone table; 12–20 real spiders chasing the camera (game thread); 12 dithered ghost stand-ins (GPU); each encounter group spawned with `Looter.SpawnCreature` and chased on its own ground; and every obstacle inside an encounter's ground checked (no group split by a change of level, no creature stuck on a rock).
- You approve: scale, route, sightlines and timings, after walking it or reading the screenshots.

**6. Ranks and legendary odds** (built).
- You get: ranks on creatures, rank tags and words, four new rank loot tables (Basic stays today's table), creature size, dropped guns taking the creature's level, `Looter.SpawnCreature <kind> <rank>`, `Looter.Loot.SimulateDrops` and the `Looter.Loot.RankOdds` test. Shown on spiders from the console only, on the tutorial island and the greybox; nothing is placed.
- You approve: blue, purple and orange spiders seen in the game, their sizes, and the simulated drop table.

**7. Level bands and XP** (built).
- You get: the first `UAreaDefinition` assets (`DA_Area_RansomsRest` with the area's band and promotion chances, and Skyreach's, marked as practice at 0 XP), the level roll, linear health and damage, kill XP with the falloff, and promotions rolled on load. `Looter.XP.Table` prints kills per level from 1 to 70.
- You approve: the numbers, and a level 8 spider against a level 8 gun.

**8. Sessions that travel** (built).
- You get: session save version 2 and the upgrade from version 1, tested on a copy of a real save first; travel that saves the right world (`Looter.Travel <area>`); arrival points (on a station's platform, or Skyreach's jetty); saved promotion and Legendary-monster times; area names in the session picker; a round trip to Skyreach and back that keeps each map's world.
- You approve: travel to the Ransom's Rest greybox from the console, Save & Quit, Continue there, a trip to Skyreach and back, and an old session that still loads on the tutorial island with its guns.

**9. Train, depot and skiff concepts** (built: the train, depot and skiff are in the game; your approval of the train waits in the bundled review).
- You get: Blender preview pictures from the art session of the train (two locomotive options, the passenger car and Tilly's hearse car, all from one parts kit with shared materials), the rail track kit (straights, curves and a buffer stop), the depot with its platform, the water tower, the signal and the station board; the tutorial's packet skiff (two options) in its packet paint and the gang's dark paint; Skyreach's jetty with its bell post and slate; and Skyreach's backdrop islands. Each model shows its LODs and triangle counts, and a few frames show the train's departure shot through Stage Gap. The horizon of Ransom's Rest comes from steps 3c and 5a. Nothing imported.
- You approve: the train, the depot and the skiff, and how the train arrives and departs (a short shot and a fade, or a fade alone).

**10. Leaving Skyreach, the station board and practice trips** (built).
- You get: first, the tutorial's six steps played end to end and fixed if needed. Then the interaction component (loot moved onto it), the jetty and skiff on the tutorial plateau, the “Board the skiff” mission, the gangplank and bell, the station board at the jetty with the first cast-off's confirm (*“You can come back to practice any time.”*), the 12 s ride into the cloud, Skyreach's backdrop islands, the title card, and waking at the family plot's grave on the Ransom's Rest greybox. Then the station board at the grey-box depot: “Skyreach (practice)” and the blank line naming the next mission, a plain fade with no cutscene to Skyreach's jetty, and the jetty's board listing every opened area, which returns the player to the depot's platform the same way. Also “Skip the tutorial” with a Common Bullpup in the coffin, and Skyreach's creatures at 0 XP with ammo-only drops on return visits. Tests and a plateau tour.
- You approve: the feel of the exit, after finishing the tutorial, casting off and waking in the grave; the skip; the station board; and a practice trip to Skyreach and back.

**11. Missions as data** (built).
- You get: mission data assets and the runner, a Missions page in the inventory, `Looter.Mission.Start/Complete`, and “Board the skiff” moved onto it as the proof.
- You approve: the Missions page and the tracker.

**12. Captions, doors and graves** (built).
- You get: captions, speaker points on doors and windows, the story-character actor, and respawn graves, shown on the greybox with a test line at Delia's door.
- You approve: how captions look, and waking at a grave.

**13. Day and dusk** (built).
- You get: the afternoon and dusk lighting states and `Looter.Light Day|Dusk`. Both also set the fog's directional inscattering and the backdrop's tint, through the material parameter collection. Dusk is measured on the deck and at Ransom's Point looking west over the canyon, where the low sun is in view, and the backdrop is checked for bright ridges at sunset.
- You approve: the dusk look and its timings.

**14. Seven Days (Main 1)** (built).
- You get: the cold open with the cloud match cut (played on the first cast-off only): the gang's skiff glides out of the evening cloud in the west, low over the plains, and drops into Gravewind Canyon toward the Mooring Ledge under Ransom's Point. Then the grave wake-up, Delia's door and Main 1 on the greybox with a placeholder bird. Hob concepts from the art session.
- You approve: Main 1's flow and tone after playing from casting off; you pick Hob.

**15. Sexton and the deal (Main 2)** (built).
- You get: Sexton concept renders first, then his seated model, the bluff-top spiders with a placed Restless, and Main 2 with the Ledger's seven names.
- You approve: Sexton's look, then the deal after playing it.
- Approved on 2026-10-06: Sexton's look (the Gentleman, from three concepts), then his seated game model (“Sexton is good”). `SM_MisterSexton` (11,974 triangles, LODs at 50% and 20%, no Nanite, one hull) sits on the lookout's `SOCKET_Sit`, with `SM_SextonLedger` on his `SOCKET_Ledger` and his captions from `SOCKET_Speaker`. His face above the shadow line never lights. The deal itself comes with Main 2.

**16. The Unpaid** (built).
- You get: three concept pictures, then the rig and mesh, `AUnpaidCreature` with its Restless and Gravebound ranks, the spawner and a Ledger page. Twelve chasing at the town gate, measured on Medium.
- You approve: the look, then a fight at the town gate.

**17. Main Street and Cold Welcome (Main 3, Side 1)** (built).
- You get: concepts, then the false-front kit; Main Street dressed; Main 3 and the posters. Re-measured: Ransom's Point, the orchard, the horizon views and the `Beyond` budget (30 draws, 0.6 ms).
- You approve: the street, then Main 3 and the posters.

**18. The chapel and Hallowed Ground (Main 4)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”).
- You get: concepts, then the chapel and the graves kit; the family plot and boot hill dressed; the smashed Reliquary (with your OK); Main 4. Re-measured, with the horizon views and the `Beyond` budget.
- You approve: the chapel and graves, then Main 4.

**19. The Sink and the Keeper's Lantern (Main 5)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”).
- You get: web cards, egg sacs, the lantern and Main 5. Re-measured, with the horizon views and the `Beyond` budget.
- You approve: Main 5.

**20. The Gravemother (Side 3)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”).
- You get: the den mesh, the Gravemother on the spider rig at 1.8×, her charge and spiderlings, her return after 20 minutes of play, and her Ledger page. A tour of the den.
- You approve: the fight.

**21. The boss framework** (built; approved on 2026-10-05).
- You get: phases, untargetable states, add waves, the fog-wall seal, the boss bar, reset on death, and enemy pellets, all shown on a test spider boss on the greybox.
- You approve: the boss bar and a test fight.

**22. Abel, the Keeper (Main 6)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”; the drafted lines stand).
- You get: Abel concepts, the burial boards deck, Abel as boss and as friend, the fade to dusk, Main 6 and the scene after. Abel's drift goes out into the fog over the canyon. Phase 3 relies on the deck's open edge and fall recovery's outside rule, tested: a gust off the deck returns the player to the deck within a second. A tour at dusk mid-fight.
- You approve: the difficulty and the ending's wording after fighting him.

**23. Heirloom** (built on 2026-10-07; Delia hands it over in Main 7).
- You get: named weapons (fixed parts, a name, a flavor line) and Heirloom.
- You approve: Heirloom as a named Epic, or a Legendary.

**24. Travel by train, and The Lantern Leans (Main 7)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”).
- You get: the train in place of the grey box (the locomotive, the passenger car and Tilly's hearse car, with their LODs and collision), the track kit through Stage Gap with its buffer stop, the depot, the platform, the water tower, the signal and the station board prop; the train's departure and arrival as chosen at step 9 (by default a short shot, then a fade), shown from the console (`Looter.Train.Depart`, `Looter.Train.Arrive`) until the *Lily* opens a real trip; the station board listing the *Gilded Lily* beside “Skyreach (practice)”; and Main 7, with Tilly's hearse car waiting at the platform. Re-measured: the depot view and Ransom's Point.
- You approve: the train and the depot; the area's ending, after playing from Abel to the station board; and a practice trip from the depot to Skyreach and back.

**25. Unfinished Business (Side 2)** (built on 2026-10-07; approved on 2026-10-08 with your play-through: “it all looks good”; Amos's drafted lines stand).
- You get: Amos, Whitlock Fields dressed, and Side 2.
- You approve: Amos's mission.

**26. Ranger caches (optional)** (approved on 2026-10-08; built on 2026-10-08, approved with your play-through, “it all looks good”:
the windmill's crate under the tower, the Sink rim's, the bluff path's, and the gang's Strongbox in the sheriff's
office).
- You get, only with your OK: the Supply Crate and Strongbox out of the backlog, `AChest`, the three caches and the sheriff's Strongbox.
- You approve: the import, then the chests after opening them.

**27. Ransom's Rest finished** (checked again on 2026-10-08 with the art session's notes all closed; approved on 2026-10-08 with your play-through: “it all looks good”).
Ransom's Rest is done. The next level (the *Gilded Lily*) is on hold: on 2026-10-08 you chose to polish other things
first, and nothing of it is built until you say so; whether it stays an airship is still open.
- You get: Ledger pages for every new creature and character, XP tuned so the main path ends near level 9, every `Looter.*` test green, a full Medium tour under 8.3 ms at every view (the horizon and boundary views included), a level check that every respawn grave, spawn point and tour view lies inside the playable boundary, and an entry in `Docs/Performance.md`.
- You approve: Ransom's Rest as done, after a new session from the tutorial to the station board at the depot and a practice trip to Skyreach and back. Then you decide whether the *Gilded Lily* stays an airship, and only then does it start.
- Where it stands (2026-10-08):
  - XP: `Looter.XP.Path` plays the missions on paper with the level as built. The main path ends at level 9.7 after 53 kills (every kill comes from a mission's encounters, egg sacs or Abel's adds; the level has no roaming creatures), and with Sides 1 and 3 at 10.1, the band's top. No retuning was needed.
  - Tests: all 226 `Looter.*` tests pass, including `Looter.World.RansomsRest.InsideBoundary` (every respawn grave, player start, encounter spot and tour view inside the boundary).
  - Ledger pages: every new creature and character has one (the Unpaid, the Gravemother, Hob, Sexton, Delia, Aldana, Tilly, Ruth, Abel, Amos, and the seven names).
  - Performance, Medium with the editor closed (2026-10-08, after the last art round): all 28 tour views run at 149-216 fps; the heaviest are ChapelOverTown with the far shadow cascade on (6.7 ms, a comparison view), the Farm and the aerial at dusk (6.6 ms). The spawn measures 6.3 ms, p95 6.9 ms (`Docs/Performance.md`). A per-pass GPU capture of the tour shows no single art asset as a real cost: the heaviest views spend 1.1-1.2 ms in the base pass, 0.5-0.7 ms in the depth prepass, at most 0.5 ms on shadows, and 0.24 ms on translucency at the deck.
  - The terrain generator rebuilds Ransom's Rest, Lvl_TerrainTest and the tutorial island byte for byte as committed (`Tools\terrain_identity.ps1`).
  - Also built since the plan: the layout's fences, walls, grave rows, cairns and yard props (`build_area_dressing.py`), every fence standing plumb and stepped down slopes; the ridge faces broken up with slope scrub in patches, crease pines and far-tree groves; the Gravemother's den dressed; the Sink's pit wall in the art's cliff panels with talus at their feet; the ramps' walkways kept clear of cliff pieces and the burial deck's boards clear of the Rim's rock.
  - Your play-test notes are all fixed: the spider boss stuck in the wall (f3e3579), the rock in the way to the tutorial island's lookout (bd96147, 8e0e50c), the player 15% smaller and slower, a look-sensitivity setting, space standing up from a crouch, the slide (8fba9e6, 73aa50f), and the spiders slowed with the player (6b03dff).
