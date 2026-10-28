# Round 1 contract (Main, 2026-10-27): the playtest's bugs and quick asks

Read first: `CLAUDE.md`, `Docs/Work/2026-10-27-Playtest/NOTES.md` (the user's words + calls), `FINDINGS.md` (where each item
lives), the `Docs/Pipeline/<Topic>.md` for your area, `CODEMAP.md` lines for your files.

## Rules for every agent
- C++ goes into a holding tree `Saved/<Job>Code/Source/AI_Looter_Shooter/...` (copy each existing file there first, then edit
  the copy; new files there too). `Saved/<Job>Code/HASHES.txt`: one "hash path" line per copied original (`git hash-object
  <path>`). Python/Art/config/data you own: edit in place. Layout changes: a patch file `Art/Levels/RansomsRest/patches/<job>_<n>.json`
  (format: `Docs/Pipeline/Workflow.md` "Layout patches"; look at `Tools/layout_patch.py` for the schema), never layout.json itself.
- No build, no editor, no Unreal, no git, no commits. Edit with the Edit/Write tools only (no PowerShell -replace).
- Every feel number a cvar or a named constant with a why-comment; plain rules get automation tests (`Source/.../Tests/`,
  `Looter.<Group>`), in your holding tree. Files under ~500 lines; one class per file; every .cpp includes what it uses (no unity
  build). UI only through the LooterUI kit, colors only `LooterUI::Color`, backgrounds call `LooterUI::MarkBackground`.
  Log to `LogLooter`. Original names and text only.
- Own only the files listed for your job. Need something outside them? Put it in your report for Main (exact change).
- Report to Main when done (under 150 words): files changed/new (holding tree + in place), CODEMAP lines to add/change, the
  scripts Main must run and in what order, test names, risks you couldn't check. Also write `Saved/<Job>Code/README.md` with the same.

## Jobs

### HUDLOOT (Opus): notes 8, 19, 26 (map pins), 41
Owns: `UI/HUD/LooterHUD.*`, `UI/HUD/PlayerHUDWidget*.{h,cpp}` (money only), `UI/Inventory/MapPins.*`, `UI/Inventory/MapIcons.*`,
`UI/Inventory/LoadoutStage*`/`StageStudio*` (only if pausing needs it), `UI/World/LootLabel*`, `UI/World/WeaponLabelWidget.*`,
`Inventory/WeaponManagerPickups.cpp`, `Inventory/WeaponManagerComponent.h` (LabelRange only).
- 8: opening the inventory (any page, incl. the map key) pauses the game like the pause menu; closing unpauses. Page sounds,
  the 3D loadout stand-in (it must still render and turn), tab switches, drag/drop, tooltips must keep working while paused. Don't
  pause vendor/bench/boards.
- 19: no money readout on the gameplay HUD (vendor purse stays). Remove it cleanly (no dead code left behind).
- 26: chests never show on the map page (pins, legend). Leave `ChestKinds.cpp` alone (LOOTDROP owns it).
- 41: many guns in one spot must not stack a tower of names: show the focused gun's label plus a few nearest (cvar), the rest
  collapse (e.g. a "+N" cluster or rarity dots). Keep legendary/rare readable.

### VENDOR (Sonnet): note 13
Owns: `UI/Vendor/*`, `Economy/VendorTrade.*`, `Economy/EconomyRules.*` (only if needed), vendor tests.
- A "REFILL ALL AMMO" row at the top of the general store's Buy list showing the total price to fill every stocked ammo type to
  max; one click (with the existing confirm style, like SELL ALL JUNK) spends once and fills all. Greyed with the reason when full
  or not affordable. Price = the sum the player would pay buying box by box (never cheaper exploit, never dearer). Tests.

### SETTINGS (Opus): note 4
Owns: `UI/Menus/SettingsMenu*`.
- Tabs (Graphics, Audio, Interface, Controls) instead of one long scroll; LB/RB and clicks switch tabs; keyboard/controller nav
  rebuilt per tab. Each row has a one- or two-sentence plain description of what it does (what the player sees/feels, and the
  cost for graphics options), shown in a description panel when the row is hovered or focused. Write every description yourself
  from what the code actually does. Concept C look.

### BOUNTY (Opus): notes 40, 22, 12, 33
Owns: `Bounties/*`, `UI/World/BountyBoard*`, `Tools/Unreal/build_area_bounties.py`, bounty tests.
- 40: no chest at the fight; the reward (the tier's loot) is handed over when the bounty is turned in at the board (a strongbox
  that opens at the board, or straight into the loot drop at the player's feet there: pick the one that feels best, say which).
  Remove the "open the strongbox" tracker line and the card's "waits where it fell" words. Old saves with an owed chest must
  still get it.
- 22: before Main 5 the postings must vary: more enemy kinds (spiders, Spitters where the story allows) and more than one
  place (not only the Scatter behind the store). Use places already in the level; list them for Main.
- 12: move the board to the Depot (the train station platform), facing where players arrive.
- 33: bounty fights slightly harder: about +25% enemy health or +1 level on the posse and mark (a cvar). Say which.

### LOOTDROP (Opus): notes 17, 14
Owns: `Loot/LootFanfare*`, `Loot/LootDropMoments.*`, `Loot/LootLibrary.*`, `Loot/ChestKinds.*`, `Loot/LootTable.h`,
`Affixes/WeaponRollLibrary.*`, `Weapons/WeaponDefinition.*`, `Creatures/CreatureRankSettings.*`, `Tools/Unreal/create_rank_assets.py`,
`World/LightBeam.*` (keep other users' behaviour), loot tests.
- 17: a legendary landing must not stutter. Remove the 0.3x time slow (the user reads it as a stutter), stop building heavy things
  on the landing frame (pool/pre-warm the 45 m column and its material, the label widget), and make the landing a "galactic
  explosion": a big bright burst, ring and sparks in the legendary colour, sized to read from 40 m, within the 8.3 ms budget
  (code-drawn FWeaponFX pieces, no lights). Any change to `UI/World/WeaponLabelWidget.*`: ask in your report (HUDLOOT owns it).
- 14: legendaries rarer early: in Ransom's Rest (area levels 1-7) normal enemies and chests almost never drop one (target about
  0.1% per regular kill, under 1% per strongbox, give your numbers); bosses (rank Boss and the named legendaries: Gravemother,
  Foreman, Abel) and bounty marks keep their odds. Gate by the item/area level so later areas restore today's odds. Update the
  odds table in `Docs/Story.md` "Enemy ranks and legendary drops" (you may edit that section). Tests + `Looter.Loot.SimulateDrops`.

### STORYFIX (Sonnet): notes 31, 18, 9, 24
Owns: `Tools/Unreal/story_texts.py`, `Tools/Unreal/create_mission_assets.py` (the Reliquary step only),
`World/RespawnMarker*`, `World/GraveTravel*`, `UI/Inventory/MapPlaces*` (only for the travel list), patch files `storyfix_*.json`.
- 31: taking the eleven's tags (Back Pay, 'Barricade') must not open a reading card; captions and the step still fire.
- 18: the player must know what the Reliquary is: reword the Main 4 objective (e.g. "Examine Saint Ada's smashed reliquary")
  and give Father Aldana one short line that says what it held and why it matters (short; the talk cut rule).
- 9: a wanted poster on Amos's house front wall (`amosHouse`, RR_Home_G at (-7000,1700)) via a layout patch; check the Side 1
  count still works (it needs 4 of N; say the new N).
- 24: fast travel stations: the Depot and Calder's Post only. The Sheriff's Office roost stays a death-respawn point but is not
  a travel destination and not listed on the map's travel list. A flag on the marker; tests.

### WORLDART (Sonnet): notes 11, 30 (water), 36
Owns: `Art/Models/Vegetation/Trees.py` (apples only), `Tools/Blender/looter_plants.py` (only if a stem helper is needed),
`Tools/Unreal/build_world_materials.py` (M_Water only), `World/BossShrine*`, `Session/SessionSubsystemBossShrines.cpp`, shrine tests.
- 11: apples hang on the tree, not floating: on or just inside the leaf shell, each with a short stem to a twig. Same mesh
  names/pivots. Tell Main the export/import commands.
- 30: the water surface must stay visible from below (two-sided M_Water, or a second underside layer: pick the cheaper that looks
  right, slightly darker/tinted underside). No swimming system (out of scope).
- 36: each vigil stone keeps its own countdown: a Save & Quit must not ready every stone at once; each counts from its own boss's
  defeat. Tests.

## Main (editor lane)
Note 20 (Knocking Below Unpaid above the cellar), 30 fences (find them first), 15 stairs (waits for the user's shots), merges,
builds, scripts, imports, tests, commits, then a DEMO play-check.
