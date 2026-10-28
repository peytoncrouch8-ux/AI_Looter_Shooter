# Pipeline: The game

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## The game

- Story: *Revenant* (`Docs/Story.md`), weird west on the planet Teropa. One area per Hollow, each designed in `Docs/Areas/`.
- Areas, map order since 2026-10-13:
  - 1 Ransom's Rest: a new game starts on the new game's intro, then its cold open; its first missions teach the controls by hints. Mountain expansion:
    `Docs/Areas/RansomsRest_Expansion.md`.
  - 2 the *Gilded Lily*: behind the north mountain, by train; not built.
  - 3 Skyreach (the old tutorial island): reached later from the Lily by skylift; off the station boards for now, test with
    `Looter.Travel Skyreach`.
- Missions: all but Main 1 come from a giver (`EMissionStart::Offered`, `FMissionOffer`): a speaker point, a notice board or an
  artifact (`AMissionArtifact`).
  - ? = pick up, ! = turn in (the user's call, 2026-10-13: the reverse of Borderlands). Gold = story, white = sides. Shown on the
    map, the minimap and over the giver (`UGiverMarkSubsystem`).
  - Offer card `UMissionOfferWidget`: Accept / Not now. The Missions tab lists only missions taken up.
    `Looter.Mission.Offer <id>` opens a card.
  - Features: optional objectives (`bOptional`: never hold a step, pay `OptionalSilver` at the turn-in); mission items
    (`AMissionItem`, carried, dropped by a creature); a turn-in with a choice of two (held to choose; story flags in
    `FCampaignRecord::StoryFlags`, read by `FStoryCondition::WithFlags`); readable props; the porch plate; an UNDERGROUND minimap
    plate (actors tagged `Underground`); any speaker point or a vendor's hatch as a turn-in.
- Hob's roosts (the user's pick, 2026-10-13): the ONE save / respawn / fast-travel point on every map: the Crow's Mast
  (`SM_HobRoost`, bell `SM_HobRoostBell`), beacon in Hob's Ember (#D8321C, core #FF8A52; `URoostSettings`, in Project Settings
  under Game > Hob's Roosts; `LooterUI::Color::Roost`).
  - Walk within 9 m (clear line) to find one: column shoots up, bell rings, crows lift, three circle the beacon, ROOST FOUND,
    early autosave. Continue wakes the player at the area's last found roost (`FCampaignRecord::LastGraves`).
  - Fast travel and deaths: Hob's crows carry the player across the screen (`URoostCarrySubsystem`, `UHudCrowVeilWidget`); Hob
    speaks at every death.
  - The actor is still `ARespawnMarker` (save ids unchanged). Placed from `layout.json` `gameplay.roosts` by
    `Tools/Unreal/build_area_roosts.py` (run alone it replaces every roost in the level). `Looter.Grave.List`,
    `Looter.Grave.Mark <id> | forget <id|all>`.
  - THREE a map at most (`MOST_PER_MAP`): the user cut them to two (2026-10-14: the Depot by the train station, Calder's Post
    up the mountain; the other nine removed, not kept for saves), then added a town roost (2026-10-23, "Town roost": deaths in
    town woke at the farm): SheriffsOffice ("Main Street"), in the street's north edge at the sheriff's office (layout cm
    [300, 800], onRoad, facing east down the street), outside the Main Street ambush's trigger. New ids only (an old save's
    find of a removed id must not find a new roost).
  - Fast-travel STATIONS are the Depot and Calder's Post only (the user, 2026-10-27, note 24: keep two, one in lower RR and one
    on the mountain; Main's reading: the Sheriff's Office roost stays the death-respawn and save point). `ARespawnMarker::
    bFastTravelStation` (default true; the layout roost entry's `fastTravel` false, `storyfix_2.json`, sets it off for
    SheriffsOffice; `build_area_roosts.py` reads it). `GraveTravelRules::CheckGrave` refuses a non-station (`NotAStation`);
    `MapPins::Gather` skips non-station roosts, so the map's pins, travel list and legend leave the town roost out (the
    minimap's roost marker still shows it). Tests `Looter.Map.Station.Rules/.World`.
  - A death wakes at the roost last found on the map (never the nearest), before any at the level's own start (RR: the
    family plot's PlayerStart), Hob's crows carrying Ellis either way on a map with roosts. Arriving by fast travel finds
    the roost too (`MarkOnArrival`). Continue wakes at the same roost, before any where saved. One lookup:
    `ARespawnMarker::FindLastFound` (`LastGraves`, else the newest `MarkedGraves` still standing: an old save naming a gone
    roost loads onto one that stands; no migration).
- Impacts and bullet marks: a hit's surface (13: dirt, grass, gravel, sand, stone, wood, metal, sheet metal, glass, water, cloth,
  hay, bone) comes from tags, then the trim sheet's strip under the hit (collision UVs), the terrain's baked surface map,
  material-name words; no physical surface types. Each has its sound, puff and mark; glancing hits ricochet.
  - Guns' after-shot (2026-10-14): brass (`FWeaponBrass`: last 60, 20 s + 1.2 s sink, SOCKET_Eject or a per-kind port on the
    visible gun; the revolver tips its empties at the reload's Eject step; `Weapon.Casing.*` by surface); a hot barrel's wisp
    (no shimmer: a distortion pass costs too much); tracers with a hot head and a smoke thread; visible ricochets (at most 2 a
    volley); impact chunks land on the hit's floor; each creature splashes its own (`ECreatureBlood`); enemy shots passing within
    1.5 m of the player's head whiz or hiss once (`NearMiss`).
  - Spider deaths (the user, 2026-10-14: more dramatic, legs curled UNDER the body): `SpiderCreatureDeath.cpp` throe
    (rear, kicks) 0-0.4 s, drop on the cry's knock with a bounce + `Creature.Spider.Thud` + dust, legs folded under back to
    front by 1.15 s, shudder before the sink; timing scales by sqrt(size). Watch any death on cue: `Looter.SpawnCreature`,
    `slomo 0.25`, `Looter.KillNearest`, grab frames, `slomo 1`.
  - Creatures react (2026-10-14, `UCreatureReactComponent`): a notice beat once a hunt (0.42-0.62 s by seed; none when hurt,
    a boss, or called by the pack), the M_FX_HitFlash overlay on a hit, deaths that linger (spiders 10 s with twitches, slimes
    splat, the Unpaid crumble to ash), rising entrances when seen within 60 m, idle dice, footfall dust (level budget 40/s).
  - Marks are pooled DBuffer decals (`Looter.BulletMarks.Max/.Hold/.Fade`, 64, 20 s + 15 s fade); atlas script-made
    (`Art/Textures/BulletMarks/make_bullet_marks.py`).
  - Re-run `Tools/Unreal/build_impact_surfaces.py` after a macro map, scatter map or new trim-sheet model changes.
- Objective highlight: within 13 m, what the tracked mission's current objective is about (optional ones too, the giver once it
  waits; never a fight's creatures) glows orange as an overlay on its meshes (a spot of a building round a door).
  `M_ObjectiveHighlight` from `build_highlight_material.py`; `Looter.Highlight.List`. A character (a story character or a
  skinned figure, never a gun; `IsFigure`) wears the figure look: rim only, body all but clear (`Figure` 1; tune the
  script's `Figure*` defaults). A thing's 20% fill turned Sexton's near-black coat solid orange (fixed 2026-10-14).
- Holds: every hold-to-use shows the kit's round ring (`SLooterProgressRing`, `LooterUI::MakeHoldKeyCap`).
- HUD (the user's list, 2026-10-14): the mission tracker sits under the minimap, right-aligned (`TopFor`, `RightInset` 36,
  `MaxWidth` 540: clear of the boss bar's 380 px either side of the middle); steps and the objective are circles, checked
  once done; a line without a count takes the count's room before wrapping. A kill's XP floats by the kill (`XpPopup*`) and
  as "+N XP" after the XP bar's numbers. The gun's notches (inked tally in its tier's colour, count) sit before its name.
  Health and XP bars 25% longer and thicker (`HudFrameShapes::BarScale`). Subtitles On/Off in Interface
  (`Looter.Subtitles`): off hides lines with a speaker (barks too); notices stay. The game has no voice acting, so off
  means NPC talk isn't shown at all.
- Playtest round 1 (2026-10-27, the user's notes; `Docs/Archive/Work/2026-10-27-Playtest/NOTES.md`), the UI and world parts:
  - The inventory pauses the game (note 8): `ALooterHUD::ApplyGamePause` (`LooterHUDPause.cpp`, with the pause menu) pauses
    while the pause menu or any inventory page (the map key too) is open (`PausesGame`); called on every open/close path.
    Vendor, bench, boards and the offer card don't pause (they `CloseInventory` first). Pausing collapses the HUD and world
    labels at once (the HUD doesn't tick paused). Anything on a page that moves must tick while paused (`StageStudio` sets
    `bTickEvenWhenPaused` on the stand and its model) and its sounds be class Interface (`bPlaysWhilePaused`; the test checks).
    Music ducks and ambience pauses under the inventory as under the pause menu (not asked about). Test `Looter.UI.Inventory.Pause`.
    Mission steps and control hints tick with the world, so while the inventory pauses the game they never see a page open
    (Main 2's Ledger step stalled): the HUD keeps the pages shown during the last visit (`ALooterHUD::WasPageShownLastVisit`,
    counted afresh as each visit opens) and the "page open" sense of the missions (`MissionPlayerObjectives.cpp`) and of the hints
    (`ControlHintSubsystemSenses.cpp`) reads `IsInventoryOpen() && GetInventoryPage() == Wanted` OR that, so closing the inventory completes the step.
    Any new "player opened X" check on a paused UI must do the same. Checked in PIE; Missions, Tutorial, TalkBusiness tests pass.
  - No silver on the gameplay HUD (note 19): the vendor's purse, the inventory's ammo line and the mission-complete purse stay.
  - Chests are off the map (note 26; `EMapPinKind::Chest`/`ChestLooted` gone, `UMapDiscoverySubsystem` and
    `FSavedMapWorld::FoundChests` removed; old saves load, the property is skipped). Unused leftovers: `AChest::bOnMap`,
    `ShowsOnMap`. "More distinguished locations" for chests: done in round 3 (`Levels.md` "Roads and chest spots").
  - Loot piles (note 41): guns within `Looter.LootLabels.PileRadius` [300 cm] show only `.PileShown` [4] names in full (looked-at
    first, then rarer, the one shown before, nearer); every Legendary always shows; the rest fold to a dot on the gun
    (`UWeaponLabelWidget::SetFolded`; round 2: a dark halo of 26 units under a rarity-coloured core of 18 with a light 2-unit rim,
    since the old 12-unit tinted marker was lost in a legendary beam's glow); looking at a dot opens its card. Wards never fold.
    0 = old way. `LootLabelLayout::PickShown`. Tests `Looter.Loot.Labels.Layout/.Pile/.Fold` (the Fold test's room 28, dot >= 20).
    Dot size and the 4 not judged by eye yet.
  - General store REFILL ALL AMMO (note 13): first row under BUY (`VendorTrade::RefillAllAmmo`, `TallyRefill`, `RefillPrice`):
    every ammo type to its most for one payment, all or nothing; price per type = whole boxes x box price + the last part box
    (`AmmoPrice`), equal to clicking Buy until full. The row shows "+N rounds across K kinds" and the price; FULL when full; red
    "Not enough silver" when too dear; otherwise a confirm panel. It is the default selection (a confirm guards it); no hotkey.
    Buy column ~66 px taller: check it fits at 1080p. Tests `Looter.Economy.Trade.RefillPrice/.RefillAll`.
  - Settings menu (note 4): four tabs (Graphics, Audio, Interface, Controls; LB/RB or PgUp/PgDn wrap; each page its own
    `UScrollBox` in a `UWidgetSwitcher`), panel fixed 940 x 840, and a 272 px description column right of the page (row name, what
    it does, a dim range/default line; follows focus under keys and pad, the pointer under the mouse; words in
    `SettingsMenuHelp.cpp`, written from the apply code and `Docs/Performance.md`'s preset costs). "Reset all" moved to Controls
    ("Reset all keys"). Hover uses `IsHovered()` on each row's `MakeRow` overlay. Round 2: the slider and choices row under the
    mouse wears the keys' focus look (steady, no breath; `AdvanceHoverLook` in `SettingsMenuTabs.cpp`) and takes the Nav focus (the
    next key press starts from it; keys taking over keep the look); buttons keep their own hover look; no automated test (needs a
    pointer). Tests `Looter.UI.SettingsMenu.Help/.Tabs`.
  - Vigil stones (note 36): each stone counts `CallBackSeconds` (5 min of play) from its own record's `DefeatedAt`; a Save & Quit
    readies none. This reverses the user's 2026-10-12 "or at once after a Save & Quit". `SessionLoads` stays in the save only.
  - Back Pay's tags open no reading card (note 31: the Barricade readable's text is empty; its sound, caption and step still
    fire; the chalk line is in Ellis's caption); round 2: its "Take the tags" prompt now goes once they are taken (a hold, step 4
    only; see Mission gates below). Main 4 step 5 says "Examine Saint Ada's smashed Reliquary, where her light
    burned." and Aldana's bell line says what it held (note 18). Amos's house has its wanted poster (note 9,
    `storyfix_1.json` posterAmos; position a guess, check the build log); Side 1 now has 8 posters, needs 4.
- Playtest round 2 (2026-10-27; contract `Docs/Archive/Work/2026-10-27-Playtest/CONTRACT2.md`), the game and UI parts. Bosses, the Unpaid's
  kit: `Enemies.md`; grenades and recoil: `Gunplay.md`.
  - Mission gates (note 38): steps open in order. `Missions/MissionGates` (`HoldSeconds` = `Looter.Missions.HoldSeconds` [0.6 s, 0 =
    taps like before], `IsMet`, `AnyMet`, `SetUse` sets a key action's hold or tap and its prompt). `AMissionItem::TakeWhen`
    (an `FStoryCondition`; empty = whenever shown; a dropped one ignores it): never focused before its step, always a hold,
    `Take()` refuses early. `AReadableProp::UsableWhen` (any of the conditions; empty = always) and `bHoldToRead`: seen but never
    focused outside its step. Lore stays a tap any time. The level scripts pass them: mountain `ITEM_GATES`/`READ_GATES` with
    `gate_item`/`gate_reading`, valley `gate()`, `readable(usable_when=)`, `item(take_when=)` (steps found in the mission data; a
    design fallback is logged); Python names `take_when`, `usable_when`, `hold_to_read` (an old module warns and places them
    ungated). Every gated prop is listed in `RansomsRest_Expansion.md` §3 "Mission gates". Run `build_area.py RansomsRest gameplay`
    (the mountain script alone drops the Foreman's vigil stone). The gang's strongbox (`build_area_caches.py`) has `open_when` =
    Calder's Caches from step 1: it counts from the world as a lasting interact, so opening it early would skip the step. Still
    taps (step-gated by their own classes; could call `SetUse(Options, Words, true)`): `AKeepersLantern`, `APorchPlate` jars,
    `Headboard_Abel`, Fenn's coffin, `AChapelReliquary`. Tests `Looter.Missions.Gates.Item/.Readable`, `.ItemPrompt`.
  - Map place names declutter (DEMO's find: WIDOW'S PINES and CHAPEL OF SAINT ADA overlapped): `MapPlaces::Declutter` (majors
    first, then minors; a name that would overlap slides up or down by its own height, up to 3 steps either way, else is left
    off), `NameSize` (0.93 x font size per letter, measured off one screenshot: a wrong guess only moves a name a line or hides it
    early; check the map at zoom 1.1 and 1.8), `MapWidgetDraw.cpp` `LayoutPlaces` (gap 4 page units). Test
    `Looter.UI.Map.Places.Declutter`.
  - A dead player could fire, reload and aim (death only blocked moving and looking): `AWeaponBase::StartFireAt`, `HandleFiring`,
    `CanReload` and `UpdateAim` (`PlayerViewAim.cpp`) check the holder's health; a held trigger, a burst under way, a queued shot and
    an Overcharge wind-up (dropped via `CancelCharge`, not fired) stop at death and the sights drop; all back at `ResetHealth` (the
    respawn). Tests `Tests/DeadHandTests.cpp`. Checked in play: 28 -> 13 rounds after a death.
  - Map place names also avoid pins and the player arrow (`MapPlaces::Declutter` takes `Blocked` boxes; tries vertical steps and a
    slide to either side, `SideCost` 1.2 steps, `SideClear` 16 units past the half width).
  - The legendary name pop drawn twice (DEMO's shots `leg_burst1-3`; not reproduced, cause read from the code): (1) `SetPop(0)`
    ran on the frame the pooled widget was taken while its screen spot was still the last frame's, so it drew there and over the
    gun: the pop now stays at opacity 0 until `NamePopSettle` (0.05 s) of life; (2) the gun's own label carried the same name
    in the pile: it is held at opacity 0 while the pop shows (unless looked at) and restored when the pop ends, the gun is taken
    or `ClearShows` runs. If two names still show in the next play-check, a second `StartShows` for another gun of the same name is
    the cause. Code `Loot/LootFanfareShows.cpp`. Round 2: the pop's label code is `Loot/LootFanfarePopLabel` (hides the gun's own
    label while it shows; `KeepClearOfLevelUp` slides it clear of the LEVEL UP banner, `LevelUpClear` 14 px).
  - Ransom's Point bounty arena moved to (-8100, -8500) (a player fell ~14 m off it at (-8682,-7016,622)): hunting ground a
    polygon [[-6800,-10100],[-6800,-7750],[-8300,-7750],[-8950,-8350],[-8950,-10100]], at least 5.9 m inside the lip; every spot
    6.6 m+ from the lip, 2.2 m+ from the nest ring, 1.7 m+ from the two rocks, 7 m+ from the lookout (checked with node against
    the plateau polygon). The Depot roost's yaw 135 -> 180 (`polish_1.json`; it was 51 deg off the board; holds only while the
    roost builder doesn't move the mast).
- Playtest round 3 (2026-10-27; contract `Docs/Archive/Work/2026-10-27-Playtest/CONTRACT3.md`), the game and UI parts. The user's calls:
  startup logos = a placeholder studio card (renamable) + the Unreal Engine card per Epic's logo rules; Cyrus Teague the required
  second boss (`Docs/Story.md`, `Docs/Progression.md`); menu backdrop = Teropa from orbit. Melee tools per hero: `Heroes.md`;
  the cast pose and Odessa's cards: `EmberPowers.md`; iron sights: `Gunplay.md`; roads, signs, chest spots: `Levels.md`.
  - Startup logos (note 1; `UStartupLogosWidget`, `FStartupLogos` clock): dark ground (`Color::Track`), lead-in 0.5 s, the studio
    card (an original hexagon/diamond/horizon mark; name from `UStartupLogosSettings::StudioName` = "PLACEHOLDER STUDIO", Project
    Settings > Game > Startup Logos or `[/Script/AI_Looter_Shooter.StartupLogosSettings]` in DefaultGame.ini), then "Powered by
    Unreal Engine" (Epic's logo from `/Game/UI/Logos/T_UnrealEngineLogo`, unaltered, at its own aspect, 220 tall; a plain wordmark
    until the user downloads it (white horizontal PNG from Epic's branding page, saved as `Art/UI/Logos/UnrealEngineLogo.png`, README
    there; never recolored or stretched) and Main imports it: TextureGroup UI, no mips; a "Powered by" lockup leaves `bEngineCaption` off),
    stings `UI.Startup.Studio`/`.Engine`, then the cover fades 0.9 s while the menu arrives. Any key or click skips one card
    (`MinShown` 0.35 s); the first launch (no `[Looter.Startup] LogosSeen=True` in the user's GameUserSettings.ini; PIE uses the
    editor's copy) shows all, unskippable. Once a run; never in tests, with `-NoLogos`, or in PIE unless `Looter.Startup.InEditor 1`.
    `Looter.Startup.Replay` plays them on the menu. Cvars `Looter.Startup.Logos/InEditor/FirstLaunchSkips/LeadIn/FadeIn/Hold/
    FadeOut/Gap/SkipFade/MinShown/Reveal`. Deliberate exception to the UI rules: the cover does not call `MarkBackground` (a full-
    screen ground, not panel glass; faded, it would show the menu world loading under the logos). `/Game/UI` is always cooked, which
    covers the logo path (nothing else names it).
  - Main menu (note 2): no glass column; the menu and title live in the left 40% of a 16:9 screen (`ColumnWidth` 768 of 1920; a
    dark shade thins out to 1000 px, `MarkBackground`); Teropa fills the right 60% (the menu camera keeps x < 768 clear:
    `ColumnEdgeNdc` -0.2). Title lockup: painted mark (an ember through a coin's broken ring, cyan horizon), REVENANT
    (`UTransitionScreenSubsystem::GameTitle`, DisplayFont 60/360), orange rule, TEROPA. Main buttons are Bare `ULooterButton`s
    (numbers 01-04, words at 22): lit = a tick rising, an orange wash, orange words (Quit: Worse red), a step of 8 px; keys light
    the focused one, the mouse the hovered one; press = `UI.Menu.Select`. Arrival: shade, title (slide 28), rule drawing out,
    buttons staggered 0.07 s, footer; `UI.Menu.Reveal`; the ember glow breathes; pages fade in. The session picker is a page, not a
    panel. Cvars `Looter.Menu.Reveal.Speed`, `.Lit.Rate`, `.Page.Fade`, `.Ember.Period`. Nav order unchanged: Single Player,
    Multiplayer (disabled), Settings, Quit; the hero select (HeroSelect*) untouched. Tests `Looter.UI.MainMenu.Look`.
  - The menu's world is `/Game/Maps/Lvl_MenuTeropa` (note 2; built by `Tools/Unreal/build_menu_materials.py` then
    `build_menu_teropa.py`; maps by `Art/Textures/MenuTeropa/make_teropa_maps.py`): Teropa from orbit, `M_Planet` an unlit
    translucent ray-traced sphere on a proxy (ground and relief, clouds on their own shell and speed, night-side lights, sea glint,
    air rim and halo, the sun's glare), a grey moon, `M_Starfield` (stars, a galaxy band, the sun). It keeps turning (material time,
    ignores pause). Actors: `Teropa` (tag `MenuPlanet`) centre (91281, 17877, -1437) cm, radius 20000 cm (`PlanetRadius` on the MI);
    camera `MenuView` at the origin, FOV 50, yaw 11.08, pitch -0.88; the sun comes from the right of that view. The planet is never
    closer than 22000 cm to the camera (inside the proxy it vanishes) nor the camera past 1.1e6 cm from the origin (the star
    sphere). The map has no lights and no precomputed lighting; World Settings give `ALooterMenuGameMode`. The menu camera
    (`ALooterMenuPlayerController`) looks through the `MenuView` camera (slow drift, cursor parallax, 5 s ease-in:
    `Looter.Menu.Drift/.Parallax/.IntroSeconds`), else circles the island as before. Cost: aim under 4 ms Medium (a menu); not
    measured here. Config: `GameDefaultMap` = Lvl_MenuTeropa (`LocalMapOptions=?game=Menu` kept), `MapsToCook` lists it beside the
    island and Ransom's Rest. `USessionSubsystem::MenuMap()` is the default map (`OpenMainMenu` and Save & Quit open it);
    `NewGameMap()` is the fixed fallback `/Game/Maps/Lvl_TutorialIsland`, where a session continues when its level is gone.
    `package.ps1`'s smoke test checks `LoadMap(/Game/Maps/Lvl_MenuTeropa`. Tests `Looter.Menu.Teropa.Motion/.Framing/.Scene`.
  - The new game's intro (note 3; the first pass, full cutscenes with animated characters later): ~78 s, once per session, skippable,
    before control and before the cold open, narration captions (no speaker, so they show with Subtitles off; hero-neutral, no
    family words; eleven lines, `DA_Lines_NewGameIntro` from `create_story_lines.py`, the code's `ScriptLines` play when the set
    is missing or short). It ends on the white that the cold open's REVENANT rises through (that is the title card). Part 1 TEROPA
    (28 s, `PlayPlanet`: in the menu world after the hero is picked, the menu's widgets fade aside, a pull-in on the planet then a
    great circle toward the dusk line; cards TEROPA, THE REACHES; `Music.Intro.Planet`) opens the level at its white. Part 2 RANSOM'S
    REST (50.5 s, `PlayRestIfDue`, a `USceneSubsystem` scene on the first level before the cold open; shots Over the Rim, Chapel,
    Home, Grave; cards RANSOM'S REST and the hero's name and role; `Music.Intro.Rest`, whiteout `Music.Intro.Whiteout`). Fallback:
    when the planet part didn't play (the menu isn't Lvl_MenuTeropa, a console new game, an old Skyreach cast-off) Part 2 opens with
    28 s of WESTWARD over the canyon (78.5 s in all). Shots are in layout cm: move one by editing `RestMoves`
    (`NewGameIntroShots.cpp`); `ClearanceLift` raises a shot whose eye path comes within 2.5 m of the ground. Letterbox 10% each
    (`Looter.Intro.Letterbox`). Skips: Part 1: hold E/Enter/Space/pad A 1 s, or Esc (pad Start) twice within 3 s, none in the first
    0.5 s (a skip brings the white up in 0.5 s, opens the level and skips Part 2 too, recorded seen); Part 2: the scene's usual
    skip (hold Interact, Esc twice, `Looter.Scene.Skip`; since round 4 also hold Space/Enter/pad A, so both parts skip alike: `UIntroWidget::SetPollsKeys`); `Looter.Intro.Skip` skips either. Seen = the campaign flag `Intro.Seen`
    (saved with the cold open's end: a quit before that replays it). Hooks: `USessionSubsystem::StartNewGame` calls
    `UNewGameIntroSubsystem::PlayPlanet` (its `Open` callback opens the level at the white); `UColdOpenSubsystem::BeginIfDue` calls
    `NewGameIntro::PlayRestIfDue` first. The planet is found by tag `MenuPlanet` (else a name with Teropa/Planet, the smallest), the
    sun is the scene's first directional light. Commands `Looter.Intro.Play [west]`, `Looter.Intro.Skip`. Tests `Looter.Intro.Script`,
    `.Due`, `.Framing`, `.SkipKeys`, `.Rest`, `.Lines`; `ColdOpenTests`' first-arrival test sets the `Intro.Seen` flag.
    Round 4 (the round 3 play-check's two nits): the intro ends on REVENANT, the title rising through the white (`ReturnFromScene(UTransitionScreenSubsystem::GameTitle())`; with no cold open to take the white it rises through it anyway, logged "nothing took the white").
    Not seen in the engine yet: the Rest's shot framing (an occluding cliff at Over the Rim's start, the chapel's height).
  - Melee (note 5, the heroes' tools: `Heroes.md`): each hero swings their own tool on the right arm; the gun drops out of view and is
    hidden through the swing (round 5; `Heroes.md` "Round 5, swing feel": smear, hit-stop 65 ms / 85 on a kill). The clock: swing 0.5 s, blow at 0.15 s (was 0.45/0.12; the third-person jab follows);
    draw from under the frame, wind-up, accelerating strike with a wrist snap, a landed bounce or a follow-through, return; hit-stop,
    impact and damage unchanged and the same for every tool (never a critical). Unarmed: the arms show for the strike. The tool's own
    sounds, effects and third-person hand (round 4): `Heroes.md` "Melee tools".
    Melee waits while a cast is under way. Fallbacks: hero mesh missing -> `/Game/Art/Melee/SM_Melee_Placeholder` (a path; no such
    asset is made) -> a code mallet (engine cylinder + cube). A forearm tool swings as held until its own mesh is in. Cvars
    `Looter.Melee.GunLower/ArcScale/Wrist/DrawDrop/Recoil/PistonPush/ToolScale/WristBack/Heft.Quick/Heft.Heavy` (heft 0.8 / 1.25).
    Tests `Looter.Melee` (`Tool.Picks`, `.Arcs`, `.Hand`, Gate, Motion, Strike, Reload, Key).
  - Ember cast pose (note 27): `EmberComponent` waits `Looter.Embers.CastLead` before a power fires: `EmberPowers.md` "Cast pose".
  - The main path is Main 1-5, Dead Lift, Back Pay (the second boss, Cyrus Teague the Foreman), Main 6, Main 7: `Docs/Story.md`,
    `Docs/Progression.md`. Mr. Sloane's side mission Lost and Found (after the Old Fare) opens Lost Property (below).
  - Perf after round 3 (road term and roadside included): RR spawn 7.8 ms, p95 8.6 (Medium 1080p). The menu map's cost is unmeasured.
- Playtest round 4 (2026-10-27; contract `Docs/Archive/Work/2026-10-27-Playtest/CONTRACT4.md`; four holding trees, HANDS4 SCENES4 SWIM4 GRENADES4, merged, then fixed after a play-check; tests Swim, BossIntro, Melee, Cards, FiftyTwoPickup, Intro, Grenade pass). The user's picks: melee polish (`Heroes.md` "Melee tools"), boss cutscenes, swimming (note 30: "entering the pond up there, it is as if the water disappears and there are no swimming or underwater effects"), grenade kinds found as loot (Pitch, Scatter, Cinder). Not measured yet: perf with a cinder bed and a scatter brood in a pack (`Tools\perf.ps1`; a brood pops ~30 pieces each, a bed ~14 flames a second).
  - Swimming (`PlayerLocomotionSwim.cpp`, `World/SwimWaterRules`; every number a `Looter.Swim.*` cvar, lengths for the full-size body and scaled by the player's 0.85): water more than chest deep over the feet swims, shallower wades as before: `StartDepth` 160 full-size cm = 136 on the player (the first pass's 135 made the island's 1.2 m pond swim; it wades now); a swimmer stands again where the bed lies less than `StartDepth` - `StandMargin` 15 under the surface, so a pool's edge never flickers. Where: an `ASwimWater` per layout pond (data; the terrain's water mesh has no collision, it is only the look), placed by `build_area_swimwater.py` (`Assets.md` "Swim waters"): Ransom's Rest's Larkspur Tarn (1.7 m: the swim) and the island's ponds; the Wallow's pools (0.3-0.6 m) and creeks wade.
    - Movement: `MOVE_Flying` while swimming (the engine's own swimming needs physics volumes, brushes the build scripts can't make) and the locomotion steers Z each frame: the eye floats `FloatEye` 12 cm over the surface on a spring with a slow bob; the crouch key dives, the jump key rises, at the surface it hops out (`JumpOut` 520 cm/s; a bank's ledge in front is climbed by the traversal catch); swimming forward with the look pitched past `DiveDeadZone` 25 deg (full at `DiveFull` 65) goes that way. Speed 0.5 of the walk, 0.72 with sprint held forward; up and down 230 cm/s; a fall keeps 30% of its speed on hitting the water (`EntryKeep`) and swims once the feet are half a swim's depth under; a jump out can't fall back in for 0.25 s. Third person has no swim pose yet (the run cycle at 0.4x).
    - Guns: lowered out of view; no fire, reload, aim, melee or throw while swimming (each asks `UPlayerLocomotionComponent::IsSwimmingActor`); ember powers stay usable. The first-person hands stroke (breaststroke, scull when treading: `FirstPersonHandsSwim`, `Looter.Hands.Swim.TreadReach/TreadPace`).
    - Breath (cheap and fair; the user's contract said "only if fair"): 22 s with the head under (`BreathSeconds`), 3.5 s to refill (`RefillSeconds`). No drowning damage: out of breath the body is carried up and can't dive until 25% is back (`BreathBack`); surfacing below 45% gasps (`GaspBelow`). A segmented cyan ring under the crosshair while the breath is short (red under 30%).
    - Creatures never follow: a player in a water's deep zone is `OffGround` to a hunter (`CheckHunt`: watched from the shore) and a prowl stops at deep water (`USwimWaterSubsystem::IsDeepAt`).
    - Underwater view (`UUnderwaterViewComponent`, in any `ASwimWater`, swimming or crouched, first or third person): the camera inside the shore and 2 cm under the surface (2 cm over it to leave, so a bob doesn't flicker). `M_UnderwaterPost` on the player's own unbound post-process component (teal fog by distance, caustic shimmer near the surface, a dark edge; after the tonemapper, so the tints read in any exposure; off above water), `SMX_Underwater` pushed (effects and ambience low-passed at 900 Hz, music 2600 Hz, the interface clear), a hum loop, a gulp going under and air coming back, bubbles (a breath every ~4 s, a few off each stroke's push; M_FX_Shape rings, at most 40). Cues `Player.Swim.*`, `Player.Underwater.*` (`Assets.md` "Sounds"). Cvars `Looter.Underwater.FadeIn/FadeOut/Muffle/LoopVolume/BreathEvery/BubbleRise/BubbleGlow`, `Looter.Swim.Debug`. Tests `Looter.Swim.Rules`, `.Shore`, `.Hands`, `.Play`.
    - Not done / unchecked by ear: the mix's low-pass needs the cues under SC_Effects/SC_Ambience/SC_Music; M_UnderwaterPost's HLSL tints (`DEFAULTS` in `build_underwater_assets.py`); a jump out that misses the bank falls back into a swim.
  - Grenade kinds (the numbers and looks: `Gunplay.md` "Grenade kinds"): the kinds are loot. Drops: `AGrenadePickup::SpawnGrenades`, which every kill, chest and bounty drop already calls, rolls the tin's kind (`GrenadeKinds::RollDrop`): salt below player level 2 (`Looter.Grenade.Kinds.FromLevel`; the tutorial teaches the salt jar), from level 2 a special at 35% (`.Share`), split evenly (`.PitchWeight/.ScatterWeight/.CinderWeight` 1). Drop chances are unchanged (`FThrowRules::DropChance`: none when full, so a full player meets new kinds only after throwing).
    - Carry rule (the simple one; the player carries ONE kind): a special tin swaps the kind carried, keeps the count and adds its jar (even at full); a plain salt tin (every salt source: vendor, the porch plate, the first gun, `AddGrenades`) fills whatever kind is carried and never changes it, so a found kind stays until another is found. `GrenadeKinds::Collect`; `UPlayerThrowComponent::CollectGrenades`, `OnGrenadeKindChanged`; the pickup plays `Throwable.Jar.Swap` on a swap. Saved in `FThrowablesSave::Kind` (old save = salt); a tin lying on the ground keeps its kind across a load (`FSavedGrenadePickup::Kind`, `AGrenadePickup::SpawnKind`).
    - Shown: the HUD's word for a special kind (PITCH, SCATTER, CINDER) in its colour left of the key tab, pips in its colour, a pop on a swap (`UHudGrenadeWidget`, reads the kind off the pawn); the pickup feed names it ("+1 Pitch jar"); the third-person hand jar and the pickup tin wear the kind's model; the vendor sells the salt jar and, with a special carried, its row reads "+1 Pitch jar (salt fills it)". Open: chest and bounty item texts still say "1 grave-salt grenade" (`ChestLoot.cpp`, `BountyPayout.cpp`).
  - Boss intros (the user's pick: a short cutscene as each boss fight starts; the user's ask: short; how they run and the three scripts: `Enemies.md` "Boss intros"): the Gravemother, Cyrus Teague and Abel, 8-15 s each (`BossIntro::ShortestSeconds/LongestSeconds`, checked by `Looter.Scenes.BossIntro.Script`), once per session per boss (campaign flag `BossIntro.Seen.<Kind>`, saved with the session), never on a vigil-stone rematch, skippable like every scene (hold E, Esc twice, `Looter.Scene.Skip`), none where scenes are off (tour, perf) or in test levels. The player can't be hurt through an intro and for `GraceSeconds` 1.5 s after (`FBossIntroGuard`: a buckshot once landed 0.5 s after the hand back). Cvars `Looter.BossIntro` (0 turns them off), `.Replay 1` (an intro at every first-fight start, seen or not); commands `Looter.BossIntro.Play <Gravemother|Foreman|Abel>` (near the boss; clears its flag and starts the fight; Abel outside Main 6: `Looter.BossIntro.Reset`, then `Looter.Abel.Fight`), `.Reset` (every intro due again). Tests `Looter.Scenes.BossIntro.Script/.Words/.Due/.Framing/.Play/.Lines`.
- Lost Loot (the user's call, 2026-10-13, so ground guns don't pile up and cost frame time): a gun on the ground lasts 5 minutes
  (not while the player stands within 6 m), then goes to the depot's LOST PROPERTY window. Leaving a map or quitting sends every
  ground gun there; a load never puts one back. 30 spaces, +10 per upgrade (five, bought with silver); the oldest unnamed gun goes
  out when full. `Looter.LostLoot.*`. Round 3 (note 37): the window is shut until Mr. Sloane's Lost and Found is turned in
  (`ALostPropertyWindow::OpenWhen`; `IsShutIn` = a level with a window and none open); while it is shut the ground clock holds
  (`ULostLootSubsystem::IsHeldShut`), so no gun vanishes into a locked window. Guns handed in by a quit-and-load or a trip before
  then go to the (shut) shelves and are kept until it opens. Test `Looter.LostLoot.ShutWindow`.
- The ore lift: `AOreLift` rides as a skippable look-only scene; the body is parked on the far landing at the start (a save,
  death or load mid-ride finds it there). `Looter.Lift.*`.
- Look: stylized realism; the user rejected faceted low-poly. Golden-hour palettes on Ransom's Rest (ochre, olive, sandstone),
  late summer. Loot colours (rarity orange, teal, gold) stay off the world.
- Third person (2026-10-14, `LooterStancePoseCarry`): low-ready after 0.8 s moving without aim/fire (~19 deg down), aim or
  fire snaps back in 0.12 s; sprint = chest carry, both hands turning with the gun (long foregrips drawn in), revolver low
  at the side one-handed; the upper body keeps ~44% of the jog's bounce. The mannequin is animated hidden; Ellis wears
  its pose (2026-10-15, "Character models", "Ellis as the player's body").
- The idle (2026-10-15, the user's picks: "Weary revenant" with breathing, glances around and hand fidgets; the armed
  stance kept; `LooterStancePoseIdle.cpp`, on the mannequin before the crouch, so Ellis copies it): standing still
  (under 10 cm/s, grounded, not crouched, sprinting, sliding or climbing) and unarmed it eases in over 1.2 s, out over
  0.25 s. Its own clock (`DescribeIdle`, the same moment always the same idle): the weight from leg to leg (18 s round,
  held near each end; the hips 2.5 cm over the standing leg, the free hip 3 degrees down, the chest 2 back, the legs
  solved back onto the planted feet), weary (chest 4 degrees forward, shoulders rounded 5, neck 7 down, head 2 back up),
  a breath every 4.2 s (chest 0.8, shoulders 1.3), a glance aside in about three of four 10 s spells (18-35 degrees,
  neck 40%), a finger flex now and then (22 degrees a joint toward the palm), the right thumb in the belt for 5-9 s in
  about two of three 24 s spells (the wrist 15 out, 12 ahead, 8 up from the hips; fingers down, palm to the body).
  First person is unaffected (the arms ride the capsule). Tests `Looter.PlayerAnim.Idle.*`.
- First-person hands (2026-10-14, `UFirstPersonHandsComponent`, the user's pick: full, like Borderlands): arms on the
  camera posed after the gun each frame; per-kind hold table in `FirstPersonHandsGrips` (where tuning happens); the support
  hand follows reload parts and carries the jar. SK_FP_Arms (approved 2026-10-14; `Art/Models/Characters/FPArms.py`,
  sheet `Saved/Concepts/Hands/fp_arms_sheet.png`) at `ArmsScale` 0.88 (the user: hands looked big). The rig reads bone
  positions only (the import turns bone axes). Hip holds (`Weapons/WeaponHipHold.cpp`; the revolver's in
  `create_revolver.py`): grip cm ahead/right/up, rifle 50, 23, -23.5; shotgun 30, 22.5, -23; revolver 50, 22, -21 (the
  user's Borderlands framing: low right, both gloves in view; `Looter.Player.Hands.OnGun` keeps the support grip point inside
  0.85 of the half-frame, the trigger's inside 1.0 (but for the shotgun's); the Borderlands framing nudges these, below). Long guns level (no muzzle-down tilt: the user, "the AR is
  tilted forward"). Support hand palm up under the fore-end, forearms from below so the sleeve's cut end (SK_FP_Arms
  ends at the elbow in a capped cut) stays under the frame (`ForeForearm` in `FirstPersonHandsGrips.cpp`).
- Long forearms (2026-10-22, `Saved/FPHands/PLAN.md` fix E): every SK_FP_Arms* forearm (elbow to wrist) is 1.25x Ellis's,
  32.62 cm (`FPArms.py` `FOREARM_STRETCH`; 26.10 real), so the framing can slant it with the elbow out of the frame.
  Shoulder, upper arm, elbow and the hand's rest turn are Ellis's; the wrist (hand with it) moves out along the forearm's
  line. Hands, cuffs, gauntlets, upper arms unchanged in size. Triangles unchanged (8018; Odessa 11676, Crane 10724,
  Gauge 10212, Pike 10412): the sleeves' upper-arm rings went 2.8 -> 6.2 cm apart (`UPPER_STEP`) to pay for the
  forearm's. Rig jsons: `lowerarm_cm` 32.62, `lowerarm_body_cm` 26.1, `forearm_stretch`; the code reads the length
  from the skeleton (`FFirstPersonHandsRig::GetForearmLength`). Preview: `Saved/ArtPreviews/Characters/FPArms_Long_<Hero>.png`.
- The heroes' first-person arms (2026-10-20): SK_FP_Arms_Odessa, _Crane, _Gauge, _Pike from
  `Art/Models/Characters/FPArms<Hero>.py` on `Tools/Blender/fp_arms_heroes.py` (+ `fp_arms_base/hands/cloth/paint/gauge.py`;
  FPArms.py's definitions are run up to its build line, FPArms.py never edited). Every one is on SK_FP_Arms' skeleton bone
  for bone (`Intermediate/FPArms/FPArms_<Hero>_rig.json` = FPArms_rig.json), so the rig, the grip tables and the motions
  serve all five unchanged; palms' undersides kept within 2 mm of Ellis's (the 3.6 cm grip offset). Looks: each hero's own
  procedural looks (from their model's tables) baked onto these meshes raised 1.5 m (dust and mud go by height), then fitted
  to first person (`BakedSet.adjust`: the 3P creases read as crazed plastic a foot from the eye: bumps 0.3-0.85, skin and
  glove colour softened), then the hand's detail painted (seams and points on a glove; nails, knuckle wrinkles, palm lines
  on bare hands); texels baked tilted past 60 degrees (decimation fold-overs) are flattened. Gauge's three fingers: blocks on
  `thumb_*`/`index_*`/`middle_*`, moved 6-7 mm apart along their curl axes (the skeleton can't be spread); ring and pinky
  bones unweighted. Worn by `UFirstPersonHandsComponent::LoadArms` from the played hero (`UHeroDefinition::FirstPersonArms`,
  naming rule SK_FP_Arms_<Id>; Ellis's while a hero's aren't in), at `FirstPersonArmsScale` (Odessa 0.84, Pike 0.86, the
  rest 0: the component's 0.88; `create_ember_assets.py` `ARMS_SCALE`); `Looter.Hero` mid-play swaps them (`WearHero`).
  Look sheet: `Tools/Blender/fp_arms_heroes_sheet.py <Hero>` (fp_arms_sheet.py's poser on the three guns at the hip holds
  and drawn sizes) into `Saved/ArtPreviews/Characters/FPArms_<Hero>/`. Export `artrun.ps1 -Export FPArms<Hero> -Family
  FPArms<Hero>`; the bake is skipped while `Art/Textures/FPArms<Hero><Set>/bake.digest` matches. Test
  `Looter.Player.Hands.HeroArms`. Wiring notes: `Saved/FPArms/WIRING.md`.
- The shotgun (the user, 2026-10-15): 20 cm nearer than the rifle ("too far forward making the arm look silly"): the
  stock leaves the frame past the bottom right and the left arm's reach is short. Every full stock hid the trigger glove
  at the old hold (only the stockless Raider grip showed it), so it sits under the frame now. The left hand cups the pump
  from the lower left, rolled 20 degrees and turned 45 in its own plane, forearm in line (straight down bent the wrist
  square: "angles abnormally"), the pump through the grip's middle, `EHandGrip::Pump` curls (fingers closed round it,
  the thumb straight along its near side: curled, it read as "a nub").
- Tune hands live in PIE: `Looter.HipHold.Kind/Ahead/Right/Up/Pitch/Yaw/Roll`, `Looter.Hands.Kind`,
  `Looter.Hands.Support*/Trigger*` (palm roll/pitch/yaw, forearm yaw/pitch, hold), `Looter.Hands.SupportFingerCurl` and
  `SupportThumbCurl` (every joint, degrees), `Looter.Hands.Scale`; `Looter.Hands.Dump` prints paste-ready table lines.
  The forearm switches barely move a far hand: past the upper arm's reach the shoulder comes to the elbow (90 cm at
  most), so move the hold instead. `Looter.GiveWeapon <gun> <rarity> <level> Slot=Key` picks parts; `play.py equip <n>`
  takes a slot in hand (number keys don't reach the game through input.ps1).
- Borderlands framing (FP-HANDS, 2026-10-22; the user: "hands show too much of the arms, clip into the guns"):
  `Player/FirstPersonHandsFraming` on top of the hip and grip tables; every number a live `Looter.FPHands.*` cvar,
  `Looter.FPHands.Dump` prints bake lines, `Looter.FPHands.Enable 0` = as before. Borderlands at 16:9 (measured, sources
  in `Saved/FPHands/PLAN.md`): muzzle 0.54-0.63 across, 0.52-0.64 down, gun to the bottom-right corner; only gloves and
  wrists show (support glove 0.80-1.0 down; BL3 sometimes a 50-60 deg slanted gauntlet), never an elbow; pistols
  one-handed. Ours now: forearms `FrameForearm` (most slanted way from 50 deg toward straight down whose elbow leaves the
  frame by 0.06 of the half-frame, 0.3 of it turned back toward the eye's plane (0.6 until FPGRIP), at most 60 deg off the table's way);
  per kind (baked in `MakeDefaults`): Rifle -12 ahead -2 right -7 up pitch +5, trigger glove 2.5 cm right until FPGRIP (no finger
  through the Bullpup's thumbhole; its support can't slide back: grip loop at u 37.8); Shotgun -1.5 up, support -3;
  Revolver -5 right +1 up roll +6, one-handed (support parked at 20, -26, -38 cm, eased 0.15 s, back on the gun for
  reloads and throws); SMG -1.5 up, support -2; Sniper 0 ahead +2.5 up pitch +2, support -6; LMG 0 ahead +3 up pitch
  +4, support -6 (to u 38); Launcher +9 ahead +2 right -2 up. Sniper since 2026-10-28: -1.5 up, arms 0.9x (the overnight
  playtest: at the hip, with the 1.2x gun, the scope hid the right third of the screen; framed like the LMG, 4 cm lower; not
  yet seen by the user). "Pull the guns in" (the user, 2026-10-22): a support wrist
  high in the frame (fore-ends ~70 cm out) could only hide its elbow by running straight down, so the rifle, LMG and
  sniper sit nearer and lower with the muzzle tipped up about the grip (never down: the user's 2026-10-14 call), muzzle
  0.60-0.64 down, support glove 0.78-0.84; forearm on screen rifle 20% of the height (was 29), LMG 18 (22), sniper 12
  (19), shotgun/launcher 17, SMG 14 (`Looter.FPHands.Play.OnScreen` logs it); ADS unchanged (sight on the crosshair:
  the aim slerps from the hip hold); sheet `Saved/FPHands/compare2.png`. `ViewFov` 75 (narrower view model, holds
  pushed out by `DepthScale`) flattens the gun but not the arms: left off. Framing checks: `Looter.FPHands.PreviewAspect
  1.7778` draws PIE at 16:9 inside the 2.17:1 editor viewport (bars at the sides; grab the middle: x 0.093-0.893, y
  0.132-0.962 of the window); before/after sheet `Saved/FPHands/compare.png`.
- Shotgun trigger finger (2026-10-28): geometry, not hands. The Ranchhand's trigger face is 13.4 cm (modelled) ahead of
  `SOCKET_Grip`, ~20 hand-cm at the drawn sizes (gun 1.32x, hands 0.88x) against ~10.6 of reach. Fix in `Ranchhand.py`
  (`trigger_group` ~5 cm back), before the shotgun's Tripo bodies. Grip tests check guns at 1.1, not the drawn 1.32.
- Grip contact (FPGRIP, 2026-10-22; the user: "grip the gun better and like an actual military officer"; style pick
  "Modern tactical, not so much hand showing"): each gun kind's handles are rounded boxes (`AddContact`,
  `FirstPersonHandsGripShapes.cpp`, numbers read off the default parts' .py); the rig seats each palm on its handle
  (skin 2.0 cm off the grip point, `GripOffPalmSkin`; gap 0.15, at most 2.5 cm, fading out to 5) and closes every finger
  till it touches (`FirstPersonHandsContact.cpp`, the look sheet's close_fingers; cached while the hand stays put on the
  gun); thumbs: base swing and spread from the table, outer joints bend 80 degrees toward the fingers' way
  (`ThumbFlexToFingers`; straight into the palm they stood off grips like pegs), their metacarpal not counted as touching.
  Trigger finger: on the trigger (searched onto `TriggerAt`, may press its glove against the grip) while firing, aiming
  or 0.6 s after a pull, else straight along the frame (`TriggerDiscipline` 1). Tuned in PIE: trigger thumbs swing 68
  round the grip's back (rifle spread 20, rifle palm turned 15 about the post); support thumbs laid forward along the
  fore-end toward the muzzle on rifle, sniper, LMG (swing -10, spread -45; swung over the top they stood up), pump thumb
  swing 0 spread 8; rifle support palm on the handguard's lower left corner facing up-right (0, 0.55, 0.83), 1 cm back
  of the socket (a full C-clamp, elbow out, showed far too much arm); the rifle's 2.5 cm `TriggerRight` dropped;
  `Looter.FPHands.ForearmBack` 0.6 -> 0.3 (the sleeve's cuff hid the glove). Tune: `Looter.Hands.Kind`, the touch cvars
  (CODEMAP), `Looter.Hands.Grip.Report` (each hand in gun cm: palm gap, touched fingers, curls, gaps, trigger miss),
  `Looter.Hands.Grip.Debug 1` (boxes; drawn in world, off the view model's projection), `Looter.Hands.ThumbFlexToFingers`
  (read at bind: `Looter.Hero Ellis` rebinds). Inspecting a grip: the `Looter.HipHold.*` nudges turn the gun (Yaw -80
  shows its left side). "Lift slightly" (the user, 2026-10-23; play-check "the Sniper is held one-handed": the trigger
  glove sat under the frame): sniper and LMG back out to the table's distance and 2.5 / 3 up, so the trigger glove shows
  at the bottom edge; the Bullpup's trigger hand stays hidden behind its shell at any lift (13 cm up still showed none),
  so the rifle keeps its pull-in. Known: the Ranchhand's trigger is 13 cm ahead of its wrist (finger can't reach; hand under the
  frame); a thumb pushed further forward on the Bullpup's post sinks its root in. Grabs `Saved/Screenshots/Tools/fp_grip_<Kind>_hip|ads.png`.
- Gun sizes (the user, 2026-10-14): drawn bigger than modelled, everywhere: rifle and shotgun 1.1, revolver 1.2
  (`UWeaponModelComponent::ScaleFor`; sockets scale with it). The pistol shows as "Revolver" (DA_Revolver's display name;
  Drover stays its parts' and files' name).
- Swaps (2026-10-14, `Player/WeaponSwapMotion`, `Inventory/WeaponManagerSwap.cpp`): the gun in hand drops and tips out
  of view (~0.19-0.24 s by handling, `Weapon.Holster` / `Weapon.Revolver.Holster`), the guns change out of sight, the
  next comes up with an overshoot and settle; a second press mid-holster retargets; no sights while swapping;
  `Looter.Swap.Motion 0` turns it off. Third person: the right arm lowers the gun, the left lets go.
- Water (2026-10-14, the user's concept, clear teal): water meshes carry vertex R shore, G depth, B/A flow (layout x, y);
  `M_Water` reads them (opaque, lit, 3 taps; defaults tuned: RippleScale 0.2, RippleStrength 0.3, Roughness 0.12, SkyTint
  0.35, DepthPower 0.6, shallow #43918A, deep #184A54); `M_Waterfall` streaks in world metres; `M_WaterMist` (Opacity 0.35,
  Brightness 1.6). Rebuild only the water: `$env:LOOTER_TERRAIN_ONLY='water'; Tools\models.ps1 -Only <Areas>`, then
  `build_area.py <Area> water` (re-places the falls in `Effects/Falls`). terrain_identity differs only in `<Area>_Water`
  until committed. Splash rings and wake: `FWaterRings`. Swimming in it and the underwater look (round 4): the "Playtest round 4" bullet above.
- Player numbers (2026-10-13): 0.85 of the mannequin's size and 0.9 pace on top (the user: "pace of play is super fast"): walk
  459, sprint 711, crouch 230, slide 783 cm/s; eye about 140 cm; jump 103.5 cm. A slide comes out of sprint plus crouch. Chasing
  creatures scale with `LooterPlayerSize::MoveScale` (size and pace); slimes don't.
