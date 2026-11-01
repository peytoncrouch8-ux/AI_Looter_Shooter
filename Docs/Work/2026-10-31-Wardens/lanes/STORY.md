# Lane STORY: Landfall's words and data, the planet intro, the rename (2026-10-31)

Nothing built, run in Unreal or committed. Python files syntax-checked with UE's own python (compile only). Step indices
counted from 0 throughout, as the scripts count.

## Files changed

- `Tools/Unreal/create_mission_assets.py`: Main1 -> **"Landfall"** exactly per the contract's table (constants
  `LANDFALL_PROW`, `DECK_CLEAR`, `LANDFALL_GRAPPLE`, `HARD_LANDING`, `LANDFALL_WINCH`, `LEDGE_CLIMBERS`, `LANDFALL_SPYGLASS`,
  `LANDFALL_HAT`, `FARM_PORCH`, `DELIA_TAG`); new summary; turn-in `Speaker_Delia`, giver "Delia Ransom", **no lines**
  (her topics say them, see "The hat variant"); `experience=450`, purse stays `SilverFor` 20. Header doc rewritten.
  - 0 Reach `Lamplighter_Prow` r 350 · 1 Event `Landfall.DeckClear` (waypoint none) · 2 Interact held `Landfall_Grapple` x2 ·
    3 Scene `HardLanding` (waypoint none) · 4 Interact held `Landfall_AnchorWinch` + Clear `LedgeClimbers` 4 · 5 Reach
    `Place_RansomsPoint` r 1800, height counted · 6 Interact `Landfall_Spyglass` (tap or hold) · 7 Reach `Speaker_Delia`
    r 1800 + optional Interact `Landfall_AbelsHat` · 8 Clear `FarmPorch` 8. Tracker words = the contract's; full sentences
    say why.
- `Tools/Unreal/create_story_lines.py`: speaker constants `HOLLOWAY` "Captain Holloway", `FINCH` "Brother Finch", `JUNIE`
  "Junie Calloway", `VANE` "Cornelius Vane", `DELIA` "Delia Ransom", `SULLY` "Sully"; all Landfall sets (below);
  `DA_Lines_NewGameIntro` lines 0-3 new (11 kept); `DA_Lines_DeliaPorch` reworded ("Go on, Warden. I'll keep the
  porch."); `DA_Lines_HobMain1` reworded. **Retired** (removed from SETS; delete the assets after the gameplay rebuild):
  `DA_Lines_DeliaMain1`, `DA_Lines_HeadboardEllis`, `DA_Lines_HobWakes`.
- `Tools/Unreal/story_texts.py`: `SULLY`, `ABELS_HAT_FLAG = 'Landfall.AbelsHat'`; READABLES `AbelsHat` (new) and
  `HeadboardEllis` marked retired; READABLE_MODELS `AbelsHat: SM_SheriffHat` (STAND-IN, one name to swap); PORCH_PLATE
  `take_when = dict(after=['Main1'])`; TOPICS `Speaker_Delia` (new); `LANDFALL_PERCHES` (new); ENCOUNTERS `LedgeClimbers`,
  `FarmPorch` (new).
- `Tools/Unreal/hero_lines.json`: decision `6_sully`; Ellis's token names -> Sully / Sully Graves / SULLY GRAVES /
  S. Graves (doc only: Ellis always reads as written); new row `speaker.Sully` (current "Sully", neutral "{Hero}").
  Old Seven Days rows (lines.DeliaMain1.0, Main1's summary and turn-in rows) left: harmless, never fire.
- `Tools/Unreal/create_ember_assets.py`: `hero(..., short='')`; DA_Hero_Ellis display name **"Sully Graves"**, short name
  **"Sully"** (id, assets, paths unchanged). Role "the Revenant" and the story blurb (Sexton, Pa's lantern) NOT changed
  (contract: name only) — they now contradict the story on the pick screen: a later round.
- `Source/AI_Looter_Shooter/Story/NewGameIntro.cpp`: `ScriptLines()` L0-L3 (the planet part) new words; **timing table
  `PlanetCaptions()` adjusted** (same six slots, same order): Teropa card 1.0 s for 3.0 (was 3.8), L0 4.2/4.4, L1 8.8/4.1,
  Reaches card 13.1/3.0 (was 3.4), L2 16.3/4.6 (was 3.5), L3 21.1/4.9; all end by PlanetWhiteAt 26. L4-L10 (the Rest part,
  no longer played for a new game) unchanged.
- `Source/AI_Looter_Shooter/Tests/SevenDaysTests.cpp`: rewritten as `Looter.Story.Landfall.Mission` and
  `Looter.Story.Landfall.Lines` (Seven Days' tests gone).
- `Source/AI_Looter_Shooter/Tests/PacingPaths.h`: `MainOneStop(const FPathCast&)` "Main 1, Landfall" with its kills
  (deck 3 riflemen + 1 hose-man at layout (-10100, -22200); LedgeClimbers 4 Unpaid at (-10100, -10200); FarmPorch 3
  riflemen, 2 hose-men, 3 Unpaid at the farmhouse (-4000, -5800)); `FPathCast::DredgerRifleman/DredgerHoseMan` found by
  class name (`ScriptCreatureOf`: `/Script/AI_Looter_Shooter.DredgerRifleman` / `.DredgerHoseMan`, null until DREDGER
  builds them, so no include of DREDGER's headers). Both callers updated.
- `Docs/Areas/RansomsRest_Expansion.md`: M1 row and "Main 1: Landfall" section rewritten.
- `Docs/Story.md`: new top section "The Wardens story (2026-10-31)" (premise, cast, what replaced what, what's still
  Revenant); the rest untouched.
- Not changed: `Tests/MissionTests.cpp` (Main1 still automatic and turned in: nothing breaks), `Tests/TalkBusinessTests.cpp`
  (makes its own Main1 in code).

CODEMAP (Main): suggest `git mv Source/AI_Looter_Shooter/Tests/SevenDaysTests.cpp Source/AI_Looter_Shooter/Tests/LandfallStoryTests.cpp`
and the line "Tests/LandfallStoryTests.cpp — Looter.Story.Landfall.Mission/.Lines: Main 1 Landfall's asset (steps, tags,
turn-in), a played-through copy (holds, the hat, Delia's topics), its line sets' counts, voices and lengths".

## Line sets (name: lines; who plays it, when)

| Set | Lines | Played by, when |
|---|---:|---|
| `DA_Lines_LandfallDeck` | 5 | FLIGHT: as control starts on deck (sailing) |
| `DA_Lines_LandfallVaneHorn` | 3 (all "Cornelius Vane") | FLIGHT: `UVaneBroadcast::Say` as step 0 is done |
| `DA_Lines_LandfallBoarders` | 3 | FLIGHT: grapples hit / first wave boards |
| `DA_Lines_LandfallBoardersWave2` | 2 | FLIGHT: second wave (optional extra; skip if absent) |
| `DA_Lines_LandfallGrapples` | 3 | FLIGHT: as step 2 starts (deck clear) |
| `DA_Lines_LandfallLanding` | 2 | FLIGHT: in `HardLanding` (~5 s) |
| `DA_Lines_LandfallAnchor` | 3 | Hob's perch arrival, step 4 (NOT FLIGHT) |
| `DA_Lines_LandfallClimb` | 3 | Hob's perch arrival, step 5 (new set: why the climb) |
| `DA_Lines_LandfallPoint` | 2 | Hob's perch arrival, step 6 |
| `DA_Lines_LandfallVaneValley` | 4 (all "Cornelius Vane") | FLIGHT: `SmogReveal`, Vane over the loudspeakers |
| `DA_Lines_LandfallTrail` | 4 (Finch 2, Holloway, Hob's hat hint last) | FLIGHT: as `SmogReveal` ends |
| `DA_Lines_HobLandfallPorch` | 1 | Hob's perch arrival, step 8 |
| `DA_Lines_DeliaFight` | 8 (all "Delia Ransom") | CREW: `ADeliaRansom` barks while fighting |
| `DA_Lines_DeliaLandfallWait` | 1 | Delia's topic during Main1 before ready |
| `DA_Lines_DeliaLandfall` | 5 | Delia's topic at the turn-in (no hat) |
| `DA_Lines_DeliaLandfallHat` | 5 | Delia's topic at the turn-in with `Landfall.AbelsHat` |
| `DA_Lines_DeliaPorch` | 1 | Delia's default after Landfall |
| `DA_Lines_HobMain1` | 1 | Hob tapped during Main1 (TOPICS, unchanged entry) |
| `DA_Lines_NewGameIntro` | 11 (0-3 new) | the planet intro (code fallback = same words) |

Speakers: crew lines name "Captain Holloway", "Brother Finch", "Junie Calloway" (CREW's speaker names), Hob "Hob", Vane
"Cornelius Vane", Delia's barks "Delia Ransom"; Delia's topic lines have no speaker (her point's name, "Delia Ransom").
All shared words: no family words, no hero rows needed.

## The hat variant (how it's chosen) and the flag

- Flag **`Landfall.AbelsHat`**, set by reading the hat: READABLES `AbelsHat` (`sets_flag`), shown only `during Main1 from
  step 7 without Landfall.AbelsHat` (hides once picked up), prompt "Pick up", lines Sully + Hob.
- Main1's turn-in has **no lines**, so `USpeakerPointComponent::Talk` says Delia's first topic that holds
  (`TOPICS['Speaker_Delia']`, in order): `during Main1 from_step 9 with Landfall.AbelsHat` -> DeliaLandfallHat;
  `during Main1 from_step 9` -> DeliaLandfall; `during Main1` -> DeliaLandfallWait. (9 = ready to turn in.)

## Exact hooks other lanes / Main must apply

**TURNIN (must):** when `TurnIn.Lines` is empty (Main1 now; Side2's Amos already), the words said after Turn In are the
giver point's `GetLinesNow()` taken **before** `Runner->TurnIn` (after it, "during Main1 from step 9" no longer holds and
she'd say her porch line), and that topic's `Event`/`SetsFlag` fire as `Talk()` does today. Without it the hat variant is
lost.

**WORLD, `build_area_farm.py` `place_delia`** (on `ADeliaRansom`'s speaker point): replace
```python
    during = story.lines(build, 'DA_Lines_DeliaMain1')
    if during:
        said.append(story.topic(story.condition(before=[story.MAIN1]), during))
```
with
```python
    # Landfall's topics first (story_texts.TOPICS['Speaker_Delia']: the turn-in, the hat's variant, mid-fight).
    said = story.texts_topics(build, 'Speaker_Delia')
```
and name her `name='Delia Ransom'` (`TOPICS['Speaker_Delia']['name']`), default set `DA_Lines_DeliaPorch`. (`said = []`
above it can go.) Keep the later topics (mine, Main 6 with `Delia.Main6`, Main 7) after these.

**WORLD, `build_area_missionprops_valley.py` `place_porch_plate`:** replace
```python
    take_step = steps.find_any(MAIN1, (('event', 'PorchPlate.Taken'), ('interact', 'PorchPlate')), 4)
```
with nothing, and
```python
    plate.set_editor_property('take_when', story.condition(during=MAIN1, from_step=take_step))
```
with
```python
    # Since Landfall the jars are a free pickup after Main 1 (story_texts.PORCH_PLATE take_when; Delia: "Salt's on the plate").
    plate.set_editor_property('take_when', story.condition(after=[MAIN1]))
```
and the log line's `(the jars from Main 1's step {take_step}, ...)` -> `(the jars free after Main 1, the biscuits in Main 7)`.

**WORLD, `build_area_story.py` `place_hob`:** add spots `'porchRail'` (Delia's porch rail, facing the yard) and
`'mooringLedge'` (by the anchor winch on the ledge; build_area_secrets' ('ledge', 'mooringPost') anchor is there), each
`(x, y, z, yaw)`. Replace the two Main 1 perches
```python
        perch(board, condition(during=MAIN1, from_step=2), 'DA_Lines_HobWakes'),
        perch(board, condition(during=MAIN1, from_step=1)),
```
with
```python
        # Landfall (story_texts.LANDFALL_PERCHES): each its first spot the build has, its arrival words said as its step starts.
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.LANDFALL_PERCHES],
```
and the last perch `perch(board, condition(after=[MAIN1])),` with
`perch(spots.get('porchRail') or spots.get('path'), condition(after=[MAIN1])),`. Hob must still be placed now that
Ellis's grave goes (`place_hob` returns early when `hob_on_board(grave_ellis)` is None): place him at `porchRail`
instead, or Main 2 has no giver.

**WORLD, spawners:** `LedgeClimbers` and `FarmPorch` `active_when` from `texts.condition(texts.ENCOUNTERS[id])`
(LedgeClimbers during Main1 steps 4 only; FarmPorch during Main1 from step 7, before 9; its own ~45 m trigger).

**WORLD, the hat:** an `AReadableProp` on the bluff path with `texts.apply_readable(prop, 'AbelsHat')`, tags
`('Readable', 'Readable_AbelsHat', 'Landfall_AbelsHat')`, `hold_to_read = True`, mesh `/Game/Art/Props/` +
`READABLE_MODELS['AbelsHat']` (SM_SheriffHat, STAND-IN). Main: add "19. Abel's hat (a keeper's wide-brim, worn felt,
lantern-soot band)" to `TRIPO_MODELS.md`.

**FLIGHT:** play the sets in the table at the moments given; do not play Anchor/Climb/Point (Hob's perches do); the
scene names `HardLanding`, `SmogReveal` and event `Landfall.DeckClear` are the mission's. Step 7 begins as the spyglass is
used (the objective is the Interact), so the reveal plays during step 7: fine (Hob's step-7 perch is silent for this).

**Main:** `Docs/Progression.md` Main 1's name and its new kills (pacing); `MissionRewards.cpp:372` comment ("Delia,
through the screen door: Grandpa's egg money") -> "Delia Ransom, on her porch"; the Lamplighter legendary's name clash.

## Run order

1. Build. 2. `create_story_lines.py` (writes 93 sets). 3. `create_mission_assets.py`. 4. `create_side_mission_assets.py`
(unchanged, as usual after). 5. `create_hero_lines.py` (one row more: speaker.Sully). 6. `create_ember_assets.py`.
7. `build_area.py RansomsRest gameplay` (with WORLD's hooks). 8. Delete `DA_Lines_DeliaMain1`, `DA_Lines_HeadboardEllis`,
`DA_Lines_HobWakes` only after 7 (an actor still pointing at one opens a modal force-delete).

## Tests

- New: `Looter.Story.Landfall.Mission`, `Looter.Story.Landfall.Lines`.
- Run too (touched): `Looter.Intro` (Script: new slots/timing; Lines: asset = code), `Looter.Lines` (Table, NoFamily),
  `Looter.Progression.Pacing`, `Looter.Missions.Definitions`, `Looter.Heroes` (names), `Looter.Missions.TurnInCard`
  (TURNIN's, for the empty-lines path).

## PIE check for Main

New game: the planet intro's four lines fit and end on the Lamplighter line; on deck the Deck lines; at the prow Vane's
horn; boarders' lines; grapples cut only by holding; landing lines; Hob on the ledge says the Anchor lines as the scene
ends; Climb and Point lines at their steps; the reveal then Finch's trail lines ending on Hob's hat hint; pick up the hat
(it vanishes, Sully/Hob lines); Hob on the porch rail; Delia's barks; Turn In: card, then her hat lines ("Forty years I
told that man to tie it on."); the plate's jars takeable after. Once more without the hat: the plain turn-in.

## Risks I couldn't check

- TURNIN's empty-lines path (above): the one thing that can lose the hat variant.
- `Looter.Progression.Pacing`: Landfall adds 4 deck + 4 ledge + 8 farm kills at levels 1-2 (16, more XP early); the end
  levels move a little (Dredgers count only once built). If a band check fails, trim Main 1's or Main 2's XP, not the kills.
- Hob perch spots `porchRail`/`mooringLedge` missing: the fallbacks (`path`, `rail`) keep the words.
- The scene objective `HardLanding` needs FLIGHT's scene to mark itself played under that exact name.
- Main 2's words (Hob: "Up top, where it happened"; Sexton's deal) still assume the Revenant story; Hob's feathers
  (mutters after Main 1: "Eighty years I've been collecting for him") too. Later rounds.
- `NewGameIntro`: the cards now stay 3.0 s (the test's minimum); if the music's card hits were tuned to the old times, the
  Teropa/Reaches card sounds move 0.8 s earlier.

## Numbers to tune

Prow radius 350; trail radius 1800; caption cap 90 chars (test); intro slots above; Delia's barks rate (CREW).
