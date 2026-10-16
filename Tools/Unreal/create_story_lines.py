"""Makes (or brings up to date) the story's line sets (UStoryLineSet, Story/StoryLineSet.h) in /Game/Data/Story: what
someone says at a speaker point, or what a headboard reads, as data a writer can change without touching the level.
Run it in the open editor once the C++ is built, before the mission scripts and Tools/Unreal/build_area_story.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_story_lines.py"
It prints a LINES line per set and "LINES done". Running it again writes these sets' lines again; other sets are left
alone. A line with no speaker takes its speaker point's name (Delia's door names her), or shows its words alone (a
headboard read aloud).

What each set is for: a speaker point's topic (said at a point in the story, its condition in the build script that
places the point), a Hob perch's arrival, or a mutter. Which point says which set when is written down in
Tools/Unreal/story_texts.py (TOPICS, PERCHES, MUTTERS), for the build scripts; the missions' own offer, turn-in and choice
lines live in the mission assets (create_mission_assets.py, create_side_mission_assets.py), and the readables' words in
story_texts.py (READABLES). Lines marked "the doc's" are Docs/Story.md's or Docs/Areas/RansomsRest_Expansion.md's; the
rest are drafts in the game's voice for the user (2026-10-13, the missions' data step).

The main story:
  Main 1   DeliaMain1 (her door during Main 1, before the turn-in: take the plate first; the doc's three lines are the
           turn-in's), DeliaPorch (after), HeadboardEllis, HeadboardAbel, HobWakes, HobMain1, HobAfterMain1
  Main 2   HobMain2Climb, HobMain2Blue, HobMain2, SextonWaiting, SextonDeal, SextonLedger, SextonAfter
  Main 3   HobMain3Road, HobMain3Gate, HobMain3Tilly, HobMain3, TillyClosed, TillyMain3 (her talk, the turn-in's words
           from step 3: the bail on the sill, the box on the step), TillyAfterMain3
  Main 4   HobMain4Road/Yard/Bell/Reliquary/Aldana, HobMain4, AldanaBarred, AldanaWaiting (the knock at the vestry door,
           step 3), AldanaBell (the bell and the Reliquary waiting), AldanaMain4 (the turn-in's words), AldanaAfterMain4
  Main 5   HobMain5Way/Sacs/Lantern/Out, HobMain5, AldanaAfterMain5
  Main 6   DeliaMain6, HobMain6Way/Post/Fight, HobMain6Bridge (the keeper's path), HobMain6, AbelOnBoard, AbelAfterMain7
  Main 7   DeliaMain7, DeliaMain7After, HobMain7Lean/Depot/Board, HobMain7, TillyMain7, TillyMain7Father (once Knocking
           Below is done), TillyAfterMain7
The valley's side missions:
  Side 1   HobSide1
  Caches   HobCaches
  Old Fare AgentDoor, AgentCoins (Mr. Sloane's door: waiting, then the confession), AgentWindow, AgentWindowCoal (during
           Dead Lift), AgentWindowLily (his ticket window after it), FennLook, FennCoins (Fenn's coffin: looked into,
           the coins laid back), FennAfter, HobOldFare; the names build_area_missionprops_valley.py looks for
  Side 2   AmosMeet (his default lines), AmosBales, AmosRoad (the twist: who's on the road), AmosHands, AmosThanks,
           AmosFence, AmosAfterMain6, AmosBaleFirst, HobBaleThird, AmosBaleLast
  Knocking TillyKnocking, TillyAfterKnocking, ThaddeusTalk, ThaddeusAfter, ThaddeusAfterMain7, HobKnocking
  Side 3   HobSide3
The mountain:
  Dead Lift  HiramWaiting, HiramFit, HiramSteam, HiramRide, HiramAfter, HiramCrew (a mutter as the lift crew rise),
             LiftTubeAfter, HaskettLift (the Haskett house's mutter once the lift runs), HobDeadLift
  Last Watch HobLastWatch
  Back Pay   HobBackPay, HobBackPayTags (his perch at the mine's mouth), TagBoardKept (the shift's voice after a kept payroll)
The Rest's seeds (2026-10-16): HobIdle (Hob's three feathers, his mutters), AldanaAda (What Ada gave up, once after Main 4),
  HobMain6Point (the road at dusk: his arrival on Pa's neighbour bier after Main 6), and AgentWindowLily's last two lines
  (the Junction and Highwater Landing).
"""
import unreal

FOLDER = '/Game/Data/Story'

HOB = 'Hob'
# Ellis speaks in the deal, at Tilly's window and at Aldana's door (the cold open names them so too).
ELLIS = 'Ellis'
# A hay bale's remark plays as captions with no speaker point to name him, so his remarks name him (as his point does).
AMOS = 'Amos Whitlock'

SETS = {
    # --- Main 1 ---
    # Her door while Main 1 runs (the topic "before Main 1"): the player reaches it at the porch-plate step, so she sends
    # them to the plate; her real talk (the doc's three lines and the plate) is Main 1's turn-in, once the plate is taken.
    'DA_Lines_DeliaMain1': [
        ('', "...Ellis? Don't. Don't you open that door."),
        ('', "There's a plate on the porch table. Take what's on it. Then we'll talk."),
    ],
    'DA_Lines_DeliaPorch': [
        ('', "There's a plate on the porch. I can't open this door to you."),
    ],
    'DA_Lines_HeadboardEllis': [
        ('', "ELLIS RANSOM. CAME HOME AT THE LAST."),
    ],
    'DA_Lines_HeadboardAbel': [
        ('', "ABEL RANSOM. KEEPER OF SAINT ADA. HE HELD THE DOOR."),
        (HOB, "Frost on a grave in August. That's no weather, sunshine."),
    ],
    'DA_Lines_HobWakes': [
        (HOB, "Morning, sunshine. Most folks who get up do it after dark."),
    ],
    'DA_Lines_HobMain1': [
        ('', "Go on up to the house, sunshine. Somebody's been setting a plate."),
    ],
    'DA_Lines_HobAfterMain1': [
        ('', "Don't look at me. I only carry the messages."),
    ],

    # --- Main 2 ---
    # Hob's offer on the porch is in the mission ("Someone's waiting on you..."): at the path's foot he only hurries Ellis on.
    'DA_Lines_HobMain2Climb': [
        (HOB, "Up the bluff path, sunshine. Mind the fallen pine. He doesn't like to be kept waiting."),
    ],
    'DA_Lines_HobMain2Blue': [
        (HOB, "See the blue on that one? Fed longer on the dark. Hits harder, and its iron's better."),
    ],
    'DA_Lines_HobMain2': [
        ('', "Go on up, sunshine. He's patient, but he keeps count."),
    ],
    'DA_Lines_SextonWaiting': [
        ('', "The spiders first, friend. I'll keep."),
    ],
    'DA_Lines_SextonDeal': [
        ('', "Shall we talk business?"),
        (ELLIS, "You were on this rail. The night they shot me."),
        ('', "I attend every death, friend. It's my trade."),
        ('', "Seven of your old friends carry seven embers that aren't theirs. Bring each one here, to this rail, at the "
             "new moon."),
        ('', "Each one paid buys back a little of you. Bring me all seven, and you rest."),
        ('', "And your father crosses. I'll see to his fare myself."),
        (ELLIS, "And if I don't?"),
        ('', "Then you ride the Gravewind, friend. For ever, and no shore at the end of it."),
        (ELLIS, "They scattered. Where do I even start?"),
        ('', "Ask a keeper. Their lanterns lean toward a saint's light."),
        (ELLIS, "The keeper's dead."),
        ('', "So are you, friend."),
        ('', "Their names are in my ledger. Do read them."),
    ],
    'DA_Lines_SextonLedger': [
        ('', "The names are in the ledger, friend. Do read them."),
    ],
    'DA_Lines_SextonAfter': [
        ('', "Seven names, friend. The new moon won't wait."),
    ],

    # --- Main 3 ---
    'DA_Lines_HobMain3Road': [
        (HOB, "Town's down the farm road, through the orchard. Don't expect a welcome, sunshine."),
    ],
    'DA_Lines_HobMain3Gate': [
        (HOB, "Here they come. Nothing draws the dead like a corpse with a gun."),
    ],
    'DA_Lines_HobMain3Tilly': [
        (HOB, "Bright & Daughter, down the street. They laid you out. Might as well say thanks."),
    ],
    'DA_Lines_HobMain3': [
        ('', "Folks shutter up when the dead come calling. Can't say I blame them."),
    ],
    'DA_Lines_TillyClosed': [
        ('', "We're closed. Back after the funeral."),
    ],
    # Her talk, Main 3's turn-in (her topic from its step 3, the step past the last; the mission's turn-in lines are the
    # same words): the doc's beats, the bail out through the window onto the sill (Main 6's item, lying there from Main 3's
    # end), and the box of effects on the step (the reward gun).
    'DA_Lines_TillyMain3': [
        ('', "Well. They said you might be up and about."),
        ('', "I laid you out, you know. You and your Pa, the same morning."),
        ('', "You've ruined my collar, by the way."),
        (ELLIS, "Pa's lantern. Was it with him?"),
        ('', "No. A keeper goes in the ground with his lantern, and his wasn't on him. I looked."),
        ('', "Only its bail, the ring it hangs by, snapped off in his fist. I couldn't open his hand till morning."),
        ('', "It's on the sill. Take it. A lantern's no use to anyone if it can't be hung."),
        ('', "There's a box of unclaimed effects on the step. Take what's useful. Nobody's coming back for it."),
    ],
    'DA_Lines_TillyAfterMain3': [
        ('', "Still closed. Mind that collar."),
    ],

    # --- Main 4 ---
    'DA_Lines_HobMain4Road': [
        (HOB, "Up the north road to the chapel, sunshine. Saint Ada's yard is full of folks who didn't stay put. "
              "You'll fit right in."),
    ],
    'DA_Lines_HobMain4Yard': [
        (HOB, "Years of the Rest's dead, come home to their own graves. They don't take to company, sunshine."),
    ],
    'DA_Lines_HobMain4Bell': [
        (HOB, "Quiet. Bell rope's inside the door. That bell used to call the dead to rest. Give it a pull, see who "
              "answers."),
    ],
    'DA_Lines_HobMain4Reliquary': [
        (HOB, "Nobody answered. Bell's fine, sunshine. It's the saint that's out. Go look at what's left of her."),
    ],
    'DA_Lines_HobMain4Aldana': [
        (HOB, "Somebody's breathing behind the vestry door. Living, by the sound of it. Knock nice."),
    ],
    'DA_Lines_HobMain4': [
        ('', "No light, no road. When the saint went out, every soul still on the Sundown Road drifted home. That's the "
             "whole yard, sunshine."),
    ],
    'DA_Lines_AldanaBarred': [
        ('', "Stay back from that door. I've no comfort left for the dead, and the yard is full of them."),
    ],
    # The knock at the vestry door (Main 4's third step, its talk): the doc's words, and the bell's reason.
    'DA_Lines_AldanaWaiting': [
        ('', "Stay back from that door."),
        (ELLIS, "Tilly sent me. Pa's lantern."),
        ('', "...Abel's child. Lord forgive me, I'd know that voice through any door."),
        ('', "If the saint still knows you, she'll answer her bell. Ring it. Then look at what was done to her."),
    ],
    # While the bell and the Reliquary wait (from Main 4's fourth step).
    'DA_Lines_AldanaBell': [
        ('', "The rope's inside the door. Ring her, Ellis. Then go and look at her. I can't."),
    ],
    # Main 4's turn-in, the doc's words, word for word: the mission says them (its turn-in lines), and his offer of Main 5
    # follows them as one conversation.
    'DA_Lines_AldanaMain4': [
        ('', "You rang her bell, and she didn't answer. Now you've seen why."),
        (ELLIS, "How do I find them?"),
        ('', "A keeper's lantern can find an ember."),
        ('', "In every town they robbed, the gang shot the keeper first and smashed his lantern."),
        ('', "Your father's fell in the dark, whole."),
        ('', "Spiders hoard anything a saint has touched."),
        ('', "Look in the Sink."),
    ],
    'DA_Lines_AldanaAfterMain4': [
        ('', "The Sink, Ellis. Your father's lantern. I'll pray it's still whole."),
    ],

    # --- Main 5 ---
    'DA_Lines_HobMain5Way': [
        (HOB, "Keepers walk the dead to the boards by lantern light. Your Pa's is down there, sunshine, somewhere under "
              "the webs. Ramp's on the west side, by the gap in the fence."),
        (HOB, "And whatever dug the hole under me, let's not wake it."),
    ],
    'DA_Lines_HobMain5Sacs': [
        (HOB, "Egg sacs. Three of them, fat and twitching. Shoot them down before they drop on their own, sunshine. Two to "
              "a sac, by the look."),
    ],
    'DA_Lines_HobMain5Lantern': [
        (HOB, "That's the last of them. There, in the webbing by the north wall. Brass and glass. That's your Pa's."),
    ],
    'DA_Lines_HobMain5Out': [
        (HOB, "Dark as a shut eye, but whole. Missing its bail. You've got that, sunshine."),
        (HOB, "Up the ramp. And don't stare at the hole in the east wall too long. Something in there stares back."),
    ],
    'DA_Lines_HobMain5': [
        ('', "Spiders hoard anything a saint has touched. Your Pa's lantern was lit from her every night he kept her. To "
             "them it smells like supper, sunshine."),
    ],
    # After Main 5 (its turn-in has the candle that won't take): a word in passing.
    'DA_Lines_AldanaAfterMain5': [
        ('', "Take it home, Ellis. Your grandmother will know what's owed a keeper."),
        ('', "I tried the candle again after you'd gone. Old habits."),
    ],

    # --- Main 6 ---
    'DA_Lines_DeliaMain6': [
        ('', "Take him the lantern. Show him the way, even if he can't go."),
    ],
    'DA_Lines_HobMain6Way': [
        (HOB, "Keeper's grave. Every Ransom who ever sat up with the dead is under that board, sunshine. Your Pa's the "
              "first one who won't stay there."),
        (HOB, "Follow the cairns to the gate. He'll be on the boards. It's dusk."),
    ],
    # On Keeper's Bridge, if Ellis takes the keeper's path (Main 6's optional).
    'DA_Lines_HobMain6Bridge': [
        (HOB, "Every Ransom for sixty years walked the dead this way at sundown. Your Pa walked it the night before they "
              "shot him."),
    ],
    'DA_Lines_HobMain6Post': [
        (HOB, "That's the keeper's post, by the steps. Hang it where he'd look for it."),
    ],
    'DA_Lines_HobMain6Fight': [
        (HOB, "He doesn't know you, sunshine. Not yet. And mind the open end: the wind's his."),
    ],
    'DA_Lines_HobMain6': [
        ('', "The Gravewind comes off the Rim at dusk and takes the dead west. It's come for him every night this week, "
             "sunshine. He keeps walking back."),
    ],
    'DA_Lines_AbelOnBoard': [
        ('', "Sun goes down the same every night, El. I never once got tired of it."),
        ('', "Your grandmother still setting a plate? Eat it, even if you can't taste it. She needs to see it gone."),
    ],
    'DA_Lines_AbelAfterMain7': [
        ('', "She still leans north-east. I'd know that lean anywhere."),
        ('', "Go on, El. I'll keep the boards."),
    ],

    # --- Main 7 ---
    'DA_Lines_DeliaMain7': [
        ('', "A keeper's buried with his lantern, not his iron."),
        ('', "His lantern wasn't on him, so I kept this back."),
        ('', "He'd want you to have it."),
        ('', "Hold the door."),
    ],
    'DA_Lines_DeliaMain7After': [
        ('', "Go on, now. Tilly's got her father's car waiting at the depot."),
        ('', "I'll keep setting a plate."),
    ],
    'DA_Lines_HobMain7Lean': [
        (HOB, "That's Purcell. The Lily's moored out that way."),
        (HOB, "Go home first, sunshine. Your grandmother's been keeping something back."),
    ],
    'DA_Lines_HobMain7Depot': [
        (HOB, "Tilly's Pa's car. The living won't share a carriage with a corpse, so you ride with the coffins. First "
              "class, sunshine."),
    ],
    'DA_Lines_HobMain7Board': [
        (HOB, "Board's by the depot door. Let's see if the railroad agrees with the lantern."),
    ],
    'DA_Lines_HobMain7': [
        ('', "North-east, over the ridges. Purcell's out there with a deck of cards and an ember that isn't his, "
             "sunshine."),
    ],
    'DA_Lines_TillyMain7': [
        ('', "Father's car is coupled up at the platform. The living won't share a carriage with you, so you'll ride "
             "with the coffins."),
        ('', "Mind the brass. I polished it for him, and he never once complained."),
    ],
    # Main 7 once Knocking Below is done: she knows now what he thought of the brass.
    'DA_Lines_TillyMain7Father': [
        ('', "Father's car is coupled up at the platform. The living won't share a carriage with you, so you'll ride "
             "with the coffins."),
        ('', "Mind the brass. He liked it, as it turns out. He just never said."),
    ],
    'DA_Lines_TillyAfterMain7': [
        ('', "The car's yours whenever the line runs. Wipe your boots."),
    ],

    # --- Side 1, Wanted: Already Dead; Calder's Caches ---
    'DA_Lines_HobSide1': [
        ('', "Your face on every post in town, sunshine. Not your best side, either."),
    ],
    'DA_Lines_HobCaches': [
        ('', "Ruth Calder lays caches for her Rangers and hangs outlaws in her spare time. Take her tags, sunshine. Leave "
             "the rope."),
    ],

    # --- The Old Fare: Mr. Sloane's door upstairs in the depot, then his ticket window; Josiah Fenn's coffin ---
    # The names are the ones build_area_missionprops_valley.py's LINES looks for (the valley-placement step).
    # His door while the Old Fare runs, until its talk.
    'DA_Lines_AgentDoor': [
        ('', "Are they gone? I can still hear them breathing down there."),
        ('', "Don't you open this door. Not till they're gone."),
    ],
    # Asked about Fenn's coins (the Old Fare's fourth step, its talk, and after it): the confession; the coins come out
    # under the door.
    'DA_Lines_AgentCoins': [
        ('', "Coins? What coins?"),
        ('', "...His people laid gold on his eyes, like the old days. Gold, on a dead man! Who does that anymore?"),
        ('', "I only borrowed it. Against a debt. I meant to put it back before the train."),
        ('', "There. Under the door. Take them back to him, and tell him I'm sorry."),
    ],
    # His ticket window, after the Old Fare.
    'DA_Lines_AgentWindow': [
        ('', "Waiting room's quiet. First night in a week I've slept."),
        ('', "...Thank you. For Fenn. And for not telling anyone."),
        ('', "No trains till the line's cleared, mind. And the line's never cleared."),
    ],
    # His window while Dead Lift runs: the coal for Hiram's boiler is the railroad's.
    'DA_Lines_AgentWindowCoal': [
        ('', "You'll be after coal for Hiram's boiler. The stage is round the side, under the chute."),
        ('', "That's railroad coal, mind. ...Oh, take it. Nothing's running."),
    ],
    # After Main 7 the line opens (the Rest's seeds: he names the Junction and Highwater Landing, as the wall map does).
    'DA_Lines_AgentWindowLily': [
        ('', "Line's open to the Lily, kid. Your car's coupled."),
        ('', "Highwater Landing, end of the line. The Lily's tied up there, over the canyon."),
        ('', "Anywhere else, you change at Halfway Junction. Once somebody clears it."),
    ],
    # Fenn's coffin looked into (the Old Fare's first use of it, and until the coins go back): his eyes bare.
    'DA_Lines_FennLook': [
        ('', "Is that the train? ...No. It's never the train."),
        ('', "Someone's had the gold off my eyes. A man can't board without his fare."),
        ('', "Sloane was the last living soul in this room. Ask him."),
        (HOB, "Somebody's been at his fare, sunshine. Rob a dead man's eyes and every soul for a mile hears about it."),
    ],
    # The coins laid back (its last use); his thanks, the turn-in's, follow.
    'DA_Lines_FennCoins': [
        (ELLIS, "Your fare, Mr. Fenn."),
        ('', "...There's the weight of it. Now I can hear the train."),
    ],
    'DA_Lines_FennAfter': [
        ('', "Still here. Waiting on the next train to Gravesend."),
        ('', "No hurry. I've paid."),
    ],
    'DA_Lines_HobOldFare': [
        ('', "Gold on a dead man's eyes. Folks paid the ferry that way, before the saints lit the road. Old habits, "
             "sunshine."),
    ],

    # --- Side 2, Unfinished Business (Amos's offer is the mission's: his meet lines) ---
    'DA_Lines_AmosMeet': [
        ('', "Well, now. You're looking right at me. Folks in town look clean through me, like a window."),
        (ELLIS, "Amos Whitlock?"),
        ('', "What's left of him. And you're Abel's young'un. I heard the shooting from the Sundown Road the night the "
             "saint went out. I'm sorry for it."),
        ('', "I died last harvest with the hay half in. Got turned back on the road and came home to find it rotting "
             "where I left it."),
        ('', "My hands go through a bale like it was smoke. Would you load it for me? Six bales, stacked under the hoist "
             "by the big doors."),
    ],
    # While the bales wait (Side 2's first step).
    'DA_Lines_AmosBales': [
        ('', "Six bales, under the hoist by the big doors. I'd do it myself if my hands would hold."),
    ],
    # The twist (its second step, a talk): who's on the road.
    'DA_Lines_AmosRoad': [
        ('', "That's the Merritt brothers out on the road, and Sol Tanner. My hands."),
        ('', "Fever winter took them, and I owed them for the harvest. Never paid. Couldn't."),
        ('', "Reckon they've come to collect."),
        ('', "Drive them off, Ellis. Gently, if there's a gently. They were good boys."),
    ],
    'DA_Lines_AmosHands': [
        ('', "Don't let them near the hay. It's all I've got to show for them."),
    ],
    'DA_Lines_AmosThanks': [
        ('', "Gone, are they? Then the hay's in, and it'll keep."),
        ('', "I'll pay them what I owe when I see them on the far side. If there's a far side."),
        ('', "They buried me with a gun I never once fired. It's yours. Better in a walking hand than a buried one."),
        ('', "I'll sit a while. The saint'll come home. I'd like to be here when she does."),
    ],
    'DA_Lines_AmosFence': [
        ('', "Still waiting on her. The hay keeps. So do I."),
    ],
    'DA_Lines_AmosAfterMain6': [
        ('', "Saw a light out on Gravewind Point at dusk. Your Pa's, I'd wager. Good to know a keeper's still keeping."),
    ],
    'DA_Lines_AmosBaleFirst': [
        (AMOS, "That's one. Mind your back. Mine's past minding."),
    ],
    'DA_Lines_HobBaleThird': [
        (HOB, "Hauling hay for a ghost. Your Pa would laugh, sunshine. Then he'd hand you another."),
    ],
    # The last bale: thanks, and the road (the talk that's Side 2's next step).
    'DA_Lines_AmosBaleLast': [
        (AMOS, "That's the last of it. Thank you, Ellis."),
        (AMOS, "...Who's that out on the road? Come here a minute."),
    ],

    # --- Knocking Below: Tilly, and her father in the cold room ---
    'DA_Lines_TillyKnocking': [
        ('', "The workshop's round the back, off the yard. Father kept the cellar key on his measuring hook."),
        ('', "I haven't moved it. I haven't moved anything."),
    ],
    'DA_Lines_TillyAfterKnocking': [
        ('', "He's still down there, measuring. I hear the tape at night. It's almost a comfort."),
        ('', "Don't tell him I said so."),
    ],
    # Talked to at his bench (Knocking Below's sixth step, and after it): the doc's words; the tape lies on the bench for
    # the next step. (build_area_missionprops_valley.py gives him his words before it, "ThaddeusWaiting".)
    'DA_Lines_ThaddeusTalk': [
        ('', "Ransom's child. I measured you, you know. You've stood up since."),
        ('', "I kept them in their boxes a week. They got past me when the door opened. A cellar wants keeping."),
        ('', "Give her my tape. Tell her the brass was too much. And that I liked it."),
    ],
    'DA_Lines_ThaddeusAfter': [
        ('', "Five foot eleven, you were, and narrow at the shoulder. Pine. No brass."),
        ('', "I remember them all, Ransom's child. Somebody has to."),
    ],
    'DA_Lines_ThaddeusAfterMain7': [
        ('', "My car's carrying you, I hear. Mind the brass. She'll know if you scuff it."),
    ],
    'DA_Lines_HobKnocking': [
        ('', "Something knocking in an undertaker's cellar. I'd say don't go down there, sunshine, but you never listen."),
    ],

    # --- Side 3, The Gravemother ---
    'DA_Lines_HobSide3': [
        ('', "A bounty on a spider the size of a hay wagon, posted by the last man who went after her. Inspiring."),
    ],

    # --- Dead Lift: Hiram Coyle at his stool, his voice up the speaking tube ---
    'DA_Lines_HiramWaiting': [
        ('', "Crank, coal and pin. She won't move for less."),
        ('', "Crank's in the sheriff's lockup. Coal at the depot's stage. Pin's on Abe, wherever Abe's got to."),
    ],
    'DA_Lines_HiramFit': [
        ('', "Crank on the winch shaft. Coal in the firebox. The pin goes in the bottom sheave, and mind your fingers."),
    ],
    'DA_Lines_HiramSteam': [
        ('', "She's taking it! Keep them off her till the needle's up!"),
    ],
    'DA_Lines_HiramRide': [
        ('', "Hold the rail. And don't look down till you're over the top."),
    ],
    'DA_Lines_HiramAfter': [
        ('', "She runs. Pull the bell either end and I'll bring her round."),
        ('', "Four years I sat by a cold boiler. Felt longer."),
    ],
    # Muttered from the winch house as the lift crew rise on the quarry floor.
    'DA_Lines_HiramCrew': [
        ('', "Ah. The boys heard you ask after the pin."),
    ],
    'DA_Lines_LiftTubeAfter': [
        ('', "Coyle here. She's yours, up or down. Pull the bell."),
    ],
    # The Haskett house on Main Street, once the lift runs.
    'DA_Lines_HaskettLift': [
        ('', "...That's the lift. Abe always said he'd hear her run."),
    ],
    'DA_Lines_HobDeadLift': [
        ('', "A dead man running a dead lift up a dead mountain. You'll fit right in, sunshine."),
    ],

    # --- The Last Watch; Back Pay ---
    'DA_Lines_HobLastWatch': [
        ('', "Light a fire so the Rangers know Larkspur stands. The same Rangers who'd hang you. Funny old world, "
             "sunshine."),
    ],
    'DA_Lines_HobBackPay': [
        ('', "Eleven men, three months' wages, one Foreman. Debts, sunshine. This whole country runs on them."),
    ],
    'DA_Lines_HobBackPayTags': [
        (HOB, "Eleven tags on IN. You do the sum, sunshine."),
    ],
    # The tag board's voice once the payroll was kept (story flag BackPay.Kept): the shift goes on.
    'DA_Lines_TagBoardKept': [
        ('', "Shift's not over."),
        ('', "Pay's Friday. Pay's always Friday."),
    ],

    # --- The Rest's seeds (Docs/Story.md "Seeds in Ransom's Rest", 2026-10-16) ---
    # Hob's feathers: his mutters from Main 1's end, one a pass, in this order (the doc's three, word for word).
    'DA_Lines_HobIdle': [
        (HOB, "Eighty years I've been collecting for him. You'd think he'd pay a man."),
        (HOB, "Careful what you shake on. His deals have a long tail."),
        (HOB, "I had hands once. Don't ask."),
    ],
    # What Ada gave up: Father Aldana's one talk after Hallowed Ground (topic sets Ada.Told), the first keeper's story.
    'DA_Lines_AldanaAda': [
        ('', "There's a thing the Rest has forgotten. You ought to hear it before you go after her."),
        ('', "The first winter, nobody on the Rest had coin for the old fare. Their dead lay unpaid."),
        ('', "Ada lay down on the first board on Gravewind Point and asked the wind to take her. It carried her to the "
             "edge of the world."),
        ('', "And there she stopped, and held up her light, so nobody of this valley would pay to cross again."),
        ('', "She never crossed herself. No saint does. Every one of them is somebody who stayed behind."),
        (ELLIS, "Sixty years at the edge of the world."),
        (HOB, "Some debts you pay standing up, sunshine."),
    ],
    # The road at dusk: Hob's arrival on the bier beside Pa's once The Gravewind is done (story_texts.PERCHES
    # gravewindPoint), the second light low over the fog; his word last (the doc's, word for word).
    'DA_Lines_HobMain6Point': [
        (ELLIS, "There's a light out past the fog. Low. Steady."),
        (HOB, "Don't look at that one, sunshine."),
    ],
}


def line_set_class():
    cls = getattr(unreal, 'StoryLineSet', None)
    if cls is None or getattr(unreal, 'StoryLine', None) is None:
        raise RuntimeError('unreal.StoryLineSet is missing: build the C++ with Story/ first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a StoryLineSet: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def story_line(speaker, words):
    """FStoryLine: who says it (empty: the speaker point's), the words, and no seconds (a reading time for its words)."""
    line = unreal.StoryLine()
    line.set_editor_property('speaker', unreal.Text(speaker))
    line.set_editor_property('text', unreal.Text(words))
    line.set_editor_property('seconds', 0.0)
    return line


def run():
    cls = line_set_class()
    for name, said in SETS.items():
        asset, created = load_or_create(name, cls)
        asset.set_editor_property('lines', [story_line(speaker, words) for speaker, words in said])
        stored = asset.get_editor_property('lines')
        if len(stored) != len(said) or str(stored[0].get_editor_property('text')) != said[0][1]:
            raise RuntimeError(f'{FOLDER}/{name}: its lines read back wrong: nothing was saved')
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError(f'{FOLDER}/{name} could not be saved')
        unreal.log(f"LINES {name} {'made' if created else 'updated'}: {len(said)} line(s), first \"{said[0][1]}\"")
    unreal.log(f'LINES done: {len(SETS)} sets')


# Run in the editor (console.ps1 "py ..."); another script may import SETS without making anything.
if __name__ == '__main__':
    run()
