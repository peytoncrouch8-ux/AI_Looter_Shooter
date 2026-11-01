# Lane HOOKS: the cross-lane hooks between STORY, WORLD, CREW, SOUND (2026-10-31)

Nothing built, run in Unreal or committed. Edit/Write only. Every touched script compiles (UE's python `py_compile`) and
has no undefined names (a symtable check); `layout.json` and `views.json` parse. DREDGER's and FLIGHT's files untouched;
C++ touched only at the one ColdWelcome line.

## Applied

1. **`Tools/Unreal/build_area_farm.py`** (Delia, CREW + STORY):
   - `delia_topics`: STORY's `texts_topics(build, 'Speaker_Delia')` FIRST, then the later ones (mine, Main 6's "Take him
     the lantern" with its `Delia.Main6` event, Main 7). WORLD's version returned STORY's topics ALONE once they existed,
     which would have dropped Main 6's start event: fixed. Default set from `TOPICS['Speaker_Delia']['default']`.
   - `ADeliaRansom` at porch floor **+ 68 cm** (`DELIA_HALF_HEIGHT`, her capsule's middle), facing out over the yard;
     name "Delia Ransom"; topics on her `speaker_point` (confirmed: `USpeakerPointComponent* SpeakerPoint` in
     DeliaRansom.h). The screen-door `ASpeakerPoint` is placed only while her class is missing (fallback), so with CREW's
     class built `Speaker_Delia` is on her alone. Docstring updated.
2. **Hob's Landfall perches, one set** (`build_area_story.py` `place_hob` + `story_texts.LANDFALL_PERCHES`):
   - STORY's `LANDFALL_PERCHES` (steps and words) is the only list; WORLD's own list, `landfall_perch()` and the
     `PERCHES` keys `landfall*` are gone. Spot names map onto WORLD's measured spots: `porchRail` = the porch beam's end
     (`build_area_landfall.hob_on_porch`), `mooringLedge` = the winch top, new `abelBoard` = Abel's headboard.
   - Result: step 4 winch (`DA_Lines_LandfallAnchor`), 5 lookout rail (`LandfallClimb`), 6 rail (`LandfallPoint`), 7 Abel's
     headboard, silent (STORY had `path`; now `('abelBoard', 'path')`, WORLD's spot: the trail runs past it), 8 porch beam
     (`HobLandfallPorch`); after Main 1 on the porch beam (else path, else headboard), where Main 2's offer (his) finds the
     player. Hob is still placed on Abel's headboard with Ellis's grave gone (WORLD's), so Main 2 has its giver.
   - A warning if a perch has no spot, or if `LANDFALL_PERCHES`' steps aren't Main 1's data's (anchor, point, spyglass,
     trail, porch from build_area_landfall's `mission_steps`).
3. **Abel's hat, one readable** (`build_area_landfall.py` `place_hat`): STORY's `READABLES['AbelsHat']` (WORLD looked for
   `'Landfall_AbelsHat'`, which doesn't exist: the hat would have had WORLD's fallback words and id). Id `AbelsHat`, tags
   `Landfall_AbelsHat`, `Readable`, `Readable_AbelsHat` (AReadableProp's own `TagFor`), STORY's `shown_when` (Main 1 from
   step 7, until taken), `hold_to_read` True, flag `Landfall.AbelsHat`. WORLD's fallback words removed (no readable = a
   warning, no hat). Mesh: `story_texts.READABLE_MODELS['AbelsHat']` = **SM_SheriffHat** (one name to swap), also on
   Delia's chair. Not WORLD's SM_AbelHat: it's the ghost Abel's hat (Abel.py: M_Ghost vertex zones), a ghost's hat on a
   sunlit path. If Main finds SM_AbelHat solid enough, swap the one name. `STAND_INS['abel_hat']` is only a fallback.
4. **Encounter timings, one source** (`build_area_landfall.py` `encounter_when`): `LedgeClimbers` and `FarmPorch`
   `active_when` = `story_texts.ENCOUNTERS[id]` (STORY's: from 4 before 5; from 7 before 9), else the data's steps; a
   mismatch is logged. Today both agree.
5. **Porch plate**: no edit needed. WORLD's `porch_plate_when` already takes STORY's `take_when=dict(after=['Main1'])`
   (a free pickup after Main 1; Delia's turn-in lines mention it). Fixed STORY's leftover `eat_lines`
   ('Grandma Delia', '...Thank you, Ellis.') -> ('Delia Ransom', '...Thank you, Warden.').
6. **`create_bestiary_pages.py`** (CREW): Delia's page (name "Delia Ransom", habitat "Ransom Farm: her porch", CREW's
   description and notes); new Friends `DA_Bestiary_Holloway`, `_Finch`, `_Junie` (STORY_CHARACTER, ledger_only, known
   after Main 1, no model), sort 1-3 after Hob; Tilly..Thaddeus shifted to 4-9 (Looter.Bestiary.Entries checks order);
   docstring. DREDGER's two entries untouched.
7. **`Art/Levels/RansomsRest/layout.json`** (WORLD): zones `landfallLedge` (polygon) and `landfallPorch` (at, r 2600),
   levels 1-2, before `west`. `gameplay` is build-only (`area_shape.BUILD_ONLY`): no terrain staleness.
8. **`Art/Levels/RansomsRest/views.json`** (WORLD): LedgeMoored, LedgeFromStairs, SpyglassBarge, FarmBarge,
   SinkUnderBarge before DeckFight_Dusk (still last). **TrailFromPoint left out**: its `Looter.Landfall 7` is FLIGHT's
   (unbuilt) and moves the player/ship; add it once that command is known not to move the tour's camera.
9. **`Tests/ColdWelcomeTests.cpp:595`** (WORLD): the farm's safe zone is `after Main1` now (WORLD's exact line).
10. **`create_ember_assets.py`**: DA_Hero_Ellis role **"the Lantern Warden"**, story "The Lantern Wardens' newest sworn
    hand: says little, misses less. Where the Great Lantern leans, Sully goes, and the stolen dead come home." (no
    pronoun, no Ransom tie; passes HeroTests' "the ..." and >40 chars). Ids, powers, colour kept.
11. **Docs**: `Docs/Progression.md` Main 1's three rows (givers table, pacing table with the 16 kills "to measure",
    purse table) + a note that later pacing rows are low until the test's new sums are copied in; `TRIPO_MODELS.md` #19
    Abel's hat + the SK_Mannequin skeleton rule (humanoid A-pose for auto-rigging); `Docs/Pipeline/Game.md` the turn-in
    card bullet (TURNIN's hook 3); `Docs/Pipeline/Heroes.md` the roles line (Sully, the Lantern Warden) and a section
    "Story people's bodies" (CREW's hook 3).

## Left for Main (and why)

- `git mv Source/AI_Looter_Shooter/Tests/SevenDaysTests.cpp Source/AI_Looter_Shooter/Tests/LandfallStoryTests.cpp`
  (no deletes with Edit/Write); CODEMAP line in STORY.md.
- C++ (REVIEW is in the C++ now; not mine): `UI/Menus/HeroSelectWidget.cpp:234` subtitle "Whoever you pick is the
  Revenant Sexton raised." now contradicts Sully's card; `Embers/HeroDefinition.h:47` comment ("the Revenant");
  `MissionRewards.cpp:372` comment -> "Delia Ransom, on her porch" (STORY); `NoticeBoardWidget.cpp` HandleOfferClosed
  (TURNIN, optional); `Combat/EnemyProjectileSubsystem.cpp` Victims + ADeliaRansom (CREW, optional).
- Checked, no edit: TURNIN's flow keeps STORY's hat variant (`TalkNow` takes `GetLinesNow` before the Talked event turns
  Main 1 in); `LooterSoundCue::TurnInStamp` is top-level in LooterSoundCues.h (TURNIN's build blocker is clear).
- Later rounds: Hob's page ("Only you can hear him", "My messenger"), Abel's ("your father, friend"), Sexton's-voice
  Ledger pages; the other four heroes' Revenant stories; Main 2's words.
- Handoff, CODEMAP: Main's.

## Run order (fits CONTRACT's)

1. Build. 2. Sounds (SOUND.md). 3. `create_story_lines.py`, `create_mission_assets.py`, `create_side_mission_assets.py`,
`create_hero_lines.py`, `create_ember_assets.py`, `create_bestiary_pages.py`. 4. `Tools\console.ps1 "py
C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"` (the new level zones -> DA_Area_RansomsRest).
5. `Saved/landfall_probe.py`, `build_landfall_materials.py`, `build_area.py RansomsRest environment`, then `gameplay`
(WORLD.md order); read the log for: "Hob: no spot for Landfall's perch", "LANDFALL_PERCHES' steps", "ENCOUNTERS says
steps", "no ReadableProp.hold_to_read", "no story_texts.READABLES". 6. Then loot/fauna; delete `DA_Lines_DeliaMain1`,
`DA_Lines_HeadboardEllis`, `DA_Lines_HobWakes` (after the rebuild only).
7. Tests (besides the lanes'): `Looter.Story.ColdWelcome`, `Looter.World.RansomsRest.InsideBoundary` (new views; the
ledge climbers' spots sit at the drop's edge: may fall outside the boundary), `Looter.Bestiary.Entries`, `Looter.Heroes`,
`Looter.Areas.Bands`, `Looter.Creatures.AreaLevels`, `Looter.Progression.Pacing`.

## Risks

- A tour from the command line plays a new game, so the ship is at `Course[0]`, not moored: LedgeMoored/LedgeFromStairs
  won't show her on the ledge unless FLIGHT's start differs for tours.
- SM_SheriffHat for Abel's hat also stands in for the sheriff's own hat (the den's): two alike hats until Tripo #19.
- PIE: Hob's porch perch faces the house's yaw; Delia's 68 cm assumes `onto_host` found the porch floor (else socket -
  150 cm + 68).
