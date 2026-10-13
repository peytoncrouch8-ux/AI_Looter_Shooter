# Ransom's Rest expansion: history

Superseded drafts from `Docs/Areas/RansomsRest_Expansion.md` (2026-10-13), moved here verbatim when the doc was condensed.
Never load by default. Everything below is WRONG now where it says "superseded"; the live doc holds the current design.

## The draft's status line (superseded: the design is approved)

> The design for the user's asks of 2026-10-13, for approval before the missions' data, the mountain, the interiors and
> the house kit are built. Once approved, section 11's edits carry it into `Docs/Story.md` and `Docs/Areas/RansomsRest.md`.
> It uses the names of wave 1's systems, being built now (`EMissionStart::Offered`, `FMissionOffer`, `AMissionArtifact`,
> `ARespawnMarker::MarkAfterMission`).

## Section 1, the decisions table as drafted

Superseded in part: the marks are ? to pick up and ! to turn in (the user's swap, 2026-10-13); the save points are Hob's
roosts, not graves (the user's call, 2026-10-13); the outpost's "save grave" is the CaldersPost roost.

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

Section 1, "What changes for the player", superseded line (graves → Hob's roosts; full clear 3 h 15 in section 2):

> - **The pace slows.** The player walks 10% slower; the main path grows from about 60 to about 75 minutes, a full clear to
>   about three hours. Ten graves, each a landmark with a lantern, are the save points.

## Section 2, the mission flow as drafted

Superseded: the marks (now ? to pick up, ! to turn in); the XP column (retuned to the user's targets, story ~5.5 and full
clear ~6.8, on the 180 × (L^2.8 − 1) curve; amounts in the mission scripts); the graves in M1's and M4's rewards (Hob's
roosts).

> In the order a player meets them. Minutes include walking. XP is the turn-in's fixed amount (section 10 retunes the
> side missions and the kill scale so the story still ends about level 6–7). Main missions carry a gold !, side missions
> a white !, each at its giver on the map, the minimap and over the giver's head; a mission ready to turn in shows a ? at
> whoever it's turned in to.

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

> A giver's ! shows once the mission's prerequisite is turned in.

## Section 3, as drafted

Superseded: section 10's sums (retuned); the graves as save points (Hob's roosts).

> Each step names its objective kind in brackets, all existing kinds unless marked **new** (section 12 lists the new
> code). "Optional:" objectives never hold a step; each done adds 10 silver (at level 1) to the turn-in, Borderlands' way,
> and no experience, so section 10's sums hold. Giver tags follow the wave 1 contract (`Speaker_<Name>`, `NoticeBoard_<Name>`,
> `Artifact_<Name>`). Lines marked as today's are unchanged; the rest are drafts for the user.

Main 1, step 2 as drafted:

> 2. Claw out of the grave: press {Jump}. [Scene GraveWake] The family plot's grave is marked as Ellis climbs out (its
>    MarkAfterMission cleared): the first save point, its lantern catching beside the headboard.

Main 6, the turn-in's last sentence as drafted:

> **Finishes with the scene** (today's). The keeper's grave is a save point the player has usually marked on the way.

## Section 6, as drafted

Superseded by the built mountain (the core grown, Larkspur High moved north, the quarry grown into a bowl and a neck):

> **Today.** Larkspur Ridge closes the valley on the north. Its foot runs along y ≈ 86–102 from the Rim (x ≈ −90) to the
> East Ridge (x ≈ 104, y ≈ 70), with an 8–12 m cliff band and steep pines above it; its crest stands about +42 m some 42 m
> behind the foot (y ≈ 130–145). Behind the crest the ground falls away, and a higher forested ridge, Larkspur High, rises
> from y ≈ 166 to about +75 m at y ≈ 211. The Old Quarry is a notch in the foot at x 8–32, y 92–100. The north boundary
> follows the foot.
>
> **After.** The valley side of Larkspur Ridge looks as it does today from below (its foot, band and pines), except for the
> lift's cable rising out of the quarry, the lift head on the skyline and Calder's watchtower showing over the ridge's west
> shoulder. Behind the crest:

The core square's contingency (not needed: the valley compare passed):

> If the compare fails, the fallback is a second core square for the mountain alone (more generator work: the seam
> between two cores).

The draft's mountain levels (superseded by the user's call, 2026-10-13: the mountain 5–6, the Foreman set 7):

| Zone | Where | Size | Levels | What's there |
|---|---|---:|---|---|
| The Old Quarry (grown; section 7) | x 0–40, y 90–120, +1 m | 1,100 m² | 5–6 | the winch house, the lift's bottom station, the lift-men's plot, the Goat Steps' foot, the derrick ruin |
| The Crest | x −10–40, y 128–150 | 1,500 m² | 5–6 | the lift head, the crest path, the Goat Steps' head, the valley's best view after Ransom's Point's |
| Calder's Post | x −88 to −45, y 140–190 | 3,200 m² | 5–6 | the knoll on the Rim: the watchtower, Ruth's cabin, the corral and lean-to, the board, the Ranger's grave |
| Larkspur Bench | x −45–60, y 140–230 | 9,000 m² | 6–7 | meadow, the tarn, a pine stand, the ore tramway, the Dinner Pail camp |
| The Providence Diggings | x 40–112, y 170–233 | 4,700 m² | 6–7 | the mine mouth and tag board, the ore bin, powder house, smithy, spoil heap, the miners' plot, the Foreman's Vigil Stone |
| The Providence (interior) | under Larkspur High, north of the mouth (74, 230) | 1,500 m² of floor | 6–7; the Foreman 8 | adit, upper gallery, the Foreman's office, decline, lower drift, the stope |

> A Legendary monster at a set level 8 (6, and Soulfed's 2), orange tag and coal, a page in the Ledger, the Legendary loot
> table; about 3,400 health (12 × a dead miner of his level), tuned to a 2–3 minute fight.

Encounters as drafted: Camp Dinner Pail, Ambush Spoil, MineUpper (N3) and MineLower (N3) at 6–7; The Foreman: "the
Foreman (8) and two waves of 3 dead miners", levels "8; 6–7".

> - **The lift**, both ways, from N1 on; and **fast travel** between the mountain's two graves and the valley's.

> Ransom's Point's tower runs about 6.5 ms today

## Section 7, as drafted

> It holds the winch house, the cage's landing, the lift-men's plot (a save grave), the QuarryCrew fight's floor ...

## Section 10, Experience and levels, as drafted (superseded)

Superseded by the user's call (2026-10-13): a full clear about level 6.8, the story alone about 5.5, the mountain 5–6,
the Foreman set 7; the XP curve is now 180 × (L^2.8 − 1) capped at 65, and `Docs/Progression.md` holds the kill rules.

> **Superseded in part (2026-10-13):** the user set a full clear at about level 6.8 and the story alone at about 5.5, with the
> Foreman at 7 (section 13). The sums below are the draft's; build step 4 redoes them to those targets.
>
> **The rules** (`Docs/Progression.md`): kill experience = 10 × the kill scale × 1.08^(level − 1) × the rank's multiplier
> (Restless 2, Gravebound 4, Soulfed 10, Boss 20), less 15% for each level the creature is below the player; missions give a
> fixed amount at the turn-in. Levels start at 100 (2), 212 (3), 337 (4), 477 (5), 634 (6), 810 (7), 1,007 (8), 1,228 (9) and
> 1,476 (10).
>
> **The change:** the kill scale 0.37 → **0.33**, and the side missions' experience as in section 2 (Side 1 35 → 30, Side 2
> 40 → 35, Side 3 40 → 35; the new ones 15–45). The main missions keep theirs (250 in all).
>
> **The new kills** (at today's 0.37, before the change, to sit beside Progression.md's table): The Old Fare's waiting room
> (levels 2–3; three Unpaid, a Restless one, a Bell-Ringer) about 24; Knocking Below's cellar (its spawner's 3–4; three and a
> Restless one) about 21; the mountain and the quarry (47 kills: 19 at 5–6 and 2 Restless there, 22 at 6–7 and 3 Restless
> there, the Foreman at 8, about 5.2, 11.3, 5.7, 12.2 and 63 each) about 346, about 312 after the falloff as the player
> climbs past them.

| Play | Kill XP at 0.37 | at 0.33 | Missions | In all | Level |
|---|---:|---:|---:|---:|---:|
| The story only: the mains, the fights on their way, the roaming dead once | 439 | 392 | 250 | 642 | 6.04 |
| The story and Sides 1 and 2 (Progression.md's "normal" today: 794, 6.91) | 471 | 420 | 315 | 735 | 6.57 |
| The story and every valley side mission (adds Calder's Caches, The Old Fare, Knocking Below) | 516 | 460 | 380 | 840 | 7.15 |
| Everything: also the optional camps (91), the Gravemother and her brood (90), the mountain (312) | 1,009 | 900 | 525 | 1,425 | 9.79 |

> So the story ends about level 6, and about 7 with the valley's side missions, as asked. A full clear ends near 9.8, at
> the *Lily*'s floor (Story.md gives it 9–15; that band was written when Ransom's Rest ran 1–10, and should start nearer 7
> when the *Lily* is designed, so a story-only player isn't three levels under). Abel, at his set 7, meets a valley-sides
> player at about level 5.6 and a story-only one at about 4.3 (4.5 today); a player who clears the mountain first meets him
> near 8.7 and finds him easier. That's the price of the mountain opening mid-story (question 1).
>
> **Zone levels** (`layout.json` `gameplay.levels`): the band 1–6 becomes **1–7**. New zones, listed before "valley":
> quarry (now the bowl's polygon, 5–6, unchanged), crest 5–6, calderPost 5–6, bench 6–7, diggings 6–7, providence (the
> interior's polygon) 6–7. The cellar's spawner keeps its own 3–4 (a spawner's level is kept, as today); the depot stays in
> the valley's 2–3. New kind: ForemanCreature at 6 (Soulfed's +2: 8). **When it opens:** the lift after Main 5, with the
> Gravemother: a player arrives at about level 5.5 for the quarry's 5–6 and climbs to about 7 by the mine's 6–7; the
> Foreman at 8 is the area's hardest fight, a Legendary over the boss as the Gravemother is over the Sink.
>
> **The checks:** `Looter.Progression.Pacing` gets the four rows above and fails outside story only 5.9–6.4, valley sides
> 6.8–7.4, everything under 10. `Looter.Economy.Balance`: the valley's new purses (Calder's 30, Fenn's 30, Thaddeus's 40)
> and their fights and containers raise a normal play's silver by about 180, to about 1,450, inside its 1,000–1,800; the
> sheriff's bounty (80), the mountain's purses and the kept payroll (about 400) come on top for an explorer.

## Section 11, superseded lines

> 11. *The game's systems*: Death and respawn becomes "Graves are save points: walking up to one lights its lantern. A death
>     wakes Ellis at the nearest open grave, Continue at the last marked one, and fast travel joins them; ten on Ransom's
>     Rest." Mission markers adds "a gold or white ! over whoever offers a mission, a ? over whoever it's turned in to".
>     *Practice on Skyreach* is removed.

> 13. *Decisions*: decisions 4–6 are marked superseded, and a 2026-10-13 entry records the map order, the skylift, the
>     opening, the givers, the save graves, the mountain and the house kit.

From the `Docs/Areas/RansomsRest.md` paragraph: "levels 1–7 (the Gravemother 6, Abel 7, the Foreman 8)" and "*The hub
afterwards* lists the ten graves" (superseded: the Foreman 7; Hob's roosts).

## Section 12, build steps as planned (steps 1–5 since built; superseded: graves → Hob's roosts, kill scale 0.33 → the retune)

> Each step ends with something to look at or play. Steps on one line run side by side on disjoint files against a shared
> contract written first.
>
> 1. **Wave 1** (running): the hold ring, the compass, a new game on the cold open with the hints in Ransom's Rest and the
>    pace 0.9, the givers, graves as save points; the art agent's house look sheet (`35_Homes.jpg`). *You approve:* this
>    document and the house look sheet.
> 2. **In parallel, after the approval:**
>    - **2a. Mission features** (code): optional objectives (`bOptional`, the turn-in bonus, "Optional:" on the tracker and
>      page); mission items (`AMissionItem` pickups, the Missions page's Carried list, a named creature dropping one); a
>      turn-in with a choice of two rewards (`FMissionTurnIn` choices, the card's two buttons); speaker points as turn-ins
>      anywhere (a coffin, a speaking tube, a tag board, Pruitt's hatch); a height band on `ASafeGround`; the minimap's
>      UNDERGROUND plate; the porch plate, keepers' roll and other small interactables. Tests per rule.
>    - **2b. The ore lift** (code): `AOreLift` (the cage on its cable, two stations, the bell pulls, the call), the ride
>      scene from `SkiffRide`'s pattern, Board.Lift, saves held during the ride; `AWatchFire`, the far answering light,
>      `AInteriorVisibility`. Tests: a ride both ways, a call, a save after.
>    - **2c. Creatures** (code): `ADeadMinerCreature` (the swing, the lamp's weak spot, working idles, barks),
>      `AForemanCreature` (`UBossComponent`: fuse sticks, the roof, the shift, the dark, the show and last word), Hiram and
>      Thaddeus as story characters (pose tables like Amos's), Ledger pages for all four. CastShots for each.
>    - **2d. Art, the house kit**: the eight homes, three fronts, well, outhouse, windmill, Whitlock barn, coal stage; then
>      **2e. interiors**: the workshop and cellar, the depot, the winch house, the cabin and the watchtower; **2f. the
>      mountain kit**: lift (cage, tower, head, cable), the Diggings, the mine's rooms; **2g. characters**: the dead miners'
>      costume set, the Foreman, Hiram and Thaddeus (a look sheet to you first for the dead miners and the Foreman); **2h.
>      props**: every artifact and item (crank, sack, pin, key, tape, coins, tags, fang, badge, hat, journal, code book,
>      payroll book, the Deacon's slip). Two art agents at most, in this order.
>    - **2i. Terrain**: the core grown to 512 m with today's valley compared within 1 cm; the Bench, Larkspur High moved,
>      the East Shoulder, the quarry bowl, the Goat Steps, the knoll and tarn, pads and paths; the boundary with its neck,
>      open edges and the mine's tongue; the scatter's alpine meadow and larkspur; `terrain_check` and the identity check
>      for the tutorial island and TerrainTest. *You see:* renders from the lift head, the watchtower and the valley.
> 3. **The level** (after 2d–2i): the houses swapped and the new lots, the mountain's buildings and interiors, the lift,
>    the ten graves, the encounters in `layout.json`'s gameplay, the loot (`build_area_loot.py`), the town life's
>    households, lights and smoke; props re-settled; path probes on the Goat Steps, the neck, the Bench's paths and the
>    mine. *You play:* the lift up and the Goat Steps down.
> 4. **The missions' data** (after 2a–2c and 3): all sixteen in `create_mission_assets.py` and
>    `create_side_mission_assets.py`, every line in `create_story_lines.py`, the Ledger pages, `gameplay.levels` (band 1–7,
>    the new zones, the Foreman's kind), the kill scale 0.33, the pacing and economy tests. *You play:* the opening to the
>    end of Main 3, then the rest.
> 5. **Sounds** (beside 3 and 4, once 2a–2c name their cues): the lift (winch, steam, whistle, the cable's hum, the cage's
>    clank and bell), the mine (drips, creaking timbers, distant picks), the dead miners' voices and swing, the Foreman
>    (fuse hiss, roof fall, the dark), the cellar's knocking, the depot's clock, the watch fire, the new mutters.
> 6. **Checks**: every `Looter.*` test; tours with section 6's views, perf with the editor closed on the watchtower's view;
>    HudShots (the offer card, the ! and ?); a full play by the Demo agent from a new game to the station board, the
>    mountain included; then `Docs/Story.md`, `Docs/Areas/RansomsRest.md`, `Docs/Progression.md`, `CODEMAP.md`,
>    `Docs/Pipeline.md` and the handoff. *You approve:* Ransom's Rest, expanded.

## Section 13, Open questions, as asked and answered (2026-10-13)

> **Decided by the user (2026-10-13), overriding the text above where they differ:**
> 1. The ore lift opens after Main 5 (as recommended).
> 2. **Experience: a full clear leaves the player at about level 6.8, the story alone at about 5.5** (the user's own answer,
>    not section 10's 9.8). So section 10's sums are redone in build step 4: the kill scale and every mission's amount come
>    down, most of a side mission's reward is silver and guns, and the mountain's levels drop with it: the zones 5–6, **the
>    Foreman at a set 7** (5 and Soulfed's 2), level with Abel. `Looter.Progression.Pacing` checks story only 5.3–5.8,
>    everything 6.6–7.0.
> 3. Back Pay ends in the choice (as recommended).
> 4. Thaddeus Bright is in the cellar (as recommended).
> 5. Ruth saw it all through her glass and closed Ellis's eyes (as recommended).
> 6. The Goat Steps and the lift both (Main's call on the recommendation).
> 7. The town board has two postings now (Main's call on the recommendation).
> 8. Hiram Coyle in person (as recommended). The main menu's subtitle says Teropa, not Skyreach.
> The homes kit (look sheet 35, 14 models) is approved for the game.
>
> Tap-to-answer, the recommendation first (as they were asked):
>
> 1. **When does the ore lift open?** (a) After Main 5, with the Gravemother: mid-story, the mountain at 6–7 and the Foreman 8
>    (recommended). (b) After Main 4: earlier, the mountain at 5–6 and the Foreman 7. (c) After Main 6: Abel stays the
>    hardest fight for every player, and the mountain is the last stretch before the train.
> 2. **Where should a full clear leave the player?** (a) Near level 9.8, at the *Lily*'s floor, with the story at 6 and the
>    valley's sides at 7 (recommended). (b) Near 8.5: the kill scale 0.30 and the mountain's missions halved (the story drops
>    to about 5.8).
> 3. **The Providence's ending?** (a) A choice at the tag board: pay the eleven (they rest, a Rare gun) or keep the payroll
>    (about 400 silver, the dead keep working) (recommended). (b) No choice: Ellis always pays them.
> 4. **Tilly's father in her cellar?** (a) Yes: Thaddeus Bright, a friendly dead man at his bench, and the tape for Tilly
>    (recommended). (b) No: the cellar's dead stay nameless and the turn-in is plain.
> 5. **Ruth's journal?** (a) She watched the robbery through her glass, saw Ellis draw on the Deacon, and closed Ellis's eyes
>    at dawn (recommended: it seeds her Act I truce). (b) She saw only the skiff, and the night stays unseen.
> 6. **The way down?** (a) The lift both ways, plus the Goat Steps, a one-way drop of rock ledges into the quarry
>    (recommended). (b) The lift only.
> 7. **The town's bounty board?** (a) Two postings now (Calder's note and the sheriff's bounty), repeatable bounties with the
>    Ledger's endgame later (recommended). (b) Also three small bounties now on today's camps (the orchard's edge, the sheep
>    fold, the Webwood).
> 8. **Hiram Coyle in person?** (a) Yes: the lift's dead engineman by his boiler, a friendly dead man like Amos, in the dead
>    miners' costume set (recommended). (b) A voice in the speaking tube only, and the lift posted on the Rangers' board.
