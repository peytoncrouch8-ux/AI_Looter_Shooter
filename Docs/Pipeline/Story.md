# Pipeline: Story in the game

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## The heroes' words (one story, whoever you play; built 2026-10-20)

Docs/Story.md "The heroes": every hero takes Ellis's place; Pa is the Keeper, Delia the Keeper's mother; the Deacon's "kid"
is whoever you are. Every text stays written as Ellis's (scripts, assets, LOCTEXT); one table swaps it where it is shown.
- **Data:** `/Game/Data/Story/DA_HeroLines` (`UHeroLineTable`, rows `FHeroLineVariant`: Id, Source, Speaker, Other,
  Heroes, bDropForOthers), written whole by `Tools/Unreal/create_hero_lines.py` from `Tools/Unreal/hero_lines.json` (the
  one place variants are written; 146 text rows + 3 painted). Per entry: `current` = Ellis's words exactly (the key),
  `neutral` = what the other four read, `heroes` = a hero's own voice, `speaker` = who says it (a row with a speaker
  swaps only in that speaker's mouth: "Who?" stays Hob's), `drop_for_others`. The script refuses (saves nothing) a
  duplicate current (ignoring case), a hero id outside Odessa/Crane/Gauge/Pike, an unknown `{token}`, or a lost `{0}`/
  `{Interact}`. Run it after `create_story_lines.py`; then `Looter.Lines.*`.
- **Precedence** (`HeroLines::Resolve`, by the text's source string): Ellis, or no row: as written. Else the hero's own
  words; else nothing if `drop_for_others`; else the neutral; else as written. Tokens in the chosen words: `{Hero}`
  (`UHeroDefinition::GetShortName`, the id), `{HeroFull}` (DisplayName), `{HERO}` (in capitals), `{Initialed}` ("O. Lark";
  one word stays: "Gauge").
- **The played hero** is a static in `HeroLines`: `USessionSubsystem` sets it in `BeginPlayWorld` (InitGame, before any
  BeginPlay) and `BeginMenuWorld`; `Looter.Hero` (via `SetNoSessionHeroId`) at once. While automation tests run it is
  Ellis unless a test opens `HeroLines::FScopedHero`, so a Looter.Hero left set never fails a test that checks Ellis's
  words. `FScopedTable` lends a test its own table.
- **Where words are swapped:** `UCaptionSubsystem::Play` (every caption and speaker: line sets, mission lines, readables'
  lines, the porch plate, C++ scenes, Abel, Hob's roosts, town life, posters' remarks); mission giver names
  (`GetGiverName`, `GetOfferGiverName`, the offer card's `GiverNameOf`); summary and briefing (offer card, Missions page,
  notice board); objectives (`GetDisplayText`/`GetShortText`, before the keys); choices; mission items (name, line,
  prompt); the readable card (heading, title, text); the Ledger (name, kind, habitat, description, notes); the boss bar's
  phase (`MakePhaseLine`, before the capitals); the heirloom's "not for sale" pattern; the barks
  (`UCreatureVoiceDirector`). A new place that shows story words calls `HeroLines::Resolve`.
- **Painted words:** `AReadableProp::HeroMeshes` (the played hero's own model) / `OtherMesh` (any hero but Ellis), applied
  in BeginPlay: Ellis's headboard readable carries `SM_Headboard_Ellis` on the grave's transform and the four others in
  `hero_meshes` (`build_area_story.py` `hero_headboards`); the keepers' roll `other_mesh` = `SM_KeepersRoll_Bare`
  (`build_area_missionprops_valley.py` `OTHER_MESHES`). Art: `Art/Models/Props/Graves.py` (Grave_Ellis has no board of
  its own: import Graves only together with a gameplay build), `MissionProps.py`. Wanted posters: in a game world
  `AWantedPoster` takes `MI_Poster{Decal,Scrap}_<Id>` (atlas `T_Posters_<Id>_BC` by `Tools/Blender/looter_posters.py
  --hero`, materials by `build_decal_materials.py`), else Ellis's; never in the editor's level (it saves its instance).
- **Writing new lines** (the user's rule): shared lines leave out "Pa", "Ellis" and "kid" (the Deacon's "kid" excepted)
  and need no row. A line that is Ellis's own (the family, Ellis's past) is written as Ellis's and gets a row with its
  neutral; a hero's own voice is a row with `heroes` only. The player character's lines name speaker "Ellis" (shows the
  played hero). Later regions' Ellis beats (the spark's "keeper by blood", "The Kid Who Came Home", the Gravesend
  confession's "came home", Delia's door in the epilogue) need rows when written.
- **The user's pending calls** (Main's provisional answers, 2026-10-20) are in `hero_lines.json` `decisions`: cold open
  kept for all; "kid" for all; the townsman Hollis renamed Harlan (Harlan Pruitt) for everyone, in C++
  (`TownLifeRules.cpp`) and `build_area_vendors.py`, no rows; epitaph "<NAME>. STOOD UP AT THE LAST." and the posters as
  drafted; Sexton's sweeteners per hero.
- **Tests:** `Looter.Lines.Table` (the asset matches the json, sources unique, tokens filled, `{0}` kept, no family word for
  any other hero), `.Precedence`, `.Wiring` (captions, objective, boss phase, readable mesh pick, no table),
  `.NoFamily` (each other hero: every line set, mission, Ledger page, named gun, Lvl_RansomsRest's game actors, game
  classes' defaults and the C++ lines read without `\b(Pa|Pa's|El|Ellis|Grandma|grandmother|your father|young'un|Ransom kid|Ransom's child|Abel's child)\b`;
  allowed: "Tilly's Pa's car"; 1559 texts on 2026-10-20). A NoFamily miss: give the line its row, or rebuild the
  placed piece whose words drifted from the script.
- **Merged 2026-10-20** (LINES-WIRE): run order was build, `create_hero_lines.py` (146 rows), `create_ember_assets.py`
  (short names), `Looter.ImportModels Intermediate/ArtExport_LinesArt` (Graves + MissionProps, 56 models),
  `build_decal_materials.py`, `build_area.py RansomsRest gameplay` (story, valley props, vendors: the headboard's
  `hero_meshes`, the roll's `other_mesh`, the Harlan greeting), tests 743/743. Seen in PIE as Crane and as Ellis: the
  headboard carving and card, the poster, the roll (bare for Crane, ELLIS painted out for Ellis), captions naming the hero,
  Delia Ransom at her door. The headboard now stands at the grave's head (the readable's own mesh), read from beside it.

## Seeds in Ransom's Rest (the saga's first clues, built 2026-10-16 overnight)

The list and the words' facts: `Docs/Story.md` "Seeds in Ransom's Rest" (the seven towns' saints in its Areas table).
Three agents built it to a contract (`Saved/Seeds/CONTRACT.md`, not committed; what it fixed is here). Everything placed
lives in the outliner folder `RansomsRest/Gameplay/Seeds`, from `Tools/Unreal/build_area_seeds.py` (hooked into
`build_area.py gameplay()` after the valley script; alone: `build_area_seeds.py RansomsRest` clears and refills only its
folder). The pieces:
- **The hooks (C++):** `AReadableProp.SetsFlag` (Python `sets_flag`; set after the `Readable.<Id>` event; reading again
  is harmless), `FSpeakerTopic.SetsFlag` (set only when that topic's own lines were said: not on a turn-in, never by a
  mutter; `without_flags` on the topic makes a one-time talk), `AFarSignalLight` `bSteady` (no Morse; a 0.5 Hz breath of
  ±8%) and `ShownState` (a lighting-state gate like `ADuskScenery`'s; None = any state; its constructor still defaults
  `ShownWhen` to The Last Watch's, so a new light overrides it), `AWantedPoster` variant `Seven` (tag `SevenPoster`, not
  `WantedPoster`: Side 1 never counts it; never torn, no hitbox; `CrossedFlags` = `Ember.Paid.Deacon/Ned/Ira/Constance/
  Barrels/Lena/Tobias`: each face gets a small `SevenCross` decal, sort order +1, once its flag holds, redrawn on
  `UMissionRunner::OnChanged`; the faces' cells `SevenFaces` in `FWantedPosterAtlas`, overridable by `Atlas=` in
  DefaultGame.ini), the cold open's spark (`AColdOpenCastGlows.cpp`: `FlareEmber()` at the `DeaconFires` moment: x3 in
  0.15 s, down to 0.35 over 1.2 s, back to 1 over 2 s; glow and light both; not while skipping). Tests `Looter.Seeds.*`.
- **The Ledger's loose pages:** six `AReadableProp`s with `SM_LedgerPage` (ids `LedgerPage_<Town>`, flag
  `Ledger.Page.<Town>`), their words generated from `story_texts.LEDGER_TOWNS` (one dict a town: saint, carrier, habitat,
  where found, the page's text, the lines after reading). Each opens a Ledger town page: `DA_Bestiary_Town_<Town>`
  (`create_bestiary_pages.py`; STORY_CHARACTER pages in the NPC section, "???" until found, `found_when with_flags`;
  not LEDGER_NAME: `Looter.Ledger` wants exactly the seven names there). The seventh (Stoneweight Quarry) has no page
  on the Rest. Where: the lockup floor by the bunk (The Needles), a trunk in the depot's baggage room (Highwater
  Landing), the vestry door's step (Ransom's Rest: entered a month before the robbery, Ellis's name twice), Ruth's table
  at Calder's Post (Dustwater Mesa), the Providence's tag board under the IN hooks (Furnace Hollow), the mooring ledge
  (Saint Agnes Mire).
- **The Deacon's letters:** `SM_DeaconLetter` (folded, sealed; one model for both). `DeaconLetter_Cleft` wedged by the
  mooring post, never sent (flag `Letter.Deacon.Cleft`); `DeaconLetter_Saloon` standing in slot 3 of `SM_PostRack` on
  the Gilt Spur's front, right of the doors, 1.4 m up (the rack's sockets `Slot_1..8` sit where a standing letter's
  bottom middle rests, leaning back 8°; the flat model is stood nose down (pitch -90 in its own frame), then given the
  slot's lean, then lifted half its length, or it sinks into the shelf). Postmarked Halfway Junction, 9 August, to Mrs.
  D. Ransom, unopened: his warning to get Abel and the lantern off the Rest.
- **The map of the Reaches:** `SM_ReachesMap` (90 x 60 cm, `MI_ReachesMap` on M_World; texture painted by
  `Art/Textures/ReachesMap/make_reaches_map.py`, numpy + Rye, BC/N/ORM, 2048 x 1024 stored from a 2048 x 1366 sheet) on
  the ticket office's north wall over `SOCKET_AgentDesk`, a readable (`ReachesMap`). Halfway Junction at the heart, the
  Rim and "UNSURVEYED" west, Highwater Landing "END OF THE LINE", "W. PIKE, SURVEYOR" (Wendell Pike, a nugget ART
  added). Sloane's window (`DA_Lines_AgentWindowLily`, and the valley script's `LINES` fallback) names the Junction
  and Highwater Landing once the line is open.
- **The Seven on the wall:** the poster in the Rangers' board's free strip (layout.json `gameplay.posters`, offset
  [0, -12, 0] between Ellis's poster and Calder's note; 25 x 29.7 cm). Art: `Tools/Blender/looter_posters.py` cells
  `Seven` (U 0.75-1, V 0.703-1: "THE SEVEN / OF THE DUNNE GANG", seven inked busts in ovals told apart by hat and
  outline, names, "DEAD OR ALIVE", Calder's signature) and `SevenCross` (U 0.25-0.359, V 0.891-1: an ink X on alpha).
  After rerendering the atlas run `build_decal_materials.py`.
- **The road at dusk:** the Gravewind's wisps were already the river of lights at Dusk; added the second light: a
  steady `AFarSignalLight` due west of `Place_GravewindPoint`, 9 km out, 30 m up, size 5500,
  strength 30, pale cold white, `shown_state Dusk`, `shown_when after Main6`; and Hob's `DA_Lines_HobMain6Point` on his
  after-Main6 bier perch (Ellis sees the light, then *"Don't look at that one, sunshine."*). Not gated on Grave Sight
  (there is still no vision mode: only the Reliquary's flash). `Looter.FarLight 1` shows a far light for a check.
- **What Ada gave up:** Aldana's topic `(after Main4, without_flags Ada.Told) -> DA_Lines_AldanaAda`, `sets_flag
  Ada.Told`, placed ABOVE the after-Main topics (first match wins); the `ReliquaryLid` readable (no mesh, at the
  Reliquary's `SOCKET_Interact`, shown after Main4, "Look": the old toll mark scratched out under the lid, which the
  model shows prised up at one edge, not turned over).
- **Hob's feathers:** `DA_Lines_HobIdle` (the three lines, in order) as Hob's speaker point's `Mutters`
  (`story_texts.MUTTERS['Speaker_Hob']`, after Main1; `place_hob` sets them). They ride the town-life mutter clock
  (one line per mutter, 75 s rest, within 7 m of his perch) and play the town-life voice cue like any muttering point.
- **The spark:** see the hooks. **Fenn's fare:** untouched.
- Lessons: a `.ps1` argument like `/Game/Maps/X` run through Git Bash becomes `C:/Program Files/Git/Game/...` (run
  perf and anything taking a `/Game` path from PowerShell); `stage_import.ps1` wants its names as ONE quoted
  comma-separated string from PowerShell (a bare comma list is split into an array and joined with spaces);
  `Looter.ImportModels` resolves a relative folder against the editor's working directory, so pass the absolute path.
  Look sheets for the user: `Saved/Concepts/Seeds/` (`Tools/Blender/seeds_sheet.py`).

## The story's talk cut, and things to find (STORY, built 2026-10-22)

The user's calls (2026-10-21): cut dialogue about half, most lines short captions said on the move, long scenes only at big
beats, fetch steps into fights or finds, things to find off the path, every beat, character and seed kept. Main's rulings:
no new canon, Fenn's fare (the Old Fare) word for word, "never a priest's to do" kept, Amos's twist after the hands' fight.
The line-by-line plan and its numbers: `Saved/Story/PLAN.md` (not committed). Spoken words 4,132 -> about 2,650 (-36%); the
talk at givers -44%.
- **How words reach the player:** every line is a caption (non-blocking); what holds the player is an offer card, which waits
  until its giver's words are said and the player still stands there, and the walk between givers. So offers are two or
  three lines, turn-ins three; Hob's perch arrivals (said as a step starts, wherever the player is) carry the story on the
  move; a perch that only gave directions is silent now (Hob still flies there). Hob's taps: Main 1's, the Old Fare's and
  the catch-all only (`story_texts.TOPICS['Speaker_Hob']`).
- **New hooks:** `AEncounterSpawner::ClearedLines` (a line set queued once the fight is won: `DA_Lines_AmosRoad` on
  WhitlockHands); `FMissionTurnIn::HandItems` (a later mission's items carried from the turn-in: Tilly hands Main 6's
  `LanternBail` over at Main 3's; the sill pickup goes as it's carried); save version 4
  (`CampaignStepMigration::UpgradeTalkCut`: Side 2's and Knocking Below's optional keys, Dead Lift's retired items).
- **Missions changed:** Side 1 tears 4 posters (Hob's remark at 4). Side 2: 3 bales, then the hands (the road talk went: the
  twist is said as the last falls). Knocking Below: the key off the hook is the first step; Thaddeus's talk and his tape are
  one. Dead Lift: only Abe's pin to get (a fight); the boiler takes the quarry's coal bin; the crank stands on its shaft
  (`LiftCrank_OnShaft`); items WinchCrank and CoalSack retired (never reuse the ids). The Old Fare: a second push
  (DepotReturn: 4 Unpaid and a Restless one) as the coins come down the stair, during its lay step. Main 7: the orchard's
  edge camp comes back at dusk (`build_area_camps.DUSK_RETURNS`: Camp_OrchardEdgeDusk, kept out of the layout's camps,
  whose tests want every camp on for good). What Ada gave up is Father Aldana's page slid under the vestry door
  (READABLES `AldanaPage`, sets `Ada.Told`).
- **Things to find off the path** (`build_area_secrets.py`, `story_texts.SECRETS`; `Gameplay/Secrets`): the Widow's
  strongbox under the middle Widow (2 guns at Luck 1.3, the legendary chance), the gang's locker on the Mooring Ledge (Hob's
  hint at Main 2's turn-in), the surveyor's tin and W. Pike's bench mark on Coffin Rock's top (a vista), the Averys' tin in
  the Burnt Homestead's hearth (town mutter after the gate), Thaddeus's bottle in the springhouse (his word after Knocking
  Below), the keepers' stores in the old lantern house, a spiders' hoard under the hanging tree, Teague's powder box on
  Larkspur Bench (Odell's report's proof), a crate fallen in the quarry blocks. Each readable sets `Secret.<Id>`.
- **Retired line sets** (deleted after the rebuild that stopped pointing at them): HobMain2Climb, HobMain2, HobMain3,
  HobMain4Road/Bell/Aldana, HobMain4, HobMain5Sacs/Lantern, HobMain5, HobMain6Bridge/Post, HobMain6, HobMain7Depot/Board,
  HobMain7, HobSide1, HobCaches, AgentWindowCoal, HobKnocking, HobSide3, HobDeadLift, HobLastWatch, HobBackPay, AldanaAda.
- **Run order:** build; `create_story_lines.py` -> `create_mission_assets.py` -> `create_side_mission_assets.py` ->
  `create_hero_lines.py`; `build_area.py RansomsRest gameplay`; then delete a retired set's asset (never before: a placed
  actor still pointing at it opens a modal force-delete dialog); tests `Looter.Lines`, `.Missions`, `.Story`,
  `.Session.StepMigration`, `.World.TownLife`, `.Roost`, `.Poster`, `.Progression.Pacing`.
- Checked in PIE (2026-10-22): the gate, the Main Street set piece (walked into: a teleport into its ground doesn't spring
  it), Tilly's turn-in hands the bail over, Side 2's twist after the fight, the Widow's strongbox (read and opened), Coffin
  Rock's tin. **Not working yet:** Coffin Rock's boulder steps (scaled `SM_Boulder_A`, 125 cm rises): the climb fails on
  the rounded tops twice; give it a real ramp or ledges in the world-variety pass.
- Lessons: `SystemLibrary.box_overlap_actors` always returns the PCG scatter actor (its bounds span the area) and the
  terrain tile on a slope: judge a spot by down-traces at its corners (each on a `Ground`-tagged actor or the ruin it stands
  in, within 18 cm) and ignore actors named `*Scatter`. A `delete_asset` that logs "deleted" can leave the file on disk (one
  of 25 came back on the next editor start): check the file is gone. A layout obstacle's centroid lands on the rock's top:
  trace the ground at the spot itself, not at the anchor.
