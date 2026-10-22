# STORY phase 1: Ransom's Rest, cut and reworked (plan only; nothing in the game changed)

**Phase 2 as built (2026-10-21), after Main's rulings — these override the sections below where they differ:**
- No new canon: secrets 6 and 7 replaced by the keepers' stores in the lantern house (loot only) and a spiders' hoard
  under boot hill's hanging tree (spiders hoard what a saint has touched). Abel's tin hint dropped (AbelOnBoard[1] cut).
  Secret 1 is "the Widow's strongbox" (a bank job's strongbox, no Ned note); the Pike variant line dropped; no Hob line
  on the pages (secret 10 is the existing collection).
- Fenn's fare and the whole Old Fare kept word for word (lines and steps); only its second push (DepotReturn) added.
- "Never a priest's to do" kept (Main 6's offer whole). Tilly's "No. It wasn't on him. I looked." keeps the beat.
- Amos's twist (DA_Lines_AmosRoad, 3 lines) plays once the hands' fight is won: AEncounterSpawner::ClearedLines (new).
- Section-0 extra cuts done: Aldana's Ada talk is a page slid under the vestry door (READABLES AldanaPage, sets Ada.Told);
  six readables' Hob quips cut; Hob's death lines to four; town mutters: only each stage's second lines cut (three), not
  "one a house" (the town-life test pins the stages).
- NoteCaches (Calder's note naming the caches) kept: the note's art names them too.
- Spoken words now about 2,650 of 4,132 (-36%).

The user's calls (Handoff "Direction" + "Overnight run", 2026-10-21): cut dialogue ~half; most lines become short
captions while walking or fighting (Borderlands radio); long scenes only at big beats; fetch steps become fights or
finds; add things to find off the path; **keep the plot** (every beat, character and saga seed). DA_HeroLines' rows
stay consistent. Word counts: section 0. Beat ledger (proof nothing is lost): section 5.

## 0. Numbers, and how the game already plays words

**What slows play today** (read from the code): every line is a non-blocking caption (`UCaptionSubsystem`, a queue), so
the player is never frozen. What holds them is (a) **an offer card waits until its giver's words are said and the
player is still standing there** (`SpeakerPointComponentOffers.cpp`); a turn-in followed by the next offer is one long
stand (Tilly's Main 3 + Main 4: 146 words; Aldana's Main 4 + Main 5: 91; Amos's offer: 97); (b) long talks the player
stops to read; (c) walking between givers (fetches). The "radio" channel already exists: **Hob's perch arrivals** play
as each step starts, queued, wherever the player is (`AHobBird` perches, `build_area_story.py`; their words are the
`DA_Lines_*` sets they name, so changing a set's words needs no build-script edit). Readables' lines, hay bales',
posters' remarks, town-life mutters, roost and boss lines are on-the-move too.

**Word counts** (words heard, each distinct line once: `DA_Lines_TillyMain3` and Main 3's turn-in are one; counted by a
script over the sources and over section 1's tables; barks, UI prompts, page texts and cards apart):

| Kind | Before | After | Change |
|---|---:|---:|---:|
| **Conversation**: words said at a giver (offers, turn-ins, talk topics, Hob's taps) | 2,566 | **1,428** | **-44%** |
| ...the same without the two scenes kept whole (Sexton's deal 123 -> 111, Delia's hand-off 28) | 2,415 | 1,289 | -47% |
| Offer lines an offer card waits on | 470 | 251 | -47% |
| Said on the move (Hob's arrivals, readables' lines, bales, posters, town, roosts, bosses, scenes) | 1,566 | 1,226 | -22% |
| **All spoken lines on Ransom's Rest** | 4,132 | **2,654** | **-36%** |
| ...of which new (the secrets' and their hints' lines, section 3) | 0 | 111 | |
| Page texts (read at will) | 973 | ~1,070 | +~100 (the secrets' pages) |
| Barks (combat) | 410 | 410 | untouched |

So the talk that stops play is cut about in half; what's left of it is two- or three-line offers and turn-ins, plus the
scenes the user's rule keeps long (Sexton's deal, one line cut and one shortened; the cold open, Sit with Pa and
Delia's hand-off whole). The on-the-move
words fall less because most of them are finds (readables), the scenes and the world's own chatter, which carry the
seeds. Going further to -50% of everything would mean cutting into those; the next things to go, if the user wants it,
in order: What Ada gave up as a talk (62 -> a readable page in the vestry, -62), the readables' Hob quips that aren't
seeds (-60), the town's mutters to one a house (-60), Hob's death lines to four (-20) = about -200, to -41%.

**Rules every phase-2 edit keeps:**
- Shared new lines are written without "Pa", "Ellis", "El", "kid" (the Deacon's/Sloane's "kid" excepted), "grandmother",
  "young'un": no hero row needed. Most cuts below *drop* family words, so rows get deleted, not rewritten.
- A changed line that keeps a family word keeps its row with `current` = the new Ellis text and the neutral/heroes
  rewritten in the same cut (section 4 lists every row touched).
- A line set taken out of `create_story_lines.py` SETS must also have its asset deleted from `/Game/Data/Story` (or kept
  word-for-word in sync): `Looter.Lines.NoFamily` scans *every* line set asset, and a stale asset with family words and
  no row fails it. Same for `DA_Lines_AldanaMain4` (no topic, kept in sync with Main 4's turn-in) and `TillyMain3`.
- `build_area_missionprops_valley.py` LINES / READABLES are fallbacks used only while a line set is missing; keep them
  in step or delete them (they never play once `create_story_lines.py` has run).
- Fenn's fare (Story.md: "keep it exactly as it is"): its beats (gold on the dead man's eyes, Sloane borrowed it against
  a debt, Ned's marker, the coins laid back) are all kept; only the chatter around them is trimmed. Main to confirm.

## 1. Every line or scene, and what happens to it

Key: **KEEP** as is; **CUT** (gone); **SHORT** (new words given); **MOVE** (new words, now said on the move; the trigger in
the Who column); **NEW**; **SAME** (a second source of a line counted elsewhere: keep it word-for-word identical to its
twin, or delete it). `[ROW]` = a DA_HeroLines row keys on it (what to do with it in section 4). Where each key lives:
`DA_Lines_*` = `Tools/Unreal/create_story_lines.py` SETS (which point or perch says it: `story_texts.py` TOPICS /
MUTTERS / PERCHES, `build_area_story.py place_hob()` lines 720-766, `build_area_farm.py`, `build_area_chapel.py`,
`build_area_deck.py`, `build_area_whitlock.py` remarks); `MainN.*` = `create_mission_assets.py` MISSIONS (offer, turn_in);
side ids (`Side1`, `CaldersCaches`, `OldFare`, `Side2`, `KnockingBelow`, `Side3`, `DeadLift`, `LastWatch`, `BackPay`) =
`create_side_mission_assets.py`; `<Readable>.lines` and `plate` = `story_texts.py` READABLES / PORCH_PLATE; `cpp:` =
`Source/AI_Looter_Shooter/` (`Scenes/ColdOpen.cpp`, `Scenes/SitWithPa.cpp`, `Bosses/AbelRules.cpp`,
`World/RoostRules.cpp`, `World/WantedPoster.cpp`, `World/TownLifeRules.cpp`, `World/OreLift.cpp`, `World/WatchFire.cpp`,
`Creatures/ForemanFight.cpp`). Fallback copies that never play once the sets exist: `build_area_missionprops_valley.py`
LINES/READABLES. Barks (`Audio/CreatureVoiceBarkLines.cpp`, 74 lines) and FIGHT-LAYOUT's set-piece beats (layout.json)
are combat chatter and not touched. Hob's taps (what he says when the player talks to him, `TOPICS['Speaker_Hob']`) all go
but the catch-all "Don't look at me. I only carry the messages.": his perch arrivals carry the story now.

The caps this pass keeps: an offer two or three short lines (the card has the rest), a turn-in three, a Hob arrival
one line of about ten words, a giver's "after" topic one line. Exempt, kept whole: the cold open, Sexton's deal, Sit with
Pa, Delia's Heirloom hand-off, Abel's fight lines, the Foreman's, the seeds' readables.

### Cold open and the grave (Main 1)

| Key | Who | Act | Words after |
|---|---|---|---|
| cpp:ColdOpen:* | Deacon, Ellis, Abel | KEEP | (the big beat, 29 words) |
| DA_Lines_HobWakes[0] | Hob (perch, the claw-out) | KEEP | |
| DA_Lines_HeadboardAbel[0] | (board) | KEEP | |
| DA_Lines_HeadboardAbel[1] | Hob | SHORT | Frost in August. That's no weather, sunshine. |
| DA_Lines_HeadboardEllis[0] | (board) | KEEP | |
| HeadboardEllis.lines[0] | Ellis | KEEP [ROW] | |
| HeadboardEllis.lines[1] | Hob | SHORT [ROW: rewrite] | Your grandmother. She did the letters twice. |
| DA_Lines_HobMain1[0] | Hob (tap) | SHORT | Up to the house, sunshine. |
| DA_Lines_HobAfterMain1[0] | Hob (tap, every mission now) | KEEP | |
| DA_Lines_DeliaMain1[0] | Delia | KEEP [ROW] | |
| DA_Lines_DeliaMain1[1] | Delia | SHORT | Take what's on the porch plate. |
| DA_Lines_DeliaPorch[0] | Delia | SHORT | I can't open this door to you. |
| plate[0] | Delia's slip | KEEP | |
| Main1.turn_in[0] | Delia | KEEP [ROW] | |
| Main1.turn_in[1] | Delia | KEEP [ROW] | |
| Main1.turn_in[2] | Delia | SHORT | A keeper doesn't lie still while his saint is dark. |
| Main1.turn_in[3] | Delia | SHORT | And eat what's on that plate. |

("He walks the boards at dusk" is said again in Main 6's offer, where it matters.)

### Main 2, Shall We Talk Business?

| Key | Who | Act | Words after |
|---|---|---|---|
| Main2.offer[*] | Hob, Ellis | KEEP [ROW on "Who?"] | (21 words) |
| DA_Lines_HobMain2Climb[0] | Hob (perch path, on accept) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain2Blue[0] | Hob (perch rock, the nest fight) | SHORT | See the blue one? Fed longer on the dark. Hits harder, drops better. |
| DA_Lines_HobMain2[0] | Hob (tap) | CUT | |
| DA_Lines_SextonWaiting[0] | Sexton | KEEP | |
| DA_Lines_SextonDeal[0..2] | Sexton, Ellis | KEEP [ROW on [1]] | ("Shall we talk business?" / "You were on this rail..." / "I attend every death, friend. It's my trade.") |
| DA_Lines_SextonDeal[3] | Sexton | SHORT [ROW: rewrite neutral] | Seven old friends of yours carry embers that aren't theirs. Bring each to this rail at the new moon. |
| DA_Lines_SextonDeal[4..11] | Sexton, Ellis | KEEP (4 rows untouched) | (a little of you back, all seven and you rest, your father's fare, "And if I don't?", the Gravewind for ever, where to start, ask a keeper, "So are you, friend.") |
| DA_Lines_SextonDeal[12] | Sexton | CUT | (his ledger line is the next topic's) |
| DA_Lines_SextonLedger[0] | Sexton | SHORT | Do read them, friend. |
| Main2.turn_in[0] | Sexton | CUT | |
| Main2.turn_in[1] | Sexton | KEEP | |
| DA_Lines_SextonAfter[0] | Sexton | SAME | (= Main 2 turn-in [1]) |
| Main2.turn_in[2] | Ellis | KEEP [ROW] | |
| Main2.turn_in[3] | Sexton | KEEP [ROW] | (he knows where it fell: seed) |
| Main2.turn_in[new] | Hob (said as Ellis leaves the rail) | NEW | South-face stairs go down to where the gang tied up, sunshine. |
| LookoutStains.lines[*] | Ellis, Hob | KEEP (1 row) | |

### Main 3, Cold Welcome (FIGHT-LAYOUT's Main Street ambush sits between the gate and Tilly)

| Key | Who | Act | Words after |
|---|---|---|---|
| Main3.offer[0] | Delia | KEEP | |
| Main3.offer[1] | Ellis | CUT | (Ellis asked Sexton the same; Delia answers unasked. The row stays: Main 2's twin keys it) |
| Main3.offer[2] | Delia | SHORT | He ran out with his lantern lit. They carried him down without it. |
| Main3.offer[3] | Delia | SHORT | Tilly Bright laid him out. Ask her. |
| Main3.offer[4] | Delia | CUT | (into Hob's road perch) |
| DA_Lines_HobMain3Road[0] | Hob (perch gate, on accept) | MOVE | Farm road, through the orchard. They'll shutter up at you. Let them. |
| DA_Lines_HobMain3Gate[0] | Hob (perch gate, the fight) | KEEP | |
| DA_Lines_HobMain3Tilly[0] | Hob (perch sign, gate won) | SHORT | Bright & Daughter's down the street. Mind the boardwalks. |
| DA_Lines_HobMain3[0] | Hob (tap) | CUT | |
| DA_Lines_TillyClosed[0] | Tilly | KEEP | |
| Main3.turn_in[0] | Tilly | CUT | |
| Main3.turn_in[1] | Tilly | KEEP [ROW] | |
| Main3.turn_in[2] | Tilly | KEEP [ROW] | |
| Main3.turn_in[3] | Ellis | KEEP [ROW] | |
| Main3.turn_in[4] | Tilly | SHORT | No. I looked. |
| Main3.turn_in[5] | Tilly | SHORT | Only its bail, snapped off in his fist. I couldn't open his hand till morning. |
| Main3.turn_in[6] | Tilly | SHORT | Here. A lantern's no use if it can't be hung. |
| Main3.turn_in[7] | Tilly | CUT | (the reward gun lands at the player's feet) |
| DA_Lines_TillyMain3[*] | Tilly | SAME | (= Main 3's turn-in) |
| DA_Lines_TillyAfterMain3[0] | Tilly | KEEP [ROW] | |

"Here." needs the bail handed over at the turn-in (section 4, code B); until it lands, [6] reads "It's on the sill. A
lantern's no use if it can't be hung." The ambush's own Hob lines ("Under the boardwalks...", "Behind you!...", "Street's
quiet. Go knock on Tilly's window...") are FIGHT-LAYOUT's, so HobMain3Tilly only hints at it.

### Main 4, Hallowed Ground

| Key | Who | Act | Words after |
|---|---|---|---|
| Main4.offer[0] | Tilly | SHORT [ROW: delete] | Father Aldana blessed that lantern every spring. He's barred in the vestry. Knock loud. |
| Main4.offer[1] | Tilly | CUT | |
| Main4.offer[2] | Tilly | CUT | (into [0]) |
| DA_Lines_HobMain4Road[0] | Hob (perch, on accept) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain4Yard[0] | Hob (perch, the yard fight) | MOVE (was his tap's lore) | No light, no road. Every soul on the Sundown Road drifted home when she went out. |
| DA_Lines_HobMain4[0] | Hob (tap) | CUT | |
| DA_Lines_HobMain4Aldana[0] | Hob (perch, yard won) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_AldanaBarred[0] | Aldana | SHORT | Stay back from that door. I've no comfort left for the dead. |
| DA_Lines_AldanaWaiting[0] | Aldana | CUT | (his barred line just said it) |
| DA_Lines_AldanaWaiting[1] | Ellis | KEEP [ROW] | |
| DA_Lines_AldanaWaiting[2] | Aldana | KEEP [ROW] | |
| DA_Lines_AldanaWaiting[3] | Aldana | SHORT | Ring her bell. If the saint still knows you, she'll answer. |
| DA_Lines_HobMain4Bell[0] | Hob (perch, the bell step) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_AldanaBell[0] | Aldana | SHORT [ROW: delete] | Ring her. Then go and look at her. |
| DA_Lines_HobMain4Reliquary[0] | Hob (perch, after the bell) | SHORT | Nobody answered. It's the saint that's out, sunshine. |
| KeepersRoll.lines[0] | Hob | KEEP [ROW] | |
| Main4.turn_in[0] | Aldana | SHORT | She didn't answer. Now you've seen why. |
| Main4.turn_in[1..2] | Ellis, Aldana | KEEP [ROW on [1]] | ("How do I find them?" / "A keeper's lantern can find an ember.") |
| Main4.turn_in[3] | Aldana | SHORT | They shot the keeper first in every town, and smashed his lantern. |
| Main4.turn_in[4..6] | Aldana | KEEP [ROW on [4]] | ("Your father's fell in the dark, whole." / spiders hoard / "Look in the Sink.") |
| DA_Lines_AldanaMain4[*] | Aldana | SAME | (no topic: keep in sync or delete the asset) |
| Main5.offer[0] | Aldana | CUT | |
| Main5.offer[1] | Aldana | SHORT [ROW: delete] | Bring it back. I'd stake the chapel on it. |
| DA_Lines_AldanaAfterMain4[0] | Aldana | SHORT [ROW: delete] | The Sink. I'll pray it's whole. |
| ReliquaryLid.lines[0] | Ellis | SHORT | A mark under the lid. A coin, with a line through it. |
| ReliquaryLid.lines[1] | Ellis | CUT | (into [0]) |
| ReliquaryLid.lines[2] | Hob | KEEP | (the toll mark: seed) |
| DA_Lines_AldanaAda[0] | Aldana | CUT | |
| DA_Lines_AldanaAda[1] | Aldana | SHORT | The first winter, nobody here could pay the old fare. |
| DA_Lines_AldanaAda[2] | Aldana | SHORT | So Ada lay down on the first board and asked the wind to take her. |
| DA_Lines_AldanaAda[3] | Aldana | SHORT | At the edge of the world she held up her light, so none of us would pay again. |
| DA_Lines_AldanaAda[4] | Aldana | SHORT | No saint crosses. Every one of them stayed behind. |
| DA_Lines_AldanaAda[5] | Ellis | KEEP [ROW] | |
| DA_Lines_AldanaAda[6] | Hob | KEEP | |

What Ada gave up stays a once-only talk after Main 4 (optional, a seed): 62 words from 110.

### Main 5, The Keeper's Lantern

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobMain5Way[0] | Hob (perch den rock, on accept) | CUT [ROW: delete] | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain5Way[1] | Hob | KEEP | (seeds the Gravemother) |
| DA_Lines_HobMain5Sacs[0] | Hob (perch, on the floor) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain5Lantern[0] | Hob (perch, sacs down) | CUT [ROW: delete] | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain5Out[0] | Hob (perch, lantern taken) | KEEP | (the doc's) |
| DA_Lines_HobMain5Out[1] | Hob | CUT | |
| DA_Lines_HobMain5[0] | Hob (tap) | CUT [ROW: delete] | |
| SheriffHat.lines[*] | Ellis, Hob | KEEP (1 row) | |
| Main5.turn_in[0] | Aldana | SHORT [ROW: delete] | Dark, but whole. Let me try the altar candle. |
| Main5.turn_in[1] | Aldana | SHORT | ...It won't take. It's lit from the saint, and the saint is out. |
| Main5.turn_in[2] | Ellis | KEEP [ROW] | |
| Main5.turn_in[3] | Aldana | SHORT [ROW: rewrite] | I don't know. Your grandmother will know what's owed a keeper. |
| DA_Lines_AldanaAfterMain5[0] | Aldana | CUT [ROW: delete] | |
| DA_Lines_AldanaAfterMain5[1] | Aldana | CUT |  |

### Main 6, The Gravewind (boss)

| Key | Who | Act | Words after |
|---|---|---|---|
| Main6.offer[0..1] | Ellis, Delia | CUT | ("Father Aldana couldn't light it." / "No. That was never a priest's to do.": Aldana's turn-in already sends Ellis to her for that reason) |
| Main6.offer[2..3] | Delia | KEEP (1 row) | (the beat: "Show him the way, even if he can't go." / "He'll be on the boards at dusk. He won't know you, Ellis. Go anyway.") |
| DA_Lines_DeliaMain6[0] | Delia | SAME | (= Main 6 offer [2]) |
| DA_Lines_HobMain6Way[0] | Hob (perch keeper's grave, on accept) | SHORT [ROW: delete] | Every keeper of this valley lies under that board. He's the first who won't stay put. |
| DA_Lines_HobMain6Way[1] | Hob | CUT | |
| DA_Lines_HobMain6Bridge[*] | Hob | CUT [ROW: delete] | (nothing plays it: `PERCHES['keepersBridge']` is never placed, build_area_story.py line 750) |
| DA_Lines_HobMain6Post[0] | Hob (perch, at the gate) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain6Fight[0] | Hob (perch, the fight) | KEEP | |
| DA_Lines_HobMain6[0] | Hob (tap) | CUT | |
| cpp:AbelRules:* | Abel, Hob | KEEP (2 rows) | (phases, the bell, Hob's catch) |
| cpp:SitWithPa:* | Abel, Ellis, Hob | KEEP (5 rows) | (the big beat) |
| DA_Lines_HobMain6Point[*] | Ellis, Hob | KEEP (1 row) | (the second light: seed) |
| DA_Lines_AbelOnBoard[0] | Abel | KEEP [ROW] | |
| DA_Lines_AbelOnBoard[1] | Abel | MOVE [ROW: rewrite] (secret 7's hint; the plate moved to Main 7's offer) | You hid under Keeper's Bridge when you'd done wrong, El. Your tin's likely still there. |
| DA_Lines_AbelAfterMain7[0] | Abel | CUT | |
| DA_Lines_AbelAfterMain7[1] | Abel | KEEP [ROW] | |

### Main 7, The Lantern Leans

| Key | Who | Act | Words after |
|---|---|---|---|
| Main7.offer[0] | Abel | KEEP [ROW] | |
| Main7.offer[1] | Abel | SHORT [ROW: rewrite] | Your grandmother's kept something back for you. Go home first. |
| Main7.offer[2] | Abel | SHORT | Eat what's on her plate, even if you can't taste it. |
| DA_Lines_HobMain7Lean[0] | Hob (perch keeper's post, on accept) | KEEP | |
| DA_Lines_HobMain7Lean[1] | Hob | CUT [ROW: delete] | |
| DA_Lines_DeliaMain7[*] | Delia | KEEP | (the Heirloom hand-off, "Hold the door.") |
| DA_Lines_DeliaMain7After[0] | Delia | CUT | (Hob's next perch says where to go) |
| DA_Lines_DeliaMain7After[1] | Delia | KEEP | |
| plate[1] | Delia | KEEP [ROW] | |
| DA_Lines_HobMain7Depot[0] | Hob (perch hearse, Heirloom in hand) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain7Board[0] | Hob (perch hearse) | CUT | (pure direction: the objective and its arrow say it; Hob still flies there) |
| DA_Lines_HobMain7[0] | Hob (tap) | CUT | |
| DA_Lines_TillyMain7[0] | Tilly | SHORT | Father's car is coupled up. The living won't ride with you, so it's the coffins. |
| DA_Lines_TillyMain7[1] | Tilly | KEEP | |
| DA_Lines_TillyMain7Father[0] | Tilly | SAME | (= TillyMain7[0]) |
| DA_Lines_TillyMain7Father[1] | Tilly | KEEP | |
| DA_Lines_TillyAfterMain7[0] | Tilly | SHORT | The car's yours whenever the line runs. |
| DA_Lines_AgentWindowLily[*] | Sloane | KEEP (1 row) | (the seed: Highwater Landing, Halfway Junction) |

### Side 1, Wanted: Already Dead

| Key | Who | Act | Words after |
|---|---|---|---|
| cpp:WantedPoster:RemarkFirst | Hob (first poster torn) | SHORT | Somebody wrote 'already' under 'dead or alive'. The town has a sense of humor. |
| cpp:WantedPoster:RemarkThird | Hob | KEEP | |
| cpp:WantedPoster:RemarkSixth | Hob (now the fourth: TornCount 6 -> 4) | SHORT | That's most of them. There's a note for you on the Rangers' board. |
| cpp:WantedPoster:NoteCaches | Calder | CUT | (it named the caches plainly; the card's riddles stay the clues) |
| cpp:WantedPoster:NoteTake | Calder | CUT | |
| cpp:WantedPoster:NoteHob | Hob | SHORT | Calder. Same hand that signed your poster. She shoots first, sunshine. |
| DA_Lines_HobSide1[0] | Hob (tap) | CUT | |
| OdellDaybook.lines[0] | Hob | KEEP | |
| Side1.turn_in[0] | Tilly | SHORT | So it was you, tearing them down. Good. |
| Side1.turn_in[1] | Ellis | KEEP [ROW] | |
| Side1.turn_in[2] | Tilly | KEEP | |
| Side1.turn_in[3] | Tilly | CUT | (the reward gun lands at the player's feet) |

### Calder's Caches

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobCaches[0] | Hob (tap) | CUT | |
| DeaconsSlip.lines[*] | Ellis, Hob | KEEP (1 row) | |
| CaldersCaches.turn_in[0] | Ellis | CUT | |
| CaldersCaches.turn_in[1] | Hob | KEEP | |
| CaldersCaches.turn_in[2] | Ellis | SHORT | Her fourth's at the Larkspur post. Up the old lift. |

### The Old Fare (Fenn's fare kept: every fact of it stays)

| Key | Who | Act | Words after |
|---|---|---|---|
| OldFare.offer[0] | Sloane | KEEP [ROW] | |
| OldFare.offer[1] | Sloane | SHORT | Every night they stand in my waiting room, waiting on a train. |
| OldFare.offer[2] | Sloane | KEEP | |
| DA_Lines_AgentDoor[0] | Sloane | SHORT | Are they gone? I can still hear them breathing. |
| DA_Lines_AgentDoor[1] | Sloane | CUT | |
| DA_Lines_FennLook[0] | Fenn | CUT | (the offer already has the dead waiting on a train) |
| DA_Lines_FennLook[1] | Fenn | KEEP | |
| DA_Lines_FennLook[2] | Fenn | SHORT | Sloane was the last living soul in here. Ask him. |
| DA_Lines_FennLook[3] | Hob (with the look; carries the old-fare lore from his cut tap) | SHORT | Gold on the eyes paid the ferry once, sunshine. Rob it, and every soul for a mile hears. |
| DA_Lines_AgentCoins[0] | Sloane | CUT |  |
| DA_Lines_AgentCoins[1] | Sloane | SHORT | ...His people laid gold on his eyes, like the old days. Gold, on a dead man! |
| DA_Lines_AgentCoins[2] | Sloane | SHORT | I only borrowed it. Against a debt. |
| DA_Lines_AgentCoins[3] | Sloane | SHORT | There. Under the door. Tell him I'm sorry. |
| AgentDesk.lines[0] | Ellis | KEEP [ROW] | |
| AgentDesk.lines[1] | Hob | KEEP | |
| AgentDesk.lines[new] | Ellis (secret 1's hint; the marker's back gets the pencilled words, section 3) | NEW | The middle widow. On the keeper's path. |
| DA_Lines_FennCoins[*] | Ellis, Fenn | KEEP (1 row) | |
| OldFare.turn_in[0] | Fenn | KEEP | |
| OldFare.turn_in[1] | Fenn | KEEP | |
| OldFare.turn_in[2] | Fenn | SHORT | There's a pistol under me. Never fired it. Take it. |
| DA_Lines_FennAfter[0] | Fenn | SHORT | Still waiting on the Gravesend train. I've paid. |
| DA_Lines_FennAfter[1] | Fenn | CUT | |
| DA_Lines_AgentWindow[0] | Sloane | KEEP | |
| DA_Lines_AgentWindow[1] | Sloane | CUT | |
| DA_Lines_AgentWindow[2] | Sloane | CUT | |
| DA_Lines_AgentWindowCoal[*] | Sloane | CUT | (Dead Lift's coal fetch is gone: delete the set, its asset and its topic) |
| DA_Lines_HobOldFare[0] | Hob (tap) | CUT | (its lore is in FennLook[3]) |

### Side 2, Unfinished Business (3 bales, then the hands come up the road; the twist is said over the last bale)

| Key | Who | Act | Words after |
|---|---|---|---|
| Side2.offer[0] | Amos | CUT |  |
| Side2.offer[1] | Ellis | KEEP [ROW] | |
| Side2.offer[2] | Amos | SHORT [ROW: rewrite, Gauge too] | What's left of him. And you're Abel's young'un. I'm sorry for it. |
| Side2.offer[3] | Amos | SHORT | Died last harvest with the hay half in. Got turned back on the road. |
| Side2.offer[4] | Amos | SHORT | My hands go through a bale like smoke. Load it for me? |
| DA_Lines_AmosMeet[*] | Amos | SAME | (= the offer) |
| DA_Lines_AmosBales[0] | Amos | SHORT | Under the hoist by the big doors. |
| DA_Lines_AmosBaleFirst[0] | Amos (bale 1) | KEEP | |
| DA_Lines_HobBaleThird[0] | Hob (bale 2 now) | KEEP [ROW] | |
| DA_Lines_AmosBaleLast[0] | Amos (bale 3, the last) | KEEP [ROW] | |
| DA_Lines_AmosBaleLast[1] | Amos | SHORT | ...Who's that out on the road? |
| DA_Lines_AmosRoad[0] | Amos (now AmosBaleLast[2], as the hands come up the road and the fight starts) | MOVE | That's the Merritt brothers out on the road, and Sol Tanner. My hands. |
| DA_Lines_AmosRoad[1] | Amos (AmosBaleLast[3]) | MOVE | Fever winter took them, and I owed them for the harvest. Never paid. Couldn't. |
| DA_Lines_AmosRoad[2] | Amos (AmosBaleLast[4]) | MOVE | Reckon they've come to collect. |
| DA_Lines_AmosRoad[3] | Amos | CUT [ROW: delete] | |
| DA_Lines_AmosHands[0] | Amos (tap in the fight) | SHORT | Don't let them near the hay. |
| DA_Lines_AmosThanks[0] | Amos | CUT |  |
| DA_Lines_AmosThanks[1] | Amos | KEEP | (the theme: paying on the far side) |
| DA_Lines_AmosThanks[2] | Amos | SHORT | They buried me with a gun I never fired. Better in a walking hand. |
| DA_Lines_AmosThanks[3] | Amos | CUT | (AmosFence says it: "Still waiting on her.") |
| DA_Lines_AmosFence[0] | Amos | KEEP | |
| DA_Lines_AmosAfterMain6[0] | Amos | SHORT [ROW: delete] | Saw a light on Gravewind Point at dusk. A keeper's still keeping. |
| WagesChalk.lines[*] | Ellis, Hob | KEEP |  |

(`DA_Lines_AmosRoad` stays as his tap during the fight with lines 0-2, the same words as AmosBaleLast[2..4]: SAME.)

### Knocking Below

| Key | Who | Act | Words after |
|---|---|---|---|
| KnockingBelow.offer[0] | Tilly | CUT [ROW: delete] |  |
| KnockingBelow.offer[1] | Tilly | SHORT | Something's knocked in my cellar every night since Wednesday. No keeper to walk them. |
| KnockingBelow.offer[2] | Tilly | KEEP | |
| DA_Lines_TillyKnocking[0] | Tilly | CUT | (the objective names the hook and the yard) |
| DA_Lines_TillyKnocking[1] | Tilly | KEEP | |
| DA_Lines_HobKnocking[0] | Hob (tap) | CUT | |
| OrdersSlate.lines[0] | Hob | KEEP | |
| DA_Lines_ThaddeusTalk[0] | Thaddeus | KEEP [ROW] | |
| DA_Lines_ThaddeusTalk[1] | Thaddeus | SHORT | They got past me when the door opened. A cellar wants keeping. |
| DA_Lines_ThaddeusTalk[2] | Thaddeus | KEEP | (the brass) |
| DA_Lines_ThaddeusAfter[0] | Thaddeus | KEEP [ROW] | |
| DA_Lines_ThaddeusAfter[1] | Thaddeus | MOVE [ROW: delete] (secret 5's hint) | My bottle's under the flat stone in the springhouse. Don't tell her. |
| DA_Lines_ThaddeusAfterMain7[0] | Thaddeus | SHORT | My car's carrying you, I hear. Mind the brass. |
| KnockingBelow.turn_in[0..2] | Tilly | KEEP | |
| KnockingBelow.turn_in[3] | Tilly | SHORT | A box of his is under the counter. Don't make me look at it. |
| DA_Lines_TillyAfterKnocking[0] | Tilly | SHORT | I hear his tape at night. It's almost a comfort. |
| DA_Lines_TillyAfterKnocking[1] | Tilly | CUT | |

(Thaddeus's "ThaddeusWaiting" words, "Shut that door behind you. They get out when it's open.", live only in
`build_area_missionprops_valley.py` LINES, set straight onto his point: kept, 11 words.)

### Side 3, The Gravemother

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobSide3[0] | Hob (tap) | CUT | |
| SheriffBadge.lines[*] | Ellis, Hob | KEEP (1 row) | |
| Side3.turn_in[0] | Mrs. Pruitt | KEEP | |
| Side3.turn_in[1] | Mrs. Pruitt | CUT | |
| Side3.turn_in[2] | Mrs. Pruitt | KEEP | (the hat line pays off Main 5's find) |

### Dead Lift (one thing to get, Abe's pin, in a fight; coal from the quarry's own bin; no crank)

| Key | Who | Act | Words after |
|---|---|---|---|
| DeadLift.offer[0] | Hiram | SHORT | Ain't had a passenger in four years. Riding up? |
| DeadLift.offer[1] | Ellis | KEEP [ROW] | |
| DeadLift.offer[2] | Hiram | SHORT | Then I need her clamp pin back. Abe Haskett took it down the hill the day of the fall. |
| DeadLift.offer[3] | Hiram | CUT | |
| DeadLift.offer[4] | Hiram | CUT | (into [2]) |
| DeadLift.offer[5] | Hob | CUT |  |
| DA_Lines_HiramWaiting[0] | Hiram | SHORT | Pin's on Abe, wherever Abe's got to. |
| DA_Lines_HiramWaiting[1] | Hiram | CUT | |
| DA_Lines_HiramCrew[0] | Hiram (mutter, the crew rise) | KEEP | |
| DA_Lines_HiramFit[0] | Hiram | SHORT | Coal in the firebox. Pin in the bottom sheave. |
| DA_Lines_HiramSteam[0] | Hiram | KEEP | |
| DA_Lines_HiramRide[0] | Hiram | SHORT | Hold the rail. Don't look down. |
| DA_Lines_HiramAfter[0] | Hiram | KEEP | |
| DA_Lines_HiramAfter[1] | Hiram | CUT | |
| DA_Lines_LiftTubeAfter[0] | Hiram | SHORT | Coyle here. Pull the bell. |
| DA_Lines_HaskettLift[0] | (a house) | SAME | (= cpp:TownLife town[16]) |
| DA_Lines_HobDeadLift[0] | Hob (tap) | CUT | |
| DeadLift.turn_in[0] | Hiram | KEEP | |
| DeadLift.turn_in[1] | Hiram | CUT | |
| DeadLift.turn_in[2] | Hiram | KEEP | |
| OdellReport.lines[0] | Hob | SHORT | A lawman with a hunch and no proof, sunshine. |
| cpp:OreLift:ClosedCold | (notice) | KEEP | |
| cpp:OreLift:ClosedSteam | (notice) | SHORT | No steam in her yet. Hiram needs her clamp pin first. |
| cpp:OreLift:HoldTheRail | Hiram | KEEP | |

### The Last Watch

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobLastWatch[0] | Hob (tap) | CUT | |
| RuthJournal.lines[0] | Hob | KEEP [ROW] | |
| CodeBook.lines[*] | Ellis, Hob | KEEP (1 row) | |
| PostLog.lines[*] | Ellis, Hob | KEEP (1 row) | |
| cpp:WatchFire:Dry | (notice) | KEEP | |
| LastWatch.turn_in[0] | Ellis | KEEP [ROW] | |
| LastWatch.turn_in[1] | Hob | KEEP | |
| LastWatch.turn_in[2] | Hob | CUT | (the cache in the cabin opens; the card says so) |

### Back Pay (FIGHT-LAYOUT's cave-in on the way out)

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobBackPayTags[0] | Hob (perch mine mouth) | KEEP | |
| DA_Lines_HobBackPay[0] | Hob (tap) | CUT | |
| PayrollBook.lines[0] | Hob | KEEP | |
| ChalkTally.lines[0] | Hob | KEEP | |
| Barricade.lines[0] | Ellis | KEEP | |
| Barricade.lines[1] | Hob | SHORT | A Foreman who never paid them. Listen. Digging. |
| cpp:ForemanFight:* | Teague | KEEP | |
| BackPay.turn_in[*] | the shift, Ellis | KEEP (1 row) | |
| BackPay.choice0[*] | the shift, Hob | KEEP | |
| BackPay.choice1[0] | the shift | KEEP | |
| BackPay.choice1[1] | Hob | SHORT | Can't spend it in the ground, sunshine. |
| DA_Lines_TagBoardKept[*] | the shift | KEEP | |

### The Rest's seeds, the roosts and the town

| Key | Who | Act | Words after |
|---|---|---|---|
| DA_Lines_HobIdle[*] | Hob (mutters) | KEEP | (Hob's feathers: seed, word for word) |
| LedgerPage_HighwaterLanding.lines[0] | Hob | SHORT | End of the line, where the Lily ties up. Purcell's luck lives there now. |
| LedgerPage_DustwaterMesa.lines[*] | Ellis, Hob | KEEP (1 row) | |
| LedgerPage_SaintAgnesMire.lines[0] | Hob | KEEP | |
| LedgerPage_FurnaceHollow.lines[*] | Ellis, Hob | KEEP | ("Note it for the lamp": seed) |
| LedgerPage_TheNeedles.lines[*] | Ellis, Hob | KEEP | (the spire: seed) |
| LedgerPage_RansomsRest.lines[*] | Ellis, Hob | KEEP (1 row) | |
| DeaconLetter_Cleft.lines[0] | Ellis | KEEP [ROW] | |
| DeaconLetter_Cleft.lines[1] | Hob | SHORT | Never sent. Left where you tied up, though. |
| DeaconLetter_Saloon.lines[*] | Ellis, Hob | KEEP | |
| ReachesMap.lines[0] | Hob | SHORT | Seven saints, one railroad. Every line runs to the Junction, sunshine. |
| cpp:RoostRules:Death0 | Hob (a death) | KEEP | |
| cpp:RoostRules:Death1 | Hob | KEEP [ROW] | |
| cpp:RoostRules:Death2 | Hob | CUT |  |
| cpp:RoostRules:Death3 | Hob | CUT | |
| cpp:RoostRules:Death4 | Hob | KEEP [ROW] | |
| cpp:RoostRules:Death5 | Hob | CUT |  |
| cpp:RoostRules:Death6 | Hob | KEEP | |
| cpp:RoostRules:Death7 | Hob | CUT | |
| cpp:RoostRules:Death8 | Hob | CUT | |
| cpp:RoostRules:Death9 | Hob | KEEP [ROW] | |
| cpp:RoostRules:Death10 | Hob | CUT | |
| cpp:RoostRules:Death11 | Hob | KEEP [ROW] | |
| cpp:RoostRules:FirstDeath | Hob | SHORT | Told you, sunshine. You can't stay dead with the debt unpaid. |
| cpp:RoostRules:FirstRoost | Hob | SHORT | One of my roosts. My crows carry you between them, and back to the last one when you drop. |
| cpp:RoostRules:FirstTravel | Hob | SHORT | Don't wriggle. My lads have never dropped anyone. |
| cpp:TownLife:town[0] | Pruitt house, Afraid (it has two) | CUT | |
| cpp:TownLife:town[3] | Pruitt house, GateWon (two) | CUT | |
| cpp:TownLife:town[6] | CottageNorth, Afraid (two) | CUT | |
| cpp:TownLife:town[11] | CottageSouth, Afraid (two) | CUT | |
| cpp:TownLife:town[new] | an old man, CottageNorth, GateWon (secret 4's hint) | NEW | Avery kept his savings up the chimney. Fire took the house, not the chimney. |

(Every other town-life line, 13 of 17 with 3 rows, is kept: one a household and stage. Roost deaths keep 6 of 12; the
switch in `RoostRules::DeathLine` and its count go with them.)

### The secrets' lines (section 3), all on pickup or reading

| Key | Who | Act | Words after |
|---|---|---|---|
| Secret.NedsStake[new] | Hob, on opening | NEW | Ned's collection box. He'll be through, sunshine. Not to collect. |
| Secret.CoffinRock[new] | Ellis, the bench mark | NEW | You can see the whole Rest from up here. |
| Secret.CoffinRock[new2] | Hob | NEW | Every map starts with somebody standing on a rock, sunshine. |
| Secret.Bottle[new] | Hob, on opening | NEW | Embalming fluid, by the label. Rye, by the smell. |
| Secret.KidsTin[new] | Ellis, the handbill (Ellis only: drop_for_others) | NEW | I was fifteen. |
| Secret.KidsTin[new2] | Hob | NEW | ...He did take you, sunshine. |
| Secret.Powder[new] | Ellis, the powder slip | NEW | Six sticks. The roof didn't give on its own. |
| Secret.Powder[new2] | Hob | NEW | Somebody tell Wade, sunshine. |
| Secret.Derrick[new] | Hob, on opening | NEW | The company never got that crate down. You did. |
| Secret.Pages[new] | Hob, on the vestry-step page | NEW | He drops pages like a dog drops hair. There'll be more. |

## 2. Fetch steps, and what replaces them

FIGHT-LAYOUT is enlarging every fight and adding the Main Street ambush (Main 3, after the gate), the Webwood drop (after
Main 4, optional) and the Providence cave-in (Back Pay, on the way out). Nothing below adds a fight of its own; two
places *ask* FIGHT-LAYOUT (or Main, after it) to re-arm an existing spawner, marked REQUEST.

| Mission, step today | Why it's a fetch | Replacement | Kind |
|---|---|---|---|
| Main 6 step 0, "Bring Pa's lantern bail from Tilly's window sill" | A player who walked past the sill at Main 3 crosses the valley back to town at dusk | Tilly hands the bail over at Main 3's turn-in (carried from then; Main 6's objective counts at once). The sill pickup stays as the fallback until code B lands | set-up (code B) |
| Main 7 step 0, home to Delia, then the turn-in at Tilly's | Point -> farm -> town -> depot | Kept: Heirloom is a beat, and the farm-to-depot walk is the way out. REQUEST (optional): re-arm Camp OrchardEdge (layout `camps`, after Main 3) during Main 7 at dusk, so Heirloom's first shots land on the farm road ("every dead thing for a mile saw that lantern lit"); its Hob line would be a set-piece-style beat in layout.json | fight (FIGHT-LAYOUT) |
| Side 1 step 0, tear down 6 of 7 posters | A town-wide chore | 4 of 7 (`create_side_mission_assets.py` count, `cpp:WantedPoster` RemarkSixth TornCount 6 -> 4, the poster tests); the note and Tilly's twist kept | trimmed chore |
| Calder's Caches step 0 | Three caches by riddle | Kept: riddle hunts are finds with clues (what the user asked for) | find |
| Old Fare steps 1-2, "Find what they were waiting on" + "Look at the coffin" | Two stops in one room | One step: "Find what they were waiting on: the coffin in the baggage room" (Interact FennCoffin; Fenn's FennLook words play) | merged |
| Old Fare steps 3-4, ask Sloane + take the coins under his door | Two stops at one door | One step: the talk and the coins together (FennsFare shown from that step, lying where he slides them) + optional the desk | merged |
| Old Fare step 5, carry the coins back down to Fenn | A walk back through an empty room | REQUEST: "Rob a dead man's eyes and every soul for a mile hears about it" (Hob's own line) pays off: a second DepotWaiting push (4 Unpaid + the area's ranks) rises in the waiting room as the coins come down the stair, `active_when during OldFare from the lay step`. A new spawner in `build_area_missionprops_valley.py` beside DEPOT_*, FIGHT-LAYOUT's file. Without it the step stays a walk | fight (FIGHT-LAYOUT) |
| Side 2 step 0, six hay bales held one by one | A chore | 3 bales (any 3 of the 6 placed; bale remarks on 1, 2, 3) | trimmed chore |
| Side 2 step 1, walk back to Amos to "ask who's on the road" | A talk that stops the action | Cut: the hands come up the road as the last bale goes in, and Amos's twist is said over the start of the fight (AmosBaleLast) | fight start |
| Knocking Below steps 0-1, reach the workshop door, then find the key | Two stops at one door | One step: "Take the cellar key off Thaddeus's measuring hook, in the workshop off the yard" (the arrow on the hook) | merged |
| Knocking Below steps 5-6, talk to Thaddeus, then take the tape | Two stops at one bench | One step: talk + the tape (shown from that step, on his bench) | merged |
| Dead Lift step 0, the winch crank from the sheriff's lockup | A walk to Main Street and back | Cut: the lift's crank never left (Hiram's line drops it). Odell's report stays in the lockup as a free read (secret 8's clue) | removed |
| Dead Lift step 0, a coal sack from the depot's coal stage | A walk to the depot and back | Cut: the boiler is stoked from the quarry's own coal bin (the Boiler fitting already shows SM_CoalSack at the winch house's CoalBin) | removed |
| Dead Lift step 0, the clamp pin from Abe Haskett | Already a fight (QuarryCrew) | Kept: now the step's only objective | fight |
| Dead Lift step 1, three fittings | Holds | Two (the boiler, the pin); LiftFitting_Crank removed from the mission and from `build_area_missionprops_mountain.py`'s tuple | trimmed |
| The Last Watch step 1, oil in Ruth's cabin | Next door to the fire | Kept (no fetch: it's in the same post, and AWatchFire's LightStep 2 / AnswerStep 4 would move) | — |

The new step lists (counted from 0; every code-locked step keeps its index):
- **Side 1:** unchanged (0 posters 4/7, 1 the note + optional daybook).
- **Old Fare:** 0 clear the waiting room (DepotWaiting, `before_step 1` unchanged); 1 the coffin in the baggage room
  (FennLook: Fenn's topic `from_step 1`); 2 Sloane's talk + Fenn's coins (FennsFare `shown_when from_step 2`; AgentCoins
  topic `from_step 2`) + optional the desk; 3 lay the coins (FennCoins topic `from_step 3`). The valley script finds
  these steps from the asset by their objectives (its docstring), so it needs no edit.
- **Side 2:** 0 load 3 bales; 1 drive off the hands (WhitlockHands; `+ optional` the wages chalk); turn-in Amos (thanks
  `from_step 2`). `build_area_whitlock.py`: `FIGHT_STEP 2 -> 1`, `THANKS_STEP 3 -> 2`, the remarks on bales 1, 2, 3
  (`remark(build, 2, 'DA_Lines_HobBaleThird')`, `remark(build, 3, 'DA_Lines_AmosBaleLast')`), hands `active_when
  from_step 1`, *and* the hands start as the third bale lands (they're on from step 1, which begins then).
- **Knocking Below:** 0 the key (CellarKey); 1 down into the cellar + optional the slate; 2 the cellar's dead
  (TillyCellar `from_step 1, before_step 3`); 3 the cold room; 4 Thaddeus + his tape (ThaddeusTalk `from_step 4`,
  ThaddeusTape `shown_when from_step 4`).
- **Dead Lift:** 0 the clamp pin (QuarryCrew/AbeHaskett `before_step 1` unchanged); 1 the boiler + the pin (fittings);
  2 hold the quarry (RaisingSteam `from_step 2, before_step 3` unchanged); 3 the ride (**stays 3**: AOreLift::OpensAtStep).
  Hiram's topics: Fit `from_step 1`, Steam 2, Ride 3 (unchanged numbers). Items WinchCrank, CoalSack retired: their ids
  are never reused (saves may hold them; the runner drops unknown carried items? check `MissionRunnerItems.cpp`).
- **Main 6, Main 3:** step lists unchanged.

**Saves mid-mission** (the user plays sessions): Old Fare, Side 2, Knocking Below and Dead Lift change their steps, so
an old save on one of them lands on the wrong step. Bump the save version (3 -> 4) and map old steps onto new in
`Session/CampaignStepMigration.cpp` as the 2026-10-13 move did (Old Fare 1->1, 2->1, 3->2, 4->2, 5->3; Side 2 1->1
(the talk) , 2->1, 3->2; Knocking 0->0, 1->0, 2->1, 3->2, 4->3, 5->4, 6->4; Dead Lift unchanged indices), with a test in
`Tests/SessionStepMigrationTests.cpp`.

## 3. Things to find off the path (10)

Each is a one-time find in a place no mission sends the player, hinted by a line or a sight, rewarding with loot and
usually a story nugget. Each sets `Secret.<Id>` (an `AReadableProp.SetsFlag` or the chest's opening), so a later
counter can count them. No XP (the pacing test is FIGHT-LAYOUT's; secrets are loot and lore). Coordinates are
layout.json cm (X north, Y east); every spot must pass `build_area_caches.py`'s level-and-clear spot rules, and the
climbs need `Tools/Unreal/reach_probe.py` / a PIE walk to prove them.

| # | Secret | Where | What it gives | How it's hinted |
|---|---|---|---|---|
| 1 | **Ned's Stake** (the legendary chance) | Under the middle of the Three Widows (widow2Rock, ~[400, -7700]), a Strongbox wedged at its foot on the side away from the keeper's path, its lid under a slab | Strongbox, 2 guns at Luck 1.3 (the Gravemother's odds, ~21% a legendary), ammo; `Secret.NedsStake`; Hob's line on opening (section 1, last table) | The back of Ned's marker in the agent's desk (Old Fare's optional readable): page text gains "On the back, in pencil: PAY UNDER THE MIDDLE WIDOW. I COLLECT ON MY WAY OUT. N.P." + Ellis "The middle widow. On the keeper's path."; an "N.P." scratched in the rock (a small decal) |
| 2 | **The gang's mooring** | The Mooring Ledge under Ransom's Point, down the cliff stairs (mooringPost [-10000, -10100]); the Deacon's letter and the Mire's Ledger page already lie there | A Footlocker (the skiff's locker: ammo, a fair gun chance); with the two seeds already there; `Secret.Mooring` (the locker) | Hob at Main 2's turn-in: "South-face stairs go down to where the gang tied up, sunshine." |
| 3 | **Coffin Rock's top** (a vista) | The 8 m butte's top (coffinRock, around coffinRockTree [-6000, -1500]); a route of three stepped boulders up its north-east face, each in mantle reach (a terrain/dressing add) | A surveyor's tin (Footlocker) and a brass bench mark readable: "REACHES RAILROAD SURVEY. BENCH MARK 112. W. PIKE, SURVEYOR." Ellis's and Hob's lines (section 1, last table; Pike played: his own `heroes` variant, "My mark. I stood here before the rails came."); `Secret.CoffinRock` | A tattered survey flag on the dead tree, seen from the orchard road and the farm; the boulder steps read as a route |
| 4 | **The Averys' chimney** | The Burnt Homestead's standing chimney on the west road (burntHomestead [4900, -7200]): a loose hearthstone at the chimney's foot | A Footlocker-kind "tin" in the hearth: a gun (Uncommon floor) and silver; a readable slip: "FOR MARY'S SCHOOLING. DON'T TOUCH IT, TOM." `Secret.Chimney` | Town-life mutter (CottageNorth, after the gate fight): "Avery kept his savings up the chimney. Fire took the house, not the chimney." (Avery is already on Thaddeus's coffin slate) |
| 5 | **Thaddeus's bottle** | The roofless springhouse over Mill Creek's spring (springhouse [-1600, 8400]), under a flat stone in the water's edge | A Footlocker: a bottle (no effect), a Rare-floor gun from the undertaker's younger days, ammo; `Secret.Bottle`; Hob's line (section 1) | Thaddeus after Knocking Below: "My bottle's under the flat stone in the springhouse. Don't tell her." |
| 6 | **The first keeper's box** | The roofless old lantern house by the keeper's grave (lanternHouse [3500, -10200]), on its back wall's lantern hook | A readable, Silas Ransom's note (first keeper): "Winter, the first year. No coin in the valley for the fare. Ada said she'd go ahead and hold a light for us. She lay down on the board at dusk and the wind took her west. I lit this lantern from her that night. It's never gone out. S.R." + a Footlocker of keeper's oil and ammo; `Secret.Silas` | The keepers' roll names SILAS first (Main 4's optional read); Hob's Main 6 perch is beside it at the keeper's grave; the hook's old lantern glints (an emissive bead) |
| 7 | **The tin under Keeper's Bridge** | Under the deck of Keeper's Bridge over the Dry Wash (keepersBridge, ~(-95, 2) m), on the wash's floor | A child's tin: a slingshot (no effect), a mailbox-sized ammo pickup and a gun chance; a readable: a folded, ten-year-old WANTED: AMBROSE DUNNE, THE DEACON handbill with "TAKE ME WITH YOU" pencilled on it; Ellis's and Hob's lines (section 1; a row: other heroes find "a child's tin", the handbill without the pencil, and no Ellis line); `Secret.KidsTin` | Abel after Main 6 (his board): "You hid under Keeper's Bridge when you'd done wrong, El. Your tin's likely still there." (row: neutral "Some child used to hide under Keeper's Bridge. Their tin's likely still there.") |
| 8 | **Teague's powder box** (mountain) | Larkspur Bench, behind benchGraniteG's rock (~[20200, -7800]), a padlocked company box half under a slab | Rocket ammo when the Powderkeg is in (12.3), else ammo, and a gun at Luck 1.0; a readable: "POWDER DRAWN. 6 STICKS, 9 JUNE. FOR THE LOWER DRIFT. C.T." (the proof Odell wanted); Ellis's and Hob's lines (section 1); `Secret.Powder` | Odell's report (the lockup: "powder burns on his cuffs"), the Foreman's payroll ("A small charge"), and a trail of three split powder kegs (dressing) from the Diggings' yard toward the rock |
| 9 | **The derrick's crate** (a vista) | The Old Quarry's ruined derrick (quarryDerrick [9500, 1900]): a crate hanging from its boom, reached by a climb over the quarry blocks onto the derrick's platform | The crate (a Supply-Crate-sized Footlocker drop: a gun, ammo) and the view over the valley; `Secret.Derrick`; Hob's line (section 1) | The crate swings on its rope, seen from the quarry track and the winch house; Hiram after Dead Lift (HiramAfter gains nothing: the sight is the hint) |
| 10 | **The loose pages** (the collection that already exists) | The six Ledger pages and two Deacon letters as placed by the seeds (lockup, baggage room, vestry step, Calder's Post, tag board, mooring ledge; saloon rack, mooring post) | Lore (the Ledger's town pages open, as today) | Hob, the first time any page is read (one line, a new `ReadableProp` "first of its kind" isn't needed: put it on the vestry-step page, the one on the main path) (section 1, last table) |

Placement: a new script `Tools/Unreal/build_area_secrets.py` (its own outliner folder `RansomsRest/Gameplay/Secrets`,
hooked in `build_area.py gameplay()` after `build_area_seeds.py`, alone: `build_area_secrets.py RansomsRest`), its
words and spots in `story_texts.py` (a new SECRETS dict: readables through `apply_readable`, chests as `AChest` with
`LootOverride`). Coffin Rock's steps, the derrick's platform reach and the powder kegs are dressing: the 12.5 VFX/world
variety agent's lane, or the secrets script places them as props tagged Obstacle. Models: the bench mark disc, the
child's tin, the bottle, the powder box are small MissionProps.py additions (Art lane); stand-ins until then.

Optional (Main's call: it's a small new piece, the freeze allows RR fun work): a "SECRET FOUND 3/10" notice and sting
when a `Secret.*` flag is first set, and the count on the Missions page. Without it the finds still work.

## 4. What code and data each change needs (for splitting phase 2)

**Clash check with FIGHT-LAYOUT** (its contract: layout.json fights/camps/patrols/ambushes, the build_area_* spawner
lines, EncounterSettings/AmbushSpawner/EncounterSpawner/PatrolSpawner/SetPiece*, RansomsRest.md fight tables,
Progression.md, the pacing tests): it may also trim mission XP (`rewards=dict(experience=...)`) in both mission scripts
to hold the story-alone level, and `story_texts.py` ENCOUNTERS names its spawners' windows. So STORY-DATA edits only the
offers, turn-ins, briefings, summaries, steps and items in the mission scripts (never `rewards`), re-reads both files
right before editing, and touches ENCOUNTERS only for TillyCellar's window (tell FIGHT-LAYOUT/Main).

**Lane STORY-DATA (no editor, no build; safe beside FIGHT-LAYOUT with the above):**
- `Tools/Unreal/create_story_lines.py`: every DA_Lines_* change in section 1; remove `DA_Lines_AgentWindowCoal`,
  `DA_Lines_HobMain6Bridge` (and delete their assets in the editor run); AmosBaleLast grows to five lines (AmosRoad's three moved onto it; AmosRoad keeps them as his tap).
- `Tools/Unreal/create_mission_assets.py`: Main 1-7 offers/turn-ins/briefings (section 1); Main 2's turn-in gains Hob's
  hint; Main 3's turn-in line [6] ("Here." once code B is in).
- `Tools/Unreal/create_side_mission_assets.py`: the side offers/turn-ins; the new step lists (section 2) for Side 1 (count
  4), Old Fare, Side 2, Knocking Below, Dead Lift (items WinchCrank and CoalSack dropped from the mission; `expect` lists
  updated); briefings trimmed to the new steps (Dead Lift: "Get the lift's clamp pin back from Abe Haskett, stoke her
  boiler from the quarry's coal, and Hiram will take you up the mountain."; Side 2 gains the twist in one sentence:
  "He owed his hired hands for that harvest, and never paid.").
- `Tools/Unreal/story_texts.py`: READABLES' lines (section 1), AgentDesk's page text (secret 1), TOPICS step numbers (Old
  Fare, Amos, Thaddeus), ENCOUNTERS (TillyCellar), ITEMS shown_when (FennsFare 2, ThaddeusTape 4), drop
  Speaker_AgentWindow's coal topic; new SECRETS dict (section 3).
- `Tools/Unreal/hero_lines.json`: every row marked [ROW] in section 1. Deleted rows (their line is gone or now
  neutral): HobMain5Way.0, HobMain5Lantern.0, HobMain5.0, HobMain6Way.0, HobMain6Bridge.0, HobMain7Lean.1,
  AldanaWaiting.3, AldanaBell.0, AldanaAfterMain4.0, AldanaAfterMain5.0, AmosRoad.3, AmosAfterMain6.0, ThaddeusAfter.1,
  mission.Main4.offer.0, mission.Main5.offer.1, mission.Main5.turnin.0, side.Knocking.offer.0 ("Pa's lantern. Where
  is it?" stays: Main 2's turn-in keys it). Rewritten (current + neutral, heroes
  where present): lines.SextonDeal.3 (neutral only), readable.HeadboardEllis line1, mission.Main5.turnin.3, mission.Main7.offer.1, lines.AmosMeet.2 (+
  Gauge; and its side.Side2 twin), lines.AbelOnBoard.1 (now the tin hint). New: the secret 7 lines (Ellis's tin,
  "I was fifteen." drop_for_others), secret 3's Pike `heroes`. Then `create_hero_lines.py` refuses any slip (a
  duplicate current, a lost token).
- `story_texts.py` TOPICS['Speaker_Hob']: only `during Main1 -> HobMain1` and `after Main1 -> HobAfterMain1` stay (his
  taps); the fourteen tap sets go (HobMain2-7, HobSide1, HobCaches, HobOldFare, HobKnocking, HobSide3, HobDeadLift,
  HobLastWatch, HobBackPay).
- **Retiring a set safely:** a placed actor still points at its asset until the gameplay build re-places it, and
  deleting a referenced asset opens a modal force-delete dialog (it blocks MCP). So: drop the name from the data, rebuild
  gameplay, then delete the asset. Until an asset is gone its hero row stays (NoFamily scans every set asset; HobMain5,
  HobMain5Lantern, HobMain6Bridge carry "Pa").
- `Tools/Unreal/build_area_secrets.py` (new) and its one-line hook in `Tools/Unreal/build_area.py`.

**Lane STORY-CODE (C++, needs the build lane; no FIGHT-LAYOUT file):**
- A. Text-only C++: `World/WantedPoster.cpp` (NoteCaches and NoteTake cut, RemarkFirst, RemarkSixth and NoteHob words,
  RemarkSixth's TornCount 6 -> 4), `World/RoostRules.cpp` (FirstDeath, FirstRoost, FirstTravel words; Death2, 3, 5, 7, 8
  and 10 cut, so `DeathLine`'s switch and its count shrink to six), `World/OreLift.cpp` (ClosedSteam), `World/TownLifeRules.cpp` (four second-lines cut, the Avery line added; the
  table allows two lines a stage, as Pruitt's Afraid has). Tests that pin these words: `Tests/PosterTests.cpp`,
  `PosterOfferTests.cpp`, `OreLiftTests.cpp`, `TownLifeTests.cpp`, the roost tests.
- B. The bail at Main 3's turn-in: `FMissionTurnIn::HandItems` (ids of a later mission's items carried from the
  turn-in; `Missions/MissionDefinition.h`, `MissionRunnerTurnIn.cpp`, `MissionRunnerItems.cpp`), the turn-in helper in
  both mission scripts takes `hand_items=`; tests in `Tests/ColdWelcomeTests.cpp` / `GravewindTests.cpp`.
- C. Save migration v3 -> v4 for the four reworked side missions: `Session/CampaignStepMigration.cpp/.h`,
  `Session/SessionSave.h` (version), `Tests/SessionStepMigrationTests.cpp`.
- D. (optional) the secrets notice: a small `World/SecretsSubsystem.h/.cpp` reading `Secret.*` flags, the notice
  through the HUD's existing notice line, the Missions page count; `Looter.Secrets.*` tests.
- Mission tests whose step lists move: `Tests/MissionTests.cpp` (Definitions), `UnfinishedBusinessTests.cpp`,
  `OreLiftTests.cpp`/`OreLiftRideTests.cpp` (OpensAtStep stays 3), `EconomyTests.cpp` (Old Fare/Dead Lift rewards and
  the mission silver: Dead Lift's purse kept), `SevenDaysTests.cpp`, `TalkBusinessTests.cpp`, `ChapelTests.cpp`,
  `KeepersLanternTests.cpp`, `LanternLeansTests.cpp`, `HallowedGroundTests.cpp` (any that pin a line's words); the Lines
  tests (`Looter.Lines.*`).

**After FIGHT-LAYOUT has finished (its files; small, mechanical):**
- `Tools/Unreal/build_area_whitlock.py`: FIGHT_STEP 1, THANKS_STEP 2, the remarks on bales 1/2/3, the hands on from
  step 1 (section 2).
- `Tools/Unreal/build_area_story.py place_hob()`: the cut perch arrivals' names out (HobMain2Climb, HobMain4Road,
  HobMain4Aldana, HobMain4Bell, HobMain5Sacs, HobMain5Lantern, HobMain6Post, HobMain7Depot, HobMain7Board): Hob still
  flies to each spot, silent. Until then a missing set only warns at build (`lines()` returns None) and he lands silent.
- `Tools/Unreal/build_area_missionprops_mountain.py`: drop `('Crank', ...)` from `place_fittings`; the crank and coal
  sack pickups (wherever ITEMS places WinchCrank/CoalSack: the lockup and the coal stage) not placed.
- `Tools/Unreal/build_area_missionprops_valley.py`: its fallback LINES/READABLES in step with section 1 (or deleted);
  the REQUEST second DepotWaiting push (section 2) if Main takes it.
- `Art/Levels/RansomsRest/layout.json` `camps`: the REQUEST OrchardEdge at dusk in Main 7, if Main takes it.

**Run order (Main, editor open, after the C++ builds):** `create_story_lines.py` -> delete the two retired sets' assets ->
`create_mission_assets.py` -> `create_side_mission_assets.py` -> `create_hero_lines.py` -> `build_area.py RansomsRest
gameplay` (readables copy their words when placed; FIGHT-LAYOUT's spawners are re-placed by the same run, so run it once
both are merged) -> tests `Looter.Lines`, `Looter.Missions`, `Looter.Story`, `Looter.Seeds`, and the changed missions'
groups -> a PIE play of Main 3, Side 2, Old Fare, Knocking Below, Dead Lift and two secrets.

## 5. Beat ledger: every beat, character and seed, and where it lands now

- Cold open, the spark, the grave, both epitaphs, frost in August, "Morning, sunshine": unchanged.
- Delia won't open the door; the plate and the salt; Abel got up Wednesday; a keeper doesn't lie still; eat the plate
  (its "I'll know if you don't" cut): Main 1 turn-in.
  Abel walks the boards at dusk: Main 6's offer (kept).
- Sexton's deal, every term (the rail, "I attend every death", seven embers at the new moon, a little of you back,
  your rest, your father's fare, the Gravewind for ever, ask a keeper, "So are you, friend", the ledger): kept, one
  line shortened and its repeated ledger line cut.
  He knows where the lantern fell: Main 2 turn-in (kept).
- Pa ran out with the lantern lit; Tilly laid him out: Main 3 offer. Tilly laid them both out, the collar, the bail in
  his fist (the effects box now just drops): Main 3 turn-in. A keeper is buried with his lantern: Main 7 (Delia, kept).
- Aldana: Abel's child through the door, the bell, the Reliquary, a lantern finds an ember, the keeper shot first and the
  lantern smashed, Pa's fell whole, spiders hoard, the Sink: Main 4 (kept). The candle won't take; lit from the saint;
  Delia will know: Main 5 turn-in. Ada lay down on the first board, no saint crosses: AldanaAda (kept, shorter). The toll
  mark under the lid: kept. The keepers' roll with ELLIS painted out: kept.
- Every soul on the Sundown Road drifted home (the Unpaid): now heard on the way into the chapel yard (it was a tap line).
- Main 6: show him the way, he won't know you (cut: "never a priest's to do"); the keeper's grave and the cairns; Abel's three
  phases; Sit with Pa (Ned shot him, the Deacon wept and kept her, the tall fella on the rail, bring Saint Ada home, she
  leans for the nearest light); the second light at dusk ("Don't look at that one"): all kept.
- Main 7: she leans north-east to Ned; Delia kept something back; eat the plate; Heirloom, "Hold the door."; "Thank you";
  Father's car, the brass; the line to the Lily, Highwater Landing, Halfway Junction: all kept.
- Side 1: the posters, Calder's signature and note, Tilly wrote ALREADY: kept. Calder's riddles, the Deacon's slip ("Kid's
  share"), "She'll still hang you", her fourth cache up the lift: kept.
- Old Fare: the dead waiting for a train, grave gold on Fenn's eyes, Sloane borrowed it against a debt, Ned's marker, the
  coins laid back, Fenn forgives him, the pistol: kept; the old-fare lore (gold on the eyes paid the ferry) moves from
  Hob's tap into his line at the coffin.
- Amos: sees Ellis, Abel's young'un, turned back on the Sundown Road, the hay, the Merritt brothers and Sol Tanner unpaid,
  "pay them on the far side", waiting for the saint (AmosFence), the light on the Point: kept (the twist now over the
  fight).
- Thaddeus: measured you, a cellar wants keeping, the tape, the brass; Tilly's "He never once came up for supper": kept.
- The Odells: the dog and the mule, the hat, the badge, the daybook, Jory in the eleven, Teague's powder burns (now with
  proof in secret 8), Mrs. Pruitt's brother and his hat: kept.
- Hiram: four years by a cold boiler, the fall, Abe's pin, the crew, "owe him for coal", the Haskett house hearing the
  lift: kept. **Changed: the crank and the depot's coal are gone** (not beats; the crank's link to Odell's report goes, the
  report stays as a find).
- Ruth: her journal (saw Ellis draw, closed their eyes), the code ("the Ransom kid walks"), the post's log: kept.
- Back Pay: the tag, the payroll, break the strike, the small charge, the chalk, "PAY US AND WE COME UP", the Foreman, the
  choice and both endings: kept.
- Seeds: the six Ledger pages and their margin notes, both Deacon letters, the map, the Seven poster, the road at dusk,
  Ada's story, Hob's three feathers, the spark, Fenn's fare: all kept word for word but two Hob after-lines trimmed.
  New seeds (secrets): Ned's collection box (the Lily), Pike's bench mark (a hero), Silas lighting the lantern from Ada
  (the keepers), the Deacon handbill (Ellis ran to him), Teague's powder (Back Pay).

**Risks to a beat:** (1) Fenn's fare "exactly as it is": its words are trimmed though every fact stays (Main to confirm
or keep AgentCoins whole, +20 words). (2) Main 3's Ellis question to Delia is cut; Delia now volunteers the answer.
(3) Amos's twist now plays over a fight: a player busy shooting may miss it; it's also his tap during the fight
(AmosRoad, SAME) and the card's briefing should carry one sentence of it. (4) The secrets 6 and 7 add canon (Silas
lighting the lantern from Ada; Ellis at fifteen wanting to ride with the Deacon): both fit Story.md (the lantern lit from
the saint; Ellis ran at sixteen) but are new facts for the user's review. (5) Secret 3 uses Pike's name as a nugget
(the map already does).

## Retired line sets (delete after the rebuild, never before: a force-delete modal blocks MCP)
/Game/Data/Story/DA_Lines_: HobMain2Climb, HobMain2, HobMain3, HobMain4Road, HobMain4Bell, HobMain4Aldana, HobMain4,
HobMain5Sacs, HobMain5Lantern, HobMain5, HobMain6Bridge, HobMain6Post, HobMain6, HobMain7Depot, HobMain7Board, HobMain7,
HobSide1, HobCaches, AgentWindowCoal, HobKnocking, HobSide3, HobDeadLift, HobLastWatch, HobBackPay, AldanaAda.
