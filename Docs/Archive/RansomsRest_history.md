# Ransom's Rest: history (archive of Docs/Areas/RansomsRest.md)

Moved out of `Docs/Areas/RansomsRest.md` on 2026-10-13, when the docs were condensed for the AI (the user's call). Never
loaded by default; read only when the history matters. Everything below is the old text, word for word, under its old
heading. **SUPERSEDED** marks text that is wrong now: the live doc and `Docs/Areas/RansomsRest_Expansion.md` (2026-10-13:
mission givers, Hob's roosts in place of save graves, the opening inside Ransom's Rest with Skyreach's tutorial retired,
Skyreach as area 3, the mission list and XP) say what holds.

## The old opening paragraph (history)

This is the first area of the Revenant campaign, and the game's hub from then on. It is the plan to approve one step at a time; nothing is built yet. It lives in the repo as `Docs/Areas/RansomsRest.md`, beside the story, `Docs/Story.md`. The six step 1 questions were decided on 2026-10-01 (see *Decisions for you, by step*). Step 2, the map, was approved the same day with your changes: Ransom's Rest is grounded on Teropa, the earth-like world where the story takes place, instead of being an island, and cliffs and obstacles are to be added to break up the open ground. Later that day you approved the valley ending at the Rim (step 2b) and four of the six story proposals, decided that travel between areas is by train, and deferred the *Gilded Lily*'s question until Ransom's Rest is complete. Building starts next, at step 3: the main session builds Ransom's Rest with several agents, to a professional standard.

**Coordinates** are in metres: x east, y north, with the origin at the centre of the play space (the same point as before, about 11 m east of the town gate). `layout.json` uses centimetres with X north and Y east, so X = y × 100 and Y = x × 100.

## At a glance (old rows)

**SUPERSEDED (2026-10-13):** Play time and Missions (expansion §2), the XP sentence of Enemy levels (expansion §10,
`Docs/Progression.md`), Arrival (expansion §1: a new game opens on the cold open; no tutorial, no practice trips), and the
Skyreach (practice) line of Exit.

| | |
|---|---|
| Size | About 309 × 205 m with Gravewind Point and Stage Gap: about 42,800 m², roughly 1.5× the tutorial island (about 28,500 m², both measured on their outlines). The valley floor is about 245 × 205 m |
| Play time | Main path about an hour; 75–90 minutes with the side missions |
| Enemy levels | By where they stand, never the player's level (2026-10-12): 1–6 before ranks, rising along the story from the bluff (1–2) to the deck (5–6); the Gravemother is set at 6 and Abel at 7 (`layout.json` `gameplay.levels`, `Docs/Progression.md`, "Creature levels"). A player who does the story and both side missions leaves at about level 6.9 (786 XP), about 7.4 after clearing every camp, patrol and ambush once (`Docs/Progression.md`, 2026-10-09) |
| Missions | 7 main, 3 side (one is the Gravemother), and optional Ranger caches |
| Legendary monster | The Gravemother, in the Sink's den |
| Arrival | The first cast-off from Skyreach: the skiff sails into a cloud, and it parts on the gang's skiff gliding out of the evening cloud in the west seven days ago, with the player as Ellis (the cold open). “Skip the tutorial” goes straight to the cold open. Every later trip here arrives on the depot's platform: by train, or back from a practice trip to Skyreach |
| Exit | The depot at the east end of town, by the undertaker's yard, where the line comes in through Stage Gap. Its station board lists “Skyreach (practice)” after the first cast-off, and the *Gilded Lily* after Main 7, when Tilly's hearse car waits at the platform |

## Size and shape (history)

Ransom's Rest is not an island. It is a high valley on Teropa, the earth-like world where the story takes place. Ridges close the basin on the north, east and south. On the west it ends at the Rim, a 70 m escarpment over Gravewind Canyon, with the sunset beyond. The valley floor is about 245 m east to west and 205 m north to south. Gravewind Point reaches 45 m west off the Rim, and Stage Gap cuts 18 m into the east ridge. In all it is about 309 × 205 m and 42,800 m², roughly 1.5× the tutorial island. Coordinates are unchanged. Heights are measured from the ground a thing stands on, and a negative height is a drop.

**What breaks up the open ground.** Every stretch that was blank on the step 2 map now has rock, trees, a wall or a ruin in it. Outside the zones no gap is wider than about 19 m, except the crossroads at the keeper's grave, which stays clear on purpose. Everything comes from kits, instanced: the cliff kit, a new freestanding outcrop kit for rocks seen from every side, boulders, pines, oaks and dead trees, fences, new stone walls, and the house trim sheet. It adds instances rather than draws. Exact outlines go into `layout.json` at step 4.

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

None of this changes a fight's ground. Inside every fight's area it is cover standing on that ground, and the wash, the gorge, Coffin Rock and the Nose are outside all of them (see *Encounters stay on one level of ground*).

  - It is the planned heaviest view, now with the canyon and the horizon in it as well, and it is measured on the greybox at steps 5a and 5b, before any art.

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

## Zones (old rows)

**SUPERSEDED (2026-10-13):** the respawn graves (now Hob's roosts, expansion §8; boot hill has no roost) and the practice
trips to Skyreach.

| Boot Hill | (0, 32), +5 m | A small hillside cemetery behind the street, with a respawn grave. The hanging tree stands by the north road, and north of it lies the Spoil Bank, the quarry's rubble heap, with the quarry track up to the Old Quarry under Larkspur Ridge | Roaming Unpaid after Main 4 |
| Undertaker's Yard and the Depot | (100, 2) | ... Every trip arrives and leaves here, including practice trips to Skyreach | Main 7, then travel |
| Chapel of Saint Ada | (−19, 60), +10 m | A white clapboard chapel with a louvered belfry and a tall steeple, the tallest building on Ransom's Rest. A churchyard of about 50 headboards behind an iron fence, with dead trees. Inside, the smashed Reliquary, dark and empty. A respawn grave. The fallen glebe wall runs down the knoll's west side | Main 4 |
| Gravewind Point | (−150, 13) | ... Knee-high cairns line the road to it from the keeper's grave (respawn) at its root (−112, 17), near the roofless old lantern house | Main 6; Abel afterwards |

## Encounters stay on one level of ground (history)

The obstacles keep to this rule. Inside every fight's ground they are cover standing on it: boulders, low walls, wagons, a rock ring, blocks on the Sink floor. Every new change of level (the Dry Wash, Mill Gorge, Coffin Rock, the Nose) is outside all fights, and nothing new stands within 5 m of the bluff path.

This is checked on the step 5b greybox by spawning each group with `Looter.SpawnCreature` and chasing it. If it is too limiting, a navmesh with cliff and ramp links comes forward to that step instead of waiting for the *Lily*.

## Missions (old section)

**SUPERSEDED (2026-10-13):** the mission list, givers, steps, rewards and XP are the expansion's (§2–4); missions are picked
up from givers and turned in. The lines kept in the live doc ("Today's lines") still stand.

Sexton's deal comes early, about 15 minutes in, on the lookout where Ellis died (decided at step 1). Missions are turned in, Borderlands' way: once its objectives are done a mission waits for its giver (Delia, Sexton, Tilly, Aldana, Amos), and its rewards come as it's turned in; each gives a fixed amount of experience, on top of kills (2026-10-08: `Docs/Progression.md` has the givers, the amounts and the count). Where a mission's last step below is a talk with its giver, that talk is now its turn-in.

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
1. Find where the Gravemother hungers. Done as you go down into the Sink: anywhere on its floor, or on its ramp from about 2 m below the rim (the rim and the ramp's head don't count, and the den needn't be entered). The arrow points at the ramp head on the rim.
2. Kill the Gravemother.

**Reward:** XP the first time, and the Legendary loot table on every kill. She comes back on an arrival after at least 20 minutes of play since her last death. An arrival is coming in at the station, by train or back from Skyreach, or loading a session. Until the *Lily* exists, that means loading the session or coming back from Skyreach.

### Optional: Ranger Caches (only if you OK bringing the backlog chests in)
Find Ruth's three Supply Crates, one each under the windmill, on the Sink's rim and on the bluff path, plus the gang's Strongbox in the sheriff's office.

## NPCs (old row)

**SUPERSEDED (2026-10-13):** practice trips to Skyreach are gone. (Hob still never appears on Skyreach in today's code.)

| Hob | Everywhere in the story; never on Skyreach | A new small crow rig. He perches near the next objective, makes dry captions only Ellis hears, and comments on ranks, respawns and falls. He doesn't come along on a practice trip to Skyreach and says nothing there | All |

Every new creature and character gets a Ledger page, checked by `Looter.Bestiary.Entries`. Delia, Tilly, Aldana, Sexton and Ruth get story-character pages that need no actor (new page type). Abel has one page: Enemies until “The Gravewind” is done, then Friends.

## The boss (old lines)

**SUPERSEDED (2026-10-13):** the arrival level (story alone now ends about level 5.5, expansion §10) and waking at
the keeper's grave (now the nearest found roost, expansion §8).

- Level: set at 7 (a 6 and the Boss rank's +1), whatever the player's level (2026-10-12). He ends the story as its hardest fight, over his adds (5–6) and the Gravemother (6); the player arrives at about level 6.

**Death and reset.** Ellis wakes at the keeper's grave at the root of Gravewind Point. Abel, his adds and the lanterns reset, and the fog wall drops.

## Loot and chests (old section)

**SUPERSEDED (2026-10-13):** the reward ladder and the first-full-clear estimate were for the old mission list (rewards now:
expansion §2); the tutorial's buried guns are gone (Ellis is buried with a Common Bullpup).

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

## The exit and the unlock (old section)

**SUPERSEDED (2026-10-13):** every “Skyreach (practice)” line, the first cast-off and practice trips (Skyreach is area 3,
reached from the *Lily* by skylift, expansion §1; the station board's code drops the practice line).

Finishing Main 7, The Lantern Leans:
- records the mission in the session's campaign record;
- fills in the Ledger's first page: Lucky Ned Purcell, the *Gilded Lily*;
- lists the *Gilded Lily* on the station board at the depot, beside “Skyreach (practice)”, which the board lists from the first day. Tilly's hearse car waits at the platform from then on. Until that level exists, choosing the *Lily* says “The line to the *Lily* isn't open yet” and the player stays on Ransom's Rest.

**The station board** is a LooterUI panel at every area's station. It lists every opened area the Keeper's Lantern has found, plus one blank line naming the mission that opens the next (for example “Finish ‘The Lantern Leans’”), and “Skyreach (practice)” after the first cast-off. Later areas aren't shown, and it always lists every opened area, so travel can't strand the player.

Ransom's Rest then stays the hub. Every later station's board lists it. Its creatures keep their place's levels (1–6, and Abel's 7) however strong the player comes back, so it is easy and low-XP to revisit. What the endgame does with the bands is still open (`Docs/Story.md`, “Endgame”).

**Skyreach stays reachable as a practice island** (decided at step 1). It stays outside the story.
- The first cast-off from Skyreach plays the cloud match cut and the cold open. Its confirm reads: *“Leave Skyreach? Your story begins. You can come back to practice any time.”* “Skip the tutorial” counts as that first cast-off.
- After that, every station board (the depot here, and every later station) lists “Skyreach (practice)” at any time. Choosing it is a plain transition, a fade with no cutscene, to Skyreach's jetty. This is a default you can change (step 10).
- Skyreach's jetty board lists every opened area and returns the player to its station the same way. A trip from Skyreach to Ransom's Rest arrives on the depot's platform.
- No story character, line or plot appears there. Hob doesn't come along and says nothing there. The story never names Skyreach; only the station boards, Skyreach's jetty board and the session picker do.
- Its creatures give 0 XP. By default they drop only ammo on return visits, never guns, so it stays practice and not a loot farm. This is a default you can change (step 10).
- The session save keeps each map's world state, so Skyreach and Ransom's Rest each keep their own world between trips.

## The hub afterwards (old section)

**SUPERSEDED (2026-10-13):** practice trips, the respawn graves (now Hob's roosts, expansion §8) and story missions
starting at Delia's door (now givers, expansion §2).

- On Ransom's Rest, every trip starts and ends at **the depot** by Tilly's yard, including a practice trip to Skyreach.
- **Respawn graves:** the family plot, the chapel yard, boot hill and the keeper's grave.
- **Delia's door** is where story missions start, and a plate is always set on the porch.
- **Revisits:** the Gravemother and the placed creatures come back; promotions reroll after 20 minutes of play. Abel isn't fought again; he's a friend now.

## Art needs (old rows)

**SUPERSEDED (2026-10-13):** the tutorial skiff, Skyreach's jetty and the cloud match cut went with the tutorial.

| Freestanding outcrop kit | New | `Art/Models/Rocks/Outcrops.py`: four tors of 4–10 m and a 12 m rock spine, finished on all sides, 1.5–3k fallback triangles. For rocks seen from every side: the Three Widows, the Stone Teeth, Hearse Rock, Den Rock, the Keeper's Gate and the Rimrock. The greybox stands cliff faces up in their place |
| Smashed Reliquary | Backlog model variant (needs your OK) | The Reliquary from `Art/Backlog/Loot/Chests.py` with a cracked lid and no crystal |
| Skiff and jetty | New, for the tutorial exit and the cold open | `Art/Models/Vehicles/Skiff.py`: packet paint (cream and teal) for the tutorial's skiff, and dark paint for the gang's skiff in the cold open. Skyreach keeps a timber jetty over its drop, with its bell post and slate |
| Backdrop islands | New | A few cheap, unlit distant islands in the sky around Skyreach only. The horizon of Ransom's Rest is the ridges, the canyon and the silhouettes above |
| Mister Sexton | New seated static mesh | The first human; concepts first |
| Cloud bank | New | A card for the tutorial skiff's ride into the cloud, the cold open's match cut, and the evening cloud in the west that the gang's skiff comes out of |

## Tech needs (old rows)

**SUPERSEDED (2026-10-13):** everything here about the tutorial, the first cast-off's ride, the skiff and jetty, practice
trips and the “Skyreach (practice)” line; “Respawn graves” are Hob's roosts (expansion §8); kill XP's formula is
`Docs/Progression.md`'s.

| Creature size | New, on `ACreatureBase` | Today `BeginPlay` resets every creature to scale 1, and spider leg IK, the capsule, attack range, steering and ledge probes and the health bar height are fixed centimetres. A `BodyScale` applied after the reset scales all of them. Tested by walking and attacking with a 0.45× and a 1.8× spider |
| Ranks | New, on the existing `ACreatureBase`, `ULootTable`, `ULootDropComponent` and `UCreatureHealthBarWidget` | `ECreatureRank`, a rank settings asset, four new rank tables (Basic stays `DA_LootTable_Default`, which `LootLibrary` and the DefaultTable test name), tag colors and words, coal colors. The creature's level is pushed into its `ULootDropComponent::Level` whenever it is set (new; today drops are level 1). Commands `Looter.SpawnCreature <kind> <rank> [count] [chase]` and `Looter.Loot.SimulateDrops`; test `Looter.Loot.RankOdds` |
| Level bands and XP | New | `UAreaDefinition` (first with just name, map, band and promotion chances): `DA_Area_RansomsRest`, and one for Skyreach marked as practice (0 XP, and ammo-only drops after the first cast-off by default). The level roll (since 2026-10-12 by where a creature stands, from the layout's `gameplay.levels`, not the player's level), linear health and damage, the kill XP formula with the falloff for low-level enemies, `Looter.XP.Table` to print it |
| Respawn rules | Changes `ACreatureBase` (today every creature respawns about 34 s after death) | Off for Legendary monsters, bosses and everything spawners and egg sacs create. A promoted creature comes back as Basic. Promotions rolled on arrival (coming in at the station, by train or back from Skyreach, or loading a session), with the 20-minute cooldown saved per map |
| Session save version 2 | Extends `ULooterSessionSave` (version 1 today) | World state per map (today one world), the campaign record (completed missions, active mission and step, opened areas, first boss defeats, the first cast-off from Skyreach, cold open seen), the arrival point (a station's platform, or Skyreach's jetty), and per-map promotion and Legendary-monster times. Each map keeps its own world, so practice trips to Skyreach and back leave both intact. Version 1 saves are upgraded inside the load, before anything can save them again, with their world filed under the tutorial map. Tested on a copy of a real version 1 save before merging |
| Travel | New; uses the save's existing `Map` field | `UAreaDefinition` gets its station and opening mission. Travel sets `Map` to the destination, clears the saved player spot, writes the arrival tag and saves, then opens the level with `?Session=N`. The save on level teardown is skipped for that trip (today it would write the old level's world over the new one), and autosave and save-soon pause during the first cast-off's ride, the train's shots, the fades and scenes. Arrival points are tagged by station: on the platform by the hearse car's door on Ransom's Rest, and at the jetty on Skyreach. A trip to Skyreach after the first cast-off is ordinary travel to Skyreach's jetty: a plain fade with no cutscene. The session picker shows area names (Skyreach, Ransom's Rest) instead of the map file's name. `Looter.Travel <area>` for testing |
| Leaving Skyreach | Extends `ATutorialDirector` and `USessionSubsystem` | The tutorial's done flag keeps meaning steps 1–6 and lowers the gangplank. Once the first cast-off is recorded, the gangplank is always down, and “Skip the tutorial” also sets the tutorial's done flag, so a practice visit can never strand a player who skipped. A new campaign flag records the first cast-off, which plays the match cut and the cold open; once it is set, the station board on Skyreach's jetty lists every opened area, and leaving is a plain fade to that area's station, with no ride. The first cast-off's confirm says the player can come back to practice any time. “Board the skiff” is its own `UMissionSubsystem` mission with a waypoint, shown whenever the tutorial is done and the player hasn't made the first cast-off, which covers skipping and older sessions. Skyreach's creatures give 0 XP, and on return visits they drop only ammo by default. “Skip the tutorial” stays in the new-session flow: it counts as the first cast-off and puts a Common Bullpup in the coffin. The jetty is a `layout.json` placement; `ASkiffJetty` (hold Interact at the gangplank to open the station board; on the first cast-off it shows one destination and the confirm, then the 12 s ride and the cloud whiteout), the bell and the slate are placed by `gameplay()` and checked by `Looter.World.TutorialIslandGameplay` |
| Station board | New: `UStationBoardWidget` (UI/World/) | A LooterUI panel that calls `MarkBackground`. It lists every opened area the Keeper's Lantern has found, plus one blank line naming the mission that opens the next, and “Skyreach (practice)” after the first cast-off. Choosing a line asks to confirm, then travels. Skyreach's jetty uses the same panel: on the first cast-off it shows one destination and its confirm, and after that every opened area. Only the first cast-off from Skyreach plays the match cut and the cold open |
| Interaction | New, generalizing the weapon manager's loot focus | One component owns the Interact key, tap and hold, for loot, the skiff's gangplank on Skyreach, the station board, doors and windows, headboards, the bell, lantern posts, hay bales and posters. Loot keeps its tap and hold rules and tests |
| Missions as data | New, on the existing `UMissionSubsystem` (committed at 45260c4) | `UMissionDefinition` data assets list objectives; their types are C++: reach, interact or hold, talk at a speaker point, kill N by class, tag or zone, kill a named actor, collect, defend for a time, play a scene, board (the tutorial skiff). Rewards: an XP share, a gun with a rarity floor, a named gun, unlocks. A runner per level feeds titles, objectives and waypoints to `UMissionSubsystem` (the minimap arrow exists). A Missions page in the inventory (LooterUI). `Looter.Mission.Start/Complete <id>` |
| Respawn graves | Extends `UPlayerVitalsSubsystem` | `ARespawnMarker`: wake at the nearest activated one |
| Tests | New | Missions and objectives, the save upgrade, travel, the skiff mission and the skip, the station board's lines (opened areas, the blank line, “Skyreach (practice)” after the first cast-off), arrival on the platform, the practice round trip (no cutscene, 0 XP, ammo-only drops, both worlds kept), rank odds and rarity order, loot level, creature size, Unpaid hit zones, enemy pellets, boss phases and reset, story-character pages. `Looter.World.PlayableArea` (`Contains()`; a capsule sweep is stopped by a closed edge and not by an open one), `Looter.World.FallRecovery` (a player off an open edge is back on the last safe spot within a second; no safe spot is ever recorded outside), `Looter.World.Minimap.Bounds` (outside texels dimmed, the boundary drawn, the canyon traced as void), `Looter.World.Station` (a capsule walked across the track never snags, and none gets past the locomotive or between the cars). The generator's two fixtures |

From *How the terrain generator changes* (superseded for cliff pieces by the 2026-10-05 measurement: they stay actors):

- **The build script.** Cliff pieces, outcrops, graves, fences and walls become instanced components, one per mesh, tagged `Obstacle` for the minimap and the scatter (today each cliff piece is its own actor). Track pieces are instanced the same way, untagged, since the player walks over them. Static building clusters are merged per material, with LODs. The script places the playable area, `KillZ`, and one `CullDistanceVolume` with a size-to-distance table read from `layout.json`. Everything past the boundary gets a `Beyond` tag and each zone a `Zone_<id>` tag.
- **The map square is set per area.** ... If step 5a's screenshots look soft it splits into 2×2 maps, one per tile quadrant, with no extra draws because the tiles are separate meshes already. ...

## Performance plan (history)

Everything past the boundary gets at most 30 draws and 0.6 ms of GPU at the heaviest view, inside the same totals. The 30 come from trimming terrain and cliffs from 120 to 110 (cliffs instanced), the farm from 110 to 100 and Main Street from 170 to 160.

Old rules, since changed (one backdrop mesh per layer; the far cascade measured and rejected; the first cast-off's whiteout gone):
- Past the boundary, a handful of merged meshes in sectors: the ring in 6–8 sectors (at most 40k triangles), the canyon wall in 4, and the backdrop in 3 layers × 4 (unlit, untextured, at most 6k triangles). Any one view draws 3–5 pieces of each. The ridges to the north, east and south hide the land behind them, so the ring there is only their outer slopes. On the west the canyon floor and far wall add about 4–6 draws plus their pixels to the sun-side view. Only the ring casts shadows, and only inside the 100 m shadow range.
- Buildings share the house trim sheet, and static building clusters are merged per material with LODs: World Partition HLODs show only for unloaded cells, so they do nothing in one unstreamed level. Headboards, crosses, fences, walls, web cards, outcrops and cliff pieces are instanced (today each cliff piece is its own actor), so the obstacles add instances, not draws.
- Shadows: Medium keeps two 1536 cascades over 100 m. From Ransom's Point the town is 120–180 m away and gets no dynamic shadows. Step 5a measures one far cascade, drawn only by buildings and cliffs (`bCastFarShadow`), against that and picks by screenshot and milliseconds. The tall ridges stay on the side away from the sun, because a 50 m ridge under a 15° sun throws a shadow about 190 m long.
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

## Risks (old section, word for word)

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
- **XP pacing.** With your level 1 numbers (100 XP for the first level, 10 XP a kill), about 80 main-path kills plus missions put a player near level 9 at the end. Spawn counts and mission shares are tuned at step 27. *2026-10-08: retuned for the story to end about level 6-7 (kills at 0.4 of their XPReward, fixed turn-in rewards): `Docs/Progression.md`.*
- **Legendary feel.** At 0.3% per basic kill, most legendaries come from ranked enemies and bosses, so the 8% and 2% promotions have to show up often enough.
- **Repeating the tutorial.** The farm, spiders and slimes are reused. The palette, Main Street, the chapel and graves, the Unpaid, the ranks and the boss set Ransom's Rest apart, and there's no gun rack beat and no slime-shooting mission.
- **Talking doors** read as cheap if they last beyond Ransom's Rest. The *Lily* needs human NPCs and human enemies anyway, so plan that pipeline while Ransom's Rest is built.
- **Thin gun variety.** Two gun bases are thin for an hour of loot. Plan a revolver base before the *Lily*; Dead Man's Hand needs a pistol anyway.
- **Story consistency.** The stories branch still says Skyreach. `Docs/Story.md` becomes the single canon at step 1, and this document becomes `Docs/Areas/RansomsRest.md` beside it.
- **Story wording on a grounded world.** `Docs/Story.md` was reworded for a grounded world; it names Teropa, and Skyreach is the only island, outside the story and never explained.

## Decisions for you, by step (superseded and old wording)

**SUPERSEDED (2026-10-13, expansion §1):** step 1's tutorial exit, practice island, “Skip the tutorial” and buried tutorial
guns, and “the story never names Skyreach”; step 2b's tutorial skiff and the cloud match cut; step 10 whole. Step 7's level
and XP options were overtaken (levels by place 2026-10-12; XP in `Docs/Progression.md`).

- **Step 1 (decided on 2026-10-01):**
  - The tutorial exit is the cloud match cut: the player's skiff sails into a cloud, and it parts on the gang's skiff arriving at Ransom's Rest seven days ago, with the player as Ellis. The story never names Skyreach.
  - Skyreach stays reachable as a practice island, outside the story (see *The exit and the unlock*). New sessions still offer “Skip the tutorial”.
  - The tutorial's guns come along as the guns Ellis was buried with. After “Skip the tutorial”, Ellis starts with a Common Bullpup in the coffin.
- **Step 2b and the story proposals (decided on 2026-10-01):**
  - The revised map picture showed:
    - The ground: a high valley in the Reaches, closed by ridges on the north, east and south, that ends on the west at the Rim, a 70 m drop into Gravewind Canyon. The beats that hung over the island's rim stand on the Rim and in the ridges.
    - The obstacle pass: the cliffs and obstacles listed under *Size and shape*. Outside the zones they leave no gap wider than about 19 m, except the crossroads at the keeper's grave.
    - The boundary and what lies past each side.
  - **Travel between areas is by train**, not by skiff: *“I think fast travel between regions should be via train.”* Every area has a station with a station board. On Ransom's Rest the line comes up from the wooded valley east of the East Ridge, through a rail cut in Stage Gap, to the depot at the east end of town by the undertaker's yard. The exit in Stage Gap became the depot, its platform, a water tower and the track through the gap, in the same places, and the rockslide over the old stage road stays. Skiffs stay only for the tutorial's skiff and jetty on Skyreach, outside the story, and for the gang's skiff in the cold open. The cloud match cut is unchanged.
  - **Build Ransom's Rest with several agents, to a professional standard:** buildings, obstacles, the environment and every asset working properly; interesting and interactive; fully optimized, with small files and good graphics; and fleshed out. Building starts at step 3.
- **Step 7:**
  - Enemy levels following the player inside each band, or fixed per area.
  - Kill XP growing 8% a level (recommended). The alternative, 12%, keeps 10 kills per level forever and runs players past every band.
  - Promotions of 8% and 2% on Ransom's Rest, with Legendaries hand-placed.
  - +8% max health per level as the first level-up reward.
- **Step 10:** the first cast-off's ride and its length (12 s); practice trips as plain fades with no cutscene (the default); and whether Skyreach's creatures keep dropping only ammo on return visits (the default).
- **Step 22:** Abel's look, chosen on 2026-10-06 from three concepts: **A, the Sunday keeper**. Still to come: his
  difficulty and the ending's wording.
- **Step 26:** bringing the Supply Crate and Strongbox out of the backlog.

## Build steps (old section, word for word)

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
  - XP (**SUPERSEDED**: XP retuned, `Docs/Progression.md`): `Looter.XP.Path` plays the missions on paper with the level as built. The main path ends at level 9.7 after 53 kills (every kill comes from a mission's encounters, egg sacs or Abel's adds; the level has no roaming creatures), and with Sides 1 and 3 at 10.1, the band's top. No retuning was needed.
  - Tests: all 226 `Looter.*` tests pass, including `Looter.World.RansomsRest.InsideBoundary` (every respawn grave, player start, encounter spot and tour view inside the boundary).
  - Ledger pages: every new creature and character has one (the Unpaid, the Gravemother, Hob, Sexton, Delia, Aldana, Tilly, Ruth, Abel, Amos, and the seven names).
  - Performance, Medium with the editor closed (2026-10-08, after the last art round): all 28 tour views run at 149-216 fps; the heaviest are ChapelOverTown with the far shadow cascade on (6.7 ms, a comparison view), the Farm and the aerial at dusk (6.6 ms). The spawn measures 6.3 ms, p95 6.9 ms (`Docs/Performance.md`). A per-pass GPU capture of the tour shows no single art asset as a real cost: the heaviest views spend 1.1-1.2 ms in the base pass, 0.5-0.7 ms in the depth prepass, at most 0.5 ms on shadows, and 0.24 ms on translucency at the deck.
  - The terrain generator rebuilds Ransom's Rest, Lvl_TerrainTest and the tutorial island byte for byte as committed (`Tools\terrain_identity.ps1`).
  - Also built since the plan: the layout's fences, walls, grave rows, cairns and yard props (`build_area_dressing.py`), every fence standing plumb and stepped down slopes; the ridge faces broken up with slope scrub in patches, crease pines and far-tree groves; the Gravemother's den dressed; the Sink's pit wall in the art's cliff panels with talus at their feet; the ramps' walkways kept clear of cliff pieces and the burial deck's boards clear of the Rim's rock.
  - Your play-test notes are all fixed: the spider boss stuck in the wall (f3e3579), the rock in the way to the tutorial island's lookout (bd96147, 8e0e50c), the player 15% smaller and slower, a look-sensitivity setting, space standing up from a crouch, the slide (8fba9e6, 73aa50f), and the spiders slowed with the player (6b03dff).
