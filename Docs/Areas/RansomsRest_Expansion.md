# Ransom's Rest: the expansion

Design of Ransom's Rest's north mountain and the reworked missions, for the user's asks of 2026-10-13. Approved by the
user (their answers: section 13). Section 11 lists the edits still owed to `Docs/Story.md` and `Docs/Areas/RansomsRest.md`.
Wave 1 system names: `EMissionStart::Offered`, `FMissionOffer`, `AMissionArtifact`, `ARespawnMarker::MarkAfterMission`.
History and superseded drafts: `Docs/Archive/RansomsRest_Expansion_history.md`.

**Coordinates** are the area doc's: metres, x east, y north, origin about 11 m east of the town gate; heights from the
valley's base (Z = 0). `layout.json` is centimetres with X north and Y east: X = y × 100, Y = x × 100.

**Built as of commit 1ae1d99:** the 512 m core, Larkspur Bench, the Old Quarry's ore lift, Calder's Post, the Diggings
and the Providence mine, the dead miners and the Foreman (set level 7), Hiram, Thaddeus, all missions from givers, Hob's
roosts, Lost Loot at the depot, the XP curve, Larkspur Falls into Gravewind Canyon, Keeper's Bridge grounded. What's
left: `Docs/Handoff.md`.

## 1. Summary

### The decisions

The user's calls (2026-10-13):
- **Map order:** 1 Ransom's Rest, 2 the *Gilded Lily*, 3 Skyreach. The *Lily* lies behind the north mountain, reached by
  Tilly's train as today (the mountain is optional, not the way to it; the *Lily* isn't built yet). Skyreach, today's
  tutorial island with Crossroads Town, becomes area 3: a sky island reached from the north side of the *Lily* by a
  skylift. Teropa gets one sky island. The main menu's subtitle says Teropa, not Skyreach.
- **The opening:** a new game plays the cold open at once (skippable), then the grave, then Main 1. Skyreach's tutorial
  and "Skip the tutorial" go; contextual hints in Mains 1–3 teach the controls (section 5). Ellis is buried with a
  Common Bullpup. The first minute: the screen is white, REVENANT rises through a bank of evening cloud, the cloud parts
  on the Dunne Gang's skiff gliding out of the west, and the cold open plays as written; then the grave, Hob, Delia's
  door.
- **Missions:** every mission is picked up from a giver (someone at a speaker point, a notice board, or an artifact
  found in the world) under a ? (gold for main, white for side), accepted on an offer card with a 2–4 sentence briefing,
  and turned in under a ! (the other way round from Borderlands). The Missions tab lists only accepted missions. Main 1
  stays automatic. Every mission is reworked for substance; five new side missions join them.
  - Givers: Delia, Tilly, Aldana, Amos, Hob, Pa on his board and four new voices (a dead engineman, Tilly's dead father,
    the station agent, a dead peddler in his coffin); two boards post work; a poster, a note and a miner's tag start it.
  - Each mission has a reason to go, a discovery or a twist between its steps, a talk, an optional objective and a
    turn-in that makes sense. Nothing finishes on "do task, do task, done".
- **The unlock:** the Old Quarry's dead ore lift at the foot of Larkspur Ridge. A side mission (Dead Lift) finds its
  winch crank, coal for its boiler and its cable's clamp pin, then the player rides it up. It opens after Main 5.
- **The mountain:** about +50% of the old 42,800 m², one level, 3–4 zones, 30–40 minutes with its missions: a Rim
  Rangers' outpost (Ruth Calder's watchtower and cabin, a board) and an old silver mine (a tunnel interior, fights,
  loot). Not a quarry works at the lift top, not a view of the *Lily*. A second storey: Larkspur Ridge gets a hanging
  valley on top, Larkspur Bench, with Calder's Post on the Rim and the Providence silver mine under the high ridge. The
  ore lift goes up; the lift (both ways) and the Goat Steps come down.
- **Mountain enemies:** dead miners, a new kind of the Unpaid on its rig (miners' gear, head lamps, picks), and the
  Foreman, a named Legendary monster like the Gravemother (set level 7, a Ledger page, a Vigil Stone after).
- **Save points:** Hob's roosts (section 8), not graves.
- **Experience:** the story alone ends about level 5.5, a full clear about 6.8; the mountain is 5–6 (section 10).
- **Story:** Back Pay ends in a choice; Thaddeus Bright is in Tilly's cellar; Ruth saw it all through her glass and
  closed Ellis's eyes; Hiram Coyle in person; the town's bounty board has two postings now.
- **Houses:** Ransom's Rest gets its own frontier house kit (8–10 homes, new Main Street fronts, its own well, outhouse
  and windmill), no model used more than twice; Skyreach keeps today's set until its own rework. The homes kit (look
  sheet 35, 14 models) is approved for the game.
- **Interiors:** the lift house, Ruth's cabin and watchtower, the mine's galleries, the depot's waiting room and ticket
  office, Tilly's coffin workshop and cellar (the sheriff's office and the chapel already are). Not the saloon or the
  empty homes.
- **Pace:** the player is 10% slower; the main path grows from about 60 to about 75 minutes (section 2).
- **Lost Loot** (the user's add): ground guns despawn after 5 min into the depot's LOST PROPERTY window; oldest out when
  full; all guns. Built: 30 slots, +10 per upgrade for 300/600/1000/1600/2400 silver (doubled 2026-10-14, the user's call).

## 2. The mission flow

In the order a player meets them. Minutes include walking. Experience: section 10. A giver's ? (gold main, white side)
shows on the map, the minimap and over the giver's head once the mission's prerequisite is turned in; a mission ready to
turn in shows a ! at whoever takes it.

| # | Mission | Giver, and where | Offered by | After | Min | Rewards | Turned in to |
|---|---|---|---|---|---:|---|---|
| M1 | Landfall (since 2026-10-31; was Seven Days) | none: a new game, on the Lamplighter's deck | automatic | — | 12 | 450 XP, 20 silver; the porch plate's 2 jars of grave salt free after it | Delia Ransom, on her porch |
| M2 | The Ferryman's Price (since 2026-11-01; was Shall We Talk Business?) | Hob, on Delia's porch rail | on the move | M1 | 12 | the Warden's Log's story pages | Mister Sexton, at his ferry under Gravewind Point |
| M3 | Under New Management (since 2026-11-01; was Cold Welcome) | Delia Ransom, on her porch | met first | M2 | 12 | Uncommon gun, 40 silver, Abel's lantern bail | Tilly Bright, at her window |
| C | Calder's Caches | Ruth Calder's note on the Rangers' board | artifact | M3 | 12 | 30 silver, three caches and the gang's strongbox | the Rangers' board |
| S1 | Wanted: Already Dead | the poster on the town gate's post | artifact | M3 | 10 | Rare gun, 50 silver | Tilly |
| V2 | The Old Fare (new) | Mr. Sloane, the station agent, through his door upstairs in the depot | talk | M3 | 12 | Rare Drover revolver, 30 silver | Josiah Fenn, in his coffin |
| V3 | Lost and Found (2026-10-27) | Mr. Sloane, at his ticket window | talk | V2 | 4 | 20 silver; Lost Property opens | Mr. Sloane |
| M4 | A Coal Under Ash (since 2026-11-01; was Hallowed Ground) | Tilly, at her window | on the move | M3 | 13 | 1,000 XP (1,300 until the 2026-11-01 Pacing trim), Saint Maud Uncommon (Hallow), 40 silver | Father Aldana, at the vestry door |
| S2 | Unfinished Business | Amos Whitlock, at his fence | talk | M4 | 12 | Epic gun, 60 silver | Amos |
| V1 | Knocking Below (new) | Tilly, at her window | talk | M4 | 12 | Rare gun, 40 silver | Tilly |
| M5 | Choke Point (since 2026-11-01; was The Keeper's Lantern) | Father Aldana, at the vestry door | on the move | M4 | 13 | 1,400 XP (3,000 until the 2026-11-01 Pacing trim), 50 silver | Brother Finch, at the Reliquary |
| S3 | The Gravemother | Sheriff Odell's bounty on the Rangers' board | board | M5 | 10 | 80 silver bounty; her Legendary table every kill | Mrs. Pruitt, at the store's hatch |
| N1 | Dead Lift (new; **main** since 2026-10-27) | Hiram Coyle, the lift's dead engineman, in the winch house | met (talk) | M5 | 15 | Rare gun, 40 silver, the ore lift | Hiram, through the speaking tube at the lift head |
| N2 | The Last Watch (new) | the board at Calder's Post | board | N1 | 12 | Calder's fourth cache, 60 silver | the post's board |
| N3 | Back Pay (new; **main** since 2026-10-27) | Hiram, up the lift head's speaking tube (was No. 7's tag, an artifact) | met (talk) | N1 | 22 | the Foreman's Legendary table; pay the eleven (Rare gun) or keep the payroll (about 400 silver) | the tag board |
| M6 | The Pilot Light (since 2026-11-01; was The Gravewind) | Captain Holloway, at the Lamplighter's wheel | met (talk) | N3 | 18 | 50 silver; the barge's manifest (Paid on Delivery) | Holloway, at the wheel |
| M7 | Given to the Wind (since 2026-11-01; was The Lantern Leans) | Holloway (one conversation) | on the move | M6 | 20 | Abel's boss loot, Heirloom by hand, 60 silver, the *Lily* on the station board, the hearse car | Delia, at the chapel door |

- **Since 2026-10-27 (the user's note 32):** the main path is M1–M5, N1, N3, M6, M7 (Cyrus Teague, the Foreman, the second
  boss; Abel the last), about 110 minutes; N1 and N3 are met first (no card), N3 straight after N1's turn-in at the
  speaking tube. V3 Lost and Found (note 37) opens the LOST PROPERTY window. Words and XP: `create_mission_assets.py`,
  `create_side_mission_assets.py`, `Docs/Progression.md`; `Docs/Areas/RansomsRest.md` "Missions".
- Main path about 74 minutes (before the mine joined it); the valley's six side missions about 68; the mountain (Dead Lift, The Last Watch, Back
  Pay, the walk and its camps) about 55, of which 35–40 on the mountain itself. About 3 hours 15 minutes for everything.
- The Rim Rangers' board at the sheriff's office becomes a bounty board with two postings now (Calder's note, the
  sheriff's bounty); repeatable bounties come later with the Ledger's endgame. Calder's Post has its own board.

## 3. The missions, reworked

- Each step names its objective kind in brackets, all existing kinds unless marked **new** (added for the expansion).
- "Optional:" objectives never hold a step; each done adds 10 silver (at level 1) to the turn-in, Borderlands' way, and
  no experience.
- Giver tags follow the wave 1 contract: `Speaker_<Name>`, `NoticeBoard_<Name>`, `Artifact_<Name>`.
- **Mission gates (2026-10-27, the user's note 38: "you can speed run quests and just spam click the use button"):** steps open
  in order. A step's items, readings, artifacts and fittings appear or work only from their step (`FStoryCondition` DuringMission +
  FromStep: `TakeWhen` on `AMissionItem`, `UsableWhen` on `AReadableProp`); the key actions (taking a mission item, a step's reading
  or fitting) need a hold of `Looter.Missions.HoldSeconds` (0.6 s; 0 = taps) instead of a tap. Lore readables stay a tap any time.
  Gated (steps counted from 0): Last Watch `Item_LampOil` taken from step 1, `Readable_CodeBook` step 4 only; Back Pay
  `Item_Payroll` seen from step 5 and taken from step 6, `Readable_PayrollBook` read from step 2, `Readable_Barricade` ("Take the
  tags", the prompt now goes once they are taken) step 4 only; Dead Lift `LiftFitting_Boiler`/`_Pin` step 1 only (1.5 s hold),
  `ManwayGate` Back Pay from step 5 (1.0 s); Knocking Below `Item_CellarKey` from step 0, `Item_ThaddeusTape` step 4; Old Fare
  `Item_FennsFare` step 4; Calder's Caches `Item_CacheTag1..3` from step 0, `Readable_DeaconsSlip` seen from step 1 and read from
  step 2; the gang's strongbox opens only from Calder's Caches step 1. Hold only: `Item_LanternBail`, the dropped `ClampPin` and
  `GravemotherFang`. Still taps (their own classes step-gate them): `AKeepersLantern`, `APorchPlate` jars, `Headboard_Abel`,
  Fenn's coffin, `AChapelReliquary`; `ColdRoomDoor` is a plain door behind the key-locked cellar door.
- Lines marked as today's are the pre-expansion ones, unchanged; the rest are this design's.

### Main 1: Landfall (automatic, about 12 minutes; since 2026-10-31)

The Wardens story's first mission (Docs/Story.md "The Wardens story"; the user: "a reason behind every action"); it
replaced Seven Days (the cold open, the grave, the headboards, Delia's screen door: their scenes compile but no new game
reaches them). Id stays `Main1`. 450 XP, 20 silver (`MissionRewards::SilverFor`). Golden hour, the valley under smog.
Contract and owners: `Docs/Work/2026-10-31-Wardens/CONTRACT.md`; words: `create_story_lines.py` (Landfall sets),
`story_texts.py` (Delia's and Hob's parts); data: `create_mission_assets.py`. Steps counted from 0, as the scripts count:
never reorder them (FLIGHT moves an old save's running Main 1 to step 5).

Before step 0 a new game plays the planet intro (new words: soul coal, Vane, the Wardens, the Lamplighter flying into the
smog) and opens on the Lamplighter's deck as she sails in from the west over Gravewind Canyon, the Bullpup in hand; the
crew talk from their stations (`DA_Lines_LandfallDeck`).

0. Go to the Great Lantern at the prow. [Reach `Lamplighter_Prow`, 3.5 m] Finch: it's leaning hard; teaches walking.
   Arriving, Vane breaks in on the ship's horn (`DA_Lines_LandfallVaneHorn`).
1. Fight off Vane's boarders. [Event `Landfall.DeckClear`] A Company tender grapples the ship and stops her; 3 riflemen
   and a hose-man board in two waves (2, 2); 2 riflemen stay on the tender firing across (uncounted). Teaches shooting,
   cover, the hose-man's tank (`DA_Lines_LandfallBoarders`, `DA_Lines_LandfallBoardersWave2`).
2. Cut the grapple lines: hold {Interact} on each chain. [Interact, held, `Landfall_Grapple` 2] Or the tender tows the
   ship to the barge; the tender's riflemen still fire (`DA_Lines_LandfallGrapples`).
3. Hold on. [Scene `HardLanding`, about 5 s, skippable] The tender falls away; the bag is torn; Holloway puts her down
   hard on the Mooring Ledge (`DA_Lines_LandfallLanding`).
4. Drop the anchor: hold {Interact} on the winch. [Interact, held, `Landfall_AnchorWinch`] + Fight off the dead. [Clear
   `LedgeClimbers` 4: Unpaid rising from the canyon side] Without the anchor she slides into the canyon; the smog stirs
   the dead (`DA_Lines_LandfallAnchor`, Hob's arrival on the ledge).
5. Climb to Ransom's Point. [Reach `Place_RansomsPoint`, 18 m, height counted] Holloway wants eyes on the valley; from
   high ground the Lantern sees what Sully sees (`DA_Lines_LandfallClimb`).
6. Look over the Rest. [Interact `Landfall_Spyglass`, tap or hold] Holloway first (`DA_Lines_LandfallPoint`). The scene
   `SmogReveal`: the barge over the Sink, its hose in the ground, the smog; Vane over the valley's loudspeakers
   (`DA_Lines_LandfallVaneValley`: your keeper is our pilot light); Finch: the Lantern has Abel, and his trail
   (`DA_Lines_LandfallTrail`).
7. Follow Abel's trail. [Reach `Speaker_Delia`, 18 m] The glow runs from the Point down the bluff path to the farm: his
   soul tore loose at the chapel door and ran home, and the gang reeled it in on Delia's steps. Optional: pick up Abel's
   hat. [Interact `Landfall_AbelsHat`: an AReadableProp `AbelsHat` on the bluff path, held; reading it sets the flag
   `Landfall.AbelsHat`] Hob points at it (the last line of `DA_Lines_LandfallTrail`).
8. Help Delia hold the porch. [Clear `FarmPorch` 8: 3 riflemen, 2 hose-men, 3 Unpaid, spawning within about 45 m of the
   farmhouse from step 7, from the orchard and the farm road] Delia fights from the porch and barks (`DA_Lines_DeliaFight`);
   Hob on the porch rail (`DA_Lines_HobLandfallPorch`).

**Turned in to Delia Ransom** (the turn-in card first, then her words): the mission has no turn-in lines; her topic says
them (`story_texts.TOPICS['Speaker_Delia']`, "during Main1 from step 9"): `DA_Lines_DeliaLandfallHat` with the hat's flag
(her best line: *"Forty years I told that man to tie it on."*), else `DA_Lines_DeliaLandfall`. Both: the hose at the
chapel door, his soul reeled in on her steps, bring him back to Saint Ada, the salt on her plate. Main 2 starts after it
(Hob's word, on the porch rail).
**The porch plate:** no Main 1 step now; its two grave-salt jars (grenades, the slip *SALT FOR THE ROAD...*) are a free
pickup from Main 1's end (`PORCH_PLATE` take_when after Main1); the biscuits stay Main 7's optional.
Abel's grave stays in the family plot; Ellis's grave and headboard went (WORLD).

### Main 2: The Ferryman's Price (Hob; about 12 minutes; since 2026-11-01)

The Wardens story's Main 2 (`Docs/Work/2026-10-31-Wardens/MAINS_2_7.md` §4, the user's call: Sexton sells the plan at his
ferry in the fog under Gravewind Point). Id stays `Main2`; the Log's step stays 3 (`Ledger::HandedOverStep`). 550 XP.
Data `create_mission_assets.py`; words `create_story_lines.py`; topics, perches, fights `story_texts.py`; world
`build_area_main2.py`; lane notes `Docs/Work/2026-10-31-Wardens/lanes/MAIN23.md`.
**Offer**, Hob on the porch rail, on the move: *"Man down on the burial boards wants a word, sunshine. Says he knows the
barge."* Sully: *"Who is he?"* Hob: *"The Ferryman. I flew for him eighty years. Don't take his first price."*
**Steps** (from 0):
0. Follow the keeper's path to Gravewind Point. [Reach `Place_GravewindPoint`, 25 m] On the way the Three Widows' nest
   (`BluffNest`, moved off the Point's top: 4 spiders + a Restless one, 2 out, two pushes; Hob's blue line as it's cleared).
   Optional: cross Keeper's Bridge [Reach `Place_KeepersBridge`]: Hob mutters as the player crosses (`Mutter_KeepersBridge`).
1. Stop the Company's hose-cart [Event `HoseCart.Stopped`: hold Interact on its valve 1.5 s, or burst its tank, 150
   damage on its tank hit] + fight off the crew [Clear `BoardsCrew` 8, count hidden]: 3 riflemen behind the biers and a
   hose-man at the cart (level 2, at work as the player comes up the cairns); the cart's spill brings 4 Unpaid rising round
   it (the spawner's 2nd wave, only on the cart's word); Hob: *"That's what the hose leaves, sunshine. Half a soul and a
   temper."*; then Vane from the Keeper's Gate loudspeaker: *"Wardens. That cart is Company property."* / *"I'm adding it to
   your bill."* Optional: silence that loudspeaker [KillNamed `Loudspeaker_KeepersGate`]: shot out mid-line it cuts him at
   *"your bi—"*. The cart is stoppable only in this step (its `pumping_when`).
2. Climb down to the Ferryman's landing and hear Mister Sexton's price. [Talk `Speaker_Sexton`] Down the rope ladder off
   the deck's west end (a 2.6 s climb scene) to a railed landing 3.8 m under the boards; the ferry hangs beside it on
   chains, swaying, its prow lantern lit, a fog bank under it. The deal: a barge must eat; starve her and her crew bring her
   down; a keeper's soul goes in his own lantern; *"Write down every soul you meet. I bill by the head."*
3. Open the Warden's Log: {Inventory}, then {Page.Bestiary}. [OpenPage Bestiary] Sexton's pages are in it.
**Turned in to Sexton** (card, then): his price, the barge's manifest (Paid on Delivery after Main 6); *"Come back when
you've silver. I know where things lie on the Rest."* (his secrets: MAINS_2_7.md §12, a later round). He stays at his
ferry after (shown from Main 2 on). The six shrouded dead stay on the boards until Main 7.

### Main 3: Under New Management (Delia; about 12 minutes; since 2026-11-01)

The Wardens story's Main 3 (MAINS_2_7.md §5). Id stays `Main3` (Main Street's safe ground and shutters, Pruitt's hatch
follow it). 1,000 XP, Uncommon gun or better, 40 silver. World `build_area_main3.py`.
**Offer**, Delia on her porch (met first): *"Tilly Bright laid my boy out. What the hose left of him."* / *"If his lantern
came home, she has it. Town's Company ground now. Mind yourself."*
**Steps** (from 0):
0. Take the farm road into town. [Reach `Place_TownGate`] As it starts (into a quiet moment), Lucky Ned's dark skiff casts
   off from Vane's barge and flies south over the Hogback and away (`AGangSkiffFlyBy`, about 22 s, once a campaign:
   flag `Main3.NedLeft`): Vane and Ned on the radio, then Hob: *"That's Lucky Ned, sunshine. He's the one put the hose to
   Abel."* Sully: *"He's getting away."* Hob: *"For now. Lantern never forgets a smell."*
1. Break the Company's checkpoint at the town gate. [Clear `TownGate` 7] Sandbags, a barrier pole and a VANE CONSOLIDATED /
   ASSESSMENT IN PROGRESS / NO ENTRY. NO EXIT. board across the gate; 3 riflemen behind the sandbags and a hose-man at the
   barrier, guarding from the road (the spawner on from step 0), then 2 Unpaid and a Restless one drawn by the smog, 4 out at
   most. Vane on the gate's loudspeaker as the fight begins: *"Ah, the Warden. Main Street is closed for assessment."* /
   *"Your dead are being counted. Please don't make me count you."* The barrier is gone after Main 3; the sandbags stay.
2. Get to Bright & Daughter. [Reach `Speaker_Tilly`, 8 m] Main Street's set piece (built), its second push gaining 2
   Dredger riflemen (layout edit); ASSESSED / n SOULS / V.C. chalked on the doors; a hose-cart assessing the Gilt Spur
   (burstable, no crew of its own); Vane on Main Street's middle loudspeaker once the street is won: *"Residents! Anyone
   hiding a Warden will be assessed first."* Optional: cut down the UNDER NEW MANAGEMENT banner [Event
   `MainStreet.BannerDown`: shoot both its ropes; it falls into the street and stays down].
**Turned in to Tilly Bright** (card, then): *"A Warden. Lord. Come close, the whole street's watching."* / *"His lantern
wasn't on him. Only its bail, snapped off in his fist."* / *"Couldn't open his hand till morning. Here. And the box: he'd want
it used."* She hands the bail over (Main 6's item `LanternBail`, carried: hand_items) and the box (the reward gun). After:
*"Every coffin in my cellar's salted, Warden. The hose won't have them."* (seeds Knocking Below).

### Main 4: A Coal Under Ash (Tilly; about 13 minutes; since 2026-11-01)

The design word for word: `Docs/Work/2026-10-31-Wardens/MAINS_2_7.md` section 6; as built: `lanes/MAIN45.md`
(create_mission_assets.py, build_area_chapel.py, build_area_main4.py). Why: a keeper's lantern is lit from his saint; is she
still alight? Ringing her bell tells the barge somebody's home.
**Steps (from 0):** 0 the chapel [Reach Place_Chapel]; 1 the yard [Clear ChapelYard 16: eight Unpaid, then the crew's two
hose-men, five Unpaid and the Gravebound Leftover] + the Company's hose out of the side window [Interact Hose_ChapelWindow,
held 2 s, or its cart stopped]; 2 the knock [Talk Speaker_Aldana]; 3 the bell [Interact Bell_Chapel, held]; 4 her ember
flares [Event GraveSight.Reliquary] + the barge's answer, a tender's drop on four ropes [Clear BellAnswered 6: four riflemen
level 3, two hose-men level 4] + optional the keepers' roll (every Ransom keeper since the saint, ABEL RANSOM the last).
**Turned in to Aldana** (card, then: she's a coal under ash; every grave open to the hose; Ned wouldn't break Abel's
lantern; he threw it down the Sink). Brother Finch stands at the Reliquary from then (Speaker_FinchChapel). The Reliquary
is whole, guttering (AChapelReliquary), its Tripo model M45-1 to come.

### Main 5: Choke Point (Aldana; about 13 minutes; since 2026-11-01)

The design: MAINS_2_7.md section 7; as built: lanes/MAIN45.md (build_area_sink.py, build_area_main5.py). Why: the lantern is
in the Sink, and so is the barge's mouth; one trip, two jobs.
**Steps (from 0):** 0 Junie's salt charges from Hob's satchel at the ramp head [Collect SaltCharges] + down [Reach
Place_SinkFloor, height counted]; 1 the three egg sacs [Event EggSac.Burst ×3]; 2 Abel's lantern [Interact Lantern_Keeper:
it must stay step 2]; 3 the Intake set piece [Interact Intake_Charge ×3, held 1.5 s, against its draw; two drops down the
hose (two riflemen and a hose-man each, the second on the second charge), three spiders down the ramp on the first; a 5 s
fuse, the draw doubled] + [Event Intake.Choked: the hose swings across the floor and hangs limp]; 4 out [Reach
Place_SinkRim, height counted] + optional the sheriff's hat (W. ODELL; the smog pool drains, the Gravemother growls: Side
3's hook). At the rim the barge lets a second hose down onto Larkspur High's new air shaft (until Back Pay is done).
**Turned in to Brother Finch** at the Reliquary (card, then: the lantern whole; it'll burn like a sunrise with his soul in
it; Junie: the second hose is down the mine's air shaft; Hob: the old ore lift). SmogThin from the turn-in.

### Main 6: The Pilot Light (Holloway; about 18 minutes; lane MAIN67, 2026-11-01)

The design: `Docs/Work/2026-10-31-Wardens/MAINS_2_7.md` section 10; as built: `lanes/MAIN67.md` (steps, tags and who
places each in `Docs/Work/2026-10-31-Wardens/lanes/CONTRACT67.md`). Starved of both hoses, the barge comes down 25 m over the chapel knoll at dusk
to hose Saint Ada herself. Met first at Holloway's wheel on the Mooring Ledge (accepting: dusk under thin smog,
`DuskSmog`). 0 cast off (the anchor winch, reversed); 1 the Lamplighter flies east over the farm and town and two Company
tenders attack (shoot their gas-bags; one boards three men); 2 the two starboard launchers grapple the barge, the plank
drops; 3 the barge's top deck (BargeDeck 9, vents flaring every 6 s) to the furnace hatch, + optional jam the hose drum
(`Pilot.DrumJammed`) and shoot out her four horns; 4 the manifest in the stoker's office (Paid on Delivery's item; Ned on
the radio); 5 the furnace hall: three draft valves under the turning hot grate ring, the Chief Stoker (Hallowed, 7) and his
crew in two pushes; 6 the soul cage: what comes out isn't Abel (CageBurst); 7 the barge dies, the jump to her deck, the
barge falls into Gravewind Canyon (BargeFall). Turned in to Holloway at the wheel as she flies home and sets down.
3,000 XP, 50 silver. After: the wreck burning in the canyon, soul coal strewn across the valley.

### Main 7: Given to the Wind (Holloway; boss Abel, corrupted; about 20 minutes; lane MAIN67)

The design: MAINS_2_7.md section 11. Started on the move after Main 6's turn-in. 0 the burial boards (+ optional free the
souls in 12 lumps of the barge's coal; the keeper's path); 1 Abel's lantern on the keeper's post; 2 the fight: Abel
Ransom, Corrupted (Smog bar, three gold "Abel" pips: at 66% and 33% the bar locks until a 1.5 s hold at the keeper's post
calls him back; tendrils; phase 2 drinks the six shrouded dead, + optional save all six; phase 3's fog surge; at zero a 3 s
hold); 3 FreeAbel (he steps into his own lantern); 4 carry him home to the chapel (WestRoadCrew's hose-men latch his
light); 5 set the lantern on the Reliquary (SaintAdaWakes: the smog burns off in a wave, the dead go west on the Gravewind,
Abel stays by her, Delia comes up the hill). Turned in to Delia at the chapel door: Heirloom by hand, the *Lily* on the
station board, the hearse car coupled. 3,000 XP, 60 silver. After: Day; Abel's soul at the Reliquary; Finch back aboard.

### Side 1: Wanted: Already Dead (the gate's poster; about 10 minutes)

**Offer**, the poster on the town gate's post, read (`Artifact_PosterGate`, an `AWantedPoster` that also offers):
**Card:** WANTED: ELLIS RANSOM, OF THE DUNNE GANG. DEAD OR ALIVE. Somebody has written ALREADY under the last line.
There's one on every post in town and on both farms.
**Steps:**
1. Tear down your wanted posters around town and the farms. [LastingInteract WantedPoster 6 of 7, hold]
2. Read the note on the Rim Rangers' board by the sheriff's office. [Interact CalderNote] It offers Calder's Caches.
3. Optional: look round the sheriff's office. [Reach the office's inside] The desk's open daybook, last entry a month
   old: *Gone down the Sink after the big one. Back by supper. — W.O.*

**Turned in to Tilly**, with a twist: *"So it was you, tearing them down. Good. They were bad for trade."* Ellis: *"Who
wrote ALREADY?"* Tilly: *"I did. On the first one. Somebody had to."* Then today's line about the stranger's effects
(the Rare gun).

### Calder's Caches (Ruth's note; about 12 minutes)

Today's optional Ranger caches, as a mission. **Offer**, Ruth Calder's note on the Rangers' board
(`Artifact_CalderNote`):
**Card:** RIM RANGERS. Three caches laid for any Ranger riding through: under the iron flower that drinks the wind; on
the lip of the hungry hole; on the path where the Ransom kid used to go to sulk. Take what you need and pin the tag. The
gang's strongbox is in the sheriff's lockup as evidence. The fourth cache is at the Larkspur post, if the lift ever runs
again. — Capt. R. Calder
**Steps:**
1. Find Calder's three caches. [LastingInteract RangerCache 3: Whitlock's windmill, the Sink's rim, the bluff path; one
   opened in Main 2 already counts]
2. Open the gang's strongbox in the sheriff's lockup. [Interact GangStrongbox, hold] Ellis knows the combination (*"The
   Deacon's birthday. He never changed it."*). Inside, with the loot, a fold of banknotes and a slip in the Deacon's
   hand: *Kid's share. For when the kid comes home. — A.D.* Hob, after a while: *"Sentimental, for a man who shot you."*

**Turned in at the Rangers' board** (`NoticeBoard_Rangers`): pin the three cache tags on Calder's tally, Ranger fashion.
Hob: *"Three tags for the Rangers. She'll still hang you, sunshine."* The note's last line points to the ore lift.
**New:** the cache tags as items (one per cache opened), the board as this mission's turn-in, the Deacon's slip.

### Side 2: Unfinished Business (Amos; about 12 minutes)

**Offer**, Amos at his fence (today's meet lines; the talk that was step 1 is the offer).
**Card:** Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint went
dark, and drifted home to find it rotting in the field. He isn't angry yet. He'd like it in.
**Steps:**
1. Load his six hay bales into the barn. [LastingInteract HayBale 6, hold]
2. **New talk:** ask Amos who's on the road. [Talk Speaker_Amos] The twist: *"That's the Merritt brothers and Sol
   Tanner. My hands. Fever winter took them, and I owed them for the harvest. Never paid. Couldn't."* *"Reckon they've
   come to collect."*
3. Drive off his old hired hands. [Clear WhitlockHands 6]
4. Optional: read what Amos owed, chalked inside the barn door. [Interact, **new** WagesChalk] Three names, three sums,
   none crossed out.

**Turned in to Amos** (today's thanks), plus: *"I'll pay them what I owe when I see them on the far side. If there's a
far side."* The Epic gun he was buried with.

### Side 3: The Gravemother (Sheriff Odell's bounty; about 10 minutes)

**Offer**, the Rangers' board (`NoticeBoard_Rangers`), the sheriff's own posting, a month old:
**Card:** BOUNTY. The big one in the Sink's den. She took my dog and the Haskett mule. The town's purse to whoever brings
me her fang, paid at Pruitt's. I'm going down after her myself. — Sheriff W. Odell
**Steps:**
1. Find where the Gravemother hungers. [Reach Place_SinkFloor, 19.5 m, height counted] (today's)
2. Kill the Gravemother. [Clear Gravemother] (today's: a kill on an earlier visit counts while she's away)
3. Take her fang. [Collect GravemotherFang, **new**: a mission item her body drops on the first kill]
4. Optional: find what's left of Sheriff Odell in her den. [Interact, **new** SheriffBadge] His badge in the webbing.

**Turned in to Mrs. Pruitt** at the store's hatch (`Speaker_Pruitt`, **new**: the vendor's door also talks): *"Lord,
the size of it. That's the town's purse, what's left of it."* *"Wade was my brother. He always said he'd die with his
boots on. Never said a word about his hat."* XP the first time; her Legendary table on every kill and her Vigil Stone,
as today.

## 4. The new side missions

### The Old Fare (Mr. Sloane, the depot; after Main 3; about 12 minutes)

The station agent stayed when half the town left. He's alive, barricaded upstairs in the depot, and heard through his
door like the rest of the living.
**Offer**, through his door at the top of the depot's stair (`Speaker_Agent`): *"Who's there? ...Lord. The Ransom
kid."* *"Every night since Wednesday they come and stand in my waiting room like they've a train to catch. I can hear
them breathing. If you're dead too, maybe they'll listen to you."*
**Card:** Mr. Sloane, the station agent, has locked himself upstairs. Every night since the saint went dark the dead
have stood in his waiting room as if for a train. Clear them out, and find out what they're waiting on.
**Steps:**
1. Clear the depot's waiting room. [Clear DepotWaiting 5: three Unpaid, a Restless one and a Bell-Ringer, in the room
   and out on the platform]
2. Find what they were waiting on. [Reach the baggage room] A pine coffin tagged GRAVESEND. JOSIAH FENN. FARE PAID.
3. Look at Fenn. [Interact FennCoffin; a Grave Sight flash] Two pale rings on the dead man's eyelids, where coins sat.
   Hob: *"Somebody's been at his fare, sunshine. Rob a dead man's eyes and every soul for a mile hears about it."*
4. Optional: look in the agent's desk. [Interact AgentDesk] A gambling marker from the *Gilded Lily*'s tables, payable
   to N. Purcell. Ellis: *"Ned's paper."* Hob: *"Small world."*
5. Ask Mr. Sloane about Fenn's coins. [Talk Speaker_Agent] *"His people laid gold on his eyes, like the old days. Gold,
   on a dead man! I only borrowed it. Against a debt."* Two old coins come out under the door. [Collect FennsFare]
6. Lay the coins back on Fenn's eyes. [Interact FennCoffin, hold]

**Turned in to Josiah Fenn**, a talking coffin (`Speaker_Fenn`): *"Much obliged. A man likes to pay his own way."*
*"Tell Sloane I forgive him. Don't tell him I said it like that."* *"There's a pistol under me. Never fired it once.
Take it."* The waiting room stays quiet from then on; Mr. Sloane comes down to his ticket window, a new talking window
with lines for the station (*"Line's open to the Lily, kid. Your car's coupled."* after Main 7).
**Why it matters:** it shows the old fare, grave gold on dead men's eyes, long before Ira's dying line, and seeds the
*Lily*. Grave gold stays story only, never money.
**New:** the depot's interiors (section 9), the coffin's speaker point, the coins as a mission item, the marker prop,
the agent's two speaker points (upstairs door, then the ticket window), the DepotWaiting encounter.

### Knocking Below (Tilly; after Main 4; about 12 minutes)

**Offer**, Tilly at her window: *"You've been in a grave and out of it, Ellis, so you'll understand me."* *"Something's
knocked in my cellar every night since Wednesday. I've four down there waiting out their seven days, and no keeper to
walk them."* *"I'd go down myself, except I'd rather not."*
**Card:** Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up. Since the saint went dark
something down there knocks all night, and Tilly hasn't been below since. The way in is through her father's workshop,
off the undertaker's yard.
**Steps:**
1. Go round to the workshop door in the undertaker's yard. [Reach WorkshopDoor]
2. Find the cellar key. [Collect CellarKey, on her father's measuring hook] Optional: read the coffin orders on the
   slate. [Interact OrdersSlate] Four names, and a fifth struck through in another hand: *T. BRIGHT. PINE. NO BRASS.*
   Hob: *"Wrote his own order, did he. And she put brass on it anyway."*
3. Go down into the cellar. [Reach CellarFloor]
4. Quiet the dead in the cellar. [Clear TillyCellar 4: three Unpaid and a Restless one, in tight rooms]
5. Find what's still knocking. [Interact ColdRoomDoor] Behind the cold-room door, at his bench, a man in an undertaker's
   apron is measuring a coffin that isn't there: Thaddeus Bright, Tilly's father, home from the Sundown Road to his
   work.
6. Talk to Thaddeus. [Talk Speaker_Thaddeus] *"Ransom's child. I measured you, you know. You've stood up since."* *"I
   kept them in their boxes a week. They got past me when the door opened. A cellar wants keeping."* *"Give her my tape.
   Tell her the brass was too much. And that I liked it."* [Collect ThaddeusTape]

**Turned in to Tilly** (the tape through the window): *"...He's down there."* *"Of course he is. He never once came up
for supper."* *"Brass handles. He'd have hated them."* *"There's a box on the bench by the cellar door. It was his.
Don't make me look at it."* (A Rare gun.) Thaddeus stays in the cold room afterwards, a friend with lines through the
story, and Tilly's later lines mention him. His car is the hearse car Ellis rides in.
**Why it matters:** a child and a dead father, Tilly's side of what Ellis does at the boards, and the "brass" she
polished for him (her Main 7 line) paid off.
**New:** the workshop and cellar (section 9), Thaddeus (a friendly dead man on the Unpaid rig, posed like Amos), the
cellar's encounter, a height band on Main Street's safe ground so the cellar under it can hold a fight.

### Dead Lift (Hiram Coyle, the Old Quarry; after Main 5; about 15 minutes)

The Providence Mining Company's ore lift ran from the Old Quarry to the top of Larkspur Ridge until the mine shut four
years ago, after "the fall" that buried eleven men. Hiram Coyle ran its winch for twenty years and died at his brake.
He hasn't left.
**Offer**, Hiram on his stool by the cold boiler (`Speaker_Hiram`): *"Ain't had a passenger in four years. You riding
up, or just looking?"* Ellis: *"Riding."* *"Then she wants three things. Her crank: the sheriff took it when they shut
her. Coal: boiler's stone cold. And her clamp pin, which went down the hill in Abe Haskett's pocket the day of the
fall."* Hob: *"Dead lift. I'll let you have that one, sunshine."*
**Card:** The old ore lift still hangs over the Old Quarry, and its engineman, Hiram Coyle, still waits by its boiler.
Find its winch crank, coal for its boiler and the clamp pin its brakeman carried down, and Hiram will take you up the
mountain.
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
*"Tell the Rangers Hiram Coyle says they owe him for coal."* *"There's a carbine in the head's tool box. Company's.
They'll never miss it."* (A Rare rifle.) From then on the lift runs both ways: hold {Interact} on either station's bell
pull. On Main Street, the Haskett house mutters: *"...That's the lift. Abe always said he'd hear her run."*
**New:** the lift (cage, tower, head, cable, winch house; `AOreLift` and the ride scene), Hiram (a friendly dead man in
the miners' costume set), the speaking tube's speaker point, the coal stage, the three fittings, the crank, sack and pin
as mission items, a named Restless dead miner who drops one, the QuarryCrew and RaisingSteam encounters.

### The Last Watch (Calder's Post's board; after Dead Lift; about 12 minutes)

**Offer**, the board by Ruth's cabin door (`NoticeBoard_CalderPost`):
**Card:** RIM RANGERS, LARKSPUR POST. Post unmanned: gone after the Dunne Gang's trail. Any Ranger riding through, light
the watch fire on the tower, so the posts down the Rim know Larkspur stands. If it stays dark they'll take the post for
fallen and pull back. — Capt. R. Calder
**Steps:**
1. Clear the post's yard. [Clear CalderPost 5: four dead miners and a Restless one, come up from the Bench since the
   post emptied]
2. Find oil for the watch fire in Ruth's cabin. [Collect LampOil] Optional: read Ruth's journal. [Interact RuthJournal]
   Her entry for the night of the robbery: *Skiff came up the canyon at dusk, running dark. Lights on Ransom's Point. A
   shot on the bluff path, then one up top. The Ransom kid drew on the Deacon at the last. Saw it through the glass.
   Doesn't wash out ten years. Rode the north trail down and round by the rail. Dawn before I got there. Closed the kid's
   eyes myself. Wasn't sorry.* And her last: *Riding south after them. If the kid's ghost is reading this: I'd have
   hanged you anyway. Probably.* Hob: *"...She closed your eyes, sunshine. Not everybody would have bothered."*
3. Light the watch fire on the tower. [Interact WatchFire, hold]
4. Keep the post until a post down the Rim answers. [Defend 45 s; the Watch: three dead miners come up from the Bench]
5. Read the answer with Ruth's code book. [Interact CodeBook] Far down the Rim a light winks on a mesa. Decoded:
   *LARKSPUR WE SEE YOU. CALDER RIDES SOUTH. ALL POSTS: WORD IS THE RANSOM KID WALKS. SHOOT ON SIGHT.* Ellis: *"Word
   travels."* Hob: *"Faster than you do."*

**Turned in at the post's board**: sign the post's log. Calder's fourth cache (a Supply Crate in the cabin) opens.
**Why it matters:** Ruth's note from Side 1 pays off; the player learns someone saw Ellis balk at the end, the seed of
Ruth's Act I ambush and truce.
**New:** Calder's Post (section 6), the watch fire (`AWatchFire`: lit or dark, kept by the save), the far answering
light (an unlit flicker card on the nearest backdrop mesa, shown by story state), the journal, code book, oil, the
post's board.

### Back Pay (a miner's tag, the Providence; after Dead Lift; about 22 minutes)

**Offer**, a brass tag on the IN side of the tag board at the Providence's mouth (`Artifact_MinersTag`): *No. 7. J.
ODELL. IN.*
**Card:** Eleven brass tags still hang on the IN side of the Providence's tag board. A miner hung his tag there going
down and took it off coming up. These eleven never came up, and something has worked the dark galleries ever since.
**Steps:**
1. Go into the Providence. [Reach UpperGallery] Hob: *"Eleven tags on IN. You do the sum, sunshine."*
2. Clear the upper gallery. [Clear MineUpper 5] Dead miners swinging picks at the walls, working for no one.
3. Search the Foreman's office. [Interact PayrollBook] The payroll: eleven men, three months' wages, unpaid, and the
   company's order, *Break the strike.* The Foreman's last entry: *They've barricaded the lower drift. Say they'll come
   up when they're paid. A small charge on the drift mouth will bring them up. — C. Teague.* Hob: *"That's the trouble
   with small charges."* Optional: read the chalk on the gallery wall. [Interact ChalkTally] *DAY 6. TEAGUE WILL DIG US
   OUT.*
4. Go down the decline and clear the lower drift. [Clear MineLower 6]
5. Take the eleven's tags from their barricade. [Interact Barricade] Chalked across its timbers: *PAY US AND WE COME
   UP.*
6. Put the Foreman down. [KillNamed Legendary_Foreman] Beyond the barricade, in the stope where the roof came down,
   Cyrus Teague has dug at the fall every night since he died.
7. Take the payroll from the stope. [Interact PayrollChest → Collect Payroll] The company's silver, never paid out.

**Turned in at the tag board** (`Speaker_TagBoard`: the shift's voice out of the dark), with a **choice** (**new**: a
turn-in with two rewards; the user's call):
- **Pay the eleven.** The tags go on OUT with their wages. The shift boss: *"Paid in full. Shift's over, boys."* The
  Providence's dead stop coming back (the Foreman's Vigil Stone still calls him); a Rare gun lies on the board, one of
  them carried down for claim-jumpers. Hob: *"First honest pay this mine ever made."*
- **Keep the payroll.** About 400 silver at level 7. The tags stay on IN, and the dead keep working the galleries (their
  encounters come back on arrival). The shift boss: *"...Company man after all."* Hob: *"Can't spend it in the ground,
  sunshine. Good thing you're not staying there."*

**Why it matters:** the Unpaid were unpaid before they ever died. Debts and who owes what, the story's theme, in a mine.
It also closes the Odell family's thread: Jory's tag, Wade's report and his hat, his sister at the hatch.
**New:** the Providence (section 6), the Foreman and the dead miners, the choice turn-in, the tag board (an artifact
that offers and a speaker point that takes the turn-in), the payroll book, chalk, barricade and chest.

## 5. The controls, taught in Mains 1–3

Hint rules (`UControlHintSubsystem`): one hint at a time, never in a scene or over captions, gone once the control is
used, never again once learned.

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

- **Larkspur Ridge** closes the valley on the north. Its foot runs along y ≈ 86–102 from the Rim (x ≈ −90) to the East
  Ridge (x ≈ 104, y ≈ 70), with an 8–12 m cliff band and steep pines above it; its crest stands about +42 m some 42 m
  behind the foot (y ≈ 130–145). From the valley it looks as before, except for the lift's cable rising out of the
  quarry, the lift head on the skyline and Calder's watchtower over the ridge's west shoulder.
- **Larkspur Bench**, behind the crest: a hanging valley of alpine meadow, granite and lodgepole pine, with drifts of
  blue larkspur (the ridge's name); x −88 to 112, y 138 to 230. Its floor is +44 m behind the crest and rises gently to
  +58 m in the north-east. A small tarn lies at (−20, 200).
- **Larkspur High** (moved north): its foot at y ≈ 228–233 with a 12–16 m cliff band, its crest about +120 m at
  y ≈ 275, past the boundary. Its far side falls toward the *Lily*'s country, never seen.
- **The west:** the Bench ends on the Rim at x ≈ −88 to −92, 115 m above Gravewind Canyon's floor. Larkspur Falls
  drops off it into Gravewind Canyon (the user's add, 2026-10-13).
- **The east:** the East Shoulder, a steep spur of the East Ridge's north arm, at x ≈ 108–115.

| Place | Height |
|---|---|
| The Old Quarry's floor | +1 m |
| Larkspur's crest | +42 to +48 m |
| The lift head | +47 m |
| The Bench | +44 m (south and west) to +58 m (north-east) |
| Calder's Post's knoll; the watchtower's deck | +50 m; +62 m |
| The Providence's mouth and upper gallery; the lower drift and the stope | +58 m; +50 m |

**The core square grows.** (Built.)
- The terrain's core is a centred 512 m square (`map.half` 20,000 → 25,600 cm). The old 400 m core kept features out of
  its outer 25 m, so they stopped at y = 175; the Bench needs y = 230.
- 4096 height cells at 12.5 cm, on the old grid, so the valley's heights match: the old valley comes out within 1 cm
  everywhere inside the old boundary (a compare script, as the terrain identity check).
- Curvature and AO rasters keep 20 cm (caps 2048 → 2600); the macro map keeps about 10 cm as four 4096 maps, one per
  quadrant (the area doc's fallback). Tiles 5×5 → 6×6; triangles at most 190k (146k before), weighted to the playable
  ground.
- The Bench is a region bench (as the farm terrace is), Larkspur High's foot is at y ≈ 230, and the core's features add
  the quarry bowl, the Goat Steps' ledges, the knoll, the tarn, the building pads and the paths.

### Zones

All mountain zones are levels 5–6 (the user's call, section 10).

| Zone | Where | Size | Levels | What's there |
|---|---|---:|---|---|
| The Old Quarry (grown; section 7) | x 0–40, y 90–120, +1 m | 1,100 m² | 5–6 | the winch house, the lift's bottom station, the lift-men's plot, the Goat Steps' foot, the derrick ruin |
| The Crest | x −10–40, y 128–150 | 1,500 m² | 5–6 | the lift head, the crest path, the Goat Steps' head, the valley's best view after Ransom's Point's |
| Calder's Post | x −88 to −45, y 140–190 | 3,200 m² | 5–6 | the knoll on the Rim: the watchtower, Ruth's cabin, the corral and lean-to, the board, the Ranger's grave |
| Larkspur Bench | x −45–60, y 140–230 | 9,000 m² | 5–6 | meadow, the tarn, a pine stand, the ore tramway, the Dinner Pail camp |
| The Providence Diggings | x 40–112, y 170–233 | 4,700 m² | 5–6 | the mine mouth and tag board, the ore bin, powder house, smithy, spoil heap, the miners' plot, the Foreman's Vigil Stone |
| The Providence (interior) | under Larkspur High, north of the mouth (74, 230) | 1,500 m² of floor | 5–6; the Foreman set 7 | adit, upper gallery, the Foreman's office, decline, lower drift, the stope |

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
  (`SkiffRide`: carried, look only, skippable), about 24 s up or down, easing at both ends. The autosave waits through
  it. Called from either end with a held bell pull; an empty cage takes 8 s to come.
- **What the first ride shows**: the quarry dropping away; at 6–12 s the whole valley opening south (Main Street's
  roofs, the steeple, the Sink's dark ring, the amber orchard, Ransom's Point on the far skyline, the canyon's haze
  beyond); the cage clanking over the tower; then over the crest the Bench opening ahead, Calder's watchtower on the Rim
  to the left, the spoil heap and the Providence's mouth under the high ridge to the right.

### Calder's Post

A Rim Rangers' outpost on a knoll at the Bench's west end, (−66, 165), +50 m, looking south over the whole valley and
west over Gravewind Canyon.
- The watchtower (4 × 4 m, 12 m, a stair inside, a roofed deck with the watch fire's brazier, the heliograph and Ruth's
  spyglass mount) at (−72, 160) on the Rim's edge.
- Ruth's cabin at (−58, 172); the corral and an empty lean-to stable at (−50, 182); the board at the cabin door.
- The Ranger's grave (Tom Abernathy, died at his post) at (−64, 178).
- The post's yard, about 30 × 25 m and level, is N2's fight ground.
- On the cabin wall, Ruth's map of the Rim: six pins down its length a month apart, and a seventh, fresh, on Ransom's
  Rest.

### The Providence

**Outside, the Diggings.** The mouth's timbered portal at (74, 230) in Larkspur High's band, facing south, with the tag
board at (70, 227). The ore tramway runs from the ore bin at (62, 204) across the Bench to the lift head (about 64 m of
narrow-gauge track, two ore cars off the rails): it carried ore to the lift, and it leads the player to the mine. A
stone powder house at (95, 214), a smithy shed at (88, 196), the spoil heap running down to the East Shoulder at
(100, 190), the miners' plot at (50, 214) with a board for the eleven never recovered. The Foreman's Vigil Stone rises by
the portal at (80, 224) after his first defeat.

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

- Each fight is on one level of ground: the gallery, the drift and the stope are flat; the decline and the manway hold
  none.
- The interior's 2D footprint (y 230–345) joins the playable polygon as a tongue whose edges are marked blocked (closed
  by rock, no foot shaping), so fall recovery records safe spots inside and no wall closes the portal.
- Inside, the minimap shows its markers over a dark plate marked UNDERGROUND (**new**), since its bake sees only the
  ridge above.

**The Foreman**, Cyrus Teague: the Unpaid rig at 1.5× in a foreman's long coat, a bowler with a carbide lamp, a pick
handle and a satchel of fuse sticks. A Legendary monster at a set level 7 (5, and Soulfed's 2), orange tag and coal, a
page in the Ledger, the Legendary loot table; about 3,400 health (12 × a dead miner of his level), tuned to a 2–3 minute
fight.
- *Fire in the hole*, all fight long: lights a fuse (sparks and a hiss for 2 s) and lobs the stick; a 6 m burst like the
  grave salt's.
- *The roof*, every 18 s or so below 66%: strikes a prop; dust trickles on four marked spots for 2 s, then rock falls
  there, and the prop splinters, so the cover changes.
- *The shift* (at 66% and 33%): three dead miners rise out of the rubble.
- *The dark* (at 50%): the stope's lanterns blow out for 12 s; his lamp, the miners' lamps and their coals are what's
  left (the exposure lifts, so it reads dark, not black).
- His coal is open while he lights a fuse and for 3 s after a roof strike (1.5× crits, the game's rule).
- Lines: *"Back to work!"*; *"Nobody leaves till the drift's cleared!"*; *"Fire in the hole!"*; in the dark, *"I can dig
  them out. I can still dig them out."*; his last word, kneeling at the fall: *"...Tell the company it was the roof."*
- Ledger, in Sexton's voice: *He brought the roof down on eleven men to save the company a quarter's wages, and has dug
  at it every night since. Remorse is a poor shovel, friend.*

**Dead miners**, a new kind of the Unpaid on its rig (`ADeadMinerCreature`, a subclass like the Bell-Ringer):
- Canvas coats, braces, felt hats or leather caps with a head lamp, a pick; three tints.
- Story: forty years of the Providence's buried dead came home to it when the saint went dark, and the eleven never
  left: nobody brought them up to be given to the wind. Abe Haskett's lift crew, buried by the quarry, came home to the
  lift.
- 180 health and 9 damage at level 1 (the Unpaid's are 160 and 8). Ranks as the Unpaid.
- An overhead pick swing with a 0.8 s wind-up in which the lamp flares; a shot to the lamp in the wind-up staggers it
  (like the bell and the venom sac).
- Unaware, they work: picks swung at the walls, a lunch pail opened.
- Barks: *"Shift's not over."* *"Pay's Friday. Pay's always Friday."* *"Who's on the lift?"* *"Teague said the roof
  would hold."* *"Mind the timbers!"* *"Eleven in. None out."*
- Ledger: *Men who went down for silver and were owed for it. They still work for wages that will never come. Some debts
  outlive the debtor, friend.*

### Encounters

Each group lives on one level of ground; its chase stops at that ground's edge, as everywhere.

| Group | Creatures | Levels | Ground | Budget | On |
|---|---|---|---|---|---|
| QuarryCrew (N1) | 4 dead miners; Abe Haskett, Restless, with the pin | 5–6 | the quarry floor | N1 | N1 accepted |
| RaisingSteam (N1, defend) | 2 spiders, a Venom Spitter, then 2 dead miners | 5–6 | the quarry floor | N1 | N1's step 3 |
| Patrol Crest | 3 spiders (the area's ranks) | 5–6 | the crest path, 20 m there and back | optional | the first ride |
| CalderPost (N2) | 4 dead miners, 1 Restless | 5–6 | the post's yard | N2 | the first ride |
| The Watch (N2, defend) | 3 dead miners | 5–6 | the post's yard | N2 | N2's step 4 |
| Camp Dinner Pail | 3 dead miners round a cold cookfire with their lunch pails, 1 Restless, a Bell-Ringer; a Supply Crate | 5–6 | the Bench's middle round (−5, 170) | optional | the first ride |
| Ambush Spoil | 3 dead miners rise out of the spoil heap | 5–6 | the Diggings' yard | N3 (main) | the first ride |
| MineUpper (N3) | 4 dead miners, 1 Restless | 5–6 | the upper gallery | N3 | always |
| MineLower (N3) | 4 dead miners, 1 Restless, a Bell-Ringer | 5–6 | the lower drift | N3 | always |
| The Foreman | the Foreman (7) and two waves of 3 dead miners | 7; 5–6 | the stope | N3 | always; then his Vigil Stone |

47 kills on the mountain and the quarry: 39 on the missions' way, 8 optional. Placed creatures come back on arrival as
elsewhere; the mine's stop coming back if the eleven are paid.

### Loot

- Chests: the Dinner Pail camp's Supply Crate; Calder's fourth cache (a Supply Crate, opened by N2); the company safe in
  the Foreman's office (a Strongbox: 2 guns at Luck 1.0); the payroll chest (N3's choice).
- Containers: a footlocker in Ruth's cabin and one in the winch house, six lunch pails in the mine and the camp
  (**new**: a small tin container opened with a tap, ammo or 4–10 silver, `Lootables.py`), two graves to dig at the
  miners' plot, a mailbox at the post.
- About 14 breakables: powder kegs and crates at the Diggings (5), in the mine (6) and at the post (3).

### Landmarks and sightlines

- From the valley, the lift's cable and head frame over the quarry and the watchtower over the ridge's west shoulder (its
  deck at +62 m clears the crest from Main Street) say "up there".
- From the lift head, two ways at once: the watchtower 95 m west on the Rim, and the tramway north-east to the spoil heap
  and the mouth; the pine stand and the camp break the line between them, so each arrival is its own.
- The watchtower has the area's second great view (the valley from the north, the canyon, the Bench below).
- No view of the *Lily*: Larkspur High's crest closes the north.

### The way back down, and the boundary

- **The lift**, both ways, from N1 on; and **fast travel** between found roosts (section 8).
- **The Goat Steps**, a one-way drop: nine rock ledges, each 4–5 m down and 3–4 m deep, from the crest at (4, 136) down
  the quarry's west wall to its floor at (4, 110). Each drop is too tall to mantle back up (a mantle reaches 1.5 m, a
  jump and a mantle about 2.7 m), too short for fall recovery's 30 m rule, and too tall for a creature to follow (they
  won't step off more than 4 m).
- **The boundary** (one polygon): the valley's north edge stays closed with its walls, except the quarry, which becomes a
  neck (x 0–36) up to the crest. The Bench is closed on the north (Larkspur High's band) and east (the East Shoulder)
  with walls behind, as the valley's ridges. Its west edge (the Rim) and its south edge (the crest's lip over the ridge
  face) are open: going over either is Hob's catch, as off the Rim. The ridge face between the valley and the Bench stays
  outside the polygon, so nobody climbs it. The mine's tongue joins at the portal.

### Performance

- **The heaviest new view** is the watchtower's deck looking south-east: the whole valley from the north (120–300 m
  off), the canyon, the Bench in front. Ransom's Point's tower ran about 6.5 ms before the expansion; this one is planned
  at 7.0 ms or less (GPU at most 7.5 ms, about 700 draws).
- **How it holds:** the Bench gets at most 70 draws (pines instanced with last-LOD cards, no masked leaves past 100 m;
  the cabin, tower and Diggings merged per material with LODs; the tramway instanced); the valley is seen at its far
  LODs; the mine's interior is hidden while the player is outside, and the outside while inside (**new**
  `AInteriorVisibility` at the portal); one spline mesh per cable span; head lamps are emissive cards; at most 3
  shadowless point lights in any view; no far shadow cascade. The bigger core adds about 40k triangles, mostly steep
  ground seen from afar.
- **Measured** (Medium, editor closed): the watchtower over the valley, the lift head over the Bench, the ride's top,
  the Diggings, the upper gallery with 6 dead miners, the stope with the Foreman and 6 adds, the cellar with 4 Unpaid,
  the depot's fight, and Ransom's Point again (the lift and tower join its skyline); the mountain at dusk too, since
  Main 6 and 7 play at dusk and a player may go up then.

## 7. The valley grows a little

Only where the new missions and the house kit need ground:
- **The Old Quarry** is cut deeper and wider into the ridge's foot, from y ≈ 100 to y ≈ 120 across x 0–40 (about
  +900 m² of floor), with two or three benches stepped up its walls. It holds the winch house, the cage's landing, the
  lift-men's plot, the OldQuarry roost at its mouth (section 8), the QuarryCrew fight's floor (about 20 × 18 m with ore
  cars, cut blocks and the derrick ruin as cover) and the Goat Steps' foot. Its zone keeps levels 5–6; the quarry
  track's patrol stays.
- Inside the old boundary: **Pen Lane**, a back lane south of Main Street between the stock pens and the store's yard,
  with two new homes at (22, −28) and (36, −30); a **dugout** on boot hill's south slope at (−12, 22); **the depot**
  rebuilt larger on its lot (14 × 8 m, from 10 × 6) with its rooms inside and a coal stage by the water tower;
  **Bright & Daughter's workshop** behind the undertaker's front at about (62, −24), over its cellar; and **Amos's
  house**, small and empty, at (20, −72) west of his barn, with his grave beside it at (16, −66).
- **Keeper's Bridge** (the keeper's path over the Dry Wash, (−95, 2)): grounded (built; the user's play note,
  2026-10-13).

## 8. Hob's roosts (were the save graves)

The user's call (2026-10-13): the save points are Hob's roosts, not graves. The Crow's Mast (look sheet 41, design B) is
the one save, respawn and fast-travel point on every map, its beacon in Hob's Ember red #D8321C once found. (Built.)
- Walking within 9 m with a clear line finds one: the column shoots up, the bell rings, the crows lift, ROOST FOUND, an
  early save.
- Continue wakes at the area's last found roost; a death is dragged by Hob's crows to the nearest found one; fast travel
  is Hob's crows carrying Ellis between found ones.
- Placed from `layout.json` `gameplay.roosts` (`build_area_roosts.py`). The grave wake in the cold open stays a scene;
  the first roost stands by the farm.

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

The ids FamilyPlot, ChapelYard and KeepersGrave are the old graves' (saves keep them). Since the user's cut (2026-10-14) only the
Depot and Calder's Post stand, and since 2026-10-23 ("Town roost") a third: SheriffsOffice, "Main Street", mast at (8, 3) m in the
street's north edge by the sheriff's office, facing 90°, found any time (a new id: MainStreet was cut). Rules: `Docs/Pipeline/Game.md`.

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

- Eight new homes (Saltbox, IHouse, BoardBatten, StoneCottage, Shotgun, Dogtrot, Dugout, Homestead) and Delia's
  farmhouse make nine; three new fronts join the four. Skyreach's LogCabin leaves Ransom's Rest, and
  SM_Cottage_Ransom's four uses on Main Street go to the new homes.
- Every new model goes into `area_shape.py`'s `FOOTPRINTS` and `BUILDINGS`, and the town life's households map to their
  new models.
- Built: the approved homes kit (look sheet 35) came in as `RR_Home_A` to `RR_Home_J` (plus the well, outhouse,
  windmill and its fan), not under the home type names above; `area_shape.py` and `layout.json` say which home stands
  where.

### Interiors

| Interior | Rooms and sizes | What's inside | Used by |
|---|---|---|---|
| Bright & Daughter's workshop and cellar | workshop 10 × 7 m; cellar 12 × 8 m, 2.6 m stone vault, down a stair from the workshop's corner; the cold room 4 × 4 m at its back | coffin benches, lumber racks, tool wall, a stove, the measuring hook, the orders slate; coffin racks, four coffins on trestles; Thaddeus's bench | Knocking Below |
| The depot | waiting room 9 × 8 m; ticket office 5 × 4 m behind the grille; baggage room 5 × 4 m onto the platform; a stair to the agent's door upstairs (closed) | benches, a stove, the schedule board, a clock; desk, till, ticket rack, telegraph key; trunks and Fenn's coffin; the LOST PROPERTY window (Lost Loot, section 1) | The Old Fare; Dead Lift's coal; the station board by the door, as before |
| The winch house | 10 × 8 m, one room open to the cage's landing | boiler and firebox, the winch drum and brake, the coal bin, the speaking tube, Hiram's stool, a footlocker | Dead Lift |
| Ruth's cabin | 6 × 8 m and a lean-to | bunk, stove, table with the post's log and Ruth's journal, the oil, the code book, her Rim map, a footlocker, the fourth cache | The Last Watch |
| The watchtower | 4 × 4 m, 12 m, an inside stair, a roofed deck | brazier, heliograph, spyglass mount | The Last Watch |
| The Providence | above (section 6) | above | Back Pay |
| The sheriff's office, the chapel | as before | the crank and Odell's report in the lockup; the keepers' roll in the nave | Dead Lift, Calder's Caches; Main 4 |

## 10. Experience and levels

- **Targets** since 2026-10-27 (the mine in the story): the story alone about level 6.9, a full clear about 7.5, each boss
  met near its level; the Foreman set 4 (6 with Soulfed), a level under Abel (`Docs/Progression.md`). Before: (the user's
  call, 2026-10-13) the story alone ends about level 5.5, a full clear about 6.8. Kill
  experience and every mission's amount came down for it; most of a side mission's reward is silver and guns. The
  draft's sums (story 6, full clear 9.8) are superseded (archive).
- **The curve** (built): reaching level L takes 180 × (L^2.8 − 1) experience in all, capped at level 65. The kill rules
  are in `Docs/Progression.md`; each mission's amount is in `create_mission_assets.py` and
  `create_side_mission_assets.py`.
- **Zone levels** (`layout.json` `gameplay.levels`): the band is 1–7. New zones, listed before "valley": the quarry (the
  bowl's polygon, id `quarryBowl`), crest, calderPost, bench, diggings, providence (the interior's polygon); all 5–6.
  The cellar's spawner keeps its own 3–4 (a spawner's level is kept); the depot stays in the valley's 2–3. New kind:
  ForemanCreature at a set 5 (Soulfed's +2: 7), level with Abel.
- **When it opens:** the lift after Main 5 (the user's call), with the Gravemother.
- **Checks:** `Looter.Progression.Pacing` fails outside story only 5.3–5.8, everything 6.6–7.0.
  `Looter.Economy.Balance` keeps a normal play's silver inside 1,000–1,800; the design's estimate: the valley's new
  purses (Calder's 30, Fenn's 30, Thaddeus's 40) with their fights and containers add about 180, to about 1,450; the
  sheriff's bounty (80), the mountain's purses and the kept payroll (about 400) come on top for an explorer.

## 11. Canon changes once approved

Edits owed to the other docs (build step 6).

**`Docs/Story.md`:**
1. *What changed*: the Setting row's "Skyreach stays a floating island, outside the story" becomes "Teropa has one sky
   island, Skyreach, reached from the north side of the *Gilded Lily* by a skylift (area 3)". The Tutorial island row and
   the Leaving the tutorial row are replaced by one row: "The opening: a new game starts on Ransom's Rest's cold open; the
   controls are taught by hints in Mains 1–3. Skyreach, with Crossroads Town, becomes area 3."
2. *Writing notes*: "Never call a story place an island; only Skyreach is one, and it is outside the story" becomes
   "Never call a story place an island but Skyreach, Teropa's one sky island."
3. *Cold open*: its first sentences become "It plays as soon as a new game starts, and can be skipped. The title
   REVENANT rises through the white of a bank of evening cloud, and the white thins on the Dunne Gang's skiff gliding out
   of it." The cloud match cut (a tutorial skiff sailing into cloud) is retired; the cloud bank, the dark skiff and the
   rest stay. Ellis comes out of the grave with the Common Bullpup they were buried with.
4. *The world*, "Outside the story" paragraph: replaced by "Skyreach, the sky island. North of the *Lily*'s country one
   island floats over the hills, and a skylift climbs to it. Nobody on Teropa knows why it floats." (Its story comes
   with area 3's design.)
5. *Getting around*: "Skiffs are rare: the tutorial's, and the Dunne Gang's" becomes "Skiffs are rare: the Dunne Gang's
   is an outlaw's airship"; add "A skylift from the *Lily*'s north side climbs to Skyreach."
6. The sections *The tutorial island, outside the story*, *Leaving the tutorial island* and *Back to Skyreach for
   practice* are replaced by a short *Skyreach, area 3*: the island and Crossroads Town stay; the tutorial, the practice
   trips, 0 XP and ammo-only drops go; Hob comes along; its band and story wait for its design.
7. *Ransom's Rest (summary)*: its size becomes "about 309 × 350 m and 64,000 m², with Larkspur Bench, Calder's Post and
   the Providence mine up the ore lift"; beat 1 becomes "Seven Days. The cold open, the grave, Hob, and Delia through the
   screen door."
8. *Campaign gating* and Decision 8: every "Skyreach (practice)" line on the station boards goes. A station board lists
   every opened area and one blank line naming the mission that opens the next; Ransom's Rest's reads "THE GILDED LILY —
   Finish 'The Lantern Leans'" until Main 7, then "The *Gilded Lily*". Skyreach is listed only once opened, from the
   *Lily*.
9. *Areas* table: Skyreach's row moves third (after the *Lily*): "Opens after: a mission on the *Lily* (to design), by
   its skylift; levels: to set". Ransom's Rest: "Opens after: a new game"; enemy levels 1–7; Legendary monsters the
   Gravemother and the Foreman. The *Lily*: a note that its 9–15 may start lower.
10. *Enemy ranks*: the Legendary row's "at most one alive per area" applies to random Soulfed promotions; hand-placed
    ones are as an area's design names them (Ransom's Rest: the Gravemother and the Foreman).
11. *The game's systems*: Mission markers adds "a gold or white ? over whoever offers a mission, a ! over whoever it's
    turned in to". *Practice on Skyreach* is removed. (Death and respawn already names Hob's roosts.)
12. *Signature moments*: "A skiff sliding into a cloud bank, and out of another one seven days earlier" becomes "The
    title rising through cloud that parts on the gang's skiff, seven days ago."
13. *Decisions*: decisions 4–6 are marked superseded, and a 2026-10-13 entry records the map order, the skylift, the
    opening, the givers, Hob's roosts, the mountain and the house kit.
14. *Cast*: add Thaddeus Bright (Tilly's dead father, in her cellar), Hiram Coyle (the lift's dead engineman), Mr.
    Sloane (the station agent), Josiah Fenn (a dead peddler bound for Gravesend), Mrs. Pruitt (the storekeeper, the late
    sheriff's sister) and Cyrus Teague, the Foreman.

**`Docs/Areas/RansomsRest.md`:** *At a glance* takes the new size, play time (main path about 75 minutes, about 3 hours
15 with everything), levels 1–7 (the Gravemother 6, Abel 7, the Foreman 7), "7 main, 9 side", both Legendary monsters,
and an arrival of "a new game: the cold open, then the grave"; *Size and shape* takes section 6's north, the neck and the
tongue; *Zones*, *Encounters*, *NPCs* (Thaddeus, Hiram, Mr. Sloane, Fenn's coffin, Mrs. Pruitt; Hob and Pa as givers)
and *Tour viewpoints* take sections 4, 6 and 9's additions; *Missions* points here; *The hub afterwards* lists Hob's
roosts (section 8); every "Skyreach (practice)" line goes from *The exit and the unlock*, *Tech needs* and *Decisions
for you*, with a pointer to the 2026-10-13 decisions. `Docs/Progression.md` takes section 10. `Docs/TutorialIsland.md`
says the tutorial is retired (done); `Docs/Polish/TutorialRework.md` gets the same line.

## 12. Build steps

Steps 1–5 are in the game as of 1ae1d99 (the list at the top; what's left: `Docs/Handoff.md`; the full step list as
planned: the archive):
1. Wave 1: the hold ring, the compass, a new game on the cold open with the hints, pace 0.9, the givers, save points.
2. 2a mission features (optional objectives `bOptional`, mission items `AMissionItem`, choice turn-ins `FMissionTurnIn`,
   speaker points as turn-ins, a height band on `ASafeGround`, the UNDERGROUND plate, small interactables); 2b `AOreLift`
   (ride scene from `SkiffRide`'s pattern, Board.Lift, saves held during the ride), `AWatchFire`, the far answering
   light, `AInteriorVisibility`; 2c `ADeadMinerCreature`, `AForemanCreature` (`UBossComponent`), Hiram and Thaddeus,
   Ledger pages; 2d–2h art (house kit, interiors, mountain kit, characters, props); 2i terrain (`terrain_check` and the
   identity check for the tutorial island and TerrainTest).
3. The level (`build_area_loot.py` for the loot). 4. The missions' data (`create_mission_assets.py`,
   `create_side_mission_assets.py`, `create_story_lines.py`). 5. Sounds.

Left, step 6:
6. **Checks**: every `Looter.*` test; tours with section 6's views, perf with the editor closed on the watchtower's
   view; HudShots (the offer card, the ? and !); a full play by the Demo agent from a new game to the station board, the
   mountain included; then `Docs/Story.md`, `Docs/Areas/RansomsRest.md`, `Docs/Progression.md` (section 11),
   `CODEMAP.md`, `Docs/Pipeline/` and the handoff. *The user approves:* Ransom's Rest, expanded.

## 13. Open questions

None open: the user answered all eight on 2026-10-13. Their answers are in section 1 ("The decisions") and in the
sections they shape: the lift after Main 5 and the experience targets (section 10), Back Pay's choice, Thaddeus, Ruth's
journal and Hiram (section 4), the Goat Steps and the lift both (section 6), the bounty board's two postings
(section 2). The questions as asked are in the archive.
