# Core gameplay job (2026-10-28 late, overnight until 6am 2026-10-29)

The user's picks (all four): guns feel different, enemy hit reactions, smarter bigger fights, things to find off the path.
Plans: GUNS.md, HITS.md, FIGHTS.md, FINDS.md (Opus planning agents). This file: the user's answers and the merged lanes.

## The user's answers before bed (2026-10-28)
- Overnight: build all four with Main's best calls; list every call in Handoff "Decided unattended" to undo in the morning.
- Difficulty: clearly harder: careless play gets you killed, standing still is punished, an elite in most fights (close to a
  Borderlands first playthrough).
- Finds pay a mix: good loot (better chests, rare guns), lore and XP, a few unique named guns for the best-hidden ones.
- Gun sounds: script-made now (each kind distinct); a royalty-free pack shown in the morning for approval before any download.
- Main's default (not asked): hit reactions stylized, no blood (ember, soul-light wisps, goo, dust).

## Main's calls overnight (the plans' questions; the user's answer in the morning overrides)
- GUNS Q1 keep the view-kick caps (3 deg, 5% FOV, push 4 cm), separate heavy guns by shape; Q2 hip move speed +5% SMG and
  revolver, -5% LMG and launcher, -3% sniper; Q3 show another year's Sonniss GDC bundle in the morning (no download).
- GUNS lane F (crosshair per kind): mockup only overnight, built after the user sees it.
- HITS Q1 spider leg shots are looks only (buckle, limp); Q2 Unpaid hats knocked off by hard head hits and head kills, lie
  6 s; Q3 strength as planned (big and readable at 25 m).
- FIGHTS Q1 the spiders stay as they are (the user's 10-28 call "leave them"): lane 3 BEASTS (pounce, hit-and-run, Spitter
  flush) not built; spiders still get HITS' reactions (looks only). Q2 arena walls at the chapel yard only (Main 4 set
  piece). Q3 the user's "clearly harder" = Borderlands-normal: standing still dies in 8-12 s, a moving player ends a
  normal fight at 30-70% health, Basic kills 1.6-2.5 s; DEMO measures a baseline first.

- FINDS Q1 the map shows the total and per-region counts; Q2 unfound finds' tell: two low circling crows plus a glint;
  Q3 the Tollman stays a find (a wandering Gravebound elite, respawns 20 min after a kill).
- Baseline perf (2026-10-28 22:35, tag card in): RR Medium 6.2 ms (p95 7.6); 8 Unpaid horde 7.8 ms (p95 11.4).

## Round 1 (8 agents, parallel; no agent builds, opens the editor, runs tests, commits, or edits CODEMAP.md / Docs:
## each reports its new files with one-line descriptions and Main writes CODEMAP and the docs)
1. GUNS A KICK (Opus): GUNS.md lane A files only.
2. GUNS B HEFT (Sonnet): GUNS.md lane B files only.
3. GUNS C+E SOUND-CODE, BRASS, RELOAD BEATS (Sonnet): GUNS.md lanes C and E files only.
4. SOUND-ART (Opus): GUNS.md lane D (Art/Sounds recipes guns_*.py, cues.json, check.py, README) plus HITS' ten cues in a
   new Art/Sounds/recipes/hit_feel.py (the names in HITS.md "Cues"); the only agent that edits cues.json this round.
   Renders with Tools\sounds.ps1 (no Unreal). Never edits Source/.
5. HITS A core (Opus): HITS.md lane A files only (it owns the contract API in HITS.md "Contract API").
6. HITS B spiders (Opus): HITS.md lane B files only; looks only (no slow).
7. HITS C Unpaid (Opus): HITS.md lane C files only.
8. HITS D slime, flash, FX (Sonnet): HITS.md lane D files except the sound recipe (agent 4 makes it); owns
   Creatures/CreatureHitCues.h.
## Round 2 (after round 1 builds and passes): FIGHTS Phase 0 (Main), lanes 1 MEASURE, 2 DIRECTOR, 4 SLIME, 5 UNPAID,
## 6 ENCOUNTERS, 7 SOUNDS (fight_kit, by the sound agent), 8 DEMO; FINDS lanes; GUNS lane F after the user's look.

## Round 1 result (2026-10-29 ~01:00)
Built; tests Gunfeel 69/69, Feedback, Weapons.FX, NewGuns, Unpaid.Kit, Fight pass; fixes in progress for Weapons.Aim.CrouchAligned
(heft push held while a slide tips the view), Audio.BankCoversCues (no SMG Punch cue), Hits.Unpaid.Hat, Anim.SpiderDeathCurl,
React.Corpses, Vfx.Hits.Budget (a fix agent).

## Round 2 lanes (same rules as round 1; FIGHTS.md and FINDS.md are the specs)
- F1 MEASURE (Sonnet): FIGHTS lane 1 files.
- F2 DIRECTOR (Opus): FIGHTS Phase 0 first (Creatures/FightEvent.h, FightDirectorSubsystem.h declarations, the ACreatureBase
  virtuals GetMeleeGap / StartsRingMove / OnTurnPlayedOut / OnPackLeaderDown and their two call sites, FEncounterGroup::bLeader),
  then FIGHTS lane 2. Spiders: GetMeleeGap 0 (the user's "leave them"); slime 0.45; Unpaid 0.
- F4 SLIME (Sonnet): FIGHTS lane 4 files.
- F6 ENCOUNTERS (Sonnet): FIGHTS lane 6 files, plus FINDS lane E's two encounter entries (MiserPlot ambush, the Tollman's
  patrol) in patches/fights.json and their test; the Pacing test updates. No spider count changes beyond FIGHTS.md's table
  where they only add a leader mark (counts at the Sink stay as today: the user's "leave them").
- N1 FINDS A core (Opus), N2 FINDS B UI and sound (Sonnet; also FIGHTS lane 7's fight_kit.py recipes; the only cues.json
  editor), N3 FINDS C level build (Opus).
- Later this round: F5 UNPAID (after the fix agent), N4 FINDS D words and named guns.
