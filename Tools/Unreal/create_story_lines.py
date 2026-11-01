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

  Intro    NewGameIntro (the new game's intro narration, Story/NewGameIntro.h: 11 lines, no speaker, in the
           captions' order; the code keeps the same words as its fallback and Looter.Intro.Lines checks they match;
           lines 0-3, the planet part, are the Wardens story's since 2026-10-31; 4-10 the old Rest part's, unplayed)
  Bosses   BossIntroGravemother, BossIntroForeman, BossIntroAbel (the boss intros' captions, Scenes/BossIntro.h: line N
           fills the script's caption slot N; the code keeps the same words as its fallback and
           Looter.Scenes.BossIntro.Lines checks they match)
The main story (2026-10-21, the user's cut of the story's talk: offers two or three lines, Hob's perch arrivals one line,
his taps only Main 1's, the Old Fare's and a catch-all; where a perch had only directions he lands silent):
  Main 1   Landfall (2026-10-31, the Wardens story): LandfallDeck, LandfallVaneHorn, LandfallBoarders,
           LandfallBoardersWave2, LandfallGrapples, LandfallLanding, LandfallAnchor, LandfallClimb, LandfallPoint,
           LandfallVaneValley, LandfallTrail, HobLandfallPorch, DeliaFight (her barks),
           DeliaLandfallWait, DeliaLandfall and DeliaLandfallHat (the turn-in, her topics), DeliaPorch (after),
           HeadboardAbel, HobMain1, HobAfterMain1 (his catch-all tap). Who plays each: the comment over them.
  Main 2   The Ferryman's Price (2026-11-01, the Wardens story): HobMain2Blue (the Widows' nest cleared), HobMain2Bridge
           (Keeper's Bridge's mutter), HobMain2Cart (his arrival at the Keeper's Gate), HobMain2Spill (the cart stopped),
           Main2VaneCart (Vane's complaint on the gate's loudspeaker), HobMain2Ladder (his arrival at the ladder),
           SextonWaiting, SextonDeal, SextonLedger, SextonAfter (Sexton at his ferry)
  Main 3   Under New Management (2026-11-01): HobMain3Road, Main3Radio and Main3NedAfter (Lucky Ned's skiff leaving),
           HobMain3Gate, Main3VaneGate (the gate's loudspeaker), Main3VaneStreet (Main Street's, the street won),
           HobMain3Tilly, TillyClosed, TillyMain3 (her talk, the turn-in's words from step 3: she hands the bail over),
           TillyAfterMain3, DeliaMain3 (her word meanwhile)
  Main 4   HobMain4Yard/Reliquary, AldanaBarred, AldanaWaiting (the knock at the vestry door, step 2), AldanaBell (the bell
           and the Reliquary waiting), AldanaMain4 (the turn-in's words), AldanaAfterMain4
  Main 5   HobMain5Way/Out, AldanaAfterMain5
  The mine (main since 2026-10-27: Dead Lift, Back Pay, Cyrus Teague the second boss) DeliaMine (her door meanwhile);
           Teague's build-up is otherwise the missions' own words (Main 5's turn-in, Back Pay's offer at the lift tube)
  Main 6   DeliaMain6, HobMain6Way/Fight, AbelOnBoard, AbelAfterMain7
  Main 7   DeliaMain7, DeliaMain7After, HobMain7Lean, TillyMain7, TillyMain7Father (once Knocking Below is done),
           TillyAfterMain7
The valley's side missions:
  Old Fare AgentDoor, AgentCoins (Mr. Sloane's door: waiting, then the confession), AgentWindow, AgentWindowLily (his
           ticket window after it), FennLook, FennCoins (Fenn's coffin: looked into, the coins laid back), FennAfter,
           HobOldFare; the names build_area_missionprops_valley.py looks for (kept whole: Fenn's fare, Docs/Story.md)
  Side 2   AmosMeet (his default lines), AmosBales, AmosRoad (the twist: said as the last of his hands falls, the
           WhitlockHands spawner's cleared_lines), AmosHands, AmosThanks, AmosFence, AmosAfterMain6, AmosBaleFirst,
           HobBaleThird (the second bale now), AmosBaleLast
  Knocking TillyKnocking, TillyAfterKnocking, ThaddeusTalk, ThaddeusAfter (his bottle in the springhouse: a secret),
           ThaddeusAfterMain7
The mountain:
  Dead Lift  HiramWaiting, HiramFit, HiramSteam, HiramRide, HiramAfter, HiramCrew (a mutter as the lift crew rise),
             LiftTubeAfter, HaskettLift (the Haskett house's mutter once the lift runs)
  Back Pay   HobBackPayTags (his perch at the mine's mouth), TagBoardKept (the shift's voice after a kept payroll)
The Rest's seeds (2026-10-16): HobIdle (Hob's three feathers, his mutters), HobMain6Point (the road at dusk: his arrival on
  Pa's neighbour bier after Main 6), and AgentWindowLily's last two lines (the Junction and Highwater Landing). What Ada
  gave up is Father Aldana's page now (story_texts.py READABLES AldanaPage).
Gone on 2026-10-21 (delete their assets after the gameplay rebuild that stops pointing at them): HobMain2Climb,
  HobMain2, HobMain3, HobMain4Road, HobMain4Bell, HobMain4Aldana, HobMain4, HobMain5Sacs, HobMain5Lantern, HobMain5,
  HobMain6Bridge, HobMain6Post, HobMain6, HobMain7Depot, HobMain7Board, HobMain7, HobSide1, HobCaches, AgentWindowCoal,
  HobKnocking, HobSide3, HobDeadLift, HobLastWatch, HobBackPay, AldanaAda.
Gone on 2026-10-31 with Seven Days (the Wardens story; delete their assets after the gameplay rebuild that stops pointing
  at them: build_area_farm.py's DeliaMain1 topic, build_area_story.py's HobWakes perch): DeliaMain1, HeadboardEllis,
  HobWakes.
"""
import unreal

FOLDER = '/Game/Data/Story'

HOB = 'Hob'
# Ellis speaks in the deal, at Tilly's window and at Aldana's door (the cold open names them so too).
ELLIS = 'Ellis'
# A hay bale's remark, and the twist said as the hands' fight is won, play with no speaker point to name him, so they
# name him (as his point does).
AMOS = 'Amos Whitlock'
# Landfall's voices (the Wardens story, 2026-10-31): the Lamplighter's crew as their speaker points name them
# (AWardenCrewMember), Vane as UVaneBroadcast names him, Delia as ADeliaRansom's point does (for her barks, said with no
# point), and the player character in the new lines: "Sully" (the hero Ellis's display name is Sully Graves; the id stays
# Ellis), which hero_lines.json's speaker.Sully turns into the played hero's name for the other four.
HOLLOWAY = 'Captain Holloway'
FINCH = 'Brother Finch'
JUNIE = 'Junie Calloway'
VANE = 'Cornelius Vane'
DELIA = 'Delia Ransom'
SULLY = 'Sully'
# Lucky Ned on the radio as he leaves the Rest (Main 3's fly-by; he's fought in a later area, the user's call).
NED = 'Lucky Ned'

SETS = {
    # --- Main 1, Landfall (the Wardens story, 2026-10-31; Docs/Work/2026-10-31-Wardens/CONTRACT.md's step table, steps
    # counted from 0). Who plays each set: FLIGHT's ship and scenes (Deck as control starts on deck, VaneHorn as step 0 is
    # done, Boarders as the first wave comes over, BoardersWave2 with the second, Grapples as step 2 starts, Landing in
    # HardLanding, VaneValley then Trail in SmogReveal); Hob's perches' arrivals (story_texts.LANDFALL_PERCHES: Anchor at
    # step 4, Climb at 5, Point at 6, HobLandfallPorch at 8; none at 7, which starts under the reveal); CREW's Delia (DeliaFight, her barks);
    # Delia's topics (story_texts.TOPICS['Speaker_Delia']: the turn-in is DeliaLandfall, or DeliaLandfallHat once Abel's hat
    # was picked up, flag Landfall.AbelsHat). Shared words: no family words (Sully Graves is no Ransom), so no hero rows; the
    # player's own lines name speaker SULLY ("Sully": hero_lines.json speaker.Sully gives the other heroes their name).
    # Short captions, each a reason or a feeling. Vane's lines are all his, said by UVaneBroadcast (radio in, hiss, out).
    'DA_Lines_LandfallDeck': [
        (HOLLOWAY, "That's Ransom's Rest under the smog. Vane's had it a week."),
        (FINCH, "The Lantern's leaning hard, Warden. Come up to the prow and see where."),
        (JUNIE, "This smog's eating my engine. Whatever we're doing, let's do it quick."),
        (HOB, "Morning, sunshine. First job. Try not to fall off."),
        (HOLLOWAY, "Eyes open. Vane doesn't like visitors."),
    ],
    'DA_Lines_LandfallVaneHorn': [
        (VANE, "Wardens! Welcome to Ransom's Rest, a wholly owned subsidiary as of Wednesday."),
        (VANE, "Mind the smog. That's somebody's Aunt Mabel."),
        (VANE, "I've sent a tender for your ship. Do let my men aboard: they're paid by the soul."),
    ],
    'DA_Lines_LandfallBoarders': [
        (HOLLOWAY, "Grapples! They've got us! Warden, clear my deck!"),
        (JUNIE, "Get behind something! Those company rifles shoot straight!"),
        (HOB, "See the one with the tank? Shoot the tank, sunshine. It pops."),
    ],
    # The second wave over the gangplank (FLIGHT plays it when that wave comes; none: nothing is said).
    'DA_Lines_LandfallBoardersWave2': [
        (HOLLOWAY, "More coming over the plank!"),
        (FINCH, "If a hose latches on, break the line before it drinks you dry!"),
    ],
    'DA_Lines_LandfallGrapples': [
        (HOLLOWAY, "Deck's clear. Now cut those grapple lines, or they tow us to the barge!"),
        (JUNIE, "Hold the cutter on the chain till it parts. Keep your head down."),
        (HOB, "Their riflemen are still on the tender. They've noticed you, sunshine."),
    ],
    # In the hard landing (about 5 s): two short lines.
    'DA_Lines_LandfallLanding': [
        (JUNIE, "They holed the bag! We're losing lift!"),
        (HOLLOWAY, "Hold on! I'm putting her down on the ledge!"),
    ],
    # As step 4 starts (Hob's arrival on the Mooring Ledge): why the anchor, and why the dead.
    'DA_Lines_LandfallAnchor': [
        (HOLLOWAY, "She's sliding! Crank that anchor winch, or she goes over the edge!"),
        (FINCH, "And the dead are climbing up out of the canyon. The smog stirs them."),
        (HOB, "Crank and shoot, sunshine. You can do both. Probably."),
    ],
    # As step 5 starts (the anchor set, the dead down): why the climb.
    'DA_Lines_LandfallClimb': [
        (JUNIE, "She's holding. Patching that bag'll take me till dark."),
        (HOLLOWAY, "Warden, get up to Ransom's Point. I want eyes on that valley."),
        (FINCH, "Get up high. From there the Lantern can see what you see."),
    ],
    # As step 6 starts (on the Point): the spyglass.
    'DA_Lines_LandfallPoint': [
        (HOLLOWAY, "You up? There's a spyglass on the lookout. Tell me what Vane's done."),
        (HOB, "Brace yourself, sunshine. It's not pretty."),
    ],
    # Vane over the valley's company loudspeakers, in the reveal (SmogReveal): the barge, the hose, the smog.
    'DA_Lines_LandfallVaneValley': [
        (VANE, "Good evening, Ransom's Rest. Cornelius Vane, your new landlord."),
        (VANE, "Your dead are fuel now. Your trains will run on time. You're welcome."),
        (VANE, "Lucky Ned tells me your keeper is our pilot light. Burns beautifully."),
        (VANE, "And Wardens: lovely ship. I'll have her melted down for buttons."),
    ],
    # Finch, as the reveal ends: the Lantern has Abel, and his trail (step 7's why); Hob last, a nudge toward the optional
    # hat. (Step 7 starts as the spyglass is used, under the reveal, so Hob's flight down to the path's foot is silent and
    # this set, said after the reveal, carries his word.)
    'DA_Lines_LandfallTrail': [
        (FINCH, "The Lantern's got him! Abel Ransom, the keeper. His light's in that barge."),
        (FINCH, "He ran from them first. See his trail? Down the bluff path to the farm."),
        (HOLLOWAY, "Follow it, Warden. Whoever's at the end of it saw what they did."),
        (HOB, "Somebody dropped a hat on that path, sunshine. Keepers don't drop hats."),
    ],
    # Hob's arrival on Delia's porch rail as step 8 starts.
    'DA_Lines_HobLandfallPorch': [
        (HOB, "She's holding that porch alone. Help her, sunshine!"),
    ],
    # Delia's barks while she fights (CREW: ADeliaRansom, queued captions, rate-limited); her name on each, as no speaker
    # point says them.
    'DA_Lines_DeliaFight': [
        (DELIA, "Off my porch!"),
        (DELIA, "Come on, then! I've buried better than you!"),
        (DELIA, "That's for my son!"),
        (DELIA, "Reloading! Cover me, Warden!"),
        (DELIA, "Hose-man coming! Shoot his tank!"),
        (DELIA, "Get off my steps!"),
        (DELIA, "Too old for this, and too mean to die!"),
        (DELIA, "Keep them off the steps, Warden!"),
    ],
    # Her topic while Main 1 runs, before it's ready to turn in (talked to mid-fight).
    'DA_Lines_DeliaLandfallWait': [
        ('', "Talk after! Shoot first!"),
    ],
    # Main 1's turn-in (her topic "during Main1 from step 9", the step past the last; the mission's own turn-in lines are
    # empty so this topic is what she says): what the gang did, the hose, why it matters, and the salt on her plate (the
    # porch plate's jars, free to take from now: story_texts.PORCH_PLATE).
    'DA_Lines_DeliaLandfall': [
        ('', "So the Wardens came. A week late, but you came."),
        ('', "Wednesday last they put a hose to my son at the chapel door. He held it anyway."),
        ('', "His soul tore loose and ran home to me. They reeled it in on my own steps."),
        ('', "Now it burns in that barge. Bring him back to Saint Ada, Warden."),
        ('', "Salt's on the plate. The dead don't care for it."),
    ],
    # The same turn-in once Abel's hat was picked up on the bluff path (flag Landfall.AbelsHat, set by the hat's readable;
    # her topic "during Main1 from step 9 with Landfall.AbelsHat", listed first): her best line.
    'DA_Lines_DeliaLandfallHat': [
        ('', "...That's his hat. Give it here."),
        ('', "Forty years I told that man to tie it on."),
        ('', "Wednesday last they put a hose to my son at the chapel door. He held it anyway."),
        ('', "His soul tore loose and ran home to me. They reeled it in on my own steps."),
        ('', "Bring him back to Saint Ada, Warden. And take the salt off my plate."),
    ],
    # Her default after Landfall (she stands on her porch now, ADeliaRansom; was the screen door's "I can't open this door").
    'DA_Lines_DeliaPorch': [
        ('', "Go on, Warden. I'll keep the porch."),
    ],
    # Abel's grave stays in the family plot (his headboard's words).
    'DA_Lines_HeadboardAbel': [
        ('', "ABEL RANSOM. KEEPER OF SAINT ADA. HE HELD THE DOOR."),
        (HOB, "Frost in August. That's no weather, sunshine."),
    ],
    # Hob talked to while Landfall runs (story_texts.TOPICS['Speaker_Hob']).
    'DA_Lines_HobMain1': [
        ('', "I'm the lookout, sunshine, not the help."),
    ],
    'DA_Lines_HobAfterMain1': [
        ('', "Don't look at me. I only carry the messages."),
    ],

    # --- The new game's intro (INTRO, 2026-10-27, playtest note 3; Docs/Story.md facts, no new canon) ---
    # Story/NewGameIntro.h: line N fills the intro's caption slot N (NewGameIntro::ScriptLines), so the order and the count
    # matter (a set with a line missing or extra is ignored, the code's own words play); the code keeps the same words as
    # its fallback and Looter.Intro.Lines checks they match. Narration, no speaker; no family words (no hero rows).
    # Teropa from orbit (or Westward over the canyon in its place): the Wardens story's words (2026-10-31), ending on the
    # Lamplighter flying into the smog over Ransom's Rest, where a new game opens on her deck.
    'DA_Lines_NewGameIntro': [
        ('', "On Teropa the dead go west, and a saint lights the way."),
        ('', "Then Cornelius Vane started dredging up the dead."),
        ('', "Soul coal, he calls it. The Lantern Wardens call it theft."),
        ('', "Now the Lamplighter flies into his smog over Ransom's Rest."),
        # Ransom's Rest: over the Rim, the chapel, home, the grave (the old intro's part: a new game no longer plays it since
        # 2026-10-31; kept so the set keeps its eleven slots).
        ('', "Every town keeps a saint, whose light shows its dead the way across."),
        ('', "Seven days ago the Dunne Gang robbed Saint Ada, and the Rest went dark."),
        ('', "Seven towns. Seven saints. Not one light left burning."),
        ('', "The dead lost the light on the road, and drifted home."),
        ('', "You rode in with that gang."),
        ('', "Seven outlaws killed you."),
        ('', "Something sent you back to bring them in."),
    ],

    # --- The boss intros (SCENES, 2026-10-27, playtest round 4; Scenes/BossIntro.h, BossIntro::ScriptLines) ---
    # Line N fills the intro's caption slot N, so the order and the count matter (a set with a line missing or extra is
    # ignored, the code's own words play); Looter.Scenes.BossIntro.Lines checks they match. No family words (no hero
    # rows). Abel's own fight line said on camera ("You brought them here, El. Right to our door.") is AbelRules' and
    # isn't here. Teague's mutter sets up his last word ("...Tell the company it was the roof."); Hob's for Abel is
    # Docs/Story.md's own phrase for why the Keeper walks.
    'DA_Lines_BossIntroGravemother': [
        (HOB, "Every spider in the Rest answers to her."),
        (HOB, "Crack that headstone off her back first, sunshine."),
    ],
    'DA_Lines_BossIntroForeman': [
        ('Cyrus Teague', "Dig. Keep digging. It was the roof."),
    ],
    'DA_Lines_BossIntroAbel': [
        (HOB, "A keeper doesn't lie still while his saint is dark."),
    ],

    # --- Main 2, The Ferryman's Price (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 4;
    # build_area_main2.py places who says each). Shared words: no family words, so no hero rows; Sully's lines name SULLY. ---
    # The Three Widows' nest (BluffNest, moved there from the Point's top): said as its last spider falls (its spawner's
    # cleared_lines), the Restless one usually last: the ranks' lesson, and why its loot is better.
    'DA_Lines_HobMain2Blue': [
        (HOB, "See the blue on that one? Fed longer on the smog. Hits harder, and its iron's better."),
    ],
    # Keeper's Bridge, the optional: Hob's word as the player crosses (a mutter point on the bridge, during Main 2).
    'DA_Lines_HobMain2Bridge': [
        (HOB, "Abel walked the dead over this every sundown for thirty years. Tuesday too."),
    ],
    # Hob's arrival on the Keeper's Gate's north rock as the cart's step begins: why the cart, how to stop it, and why the
    # loudspeaker (the optional).
    'DA_Lines_HobMain2Cart': [
        (HOB, "They're pumping the dead off the boards, sunshine. Shut that valve, or burst the tank."),
        (HOB, "And that horn's Vane's mouth. Shoot it and he stops talking."),
    ],
    # Said as the cart is stopped (ACompanyHoseCart's spill_lines), before Vane's complaint: the twist named.
    'DA_Lines_HobMain2Spill': [
        (HOB, "That's what the hose leaves, sunshine. Half a soul and a temper."),
    ],
    # Vane on the Keeper's Gate's loudspeaker as the cart is stopped (its stop_broadcast); a shot horn cuts him at "your bi-".
    'DA_Lines_Main2VaneCart': [
        (VANE, "Wardens. That cart is Company property."),
        (VANE, "I'm adding it to your bill."),
    ],
    # Hob's arrival on the ladder's crossbar as the talk step begins: why down the ladder.
    'DA_Lines_HobMain2Ladder': [
        (HOB, "He's down the ladder, sunshine. The Ferryman doesn't climb for anybody."),
    ],
    # Sexton at his ferry (story_texts.TOPICS['Speaker_Sexton']): talked to before the boards are clear; the deal (step 2's
    # talk, the plan's words, ending on the Log handed up); while the Log waits; after Main 2, his price again.
    'DA_Lines_SextonWaiting': [
        ('', "The boards first, friend. Vane's men are drinking my fares."),
    ],
    'DA_Lines_SextonDeal': [
        ('', "Warden. You've come about the barge. Everyone does, eventually."),
        ('', "She's out of reach. But a barge must eat, and Company men are greedy."),
        ('', "Starve her, and they'll bring her down to where the food is."),
        ('', "Mind: a keeper's soul goes in his own lantern, or it goes out like a match."),
        ('', "Write down every soul you meet. I bill by the head."),
    ],
    'DA_Lines_SextonLedger': [
        ('', "Open it, friend. My pages are in your Log now."),
    ],
    'DA_Lines_SextonAfter': [
        ('', "The barge's manifest, friend. Bring it, and we're square."),
    ],

    # --- Main 3, Under New Management (the Wardens story, 2026-11-01; MAINS_2_7.md section 5; build_area_main3.py) ---
    # Hob's arrival on the town gate's post as Main 3 starts (said before Ned's skiff casts off: it waits for quiet).
    'DA_Lines_HobMain3Road': [
        (HOB, "Farm road, sunshine. Town's past the orchard, and so's the Company."),
    ],
    # Lucky Ned leaving (AGangSkiffFlyBy): Vane and Ned on the radio (UVaneBroadcast: radio_lines), then Hob and Sully once
    # the radio's off the air (after_lines).
    'DA_Lines_Main3Radio': [
        (VANE, "Off you go, Ned. The Deacon wants you down south."),
        (NED, "Keep my keeper burning, Cornelius. Keepers are lucky."),
    ],
    'DA_Lines_Main3NedAfter': [
        (HOB, "That's Lucky Ned, sunshine. He's the one put the hose to Abel."),
        (SULLY, "He's getting away."),
        (HOB, "For now. Lantern never forgets a smell."),
    ],
    # Hob on the gate's post as the checkpoint's step begins (Vane's gate cue waits for him to finish).
    'DA_Lines_HobMain3Gate': [
        (HOB, "Company checkpoint. They won't let a Warden through, sunshine. Make them."),
    ],
    # Vane on the town gate's loudspeaker as the checkpoint fight begins (its cue: during Main 3 from step 1).
    'DA_Lines_Main3VaneGate': [
        (VANE, "Ah, the Warden. Main Street is closed for assessment."),
        (VANE, "Your dead are being counted. Please don't make me count you."),
    ],
    # Vane on Main Street's loudspeaker as the street is won (its cue: during Main 3 with SetPiece.MainStreet): why every
    # shutter slams.
    'DA_Lines_Main3VaneStreet': [
        (VANE, "Residents! Anyone hiding a Warden will be assessed first."),
    ],
    # Hob's arrival on Bright & Daughter's sign once the gate is won.
    'DA_Lines_HobMain3Tilly': [
        (HOB, "Bright & Daughter's down the street, sunshine. Every shutter's shut against the Company."),
    ],
    'DA_Lines_TillyClosed': [
        ('', "Closed. The Company counts every door that opens on this street."),
    ],
    # Her talk, Main 3's turn-in (her topic from its step 3, the step past the last; the mission's turn-in lines are the
    # same words, said after its card): the bail from his fist (Main 6's item), the box of effects (the reward gun).
    'DA_Lines_TillyMain3': [
        ('', "A Warden. Lord. Come close, the whole street's watching."),
        ('', "His lantern wasn't on him. Only its bail, snapped off in his fist."),
        ('', "Couldn't open his hand till morning. Here. And the box: he'd want it used."),
    ],
    # After it: why she opened anyway (seeds Knocking Below).
    'DA_Lines_TillyAfterMain3': [
        ('', "Every coffin in my cellar's salted, Warden. The hose won't have them."),
    ],
    # Delia while Main 3 runs (story_texts.TOPICS['Speaker_Delia']).
    'DA_Lines_DeliaMain3': [
        ('', "Tilly's at Bright & Daughter, end of Main Street. Mind the Company at the gate."),
    ],

    'DA_Lines_HobMain4Yard': [
        (HOB, "No light, no road. Every soul on the Sundown Road drifted home when she went out."),
    ],
    'DA_Lines_HobMain4Reliquary': [
        (HOB, "Nobody answered. It's the saint that's out, sunshine."),
    ],
    'DA_Lines_AldanaBarred': [
        ('', "Stay back from that door. I've no comfort left for the dead."),
    ],
    # The knock at the vestry door (Main 4's third step, its talk): the doc's words, and the bell's reason.
    'DA_Lines_AldanaWaiting': [
        (ELLIS, "Tilly sent me. Pa's lantern."),
        ('', "...Abel's child. Lord forgive me, I'd know that voice through any door."),
        ('', "Ring her bell. If the saint still knows you, she'll answer."),
    ],
    # While the bell and the Reliquary wait (from Main 4's fourth step).
    'DA_Lines_AldanaBell': [
        ('', "Ring her. Then go and look at the Reliquary."),
        ('', "Saint Ada's light burned over it. Our dead steered home by that. Now it's smashed."),
    ],
    # Main 4's turn-in, the doc's words, word for word: the mission says them (its turn-in lines), and his offer of Main 5
    # follows them as one conversation.
    'DA_Lines_AldanaMain4': [
        ('', "She didn't answer. Now you've seen why."),
        (ELLIS, "How do I find them?"),
        ('', "A keeper's lantern can find an ember."),
        ('', "They shot the keeper first in every town, and smashed his lantern."),
        ('', "Your father's fell in the dark, whole."),
        ('', "Spiders hoard anything a saint has touched."),
        ('', "Look in the Sink."),
    ],
    'DA_Lines_AldanaAfterMain4': [
        ('', "The Sink. I'll pray it's whole."),
    ],

    # --- Main 5 ---
    'DA_Lines_HobMain5Way': [
        (HOB, "Whatever dug the hole under me, let's not wake it."),
    ],
    'DA_Lines_HobMain5Out': [
        (HOB, "Dark as a shut eye, but whole. Missing its bail. You've got that, sunshine."),
    ],
    # After Main 5 (its turn-in has the candle that won't take): a word in passing.
    'DA_Lines_AldanaAfterMain5': [
        ('', "I tried the candle again after you'd gone. Old habits."),
    ],

    # --- The mine, between Main 5 and Main 6 (2026-10-27, the user's note 32: Cyrus Teague the story's second boss) ---
    # Delia's door while Dead Lift and Back Pay run (build_area_farm.py's topic "after Main5, before BackPay"): who Teague is
    # to the family, and why he comes before the boards. Shared words: no "Pa", "Ellis", "kid" or "grandmother".
    'DA_Lines_DeliaMine': [
        ('', "Cyrus Teague. Abel went up to the Providence every spring to ask for those eleven men."),
        ('', "Teague ran him off company ground every time. Go on. I'll keep the plate."),
    ],

    # --- Main 6 ---
    'DA_Lines_DeliaMain6': [
        ('', "Take him the lantern. Show him the way, even if he can't go."),
    ],
    'DA_Lines_HobMain6Way': [
        (HOB, "Every keeper of this valley lies under that board. He's the first who won't stay put."),
    ],
    'DA_Lines_HobMain6Fight': [
        (HOB, "He doesn't know you, sunshine. Not yet. And mind the open end: the wind's his."),
    ],
    'DA_Lines_AbelOnBoard': [
        ('', "Sun goes down the same every night, El. I never once got tired of it."),
    ],
    'DA_Lines_AbelAfterMain7': [
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
        ('', "I'll keep setting a plate."),
    ],
    'DA_Lines_HobMain7Lean': [
        (HOB, "That's Purcell. The Lily's moored out that way."),
    ],
    'DA_Lines_TillyMain7': [
        ('', "Father's car is coupled up. The living won't ride with you, so it's the coffins."),
        ('', "Mind the brass. I polished it for him, and he never once complained."),
    ],
    # Main 7 once Knocking Below is done: she knows now what he thought of the brass.
    'DA_Lines_TillyMain7Father': [
        ('', "Father's car is coupled up. The living won't ride with you, so it's the coffins."),
        ('', "Mind the brass. He liked it, as it turns out. He just never said."),
    ],
    'DA_Lines_TillyAfterMain7': [
        ('', "The car's yours whenever the line runs."),
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
        (ELLIS, "Amos Whitlock?"),
        ('', "What's left of him. And you're Abel's young'un. I'm sorry for it."),
        ('', "Died last harvest with the hay half in. Got turned back on the road."),
        ('', "My hands go through a bale like smoke. Load it for me?"),
    ],
    # While the bales wait (Side 2's first step).
    'DA_Lines_AmosBales': [
        ('', "Under the hoist by the big doors."),
    ],
    # The twist (its second step, a talk): who's on the road.
    'DA_Lines_AmosRoad': [
        (AMOS, "That's the Merritt brothers, and Sol Tanner. My hands."),
        (AMOS, "Fever winter took them, and I owed them for the harvest. Never paid. Couldn't."),
        (AMOS, "Reckon they came to collect."),
    ],
    'DA_Lines_AmosHands': [
        ('', "Don't let them near the hay."),
    ],
    'DA_Lines_AmosThanks': [
        ('', "I'll pay them what I owe when I see them on the far side. If there's a far side."),
        ('', "They buried me with a gun I never fired. Better in a walking hand."),
    ],
    'DA_Lines_AmosFence': [
        ('', "Still waiting on her. The hay keeps. So do I."),
    ],
    'DA_Lines_AmosAfterMain6': [
        ('', "Saw a light on Gravewind Point at dusk. A keeper's still keeping."),
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
        (AMOS, "...Who's that out on the road?"),
    ],

    # --- Knocking Below: Tilly, and her father in the cold room ---
    'DA_Lines_TillyKnocking': [
        ('', "I haven't moved it. I haven't moved anything."),
    ],
    'DA_Lines_TillyAfterKnocking': [
        ('', "I hear his tape at night. It's almost a comfort."),
    ],
    # Talked to at his bench (Knocking Below's sixth step, and after it): the doc's words; the tape lies on the bench for
    # the next step. (build_area_missionprops_valley.py gives him his words before it, "ThaddeusWaiting".)
    'DA_Lines_ThaddeusTalk': [
        ('', "Ransom's child. I measured you, you know. You've stood up since."),
        ('', "They got past me when the door opened. A cellar wants keeping."),
        ('', "Give her my tape. Tell her the brass was too much. And that I liked it."),
    ],
    'DA_Lines_ThaddeusAfter': [
        ('', "Five foot eleven, you were, and narrow at the shoulder. Pine. No brass."),
        ('', "My bottle's under the flat stone in the springhouse. Don't tell her."),
    ],
    'DA_Lines_ThaddeusAfterMain7': [
        ('', "My car's carrying you, I hear. Mind the brass."),
    ],


    # --- Dead Lift: Hiram Coyle at his stool, his voice up the speaking tube ---
    'DA_Lines_HiramWaiting': [
        ('', "Pin's on Abe, wherever Abe's got to."),
    ],
    'DA_Lines_HiramFit': [
        ('', "Coal in the firebox. Pin in the bottom sheave."),
    ],
    'DA_Lines_HiramSteam': [
        ('', "She's taking it! Keep them off her till the needle's up!"),
    ],
    'DA_Lines_HiramRide': [
        ('', "Hold the rail. Don't look down."),
    ],
    'DA_Lines_HiramAfter': [
        ('', "She runs. Pull the bell either end and I'll bring her round."),
    ],
    # Muttered from the winch house as the lift crew rise on the quarry floor.
    'DA_Lines_HiramCrew': [
        ('', "Ah. The boys heard you ask after the pin."),
    ],
    'DA_Lines_LiftTubeAfter': [
        ('', "Coyle here. Pull the bell."),
    ],
    # The Haskett house on Main Street, once the lift runs.
    'DA_Lines_HaskettLift': [
        ('', "...That's the lift. Abe always said he'd hear her run."),
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
