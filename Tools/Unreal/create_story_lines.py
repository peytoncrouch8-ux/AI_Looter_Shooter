"""Makes (or brings up to date) the story's line sets (UStoryLineSet, Story/StoryLineSet.h) in /Game/Data/Story: what
someone says at a speaker point, or what a headboard reads, as data a writer can change without touching the level.
Run it in the open editor once the C++ is built, before Tools/Unreal/build_area_story.py places what uses them:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_story_lines.py"
It prints a LINES line per set and "LINES done". Running it again writes these sets' lines again; other sets are left
alone. A line with no speaker takes its speaker point's name (Delia's door names her), or shows its words alone (a
headboard read aloud).

Main 1, "Seven Days" (Docs/Areas/RansomsRest.md; Docs/Story.md, Cold open):
  DA_Lines_DeliaMain1      Grandma Delia through the screen door during Main 1 (the doc's words)
  DA_Lines_DeliaPorch      her door after it (a first draft, for the user to judge)
  DA_Lines_HeadboardEllis  Ellis's own headboard, read
  DA_Lines_HeadboardAbel   Abel's, read, and Hob's remark on the frost (the remark is a first draft)
  DA_Lines_HobWakes        Hob as he lands on Ellis's headboard once Ellis is out (Docs/Story.md's line)
  DA_Lines_HobMain1        Hob talked to during Main 1 (a first draft)
  DA_Lines_HobAfterMain1   Hob talked to after it (a first draft)

Main 2, "Shall We Talk Business?" (the doc's words where it gives them; the rest are first drafts for the user):
  DA_Lines_HobMain2Climb   Hob at the bluff path's foot as Main 2 begins (the doc's line)
  DA_Lines_HobMain2Blue    Hob on the bluff top's rock as the fight begins: the blue one (the doc's line)
  DA_Lines_HobMain2        Hob talked to during Main 2 (a draft)
  DA_Lines_SextonWaiting   Mister Sexton talked to before the spiders are cleared (a draft)
  DA_Lines_SextonDeal      the deal: "Shall we talk business?" through "So are you, friend." (Docs/Story.md and the
                           doc's lines; the rail, the threat and the hand-over of the ledger are drafts)
  DA_Lines_SextonLedger    Sexton talked to again while the Ledger waits to be opened (a draft)
  DA_Lines_SextonAfter     Sexton talked to after the deal, before he's gone (a draft)

Main 3, "Cold Welcome":
  DA_Lines_HobMain3Road    Hob on the town gate's post as Main 3 begins (a draft)
  DA_Lines_HobMain3Gate    Hob as the Unpaid come at the gate (a draft)
  DA_Lines_HobMain3Tilly   Hob on Bright & Daughter's sign once the gate is won (a draft)
  DA_Lines_HobMain3        Hob talked to during Main 3 (a draft)
  DA_Lines_TillyClosed     Tilly at her window before Main 3's last step (a draft, from the card in her window)
  DA_Lines_TillyMain3      Tilly at her window: she laid out both Ransoms, Abel's lantern wasn't on him, "You've ruined
                           my collar, by the way." (the doc's beats and line; the rest is a draft), and the unclaimed
                           effects on the step (Main 3's Uncommon gun)
  DA_Lines_TillyAfterMain3 Tilly after Main 3 (a draft)

Main 4, "Hallowed Ground" (the doc gives Aldana's words at the vestry door; everything else is a first draft for the user):
  DA_Lines_HobMain4Road      Hob on the chapel's door hood as Main 4 begins (a draft)
  DA_Lines_HobMain4Yard      Hob as the chapel yard's Unpaid rise (a draft)
  DA_Lines_HobMain4Bell      Hob once the yard is quiet: the bell (a draft)
  DA_Lines_HobMain4Reliquary Hob once the bell has rung and nothing answered: the Reliquary (a draft)
  DA_Lines_HobMain4Aldana    Hob on the vestry lantern's bracket after the sight: Aldana's door (a draft)
  DA_Lines_HobMain4          Hob talked to during Main 4 (a draft)
  DA_Lines_AldanaBarred      Father Aldana through the vestry door before the yard is quiet, and before Main 4 (a draft)
  DA_Lines_AldanaWaiting     Aldana once the yard is quiet, while the bell and the Reliquary wait (a draft)
  DA_Lines_AldanaMain4       Aldana at Main 4's last step: "A keeper's lantern can find an ember." through "Look in the
                             Sink." (the doc's words, word for word; his first line and Ellis's question are drafts)
  DA_Lines_AldanaAfterMain4  Aldana after Main 4 (a draft)

Main 5, "The Keeper's Lantern" (the doc gives no words but its line "The keepers walk the dead to the boards by lantern
light"; everything here is a first draft for the user):
  DA_Lines_HobMain5Way       Hob on Den Rock over the den as Main 5 begins: the way down, and the hole under him (a draft
                             around the doc's line)
  DA_Lines_HobMain5Sacs      Hob on a block on the Sink's floor once the player is down: the egg sacs (a draft)
  DA_Lines_HobMain5Lantern   Hob on the block once the sacs are down: the lantern in the webbing (a draft)
  DA_Lines_HobMain5Out       Hob on the Sink road's dead tree once the lantern is taken: it's dark, climb out, the den (a
                             draft)
  DA_Lines_HobMain5          Hob talked to during Main 5 (a draft)
  DA_Lines_AldanaAfterMain5  Aldana after Main 5: the lantern found, take it home to Delia (a draft)

Main 6, "The Gravewind" (the doc gives Delia's words and the scene's; the scene's lines are in the C++, Scenes/SitWithPa;
everything here but Delia's is a first draft for the user):
  DA_Lines_DeliaMain6        Delia through the door after Main 5, until Main 6 is done: "Take him the lantern. Show him the
                             way, even if he can't go." (the doc's words); her topic's event starts Main 6
  DA_Lines_HobMain6Way       Hob on the keeper's grave's board as Main 6 begins: the way to the deck (a draft)
  DA_Lines_HobMain6Post      Hob on the Keeper's Gate's rock at the deck: the keeper's post (a draft)
  DA_Lines_HobMain6Fight     Hob as the lantern hangs and Pa turns (a draft)
  DA_Lines_HobMain6          Hob talked to during Main 6 (a draft)
  DA_Lines_AbelOnBoard       Abel on his board after Main 6 (drafts; each ember paid later adds its own set)
  DA_Lines_AbelAfterMain7    Abel on his board once the lantern has led Ellis on (after Main 7; drafts)

Main 7, "The Lantern Leans" (the doc gives Delia's words and Hob's line at the lean; everything else is a first draft):
  DA_Lines_DeliaMain7        Delia through the door at Main 7's first step, as she hands Heirloom out: "A keeper's buried
                             with his lantern, not his iron." through "Hold the door." (the doc's words, word for word)
  DA_Lines_DeliaMain7After   Delia once she has (a draft)
  DA_Lines_HobMain7Lean      Hob on the keeper's post as Main 7 begins: "That's Purcell. The Lily's moored out that way."
                             (the doc's line) and the way home (a draft)
  DA_Lines_HobMain7Depot     Hob on Tilly's hearse car's roof once Delia has handed it over (a draft)
  DA_Lines_HobMain7Board     Hob there at the depot: the station board (a draft)
  DA_Lines_HobMain7          Hob talked to during Main 7 (a draft)
  DA_Lines_TillyMain7        Tilly at her window during Main 7: her father's car at the platform (a draft)
  DA_Lines_TillyAfterMain7   Tilly after Main 7 (a draft)

Side 2, "Unfinished Business" (the doc gives Amos's story but no words: every line here is a first draft for the user):
  DA_Lines_AmosMeet          Amos at his fence before he's asked (Side 2's first step, and his lines with no topic): Ellis
                             can see him; the hay he never got in; would Ellis load it
  DA_Lines_AmosBales         Amos while the bales wait (its second step)
  DA_Lines_AmosHands         Amos while his old hired hands are in the yard (its third)
  DA_Lines_AmosThanks        Amos at its last step: the hay's in, the gun he was buried with, and he'll sit and wait
  DA_Lines_AmosFence         Amos on his fence after Side 2, waiting for the saint
  DA_Lines_AmosAfterMain6    Amos on his fence after Side 2 and Main 6: a word of Abel's light on the point
  DA_Lines_AmosBaleFirst     Amos as the first bale goes into the stack (a remark, so it names him)
  DA_Lines_HobBaleThird      Hob at the third
  DA_Lines_AmosBaleLast      Amos at the last: thanks, and the hands in the yard (Side 2's third step begins)
"""
import unreal

FOLDER = '/Game/Data/Story'

HOB = 'Hob'
# Ellis speaks in the deal, at Tilly's window and at Aldana's door (the cold open names them so too).
ELLIS = 'Ellis'
# A hay bale's remark plays as captions with no speaker point to name him, so his remarks name him (as his point does).
AMOS = 'Amos Whitlock'

SETS = {
    'DA_Lines_DeliaMain1': [
        ('', "I was to lay you on the boards tonight."),
        ('', "Your Pa got up Wednesday night, came up through the dirt like it was fog."),
        ('', "A keeper doesn't lie still while his saint is dark. He walks the boards at dusk."),
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
    'DA_Lines_HobMain2Climb': [
        (HOB, "Someone's waiting on you. Up top, where it happened."),
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
    'DA_Lines_TillyMain3': [
        ('', "Well. They said you might be up and about."),
        ('', "I laid you out, you know. You and your Pa, the same morning."),
        ('', "You've ruined my collar, by the way."),
        (ELLIS, "Pa's lantern. Was it with him?"),
        ('', "No. A keeper goes in the ground with his lantern, and his wasn't on him. I looked."),
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
    'DA_Lines_AldanaWaiting': [
        ('', "They're quiet. Ring her bell, then look at what was done to her. Then we'll talk."),
    ],
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
        (HOB, "Dark as a shut eye, but whole. Up the ramp, sunshine."),
        (HOB, "And don't stare at the hole in the east wall too long. Something in there stares back."),
    ],
    'DA_Lines_HobMain5': [
        ('', "Spiders hoard anything a saint has touched. Your Pa's lantern was lit from her every night he kept her. To "
             "them it smells like supper, sunshine."),
    ],
    'DA_Lines_AldanaAfterMain5': [
        ('', "You found it. Dark, but whole."),
        ('', "Take it home to your grandmother, Ellis. She'll know what's owed a keeper."),
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
    'DA_Lines_TillyAfterMain7': [
        ('', "The car's yours whenever the line runs. Wipe your boots."),
    ],

    # --- Side 2 (all drafts) ---
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
    'DA_Lines_AmosBales': [
        ('', "Six bales, under the hoist by the big doors. I'd do it myself if my hands would hold."),
    ],
    'DA_Lines_AmosHands': [
        ('', "Those are my hired men in the yard. The fever took them the winter before I went, and they came home "
             "hungry."),
        ('', "They were good boys once. Drive them off before they get at the hay."),
    ],
    'DA_Lines_AmosThanks': [
        ('', "Gone, are they? Then the hay's in, and it'll keep."),
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
    'DA_Lines_AmosBaleLast': [
        (AMOS, "That's the last of it. Thank you, Ellis."),
        (AMOS, "Now mind the yard. My old hired hands are about, and they've come home angry."),
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
    unreal.log('LINES done')


run()
