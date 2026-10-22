# Polish round contract (Main, 2026-10-22 evening)

The user: "Keep polishing" (after the next-builds queue: shield 067ceca, makers 067ceca, bounty 811851f, playtest 892506c).
Read CLAUDE.md, Docs/Handoff.md (top: "Next builds"), the job's design note (`Docs/Work/2026-10-22-Next/SHIELD.md`,
`BOUNTY.md`, `MAKERS.md`), Docs/Pipeline/ topic file for your area, CODEMAP.md.

## Rules for every agent
- C++ goes into a holding tree `Saved/<Job>Code/Source/AI_Looter_Shooter/...` (copy each existing file first; `HASHES.txt`
  "hash path" per line with each original's `git hash-object`). Python/Art/recipes/config may be edited in place if no other
  job below owns them. No build, no editor, no commits unless your section says so. Report when ready (under 120 words).
- Every feel number a cvar, plain rules with tests, files under ~500 lines, comments say why, UI only through the LooterUI
  kit and `LooterUI::Color`. Sounds script-made. Original names only.
- Each job owns only its files below; anything else goes in your report for Main.

## DEMO (Opus): plays and judges (owns the editor lane first)
In PIE (`Tools\pie.ps1`, `Looter.PlayLevel`, dev commands; `Tools\input.ps1` with its safety rules: never Ctrl+key or
Delete; single `Tools\grab.ps1` shots, never Python capture batches): (1) a real fight with the Common starter ward at
`Looter.Fight.DamageScale` 1.8: walk into 6-8 spiders and a pack of Unpaid in town, fight with the starter gun; record how
much of ward+health a fight drains, how often the ward breaks, whether it feels fair; try 2.0 too; verdict with numbers.
(2) The maker marks on every gun kind (`Looter.GiveWeapon <Kind> Rare 5 Maker=<Maker>`; Rustler and Thresher were
enlarged unseen; Saint Maud now #9a6a1c), both sides of the gun (a mirrored mark?), first and third person. Tune
`MARKS` in `Tools/Unreal/setup_gun_parts.py` and rerun it if needed (you own that file). (3) Open a bounty Strongbox by
hand. List every bug or ugly thing you see with a grab path. Stop PIE when done; leave the editor open and idle; report
under 150 words. Don't commit.

## SHIELD-POLISH (Opus): the ward's loose ends
Owns: `Bosses/BossLootShower.*` (plan and throw wards: `FLootRoll::Shields`), `Tools/Unreal/create_shields.py` (turn the
boss table on), `UI/HUD/HudScreenEdgeWidget.*` and `UI/HUD/HudPortraitWidget.*` (a hit the ward took whole tints the
edges cyan with no red and no portrait flinch; a breaking hit both; read `UHealthComponent::GetLastHealthLoss` and the
ward component), `Dev/HudShotScene.*` + `Dev/HudShotDevCommands.cpp` (states with a ward: full, chipped, broken, so
`Tools\hudshots.ps1` shows it), `Audio/LooterSoundCues.h` + `Shields/ShieldSounds.h` (move the `Shield.*` names into
`LooterSoundCue::Shield`, keep `ShieldCue` as aliases or replace uses) and the `Bounty.*` names check (BOUNTY added a
`Bounty` namespace: make the bank's check stop warning), tests for the new rules (`Looter.Shields.*`, `Looter.HUD.*`).

## BOUNTY-ART (Sonnet): the board's paper
Owns: `Tools/Blender/looter_posters.py` (a bounty sheet poster cell: "WANTED BY THE RIM RANGERS", the mark's ink silhouette
by creature kind: Unpaid, spider, slime, hound; original wording), the board's sheet materials/meshes if made by script
(`Bounties/BountyBoard.*` C++ only through the holding tree), `UI/World/BountyBoardWidget*` (a BOUNTIES header worthy of
the kit: gunmetal plate, orange rule), `UI/HUD/HudMissionCompleteWidget*` (a "BOUNTY PAID" variant: the purse line,
silver and XP counting up). Previews as PNG in `Saved/ArtPreviews/Bounty/`. Report what Main must import/run.

## PACKAGE-NOISE (Sonnet): a clean packaged log
Read `Saved/Logs/package_2026-10-22_1518_smoke_ransomsrest.log` (packaged game) and fix what's ours: (1) the handled ensure
at startup "Failed to load class /Script/GameFeatures.GameFeatureData for Primary Asset Type GameFeatureData" (find where
that primary asset type is configured: Config/*.ini, plugins in the .uproject; remove or enable properly, saying which and
why); (2) ~20 "Failed to create compute pipeline ... error 80070057" D3D12 errors at startup (PSO precache: find the cause
in UE 5.8 docs/config; a config fix only if it is safe, else a note); (3) any other Error/Warning lines from our code or
content (LogLooter, missing assets, cook warnings in `Saved/Logs/package_2026-10-22_1518.log`). Owns `Config/*.ini` and
`AI_Looter_Shooter.uproject` (edit in place, list each change). Don't package or touch the editor; report the fixes and
what Main should verify in the next package.
