# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (2026-10-27): the user's playtest notes
The user played the 10-23 build and sent 41 notes: `Docs/Work/2026-10-27-Playtest/NOTES.md` (word for word + their calls),
`FINDINGS.md` (where each lives), `CONTRACT.md` (round 1). Calls: sniper ground ring dropped; order = round 1 bugs + quick
asks, round 2 fights (bosses, bounties, Unpaid AI, grenades, legendaries, recoil), story/cutscenes/logos after; fast travel =
Depot + Calder's Post only (Sheriff's Office roost kept as save/death-respawn point); legendaries rarer early, bosses and
bounties keep odds; recoil: more kick, a high accuracy stat calms it a lot.
Round 1 DONE and pushed (10843096, fdb7ab58+97223247, 696fea31 + level): cellar Unpaid upstairs (SnapToGround), Refill All
Ammo, roost stations, Amos poster, Back Pay tags no card, Reliquary words, inventory pauses, no HUD money, no chest pins, gun
pile labels fold, settings tabs + descriptions, legendary landing (no slow, pooled, burst), early legendary cap, vigil stones
own countdowns (Save & Quit shortcut gone), apples on stems, M_Water two-sided, bounties pay at the board (+25% health, new
arenas, board at the Depot). DEMO play-check: all PASS.
Round 2 DONE and pushed (`CONTRACT2.md`; user calls in NOTES.md: steps unlock in order + hold, rim shots do nothing, bosses
~1.6x, grenade classes later): bosses (1.6x health, 1.25x hits, Gravemother's stone gate + rim shrug), the Unpaid kit (led
lunge, rake, flank, cinders, rally/frenzy; tuned fair after DEMO), grenades (10% strength to level 3, full at 12; jar 2x;
real explosion), recoil by accuracy + aimed sway, mission gates (step-gated props, 0.6 s hold), polish (map declutter, pile
dots, name pop, settings hover, Ransom's Point arena, Depot roost yaw), a dead player can't fire. DEMO round 2 PASS after
fixes; perf RR spawn 7.8 ms / p95 8.4 (unchanged). Lesson: a gameplay pass right after opening the level found no ground
(now refused: `require_ground`); always read its warning count (0) before committing.
Test build `Builds\Playtest\2026-10-27_1006` (1.27 GB, smoke menu + RR PASS; `Play Latest Build.bat`) has rounds 1-2; the user hasn't played it yet.
Round 3 DONE and pushed (dbf6cfa4 + camp fix + docs; `CONTRACT3.md`): melee tools per hero (user picks: Ellis Grave Spade,
Odessa Razor Fan, Crane Ferry Gaff, Gauge Piston Fist, Pike Rock Pick; /Game/Art/Melee), cast pose, Odessa's card quads, iron
sights, Cyrus as boss two (Dead Lift + Back Pay main; Foreman level 6), Lost and Found, roads (M_Terrain road term, signs,
roads on maps), chest spots, startup logos (placeholder studio; engine card shows a wordmark until the official logo is in
Art/UI/Logos/UnrealEngineLogo.png: the user asked Main to download it; the built-in browser's policy check failed 3x on
2026-10-27, so not yet), Teropa menu (Lvl_MenuTeropa = GameDefaultMap; MenuMap() vs NewGameMap() fallback), new-game intro.
DEMO round 3: all PASS; nits: intro in RR ignores Space (menu says HOLD SPACE), RR intro ends on white without a title card;
Crane's fingers open beside the gaff, Ellis's spade reads like a spear edge-on; cards vanish 1 s after a body dies; road ruts
faint; orchard overhang rock texture smeared. Perf RR 7.8 ms, p95 8.6.
Round 4 DONE and pushed (`CONTRACT4.md`; user picks: melee polish, boss cutscenes, swimming, grenade types Pitch/Scatter/Cinder;
all play-checked, fixes in): per-tool melee sounds/sparks/steam + 3P tool; boss intros (skippable, name cards, player untouchable +
1.5 s, hostiles within 60 m held); swimming (starts at 136 cm water; RR tarn + 6 island waters via build_area_swimwater.py;
underwater tint/fog/mix/bubbles, stroke, breath carries you up, no shooting, creatures stay ashore); grenade kinds (35% special from
level 2, a special swaps your kind and keeps the count). Round 3 nits fixed too. Lesson: a gameplay/swim pass needs the level
open ~30 s first; render every new recipe's cues. Remaining small: spade/pick sparks tuning, chest logs still say grave-salt
grenade, road ruts faint, a smeared rock texture, Unreal logo (browser policy check failing). Test build `Builds\Playtest\2026-10-27_1711` (1.29 GB, smoke menu + RR PASS) has rounds 1-4; the user hasn't played it yet. (A test build with rounds 1-4
was packaging at this writing (newest `Builds\Playtest\`).
Still open from the notes: 15 stairs invisible + 10 terrain + 30 floating fences (need the user's screenshots: which/where;
asked 2026-10-27, no answer yet); later: 1 logos, 2 menu, 3 intro story, boss cutscenes, 32 Cyrus as boss
two, 37 Lost Loot quest, 5 melee tools, 6 Odessa cards, 27 action skill anim, 35 iron sights, 16 roads, 26 chest spots, 25
level scaling (after the Lily). Underwater/swimming effects: none exist (new system; not asked yet).

## State at the session swap (2026-10-27)
Last commit f386c39 (all pushed, tree clean). Test build `Builds\Playtest\2026-10-23_0658` (1.27 GB, smoke menu + RR
PASS; `Play Latest Build.bat`) has everything through 94699b1. **The user hasn't reported playing it yet: ask first.**
Usage note: on 2026-10-23 the account's weekly all-models limit hit 100% (reset 2026-10-27 01:00 UTC); Fable 5.1 has
its own weekly allowance and kept working; agents inherit the main session's model.

Done 2026-10-21 evening to 2026-10-23 (details in git and `Docs/Archive/Handoff_history.md`, section "archived at the
session swap of 2026-10-27"):
- Guns: four new kinds (Rustler SMG, Longhorn sniper, Thresher LMG, Powderkeg launcher); Bullpup/Ranchhand/Drover
  restyled frontier-tech; story elements (Ember, Soul-light, Grave-salt); four makers (Roundhouse, Saint Maud,
  Windfall, Meridian); gun feel pass (recorded reload foley, shotgun shell by shell + rack, slower empty reloads, no
  fast-reload window, kick patterns, impacts per kind, hit markers); muzzle flash reworked (soft burst off the
  crosshair; sparks/tracers as streaks); FP hands framed, pulled in, real grips, sniper/LMG hip lifted.
- Player: the Ward Box shield (rarities, traits, HUD bar, loadout slot, drops incl. bosses; cyan edges on ward hits).
- Enemies: fight director, hit reactions, placeholder clay creatures; Brown Spider 230 HP / bite 11 (Gravemother keeps
  300/12, spiderlings 60); arachnids restyled (legs, fangs, matte fur, darker coat with pale bands); hit flash a rim;
  early fights softened (bluff nest, town gate, Main Street); **creature levels by zone** (they rolled 2-3 everywhere
  since 2026-10-13); fights never spawn nothing; leftovers leave when the story moves on. DamageScale stays 1.8.
- Missions/story: talk cut ~1/3, on the move; main missions start by themselves; side missions gated by story; offers
  accept at once and cut to the accept lines; 10 secrets; giver ?/! marks fixed; bounty board (3 varied postings,
  Strongbox, posters, BOUNTY PAID banner); objectives usable from any reasonable spot (interaction boxes).
- World/UI/sound/tools: per-place ground detail, world variants, thinner tall grass, town roost by the sheriff's office;
  menus hide the tracker, interact grace, loot beams merge, screens close past 4.5 m; three Sonniss impact packs
  (user-approved, licence kept); `Tools\package.ps1 -CloseEditor -SmokeTest`; packaged-log noise fixed.
- Perf after all of it: RR spawn 7.8 ms (p95 8.3), 8 spiders 7.9 (p95 8.4); target 8.3.

Open (ask or do next):
1. The user plays the test build; fix what they find (play-check by an agent after each fix: memory playtest-after-features).
2. Ask: the sniper's ground-impact ring/column failed three fixes (rule 6) â€” keep trying or drop it.
3. Ask: Tilly (a "talking window" by design) â€” a silhouette behind her curtain?
4. Gunplay step 4 still pending: the user tunes `Looter.Gunfeel.*` in PIE, Main bakes their values into the defaults.
5. Small, known: decal mission items (the gate poster) can't wear the highlight overlay (barely visible past 5 m);
   bounty variety after Main 5 is limited (only the Scatter/Unpaid open: proposal Webwood after Main 4 + a spider arena);
   ~66 D3D12 PSO compute errors in the packaged game (RX 580 driver; `r.PSOPrecache.GlobalShaders=0` untried); the
   shotgun's trigger finger can't reach its trigger (below frame at the hip); the rifle's trigger hand stays hidden at the
   hip (bullpup body); third person never shows a gun's maker mark (framing); early fights now end ~66% shield+health
   (aim was 30-60%; the user's playtest decides).

The user's calls this stretch (kept; most also in the pipeline docs): muzzle "Rework it"; spider "Darker coat"; after a
feature, a short agent play-check (no nightly full runs); FP grip modern tactical "but not so much hand showing", hip
"Lift slightly"; early fights "Soften early only"; respawn "Town roost"; sound downloads approved per file (rule: name
file, source, size first); missions flow (verbatim in the archive); spiders softened.

How the work ran (keep doing it): one editor lane; agents write C++ into holding trees `Saved/<Job>Code/` with
`HASHES.txt`; Main checks with `Tools/hashcheck.sh`, copies (the user approved Main doing the copies), builds, runs only
the touched tests, commits and pushes; a DEMO agent play-checks in PIE with single `Tools\grab.ps1` shots (Slomo 0.1 to
catch effects); fixes go back to the agent that knows the area.

## Direction (the user's answers, grill session 2026-10-21) â€” overrides "next" below

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
- **Release plan:** single-player at launch (co-op not planned). 1.0 keeps 15â€“18 regions. A free Steam demo once the
  game is 60â€“70% built, within a year (~Oct 2027). Store page ("Coming Soon") goes up once the core is fun, with a
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
