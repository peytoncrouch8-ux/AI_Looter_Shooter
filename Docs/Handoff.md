# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

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
2. Ask: the sniper's ground-impact ring/column failed three fixes (rule 6) — keep trying or drop it.
3. Ask: Tilly (a "talking window" by design) — a silhouette behind her curtain?
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
