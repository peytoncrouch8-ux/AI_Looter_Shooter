# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed; the 2026-10-27 playtest rounds 1-5 are there).

## 2026-10-28 day (the user at work, no playtest from them)
The user's picks: "Fix the unpaid not hitting but leave the spiders where they are. Fix Sniper framing" (a193ab75), then all four
of gallery fix, gun looks, melee hand bugs, small cleanups (76c191db). All pushed; nothing seen in play by the user yet.
- Unpaid lunge commits its last 0.3 s through the hit slow; the Unpaid take that slow at most once per 0.5 s. Spiders untouched.
- Sniper hip: HipUp -1.5, ArmsScale 0.9 (like the LMG).
- Unpaid gallery: shot from within 8 m past their ground's edge, the pack fights there and the one hit blinks in (Pipeline/Enemies.md).
- No rocket ammo unless a launcher is carried or in the drop.
- Gun looks: camo in two close tones of the gun's own paint; pale enamel (Saint Maud) darker with dark chipped edges (M_Gun 822
  instructions, was 800); AR Reflex dot a 4 mm disc + 1.1 cm ring (the Tripo set 3 sight is the one in the game).
- Melee hands: the overnight Crane/Odessa grabs predate f2423b2d, which fixed both; new tests only.
- Split for ~500 lines: UnpaidCreatureLook.cpp, UnpaidCreatureArmGuard.cpp, UnpaidBodyParts.h.
- Open, the user's call: the shotgun trigger finger can't reach (geometry: the Ranchhand's trigger is ~13.4 cm ahead of
  SOCKET_Grip, ~20 hand-cm drawn vs ~10.6 reach). Fix = Ranchhand.py `trigger_group` ~5 cm back, best before the shotgun's
  Tripo modelling (its views show today's trigger). Side note: grip tests check guns at 1.1, not the drawn 1.32.
- Not built: spider balance (the user: leave them), the old builds still unplayed.
- Play-checks (Demo agent, PIE, God, aimbot): sniper hip PASS (Saved/Screenshots/Tools/sniper_hip2.png), Saint Maud and camo
  PASS (maud_shotgun2.png, camo1-3.png), Reflex dot PASS (ads_reflex.png). Unpaid hits landing: ~25% of attacks started
  (overnight 3.4%), rough.
- **Unpaid gallery: FAILED twice in play; the user was asked how to go on (rule 6).**
  - Run 1: the press never blinked anyone in. Cause: a fight begun in the yard pressed the whole pack, so a later shot from
    past the fence owed nothing, and an Unpaid waiting its turn may never step. Fixed in the last commit (hit from past its own
    edge always owes the blink; `WaitGoesNowhere`, `Looter.Unpaid.BlindStep`).
  - Run 2: still 0 phase-steps. "is pressed past its edge" fired 202 times (every hit: log spam, Verbose only), the pack
    lined up 0.5-1.8 m inside the west fence 7-8 m from the player and never stepped; 2-4 walked out through gaps. Likely
    `FindPhaseSpot` failing silently (retries every 0.75 s, no log): suspects the landing spot's sight trace, the Obstacle-tag
    rule, or Search state. Next: a log line on its failure.
  - The chapel nave is INSIDE the yard's ground (fence polygon x 5600-7800, y -3600..-200 encloses it). Standing in the nave,
    Unpaid walked in through the doors in ones and twos, the rest along the outside walls 3.4-5.5 m off (too near to step:
    MinStep 3 + NearestToTarget 2.5). No line at the back door this time.

## State at the session swap (2026-10-28 morning)
All committed and pushed (last: the handoff commit). Test build `Builds\Playtest\2026-10-28_0250` (1.45 GB, smoke PASS;
`Play Latest Build.bat`) has everything below. **The user hasn't played it yet: ask first.** Older builds (10-27_1006,
_1711, _1818, 10-23_0658) were never reported played either.

Done 2026-10-27 evening to 10-28 overnight (details in git; the user's orders and calls kept here):
- First person (the user's playtest asks):
  - GunScale 1.2 in first person only (Looter.FPHands.GunScale, per kind .<Kind>.GunScale).
  - Guns pushed forward; forearms steeper (72/78 degrees, ForearmBack 0.5).
  - The support hand cups palm-up under each gun's middle (FirstPersonHandsCup.cpp, the user's rule: "cup beneath the trigger housing... on the magazine of the LMG... keep this in mind for all weapons").
  - Cup fitted at runtime to whatever parts the gun has (FirstPersonHandsCupFit.cpp: rays on the parts' collision, Looter.Hands.CupFit.*).
  - LMG: SupportAhead 3, HipAhead -2, HipUp -3, ArmsScale 0.9 (new per-kind Looter.FPHands.<Kind>.ArmsScale). The user tuned it by eye; the LMG's support hand now sits below the frame, which is accepted.
- Tripo (the user models on a PAID plan from Main's views; Main checks, fixes and fits; method in Docs/Pipeline/Assets.md "Tripo parts"; memory tripo-gun-parts):
  - AR: all 4 maker sets in the game (Art/Models/Weapons/Tripo/parts_set1-4.py merged by Bullpup.py; LOOTER_BULLPUP_TRIPO=0 scripted).
  - Melee tools: all 5 in the game (Art/Models/Weapons/Melee/parts_tripo.py; LOOTER_MELEE_TRIPO=0).
  - Chest tiers: 5 in the game (Art/Models/Loot/ChestTiers.py).
  - Shotgun views READY for the user to model: Art/Concepts/Weapons/Shotgun/<Slot>_<Key>/upload (24 parts, 4 sets: Saint Maud Standard/Field/Crown/Tube6/Flip/Skeleton, Roundhouse Armored/Shielded/Brake/Drum20/Prism/Mule, Windfall Salvaged/Short/Duckbill/Box8/GhostRing/Folding, Meridian Tactical/Slug/Choke/Tube6Window/Variable/Thumbhole; the pump has no views).
  - The user's rule: each set is a genuinely new design, never a recolour.
  - Heroes: Tripo DROPPED by the user (clipping, z-fighting, unfilled parts): "expand on what you already have and get it perfect". Views stay in Art/Concepts/Heroes (Tools/Blender/hero_views.py).
  - The Tripo API trial was dropped (no API page on the user's account); the user models by hand.
- Gun pattern variety (the user's call: rarer = fancier, subtle and worn-in):
  - WeaponModelPattern.cpp rolls from the seed, rarity and maker into custom data 29-35.
  - M_Gun's pattern layer: 1 grime, 2 worn edges, 3 stripes, 4 chevrons, 5 camo, 6 stencil, 7 clean bold.
  - Element stains on every part (Looter.Weapons.ElementLook.Spread 0.25). M_Gun is 800 instructions, up from 497.
  - Force a pattern with Looter.Weapons.Pattern.Force.
- Loot chest tiers 1-5 (the user: "the better the chest the better the loot"; "steady climb"):
  - T1 ammo + 35% one gun; T2 one gun; T3 1-2 guns uncommon+; T4 two rare+; T5 three epic+ past the early cap (~70% legendary).
  - The "escalating show" (ChestShow.cpp): T1-2 latches pop; T3 the lock spins green; T4 orb pulse and gold trim sweep; T5 crest unfold, burst and rising tone, then a loot fountain thrown away from the opener. Eight Loot.Chest.* cues.
  - Ransom's Rest's 13 chests tiered by Tools/Unreal/chest_tiers.py + set_chest_tiers.py. Ruth's bluff cache is T2 (the swap lesson).
  - The Supply Crate and Strongbox models are replaced by the tiers; Coffin, Grave, Mailbox and Footlocker are unchanged.
- Heroes "perfect", the user's ask:
  - Fit pass on all five (Tools/Blender/fit_audit.py, looter_fit.py).
  - Cloth springs kept off the limbs (BodySwing: back boot, hems, hands).
  - TurnCarry: cloth turns with the body.
  - The hero select orbits its camera and lights round a still hero, a turntable (the user: "cloth acts unnaturally" when panned).
- Melee:
  - Crane's hand wraps the gaff (bRoundHandle).
  - Odessa's fan winds up 45 cm from the eye.
- Other: sniper scope glass (GunScopeLens). Tools/zenwait.ps1: launch.ps1 presses Yes on the editor's Zen "keep waiting" box (the user saw it).
- Perf: RR Medium 7.6 ms (132 fps), p95 7.9 (target 8.3).
- Overnight playtest: Docs/Work/2026-10-28-Overnight/PLAYTEST.md.

Open (ask the user first; tap choices):
1. Feel calls from the overnight playtest: b-f BUILT 2026-10-28 (above); a) spiders left as they are by the user's call.
   - a) Spider packs kill in 3-4 s while the Unpaid land almost nothing. Suggested: spider melee tokens 1 or bite cadence 1.6 s while 2+ bite; the Unpaid lunge commits its last 0.3 s, hit-slow at most every 0.5 s.
   - b) The sniper at the hip hides a third of the screen (per-kind framing like the LMG).
   - c) The Unpaid wait at their ground's edge, a shooting gallery (phase-step in, or cinders).
   - d) Saint Maud's clean finish reads flat, and camo on red/cream reads as cow hide.
   - e) The AR reflex dot is tiny (2x, a ring, or cyan).
   - f) No rocket ammo while no launcher is carried.
2. The user models the shotgun's 24 parts in Tripo; then fit them like the AR (one agent per set, the cup fit covers the hands).
3. Still open from the 10-27 notes:
   - Note 15 (stairs invisible), note 10 (terrain), note 30 (floating fences): the user's screenshots are needed, asked 10-27 with no answer.
   - The Unreal Engine logo download: the user asked Main to do it. The built-in browser's policy check failed 3x. Retry, naming the file, source and size first.
   - Not yet checked by eye in play: the Niagara Cinder fire, the boss attack rules.
4. Older, small (the 10-23 list, archived): the sniper ground ring (ask: drop it), Tilly's silhouette, gunfeel cvar tuning by the user, decal mission items can't wear the highlight, bounty variety after Main 5, ~66 D3D12 PSO errors in the packaged game, the shotgun trigger finger, third person never shows a maker mark.

How the work ran 10-27/28 (keep doing it):
- Main orchestrates up to 8 agents on disjoint files.
- Blender agents run one job each (16 GB RAM): more RAM was suggested to the user as the biggest speed-up.
- One editor lane: agents SendMessage Main before a build or long PIE use, and Main hands the editor over.
- Main imports (Looter.ImportModels <ABSOLUTE path>), runs only the touched tests, commits and pushes after each step.
Lessons:
- A "Save Content" dialog blocks close.ps1. Read it with Tools\winshot.ps1, then answer with Tools\dialogclick.ps1 (Don't Save at 493 502). Reload the level clean first: Tools/Unreal/open_clean.py.
- A build compiles every agent's in-progress files: warn them first.
- git "Permission denied" on .git/objects passes: retry after a few seconds.
- Check what exists before an art job: render existing models, don't redraw them (the user caught a redraw; memory reuse-existing-assets-first).
- An editor test run stops another agent's PIE: tell it.

The user's calls this stretch:
- Reports in 3 sentences.
- "Run multiple agents as needed."
- "Don't worry about checking or taking screenshots" (for small framing tweaks the user watches themself).
- Fast mode isn't worth it here.
- The user may be on Remote Control: put questions in the closing message too.

## Direction (the user's answers, grill session 2026-10-21) - overrides "next" below

- **Core-loop freeze:** no new regions, heroes or systems (the *Lily* stays on hold) until shooting, fights, loot and
  pacing are fun in Ransom's Rest. The user played it through after the heroes: "something's off" in gunplay, pacing,
  enemies, loot and art.
  - Gunplay: enemy hit reactions, gun sound and kick, guns feel alike (not hit confirmation).
  - Enemies: too easy, dumb AI, fights too small (variety not picked as the problem).
  - Loot: all four wanted: more gun kinds, part effects that change behaviour, elements, bigger drop moments.
  - Art: characters, samey world, weak VFX (not lighting).
  - Pacing: story slows it down, nothing to find off the path, fetch-quest missions (walking not picked).
  - Story delivery: cut dialogue roughly in half; most lines as short captions while walking or fighting
    (Borderlands-radio style); long scenes only at big beats. Still no voice acting.
- **Freeze ends when** the user and their one playtester friend each play Ransom's Rest start to finish on a packaged
  build and both want to replay it with another hero.
- **Sounds:** paid/free royalty-free libraries licensed for games are now allowed for guns and impacts (e.g. Sonniss
  GDC bundle), processed through the script pipeline; script-made stays the rule elsewhere.
- **Release plan:** single-player at launch (co-op not planned). 1.0 keeps 15-18 regions. A free Steam demo once the
  game is 60-70% built, within a year (~Oct 2027). Store page ("Coming Soon") goes up once the core is fun, with a
  Ransom's Rest trailer. Steam's AI-content survey answered plainly (made with AI tools, nothing generated live).
- Playtesters: the user and one friend.

## Suggestions for the user (Main's; none built without a go-ahead)
- A decal-friendly objective highlight or a small world marker for flat mission items (posters, papers).
- More bounty variety before Main 5: open the Webwood arena after Main 4 and add a spider/Spitter arena.
- A ward-hit tint in the HUD portrait was built; next could be a short "shield down" voice bark per hero.
- Gun maker sound flavour (Roundhouse heavy, Saint Maud ringing, Windfall rattly, Meridian clean) as Mech pitch/rattle.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
