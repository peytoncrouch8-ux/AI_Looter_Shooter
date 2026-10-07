# Ransom's Rest: the bundled review

Everything in Ransom's Rest (`Docs/Areas/RansomsRest.md`) that waits for your decision, in one place, as of 2026-10-08.
Everything here is built and in the game. Each item says what to look at and what happens if you say nothing.

## Approvals waiting

| Step | What to approve | Where to see it |
|---|---|---|
| 18 | Main 4, "Hallowed Ground": the chapel yard's two waves, the bell, the smashed Reliquary (the backlog design) and Aldana's door | A session past Main 3, or `Looter.Mission.Start Main4` |
| 19 | Main 5, "The Keeper's Lantern": the Sink, its egg sacs and webs, the lantern in its snare | `Looter.Mission.Start Main5`; tour views SinkRim, SinkFloor |
| 20 | Side 3, the Gravemother's fight (her den is now dressed with bones, cocoons and her larder) | `Looter.Legendary.Return Gravemother`; tour view GravemotherDen |
| 22 | Abel, the Keeper: the fight's difficulty (about 10,500 health at level 9, three phases, 3-4 minutes) and the ending's wording (the scene's lines below) | `Looter.Abel.Fight here`; `Looter.Abel.Scene` |
| 24 | The train and the depot; the area's ending, played from Abel to the station board; a practice trip from the depot to Skyreach and back | `Looter.Mission.Start Main7`; `Looter.Train.Depart` / `.Arrive` |
| 25 | Amos's mission: the bales, his hired hands, his thanks | A session past Main 4; tour view WhitlockFields |
| 27 | Ransom's Rest as done, after a new session from the tutorial to the station board at the depot and a practice trip to Skyreach and back. Then: does the *Gilded Lily* stay an airship? | A new session |

## Running on the recommended defaults (say if you want any changed)

- Step 6: legendary odds per rank 0.3%, 2.2%, 11.8%, 21.4%, 30.3%; ranked enemies drop guns more often than your 20-40%
  rule (Rare 60%, Epic and up every kill; Basic 30%); Restless blue, Gravebound purple, Soulfed orange, a little bigger.
- Step 7: enemy levels follow the player inside each band; kill XP grows 8% a level; promotions 8% and 2%; +8% max health
  a level. Measured with the level as built: the main path ends at level 9.7, Sides 1 and 3 on top bring it to 10.1.
- Step 8: a 20-minute cooldown on rerolled promotions and on the Legendary monster.
- Step 10: the first cast-off's ride (12 s), practice trips as plain fades, Skyreach's creatures dropping only ammo on
  return visits.
- The lookout tower stays at (-100, -86), hidden from the grave by the bluff's edge (the other spot: (-106, -66), where the
  grave sees it).
- Skyreach is named in the game outside the story: the tutorial's mission title ("Welcome to Skyreach"), the main menu
  and the board's "Skyreach (practice)". The story itself never names it. Say if the tutorial and menu should drop the
  name too.

## Waiting for your OK

- Step 26, the Ranger caches: the Supply Crate and Strongbox out of the backlog, three caches and the sheriff's
  Strongbox.

## Drafted lines

Mains 1 to 3 are approved as written. These are from Main 4 on. Unless a note says otherwise, every set is a first draft;
the doc's own words are marked. Each can be changed in `Tools/Unreal/create_story_lines.py`.

### Abel's fight and the scene after (in the C++: `Bosses/AbelRules.cpp`, `Scenes/SitWithPa.cpp`)

- Abel, phase 1: "You brought them here, El. Right to our door." (a draft)
- Abel, phase 2: "Hear her ring? Ringing for a saint that isn't there." (a draft)
- Caption: "[The chapel bell tolls across the valley.]"
- Abel, phase 3: "Let me go, El. The wind's come for me." (a draft)
- Hob, when the player falls off the end: "Caught you. Keep off the end, sunshine. The wind's his, not yours." (a draft)
- The scene: Abel "...El? You came home." (the doc's); Ellis "Pa."; Abel "Ned Purcell shot me. I never got the lantern
  up."; "The Deacon wept over you, El. Wept, and kept her all the same."; "And there was a tall fella on the rail who
  never lifted a finger."; "Bring Saint Ada home, El. I'll wait." (the doc's); "There. She leans for the nearest light
  still burning. Follow her."; Hob "Well. I've seen worse reunions." (the doc's). The rest are drafts.

### Main 4, "Hallowed Ground"

**HobMain4Road** (a draft)

- Hob: "Up the north road to the chapel, sunshine. Saint Ada's yard is full of folks who didn't stay put. You'll fit right in."

**HobMain4Yard** (a draft)

- Hob: "Years of the Rest's dead, come home to their own graves. They don't take to company, sunshine."

**HobMain4Bell** (a draft)

- Hob: "Quiet. Bell rope's inside the door. That bell used to call the dead to rest. Give it a pull, see who answers."

**HobMain4Reliquary** (a draft)

- Hob: "Nobody answered. Bell's fine, sunshine. It's the saint that's out. Go look at what's left of her."

**HobMain4Aldana** (a draft)

- Hob: "Somebody's breathing behind the vestry door. Living, by the sound of it. Knock nice."

**HobMain4** (a draft)

- : "No light, no road. When the saint went out, every soul still on the Sundown Road drifted home. That's the whole yard, sunshine."

**AldanaBarred** (a draft)

- : "Stay back from that door. I've no comfort left for the dead, and the yard is full of them."

**AldanaWaiting** (a draft)

- : "They're quiet. Ring her bell, then look at what was done to her. Then we'll talk."

**AldanaMain4** (the doc's words from "A keeper's lantern can find an ember." to "Look in the Sink."; his first line and Ellis's question are drafts)

- : "You rang her bell, and she didn't answer. Now you've seen why."
- Ellis: "How do I find them?"
- : "A keeper's lantern can find an ember."
- : "In every town they robbed, the gang shot the keeper first and smashed his lantern."
- : "Your father's fell in the dark, whole."
- : "Spiders hoard anything a saint has touched."
- : "Look in the Sink."

**AldanaAfterMain4** (a draft)

- : "The Sink, Ellis. Your father's lantern. I'll pray it's still whole."

**AldanaAfterMain5** (a draft)

- : "You found it. Dark, but whole."
- : "Take it home to your grandmother, Ellis. She'll know what's owed a keeper."

### Main 5, "The Keeper's Lantern"

**HobMain5Way** (a draft around the doc's line about the keepers walking the dead by lantern light)

- Hob: "Keepers walk the dead to the boards by lantern light. Your Pa's is down there, sunshine, somewhere under the webs. Ramp's on the west side, by the gap in the fence."
- Hob: "And whatever dug the hole under me, let's not wake it."

**HobMain5Sacs** (a draft)

- Hob: "Egg sacs. Three of them, fat and twitching. Shoot them down before they drop on their own, sunshine. Two to a sac, by the look."

**HobMain5Lantern** (a draft)

- Hob: "That's the last of them. There, in the webbing by the north wall. Brass and glass. That's your Pa's."

**HobMain5Out** (a draft)

- Hob: "Dark as a shut eye, but whole. Up the ramp, sunshine."
- Hob: "And don't stare at the hole in the east wall too long. Something in there stares back."

**HobMain5** (a draft)

- : "Spiders hoard anything a saint has touched. Your Pa's lantern was lit from her every night he kept her. To them it smells like supper, sunshine."

### Main 6, "The Gravewind" (the deck and after)

**DeliaMain6** (the doc's words)

- : "Take him the lantern. Show him the way, even if he can't go."

**HobMain6Way** (a draft)

- Hob: "Keeper's grave. Every Ransom who ever sat up with the dead is under that board, sunshine. Your Pa's the first one who won't stay there."
- Hob: "Follow the cairns to the gate. He'll be on the boards. It's dusk."

**HobMain6Post** (a draft)

- Hob: "That's the keeper's post, by the steps. Hang it where he'd look for it."

**HobMain6Fight** (a draft)

- Hob: "He doesn't know you, sunshine. Not yet. And mind the open end: the wind's his."

**HobMain6** (a draft)

- : "The Gravewind comes off the Rim at dusk and takes the dead west. It's come for him every night this week, sunshine. He keeps walking back."

**AbelOnBoard** (a draft)

- : "Sun goes down the same every night, El. I never once got tired of it."
- : "Your grandmother still setting a plate? Eat it, even if you can't taste it. She needs to see it gone."

**AbelAfterMain7** (a draft)

- : "She still leans north-east. I'd know that lean anywhere."
- : "Go on, El. I'll keep the boards."

### Main 7, "The Lantern Leans"

**DeliaMain7** (the doc's words, word for word)

- : "A keeper's buried with his lantern, not his iron."
- : "His lantern wasn't on him, so I kept this back."
- : "He'd want you to have it."
- : "Hold the door."

**DeliaMain7After** (a draft)

- : "Go on, now. Tilly's got her father's car waiting at the depot."
- : "I'll keep setting a plate."

**HobMain7Lean** (the first line is the doc's; the second is a draft)

- Hob: "That's Purcell. The Lily's moored out that way."
- Hob: "Go home first, sunshine. Your grandmother's been keeping something back."

**HobMain7Depot** (a draft)

- Hob: "Tilly's Pa's car. The living won't share a carriage with a corpse, so you ride with the coffins. First class, sunshine."

**HobMain7Board** (a draft)

- Hob: "Board's by the depot door. Let's see if the railroad agrees with the lantern."

**HobMain7** (a draft)

- : "North-east, over the ridges. Purcell's out there with a deck of cards and an ember that isn't his, sunshine."

**TillyMain7** (a draft)

- : "Father's car is coupled up at the platform. The living won't share a carriage with you, so you'll ride with the coffins."
- : "Mind the brass. I polished it for him, and he never once complained."

**TillyAfterMain7** (a draft)

- : "The car's yours whenever the line runs. Wipe your boots."

### Side 2, "Unfinished Business"

**AmosMeet** (a draft)

- : "Well, now. You're looking right at me. Folks in town look clean through me, like a window."
- Ellis: "Amos Whitlock?"
- : "What's left of him. And you're Abel's young'un. I heard the shooting from the Sundown Road the night the saint went out. I'm sorry for it."
- : "I died last harvest with the hay half in. Got turned back on the road and came home to find it rotting where I left it."
- : "My hands go through a bale like it was smoke. Would you load it for me? Six bales, stacked under the hoist by the big doors."

**AmosBales** (a draft)

- : "Six bales, under the hoist by the big doors. I'd do it myself if my hands would hold."

**AmosHands** (a draft)

- : "Those are my hired men in the yard. The fever took them the winter before I went, and they came home hungry."
- : "They were good boys once. Drive them off before they get at the hay."

**AmosThanks** (a draft)

- : "Gone, are they? Then the hay's in, and it'll keep."
- : "They buried me with a gun I never once fired. It's yours. Better in a walking hand than a buried one."
- : "I'll sit a while. The saint'll come home. I'd like to be here when she does."

**AmosFence** (a draft)

- : "Still waiting on her. The hay keeps. So do I."

**AmosAfterMain6** (a draft)

- : "Saw a light out on Gravewind Point at dusk. Your Pa's, I'd wager. Good to know a keeper's still keeping."

**AmosBaleFirst** (a draft)

- Amos Whitlock: "That's one. Mind your back. Mine's past minding."

**HobBaleThird** (a draft)

- Hob: "Hauling hay for a ghost. Your Pa would laugh, sunshine. Then he'd hand you another."

**AmosBaleLast** (a draft)

- Amos Whitlock: "That's the last of it. Thank you, Ellis."
- Amos Whitlock: "Now mind the yard. My old hired hands are about, and they've come home angry."
