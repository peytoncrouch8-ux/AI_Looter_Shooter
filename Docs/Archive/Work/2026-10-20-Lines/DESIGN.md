# Hero line variants: how the game picks a line by hero (LINES, 2026-10-20)

Rules: `Docs/Story.md` "The heroes" (the user's: one story, Pa is the Keeper, Delia the Keeper's mother, the Deacon's
"kid" is whoever you are, the headboard carries the hero's name, Ruth's "the kid drew"). The words: `VARIANTS.json` here
(148 text entries + 3 painted). Ellis-only beats: `ELLIS_ONLY.md` here.

## The idea

One table, keyed by Ellis's words. Every text stays exactly as it is in every script, asset and LOCTEXT (Ellis's). A new
data asset `/Game/Data/Story/DA_HeroLines` maps each of Ellis's source strings to the neutral text and per-hero overrides.
Where words reach the screen, one call swaps them for the played hero. No line set, mission, readable or Ledger page is
rewritten, and every C++ caption (cold open, Sit with Pa, Abel's fight, Hob's roosts, town life) is covered for free,
because all of them play through `UCaptionSubsystem::Play`.

Why not a new field on FStoryLine: words live in ~12 storage kinds (FStoryLine in five structs, FText fields on missions,
objectives, items, readables, Ledger pages, LOCTEXT, TEXT() tables). Fields on each would touch six scripts, a dozen
structs and every reader. The table touches ~15 read sites and adds one script. Its risk (a key drifting when Ellis's
words are edited) is closed by the test below, which fails on any family word a non-Ellis hero would see.

Precedence for hero H and source text S (`HeroLines::Resolve`):
1. H is Ellis (`UHeroDefinition::DefaultId()`), or S has no row: S unchanged.
2. Row has `Heroes[H]`: that.
3. Row `bDropForOthers`: empty (a caption with no words is skipped by `FCaptionQueue::Add` already).
4. Row `Other` not empty: that. Else S (a row only for some heroes' voices).
5. Tokens filled in the chosen variant only: `{Hero}` ShortName, `{HeroFull}` DisplayName, `{HERO}` DisplayName upper,
   `{Initialed}` "O. Lark" (first initial + last word; one word stays: "Gauge"). FString::Replace, then FText::FromString.

## 1. C++ to add

**`Story/HeroLineTable.h`** (data only):
```cpp
USTRUCT(BlueprintType)
struct FHeroLineVariant
{
	GENERATED_BODY()
	/** Ellis's words, exactly as the data or the LOCTEXT has them: the key. */
	UPROPERTY(EditAnywhere) FString Source;
	/** What every hero but Ellis reads. Empty: Source (a row only for Heroes' own voices). */
	UPROPERTY(EditAnywhere, meta = (MultiLine = true)) FText Other;
	/** A hero's own words over Other, by UHeroDefinition::Id. */
	UPROPERTY(EditAnywhere) TMap<FName, FText> Heroes;
	/** Ellis's alone: other heroes get nothing here unless Heroes has them. */
	UPROPERTY(EditAnywhere) bool bDropForOthers = false;
};

UCLASS(BlueprintType)
class UHeroLineTable : public UDataAsset   // /Game/Data/Story/DA_HeroLines, written by create_hero_lines.py
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TArray<FHeroLineVariant> Lines;
};
```

**`Story/HeroLines.h/.cpp`** (namespace, ~120 lines):
```cpp
namespace HeroLines
{
	/** The hero whose words are shown. USessionSubsystem sets it; Ellis until it does. */
	AI_LOOTER_SHOOTER_API FName GetPlayedHeroId();
	AI_LOOTER_SHOOTER_API void SetPlayedHero(FName HeroId);
	AI_LOOTER_SHOOTER_API bool IsDefaultHero();            // Ellis: everything as written

	/** The words for the played hero (precedence above). Cheap: one TMap find; Ellis returns Text itself. */
	AI_LOOTER_SHOOTER_API FText Resolve(const FText& Text);
	AI_LOOTER_SHOOTER_API FString ResolveString(const FString& Text);
	/** A line's Speaker and Text resolved; Seconds kept (scenes time their lines). */
	AI_LOOTER_SHOOTER_API FStoryLine ResolveLine(const FStoryLine& Line);

	/** For tests: the played hero for a scope, then back. */
	struct AI_LOOTER_SHOOTER_API FScopedHero { explicit FScopedHero(FName Id); ~FScopedHero(); FName Was; };

	/** The table (null when not made yet: everything reads as written) and its row for a source string. */
	AI_LOOTER_SHOOTER_API const UHeroLineTable* GetTable();
	AI_LOOTER_SHOOTER_API const FHeroLineVariant* FindRow(const FString& Source);
}
```
- Key: `const FString* Src = FTextInspector::GetSourceString(Text); Key = Src ? *Src : Text.ToString();` (culture-proof).
- Table: `LoadObject<UHeroLineTable>(nullptr, TEXT("/Game/Data/Story/DA_HeroLines.DA_HeroLines"), nullptr, LOAD_NoWarn)` on
  first use, held in a `TStrongObjectPtr`; a `TMap<FString, int32>` built once. Tokens: `UHeroDefinition::Load(Id)`.
- A formatted text (`FText::Format`) never matches: resolve its parts (an item's name) or its pattern before formatting.

**`UHeroDefinition`** (`Embers/HeroDefinition.h`): `UPROPERTY(EditDefaultsOnly, Category="Hero") FText ShortName;` (the
caption's speaker: "Ellis", "Odessa", "Crane", "Gauge", "Pike"; empty: Id). create_ember_assets.py fills it.

**`USessionSubsystem`**: `HeroLines::SetPlayedHero(GetPlayedHeroId())` at the end of `BeginPlayWorld`
(SessionSubsystem.cpp:221, after `Current` is set; check it runs before actors' BeginPlay), in `BeginMenuWorld`
(:292, after `Current = nullptr`) and in `SetNoSessionHeroId` (SessionSubsystemHero.cpp:23, so `Looter.Hero` swaps
words at once). A static, so context-free code (`LooterEconomy::CanSell`) can resolve too: one player, one hero.

## 2. Where words are resolved, by storage kind

| Kind | Where the words live | Change (one line each unless said) |
|---|---|---|
| FStoryLine, every caption (line sets DA_Lines_*, mission offer/turn-in/choice lines, readable lines, porch plate, speaker topics, mutters, C++ scenes, Abel's fight, Hob's roosts, posters' remarks, town life, ore lift) | data assets and LOCTEXT | `UCaptionSubsystem::Play` (Story/CaptionSubsystem.cpp:34): copy `Lines` through `HeroLines::ResolveLine` before `Queue.Interrupt/Enqueue`. Speaker names ("Ellis", "Grandma Delia") resolve here too: a speaker point's name is filled in before Play (SpeakerPointComponent `InOwnName`). |
| Mission giver names ("Pa", "Grandma Delia") | FMissionOffer/FMissionTurnIn.GiverName | Missions/MissionDefinition.cpp:29-31 and :55-57 (`GetGiverName`, `GetOfferGiverName`: wrap the return); UI/World/MissionOfferWidgetCard.cpp:161-163 |
| Mission summary, briefing | UMissionDefinition.Summary, Offer.Briefing | MissionOfferWidgetCard.cpp:265-266; UI/Inventory/MissionsWidgetDetails.cpp:92; UI/World/NoticeBoardWidgetCards.cpp:218. Titles need no variant (all neutral). |
| Objective words | UMissionObjective.Text, ShortText | Missions/MissionObjective.cpp:136 and :151: resolve before `MissionText::ResolveKeys` (keys like {Interact} survive: tokens are replaced by name only) |
| Mission items | FMissionItemInfo.Name, Line; AMissionItem.Prompt | UI/Inventory/MissionsWidgetFeatures.cpp:171-175; World/MissionItem.cpp:155 (`GetItemName` return) and :279 (`Prompt`) |
| Readables (card) | AReadableProp.Title, Heading, Text | MissionOfferWidgetCard.cpp:363-366 |
| The Ledger | UBestiaryEntry.DisplayName, Kind, Habitat, Description, Notes | UI/Bestiary/BestiaryWidget.cpp:355; BestiaryWidgetDetails.cpp:73, 76, 80, 118, 139-148 |
| Boss phase names ("You brought them here") | FBossPhase.Name (LOCTEXT) | UI/HUD/HudBossBarWidget.cpp:69 (`MakePhaseLine`) |
| LOCTEXT not through captions | Economy/EconomyRules.cpp:193 | resolve the pattern, then `FText::Format` |
| TEXT() barks | Audio/CreatureVoiceBarkLines.cpp | Audio/CreatureVoiceDirector.cpp:138: `const FString Text = HeroLines::ResolveString(CreatureBarks::AllLines()[Line].Text);` (murmur and words both use it) |
| TEXT() town-life mutters | World/TownLifeRules.cpp | none: they become FStoryLines and play through captions. Rename "Hollis" to "Harlan" in the C++ itself (two lines, every hero: Hollis Crane's name) |
| Painted words | meshes, the poster atlas | below |

**Painted words** (VARIANTS.json `painted.*`):
- **The headboard.** `AReadableProp` gains `UPROPERTY(EditAnywhere) TMap<FName, TObjectPtr<UStaticMesh>> HeroMeshes;` and
  `UPROPERTY(EditAnywhere) TObjectPtr<UStaticMesh> OtherMesh;` applied in `BeginPlay` (World/ReadableProp.cpp:43): the
  played hero's mesh, else OtherMesh for any non-Ellis hero, else as placed. Art: Graves.py splits the board out of
  Grave_Ellis into `Headboard_<Hero>` (five; the grave's pivot, Ellis's lean; carved `<NAME> / STOOD UP / AT THE LAST`);
  Grave_Ellis keeps its sockets (Interact, Respawn: GraveWake and Hob's first perch use them). build_area_story.py
  `place_headboards`: the HeadboardEllis readable's Mesh = SM_Headboard_Ellis at the grave's transform, `hero_meshes`
  the four others.
- **The keepers' roll.** MissionProps.py `KeepersRoll_Bare` (no painted-out ELLIS: a fresh primed band under ABEL);
  build_area_missionprops_valley.py `readable('KeepersRoll')` sets `other_mesh` to it.
- **The wanted posters.** looter_posters.py `--hero <Id>` paints `Art/Textures/Posters/T_Posters_<Id>_BC.png` (the whole
  atlas; cell 1 with the hero's name, line and woodcut bust, so the remnant and scrap match); build_decal_materials.py
  imports each that exists and makes `MI_PosterDecal_<Id>`, `MI_PosterScrap_<Id>`. World/WantedPoster.cpp:314 and :337:
  for a non-Ellis hero try `/Game/Art/Materials/MI_Poster{Decal,Scrap}_<Id>` first (LOAD_NoWarn), else today's.
- Not text, not here: the HUD portrait per hero (HUD-PORTRAITS), the first-person arms the cold open shows (arms job).

## 3. Scripts

- **`Tools/Unreal/hero_lines.json`**: this VARIANTS.json, copied in and committed (Saved/ isn't). The one place for variants.
- **`Tools/Unreal/create_hero_lines.py`** (new, ~80 lines): reads it, skips `painted.*`, writes DA_HeroLines: per entry
  Source = current, Other = neutral (empty when it equals current), Heroes, bDropForOthers. Raises (saves nothing) on a
  duplicate source, a hero id not in UHeroDefinition::RosterIds, or a token not in the four. Prints `HEROLINES done: N`.
  Run after create_story_lines.py.
- **create_ember_assets.py**: `short_name` per hero (ShortName above).
- **build_area_story.py**, **build_area_missionprops_valley.py**: the mesh maps above.
- **Graves.py**, **MissionProps.py**, **looter_posters.py**, **build_decal_materials.py**: an art agent (models.ps1 -Only
  Graves / MissionProps; the posters' renders looked at).
- Nothing else changes: every Ellis text stays where it is.

**Two copies of the valley readables (found on the way).** build_area_missionprops_valley.py places LookoutStains,
KeepersRoll, SheriffHat, SheriffBadge, OdellReport, OdellDaybook, WagesChalk, OrdersSlate, AgentDesk and DeaconsSlip with
its own `READABLES` words, not story_texts.READABLES (they differ: the roll's text, the desk's marker, the badge's Ellis
line exists only in story_texts). The mountain and seeds scripts already use `story_texts.apply_readable`; switching the
valley script to it makes one place. VARIANTS.json keys both copies until then.

## 4. Tests (`Looter.Lines.*`, Tests/HeroLinesTests.cpp)

- `Looter.Lines.Table`: DA_HeroLines loads; sources unique; every hero key is a roster id; every token resolves.
- `Looter.Lines.Precedence`: a test row resolves Ellis to Source, Odessa to her override, Crane to Other, a dropped row to
  empty; `{HERO}` gives "ODESSA LARK".
- `Looter.Lines.NoFamily`: under FScopedHero for each of the four, walk every UStoryLineSet in /Game/Data/Story, every
  UMissionDefinition (offer, turn-in, choices, objectives, items, summary, briefing), every UBestiaryEntry, every
  AReadableProp/APorchPlate in Lvl_RansomsRest's saved actors if cheap (else the story_texts words through the line sets),
  `ColdOpenBeats::LineFor` (each ELine), `SitWithPa::Lines()`, AbelRules' lines and phase names, RoostRules, the barks, the
  town-life lines: resolve each and fail on `\b(Pa|Pa's|El|Ellis|Grandma|grandmother|your father|young'un|Ransom kid|Ransom's child|Abel's child)\b`
  (an allow-list for "Tilly's Pa's car"). This is what keeps new lines honest.
- Existing tests that read Ellis's words (SevenDaysTests "CAME HOME AT THE LAST", the cold open, Sit with Pa) take
  `HeroLines::FScopedHero(UHeroDefinition::DefaultId())` so a `Looter.Hero Odessa` left set can't fail them.

## 5. Writing new lines from now on

Per the user's rule: shared lines are written without "Pa", "Ellis" or "kid" (the Deacon's "kid" excepted) and need no
row. A line that is Ellis's own (the family, Ellis's past) is written as Ellis's and gets a row in hero_lines.json with
its neutral; a hero's own voice is a row with `heroes` only. The player character's lines name speaker "Ellis" (it shows
the played hero). Later regions' Ellis beats (the spark's "keeper by blood", "The Kid Who Came Home", the Gravesend
confession's "came home", Delia's door in the epilogue) need their rows when written.

## 6. Order and time (Main, after BARROW-CODE)

1. C++: HeroLineTable.h, HeroLines.h/.cpp, ShortName, the session hooks, the ~15 read sites, AReadableProp's meshes,
   WantedPoster's materials, the bark and economy lines, the Hollis rename (about 1 h).
2. Copy VARIANTS.json to Tools/Unreal/hero_lines.json; write create_hero_lines.py; run it (20 min).
3. Tests (20 min); `Looter.Hero Odessa` + `Looter.PlayLevel` through Main 1-3 in PIE for a look.
4. Art (an agent, separately): the five headboards, the bare roll, four poster atlases; then the two build scripts.
Until the art lands, non-Ellis heroes read the right words over Ellis's carved board and poster: ship-blocking for them,
not for Ellis.

## 7. Calls for the user (the drafts assume the first option of each)

1. **The cold open vs the four select-screen stories.** The user's "you take Ellis's place" and "the Deacon's kid is
   whoever you are" put every hero on the gang's skiff and under the Deacon's gun on the Point (Ruth: "the kid drew").
   But the select screen already says Odessa caught Ned's bullet, Pike was left in a ravine, Crane is forty years dead,
   Gauge woke in a wreck, and Story.md tells each hero's "own death" in a later region. (a) Keep the cold open for all;
   the Point is the hero's death on the Rest, and their region tells their earlier past with the gang (the drafts; a
   Revenant can die twice, as Ellis does at the new moon; the spark of Ada in the wound then works for every hero).
   (b) Cut the hero out of the cold open for the four (camera on the gang and Abel only): about 15 rows would need a
   second neutral (Delia's turn-in, the headboard, Ruth's journal, Amos, Aldana, Abel's "You rode in with them", the
   Deacon's letter and slip, the barks "You rode in with them", the strongbox).
2. **The Deacon's "kid" for Crane and Gauge** too (the user's words; drafts keep it), or a word of their own.
3. **"Bar the door, Hollis"** (a townsman, World/TownLifeRules.cpp): renamed Harlan for every hero, Ellis included.
4. **The non-Ellis epitaph** STOOD UP AT THE LAST, and the posters' lines (Gauge's "RAILROAD PROPERTY").
5. **Sexton's sweetener** drawn from each hero's select-screen story (Odessa's marker, Crane's fare, Gauge's spark,
   Pike's last road) in place of "your father crosses".
