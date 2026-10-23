# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## User's answers (2026-10-22 late)
Muzzle flash: "Rework it" (MUZZLE agent, Opus, Saved/MuzzleCode: layered burst, star feel, never over the target).
Spider read at range: "Darker coat" (darker cooler brown, pale bands; art agent). Next push after this round: **gun feel
pass** (sounds, recoil, impacts, reloads per gun kind vs top shooters). Play-tests: a short in-game check by an agent
after each feature (memory playtest-after-features). Missions flow committed a4f489f; DEMO walking its checklist; two
older test failures to look at: Looter.Missions.Highlight.Figure (Sexton's "figure look"), Looter.Story.UnfinishedBusiness.
Placed (Whitlock Fields fences).

## Missions flow (the user, 2026-10-22 night)
Verbatim: "The main missions shouldn't have to be accepted and should automatically be added to the missions list. The
side missions are all automatically placed in the world and should only be available after progression is made. Lastly;
I shouldn't have to listen to dialogue before being able to accept a quest. Once I accept a quest it should move or skip
to the dialogue that plays once you accept, borderlands style." Main's calls: main missions auto-start when available
(previous turned in / new game), talk as captions on the move, tracked at once, old saves start them on load; every side
mission gated by a main step (no "?" or offer before it); offer cards take Accept at once, Accept cuts to the accept
lines. MISSIONS agent (Opus) writing Saved/MissionsCode + Docs/Work/2026-10-22-Missions/MISSIONS.md. Also running: spider
fangs floating (since the slim chelicerae, a7bc507) + "skin blurry, lacks texture" (art agent, Spider.py).

## Spiders (the user, 2026-10-22 late: "Soften the spiders" + "they look almost see through... not correctly stylized")
Done and pushed: Brown Spider 300 -> 230 health, bite 12 -> 11 (92d89ed; Gravemother keeps 300/12, spiderlings 60). Look
(b956124): the "see-through" was the hit flash (M_FX_HitFlash overlay, a whole-body white wash re-fired by every bullet);
now a rim (Even 0.08, Rim 1.6, RimPower 3; repeats within 0.15 s at 30%); Spider.py redone (jointed tapered legs, painted
coat). Play-check (level 2, Common rifle + 40 ward, 1.8): 6 chasing spiders survived, 11 s, lowest 121/148; 8 chasing:
lowest 75 (51%), 2 breaks; before, 6 killed in 9.5 s. Maybe now on the easy side: the user's playtest decides (knobs:
spider health/bite in SpiderCreature.cpp, Looter.Fight.MeleeTokens). Notes: at 15 m in daylight the brown coat blends into
the ochre ground (legs read; the Sink's ash reads well); hip-fire muzzle flash's star spikes read as flat yellow diamonds
over close targets (flash_spider_burst.png) — the star flash was the user's pick, so ask before changing it.

## Polish round (the user, 2026-10-22 evening: "Keep polishing")
Contract `Docs/Work/2026-10-22-Polish/CONTRACT.md`.
DEMO verdict (level 2, Common rifle, 40 ward): **keep DamageScale 1.8** (2.0 too harsh: 10 Unpaid left 45%). 4 spiders: lowest
ward+health 69%; 7 Unpaid: 79%; a spider bite (22-30) breaks a Common ward on the second bite. 6 spiders kill at any
scale (10.8 s at 1.5 no ward, 9.5 s at 1.8): 324 HP spiders, 3 biting at once (tokens) — a balance question for the
user, not the scale. Marks: not mirrored; Thresher (24/-0.4/4.0) and Powderkeg (size 3.4) fixed in MARKS (DA_LMG,
DA_Launcher changed). Bounty Strongbox opens by hand.
Committed: ed9c923 (SHIELD-POLISH, BOUNTY-ART, PACKAGE-NOISE), d158cfb (BUGFIX: paid line, "2 guns or wards",
SM_NoticeBoard_Bounty, loot labels step apart `ULootLabelSubsystem`, salvage bodies take maker shell colour, Longhorn/
Powderkeg marks forward), 8360ab7 (ward edges at 0.8 strength: at the red's 0.39 the cyan was invisible on the bright
world; Powderkeg mark 3.0 cm). DEMO re-check passed: sheets, paid line, BOUNTY PAID banner, labels apart, FP marks,
salvage paint.
Open (small): the bounty board model (SM_NoticeBoard_Bounty seen in PIE 2026-10-22,
the class default is SM_NoticeBoard_Bounty after the restart, checked 2026-10-22; the placed board follows it unless the level overrides it); third-person view never shows a gun's side, so marks are first
person only (framing, accepted); turn-in "+XP" popup moved above the crosshair (MissionUp -28 -> 32; it sat on the Interact prompt); board title 34 -> 27 (its line fell onto the rule; not seen since); 6 spiders kill a
level-2 player at any DamageScale (324 HP, 3 biting at once): a balance question for the user; PSO compute errors in
the packaged game on the RX 580 (`r.PSOPrecache.GlobalShaders=0` untried).

## Next builds (the user, 2026-10-22 noon: "Keep going!" and picked all four suggestions)
Player shield, gun manufacturers (a concept sheet first for the user's pick), a bounty board, playtest builds; plus
per-place ground detail and small fixes. Contract `Docs/Work/2026-10-22-Next/CONTRACT.md`. Lane: PERF first (quiet),
then merges as jobs report; PLAYTEST packages last.
Perf (quiet noon): tour worst 8.1 ms (Aerial, Aerial_Dusk, PointOverValley_FarShadow; render-thread bound); fights
7.6-7.8 avg, p95 8.5-9.4; draws +250-400 since 10-14 (new dressing should be instanced). **The user approved the four
makers as shown** (Roundhouse, Saint Maud, Windfall, Meridian; `Saved/ArtPreviews/Weapons/Concepts/Makers.png`,
`Docs/Work/2026-10-22-Next/MAKERS.md`). `Tools/hashcheck.sh <HASHES.txt>` checks a holding tree's originals.
Landed: SMALL-FIXES (bc9a98c), GROUND-DETAIL (4a32212; also carries MAKERS' M_Gun mark code in build_world_materials).
Evening: MAKERS + SHIELD committed together (067ceca; built together, Main hand-merged the shared hunks). Main's SHIELD
hooks done: `Looter.Fight.DamageScale` 1.5 -> 1.8 (SHIELD's retune; **no played-fight verdict yet**: spiders never reached
the test player; the user's playtest decides, 2.0 if soft), no hurt grunt on a ward hit, Borrowed Time debt = health lost,
wards lying on the ground kept in the map save (`FSavedMapWorld::ShieldPickups`, put back where they lay; guns still go to
Lost Property), `/Game/Art/Shields` cooked. Saint Maud's mark darkened to #9a6a1c (faint on ivory; not yet seen in game).
Not done (SHIELD.md "For Main's files"): boss shower `Roll.Shields` (boss table off), cyan hit edges for ward hits,
HudShotScene ward state, wards in Lost Property, `Shield.*`/`Bounty.*` cue names into LooterSoundCues.h (bank warns
only); MAKERS: marks on Rustler/Thresher enlarged but unseen, far-side mirroring unchecked.
BOUNTY committed (811851f): board on Main Street + 5 arenas placed in Lvl_RansomsRest; Looter.Bounty 6/6, Pacing pass; one
bounty played through in PIE (take, tracker, mark, Strongbox, +XP/silver, banner); Strongbox not opened by hand, other
arenas/tiers untried. Status-line wording change in BountyBoardWidget.cpp built with the evening's last editor build.
Later (BOUNTY.md item 6): real bounty sheet art, BOUNTIES header, a "BOUNTY PAID" banner.
PLAYTEST done: `Toolspackage.ps1 -CloseEditor -SmokeTest` -> `BuildsPlaytest<stamp>WindowsAI_Looter_Shooter.exe`
(first one 2026-10-22_1518, 1.26 GB, ~4 min incremental / ~20 min cold; smoke: menu and Ransom's Rest load PASS);
`Play Latest Build.bat` starts the newest. Fixes on the way: package.ps1 missed the editor (PS 5.1: one pscustomobject has
no .Count; Get-Process + @() now), ImpactSurfaceTests' SetParentEditorOnly inside #if WITH_EDITOR (the game target compiles
the tests). Packaged-game log noise to look at later: a handled ensure at startup (GameFeatureData primary asset type
without the GameFeatures plugin) and ~20 D3D12 compute pipeline creation errors (PSO precache; game runs).
**The next-builds queue is done.** For the user: play the packaged build; judge a real fight's drain with the ward at
DamageScale 1.8 (2.0 if soft); Saint Maud's darker mark; a bounty.

## State at 2026-10-22 noon: the queue is empty
Done since the morning list (all committed and pushed, last 2011943): VFX-2 (fire explosions, no white-ghost kill), FP
rifle/LMG/sniper pulled in, loot polish (element numbers, element deaths, WEAK/RESIST), hero stances + 3P aim-in +
Gauge's head up + vent sound, bolder area tones (read from above only: suggestion "per-place ground detail"), element
parts' own models, painted ghosts (glow fixed), Gauge enamel, Ellis value, Crane/Odessa faces + Crane hair (Odessa's
side hair fixed), Gravemother granite, Sexton, ghost tatters/hands, smooth slime core. Next = the user's playtest and
tuning (Looter.Fight.*, Looter.Gunfeel.*, Looter.FPHands.*, Looter.Pose.*, Looter.Element.*); open small items: Gauge's
revolver aim splays over his pauldrons; the Rustler's orange count glow drowns its Soul-light part; Ranchhand element
tube parts small in first person. The overnight heartbeat cron was stopped (queue empty).

## Morning of 2026-10-22: what to try first (for the user; tell them in 3 sentences)

The overnight run built 12.2-12.5 (commits 4a84f74 ... d79e293). Try in this order, in Ransom's Rest (editor Play):
1. **A fight**: the town gate (Main 3) and the Main Street ambush right after: enemies take turns, surround you, hit
   harder (x1.5), chase faster (x1.3). Tune `Looter.Fight.*` / `Looter.Gunfeel.*` if it's too hard or too easy.
2. **The new guns**: `Looter.GiveWeapon SMG|Sniper|LMG|Launcher Epic 5` (the Rustler's helix, the Longhorn's scope and
   breath, the Thresher's spin-up, the Powderkeg's rockets) and the restyled Bullpup/Ranchhand/Drover.
3. **Elements and drops**: an element gun on spiders (Ember) and the Unpaid (Soul-light); a Legendary drop
   (`Looter.Element.Apply` to test statuses).
4. **The story**: less talk, Hob on the move; find a secret (the Widow's strongbox, Coffin Rock's top).
5. **The clay placeholders**: Knockers at the Bench tramway, Coffin Rats in Tilly's cellar, bandits on Boot Hill
   (confirm you want living human bandits in the Rest).
Open questions for the user: Boot Hill bandits (tone); the rifle/LMG first-person forearm still near-upright (fore-ends
~70 cm out): accept, or move the guns/hands further (a design call); world variety reads subtle: stronger place tones?
VFX-2 (explosion fire, the white-ghost kill, Slag Bomb spikes, impacts) was still running at 05:40 (see below).
**The user's answers (2026-10-22 morning):** keep the Boot Hill bandits; FP rifle/LMG: **pull the guns in** (closer and
lower so the left hand sits low like Borderlands); world: **bolder per-area ground tones** (repaint).
**Character art (the user, 2026-10-22, from CHAR-ART's sheets `Saved/ArtPreviews/Characters/Concepts/CharArt_1-3`;
plan `Docs/Work/2026-10-22-CharArt/PLAN.md`):** approved all four: the painted ghost (every M_Ghost user), a stance per
hero (Ellis keeps his weary idle), the hero value and hair pass, the Gravemother and Sexton looks; plus Gauge's enamel
(back to his pick) and the no-OK items. **New asks:** Gauge looks down all the time: his head tilts up slightly; and
**every hero needs a third-person aim-down-sights pose** (today none aims in when ADS in third person).
Running: CHAR-MAT (materials: ghost, Gauge enamel, hero value, Gravemother/Sexton), CHAR-POSE (stances, hand curl,
Gauge's head, 3P ADS aim; C++ holding tree), CHAR-FACE (Blender: Crane's hair/mustache, faces).
VFX-2 committed (4486136): M_FX_Fire explosions, the white-ghost kill fixed, Slag Bomb cheaper, impacts that read at
30 m; a white spider still showed once in the Slag Bomb after-shot (watch); final p95 not re-measured on a quiet PC.
CHAR-POSE tree ready (`Saved/CharPoseCode`, MERGE.md; Odessa's card is a small cube: make it a flat card at merge; the
stand's capture can't show steam). Lane queue: FP pull-in (running) -> LOOT-POLISH merge -> CHAR-POSE -> tones import ->
element parts import -> CHAR-MAT -> CHAR-FACE import; then build_sound_bank for Hero.Gauge.Vent.
FP pull-in committed (056cea5); Hero.Gauge.Vent rendered (6aec834). LOOT-POLISH's copy into Source was refused by the
permission system for the agent; **the user approved and Main copied it** (hashes matched); the agent builds/tests now.
Bolder tones repaint done (`Saved/ArtPreviews/Terrain/RansomsRest/macro.png`), import waits for the lane (WORLD-ART).
Loot polish committed (142b647). Art scripts committed, not imported (ba175de): element parts' own models (20 rows;
new kit materials need MIs), Unpaid tatters/hands + smooth slime core (`Intermediate/ArtExport_GhostSlime_after`),
Crane/Odessa faces + Crane hair (`ArtExport_Crane/_Odessa`), CHAR-MAT scripts (its GO steps: build_world_materials
M_Character M_CharacterGlow -> build_creature_materials M_Ghost -> ImportModels ArtExport_CharMat_Gauge/_Ellis/_FPArms/
_Gravemother/_Sexton -> build_character_glows). **The user approved Main doing today's holding-tree copies** after hash
checks. CHAR-POSE copied and building now. Remaining lane: tones import -> one IMPORT job (element parts + setup_gun_parts,
ghost/slime, CHAR-MAT steps, faces) -> perf.

## Direction (the user's answers, grill session 2026-10-21) — overrides "next" below

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
- **Release plan:** single-player at launch (co-op not planned). 1.0 keeps 15–18 regions. A free Steam demo once the
  game is 60–70% built, within a year (~Oct 2027). Store page ("Coming Soon") goes up once the core is fun, with a
  Ransom's Rest trailer. Steam's AI-content survey answered plainly (made with AI tools, nothing generated live).
- Playtesters: the user and one friend.

**Gunplay, Phase 12.1 (running 2026-10-21; the user's start prompt: steps 1-3, then they tune in PIE; no friend build;
tests only for new work at commit; perf once before and once after; no capture runs).** The user's calls: Sonniss GDC
bundles for gun/impact sounds (only the zip parts with gun/impact recordings; licence: no resale, no AI training);
the revolver's crit 1.75x. Baseline perf (RR, 8 spiders, Medium): 7.9 ms, p95 9.0. Contract
`Saved/Gunfeel/CONTRACT.md` (not committed: read it on resume). Main wrote `Weapons/GunshotSound` (stub: today's Fire
cue) and FireShot calls it. Running: GUNFEEL-REACT (Opus: hit reactions + the guns' jobs on the target) and
GUNFEEL-KICK (Sonnet: kick, rumble, handling, rifle bloom, shotgun falloff, revolver crit), neither builds; Main
builds once, checks in PIE, commits. GUNFEEL-SOUND starts once the pack parts are chosen and downloaded.
- REACT + KICK built and committed (step 1-3 code, one batch): tests `Looter.Gunfeel` 17/17, Feedback 8, Melee 8,
  Creatures.Anim 8, Weapons 44, all pass. PIE (RR, rifle/shotgun/revolver vs spider hordes, `play.py fight`): ~50 hits,
  slows, heavy hits, rifle pressure, crits, no errors; not seen by eye (no screenshots, the user's rule), rumble not tried
  on a pad. The sound pack: sonniss.com's host sits behind a bot check, so the files come from its listed mirror
  ftpmirror.your.org/pub/misc/sonniss2016|2017; those bundles are samplers (~150 files a part), so only the 59 gun/impact
  files were pulled by range requests (332 MB, `Saved/SoundPacks/Sonniss`, tool `/tmp/zx.js` pattern in Pipeline later).
  GUNFEEL-SOUND done and committed (8dafc34): layered gunshots from 31 recorded excerpts (`Art/Sounds/packs/Sonniss`,
  sources and licence in its README), shaped in script; 161 sounds imported; tests Gunfeel 20/20, Audio 14/14; PIE fight
  clean. Perf after (RR, 8 spiders, Medium): 7.6 ms, p95 8.0 (baseline 7.9 / 9.0). Main added the layer cues to
  LooterSoundCues.h (the bank's check) and `Weapon.*.Core/Mech/Tail*` to check.py's FREQUENT list.
- **12.2 Enemies, the user's calls (2026-10-21, "keep going"; gunfeel tuning by the user still pending):**
  difficulty = Borderlands normal (real damage, keep moving, a careless player dies now and then, a big fight drains
  most of your health); smarter AI = packs surround and take turns + ranged enemies use their range (not dodging, not
  reacting to the player); bigger fights = more enemies at once (6-10 in a normal fight) + a few hand-made set-piece
  fights per area (not waves, not new mixed packs).
- **Overnight run (the user, 2026-10-21 night: "keep working on all things throughout the night", going to bed).**
  Their calls before bed: build enemies now (gunplay tuning by the user later; tune both together); single
  `Tools\grab.ps1` window grabs allowed to check looks (no Python capture batches); new gun kinds **SMG, sniper rifle,
  LMG, rocket launcher** (all four); elements = **story elements: Ember (burn), Soul-light (vs undead), Grave-salt
  (slow, vs spirits)**; story: **cut and rework missions overnight, keep the plot** (cut lines ~half, most as
  on-the-move captions, fetch steps into fights or finds; every story beat kept; list the changes here for review); art:
  **VFX + world variety** (hit, death, muzzle, power effects; vary props, rocks, buildings and ground in RR).
  Order: 12.2 enemies -> 12.3 loot (the four kinds, elements, part effects, bigger drop moments) -> 12.4 pacing/story
  -> 12.5 VFX + world variety. Main's calls go under "Decided unattended (2026-10-21 night)" below.
  **Gun look (the user, from concept sheets in `Saved/ArtPreviews/Weapons/Concepts/`):** concept B "frontier-tech"
  (`NewGuns_ConceptB.png`: sand polymer shells, gunmetal, brass fittings, ember-orange heat glow, cyan optics) for the
  Longhorn sniper, Thresher LMG and Powderkeg launcher; concept A (western walnut/blued) rejected as not sci-fi enough and
  its SMG too like the LMG; the SMG is **option 7 "Helix"** (`SMG_Options2.png`: long round magazine under the barrel
  with a glowing spiral showing the rounds left, wire stock, long and low). Names: Rustler (SMG), Longhorn (sniper),
  Thresher (LMG), Powderkeg (rocket launcher).
  **Last calls before bed:** world variety in RR: **anything** goes (terrain and buildings may change too); elements:
  each element's effect (burn/ignite/damage over time and the like) **deals damage and is more effective against the
  monsters weakest to it** (Main's mapping: Ember vs flesh = spiders, slimes; Soul-light vs undead = the Unpaid, miners;
  Grave-salt vs spirits/wards/Ethereal; slows too); if the context fills overnight, **keep going** (the handoff is kept
  current; this overrides CLAUDE.md rule 10 for tonight only); after a 5-hour usage limit, **continue at the reset**
  (a session cron heartbeat at :17 every hour resumes the run; it dies if the app closes).
- **Steps 1-3 DONE. Next: step 4, the user tunes `Looter.Gunfeel.*` in PIE; Main bakes their values into the defaults
  (React: `FGunfeelReactTuning` fields; Kick: `GunfeelKick::Default`; Sound: `GunfeelSoundTuning`) and the tests that
  read them.** Not verified: the look by eye, rumble on a real pad, the surroundings probe's choices in town/mine, the
  revolver's own layers can peak at -0.1 dBFS indoors (lower `Sound.Low`/`Sound.Mech` if it crackles). Over ~500 lines:
  `CreatureHitReactionComponent.cpp` 504, `UnpaidCreatureArms.cpp` 506, `SettingsMenuWidget.cpp` 511.

## Suggestions and overnight decisions (2026-10-20 to 2026-10-22)

### Suggestions for the user (Main's, newest last; none built without a go-ahead)
- A **player shield** (Borderlands' core loop: a recharging shield over health, found as loot with rarity and
  traits): harder fights then feel fair, and it adds a loot slot. Medium cost (HUD bar, a gear slot, drops, save).
- **Gun manufacturers** (3-4 in-world makers with their own handling quirks and looks, like the parts' families):
  makes every drop's name and feel promise something. Medium-large.
- A **daily bounty board** on Ransom's Rest using the set-piece machinery (a rotating hard fight with a chest): replay
  value before the next region exists. Small-medium.
- **Quick playtest builds**: a nightly packaged build the user can play on the couch without the editor. Small.
- **Per-place ground detail** in Ransom's Rest: the bolder tone repaint (done 2026-10-22, the user's yes) reads from
  above but barely at eye level, where roads, boardwalks and the close ground textures dominate; the lever is per-place
  ground detail layers (town dust, Sink ash, quarry grit) or decals along the paths. Medium.
- **A translucent fire material** for explosions (rockets, Slag Bomb): additive fire washes out in daylight. Small-medium.

### Decided unattended (2026-10-21 night), for the user's review
(Main's calls while the user slept; newest last. Contracts: `Saved/Fight/CONTRACT.md` (+ AUDIT.md), `Saved/Guns/`,
`Saved/Loot/CONTRACT.md`; not committed.)
- 12.2 numbers: 3 melee + 2 ranged attackers at once, a waiting ring at 4.8 m (8 slots, drifting, feints, biased to the
  player's sides/back), chase x1.3 (a walking player gets caught, a sprinting one escapes), creature damage x1.5,
  regen after 7 s at 7%/s (was 6 s, 12%). Fights 6-10, caps 20 near the player / 16 Unpaid. Set pieces: Main Street
  ambush (Main 3), Webwood drop, Providence cave-in, each once per session with a sting, shake, caption and a reward.
- FIGHT-AI's own call: an Unpaid waiting on the ring doesn't phase-step (it would step through the ring); boss adds
  (spiderlings, Abel's risen) take turns too; the slime keeps its own hop rhythm (not faster).
- 12.3: the Longhorn sniper crits x2.0 (its role; the revolver 1.75x was the user's); part effects Ricochet, Burst,
  Explosive, Piercing, Overcharge, Twin (Epic+, a few Rare); elements on parts from Uncommon, ~20-35% a hit; weakness
  x1.75 / resist x0.5: Ember strong vs spiders/Spitter/slimes/Gravemother, weak vs the Unpaid; Soul-light strong vs the
  Unpaid/miners/Bell-Ringer/Abel's risen, weak vs slimes; Grave-salt strong vs Warded/Phasing/Ethereal/the coal, weak vs
  spiders; bosses take statuses at half duration; the Grave-salt grenade becomes the element's damage type. Drop
  moments: rarity beams taller, stingers, Epic+ spin and ground ring, Legendary a 0.25 s slow-down and a 60 m column.
- Rocket ammo is a new class (1-3 per kill drop leaning to the kill gun); the LMG uses rifle ammo.
- FP-HANDS (plan `Saved/FPHands/PLAN.md`, Borderlands measured from 10 BL2/BL3 shots: gloves/wrists only, low; gun on
  a diagonal to the bottom-right; pistols one-handed; no hands in ADS): our support forearm stands upright (~30% of the
  frame) because the FP forearm is 23 cm and the fore-end 65-75 cm out. **Main's call: lengthen the FP forearms ~1.25x
  in the arms art** (all five heroes' FP arms; reversible) so the forearm can slant like Borderlands'; the revolver
  becomes one-handed at the hip (support hand back for reloads/throws); `Looter.FPHands.*` cvars, `ViewFov` off by default.
- ROSTER (Story.md "Creature roster", marked † in the Areas table): Coffin Rat (rat), Bone Hound (the story's coyote) /
  Whiteback Wolf (wolf), Knocker (goblin), Gravepicker and Lookout (human bandits), Slagback / Cairn Walker /
  Spikedriver (golems), Pallbearer (giant), Mesa Stinger (scorpion), Gloam Bat (big bat), Sallow (vampire), Cinder
  Drake (dragon), Sexton Beetle / Bog Mantis (big insects), extras Rattler, Dust Wight, Strawman. RR placeholders: 4
  rats in Tilly's cellar + 4 in the depot baggage room, 4+1 Bone Hounds on the keeper's road (after Main 4, before
  Main 6), 5+1 Knockers at the Bench ore tramway (after Dead Lift), **4 Gravepickers + 2 Lookouts on Boot Hill (after
  Main 3, before Main 4): the Rest's first living human enemies — a tone call for the user to confirm.** Code in
  `Saved/RosterCode` (merge after STORY and VFX; `Saved/Roster/PLAN.md` has the run order, CODEMAP and Pipeline text);
  placeholder hits splash ichor until CreatureBlood gets a clay kind.
- 12.3 C++ agents write into holding trees (`Saved/GunsCode`, `Saved/LootCode`, `Saved/GunsSound`) while 12.2 owns the
  build; each then gets the editor lane in turn (GUNS-CODE first) to merge, build, import, test.
- Progress (newest last): gun models committed (c0a1f3c Rustler/Thresher, 1415246 Longhorn/Powderkeg; previews
  `Saved/ArtPreviews/Weapons/`); world variants phase 1 committed (852087e: 30 models, plan `Saved/World/PLAN.md`; phase 2
  = import, swaps in build_area/dressing/scatter, terrain tone per zone); **12.2 committed (4a84f74)**: tests pass
  (Fight 6, SetPiece 2, Encounters 15, Creatures 114, Story 47, Progression, Recovery, Locomotion, Gunfeel); PIE: at
  most 3 attackers at once, spider hits 22, the Webwood set piece ran (caption, push 2 at 3 left, shower, flag). Perf: the 9.1 ms runs were a busy PC (agents rendering; Main forgot SavedArtPause.flag); quiet, 10 spiders
  fighting in town 7.5-7.7 ms, the director under 0.05 ms (8ac1be7, CSV stats in Pipeline/Enemies.md "Fight: AI"). Gun/element sounds committed (987ea59, not imported). LOOT-CODE phase 1 done
  (`Saved/LootCode/README.md`: hooks other owners must add: WeaponBaseFiring `PlanFor` for Burst/Overcharge,
  `RefreshLootBeam` from `LootDropMoments`, the rocket calling `TryApply`). STORY phase 2 running with Main's rulings
  (no new canon: secrets 6-7 replaced; Fenn's fare exact; "Never a priest's to do" kept; Amos's twist after the fight).
  GUNS-CODE phase 1 done (`Saved/GunsCode/PHASE2.txt`); Main approved its extra files (FirstPersonHandsReload/
  Component/Types, Tests/LootTests.cpp); phase 2 must also: call LOOT-CODE's `TryApply` from the rocket's blast (rockets
  carry elements and Explosive-style effects stay off), give rocket ammo 3 in `BossLootShower.cpp` (not 36), match the
  LMG spin-up cue to its 0.35 s cvar, merge WeaponSounds.h with GUNS-SOUND's by hand.
  STORY phase 2 written (unbuilt): spoken words 4,132 -> ~2,650 (-36%; town mutters cut only where two per stage);
  Python/json changed in place (create_story_lines.py, story_texts.py, mission scripts, hero_lines.json 17 deleted/7
  rewritten, new build_area_secrets.py + hook, build_area_whitlock/story/missionprops_mountain/valley/camps), C++ in
  `Saved/StoryCode/Source` (WantedPoster, RoostRules, OreLift, TownLifeRules, EncounterSpawner `ClearedLines` for Amos's
  twist after the fight, the bail at Main 3, save v4 migration, 13 tests). Secrets 6/7 now loot-only (keepers' stores
  in the old lantern house, a spiders' hoard under the hanging tree); secret 1 = "the Widow's strongbox". Its run order:
  merge+build, create_story_lines -> create_mission_assets -> create_side_mission_assets -> create_hero_lines,
  build_area RansomsRest gameplay, THEN delete 25 retired `/Game/Data/Story` DA_Lines_* (list in its report, kept in
  `Saved/Story/PLAN.md`; deleting first opens a modal), tests Lines/Missions/Story/Session.StepMigration/
  World.TownLife/Roost/Poster/Progression.Pacing.
  **The user, later that night (two art asks):** (1) apple trees' apples only show within ~5 ft (1.5 m) of the tree:
  fix so they read at normal distances; (2) first-person hands: compare with Borderlands' first-person view and match it:
  ours show too much of the arms, clip into the guns, or don't hold the weapons properly. Queued: APPLES investigation now
  (read-only, the fix applied in the lane), FP-HANDS after GUNS-CODE phase 2 (it owns FirstPersonHands* tonight).
  (3) **the existing guns (Bullpup, Ranchhand, Drover) must match the new frontier-tech style** (concept B). GUNS-RESTYLE
  started: Blender scripts only, part keys and sockets unchanged (saved guns and hand holds keep working); import in the lane.
  Apples fixed in Trees.py (bf9d42c, 020901b: buried inside the leaf cards and tiny; now hung outside, ~20 cm, twice as
  many, LODs 70/30% at 0.3/0.1; `Saved/World/APPLES_FIX.md`): **reimport pending** (`Tools\models.ps1 -Only Trees`, then
  check at 8/15 m with r.ForceLOD 0/1; if apples vanish at LOD1 use '100,40').
  **The user, later still (creature roster):** add more enemies and creatures as the game goes on; future regions may
  have **Goblins, Rats, Human Bandits, Golems, Wolves, Giants, Scorpions, Big Flying Bats, Vampires, Dragons, Big Insects
  and more** ("subject to change, but the storyline should read as potentially having these creatures"); where the story
  gives a reason for a different enemy in an area, place **a placeholder (a blob of clay or similar) with the enemy's
  name**. ROSTER agent started: fits them into the Revenant saga per region (Story.md), then a placeholder creature in a
  holding tree and placements where the story supports them.
  VFX phase 1 written (`Saved/Vfx/PLAN.md`, `Saved/VfxCode/README.md`; base 8ac1be7, blob hashes in
  `Saved/VfxCode/Originals/HASHES.txt`): bigger hit splats/starbursts, impacts by surface, kill gore by gun and tier,
  every power showing on the body, `M_FX_Shape/Puff/SplatDecal` via `build_vfx_materials.py` (unrun), `Looter.Vfx.*`;
  est. +0.2-0.4 ms; muzzle/rocket/tracers only as a proposal for GUNS-CODE's files; at merge split EmberDustDevil.cpp
  (755 lines). Every holding-tree merge is a 3-way merge against the base its README names (others' merges land first).
  Lane queue after GUNS-CODE phase 2: LOOT-CODE -> STORY -> VFX -> ROSTER -> trees reimport + WORLD-ART phase 2 ->
  GUNS-RESTYLE import -> FP-HANDS.
  **Docs organisation (the user, 2026-10-22):** the Project Library page (pinned; https://claude.ai/artifact/
  1yHV6GGJvvcwxWwZqi4T4s, built by `node Tools/docindex.js` with `Docs/INDEX.md`; republish at each session end).
  Their picks, all four: working notes into git (`Docs/Work/<date>-<job>/`, archived when the job is committed),
  split Pipeline.md into a topic folder with a short index, keep Handoff.md to now/next (older sessions to the
  archive), archive the Style Lab docs. Placeholders give XP and loot like real enemies (told ROSTER). Do the moves when
  no agent is reading those files (contracts of running jobs move after their commit; the Pipeline split when no
  phase-2 agent is writing it). DONE 2026-10-22: Pipeline split into `Docs/Pipeline/<Topic>.md` (index `Docs/Pipeline.md`), Handoff trimmed (older text in `Docs/Archive/Handoff_history.md`), finished jobs' notes of 2026-10-21 in `Docs/Archive/Work/2026-10-21-<Job>/` (Saved/Vfx2 still running, stays in Saved/); new jobs use `Docs/Work/`.
  More calls (2026-10-22): FP-HANDS may look up Borderlands gameplay screenshots online to compare framing (nothing copied
  into the game); placeholders show "<in-world name> (<type>)", e.g. "Tommyknocker (goblin)" (told ROSTER).
  **New guns committed (62bf264)** and checked in PIE (grabs `Saved/Screenshots/Tools/newguns_*`): sniper scope picture
  `UI/HUD/HudScopeWidget`, rocket self-damage capped at 35% (`Looter.Gunfeel.Launcher.SelfCap`). Art follow-ups: the
  Rustler's Holo sight has no cut window (aiming shows its solid back); the Rustler's helix (its signature, the
  rounds-left glow) can't be seen in first person (under the gun): show it (move it to the side, angle the hip hold, or a
  side window). LOOT-CODE phase 2 running (owns the lane).
  **Loot committed (64c6128):** elements, part effects, drop moments; all seven families' parts set up; tests pass;
  8 burning spiders 7.6 ms. Left: a status kill still plays the gun's kill knock; element damage numbers not coloured;
  element parts reuse other parts' models (art later). Restyle: Bullpup and Drover round 2 done (sand/gunmetal/ember,
  sockets identical; Drover renames in `Saved/Guns/RESTYLE_RENAMES.md`), Ranchhand fork still working; commit + import
  in the lane after it lands. STORY phase 3 (merge/build/run) now owns the lane.
  **Story committed (a43b80e):** talk 4,132 -> 2,670 words (-35%, givers -44%), 10 secrets (9 chests), bail at Main 3,
  Amos after the fight, save v4; tests pass; PIE grabs `Saved/Screenshots/Tools/story_*`. **Coffin Rock's boulder steps
  can't be climbed** (falls off the rounded tops after four): give it a ramp/ledges in WORLD-ART phase 2. VFX phase 2 now
  owns the lane (may also do the muzzle-per-kind/rocket/tracer proposal in GUNS-CODE's files).
  **VFX committed (5b573f3)**; grabs `Saved/Screenshots/Tools/vfx_*`. Weak and queued for a VFX-2 follow-up: the rocket
  blast reads as a pale puff in a flat ring (additive fire washes out in daylight: needs a translucent fire material);
  a shotgun-killed spider turns into a white ghost (a hit flash/overlay held through death); Slag Bomb p95 8.6 ms
  (cvars Looter.Vfx.Blast.Scale / .Smoke); dirt and stone impacts small and pale; not seen: tier kill rings, slime gel
  puddle, Last Rites column; sniper vapour trail not done (BulletSubsystem.cpp). Restyle scripts + renames committed
  (1f30423, 87262e9). ROSTER phase 2 now owns the lane; then trees reimport + WORLD-ART phase 2 (+ Coffin Rock steps),
  restyle import (setup_gun_parts.py after), VFX-2, FP-HANDS, Rustler sight/helix fixes.
  **Placeholders committed (37636b4):** 5 kinds fought in PIE (XP, loot, "Name (type)" labels, clay hits; grabs
  `roster_*`); the Bone Hounds moved 20 m in from the Rim (they followed the player over the edge). WORLD-ART phase 2
  now owns the lane (variants, scatter, terrain tones, apples reimport, Coffin Rock ramp).
  **World variety committed (5cc096a)**: honest read: the differences are subtle in before/after (`Saved/World/
  compare_*.png`); the ground tones read faintly; a stronger-tone repaint (~46 min) is the next lever (a suggestion for
  the user). RESTYLE-IMPORT (Sonnet) owns the lane now; RUSTLER-FIX (Blender: open sights, visible helix) in parallel.
  Then FP-HANDS, VFX-2, the Rustler import. RUSTLER-FIX done in Blender (open Holo sight in the shared kit, also on
  the Thresher; helix moved 1.2 cm left and enlarged; Foregrip socket now (25, -1.2, -8.45)): reimport
  `Tools\models.ps1 -Only Rustler,Thresher` in FP-HANDS phase 2 (its left-hand grip must follow the Foregrip move).
  FP forearms lengthened (committed; FPArms.py FOREARM_STRETCH 1.25, 26.1 -> 32.6 cm): import SK_FP_Arms + the four
  heroes' arms and texture sets in FP-HANDS phase 2; its tests need FirstPersonHandsTests.cpp:211 rest grip point
  (40.738, 9.539, -29.85), FPHandsFramingTests.cpp:105 Length 23 -> 28.7, and the comment at
  FirstPersonHandsFraming.cpp:358-359.
  Order of the build lane next: FIGHT-AI perf -> GUNS-CODE phase 2 (merge `Saved/GunsCode`, then `Saved/GunsSound`, import
  the four models) -> LOOT-CODE phase 2 -> STORY merge -> WORLD-ART phase 2.

### Decided unattended (2026-10-20 night), for the user's review
(Main's calls while the user slept; newest last.)
- BARROW-ART accepted and committed (b0e4ed6): SK_Gravemother on Spider.py's 31 bones (14.97k triangles), the stone's
  three states with roots, 5 shards, 3 chips, the fight's earth, FX/decal textures; her hide reads a little paler than
  the sheet's #8C8070 under the preview's warm light: judged in game. BARROW-CODE imports them and runs
  `build_grave_fight_materials.py` between its builds. Its card colours are passed by code (MI tint white).
- A ninth job as ART freed its slot: LINES drafts every hero variant of the existing lines plus the pick-by-hero design
  into `Saved/Lines/` (VARIANTS.json, DESIGN.md); Main wires it after BARROW-CODE. First-person arms start when a hero
  model agent finishes.
- HUD-PORTRAITS accepted and committed (d6272f0): the four inked HUD portraits; Crane's single cyan eye beside the coin,
  Gauge's warm lamp (visor drops when hurt), Odessa without the hand card, Pike with the tripod's legs. Wiring after
  BARROW-CODE from `Saved/HudPortraits/WIRING.md`.
- LINES done (`Saved/Lines/`, git-ignored: VARIANTS.json 148 rows, DESIGN.md, ELLIS_ONLY.md): a lookup table
  `DA_HeroLines` keyed by Ellis's exact text, resolved in `UCaptionSubsystem::Play` and ~15 read sites, tokens {Hero}...,
  test `Looter.Lines.NoFamily`. Its five questions, Main's provisional calls (**ASK THE USER at 5 am, the first one
  above all**): (1) the cold open contradicts the four select-screen stories (Odessa caught Ned's bullet, Pike left in a
  ravine, Crane forty years dead, Gauge woke in a wreck, yet every hero dies on the Point): provisional (a) keep the cold
  open for all, their own regions tell their earlier past; the other choice (b) cuts the hero out of the cold open for the
  four (~15 rows need a second neutral). (2) The Deacon says "kid" to every hero, Crane and Gauge too (the user's own
  words: "the Deacon's kid is whoever you are"). (3) The townsman "Hollis" renamed Harlan for every hero (Crane is
  Hollis). (4) The non-Ellis epitaph "<NAME>. STOOD UP AT THE LAST." and each hero's poster line (Gauge "RAILROAD
  PROPERTY") kept as drafted. (5) Sexton's per-hero sweeteners (Odessa's marker, Crane's fare, Gauge's spark, Pike's last
  road) in place of "your father crosses". Art the lines need: SM_Headboard_<Hero> x5 split from Grave_Ellis, a bare
  keepers' roll, four poster atlases (not started: waits on the user's cold-open call).
- FP-ARMS started (the slot HUD-PORTRAITS freed): SK_FP_Arms_<Hero> x4 on FPArms.py from its own file (FPArms.py
  untouched: its digest would re-bake Ellis's arms), rig jsons `Intermediate/FPArms/FPArms_<Hero>_rig.json`, wiring
  notes `Saved/FPArms/WIRING.md`.
- BARROW-CODE done and committed (50a6f1e): the Barrow fight on the real assets, 726/726 tests; its fix to ART's
  materials script (M_GraveDecal's Tiles from RGBA); the bestiary page updated. Not checked: perf (§11.4: the PC was under
  memory pressure from the agents; PIE hitches were PCG and Mass), the decals' look by eye (Reveal direction, the crack's
  end cap joining), real bullets on the stone/clod/mounds by hand (tests cover them), one plough on bumpy ground stopped
  at 5.8 of 12.2 m. New palette entries LooterUI::Color::GraniteLight/Granite/GraniteDark (the stone bar).
- With the build lane free: HUD-WIRE (Sonnet) applies `Saved/HudPortraits/WIRING.md` (incl. Gauge's hurt flicker),
  builds, tests, shoots each hero's portrait in PIE; LINES-WIRE (Opus) writes the line-variant code into a holding tree
  `Saved/LinesWire/Source/...` (+ `Tools/Unreal/hero_lines.json`, `create_hero_lines.py`) on Main's provisional §7 calls
  (cold open kept for all; "kid" for all; Hollis -> Harlan; epitaph and posters as drafted; Sexton's sweeteners), to be
  merged and built by it after HUD-WIRE and the hero imports.
- HUD-WIRE done and committed with this note: each hero's own HUD portrait (live on `Looter.Hero`), test
  `Looter.HUD.PortraitSets`, 727/727. GAUGE reported done (not yet reviewed or imported): SK_Gauge 31,956 triangles, 67
  bones (no swing chains), five 2048 sets GaugeHead/Chest/Belly/Arms/Legs, slots + GaugeLamp/GaugeFire glows, export in
  `Intermediate/ArtExport_Gauge`; expect rifle stocks to clip its boiler/right pauldron when aiming; check its 1.28x hands.
- **USAGE LIMIT hit (2026-10-20 ~02:00). Every running agent was STOPPED mid-step; their work is on disk, nothing of
  theirs committed.** Where each stood: CRANE (previews read as the portrait; was about to rebake with fixes + audit r2),
  PIKE (clay reads well; was starting the bake with previews), ODESSA (round-2 daylight renders), POWERS (writing
  `Saved/Powers/`; was about to set the twelve entries in `create_ember_assets.py`), FP-ARMS (building Odessa's arms),
  LINES-WIRE (writing `Saved/LinesWire/`; at the Bestiary widgets). **On resume:** resume each with SendMessage (same
  agent ids in this session, else new agents briefed from the two contracts and their files), saying what landed on disk;
  review Gauge's previews, import (`Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_Gauge`), run
  `build_hero_retarget.py Gauge` (untested), `create_ember_assets.py`, check in PIE with `Looter.Hero Gauge`; then each
  hero as it lands; then merge POWERS' and LINES-WIRE's holding trees (they build and fix their own); ask the user the
  cold-open question (Lines §7) first thing.
- **Resumed ~01:30** (the limit lifted): the six agents resumed with SendMessage. **Gauge is in the game**: imported
  (68 bones, 7 slots, 17 hit zones), `build_hero_retarget.py Gauge` (auto aligned), `create_ember_assets.py` filled
  DA_Hero_Gauge's mesh/retargeter; checked in PIE (`Looter.Hero.Unlock 1`, `Looter.Hero Gauge`): third person from
  behind and front, its HUD portrait, Slag Bomb on Q (no code yet). Not yet looked at: aiming (rifle stock vs boiler and
  right pauldron), its 1.28x hands on the grips, running. The DA_Ember_* assets show modified (POWERS' data, re-saved by
  that run): left out of Gauge's commit, they go in with the POWERS merge.
- POWERS done in its holding tree (`Saved/Powers/README.md`; the reviewer's items fixed): twelve powers, icons, 35 cues
  (95 WAVs, check 0 flagged), tests `Looter.Embers.Power.<Id>`; BulletSubsystem.h/.cpp and LooterSoundCues.h apply as
  patches (`Saved/Powers/Patches/`). Its calls: damage = the demo's x1.43 (the demo's 14-damage round vs today's 20),
  Last Rites 60 burst / 90 strike, Long Shot +15% per 10 m, Grave Ward heals 5/s, Double or Nothing never misses twice
  running (cooldown 15), Long Shot marks its own targets, Lucky Streak crits every third pull, Grave Ward slows by
  CustomTimeDilation, Spyglass without a through-wall silhouette (a mark widget and an orange wash instead).
  POWERS phase 2 running: it merges its tree, builds, runs the asset/sound scripts and tests, checks each power in PIE
  (it owns the editor until it reports).
- LINES-WIRE done in its holding tree (`Saved/LinesWire/README.md`: 8 new files, 23 replaced as patches, all applying;
  `Tools/Unreal/hero_lines.json` + `create_hero_lines.py` in place; `build_area_vendors.py` Hollis -> Harlan). Merge after
  POWERS: resume it (it builds and fixes its own), run order in its README (create_hero_lines.py expects "HEROLINES done:
  146"; re-run `build_area_vendors.py RansomsRest`). Its choices: tests read Ellis's words unless a test sets a hero
  (FScopedHero); a row can name its speaker; a few extra read sites (choice titles, board/artifact titles). WantedPoster.cpp
  (507) and SessionSubsystem.cpp (505) end just over the ~500-line limit.
- PIKE done and committed (d97f803, not imported: POWERS imports him at its next break, with `build_hero_retarget.py Pike`):
  34,990 triangles, 92 bones (skirt chains fl/sl/bl/bm/br/sr/fr, bandana, hat), no holster slot; the pack bends with the
  back; spectacles are rims only (no glass master); audit leftovers 3 cm or less (lips through the mustache cards, the skirt
  top through the belt's dip, coat edges through straps, the spyglass into the skirt in a stride); his side whiskers
  stair-step up close (a polish item). His agent found `looter_looks.skin` multiplies by a `zones` attribute Ellis.py never
  sets, so T_EllisSkin_BC baked near-black: the user asked for Ellis's face "kept dark" (2026-10-15), so check before
  changing it. His shoulders sit 6.5 cm lower than Ellis's (retarget).
- CRANE done and committed (2fe2b16, not imported: POWERS imports him with Pike): 33,507 triangles, 144 bones (skirt ring,
  cape as Ellis's, hair/hair_fl/hair_fr, lantern_01 = the hook, hat), the stoop posed into the bind pose (head 7 cm ahead
  of the neck: check the retarget), no waistcoat, no holster; small audit leftovers (coat top through the cape behind the
  neck, coat through the sleeves at the armpits, chain links into the coat, hem 1.4 cm under the ground crouched); fine
  pleats in the coat's back at the seat. Shared-code wishes in his report: ellis_hat brow height, ellis_hair colours as
  parameters, looter_human posed joints; the portrait Crane.py's coin faces point inward.
- FP-ARMS done and committed (e7d569d, not imported or wired): SK_FP_Arms_<Hero> x4 on Ellis's arm skeleton (rig jsons
  match: the hands code binds them unchanged); wiring in `Saved/FPArms/WIRING.md` (UHeroDefinition FirstPersonArms +
  scale: Odessa 0.84, Pike 0.86, the rest 0.88; the hands component loads the played hero's; a test); resume FP-ARMS for
  it after the merges. Gauge's are 17.7k triangles (could be trimmed); its finger blocks may sink 2-3 mm into thin grips.
- LINES-ART done and committed (588e6af, not imported): SM_Headboard_<Hero> x5 (Grave_Ellis loses its board: import
  Graves only together with the story script run), KeepersRoll_Bare, T_Posters_<Id>_BC x4 + MI_PosterDecal/Scrap_<Id> via
  `build_decal_materials.py`; run order in the commit message (after the LINES-WIRE merge).
- POWERS merged, built and committed (425d1c3): the twelve powers, Crane and Pike imported with their retargeters,
  DA_Hero_Crane/Pike filled: **Crane, Gauge and Pike are playable; Odessa waits for her model.** 739/739. Each power played
  in PIE and tuned for daylight (bigger slag blobs, Iron Hide's forge glow, Rail Spike's glow line, thicker tripwire,
  bigger chain links and cards). Weak or unchecked: the Slag Bomb pool reads as chips in a fire ring, not molten slag (a
  flat emissive mesh or decal: an art call); Rail Spike's line is seen end-on from behind; Crane's hook swing not seen in
  motion; Last Rites' death burst only by its test; Iron Hide's glow in first person. Perf skipped (PC busy).
  `Saved/Powers/` is stale now (Source is the truth). Next: LINES-WIRE phase 2 (merge, build, the line art's imports and
  area scripts), then FP-ARMS wiring, then Odessa.
- ODESSA done and committed (283ff95, not imported): 34,909 triangles, 92 bones (tails on skirt_sl/bl/bli/bri/br/sr, hat),
  no cloth simulation, hair a combed shell + knot (Character master), the ace in the hat band, the card case on her right
  side. Her import (+ `build_hero_retarget.py Odessa`, `create_ember_assets.py`) goes with the FP-ARMS wiring step; then
  no hero is left unplayable, so the hero tests need a synthetic unplayable hero. BodySwing's skirt settings were tuned
  for a duster: her short tails may want their own.
- **Found by ODESSA: `bmesh.ops.create_uvsphere` orders faces differently each Blender run**, and a kept bake (digest
  match) may then not fit a later export's UVs (decimate, smart UV and the atlas pack follow the order): scrambled
  texture in game. Ellis, Pike, Crane (and maybe Gauge, the FP arms) call it. BAKE-CHECK agent diagnosing per model
  (no fix yet); the likely fix is a shared fixed-order sphere (odessa_kit.uv_sphere) and a rebake + re-export of the
  models that mismatch.
- LINES-WIRE merged, built and committed (02a3628): DA_HeroLines (146 rows), resolution in captions and every read site,
  the painted words imported and placed by `build_area.py RansomsRest gameplay`; 743/743. Checked in PIE as Crane (his
  headboard, card, captions, poster, bare roll, Delia's line) and as Ellis (unchanged). The headboard is now the readable's
  own mesh, read from beside the grave (~2 m).
- FP-ARMS phase 2 running (owns the editor): wiring, the four arms' import, Odessa's import + retarget, the hero tests
  with a synthetic unplayable hero, PIE per hero in first person.
- Git hiccup seen three times tonight: `unable to write file .git/objects/..: Permission denied` on `git add` of an asset
  the editor had just saved; nothing staged; the same command a few seconds later works (a transient lock: the editor's
  revision control or the virus scanner). Retry, don't change permissions.
- `Art/Models/Characters/Crane.drape.json` shows modified (BAKE-CHECK's test export re-simulated the coat: evidence for
  the sphere-order problem); left uncommitted until BAKE-CHECK reports.
- **BAKE-CHECK's diagnosis** (evidence `Saved/BakeCheck/`): mismatched in game now: SK_Crane (Coat 4.2%, Gear 15.4% of
  island pixels on unbaked background), SK_Pike (Gear 9.7%), SK_Odessa (Gear 15.1%, imported 03:18 by FP-ARMS),
  SK_FP_Arms_Crane (Skin 6.5%); fit: Ellis, Gauge, SK_FP_Arms, the other three FP arms. Causes: (1) **thread count**: the
  bakes ran at artrun's -Threads 6, the exports through models.ps1 at all threads (Decimate and push-outs differ);
  (2) create_uvsphere's random face order (Ellis's buttons and knot, Crane's left eye and star, Pike's buttons, knot,
  pouches, studs); (3) the cloth cache rounds points to 5 decimals but the mesh kept full precision. It changed
  `Crane.drape.json` and `Ellis.drape.json` by mistake and asked Main to `git checkout` them (its own attempt was denied):
  not done on its say-so; the fix's re-export rewrites both. **PIPE-FIX running**: models.ps1 and artrun at one thread
  count (6), a shared fixed-order `uv_sphere`, rounded simulate points, a packed-UV hash guard beside bake.digest, then
  re-exports of Crane, Pike, Ellis, Odessa, FPArmsCrane/Pike/Odessa, each checked to fit; Main reimports after FP-ARMS.
- FP-ARMS phase 2 done and committed (898a8ca): each hero's first-person arms load by the played hero (UHeroDefinition
  FirstPersonArms + scale: Odessa 0.84, Pike 0.86, others the component's 0.88), swap on `Looter.Hero`; SK_Odessa imported
  and retargeted: **all five heroes playable**; 744/744. Checked in PIE: all five with rifle, shotgun, revolver, hip and
  aim; Gauge's wrist balls read big in the revolver hold (in character), Crane's wide cuffs fill the bottom of the
  revolver view. **Odessa's tailcoat doesn't show in game** (only the gold waistcoat and dark sleeves): ODESSA resumed to
  find and fix it (likely inward faces) in its own files; PIPE-FIX re-exports her last, then Main reimports.
  **Found and fixed by Main:** not the mesh (ODESSA rendered the export one-sided: whole and wine) but hidden sections
  kept by index on the component across a mesh change: Ellis's slot 5 is EllisHolsterGun (hidden while a revolver is
  drawn), and slot 5 is Odessa's (and Crane's) coat; `Looter.Hero` swapped meshes with it still hidden.
  `UBodyFigureComponent::Wear` now calls ShowAllMaterialSections on every LOD of a new mesh. Checked in PIE: Ellis with a
  revolver, then `Looter.Hero Odessa`: her wine coat shows. Odessa's model needs no change (PIPE-FIX re-exports her for the
  textures only; told to wait, now free to go).
- PIPE-FIX done and committed with this note: one thread count (6) for every model run, `looter_model.uv_sphere` and
  `fixed_order`, rounded simulate points, the uv.digest guard (Pipeline/CharacterModels.md "A bake and an export must build the same
  mesh"); re-exported and reimported SK_Ellis, SK_Crane, SK_Pike, SK_Odessa, SK_FP_Arms and the Crane/Pike/Odessa arms
  (Gauge's unchanged). Islands off: all sets 0-0.2% except Odessa's Gear 0.43% (one 12-face island at the start of her
  watch-chain tube gets no bake source: `odessa_clothes.build_watch_chain`). Each export repeats bit-identically.
- In-game front shots of all five after the reimport (`Saved/Screenshots/Tools/hero_front_<Hero>.png`): right, except
  **Pike shows a dark patch low on his coat front at the right hip** (the spyglass frog's leather or the coat's inside
  showing: look next). PIKE resumed to diagnose and fix it (re-export; Main reimports).
- **Morning state (2026-10-20 ~04:50): the hero pick is built end to end**: five playable heroes with their own bodies,
  first-person arms, HUD portraits, three powers each and their own words; the Gravemother's Barrow fight; all committed
  and pushed (last 6a5c172), 744/744. **The user's call (2026-10-20 morning): the cold open is kept for every hero**
  (Story.md "The heroes"); Main's other four provisional Lines calls stand unless the user says otherwise. Waiting on the
  user: their playtest.
- **Performance pass (the user's pick, 2026-10-20 morning; Docs/Performance.md rows):** RR spawn on Medium, each hero
  worn: all five 7.7 ms (129-131 fps; Ellis 7.7 before the heroes). With 8 spiders: baseline 7.7 (p95 8.3); powers
  repeating: Slag Bomb 7.8, Iron Hide 7.7, Fifty-Two Pickup 7.8, Spyglass 7.8, Sundown 7.7, Grave Ward 8.0 -> **7.8 after
  power lights went High/Epic only** (`AEmberPropActor::LightsAllowed`; the light was the whole +0.3 ms, proven with
  `ShowFlag.PointLights 0`). The Gravemother's Barrow fight over 2,400 frames: 7.7 (p95 8.2). New dev command
  `Looter.Embers.Repeat <id> <seconds>|off`. Not measured: the 37-view tour (nothing changed the world's views).
- **Polish (the user's pick next):** POLISH-ART (Blender) makes SM_SlagPool (molten slag as an emissive surface, notes
  in `Saved/Polish/SLAGPOOL.md`) and fixes Odessa's black watch-chain island; POLISH-CODE (owns the editor) makes Rail
  Spike read from behind and in first person, checks and fixes the Gravemother's ground decals by eye, tunes Crane's hook
  (and hair, coat, cape) swing in motion, then wires the slag pool once Main imports it. PIKE still on his coat patch.
- POLISH-ART done and committed (746b33e): SM_SlagPool (+ SlagPoolCrust, SlagSplatA/B; slots SlagCrust, SlagMolten,
  SlagWhiteHot, SlagGlowEdge; `Saved/Polish/SLAGPOOL.md`), and Odessa's watch chain (its game tube ran through her body:
  OdessaGear 0.43% -> 0%). POLISH-CODE imports both and wires the pool.
- PIKE's coat patch found and fixed (f4c49a9, rig only, mesh identical): BodySwing keeps skirt points ~5 cm from the
  hand and ~7 cm from the forearm; his side skirt chains hung where his idle hand hangs, the springs threw the front panel
  over the pocket. Side chains now behind the hip (0.65 of the way round), front ones outside the thigh. POLISH-CODE
  reimports SK_Pike and checks whether Crane's left-hand flare is the same (a code-side rule preferred). Pike's leftovers
  (not done, ask): the cuff tabs bake near-black (turn them out in `pike_clothes.build_cuffs`, a rebake); a dark sliver
  on the left hip from behind; the coat front tucks under the thighs crouched.
- POLISH-CODE done and committed (582517d), 747/747: Rail Spike reads from behind (head glow, shock rings, steam, side
  sparks, ground dust, a glowing AGroundCrack scar); the Gravemother's decals (Reveal now crawls, soft FadeIn/FadeOut
  ends, no tiles floating over old berms; the end cap joins within ~1.5 cm by measurement, never seen; the clod's
  landing shadow never caught in a frame); BodySwing (BodySwingKinds.cpp): a hook's and long hair's own settings with a
  `Carry` share, pushed cloth drops its speed into a limb (Crane's 14 cm standing wag gone, all heroes), hands push the
  shortest way at most 3 cm (Crane's flared coat; same cause as Pike's, now fixed in code for all); tests
  `.Hook/.Rest/.Hand`; the slag pool wired (`AEmberPropActor::GetModel/AddPiece`). Not measured: perf of the new Rail
  Spike and slag looks (the PC was busy).
- User said "keep working": PIKE does his leftovers (cuffs, whiskers, hip sliver); PORTRAIT-CLEANUP (Sonnet) merges the
  portrait scripts' helpers into bust.py with scenes and model exports compared identical; SELECT resumed for **a live
  3D hero stand** in the select screen's middle (the chosen hero's model idling on a stand, turning, draggable, a rim in
  the hero's colour; portraits stay on the cards).
- PORTRAIT-CLEANUP done and committed (520ee4a): `Art/Portraits/bustkit.py` (955) + bust.py (862) hold the shared helpers;
  hero portraits Crane 603, Pike 540, Gauge 521, Odessa 709, Ellis 567; proven identical (15 scenes, 4 model exports;
  tools `Saved/PortraitCleanup/scene_cmp.py`). Model scripts list `bustkit.py` in BAKE_SOURCES (Pike's via PIKE). The FP
  arms of Crane and Pike rebake at their next export (fp_arms_heroes digests the portrait files).
- SELECT's live 3D stand done and committed with this note: `AHeroSelectStage` (an unpossessed pawn: a hidden mannequin
  playing ABP_Unarmed, the hero's body copying it through their retargeter via `UBodyFigureComponent::SetShownHero`) on a
  plinth in a 720x900 capture, swaying around three-quarter view, drag/right stick to turn; key, fill, a hero-colour rim
  and a kicker in their first ember's colour; a 0.28 s fade + 0.6 s turn-in on a switch; a 76 px painted face on the
  panel. Cost only while the select is open: +1.4 ms GPU, +2.1 ms render thread (PIE, Medium). Live tuning
  `Looter.HeroStand.Key/.Fill/.Rim/.Kicker/.Exposure`, `Looter.HeroStand.Run 0` holds it. 748/748, `Looter.Heroes.Stand`.
  Polish noted: Crane's face reads blotchy at the stand's size under the rim light.
- Perf of the polish (RR spawn, Medium, Gauge, 8 spiders): baseline 7.7 ms; Rail Spike every 3 s with its new look 7.8;
  Slag Bomb every 8 s with the slag pool model 7.8 (Performance.md rows).
- POLISH-CODE round 2 committed (7154cad), 748/748: a skirt with no front chains is a tailcoat with its own swing
  (Odessa's tails 60-77 deg out at a run -> 40-47, settle in 0.5 s); the stand's rim colour lights at most luminance 0.6
  (`AHeroSelectStage::RimShareFor`: Crane x0.71, Pike x0.85); the clod's landing shadow reads through grass
  (`M_GraveDecal` AlphaScale, 1.8 on the shadow); the crack's end cap seen joining. Then Crane's skin SubsurfaceAmount 0.15
  (1ead855; Crane.py's GAME_MATERIALS and MI_CraneSkin set directly; his next export rebakes as the digest changed).
  Known test trap: typing `Looter.Hero.Unlock` at the console fails three hero tests until the editor restarts (fixed
  since: 88ec205).
- **Enemy tiers (the user, 2026-10-20): Regular (white), Restless (blue), Gravebound (purple), Hallowed (yellow),
  Ethereal (red)** for every regular enemy (not bosses, not named Legendary monsters). The design with every number and
  the user's calls (tier-only colours; crits pierce half the defense; Ethereals 1 in 1,250 on Ransom's Rest; numbers
  approved): `Saved/Tiers/DESIGN.md`; the build contract `Saved/Tiers/CONTRACT.md` (neither committed; Story.md's ranks
  table is updated by TIER-CORE). Running: TIER-CORE (owns the editor: enum appended Hallowed=5, Ethereal=6, settings,
  defense, promotions, loot, tag, tests), TIER-TRAITS and TIER-LOOK (holding trees `Saved/Tiers/Traits|Look/`, merge
  after CORE in that order), TIER-SOUND (13 cues). PIKE's leftovers done and committed (872b030: cuff tabs, softer
  chops, the hip patch; 34,900 triangles); TIER-CORE reimports SK_Pike at a break.
  TIER-SOUND done and committed (d1db327, 13 cues; WardHit asked for after). TIER-TRAITS done in its holding tree
  (`Saved/Tiers/Traits/README.md`): replaces CORE's 3 stand-ins, adds the component's 5 topic files, UCreatureRallied,
  CreatureTraitRules, UCreatureTraitSettings (every number), `Looter.Traits.Give|Show`, 9 tests; small changes to
  HealthComponent (an `FHealthShield` delegate after defense) and CreatureBase (`SettleBody()`). Its calls: a "hard hit"
  for Phasing = 8% of max health within 1 s; the Spitter and Bell-Ringer never Fleet or Grave-burst, the Bell-Ringer
  never Vengeful; every kind can phase; the ward doesn't regenerate; an extra cue `Creature.Trait.WardHit`.
  TIER-LOOK done in its holding tree (`Saved/Tiers/Look/README.md`): the tier glow and Ethereal's ghost-rim as a "tier
  shell" (a leader-posed copy of the body in a new `M_CreatureTier`, one translucent draw per tiered creature: the
  overlay slot is taken by the hit flash, and the bodies' own slots share M_World/FoliagePalette), the tag per tier
  (frame and clamps, the word popping on notice, a dim trait line), notice stings over the rank sting, death shows
  (pillar 9 m/1.7 s Hallowed, 13 m/2.1 s Ethereal), loot pops 0.15-0.18 s after, the XP popup tinted. Unverified until
  PIE: the spider eye UV patch (if the abdomen glows instead, V 0-0.125) and the slime core's place. Merge after TRAITS
  (`git merge-file` against its patches/base), then `build_tier_material.py`; perf with `Looter.Perf.Horde Spider 8
  Ethereal`. To save time (the user: "multitask"): TIER-TRAITS stages ONE combined TRAITS+LOOK merge in
  `Saved/Tiers/Merged/` against CORE's working copy now, and does the single merge/build/test/PIE/perf after CORE.
  TIER-CORE done and committed (b6fbda0, 753/753): area rise by the area's lowest creature level (1 = RR odds, 50+ =
  late, `LateAreaLevel`), caps count living creatures of the tier and a roll past a cap steps down, no save change
  (ranks aren't saved; version 3), trait seed CRC32(actor name) x the promotion roll (runtime spawns their UniqueID);
  Looter.Tier.Odds legendary 0.30/2.19/5.51/13.33/21.38%, RR spawns 7.86/2.04/0.41/0.083%. Not done: bestiary defeats per
  tier (needs a save field). Main's calls sent with the merge GO: silver restored for Regular/Restless/Gravebound (CORE
  had cut it), Hallowed 1.6x and Ethereal 2.5x Gravebound's; the Legendary rank's tag to the legendary orange #FF9900
  (Hallowed's #F5C842 was close to its #FFCC00). The ward bar (pale cyan over health) added to the staged tag.
  **TRAITS + LOOK merged, built and committed (895c9a4), 767/767**; fixed in play: Phasing never fired (melee creatures
  are always mid-attack: a hard hit now blinks once the attack ends; fences blocked every spot: checks trace 150 cm up),
  the slime's core never lit (no vertex colour: the shell lights the gel round it), flares and rings softened/lifted.
  Pipeline/Enemies.md "Enemy tiers" has every trait and the look. Perf at the start (inside the safe zone): 8 Regular 7.7, Hallowed
  7.8, Ethereal 7.8 ms; fighting in town at (0, -1400): 8 Regular 7.6, 8 Hallowed 7.7, 8 Ethereal 7.7 ms (p95 8.2-8.3;
  Performance.md rows). Not done: bestiary defeats per tier; the dead
  miner's lamp glow unconfirmed at range; the Fleet wind streaks unconfirmed.
- **The user's next pick: "Bestiary tiers and more polish".** Running: BESTIARY-TIERS (Sonnet, owns the editor: defeats
  per tier on each page, a save field); CRANE (coat top through the cape at the neck, coat through sleeves, chain links,
  hem), ODESSA (sleeves into the tails, trousers over the belt crouched, vest past the cravat, card case), FP-ARMS
  (Gauge's arms 17.7k -> ~10k, fingers out of thin grips): Blender, re-export twice, Main reimports. Then on the lane: the
  four new heroes in third person with each gun (hands on grips, Gauge's stock vs his boiler/pauldron), the miner's lamp
  glow and Fleet's streaks.
  BESTIARY-TIERS done and committed (e5c3e9a, 770/770: `DefeatedByTier`, an older save reads as tiers unknown, the page
  line "Restless 3 · Gravebound 1 · ◆ Hallowed 1"). CRANE polish committed (efc17ff: coat top under the cape, chain and
  star on the coat's shell, seat pleats smoothed; the armpit "hits" were the right layering). ODESSA polish committed
  (6988e78): **her coat recut as a square-front swallowtail** (Main's call to accept: it still reads as the picked
  tailcoat; the old cutaway curved from the front and sank the sleeves into the tails), belt and buckle on skin weights,
  cravat over the vest, a slimmer card case; left: the card case 0.8 cm into the thigh crouched, the boot tops show jagged
  holes looking down into them. POLISH-HEROES (owns the editor) reimports both and runs batched capture sheets per hero.
  FP-ARMS committed: SK_FP_Arms_Gauge 10.2k triangles, fingers resting on grips (`fp_arms_gauge_check.py`). **Found, not
  fixed (a shared-helper follow-up):** `looter_bake.bake_set` bakes occlusion at a quarter size but keeps the full
  size's 12 px margin, and Blender applies each object's margin alone, so one part's margin overwrites neighbours'
  islands (black squares) on any set made of several objects; fix there = scale the AO margin (margin x ao_size / size),
  then drop fp_arms_paint's Gauge-only `fix_ao` re-bake; every model's digest changes, so rebake and re-export all, checked
  identical-but-for-the-fix. The next exports of Odessa/Crane/Pike FP arms rebake (fp_arms_paint/heroes are in their
  digests; textures should come out the same).
  POLISH-HEROES done (771/771): every hero's gun sat where the hidden mannequin's shoulders are (their hands are pinned
  to its hands): `UHeroDefinition::ThirdPersonHold` moves the mannequin's hands per hero (Crane +16.5 up, Pike -6.5,
  Odessa -4.5, Gauge 5 ahead 3 out -8; Pipeline/Heroes.md "The third-person hold"; contact sheets `Tools\heroguns.ps1`, the before
  and after in `Saved\Screenshots\HeroGuns`). The Hallowed miner's lamp now has a tier-coloured halo (read white at
  range), Fleet streaks start behind the body, the Bestiary habitat line wraps; Crane, Odessa and SK_FP_Arms_Gauge
  reimported. The editor crashed twice in Python ~30 s after long capture runs (cause unknown; restart after those).
  Left as found (Ellis's too): left palm open under the foregrip, the Bullpup reload tips its stock across the face, the
  Ranchhand reload swings the left arm out, the camera ignores hero size (Crane's hat and Gauge's stacks fill the left of
  the aimed view), red patches on Pike's/Crane's/Odessa's skin (skin material), Crane's coat flares wide sprinting.
- **The user's next pick: "Fix the leftovers"** (2026-10-20), DONE: SKIN (7c0f858: M_CharacterSkin on Preintegrated
  Skin, the blotches were the Separable SSS filter on Medium/High), HANDS (41e6bb6: the third-person left hand grips the
  handguard, pump or six-gun hand), CAMERA (per-hero over-the-shoulder camera, tuned; values in Pipeline/Heroes.md), and the
  hero-swap crash in `Wear` (sections shown before InitAnim). 777/777 before the swap fix; hero tests 10/10 after it.
  The user said skip screenshot capture runs (the editor crashed in python311 ~30 s after them, three times today).
  **Not done: perf of the Preintegrated Skin** (expected neutral or cheaper: no SSS pass; run perf.ps1 Medium with a hero
  when the PC is quiet), the camera with the Ranchhand/Drover and Gauge/Pike at 4x. The user is playing in the editor.
  Not yet done: perf with the PC quiet (heroes, powers, the Barrow fight), the Gravemother decals by eye, the Slag Bomb
  pool's look, Rail Spike's end-on line, Crane's hook swinging in motion, the portrait/model shared-helper merges.
- LINES-ART started in its slot: SM_Headboard_<Hero> x5 (epitaph a parameter), a bare keepers' roll, four poster atlases
  + MI_PosterDecal/Scrap_<Id>, and the `hero_meshes`/`other_mesh` lines in `build_area_story.py` and
  `build_area_missionprops_valley.py` (run by Main after the merge).

**On hold (the user's calls):** the Gilded Lily (when it starts: a World Partition map, Pipeline/Levels.md "Making a level");
Skyreach's own house kit; no style work unless asked.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
