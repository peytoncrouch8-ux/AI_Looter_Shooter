"""The missions' words that live on things in the level, as data for the build scripts that place those things
(Docs/Areas/RansomsRest_Expansion.md sections 3-4; the missions' data step, 2026-10-13). Nothing here is made or placed:
a build script imports this module and copies what it needs onto the actor it places. The missions themselves (their
offers, steps, turn-ins and choices) are Tools/Unreal/create_mission_assets.py's and create_side_mission_assets.py's; the
line sets the topics below name are Tools/Unreal/create_story_lines.py's.

  READABLES   every AReadableProp's words by readable id (its title, heading, text, prompt and the lines said after
              reading it), when it's there (shown_when, a condition as below; none: always) and the story flag reading
              it sets (sets_flag). apply_readable() copies them onto a placed prop; READABLE_MODELS names the ones that
              bring a model of their own.
  LEDGER_TOWNS the seven towns the Dunne Gang robbed, one loose Ledger page each (the Rest's seeds, 2026-10-16): the
              page's words, its saint, where it lies on Ransom's Rest. The six found here are READABLES
              (LedgerPage_<Town>, placed by build_area_seeds.py); all seven are bestiary pages
              (create_bestiary_pages.py, open once the page's flag Ledger.Page.<Town> is set).
  PORCH_PLATE the porch plate's (APorchPlate) prompts, the slip read as the jars are taken, Delia's answer as the biscuits
              are eaten, and its two conditions.
  ARTIFACTS   each AMissionArtifact's title, prompt and whether it goes once its mission is taken (its card's words are
              its mission's briefing).
  ITEMS       each AMissionItem's mission, where it lies, its prompt and when it's there (the missions' own FMissionItemInfo
              names and Carried lines are in the mission scripts).
  TOPICS      what each speaker point says when, by its actor tag: (condition, line set) pairs, the first that holds is
              said (build_area_story.topics() takes them as they are once each condition is made with condition()). A
              condition dict may carry sets_flag: the story flag the topic sets when it's said (FSpeakerTopic::SetsFlag),
              so with that flag in its without_flags it's a talk given once (Aldana's Ada.Told).
  MUTTERS     lines a speaker point says unasked as the player passes, with when.
  PERCHES     Hob's new perches' arrival lines (the spot is the placing script's), with when.
  ENCOUNTERS  when each mission fight is on (an encounter spawner's active_when), by its spawner id.
  SECRETS     the things to find off the path (2026-10-21): each one's chest, its loot, its anchor and its readable
              (build_area_secrets.py places them).

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
               (HOB, 'Your grandmother. She did the letters twice.')]),
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
        lines=[(ELLIS, "Wade Odell's badge. He gave me a hiding once, for stealing apples.")]),
    # Dead Lift, optional: pinned up in the sheriff's lockup, by the winch crank.
    'OdellReport': dict(
        title="Sheriff Odell's report", heading='Pinned up in the lockup', prompt='Read',
        text=('PROVIDENCE MINING CO. THE FALL, LOWER DRIFT.\n\n'
              "Eleven men. Teague swears the roof gave on its own. Then why'd he have powder burns on his cuffs?\n\n"
              'My brother Jory was one of the eleven.\n\n' + DASH + ' W.O.'),
        lines=[]),
    # Side 1, optional: the sheriff's daybook, open on his desk in the office.
    'OdellDaybook': dict(
        title="The sheriff's daybook", heading='Open on his desk', prompt='Read',
        text=('Haskett mule gone. Tracks to the Sink.\n'
              'My dog gone. Same tracks.\n'
              'Gone down the Sink after the big one. Back by supper.\n\n' + DASH + ' W.O.'),
        lines=[]),
    # Side 2, optional: chalked inside the Whitlock barn's door (SM_RR_Barn_Whitlock's WagesChalk socket).
    'WagesChalk': dict(
        title='What Amos owed', heading='Chalked inside the barn door', prompt='Read',
        text='OWED\n\nMERRITT  14.00\nMERRITT  14.00\nTANNER  11.50',
        lines=[(ELLIS, 'Three names. Nothing crossed out.')]),
    # Knocking Below, optional: the coffin orders on the slate by the workshop door (OrdersSlate socket).
    'OrdersSlate': dict(
        title='Coffin orders', heading='Chalk on the slate by the workshop door', prompt='Read',
        text=('DOLAN. PINE. 6 FT.\nAVERY. PINE. 5 FT 5.\nPECK. PINE. 5 FT 10.\nLOCKETT. OAK. 5 FT 2.\n\n'
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
        lines=[(ELLIS, 'Larkspur stands.')]),
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
        lines=[]),
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
               (HOB, 'A Foreman who never paid them. Listen. Digging.')]),
    # Calder's Caches: reading it is the step; it lies with the gang's strongbox in the sheriff's lockup (on the banknotes
    # inside, or on the shelf beside it), there from the start, so it can be read again once the mission asks.
    'DeaconsSlip': dict(
        title="A slip in the Deacon's hand", heading='Folded in with the banknotes', prompt='Read',
        text="Kid's share.\nFor when the kid comes home.\n\n" + DASH + ' A.D.',
        lines=[(ELLIS, 'He kept my share.'),
               (HOB, '...Sentimental, for a man who shot you.')]),

    # --- The Rest's seeds (Docs/Story.md "Seeds in Ransom's Rest"; build_area_seeds.py places them) ---
    # The Deacon's letter never sent, wedged in the rock by the gang's mooring post (the Story's first line, word for word).
    'DeaconLetter_Cleft': dict(
        title="A letter in the Deacon's hand", heading='Folded small, wedged in a crack by the mooring post. Never sent.',
        prompt='Read', sets_flag='Letter.Deacon.Cleft',
        text=("Kid. The man who pays us isn't a man.\n"
              "He paid us in coin off dead men's eyes, and I took it. God help me, I'd take it again.\n"
              "Six towns now, and he wants yours worst of all. I can't tell you why. I'm afraid I'll find out.\n"
              "When we're done here, take your share and ride. Don't come looking for me.\n\n" + DASH + ' A.D.'),
        lines=[(ELLIS, 'Six towns. Then mine.'),
               (HOB, 'Never sent. Left where you tied up, though.')]),
    # The Deacon's letter to Delia in the saloon's post rack, mailed before the robbery and never collected: his warning.
    'DeaconLetter_Saloon': dict(
        title='A letter in the post rack', heading='To Mrs. D. Ransom, Ransom Farm. Postmarked Halfway Junction, 9 August. '
                                                  'Unopened.',
        prompt='Read', sets_flag='Letter.Deacon.Saloon',
        text=("Mrs. Ransom,\n\n"
              "You don't know my hand. You know my name, and you've cause to curse it.\n\n"
              "Get Abel and his lantern off the Rest before the month is out. Your sister's, anywhere. Don't ask me why. "
              "I can't put it in a letter.\n\n"
              "The kid is well. The kid doesn't know, and I'd sooner it stayed that way.\n\n"
              "Burn this.\n\n" + DASH + ' A. Dunne'),
        lines=[(ELLIS, 'He tried to warn her.'),
               (HOB, 'Tried. Then he came anyway. Mind the difference, sunshine.')]),
    # The smashed Reliquary's lid after Hallowed Ground (no model: the lid prised askew is the Reliquary's own).
    'ReliquaryLid': dict(
        title="The Reliquary's lid", heading='Prised up at one edge', prompt='Look', text='',
        shown_when=dict(after=['Main4']),
        lines=[(ELLIS, 'A mark under the lid. A coin, with a line through it.'),
               (HOB, 'The old toll mark. Somebody made sure she never paid it. Or anyone after her, sunshine.')]),
    # The railroad's map on the ticket office's wall over the agent's desk (SM_ReachesMap).
    'ReachesMap': dict(
        title='The Reaches', heading='A railroad map, pinned over the agent\'s desk', prompt='Read',
        text=("THE REACHES. THE RAILROAD AND ITS TOWNS.\n\n"
              "Every line runs in to HALFWAY JUNCTION, inked heavy at the heart of the sheet.\n\n"
              "RANSOM'S REST, on the Rim at the west edge.\n"
              "HIGHWATER LANDING: END OF THE LINE.\n"
              "DUSTWATER MESA. SAINT AGNES MIRE. FURNACE HOLLOW. THE NEEDLES. STONEWEIGHT QUARRY.\n\n"
              "Seven towns, each with a little sun inked by its name: a saint."),
        lines=[(HOB, 'Seven saints, one railroad. Every line runs to the Junction, sunshine.')]),
    # What Ada gave up (the Rest's seeds; a page since 2026-10-21, the user's cut of the story's talk): Father Aldana's
    # words, slid out under the vestry door after Hallowed Ground (build_area_secrets.py places it); reading it sets
    # Ada.Told as his talk did.
    'AldanaPage': dict(
        title="A page in Father Aldana's hand", heading='Slid out under the vestry door', prompt='Read',
        sets_flag='Ada.Told', shown_when=dict(after=['Main4']),
        text=("There's a thing the Rest has forgotten.\n\n"
              "The first winter, nobody on the Rest had coin for the old fare. Their dead lay unpaid.\n\n"
              "Ada lay down on the first board on Gravewind Point and asked the wind to take her. It carried her to the "
              "edge of the world.\n\n"
              "And there she stopped, and held up her light, so nobody of this valley would pay to cross again.\n\n"
              "She never crossed herself. No saint does. Every one of them is somebody who stayed behind.\n\n"
              + DASH + ' M.A.'),
        lines=[(ELLIS, 'Sixty years at the edge of the world.'),
               (HOB, 'Some debts you pay standing up, sunshine.')]),

    # --- Things to find off the path (2026-10-21; SECRETS below places their chests): each found readable sets its
    # Secret.<Id> flag. Only facts the story already has (Docs/Story.md): strongboxes are the gang's old bank jobs'
    # safes, spiders hoard what a saint has touched, Pike surveyed the Reaches (the map's "W. PIKE, SURVEYOR"), Teague's
    # powder burns (Odell's report) and his small charge (the payroll), Thaddeus's own habits.
    'Secret_WidowsBox': dict(
        title='A strongbox in the rock', heading='Wedged under the middle Widow', prompt='Look', text='',
        sets_flag='Secret.WidowsBox',
        lines=[(HOB, "A bank job's strongbox. Somebody meant to come back for it, sunshine.")]),
    'Secret_BenchMark': dict(
        title='A brass bench mark', heading="Set in the rock on Coffin Rock's top", prompt='Read',
        sets_flag='Secret.CoffinRock',
        text='REACHES RAILROAD SURVEY.\nBENCH MARK 112.\nW. PIKE, SURVEYOR.',
        lines=[(ELLIS, 'You can see the whole Rest from up here.'),
               (HOB, 'Every map starts with somebody standing on a rock, sunshine.')]),
    'Secret_Bottle': dict(
        title='A bottle under a stone', heading='In the springhouse, under the flat stone', prompt='Look',
        text='EMBALMING FLUID, says the label.', sets_flag='Secret.Bottle',
        lines=[(HOB, "Rye, by the smell. Don't tell her.")]),
    'Secret_Hoard': dict(
        title="A spider's hoard", heading='Webbed up under the hanging tree', prompt='Look', text='',
        sets_flag='Secret.Hoard',
        lines=[(HOB, 'Spiders hoard what a saint has touched, sunshine. Even a graveyard\'s leavings.')]),
    'Secret_Powder': dict(
        title='A powder slip', heading="In the company's powder box", prompt='Read', sets_flag='Secret.Powder',
        text='PROVIDENCE MINING CO. POWDER DRAWN.\n6 STICKS. FOR THE LOWER DRIFT.\n\n' + DASH + ' C.T.',
        lines=[(ELLIS, "Six sticks. The roof didn't give on its own."),
               (HOB, 'Somebody tell Wade, sunshine.')]),
    'Secret_Crate': dict(
        title='A fallen crate', heading='In the quarry blocks, under the derrick', prompt='Look', text='',
        sets_flag='Secret.Crate',
        lines=[(HOB, 'Fell off the boom. The company never came for it.')]),
}

# The seven towns the Dunne Gang robbed, in the order the Keeper's Lantern finds their embers (the seven names' order),
# Ransom's Rest last (the Deacon's). Each loose page is Sexton's entry on its town: the saint, the fares the town's dead
# crossed free since the saint was lit (owing at the old fare, a coin on each eye), the robbery, and a note in the margin.
# The Rest's page is dated a month before its robbery, with the Revenant's name already entered (Ellis's variant: the
# hero pick changes the name). found: where it lies on Ransom's Rest (None: Stoneweight Quarry's, found later in the
# campaign). The flag reading it sets opens its bestiary page.
LEDGER_TOWNS = {
    'HighwaterLanding': dict(
        name='Highwater Landing', saint='Saint Hester', carrier='Lucky Ned Purcell',
        habitat='The Rim, where the railroad ends: the Gilded Lily ties up over the canyon',
        found="In the depot's baggage room on Ransom's Rest, on a trunk",
        heading='On a trunk in the baggage room',
        text=('HIGHWATER LANDING\n'
              'Saint Hester, who staked the landing. Lit nineteen years since.\n'
              'Fares carried free since: 1,102 souls.\n'
              'Owing, at two coins the soul: 2,204.\n\n'
              'Collected 3 March, by the Dunne Gang. Keeper settled first.\n'
              'Carried by N. Purcell.\n\n'
              'In the margin: Purcell will keep it by the cards and call it luck. Let him.'),
        lines=[(HOB, "End of the line, where the Lily ties up. Purcell's luck lives there now.")]),
    'DustwaterMesa': dict(
        name='Dustwater Mesa', saint='Saint Ezra', carrier='Whistling Ira Gale',
        habitat='Wind-scoured mesas joined by rope bridges',
        found="In Ruth Calder's cabin at Calder's Post, on the table",
        heading="On Ruth's table, under the lamp",
        text=('DUSTWATER MESA\n'
              'Saint Ezra, who dug the cistern. Lit seventy years since.\n'
              'Fares carried free since: 2,951 souls.\n'
              'Owing, at two coins the soul: 5,902.\n\n'
              'Collected 1 April, by the Dunne Gang. Keeper settled first.\n'
              'Carried by I. Gale.\n\n'
              'In the margin: Gale asked what the coin was. Old money, I told him. True.'),
        lines=[(ELLIS, 'Two coins the soul.'),
               (HOB, 'He counts careful. Always has.')]),
    'SaintAgnesMire': dict(
        name='Saint Agnes Mire', saint='Saint Agnes', carrier='Sister Constance Holloway',
        habitat='The river delta: a mission church among the cypress and black water',
        found="On the ledge by the gang's mooring post, under Ransom's Point",
        heading='On the ledge by the mooring post',
        text=('SAINT AGNES MIRE\n'
              'Saint Agnes, who built the mission. Lit a hundred and four years since.\n'
              'Fares carried free since: 9,630 souls.\n'
              'Owing, at two coins the soul: 19,260.\n\n'
              'Collected 9 May, by the Dunne Gang. Keeper settled first.\n'
              'Carried by C. Holloway.\n\n'
              'In the margin: The Sister means to be merciful with it. Mercy keeps the dead close. Useful.'),
        lines=[(HOB, '"Useful." He does like that word, sunshine.')]),
    'FurnaceHollow': dict(
        name='Furnace Hollow', saint='Saint Maud', carrier='Barrels Kessler',
        habitat='A smelter town of chimneys, slag and ore carts',
        found="Pinned on the Providence's tag board, under the IN hooks",
        heading='Pinned on the tag board, under the IN hooks',
        text=('FURNACE HOLLOW\n'
              'Saint Maud, the smith. Her Reliquary cast iron, at her asking. Lit fifty years since.\n'
              'Fares carried free since: 5,412 souls.\n'
              'Owing, at two coins the soul: 10,824.\n\n'
              'Collected 14 June, by the Dunne Gang. Keeper settled first.\n'
              'Carried by B. Kessler.\n\n'
              'In the margin: Iron holds a light well. Note it for the lamp.'),
        lines=[(ELLIS, 'What lamp?'),
               (HOB, "Couldn't say, sunshine. Couldn't say.")]),
    'TheNeedles': dict(
        name='The Needles', saint='Saint Corliss', carrier='Lena Okoro',
        habitat='Stone spires joined by rope bridges',
        found="On the floor of the sheriff's lockup on Ransom's Rest, by the bunk",
        heading='On the cell floor, by the bunk',
        text=('THE NEEDLES\n'
              'Saint Corliss, who strung the first bridge. Lit thirty-eight years since.\n'
              'Fares carried free since: 1,480 souls.\n'
              'Owing, at two coins the soul: 2,960.\n\n'
              'Collected 12 July, by the Dunne Gang. Keeper settled first.\n'
              'Carried by L. Okoro.\n\n'
              'In the margin: Okoro sees too far. Keep her off the top spire at dusk.'),
        lines=[(ELLIS, 'Why keep her off the spire?'),
               (HOB, "Some views aren't good for a body, sunshine.")]),
    'StoneweightQuarry': dict(
        name='Stoneweight Quarry', saint='Saint Gideon', carrier='Tobias Grant',
        habitat='A terraced quarry: cranes, and stone hanging over the drop',
        found=None,
        heading='',
        text=('STONEWEIGHT QUARRY\n'
              'Saint Gideon, who cut the first terrace. Lit fifty-six years since.\n'
              'Fares carried free since: 4,077 souls.\n'
              'Owing, at two coins the soul: 8,154.\n\n'
              'Collected 2 August, by the Dunne Gang. Keeper settled first.\n'
              'Carried by T. Grant.\n\n'
              'In the margin: Grant did not want it. He carried it all the same. Carrying is what he is for.'),
        lines=[]),
    'RansomsRest': dict(
        name="Ransom's Rest", saint='Saint Ada', carrier='the Deacon, Ambrose Dunne',
        habitat='The Rim: a high farm valley over Gravewind Canyon',
        found="On the step outside the vestry door of the Chapel of Saint Ada",
        heading='On the step outside the vestry door',
        text=("RANSOM'S REST\n"
              'Saint Ada, who lay down on the first board. Lit sixty-one years since.\n'
              'Fares carried free since: 2,207 souls.\n'
              'Owing, at two coins the soul: 4,414.\n\n'
              'To be collected 21 August, at dusk, by the Dunne Gang. Guided in by E. Ransom.\n'
              'Keeper: A. Ransom. To be settled on the bluff path.\n'
              'Collector, thereafter: E. Ransom. Engaged from the 28th.\n\n'
              'Entered 21 July.\n\n'
              'In the margin: Dunne will keep this one for himself. Allow it. For now.'),
        lines=[(ELLIS, 'July. He wrote my name in July.'),
               (HOB, "...Put it away, sunshine. Some accounts don't improve on a second reading.")]),
}

# The six pages found on Ransom's Rest, as readables (LedgerPage_<Town>): reading one sets Ledger.Page.<Town>.
for _town, _page in LEDGER_TOWNS.items():
    if _page['found']:
        READABLES['LedgerPage_' + _town] = dict(
            title='A page of the Ledger: ' + _page['name'], heading=_page['heading'], prompt='Read', text=_page['text'],
            lines=_page['lines'], sets_flag='Ledger.Page.' + _town)
del _town, _page

# The props' own models (Art/Models/Props/MissionProps.py, imported to /Game/Art/Props/) for the readables that bring one;
# the rest are read off the dressing they're on (a headboard, a slate, a barn door, a wall, a desk).
READABLE_MODELS = {
    'KeepersRoll': 'SM_KeepersRoll', 'SheriffHat': 'SM_SheriffHat', 'SheriffBadge': 'SM_SheriffBadge',
    'RuthJournal': 'SM_Journal', 'CodeBook': 'SM_CodeBook', 'PostLog': 'SM_PostLog', 'PayrollBook': 'SM_PayrollBook',
    'DeaconsSlip': 'SM_DeaconsSlip',
    # The Rest's seeds (the props art's; build_area_seeds.py falls back to SM_DeaconsSlip while one isn't imported).
    'DeaconLetter_Cleft': 'SM_DeaconLetter', 'DeaconLetter_Saloon': 'SM_DeaconLetter', 'ReachesMap': 'SM_ReachesMap',
    # Slips (build_area_secrets.py falls back to SM_DeaconsSlip, the slip it is): Aldana's page, Teague's powder slip.
    'AldanaPage': 'SM_DeaconsSlip', 'Secret_Powder': 'SM_DeaconsSlip',
}
READABLE_MODELS.update({'LedgerPage_' + town: 'SM_LedgerPage' for town, page in LEDGER_TOWNS.items() if page['found']})

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
    # Side 1: the poster on the town gate's south post (a handbill, laid over the posterGate decal). The decal can't be torn
    # until Side 1 is taken (AWantedPoster::IsAwaitingOffer finds this card over it by itself, no property to set), so a
    # tap reads the poster first; then it tears like the others and counts.
    'Artifact_PosterGate': dict(title='Your wanted poster', prompt='Read the poster', text='', model='',
                                take_on_accept=False),
    # Calder's Caches: Ruth Calder's note, pinned on the Rim Rangers' board by the sheriff's office (tagged CalderNote too,
    # so a read through it is Side 1's "read the note"; the board's screen offers the job from it as well).
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
                         prompt="Take Thaddeus's tape measure", shown_when=dict(during='KnockingBelow', from_step=4)),
    # (WinchCrank and CoalSack went with Dead Lift's fetches, 2026-10-21: never reuse their ids.)
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
        # (What Ada gave up is a page slid under his door now: READABLES AldanaPage.)
        (dict(after=['Main5']), 'DA_Lines_AldanaAfterMain5'),
        (dict(after=['Main4']), 'DA_Lines_AldanaAfterMain4'),
    ]),
    # build_area_whitlock: the bales are Side 2's first step (LOAD_STEP 0), the hands its second (FIGHT_STEP 1; the road
    # talk went into the fight's end, the hands' spawner's cleared_lines DA_Lines_AmosRoad), the thanks the turn-in
    # (THANKS_STEP 2).
    'Speaker_Amos': dict(default='DA_Lines_AmosMeet', topics=[
        (dict(during='Side2', from_step=2), 'DA_Lines_AmosThanks'),
        (dict(during='Side2', from_step=1), 'DA_Lines_AmosHands'),
        (dict(during='Side2'), 'DA_Lines_AmosBales'),
        (dict(after=['Side2', 'Main6']), 'DA_Lines_AmosAfterMain6'),
        (dict(after=['Side2']), 'DA_Lines_AmosFence'),
    ]),
    # build_area_story.place_hob: his talk, the side missions after the main ones (the story first).
    # (2026-10-21, the user's cut of the story's talk: his perch arrivals carry the story; talked to, he has Main 1's word,
    # the Old Fare's, kept whole as that mission is, and his catch-all.)
    'Speaker_Hob': dict(default=None, topics=[
        (dict(during='Main1'), 'DA_Lines_HobMain1'),
        (dict(during='OldFare'), 'DA_Lines_HobOldFare'),
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
        (dict(during='KnockingBelow', from_step=4), 'DA_Lines_ThaddeusTalk'),
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
    # (Mr. Sloane's coal line went with Dead Lift's coal fetch, 2026-10-21.)
    # Hiram from the winch house as the lift crew rise (Dead Lift's first step).
    'Speaker_Hiram': [(dict(during='DeadLift', before_step=1), 'DA_Lines_HiramCrew')],
    # Hob's feathers (the Rest's seeds): three lines that only make sense later, one a pass as the player comes near him,
    # in order, from the claw-out's mission on (build_area_story.place_hob).
    'Speaker_Hob': [(dict(after=['Main1']), 'DA_Lines_HobIdle')],
    # (The Haskett house's "...That's the lift. Abe always said he'd hear her run." once Dead Lift is done is a town-life
    # door's mutter: World/TownLifeRules.cpp's News, the CottageNorth household, which ATownLifeDoor says by itself. No
    # speaker point carries it, so it isn't listed here; DA_Lines_HaskettLift holds the same words.)
}

PERCHES = {
    # On the Providence's tag board as Back Pay begins.
    'mineMouth': (dict(during='BackPay', before_step=1), 'DA_Lines_HobBackPayTags'),
    # On the bier beside Pa's once The Gravewind is done (the scene's spot: he lands there again with no flight, so this
    # is said as the scene's words end, still at dusk), until Pa's Main 7 is taken: the road at dusk, the second light low
    # over the fog (build_area_seeds.py), Hob's word on it last.
    'gravewindPoint': (dict(after=['Main6']), 'DA_Lines_HobMain6Point'),
}

# When each mission fight is on (an encounter's active_when): the missions count their kills from the spawner, so a fight
# cleared before its step began still counts.
ENCOUNTERS = {
    'DepotWaiting': dict(during='OldFare', before_step=1),
    # "Rob a dead man's eyes and every soul for a mile hears about it": the waiting room fills again as Fenn's coins come
    # down the stair (the Old Fare's lay step, from 0: 5), until they're back on his eyes (build_area_missionprops_valley).
    'DepotReturn': dict(during='OldFare', from_step=5, before_step=6),
    'TillyCellar': dict(during='KnockingBelow', from_step=1, before_step=3),
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


# Things to find off the path (2026-10-21): each an AChest (its chest_id kept by the session: never rename one), its kind,
# its loot (FChestLootOverride's fields; none: the kind's own) and the readable found with it (READABLES; None: the chest
# alone). build_area_secrets.py finds each spot from its anchor (a placed actor's label, a layout obstacle's middle, or
# a layout cm spot) and the search ring round it; widows and buttes are layout obstacles.
SECRETS = {
    # The legendary chance: a strongbox from the gang's old bank jobs (Docs/Story.md, Loot) under the middle Widow, on its
    # north side, away from the keeper's path. The Gravemother's odds: two guns at Luck 1.3.
    'WidowsBox': dict(chest_id='Secret_WidowsStrongbox', kind='Strongbox', anchor=('obstacle', 'widow2Rock'),
                      prefer=(1.0, 0.0), ring=(260.0, 420.0),
                      loot=dict(guns=2, gun_chance=1.0, luck=1.3, ammo_pickups=2, mote_chance=0.0),
                      readable='Secret_WidowsBox'),
    # The skiff's locker on the Mooring Ledge, by the Deacon's letter and the Mire's page (Hob's hint at Main 2's end).
    'Mooring': dict(chest_id='Secret_MooringLocker', kind='Footlocker', anchor=('ledge', 'mooringPost'),
                    loot=dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=2, mote_chance=0.0), readable=None),
    # Coffin Rock's top (a vista): the surveyor's tin and his bench mark, reached by stepping rocks up its north face.
    'CoffinRock': dict(chest_id='Secret_SurveyorsTin', kind='Footlocker', anchor=('top', 'coffinRock'),
                       loot=dict(guns=1, gun_chance=1.0, luck=0.6, ammo_pickups=1, mote_chance=0.0),
                       readable='Secret_BenchMark', steps=('north', 6, 125.0)),
    # The Burnt Homestead's chimney (the town's mutter after the gate: "Avery kept his savings up the chimney").
    'Chimney': dict(chest_id='Secret_AveryTin', kind='Footlocker', anchor=('obstacle', 'burntHomestead'),
                    prefer=(0.0, 1.0), ring=(120.0, 300.0),
                    loot=dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=1, mote_chance=0.0), readable=None),
    # Thaddeus's bottle in the springhouse (his word after Knocking Below).
    'Bottle': dict(chest_id='Secret_ThaddeusBottle', kind='Footlocker', anchor=('obstacle', 'springhouse'), inside=True,
                   prefer=(0.0, 0.0), ring=(0.0, 200.0),
                   loot=dict(guns=1, gun_chance=1.0, luck=0.8, ammo_pickups=1, mote_chance=0.0), readable='Secret_Bottle'),
    # The keepers' stores in the old lantern house by the keeper's grave: oil and shells for nights on the boards.
    'LanternHouse': dict(chest_id='Secret_KeepersStores', kind='Footlocker', anchor=('obstacle', 'lanternHouse'), inside=True,
                         prefer=(0.0, 0.0), ring=(0.0, 200.0),
                         loot=dict(guns=1, gun_chance=0.5, luck=0.5, ammo_pickups=2, mote_chance=0.0), readable=None),
    # A spiders' hoard under boot hill's hanging tree (spiders nest in graveyards and hoard what a saint has touched).
    'Hoard': dict(chest_id='Secret_SpiderHoard', kind='Footlocker', anchor=('actor', 'hangingTree'),
                  prefer=(0.0, 0.0), ring=(180.0, 320.0),
                  loot=dict(guns=1, gun_chance=1.0, luck=0.7, ammo_pickups=1, mote_chance=0.0), readable='Secret_Hoard'),
    # Teague's powder box on Larkspur Bench, behind granite G (Odell's report, the payroll's small charge).
    'Powder': dict(chest_id='Secret_PowderBox', kind='Footlocker', anchor=('obstacle', 'benchGraniteGRock'),
                   prefer=(-1.0, 0.0), ring=(260.0, 420.0),
                   loot=dict(guns=1, gun_chance=1.0, luck=1.0, ammo_pickups=2, mote_chance=0.0), readable='Secret_Powder'),
    # A crate fallen off the old derrick's boom, among the quarry blocks.
    'Crate': dict(chest_id='Secret_DerrickCrate', kind='Footlocker', anchor=('obstacle', 'quarryBlocks'),
                  prefer=(0.0, 1.0), ring=(260.0, 480.0),
                  loot=dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=2, mote_chance=0.0), readable='Secret_Crate'),
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
    if spec.get('sets_flag'):
        # AReadableProp::SetsFlag (2026-10-16): set on reading. Guarded, so a game module built before it still places the
        # readable (it reads, and only the flag is missing).
        try:
            prop.set_editor_property('sets_flag', unreal.Name(spec['sets_flag']))
        except Exception as error:
            unreal.log_warning(f'{readable_id}: no ReadableProp.sets_flag ({error}): reading it sets no '
                               f'{spec["sets_flag"]} (build the game module)')
    return prop
