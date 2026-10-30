# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed; the 2026-10-27 playtest rounds 1-5 are there).

## 2026-10-29 day: UI round 3 BUILT (contract Docs/Archive/Work/2026-10-29-UI3Build/CONTRACT.md)
The user picked from six concepts: pause/settings = mix of 1 Ledger & Brass + 3 Halo Reticle, main menu = 3 with a new logo
(two logo rounds; picked 6 Spur Rowel), level-up = 5 Signal Glass. All built, photographed with the new `Tools\uishots.ps1`
and compared with the concepts; notes Pipeline/Game.md "UI round 3". The user's two asks mid-job: the rifle's first-person
hold 3 cm nearer (WeaponHipHold 50 -> 47 cm ahead), the E / HOLD E caps centred (and hollow again: drawn filled since the
tag card). Tests Looter.UI 64, Settings 11, FPHands 6, Player.Hands 13, Inventory 10, Menu 3 pass.
- Gun card after (the user's asks): the maker badge bigger (15 px words), no "Maker · Kind" line under the name, the
  element as the stat list's last row ("Normal" without one; was a gem by the badge), no Zoom row. Tests UI.GunCard 3, WorldCard 2, Inventory 10 pass.
  Then Borderlands' element look: element rows in its colour ("Burn dmg/sec", "Chance to burn"; salt, smite alike), its
  bullet line coloured and named, its glyph and the gun's kind (rarity colour) by the maker badge. Tests UI.GunCard 3, WorldCard 2 pass.
  Then: no maker trait line (only special traits on the card), a gap before the element rows, the maker at the top over
  the name with no box. Tests UI.GunCard 3, WorldCard 2, Inventory 10 pass.
  Then element glyphs picked from four concepts each: Ember Hearth, Soul-light Sunburst, Grave-salt Salt Cluster, Normal
  Empty Ring (Docs/Archive/Work/2026-10-29-Embers). Tests UI.GunCard 3 pass.
- Elements renamed for the player (the user's picks from options): Cinder (Ember), Hallow (Soul-light), Wardbreak (Grave-salt);
  code names unchanged (Pipeline Gunplay.md "Names").
- Two new elements BUILT (the user's picks: Thunder = a jolt that staggers and arcs to the living, Rot = a growing rot that
  spreads from the dying; Pipeline Gunplay.md "Thunder and Rot"): rules, statuses, damage types, cvars, sounds, CSV parts,
  card rows and glyphs (Bolt, Skull). Wardbreak recoloured teal with a Cracked Ward glyph, Thunder deep blue (the user's
  picks). Looks (FX, finishes) by lane F: see the commit. Not yet: the ward nova's element list (ShieldRules::NovaElementFor)
  still rolls only the first three; element finishes reuse frosted (Thunder) and scorched (Rot) patterns.
- Card: a plain gun's row reads just "Normal". Rifle hip hold 43 cm ahead (the user: 4 cm back). The AR's left hand: the
  user keeps it cupped under the trigger housing (fit the fingers onto the loop, lift the gun so it shows), and asked
  whether a different hands approach would stop per-gun tuning (answered with options; see the next entry when built).
- Hands rework IN PROGRESS (the user's pick: grip spots on the parts): contract Docs/Work/2026-10-29-GripSpots/CONTRACT.md
  (marker sockets SOCKET_<Hold|Rack|Reload>_<L|R>_<Pose>, a pose library, the game reaching the arms to them, a Blender
  preview sheet; the old tables stay as the fallback until every part is marked). Lanes A (Blender), B (poses), C (C++)
  started 2026-10-29 evening; then Bullpup markers (AR cup under the trigger housing, visible), then the other guns.
- Main's calls (undo any): from the main menu the reticle row says BACK and the footer Back chip is hidden (the row was an
  empty band); the RESUME wash runs to the right bracket; slider end labels 5 px lower; arrowheads point out (c5's picture).
- Not seen by the user in game yet. Next: the user's look, then the rest of round 2's list (HUD mission popup, bestiary
  rows, vendor, bench, boards, Lost Loot) in this language; playtest Friday 2026-10-30 (don't package yet).

## 2026-10-29 morning (the user awake briefly; contract Docs/Work/2026-10-29-Morning/CONTRACT.md)
The user's answers: per-kind crosshairs yes, colour picked in Settings; the Sonniss GDC 2026 bundle yes (downloaded, 14 GB
in Saved/SoundPacks/Sonniss/2026: no gunshots in it; the user then said no to 2018 / 2021-23, which have guns); next = the
open issues first, then UI round 2. Built and pushed (bba515e8 + the gold fix): crosshair per kind (seen in PIE: SMG,
shotgun ring, sniper hairs; colour row in Settings > Interface), LMG climb cap + aimed sway fix, find crows low with one
perched, under-water finds glow, capsule hits take the nearest bone's zone, aim-in/out cloth + sniper bolt latch + SMG/LMG
mech clacks from the 2026 bundle. Not checked by hand: the LMG burst, the crows from the farm yard, the tarn glow, by-ear sounds.
- Next: UI round 2 (Slate & Salt to the main and pause menus, HUD popups, bestiary rows, then vendor, bench, boards, Lost
  Loot); the user playtests Friday 2026-10-30 (don't package yet).

## 2026-10-29 night: core gameplay job BUILT (the user picked all four: guns feel different, enemy hit reactions,
smarter bigger fights, things to find off the path; asleep, answers in Docs/Work/2026-10-28-Core/CONTRACT.md)
All pushed (122556fd round 1, 58e7faa2 round 2, 597d0d6c fixes). Notes: Pipeline/Gunplay.md "Gun identity and hit feel",
Enemies.md "Fight: pack brains", Levels.md "Finds". Play-check: Docs/Work/2026-10-28-Core/RESULTS.md. Nobody has played it yet.
- Guns: per-kind kick, view shakes, heft and move speed (+-5%), reload beats, brass, punch and aim-in sounds (script-made).
  `Looter.Gunfeel.Identity 0` = the old feel.
- Hits: zones, hit weight, side, poise staggers, overkill; spider leg buckle/limp (looks only), Unpaid whiplash, arm flings,
  hats knocked off; slime air knock and pops; kill styles; ten hit sounds. Staggers need a creature in a fight (an idle one
  only flinches; the demo's "never staggers" was that). `Looter.Hits.Test Body 30` on a chasing Unpaid logs "heavy (..%)".
- Fights: fight director (melee gaps by kind, player read, close-call ease, camping punished), pack leaders and leader-down
  falter, slime split and dodge, Unpaid leader-only rally, fade-back, dais hold; Saint Ada's Yard arena with a fog gate,
  Barn Doors, leaders in most fights. Sim: 6 Unpaid kill a still player in ~10 s; strafing ends at 63% lowest health.
  `Looter.Fight.Sim` / `Looter.Fight.Log`.
- Finds: 15 off the path with crows and glints, FOUND banner, map counts (total and per region), Pike's cache, the tarn
  lockbox, the bolt-hole, the Ranger Post, the miser's grave, the Tollman; named guns Still Water, Bench Mark, Ferryman's Due.
- Perf after (tutorial island default, Medium): 6.1 ms p95 6.5 (before 6.2 / 7.6); 8 Unpaid horde 7.0 p95 8.1 (before 7.8 / 11.4).
- Decided unattended (undo any): CONTRACT.md "Main's calls overnight" (heavy-gun kick caps kept; spiders' fight balance
  untouched, the user's 10-28 "leave them"; arena at the chapel yard only; Borderlands-normal targets; crows+glint tells; the
  Tollman a find); Regular Unpaid silver chance 55% -> 40% (Progression.md 10-21 said 40%); FOUND banner moved to 78% down
  (the card over loot covered it); the SMG has no punch sound.
- Morning asks answered (see 2026-10-29 morning above).
- Open (not built or not checked): launcher not tried; Hob's caption at the town-gate leader's death; Sink/Webwood spider additions; news when a find is found;
  E6 grenade tins; Pike's footlocker body (needs a C++ flag); art: laid rope, card+knife, bigger cache tag; FightPace
  uptimes need fitting to play. Files over 500 lines to split: CreatureBase.h 694, GunfeelSoundTests 636, WeaponBase.h 616,
  WeaponBrass.cpp ~565, GunfeelSoundTuning.cpp 511, SpiderCreatureHurt.cpp 508, UnpaidCreature.h 506.

## 2026-10-28 evening: UI character, round 1 (the user at work; playtest Friday 2026-10-30; "don't package yet")
Committed and pushed. Contract and mockups: Docs/Work/2026-10-28-UICharacter/ (kept open for round 2). Notes: Pipeline/Game.md
"The gun card". The user picked the look "Wide Sweep" (mockup/wide.png) after four rounds; saw it in game only as screenshots.
- Round 2 built (later the same evening, after ~10 mockup rounds the user steered): the tag card (Borderlands-like list,
  Slate & Salt, level gem, keys under it, like-with-like compare). The user's pick: Slate & Salt for the whole UI (next rounds).
- Built: the gun card kit (UI/Cards), the card over a looked-at gun with hollow keys, the in-hand card on the HUD's right, the
  four makers' card looks and open sounds (UI.Card.*), the inventory back to two columns (backpack right), card on screen clamp.
- Not heard by ear: the card sounds. Bars use the all-kinds rating, so shotgun fire rate/magazine bars sit low (maybe per-kind later).
- Next (round 2, the user's list): the same look and motion on the main and pause menus, the HUD popups (level-up, mission,
  pickups), the bestiary rows; then the other screens (vendor, bench, boards, Lost Loot) still in the old boxy panels.
- The Gilded Lily: design only, H2 The Spine picked; not to build until the user says.

## 2026-10-28 afternoon (the user at work; their picks: Unpaid balance, maker sounds, packaged fixes, cleanups)
All pushed (last: 683405f9 + this handoff). Nothing played by the user yet. Contract: Docs/Work/2026-10-28-Polish2/CONTRACT.md.
- Unpaid (play-checked by DEMO each round, numbers in Pipeline/Enemies.md):
  - LungeHitRadius 85 -> 100 (at 85 no lunge ever landed on a still player); FlankChance 0.6 -> 0.4.
  - Sight: no turn or lunge through walls; a blind Unpaid lets its turn go and steps to a spot that sees the player
    (reaches the chapel dais now); one blink per player per 1.2 s across the pack, step clock jittered 0-1.5 s;
    no attack 0.6 s after a blink lands (ArrivalHold). Standing still on the nave floor: dead in 49 s (was 12).
  - Open: the dais still hardest (landings at 1.5 m; a still player dies in 23 s). Idea: 0.8 s hold for 1.5 m landings.
    UnpaidCreature.h is 544 lines (over ~500): split when next touched.
- Gun maker sounds: MakerSoundFlavor (Roundhouse heavy, Saint Maud ring, Windfall rattle, Meridian clean); heard in PIE
  via au.Debug only, not by ear. Watch for clipping on a Roundhouse/Saint Maud revolver indoors.
- Paper marker: decal/sheet mission items (the gate poster) get an orange ring-diamond while lit (PASS in PIE).
- Maker marks were mirrored on both sides: fixed (MARK_CODE); T_GunMakerMarks never streams. Third person can't show a mark
  (camera sees the gun end-on; accepted since 10-22).
- Shotgun trigger: Grip.Report 2.23 cm (test now 3 cm, indexed). The shotgun's support hand reads a bit off (cup box sits on
  the housing front, fingers not wrapping): the user to look.
- Packaged game: r.PSOPrecache.GlobalShaders=0 (22 compute PSO errors gone on the old build with -dpcvars); not yet in a
  packaged build.
- Part swapping waits for Ozias after the Lily (the user): Ransom Farm's and the tutorial island's benches removed.
- Plan.md refreshed (phases 9-11 built); the user: stay on Ransom's Rest, no tutorial-island work.
- The Gilded Lily DESIGN only (the user: "I don't want to make the gilded lily yet"): Docs/Areas/GildedLily.md. A
  Titanic-like liner anchored at sea (not an airship), bigger than RR, town first and ship last, Ned's rigged card game,
  slots, Ozias, hostile boarding. Concept maps A-H2 in Art/Concepts/Areas/GildedLily/; **the user picked H2 The Spine**
  (Hulltown in Crescent Bay: a beached wreck, the Sovereign, across the bay). Still not to build until the user says.
- Element answer given to the user: Ember, Soul-light, Grave-salt; any maker; Saint Maud element parts x3.

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
- **Unpaid gallery: the user said "Dig in" after two failed play-checks. Found and fixed (Pipeline/Enemies.md): every
  phase-step and flank had been refused game-wide by the scatter's PCG volume answering the landing spot's sight trace.**
  Main's PIE check after: past the west fence the yard's pack blinked over (19 steps in ~10 s). The Unpaid are now much more
  aggressive than any playtest saw (blinks and flanks work for the first time): the user should feel it out. The press path
  itself (a hit from past the edge) is covered by `.Kit.Press`; no shot of Main's landed in that PIE check.
- Shotgun trigger moved back at the user's go-ahead (Pipeline/Game.md); body views redrawn; not seen in play.
- History of the two failures:
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

## 2026-10-29 night: elements matter BUILT, AR grip spots IN
- Elements matter (Pipeline Gunplay.md "Elements matter", Enemies.md "The Stone Sentinel"): all seven lanes built, tests
  pass (Elements 15, Creatures Touched/Sentinel/StatusPose/TierLook, UI StatusIcons/Tier/HitMarker/DamageNumbers, Shield 11,
  Loot.TouchedLean, Vfx.Element 3, BulletMarks 4, Weapons.ElementLook 2, Bestiary 5). Imported: sounds (38), the Sentinel
  stand-in, the 8x8 mark atlas + M_BulletMark; 4 Sentinels placed. Not yet looked at in play: reaction poses, trails, marks,
  typed ward colours. Files over 500 lines to split later: CreatureBase.h, CreatureDevCommands.cpp, WeaponFX.h,
  FirstPersonHandsComponent.cpp, FirstPersonHandsReload.cpp, fp_hands_parts_sheet.py.
- AR grip spots: every Bullpup body/magazine marked (Art/Models/Weapons/Bullpup.py spot tables); left hand cupped at the
  bottom of the trigger housing (the user's pick), hold 43, 23, -27.5 (the user: back 4, down 4); in PIE palm gap 0.15,
  all fingers touching. The other guns get spots with their new parts.
- Tripo remakes in progress by the user: Stone Sentinel and the Unpaid (prompts in this session; Enemies.md).

## Queue (the user's picks, 2026-10-29 night)
1. The AR's grip spots (in progress), 2. Make elements matter (Docs/Archive/Work/2026-10-29-NewElements/ELEMENT_DESIGN.md:
typed wards, bodies, immunities, Touched variants, Stone Sentinel, x2 / x0.33; plus element feedback: firing looks,
status reactions, status icons + callouts, hit marks + bigger deaths, all four the user's picks), 3. The other six guns' grip spots: placed as each gun's NEW parts are fitted (the user is modelling new parts; marking
today's parts would be done twice), the old tables stay as their fallback until then, 4. Loot and pacing pass (play 1-7, drops, XP, money, difficulty, unique legendaries).
Offered and not picked: real models/moves for the placeholder enemies; a performance pass (70-88 fps in the editor).

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
