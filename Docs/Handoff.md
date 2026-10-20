# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Small leftovers", 2026-10-15, local)

**The saga (the user, 2026-10-15 night: a long-running Borderlands-like story; "Why is Ellis here? Where does the story
go? What's the goal? Who's the final boss? What is the world lore?"):** the campaign already existed (Revenant); the user
chose to deepen and scale it (15–18 regions) and seed it into Ransom's Rest. Written: the bible page for the user
(https://claude.ai/artifact/Gr66uUTXWzetBTaHvE9j9Z) and the same canon in `Docs/Story.md`, marked *(saga)*: the
saints as souls who refused to cross, the spark of Ada in Ellis, Sexton's false lighthouse (the Act II turn), the Well on
Skyreach, three new regions (Halfway Junction, Hoarfrost Pass, the Sundown Road), the cloud heroes as companions, and
the list of seeds to build in Ransom's Rest (Story.md, "Seeds in Ransom's Rest"). The user's calls (Decisions 15):
the *Lily* an airship, the false lighthouse, all three new regions, and **five playable heroes picked before the story
starts, like BL2**; a non-Ellis pick **takes Ellis's place** (one story for everyone; heroes not picked don't appear;
Story.md "The heroes"); the pick is built **after the Rest's seeds, before the *Lily***. All calls in. **Next, in
order (the user, revised 2026-10-15 night):** the ember-power system with Ellis's three (Dust Devil, Sundown, Borrowed
Time; one equipped at a time, all three owned from the start; Story.md "The heroes"), then the hero pick (select
screen, four game models, their twelve powers, line variants), the Rest's seeds alongside (Story.md "Seeds in Ransom's
Rest"), then the *Lily* as an airship.
**Overnight run (2026-10-15 night; the user: "Start building. I am going to bed... don't ask anymore until I prompt
again"):** the user's last calls: power key **Q** (rebindable), the HUD socket **beside the weapon column,
bottom-right**, the swap UI **its own inventory page as the second tab** (the Ledger third), and after the powers
**plant the seeds in Ransom's Rest** (looks that need approval are left as look sheets). The app refused to start a separate
session on that turn, so Main builds the powers here, with agents, unattended, recording decisions in this file under
"Decided unattended"; the seeds follow the same way. No questions to the user until they prompt.
**Ember powers round 1: BUILT (89210af, 92cb87e).** Checked in PIE at the Rest's spawn: the socket by the weapon column
(ready ring, running arc, cooldown pie with the count, the denied shake), each power fired on Q and timed by
`Looter.Embers.State`, the Embers page (three cards, E equips). Tests 705/705 (7 new `Looter.Embers.*`); RR spawn 7.7 ms
on Medium (was 7.6; +14 draws, the socket). Not yet: the user's own play; the feel of each power (the dash's knock, the
dusk tint Sundown skipped, Borrowed Time's pay-back) is tuned by numbers in `Tools/Unreal/create_ember_assets.py`.
**The seeds: BUILT (fe2c4df hooks, cc031ac words, 2790b15 props, 4567413 placed, then the map moved).** Pipeline "Seeds
in Ransom's Rest" has every piece. Checked in PIE: the six pages lie where they should (the lockup page read: the card
"A PAGE OF THE LEDGER: THE NEEDLES"), the letter in the rack read, the Seven between Ellis's poster and Calder's note,
the map on the ticket office's wall by the pigeonholes (it first hung over the window: `MAP_ASIDE`), Ruth's page by her
lamp. Look sheets in `Saved/Concepts/Seeds/`. NOT yet seen in play: Hob's three mutters (town-life clock, within 7 m of
his perch), Aldana's Ada talk (after Main 4), the second light at dusk after Main 6 (`Looter.FarLight 1` for a look), a
crossed face (no `Ember.Paid.*` flag is set by anything yet), the Ledger's town pages in the inventory once a page is
read, the cold open's spark. Tests 710/710 (5 new `Looter.Seeds.*`). Perf not remeasured (a dozen small props).
Hob's mutters use the town-life voice cue.
**Where the night stopped / next:** the hero pick (the user's order: select screen, four game models, twelve powers,
line variants). Its first step by the pipeline ("Character portraits") is a painted bust per hero for the user to pick
from, and the paint step runs on Node + Playwright. **The user (2026-10-16 morning): "Install Node and Playwright on
this PC and paint the four heroes' portraits with the existing portrait pipeline."** Done: Node 22.23.3 (the zip from
nodejs.org, unpacked to `%LOCALAPPDATA%\Programs\node`, on the user PATH), `npm install` (playwright ^1.64, `package.json`
at the root, `node_modules/` ignored), `npx playwright install chromium`; Ellis A rendered under Blender 4.4 and painted
here end to end (d20f275). Then (Pipeline "Character portraits", the four heroes' round): `bust.py` split out of
`EllisBlender.py` (3b30400), per-hero frames and `sheet.js --hero` (68ad752), the four hero scripts (2187ba2 Crane,
e7917bb Pike, 08a3677 Gauge, 87b0152 Odessa), `Tools\portraits.ps1`, the aux pass's Raw fix (3d0f206: every earlier
portrait was painted with the ids misread). The four sheets `Saved/ArtPreviews/Portraits/<Hero>/Sheet_<Hero>.png` were
sent to the user. (Paused by the user 2026-10-16 to 2026-10-19 21:00 for usage.) **Resumed 2026-10-19:** all four
repainted with `-AuxOnly` after the Raw fix (that switch had passed Blender `--aux-only` letter by letter, so it
re-rendered the colour pass too; fixed in `portraits.ps1`), the four sheets looked over and sent. **The user's picks
(2026-10-19): Crane A The Marshal, Pike A The Surveyor, Gauge C Company Iron, Odessa A The Dealer** (Story.md "The
heroes"). Then the user's calls: **all four game models at once** (four agents) and **the select screen alongside**
(built now with the painted portraits; models drop in as they land). **Running (2026-10-19 evening):** five agents on
one contract, `Saved/Heroes/CONTRACT.md` (not committed: read it on resume): CRANE, PIKE, GAUGE, ODESSA each write
`Art/Models/Characters/<Hero>.py` -> SK_<Hero> (Ellis.py's way; up to 35k triangles; mannequin bones + twist + swing
chains on the game's prefixes; `<Hero><Set>` texture sets; previews in `Saved/ArtPreviews/Characters/<Hero>/`; export
test into `Intermediate/ArtExport_<Hero>`), SELECT writes the hero select screen and the data-driven hero
(`UHeroDefinition` Role/Story/Color/Portrait/FigureMesh/Retargeter/IsPlayable, DA_Hero_* for five, the twelve powers as
data, `ULooterSessionSave::HeroId`, `StartNewGame(Index, HeroId)`, `Looter.Hero`, tests `Looter.Heroes.*`). Agents
never commit; Main reviews, imports each model, builds its retargeter, checks in PIE, commits. Blender runs queue two at a
time through `artrun.ps1` (Saved\BlenderSlots). Not this round: first-person arms per hero, the HUD portrait per hero,
the twelve powers' code, line variants. **The Gravemother's Barrow fight alongside (the user, 2026-10-19), their calls:
upgrade in place** (charge -> Furrow, brood -> Unearthed, spit -> Grave Clod; Gravequake, roar, staggers stay), **the
headstone breakable** (its own health, not plain armour), **the timings kept** (clod tell 1.2 s, shot-in-grip stun 2 s,
rubble 10 s, furrow tell 2 s, berms 20 s, mounds bulge 1 s), **+25% speed in the last third**. Main's numbers: stone
health 20% of hers, three states (66%/33% of its health), an early break staggers her but doesn't bring the last third.
Three more agents on `Saved/Gravemother/CONTRACT.md` (not committed): BARROW-ART (SK_Gravemother on Spider.py's rig,
the headstone meshes and shards, the fight's earth meshes, FX and decal textures + `build_grave_fight_materials.py`),
BARROW-SOUND (eleven `Boss.Gravemother.*` cues), BARROW-CODE (phase 1 a plan in `Saved/Gravemother/PLAN.md`; phase 2
the C++ once SELECT has finished with the editor). BARROW-SOUND done and committed (aed611d, 13 cues; Main still
imports the bank). BARROW-CODE's plan is in `Saved/Gravemother/PLAN.md`; Main's answers for its phase 2 (send when the
editor is free): the long stagger = her boss stagger 2.5 s, the clod stun the same at 2 s; burning cracks dropped; the
clod's burst on her does no damage; **a popped mound gives the spiderling's kill XP**; **a thin stone bar under her
boss bar** while the stone stands; spiderlings stay the plain spider; tests `Looter.Creatures.Gravemother.*`; small
additive changes to SpiderCreature, BossComponent, BulletSubsystem approved. **SELECT done and committed (e2aaab8)**:
the hero select works (716/716 tests); after each model lands rerun `create_ember_assets.py` (it fills a hero's
portrait/mesh/retargeter fields only once the asset exists). BARROW-CODE phase 2 started (owns the editor and builds
until it reports). A seventh agent, HUD-PORTRAITS, draws the four heroes' inked HUD portraits
(`Art/Icons/HudPortrait_<Hero>.svg` -> `HudPortraitData_<Hero>.inl`, not included yet) and writes
`Saved/HudPortraits/WIRING.md` for Main's C++ hookup once BARROW-CODE is done. Main's own untested piece:
`Tools/Unreal/build_hero_retarget.py <Hero>` (tested on Gauge; it replaced `build_ellis_retarget.py`). **The user (2026-10-19, "what can speed it up"): start the twelve powers now, keep the 8-agent
cap.** An eighth agent, POWERS, writes the twelve powers' C++ into a holding tree `Saved/Powers/Source/...` (not under
Source: BARROW-CODE owns the builds), edits `create_ember_assets.py` and the powers' sound recipes/cues in place, and
writes `Saved/Powers/README.md` (files, run order, compile risks); Main moves the tree into Source and builds once
BARROW-CODE reports. First-person arms and line variants start as agents free up. **The user went to bed
2026-10-20 ~00:00: "No questions until 5am."** Main runs unattended until then, its calls under "Decided unattended
(2026-10-20 night)" below. Clean-up later: the portrait scripts' shared helpers into `bust.py` (`tube`,
`chain`, `turn`, `ramp`, `ball`, `tear`, `point_light`), and the hero model scripts' shared machinery out of Ellis.py.

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
- LINES-ART started in its slot: SM_Headboard_<Hero> x5 (epitaph a parameter), a bare keepers' roll, four poster atlases
  + MI_PosterDecal/Scrap_<Id>, and the `hero_meshes`/`other_mesh` lines in `build_area_story.py` and
  `build_area_missionprops_valley.py` (run by Main after the merge).

### Decided unattended (2026-10-15 night), for the user's review
Main's calls (the agents add theirs below):
- The gamepad's ember key is the left shoulder (the grenade took the right bumper; the key-binding code kept the left
  "for the ember powers to come").
- The HUD socket stands in the Arms row to the LEFT of the weapon slots (bottom-right corner, beside the column).
- The session saves the equipped power's id and its cooldown left; a running power isn't saved (a load finds it ended).
- A new game equips the hero's first power (Dust Devil); the hero defaults to DA_Hero_Ellis when the Blueprint sets none.
- Powers deal damage through UEmberDamageType; a power's kill gives XP and notches no gun.
- Sundown: the world at 0.25 speed for 6 s, the player compensated to 1.0 (their bullets too); Borrowed Time: the debt
  paid back over 3 s after it ends, each kill strikes a quarter off; Dust Devil: untouchable for its 0.3 s.
- The powers page is the inventory's second tab, "Embers"; the Ledger moves to third.

CORE's calls (component, key, save, assets):
- Cooldowns not in the concepts: Sundown 20 s, Borrowed Time 24 s (Dust Devil keeps the demo's 7 s). Dust Devil runs
  as a 0.3 s duration (its dash), not instant.
- A swap mid-cooldown keeps the wait (held to the new power's cooldown), so swapping is never a way round it; no swap
  while a power runs.
- The gate also refuses mid melee strike and mid grenade throw (Busy), as the grenade refuses mid strike.
- A denied press clicks (`Ember.Denied`, half volume) for Cooldown, Active, NoPower and a power's refusal; silent while
  dead or Busy. The component plays it; the HUD only shakes.
- Death or a scene starting cuts a running power short (checked each tick: the header had no death handler to bind).
- `Ember.Ready` plays when a cooldown ends, and when a power ends with its cooldown already over; not after an
  interruption.
- `Looter.Embers.Use` fires past the cooldown through the public API (Interrupt, RestoreEmbers with no cooldown, TryUse),
  so the body's gate (scene, menu, swap) still holds.
- A power with no class, or an abstract one, gets no object and reads NoPower.
- The numbers' fields: Dust Devil Damage 15, Range 900 (dash), Radius 260, Extras FlingMin 400, FlingMax 500, Dizzy 1,
  FovKick 14; Sundown Strength 0.25 (world speed); Borrowed Time Strength 0.25 (share struck off per kill), Extras
  PayBackSeconds 3.
- Tests use a probe power class in its own header (`Tests/EmberProbePower.h`): a UCLASS can't live in a .cpp.

UI's calls (HUD socket, powers page, icons):
- The socket stands 34 px left of the slots (not the slots' 12): a slot in hand lifts 22 px further left and would touch it.
- (Main's follow-up) The socket stands apart from the weapon cluster, so it shows unarmed too, in the same place (a row
  of its own with a spacer for the slots and cartridge); it fades with the cluster while the cluster shows.
- Ready: an orange ring plus a slow glow in the power's colour; running: an arc in its colour shrinking from 12 o'clock;
  cooling: a dark pie drained clockwise with whole seconds rounded up (never "0"); a pop and white ring flash when ready.
- The key tab grows with the key's name (22 px for one letter, 8 px per extra letter, up to 80).
- The page: three big cards side by side (not a list and a details column like the Ledger), since there are only three;
  clicking a card equips it at once; Space and the Interact key equip as well as E/Enter/A; the choice moving plays
  `UI.Hover`, an equip the loadout's `UI.Equip` clack, a refusal `UI.Denied` and a line under the cards.
- Page keys: 1-5 and the shoulders to neighbouring pages on every page (the loadout's LB stays Move, so it has RB only);
  the inventory key closes, as before (the brief said "Tab cycles pages": Tab is the inventory key, which closes).
- The control hints' words follow the new numbers ("Then 3: the Ledger", "Then 4: your missions"); the Map's prompt reads
  "1-5 Pages".
- Icons: Dust Devil three tapered spiral streaks round a ringed eye; Sundown a half sun on a horizon bar with four
  tapered rays and two bands; Borrowed Time an hourglass with a slanted ledger rule through it (no frame posts).
- The colours are the contract's hexes exactly; no `EmberReady` colour (the ready ring is the kit's Accent).

POWERS' calls (the three powers, knocks, sounds):
- Dust Devil goes the way of the keys held, else the way the body moves (over 1.5 m/s), else the way the player looks;
  under 1 m of room (a wall, a drop) the press is refused. It follows the ground (walk-sized rises, 45-degree slopes),
  stops short of walls and drops, never leaves the playable area; from high in the air it dashes level and ends falling.
- The dash is fastest as it leaves and still quick as it arrives; the player walks on at 7 m/s after it.
- Creatures behind a wall from the path aren't struck; a fling is shortened (to 0.75, 0.5, 0.3) to land on ground, else
  the creature only reels. Flings by rank: Rare 85%, Epic 70%, Legendary 45%, a boss none (and no daze).
- Flung creatures cartwheel once in the air and land upright; dizzy shows as three gold sparks circling the head and a
  sway; every knock reels at least 0.2 s so the body settles before the brain takes up again.
- Sundown also keeps the player's controller, camera manager and HUD actor at full speed, re-checks carried guns each
  frame, pitches the world's audio to 0.75, and is refused while time is already slowed (a boss's slow-motion death).
  The dusk tint was skipped (no Grave Sight post-process exists; the UI agent owns any vignette).
- (Main's follow-up, done) Under Sundown the guns fired and reloaded at the world's quarter speed: `AWeaponBase::
  GetOwnerTimeScale` now divides the fire and reload timers' seconds (`WeaponBaseFiring.cpp`, `WeaponBaseReload.cpp`).
  The melee and throw cooldowns still read world time (short; left).
- (Main's fix) The Dust Devil fling is squared to the dash: measured from the nearest path point the way out was a
  diagonal (the test saw a fling more along the path than off it), so the along-path part is removed first.
- Borrowed Time adds the full blow to the debt (a killing blow's too), heals back what it really took, and kills only
  strike the debt while it runs (not during the pay-back); a debt under 0.5 isn't called in; death forgives it; a
  console use during a pay-back carries the rest into the new ledger.
- Sounds: the seven ember cues are 2D; Ready and Denied in the Interface class, the powers in Effects; Ready and
  Sundown.End fade in on purpose. cues.json carries guessed volumes until the render's mix measures them.

The seeds (same night; Pipeline "Seeds in Ransom's Rest" has the how). Main's calls: the seven towns = Ransom's Rest,
Highwater Landing, Dustwater Mesa, Saint Agnes Mire, Furnace Hollow, The Needles, Stoneweight Quarry (six pages on the
Rest, the Quarry's found later); flags `Ledger.Page.<Town>`, `Letter.Deacon.Cleft/Saloon`, `Ada.Told`; one new model each
for the page, the letter, the post rack and the map; the Seven poster as an atlas cell. The agents' calls:
- CORE: the X on a face is a second small decal (`SevenCross` cell), not a material mask; the steady light is a mode of
  `AFarSignalLight`, not a new class; no StoryLighting change (dusk already holds through Main 6 and 7); the second
  light isn't gated on Grave Sight (no vision mode exists); a topic's flag is set only when that topic was said; the
  spark only when the scene isn't skipped.
- DATA: the town pages are STORY_CHARACTER pages in the NPC section ("???" until found; `Looter.Ledger` wants exactly
  seven LedgerName pages); the saints Hester (Highwater Landing), Ezra (Dustwater), Agnes (the Mire), Maud (Furnace
  Hollow, the smith), Corliss (the Needles), Gideon (Stoneweight), Ada; robbery dates 3 Mar, 1 Apr, 9 May, 14 Jun, 12
  Jul, 2 Aug, the Rest 21 Aug (its page entered 21 July, "E. Ransom" twice); the saloon letter is the Deacon warning
  Delia (postmarked Halfway Junction, 9 Aug, never collected), the cleft letter to "Kid", never sent; Hob says the Point
  line on his after-Main6 bier perch; the second light due west of the Point, 9 km out, 30 m up, size 5500, strength 30;
  the Seven poster at offset [0, -12, 0]; the rack 1.4 m up, 110 cm right of the saloon doors; the lid's lines say
  "prised up at one edge" to match the model. Open: the town pages' "not met" help reads "You two haven't met, friend"
  (a place wording needs C++); Hob's mutters play the town-life voice cue; Sloane's "kid" line left as it was.
- ART: one letter model (sealed, so "never sent" and "unopened" both fit) and one page model for all; the rack has two
  old letters baked into slots 6 and 8; the map's layout (west line splits for Highwater Landing and Dustwater Mesa
  then runs to the Rest; north the Needles, north-east the Quarry, east Furnace Hollow with a branch to the Mire) and
  "W. PIKE, SURVEYOR" (Wendell Pike; drop it if unwanted); poster wording "THE SEVEN / OF THE DUNNE GANG", first names,
  "DEAD OR ALIVE", "CAPT. R. Calder".

Done this session (the user picked "small leftovers" first): `WeaponManagerComponent.h` split (its events and small
types in `Inventory/WeaponManagerTypes.h`, 491 lines); the editor-exit ensure on Hiram's figure fixed in all six
poseable-mesh constructors (Pipeline, Bugs); the shotgun reworked with the user watching: held 20 cm nearer, the left hand
cupping the pump with a new `EHandGrip::Pump`, the thumb along the pump (the user approved the close-up); live switches
for the support hand's finger and thumb curl; `play.py equip <n>`. Many other files are still over ~500 lines
(`CreatureBase.h` 562, `BossShrine.cpp` 568, `LooterUIStyle.cpp` 765, `LooterSoundCues.h` 847, several tests).

**Ellis is the player's body (the user's ask mid-session, done: 116316b, fdd3894):** the cloud branch merged (ffb56c2),
SK_Ellis built and baked here, imported; the mannequin stays the animated body, hidden, and Ellis copies it through
RTG_MannyToEllis (Pipeline, "Character models", "Ellis as the player's body"). The user's calls: MakeHuman's base.obj
fetched here; "retarget live" (not every clip retargeted onto him). Checked in PIE: third person from behind, standing
and running; front view aiming the rifle and the revolver (hands on the guns); first person (he's invisible to the
player). Tests 687/687; RR spawn 7.5 ms on Medium (was 7.7). The user saw the first two shots; not yet played.

**Ellis quality loop (the user, 2026-10-15: "it looks plastic, stiff and unlike a real fps character. Improve it until
I tell you stop and run all decisions by me"):** the user's picks: holster empty only while the revolver is drawn;
first-person gloves and sleeves match him; all four rounds (materials, coat and hair that move, pose and hands, remodel
the silhouette); budget raised to ~35k triangles. Done: round 1 materials (approved; 3512eba), the holster, FP arms matched
(17e2348), round 2 (39683fe, b26dbf0): swing bones + springs, the hat tipping back when aiming plus a 12% smaller brim
(the user's "both"), hands 15% thicker, the coat and cape looser, the bandana off his neck, facing +Y (the fingers now
auto aligned: no claws). Round 3 (an audit in Blender, `Saved/Concepts/EllisAudit/defects.md`): flipped faces (the
waistcoat's holes), the cape's wings and fin, gauntlets, collar, lantern hook, holster; hands kept out of the coat
(f9be3bf). Round 4, the user's picks: **"Apply C in full"** (squarer shoulders, pointed cape back, deeper hat crease, harder
brim curl, the coat to the ankle), **proper boots** (stovepipe on a real last), **face kept dark**, **the waistcoat as
cloth** (smooth black wool, brass buttons), **the coat's back hanging straight** (the user: "Back of the cape is deforming
in between his legs. It should be straight": the cloth simulated over a filler behind the legs, the vent closed). 34.2k
triangles, 113 bones (each swing chain ends in an unweighted `<chain>_end`; a 7th skirt chain `skirt_bm` down the vent).
The springs: the bone between two points pushed out the body's side of a limb, the points kept over the ground with the
hem gathering there, nothing rising over what it hangs from (Pipeline, "Swing"). Audit: the stride's boot through the
coat 53 -> 34 points, the crouch's hem under the ground 11.7 cm -> 4.9 cm. Checked in PIE: back standing, walking,
sprinting; crouched front and back. Shots sent to the user (round 4 before/after, in game).
**Next in the loop (ask first, tap-to-answer):** the pose round (idle weight shift, stance). Still open: a full stride
still shows the shin below the knee through the coat's back (34 points) and the left hand sits 4.9 cm into the coat's
front in it; the cape's corners lift with the arms when sprinting (CapeFollowFront/Back); the hair reads as ribbons up
close; the crouch's holster and lantern touch the thigh; the collar's turned-up corner reads like a horn from below; FBX
import warns "Not valid bind pose" (recreated fine); the spin viewer artifact predates the bake here.

**After round 4 (the user's notes, 2026-10-15):** "When crouching should his knee be touching the ground?" answered:
no, the crouch is a squat on both feet (code pose, LooterStancePose); the idle stance's right foot is back so that knee
comes down to about 20 cm (the left 34). "cape still clips through the boots and shins": foot limbs and the ring
(Pipeline, "Swing"). "His wrists are either invisible or don't match the gloves": forearm twist bones + FBodyForearmTwist
(Pipeline, "The forearms"), the glove's cuff up into the gauntlet, the sleeve stopped inside it (round 4g; 117 bones,
34.3k triangles); checked in PIE, before/after sent. Left: a small crease at the right gauntlet's lip by the thumb when
the hand is turned and bent.

**The Gravemother fight concepts (the user, mid-session 2026-10-15):** "generate a sprite sheet for the grave mother
boss ... projectiles, the spiders ground charge, the spider itself, attack particles, etc. ... give it some character and
feel like a real boss fight. Just generate 3 to 5 concept arts for now, we will turn them into game assets once I choose
the ones I like." Four sheets made and sent (`Art/Backlog/Creatures/GravemotherFightConcepts.py`, sheets in `Saved/Concepts/Gravemother/`
by `GravemotherFightSheets.ps1`): A Bone Mother (brood riding her, milk spit, grave plough), B Vigil (chapel candles as
her phase meter, corpse-lights, a procession of ghost-flames), C Shroud (ivory and oxblood, burial silk, net shot,
drag-lines, egg sacs), D Barrow (the bold one: a headstone shell, grave clods, a furrow charge that leaves cover,
unearthed spiderlings). **The user picked D, Barrow (2026-10-15)**, to build after the Ellis work they asked for first.
The user's review ask ("Make it better"): the real sprite sheets are made and sent (`Art/Backlog/Creatures/
GravemotherBarrowSheet.py` + `GravemotherBarrowSheets.ps1`; `Saved/Concepts/Gravemother/Barrow_{Moves,FX,Hero,
Spiderling}.png`; renders and `BW.json` in `Saved/ArtPreviews/RansomsRest/Concepts/GravemotherBarrow/`): every move
as 6 timed frames, the FX pieces isolated with alpha and sizes in cm (clod x3 sizes, dust burst cards at 0.1/0.4/1.0 s,
debris x5, rubble, the heave crack strip + end cap, dust jets, berm segment, mound closed/burst, stone chips, sparks,
stun stars; plus a landing-shadow decal, a furrow decal, fresh-earth dust #806B55 vs pale floor dust), her turnaround,
the spiderling (135 x 132 x 57 cm). The headstone is grey granite (#8E8B86), not sandstone (it vanished against the
Sink's wall). Not numbered yet: her faster run after 33%. Design calls made by Main for it (the user
hasn't confirmed them; ask if they object): the berms stay 20 s then crumble; the headstone is armour that deflects
(sparks, chips; no health), cracking off at 33% to bare the Bone abdomen; crits head + the abdomen's soft rear, the
whole abdomen after 33%; Grave Clod 1.2 s tell, shot in her grip = burst on her + 2 s stun, a 1.5 s lob to 20 m, a
rubble pile 10 s; Furrow 2 s heave tell then the plough; 4 spiderlings at 66% and 33% from mounds that bulge 1 s
(shootable).

**The user's next picks (2026-10-15, tap answers):** "Both 1 and 2": Ellis's leftovers (all four: the shin through the
coat's back, the cape lifting when sprinting, the hair reading as ribbons, the crouch's holster and lantern) and the pose
and idle round. Idle: **"Weary revenant"** with **breathing, glances around, hand fidget** (not the hat touch); the armed
stance **kept as it is**. Built: the idle (LooterStancePoseIdle.cpp; Pipeline, "The idle"), the cape's corners following
an eased arm (BodySwing, `FollowEaseSeconds`), the ring order for two new back chains, the lantern's `Bulk` (2.5 cm
more room off the limbs); round 5 in Ellis.py: `skirt_bli`/`skirt_bri` (a fifth of the way from each back chain to bm;
the stride's shin through the coat 28 points -> 0), hair as closed lens-shaped locks in two layers (5.8k triangles; still a
little chunky up close from the back), the holster pouch on the left thigh (crouch contact 3.6 -> 2.2 cm; the gun now
grazes the trousers 0.9 cm in a stride), the gauntlet lip rolled out; 125 bones, 34.9k triangles. Checked in PIE: idle,
sprint from behind, hair, crouch front and right side. Shots sent. Tests 698/698.
**Round 6 (the user on round 5's shots, 2026-10-15: "Cape clippings, hair looks like mud, can see through the
head/hat, hat doesn't seat on his head properly, don't stop until you get it right"; then "Run multiple agents if you
need to"):** three Blender agents on one contract (`Saved/Concepts/EllisRound6/CONTRACT.md`): HAT (seat the band on
the skull, close the crown's inside and the scalp), HAIR (cards with a script-made strand texture, masked; maybe a new
M_CharacterHair master), CAPE (cape and collar clipping in every pose). Found: every Ellis material is one-sided, so
a single surface seen from behind vanishes (the crown's inside). In-game evidence: `Saved/Screenshots/Tools/aud_*.png`.
**Round 6 done (2026-10-15 night):** HAT seated the hat and closed the head (`ellis_hat.py`); HAIR made cards on a
script-made strand atlas with a new master `M_CharacterHair` (`ellis_hair.py`, `Art/Textures/EllisHair`); CAPE rebuilt
the cape's weights and chains (9, hung from the shoulder joint, `cape_arm_<side>` idle arms) and lined the collar
(`ellis_cape_audit.py`), with BodySwing's follow shares, idle-arm rests, rising chains and cape bulk 6 to match
(Pipeline, "Swing"). 142 bones, 33.2k triangles. Checked in PIE from 20 head/body angles plus carry, aim (5 angles) and
sprint: no see-through, the hat on the head, hair reads as hair, no cape in the sleeves. The hair glowed light brown
backlit: MI_EllisHair Tint 0.55 (also in Ellis.py's EllisHair slot so a re-import keeps it). Shots sent. Tests
698/698; perf at the RR spawn 7.6 ms on Medium (as round 2; game thread 3.7 -> 3.9 ms for the bones and chains, GPU
unchanged). Committed 0ad1801. Cleanup later: `Saved/Concepts/EllisRound6/cape/wip` (1.7 GB of the agent's trials).
Main imports, checks in PIE, commits. Then Gravemother. Found on the way (HAT): the game skin was a "crystal":
`make_lows` spared everything within 3 cm of the eyes, so the skull's back and top collapsed into ~12 triangles (the
holes); now decimated by region. Leftover for later: BandanaKnot sits inside the neck (mid-neck, not at its back) with
36 points of the bandana's lower side edge: invisible, wasted triangles; move the knot to the nape or drop it.
**Next:** the user's notes on round 6; then the Gravemother, Barrow (D) as the real fight (concepts first done: model,
projectile, charge, FX, sounds; ask the user the open calls with tap-to-answer choices).

## Before that (session "Ellis 3D model", 2026-10-15, cloud; branch `claude/ellis-3d-character-model-tvwr9z`, merged)

**The request:** turn Ellis portrait A into the game's 3D model, realistic enough for the game, textured from boots to head,
and something the user can spin round. Midway the user: "I'm liking the look so far, but his texturing looks too smooth ...
More realistic modeling and less smooth looking." (done: cloth-simulated duster, creases, seams, stitching, grain, wear).
**Built:** `Art/Models/Characters/Ellis.py` -> `SK_Ellis` (Pipeline, "Character models"): MakeHuman's body shaped as the
portrait; a cloth-simulated duster to mid-calf (back vent, seams, topstitched hems), fluted cape, rolled turned-up collar
with the sun-ring pin; waistcoat, work shirt, trousers tucked into stovepipe boots with stacked soles and cuban heels; roper
gloves with gauntlets and conchos; cartridge belt, Mexican-loop cross-draw holster with the revolver, Pa's lantern burning
at the right hip; hat, hair, pointed bandana, glowing slit eyes. 63 mannequin bones, four 2048 texture sets baked from
procedural looks, hit zones, LODs 50/25; ~20.6k triangles (the user asked for Borderlands' 10-20k: the ~108k detailed build
is now only the bake source, its folds and creases baked into the game mesh's maps). Helpers `looter_human`, `looter_garments`, `looter_looks`,
`looter_bake`. Merged `claude/ellis-portraits` (portraits, MakeHuman assets) into this branch. The spin viewer: an artifact
page (three.js; Spin, Look at Head/Hands/Belt/Boots, Dusk/Day light) with the posed, textured glTF: https://claude.ai/artifact/HJCgTaXnSRbEb5k7SXjdDa (source `Tools/CharacterViewer/`).
**Not done (needs the PC):** the textures aren't committed (LFS): `Tools\models.ps1 -Only Ellis` bakes them (minutes), then
commit `Art/Textures/Ellis*`; the script was run only with Blender 5.2's Python module here, not the PC's Blender (watch
for API differences on the first run); the IK retargeter from the mannequin; SK_Ellis on the player in place of the white
mannequin (third person); the first-person arms against it (SK_FP_Arms' sleeves are #8f7052, lighter than the model's
dust-brown oilskin, and its gloves #a47c56 lighter than the model's: the user's eye decides which wins).
**The user's decisions (keep):** as the portraits session's; Ellis A is the spec; less smooth (folds, creases, seams, wear);
a game budget of about 20k triangles (picked from ~20k / ~35k / keep 108k, 2026-10-15).
**Next:** the user's notes on the viewer; then on the PC: import, retarget, wire SK_Ellis into third person, check the hands;
then the other four heroes' portraits and models the same way.

## Before that (session "Ellis portraits", 2026-10-15, cloud; branch `claude/ellis-portraits`, merged into the above)

**The request:** the heroes' models "suck"; first paint portraits like the user's reference (richly framed painted busts),
pick one, then build the 3D model from it. Ellis only for now, three portraits.
**Built:** `Art/Portraits/` (Pipeline, "Character portraits"): Ellis A The Revenant, B Came Home at the Last, C Dressed for
the Grave, rendered in Blender from MakeHuman's CC0 base, painted and framed; the frame generator; `sheet.js`. The
images aren't committed (PNGs go to Git LFS, which the cloud box can't push): remake them with the Pipeline's commands
(~8 min for all three).
**The user's decisions (keep):** Blender's `bpy` may be installed in the cloud box; MakeHuman's CC0 assets may be used
(`Art/ThirdParty/MakeHuman`); the code-only SDF busts were passed over for Blender; **Ellis is option A, The Revenant**
(picked 2026-10-15): cattleman hat low, red bandana up, cyan eyes burning as slits in the brim's shadow, collar-length dark
hair, a dust-brown duster with a fluted shoulder cape and the collar turned up, Saint Ada's sun-ring pin on the collar, a
black waistcoat with brass buttons.
**Next then:** build Ellis A as the in-game model (done in the session above).


## Before that (end of session "Polish: make it alive", 2026-10-14)

All committed and pushed (HEAD 8a005cb). Tests 687/687 at 57264b8. That session's full log:
`Docs/Archive/Handoff_history.md` (last section). How each piece works: `Docs/Pipeline.md`; perf numbers:
`Docs/Performance.md`; release-only work: `Docs/Release.md`.

**Built this session:** the old handoff items; world life (lamps flicker, window life, ambient air, gusts), the cliff and
rock review, cliff life, the town's dressing; two crow stations a map; creatures react; guns that feel physical; small
animals; rewards you can see; the user's seven HUD/control items; High/Epic fixed (small scatter out of Lumen, High
without Lumen); spider deaths (legs curl under); clear teal water and waterfalls; the third-person carry; first-person
hands (SK_FP_Arms, Ellis's gloves and sleeves) with raised, level holds and hands at 0.88; guns drawn bigger (rifle and
shotgun 1.1, revolver 1.2); the pistol named "Revolver"; swap animations with holster sounds.

**The user's decisions this session (keep):** Lost Property shelves 300/600/1000/1600/2400; third person keeps its own
FOV; endgame level scaling decided later; two crow stations per map (RR: the Depot and Calder's Post), a death wakes at
the last one activated; animals small only (hens, cat, swallows; no mule); Medium stays as it is (no grass trade-off),
new additions save their own cost; no optimization work now (smoothness waits in `Docs/Release.md`); the Gilded Lily
starts as a World Partition map; water clear teal; first-person hands the full Borderlands way, guns raised so both
gloves show, level (no forward tilt); Ellis's own third-person body LATER (the user asked for it on 2026-10-15: Now).

**Waiting on the user:** a playtest of the hands after d0d9d2e. Not seen in PIE: the support hand during reloads (tests
cover the paths) and the hands while aiming down sights.

## Next (in order)

1. The user's playtest notes first (Ellis in third person, the shotgun's new hold, hands, guns, swaps, water, spiders).
2. Small leftovers: the revolver reads tipped up at the hip (perspective; tilting it up looked worse); the rifle's and
   revolver's support hands weren't looked at again after the shotgun's rework (same kind of check if the user asks).
3. Keep polishing until the user says stop: nothing major, concepts before art, ask with tap-to-answer choices.
4. **Older open items:** Story.md's later areas' level bands predate the new curve (retune when the next area is
   designed); the user hasn't heard the newest sounds (republish the listening page); the slide is a code-drawn pose;
   from the cloud handoff, the ember powers and heroes aren't built (major: only if the user asks); `area_faces.fields`
   gives the East Shoulder's face ground ~748 m^2 inside the boundary where the ridge doesn't stand (Stage Gap's depot
   side; changes the macro maps: needs the user's eye).

**On hold (the user's calls):** the Gilded Lily (when it starts: a World Partition map, Pipeline "Making a level");
Skyreach's own house kit; no style work unless asked.

## Sessions

- Main's agents model in Blender this session (the AILS Artwork session isn't running); Main imports and commits.
- The session "Polish: make it alive" ended 2026-10-14 at the user's call; the next session picks up here.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
