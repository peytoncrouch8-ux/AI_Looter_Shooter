# Mains 2-7, the mine and the Revenant sides, in the Wardens story (design, 2026-10-31 night)

Lane STORY2. Design only: nothing built, no data or code touched. For the user's approval, then one mission at a time (the
build order in `DECISIONS.md`). Read with `DECISIONS.md` (picks and every rejected idea), `CONTRACT.md` (Landfall),
`lanes/STORY.md` (Landfall's lines), `Docs/Areas/RansomsRest_Expansion.md` §2-4 (today's missions), `Docs/Areas/RansomsRest.md`
(places, bosses), `Docs/Progression.md`.

Rules kept: mission ids unchanged (`Main2`..`Main7`, Dead Lift, Back Pay, the sides; saves and conditions key on them);
steps counted from 0 as the scripts count; every talk or board turn-in is **card first, then 2-4 lines**; captions under
90 characters; Sully is "they", Delia calls them "Warden", Hob "sunshine", Sexton "friend"; XP per mission kept unless
noted (pacing note at the end). Rejected ideas not reused (no grave, no dying, no Revenant, Sexton not a villain, no
"Hold the Door" / "Grave Gold" / defend-the-farm titles).

## 1. The spine: why each mission leads to the next

Landfall ends on Delia's porch: Abel's soul burns in the barge as its pilot light; "bring him back to Saint Ada". The
barge hangs 45 m over the Sink, out of reach. Getting him back takes four things, and each main earns one:

1. **How to get at the barge** (Main 2). Sexton the Ferryman sells it: *a barge must eat; starve her and Ned brings her
   down to where the food is.* And: *a keeper's soul goes in his own lantern, or it goes out like a match.* His price:
   Ned's manifest, paid on delivery (taken in Main 6, handed over in Main 7).
2. **Abel's own lantern** (Mains 3 and 5). Its bail is at Tilly's (snapped off in his fist); the lantern is in the Sink,
   where Ned threw it (a gambler never breaks a keeper's lantern: the worst luck there is).
3. **The clock** (Main 4). Saint Ada is guttering without her keeper ("a coal under ash"). When she goes out, every grave
   on the Rest is open to the hose. Brother Finch comes ashore to keep her alive until Abel is back.
4. **Starving the barge** (Main 5, Dead Lift, Back Pay). Main 5 chokes the Sink intake; Ned drops a second hose down the
   Providence's air shaft, where Vane's men have Cyrus Teague's dead shift digging the eleven out of the fall. Back Pay cuts
   it. Starved, Ned brings the barge down over the chapel at dusk to hose Saint Ada herself (Main 6's reason and timing).

Then **Main 6** boards her there, beats Lucky Ned, pulls Abel into his lantern; the barge falls. **Main 7** carries Abel
home the way he ran, puts him back in the Reliquary (the saint wakes, the smog lifts), and Sully walks the week's dead to
the burial boards as the keeper would: the Gravewind takes them west; the Great Lantern leans to the next region.

**The world changes as the player wins** (feedback for every mission, cheap: lighting states and story props):
- Smog: thick through Main 5 (`Smog`, WORLD's state); **`SmogThin`** (new state, fog 0.007, exposure 0.3) from Main 5's
  turn-in (one hose choked, the barge's plumes cut to one stack); burning wreck smoke in the canyon after Main 6; clean
  dusk gold from Main 7's relighting scene; Day afterwards.
- The Lamplighter on the Mooring Ledge: its bag under tarps after Landfall, a sewn panel from Main 4, full and lifting
  against its anchor by Back Pay (three story props on the bag, one shown at a time). The crew's topics follow it.
- Abel's grave: frost and black flowers until Main 7; thawed, the flowers alive after.
- Company loudspeakers (`TRIPO_MODELS.md` 18) at the Keeper's Gate, the town gate, Main Street, the chapel, the Diggings,
  the barge: Vane talks from each in turn. **New small actor `ACompanyLoudspeaker`**: its horns can be shot (sparks, a
  crackle, Vane's line cut mid-word), the optional objective in several mains.

## 2. Lore added (to `Docs/Story.md` once approved)

- **Warden's eyes.** A sworn Warden has had the Great Lantern's light put in their eyes: they see and hear the dead and the
  soul-light ranks (Restless blue, Gravebound purple...). The living of the Rest barely see the dead. Replaces "only Ellis
  sees them".
- **The Unpaid, now.** Two kinds of the same cold dead: what the hose tears half out of a grave and drops (a hose spills
  what it can't hold), and the Rest's dead on the Sundown Road who lost Saint Ada's dimming light and drifted home. Same
  rig, same lore of the coal heart.
- **A saint and her keeper.** A saint's light holds her town's dead in their graves against any hose; her keeper lights his
  lantern from her each night and sits up with her dead; a saint with no keeper gutters. Abel held the chapel door so the
  hose couldn't reach Saint Ada, and the gang took him instead. The Reliquary is **whole**, not smashed (was: smashed by the
  Deacon); dim since Wednesday.
- **A keeper's soul** burns longest (contract) and can only be carried in his own lantern. Lit by his soul, the lantern
  "burns like a sunrise". Back in the Reliquary, the soul stays: Abel keeps Saint Ada in death (a saint's keeper never
  crosses while she burns). The emptied lantern is relit from the saint, as a keeper's should be.
- **Lucky Ned's superstition:** he never breaks a keeper's lantern.
- **Hob** flew for the Ferryman eighty years before he signed on with the Wardens (keeps his built feather mutters,
  "Eighty years I've been collecting for him"; Main 2's offer says it).
- **The Doorward:** the hooded stone mourner the first settlers set by the chapel door, kin of the four Stone Sentinels on
  Boot Hill and the churchyard; the saint's light keeps them. Smog-blind, they can't tell a Warden from a Dredger.
- **Soul coal on the Rest:** the barge presses the dead into coal; tenders carry the sacks to the Company's hopper cars on
  the depot's siding (so the depot is Company ground; The Old Fare's dead wait there for that train).
- Rename the chapel yard's Gravebound leader "the Sexton's Debt" (`PacingPaths.h` comment, its group's DisplayName) to
  **"the Leftover"** (Sexton isn't a villain now).

## 3. The flow (new titles; ids, givers and XP unchanged unless marked)

| # | Id | Title (was) | Giver, where | After | Turned in to | XP | Rewards | Min |
|---|---|---|---|---|---|---:|---|---:|
| M2 | Main2 | **The Ferryman's Price** (Shall We Talk Business?) | Hob, Delia's porch rail | M1 | Mister Sexton, his ferry under Gravewind Point | 550 | the Ledger | 12 |
| M3 | Main3 | **Under New Management** (Cold Welcome) | Delia, her porch (meet first) | M2 | Tilly, her window | 1,000 | Uncommon+, 40, Abel's lantern bail | 12 |
| M4 | Main4 | **A Coal Under Ash** (Hallowed Ground) | Tilly | M3 | Father Aldana, the vestry door | 1,300 | Saint Maud Uncommon (Hallow), 40 | 15 |
| M5 | Main5 | **Choke Point** (The Keeper's Lantern) | Aldana (one conversation) | M4 | **Brother Finch, at the Reliquary** | 3,000 | 50 | 15 |
| N1 | (kept) | **Dead Lift** (kept) | Hiram Coyle, the winch house (met) | M5 | Hiram, the lift head's tube | 5,000 | Rare rifle, 40, the lift | 15 |
| N3 | (kept) | **Back Pay** (kept) | Hiram, up the tube (met) | N1 | the tag board: pay or keep | 6,500 | Rare / 400 silver | 22 |
| M6 | Main6 | **The Pilot Light** (The Gravewind) | **Captain Holloway, the Lamplighter's wheel** (meet first) | N3 | **Holloway, at the wheel** | 5,000 | Ned's Boss table | 20 |
| M7 | Main7 | **Given to the Wind** (The Lantern Leans) | Holloway (one conversation) | M6 | **Delia, on the burial boards** | 4,500 | Heirloom by hand, 60, the next region | 18 |

Titles kept for Dead Lift and Back Pay: neither names the old premise, and "Back Pay" fits a story about a Company even
better. Sides: §11.

---

## 4. Main 2: The Ferryman's Price (Hob; about 12 minutes)

**Why:** Landfall says what was done; Main 2 buys the plan. Teaches the keeper's path (walked back in Main 7), ranks
(Hob's blue line), the Ledger, and what the Unpaid are.

**Offer**, Hob on Delia's porch rail:
- Hob: *"Man down on the burial boards wants a word, sunshine. Says he knows the barge."*
- Sully: *"Who is he?"*
- Hob: *"The Ferryman. I flew for him eighty years. Don't take his first price."*

**Card:** Mister Sexton, the old Ferryman, is waiting at the burial boards on Gravewind Point and says he can sell a way at
the barge. Hob flew for him once and says don't take his first price. Go by the keeper's path, the way Abel walked the dead.

**Steps:**
0. **Follow the keeper's path to Gravewind Point.** [Reach `Place_GravewindPoint`] Sexton's there. On the way, the Three
   Widows' nest: today's `BluffNest` (4 spiders, then the Restless one; softened numbers kept) **moved from the Point's top
   to the Widows** (Landfall already climbs the Point). Hob's rank line, reworded: *"See the blue on that one? Fed longer
   on the smog. Hits harder, and its iron's better."* Optional: cross Keeper's Bridge. [Reach `Place_KeepersBridge`]
   Hob: *"Abel walked the dead over this every sundown for thirty years. Tuesday too."*
1. **Stop the Company's hose-cart on the boards.** [Interact, held 1.5 s, `HoseCart_Boards` + Clear `BoardsCrew` 8]
   The week's dead lie shrouded on six of the eight burial boards (laid out by Delia; no keeper to walk them), and a
   Dredger crew is pumping them into a hose-cart. Set piece `BoardsCrew` (an `ASetPieceSpawner`): 3 riflemen in the boards'
   cover, a hose-man by the cart. **Twist:** shutting the cart's valve (or bursting its tank: a crit spot, the hose-man's
   pop) spills what it drank: the second push, 4 half-dredged dead rising out of a black spill and coming at Sully. Hob:
   *"That's what the hose leaves, sunshine. Half a soul and a temper."* (the player learns what the Unpaid are). **Set
   level 2** (an encounter level override: the deck's zone is 5-6).
2. **Talk to Mister Sexton.** [Talk `Speaker_Sexton`] Sexton's black ferry hangs on rotted chains in the canyon fog under
   the deck's west end; he sits in its stern (his seated model, reused). A rope ladder from the deck down 6 m to a timber
   landing. The deal:
   - Sexton: *"Warden. You've come about the barge. Everyone does, eventually."*
   - Sexton: *"She's out of your reach. But a barge must eat, and Ned is greedy."*
   - Sexton: *"Starve her, and he'll bring her down to where the food is."*
   - Sexton: *"Mind: a keeper's soul goes in his own lantern, or it goes out like a match."*
3. **Open the Ledger: {Inventory}, then 2.** [OpenPage Bestiary] Must stay step 3 (`Ledger::HandedOverStep`). Sexton, as
   he hands it up: *"Write down every soul you meet. I bill by the head."*

**Vane** (the Keeper's Gate loudspeaker, as the valve shuts): *"Wardens. That cart is Company property."* / *"I'm adding
it to your bill."* Optional: silence the Keeper's Gate loudspeaker. [Hit `Loudspeaker_KeepersGate`] (cuts him at "your
bi—").

**Turned in to Sexton** (card, then):
- Sexton: *"My price: Ned's manifest. Every soul Vane burned, with my fare beside it."*
- Sully: *"And if I don't bring it?"*
- Sexton: *"Then you'll owe me, friend. Ask the crow how that goes."*
- Hob: *"...Bring him the book, sunshine."*

Then Delia's ? for Main 3 (her word, meet first, as today).

**Reuses:** the keeper's path, cairns, Keeper's Bridge, the Three Widows, Gravewind Point's deck (8 boards, 3 lantern
posts, Keeper's Gate), Sexton's seated model, `Speaker_Sexton`, `BluffNest` (moved), DREDGER classes, the Unpaid, the
Ledger step.
**New in the world:** the Ferryman's landing (a 4 x 6 m timber stage hung under the deck's west end, a rope ladder; a small
cliff notch cut for it; an always-on fog card below it); Sexton's ferry on chains; 6 shrouded dead on the boards (shown
before Main 7's last scene); the hose-cart; a loudspeaker post at the Keeper's Gate. Retired: Sexton's railing seat on the
Point, the `LookoutStains` optional. Tripo: §13 items 25-27, 18.

## 5. Main 3: Under New Management (Delia; about 12 minutes)

**Why:** Sexton said "his own lantern"; it wasn't on Abel. The town is where his body went, and the Company holds it.

**Offer**, Delia on her porch (meet first):
- Delia: *"Tilly Bright laid my boy out. What the hose left of him."*
- Delia: *"If his lantern came home, she has it. Town's Company ground now. Mind yourself."*

**Card:** Abel's soul has to ride home in his own keeper's lantern, and nobody has seen it since Wednesday. Tilly Bright, the
undertaker, laid out his body. Vane's Company has taken the town, and its men hold the gate.

**Steps:**
0. **Take the farm road into town.** [Reach `Place_TownGate`] Company notices on every fence post (Side 1's posters).
   Vane on the gate's loudspeaker as Sully nears (below).
1. **Break the Company's checkpoint at the town gate.** [Clear `TownGate` 7] Rebuilt: sandbags, a striped barrier and a
   VANE CONSOLIDATED - ASSESSMENT IN PROGRESS board across the Gate Stones; 3 riflemen behind the sandbags, a hose-man, and
   3 Unpaid drawn by the smog (one Restless), four out at most (today's soft rule). Reason: nobody goes in or out while the
   town's dead are "assessed".
2. **Get to Bright & Daughter.** [Reach `Speaker_Tilly`, 8 m] The **Main Street set piece** (built, flag
   `SetPiece.MainStreet`): first push as built (Unpaid from under the boardwalks, the Bell-Ringer on the Gilt Spur's
   balcony) plus a Dredger hose-cart "assessing" the saloon; the second push from the alleys gains 2 riflemen. Doors carry
   chalked ASSESSED marks; shutters slam. Optional: **cut down the banner.** [Hit `Banner_Rope` 2] UNDER NEW MANAGEMENT
   across Main Street falls into the street.

**Twist:** the shutters slam for fear of the Company, not the dead: Vane promised the town that "any household harbouring a
Warden will be assessed first". Tilly opens anyway; she's been salting every coffin in her cellar so the hose can't take
them (seeds Knocking Below).

**Vane** (the gate's loudspeaker): *"Ah, the Warden. Main Street is closed for assessment."* / *"Your dead are being
counted. Please don't make me count you."* (Main Street's, as the street is won): *"Residents! Anyone hiding a Warden will
be assessed first."*

**Turned in to Tilly** (card, then; she hands the bail and the box as built, `hand_items`):
- Tilly: *"A Warden. Lord. Come close, the whole street's watching."*
- Tilly: *"His lantern wasn't on him. Only its bail, snapped off in his fist."*
- Tilly: *"Couldn't open his hand till morning. Here. And the box: he'd want it used."*

**Reuses:** the farm road, orchard, `Place_TownGate`, `TownGate` (rebuilt with Dredgers), the Main Street set piece and
safe-zone rule, shutters, `Speaker_Tilly`, `Item_LanternBail`, the reward gun, Main 3's id ties (safe ground, Pruitt's hatch).
**New in the world:** the checkpoint (sandbags, barrier, sign), the banner and its two ropes (shootable), ASSESSED chalk
decals on doors, loudspeaker posts at the gate and mid-street, a hose-cart at the Gilt Spur. Dredgers level 2-3. Tripo: 27,
28, 29, 18.

## 6. Main 4: A Coal Under Ash (Tilly; about 15 minutes)

**Why:** a keeper's lantern is lit from his saint; is she still alight? Sets the clock. Boss: the Doorward.

**Offer**, Tilly:
- Tilly: *"Father Aldana blessed Abel's lantern every spring. If Saint Ada's still lit, he'd know."*
- Tilly: *"He's barred in the vestry since Wednesday. The Company's at the chapel. Knock loud."*

**Card:** A keeper's lantern is lit from his saint, and nobody has seen Saint Ada burn since Abel was taken. Father Aldana
has barred himself in the chapel's vestry. A Company crew is in the chapel yard.

**Steps:**
0. **Go up the north road to the Chapel of Saint Ada.** [Reach `Place_Chapel`]
1. **Clear the chapel yard and pull the hose out of the chapel.** [Clear `ChapelYard` 16 + Interact, held 2 s,
   `Hose_ChapelWindow`] The door Abel held won't open, so the Company ran a hose through the broken side window to drink
   the saint. First push 8 Unpaid (the dead the crew spilled), second 2 riflemen + 2 hose-men working the hose, 3 Unpaid
   and the Restless "Leftover". Pulling the hose: it whips out of the window and coughs smog.
2. **Knock at the vestry door.** [Talk `Speaker_Aldana`]
   - Aldana: *"Stay back from that door!"*
   - Sully: *"Warden. Tilly sent me."*
   - Aldana: *"Then ring her bell. If she can still answer, she will."*
3. **Ring the chapel bell: hold {Interact} on the rope.** [Interact, hold, `Bell_Chapel`]
4. **Put the Doorward down.** [KillNamed `Legendary_Doorward`] **Set piece and boss.** The bell is the saint's call to her
   guard: the stone mourner by the door (a named Stone Sentinel at 1.5x, a stone lantern at its waist) shudders off its
   plinth, smog streaming from its hood, and comes at Sully: it can't tell a Warden from a Dredger. The Sentinel's whole kit
   (boulder, shockwave, slam, the open window, the rock in its grip shot to burst; `Docs/Pipeline/Enemies.md`), on the
   knoll among the churchyard's headboards (the crypt walls stop its shockwave). Named Legendary like the Gravemother:
   orange tag, its own Ledger page, **set level 4**, about 3,000 health (tune to 2-3 minutes), the Legendary table once.
   At its 50% two smaller Sentinels (the churchyard pair, woken early) join. **Twist:** at zero it doesn't break: it kneels
   back onto its plinth and the smog steams off it "like breath in winter". Aldana, through the door: it has held the
   door since Abel fell at its feet.
5. **Look at the Reliquary.** [Event `GraveSight.Reliquary`] Saint Ada's ember: a coal under ash, faint orange under grey,
   guttering. (Its look_when moves "from step 4" -> "from step 5".) Optional: **read the keepers' roll.** [Interact
   `KeepersRoll`] (new text) Every Ransom keeper since the saint; ABEL the last, and under him a blank, primed board.
   Hob: *"Nobody's name goes under his till somebody sits the night. Job's open, sunshine."* (seeds Main 7)

**Vane** (the chapel's loudspeaker, as the bell rings): *"Ringing for help? Charming."* / *"There's nobody left to come."*

**Turned in to Aldana** (card, then):
- Aldana: *"She's a coal under ash, Warden. Without her keeper she's going out."*
- Aldana: *"When she does, every grave on the Rest is open to that hose."*
- Aldana: *"I watched from the vestry. Ned Purcell wouldn't break Abel's lantern. Bad luck, he said."*
- Aldana: *"He threw it down the Sink. Spiders hoard what a saint has touched."*

Then Main 5 offered (one conversation). **From this turn-in Brother Finch stands at the Reliquary** (CREW's
`AWardenCrewMember` Keeper, a second placement, story-gated): he keeps the saint alive by hand until Abel is back. His topic:
*"I'll sit with her. Somebody has to keep her lit."*

**Reuses:** the north road, chapel, `ChapelYard`, `Speaker_Aldana`, `Bell_Chapel`, `AChapelReliquary`,
`GraveSight.Reliquary`, `KeepersRoll`, `AStoneSentinelCreature` and the user's Tripo statue (scaled), its climb-back and
Vigil Stone code, the four placed Sentinels, Saint Maud Uncommon reward, Finch's model.
**New in the world:** the Doorward's wide plinth by the chapel door; the side window broken with the hose rig through it;
hose-drag scars and a soot handprint on the door Abel held (decals); the Reliquary **whole** with a guttering ember state
(the old smashed look retired); a loudspeaker post by the lychgate. Tripo: 32, 33, 13, 18.

## 7. Main 5: Choke Point (Aldana; about 15 minutes)

**Why:** the lantern is in the Sink, and so is the barge's mouth: one trip, two jobs (the lantern; starve her).
Boss: the Gravemother (Decision 2 A).

**Offer**, Aldana (one conversation after Main 4):
- Aldana: *"Bring up Abel's lantern. And that hose drinks from the Sink's floor."*
- Junie (the tube): *"Choke it? Salt round the intake collar and blow it. Hob's bringing you charges."*
- Hob: *"I'm a lookout, not a mule. ...Fine."*

**Card:** Ned threw Abel's lantern into the Sink, where the spiders hoard what a saint has touched. The barge's hose drinks
from the Sink's floor too. Junie says three salt charges on its intake collar will choke it: starve the barge, Sexton said,
and Ned has to come down.

**Steps:**
0. **Take Junie's salt charges from Hob, and climb down into the Sink.** [Collect `Item_SaltCharges` at the ramp head +
   Reach `Place_SinkFloor`, height counted] Hob drops a canvas satchel on the rim.
1. **Shoot down the three egg sacs.** [Event `EggSac.Burst` x3] The hoard is behind them (the floor's spiders as built).
2. **Take Abel's lantern from the webbing.** [Interact `Lantern_Keeper`] Must stay step 2 (`AKeepersLantern::TakeStep`).
   Hob (kept): *"Dark as a shut eye, but whole. Missing its bail. You've got that, sunshine."* The bail goes on here
   (`Item_LanternBail`'s mission moves from Main6 to Main5).
3. **Choke the hose: set three salt charges on the intake.** [Interact, held 1.5 s, `Intake_Charge` 3] **Set piece "the
   Intake":** within 10 m the intake's draw pulls Sully toward its mouth (the hose-man's pull, scaled up; the cyan-black
   edge; a drain only inside 3 m), so every charge is set leaning against it. Dredgers **rappel down the hose** from the
   barge (2 riflemen, 2 hose-men, level 4-5; a new "Rappel" entrance like Rise). Ned speaks for the first time. The third
   charge set: they blow (event `Intake.Choked`): the hose bucks like a snake, coughs a black cloud and hangs limp; the barge
   lurches overhead and two of its three plumes die.
4. **Kill the Gravemother.** [KillNamed `Gravemother`] **Twist and boss:** she has been drinking the smog that pools at her
   den's mouth; choked, the pool drains, and she comes out starving. Her Barrow fight as built (headstone gate, furrow,
   grave clod, unearthed brood; the rim-shot rule), **set level 5** for this (was 6; Decision 2). Her fang drops (Side 3's
   item) and her Vigil Stone rises.
5. **Climb out of the Sink.** [Reach `Place_SinkRim`] Optional (kept): **look at the hat by the den.** [Interact
   `SheriffHat`] W. ODELL. Hob: *"Whatever lives in that hole had the sheriff for supper."* At the rim: the barge swings
   north and drops a **second hose onto Larkspur Ridge**. Hob: *"She's not starving yet, sunshine. She's found the mine."*

**Ned** (the barge's horn, as the first charge goes on): *"Evening, Warden! Lucky Ned. That's my supper you're fiddling
with."* (choked): *"...Well. That's bad luck. Yours, not mine."* **Vane** (the Sink's loudspeaker): *"Ned, my furnace is
coughing."* / *"Fix it, or I'll feed it you."*

**Turned in to Brother Finch, at the Reliquary** (card, then):
- Finch: *"Abel's lantern. Whole. Ned's luck is ours, Warden."*
- Finch: *"It'll stay cold till his soul's in it. Then it'll burn like a sunrise."*
- Junie (the tube): *"That second hose is down the old mine's air shaft. Up the ridge."*
- Hob: *"Old ore lift at the quarry. Somebody dead's sat by it four years."*

`SmogThin` from this turn-in.

**Reuses:** the Sink, ramp, egg sacs, the floor's spiders, `AKeepersLantern`, `SheriffHat`, the Gravemother, her Barrow fight,
fang and Vigil Stone, WORLD's intake and hose, Hob.
**New in the world:** the satchel; three clamp spots on the intake collar; rappel ropes down the hose; the limp hose (a
second baked shape after `Intake.Choked`); the smog pool at the den mouth (a 0.5 m dip in the Sink floor's terrain with a
black glinting water plane, drained after); the second hose from the barge north to a new air-shaft head on Larkspur High
(shown from step 5 until Back Pay is done); the Sink's loudspeaker on the warning fence. Tripo: 30, 46, 13.

## 8. Dead Lift (kept title; Hiram; about 15 minutes)

**Why now:** the second hose is in the Providence; the Company climbed the ridge by rope and cut it behind them; Hiram's
lift is the only way up. Steps as built (the talk cut: only Abe's pin; the crank stays on the winch; the boiler takes the
quarry's coal; raise steam; ride).

**Offer**, Hiram on his stool (met):
- Hiram: *"Ain't had a passenger in four years. Now Company men, climbing my ridge on rope."*
- Hiram: *"Cut it behind 'em. You'll want my lift. She wants her clamp pin back."*
- Hob (kept): *"Dead lift. I'll let you have that one, sunshine."*

**Card:** The barge has dropped a hose down the Providence's air shaft, up Larkspur Ridge, and the Company's men climbed up
and cut their rope behind them. The old ore lift is the only way up, and its dead engineman still waits by its boiler. Its
clamp pin went down the hill in Abe Haskett's pocket the day of the fall.

**Steps:** as built. One change: **RaisingSteam's second wave** is 2 Dredger riflemen and a hose-man coming down the quarry
track to stop the lift (in place of its 2 dead miners; level 5). Hiram: *"Company says nobody rides. Company can wait."*
The frayed Company rope and grapple hang on the ridge face by the Goat Steps' top (story prop; Tripo 15).

**Turned in to Hiram** through the tube (card, then):
- Hiram: *"Hear that? She still runs."*
- Hiram: *"Company's in the Providence. They've got Cyrus Teague driving the dead shift again."*
- Hiram: *"There's a carbine in the head's tool box. Company's. They'll never miss it."*

Then Back Pay (met, as built).

## 9. Back Pay (kept title; Hiram; about 22 minutes)

**Why:** cut the barge's last mouth. **Boss:** Cyrus Teague, the Foreman (kept), now driving his dead for the Company.

**Offer**, Hiram up the tube (met):
- Hiram: *"Teague brought the roof down on eleven men to save a quarter's wages."*
- Hiram: *"Now the Company's paying him to dig 'em out. For the barge. Eleven souls, four years ripe."*
- Hob: *"Company man to the end, sunshine. Past it, even."*

**Card:** Four years ago the Foreman, Cyrus Teague, brought the Providence's roof down on eleven striking miners. Now Vane's
Dredgers have Teague's dead shift digging the eleven out of the fall, to feed them up the barge's hose. Their tags still
hang on IN at the tag board.

**Steps** (built ones kept; Dredgers and the hose added):
0. **Go into the Providence.** [Reach `UpperGallery`] The Diggings are a Company camp: tents, VC crates, a pump on the new
   air-shaft head, a loudspeaker. Ambush Spoil: 3 dead miners rise out of the spoil heap and 2 riflemen + a hose-man come
   out of the tents (level 5-6).
1. **Clear the upper gallery.** [Clear `MineUpper` 7] 4 dead miners + the Restless one, working under 2 Dredger riflemen.
2. **Search the Foreman's office.** [Interact `PayrollBook`] The payroll and Teague's last entry (kept), and on top a new
   page: *VANE CONSOLIDATED. C. Teague, Foreman (deceased), retained to recover eleven (11) head, lower drift. Rate: one
   soul's rest per head delivered.* Hob: *"Paid in his own rest. That's the Company for you."* Optional (kept): the chalk
   tally.
3. **Go down the decline and clear the lower drift.** [Clear `MineLower` 6] (kept)
4. **Take the eleven's tags from their barricade.** [Interact `Barricade`] PAY US AND WE COME UP (kept). Past it, the
   barge's hose hangs out of the stope's roof over the fall.
5. **Put the Foreman down.** [KillNamed `Legendary_Foreman`] His fight as built, plus: **the hose's draw** at the fall (Main
   5's pull) during "The shift", dragging toward the rubble while his miners rise. Lines: kept, but in the dark: *"I can dig
   them out. The Company promised. I can still dig them out."*; his last word (kept): *"...Tell the Company it was the
   roof."*
6. **Cut the hose and take the payroll.** [Interact, held 2 s, `ShaftHose` + Interact `PayrollChest` -> Collect `Payroll`]
   Cut, the hose whips up the shaft and away; Ned on the barge: *"Ned's not lucky tonight. Ned's hungry."* The cave-in set
   piece on the way out (kept).

**Turned in at the tag board**, the choice (kept):
- **Pay the eleven.** The tags go to OUT. Shift boss (kept): *"Paid in full. Shift's over, boys."* New: eleven lights rise
  out of the mine mouth and drift west, past the barge's reach. Hob (kept): *"First honest pay this mine ever made."*
  Vane (the Diggings' loudspeaker): *"You PAID them? With Company silver?"*
- **Keep the payroll.** About 400 silver; the tags stay on IN, the dead keep working. Shift boss (kept): *"...Company man
  after all."* Vane: *"Now that's a Company man. Ned, hire that Warden."* Hob (new): *"Can't spend it in the ground,
  sunshine. Neither could they."*

After either: Hob: *"That's both her mouths shut. Ned's starving now, sunshine. Back to the ship."* (Holloway's ? for Main 6.)

**New in the world:** the Company camp at the Diggings; the air-shaft head on Larkspur High above the stope and a shaft
opening in the stope's roof with the hose hanging over the fall; the contract page; the cut hose's collar. Tripo: 46, 47.

## 10. Main 6: The Pilot Light (Holloway; boss Lucky Ned; about 20 minutes)

**Why:** starved, Ned is bringing the barge down over the chapel at dusk to hose Saint Ada herself. It's now or never: the
one time she's in reach. Accepting fades the Rest to dusk (today's `AStoryLighting` Dusk rule for Main 6).

**Offer**, Captain Holloway at the Lamplighter's wheel on the Mooring Ledge (meet first):
- Holloway: *"Ned's starving. He's bringing her down over the chapel to drink Saint Ada dry."*
- Junie: *"Bag's patched. Their grapples were still in my hull, so I re-rigged them. Waste not."*
- Holloway: *"We go now, Warden. Bring Abel's lantern."*

**Card:** Starved of both his hoses, Lucky Ned is bringing the barge down over the chapel at dusk to hose Saint Ada herself.
The Lamplighter is patched, and Junie has re-rigged the Company's own grapples. Board the barge, beat Ned, and pull Abel out
of the furnace in his own lantern.

**Steps:**
0. **Cast off: hold {Interact} on the anchor winch.** [Interact, held, `Landfall_AnchorWinch`] Landfall's winch, reversed.
1. **Fight off the Company's tenders.** [Event `Pilot.TendersDown`] The Lamplighter flies a new course from the ledge east
   over the farm and the town (about 90 s, FLIGHT's moving deck). Two Company tenders come at her: riflemen fire from their
   decks; **a tender's gas-bag is its weak spot** (shot enough it tears, and the tender falls burning into the fields: a big
   fall, a crash and a smoke column). One grapples on and boards 2 riflemen + a hose-man (Landfall's boarding, reused).
   Junie: *"Shoot their bags! They burn lovely!"*
2. **Grapple the barge.** [Interact, held 1 s, `Lamplighter_GrappleLauncher` 2] The two launchers on the starboard rail
   fire; the chains pull the ships together over the chapel knoll and a gangplank drops (scene, 3 s).
3. **Fight across the barge's top deck.** [Clear `BargeDeck` 9 + Reach `Barge_FurnaceHatch`] About 70 m of riveted deck:
   three stacks, a crane, the hose drum gantry, VANE CONSOLIDATED painted under foot. 6 riflemen in cover, 3 hose-men, level
   6. Hazard: furnace vents along the deck flare every 6 s (a 1 s glow tell, then a burn column). The barge's horns carry
   Ned and Vane bickering. Optional: **silence the barge's horns.** [Hit `Barge_Horn` 4]
4. **Take Ned's manifest from his office.** [Interact `Barge_Manifest`] Sexton's price, off the furnace hall's catwalk: a
   thick VC ledger, 412 names (the Rest's and four other towns'). Sully: *"Four hundred and twelve."* Hob: *"Vane's fare
   bill. The Ferryman'll frame it."*
5. **Beat Lucky Ned.** [KillNamed `Boss_Ned`] The fight below.
6. **Pull Abel from the furnace.** [Interact, held 4 s, `Furnace_SoulCage`, with the lantern carried] The hall shakes,
   alarms, steam; Abel's light pours out of the cage into his lantern, and it lights gold for the first time (Finch's
   sunrise).
7. **Get off the barge!** [Reach `Lamplighter_Deck`] The escape: the barge lists 12°, its back half burning; a stack
   topples across the route (scripted); fleeing Dredgers jump for a tender; Holloway brings the Lamplighter under the
   broken gangway: *"Jump, Warden!"* (Hob's fall catch as everywhere.) Scene **`BargeFall`** (~12 s, skippable) from her
   deck: the barge drifts west on the dusk wind, raining soul coal over the valley, and goes down past the Rim into
   Gravewind Canyon: a fireball, a smoke column; the smog's source gone.

**The boss: "Lucky" Ned Purcell.** A living man, the Dunne Gang's gambler, Vane's man on this barge (and the one who put
the hose to Abel). Brocade waistcoat under an open Dredger coat, a bowler with the ace of spades in its band, his gas mask
on his chest, two nickel revolvers. **Set level 7, Boss rank** (Abel's numbers carried over: about 15,150 health at 7, hits
x1.25, tune to 4-5 minutes); crits: his head and the dealer's shoe on his belt. **Arena: the furnace hall**, a round
riveted room 26 m across and 9 m high: the soul furnace in the middle (6 m, a glass-windowed cage at its heart where Abel's
light beats like a heart), a floor ring of 12 grate segments round it, a catwalk ring at 4 m, 3 draft valves on the walls,
2 stokehold doors.
- **Phase 1, "House Rules" (100-60%).** Ned works the catwalk and floor; his bullets ricochet once off iron, bending toward
  the player (a visible tracer at the bounce). Every 15 s he deals a card from his soul-fired shoe: it spins glowing over
  his head for 2 s (the tell): **Ace of Spades** (his next 6 shots crit), **the Joker** (2 riflemen through a stokehold
  door), **Aces and Eights** (bad luck: he's open 3 s). **Shoot the card in its 2 s** and it burns: no effect, he staggers
  (*"That's cheating!"*). (The Sentinel's rock-in-the-grip counter, the bell's tell, in a new shape.)
- **Phase 2, "Double or Nothing" (60-25%).** He throws the furnace's draft open: the grate ring heats like a roulette wheel
  (4 of 12 segments glow, then burn, the pattern turning every 3 s); Abel's light flares. Finch on the tube: *"He's burning
  Abel faster! Shut the drafts!"* Hose-men come through the stokeholds (2 at a time, 4 in all). **Shut the 3 draft valves**
  (hold 1.5 s each, under fire: Abel's old lantern-relighting pattern): each cools the ring and staggers Ned 3 s; all three
  end the phase.
- **Phase 3, "All In" (25-0%).** Ned plugs a soul-hose from the furnace into his coat's harness: a cyan-black soul ward over
  his health that refills while the hose is on. **Break the hose** (3 collars, the hose-man's line rule): the ward drops,
  he's stunned 3 s, and re-hooks 12 s later. Both revolvers fanned, faster.
- Lines: *"Cut the deck, Warden?"* / *"House always wins. I'm the house."* / *"Lady Luck, don't you dare."*
- **At zero** he kneels by the furnace, laughing: *"Lady Luck. Always ran off with somebody else."* **Twist:** Vane on the
  hall's speaker: *"Ned. Ned, Ned, Ned. You've lost my barge."* / *"You're fired."* The furnace's safeties blow from afar
  (*"Company property does not fall into Warden hands."*); Ned: *"Mister Vane, wait, I've still got the—"* and the back
  wall goes out with him (Decision 4). Step 6 starts in the alarms.
- **Loot:** Abel's Boss table, once (3 guns at Luck 1.3, 30% legendary). No rematch (the barge is gone; Decision 4).

**Turned in to Holloway at the wheel** (card, then; the ship sets back down on the ledge as she talks):
- Holloway: *"Eleven years since Vane's first barge took my town."*
- Holloway: *"That's one out of the sky. That one's for them."*
- Holloway: *"Take him home, Warden. Finch is holding her at the chapel. Barely."*
- Hob: *"Coal all over the valley, sunshine. Every lump of it's somebody."*

Then Main 7 (one conversation).

**Reuses:** `ALamplighterShip` and its moving deck, `ACompanyTender`, the grapples (reversed), the winch, DREDGER classes,
the Dusk rule, Abel's Boss table and numbers, the relight-under-pressure pattern (draft valves), the hose-man's line break
(Ned's hose), `UBossComponent`, Hob's fall catch.
**New in the world (the big piece):** the barge's **boarding route** as a walkable kit (top deck, stacks, crane, hose drum,
vents, horns; the furnace hall interior with its furnace, cage, grate ring, catwalk, valves, stokeholds; Ned's office),
built like the Providence's interior (its own mesh kit, shown only during Main 6, culled; Main measures perf); the barge's
**second placement**, lowered to about 25 m over the chapel knoll for Main 6; the Lamplighter's second course (ledge ->
chapel) and its two grapple launchers; tender gas-bags as a weak spot (code); the `BargeFall` scene; the **wreck** in
Gravewind Canyon below the Rim south of the boards (backdrop only, a smoke column) after Main 6; **soul coal strewn** across
the valley after Main 6 (instanced glowing lumps, 12 of them interactable for Main 7). New code: the Ned boss
(`ANedPurcellBoss`: cards, ricochets, ring, valves, ward hose). Tripo: 20-22, 34-44, 45, 48, 31.

**Code ties retired:** Main 6's Abel step constants (HangStep 1, FightStep 2, SceneStep 3), `SitWithPa`, Delia's
`Delia.Main6` topic, Abel's Vigil Stone by the Keeper's Gate (Decision 5).

## 11. Main 7: Given to the Wind (Holloway; about 18 minutes)

**Why:** Abel burns in his own lantern now, and Saint Ada is nearly out. Then the keeper's job, done once by a Warden.
Dusk all through (Main 6's rule continues), clean gold after the relighting.

**Offer**, Holloway (one conversation after Main 6):
- Holloway: *"The Lantern says Finch is still holding her. Barely."*
- Hob: *"Take him the way he ran, sunshine. Past his ma's porch."*

**Card:** Abel's soul burns in his own lantern now, and Saint Ada is almost out. Carry him to the chapel the way he ran,
past Delia's porch, and put him back where he belongs. The barge rained soul coal over the valley as it fell: every lump is
somebody.

**Steps:**
0. **Carry Abel home, the way he ran.** [Reach `Place_Chapel`, the lantern carried, lit] Landfall's trail backwards: the
   cliff stairs, Ransom's Point, the bluff path, the farm, the farm road, the north road. The lantern draws trouble:
   **Dredgers who bailed out of the barge** (3 riflemen, 2 hose-men on the farm road, level 6), and **their hose-men latch
   the lantern, not Sully**: a line on it drains Abel's light (the lantern dims on the HUD's carried item; break the line;
   nothing fails, the light refills once free); and the dead drawn to his light (OrchardEdgeDusk, built). At the farm Delia
   comes down her steps: *"Abel? ...Go on. Don't stop for me. I'll come up."* Optional: **free the souls in the fallen
   coal.** [LastingInteract `SoulCoal` 12, held 1 s] Each lump cracks; a soul rises and follows the lantern (a growing
   trail of wisps behind Sully).
1. **Hold the chapel yard.** [Clear `ChapelLast` 8] Vane's last tender hangs over the knoll and drops 6 riflemen and 2
   hose-men by rope: *"If I can't have the keeper, nobody can."* **Payoff:** the Doorward sees the lit lantern, rises and
   fights beside Sully (its boulder thrown at the tender drives it off; Decision 3), then kneels and swings the chapel door
   open.
2. **Put Abel's lantern on the Reliquary.** [Interact, held 3 s, `Reliquary_Lantern`] Scene **`SaintAdaWakes`** (~25 s,
   skippable): Abel's light pours into the Reliquary; the coal under ash catches; Saint Ada blazes white-gold; a ring of
   light washes out over the valley and the smog burns off in a wave (Smog -> clean dusk); across the Rest the Unpaid go
   quiet and sink into their graves. Abel stands by the Reliquary, a soul in his keeper's coat; Delia has come up the north
   road:
   - Abel: *"Ma. You came up the hill. Your knees."*
   - Delia: *"Hush. You held the door."*
   - Abel: *"Warden. A week of the Rest's dead, and nobody's walked them."*
   - Abel: *"Take my lantern to the boards. She'll light it. I can't leave her now."*
   The emptied lantern relights from the saint, gold, as a keeper's should; Abel hands it over.
3. **Walk the dead to the burial boards.** [Reach `Place_GravewindPoint`] The procession: every freed soul (the coal's, the
   boards' six, the week's dead) follows the lantern as a river of lights. **Bone Hounds** (the placeholder pack, 4 and a
   Restless; moved to this step from "after Main 4, before Main 6") come at the procession on the keeper's road: they raid
   boards no keeper sits up with. Optional (kept): **by the keeper's path.** [Reach `Place_KeepersBridge`] Hob: *"Every
   Ransom walked the dead this way at sundown. Tonight it's you."*
4. **Hang the lantern on the keeper's post.** [Interact `LanternPost_Keeper`] Scene **`GivenToTheWind`** (~15 s): the
   Gravewind pours off the Rim; the shrouded dead's souls lift off the boards and the river of lights streams west into the
   sunset, past the wreck's smoke, down the Sundown Road. Far off on the ledge, the Great Lantern swings north-east.
   Optional: **pay Sexton his price.** [Talk `Speaker_Sexton`, at his ferry below the boards] Sexton: *"Four hundred and
   twelve. Vane owes me a fortune, friend."* / *"A tip, for prompt payment: the next barge is over the Gilded Lily."*
   (and a Rare gun from the ferry's bilge).

**Vane** (the last working loudspeaker, as the smog lifts): *"That was a very expensive barge, Warden."* / *"I have forty
more. Do come and see them."*

**Turned in to Delia, on the burial boards** (card, then; Heirloom by hand, `hand_items`):
- Delia: *"Thirty years he walked them. Tonight a Warden did."*
- Delia: *"He kept his iron back for me. It's yours now."*
- Delia: *"Hold the door, Warden. Wherever that lantern of yours leans next."*
- Holloway (the tube): *"Lantern's leaning north-east, Warden. We fly at dawn."*

Then the next region opens (Decision 6). After Main 7: Abel stays at the Reliquary as a friend (`Speaker_Abel` moves from
his board to the chapel; his lines after each later region), Finch back aboard, Delia on her porch, Abel's grave thawed,
the keepers' roll's blank board reads SULLY GRAVES, WARDEN. ONE NIGHT.

**Reuses:** the Keeper's Lantern (lit look), the Reliquary (a lit state), `LanternPost_Keeper`, the keeper's path, west road,
cairns, Keeper's Bridge, Gravewind Point and the boards, OrchardEdgeDusk, the Bone Hound placeholders, Abel's model (as his
soul, friendly), the old SitWithPa scene code's pattern, Heirloom, the Dusk rule, the Doorward, Sexton's ferry.
**New in the world:** soul coal lumps (12 interactable); the procession (instanced wisps following the player, capped and
cheap); the light wave (a lighting blend Smog -> clean); Abel's soul by the Reliquary; Delia's story positions (chapel,
boards); the hose-man's lantern-latch (a target switch); a friendly Doorward (Decision 3 A). Tripo: 23, 31.

**Code ties retired:** Main 7's step 0 `ADoorHandoff` (Heirloom now by `hand_items` at the turn-in), Tilly as Main 7's
turn-in.

---

## 12. The sides that told the old premise

Each keeps its id, steps and XP; only the words and the reasons change, plus noted fights.

### Side 1: Dead Preferred (was Wanted: Already Dead; the gate's poster)
**Card:** WANTED BY VANE CONSOLIDATED: ONE LANTERN WARDEN. 500 SILVER. DEAD PREFERRED: WE CAN USE THE PARTS. Somebody has
written TRY IT under the last line. There's one on every post in town and on both farms.
**Steps:** as built (tear down 4, hold; the Rangers' board note; optional the sheriff's daybook, kept). New: at the fourth
poster, **bounty hunters**: today's Gravepickers (placeholders, living bandits on Company piecework) spring 3 + a Lookout
for the 500.
**Turned in to Tilly** (card, then): *"So it was you, tearing them down. Good. They were bad for trade."* / Sully: *"Who
wrote TRY IT?"* / *"I did. On the first one. Somebody had to."* (two kept) The Rare gun: *"A Ranger left this box years
back. Use it."*

### Calder's Caches (Ruth's note)
Steps kept. **The strongbox:** Sully doesn't know a combination now; Hob does it: *"Crows hear tumblers, sunshine. Left,
right, left."* Inside (in place of the Deacon's "Kid's share" slip): a Company contract, *Dunne & Co., soul recovery,
Ransom's Rest. One keeper (premium). Paid on delivery. C. Vane*, and the Deacon's note, *Ned: the keeper, not the saint.
The saint is Company business. A.D.* (seeds Main 6: Vane always meant to take Saint Ada). Hob: *"Premium. He'd have liked
that, your keeper."* Turn-in at the Rangers' board, Hob: *"Three tags for the Rangers. Ruth Calder'll want a word with
your captain."*

### The Old Fare (Mr. Sloane)
Steps kept. **Offer:** *"Who's there? ...A Warden? Lord. I heard your ship come down."* / *"Every night since Wednesday
they stand in my waiting room like they've a train to catch."* / *"You see the dead. Maybe they'll listen to you."*
**Why they wait:** for the Company's coal train on the siding (the hopper cars stencilled VC), which would carry their
kin. **Twist (deeper):** the desk's marker payable to N. Purcell now has a note pinned to it: *Keeper's hours. Debt
cleared. N.P.* Sloane sold Ned the hour Abel locked the chapel alone. Sully: *"Ned's paper."* Hob: *"He sold the keeper's
hours, sunshine. For a marker."* Fenn's turn-in (kept) lands harder: *"Tell Sloane I forgive him. Don't tell him I said it
like that."* After Main 7 Sloane's window line *"Line's open to the Lily, kid. Your car's coupled."* -> per Decision 6.
**New:** 2 hopper cars on the depot siding (Tripo 49, optional), soul coal sacks on the platform.

### Knocking Below (Tilly)
Steps kept. **Offer:** *"You see the dead, Warden. So you'll understand me."* / *"I've four in my cellar, salted, hid
from the hose till they can go to the boards. Something knocks every night."* / *"I'd go down myself, except I'd rather
not."* **Card:** Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up, salted now so the hose
can't find them. Since Wednesday something down there knocks all night. **Thaddeus** (home from the Sundown Road when the
saint dimmed): *"A Warden. I measured a Warden once. Tall. Pine."* / *"I kept them in their boxes a week. They got past me
when the door opened. A cellar wants keeping."* / *"Give her my tape. Tell her the brass was too much. And that I liked
it."* (two kept) "His car is the hearse car Ellis rides in" -> per Decision 6.

### The Last Watch (Calder's Post)
Steps kept. **Ruth's journal** (night of the robbery): *Barge came over the East Ridge Wednesday dusk, big as a church,
running dark. Skiff down to the chapel. Abel Ransom at the door with his shotgun. They put a hose on him. Never saw a man
hold so long. Rode down. Too late.* Her last: *Riding south after Purcell's skiff. If a Warden reads this: your ship's
late. Probably not your fault.* Hob: *"...She saw it all. And she still rode down."* **The decoded answer:** *LARKSPUR WE
SEE YOU. CALDER RIDES SOUTH. ALL POSTS: COMPANY FLEET ON THE RIM. WARDENS DOWN ON RANSOM'S.* Sully: *"Word travels."* Hob:
*"Faster than you do."* (kept) Why light the fire, now: dark posts pull back, and the Company takes the Rim.

### Side 2: Unfinished Business (Amos) and Side 3: The Gravemother (the sheriff's bounty)
- Side 2: steps and twist kept; "only Ellis can see him" -> a Warden's eyes; he drifted home when Saint Ada dimmed. Payoff
  after Main 7 (new): *"Saw her light come back over the knoll. Reckon I'll go at sundown."*, and he's gone from his fence.
- Side 3: with Decision 2 A it opens after Main 5 as **the bounty**: the kill in Main 5 counts ("a kill on an earlier visit
  counts", built), take the fang to Mrs. Pruitt; steps and turn-in kept. Hob, new: *"She was big before the smog, sunshine.
  The smog made her bigger."* With 2 B, unchanged.

(Lost and Found tells nothing of the old premise: unchanged.)

## 13. Models to add to `TRIPO_MODELS.md` (numbered after 19, Abel's hat)

Stand-ins until they land, one soft path each (the contract's rule). Sizes real-world; people A-pose, empty hands.

People:
20. **Lucky Ned Purcell.** Forties, lean, 1.78 m: brocade waistcoat and shirtsleeves under an open black Dredger rubber
    coat, a bowler with an ace of spades tucked in the band, a gas mask hanging on his chest, a gold watch chain, two empty
    holsters, a gold tooth.
21. **Ned's revolvers.** A pair of nickel-plated revolvers, ivory grips inlaid with dice pips.
22. **Ned's dealer's shoe.** A brass and iron card shoe on a belt clip with a small glowing furnace grille, 25 cm.
23. **Abel Ransom.** Sixties, tall, 1.85 m: a long keeper's coat, grey beard, a keeper's lantern hook on the belt, the
    wide-brim hat (19). Shown as his soul (gold-white material) in Main 7 and after.

Props and places:
24. (none: Sexton's seated model is reused in the ferry's stern.)
25. **Sexton's ferry.** A long black wooden ferry, 9 m: a high curved prow with an iron lantern pole, rotted planks, one
    long sweep oar, chain eyes at bow and stern (it hangs on chains).
26. **Shrouded dead.** A body wrapped in a pale sheet tied with rope, lying, 1.8 m; and a small one, 1.1 m.
27. **Company hose-cart.** A two-wheeled iron pump cart, 2.2 m: a riveted tank with gauges and a valve wheel, a hand-crank
    pump, a coiled black hose on a reel, a brass VC plate.
28. **Company checkpoint.** A striped barrier pole on a post (4 m), a sandbag wall section (2 m), a sign board "VANE
    CONSOLIDATED - ASSESSMENT IN PROGRESS".
29. **Company banner.** A 12 m canvas banner "UNDER NEW MANAGEMENT" with VC lettering, rope ties at each end.
30. **Junie's salt charges.** A canvas satchel; each charge a bundle of three salt jars wrapped in wire, a fuse and a magnet
    clamp, 40 cm.
31. **Soul coal.** A lump of black faceted coal with glowing cyan veins (30 cm); a burlap sack stencilled VC.
32. **The Doorward.** A hooded stone mourner statue, 3 m, robed, both hands on a stone lantern hung at its waist, worn and
    lichened; for the rigid-piece cut like the Stone Sentinel (fists free of the robe).
33. **Saint Ada's Reliquary, whole.** Granite, banded in gold, a recess on the lid for the ember (the backlog design,
    unbroken), 2.2 m.

The barge's boarding kit (walkable; Main adds collision; Company iron, rust and VC paint):
34. Riveted iron deck plate sections, 10 x 10 m, with a hatch variant; railing sections.
35. Smokestack, 3 m across, 14 m tall, an iron ladder up one side.
36. Deck jib crane, 12 m.
37. Hose drum on its gantry frame, drum 6 m across.
38. Furnace vent grille for the deck, 2 x 2 m, glowing slots.
39. **The soul furnace.** A 6 m iron furnace with pipes, a glass-windowed soul cage at its heart.
40. Draft valve: a big wheel valve on a pipe run, 1.5 m.
41. Furnace hall shell: a round riveted room, 26 m across, 9 m high, a catwalk ring at 4 m, two stokehold doors (or kit
    pieces Main assembles).
42. Floor grate wedge (12 make the ring round the furnace).
43. Ned's office set: a desk, an iron safe, the manifest (a thick ledger with brass VC corners), a card table with chips.
44. Barge horn cluster: brass horns on a bracket (as 18's horns).
45. **The barge wreck:** the barge (12) broken in two and burning, for the canyon floor (seen from afar).
46. Company air-shaft head: a timbered shaft head with a pulley frame, 4 m, for the Providence.
47. Company camp: a canvas wall tent with a VC stencil, stacked VC crates (3 sizes).
48. Grapple launcher: the tender's harpoon-grapple launcher (14) as its own model, to mount on the Lamplighter's rail.
49. (optional) Company hopper car: a rail coal hopper in VC paint, for the depot's siding.

## 14. Notes for Main (what the build will need)

- **New code, small:** `ACompanyLoudspeaker` (shootable horns that cut a broadcast), the Rappel entrance, the intake's draw
  zone (Main 5, Back Pay), tender gas-bag weak spot, the hose-man's lantern target (Main 7), the soul procession
  (followers), scenes `BargeFall`, `SaintAdaWakes`, `GivenToTheWind`, the Reliquary's guttering and lit states, `SmogThin`.
  **Bigger:** `ANedPurcellBoss` and the furnace hall; the barge's walkable kit; the friendly Doorward (Decision 3 A).
- **Step indices that code reads:** Main 2's Ledger step stays 3; Main 5's lantern step stays 2; Main 4's Reliquary look
  moves to step 5; Main 6's Abel constants and Main 7's `ADoorHandoff` retire; `LanternBail` item -> Main5.
- **Pacing (`Looter.Progression.Pacing`, `Tests/PacingPaths.h`):** Dredgers join Mains 2-7 and the mine (about +60 kills on
  the story path at levels 2-6), the Doorward (level 4) and the Gravemother (level 5, Decision 2 A) move into the story,
  Ned replaces Abel (same Boss kill XP at 7). Trim mission XP, not kills, to hold 6.7-7.1 (story) and 7.3-7.7
  (everything); the bosses' arrival checks will need the Doorward and the Gravemother added.
- **Performance:** the boarding kit and furnace hall only during Main 6 (story-gated, culled; consider a streamed sublevel),
  the procession's wisps capped, coal lumps instanced; `Tools\perf.ps1` on the deck and in the hall.
- **Words:** the Seven-Days-era topics of Delia, Tilly, Aldana, Hob's arrivals, Pa's board and the heroes' pick blurbs need
  the same pass (each mission's round).

## 15. Decisions for the user (recommendation first)

1. **Where Sexton sells his secret (Main 2).** A) His ferry hanging in the fog under Gravewind Point (new landing; the
   Point was just used by Landfall) · B) The Point's railing, as today (no build).
2. **The Gravemother.** A) Main 5's boss when the hose is choked (set level 5; Side 3 becomes the fang bounty after) ·
   B) Stays optional (Side 3); Main 5 ends on the intake set piece.
3. **The Doorward (a named Stone Sentinel) in Main 4.** A) Yes, and in Main 7 it fights beside you (new: a friendly
   creature) · B) Yes; in Main 7 it only kneels and opens the door · C) No Doorward; the Sentinels stay as placed.
4. **Lucky Ned's end.** A) Vane fires him and blows the furnace with him in it · B) He jumps to a tender and gets away,
   back in a later region · C) Sully takes him alive for Ruth Calder's Rangers.
5. **Abel's old boss fight at Gravewind Point.** A) Retired: Ned is the Rest's last boss; Abel's rig becomes his soul at
   the chapel · B) Kept only as a Vigil Stone rematch ("the Keeper's echo") · C) Kept in Main 7: the furnace scorched his
   soul wild, and you calm him at the boards.
6. **How Sully leaves the Rest.** A) On the Lamplighter: her chart table replaces the station board; the depot stays the
   Company's coal yard · B) By train as today (Tilly's hearse car) · C) Both: the Lamplighter for the story, the train for
   return trips.
7. **Where the barge falls.** A) Into Gravewind Canyon on the dusk wind (a wreck below the Rim; nothing played on breaks) ·
   B) Into the Sink (a wreck to explore later; the Gravemother's floor must move).
8. **Sexton after Main 2.** A) Stays at his ferry and sells secrets for silver (marks the Rest's 15 finds on the map) ·
   B) Story only (Main 2, Main 7's payment).
