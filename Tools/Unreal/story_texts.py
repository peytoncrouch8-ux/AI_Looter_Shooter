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
  LANDFALL_PERCHES Hob's perches through Landfall (Main 1 since 2026-10-31), in first-match order: each its spots (the
              first the placing script has), when, and its arrival line set; they replace the old Main 1 board perches.
  MAIN23_PERCHES, MAIN23_AFTER_PERCHES Hob's perches through Mains 2 and 3 (the Wardens story, 2026-11-01), as
              LANDFALL_PERCHES; they replace the old Main 2/3 perches in build_area_story.place_hob.
  MAIN45_PERCHES Hob's perches through Mains 4 and 5 (A Coal Under Ash, Choke Point; lane MAIN45, 2026-11-01), as
              LANDFALL_PERCHES; they replace the old Main 4/5 perches in build_area_story.place_hob. The same block adds
              Mains 4-5's LOUDSPEAKER_CUES and ENCOUNTERS (BellAnswered, IntakeDrop, IntakeSpiders).
  MUTTER_POINTS lines said unasked at a spot with no one there (an unseen speaker point's mutters), by its tag.
  LOUDSPEAKER_CUES what Vane says from each Company loudspeaker (ACompanyLoudspeaker), by its tag; NED_LEFT_FLAG and
              BANNER_DOWN_FLAG, Main 3's story flags.
  ENCOUNTERS  when each mission fight is on (an encounter spawner's active_when), by its spawner id.
  SECRETS     the things to find off the path (2026-10-21): each one's chest, its loot, its anchor and its readable
              (build_area_secrets.py places them).
  FINDS       the find system's words (2026-10-28, Docs/Work/2026-10-28-Core/FINDS.md): each of the fifteen finds' banner
              name, region and the lines Hob says after the banner; FIND_REGIONS, FIND_COMPLETE. The new finds' readables
              (Find_*) are in READABLES. The ids are AFindSpot ids (saved as Find.<Id>): never rename one.

A condition is a dict of build_area_story.condition()'s words (after, before, during, from_step, before_step; steps
counted from 0, as the missions' are) plus with_flags and without_flags (story flags); condition() below makes the
unreal.StoryCondition from one. The missions' step numbers these name are listed in each mission script's docstring.
The file stays plain ASCII: a signature's dash is DASH, an escape.
"""

HOB = 'Hob'
ELLIS = 'Ellis'
# The player character in the Wardens story's new lines (2026-10-31): the hero Ellis shows as Sully Graves (id kept), and
# hero_lines.json's speaker.Sully names the played hero for the other four.
SULLY = 'Sully'
# The long dash before a signature (an em dash), made from its code so the file stays plain ASCII.
DASH = chr(0x2014)

# Landfall's story flag (Main 1 since 2026-10-31): set by reading Abel's hat (READABLES AbelsHat, its sets_flag); Delia's
# turn-in topic with it comes first (TOPICS['Speaker_Delia']), her best line.
ABELS_HAT_FLAG = 'Landfall.AbelsHat'

# ---------------------------------------------------------------------------
# Readables (AReadableProp: readable_id, title, heading, text, lines, prompt, shown_when)
# ---------------------------------------------------------------------------

READABLES = {
    # Landfall (Main 1), step 7's optional: Abel's hat on the bluff path, where he lost it running down to warn his mother
    # the night the barge came (the trail's glow passes it). Placed by WORLD's build_area_landfall.py as an AReadableProp
    # with apply_readable(prop, 'AbelsHat'), also tagged Landfall_AbelsHat (the mission's Interact objective counts the
    # interaction by that tag) and hold_to_read on (a step's key action, the mission gates' rule). Only during Main 1 from
    # step 7, and gone once picked up (the flag in without_flags: the prop hides as the runner changes). Reading it sets
    # Landfall.AbelsHat: Delia's turn-in says her hat lines (TOPICS['Speaker_Delia']).
    'AbelsHat': dict(
        title="Abel's hat", heading='Dropped on the bluff path', prompt='Pick up', text='',
        lines=[(SULLY, "A keeper's hat, dropped on the run."),
               (HOB, 'Take it down to the farm, sunshine. Somebody will want it.')],
        shown_when=dict(during='Main1', from_step=7, without_flags=[ABELS_HAT_FLAG]),
        sets_flag=ABELS_HAT_FLAG),
    # (Retired 2026-10-31 with Seven Days: WORLD stops placing Ellis's grave and headboard. Kept so an old level's prop still
    # reads; delete once the gameplay rebuild has removed it.) Ellis's own headboard, beside Abel's in the family plot.
    'HeadboardEllis': dict(
        title="Ellis's headboard", heading='Fresh pine, in the family plot', prompt='Read',
        text='ELLIS RANSOM.\nCAME HOME AT THE LAST.',
        lines=[(ELLIS, 'Who carved this?'),
               (HOB, 'Your grandmother. She did the letters twice.')]),
    # (Retired 2026-11-01 with "Shall We Talk Business?": Main 2 is The Ferryman's Price now and no step reads it; kept so
    # an old level's prop still reads until the gameplay rebuild removes it.) The lookout's boards where the Deacon shot Ellis.
    'LookoutStains': dict(
        title='The boards where it happened', heading="Ransom's Point, the lookout's deck", prompt='Look', text='',
        lines=[(ELLIS, 'He was standing right here.'),
               (HOB, 'So were you, sunshine. Briefly.')]),
    # Main 4 (A Coal Under Ash, the Wardens story since 2026-11-01), step 4's optional: the keepers' roll, a painted plank in
    # the nave (SM_KeepersRoll_Bare: no name painted out under ABEL now; the hero is Sully, a Warden, no Ransom). Every
    # Ransom keeper since the saint, Abel the last (MAINS_2_7.md section 6). The words build_area_missionprops_valley.py
    # places are its own READABLES' (lane MAIN45's exact edit E6 makes them these).
    'KeepersRoll': dict(
        title="The keepers' roll", heading='Painted on a plank in the nave', prompt='Read',
        text=("KEEPERS OF THE LANTERN\nSAINT ADA'S, RANSOM'S REST\n\nSILAS RANSOM\nHANNAH RANSOM\nJONAS RANSOM\n"
              'ABEL RANSOM\n\nThe last name is fresh paint. There is room under it, and nobody to put there.'),
        lines=[(HOB, 'Ninety years of Ransoms kept her lit. Not one of them got a hose.')]),
    # Main 5 (Choke Point), step 4's optional: the sheriff's hat by the den, on the Sink's floor (SM_SheriffHat, chewed).
    # Sully is a Warden and never knew him: Hob's word only.
    'SheriffHat': dict(
        title="A lawman's hat", heading='In the webbing by the den', prompt='Look',
        text='Chewed through the crown. The band is stamped W. ODELL.',
        lines=[(HOB, 'Whatever lives in that hole had the sheriff for supper.')]),
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
    # No card (the user, 2026-10-27: taking tags shouldn't open a page to read): text is empty, so AReadableProp::Read opens
    # nothing and the sound, captions and step event still fire. The chalk moved into Ellis's caption.
    'Barricade': dict(
        title="The eleven's barricade", heading='Timbers across the lower drift', prompt='Take the tags', text='',
        lines=[(ELLIS, 'Chalked on the timbers: pay us and we come up. Eleven tags.'),
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
    # (Retired 2026-11-01 with Hallowed Ground: the Reliquary is whole in the Wardens story, its lid never prised, and the
    # toll mark was the old Sexton's; lane MAIN45's exact edit E7 stops build_area_seeds.py placing it. Kept so an old
    # level's prop still reads until the gameplay rebuild removes it.) The smashed Reliquary's lid after Hallowed Ground.
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
        # (Sully's since 2026-11-01, lane MAIN45: the hero is a Warden; no count of years, the keepers' roll says ninety.)
        lines=[(SULLY, 'All that time at the edge of the world, holding up her light.'),
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

    # --- Finds off the path, round 2 (2026-10-28; FINDS below; build_area_finds.py places them). Only facts the story has:
    # Pike surveyed the Reaches (the map's "W. PIKE, SURVEYOR"; bench mark 112 is Coffin Rock's), Ruth Calder's Rangers cache
    # their supplies, the dead never follow into deep water, Ned Purcell gambles (the agent's desk), the old fare is a coin
    # on each eye. The scrap is in Coffin Rock's tin; its bearing is honest (the Nose's foot, S. 40 E.).
    'Find_PikeScrap': dict(
        title="A scrap of W. Pike's field book", heading="Folded small in the surveyor's tin", prompt='Read',
        text=("FIELD BOOK, W. PIKE.\n\n"
              "FROM B.M. 112, S. 40 E., 26 YARDS, TO THE NOSE'S FOOT.\n"
              "INSTRUMENTS, POWDER, MY GOOD RIFLE.\n"
              "DIG BY THE RED STONE."),
        lines=[(ELLIS, 'Twenty-six yards, south-east. An honest bearing.'),
               (HOB, 'A surveyor buries what he cannot carry, sunshine.')]),
    'Find_RangerTag': dict(
        title='A Ranger cache tag', heading="Tied to a stake on the tarn's north shore, a rope running into the water",
        prompt='Read', text="RANGER CACHE.\nR.C.\n\nWhere they can't wade.",
        lines=[(ELLIS, "Ruth's mark. The rope goes under."),
               (HOB, "The dead won't follow into deep water, sunshine. Take a breath.")]),
    'Find_MarkedCard': dict(
        title='A nicked ace', heading='Pinned to the rock with a knife', prompt='Look',
        text='An ace of spades, nicked at one corner. You would feel it dealing.\n\nOn the back, in pencil: N.P.',
        lines=[(ELLIS, 'A marked card.'),
               (HOB, "Purcell's. He never could leave a deck alone, sunshine.")]),
    'Find_RangerNote': dict(
        title="Ranger Post Three's note", heading="Under a stone on the post's lookout", prompt='Read',
        text='RANGER POST 3.\nWATCH THE NOTCH AT DUSK.\n\n' + DASH + ' R.C.',
        lines=[(ELLIS, 'Another of her posts.'),
               (HOB, 'Wait for dusk, sunshine. The canyon earns its name then.')]),
    'Find_MiserBoard': dict(
        title='A headboard set apart', heading='Past the last row on boot hill', prompt='Read',
        text='EZEKIEL MOSS.\nHE TOOK HIS FARE WITH HIM.',
        lines=[(ELLIS, 'Gold on the mound.'),
               (HOB, "His fare, sunshine. Dig him up and he'll want it back.")]),
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
            title="A page of the Warden's Log: " + _page['name'], heading=_page['heading'], prompt='Read', text=_page['text'],
            lines=_page['lines'], sets_flag='Ledger.Page.' + _town)
del _town, _page

# The props' own models (Art/Models/Props/MissionProps.py, imported to /Game/Art/Props/) for the readables that bring one;
# the rest are read off the dressing they're on (a headboard, a slate, a barn door, a wall, a desk).
READABLE_MODELS = {
    'KeepersRoll': 'SM_KeepersRoll', 'SheriffHat': 'SM_SheriffHat', 'SheriffBadge': 'SM_SheriffBadge',
    'RuthJournal': 'SM_Journal', 'CodeBook': 'SM_CodeBook', 'PostLog': 'SM_PostLog', 'PayrollBook': 'SM_PayrollBook',
    'DeaconsSlip': 'SM_DeaconsSlip',
    # Landfall's hat: the user's Tripo hat (/Game/Art/Props/SM_AbelsHat, lanes/IMPORT.md; the stand-in was SM_SheriffHat).
    'AbelsHat': 'SM_AbelsHat',
    # The Rest's seeds (the props art's; build_area_seeds.py falls back to SM_DeaconsSlip while one isn't imported).
    'DeaconLetter_Cleft': 'SM_DeaconLetter', 'DeaconLetter_Saloon': 'SM_DeaconLetter', 'ReachesMap': 'SM_ReachesMap',
    # Slips (build_area_secrets.py falls back to SM_DeaconsSlip, the slip it is): Aldana's page, Teague's powder slip.
    'AldanaPage': 'SM_DeaconsSlip', 'Secret_Powder': 'SM_DeaconsSlip',
    # Finds (2026-10-28): paper stands in as a scrap, a card and a note; the tag and the headboard are read off the props
    # build_area_finds.py places (SM_CacheTag, SM_Grave_Headboard_OldC).
    'Find_PikeScrap': 'SM_LedgerPage', 'Find_MarkedCard': 'SM_DeaconsSlip', 'Find_RangerNote': 'SM_DeaconsSlip',
}
READABLE_MODELS.update({'LedgerPage_' + town: 'SM_LedgerPage' for town, page in LEDGER_TOWNS.items() if page['found']})

# ---------------------------------------------------------------------------
# The porch plate (APorchPlate, on the farmhouse's porch table)
# ---------------------------------------------------------------------------

PORCH_PLATE = dict(
    take_prompt='Take the jars', eat_prompt='Eat the biscuits',
    # The slip under the jars, read as they're taken (two grave-salt grenades).
    take_lines=[("Delia's slip", "SALT FOR THE ROAD. THEY DON'T CARE FOR IT. " + DASH + ' D.')],
    # Delia, as the biscuits go (Main 7's optional). (Since 2026-10-31 she's Delia Ransom, the hero a Warden, no kin.)
    eat_lines=[('Delia Ransom', '...Thank you, Warden.')],
    # Since Landfall (2026-10-31) the plate is no Main 1 step: Delia's turn-in sends the player to it ("Salt's on the
    # plate"), and the jars are a free pickup from then on, once (its TakenFlag). WORLD's valley script takes this as the
    # plate's take_when as it is, without looking for a mission step.
    take_when=dict(after=['Main1']),
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
    # Main 5's item (it goes on Abel's lantern there; Main 3's own items vanish as it's turned in): Tilly puts it out on her
    # sill at Main 3's turn-in, and it lies there until it's taken.
    'LanternBail': dict(mission='Main5', where="on Tilly's window sill (Bright & Daughter's Speaker socket, in front)",
                        prompt="Take Pa's lantern bail", shown_when=dict(after=['Main3'])),
    # Choke Point (Main 5, lane MAIN45, 2026-11-01): Junie's three salt charges in the canvas satchel Hob drops on the Sink's
    # rim at the ramp head (build_area_main5.py places it there, tagged Item_SaltCharges); step 0 asks for them, step 3
    # sets them on the intake's collar. Shown while Main 5 runs, until taken.
    'SaltCharges': dict(mission='Main5', name="Junie's salt charges",
                        where="in Hob's satchel on the Sink's rim by the ramp head (build_area_main5.py)",
                        prompt="Take Junie's salt charges", shown_when=dict(during='Main5')),
    # Shown from the story's start, so the bluff path's (by the crate Main 2 opens) can be taken then and counts later.
    'CacheTag1': dict(mission='CaldersCaches', where="on the windmill's Ranger cache (RangerCache_Windmill's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'CacheTag2': dict(mission='CaldersCaches', where="on the Sink rim's Ranger cache (RangerCache_SinkRim's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'CacheTag3': dict(mission='CaldersCaches', where="on the bluff path's Ranger cache (RangerCache_BluffPath's lid)",
                      prompt="Take the Ranger's tag", shown_when=dict(after=['Main1'])),
    'FennsFare': dict(mission='OldFare', where="on the landing under Mr. Sloane's door (SM_RR_Depot's AgentDoor socket)",
                      prompt="Take Fenn's coins", shown_when=dict(during='OldFare', from_step=4)),
    # Lost and Found (2026-10-27, the user's note 37: the quest that opens Lost Property): Mr. Sloane's keys, kicked under the
    # waiting room's benches as he ran for the stair; there only while the mission runs (build_area_lostloot.py places them,
    # with the window and its shutter; name: the prompt's and the stand-in's).
    'LostPropertyKeys': dict(mission='LostAndFound', name="Mr. Sloane's keys",
                             where="on the waiting room's floor by its benches (SM_RR_Depot's Wait_2 socket, off it)",
                             prompt="Take Mr. Sloane's keys", shown_when=dict(during='LostAndFound')),
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
    # Delia Ransom on her porch (ADeliaRansom, CREW; placed by WORLD's build_area_farm.py, which puts these FIRST in her
    # point's topics, before its own later ones: the mine, Main 6's "Take him the lantern" with its event, Main 7's). Main 1's
    # turn-in has no lines of its own, so what she says as it's turned in is the first of these that holds: steps counted
    # from 0, Landfall has nine, so from_step 9 is "ready to turn in" (the step past the last). The hat's first, then the
    # plain turn-in, then a word if she's talked to mid-fight. default: after Landfall.
    'Speaker_Delia': dict(default='DA_Lines_DeliaPorch', name='Delia Ransom', topics=[
        (dict(during='Main1', from_step=9, with_flags=[ABELS_HAT_FLAG]), 'DA_Lines_DeliaLandfallHat'),
        (dict(during='Main1', from_step=9), 'DA_Lines_DeliaLandfall'),
        (dict(during='Main1'), 'DA_Lines_DeliaLandfallWait'),
        # Main 3 (Under New Management, 2026-11-01): her word while Sully's on the way to Tilly's (her own talk started it).
        (dict(during='Main3'), 'DA_Lines_DeliaMain3'),
    ]),
    # Mister Sexton in his ferry's stern under Gravewind Point (Main 2, The Ferryman's Price, 2026-11-01; build_area_main2.py
    # sits him there, shown from Main 2 on: he stays at his ferry). The first that holds: the Log waiting (step 3 on), the
    # deal (step 2's talk: its last line hands the Log up), the boards first (steps 0-1), his price after Main 2.
    'Speaker_Sexton': dict(default='DA_Lines_SextonAfter', name='Mister Sexton', topics=[
        (dict(during='Main2', from_step=3), 'DA_Lines_SextonLedger'),
        (dict(during='Main2', from_step=2), 'DA_Lines_SextonDeal'),
        (dict(during='Main2'), 'DA_Lines_SextonWaiting'),
        (dict(after=['Main2']), 'DA_Lines_SextonAfter'),
    ]),
    # build_area_story.place_tilly: her talk is Main 3's turn-in (from its step 3, the step past the last of its three; the
    # mission's turn-in lines say the same, after its card), her Main 7 words know her father.
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
    # (A Coal Under Ash since 2026-11-01, lane MAIN45: while the barge's answer is fought, step 4, he calls through the
    # door; before_step 5 so the turn-in, from step 5, says the mission's own words.)
    'Speaker_Aldana': dict(default='DA_Lines_AldanaBarred', topics=[
        (dict(during='Main4', from_step=4, before_step=5), 'DA_Lines_AldanaAnswer'),
        (dict(during='Main4', from_step=3), 'DA_Lines_AldanaBell'),
        (dict(during='Main4', from_step=2), 'DA_Lines_AldanaWaiting'),
        # (What Ada gave up is a page slid under his door now: READABLES AldanaPage.)
        (dict(after=['Main5']), 'DA_Lines_AldanaAfterMain5'),
        (dict(after=['Main4']), 'DA_Lines_AldanaAfterMain4'),
    ]),
    # Brother Finch at the Reliquary (Main 4's turn-in on, lane MAIN45: CREW's AWardenCrewMember, Keeper, a second placement
    # by build_area_main4.py, tagged Speaker_FinchChapel so the ship's Finch keeps Speaker_Finch). He keeps the saint alive
    # by hand until Abel is back; Main 5 is turned in to him (its own words). After Main 5, the lantern by him.
    'Speaker_FinchChapel': dict(default='DA_Lines_FinchReliquary', name='Brother Finch', topics=[
        (dict(after=['Main5']), 'DA_Lines_FinchAfterMain5'),
        (dict(after=['Main4']), 'DA_Lines_FinchReliquary'),
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
    # the Old Fare's, kept whole as that mission is, and his catch-all. Main 1's word is Landfall's since 2026-10-31.)
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
    # His ticket window (SM_RR_Depot's TicketWindow socket), from the Old Fare's end; the coal while Dead Lift runs. It
    # offers Lost and Found and takes its turn-in (the mission's own words, 2026-10-27), which opens the LOST PROPERTY
    # window.
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

# Hob through Landfall (Main 1 since 2026-10-31; steps counted from 0), in first-match order, as build_area_story.place_hob
# takes perches: each (spots, when, arrival line set or None). spots: the place_hob spot names to try in order, the first
# one the build has (a perch whose spots are all missing is left out, and its arrival words with it, so each ends on a spot
# that exists today). Landfall's spots in place_hob's spots (measured by build_area_landfall.place): 'porchRail' (the
# porch beam's end over Delia's steps, facing the yard), 'mooringLedge' (the anchor winch's top on the Mooring Ledge);
# 'abelBoard' is Abel's headboard in the family plot, which the trail runs past.
# While the Lamplighter sails (steps 0-3) no perch holds and he is away (FLIGHT may show a stand-in on the rigging): the
# crew's words carry those steps. The arrivals are said as their step starts, wherever the player is: the Anchor, Climb and
# Point sets are the crew's words (their speakers are on the lines). After Landfall he stays on the porch rail
# (place_hob's last perch: porchRail, else path), where Main 2's offer finds him. place_hob warns if these steps aren't
# the ones Main 1's data asks for (anchor, point, spyglass, trail, porch).
LANDFALL_PERCHES = [
    # Step 8: on the porch rail as the fight at the farm begins; there for the turn-in too.
    (('porchRail', 'path'), dict(during='Main1', from_step=8), 'DA_Lines_HobLandfallPorch'),
    # Step 7: on Abel's headboard, which the trail runs past on its way to the farm (else the bluff path's foot). Silent:
    # the step starts as the spyglass is used, under the reveal's words; his word on the hat ends DA_Lines_LandfallTrail,
    # said after it.
    (('abelBoard', 'path'), dict(during='Main1', from_step=7), None),
    # Step 6: on the lookout's rail by the spyglass (Holloway's word, then his).
    (('rail',), dict(during='Main1', from_step=6), 'DA_Lines_LandfallPoint'),
    # Step 5: on the rail already, waiting at the top of the climb (the crew's word on why).
    (('rail',), dict(during='Main1', from_step=5), 'DA_Lines_LandfallClimb'),
    # Step 4: on the Mooring Ledge by the winch (the crew's word on the anchor and the dead).
    (('mooringLedge', 'rail'), dict(during='Main1', from_step=4), 'DA_Lines_LandfallAnchor'),
]

# Hob through Mains 2 and 3 (the Wardens story, 2026-11-01: The Ferryman's Price, Under New Management), as
# LANDFALL_PERCHES (spots tried in order, when, arrival set or None), in first-match order: build_area_story.place_hob
# takes them in place of its old Main 2/3 perches (Docs/Work/2026-10-31-Wardens/lanes/MAIN23.md, its exact edit). Spots:
# 'widows' (the middle Widow's top, over the keeper's path) and 'ladderTop' (the ferry ladder's crossbar on the deck's
# edge) from build_area_main2.place; 'gateRock' (the Keeper's Gate's north rock), 'gate' (the town gate's post) and 'sign'
# (Bright & Daughter's sign) as place_hob has them already.
MAIN23_PERCHES = [
    # Main 2: on the ladder for the talk and the Log (why down the ladder); on the gate's north rock for the cart (why the
    # cart, how, and the loudspeaker); on the middle Widow for the way out, silent (the offer's words are said on the move).
    (('ladderTop', 'gateRock'), dict(during='Main2', from_step=2), 'DA_Lines_HobMain2Ladder'),
    (('gateRock',), dict(during='Main2', from_step=1), 'DA_Lines_HobMain2Cart'),
    (('widows', 'path'), dict(during='Main2'), None),
    # Main 3: on the undertaker's sign once the checkpoint is broken; on the gate's post for its fight (Vane's gate cue
    # comes after his word) and for the farm road (his word, then Ned's skiff, which waits for quiet).
    (('sign',), dict(during='Main3', from_step=2), 'DA_Lines_HobMain3Tilly'),
    (('gate',), dict(during='Main3', from_step=1), 'DA_Lines_HobMain3Gate'),
    (('gate',), dict(during='Main3'), 'DA_Lines_HobMain3Road'),
]
# Where he waits after each (place_hob's after-perches, latest mission first): on the sign after Main 3, on the ladder's
# crossbar after Main 2 (by the Ferryman, whom he flew for).
MAIN23_AFTER_PERCHES = [
    (('sign',), dict(after=['Main3']), None),
    (('ladderTop', 'gateRock'), dict(after=['Main2']), None),
]

# Lines said unasked as the player passes a spot with no one there (an unseen speaker point: talkable off, its mutters
# these), by the point's tag; build_area_main2.py places Keeper's Bridge's (its middle, the bridge's own build).
MUTTER_POINTS = {
    'Mutter_KeepersBridge': dict(name='Hob', radius=700.0, mutters=[(dict(during='Main2'), 'DA_Lines_HobMain2Bridge')]),
}

# What Vane says from each Company loudspeaker (ACompanyLoudspeaker cues: once a visit each, the first time its condition
# holds with the player in earshot, into a quiet moment), by the loudspeaker's tag. The Keeper's Gate's speaks only when
# the boards' hose-cart is stopped (the cart's stop_broadcast, DA_Lines_Main2VaneCart): no cue of its own.
LOUDSPEAKER_CUES = {
    'Loudspeaker_KeepersGate': [],
    'Loudspeaker_TownGate': [(dict(during='Main3', from_step=1), 'DA_Lines_Main3VaneGate')],
    'Loudspeaker_MainStreet': [(dict(during='Main3', with_flags=['SetPiece.MainStreet']), 'DA_Lines_Main3VaneStreet')],
}

# Mains 2-3's story flags: Lucky Ned's skiff has flown (AGangSkiffFlyBy's done_flag), Main Street's banner is down
# (ACompanyBanner's down_flag; its down_event is what Main 3's optional counts).
NED_LEFT_FLAG = 'Main3.NedLeft'
BANNER_DOWN_FLAG = 'MainStreet.BannerDown'

# When each mission fight is on (an encounter's active_when): the missions count their kills from the spawner, so a fight
# cleared before its step began still counts.
ENCOUNTERS = {
    # Landfall (Main 1, 2026-10-31; WORLD's build_area_landfall.py places both spawners): four Unpaid rising from the canyon
    # side of the Mooring Ledge while the anchor is dropped (step 4); and the farm's fight, three riflemen, two hose-men and
    # three Unpaid at Delia's steps, on from step 7 (the spawner's own trigger, about 45 m from the farmhouse, starts it as
    # the player comes down the trail), cleared in step 8.
    'LedgeClimbers': dict(during='Main1', from_step=4, before_step=5),
    'FarmPorch': dict(during='Main1', from_step=7, before_step=9),
    # The Ferryman's Price (Main 2, 2026-11-01; build_area_main2.py): the Three Widows' spiders on the keeper's path, on the
    # whole way out (step 0) and through the boards' fight, so a nest left for later is still there; the boards' crew from
    # the way out (they stand at work by the cart as the player comes up the cairns) until the talk (step 2).
    'BluffNest': dict(during='Main2', before_step=2),
    'BoardsCrew': dict(during='Main2', before_step=2),
    # Under New Management (Main 3; build_area_main3.py): the checkpoint's men at their sandbags from the farm road (step 0)
    # through its fight (step 1), so a rifleman shot from the road counts and a fight won doesn't come back on a later load.
    'TownGate': dict(during='Main3', before_step=2),
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
    # north side, away from the keeper's path. A loot chest tier 5 since 2026-10-27 (chest_tiers.py: three guns, Epic or
    # better, its legendary odds kept); placed and judged as a strongbox. Its loot before: two guns at Luck 1.3.
    'WidowsBox': dict(chest_id='Secret_WidowsStrongbox', kind='Strongbox', anchor=('obstacle', 'widow2Rock'),
                      prefer=(1.0, 0.0), ring=(260.0, 420.0),
                      readable='Secret_WidowsBox'),
    # The skiff's locker on the Mooring Ledge, by the Deacon's letter and the Mire's page (Hob's hint at Main 2's end).
    # (FINDS: two guns at Luck 1.0 now, was one at 0.5: a find pays at least a camp crate.)
    'Mooring': dict(chest_id='Secret_MooringLocker', kind='Footlocker', anchor=('ledge', 'mooringPost'),
                    loot=dict(guns=2, gun_chance=1.0, luck=1.0, ammo_pickups=2, mote_chance=0.0), readable=None),
    # Coffin Rock's top (a vista): the surveyor's tin and his bench mark, reached up the fallen slab on its north face
    # (steps: only the direction is read now, build_area_secrets.place_steps).
    # FINDS: two guns at Luck 1.0 (was one at 0.6), and the tin also holds Pike's field-book scrap (READABLES Find_PikeScrap,
    # 'scrap': build_area_finds.py places it beside the bench mark; it leads to Pike's Cache). Unknown keys are ignored by
    # build_area_secrets.py.
    'CoffinRock': dict(chest_id='Secret_SurveyorsTin', kind='Footlocker', anchor=('top', 'coffinRock'),
                       loot=dict(guns=2, gun_chance=1.0, luck=1.0, ammo_pickups=1, mote_chance=0.0),
                       readable='Secret_BenchMark', scrap='Find_PikeScrap', steps=('north', 6, 125.0)),
    # The Burnt Homestead's chimney (the town's mutter after the gate: "Avery kept his savings up the chimney"). FINDS E4's
    # 45 silver are its AFindSpot's Silver, not the chest's loot.
    'Chimney': dict(chest_id='Secret_AveryTin', kind='Footlocker', anchor=('obstacle', 'burntHomestead'),
                    prefer=(0.0, 1.0), ring=(120.0, 300.0),
                    loot=dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=1, mote_chance=0.0), readable=None),
    # Thaddeus's bottle in the springhouse (his word after Knocking Below).
    'Bottle': dict(chest_id='Secret_ThaddeusBottle', kind='Footlocker', anchor=('obstacle', 'springhouse'), inside=True,
                   prefer=(0.0, 0.0), ring=(0.0, 200.0),
                   loot=dict(guns=1, gun_chance=1.0, luck=0.8, ammo_pickups=1, mote_chance=0.0), readable='Secret_Bottle'),
    # The keepers' stores in the old lantern house by the keeper's grave: oil and shells for nights on the boards. (FINDS E6
    # adds two grenade tins; the chest override has no grenade field, so the find system pays them.)
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
                   loot=dict(guns=2, gun_chance=1.0, luck=1.0, ammo_pickups=2, mote_chance=0.0), readable='Secret_Powder'),
    # A crate fallen off the old derrick's boom, among the quarry blocks.
    'Crate': dict(chest_id='Secret_DerrickCrate', kind='Footlocker', anchor=('obstacle', 'quarryBlocks'),
                  prefer=(0.0, 1.0), ring=(260.0, 480.0),
                  loot=dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=2, mote_chance=0.0), readable='Secret_Crate'),
}


# The find system's words (2026-10-28, Docs/Work/2026-10-28-Core/FINDS.md), by FindId (AFindSpot::FindId, saved as the story
# flag Find.<Id>: never rename one). name: the banner's name (AFindSpot::DisplayName); region: one of FIND_REGIONS (the
# map's rows, in order); lines: what Hob says once the banner ends (AFindSpot::Lines, through UCaptionSubsystem::Play). A
# find whose readable already has Hob's remark (READABLES) has no lines here, so he never says two things at once. Short,
# in his voice ("sunshine"); no family words (Docs/Pipeline/Story.md "The heroes' words").
FIND_REGIONS = ['Farm & Point', 'West Road', 'Town & Boot Hill', 'South Fields', 'The Mountain']

FINDS = {
    'WidowsBox': dict(name="The Widow's Strongbox", region='Farm & Point', lines=[]),
    'Mooring': dict(name="The Gang's Locker", region='Farm & Point',
                    lines=[(HOB, 'Left by the skiff, sunshine. Nobody came back.')]),
    'RangerPost': dict(name='Ranger Post Three', region='Farm & Point', lines=[]),
    'BoltHole': dict(name='The Bolt-Hole', region='Farm & Point',
                     lines=[(HOB, 'The gang waited out the day in here, sunshine.')]),
    'Chimney': dict(name="The Averys' Savings", region='West Road',
                    lines=[(HOB, 'Fire took the house. The chimney kept the savings.')]),
    'LanternHouse': dict(name="The Keepers' Stores", region='West Road',
                         lines=[(HOB, 'Oil and shells. Keepers plan for long nights.')]),
    'Tollman': dict(name='The Tollman', region='West Road',
                    lines=[(HOB, 'Somebody finally collected from the collector, sunshine.')]),
    'Hoard': dict(name="The Spiders' Hoard", region='Town & Boot Hill', lines=[]),
    'MiserGrave': dict(name="The Miser's Fare", region='Town & Boot Hill',
                       lines=[(HOB, 'He took his fare with him. You took it back, sunshine.')]),
    'Bottle': dict(name="Thaddeus's Bottle", region='South Fields', lines=[]),
    'CoffinRock': dict(name="The Surveyor's Tin", region='South Fields', lines=[]),
    'PikesCache': dict(name="Pike's Cache", region='South Fields',
                       lines=[(HOB, 'Twenty-six yards, and he never came back for it.')]),
    'Powder': dict(name="Teague's Powder Box", region='The Mountain', lines=[]),
    'Crate': dict(name='The Fallen Crate', region='The Mountain', lines=[]),
    'Tarn': dict(name="Where They Can't Wade", region='The Mountain',
                 lines=[(HOB, 'Clever woman, Ruth. Sank it where the dead will not go.')]),
}

# All fifteen found (the "EVERY FIND IN RANSOM'S REST" banner's moment): Hob, after the banner.
FIND_COMPLETE = dict(
    banner="EVERY FIND IN RANSOM'S REST",
    lines=[(HOB, "That's all of it, sunshine. Even I didn't know about the tarn.")])


# ---------------------------------------------------------------------------
# Mains 4-5 (lane MAIN45, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md sections 6-7, lanes/MAIN45.md):
# A Coal Under Ash (Main 4) and Choke Point (Main 5). Their own block, so the other lanes' blocks stay apart.
# ---------------------------------------------------------------------------

# Hob through Mains 4 and 5, as LANDFALL_PERCHES (spots tried in order, when, arrival set or None), in first-match order:
# build_area_story.place_hob takes them in place of its old Main 4/5 perches (lanes/MAIN45.md, exact edit E2). Spots:
# 'hood' and 'lantern' (the chapel's door hood and vestry lantern, build_area_chapel.hob_spots), 'denRock', 'block',
# 'roadTree' (build_area_sink.hob_spots) as place_hob has them; 'satchel' (the satchel's rock on the Sink's rim by the ramp
# head) from build_area_main5.place.
MAIN45_PERCHES = [
    # Main 4: on the door hood for the barge's answer (his word first; Vane's horn waits for quiet), the bell and the yard
    # (why the hose, and how); on the vestry lantern's bracket for the knock; on the hood for the way up, silent.
    (('hood',), dict(during='Main4', from_step=4), 'DA_Lines_HobMain4Reliquary'),
    (('hood',), dict(during='Main4', from_step=3), None),
    (('lantern', 'hood'), dict(during='Main4', from_step=2), None),
    (('hood',), dict(during='Main4', from_step=1), 'DA_Lines_HobMain4Yard'),
    (('hood',), dict(during='Main4'), None),
    # Main 5: on the Sink road's dead tree at the rim as the barge drops its second hose (ready to turn in: from step 5);
    # there for the way out (step 4, silent: Vane and the growl have the words); on the floor's block once the lantern is
    # taken (the lantern, then the intake), and for the sacs; on the satchel's rock at the ramp head for the way down.
    (('roadTree',), dict(during='Main5', from_step=5), 'DA_Lines_HobMain5Mine'),
    (('roadTree',), dict(during='Main5', from_step=4), None),
    (('block',), dict(during='Main5', from_step=3), 'DA_Lines_HobMain5Out'),
    (('block',), dict(during='Main5', from_step=1), None),
    (('satchel', 'denRock'), dict(during='Main5'), 'DA_Lines_HobMain5Satchel'),
]

# What Vane says from Mains 4-5's loudspeakers (ACompanyLoudspeaker cues, as LOUDSPEAKER_CUES above): the lychgate's horn
# as the barge answers the bell (step 4); the Sink fence's as Main 5 starts. The barge's own horn over the Sink
# (Loudspeaker_Barge, a voice only) is ASinkIntake's to cue (its charge and choke lines), so it has none here.
LOUDSPEAKER_CUES.update({
    'Loudspeaker_Lychgate': [(dict(during='Main4', from_step=4), 'DA_Lines_Main4VaneBell')],
    'Loudspeaker_SinkFence': [(dict(during='Main5', before_step=1), 'DA_Lines_Main5VaneSink')],
    'Loudspeaker_Barge': [],
})

# When Mains 4-5's new fights are on (as ENCOUNTERS above). BellAnswered: the tender's drop over the chapel knoll, from the
# bell (step 4) until the turn-in. IntakeDrop (the Dredgers down the hose, two drops) and IntakeSpiders (down the webbed
# ramp) from the intake's step (3) through the climb out (4), so a man still standing when the hose blows fights on.
# (ChapelYard stays build_area_chapel.py's set piece: during Main 4's second step.)
ENCOUNTERS.update({
    'BellAnswered': dict(during='Main4', from_step=4, before_step=5),
    'IntakeDrop': dict(during='Main5', from_step=3, before_step=5),
    'IntakeSpiders': dict(during='Main5', from_step=3, before_step=5),
})


# ---------------------------------------------------------------------------
# Mains 6-7 (lane MAIN67, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md sections 10-11, lanes/MAIN67.md,
# main67_src/CONTRACT67.md): The Pilot Light (Main 6) and Given to the Wind (Main 7). Their own block.
# ---------------------------------------------------------------------------

# Hob through Mains 6 and 7, as LANDFALL_PERCHES (spots tried in order, when, arrival set or None), in first-match order:
# build_area_story.place_hob takes them in place of its old Main 6/7 perches (lanes/MAIN67.md, exact edit). Spots:
# 'wheel' (the Lamplighter's wheel housing while she's moored, build_area_main6_sortie.place through build_area_main6),
# 'roadCoal' (a lump of the barge's coal by the west road) and 'chapelDoor' (over the chapel's front door) from
# build_area_main7.place; 'keeperGrave', 'gateRock' (build_area_deck.hob_spots) and 'hood' (the chapel's door hood) as
# place_hob has them. A perch is a fixed world point, so none holds while the Lamplighter flies (Main 6's steps 1-7: he
# rides with the crew, a voice in the captions); he waits on the chapel's door hood under the barge then, silent.
MAIN67_PERCHES = [
    # Main 7: on the chapel door for the carry home (why the road is dangerous) and the Reliquary; on the Keeper's Gate's
    # north rock for the fight (his built word on the open end) and the scene; on the keeper's grave's board as the lantern
    # is to be hung; on the coal by the west road as Main 7 starts (why the coal matters).
    (('chapelDoor', 'hood'), dict(during='Main7', from_step=5), None),
    (('chapelDoor', 'hood'), dict(during='Main7', from_step=4), 'DA_Lines_HobMain7Chapel'),
    (('gateRock',), dict(during='Main7', from_step=3), None),
    (('gateRock',), dict(during='Main7', from_step=2), 'DA_Lines_HobMain6Fight'),
    (('keeperGrave', 'gateRock'), dict(during='Main7', from_step=1), 'DA_Lines_HobMain6Way'),
    (('roadCoal', 'keeperGrave'), dict(during='Main7'), 'DA_Lines_HobMain7Point'),
    # Main 6: on the door hood while she flies and fights (silent); on her wheel housing for the cast-off.
    (('hood',), dict(during='Main6', from_step=1), None),
    (('wheel', 'mooringLedge'), dict(during='Main6'), None),
    # After Back Pay, until Main 6 is taken: on her wheel housing, calling Sully to Holloway (her offer, met first).
    (('wheel', 'mooringLedge'), dict(after=['BackPay'], before=['Main6']), 'DA_Lines_HobMain6Wheel'),
]

# What Vane says from Mains 6-7's loudspeakers (as LOUDSPEAKER_CUES above). The Keeper's Gate's horn as the fog wall drops
# after FreeAbel (Main 7 from step 4). The barge's bow horn (Barge_Horn) is build_area_main6.py's own cue
# (DA_Lines_Main6VaneDeck, Main 6 from step 3).
LOUDSPEAKER_CUES['Loudspeaker_KeepersGate'] = [(dict(during='Main7', from_step=4), 'DA_Lines_Main7Gate')]

# When Mains 6-7's fights are on (as ENCOUNTERS above; build_area_main6.py and build_area_main7.py read them). BargeDeck:
# from the grapples (step 2) until the manifest, so a man still up when the hatch is reached fights on. FurnaceHall: the
# valves' step. WestRoadCrew: the carry home until the turn-in.
ENCOUNTERS.update({
    'BargeDeck': dict(during='Main6', from_step=2, before_step=4),
    'FurnaceHall': dict(during='Main6', from_step=5, before_step=6),
    'WestRoadCrew': dict(during='Main7', from_step=4, before_step=6),
})

# Speakers new in Mains 6-7 (as TOPICS above). Delia at the chapel door (a second ADeliaRansom, build_area_main7.py): Main 7's
# turn-in has no lines of its own, so these say it once it's ready (from step 6, the step past the last): the hose drum's
# flag (Main 6's optional) first. Abel's soul by the Reliquary after Saint Ada wakes (AAbelOnBoard standing): his word.
TOPICS.update({
    'Speaker_DeliaChapel': dict(default=None, name='Delia Ransom', topics=[
        (dict(during='Main7', from_step=6, with_flags=['Pilot.DrumJammed']), 'DA_Lines_DeliaMain7Jammed'),
        (dict(during='Main7', from_step=6), 'DA_Lines_DeliaMain7'),
    ]),
    'Speaker_Abel': dict(default='DA_Lines_AbelReliquary', name='Abel', topics=[]),
})


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


# ---------------------------------------------------------------------------
# SECRETS lane (2026-11-01)
# ---------------------------------------------------------------------------
# Mister Sexton's secrets for silver (MAINS_2_7.md section 12; DECISIONS.md: the neutral Ferryman sells secrets at his
# ferry under Gravewind Point; Docs/Work/2026-10-31-Wardens/lanes/SECRETS.md). Tools/Unreal/create_secret_assets.py
# writes SEXTON_SECRETS into the level's catalog, /Game/Data/Secrets/DA_Secrets_RansomsRest (USecretCatalog,
# Economy/SecretCatalog.h); his tap at the ferry opens its list (USecretsWidget) once open_when holds. Each secret bought
# (story flag SecretSold.<id>: never rename an id) marks its place on the map page and the minimap until it's found (its
# finds' Find.<FindId> flags) or done (done_when); then it drops off the list. Prices are level-1 silver, grown with the
# player's level as the vendors' are (LooterEconomy::AtLevel), a quarter off once discount_mission (Paid on Delivery) is
# done. Each secret's when (its available_when) is the story's step that puts it on his list (a region as the story
# reaches it). Lines fit a caption (90 characters); his voice: dry, "friend", a ferryman's trade.
#   asset, title, tab  the catalog's asset name, the screen's title, its panel's tab
#   open_when          when he trades at all (after Main 2: his turn-in says "Come back when you've silver")
#   greetings          (condition, line) pairs: the first that holds is his line over the list
#   sold_out           his line when nothing is left to sell
#   secrets            id, title (the find's banner name), teaser (shown before buying), line (his, once bought),
#                      region (FIND_REGIONS' or 'The Rest'), kind (Cache, Strongbox, Named, Elite, Lore, Story: the price
#                      tier and the row's mark), price (level 1), finds (the AFindSpot ids it marks; done when all are
#                      found), mark_tag (a story secret: the actors carrying it are marked instead), when, done_when (a
#                      story secret's end), sets_flag (a story flag set as it's sold, for a line that knows)
_SECRETS_AFTER_MAIN2 = dict(after=['Main2'])
_SECRETS_AFTER_MAIN3 = dict(after=['Main3'])
_SECRETS_AFTER_MAIN4 = dict(after=['Main4'])
_SECRETS_AFTER_MAIN5 = dict(after=['Main5'])
_SECRETS_AFTER_LIFT = dict(after=['DeadLift'])

SEXTON_SECRETS = dict(
    asset='DA_Secrets_RansomsRest',
    title="The Ferryman's Secrets",
    tab='Mister Sexton',
    open_when=_SECRETS_AFTER_MAIN2,
    discount_mission='PaidOnDelivery',
    discount=0.25,
    greetings=[
        (dict(after=['PaidOnDelivery']), "Paid in full, friend. For prompt payment, a quarter off whatever I know."),
        (dict(), "The barge's manifest, friend. Bring it and we're square. Meanwhile, I sell what I know."),
    ],
    sold_out="That's all I know of the Rest, friend. For now.",
    secrets=[
        # --- Farm & Point and the West Road (after Main 2: he's just been met) ---
        dict(id='WidowsBox', title="The Widow's Strongbox", teaser="A strongbox from the gang's bank years.",
             line='Under the middle Widow, friend. North side, off the path. Mind your back.',
             region='Farm & Point', kind='Strongbox', price=100, finds=['WidowsBox'], when=_SECRETS_AFTER_MAIN2),
        dict(id='Mooring', title="The Gang's Locker", teaser='A locker left beside a skiff.',
             line='The gang left a locker on the Mooring Ledge. Nobody came back to pay for it.',
             region='Farm & Point', kind='Cache', price=40, finds=['Mooring'], when=_SECRETS_AFTER_MAIN2),
        dict(id='BoltHole', title='The Bolt-Hole', teaser='Where the gang waited out a day.',
             line="When the Dunnes ran, they ran to the Tumble. There's a hole under the rockfall.",
             region='Farm & Point', kind='Cache', price=40, finds=['BoltHole'], when=_SECRETS_AFTER_MAIN2),
        dict(id='RangerPost', title='Ranger Post Three', teaser="A Rangers' post nobody climbs to.",
             line='The Rangers kept a third post up on the Rimrock. Climb, friend. The stores are there.',
             region='Farm & Point', kind='Cache', price=40, finds=['RangerPost'], when=_SECRETS_AFTER_MAIN2),
        dict(id='Chimney', title="The Averys' Savings", teaser="A family's savings, kept from the fire.",
             line="The Averys never trusted a bank. Look up the chimney of what's left.",
             region='West Road', kind='Cache', price=40, finds=['Chimney'], when=_SECRETS_AFTER_MAIN2),
        dict(id='LanternHouse', title="The Keepers' Stores", teaser='What the keepers kept for long nights.',
             line="The old lantern house by the keeper's grave. Keepers stocked it for long nights.",
             region='West Road', kind='Cache', price=40, finds=['LanternHouse'], when=_SECRETS_AFTER_MAIN2),
        # --- After Main 3: the town is open ---
        dict(id='Tollman', title='The Tollman', teaser="A collector who walks the dead's road.",
             line='A tollman walks the cairns road, collecting from the dead. Collect from him.',
             region='West Road', kind='Elite', price=150, finds=['Tollman'], when=_SECRETS_AFTER_MAIN3),
        dict(id='Hoard', title="The Spiders' Hoard", teaser='What the spiders keep.',
             line='Spiders keep a hoard under the hanging tree on boot hill. Bring a light.',
             region='Town & Boot Hill', kind='Cache', price=40, finds=['Hoard'], when=_SECRETS_AFTER_MAIN3),
        dict(id='MiserGrave', title="The Miser's Fare", teaser="A miser's fare, taken into the ground.",
             line="A miser on boot hill took his fare into the ground. Dig, friend. He won't mind.",
             region='Town & Boot Hill', kind='Cache', price=40, finds=['MiserGrave'], when=_SECRETS_AFTER_MAIN3),
        dict(id='LedgerPages', title="The Gang's Ledger Pages", teaser="Six pages of a dead man's accounts.",
             line="Six pages of the gang's ledger blew about the Rest. I've marked where they settled.",
             region='The Rest', kind='Lore', price=60,
             finds=['LedgerPage_HighwaterLanding', 'LedgerPage_DustwaterMesa', 'LedgerPage_SaintAgnesMire',
                    'LedgerPage_FurnaceHollow', 'LedgerPage_TheNeedles', 'LedgerPage_RansomsRest'],
             when=_SECRETS_AFTER_MAIN3),
        # --- South Fields (after Main 4) ---
        dict(id='Bottle', title="Thaddeus's Bottle", teaser="A bottle Tilly mustn't hear about.",
             line='Thaddeus Bright hid a bottle in the springhouse. Under the flat stone. Not a word.',
             region='South Fields', kind='Cache', price=40, finds=['Bottle'], when=_SECRETS_AFTER_MAIN4),
        dict(id='CoffinRock', title="The Surveyor's Tin", teaser="A surveyor's tin, up high.",
             line='A surveyor left his tin on top of Coffin Rock. The north face slab is your stair.',
             region='South Fields', kind='Cache', price=40, finds=['CoffinRock'], when=_SECRETS_AFTER_MAIN4),
        dict(id='PikesCache', title="Pike's Cache", teaser="A man's pay, buried by the yard.",
             line='Pike buried his pay off his own bench mark. Find the mark, friend, then count yards.',
             region='South Fields', kind='Named', price=100, finds=['PikesCache'], when=_SECRETS_AFTER_MAIN4),
        # --- After Main 5: the Sink (a story secret); after Dead Lift: the mountain ---
        dict(id='Gravemother', title="The Gravemother's Barrow", teaser='Where the Gravemother sleeps.',
             line="She keeps her barrow at the bottom of the Sink. Nobody's come back to complain.",
             region='The Rest', kind='Story', price=60, finds=[], mark_tag='Place_SinkFloor',
             when=_SECRETS_AFTER_MAIN5, done_when=dict(after=['Side3'])),
        dict(id='Powder', title="Teague's Powder Box", teaser="A powder box from the mine's boss.",
             line='Teague kept a powder box on Larkspur Bench, behind the big granite. Carry it gently.',
             region='The Mountain', kind='Cache', price=40, finds=['Powder'], when=_SECRETS_AFTER_LIFT),
        dict(id='Crate', title='The Fallen Crate', teaser='A crate the old derrick dropped.',
             line='The old derrick dropped a crate among the quarry blocks. Nobody went down for it.',
             region='The Mountain', kind='Cache', price=40, finds=['Crate'], when=_SECRETS_AFTER_LIFT),
        dict(id='Tarn', title="Where They Can't Wade", teaser="A lockbox where the dead won't follow.",
             line="Ruth Calder sank a lockbox in Larkspur Tarn, where the dead won't wade. Hold your breath.",
             region='The Mountain', kind='Named', price=100, finds=['Tarn'], when=_SECRETS_AFTER_LIFT),
    ],
)
