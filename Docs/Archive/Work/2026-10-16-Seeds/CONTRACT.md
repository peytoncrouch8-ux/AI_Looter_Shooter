# Seeds in Ransom's Rest: the build contract (2026-10-16, overnight, unattended)

The seeds: `Docs/Story.md` "Seeds in Ransom's Rest" (read it first; keep Fenn's fare as it is). Rules: `CLAUDE.md`.
How the level is built: `Docs/Pipeline.md` "Making a level (grounded areas)" (line ~215; the gameplay hooks ~288; the
rerun rule: `build_area.py` clears the outliner folder it owns by tag before placing again). Nobody asks the user
anything tonight: decide, and write every decision into your report so Main records it in `Docs/Handoff.md`
"Decided unattended". Nobody opens or drives the Unreal editor: Main rebuilds, imports and runs the scripts. Blender runs
through `Tools\artrun.ps1` and exports with `Tools\models.ps1 -NoImport`.

## The facts every piece shares

- **The seven towns** (the Reliquaries the gang robbed; one page each; Main's call from the Areas table): Ransom's Rest
  (Saint Ada; robbed last, by the Deacon; the cold open), Highwater Landing (the *Lily*'s rim boomtown; Ned's ember),
  Dustwater Mesa (Ira), Saint Agnes Mire (Constance), Furnace Hollow (Barrels; its saint a smith, the Reliquary cast
  iron), The Needles (Lena), Stoneweight Quarry (Tobias). Six pages are in Ransom's Rest; the seventh (Stoneweight
  Quarry) is found later in the campaign, so its Ledger page stays unfound here.
- **Story flags** (`UMissionRunner::SetFlag`, `FStoryCondition.WithFlags/WithoutFlags`; `with_flags=[...]` in the Python
  condition dicts): `Ledger.Page.RansomsRest`, `Ledger.Page.HighwaterLanding`, `Ledger.Page.DustwaterMesa`,
  `Ledger.Page.SaintAgnesMire`, `Ledger.Page.FurnaceHollow`, `Ledger.Page.TheNeedles`, `Ledger.Page.StoneweightQuarry`;
  `Letter.Deacon.Cleft`, `Letter.Deacon.Saloon`; `Ada.Told` (Aldana's one talk given).
- **Readable ids** (`AReadableProp.ReadableId`, tags `Readable_<Id>`, event `Readable.<Id>`): `LedgerPage_RansomsRest`,
  `LedgerPage_HighwaterLanding`, `LedgerPage_DustwaterMesa`, `LedgerPage_SaintAgnesMire`, `LedgerPage_FurnaceHollow`,
  `LedgerPage_TheNeedles`; `DeaconLetter_Cleft`, `DeaconLetter_Saloon`; `ReliquaryLid`.
- **Where the six pages lie** (one each): the sheriff's lockup (the cell, on the bunk or the floor by the crank; the
  sheriff's frame and helpers are in `build_area_missionprops_valley.py` `place_sheriff`) -> `TheNeedles`; the depot's
  baggage room (on a trunk; `SOCKET_BaggageCoffin`, the trunks near it) -> `HighwaterLanding`; the vestry door's step
  (no interior; outside Aldana's door, `SOCKET_Speaker` of the chapel, on the sill or step) -> `RansomsRest`; Calder's
  Post (inside Ruth's cabin, on the table or shelf; `RR_RangerCabin` sockets) -> `DustwaterMesa`; the Providence's tag
  board (pinned on the board, `on_board` in `build_area_providence.py`) -> `FurnaceHollow`; the gang's mooring cleft
  (by the `mooringPost` placement, on the ledge) -> `SaintAgnesMire`.
- **Line sets** (`/Game/Data/Story/DA_Lines_*`, made by `Tools/Unreal/create_story_lines.py` SETS): `DA_Lines_HobIdle`
  (the three feather lines, one set, in the Story's order), `DA_Lines_AldanaAda` (What Ada gave up), lines appended to
  `DA_Lines_AgentWindowLily` (the Junction and Highwater Landing; also in the valley script's `LINES` fallback),
  `DA_Lines_HobMain6Point` or the Point perch's set: add Hob's *"Don't look at that one, sunshine."* as its LAST line.
- **Ledger pages** (`Tools/Unreal/create_bestiary_pages.py` PAGES; `UBestiaryEntry`): seven new `LEDGER_NAME`-style
  town pages, `bLedgerOnly`, section `PLACE` if the enum has one (else the nearest; report it), assets
  `DA_Bestiary_Town_<Town>`, `found_when=dict(with_flags=['Ledger.Page.<Town>'])`, sorted after the seven names.
  Each page's description is the page's own text (the lore), its notes the place the page was found.
- **Models** (`Art/Models/Props/`, imported to `/Game/Art/Props/`; prefixes per CLAUDE.md): `SM_LedgerPage` (a loose
  ledger leaf, ruled, ~20 x 32 cm, one corner lifted, Sexton's copperplate as scrawl ribbons; material slot
  `SextonLedgerPages`, the shared MI the slip uses), `SM_DeaconLetter` (a folded letter, cheap paper, ~11 x 18 cm,
  reuse the slip's builder with a different fold), `SM_PostRack` (the saloon's post rack: a slotted pine letter rack,
  ~60 x 40 cm, 8 slots, nailed to the saloon's boarded front beside the doors; a letter standing in one slot is the
  readable), `SM_ReachesMap` (the depot's wall map: a pinned paper sheet ~90 x 60 cm, the lines inked, seven towns and
  Halfway Junction named in Rye; texture painted by script into `Art/Textures/ReachesMap/`), the Seven poster: a new
  atlas cell in `Tools/Blender/looter_posters.py` (see the ART owner).
- **Sounds:** none new. Reading plays `LooterSoundCue::MissionFeatures::Read` as every readable does.
- **Colours and UI:** nothing new in the UI; the Ledger pages use the bestiary's own page kinds.

## Who owns what (disjoint files; nobody edits another owner's file)

**CORE (C++, strongest model):**
- `World/ReadableProp.h/.cpp`: `FName SetsFlag` (on Read, after the event: `Runner->SetFlag(SetsFlag)`; a second read
  sets it again harmlessly); `FName CountsAs` not needed.
- `Story/SpeakerPointComponent.h/.cpp` (+ wherever topics are said): `FSpeakerTopic.SetsFlag` (FName; set when the topic
  is said, so `without_flags` makes a one-time talk).
- `Scenes/ColdOpenCast.h/.cpp`, `ColdOpenPoint.cpp`: the spark. At `DeaconFires` the ember in his hand flares (strength
  x3, light x3 over 0.15 s) and dims to 0.35 of its rest over 1.2 s, then eases back to rest over 2 s (`FlareAt` time,
  `UpdateGlows` reads it). Keep the shot's timings.
- `World/FarSignalLight.h/.cpp` (or a sibling `World/FarSteadyLight` if cleaner): a **steady** mode (no Morse): `bSteady`
  with a slow 0.5 Hz breath of ±8%; a `ShownState` lighting-state gate (shown only while the lighting state is
  `Dusk`, like `ADuskScenery`) beside `ShownWhen`/`ShownFlag`. The second light sits far west, low over the fog.
- `World/StoryLighting`: nothing, unless a rule "after Main6, during no scene" is needed for the Point's dusk; report.
- `World/WantedPoster.h/.cpp`: variant `EWantedPosterVariant::Seven` with atlas cell `Seven` (`FWantedPosterAtlas`,
  U 0.75-1.0, V 0.703-1.0, 0.25 x 0.297 m: ART paints it there), never torn (hold does nothing on it; tap reads nothing),
  and `CrossedFlags` (TArray<FName>, seven, in the Seven's order Deacon, Ned, Ira, Constance, Barrels, Lena, Tobias):
  each face crossed out when its flag holds, by drawing the X as a second decal cell or a material parameter mask
  (your call; the Deacon's flag is `Ember.Paid.Deacon`, set by nothing yet: a later mission).
- Tests: `Tests/SeedTests.cpp` (`Looter.Seeds.*`): the readable's flag, the topic's flag, the spark's curve, the
  steady light's gate.
- `CODEMAP.md` lines for your files.

**DATA (Python and words, strongest model):**
- `Tools/Unreal/story_texts.py`: READABLES for the eight readables and `ReliquaryLid` (`shown_when=dict(after=['Main4'])`,
  prompt 'Look', no text: lines only: Ellis sees the lid turned over, the old toll mark scratched out underneath, Hob's
  word on it), TOPICS: `Speaker_Aldana` gets `(dict(after=['Main4'], without_flags=['Ada.Told']), 'DA_Lines_AldanaAda')`
  ABOVE the AfterMain5/AfterMain4 topics, with `sets_flag='Ada.Told'` (the topic helper must pass it: see
  `build_area_story.py` `topics()`/`texts_topics`); `Speaker_AgentWindow` the Lily lines; MUTTERS `Speaker_Hob`:
  `[(dict(after=['Main1']), 'DA_Lines_HobIdle')]`; PERCHES: the Point line.
- `Tools/Unreal/create_story_lines.py`: the sets above. `Tools/Unreal/create_bestiary_pages.py`: the seven town pages.
- `Tools/Unreal/build_area_story.py` `place_hob` (~line 706): set the speaker point's `mutters` from MUTTERS; the
  `topics()` helper passes `sets_flag` when a topic dict carries it.
- `Tools/Unreal/build_area_seeds.py` (NEW; folder `RansomsRest/Gameplay/Seeds`; a standalone `run()` that clears only
  its folder, and a `place(build)` hook): the six pages, the two letters (the saloon's in the post rack: ART's
  `SM_PostRack` placed by you on the saloon's front, right of the doors, 1.4 m up; the letter in its third slot), the
  Reliquary lid readable (no mesh, at the Reliquary's `SOCKET_Interact`, shown after Main4), the Reaches map
  (`SM_ReachesMap` on the ticket office's north wall over the desk: from `SOCKET_AgentDesk`, 1.55 m up, flat to the
  wall; a readable with a short text: the lines, the Junction at their heart, seven towns, "Highwater Landing: end of
  the line"), the second light (`AFarSignalLight` steady, from Gravewind Point's bearing west, 9 km out, 40 m over the
  fog, shown after Main6 and only at Dusk), the Seven poster on the Rangers' board (the free strip: offset
  [0, -12, 0] between the Ellis poster and Calder's note; if it overlaps, under the note at [0, -40, -30]). Hook it in
  `build_area.py` `gameplay()` after the valley script. Every placed actor gets the area tag and the folder.
- `Tools/Unreal/build_area_missionprops_valley.py`: ONLY the `LINES` fallback for `AgentWindowLily` (keep the rest).
- The words (player-facing; the Story's voice: terse, dry, western; no copyrighted lines): six pages in Sexton's hand
  (copperplate; each a ledger entry on a town: the saint's name, what the town owed, the date of the robbery, a note in
  the margin that gives the saga away a little: the Ransom's Rest page dated a month BEFORE the robbery, with Ellis's
  own name already entered), the letter never sent (*"Kid. The man who pays us isn't a man."* and three more lines), the
  letter in the post rack (addressed to the Rest, unopened: Ellis reads it anyway; the Deacon writing to Delia), Hob's
  three feather lines word for word from the Story, Aldana's Ada talk (the first keeper's story, 5-7 lines, Aldana then
  Ellis then Hob), Sloane's two new lines, the lid's lines. Write every saint's name you invent into `Docs/Story.md` (the
  Cast or the Areas table) so there is one place for them.
- `Docs/Story.md`: the seven towns' saints and the pages' facts; "Seeds in Ransom's Rest": mark each seed built.
- `CODEMAP.md` lines for your files.

**ART (Blender, Sonnet or stronger):**
- `Art/Models/Props/MissionProps.py` (BUILDERS): `SM_LedgerPage`, `SM_DeaconLetter`, `SM_PostRack`, `SM_ReachesMap`
  (the map's texture: `Art/Textures/ReachesMap/T_ReachesMap_BC.png` painted by a script beside it, Rye font from
  `Art/Fonts/Rye-Regular.ttf`, the seven towns and Halfway Junction placed on a hand-drawn line network, Ransom's Rest
  on the Rim at the west edge, Highwater Landing at the line's end, the Junction at the heart; a material instance of
  the paper master with it). Export with `Tools\artrun.ps1` / `Tools\models.ps1 -Only <Name> -NoImport`; Main imports.
- `Tools/Blender/looter_posters.py`: the `Seven` cell at U 0.75-1.0, V 0.703-1.0: "THE SEVEN" over seven small inked
  busts in two rows (hats and silhouettes tell them apart: the Deacon's preacher hat, Ned's bowler, Ira's whistle, the
  Sister's veil, Barrels's apron, Lena's spectacles, Tobias's square bulk), names under them, "Capt. R. Calder" signed
  low right; plus a `SevenCross` cell for the X mark if CORE asks for a cell (coordinate with CORE's choice through
  Main: default = CORE masks by material parameter, no extra cell). Re-render the atlas PNG; Main reruns
  `build_decal_materials.py`.
- Look sheets for the user's morning (`Saved/Concepts/Seeds/` as JPG, 1600 px wide): the four props and the poster,
  rendered with `Tools/Blender` helpers as the roost sheet was (`roost_sheet.py` pattern).
- `Art/README.md` only if a rule changes.

## Done means

- CORE: builds clean, `Looter.Seeds.*` pass, every other test still passes (Main runs them).
- DATA: `create_story_lines.py`, `create_bestiary_pages.py`, `build_area_seeds.py` run clean through Main's console;
  `build_area.py RansomsRest gameplay` still runs; the words in; Story.md and CODEMAP updated.
- ART: the four models export without warnings; the atlas PNG rerendered; look sheets written.
- Each report: what you did, every decision you made, what you could not do, in under 40 lines.
