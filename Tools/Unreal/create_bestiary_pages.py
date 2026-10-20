"""Makes (or brings up to date) bestiary pages (UBestiaryEntry, Bestiary/BestiaryEntry.h) in /Game/Data/Bestiary: the
pages the Ledger shows. Run it in the open editor once the C++ with each page's actor class is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_bestiary_pages.py [Unpaid Sexton ...]"
With no names it makes every page below, and writes the places of the older, hand-made pages (OLDER_HABITATS: the brown
spider, the meadow slime and the training dummy; their other fields are the assets' own). It prints a BESTIARY line per page
and "BESTIARY done". Running it again writes
these fields again (name, section, kind, description, habitat, notes, page type, the Ledger's flags and conditions, actor
class, stand model and what it wears on its bones, its turn when given, order) and leaves the others as they are; other pages are never touched.

Three kinds of page (UBestiaryEntry::Page, Docs/Areas/RansomsRest.md "The Ledger"):
  - an actor page (ACTOR): something met in the world, its numbers read from its actor class and its model on the stand.
    It waits for its model: Looter.Bestiary.Entries wants a model on every actor page's stand, so until the model is
    imported the page isn't made, and the script says so.
  - a story character's page (STORY_CHARACTER): words, and a model on the stand when it has one (Hob's SK_Hob, or still
    ones: Mister Sexton seated on a length of the lookout's rail, his ledger on his knee), but no actor, numbers or
    kills (Sexton, Grandma Delia, Tilly, Father Aldana, Ranger Calder: a seated figure and talking doors and windows;
    Hob, who is never fought). Open once its known_when holds (the mission that introduces them).
  - a Ledger name (LEDGER_NAME): one of the seven names Sexton writes in the Ledger, its whereabouts (habitat) blank until
    found_when holds (the Keeper's Lantern finds Lucky Ned in Main 7; the others wait for their missions).
Pages ledger_only (and every Ledger name) are listed only once Sexton has handed the Ledger over, Main 2's last step; they
are written in his voice (first drafts, for the user to judge).

  DA_Bestiary_Unpaid      The Unpaid (AUnpaidCreature), in Enemies; written before the Ledger, in the field guide's voice.
  DA_Bestiary_BellRinger  The Bell-Ringer (ABellRingerCreature), in Enemies after the Unpaid; in the Ledger (met after
                          Main 3), in Sexton's voice. Its stand is SK_Unpaid wearing the bell and the rope.
  DA_Bestiary_Gravemother The Gravemother (AGravemotherCreature, Side 3), in Creatures after the brown spider; in the
                          Ledger, in Sexton's voice.
  DA_Bestiary_Spitter     The Venom Spitter (ASpitterSpider), in Creatures after the brown spider; written before the
                          Ledger, in the field guide's voice (as the brown spider's page). Waits for SK_Spitter.
  DA_Bestiary_Hob         Hob, in Friends: a story character's page with SK_Hob on the stand; in the Ledger, known after
                          Main 1 (he never fights, so the player never "meets" him the way they meet a creature).
  DA_Bestiary_Sexton      Mister Sexton, NPCs, seated on the stand; in the Ledger, known from the start (he wrote it).
  DA_Bestiary_Delia       Grandma Delia Ransom, NPCs; known after Main 1.
  DA_Bestiary_Tilly       Tilly Bright, Friends (Docs/Story.md's cast); known after Main 3.
  DA_Bestiary_Aldana      Father Moses Aldana, NPCs; known after Main 4.
  DA_Bestiary_Ruth        Ranger Ruth Calder, Friends (the cast's); known after Side 1 (her note on the board).
  DA_Bestiary_Amos        Amos Whitlock, Friends (Docs/Story.md: "Friend"): a story character's page with SK_Amos on the
                          stand wearing his hat and hay fork on their bones (he has no actor class to read them from:
                          bone_parts); in the Ledger, known after Main 4 (when Side 2 opens).
  DA_Bestiary_DeadMiner   The dead miners (ADeadMinerCreature), in Enemies after the Bell-Ringer; in the Ledger, in Sexton's
                          voice. Its stand is SK_DeadMiner with the felt hat and pick (bone_parts).
  DA_Bestiary_Foreman     The Foreman, Cyrus Teague (AForemanCreature, Back Pay), in Enemies after the dead miners; in the
                          Ledger, in Sexton's voice. SK_Foreman with his bowler, pick handle and satchel on the stand.
  DA_Bestiary_Hiram       Hiram Coyle, Friends: a story character's page, SK_Hiram and his cap (bone_parts); in the Ledger,
                          known after Main 5 (when Dead Lift opens).
  DA_Bestiary_Thaddeus    Thaddeus Bright, Friends: as Hiram's, SK_Thaddeus and his spectacles; known after Knocking Below
                          (he's its discovery, not its giver).
  DA_Bestiary_Ned ... _Deacon  the seven names, in Enemies before the Unpaid, in the order the lantern finds them.
  DA_Bestiary_Town_<Town> the seven robbed towns (TownHighwaterLanding ... TownRansomsRest), NPCs after Aldana: a story
                          character's page each, its words the loose Ledger page's (story_texts.py LEDGER_TOWNS), open once
                          that page is read (with_flags Ledger.Page.<Town>).
"""
import importlib
import os
import sys

import unreal

FOLDER = '/Game/Data/Bestiary'

# The seven names: the Ledger lists them first among the Enemies, in the order the lantern will find them.
NAMES_FIRST = -7

PAGES = {
    'Unpaid': dict(
        asset='DA_Bestiary_Unpaid',
        name='The Unpaid',
        section='ENEMY',
        kind='Restless dead',
        actor='/Script/AI_Looter_Shooter.UnpaidCreature',
        model='/Game/Art/Creatures/SK_Unpaid',
        habitat="Ransom's Rest: the town gate, the chapel yard, boot hill and the north road",
        description=("The dead of Ransom's Rest, come home. When Saint Ada went dark, every soul of the town still on the "
                     "Sundown Road lost the light and drifted back to its own grave. They're pale shapes in the clothes "
                     "they died in, fading below the waist into a trailing shroud, with a coal burning where the heart "
                     "should be. Cold, hungry and angry, and they know a gun in a dead hand can hurt them."),
        notes=[
            "The coal in its chest is the weak spot: a shot whose line runs through it is critical, for 1.5 times the "
            "damage. It only shows from the front, and an arm in the way takes the hit.",
            "It drifts after you, then shrieks and lunges. The shriek is the warning: back off and the lunge falls short.",
            "Leave it stuck behind a fence, or far behind you, and it fades out and comes back three to five meters closer.",
            "Its coal burns in its rank's color: dull red, blue for the Restless (two and a half times as tough, and quicker "
            "to lunge), purple for the Gravebound (five times as tough, and its shriek sends out a ring that slows you for "
            "two seconds).",
            "They won't cross Delia's salt line round the farm.",
            "Every Unpaid drops ammo, most often for the gun that killed it, and about one in three drops a weapon. The "
            "ones a fight brings don't come back.",
        ],
        sort_order=0,
    ),
    'BellRinger': dict(
        asset='DA_Bestiary_BellRinger',
        name='The Bell-Ringer',
        section='ENEMY',
        kind="The chapel's ringer",
        actor='/Script/AI_Looter_Shooter.BellRingerCreature',
        model='/Game/Art/Creatures/SK_Unpaid',
        ledger_only=True,
        habitat="Ransom's Rest: behind the Unpaid at the orchard's edge, and among the churchyard's graves",
        description=("He rang Saint Ada's bell for forty years, friend: Sundays, weddings, and every burial on the Rest, "
                     "walking ahead of the coffin with his hand bell. Nobody rang one for him. He came home with the rest "
                     "of the Unpaid when she went dark, and he still walks ahead of the dead. They heed that bell. When it "
                     "tolls over a fight, the fallen get back up."),
        notes=[
            "Kill him first. He hangs back behind the others, ten meters or so off, and backs away if you close on him. Run "
            "him down and he swings the bell at you, though not hard.",
            "Before he tolls he lifts the bell high beside his head and shakes it, and a halo of light gathers round it. "
            "Shoot the bell then and the toll is lost: he reels, and it's a while before he tries again.",
            "The bell is a weak spot whenever you can hit it: critical, for 1.5 times the damage. So is his coal.",
            "His toll raises one of the Unpaid that fell near him in the last twenty seconds, at half its strength. He raises "
            "two at most, and the ones he raises stay down the second time.",
            "With nobody fallen to raise, the toll quickens the Unpaid round him, and him, for a few seconds: a pale ring "
            "at their feet.",
            "A Restless ringer tolls sooner. A Gravebound one's bell is cracked, and its sour toll slows you, as their "
            "shriek does.",
            "He drops ammo, most often for the gun that killed him, and about one in three times a weapon.",
        ],
        sort_order=1,
    ),
    # The Providence's dead (Docs/Areas/RansomsRest_Expansion.md, section 6): the mountain's enemies, after the Bell-Ringer.
    'DeadMiner': dict(
        asset='DA_Bestiary_DeadMiner',
        name='Dead Miners',
        section='ENEMY',
        kind="The Providence's dead",
        actor='/Script/AI_Looter_Shooter.DeadMinerCreature',
        model='/Game/Art/Creatures/SK_DeadMiner',
        bone_parts=[('/Game/Art/Creatures/SM_DeadMinerHat', 'hat'), ('/Game/Art/Creatures/SM_DeadMinerPick', 'pick')],
        ledger_only=True,
        habitat="Ransom's Rest: the Old Quarry, Larkspur Bench, Calder's Post and the Providence mine",
        description=("Men who went down for silver and were owed for it, friend. Forty years of the Providence's buried dead "
                     "came home to the mine when Saint Ada went dark, and the eleven of the fall never left it: nobody brought "
                     "them up to be given to the wind. They still work for wages that will never come. Some debts outlive the "
                     "debtor."),
        notes=[
            "The lamp on its cap is a weak spot: a shot on it is critical, for 1.5 times the damage, and the first one puts "
            "it out. So is its coal.",
            "Before it swings it heaves the pick over its head and its lamp flares. Back off then, or shoot the lamp: the "
            "swing is lost and it reels.",
            "After a swing its pick sits in the floor a moment. That's your moment.",
            "Left alone they work, picks at the walls or over their lunch pails. Listen for the ringing in the dark.",
            "A Restless one swings sooner. A Gravebound one's blow rings through the rock and slows you, as the Unpaid's "
            "shriek does.",
            "They drop ammo, most often for the gun that killed them, and about one in three times a weapon.",
        ],
        sort_order=2,
    ),
    'Foreman': dict(
        asset='DA_Bestiary_Foreman',
        name='The Foreman',
        section='ENEMY',
        kind='Soulfed foreman',
        actor='/Script/AI_Looter_Shooter.ForemanCreature',
        model='/Game/Art/Creatures/SK_Foreman',
        bone_parts=[('/Game/Art/Creatures/SM_ForemanBowler', 'hat'), ('/Game/Art/Creatures/SM_ForemanPickHandle', 'pick'),
                    ('/Game/Art/Creatures/SM_ForemanSatchel', 'spine_02')],
        ledger_only=True,
        habitat="Ransom's Rest: the stope at the bottom of the Providence, digging at the fall",
        description=("Cyrus Teague, foreman of the Providence Mining Company. He brought the roof down on eleven men to save "
                     "the company a quarter's wages, and has dug at it every night since. Remorse is a poor shovel, friend."),
        notes=[
            "His coat covers his coal. It shows while he lights a fuse, and for a few seconds after he strikes the roof: then "
            "a shot through it is critical, for 1.5 times the damage. Enough of those close together and he goes down on a "
            "knee.",
            "A fuse hissing in his fist means it's coming your way. Where it lands the floor cracks in a ring: be out of it "
            "before it goes. Knock him down while it's lit and he drops it at his own feet.",
            "Past two thirds he takes to the props. When dust sifts from the roof, move: rock follows, and the prop you were "
            "behind may not be there after.",
            "He calls up his shift at two thirds and again at one third: three of the dead out of the rubble each time.",
            "At half he puts the lamps out. Watch for the miners' lamps and their coals.",
            "Every kill drops two guns from a Legendary's table. Beaten, he's gone a while; the vigil stone by the mine's "
            "mouth calls him back.",
        ],
        sort_order=3,
    ),
    'Gravemother': dict(
        asset='DA_Bestiary_Gravemother',
        name='The Gravemother',
        section='CREATURE',
        kind='Soulfed spider',
        actor='/Script/AI_Looter_Shooter.GravemotherCreature',
        model='/Game/Art/Creatures/SK_Gravemother',
        ledger_only=True,
        habitat="Ransom's Rest: the Sink, in the den under Den Rock",
        description=("The Sink's old queen. She was a Meadow Wolf once, friend, before she grew fat on what the Unpaid "
                     "leave lying about; now she's broad as a hearse and caked in grave mud, with somebody's headstone "
                     "bound to her back by its roots, and every spider in that hole is hers. She doesn't care for callers."),
        notes=[
            "That headstone on her back turns bullets. Break it (it cracks twice before it goes), or put your rounds under "
            "its back edge, where she's soft. Her head's soft too. Once the stone's off, all of her backside is.",
            "When she digs in head-low and the ground splits toward you, step off the line: she ploughs it, and can't turn "
            "once she's going. The earth she throws up either side is cover a while.",
            "At the end of a plough she slams down and is slow to rise. That's your moment.",
            "When she tears a clod out of the floor, shoot it while she's holding it: it bursts all over her and she's "
            "dazed a moment. Thrown, watch for its shadow. The pile it leaves is cover.",
            "Bleed her past two thirds, and again past one third, and she slams the floor: four mounds swell up round her. "
            "Shoot them before they break and nothing climbs out.",
            "Past one third that stone comes off by itself, and she gets quicker on her feet.",
            "She keeps to the Sink and her den. Go up the ramp and she lets you be.",
            "Every kill drops two guns, at the best odds this side of a Keeper. Kill her and she's gone a while: come back "
            "after twenty minutes or more and she'll be home.",
        ],
        sort_order=1,
    ),
    'Spitter': dict(
        asset='DA_Bestiary_Spitter',
        name='Venom Spitter',
        section='CREATURE',
        kind='Spitting spider',
        actor='/Script/AI_Looter_Shooter.SpitterSpider',
        model='/Game/Art/Creatures/SK_Spitter',
        habitat="Ransom's Rest: the quarry track and the Webwood",
        description=("A thin-legged cousin of the hunting spider that would rather not come near you. It carries its venom "
                     "in a swollen sac on its back, glowing green through the skin, and lobs it from a safe distance: a "
                     "glob that bursts where it lands and leaves a puddle that eats at whatever stands in it."),
        notes=[
            "The glowing sac on its back is its soft spot, and so is its head: hits there are critical, for 1.5 times the "
            "damage.",
            "Before it spits it rears up, its sac swells and throbs bright, and it hisses. Keep moving and change direction "
            "then: it aims a little ahead of you.",
            "Burst the sac while it's swelling and the spit chokes. Nothing flies, and it has to start over.",
            "Its puddles burn for a few seconds. Step out of them.",
            "It keeps fifteen meters or so between you, backing off as you come and sidestepping while you shoot. It backs "
            "up slower than you walk, and it only bites when it's caught or cornered.",
            "A Restless one spits sooner. A Gravebound one throws three globs at once.",
            "Every Spitter drops ammo, most often for the gun that killed it, and about one in three drops a weapon. The "
            "ones a fight brings don't come back.",
        ],
        sort_order=1,
    ),
    'Hob': dict(
        asset='DA_Bestiary_Hob',
        page='STORY_CHARACTER',
        name='Hob',
        section='FRIEND',
        kind='One-eyed crow',
        model='/Game/Art/Creatures/SK_Hob',
        ledger_only=True,
        known_when=dict(after=['Main1']),
        habitat="Wherever you ought to be going next",
        description=("My messenger. Only you can hear him, which is a mercy to everyone else. He talks too much and flies "
                     "too little, and he's never far from where you ought to be."),
        notes=[
            "He perches near the next thing to be done.",
            "Go off a high edge and he'll catch you, and complain about the weight.",
        ],
        sort_order=0,
    ),

    # --- The story's characters: words only ---
    'Sexton': dict(
        asset='DA_Bestiary_Sexton',
        page='STORY_CHARACTER',
        name='Mister Sexton',
        section='NPC',
        kind='A man of business',
        ledger_only=True,
        habitat="Ransom's Point: the lookout's rail",
        # Seated on a length of the lookout's rail, as the lookout has him, with his ledger on his knee: still models,
        # not skinned ones, on the stand piece's sockets.
        still_model='/Game/Art/Characters/SM_SextonStand',
        still_parts=[('/Game/Art/Characters/SM_MisterSexton', 'Sit'), ('/Game/Art/Characters/SM_SextonLedger', 'Ledger')],
        description=("The undersigned. I attend every death, friend; it's my trade. I keep this book and the accounts in "
                     "it, and I see that what's owed is paid."),
        notes=[
            "Terms: seven embers, each brought to the lookout on Ransom's Point at the new moon.",
            "In return: your rest, and your father's fare across.",
            "Mister Sexton keeps his word. He would thank you to keep yours.",
        ],
        sort_order=0,
    ),
    'Delia': dict(
        asset='DA_Bestiary_Delia',
        page='STORY_CHARACTER',
        name='Grandma Delia Ransom',
        section='NPC',
        kind="The Keeper's mother",
        ledger_only=True,
        known_when=dict(after=['Main1']),
        habitat="Ransom Farm: behind the screen door",
        description=("Abel Ransom's mother. She buried her son and her grandchild on the same day, and her son got up on "
                     "the Wednesday night. She won't open her door to you, friend, but she still sets a plate."),
        notes=[
            "She keeps the salt line round the farm. The Unpaid won't cross it.",
            "Talk to her through the screen door. She doesn't open it to the dead.",
        ],
        sort_order=1,
    ),
    'Aldana': dict(
        asset='DA_Bestiary_Aldana',
        page='STORY_CHARACTER',
        name='Father Moses Aldana',
        section='NPC',
        kind="Preacher of Ransom's Rest",
        ledger_only=True,
        known_when=dict(after=['Main4']),
        habitat="The Chapel of Saint Ada: the vestry door",
        description=("The Rest's preacher. He is terrified of you, friend, and he has reason: his saint is dark and his "
                     "flock walks the chapel yard. Frightened men often come round. He will."),
        notes=[
            "He talks through the vestry door.",
        ],
        sort_order=2,
    ),
    'Tilly': dict(
        asset='DA_Bestiary_Tilly',
        page='STORY_CHARACTER',
        name='Tilly Bright',
        section='FRIEND',
        kind="The undertaker's daughter",
        ledger_only=True,
        known_when=dict(after=['Main3']),
        habitat="Main Street: Bright & Daughter, Undertakers",
        description=("She dressed you and your father for the box, and she was the first in town to speak to you after. "
                     "Practical, which in her trade is a kindness. Her late father's hearse car still runs on the line."),
        notes=[
            "She talks through her shop window. The card says she's back after the funeral.",
            "She knows the old rites.",
        ],
        sort_order=1,
    ),
    'Ruth': dict(
        asset='DA_Bestiary_Ruth',
        page='STORY_CHARACTER',
        name='Ranger Ruth Calder',
        section='FRIEND',
        kind='Rim Ranger',
        ledger_only=True,
        known_when=dict(after=['Side1']),
        habitat="Her signature on the wanted posters",
        description=("A Rim Ranger hunting the Dunne Gang. Her name is on every poster of you in this valley. She'll shoot "
                     "on sight, friend; she doesn't yet know what you are."),
        notes=[
            "She signs herself Capt. R. Calder.",
            "She leaves caches for her Rangers.",
        ],
        sort_order=2,
    ),
    # The doc has him an Enemy until The Gravewind is done, then a Friend; a page can't move sections yet, so he waits
    # in Friends until his fight is over, with his fight's stats on his page.
    'Abel': dict(
        asset='DA_Bestiary_Abel',
        name='Abel Ransom, the Keeper',
        section='FRIEND',
        kind='Keeper of Saint Ada',
        actor='/Script/AI_Looter_Shooter.AbelKeeper',
        model='/Game/Art/Creatures/SK_Abel',
        ledger_only=True,
        known_when=dict(after=['Main6']),
        habitat="Ransom's Rest: his board on Gravewind Point's burial deck",
        description=("Abel Ransom, Keeper of Saint Ada, and your father, friend. He hung the lantern on Gravewind Point at "
                     "dusk so the dead could find their way west. Ned Purcell shot him before he could raise it, and he got "
                     "up out of his grave to keep the boards himself. He's quieter now. He sits on his board facing the "
                     "sunset and waits for you to bring Saint Ada home."),
        notes=[
            "His coal is soft only while he grieves, turned to the sunset with his lantern arm down, or while he's stunned "
            "from being dragged back. Then a shot through it is critical, for 1.5 times the damage.",
            "When his lantern flares, a spread of spectral buckshot follows a second later. Put a bier between you.",
            "Come too close and he swings the stock.",
            "When the chapel bell rings he drifts out over the canyon where no gun reaches him. Relight the deck's three "
            "lanterns (hold at each) and they drag him back.",
            "At the last the Gravewind pours across the deck toward the open end. Keep off the end. Hob will fetch you back, "
            "and he'll have something to say about it.",
            "He isn't fought again. He keeps the boards now.",
        ],
        sort_order=3,
    ),
    # A story character with a skinned model: his hat and fork go on its bones as he wears them at his fence.
    'Amos': dict(
        asset='DA_Bestiary_Amos',
        page='STORY_CHARACTER',
        name='Amos Whitlock',
        section='FRIEND',
        kind='A patient ghost',
        model='/Game/Art/Creatures/SK_Amos',
        bone_parts=[('/Game/Art/Creatures/SM_AmosHat', 'hat'), ('/Game/Art/Creatures/SM_AmosFork', 'fork')],
        ledger_only=True,
        known_when=dict(after=['Main4']),
        habitat="Ransom's Rest: his fence in Whitlock Fields, by the gate where the fields path passes",
        description=("Amos Whitlock farmed the fields along Mill Creek. He died last harvest with his hay half in, a year after "
                     "the fever winter took his hired men. He was still on the Sundown Road when Saint Ada went dark, and "
                     "he drifted home to find the hay rotting where he left it. A patient man. He isn't angry yet, friend. "
                     "Give it time."),
        notes=[
            "Only you can see him. To the living he's a cold spot by a fence.",
            "He can't lift a straw: his hands pass through it. He'd be obliged if yours didn't.",
            "His coal is banked low, under ash. He has no quarrel with you.",
            "Since his hay went in he sits on his fence, waiting for Saint Ada to come home.",
        ],
        sort_order=4,
    ),
    # The friendly dead of the expansion (Docs/Areas/RansomsRest_Expansion.md, section 4): story characters, each on their
    # own model (sheet 40) with their things as bone parts.
    'Hiram': dict(
        asset='DA_Bestiary_Hiram',
        page='STORY_CHARACTER',
        name='Hiram Coyle',
        section='FRIEND',
        kind="The ore lift's engineman",
        model='/Game/Art/Creatures/SK_Hiram',
        bone_parts=[('/Game/Art/Creatures/SM_HiramCap', 'hat')],
        ledger_only=True,
        known_when=dict(after=['Main5']),
        habitat="Ransom's Rest: the winch house at the Old Quarry, on his stool by the boiler",
        description=("Hiram Coyle ran the Providence's ore lift for twenty years and died at his brake. He hasn't left, "
                     "friend. He sits by a boiler he can't light and waits for a passenger, as he has since the mine shut."),
        notes=[
            "Bring him his lift's crank, coal for the boiler and its clamp pin, and he'll take you up the mountain.",
            "He talks to the lift head through the speaking tube. Anyone talking up there is talking to Hiram.",
            "He keeps an account of the coal the Rangers owe him. He means to collect.",
        ],
        sort_order=5,
    ),
    'Thaddeus': dict(
        asset='DA_Bestiary_Thaddeus',
        page='STORY_CHARACTER',
        name='Thaddeus Bright',
        section='FRIEND',
        kind="Tilly's father",
        model='/Game/Art/Creatures/SK_Thaddeus',
        bone_parts=[('/Game/Art/Creatures/SM_ThaddeusSpectacles', 'hat')],
        ledger_only=True,
        known_when=dict(after=['KnockingBelow']),
        habitat="Main Street: the cold room under Bright & Daughter, Undertakers",
        description=("Thaddeus Bright, undertaker, and Tilly's father. He measured every soul on the Rest for its box, yours "
                     "among them, friend. He came home from the Sundown Road to his work, and his work is in his cellar."),
        notes=[
            "He measures at his bench in the cold room, a coffin that isn't there.",
            "The knocking under Main Street at night is his knuckles on the board.",
            "He wrote his own order: pine, no brass. Tilly put brass on it anyway. He liked it.",
        ],
        sort_order=6,
    ),

    # --- The seven names: Enemies, before the Unpaid, whereabouts blank until the lantern finds them ---
    'Ned': dict(
        asset='DA_Bestiary_Ned',
        page='LEDGER_NAME',
        name='\u201cLucky\u201d Ned Purcell',
        section='ENEMY',
        kind='The gambler',
        habitat='The Gilded Lily',
        found_when=dict(after=['Main7']),
        description=("Plays cards for a living and calls it luck. He shot your father on the bluff path. He carries an "
                     "ember that isn't his, and fortune follows it about like a dog."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST,
    ),
    'Ira': dict(
        asset='DA_Bestiary_Ira',
        page='LEDGER_NAME',
        name='\u201cWhistling\u201d Ira Gale',
        section='ENEMY',
        kind='The horse thief',
        description=("Steals horses and whistles while he does it. The wind has taken a liking to him since he took his "
                     "ember."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 1,
    ),
    'Constance': dict(
        asset='DA_Bestiary_Constance',
        page='LEDGER_NAME',
        name='Sister Constance Holloway',
        section='ENEMY',
        kind="The gang's medic",
        description=("She patched up the gang for years, and she believes she's helping still. Mercy is a dangerous thing to "
                     "carry about, friend."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 2,
    ),
    'Barrels': dict(
        asset='DA_Bestiary_Barrels',
        page='LEDGER_NAME',
        name='Bartholomew \u201cBarrels\u201d Kessler',
        section='ENEMY',
        kind='The powder man',
        description="He blew the safes and the bridges. His ember runs hot in him; he'll be burning by now.",
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 3,
    ),
    'Lena': dict(
        asset='DA_Bestiary_Lena',
        page='LEDGER_NAME',
        name='Lena \u201cSpyglass\u201d Okoro',
        section='ENEMY',
        kind='The sniper',
        description="She never misses, and she never forgets a face. She will have seen you before you see her.",
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 4,
    ),
    'Tobias': dict(
        asset='DA_Bestiary_Tobias',
        page='LEDGER_NAME',
        name='Tobias \u201cMule\u201d Grant',
        section='ENEMY',
        kind='The strongman',
        description=("He carried the gang's heavy loads and never once asked why. He doesn't want a fight. That won't save "
                     "him."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 5,
    ),
    'Deacon': dict(
        asset='DA_Bestiary_Deacon',
        page='LEDGER_NAME',
        name='Ambrose Dunne, the Deacon',
        section='ENEMY',
        kind="The gang's leader",
        description=("A preacher thrown out of his church, and the man who shot you. He kept this valley's ember for "
                     "himself. Last on the list, friend, and the hardest."),
        notes=["Owes one ember: Saint Ada's."],
        sort_order=NAMES_FIRST + 6,
    ),
}


# The seven towns the gang robbed (the Rest's seeds, 2026-10-16): one page each, its words the loose Ledger page's own
# (Tools/Unreal/story_texts.py LEDGER_TOWNS, the one place for them). A story character's page in the NPCs, in the
# Ledger, closed ("???") until its loose page is read (the readable sets Ledger.Page.<Town>): the bestiary has no Places
# section, and a LEDGER_NAME page would show its words from the start and break Looter.Ledger's seven names. Sorted after
# Sexton, Delia and Aldana, in the order the lantern finds the names, Ransom's Rest last. Stoneweight Quarry's page lies
# elsewhere in the campaign, so it stays closed on Ransom's Rest.
TOWNS_FIRST = 10


def town_pages():
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import story_texts
    texts = importlib.reload(story_texts)
    pages = {}
    for order, (town, page) in enumerate(texts.LEDGER_TOWNS.items()):
        flag = 'Ledger.Page.' + town
        pages['Town' + town] = dict(
            asset='DA_Bestiary_Town_' + town,
            page='STORY_CHARACTER',
            name=page['name'],
            section='NPC',
            kind=page['saint'] + "'s town",
            ledger_only=True,
            known_when=dict(with_flags=[flag]),
            found_when=dict(with_flags=[flag]),
            habitat=page['habitat'],
            description=page['text'],
            notes=['Found: ' + (page['found'] or 'not yet') + '.', 'Its ember: carried by ' + page['carrier'] + '.'],
            sort_order=TOWNS_FIRST + order,
        )
    return pages


PAGES.update(town_pages())


# Pages made by hand before this script, whose words, numbers and models are the assets' own: only where they are found
# is written here. They read "Tutorial Island: ..." until 2026-10-14; the map order is Ransom's Rest first now (spiders nest
# on Ransom's Point at the top of the bluff path, in the Sink and on the mountain's crest; the slimes are Mill Creek's,
# above Mill Falls), Skyreach (the old tutorial island) third, and the dummies stand only there (Docs/Areas/RansomsRest.md
# "Encounters stay on one level of ground", RansomsRest_Expansion.md section 6). Written again as they are, so a rerun
# changes nothing.
OLDER_HABITATS = {
    'BrownSpider': ('DA_Bestiary_BrownSpider',
                    "Ransom's Rest: Ransom's Point at the top of the bluff path, the Sink, the quarry track and the "
                    "mountain's crest"),
    'MeadowSlime': ('DA_Bestiary_MeadowSlime', "Ransom's Rest: Mill Creek's bottom, above Mill Falls"),
    'TrainingDummy': ('DA_Bestiary_TrainingDummy', 'Skyreach: the meadow under the windmill'),
}


def fix_older_habitat(key):
    """Writes the whereabouts of one of the older pages (OLDER_HABITATS); False when it isn't there."""
    asset_name, habitat = OLDER_HABITATS[key]
    path = f'{FOLDER}/{asset_name}'
    asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if asset is None:
        unreal.log_warning(f'BESTIARY {path} is not there: its place is left alone')
        return False
    asset.set_editor_property('habitat', unreal.Text(habitat))
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    unreal.log(f'BESTIARY {path} updated: found at "{habitat}"')
    return True


def entry_class():
    cls = getattr(unreal, 'BestiaryEntry', None)
    if cls is None or getattr(unreal, 'BestiaryPage', None) is None:
        raise RuntimeError('unreal.BestiaryEntry or unreal.BestiaryPage is missing: build the C++ first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a BestiaryEntry: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def condition(after=(), before=(), with_flags=(), without_flags=()):
    """FStoryCondition: after these missions, before those, with and without these story flags (empty: always, or for
    found_when, never)."""
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    made.set_editor_property('before_missions', [unreal.Name(m) for m in before])
    made.set_editor_property('with_flags', [unreal.Name(f) for f in with_flags])
    made.set_editor_property('without_flags', [unreal.Name(f) for f in without_flags])
    return made


def set_actor_class(asset, path):
    """The page's actor class, a soft class reference, by its path; read back to be sure."""
    try:
        asset.set_editor_property('actor_class', unreal.SoftClassPath(path))
    except TypeError:
        asset.set_editor_property('actor_class', unreal.load_class(None, path))
    stored = asset.get_editor_property('actor_class')
    text = stored.export_text() if hasattr(stored, 'export_text') else (stored.get_path_name() if stored else '')
    if path not in text:
        raise RuntimeError(f"{asset.get_path_name()}: its actor class reads back as '{text}', not {path}: nothing was saved")


def set_model(asset, path, prop='preview_mesh'):
    """The stand's model, set on the page itself: the class's own mesh is found only once the editor has started with it.
    prop is preview_mesh (skinned) or preview_static_mesh (a still figure)."""
    object_path = f"{path}.{path.rsplit('/', 1)[-1]}"
    try:
        asset.set_editor_property(prop, unreal.SoftObjectPath(object_path))
    except TypeError:
        asset.set_editor_property(prop, unreal.load_asset(path))


def setup(spec, cls):
    path = f"{FOLDER}/{spec['asset']}"
    page = spec.get('page', 'ACTOR')
    if page == 'ACTOR' and unreal.load_class(None, spec['actor']) is None:
        raise RuntimeError(f"{spec['actor']} is not a class: build the C++ first; nothing was changed")
    models = ([spec[key] for key in ('model', 'still_model') if key in spec] + [p for p, _ in spec.get('still_parts', [])]
              + [p for p, _ in spec.get('bone_parts', [])])
    for model in models:
        if not unreal.EditorAssetLibrary.does_asset_exist(model):
            unreal.log_warning(f"BESTIARY {path} waits for its model {model}: import it first (Looter.Bestiary.Entries wants "
                               f"a model on every actor page's stand, and the one a page names); nothing was changed")
            return False
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('category', getattr(unreal.BestiaryCategory, spec['section']))
    asset.set_editor_property('kind', unreal.Text(spec['kind']))
    asset.set_editor_property('description', unreal.Text(spec['description']))
    asset.set_editor_property('habitat', unreal.Text(spec.get('habitat', '')))
    asset.set_editor_property('notes', [unreal.Text(note) for note in spec['notes']])
    asset.set_editor_property('sort_order', spec['sort_order'])
    asset.set_editor_property('page', getattr(unreal.BestiaryPage, page))
    asset.set_editor_property('ledger_only', spec.get('ledger_only', False))
    asset.set_editor_property('known_when', condition(**spec.get('known_when', {})))
    asset.set_editor_property('found_when', condition(**spec.get('found_when', {})))
    if page == 'ACTOR':
        set_actor_class(asset, spec['actor'])
    if 'model' in spec:
        set_model(asset, spec['model'])
    if 'still_model' in spec:
        set_model(asset, spec['still_model'], 'preview_static_mesh')
    if 'still_parts' in spec:
        parts = []
        for mesh, socket in spec['still_parts']:
            part = unreal.BestiaryStillPart()
            part.set_editor_property('mesh', unreal.load_asset(mesh))
            part.set_editor_property('socket', socket)
            parts.append(part)
        asset.set_editor_property('preview_still_parts', parts)
    if 'bone_parts' in spec:
        worn = []
        for mesh, bone in spec['bone_parts']:
            part = unreal.BestiaryBonePart()
            part.set_editor_property('mesh', unreal.load_asset(mesh))
            part.set_editor_property('bone', bone)
            worn.append(part)
        asset.set_editor_property('preview_bone_parts', worn)
    if 'stand_yaw' in spec:
        asset.set_editor_property('preview_yaw', spec['stand_yaw'])
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    what = (f"actor {spec['actor']}, stand {spec['model']}" if page == 'ACTOR'
            else 'one of the seven names' if page == 'LEDGER_NAME'
            else 'a story character, no actor' + (f", stand {spec.get('model') or spec.get('still_model')}"
                                                     if 'model' in spec or 'still_model' in spec else ''))
    when = spec.get('known_when', {}).get('after') or spec.get('found_when', {}).get('after')
    unreal.log(f"BESTIARY {path} {'made' if created else 'updated'}: \"{spec['name']}\" in {spec['section'].title()} "
               f"({spec['kind']}), {len(spec['notes'])} notes, {what}"
               f"{', in the Ledger' if spec.get('ledger_only') or page == 'LEDGER_NAME' else ''}"
               f"{', after ' + ', '.join(when) if when else ''}")
    return True


def run():
    cls = entry_class()
    # No names: every page below and the older pages' places. Names: only those (PAGES' keys or OLDER_HABITATS').
    asked = [name for name in sys.argv[1:] if name in PAGES or name in OLDER_HABITATS]
    wanted = [name for name in asked if name in PAGES] or ([] if asked else list(PAGES))
    older = [name for name in asked if name in OLDER_HABITATS] or ([] if asked else list(OLDER_HABITATS))
    waiting = [key for key in wanted if not setup(PAGES[key], cls)]
    for key in older:
        fix_older_habitat(key)
    if waiting:
        unreal.log(f"BESTIARY done; waiting for their models: {', '.join(waiting)}")
    else:
        unreal.log('BESTIARY done')


run()
