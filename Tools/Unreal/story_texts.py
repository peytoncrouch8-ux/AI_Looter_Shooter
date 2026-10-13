"""The missions' words that live on things in the level, as data for the build scripts that place those things
(Docs/Areas/RansomsRest_Expansion.md sections 3-4; the missions' data step, 2026-10-13). Nothing here is made or placed:
a build script imports this module and copies what it needs onto the actor it places. The missions themselves (their
offers, steps, turn-ins and choices) are Tools/Unreal/create_mission_assets.py's and create_side_mission_assets.py's; the
line sets the topics below name are Tools/Unreal/create_story_lines.py's.

  READABLES   every AReadableProp's words by readable id (its title, heading, text, prompt and the lines said after
              reading it), and when it's there (shown_when, a condition as below; none: always). apply_readable() copies
              them onto a placed prop; READABLE_MODELS names the ones that bring a model of their own.
  PORCH_PLATE the porch plate's (APorchPlate) prompts, the slip read as the jars are taken, Delia's answer as the biscuits
              are eaten, and its two conditions.
  ARTIFACTS   each AMissionArtifact's title, prompt and whether it goes once its mission is taken (its card's words are
              its mission's briefing).
  ITEMS       each AMissionItem's mission, where it lies, its prompt and when it's there (the missions' own FMissionItemInfo
              names and Carried lines are in the mission scripts).
  TOPICS      what each speaker point says when, by its actor tag: (condition, line set) pairs, the first that holds is
              said (build_area_story.topics() takes them as they are once each condition is made with condition()).
  MUTTERS     lines a speaker point says unasked as the player passes, with when.
  PERCHES     Hob's new perches' arrival lines (the spot is the placing script's), with when.
  ENCOUNTERS  when each mission fight is on (an encounter spawner's active_when), by its spawner id.

A condition is a dict of build_area_story.condition()'s words (after, before, during, from_step, before_step; steps
counted from 0, as the missions' are) plus with_flags and without_flags (story flags); condition() below makes the
unreal.StoryCondition from one. The missions' step numbers these name are listed in each mission script's docstring.
The file stays plain ASCII: a signature's dash is DASH, an escape.
"""

HOB = 'Hob'
ELLIS = 'Ellis'
# The long dash before a signature (an em dash), made from its code so the file stays plain ASCII.
DASH = chr(0x2014)

# ---------------------------------------------------------------------------
# Readables (AReadableProp: readable_id, title, heading, text, lines, prompt, shown_when)
# ---------------------------------------------------------------------------

READABLES = {
    # Main 1, optional: Ellis's own headboard, beside Abel's in the family plot (Grave_Ellis's Interact socket).
    'HeadboardEllis': dict(
        title="Ellis's headboard", heading='Fresh pine, in the family plot', prompt='Read',
        text='ELLIS RANSOM.\nCAME HOME AT THE LAST.',
        lines=[(ELLIS, 'Who carved this?'),
               (HOB, 'Your grandmother. Took her all night, sunshine. She did the letters twice.')]),
    # Main 2, optional: the lookout's boards where the Deacon shot Ellis (Ransom's Point).
    'LookoutStains': dict(
        title='The boards where it happened', heading="Ransom's Point, the lookout's deck", prompt='Look', text='',
        lines=[(ELLIS, 'He was standing right here.'),
               (HOB, 'So were you, sunshine. Briefly.')]),
    # Main 4, optional: the keepers' roll, a painted plank in the nave (SM_KeepersRoll).
    'KeepersRoll': dict(
        title="The keepers' roll", heading='Painted on a plank in the nave', prompt='Read',
        # The names the board itself carries (SM_KeepersRoll, the props art).
        text=('KEEPERS\nOF SAINT ADA\n\nSILAS\nHANNAH\nJONAS\nABEL\n\n'
              'Under ABEL a name has been painted out. It still shows through:\nELLIS'),
        lines=[(HOB, 'Heir to the family business, sunshine. Funny how that goes.')]),
    # Main 5, optional: the sheriff's hat by the den, on the Sink's floor (SM_SheriffHat, chewed).
    'SheriffHat': dict(
        title="A lawman's hat", heading='In the webbing by the den', prompt='Look',
        text='Chewed through the crown. The band is stamped W. ODELL.',
        lines=[(ELLIS, 'Wade Odell. The sheriff.'),
               (HOB, 'Whatever lives in that hole had the sheriff for supper.')]),
    # Side 3, optional: his badge, deep in the Gravemother's den (SM_SheriffBadge).
    'SheriffBadge': dict(
        title="Sheriff Odell's badge", heading='Caught in the webbing, deep in her den', prompt='Look', text='',
        lines=[(ELLIS, "Wade Odell's badge. He gave me a hiding once, for stealing apples."),
               (HOB, "Somebody at Pruitt's will want that back, sunshine.")]),
    # Dead Lift, optional: pinned up in the sheriff's lockup, by the winch crank.
    'OdellReport': dict(
        title="Sheriff Odell's report", heading='Pinned up in the lockup', prompt='Read',
        text=('PROVIDENCE MINING CO. THE FALL, LOWER DRIFT.\n\n'
              "Eleven men. Teague swears the roof gave on its own. Then why'd he have powder burns on his cuffs?\n\n"
              'My brother Jory was one of the eleven.\n\n' + DASH + ' W.O.'),
        lines=[(HOB, 'A lawman with a hunch and no proof. Sounds about right for the Rest, sunshine.')]),
    # Side 1, optional: the sheriff's daybook, open on his desk in the office.
    'OdellDaybook': dict(
        title="The sheriff's daybook", heading='Open on his desk', prompt='Read',
        text=('Haskett mule gone. Tracks to the Sink.\n'
              'My dog gone. Same tracks.\n'
              'Gone down the Sink after the big one. Back by supper.\n\n' + DASH + ' W.O.'),
        lines=[(HOB, "That was a month ago. Supper's cold, sunshine.")]),
    # Side 2, optional: chalked inside the Whitlock barn's door (SM_RR_Barn_Whitlock's WagesChalk socket).
    'WagesChalk': dict(
        title='What Amos owed', heading='Chalked inside the barn door', prompt='Read',
        text='OWED\n\nMERRITT  14.00\nMERRITT  14.00\nTANNER  11.50',
        lines=[(ELLIS, 'Three names. Nothing crossed out.'),
               (HOB, "An honest man's ledger, sunshine. Nobody else round here keeps one.")]),
    # Knocking Below, optional: the coffin orders on the slate by the workshop door (OrdersSlate socket).
    'OrdersSlate': dict(
        title='Coffin orders', heading='Chalk on the slate by the workshop door', prompt='Read',
        text=('MERCER, J. PINE.\nHASKETT, R. PINE.\nPELL, A. PINE.\nSTRAND, M. PINE, SMALL.\n\n'
              'Struck through, in another hand:\nT. BRIGHT. PINE. NO BRASS.'),
        lines=[(HOB, 'Wrote his own order, did he. And she put brass on it anyway.')]),
    # The Old Fare, optional: the agent's desk in the ticket office (SM_RR_Depot's AgentDesk socket).
    'AgentDesk': dict(
        title="The agent's desk", heading='The drawer, left open', prompt='Look',
        text='GILDED LILY. HOUSE MARKER.\nGood for 40 in silver against the bearer.\nPayable to N. PURCELL.',
        lines=[(ELLIS, "Ned's paper."),
               (HOB, 'Small world.')]),
    # The Last Watch, optional: on the cabin table (SM_RR_RangerCabin's Journal socket).
    'RuthJournal': dict(
        title="Ruth Calder's journal", heading='On the cabin table', prompt='Read',
        text=("The night of the robbery. Skiff came up the canyon at dusk, running dark. Lights on Ransom's Point. A shot "
              'on the bluff path, then one up top. The Ransom kid drew on the Deacon at the last. Saw it through the glass. '
              "Doesn't wash out ten years. Rode the north trail down and round by the rail. Dawn before I got there. Closed "
              "the kid's eyes myself. Wasn't sorry.\n\n"
              "Last entry. Riding south after them. If the kid's ghost is reading this: I'd have hanged you anyway. "
              'Probably.\n\n' + DASH + ' R.C.'),
        lines=[(HOB, '...She closed your eyes, sunshine. Not everybody would have bothered.')]),
    # The Last Watch: reading it is the step (the cabin's CodeBook socket); it decodes the far light.
    'CodeBook': dict(
        title="Ruth's code book", heading="The Rim Rangers' signal code", prompt='Read',
        text=('The far light, read off letter by letter:\n\n'
              'LARKSPUR WE SEE YOU.\nCALDER RIDES SOUTH.\nALL POSTS: WORD IS THE RANSOM KID WALKS. SHOOT ON SIGHT.'),
        lines=[(ELLIS, 'Word travels.'),
               (HOB, 'Faster than you do.')]),
    # The Last Watch, optional: the post's log on the cabin table (PostLog socket); reading it signs it.
    'PostLog': dict(
        title="The post's log", heading='Larkspur Post, Rim Rangers', prompt='Sign',
        text=('T. ABERNATHY. Post held. Wind from the west.\n'
              'T. ABERNATHY. Post held. Cough no better.\n'
              'R. CALDER. Tom buried by the cabin. Post held.\n'
              'R. CALDER. Post unmanned. Gone south after the Dunne Gang.'),
        lines=[(ELLIS, 'Larkspur stands.'),
               (HOB, 'Use your own name, sunshine. Bold.')]),
    # Back Pay: reading it is the step (SM_RR_Mine_Office's PayrollBook socket).
    'PayrollBook': dict(
        title="The Foreman's payroll", heading='Providence Mining Co.', prompt='Read',
        text=('PAYROLL, LOWER DRIFT. THREE MONTHS OWING.\n'
              'No. 1 S. KESTLE   No. 2 A. BRANNOCK   No. 3 H. VOSS   No. 4 D. MCCREE\n'
              'No. 5 P. ORRIN   No. 6 T. LANE   No. 7 J. ODELL   No. 8 W. FARRIS\n'
              'No. 9 E. COOMBE   No. 10 R. DACEY   No. 11 N. HOLT\n\n'
              'COMPANY ORDER: Break the strike.\n\n'
              "They've barricaded the lower drift. Say they'll come up when they're paid. A small charge on the drift "
              'mouth will bring them up.\n' + DASH + ' C. Teague'),
        lines=[(HOB, "That's the trouble with small charges.")]),
    # Back Pay, optional: chalk on the upper gallery's wall (SM_RR_Mine_Gallery's ChalkTally socket).
    'ChalkTally': dict(
        title='Chalk on the gallery wall', heading='The upper gallery', prompt='Read',
        text='DAY 1. SHIFT OF 11.\nDAY 2. AIR BAD.\nDAY 4. WATER GONE.\nDAY 6. TEAGUE WILL DIG US OUT.',
        lines=[(HOB, "...He didn't, sunshine.")]),
    # Back Pay: reading it is the step (SM_RR_Mine_Barricade's Barricade socket): the eleven's tags come off its nail.
    'Barricade': dict(
        title="The eleven's barricade", heading='Timbers across the lower drift', prompt='Take the tags',
        text='Chalked across the timbers:\n\nPAY US AND WE COME UP.\n\nEleven brass tags hang from a nail.',
        lines=[(ELLIS, 'Eleven tags.'),
               (HOB, 'And a Foreman who never paid them. Listen, sunshine. Digging.')]),
    # Calder's Caches: reading it is the step; it lies with the gang's strongbox in the sheriff's lockup (on the banknotes
    # inside, or on the shelf beside it), there from the start, so it can be read again once the mission asks.
    'DeaconsSlip': dict(
        title="A slip in the Deacon's hand", heading='Folded in with the banknotes', prompt='Read',
        text="Kid's share.\nFor when the kid comes home.\n\n" + DASH + ' A.D.',
        lines=[(ELLIS, 'He kept my share.'),
               (HOB, '...Sentimental, for a man who shot you.')]),
}

# The props' own models (Art/Models/Props/MissionProps.py, imported to /Game/Art/Props/) for the readables that bring one;
# the rest are read off the dressing they're on (a headboard, a slate, a barn door, a wall, a desk).
READABLE_MODELS = {
    'KeepersRoll': 'SM_KeepersRoll', 'SheriffHat': 'SM_SheriffHat', 'SheriffBadge': 'SM_SheriffBadge',
    'RuthJournal': 'SM_Journal', 'CodeBook': 'SM_CodeBook', 'PostLog': 'SM_PostLog', 'PayrollBook': 'SM_PayrollBook',
    'DeaconsSlip': 'SM_DeaconsSlip',
}

# ---------------------------------------------------------------------------
# The porch plate (APorchPlate, on the farmhouse's porch table)
# ---------------------------------------------------------------------------

PORCH_PLATE = dict(
    take_prompt='Take the jars', eat_prompt='Eat the biscuits',
    # The slip under the jars, read as they're taken (two grave-salt grenades).
    take_lines=[("Delia's slip", "SALT FOR THE ROAD. THEY DON'T CARE FOR IT. " + DASH + ' D.')],
    # Delia through the door, as the biscuits go (Main 7's optional).
    eat_lines=[('Grandma Delia', '...Thank you, Ellis.')],
    take_when=dict(during='Main1', from_step=4),
    eat_when=dict(during='Main7'),
)

# ---------------------------------------------------------------------------
# Artifacts (AMissionArtifact): their cards show their mission's briefing (Text left empty)
# ---------------------------------------------------------------------------

ARTIFACTS = {
    # Side 1: the poster on the town gate's south post (a handbill, beside the tearable posterGate decal).
    'Artifact_PosterGate': dict(title='Your wanted poster', prompt='Read the poster', text='', model='',
                                take_on_accept=False),
    # Calder's Caches: Ruth Calder's note, pinned on the Rim Rangers' board by the sheriff's office.
    'Artifact_CalderNote': dict(title="Ruth Calder's note", prompt='Read the note', text='', model='', take_on_accept=False),
    # Back Pay: No. 7's brass tag on the IN side of the Providence's tag board (SM_RR_TagBoard_TagsIn). Its card opens
    # on "No. 7. J. ODELL. IN.", the mission's briefing's first words.
    'Artifact_MinersTag': dict(title="A miner's brass tag", prompt='Take the tag', text='', model='SM_MinersTag',
                               take_on_accept=True),
}

# ---------------------------------------------------------------------------
# Mission items (AMissionItem): where they lie and when they're there (their names and lines are the missions')
# ---------------------------------------------------------------------------

ITEMS = {
    # Main 6's item (Main 3's own items vanish as it's turned in): Tilly puts it out on her sill at Main 3's turn-in, and
    # it lies there until it's taken; Main 6's first step asks for it, so most players carry it by then.
    'LanternBail': dict(mission='Main6', where="on Tilly's window sill (Bright & Daughter's Speaker socket, in front)",
                        prompt="Take Pa's lantern bail", shown_when=dict(after=['Main3'])),
    # Shown from the story's start, so the bluff path's (by the crate Main 2 opens) can be taken then and counts later.
    'CacheTag1': dict(mission='CaldersCaches', where="on the windmill's Ranger cache (RangerCache_Windmill's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'CacheTag2': dict(mission='CaldersCaches', where="on the Sink rim's Ranger cache (RangerCache_SinkRim's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'CacheTag3': dict(mission='CaldersCaches', where="on the bluff path's Ranger cache (RangerCache_BluffPath's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'FennsFare': dict(mission='OldFare', where="on the landing under Mr. Sloane's door (SM_RR_Depot's AgentDoor socket)",
                      prompt="Take Fenn's coins", shown_when=dict(during='OldFare', from_step=4)),
    'CellarKey': dict(mission='KnockingBelow', where="on Thaddeus's measuring hook (SM_RR_Workshop_Bright's MeasuringHook)",
                      prompt='Take the cellar key', shown_when=None),
    'ThaddeusTape': dict(mission='KnockingBelow', where="on Thaddeus's bench in the cold room (ThaddeusBench socket)",
                         prompt="Take Thaddeus's tape measure", shown_when=dict(during='KnockingBelow', from_step=6)),
    'WinchCrank': dict(mission='DeadLift', where="in the sheriff's lockup, by Odell's report", prompt='Take the winch crank',
                       shown_when=None),
    'CoalSack': dict(mission='DeadLift', where="at the depot's coal stage (SM_RR_CoalStage's Sack socket)",
                     prompt='Fill a coal sack', shown_when=None),
    'ClampPin': dict(mission='DeadLift', where="dropped where Abe Haskett falls (the lift crew's Restless brakeman, tagged "
                                               'AbeHaskett)', prompt='Take the clamp pin', shown_when=None),
    'LampOil': dict(mission='LastWatch', where="in Ruth's cabin (SM_RR_RangerCabin's LampOil socket)",
                    prompt='Take the lamp oil', shown_when=None),
    'GravemotherFang': dict(mission='Side3', where='dropped where the Gravemother falls, the first time',
                            prompt='Take her fang', shown_when=None),
    # Only once the Foreman is down (Back Pay's sixth step on), so it can't be taken past him.
    'Payroll': dict(mission='BackPay', where="the payroll chest in the stope (SM_RR_Mine_Stope's PayrollChest socket), "
                                             'tagged PayrollChest too', prompt='Take the payroll',
                    shown_when=dict(during='BackPay', from_step=5)),
}

# ---------------------------------------------------------------------------
# Speaker points: what each says when (first that holds; the default is the point's own line set)
# ---------------------------------------------------------------------------

TOPICS = {
    # build_area_story.place_tilly: her talk is Main 3's turn-in (from its step 3, the step past the last of its three; the
    # mission's turn-in lines say the same), her Main 7 words know her father.
    'Speaker_Tilly': dict(default='DA_Lines_TillyClosed', topics=[
        (dict(during='Main3', from_step=3), 'DA_Lines_TillyMain3'),
        (dict(during='KnockingBelow'), 'DA_Lines_TillyKnocking'),
        (dict(during='Main7', after=['KnockingBelow']), 'DA_Lines_TillyMain7Father'),
        (dict(during='Main7'), 'DA_Lines_TillyMain7'),
        (dict(after=['Main7']), 'DA_Lines_TillyAfterMain7'),
        (dict(after=['KnockingBelow']), 'DA_Lines_TillyAfterKnocking'),
        (dict(after=['Main3']), 'DA_Lines_TillyAfterMain3'),
    ]),
    # build_area_chapel: the knock is Main 4's third step (2); the bell and the Reliquary follow (3, 4); his Main 4 words
    # are the mission's turn-in lines now (AldanaMain4 is no topic). The Reliquary's look_when: during Main4 from_step 4.
    'Speaker_Aldana': dict(default='DA_Lines_AldanaBarred', topics=[
        (dict(during='Main4', from_step=3), 'DA_Lines_AldanaBell'),
        (dict(during='Main4', from_step=2), 'DA_Lines_AldanaWaiting'),
        (dict(after=['Main5']), 'DA_Lines_AldanaAfterMain5'),
        (dict(after=['Main4']), 'DA_Lines_AldanaAfterMain4'),
    ]),
    # build_area_whitlock: the bales are Side 2's first step now (LOAD_STEP 0, their load_when from_step 0), the road talk
    # its second (1), the hands its third (FIGHT_STEP 2, as before), the thanks the turn-in (THANKS_STEP 3).
    'Speaker_Amos': dict(default='DA_Lines_AmosMeet', topics=[
        (dict(during='Side2', from_step=3), 'DA_Lines_AmosThanks'),
        (dict(during='Side2', from_step=2), 'DA_Lines_AmosHands'),
        (dict(during='Side2', from_step=1), 'DA_Lines_AmosRoad'),
        (dict(during='Side2'), 'DA_Lines_AmosBales'),
        (dict(after=['Side2', 'Main6']), 'DA_Lines_AmosAfterMain6'),
        (dict(after=['Side2']), 'DA_Lines_AmosFence'),
    ]),
    # build_area_story.place_hob: his talk, the side missions after the main ones (the story first).
    'Speaker_Hob': dict(default=None, topics=[
        (dict(during='Main7'), 'DA_Lines_HobMain7'),
        (dict(during='Main6'), 'DA_Lines_HobMain6'),
        (dict(during='Main5'), 'DA_Lines_HobMain5'),
        (dict(during='Main4'), 'DA_Lines_HobMain4'),
        (dict(during='Main3'), 'DA_Lines_HobMain3'),
        (dict(during='Main2'), 'DA_Lines_HobMain2'),
        (dict(during='Main1'), 'DA_Lines_HobMain1'),
        (dict(during='BackPay'), 'DA_Lines_HobBackPay'),
        (dict(during='LastWatch'), 'DA_Lines_HobLastWatch'),
        (dict(during='DeadLift'), 'DA_Lines_HobDeadLift'),
        (dict(during='Side3'), 'DA_Lines_HobSide3'),
        (dict(during='KnockingBelow'), 'DA_Lines_HobKnocking'),
        (dict(during='OldFare'), 'DA_Lines_HobOldFare'),
        (dict(during='CaldersCaches'), 'DA_Lines_HobCaches'),
        (dict(during='Side1'), 'DA_Lines_HobSide1'),
        (dict(after=['Main1']), 'DA_Lines_HobAfterMain1'),
    ]),
    # The Old Fare's speakers, as build_area_missionprops_valley.py places them (its LINES names; it finds the steps from
    # the mission asset: the talk 3, the look the first Interact FennCoffin 2, the lay the last 5).
    # Mr. Sloane's upstairs door in the depot (SM_RR_Depot's AgentDoor socket): shown from Main 3 until the Old Fare is
    # done (its offer, then its talk), then he's at his ticket window instead.
    'Speaker_Agent': dict(default='DA_Lines_AgentDoor', name='Mr. Sloane',
                          shown_when=dict(after=['Main3'], before=['OldFare']), topics=[
        (dict(during='OldFare', from_step=3), 'DA_Lines_AgentCoins'),
        (dict(during='OldFare'), 'DA_Lines_AgentDoor'),
    ]),
    # His ticket window (SM_RR_Depot's TicketWindow socket), from the Old Fare's end; the coal while Dead Lift runs.
    'Speaker_AgentWindow': dict(default='DA_Lines_AgentWindow', name='Mr. Sloane', shown_when=dict(after=['OldFare']),
                                topics=[
        (dict(after=['OldFare', 'Main7']), 'DA_Lines_AgentWindowLily'),
        (dict(after=['OldFare'], during='DeadLift'), 'DA_Lines_AgentWindowCoal'),
        (dict(after=['OldFare']), 'DA_Lines_AgentWindow'),
    ]),
    # Josiah Fenn's coffin in the baggage room (BaggageCoffin socket): one tap actor (tagged Speaker_Fenn and FennCoffin),
    # looked into from the Old Fare's step 2, the coins laid back at its step 5, its turn-in; then a word (the placing
    # script may add it).
    'Speaker_Fenn': dict(default=None, name='Josiah Fenn', missions_only_before=['OldFare'], topics=[
        (dict(during='OldFare', from_step=5), 'DA_Lines_FennCoins'),
        (dict(during='OldFare', from_step=2), 'DA_Lines_FennLook'),
        (dict(after=['OldFare']), 'DA_Lines_FennAfter'),
    ]),
    # AThaddeusBright at his bench in the cold room (shown after Main 4); before his talk the placing script's own words.
    'Speaker_Thaddeus': dict(default='DA_Lines_ThaddeusAfter', name='Thaddeus Bright', topics=[
        (dict(after=['KnockingBelow', 'Main7']), 'DA_Lines_ThaddeusAfterMain7'),
        (dict(after=['KnockingBelow']), 'DA_Lines_ThaddeusAfter'),
        (dict(during='KnockingBelow', from_step=5), 'DA_Lines_ThaddeusTalk'),
    ]),
    # AHiramCoyle on his stool in the winch house (shown after Main 5).
    'Speaker_Hiram': dict(default='DA_Lines_HiramAfter', name='Hiram Coyle', topics=[
        (dict(during='DeadLift', from_step=3), 'DA_Lines_HiramRide'),
        (dict(during='DeadLift', from_step=2), 'DA_Lines_HiramSteam'),
        (dict(during='DeadLift', from_step=1), 'DA_Lines_HiramFit'),
        (dict(during='DeadLift'), 'DA_Lines_HiramWaiting'),
        (dict(after=['DeadLift']), 'DA_Lines_HiramAfter'),
    ]),
    # The lift head's speaking tube (build_area_lift.py): Dead Lift's turn-in, then Hiram's word.
    'Speaker_LiftTube': dict(default=None, name='Hiram Coyle', missions_only_before=['DeadLift'], topics=[
        (dict(after=['DeadLift']), 'DA_Lines_LiftTubeAfter'),
    ]),
    # The Providence's tag board (missions only: Back Pay's turn-in and its choice); the shift's voice after a kept payroll.
    'Speaker_TagBoard': dict(default=None, name='The shift', missions_only_before=['BackPay'], topics=[
        (dict(after=['BackPay'], with_flags=['BackPay.Kept']), 'DA_Lines_TagBoardKept'),
    ]),
}

MUTTERS = {
    # (Mr. Sloane's word about the coal is his window's topic during Dead Lift now: DA_Lines_AgentWindowCoal.)
    # Hiram from the winch house as the lift crew rise (Dead Lift's first step).
    'Speaker_Hiram': [(dict(during='DeadLift', before_step=1), 'DA_Lines_HiramCrew')],
    # The Haskett house on Main Street once the lift runs (a town-life household's mutter).
    'Haskett': [(dict(after=['DeadLift']), 'DA_Lines_HaskettLift')],
}

PERCHES = {
    # On Keeper's Bridge as Main 6 begins, for a player taking the keeper's path (its optional).
    'keepersBridge': (dict(during='Main6', before_step=1), 'DA_Lines_HobMain6Bridge'),
    # On the Providence's tag board as Back Pay begins.
    'mineMouth': (dict(during='BackPay', before_step=1), 'DA_Lines_HobBackPayTags'),
}

# When each mission fight is on (an encounter's active_when): the missions count their kills from the spawner, so a fight
# cleared before its step began still counts.
ENCOUNTERS = {
    'DepotWaiting': dict(during='OldFare', before_step=1),
    'TillyCellar': dict(during='KnockingBelow', from_step=2, before_step=4),
    'QuarryCrew': dict(during='DeadLift', before_step=1),
    # Abe Haskett, the crew's Restless brakeman with the clamp pin, risen with them.
    'AbeHaskett': dict(during='DeadLift', before_step=1),
    'RaisingSteam': dict(during='DeadLift', from_step=2, before_step=3),
    # Three spiders on the crest path, there from the first ride (optional).
    'CrestPatrol': dict(after=['DeadLift']),
    'CalderPost': dict(after=['DeadLift']),
    'Watch': dict(during='LastWatch', from_step=3, before_step=4),
    'SpoilAmbush': dict(after=['DeadLift']),
    'DinnerPail': dict(after=['DeadLift']),
    'MineUpper': dict(),
    'MineLower': dict(),
    'Foreman': dict(),
}


def condition(spec):
    """An unreal.StoryCondition from a condition dict (None or {}: always)."""
    import unreal
    spec = spec or {}
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in spec.get('after', ())])
    made.set_editor_property('before_missions', [unreal.Name(m) for m in spec.get('before', ())])
    made.set_editor_property('with_flags', [unreal.Name(f) for f in spec.get('with_flags', ())])
    made.set_editor_property('without_flags', [unreal.Name(f) for f in spec.get('without_flags', ())])
    made.set_editor_property('during_mission', unreal.Name(spec.get('during', '')))
    made.set_editor_property('from_step', spec.get('from_step', 0))
    if spec.get('before_step'):
        made.set_editor_property('before_step', spec['before_step'])
    return made


def story_lines(pairs):
    """unreal.StoryLine list from (speaker, words) pairs; an empty speaker is whoever's point (or title) says it."""
    import unreal
    made = []
    for speaker, words in pairs:
        line = unreal.StoryLine()
        line.set_editor_property('speaker', unreal.Text(speaker))
        line.set_editor_property('text', unreal.Text(words))
        made.append(line)
    return made


def apply_readable(prop, readable_id):
    """Copies READABLES[readable_id] onto a placed unreal.ReadableProp (its id, words, lines, prompt and when)."""
    import unreal
    spec = READABLES[readable_id]
    prop.set_editor_property('readable_id', unreal.Name(readable_id))
    prop.set_editor_property('title', unreal.Text(spec['title']))
    prop.set_editor_property('heading', unreal.Text(spec.get('heading', '')))
    prop.set_editor_property('text', unreal.Text(spec.get('text', '')))
    prop.set_editor_property('prompt', unreal.Text(spec.get('prompt', '')))
    prop.set_editor_property('lines', story_lines(spec.get('lines', ())))
    if spec.get('shown_when'):
        prop.set_editor_property('shown_when', condition(spec['shown_when']))
    return prop
