# LINES-WIRE holding tree (2026-10-20)

Implements `Saved/Lines/DESIGN.md`: the played hero's words. Nothing under `Source/` was touched, nothing built, no
editor run, no git state changed. Every replaced file was copied from `Source/` at HEAD 52946bb (the blob hashes below
still match HEAD after that commit; it touched none of these files) and edited; `Patches/<path>.patch` is each one's
unified diff (`git apply --check` passes on all 23 against the current tree).

## Files

### New (copy into `Source/AI_Looter_Shooter/`)
- `Story/HeroLineTable.h` / `.cpp`: `FHeroLineVariant` (Id, Source, Speaker, Other, Heroes, bDropForOthers) and
  `UHeroLineTable : UDataAsset` (Lines; `AssetPath()` = `/Game/Data/Story/DA_HeroLines.DA_HeroLines`; editor-only
  `PostEditChangeProperty` calls `HeroLines::ForgetTable`).
- `Story/HeroLines.h` / `.cpp` (namespace): `GetPlayedHeroId`, `SetPlayedHero`, `IsDefaultHero(Id)`, `Resolve`,
  `ResolveString`, `ResolveLine(s)`, `Resolve*For(HeroId, ...)`, `PickWords`/`Pick` (the precedence on one row),
  `SourceOf` (FTextInspector source string), `TokenNames`, `FillTokens`, `Initialed`, `HeroVariantPath` (painted assets),
  `GetTable`, `FindRow`, `ForgetTable`, `FScopedHero`, `FScopedTable` (tests).
- `Tests/HeroLinesTestKit.h` (OtherHeroes, `FFamilyWords`: DESIGN's regex + allow-list "Tilly's Pa's car").
- `Tests/HeroLinesTests.cpp`: `Looter.Lines.Table`, `Looter.Lines.Precedence`, `Looter.Lines.Wiring`.
- `Tests/HeroLinesFamilyTests.cpp`: `Looter.Lines.NoFamily`.

### Replaced (whole new version here; base blob = `git hash-object` of the original)
| File | Base blob | Change |
|---|---|---|
| Story/CaptionSubsystem.cpp | a03425e2 | `Play` resolves the lines (`HeroLines::ResolveLines`) before the queue |
| Embers/HeroDefinition.h | 9c998a8d | `UPROPERTY FText ShortName`; `FText GetShortName() const` |
| Embers/HeroDefinition.cpp | a6a81549 | `GetShortName` (ShortName, else Id) |
| Session/SessionSubsystem.cpp | edb067ec | `HeroLines::SetPlayedHero(GetPlayedHeroId())` at the end of `BeginPlayWorld` (runs in InitGame, before any BeginPlay) and `BeginMenuWorld` |
| Session/SessionSubsystemHero.cpp | 8631d40c | `SetNoSessionHeroId` sets the words too (Looter.Hero swaps them at once) |
| Missions/MissionDefinition.cpp | b4c7ba1c | `GetGiverName`, `GetOfferGiverName` resolve the data's GiverName |
| Missions/MissionObjective.cpp | 0d94252d | `GetDisplayText`, `GetShortText` resolve Text/ShortText before `MissionText::ResolveKeys` |
| Missions/MissionPlayerObjectives.cpp | d734954d | collect-item `DescribeRule` resolves the item's name |
| World/MissionItem.cpp | d85c3d21 | `GetItemName` (DisplayName and Info->Name) and the `Prompt` resolved |
| World/ReadableProp.h | 9d9b8878 | `TMap<FName, TObjectPtr<UStaticMesh>> HeroMeshes`, `TObjectPtr<UStaticMesh> OtherMesh`, `PickMeshFor(HeroId)` |
| World/ReadableProp.cpp | ec86b9c8 | BeginPlay swaps Mesh to `PickMeshFor(played hero)` (own, else OtherMesh for non-Ellis, else as placed) |
| World/WantedPoster.cpp | 1a350b95 | decal and scrap materials: in a game world, `MI_PosterDecal_<Id>` / `MI_PosterScrap_<Id>` first when that package exists, else today's |
| UI/World/MissionOfferWidgetCard.cpp | be5c799b | `GiverNameOf` (offer giver, board/artifact titles), Briefing (artifact text/briefing/summary), choice question/titles/lines, reading card Heading/Title/Text |
| UI/Inventory/MissionsWidgetDetails.cpp | 41f3ae53 | Summary |
| UI/Inventory/MissionsWidgetFeatures.cpp | 6126b432 | carried item Name/Line, choice Title/Line |
| UI/World/NoticeBoardWidgetCards.cpp | 1575dc14 | Summary |
| UI/Bestiary/BestiaryWidget.cpp | fc355bfe | list card DisplayName, Kind |
| UI/Bestiary/BestiaryWidgetDetails.cpp | 939cb132 | Kind, DisplayName, Habitat (both), Description, Notes |
| UI/HUD/HudBossBarWidget.cpp | 8d58fe01 | `MakePhaseLine` resolves before `ToUpper` |
| Economy/EconomyRules.cpp | 78c3e231 | NamedNotForSale pattern resolved, then `FText::Format` |
| Audio/CreatureVoiceDirector.cpp | 9afeed76 | bark text `HeroLines::ResolveString`d (murmur follows); empty -> no bark |
| World/TownLifeRules.cpp | 22f8e08c | Hollis -> Harlan (two mutters, every hero) |
| Tests/TownLifeTests.cpp | 350cc0f5 | expects "Bar the door, Harlan." |

### Written in place (outside Source)
- `Tools/Unreal/hero_lines.json` (new): VARIANTS.json with (a) a `decisions` block recording Main's five provisional
  calls and where each changes, (b) the two Hollis mutter rows removed (the rename is in C++), (c) `about` updated.
  146 text rows + 3 painted. Checked offline: every `current` exists word for word in the scripts/C++; no duplicate
  (ignoring case); no family word in any non-Ellis result; tokens valid, `{0}` kept.
- `Tools/Unreal/create_hero_lines.py` (new): writes DA_HeroLines (raises, saves nothing, on a duplicate current, a bad
  hero id, an unknown token or a lost source token; `check()` tested offline with a stub). Prints `HEROLINES done: N`.
- `Tools/Unreal/build_area_vendors.py` (edited, one word): Pruitt's greeting "says Hollis Pruitt" -> "Harlan Pruitt".

## Deviations from DESIGN.md (all small; say if Main wants the design's way)
1. Tests: while `GIsAutomationTesting`, `GetPlayedHeroId` is Ellis unless an `FScopedHero` is active. So no existing
   test needs editing (DESIGN wanted FScopedHero in each; ~13 files read Ellis's words: SevenDays, ColdOpen, ColdWelcome,
   TalkBusiness, Story, Gravewind, HallowedGround, MissionItemPrompt, MissionBoardNote, Subtitle, AbelFight, Abel,
   TownLife). FScopedHero overrides it (my tests use it).
2. Row `Speaker` (from json `speaker`): `ResolveLine` applies a row with a speaker only when the line's source speaker
   matches (Ellis's "Who?" / "Riding." stay as they are in anyone else's mouth). `Resolve(FText)` ignores it. Row `Id` kept.
3. More read sites than listed: board/artifact titles, choice question/titles/lines (offer card and Missions page),
   collect-item DescribeRule. Cheap and future-proof.
4. Table kept rooted once loaded (AddToRoot, like the UI fonts); `SetPlayedHero` (each level/menu start) and
   `PostEditChangeProperty` force a fresh look-up, so a table made mid-session is found.
5. Poster hero materials only in game worlds (an editor-placed poster saves its MID; it must stay Ellis's).
6. No change to the cold open, Sit with Pa, Abel, roosts, mutters, ore lift C++: all play through `UCaptionSubsystem::Play`.

## Run order after the merge
1. Copy the 8 new files and the 23 replaced ones into `Source/AI_Looter_Shooter/` (or `git apply Saved/LinesWire/Patches/*.patch`
   + copy the new files). Re-check base blobs first if anything moved (`git hash-object`; table above).
2. `Tools\launch.ps1 -Build`.
3. `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_hero_lines.py"` -> `HEROLINES done: 146`.
4. With Lvl_RansomsRest open: `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_vendors.py RansomsRest"`
   (re-places Gameplay/Vendors with the Harlan greeting; saves). Check what else it overwrites first.
5. After POWERS lands its create_ember_assets.py: add `asset.set_editor_property('short_name', unreal.Text(spec['short']))`
   in `setup_hero` and a `short` per hero (Ellis, Odessa, Crane, Gauge, Pike); run it. Optional: empty ShortName reads the Id,
   which is the same word for all five today.
6. `Tools\runtests.ps1` (all `Looter.*`; new: `Looter.Lines.Table`, `.Precedence`, `.Wiring`, `.NoFamily`).
7. Look: `Looter.Hero Odessa` (Looter.Hero.Unlock 1 if she isn't playable) + `Looter.PlayLevel /Game/Maps/Lvl_RansomsRest`,
   Main 1-3 in PIE: headboard card, Delia, Sexton's deal, barks, Ledger.
8. Later (art agent, separate): Graves.py SM_Headboard_<Hero>, MissionProps.py SM_KeepersRoll_Bare, looter_posters.py
   `--hero`, build_decal_materials.py MI_Poster{Decal,Scrap}_<Id>; then build_area_story.py `place_headboards`
   (`hero_meshes`) and build_area_missionprops_valley.py `readable('KeepersRoll')` `other_mesh`. Until then non-Ellis
   heroes read the right words over Ellis's carved board and poster (code falls back).

## Overlaps with Saved/Powers
- No file in both trees. Powers replaces EmberCommands/EmberComponent/HealthComponent/BulletSubsystem/LooterSoundCues/
  HeroTests (+ its Embers/Powers files); I replace none of those.
- Shared ground: `Tools/Unreal/create_ember_assets.py` (Powers edits it in place now; I did not touch it, step 5 above);
  `Embers/HeroDefinition.h` (I add ShortName/GetShortName; Powers only includes it). Powers' HeroTests calls
  `SetNoSessionHeroId`, which now also sets the words; it restores its value, so no effect.
- 52946bb (HUD portraits: HudPortraitWidget.*, HeroDevCommands.cpp) touches none of my files.

## Compile risks (unbuilt; check these first)
- `TMap<FName, TObjectPtr<UStaticMesh>>` UPROPERTY with UStaticMesh forward-declared (ReadableProp.h): fine elsewhere
  for TArray; include Engine/StaticMesh.h in the header if UHT/MSVC complains.
- `FTextInspector::GetSourceString` (Text.h), `GIsAutomationTesting` (CoreGlobals.h): checked in the 5.8 headers.
- Shadowing is an error here (/we4456 /we4458 /we4459): locals were named to avoid globals and members (checked by hand).
- HeroLinesFamilyTests reflection: `FTextProperty::GetPropertyValue`, `FScriptArrayHelper(const FArrayProperty*, const void*)`,
  `FObjectProperty::GetObjectPropertyValue`, `ContainerPtrToValuePtr<void>(const void*, int32)`, `FProperty::ArrayDim`
  (public in 5.8): all checked in UnrealType.h. `GetDefaultObject()` on `const UClass*`.
- JSON in Looter.Lines.Table: `TryGetArrayField`, `TryGetStringField`, `HasField`, `GetObjectField(...)->Values`.
- `Looter.Lines.Wiring` spawns an AReadableProp in an EditorPreview FTestWorldWrapper world (no BeginPlay); loads
  /Engine/BasicShapes/Cube and Sphere.
- `FRegexMatcher::GetCaptureGroup(0)` for the whole match (ICU group 0).
- Python: struct property names `id`, `source`, `speaker`, `other`, `heroes` (dict of unreal.Name -> unreal.Text),
  `drop_for_others`; `unreal.HeroLineVariant()`. If set_editor_property on the map fails, build `unreal.Map(unreal.Name, unreal.Text)`.
- Lengths: WantedPoster.cpp 507, SessionSubsystem.cpp 505 lines (were 492/501).

## Runtime notes
- `Looter.Lines.NoFamily` walks the line sets, missions (+ objectives as shown, givers, turn-in text), Ledger pages,
  named guns (+ CanSell refusal), Lvl_RansomsRest's game-class actors (reflection over FText, structs, arrays, game
  subobjects), every game actor/component class default, and the C++ lines; it reports up to 60 misses. Offline scans
  found no family word outside the json rows except "Tilly's Pa's car" (allowed); a miss after the merge most likely
  means a placed actor's words differ from the scripts (rebuild that piece) or a key drifted.
- Ellis plays exactly as before: every resolve returns its input for Ellis.

## CODEMAP lines
- Add after `Story/StoryLineSet.h`: `- \`Story/HeroLineTable\`: \`UHeroLineTable\`, \`FHeroLineVariant\`: every hero
  variant of Ellis's words as one data asset (\`/Game/Data/Story/DA_HeroLines\`, from \`Tools/Unreal/create_hero_lines.py\`
  and \`hero_lines.json\`). \`Story/HeroLines\`: the played hero (set by the session, Looter.Hero; Ellis in tests unless
  \`FScopedHero\`), \`Resolve\`/\`ResolveLine\`/\`ResolveString\` by a text's source string (precedence: the hero's own,
  dropped, neutral, as written), tokens {Hero} {HeroFull} {HERO} {Initialed}, \`HeroVariantPath\` for painted assets.`
- `Story/CaptionQueue` line: add "`Play` speaks the played hero's words (HeroLines)".
- `Embers/HeroDefinition` line: add `ShortName`/`GetShortName` (the caption's speaker name).
- `World/ReadableProp` part of the World line: add "`HeroMeshes`/`OtherMesh`: the played hero's carving".
- `World/WantedPoster`: add "the played hero's atlas `MI_Poster{Decal,Scrap}_<Id>` once made".
- Tests, story line: add `HeroLinesTests.cpp` (`Looter.Lines.Table`, `.Precedence`, `.Wiring`), `HeroLinesFamilyTests.cpp`
  (`Looter.Lines.NoFamily`; kit `HeroLinesTestKit.h`).
- Tools: `- \`Tools/Unreal/create_hero_lines.py\`: DA_HeroLines from \`hero_lines.json\` (the one place hero variants are
  written; painted entries are the art's).`

## For the docs at merge (Main)
Pipeline: "Writing lines from now on" (DESIGN.md section 5) + the run order; Handoff: the five provisional calls (json
`decisions`), art still owed (headboards, bare roll, four poster atlases), and step 5 (ShortName).

## Phase 2 (2026-10-20, done)
Tree moved into Source (all 23 patches still applied at HEAD 425d1c3; nothing to rebase). Built first time. create_hero_lines.py
146 rows; create_ember_assets.py now writes `short_name` (= id); Looter.ImportModels ArtExport_LinesArt (56 models, no
warnings); build_decal_materials.py (MI_Poster{Decal,Scrap}_<4 heroes>); build_area.py RansomsRest gameplay (story +
valley + vendors: hero_meshes 4, other_mesh set, Harlan greeting); tests 743/743. PIE as Crane and Ellis checked;
screenshots in Saved/Screenshots/Tools/lines_*.png. CODEMAP lines and a Pipeline section added. Nothing committed.
