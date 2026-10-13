# Ransom's Rest: the expansion

The design for the user's asks of 2026-10-13, for approval before the missions' data, the mountain, the interiors and
the house kit are built. Once approved, section 11's edits carry it into `Docs/Story.md` and `Docs/Areas/RansomsRest.md`.
It uses the names of wave 1's systems, being built now (`EMissionStart::Offered`, `FMissionOffer`, `AMissionArtifact`,
`ARespawnMarker::MarkAfterMission`).

**Coordinates** are the area doc's: metres, x east, y north, the origin about 11 m east of the town gate; heights from
the valley's base (Z = 0). `layout.json` is centimetres with X north and Y east, so X = y × 100 and Y = x × 100.

## 1. Summary

### The decisions

| | The user's call (2026-10-13) |
|---|---|
| Map order | 1 Ransom's Rest, 2 the *Gilded Lily*, 3 Skyreach. The *Lily* lies behind the north mountain and is reached by Tilly's train, as today (the mountain is optional, not the way to it; the *Lily* isn't built yet). Skyreach, today's tutorial island with Crossroads Town, becomes area 3: a sky island reached from the north side of the *Lily* by a skylift. Teropa gets one sky island |
| The opening | A new game plays the cold open at once (skippable), then the grave, then Main 1. Skyreach's tutorial and "Skip the tutorial" go; the controls are taught by contextual hints in Mains 1–3. Ellis is buried with a Common Bullpup |
| Missions | Every mission is picked up from a giver (someone at a speaker point, a notice board, or an artifact found in the world) under a gold ! (main) or a white ! (side), accepted on an offer card with a 2–4 sentence briefing, and turned in under a ?. Main 1 stays automatic. Every mission is reworked for substance, and five new side missions join them |
| The unlock | The Old Quarry's dead ore lift at the foot of Larkspur Ridge: a side mission finds its winch crank, coal for its boiler and its cable's clamp pin, then the player rides it up |
| The mountain | About +50% of today's 42,800 m², one level, 3–4 zones, 30–40 minutes with its missions: a Rim Rangers' outpost (Ruth Calder's watchtower and cabin, a board, a save grave) and an old silver mine (a tunnel interior, fights, loot). Not a quarry works at the lift top, not a view of the *Lily* |
| Mountain enemies | Dead miners, a new kind of the Unpaid on its rig (miners' gear, head lamps, picks), and the Foreman, a named Legendary monster like the Gravemother (a set level, a Ledger page, a Vigil Stone after) |
| Save points | Respawn graves are marked by walking up to them; more graves in town, at the depot, the quarry and on the mountain |
| Houses | Ransom's Rest gets its own frontier house kit (8–10 homes, new Main Street fronts, its own well, outhouse and windmill), no model used more than twice; Skyreach keeps today's set until its own rework |
| Interiors | The lift house, Ruth's cabin and watchtower, the mine's galleries, the depot's waiting room and ticket office, Tilly's coffin workshop and cellar (the sheriff's office and the chapel already are). Not the saloon or the empty homes |
| Pace | The player is 10% slower |

### What changes for the player

- **The first minute.** New game, and the screen is white: REVENANT rises through a bank of evening cloud, the cloud parts
  on the Dunne Gang's skiff gliding out of the west, and the cold open plays as written. Then the grave, Hob, Delia's door.
  The hints teach the keys as the story needs them (section 5).
- **Missions are things people ask of you.** Delia, Tilly, Aldana, Amos, Hob, Pa on his board and four new voices (a dead
  engineman, Tilly's dead father, the station agent, a dead peddler in his coffin) offer work; two boards post it; a poster,
  a note and a miner's tag start it. Each mission has a reason to go, a discovery or a twist between its steps, a talk, an
  optional objective and a turn-in that makes sense. Nothing finishes on "do task, do task, done".
- **The pace slows.** The player walks 10% slower; the main path grows from about 60 to about 75 minutes, a full clear to
  about three hours. Ten graves, each a landmark with a lantern, are the save points.
- **A second storey.** Larkspur Ridge gets a hanging valley on top, Larkspur Bench, with Calder's Post on the Rim and the
  Providence silver mine under the high ridge: the ore lift goes up, the lift and the Goat Steps come down.

## 2. The mission flow

In the order a player meets them. Minutes include walking. XP is the turn-in's fixed amount (section 10 retunes the
side missions and the kill scale so the story still ends about level 6–7). Main missions carry a gold !, side missions
a white !, each at its giver on the map, the minimap and over the giver's head; a mission ready to turn in shows a ? at
whoever it's turned in to.

| # | Mission | Giver, and where | Offered by | After | Min | XP | Rewards | Turned in to |
|---|---|---|---|---|---:|---:|---|---|
| M1 | Seven Days | none: a new game | automatic | — | 10 | 20 | 20 silver, 2 jars of grave salt, the family plot's grave | Delia, at her screen door |
| M2 | Shall We Talk Business? | Hob, on Delia's porch rail | talk | M1 | 10 | 30 | the Ledger | Mister Sexton, on the rail |
| M3 | Cold Welcome | Delia, at her door | talk | M2 | 12 | 30 | Uncommon gun, 40 silver, Pa's lantern bail | Tilly, at her window |
| C | Calder's Caches | Ruth Calder's note on the Rangers' board | artifact | M3 | 12 | 15 | 30 silver, three caches and the gang's strongbox | the Rangers' board |
| S1 | Wanted: Already Dead | the poster on the town gate's post | artifact | M3 | 10 | 30 | Rare gun, 50 silver | Tilly |
| V2 | The Old Fare (new) | Mr. Sloane, the station agent, through his door upstairs in the depot | talk | M3 | 12 | 25 | Rare Drover revolver, 30 silver | Josiah Fenn, in his coffin |
| M4 | Hallowed Ground | Tilly, at her window | talk | M3 | 12 | 40 | 40 silver, the chapel yard's grave | Father Aldana, at the vestry door |
| S2 | Unfinished Business | Amos Whitlock, at his fence | talk | M4 | 12 | 35 | Epic gun, 60 silver | Amos |
| V1 | Knocking Below (new) | Tilly, at her window | talk | M4 | 12 | 25 | Rare gun, 40 silver | Tilly |
| M5 | The Keeper's Lantern | Father Aldana, at the vestry door | talk | M4 | 10 | 40 | 50 silver | Father Aldana |
| S3 | The Gravemother | Sheriff Odell's bounty on the Rangers' board | board | M5 | 10 | 35 | 80 silver bounty; her Legendary table every kill | Mrs. Pruitt, at the store's hatch |
| N1 | Dead Lift (new) | Hiram Coyle, the lift's dead engineman, in the winch house | talk | M5 | 15 | 30 | Rare gun, 40 silver, the ore lift | Hiram, through the speaking tube at the lift head |
| N2 | The Last Watch (new) | the board at Calder's Post | board | N1 | 12 | 35 | Calder's fourth cache, 60 silver | the post's board |
| N3 | Back Pay (new) | a brass tag on the Providence's tag board | artifact | N1 | 22 | 45 | the Foreman's Legendary table; pay the eleven (Rare gun) or keep the payroll (about 400 silver) | the tag board |
| M6 | The Gravewind | Delia, at her door | talk | M5 | 12 | 45 | Abel's boss loot | none: the scene with Pa |
| M7 | The Lantern Leans | Pa, on his board | talk | M6 | 8 | 45 | Heirloom, 60 silver, the *Lily* on the station board | Tilly |

Main path about 74 minutes; the valley's six side missions about 68; the mountain (Dead Lift, The Last Watch, Back Pay,
the walk and its camps) about 55, of which 35–40 on the mountain itself. About 3 hours 15 minutes for everything. A
giver's ! shows once the mission's prerequisite is turned in. The Rim Rangers' board at the sheriff's office becomes a
bounty board (Calder's note, the sheriff's bounty); Calder's Post has its own.

## 3. The missions, reworked

Each step names its objective kind in brackets, all existing kinds unless marked **new** (section 12 lists the new
code). "Optional:" objectives never hold a step; each done adds 10 silver (at level 1) to the turn-in, Borderlands' way,
and no experience, so section 10's sums hold. Giver tags follow the wave 1 contract (`Speaker_<Name>`, `NoticeBoard_<Name>`,
`Artifact_<Name>`). Lines marked as today's are unchanged; the rest are drafts for the user.

### Main 1: Seven Days (automatic, about 10 minutes)

The cold open plays as the new game starts, skippable; the grave wake-up follows. **Steps:**
1. Seven days ago. [Scene ColdOpen]
2. Claw out of the grave: press {Jump}. [Scene GraveWake] The family plot's grave is marked as Ellis climbs out (its
   MarkAfterMission cleared): the first save point, its lantern catching beside the headboard.
3. Read the headboard beside yours. [Interact Headboard_Abel] Optional: read your own. [Interact Headboard_Ellis]
4. Go up to the farmhouse. [Reach Speaker_Delia, 9 m]
5. Take what's on the porch plate. [Interact, **new** PorchPlate] Cold biscuits Ellis can't taste, and two jars of grave
   salt with a slip in Delia's hand: *SALT FOR THE ROAD. THEY DON'T CARE FOR IT. — D.* (Her salt line keeps the Unpaid off
   the farm; the jars are the grave-salt grenades.)

**Turned in to Delia** (today's Main 1 lines), plus: *"And eat what's on that plate. I'll know if you don't."* Ellis
leaves the biscuits; Main 7 pays that off.
**New:** the porch plate (an interactable on the porch table that gives two grenades, once), its slip.

### Main 2: Shall We Talk Business? (Hob; about 10 minutes)

**Offer**, Hob on the porch rail: *"Someone's waiting on you, sunshine. Up top, where it happened."* Ellis: *"Who?"*
Hob: *"Tall. Polite. Keeps a ledger. You'll like him. Nobody does."*
**Card:** Someone is waiting on Ransom's Point, the lookout where the Deacon shot you seven days ago. Spiders have nested at
the top since. Hob won't say who it is, only that he keeps a ledger.
**Steps:**
1. Climb the bluff path to Ransom's Point. [Reach Place_RansomsPoint, height counted] On the way, Ruth's Ranger cache on
   the bluff path (a Supply Crate, hold to open) gives a Common Ranchhand the first time when Ellis carries no shotgun:
   the second gun the swap lesson needs.
2. Clear the spiders nesting at the top. [Clear BluffNest 5] Hob's blue line on the Restless one (today's).
3. Optional: look at the boards where it happened. [Interact, **new** LookoutStains] Ellis: *"He was standing right
   here."* Hob: *"So were you, sunshine. Briefly."*
4. Talk to Mister Sexton. [Talk Speaker_Sexton] The deal, today's lines.
5. Open the Ledger: {Inventory}, then 2. [OpenPage Bestiary]

**Turned in to Sexton** (today's two lines), then: Ellis: *"Pa's lantern. Where is it?"* Sexton: *"I attend deaths,
friend, not lost property. Ask your grandmother what he ran out with."* He knows exactly where it fell; the player
learns that much later.

### Main 3: Cold Welcome (Delia; about 12 minutes)

**Offer**, Delia through the screen door: *"You've been up there. I can smell the cold on you."* Ellis: *"Pa's lantern.
Where is it?"* Delia: *"He ran out with it lit. They carried him down without it. Tilly Bright laid him out. Whatever
came home with him, she has."* *"Go by the farm road. They'll shutter up at you. Let them."*
**Card:** Pa ran up the bluff path with his keeper's lantern lit, and nobody has seen it since. Tilly Bright, the
undertaker's daughter, laid him out. Go into town and ask her what came home with him.
**Steps:**
1. Follow the farm road through the orchard into town. [Reach Place_TownGate]
2. Fight off the Unpaid at the town gate. [Clear TownGate 4] Hob: *"Here they come. Nothing draws the dead like a corpse
   with a gun."* (today's)
3. Optional: read the poster on the gate. [Interact posterGate] ELLIS RANSOM, DEAD OR ALIVE, and ALREADY scrawled under
   it. (After Main 3 the same poster offers Side 1.)
4. Find Bright & Daughter, Undertakers. [Reach Speaker_Tilly] Shutters slam the length of Main Street.

**Turned in to Tilly** (today's Main 3 lines), with one new beat after *"his wasn't on him. I looked."*: *"Only its
bail, the ring it hangs by, snapped off in his fist. I couldn't open his hand till morning."* She passes the bail out
through the window with the box of effects (the Uncommon gun). Ellis carries the bail to Main 6, where the lantern goes
on the keeper's post by it.
**New:** the lantern's bail as a mission item (`AMissionItem`, section 12), listed under Carried on the Missions page.

### Main 4: Hallowed Ground (Tilly; about 12 minutes)

**Offer**, Tilly: *"Father Aldana sat up with your Pa the night they brought him down. If anyone knows where a keeper's
lantern goes, it's the man who blessed it."* *"He's barred himself in the vestry since the dead came home. Knock loud."*
**Card:** Father Aldana blessed your father's lantern every spring and sat up with his body the night he died. Since the
dead came home he has barred himself in the chapel's vestry. The chapel yard is full of them.
**Steps:**
1. Go up the north road to the Chapel of Saint Ada. [Reach Place_Chapel]
2. Clear the chapel yard. [Clear ChapelYard 13]
3. Knock at the vestry door. [Talk Speaker_Aldana] **New talk:** *"Stay back from that door."* Ellis: *"Tilly sent me.
   Pa's lantern."* Aldana: *"Abel's child. ...If the saint still knows you, she'll answer her bell. Ring it. Then look at
   what was done to her."* (Today's AldanaWaiting line moves here.)
4. Ring the chapel bell: hold {Interact} on the rope. [Interact Bell_Chapel, hold]
5. Look at the Reliquary. [Event GraveSight.Reliquary]
6. Optional: read the keepers' roll. [Interact, **new** KeepersRoll] A board in the nave lists every Ransom keeper since
   the saint. Under ABEL a name has been painted out, and still shows through: ELLIS. Hob: *"Heir to the family business,
   sunshine. Funny how that goes."*

**Turned in to Aldana** (today's Main 4 lines: *"A keeper's lantern can find an ember"* through *"Look in the Sink."*).
**New:** the keepers' roll (a plank prop in the nave with an interact point and its text).

### Main 5: The Keeper's Lantern (Aldana; about 10 minutes)

**Offer**, Aldana: *"Spiders hoard what a saint has touched, and your father's lantern was lit from her every night for
thirty years. It's in the Sink, Ellis. I'd stake the chapel on it."*
**Card:** The gang smashed the keeper's lantern in every town they robbed, but Pa's fell in the dark, whole. Spiders hoard
anything a saint has touched. Go down into the Sink and bring it back.
**Steps:**
1. Climb down into the Sink. [Reach Place_SinkFloor, height counted]
2. Shoot down the three egg sacs. [Event EggSac.Burst ×3]
3. Take the Keeper's Lantern from the webbing. [Interact Lantern_Keeper] Hob: *"Dark as a shut eye, but whole. Missing
   its bail. You've got that, sunshine."*
4. Optional: look at the hat by the den. [Interact, **new** SheriffHat] A lawman's hat, chewed, the band reading W. ODELL.
   Hob: *"Whatever lives in that hole had the sheriff for supper."* (Seeds Side 3.)
5. Climb out of the Sink. [Reach Place_SinkRim, height counted]

**Turned in to Aldana**, with a twist before today's lines: he tries the altar candle on it and it won't take. *"It won't
take a candle's flame. A keeper's lantern is lit from the saint, and the saint is out."* Ellis: *"Then how do I light
it?"* *"I don't know. Take it home to your grandmother, Ellis. She'll know what's owed a keeper."* The question is Abel's to
answer at the boards.

### Main 6: The Gravewind (Delia; boss, about 12 minutes)

**Offer**, Delia: *"Take him the lantern. Show him the way, even if he can't go."* (today's) *"He'll be on the boards at
dusk. He won't know you, Ellis. Go anyway."* Accepting fades the Rest to dusk (`AStoryLighting`, as today).
**Card:** At dusk Pa walks the burial boards on Gravewind Point, where the wind pours off the Rim. Hang the Keeper's
Lantern on his post and show him the way, even if he can't go.
**Steps:**
1. Carry the lantern to Gravewind Point, by the west road or the keeper's path. [Reach Place_GravewindPoint] Optional:
   take the keeper's path, the way the Ransoms walked their dead. [Reach Keeper's Bridge] Hob: *"Every Ransom for sixty
   years walked the dead this way at sundown. Your Pa did it Tuesday."*
2. Hang the lantern on the keeper's post by Pa's bail. [Interact LanternPost_Keeper]
3. Defeat Abel Ransom, the Keeper. [KillNamed Boss_Abel]
4. Sit with Pa. [Scene SitWithPa] He lights it from his ghost light; it leans north-east.

**Finishes with the scene** (today's). The keeper's grave is a save point the player has usually marked on the way.

### Main 7: The Lantern Leans (Pa; about 8 minutes)

**Offer**, Pa on his board: *"She leans north-east, El. That's Purcell."* *"Your grandmother's kept something back for
you. Go home and get it. Then go and bring Saint Ada home."*
**Card:** Pa lit the Keeper's Lantern, and its flame leans north-east, over the ridges, toward Lucky Ned Purcell and the
*Gilded Lily*. Go home to Delia first. Tilly's hearse car waits at the depot.
**Steps:**
1. Go home to Delia. [Talk Speaker_Delia] Heirloom out through the door (today's lines and `ADoorHandoff`).
2. Optional: eat what's on the porch plate. [Interact PorchPlate] Pa asked it on his board (*"Eat it, even if you can't
   taste it. She needs to see it gone."*). Delia, through the door: *"...Thank you, Ellis."*

**Turned in to Tilly** (today's Main 7 lines), the *Lily* on the station board.

### Side 1: Wanted: Already Dead (the gate's poster; about 10 minutes)

**Offer**, the poster on the town gate's post, read (`Artifact_PosterGate`, an `AWantedPoster` that also offers):
**Card:** WANTED: ELLIS RANSOM, OF THE DUNNE GANG. DEAD OR ALIVE. Somebody has written ALREADY under the last line. There's
one on every post in town and on both farms.
**Steps:**
1. Tear down your wanted posters around town and the farms. [LastingInteract WantedPoster 6 of 7, hold]
2. Read the note on the Rim Rangers' board by the sheriff's office. [Interact CalderNote] It offers Calder's Caches.
3. Optional: look round the sheriff's office. [Reach the office's inside] The desk's open daybook, last entry a month
   old: *Gone down the Sink after the big one. Back by supper. — W.O.*

**Turned in to Tilly**, with a twist: *"So it was you, tearing them down. Good. They were bad for trade."* Ellis: *"Who
wrote ALREADY?"* Tilly: *"I did. On the first one. Somebody had to."* Then today's line about the stranger's effects (the
Rare gun).

### Calder's Caches (Ruth's note; about 12 minutes)

Today's optional Ranger caches, as a mission. **Offer**, Ruth Calder's note on the Rangers' board (`Artifact_CalderNote`):
**Card:** RIM RANGERS. Three caches laid for any Ranger riding through: under the iron flower that drinks the wind; on the
lip of the hungry hole; on the path where the Ransom kid used to go to sulk. Take what you need and pin the tag. The
gang's strongbox is in the sheriff's lockup as evidence. The fourth cache is at the Larkspur post, if the lift ever runs
again. — Capt. R. Calder
**Steps:**
1. Find Calder's three caches. [LastingInteract RangerCache 3: Whitlock's windmill, the Sink's rim, the bluff path; one
   opened in Main 2 already counts]
2. Open the gang's strongbox in the sheriff's lockup. [Interact GangStrongbox, hold] Ellis knows the combination (*"The
   Deacon's birthday. He never changed it."*). Inside, with the loot, a fold of banknotes and a slip in the Deacon's hand:
   *Kid's share. For when the kid comes home. — A.D.* Hob, after a while: *"Sentimental, for a man who shot you."*

**Turned in at the Rangers' board** (`NoticeBoard_Rangers`): pin the three cache tags on Calder's tally, Ranger fashion.
Hob: *"Three tags for the Rangers. She'll still hang you, sunshine."* The note's last line points to the ore lift.
**New:** the cache tags as items (one per cache opened), the board as this mission's turn-in, the Deacon's slip.

### Side 2: Unfinished Business (Amos; about 12 minutes)

**Offer**, Amos at his fence (today's meet lines; the talk that was step 1 is the offer).
**Card:** Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint went dark,
and drifted home to find it rotting in the field. He isn't angry yet. He'd like it in.
**Steps:**
1. Load his six hay bales into the barn. [LastingInteract HayBale 6, hold]
2. **New talk:** ask Amos who's on the road. [Talk Speaker_Amos] The twist: *"That's the Merritt brothers and Sol Tanner.
   My hands. Fever winter took them, and I owed them for the harvest. Never paid. Couldn't."* *"Reckon they've come to
   collect."*
3. Drive off his old hired hands. [Clear WhitlockHands 6]
4. Optional: read what Amos owed, chalked inside the barn door. [Interact, **new** WagesChalk] Three names, three sums,
   none crossed out.

**Turned in to Amos** (today's thanks), plus: *"I'll pay them what I owe when I see them on the far side. If there's a
far side."* The Epic gun he was buried with.

### Side 3: The Gravemother (Sheriff Odell's bounty; about 10 minutes)

**Offer**, the Rangers' board (`NoticeBoard_Rangers`), the sheriff's own posting, a month old:
**Card:** BOUNTY. The big one in the Sink's den. She took my dog and the Haskett mule. The town's purse to whoever brings me
her fang, paid at Pruitt's. I'm going down after her myself. — Sheriff W. Odell
**Steps:**
1. Find where the Gravemother hungers. [Reach Place_SinkFloor, 19.5 m, height counted] (today's)
2. Kill the Gravemother. [Clear Gravemother] (today's: a kill on an earlier visit counts while she's away)
3. Take her fang. [Collect GravemotherFang, **new**: a mission item her body drops on the first kill]
4. Optional: find what's left of Sheriff Odell in her den. [Interact, **new** SheriffBadge] His badge in the webbing.

**Turned in to Mrs. Pruitt** at the store's hatch (`Speaker_Pruitt`, **new**: the vendor's door also talks): *"Lord, the
size of it. That's the town's purse, what's left of it."* *"Wade was my brother. He always said he'd die with his boots on.
Never said a word about his hat."* XP the first time; her Legendary table on every kill and her Vigil Stone, as today.

## 4. The new side missions

### The Old Fare (Mr. Sloane, the depot; after Main 3; about 12 minutes)

The station agent stayed when half the town left. He's alive, barricaded upstairs in the depot, and heard through his
door like the rest of the living.
**Offer**, through his door at the top of the depot's stair (`Speaker_Agent`): *"Who's there? ...Lord. The Ransom
kid."* *"Every night since Wednesday they come and stand in my waiting room like they've a train to catch. I can hear them
breathing. If you're dead too, maybe they'll listen to you."*
**Card:** Mr. Sloane, the station agent, has locked himself upstairs. Every night since the saint went dark the dead have
stood in his waiting room as if for a train. Clear them out, and find out what they're waiting on.
**Steps:**
1. Clear the depot's waiting room. [Clear DepotWaiting 5: three Unpaid, a Restless one and a Bell-Ringer, in the room
   and out on the platform]
2. Find what they were waiting on. [Reach the baggage room] A pine coffin tagged GRAVESEND. JOSIAH FENN. FARE PAID.
3. Look at Fenn. [Interact FennCoffin; a Grave Sight flash] Two pale rings on the dead man's eyelids, where coins sat. Hob:
   *"Somebody's been at his fare, sunshine. Rob a dead man's eyes and every soul for a mile hears about it."*
4. Optional: look in the agent's desk. [Interact AgentDesk] A gambling marker from the *Gilded Lily*'s tables, payable to
   N. Purcell. Ellis: *"Ned's paper."* Hob: *"Small world."*
5. Ask Mr. Sloane about Fenn's coins. [Talk Speaker_Agent] *"His people laid gold on his eyes, like the old days. Gold, on
   a dead man! I only borrowed it. Against a debt."* Two old coins come out under the door. [Collect FennsFare]
6. Lay the coins back on Fenn's eyes. [Interact FennCoffin, hold]

**Turned in to Josiah Fenn**, a talking coffin (`Speaker_Fenn`): *"Much obliged. A man likes to pay his own way."*
*"Tell Sloane I forgive him. Don't tell him I said it like that."* *"There's a pistol under me. Never fired it once. Take
it."* The waiting room stays quiet from then on; Mr. Sloane comes down to his ticket window, a new talking window with
lines for the station (*"Line's open to the Lily, kid. Your car's coupled."* after Main 7).
**Why it matters:** it shows the player the old fare, grave gold on dead men's eyes, long before Ira's dying line, and
seeds the *Lily*. Grave gold stays story only, never money.
**New:** the depot's interiors (section 9), the coffin's speaker point, the coins as a mission item, the marker prop, the
agent's two speaker points (upstairs door, then the ticket window), the DepotWaiting encounter.

### Knocking Below (Tilly; after Main 4; about 12 minutes)

**Offer**, Tilly at her window: *"You've been in a grave and out of it, Ellis, so you'll understand me."* *"Something's
knocked in my cellar every night since Wednesday. I've four down there waiting out their seven days, and no keeper to walk
them."* *"I'd go down myself, except I'd rather not."*
**Card:** Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up. Since the saint went dark
something down there knocks all night, and Tilly hasn't been below since. The way in is through her father's workshop,
off the undertaker's yard.
**Steps:**
1. Go round to the workshop door in the undertaker's yard. [Reach WorkshopDoor]
2. Find the cellar key. [Collect CellarKey, on her father's measuring hook] Optional: read the coffin orders on the slate.
   [Interact OrdersSlate] Four names, and a fifth struck through in another hand: *T. BRIGHT. PINE. NO BRASS.* Hob:
   *"Wrote his own order, did he. And she put brass on it anyway."*
3. Go down into the cellar. [Reach CellarFloor]
4. Quiet the dead in the cellar. [Clear TillyCellar 4: three Unpaid and a Restless one, in tight rooms]
5. Find what's still knocking. [Interact ColdRoomDoor] Behind the cold-room door, at his bench, a man in an undertaker's
   apron is measuring a coffin that isn't there: Thaddeus Bright, Tilly's father, home from the Sundown Road to his work.
6. Talk to Thaddeus. [Talk Speaker_Thaddeus] *"Ransom's child. I measured you, you know. You've stood up since."* *"I
   kept them in their boxes a week. They got past me when the door opened. A cellar wants keeping."* *"Give her my tape.
   Tell her the brass was too much. And that I liked it."* [Collect ThaddeusTape]

**Turned in to Tilly** (the tape through the window): *"...He's down there."* *"Of course he is. He never once came up for
supper."* *"Brass handles. He'd have hated them."* *"There's a box on the bench by the cellar door. It was his. Don't
make me look at it."* (A Rare gun.) Thaddeus stays in the cold room afterwards, a friend with lines through the story, and
Tilly's later lines mention him. His car is the hearse car Ellis rides in.
**Why it matters:** a child and a dead father, Tilly's side of what Ellis does at the boards, and the "brass" she
polished for him (her Main 7 line) paid off.
**New:** the workshop and cellar (section 9), Thaddeus (a friendly dead man on the Unpaid rig, posed like Amos), the
cellar's encounter, a height band on Main Street's safe ground so the cellar under it can hold a fight.

### Dead Lift (Hiram Coyle, the Old Quarry; after Main 5; about 15 minutes)

The Providence Mining Company's ore lift ran from the Old Quarry to the top of Larkspur Ridge until the mine shut four
years ago, after "the fall" that buried eleven men. Hiram Coyle ran its winch for twenty years and died at his brake. He
hasn't left.
**Offer**, Hiram on his stool by the cold boiler (`Speaker_Hiram`): *"Ain't had a passenger in four years. You riding up,
or just looking?"* Ellis: *"Riding."* *"Then she wants three things. Her crank: the sheriff took it when they shut her.
Coal: boiler's stone cold. And her clamp pin, which went down the hill in Abe Haskett's pocket the day of the fall."* Hob:
*"Dead lift. I'll let you have that one, sunshine."*
**Card:** The old ore lift still hangs over the Old Quarry, and its engineman, Hiram Coyle, still waits by its boiler. Find
its winch crank, coal for its boiler and the clamp pin its brakeman carried down, and Hiram will take you up the mountain.
**Steps:**
1. Three things, any order (one step of three objectives):
   - Take the winch crank from the sheriff's lockup. [Collect WinchCrank] Optional: read Sheriff Odell's report on the
     fall. [Interact OdellReport] *Eleven men. Teague swears the roof gave on its own. Then why'd he have powder burns on
     his cuffs? My brother Jory was one of the eleven. — W.O.*
   - Fill a coal sack at the depot's coal stage. [Interact CoalStage, hold → Collect CoalSack] Mr. Sloane mutters
     through his window: *"That's railroad coal! ...Oh, take it. Nothing's running."*
   - Take the clamp pin from Abe Haskett. [Collect ClampPin] As Ellis leaves the winch house after accepting, the lift
     crew rise on the quarry floor: four dead miners and Abe, a Restless one with the pin on a cord round his neck
     (QuarryCrew). Hiram: *"Ah. The boys heard you ask after the pin."*
2. Fit the crank, stoke the boiler, drive the pin. [LastingInteract LiftFitting 3, hold, in the winch house and at the
   bottom sheave]
3. Hold the quarry while she raises steam. [Defend 60 s; RaisingSteam: two spiders and a Venom Spitter off the quarry
   track, then two dead miners] The boiler's thump and the whistle carry across the valley.
4. Ride the lift to the top. [Board Lift] Hiram: *"Hold the rail."* The first ride (section 6).

**Turned in to Hiram** through the speaking tube at the lift head (`Speaker_LiftTube`): *"Hear that? She still runs."*
*"Tell the Rangers Hiram Coyle says they owe him for coal."* *"There's a carbine in the head's tool box. Company's. They'll
never miss it."* (A Rare rifle.) From then on the lift runs both ways: hold {Interact} on either station's bell pull. On
Main Street, the Haskett house mutters: *"...That's the lift. Abe always said he'd hear her run."*
**New:** the lift (cage, tower, head, cable, winch house; `AOreLift` and the ride scene), Hiram (a friendly dead man in
the miners' costume set), the speaking tube's speaker point, the coal stage, the three fittings, the crank, sack and pin
as mission items, a named Restless dead miner who drops one, the QuarryCrew and RaisingSteam encounters.

### The Last Watch (Calder's Post's board; after Dead Lift; about 12 minutes)

**Offer**, the board by Ruth's cabin door (`NoticeBoard_CalderPost`):
**Card:** RIM RANGERS, LARKSPUR POST. Post unmanned: gone after the Dunne Gang's trail. Any Ranger riding through, light the
watch fire on the tower, so the posts down the Rim know Larkspur stands. If it stays dark they'll take the post for
fallen and pull back. — Capt. R. Calder
**Steps:**
1. Clear the post's yard. [Clear CalderPost 5: four dead miners and a Restless one, come up from the Bench since the post
   emptied]
2. Find oil for the watch fire in Ruth's cabin. [Collect LampOil] Optional: read Ruth's journal. [Interact RuthJournal]
   Her entry for the night of the robbery: *Skiff came up the canyon at dusk, running dark. Lights on Ransom's Point. A
   shot on the bluff path, then one up top. The Ransom kid drew on the Deacon at the last. Saw it through the glass.
   Doesn't wash out ten years. Rode the north trail down and round by the rail. Dawn before I got there. Closed the kid's
   eyes myself. Wasn't sorry.* And her last: *Riding south after them. If the kid's ghost is reading this: I'd have
   hanged you anyway. Probably.* Hob: *"...She closed your eyes, sunshine. Not everybody would have bothered."*
3. Light the watch fire on the tower. [Interact WatchFire, hold]
4. Keep the post until a post down the Rim answers. [Defend 45 s; the Watch: three dead miners come up from the Bench]
5. Read the answer with Ruth's code book. [Interact CodeBook] Far down the Rim a light winks on a mesa. Decoded: *LARKSPUR
   WE SEE YOU. CALDER RIDES SOUTH. ALL POSTS: WORD IS THE RANSOM KID WALKS. SHOOT ON SIGHT.* Ellis: *"Word travels."* Hob:
   *"Faster than you do."*

**Turned in at the post's board**: sign the post's log. Calder's fourth cache (a Supply Crate in the cabin) opens.
**Why it matters:** Ruth's note from Side 1 pays off; the player learns someone saw Ellis balk at the end, which is the
seed of Ruth's Act I ambush and truce.
**New:** Calder's Post (section 6), the watch fire (`AWatchFire`: lit or dark, kept by the save), the far answering light
(an unlit flicker card on the nearest backdrop mesa, shown by story state), the journal, code book, oil, the post's board.

### Back Pay (a miner's tag, the Providence; after Dead Lift; about 22 minutes)

**Offer**, a brass tag on the IN side of the tag board at the Providence's mouth (`Artifact_MinersTag`): *No. 7. J. ODELL.
IN.*
**Card:** Eleven brass tags still hang on the IN side of the Providence's tag board. A miner hung his tag there going down
and took it off coming up. These eleven never came up, and something has worked the dark galleries ever since.
**Steps:**
1. Go into the Providence. [Reach UpperGallery] Hob: *"Eleven tags on IN. You do the sum, sunshine."*
2. Clear the upper gallery. [Clear MineUpper 5] Dead miners swinging picks at the walls, working for no one.
3. Search the Foreman's office. [Interact PayrollBook] The payroll: eleven men, three months' wages, unpaid, and the
   company's order, *Break the strike.* The Foreman's last entry: *They've barricaded the lower drift. Say they'll come up
   when they're paid. A small charge on the drift mouth will bring them up. — C. Teague.* Hob: *"That's the trouble with
   small charges."* Optional: read the chalk on the gallery wall. [Interact ChalkTally] *DAY 6. TEAGUE WILL DIG US OUT.*
4. Go down the decline and clear the lower drift. [Clear MineLower 6]
5. Take the eleven's tags from their barricade. [Interact Barricade] Chalked across its timbers: *PAY US AND WE COME UP.*
6. Put the Foreman down. [KillNamed Legendary_Foreman] Beyond the barricade, in the stope where the roof came down,
   Cyrus Teague has dug at the fall every night since he died.
7. Take the payroll from the stope. [Interact PayrollChest → Collect Payroll] The company's silver, never paid out.

**Turned in at the tag board** (`Speaker_TagBoard`: the shift's voice out of the dark), with a **choice** (**new**: a
turn-in with two rewards):
- **Pay the eleven.** The tags go on OUT with their wages. The shift boss: *"Paid in full. Shift's over, boys."* The
  Providence's dead stop coming back (the Foreman's Vigil Stone still calls him); a Rare gun lies on the board, one of
  them carried down for claim-jumpers. Hob: *"First honest pay this mine ever made."*
- **Keep the payroll.** About 400 silver at level 7. The tags stay on IN, and the dead keep working the galleries (their
  encounters come back on arrival). The shift boss: *"...Company man after all."* Hob: *"Can't spend it in the ground,
  sunshine. Good thing you're not staying there."*

**Why it matters:** the Unpaid were unpaid before they ever died. Debts and who owes what, the story's theme, in a mine.
It also closes the Odell family's thread: Jory's tag, Wade's report and his hat, his sister at the hatch.
**New:** the Providence (section 6), the Foreman and the dead miners, the choice turn-in, the tag board (an artifact that
offers and a speaker point that takes the turn-in), the payroll book, chalk, barricade and chest.

## 5. The controls, taught in Mains 1–3

For the hints agent: one hint at a time, never in a scene or over captions, gone once the control is used, never again
once learned (today's `UControlHintSubsystem` rules).

| Lesson | Where it lands | What sets it off |
|---|---|---|
| Look and move | M1, standing at the grave's foot after the claw-out | control handed back; no input for 2 s |
| Jump | M1, the claw-out (three presses, the scene's own prompt) | the scene; no separate hint |
| Interact (tap) | M1, Abel's headboard | looking at it within 3 m |
| Sprint | M1, the 40 m path from the family plot to the farmhouse | walking forward 3 s on open ground |
| Talk / turn in | M1, Delia's door | the door's prompt, first time |
| The offer card and the Missions page | M2, Hob's offer on the porch rail | the first card; after Accept, "Missions: {Missions}" |
| Mantle | M2, the fallen pine across the bluff approach | a ledge ahead (today's sense) |
| Interact (hold) | M2, Ruth's cache on the bluff path | looking at it |
| Swap guns | M2, right after the cache's Ranchhand | a second gun carried |
| Aim and fire | M2, the bluff nest's first spider | a far target under the crosshair |
| Reload | M2, the nest fight | magazine under a quarter |
| Crouch | M2, the nest ring's waist-high rocks | taking hits near low cover |
| Melee | M2 or M3, a spider or a lunging Unpaid close in | a creature close in front |
| Inventory and Ledger | M2, step 5 asks for it; the inventory hint at the first gun picked up | the step; a pickup |
| Map | M3, after accepting Delia's offer (Bright & Daughter is off the minimap) | the first tracked objective out of minimap range |
| Slide | M3, the farm road's run down off the terrace through the orchard | sprinting downhill |
| Grenade | M3, the town gate fight (Delia's salt: ×1.5 on the Unpaid) | three or more Unpaid within 12 m |

Nothing in Mains 1–3 assumes Skyreach was played: the Bullpup is in the coffin, the second gun is the bluff cache's,
the grenades are Delia's salt.

## 6. The mountain

### Geography

**Today.** Larkspur Ridge closes the valley on the north. Its foot runs along y ≈ 86–102 from the Rim (x ≈ −90) to the
East Ridge (x ≈ 104, y ≈ 70), with an 8–12 m cliff band and steep pines above it; its crest stands about +42 m some 42 m
behind the foot (y ≈ 130–145). Behind the crest the ground falls away, and a higher forested ridge, Larkspur High, rises
from y ≈ 166 to about +75 m at y ≈ 211. The Old Quarry is a notch in the foot at x 8–32, y 92–100. The north boundary
follows the foot.

**After.** The valley side of Larkspur Ridge looks as it does today from below (its foot, band and pines), except for the
lift's cable rising out of the quarry, the lift head on the skyline and Calder's watchtower showing over the ridge's west
shoulder. Behind the crest:
- **Larkspur Bench**, a hanging valley of alpine meadow, granite and lodgepole pine, with drifts of blue larkspur (the
  ridge's name), x −88 to 112 and y 138 to 230. Its floor is +44 m behind the crest and rises gently to +58 m in the
  north-east. A small tarn lies at (−20, 200).
- **Larkspur High** moves north: its foot at y ≈ 228–233 with a 12–16 m cliff band, its crest about +120 m at y ≈ 275,
  past the boundary. Its far side falls toward the *Lily*'s country, which is never seen.
- **The west**: the Bench ends on the Rim at x ≈ −88 to −92, 115 m above Gravewind Canyon's floor.
- **The east**: the East Shoulder, a steep spur of the East Ridge's north arm, at x ≈ 108–115.

| Place | Height |
|---|---|
| The Old Quarry's floor | +1 m |
| Larkspur's crest | +42 to +48 m |
| The lift head | +47 m |
| The Bench | +44 m (south and west) to +58 m (north-east) |
| Calder's Post's knoll; the watchtower's deck | +50 m; +62 m |
| The Providence's mouth and upper gallery; the lower drift and the stope | +58 m; +50 m |

**The core square grows.** The terrain's core is a centred 400 m square today, and nothing may stand in its outer 25 m,
so features stop at y = 175. The Bench needs y = 230. The core becomes a 512 m square (`map.half` 20,000 → 25,600 cm):
4096 height cells at today's 12.5 cm, on the same grid as today's, so the valley's heights can match. Today's valley must
come out within 1 cm everywhere inside today's boundary (a compare script, as the terrain identity check). The curvature
and AO rasters keep their 20 cm (caps 2048 → 2600); the macro map keeps about 10 cm as four 4096 maps, one per quadrant
(the area doc's fallback). Tiles 5×5 → 6×6; triangles at most 190k (146k today), weighted to the playable ground. The
Bench is a region bench (as the farm terrace is), Larkspur High's foot moves to y ≈ 230, and the core's features add the
quarry bowl, the Goat Steps' ledges, the knoll, the tarn, the building pads and the paths. If the compare fails, the
fallback is a second core square for the mountain alone (more generator work: the seam between two cores).

### Zones

| Zone | Where | Size | Levels | What's there |
|---|---|---:|---|---|
| The Old Quarry (grown; section 7) | x 0–40, y 90–120, +1 m | 1,100 m² | 5–6 | the winch house, the lift's bottom station, the lift-men's plot, the Goat Steps' foot, the derrick ruin |
| The Crest | x −10–40, y 128–150 | 1,500 m² | 5–6 | the lift head, the crest path, the Goat Steps' head, the valley's best view after Ransom's Point's |
| Calder's Post | x −88 to −45, y 140–190 | 3,200 m² | 5–6 | the knoll on the Rim: the watchtower, Ruth's cabin, the corral and lean-to, the board, the Ranger's grave |
| Larkspur Bench | x −45–60, y 140–230 | 9,000 m² | 6–7 | meadow, the tarn, a pine stand, the ore tramway, the Dinner Pail camp |
| The Providence Diggings | x 40–112, y 170–233 | 4,700 m² | 6–7 | the mine mouth and tag board, the ore bin, powder house, smithy, spoil heap, the miners' plot, the Foreman's Vigil Stone |
| The Providence (interior) | under Larkspur High, north of the mouth (74, 230) | 1,500 m² of floor | 6–7; the Foreman 8 | adit, upper gallery, the Foreman's office, decline, lower drift, the stope |

About +21,000 m² in all (+49%): the Bench 18,400 m², the quarry bowl and the lift's neck about 1,100, the mine's floor
1,500. Ransom's Rest becomes about 309 × 350 m and 64,000 m².

### The ore lift

- **Bottom station**: the winch house at (28, 110) on the quarry floor, the cage's landing under the head sheave at
  (20, 114). Inside: the boiler, the winch drum and brake lever, the coal bin, the speaking tube, Hiram's stool.
- **The line**: one timber tower on the ridge face at about (23, 126), its sheaves near +30 m; about 55 m of cable to
  the head.
- **Top station**: the lift head at (27, 146), +47 m, a timber head frame with the return sheave and a landing on a
  granite shoulder just behind the crest. Not a works: a frame, a landing, a bell pull, the speaking tube, a tool box.
- **The cage**: an open ore cage with a rail, room for one, hung from the cable. The ride is a scene like the skiff ride
  today (`SkiffRide`: carried, look only, skippable), about 24 s up or down, easing at both ends. The autosave waits
  through it. Called from either end with a held bell pull; an empty cage takes 8 s to come.
- **What the first ride shows**: the quarry dropping away; at 6–12 s the whole valley opening south (Main Street's roofs,
  the steeple, the Sink's dark ring, the amber orchard, Ransom's Point on the far skyline, the canyon's haze beyond); the
  cage clanking over the tower; then over the crest the Bench opening ahead, Calder's watchtower on the Rim to the left,
  the spoil heap and the Providence's mouth under the high ridge to the right.

### Calder's Post

A Rim Rangers' outpost on a knoll at the Bench's west end, (−66, 165), +50 m, looking south over the whole valley and west
over Gravewind Canyon. The watchtower (4 × 4 m, 12 m, a stair inside, a roofed deck with the watch fire's brazier, the
heliograph and Ruth's spyglass mount) stands at (−72, 160) on the Rim's edge; Ruth's cabin at (−58, 172); the corral and
an empty lean-to stable at (−50, 182); the board at the cabin door; the Ranger's grave (Tom Abernathy, died at his post)
at (−64, 178). The post's yard, about 30 × 25 m and level, is N2's fight ground. On the cabin wall, Ruth's map of the Rim:
six pins down its length a month apart, and a seventh, fresh, on Ransom's Rest.

### The Providence

**Outside, the Diggings.** The mouth's timbered portal at (74, 230) in Larkspur High's band, facing south, with the tag
board at (70, 227). The ore tramway runs from the ore bin at (62, 204) across the Bench to the lift head (about 64 m of
narrow-gauge track, two ore cars off the rails): it carried ore to the lift, and it leads the player to the mine. A stone
powder house at (95, 214), a smithy shed at (88, 196), the spoil heap running down to the East Shoulder at (100, 190),
the miners' plot at (50, 214) with a board for the eleven never recovered. The Foreman's Vigil Stone rises by the portal
at (80, 224) after his first defeat.

**Inside** (a separate mesh kit set into the ridge, its floor 40–60 m under the ridge's surface; local layout from the
mouth, north up):

| Room | Size | Floor | What's there | Fight |
|---|---|---|---|---|
| Adit | 30 m long, 4 × 3.5 m | +58 | timber sets, a rail, hung lanterns lit by no one | none |
| Upper gallery | 24 × 16 m, 5 m high | +58 | timber pillars, ore cars, a rail spur, chalk on the walls | MineUpper |
| The Foreman's office | 6 × 5 m, off the gallery's east side | +58 | desk, payroll book, the company's notices, a cot, the company safe (a Strongbox) | none |
| Decline | 26 m, dropping 8 m (17°), with a dog-leg | +58 to +50 | a winch and a runaway car in the ditch | none |
| Lower drift | 36 × 6 m, two side stopes of 8 × 6 m | +50 | lunch pails, picks, the eleven's barricade at its end | MineLower |
| The stope (the Foreman's arena) | 26 × 20 m, 7 m high | +50 | six timber props, two ore cars, the fall (a rubble slope at the north end), the payroll chest | the Foreman |
| Manway | an enclosed stair from the stope's east side up to the gallery's north end | +50 to +58 | a barred gate opened from below | none (a way out) |

Each fight is on one level of ground: the gallery, the drift and the stope are flat; the decline and the manway hold
none. The interior's 2D footprint (y 230–345) joins the playable polygon as a tongue whose edges are marked blocked
(closed by rock, no foot shaping), so fall recovery records safe spots inside and no wall closes the portal. Inside, the
minimap shows its markers over a dark plate marked UNDERGROUND (**new**), since its bake sees only the ridge above.

**The Foreman**, Cyrus Teague: the Unpaid rig at 1.5× in a foreman's long coat, a bowler with a carbide lamp, a pick handle
and a satchel of fuse sticks. A Legendary monster at a set level 8 (6, and Soulfed's 2), orange tag and coal, a page in
the Ledger, the Legendary loot table; about 3,400 health (12 × a dead miner of his level), tuned to a 2–3 minute fight.
- *Fire in the hole*, all fight long: lights a fuse (sparks and a hiss for 2 s) and lobs the stick; a 6 m burst like the
  grave salt's.
- *The roof*, every 18 s or so below 66%: strikes a prop; dust trickles on four marked spots for 2 s, then rock falls
  there, and the prop splinters, so the cover changes.
- *The shift* (at 66% and 33%): three dead miners rise out of the rubble.
- *The dark* (at 50%): the stope's lanterns blow out for 12 s; his lamp, the miners' lamps and their coals are what's
  left (the exposure lifts, so it reads dark, not black).
- His coal is open while he lights a fuse and for 3 s after a roof strike (1.5× crits, the game's rule).
- Lines: *"Back to work!"*; *"Nobody leaves till the drift's cleared!"*; *"Fire in the hole!"*; in the dark, *"I can dig them
  out. I can still dig them out."*; his last word, kneeling at the fall: *"...Tell the company it was the roof."*
- Ledger, in Sexton's voice: *He brought the roof down on eleven men to save the company a quarter's wages, and has dug
  at it every night since. Remorse is a poor shovel, friend.*

**Dead miners**, a new kind of the Unpaid on its rig (`ADeadMinerCreature`, a subclass like the Bell-Ringer): canvas coats,
braces, felt hats or leather caps with a head lamp, a pick; three tints. Forty years of the Providence's buried dead came
home to it when the saint went dark, and the eleven never left: nobody brought them up to be given to the wind. Abe
Haskett's lift crew, buried by the quarry, came home to the lift. 180 health and 9 damage at level 1 (the Unpaid's
are 160 and 8). An overhead pick swing with a 0.8 s wind-up in which the lamp flares; a shot to the lamp in the wind-up
staggers it (like the bell and the venom sac). Unaware, they work: picks swung at the walls, a lunch pail opened. Ranks as
the Unpaid. Barks: *"Shift's not over."* *"Pay's Friday. Pay's always Friday."* *"Who's on the lift?"* *"Teague said the
roof would hold."* *"Mind the timbers!"* *"Eleven in. None out."* Ledger: *Men who went down for silver and were owed for it.
They still work for wages that will never come. Some debts outlive the debtor, friend.*

### Encounters

Each group lives on one level of ground; its chase stops at that ground's edge, as everywhere.

| Group | Creatures | Levels | Ground | Budget | On |
|---|---|---|---|---|---|
| QuarryCrew (N1) | 4 dead miners; Abe Haskett, Restless, with the pin | 5–6 | the quarry floor | N1 | N1 accepted |
| RaisingSteam (N1, defend) | 2 spiders, a Venom Spitter, then 2 dead miners | 5–6 | the quarry floor | N1 | N1's step 3 |
| Patrol Crest | 3 spiders (the area's ranks) | 5–6 | the crest path, 20 m there and back | optional | the first ride |
| CalderPost (N2) | 4 dead miners, 1 Restless | 5–6 | the post's yard | N2 | the first ride |
| The Watch (N2, defend) | 3 dead miners | 5–6 | the post's yard | N2 | N2's step 4 |
| Camp Dinner Pail | 3 dead miners round a cold cookfire with their lunch pails, 1 Restless, a Bell-Ringer; a Supply Crate | 6–7 | the Bench's middle round (−5, 170) | optional | the first ride |
| Ambush Spoil | 3 dead miners rise out of the spoil heap | 6–7 | the Diggings' yard | N3 (main) | the first ride |
| MineUpper (N3) | 4 dead miners, 1 Restless | 6–7 | the upper gallery | N3 | always |
| MineLower (N3) | 4 dead miners, 1 Restless, a Bell-Ringer | 6–7 | the lower drift | N3 | always |
| The Foreman | the Foreman (8) and two waves of 3 dead miners | 8; 6–7 | the stope | N3 | always; then his Vigil Stone |

47 kills on the mountain and the quarry: 39 on the missions' way, 8 optional. Placed creatures come back on arrival as
today; the mine's stop coming back if the eleven are paid.

### Loot

Chests: the Dinner Pail camp's Supply Crate; Calder's fourth cache (a Supply Crate, opened by N2); the company safe in the
Foreman's office (a Strongbox: 2 guns at Luck 1.0); the payroll chest (N3's choice). Containers: a footlocker in Ruth's
cabin and one in the winch house, six lunch pails in the mine and the camp (**new**: a small tin container opened with a
tap, ammo or 4–10 silver, `Lootables.py`), two graves to dig at the miners' plot, a mailbox at the post. About 14
breakables: powder kegs and crates at the Diggings (5), in the mine (6) and at the post (3).

### Landmarks and sightlines

From the valley, the lift's cable and head frame over the quarry and the watchtower over the ridge's west shoulder (its
deck at +62 m clears the crest from Main Street) say "up there". From the lift head, two ways at once: the watchtower
95 m west on the Rim, and the tramway north-east to the spoil heap and the mouth; the pine stand and the camp break the
line between them, so each arrival is its own. The watchtower has the area's second great view (the valley from the
north, the canyon, the Bench below). No view of the *Lily*: Larkspur High's crest closes the north.

### The way back down, and the boundary

- **The lift**, both ways, from N1 on; and **fast travel** between the mountain's two graves and the valley's.
- **The Goat Steps**, a one-way drop: nine rock ledges, each 4–5 m down and 3–4 m deep, from the crest at (4, 136) down the
  quarry's west wall to its floor at (4, 110). Each drop is too tall to mantle back up (a mantle reaches 1.5 m, a jump and
  a mantle about 2.7 m), too short for fall recovery's 30 m rule, and too tall for a creature to follow (they won't step
  off more than 4 m).
- **The boundary** (one polygon): the valley's north edge stays closed with its walls, except the quarry, which becomes a
  neck (x 0–36) up to the crest. The Bench is closed on the north (Larkspur High's band) and east (the East Shoulder) with
  walls behind, as today's ridges. Its west edge (the Rim) and its south edge (the crest's lip over the ridge face) are
  open: going over either is Hob's catch, as off the Rim today. The ridge face between the valley and the Bench stays
  outside the polygon, so nobody climbs it. The mine's tongue joins at the portal.

### Performance

- **The heaviest new view** is the watchtower's deck looking south-east: the whole valley from the north (120–300 m off),
  the canyon, the Bench in front. Ransom's Point's tower runs about 6.5 ms today; this one is planned at 7.0 ms or less
  (GPU at most 7.5 ms, about 700 draws).
- **How it holds:** the Bench gets at most 70 draws (pines instanced with last-LOD cards, no masked leaves past 100 m;
  the cabin, tower and Diggings merged per material with LODs; the tramway instanced); the valley is seen at its far LODs;
  the mine's interior is hidden while the player is outside, and the outside while inside (**new** `AInteriorVisibility`
  at the portal); one spline mesh per cable span; head lamps are emissive cards; at most 3 shadowless point lights in any
  view; no far shadow cascade. The bigger core adds about 40k triangles, mostly steep ground seen from afar.
- **Measured** (Medium, editor closed): the watchtower over the valley, the lift head over the Bench, the ride's top, the
  Diggings, the upper gallery with 6 dead miners, the stope with the Foreman and 6 adds, the cellar with 4 Unpaid, the
  depot's fight, and Ransom's Point again (the lift and tower join its skyline); the mountain at dusk too, since Main 6
  and 7 play at dusk and a player may go up then.

## 7. The valley grows a little

Only where the new missions and the house kit need ground:
- **The Old Quarry** is cut deeper and wider into the ridge's foot, from y ≈ 100 to y ≈ 120 across x 0–40 (about +900 m²
  of floor), with two or three benches stepped up its walls. It holds the winch house, the cage's landing, the lift-men's
  plot (a save grave), the QuarryCrew fight's floor (about 20 × 18 m with ore cars, cut blocks and the derrick ruin as
  cover) and the Goat Steps' foot. Its zone keeps today's levels 5–6; the quarry track's patrol stays.
- Inside today's boundary: **Pen Lane**, a back lane south of Main Street between the stock pens and the store's yard,
  with two new homes at (22, −28) and (36, −30); a **dugout** on boot hill's south slope at (−12, 22); **the depot**
  rebuilt larger on its lot (14 × 8 m, from 10 × 6) with its rooms inside and a coal stage by the water tower;
  **Bright & Daughter's workshop** behind the undertaker's front at about (62, −24), over its cellar; and **Amos's house**,
  small and empty, at (20, −72) west of his barn, with his grave beside it at (16, −66).

## 8. Hob's roosts (were the save graves)

**Superseded (2026-10-13, the user's call):** the save points are Hob's roosts, not graves: the Crow's Mast (look sheet
41, design B), the one save, respawn and fast-travel point on every map, its beacon in Hob's Ember once found. Walking
within 9 m with a clear line finds one (the column shoots up, the bell rings, the crows lift, ROOST FOUND, an early
save). Continue wakes at the area's last found roost; a death is dragged by Hob's crows to the nearest found one; fast
travel is Hob's crows carrying Ellis between found ones. Placed from `layout.json` `gameplay.roosts`
(`build_area_roosts.py`); the grave wake in the cold open stays a scene, and the first roost stands by the farm.

| Roost (id) | Name | Mast at (m) | Faces | Where | Opens |
|---|---|---|---|---|---|
| FamilyPlot | Ransom Farm | (−74.4, −34.5) | 95° | outside the family plot's east gate by the farm path | found as Ellis leaves the plot |
| MainStreet | Main Street | (30.5, 0) | 90° | the street's middle, 6.5 m east of the memorial | anytime |
| ChapelYard | Chapel of Saint Ada | (−7, 52.5) | 120° | the knoll's forecourt east of the churchyard gate | Main 4 |
| Depot | The Depot | (102.5, 7.5) | 135° | the yard road's end at the platform's west end | anytime |
| KeepersGrave | Keeper's Crossroads | (−110, 9) | −81° | the crossroads, facing the Keeper's Gate | Main 5 |
| WhitlockFields | Whitlock Fields | (46.5, −51) | −150° | by the barn yard's NE corner and the fields path | anytime |
| SinkRim | The Sink | (43, 33) | 59° | the SW rim by the fence's end, short of the ramp head | anytime |
| OldQuarry | Old Quarry | (30, 95) | 0° | the quarry mouth where the track enters | anytime |
| LiftHead | Lift Head | (18.5, 147) | 180° | the crest, where the crest path and the Bench trail part | (the lift) |
| CaldersPost | Calder's Post | (−53, 163.5) | −100° | the post's yard entrance from the Bench trail | (the lift) |
| Diggings | The Diggings | (57, 213) | 45° | between the miners' plot and the ore bin, facing the mine | (the lift) |

The ids FamilyPlot, ChapelYard and KeepersGrave are the old graves' (saves keep them).

## 9. Buildings

### Every building, and its model

No house model is used more than twice; Skyreach keeps today's set and shares none of these. Placement keys are
`layout.json`'s (the lived-in households keep their lights, smoke and town-life sounds).

| Where | Building | Model | Uses |
|---|---|---|---|
| The farm | Delia's farmhouse | SM_Farmhouse_Ransom (existing, RR's own) | 1 |
| The farm | the Ransom barn, the bench against it | SM_Barn_Ransom (existing, RR's own) | 1 |
| The farm; Main Street's west end by the gate | wells | SM_RR_Well (new: stone curb, windlass, shingle roof) | 2 |
| The farm; Calder's Post | outhouses | SM_RR_Outhouse (new) | 2 |
| Main Street N, (0, 11) | Providence Mining Co. assay office, shut since the fall | SM_RR_Front_Assay (new false front) | 1 |
| Main Street N, (12, 13) | the sheriff's office, walk-in | FalseFront_Sheriff (existing) | 1 |
| Main Street N, (25.5, 11) | the Haskett house (lived in) | SM_RR_Saltbox (new) | 1 |
| Main Street N, (34, 15) | the Gilt Spur saloon, boarded | FalseFront_Saloon (existing) | 1 |
| Main Street N, (44, 11) | the Larkspur House hotel, boarded | SM_RR_Front_Hotel (new false front, two storeys) | 1 |
| Main Street N, (57, 11) | the company manager's house, empty | SM_RR_IHouse (new: a two-storey I-house, double porch) | 1 |
| Main Street S, (2, −11) | an empty cabin | SM_RR_BoardBatten (new: board-and-batten, lean-to, stovepipe) | 2 |
| Main Street S, (13, −11) | the barber's (SHAVE · BATH · TEETH), shut | SM_RR_Front_Barber (new false front) | 1 |
| Main Street S, (30, −14.5) | Pruitt's General Store | FalseFront_Store (existing) | 1 |
| Main Street S, (50.5, −11) | the Mercer house (lived in) | SM_RR_StoneCottage (new: fieldstone below, timber gable) | 1 |
| Main Street S, (60, −14) | Bright & Daughter, Undertakers | FalseFront_Undertaker (existing) | 1 |
| Behind Bright & Daughter | the coffin workshop and cellar, enterable | SM_RR_Workshop_Bright (new) | 1 |
| Pen Lane, (22, −28) | an empty house | SM_RR_Shotgun (new: one room wide, deep, gable to the lane) | 1 |
| Pen Lane, (36, −30) | an empty house | SM_RR_Dogtrot (new: two log pens, an open breezeway) | 1 |
| Boot hill, (−12, 22) | a dugout | SM_RR_Dugout (new) | 1 |
| Calder's Post | the Rangers' bunkhouse, shut | SM_RR_BoardBatten (second use, other trim) | 2 |
| Whitlock Fields | Amos's barn | SM_RR_Barn_Whitlock (new: a long, low hay barn with an open bay) | 1 |
| Whitlock Fields | the windmill | SM_RR_Windmill (new: a timber tower; the steel one stays Skyreach's) | 1 |
| Whitlock Fields, (20, −72) | Amos's house, empty | SM_RR_Homestead (new: a small frame farmhouse) | 1 |
| The undertaker's yard | the coffin shed | CoffinShed (existing) | 1 |
| The depot | the station house, enterable | SM_RR_Depot (new, replaces SM_Depot) | 1 |
| The depot | water tower; coal stage | WaterTower (existing); SM_RR_CoalStage (new) | 1; 1 |
| The chapel knoll | the Chapel of Saint Ada, enterable | Chapel (existing) | 1 |
| Ransom's Point | the lookout | Lookout (existing) | 1 |
| The Old Quarry | the winch house, enterable | SM_RR_LiftHouse (new) | 1 |
| The lift; Calder's Post | tower, head, cage, cable; the watchtower and Ruth's cabin (enterable), the lean-to | SM_RR_LiftTower, _LiftHead, _LiftCage, _LiftCable; SM_RR_Watchtower, _RangerCabin, _LeanTo (new) | 1 each |
| The Diggings; the Providence | portal, ore bin, powder house, smithy; the rooms | SM_RR_MinePortal, _OreBin, _PowderHouse, _Smithy; the SM_RR_Mine_* kit (adit, gallery, office, decline, drift, stope, manway, timber sets, barricade, the fall) (new) | 1 each |

Eight new homes (Saltbox, IHouse, BoardBatten, StoneCottage, Shotgun, Dogtrot, Dugout, Homestead) and Delia's farmhouse
make nine; three new fronts join the four. Skyreach's LogCabin leaves Ransom's Rest, and SM_Cottage_Ransom's four uses
on Main Street go to the new homes. Every new model goes into
`area_shape.py`'s `FOOTPRINTS` and `BUILDINGS`, and the town life's households map to their new models.

### Interiors

| Interior | Rooms and sizes | What's inside | Used by |
|---|---|---|---|
| Bright & Daughter's workshop and cellar | workshop 10 × 7 m; cellar 12 × 8 m, 2.6 m stone vault, down a stair from the workshop's corner; the cold room 4 × 4 m at its back | coffin benches, lumber racks, tool wall, a stove, the measuring hook, the orders slate; coffin racks, four coffins on trestles; Thaddeus's bench | Knocking Below |
| The depot | waiting room 9 × 8 m; ticket office 5 × 4 m behind the grille; baggage room 5 × 4 m onto the platform; a stair to the agent's door upstairs (closed) | benches, a stove, the schedule board, a clock; desk, till, ticket rack, telegraph key; trunks and Fenn's coffin | The Old Fare; Dead Lift's coal; the station board by the door, as today |
| The winch house | 10 × 8 m, one room open to the cage's landing | boiler and firebox, the winch drum and brake, the coal bin, the speaking tube, Hiram's stool, a footlocker | Dead Lift |
| Ruth's cabin | 6 × 8 m and a lean-to | bunk, stove, table with the post's log and Ruth's journal, the oil, the code book, her Rim map, a footlocker, the fourth cache | The Last Watch |
| The watchtower | 4 × 4 m, 12 m, an inside stair, a roofed deck | brazier, heliograph, spyglass mount | The Last Watch |
| The Providence | above (section 6) | above | Back Pay |
| The sheriff's office, the chapel | as today | the crank and Odell's report in the lockup; the keepers' roll in the nave | Dead Lift, Calder's Caches; Main 4 |

## 10. Experience and levels

**Superseded in part (2026-10-13):** the user set a full clear at about level 6.8 and the story alone at about 5.5, with the
Foreman at 7 (section 13). The sums below are the draft's; build step 4 redoes them to those targets.

**The rules** (`Docs/Progression.md`): kill experience = 10 × the kill scale × 1.08^(level − 1) × the rank's multiplier
(Restless 2, Gravebound 4, Soulfed 10, Boss 20), less 15% for each level the creature is below the player; missions give a
fixed amount at the turn-in. Levels start at 100 (2), 212 (3), 337 (4), 477 (5), 634 (6), 810 (7), 1,007 (8), 1,228 (9) and
1,476 (10).

**The change:** the kill scale 0.37 → **0.33**, and the side missions' experience as in section 2 (Side 1 35 → 30, Side 2
40 → 35, Side 3 40 → 35; the new ones 15–45). The main missions keep theirs (250 in all).

**The new kills** (at today's 0.37, before the change, to sit beside Progression.md's table): The Old Fare's waiting room
(levels 2–3; three Unpaid, a Restless one, a Bell-Ringer) about 24; Knocking Below's cellar (its spawner's 3–4; three and a
Restless one) about 21; the mountain and the quarry (47 kills: 19 at 5–6 and 2 Restless there, 22 at 6–7 and 3 Restless
there, the Foreman at 8, about 5.2, 11.3, 5.7, 12.2 and 63 each) about 346, about 312 after the falloff as the player
climbs past them.

| Play | Kill XP at 0.37 | at 0.33 | Missions | In all | Level |
|---|---:|---:|---:|---:|---:|
| The story only: the mains, the fights on their way, the roaming dead once | 439 | 392 | 250 | 642 | 6.04 |
| The story and Sides 1 and 2 (Progression.md's "normal" today: 794, 6.91) | 471 | 420 | 315 | 735 | 6.57 |
| The story and every valley side mission (adds Calder's Caches, The Old Fare, Knocking Below) | 516 | 460 | 380 | 840 | 7.15 |
| Everything: also the optional camps (91), the Gravemother and her brood (90), the mountain (312) | 1,009 | 900 | 525 | 1,425 | 9.79 |

So the story ends about level 6, and about 7 with the valley's side missions, as asked. A full clear ends near 9.8, at
the *Lily*'s floor (Story.md gives it 9–15; that band was written when Ransom's Rest ran 1–10, and should start nearer 7
when the *Lily* is designed, so a story-only player isn't three levels under). Abel, at his set 7, meets a valley-sides
player at about level 5.6 and a story-only one at about 4.3 (4.5 today); a player who clears the mountain first meets him
near 8.7 and finds him easier. That's the price of the mountain opening mid-story (question 1).

**Zone levels** (`layout.json` `gameplay.levels`): the band 1–6 becomes **1–7**. New zones, listed before "valley":
quarry (now the bowl's polygon, 5–6, unchanged), crest 5–6, calderPost 5–6, bench 6–7, diggings 6–7, providence (the
interior's polygon) 6–7. The cellar's spawner keeps its own 3–4 (a spawner's level is kept, as today); the depot stays in
the valley's 2–3. New kind: ForemanCreature at 6 (Soulfed's +2: 8). **When it opens:** the lift after Main 5, with the
Gravemother: a player arrives at about level 5.5 for the quarry's 5–6 and climbs to about 7 by the mine's 6–7; the
Foreman at 8 is the area's hardest fight, a Legendary over the boss as the Gravemother is over the Sink.

**The checks:** `Looter.Progression.Pacing` gets the four rows above and fails outside story only 5.9–6.4, valley sides
6.8–7.4, everything under 10. `Looter.Economy.Balance`: the valley's new purses (Calder's 30, Fenn's 30, Thaddeus's 40)
and their fights and containers raise a normal play's silver by about 180, to about 1,450, inside its 1,000–1,800; the
sheriff's bounty (80), the mountain's purses and the kept payroll (about 400) come on top for an explorer.

## 11. Canon changes once approved

**`Docs/Story.md`:**
1. *What changed*: the Setting row's "Skyreach stays a floating island, outside the story" becomes "Teropa has one sky
   island, Skyreach, reached from the north side of the *Gilded Lily* by a skylift (area 3)". The Tutorial island row and
   the Leaving the tutorial row are replaced by one row: "The opening: a new game starts on Ransom's Rest's cold open; the
   controls are taught by hints in Mains 1–3. Skyreach, with Crossroads Town, becomes area 3."
2. *Writing notes*: "Never call a story place an island; only Skyreach is one, and it is outside the story" becomes "Never
   call a story place an island but Skyreach, Teropa's one sky island."
3. *Cold open*: its first sentences become "It plays as soon as a new game starts, and can be skipped. The title REVENANT
   rises through the white of a bank of evening cloud, and the white thins on the Dunne Gang's skiff gliding out of it."
   The cloud match cut (a tutorial skiff sailing into cloud) is retired; the cloud bank, the dark skiff and the rest stay.
   Ellis comes out of the grave with the Common Bullpup they were buried with.
4. *The world*, "Outside the story" paragraph: replaced by "Skyreach, the sky island. North of the *Lily*'s country one
   island floats over the hills, and a skylift climbs to it. Nobody on Teropa knows why it floats." (Its story comes with
   area 3's design.)
5. *Getting around*: "Skiffs are rare: the tutorial's, and the Dunne Gang's" becomes "Skiffs are rare: the Dunne Gang's
   is an outlaw's airship"; add "A skylift from the *Lily*'s north side climbs to Skyreach."
6. The sections *The tutorial island, outside the story*, *Leaving the tutorial island* and *Back to Skyreach for
   practice* are replaced by a short *Skyreach, area 3*: the island and Crossroads Town stay; the tutorial, the practice
   trips, 0 XP and ammo-only drops go; Hob comes along; its band and story wait for its design.
7. *Ransom's Rest (summary)*: its size becomes "about 309 × 350 m and 64,000 m², with Larkspur Bench, Calder's Post and the
   Providence mine up the ore lift"; beat 1 becomes "Seven Days. The cold open, the grave, Hob, and Delia through the screen
   door."
8. *Campaign gating* and Decision 8: every "Skyreach (practice)" line on the station boards goes. A station board lists
   every opened area and one blank line naming the mission that opens the next; Ransom's Rest's reads "THE GILDED LILY —
   Finish 'The Lantern Leans'" until Main 7, then "The *Gilded Lily*". Skyreach is listed only once opened, from the
   *Lily*.
9. *Areas* table: Skyreach's row moves third (after the *Lily*): "Opens after: a mission on the *Lily* (to design), by its
   skylift; levels: to set". Ransom's Rest: "Opens after: a new game"; enemy levels 1–7; Legendary monsters the Gravemother
   and the Foreman. The *Lily*: a note that its 9–15 may start lower.
10. *Enemy ranks*: the Legendary row's "at most one alive per area" applies to random Soulfed promotions; hand-placed ones
    are as an area's design names them (Ransom's Rest: the Gravemother and the Foreman).
11. *The game's systems*: Death and respawn becomes "Graves are save points: walking up to one lights its lantern. A death
    wakes Ellis at the nearest open grave, Continue at the last marked one, and fast travel joins them; ten on Ransom's
    Rest." Mission markers adds "a gold or white ! over whoever offers a mission, a ? over whoever it's turned in to".
    *Practice on Skyreach* is removed.
12. *Signature moments*: "A skiff sliding into a cloud bank, and out of another one seven days earlier" becomes "The title
    rising through cloud that parts on the gang's skiff, seven days ago."
13. *Decisions*: decisions 4–6 are marked superseded, and a 2026-10-13 entry records the map order, the skylift, the
    opening, the givers, the save graves, the mountain and the house kit.
14. *Cast*: add Thaddeus Bright (Tilly's dead father, in her cellar), Hiram Coyle (the lift's dead engineman), Mr. Sloane
    (the station agent), Josiah Fenn (a dead peddler bound for Gravesend), Mrs. Pruitt (the storekeeper, the late
    sheriff's sister) and Cyrus Teague, the Foreman.

**`Docs/Areas/RansomsRest.md`:** *At a glance* takes the new size, play time (main path about 75 minutes, about 3 hours
15 with everything), levels 1–7 (the Gravemother 6, Abel 7, the Foreman 8), "7 main, 9 side", both Legendary monsters,
and an arrival of "a new game: the cold open, then the grave"; *Size and shape* takes section 6's north, the neck and the
tongue; *Zones*, *Encounters*, *NPCs* (Thaddeus, Hiram, Mr. Sloane, Fenn's coffin, Mrs. Pruitt; Hob and Pa as givers) and
*Tour viewpoints* take sections 4, 6 and 9's additions; *Missions* points here; *The hub afterwards* lists the ten graves;
every "Skyreach (practice)" line goes from *The exit and the unlock*, *Tech needs* and *Decisions for you*, with a pointer
to the 2026-10-13 decisions. `Docs/Progression.md` takes section 10; `Docs/TutorialIsland.md` and
`Docs/Polish/TutorialRework.md` get a line saying the tutorial is retired.

## 12. Build steps

Each step ends with something to look at or play. Steps on one line run side by side on disjoint files against a shared
contract written first.

1. **Wave 1** (running): the hold ring, the compass, a new game on the cold open with the hints in Ransom's Rest and the
   pace 0.9, the givers, graves as save points; the art agent's house look sheet (`35_Homes.jpg`). *You approve:* this
   document and the house look sheet.
2. **In parallel, after the approval:**
   - **2a. Mission features** (code): optional objectives (`bOptional`, the turn-in bonus, "Optional:" on the tracker and
     page); mission items (`AMissionItem` pickups, the Missions page's Carried list, a named creature dropping one); a
     turn-in with a choice of two rewards (`FMissionTurnIn` choices, the card's two buttons); speaker points as turn-ins
     anywhere (a coffin, a speaking tube, a tag board, Pruitt's hatch); a height band on `ASafeGround`; the minimap's
     UNDERGROUND plate; the porch plate, keepers' roll and other small interactables. Tests per rule.
   - **2b. The ore lift** (code): `AOreLift` (the cage on its cable, two stations, the bell pulls, the call), the ride
     scene from `SkiffRide`'s pattern, Board.Lift, saves held during the ride; `AWatchFire`, the far answering light,
     `AInteriorVisibility`. Tests: a ride both ways, a call, a save after.
   - **2c. Creatures** (code): `ADeadMinerCreature` (the swing, the lamp's weak spot, working idles, barks),
     `AForemanCreature` (`UBossComponent`: fuse sticks, the roof, the shift, the dark, the show and last word), Hiram and
     Thaddeus as story characters (pose tables like Amos's), Ledger pages for all four. CastShots for each.
   - **2d. Art, the house kit**: the eight homes, three fronts, well, outhouse, windmill, Whitlock barn, coal stage; then
     **2e. interiors**: the workshop and cellar, the depot, the winch house, the cabin and the watchtower; **2f. the
     mountain kit**: lift (cage, tower, head, cable), the Diggings, the mine's rooms; **2g. characters**: the dead miners'
     costume set, the Foreman, Hiram and Thaddeus (a look sheet to you first for the dead miners and the Foreman); **2h.
     props**: every artifact and item (crank, sack, pin, key, tape, coins, tags, fang, badge, hat, journal, code book,
     payroll book, the Deacon's slip). Two art agents at most, in this order.
   - **2i. Terrain**: the core grown to 512 m with today's valley compared within 1 cm; the Bench, Larkspur High moved,
     the East Shoulder, the quarry bowl, the Goat Steps, the knoll and tarn, pads and paths; the boundary with its neck,
     open edges and the mine's tongue; the scatter's alpine meadow and larkspur; `terrain_check` and the identity check
     for the tutorial island and TerrainTest. *You see:* renders from the lift head, the watchtower and the valley.
3. **The level** (after 2d–2i): the houses swapped and the new lots, the mountain's buildings and interiors, the lift,
   the ten graves, the encounters in `layout.json`'s gameplay, the loot (`build_area_loot.py`), the town life's
   households, lights and smoke; props re-settled; path probes on the Goat Steps, the neck, the Bench's paths and the
   mine. *You play:* the lift up and the Goat Steps down.
4. **The missions' data** (after 2a–2c and 3): all sixteen in `create_mission_assets.py` and
   `create_side_mission_assets.py`, every line in `create_story_lines.py`, the Ledger pages, `gameplay.levels` (band 1–7,
   the new zones, the Foreman's kind), the kill scale 0.33, the pacing and economy tests. *You play:* the opening to the
   end of Main 3, then the rest.
5. **Sounds** (beside 3 and 4, once 2a–2c name their cues): the lift (winch, steam, whistle, the cable's hum, the cage's
   clank and bell), the mine (drips, creaking timbers, distant picks), the dead miners' voices and swing, the Foreman
   (fuse hiss, roof fall, the dark), the cellar's knocking, the depot's clock, the watch fire, the new mutters.
6. **Checks**: every `Looter.*` test; tours with section 6's views, perf with the editor closed on the watchtower's view;
   HudShots (the offer card, the ! and ?); a full play by the Demo agent from a new game to the station board, the
   mountain included; then `Docs/Story.md`, `Docs/Areas/RansomsRest.md`, `Docs/Progression.md`, `CODEMAP.md`,
   `Docs/Pipeline.md` and the handoff. *You approve:* Ransom's Rest, expanded.

## 13. Open questions

**Decided by the user (2026-10-13), overriding the text above where they differ:**
1. The ore lift opens after Main 5 (as recommended).
2. **Experience: a full clear leaves the player at about level 6.8, the story alone at about 5.5** (the user's own answer,
   not section 10's 9.8). So section 10's sums are redone in build step 4: the kill scale and every mission's amount come
   down, most of a side mission's reward is silver and guns, and the mountain's levels drop with it: the zones 5–6, **the
   Foreman at a set 7** (5 and Soulfed's 2), level with Abel. `Looter.Progression.Pacing` checks story only 5.3–5.8,
   everything 6.6–7.0.
3. Back Pay ends in the choice (as recommended).
4. Thaddeus Bright is in the cellar (as recommended).
5. Ruth saw it all through her glass and closed Ellis's eyes (as recommended).
6. The Goat Steps and the lift both (Main's call on the recommendation).
7. The town board has two postings now (Main's call on the recommendation).
8. Hiram Coyle in person (as recommended). The main menu's subtitle says Teropa, not Skyreach.
The homes kit (look sheet 35, 14 models) is approved for the game.

Tap-to-answer, the recommendation first (as they were asked):

1. **When does the ore lift open?** (a) After Main 5, with the Gravemother: mid-story, the mountain at 6–7 and the Foreman 8
   (recommended). (b) After Main 4: earlier, the mountain at 5–6 and the Foreman 7. (c) After Main 6: Abel stays the
   hardest fight for every player, and the mountain is the last stretch before the train.
2. **Where should a full clear leave the player?** (a) Near level 9.8, at the *Lily*'s floor, with the story at 6 and the
   valley's sides at 7 (recommended). (b) Near 8.5: the kill scale 0.30 and the mountain's missions halved (the story drops
   to about 5.8).
3. **The Providence's ending?** (a) A choice at the tag board: pay the eleven (they rest, a Rare gun) or keep the payroll
   (about 400 silver, the dead keep working) (recommended). (b) No choice: Ellis always pays them.
4. **Tilly's father in her cellar?** (a) Yes: Thaddeus Bright, a friendly dead man at his bench, and the tape for Tilly
   (recommended). (b) No: the cellar's dead stay nameless and the turn-in is plain.
5. **Ruth's journal?** (a) She watched the robbery through her glass, saw Ellis draw on the Deacon, and closed Ellis's eyes
   at dawn (recommended: it seeds her Act I truce). (b) She saw only the skiff, and the night stays unseen.
6. **The way down?** (a) The lift both ways, plus the Goat Steps, a one-way drop of rock ledges into the quarry
   (recommended). (b) The lift only.
7. **The town's bounty board?** (a) Two postings now (Calder's note and the sheriff's bounty), repeatable bounties with the
   Ledger's endgame later (recommended). (b) Also three small bounties now on today's camps (the orchard's edge, the sheep
   fold, the Webwood).
8. **Hiram Coyle in person?** (a) Yes: the lift's dead engineman by his boiler, a friendly dead man like Amos, in the dead
   miners' costume set (recommended). (b) A voice in the speaking tube only, and the lift posted on the Rangers' board.
