# Mains 2-7, the mine and the Revenant sides, in the Wardens story (design, 2026-10-31 night; revised to the user's calls)

Lane STORY2. Design only: nothing built, no data or code touched. For the user's approval, then one mission at a time (the
build order in `DECISIONS.md`). Read with `DECISIONS.md` (picks, every rejected idea, and "Mains 2-7 calls", which this
revision follows), `CONTRACT.md` (Landfall), `lanes/STORY.md` (Landfall's lines), `Docs/Areas/RansomsRest_Expansion.md` §2-4
(today's missions), `Docs/Areas/RansomsRest.md` (places, bosses), `Docs/Progression.md`.

Rules kept: mission ids unchanged (`Main2`..`Main7`, Dead Lift, Back Pay, the sides; saves and conditions key on them);
steps counted from 0 as the scripts count; every talk or board turn-in is **card first, then 2-4 lines**; captions under
90 characters; Sully is "they", Delia calls them "Warden", Hob "sunshine", Sexton "friend"; XP per mission kept unless
noted (pacing note in §14). Rejected ideas not reused (no grave, no dying, no Revenant, Sexton not a villain, no "Hold the
Door" / "Grave Gold" / defend-the-farm titles).

**The user's calls applied (2026-10-31 night):** Main 2 at Sexton's ferry in the fog under Gravewind Point; the
Gravemother stays optional (Side 3), Main 5 ends on the intake; no Doorward (the four Sentinels stay as placed); the
Rest's last boss is **Abel, corrupted** (the furnace twisted his soul; you fight to free him, then return him to Saint
Ada); **Lucky Ned is not fought on the Rest** (he's fought midway through the story, area 4 or 5; the Rest's barge is
crewed by the Company; Ned is named, heard and seen leaving); leaving by **train** (Tilly's hearse car); the barge falls
into **Gravewind Canyon**; **Sexton stays at his ferry and sells secrets for silver**.

## 1. The spine: why each mission leads to the next

Landfall ends on Delia's porch: Abel's soul burns in the barge as its pilot light; "bring him back to Saint Ada". The
barge hangs 45 m over the Sink, out of reach. Getting him back takes four things, and each main earns one:

1. **How to get at the barge** (Main 2). Sexton the Ferryman sells it: *a barge must eat; starve her and her crew bring
   her down to where the food is.* And: *a keeper's soul goes in his own lantern, or it goes out like a match.* His price:
   the barge's manifest, paid on delivery (taken in Main 6, handed over at his ferry after).
2. **Abel's own lantern** (Mains 3 and 5). Its bail is at Tilly's (snapped off in his fist); the lantern is in the Sink,
   where Lucky Ned threw it on Wednesday (a gambler never breaks a keeper's lantern: the worst luck there is).
3. **The clock** (Main 4). Saint Ada is guttering without her keeper ("a coal under ash"). When she goes out, every grave
   on the Rest is open to the hose. Brother Finch comes ashore to keep her alive until Abel is back.
4. **Starving the barge** (Main 5, Dead Lift, Back Pay). Main 5 chokes the Sink intake; the barge drops a second hose down
   the Providence's air shaft, where the Company's men have Cyrus Teague's dead shift digging the eleven out of the fall.
   Back Pay cuts it. Starved, the barge comes down over the chapel at dusk to hose Saint Ada herself (Main 6).

**Main 6** boards her there, kills the furnace and breaks Abel's cage. What comes out is not Abel: a week in the furnace
has twisted him into smog with a keeper's memory. He tears out of the barge, and with its pilot light gone the barge
falls into Gravewind Canyon. He flies to the burial boards on Gravewind Point, where six of the Rest's dead lie unwalked,
and starts to drink them. **Main 7** fights him there to burn the smog off and call him back into his own lantern, then
carries him home to Saint Ada: she wakes, the smog lifts, the Gravewind takes the dead west, and Tilly's hearse car is
coupled for the next region.

**Lucky Ned on the Rest:** named by Vane in Landfall; the man who put the hose to Abel (Delia, Aldana, Calder's Caches, The
Old Fare, The Last Watch say so); **seen leaving** in Main 3 (the gang's dark skiff lifts off the barge and flies south over
the Hogback, Ned and Vane on the radio); heard once more in Main 6 (a radio call to the barge). The player leaves the Rest
wanting him; he's met in area 4 or 5.

**The world changes as the player wins** (cheap: lighting states and story props):
- Smog: thick through Main 5 (`Smog`, WORLD's state); **`SmogThin`** (new state, fog 0.007, exposure 0.3) from Main 5's
  turn-in (one hose choked, the barge's plumes cut to one stack); after Main 6 the barge's wreck burns in the canyon below
  the Rim (a smoke column seen from the Point and the boards) and the smog stays thin with no source; clean dusk gold from
  Main 7's relighting scene; Day afterwards.
- The Lamplighter on the Mooring Ledge: its bag under tarps after Landfall, a sewn panel from Main 4, full and lifting
  against its anchor by Back Pay (three story props on the bag, one shown at a time). The crew's topics follow it.
- Abel's grave: frost and black flowers until Main 7; thawed, the flowers alive after.
- Company loudspeakers (`TRIPO_MODELS.md` 18) at the Keeper's Gate, the town gate, Main Street, the chapel, the Sink, the
  Diggings and the barge: Vane talks from each in turn. **New small actor `ACompanyLoudspeaker`**: its horns can be shot
  (sparks, a crackle, Vane's line cut mid-word), the optional objective in several missions.

## 2. Lore added (to `Docs/Story.md` once approved)

- **Warden's eyes.** A sworn Warden has had the Great Lantern's light put in their eyes: they see and hear the dead and the
  soul-light ranks (Restless blue, Gravebound purple...). The living of the Rest barely see the dead. Replaces "only Ellis
  sees them".
- **The Unpaid, now.** Two kinds of the same cold dead: what the hose tears half out of a grave and drops (a hose spills
  what it can't hold), and the Rest's dead on the Sundown Road who lost Saint Ada's dimming light and drifted home. Same
  rig, same coal heart.
- **A saint and her keeper.** A saint's light holds her town's dead in their graves against any hose; her keeper lights his
  lantern from her each night and sits up with her dead; a saint with no keeper gutters. Abel held the chapel door so the
  hose couldn't reach Saint Ada, and the gang took him instead. The Reliquary is **whole**, not smashed (was: smashed by the
  Deacon); dim since Wednesday.
- **A keeper's soul** burns longest (contract) and can only be carried in his own lantern; lit by his soul, the lantern
  "burns like a sunrise". Back in the Reliquary the soul stays: Abel keeps Saint Ada in death (a keeper never crosses while
  his saint burns).
- **What the furnace does to a soul.** A soul burned long enough in soul smog takes on the hose's hunger: it drinks other
  souls, and keeps only its strongest habit. Abel's is the keeper's: at dusk he goes to the boards to his dead, and now he
  drinks them. Smog can be burned off a soul with enough hurt to it (gunfire, salt, light), and what's left can be called
  home by something it loved. Freed, the soul is itself again, scorched but whole.
- **Lucky Ned's superstition:** he never breaks a keeper's lantern.
- **The Rest's barge** is a Company boat: Dredgers under a chief stoker; the Dunne Gang only do the stealing (Ned took Abel
  and left). Vane speaks through its horns as he does through every loudspeaker.
- **Hob** flew for the Ferryman eighty years before he signed on with the Wardens (keeps his built feather mutters,
  "Eighty years I've been collecting for him").
- **Sexton's trade:** he hears every soul that passes the fog and every one that's burned instead; he knows where things
  lie, and sells it, never for free.
- **Soul coal on the Rest:** the barge presses the dead into coal; tenders carry the sacks to the Company's hopper cars on
  the depot's siding (the passenger train and Tilly's hearse car stand at the platform as today).
- **Why the train out:** Vane's fleet watches the sky for the Lamplighter; nobody searches a hearse car. The Lamplighter
  flies ahead dark and meets Sully at the next region (that region's design).
- Rename the chapel yard's Gravebound leader "the Sexton's Debt" (`PacingPaths.h` comment, its group's DisplayName) to
  **"the Leftover"** (Sexton isn't a villain now).

## 3. The flow (new titles; ids, givers and XP unchanged unless marked)

| # | Id | Title (was) | Giver, where | After | Turned in to | XP | Rewards | Min |
|---|---|---|---|---|---|---:|---|---:|
| M2 | Main2 | **The Ferryman's Price** (Shall We Talk Business?) | Hob, Delia's porch rail | M1 | Mister Sexton, his ferry under Gravewind Point | 550 | the Ledger | 12 |
| M3 | Main3 | **Under New Management** (Cold Welcome) | Delia, her porch (meet first) | M2 | Tilly, her window | 1,000 | Uncommon+, 40, Abel's lantern bail | 12 |
| M4 | Main4 | **A Coal Under Ash** (Hallowed Ground) | Tilly | M3 | Father Aldana, the vestry door | 1,300 | Saint Maud Uncommon (Hallow), 40 | 13 |
| M5 | Main5 | **Choke Point** (The Keeper's Lantern) | Aldana (one conversation) | M4 | **Brother Finch, at the Reliquary** | 3,000 | 50 | 13 |
| N1 | (kept) | **Dead Lift** (kept) | Hiram Coyle, the winch house (met) | M5 | Hiram, the lift head's tube | 5,000 | Rare rifle, 40, the lift | 15 |
| N3 | (kept) | **Back Pay** (kept) | Hiram, up the tube (met) | N1 | the tag board: pay or keep | 6,500 | Rare / 400 silver | 22 |
| M6 | Main6 | **The Pilot Light** (The Gravewind) | **Captain Holloway, the Lamplighter's wheel** (meet first) | N3 | **Holloway, at the wheel** | 5,000 | 50 | 18 |
| M7 | Main7 | **Given to the Wind** (The Lantern Leans) | Holloway (one conversation) | M6 | **Delia, at the chapel door** | 4,500 | Abel's Boss table, Heirloom by hand, 60, the hearse car | 20 |
| P | new | **Paid on Delivery** (new, short) | none: carrying the manifest | M6 | Sexton, his ferry | 300 | Rare gun; his secrets at a discount | 3 |

Titles kept for Dead Lift and Back Pay: neither names the old premise, and "Back Pay" fits a story about a Company even
better. Sides: §13. Sexton's secrets: §12.

---

## 4. Main 2: The Ferryman's Price (Hob; about 12 minutes)

**Why:** Landfall says what was done; Main 2 buys the plan. Teaches the keeper's path (walked again in Main 7), ranks
(Hob's blue line), the Ledger, what the Unpaid are, and the boards where the Rest's last fight will be.

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
   *"That's what the hose leaves, sunshine. Half a soul and a temper."* **Set level 2** (an encounter level override: the
   deck's zone is 5-6). The six dead stay on the boards: Main 7's stakes.
2. **Talk to Mister Sexton.** [Talk `Speaker_Sexton`] His black ferry hangs on rotted chains in the canyon fog under the
   deck's west end; he sits in its stern (his seated model, reused). A rope ladder from the deck down 6 m to a timber
   landing. The deal:
   - Sexton: *"Warden. You've come about the barge. Everyone does, eventually."*
   - Sexton: *"She's out of reach. But a barge must eat, and Company men are greedy."*
   - Sexton: *"Starve her, and they'll bring her down to where the food is."*
   - Sexton: *"Mind: a keeper's soul goes in his own lantern, or it goes out like a match."*
3. **Open the Ledger: {Inventory}, then 2.** [OpenPage Bestiary] Must stay step 3 (`Ledger::HandedOverStep`). Sexton, as
   he hands it up: *"Write down every soul you meet. I bill by the head."*

**Vane** (the Keeper's Gate loudspeaker, as the valve shuts): *"Wardens. That cart is Company property."* / *"I'm adding
it to your bill."* Optional: silence the Keeper's Gate loudspeaker. [Hit `Loudspeaker_KeepersGate`] (cuts him at "your
bi—").

**Turned in to Sexton** (card, then):
- Sexton: *"My price: the barge's manifest. Every soul Vane burned, with my fare beside it."*
- Sully: *"And if I don't bring it?"*
- Sexton: *"Then you'll owe me, friend. Ask the crow how that goes."*
- Sexton: *"Come back when you've silver. I know where things lie on the Rest."* (opens his secrets, §12)

Then Delia's ? for Main 3 (her word, meet first, as today).

**Reuses:** the keeper's path, cairns, Keeper's Bridge, the Three Widows, Gravewind Point's deck (8 boards, 3 lantern
posts, Keeper's Gate), Sexton's seated model, `Speaker_Sexton`, `BluffNest` (moved), DREDGER classes, the Unpaid, the
Ledger step.
**New in the world:** the Ferryman's landing (a 4 x 6 m timber stage hung under the deck's west end, a rope ladder; a small
cliff notch cut for it; an always-on fog card below it); Sexton's ferry on chains; 6 shrouded dead on the boards (shown
until Main 7's scene); the hose-cart; a loudspeaker post at the Keeper's Gate. Retired: Sexton's railing seat on the Point,
the `LookoutStains` optional. Tripo: §13 items 21-23, 18.

## 5. Main 3: Under New Management (Delia; about 12 minutes)

**Why:** Sexton said "his own lantern"; it wasn't on Abel. His body went to town, and the Company holds the town. The man
who took him is seen getting away.

**Offer**, Delia on her porch (meet first):
- Delia: *"Tilly Bright laid my boy out. What the hose left of him."*
- Delia: *"If his lantern came home, she has it. Town's Company ground now. Mind yourself."*

**Card:** Abel's soul has to ride home in his own keeper's lantern, and nobody has seen it since Wednesday. Tilly Bright, the
undertaker, laid out his body. Vane's Company has taken the town, and its men hold the gate.

**Steps:**
0. **Take the farm road into town.** [Reach `Place_TownGate`] Company notices on every fence post (Side 1's posters).
   **Ned leaves** (a fly-by as the step starts, ~20 s, seen from the whole farm road): the Dunne Gang's dark skiff (the
   built gang skiff) casts off from the barge and flies south over the Hogback, over the radio:
   - Vane: *"Off you go, Ned. The Deacon wants you down south."*
   - Ned: *"Keep my keeper burning, Cornelius. Keepers are lucky."*
   - Hob: *"That's Lucky Ned, sunshine. He's the one put the hose to Abel."*
   - Sully: *"He's getting away."* Hob: *"For now. Lantern never forgets a smell."*
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
safe-zone rule, shutters, `Speaker_Tilly`, `Item_LanternBail`, the reward gun, Main 3's id ties (safe ground, Pruitt's
hatch), the gang skiff (SM_Skiff_A_Gang, dark paint) and the cold open's skiff-flight code for the fly-by.
**New in the world:** the checkpoint (sandbags, barrier, sign), the banner and its two ropes (shootable), ASSESSED chalk
decals on doors, loudspeaker posts at the gate and mid-street, a hose-cart at the Gilt Spur, the skiff's fly-by path (barge
-> south over the Hogback, out of the map). Dredgers level 2-3. Tripo: 23-25, 18.

## 6. Main 4: A Coal Under Ash (Tilly; about 13 minutes)

**Why:** a keeper's lantern is lit from his saint; is she still alight? Sets the clock. Ringing her bell tells the barge
somebody's home, and it answers.

**Offer**, Tilly:
- Tilly: *"Father Aldana blessed Abel's lantern every spring. If Saint Ada's still lit, he'd know."*
- Tilly: *"He's barred in the vestry since Wednesday. The Company's at the chapel. Knock loud."*

**Card:** A keeper's lantern is lit from his saint, and nobody has seen Saint Ada burn since Abel was taken. Father Aldana
has barred himself in the chapel's vestry. A Company crew is in the chapel yard.

**Steps:**
0. **Go up the north road to the Chapel of Saint Ada.** [Reach `Place_Chapel`]
1. **Clear the chapel yard and pull the hose out of the chapel.** [Clear `ChapelYard` 16 + Interact, held 2 s,
   `Hose_ChapelWindow`] The door Abel held won't open, so the Company ran a hose through the broken side window to drink
   the saint. First push 8 Unpaid (the dead the crew spilled), second 2 hose-men working the hose, 5 Unpaid and the
   Restless "Leftover". Pulled, the hose whips out of the window and coughs smog.
2. **Knock at the vestry door.** [Talk `Speaker_Aldana`]
   - Aldana: *"Stay back from that door!"*
   - Sully: *"Warden. Tilly sent me."*
   - Aldana: *"Then ring her bell. If she can still answer, she will."*
3. **Ring the chapel bell: hold {Interact} on the rope.** [Interact, hold, `Bell_Chapel`]
4. **Look at the Reliquary** [Event `GraveSight.Reliquary`; its look_when stays "during Main4 from step 4"] **and hold the
   chapel against the barge's answer.** [Clear `BellAnswered` 6] Saint Ada answers: her ember, a coal under ash, flares
   once through the grey (the Grave Sight flash). So does the barge: a Company tender swings off it, hangs over the knoll
   and drops 4 riflemen and 2 hose-men by rope into the yard (the Rappel entrance, §14), level 3-4. Vane: *"Ringing for
   help? Charming."* / *"Somebody's home, then. Tender, go and see who."* Optional (new text): **read the keepers' roll.**
   [Interact `KeepersRoll`] Every Ransom keeper since the saint, ABEL RANSOM the last. Hob: *"Ninety years of Ransoms kept
   her lit. Not one of them got a hose."*

**Turned in to Aldana** (card, then):
- Aldana: *"She's a coal under ash, Warden. Without her keeper she's going out."*
- Aldana: *"When she does, every grave on the Rest is open to that hose."*
- Aldana: *"I watched from the vestry. Ned Purcell wouldn't break Abel's lantern. Bad luck, he said."*
- Aldana: *"He threw it down the Sink. Spiders hoard what a saint has touched."*

Then Main 5 offered (one conversation). **From this turn-in Brother Finch stands at the Reliquary** (CREW's
`AWardenCrewMember` Keeper, a second placement, story-gated): he keeps the saint alive by hand until Abel is back. His topic:
*"I'll sit with her. Somebody has to keep her lit."*

**Reuses:** the north road, chapel, `ChapelYard`, `Speaker_Aldana`, `Bell_Chapel`, `AChapelReliquary`,
`GraveSight.Reliquary`, `KeepersRoll`, `ACompanyTender` (Landfall's, hovering), Saint Maud Uncommon reward, Finch's model.
The four Stone Sentinels stay as placed (Boot Hill, churchyard west; after Main 4).
**New in the world:** the side window broken with the hose rig through it; hose-drag scars and a soot handprint on the door
Abel held (decals); the Reliquary **whole** with a guttering ember state (the old smashed look retired); a loudspeaker post
by the lychgate; rope drop points round the yard. Tripo: 28, 13, 18.

## 7. Main 5: Choke Point (Aldana; about 13 minutes)

**Why:** the lantern is in the Sink, and so is the barge's mouth: one trip, two jobs (the lantern; starve her). The
Gravemother stays optional (Side 3).

**Offer**, Aldana (one conversation after Main 4):
- Aldana: *"Bring up Abel's lantern. And that hose drinks from the Sink's floor."*
- Junie (the tube): *"Choke it? Salt round the intake collar and blow it. Hob's bringing you charges."*
- Hob: *"I'm a lookout, not a mule. ...Fine."*

**Card:** Ned threw Abel's lantern into the Sink, where the spiders hoard what a saint has touched. The barge's hose drinks
from the Sink's floor too. Junie says three salt charges on its intake collar will choke it: starve the barge, Sexton said,
and her crew will have to bring her down.

**Steps:**
0. **Take Junie's salt charges from Hob, and climb down into the Sink.** [Collect `Item_SaltCharges` at the ramp head +
   Reach `Place_SinkFloor`, height counted] Hob drops a canvas satchel on the rim.
1. **Shoot down the three egg sacs.** [Event `EggSac.Burst` x3] The hoard is behind them (the floor's spiders as built).
2. **Take Abel's lantern from the webbing.** [Interact `Lantern_Keeper`] Must stay step 2 (`AKeepersLantern::TakeStep`).
   Hob (kept): *"Dark as a shut eye, but whole. Missing its bail. You've got that, sunshine."* The bail goes on here
   (`Item_LanternBail`'s mission moves from Main6 to Main5).
3. **Choke the hose: set three salt charges on the intake.** [Interact, held 1.5 s, `Intake_Charge` 3] **Set piece "the
   Intake"**, the mission's climax: within 10 m the intake's draw pulls Sully toward its mouth (the hose-man's pull, scaled
   up; the cyan-black edge; a drain only inside 3 m), so every charge is set leaning against it. Dredgers **rappel down the
   hose** from the barge in two drops (2 riflemen + a hose-man, then 2 riflemen + a hose-man, level 4-5), the second as the
   second charge goes on. The spiders, stirred, come down the webbed ramp behind them (3). The third charge set, a 5 s fuse
   (get clear: the draw doubles as it burns): they blow (event `Intake.Choked`): the hose bucks like a snake, coughs a
   black cloud, swings across the floor (the swing knocks down anything in its arc, enemies too) and hangs limp; the barge
   lurches overhead and two of its three plumes die.
4. **Climb out of the Sink.** [Reach `Place_SinkRim`] Optional (kept): **look at the hat by the den.** [Interact
   `SheriffHat`] W. ODELL. Hob: *"Whatever lives in that hole had the sheriff for supper."* As the smog pool by the den
   drains, something huge growls in the dark (the Gravemother, seen and not fought: Side 3's hook). Hob: *"Whatever that
   was, it was drinking that runoff. Now it's hungry."* At the rim the barge swings north and drops a **second hose onto
   Larkspur Ridge**. Hob: *"She's not starving yet, sunshine. She's found the mine."*

**The barge's horn** (Vane, as the first charge goes on): *"Warden, that's my supper you're fiddling with."* (choked):
*"...Coughing. My barge is coughing."* / *"Stoker! Find me another mouth."*

**Turned in to Brother Finch, at the Reliquary** (card, then):
- Finch: *"Abel's lantern. Whole. Ned's luck is ours, Warden."*
- Finch: *"It'll stay cold till his soul's in it. Then it'll burn like a sunrise."*
- Junie (the tube): *"That second hose is down the old mine's air shaft. Up the ridge."*
- Hob: *"Old ore lift at the quarry. Somebody dead's sat by it four years."*

`SmogThin` from this turn-in.

**Reuses:** the Sink, ramp, egg sacs, the floor's spiders, `AKeepersLantern`, `SheriffHat`, the Gravemother's den (seen),
WORLD's intake and hose, Hob.
**New in the world:** the satchel; three clamp spots on the intake collar; rappel ropes down the hose; the hose's swing and
its limp shape (a second baked shape after `Intake.Choked`); the smog pool at the den mouth (a 0.5 m dip in the Sink floor's
terrain, a black glinting water plane, drained after); the second hose from the barge north to a new air-shaft head on
Larkspur High (shown from step 4 until Back Pay is done); the Sink's loudspeaker on the warning fence. Tripo: 26, 35, 13.

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
5. **Put the Foreman down.** [KillNamed `Legendary_Foreman`] His fight as built, plus **the hose's draw** at the fall (Main
   5's pull) during "The shift", dragging toward the rubble while his miners rise. Lines kept, but in the dark: *"I can dig
   them out. The Company promised. I can still dig them out."*; his last word (kept): *"...Tell the Company it was the
   roof."*
6. **Cut the hose and take the payroll.** [Interact, held 2 s, `ShaftHose` + Interact `PayrollChest` -> Collect `Payroll`]
   Cut, the hose whips up the shaft and away. The barge's horn, far off: *"Stoker. Stoker! That was my last mouth!"* The
   cave-in set piece on the way out (kept).

**Turned in at the tag board**, the choice (kept):
- **Pay the eleven.** The tags go to OUT. Shift boss (kept): *"Paid in full. Shift's over, boys."* New: eleven lights rise
  out of the mine mouth and drift west, past the barge's reach. Hob (kept): *"First honest pay this mine ever made."*
  Vane (the Diggings' loudspeaker): *"You PAID them? With Company silver?"*
- **Keep the payroll.** About 400 silver; the tags stay on IN, the dead keep working. Shift boss (kept): *"...Company man
  after all."* Vane: *"Now that's a Company man. I'd hire you, Warden."* Hob (new): *"Can't spend it in the ground,
  sunshine. Neither could they."*

After either: Hob: *"That's both her mouths shut. She's starving now, sunshine. Back to the ship."* (Holloway's ? for
Main 6.)

**New in the world:** the Company camp at the Diggings; the air-shaft head on Larkspur High above the stope and a shaft
opening in the stope's roof with the hose hanging over the fall; the contract page; the cut hose's collar. Tripo: 35, 36.

## 10. Main 6: The Pilot Light (Holloway; about 18 minutes)

**Why:** starved, the barge is coming down over the chapel at dusk to hose Saint Ada herself. The one time she's in reach:
board her, kill her furnace, take Abel out. Accepting fades the Rest to dusk (today's `AStoryLighting` Dusk rule, kept for
Main 6 and Main 7).

**Offer**, Captain Holloway at the Lamplighter's wheel on the Mooring Ledge (meet first):
- Holloway: *"She's starving. She's coming down over the chapel to drink Saint Ada dry."*
- Junie: *"Bag's patched. Their grapples were still in my hull, so I re-rigged them. Waste not."*
- Holloway: *"We go now, Warden. Bring Abel's lantern."*

**Card:** Starved of both her hoses, Vane's barge is coming down over the chapel at dusk to hose Saint Ada herself. The
Lamplighter is patched, and Junie has re-rigged the Company's own grapples. Board the barge, kill her furnace, and take Abel
out of it in his own lantern.

**Steps:**
0. **Cast off: hold {Interact} on the anchor winch.** [Interact, held, `Landfall_AnchorWinch`] Landfall's winch, reversed.
1. **Fight off the Company's tenders.** [Event `Pilot.TendersDown`] The Lamplighter flies a new course from the ledge east
   over the farm and the town (about 90 s, FLIGHT's moving deck). Two Company tenders come at her: riflemen fire from their
   decks; **a tender's gas-bag is its weak spot** (shot enough it tears, and the tender falls burning into the fields: a big
   fall, a crash, a smoke column). One grapples on and boards 2 riflemen + a hose-man (Landfall's boarding, reused).
   Junie: *"Shoot their bags! They burn lovely!"*
2. **Grapple the barge.** [Interact, held 1 s, `Lamplighter_GrappleLauncher` 2] The two launchers on the starboard rail
   fire; the chains pull the ships together 25 m over the chapel knoll, the barge's hose already snaking down toward the
   chapel roof; a gangplank drops (scene, 3 s). Finch on the tube, from the Reliquary: *"Her light's flickering! Hurry!"*
3. **Fight across the barge's top deck.** [Clear `BargeDeck` 9 + Reach `Barge_FurnaceHatch`] About 70 m of riveted deck:
   three stacks, a crane, the hose drum gantry, VANE CONSOLIDATED painted under foot. 6 riflemen in cover, 3 hose-men, level
   6. Hazard: furnace vents along the deck flare every 6 s (a 1 s glow tell, then a burn column). Optional: **jam the hose
   drum** [Interact, held 2 s, `Barge_HoseDrum`]: the hose stops short of the chapel roof (Finch: *"It's stopped! Bless
   you, Warden."*; the rest of the mission plays without his flicker lines, and Delia's turn-in gains a line). Optional:
   **silence the barge's horns.** [Hit `Barge_Horn` 4]
4. **Take the barge's manifest from the Company office.** [Interact `Barge_Manifest`] Sexton's price, in the stoker's
   office off the furnace hall's catwalk: a thick VC ledger, 412 names (the Rest's and four other towns'), signed for at
   the top *Rec'd from Dunne & Co.: 1 keeper (premium). N. Purcell.* Hob: *"Vane's fare bill. The Ferryman'll frame it."*
   On the office's radio, Ned, far off: *"Barge, this is Purcell. Is my keeper still burning? I've money on him."*
5. **Kill the furnace: shut the three draft valves.** [Interact, held 1.5 s, `Furnace_DraftValve` 3 + Clear `FurnaceHall`
   8] **The set piece.** The furnace hall: a round riveted room 26 m across and 9 m high; the soul furnace in the middle
   (6 m, a glass-windowed cage at its heart where Abel's light beats like a heart, flaring black at the edges); a floor
   ring of 12 grate segments round it that heat like a roulette wheel (4 of 12 glow, then burn, the pattern turning every
   3 s); a catwalk ring at 4 m; two stokehold doors. The **Chief Stoker** (a Hallowed hose-man, existing rank: gold tag,
   x6 health, set level 7, two tanks, both crit spots) works the furnace with 4 riflemen and 3 stokers (hose-men) coming
   through the stokeholds in two pushes. Each valve shut cools the ring and dims the furnace one notch; the Stoker's hose
   latches harder as it cools (*"Keep her lit! Mister Vane's watching!"*). Vane on the hall's speaker: *"Warden, those
   valves are worth more than you are."*
6. **Open the soul cage.** [Interact, held 3 s, `Furnace_SoulCage`, with Abel's lantern carried] **Twist:** what comes out
   isn't Abel. The cage bursts; a black shape in a keeper's coat, smog pouring off it, a coal heart behind black ribs, a
   black lantern in one fist, throws Sully across the hall, tears out through the furnace's back wall and streaks west
   into the dusk. With its pilot light gone the barge dies: the furnace gutters, every light goes out, the deck tilts.
   Vane: *"Oh, that's not good."* / *"Gentlemen, abandon my barge. Leave the Warden."*
7. **Get off the barge!** [Reach `Lamplighter_Deck`] The escape: the barge lists 12° and drifts west on the dusk wind; its
   back half burning, a stack topples across the route (scripted); fleeing Dredgers jump for a tender; Holloway cuts the
   grapples and brings the Lamplighter under the broken gangway: *"Jump, Warden!"* (Hob's fall catch as everywhere.) Scene
   **`BargeFall`** (~12 s, skippable) from her deck: the barge drifts west over the Rest raining soul coal, crosses the
   Rim and goes down into Gravewind Canyon: a fireball, a smoke column from below the Rim. Ahead of it the black streak
   that was Abel comes down on the burial boards of Gravewind Point.

**Turned in to Holloway at the wheel** (card, then; the ship sets back down on the ledge as she talks):
- Holloway: *"Eleven years since Vane's first barge took my town. That one's for them."*
- Finch (the tube): *"That wasn't Abel, Warden. That was a week of furnace wearing him."*
- Finch: *"He's gone to the boards. A keeper goes where his dead are."*
- Hob: *"Six of them laid out there, sunshine. And he's thirsty."*

Then Main 7 (one conversation). The barge's manifest is carried from step 4 (`Item_BargeManifest`), for Paid on Delivery.

**Reuses:** `ALamplighterShip` and its moving deck, `ACompanyTender`, the grapples (reversed), the winch, DREDGER classes
and the rank system (the Chief Stoker), the Dusk rule, the barge stand-in (outside), the relight-under-pressure pattern
(the valves), Hob's fall catch.
**New in the world (the big piece):** the barge's **boarding route** as a walkable kit (top deck, stacks, crane, hose drum,
vents, horns; the furnace hall with its furnace, cage, grate ring, catwalk, valves, stokeholds; the stoker's office), built
like the Providence's interior (its own mesh kit, shown only during Main 6, culled; Main measures perf); the barge's
**second placement**, lowered to about 25 m over the chapel knoll, its hose reaching for the chapel roof; the Lamplighter's
second course (ledge -> chapel) and its two grapple launchers; tender gas-bags as a weak spot (code); the `BargeFall` scene;
the **wreck** in Gravewind Canyon below the Rim, south of the boards (backdrop only, a smoke column seen from the Point and
the boards) after Main 6; **soul coal strewn** across the valley and the canyon rim after Main 6 (instanced glowing lumps,
12 of them interactable for Main 7). Tripo: 27, 29-34, 37.

**Code ties retired here:** Main 6's Abel steps move to Main 7 (§11; the same numbers), `SitWithPa`, Delia's `Delia.Main6`
topic.

## 11. Main 7: Given to the Wind (Holloway; boss Abel, corrupted; about 20 minutes)

**Why:** the furnace twisted Abel, and he's drinking the dead he spent thirty years walking. Burn the smog off him, call him
back into his own lantern, and take him home before Saint Ada goes out. Dusk all through; clean gold after the relighting.

**Offer**, Holloway (one conversation after Main 6):
- Finch (the tube): *"Burn the smog off him. Then hang his lantern where he can see it. He'll come."*
- Holloway: *"Go, Warden. Before he drinks those six."*

**Card:** The furnace twisted Abel's soul, and when the cage broke he fled to the burial boards on Gravewind Point, where six
of the Rest's dead lie unwalked. Burn the smog off him and call him back into his own lantern. Then take him home to Saint
Ada before she goes out.

**Steps** (Abel's step constants keep their numbers, now read on Main7: HangStep 1, FightStep 2, SceneStep 3):
0. **Go to the burial boards on Gravewind Point.** [Reach `Place_GravewindPoint`] By the west road or the keeper's path;
   the barge's soul coal lies smoking along the way. Optional: **free the souls in the fallen coal.** [LastingInteract
   `SoulCoal` 12, held 1 s] Each lump cracks and a soul rises and follows the lantern (a growing trail of wisps behind
   Sully; at the boards they wait by the Keeper's Gate). Optional (kept): **by the keeper's path.** [Reach
   `Place_KeepersBridge`] Hob: *"Every Ransom walked the dead this way at sundown. Tonight he's waiting at the end."* The
   Bone Hounds (placeholder pack, 4 and a Restless; "they raid boards no keeper sits up with") are on the keeper's road
   for this (moved from "after Main 4, before Main 6").
1. **Hang Abel's lantern on the keeper's post.** [Interact `LanternPost_Keeper`] (the built post and HangStep) His own
   lantern on its post calls him: the fog wall seals the Keeper's Gate (built), and he comes up out of the canyon fog.
2. **Free Abel.** [KillNamed `Boss_Abel` (counts freed)] + Optional: **save all six of the dead on the boards.** The fight
   below.
3. **Abel comes home to his lantern.** [Scene `FreeAbel`] (SceneStep, the old SitWithPa slot) Below.
4. **Carry Abel home to Saint Ada.** [Reach `Place_Chapel`, the lantern carried, lit] By the west road (the deck to the
   chapel): the Company men who bailed out of the barge (3 riflemen, 2 hose-men, level 6) are on it, and **their hose-men
   latch the lantern, not Sully**: a line on it drains Abel's light (the lantern dims on the HUD's carried item; break the
   line; nothing fails, the light refills once free). Camp SheepFold's dead (built, on the west road) are drawn to his
   light. The freed coal souls follow behind.
5. **Put Abel's lantern on the Reliquary.** [Interact, held 3 s, `Reliquary_Lantern`] Scene **`SaintAdaWakes`** (~25 s,
   skippable): Finch steps back; Abel's light pours out of the lantern into the Reliquary; the coal under ash catches;
   Saint Ada blazes white-gold; a ring of light washes out over the valley and the smog burns off in a wave (thin smog ->
   clean dusk); across the Rest the Unpaid go quiet and sink into their graves; and from the knoll, far off on Gravewind
   Point, the Gravewind lifts the six dead off the boards and the coal's souls with them, a river of lights going west
   into the sunset past the wreck's smoke. Abel stands by the Reliquary, himself, a soul in his keeper's coat; Delia has
   come up the north road:
   - Abel: *"Ma. You came up the hill. Your knees."*
   - Delia: *"Hush. You held the door."*
   - Abel: *"I'll keep her now. From in here. She won't go out again."*
   - Abel: *"Warden. Whatever they took me for, there's more of it. Go and get them back."*

**The boss: Abel Ransom, Corrupted.** The built `Boss_Abel` (Abel's class, phases and arena) reworked: the Unpaid rig at
1.3x in his keeper's coat, now smog-black, soul smog streaming off him like smoke off a wet fire; his coal heart
furnace-orange behind a cage of black ribs; in one hand a **black furnace lantern** (the built ghost lantern, smog-black
glass with a cyan glint), in the other his spectral **Ranchhand** (Heirloom's silhouette) firing smog buckshot. **Set
level 7, Boss rank**, about 15,150 health (today's numbers, `Looter.Abel.HealthScale`, hits x1.25; tune to 5-6 minutes).
His bar reads **"Smog"** and drains as he's hurt; under it three gold pips, **"Abel"**, fill as he's called back. Arena:
the burial boards deck at dusk (built: 8 boards on posts for cover, 3 keeper's lantern posts, the Keeper's Gate sealed by
the fog wall, the sun setting behind him), the six shrouded dead on their boards, the wreck burning in the canyon below.
- **Phase 1, "Stoker" (Smog 100-66%).** The furnace in him. Smog buckshot: slow visible pellets in a cone after a 1 s
  flare of the black lantern (built); the stock lunge up close (built); new, **the smog tendril**: from the black lantern
  a line latches Sully like a hose-man's (a drain and a slow pull; it breaks at 14 m, on a staggering hit, or by shooting
  the black lantern during its 1 s flare). Every 25 s two Unpaid rise through the boards (built, at most 4). Every 12 s
  he turns toward the six dead and **tries to walk them** (the keeper in him; the built sunset turn): for 3 s his coal is
  open, and he mutters fragments (*"Dusk. It's dusk. Who's sitting with them?"*).
- **The call back (each third).** At 66% and 33% the Smog bar **locks** (shots only spark, as the Gravemother's stone gate
  shows it) and he staggers to his knees for 6 s: **hold {Interact} on the keeper's post, 1.5 s** ("Call him back"). His
  lantern brightens a third; a pip fills; a little of the smog boils off him for good (his hat and coat get their colour
  back, a third at a time); he says something of himself: *"...Ma? Is that my lantern?"*, then *"Who are you? You're not
  from here."* Missed, he rises, the bar stays locked, and the next stagger comes after 12 s, so the player can't skip it
  and can't fail it.
- **Phase 2, "The Hose" (66-33%).** He drifts out over the canyon into the fog, untargetable (built), and **drinks**: a smog
  line runs from him to one of the six shrouded dead at a time (12 s each). Shoot the line where it meets the body (its
  collar of smog, 3 hits) to break it. A body drunk out heals him 5% and rises as a Restless Unpaid (and the optional
  fails). Meanwhile the three keeper's lanterns on the deck go dark (built): relighting all three (hold 1.5 s each, under
  pressure, built) drags him back in, as today. Two Unpaid rise every 25 s (built, at most 4).
- **Phase 3, "Keeper" (33-0%).** The smog from the wreck below pours up over the deck: the fog thickens to about 15 m of
  sight (a height-fog density change; the coal, his lantern and the keeper's post's lantern are what to aim by), and gusts
  push toward the deck's open end (built; a fall is caught by Hob, built). He fights faster: two tendrils, double buckshot.
  Between attacks the keeper in him walks to a lantern post to hang his black lantern there and **the furnace in him pulls
  him back** (the built "dark saint pulls him back", renamed): each pull stuns him 3 s, coal open.
- **Freeing him (at zero Smog).** He kneels; the last call: **hold {Interact} on the keeper's post, 3 s**, while the last of
  the smog fights back (a tendril on Sully, a drain to hold through). The black lantern cracks and falls into the canyon.
  Scene **`FreeAbel`** (step 3, ~15 s, skippable): the smog boils off him into the fog; what's left is gold-white and
  scorched but whole: Abel himself. He looks at Sully, then at the post: *"That's my lantern. You've been looking after
  it."* *"I can't get home on my own. Will you carry me?"* He steps into it, and it burns like a sunrise. Sully takes it
  down (lit) as the scene ends; the fog wall drops.
- **Loot:** Abel's Boss table once (3 guns at Luck 1.3, 30% legendary; built). His adds use the Basic table. Death and
  reset as built (the Keeper's Crossroads roost; Abel, adds, lanterns, the six dead and the fog wall reset).
- **Why it lands:** every hit is burning the furnace off a man, not killing him; the best damage comes when he tries to do
  his job; saving his dead is saving what he'd want; and the win is a man stepping into his own lantern.
- **Vigil Stone:** Decision 1.

**Vane** (the Keeper's Gate loudspeaker, as the fog wall drops): *"That was a very expensive barge, Warden."* / *"I have
forty more. Do come and see them."*

**Turned in to Delia, at the chapel door** (card, then; Heirloom by hand, `hand_items`):
- Delia: *"Forty years he kept her. Looks like he'll keep her forty more."*
- Delia: *"A keeper's buried with his lantern, not his iron. I kept this back. It's yours."*
- Delia: *"Hold the door, Warden. Wherever that train of yours goes."*
- Holloway (the tube): *"Vane's fleet watches the sky. Nobody searches a hearse car. We'll meet you there."*
  (with the hose drum jammed in Main 6, before it, Delia: *"Finch says you stopped that hose short of her roof."*)

After it: the **Gilded Lily** on the depot's station board and Tilly's hearse car waiting with its door open (built, now
from Main 7's turn-in; Tilly's Main 7 topic: *"Your car's coupled, Warden. Mind the brass."*). Abel stays at the Reliquary
as a friend (`Speaker_Abel` moves from his board to the chapel; lines after each later region); Finch back aboard the
Lamplighter (she lifts off dark at dawn, a story prop gone from the ledge on the next visit); Delia on her porch; Abel's
grave thawed; the six boards empty.

**Reuses:** the built Abel boss (class, phases 2 and 3's lantern relighting and wind, adds, fog wall, roost reset, Boss
table), the Keeper's Lantern (lit look), `LanternPost_Keeper`, the Reliquary (a lit state), the keeper's path, west road,
Keeper's Bridge, cairns, Gravewind Point and its boards, Camp SheepFold, the Bone Hound placeholders, Abel's model (smog
and soul looks), the old SitWithPa scene's pattern, Heirloom, the Dusk rule, the station board's Lily line and the hearse
car.
**New:** Abel's smog look (a material and a smog-plume card set on his rig; the built ghost lantern blackened), the smog
tendril, the call-back lock and pips, the drink lines to the boards' dead, Phase 3's fog surge, `FreeAbel` and
`SaintAdaWakes` scenes, the soul coal lumps (12 interactable) and their following wisps (instanced, capped), the light wave
(a lighting blend), Abel's soul by the Reliquary, Delia's story positions (the chapel), the hose-man's lantern-latch (a
target switch). Tripo: 20, 27, 28, 31.

**Code ties retired:** Main 7's step 0 `ADoorHandoff` (Heirloom by `hand_items` at the turn-in), Tilly as Main 7's turn-in
(the hearse car now opens on Main 7's turn-in).

## 12. Paid on Delivery, and Sexton's secrets (new)

**Paid on Delivery** (after Main 6; no offer card: it starts when the manifest is picked up in Main 6, tracked "Pay the
Ferryman"; about 3 minutes; 300 XP). 0. **Take the manifest to Sexton's ferry.** [Talk `Speaker_Sexton`] **Turned in to
Sexton** (card, then):
- Sexton: *"Four hundred and twelve. Vane owes me a fortune, friend."*
- Sexton: *"Paid in full. I keep my accounts, and you're in the black."*
- Sexton: *"A tip, for prompt payment: Purcell's gone south. The Deacon's calling the gang in."*
A Rare gun from the ferry's bilge; his secrets cost a quarter less from now on.

**Sexton's secrets** (from Main 2's turn-in; a vendor talk at his ferry, `AVendor`'s pattern with a LooterUI list): each of
the Rest's 15 finds (`story_texts.SECRETS` and the 2026-10-29 finds, `Docs/Pipeline/Levels.md`) can be bought as a map mark
(the minimap and map show its spot; the find's crows already tell it close up). Prices by the find's loot: footlockers and
tins 40 silver, strongboxes and named guns' finds 100, the Tollman's whereabouts 150 (at level 1, grown with the economy;
numbers to tune). One line each, in his voice (*"Under the middle Widow, friend. Mind your back."*). Found finds drop off
the list. Later regions: their finds, and story secrets (the next barge, the gang's ground).

## 13. The sides that told the old premise

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
Steps kept. **The strongbox:** Sully doesn't know a combination; Hob does it: *"Crows hear tumblers, sunshine. Left, right,
left."* Inside (in place of the Deacon's "Kid's share" slip): a Company contract, *Dunne & Co., soul recovery, Ransom's
Rest. One keeper (premium). Paid on delivery. C. Vane*, and the Deacon's note, *Ned: the keeper, not the saint. The saint
is Company business. A.D.* (seeds Main 6: the Company always meant to take Saint Ada). Hob: *"Premium. He'd have liked
that, your keeper."* Turn-in at the Rangers' board, Hob: *"Three tags for the Rangers. Ruth Calder'll want a word with
your captain."*

### The Old Fare (Mr. Sloane)
Steps kept. **Offer:** *"Who's there? ...A Warden? Lord. I heard your ship come down."* / *"Every night since Wednesday
they stand in my waiting room like they've a train to catch."* / *"You see the dead. Maybe they'll listen to you."*
**Why they wait:** for the Company's coal train on the siding (hopper cars stencilled VC), which would carry their kin.
**Twist (deeper):** the desk's marker payable to N. Purcell has a note pinned to it: *Keeper's hours. Debt cleared. N.P.*
Sloane sold Ned the hour Abel locked the chapel alone. Sully: *"Ned's paper."* Hob: *"He sold the keeper's hours,
sunshine. For a marker."* Fenn's turn-in (kept) lands harder: *"Tell Sloane I forgive him. Don't tell him I said it like
that."* After Main 7 Sloane's window line becomes *"Line's open to the Lily, Warden. Your car's coupled."*
**New:** 2 hopper cars on the depot siding (Tripo 38, optional), soul coal sacks on the platform.

### Knocking Below (Tilly)
Steps kept. **Offer:** *"You see the dead, Warden. So you'll understand me."* / *"I've four in my cellar, salted, hid
from the hose till they can go to the boards. Something knocks every night."* / *"I'd go down myself, except I'd rather
not."* **Card:** Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up, salted now so the hose
can't find them. Since Wednesday something down there knocks all night. **Thaddeus** (home from the Sundown Road when the
saint dimmed): *"A Warden. I measured a Warden once. Tall. Pine."* / *"I kept them in their boxes a week. They got past me
when the door opened. A cellar wants keeping."* / *"Give her my tape. Tell her the brass was too much. And that I liked
it."* (two kept) His hearse car is the one Sully rides out in (kept, with "Ellis" -> "the Warden").

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
- Side 3 (optional, the user's call): steps, levels and turn-in kept. New reason: Main 5's choked intake drained the smog
  pool she was drinking, and she hunts the floor hungry. Hob, new: *"She was big before the smog, sunshine. The smog made
  her bigger."*

(Lost and Found tells nothing of the old premise: unchanged.)

## 14. Models to add to `TRIPO_MODELS.md` (numbered after 19, Abel's hat)

Stand-ins until they land, one soft path each (the contract's rule). Sizes real-world; people A-pose, empty hands. Lucky
Ned stays in the "later rounds" list (fought in area 4 or 5); the Rest uses the built gang skiff for his fly-by.

People:
20. **Abel Ransom.** Sixties, tall, 1.85 m: a long keeper's coat, grey beard, a lantern hook on the belt, the wide-brim hat
    (19). Two looks by material: corrupted (smog-black, ribs over a furnace-orange coal) for the fight; his soul (gold-white)
    in the scenes and after. Rigged to the Unpaid rig's skeleton (his boss runs on it).

Props and places:
21. **Sexton's ferry.** A long black wooden ferry, 9 m: a high curved prow with an iron lantern pole, rotted planks, one
    long sweep oar, chain eyes at bow and stern (it hangs on chains).
22. **Shrouded dead.** A body wrapped in a pale sheet tied with rope, lying, 1.8 m; and a small one, 1.1 m.
23. **Company hose-cart.** A two-wheeled iron pump cart, 2.2 m: a riveted tank with gauges and a valve wheel, a hand-crank
    pump, a coiled black hose on a reel, a brass VC plate.
24. **Company checkpoint.** A striped barrier pole on a post (4 m), a sandbag wall section (2 m), a sign board "VANE
    CONSOLIDATED - ASSESSMENT IN PROGRESS".
25. **Company banner.** A 12 m canvas banner "UNDER NEW MANAGEMENT" with VC lettering, rope ties at each end.
26. **Junie's salt charges.** A canvas satchel; each charge a bundle of three salt jars wrapped in wire, a fuse and a magnet
    clamp, 40 cm.
27. **Abel's furnace lantern.** A keeper's lantern of black iron, its glass soot-black, a cyan glint inside (the
    corrupted Abel's; it falls into the canyon when he's freed).
28. **Saint Ada's Reliquary, whole.** Granite, banded in gold, a recess on the lid for the ember (the backlog design,
    unbroken), 2.2 m.
29. **Dredger chief stoker.** The hose-man (6) in a furnace-proof leather hood and long gauntlets, two tanks on his back.

The barge's boarding kit (walkable; Main adds collision; Company iron, rust and VC paint):
30. Riveted iron deck plate sections, 10 x 10 m, with a hatch variant; railing sections.
31. **Soul coal.** A lump of black faceted coal with glowing cyan veins (30 cm); a burlap sack stencilled VC.
32. Smokestack, 3 m across, 14 m tall, an iron ladder up one side; a deck jib crane, 12 m; the hose drum on its gantry,
    drum 6 m across (with a jam lever); a furnace vent grille, 2 x 2 m, glowing slots; a brass horn cluster on a bracket.
33. **The soul furnace.** A 6 m iron furnace with pipes and a glass-windowed soul cage at its heart (an intact and a
    burst version).
34. Furnace hall: a round riveted shell 26 m across and 9 m high, a catwalk ring at 4 m, two stokehold doors; a floor
    grate wedge (12 make the ring); a draft valve (a wheel valve on a pipe run, 1.5 m); the stoker's office set (a desk,
    an iron safe, a radio set, the manifest: a thick ledger with brass VC corners).
35. Company air-shaft head: a timbered shaft head with a pulley frame, 4 m, for the Providence.
36. Company camp: a canvas wall tent with a VC stencil, stacked VC crates (3 sizes).
37. **The barge wreck:** the barge (12) broken in two and burning, for the canyon floor (seen from afar); and a
    grapple launcher (the tender's, 14) as its own model for the Lamplighter's rail.
38. (optional) Company hopper car: a rail coal hopper in VC paint, for the depot's siding.

Gone from the first draft: Lucky Ned and his revolvers and dealer's shoe (later rounds), the Doorward.

## 15. Notes for Main (what the build will need)

- **New code, small:** `ACompanyLoudspeaker` (shootable horns that cut a broadcast), the Rappel entrance (tenders and the
  hose), the intake's draw zone and the hose's swing (Main 5, Back Pay), tender gas-bag weak spot, the hose drum jam, the
  hose-man's lantern target (Main 7), soul followers (Main 7), scenes `BargeFall`, `FreeAbel`, `SaintAdaWakes`, the gang
  skiff's fly-by (Main 3, the cold open's skiff flight), the Reliquary's guttering and lit states, `SmogThin`, Sexton's
  secrets list (a vendor page) and the map marks.
- **Bigger:** the barge's walkable kit and furnace hall (the grate ring, valves, cage); Abel's rework (`Boss_Abel`: the
  smog look, the tendril, the call-back lock and pips at 66/33% and the final hold at the keeper's post, the drink lines to
  six bodies with the optional, Phase 3's fog surge; phases 2 and 3's lanterns and wind kept).
- **Step indices that code reads:** Main 2's Ledger step stays 3; Main 4's Reliquary look stays step 4; Main 5's lantern
  step stays 2; Abel's HangStep 1, FightStep 2, SceneStep 3 move from Main6 to **Main7** (same numbers); Main 7's
  `ADoorHandoff` retires; `LanternBail` item -> Main5; the station board's Lily line and the hearse car's open door stay on
  Main 7.
- **Pacing (`Looter.Progression.Pacing`, `Tests/PacingPaths.h`):** Dredgers join Mains 2-7 and the mine (about +60 kills on
  the story path at levels 2-7, the Chief Stoker a Hallowed one at 7), Abel moves from Main 6 to Main 7 (his 2,066 kill XP
  with him), Paid on Delivery adds 300. Trim mission XP, not kills, to hold 6.7-7.1 (story) and 7.3-7.7 (everything); Abel's
  arrival check reads Main 7 now.
- **Performance:** the boarding kit and furnace hall only during Main 6 (story-gated, culled; consider a streamed
  sublevel), the followers' wisps capped, coal lumps instanced, Phase 3's fog surge a height-fog change (no volumetrics);
  `Tools\perf.ps1` on the barge's deck, in the hall and on the boards in Phase 3.
- **Words:** the Seven-Days-era topics of Delia, Tilly, Aldana, Hob's arrivals, Abel's board and the heroes' pick blurbs
  need the same pass (each mission's round).

## 16. Decisions for the user (recommendation first)

1. **Abel's Vigil Stone (the rematch by the Keeper's Gate).** A) Keep it as "the smog's echo of him": the furnace's shadow
   of Abel, fought again for his loot; the real Abel stays at the chapel · B) Retire it: a freed soul isn't fought again.
2. **Calling Abel back.** A) Required at each third: his Smog bar locks until you hold at the keeper's post (no fail, no
   skip) · B) Automatic: a short scene at 66% and 33%, no input.
3. **The six dead on the boards in Abel's fight.** A) An optional "save all six" (each one drunk heals him and rises
   Restless) · B) Leave them out: a plainer fight, the dead lie still.
4. **The furnace hall's guard in Main 6.** A) The Chief Stoker, a Hallowed hose-man (existing rank, a Tripo look later) ·
   B) No guard: the valves under the turning hot ring and the stokers' pushes only.
