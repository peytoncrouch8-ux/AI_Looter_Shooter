# Lane SECRETS: Sexton's secrets for silver and "Paid on Delivery" (2026-11-01)

Built from `MAINS_2_7.md` §12 to the user's calls (`DECISIONS.md`: Sexton is the neutral Ferryman, stays at his ferry under
Gravewind Point and sells secrets for silver, e.g. the Rest's finds marked on the map). Nothing built, run in Unreal or
committed (the C++ was written after Main's packaging lock lifted). Python compiled with UE's python (`py_compile`); `create_secret_assets.check()` run on the data with a stub
`unreal` (no problems). C++ written against the headers it calls (read, not compiled: risks below). No new sounds, models,
materials or textures; no Tripo.

## What the player gets

**Sexton's secrets.** From Main 2's turn-in on ("Come back when you've silver. I know where things lie on the Rest."),
a tap of Interact on Sexton in his ferry's stern says **Buy a secret** and opens his list (a mission turn-in or offer
waiting at him still comes first: the talk, as at Pruitt's hatch). The screen, a LooterUI panel over the dimmed fog:
tab MISTER SEXTON, title THE FERRYMAN'S SECRETS, his line for where the story is (before Paid on Delivery: "The barge's
manifest, friend. Bring it and we're square. Meanwhile, I sell what I know."; after: "Paid in full, friend. For prompt
payment, a quarter off whatever I know."), the purse on the right (counts down on a buy, red flash), A QUARTER OFF under it
once earned. FOR SALE: one row per secret, its kind's keyhole mark in its colour, the find's name, the teaser ("A
strongbox from the gang's bank years.") and its region, the price on the right (coin + amount, red when too dear). Bought
ones sit below under MARKED: his line in gold ("Under the middle Widow, friend. North side, off the path. Mind your
back.") and MARKED (or "6 PLACES MARKED"). W/S or the mouse choose (gold line = selected), E buys, Esc closes; walking
off (6.5 m) or dying closes it.

A buy: coins on the counter (`Vendor.Buy`) and a high tink (`World.Find.Glint`), the row flashes soul-gold and its MARKED
stamps in (scale 1.35 to 1, fade in, 0.35 s), the purse counts down, the status says "Marked on your map: The Widow's
Strongbox." Too dear: the light purse (`Vendor.NotEnough`) and how much more is needed. Bought stays bought (story flag
`SecretSold.<Id>` in the session's campaign record, saved at once as story flags are). The place shows as a gold keyhole
pin on the map page (name: the find's, detail: his line; a legend row "Sexton's secret") and on the minimap (its place
when near, the two nearest on the rim from afar, like found lantern posts) until it's found (its find's `Find.<Id>`), then
the secret drops off the list and the marks go. The finds' own crows and glints still tell it close up.

**17 secrets on Ransom's Rest, unlocked in steps** (`story_texts.SEXTON_SECRETS`; prices at level 1, grown 8% a level
compounding as the vendors' are, `LooterEconomy::AtLevel`):

| From | Secret (id) | Marks | Price |
|---|---|---|---:|
| Main 2 | The Widow's Strongbox (WidowsBox), The Gang's Locker (Mooring), The Bolt-Hole, Ranger Post Three, The Averys' Savings (Chimney), The Keepers' Stores (LanternHouse) | their finds | 100, 40 x5 |
| Main 3 | The Tollman (his beat's find), The Spiders' Hoard, The Miser's Fare, The Gang's Ledger Pages (six Lore pages, one secret) | their finds | 150, 40, 40, 60 |
| Main 4 | Thaddeus's Bottle, The Surveyor's Tin (CoffinRock), Pike's Cache | their finds | 40, 40, 100 |
| Main 5 | The Gravemother's Barrow (a story secret) | `Place_SinkFloor`; off the list after Side 3 | 60 |
| Dead Lift | Teague's Powder Box, The Fallen Crate, Where They Can't Wade (Tarn) | their finds | 40, 40, 100 |

A secret whose find isn't in the level (no `AFindSpot` with its id: RangerPost if its builder skipped it) isn't listed.
Later regions: their own catalog `DA_Secrets_<Level>` (the next barge, the gang's ground: §12).

**Paid on Delivery** (side, 300 XP, ~3 min). No card: it starts by itself once Main 6 is done (prerequisite Main6; the
manifest is its own item `BargeManifest`, so it's in Carried from Main 6's office until handed over). Tracker "Pay the
Ferryman"; 0: reach the Ferryman's landing (the `SextonsFerry` actor within 4 m, height counted: never from the deck).
Turned in to Mister Sexton (`Speaker_Sexton`): the card, then "Four hundred and twelve. Vane owes me a fortune, friend." /
"Paid in full. I keep my accounts, and you're in the black." / "A tip, for prompt payment: Purcell's gone south. The
Deacon's calling the gang in." A Rare gun at his feet ("from the ferry's bilge"); his secrets cost a quarter less from
then on (`USecretCatalog::DiscountMission`), and his greeting says so.

## Files

New C++ (all `Source/AI_Looter_Shooter/`):
| File | What |
|---|---|
| `Economy/SecretRules.h/.cpp` | `ESecretKind`, `FSecretOffer`, `FSecretGreeting` (USTRUCTs), `ESecretState`, `FSecretRow`, `FSecretMark`, `ESecretBuy`; `SecretRules`: `SoldFlag` ("SecretSold.<Id>"), `BasePrice` (Cache 40, Strongbox/Named 100, Elite 150, Lore/Story 60), `PriceAt`/`PriceFor` (AtLevel, then the discount, at least 1), `IsFound`, `StateOf` (Hidden / ForSale / Sold / Done), `IsListed`, `CatalogPathFor` ("Lvl_RansomsRest" -> `/Game/Data/Secrets/DA_Secrets_RansomsRest`), `KindWord`, `BuyWords`. |
| `Economy/SecretCatalog.h/.cpp` | `USecretCatalog` (UPrimaryDataAsset): Title, Tab, OpenWhen, DiscountMission, Discount, Greetings, SoldOut, Secrets; `FindOffer`, `IsOpen`, `DiscountFor`, `GreetingFor`. |
| `Economy/SecretSubsystem.h/.cpp` | `USecretSubsystem` (world subsystem; Game, PIE, editor preview): the level's catalog (loaded once by the level's name; `SetCatalogForTesting`), `GetRows` (listed, for sale first, prices for the player's level), `Buy` (wallet, flag, `SetsFlag`, `OnSold`), `GiveFree`, `Forget`/`ForgetAll`, `GetMarks` (sold, unfound places: find spots by id, else actors by `MarkTag`; looked up once, again at most every 5 s while missing). |
| `Economy/SecretBroker.h/.cpp` | `USecretBrokerComponent`: who sells them (on `AMisterSexton` by default): `IsOpen`, `HasStoryBusiness` (a turn-in or offer at its owner), `ShouldTrade`, `Prompt` ("Buy a secret"), `OpenFor`. |
| `UI/Vendor/SecretsWidget.h/.cpp`, `UI/Vendor/SecretsWidgetRows.cpp` | `USecretsWidget`: the screen (`OpenFor`/`GetOpen`/`Close`, `Select`, `BuySelected`, `GetStatus`, `GetRowCount`), `SecretsLayout`; rows, purse, flash/stamp motion, keys and mouse. |
| `UI/HUD/HudMinimapWidgetSecrets.cpp` | `UHudMinimapWidget::BuildSecrets`/`UpdateSecrets`: six pooled keyhole markers (PlaceRoosts: on the map near, two on the rim). |
| `Dev/SecretsDevCommands.cpp` | `Looter.Secrets.List`, `Looter.Secrets.Buy <Id|all> [free]`, `Looter.Secrets.Forget [Id]`, `Looter.Secrets.Open` (anywhere, once he trades), `Looter.GiveSilver <amount>` (500 without one; there was no silver command). |
| `Tests/SecretsTests.cpp` | `Looter.Story.Secrets.Rules`, `.Catalog`, `.Buy`, `.Marks`, `.Broker`. |
| `Tests/PaidOnDeliveryTests.cpp` | `Looter.Story.PaidOnDelivery.Mission`, `.Flow`, `.Discount`. |

Edited C++ (mine, done in place; small):
- `Story/MisterSexton.h/.cpp`: a `Secrets` (`USecretBrokerComponent`) default subobject; `GetInteractionOptions`/`Interact`
  overrides: trades (tap "Buy a secret") when `ShouldTrade`, else the story character's talk as before.
- `UI/Inventory/MapPins.h/.cpp`: `EMapPinKind::Secret` (between Station and Grave; Objective stays last), gathered from
  `USecretSubsystem::GetMarks`; `KindName` "Sexton's secret". (The header's "no chests" rule stands: these are bought.)
- `UI/Inventory/MapIcons.h/.cpp`: the Secret kind's colour (`Color::SoulGold`), size 22, keyhole icon; `SecretBrush`.
- `UI/Inventory/MapWidget.cpp`: the legend's Secret row (last).
- `UI/HUD/HudMinimapWidget.h/.cpp`: the secrets' members, `BuildSecrets` after `BuildRoosts`, `UpdateSecrets` after
  `UpdateRoosts`.
- `UI/HUD/LooterHUD.cpp`: `IsMenuOpen` counts `USecretsWidget::GetOpen()`.

Python (`Tools/Unreal/`):
- `story_texts.py`: the SECRETS lane block at the end: `SEXTON_SECRETS` (catalog data, docstring there).
- `create_secret_assets.py` (new): writes `/Game/Data/Secrets/DA_Secrets_RansomsRest` from it, with checks.
- `create_side_mission_assets.py`: docstring entry, `SEXTON_TAG`, `FERRY_TAG`, `FERRY_REACH`, `paid_on_delivery_steps`,
  the `DA_Mission_PaidOnDelivery` spec (last in MISSIONS).
- `create_story_lines.py`: nothing (his lines are the catalog's and the mission's).

## The in-place edits, as made (done in Source/ after Main's "C++ OK"; kept here for the review)

All the new files above are in `Source/AI_Looter_Shooter/`. The six edits to existing files:

**A. `Story/MisterSexton.h`**: after `class UStaticMeshComponent;` add `class USecretBrokerComponent;`; in `public:` after
`AMisterSexton();` add:
```cpp
	// --- IInteractable: his secrets for silver once he trades (lanes/SECRETS.md), else the story character's talk ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;

	/** His secrets for silver: the level's catalog, sold from his stern (a mission waiting at him still talks first). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USecretBrokerComponent> Secrets;
```
**`Story/MisterSexton.cpp`**: includes `Economy/SecretBroker.h`, `Interaction/InteractionComponent.h`; in the constructor,
before `static UStaticMesh* const Seated` (the early return): `Secrets = CreateDefaultSubobject<USecretBrokerComponent>(TEXT("Secrets"));`;
and at the end:
```cpp
FInteractionOptions AMisterSexton::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options = Super::GetInteractionOptions(User);
	if (!Secrets || !Secrets->ShouldTrade())
	{
		return Options;
	}
	// Trading: a tap opens his list, at the talk's reach. A mission waiting at him (Paid on Delivery) talks first: ShouldTrade.
	Options.bUsable = true;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Secrets->Prompt;
	return Options;
}

bool AMisterSexton::Interact(UInteractionComponent& User, bool bHeld)
{
	if (Secrets && Secrets->ShouldTrade())
	{
		return !bHeld && Secrets->OpenFor(User.GetOwner());
	}
	return Super::Interact(User, bHeld);
}
```
**B. `UI/Inventory/MapPins.h`**: in `EMapPinKind`, between `Station,` and the Grave comment, add
```cpp
	/** A place one of Sexton's secrets marks (bought from him, until it's found): the player's purchase, not a chest given away. */
	Secret,
```
**`MapPins.cpp`**: includes `Economy/SecretRules.h`, `Economy/SecretSubsystem.h`; after the vendors' loop (before "Where trips
leave from"):
```cpp
		// Sexton's secrets bought and not found yet (USecretSubsystem): paid for, so shown (lanes/SECRETS.md).
		if (const USecretSubsystem* Secrets = Sources.Campaign ? USecretSubsystem::Get(World) : nullptr)
		{
			for (const FSecretMark& Mark : Secrets->GetMarks(*Sources.Campaign, Sources.Runner))
			{
				FMapPin& Pin = Pins.AddDefaulted_GetRef();
				Pin.Kind = EMapPinKind::Secret;
				Pin.Location = Mark.Location;
				Pin.Name = Mark.Title;
				Pin.Detail = Mark.Line;
				Pin.Id = Mark.Id;
			}
		}
```
and in `KindName`: `case EMapPinKind::Secret:     return LOCTEXT("KindSecret", "Sexton's secret");`.
**C. `UI/Inventory/MapIcons.h`**: after `RoostBrush`'s declaration add
`/** Sexton's secret's keyhole at a size, tinted: the map's pin, the minimap's marks, the secrets screen's rows. */`
`AI_LOOTER_SHOOTER_API FSlateBrush SecretBrush(float IconSize, const FLinearColor& Tint);` (and "a keyhole for a secret bought"
in the namespace comment). **`MapIcons.cpp`**: in the anonymous namespace after `StationIcon()`:
```cpp
	/** A keyhole in a ring: a place Sexton sold the player (his secrets, lanes/SECRETS.md). */
	const FVectorIcon& SecretIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 2.f;
			TArray<FVector2D> Ring = Circle(FVector2D(12.0, 12.0), 9.0, 24);
			Ring.Add(Ring[0]);
			Result.Strokes.Add(Ring);
			Result.Fills.Add(Circle(FVector2D(12.0, 10.0), 3.0, 16));
			Result.Fills.Add(TArray<FVector2D>{ FVector2D(10.6, 11.0), FVector2D(13.4, 11.0), FVector2D(14.4, 17.5), FVector2D(9.6, 17.5) });
			return Result;
		}();
		return Icon;
	}
```
`Color`: `case EMapPinKind::Secret:      return LooterUI::Color::SoulGold();`; `Size`: `case EMapPinKind::Secret:      return 22.f;`;
`IconBrush`: `case EMapPinKind::Secret:      return SecretBrush(IconSize, Tint);`; and after `RoostBrush`:
```cpp
	FSlateBrush SecretBrush(float IconSize, const FLinearColor& Tint)
	{
		const float Pixels = IconSize < 16.f ? 4.f : PixelsPerUnit;
		return LooterUI::IconBrush(TEXT("MapSecret"), SecretIcon(), Pixels, FVector2D(IconSize, IconSize), Tint);
	}
```
**D. `UI/Inventory/MapWidget.cpp`** `LegendKinds`: `EMapPinKind::Shrine,` -> `EMapPinKind::Shrine, EMapPinKind::Secret,`.
**E. `UI/HUD/HudMinimapWidget.h`**: after `float RoostLookLeft = 0.f;` add
```cpp

	// --- Sexton's secrets bought (HudMinimapWidgetSecrets.cpp) ---

	/** Builds the secrets' markers (a dark badge ringed gold, the keyhole over it) into Layer, over the roosts. */
	void BuildSecrets(UCanvasPanel* Layer);
	/** Asks for the bought secrets' places now and then (USecretSubsystem::GetMarks) and places their markers. */
	void UpdateSecrets(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm, float DeltaSeconds);

	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SecretBadges;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SecretIcons;
	/** The marks as last asked for, and the seconds until the next ask. */
	TArray<FMinimapRoost> SecretCache;
	float SecretLookLeft = 0.f;
```
**`HudMinimapWidget.cpp`**: after `BuildRoosts(MarkerLayer);` add `BuildSecrets(MarkerLayer);`; after
`UpdateRoosts(Location, Yaw, Radius, PixelsPerCm, InDeltaTime);` add `UpdateSecrets(Location, Yaw, Radius, PixelsPerCm, InDeltaTime);`.
**F. `UI/HUD/LooterHUD.cpp`**: include `UI/Vendor/SecretsWidget.h`; `IsMenuOpen`'s last line
`|| UBossShrineWidget::GetOpen() != nullptr || ULostLootWidget::GetOpen() != nullptr || UBountyBoardWidget::GetOpen() != nullptr;`
-> `... || UBountyBoardWidget::GetOpen() != nullptr || USecretsWidget::GetOpen() != nullptr;`.

## CODEMAP lines (Main writes)

Under the economy block (after `Economy/SilverDrops`):
- `Economy/SecretRules`: `ESecretKind`, `FSecretOffer`, `FSecretGreeting`, `ESecretState`, `FSecretRow`, `FSecretMark`, `ESecretBuy`, `SecretRules` (sold flag `SecretSold.<Id>`, prices by kind grown with level, a quarter off after Paid on Delivery, a secret's state from the record). `Economy/SecretCatalog`: `USecretCatalog`, a level's secrets for sale (`/Game/Data/Secrets/DA_Secrets_<Level>`, from `Tools/Unreal/create_secret_assets.py`). `Economy/SecretSubsystem`: `USecretSubsystem`, the level's catalog, rows, buying, marks (map page and minimap). `Economy/SecretBroker`: `USecretBrokerComponent`, who sells them (Mister Sexton at his ferry: tap "Buy a secret" when no mission waits at him).
Under UI (after `UI/Vendor/VendorWidget.h`):
- `UI/Vendor/SecretsWidget`: `USecretsWidget`, Sexton's secrets screen (`SecretsLayout`; for sale then MARKED, the purse, a buy's flash and stamp; W/S, E, Esc, mouse). `SecretsWidgetRows.cpp` the rows and the motion.
Under the minimap line: add "`HudMinimapWidgetSecrets.cpp` `BuildSecrets`/`UpdateSecrets` (Sexton's secrets bought: keyhole marks, near and on the rim)".
Under MapPins: add `Secret` (Sexton's secrets bought, until found) to the kinds listed.
Dev: `Dev/SecretsDevCommands.cpp`: `Looter.Secrets.List`, `.Buy <Id|all> [free]`, `.Forget [Id]`, `.Open`; `Looter.GiveSilver <amount>`.
Tests: `SecretsTests.cpp` (`Looter.Story.Secrets.Rules/.Catalog/.Buy/.Marks/.Broker`), `PaidOnDeliveryTests.cpp` (`Looter.Story.PaidOnDelivery.Mission/.Flow/.Discount`).
Tools: `Tools/Unreal/create_secret_assets.py`: Sexton's secrets catalog from `story_texts.SEXTON_SECRETS` (checks ids, finds, prices, caption lengths).

## Exact edits for Main (in others' files)

**E1 (required for Pacing) `Source/AI_Looter_Shooter/Tests/PacingPaths.h`**, the everything path's end. Old:
```cpp
		Path.Add(BackPayStop(Foes));
		Path.Add(MainSixStop(Foes));
		Path.Add(MainSevenStop());
		return Path;
```
New:
```cpp
		Path.Add(BackPayStop(Foes));
		Path.Add(MainSixStop(Foes));
		// Paid on Delivery (2026-11-01, lanes/SECRETS.md): Sexton's price, the manifest handed over at his ferry. No fight.
		Path.Add({ TEXT("Paid on Delivery"), TEXT("PaidOnDelivery"), {}, 300 });
		Path.Add(MainSevenStop());
		return Path;
```
**E2 (for whoever builds Main 6; `create_mission_assets.py` is MAIN45's now)**: the manifest is Paid on Delivery's item
`BargeManifest` (tag `Item_BargeManifest`, `AMissionItem::TagFor`). Main 6's step 4 picks it up: an `AMissionItem` with
`item_id` BargeManifest in the stoker's office (shown during Main6 from step 4), and the step's objective
`interact(asset, "Take the barge's manifest from the Company office.", 'Item_BargeManifest')` (an Interact on the item's
tag: a collect objective there would name another mission's item, which the script refuses). Until the office exists,
add to Main 6's `turn_in=dict(...)`: `hand_items=['BargeManifest'],` so the manifest is carried into Paid on Delivery.
Either way Paid on Delivery runs without it (it only adds the Carried row).
**E3 `Docs/Progression.md`**: the side missions' table: "Paid on Delivery, after Main 6, 300 XP, Rare gun"; the
everything band's sum +300 (re-copy from `Looter.Progression.Pacing` once it runs; MAINS_2_7 §15: trim mission XP, not
kills, if the band fails).
**E4 `Docs/Pipeline/Game.md`**, a new bullet beside the vendors ("Economy"/vendor screen section):
"**Sexton's secrets** (2026-11-01, lanes/SECRETS.md): `USecretCatalog` per level (`/Game/Data/Secrets/DA_Secrets_<Level>`,
`create_secret_assets.py` from `story_texts.SEXTON_SECRETS`), sold by `USecretBrokerComponent` (on `AMisterSexton`: tap
"Buy a secret" unless a mission waits at him) through `USecretsWidget` (opens itself like the bounty board: `GetOpen`,
counted in `ALooterHUD::IsMenuOpen`). A buy sets `SecretSold.<Id>` (saved with the story flags) and spends silver at
`SecretRules::PriceFor` (level-grown, a quarter off after `DiscountMission`). Marks: `USecretSubsystem::GetMarks` (find
spots by id until `Find.<Id>`, or actors by `MarkTag` until `DoneWhen`) feed `MapPins` (`EMapPinKind::Secret`) and the
minimap (`HudMinimapWidgetSecrets.cpp`, PlaceRoosts). A secret whose find isn't in the level isn't listed. Dev:
`Looter.Secrets.*`."
**E5 `Docs/Pipeline.md`**: Game.md's line gains "Sexton's secrets (2026-11-01)".
**E6 CODEMAP** (above). **E7 `Docs/Handoff.md`**: the "Known: Sexton's secrets-for-silver vendor page not built ... nor
Paid on Delivery" line goes (built, this lane).

## Run order (Main)

1. E1. 2. Build (`Tools\launch.ps1 -Build`). 3. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_secret_assets.py"`
   (read `SECRETS done: ... 17 secrets`); `create_side_mission_assets.py` as usual (read `SIDEMISSIONS DA_Mission_PaidOnDelivery
   made`; it needs `Main6` made: create_mission_assets first). No level rebuild: Sexton gets his component from the class;
   nothing is placed. 4. Tests below. 5. PIE walkthrough. Perf: nothing drawn in the world; the minimap's six markers are
   pooled (looks 2 Hz), the screen only while open.

## Tests (run once at commit, `Tools\runtests.ps1 -Filter ...`)

New: `Looter.Story.Secrets` (Rules, Catalog, Buy, Marks, Broker), `Looter.Story.PaidOnDelivery` (Mission, Flow, Discount).
Touched: `Looter.UI.Map` (a new pin kind; the old counts don't change), `Looter.Missions.Definitions` (a new mission and
item), `Looter.Progression.Pacing` (after E1), `Looter.Story.FerrymansPrice` (Sexton's class changed: his talk still works
before Main 2 is done), `Looter.UI.Minimap` if present.

## PIE walkthrough

1. `Looter.Mission.Start Main3` (Main 2 done), or finish Main 2 at the ferry: right after his turn-in lines his prompt
   reads "Buy a secret". Tap: the screen opens over the fog (UI.Open), the greeting about the manifest, the purse.
2. With no silver: E on The Gang's Locker: the light purse and "Not enough silver: 40 needed, you carry 0."
   `Looter.GiveSilver 300`, E again: coins + tink, the row drops under MARKED ON YOUR MAP with his line in gold, flashing
   soul-gold, MARKED stamping in; the purse counts down with a red flash. Esc closes (UI.Close), the game has its keys back.
3. Open the map page (M): a gold keyhole pin on the Mooring Ledge, its name and his line on hover; the legend's
   "Sexton's secret" row counts 1. Walk toward it: the minimap shows the keyhole on the rim, then at its place.
4. Open the locker: the FOUND banner as before; reopen Sexton's list: the locker is gone; the map pin is gone.
5. `Looter.Secrets.List` (every state and price), `Looter.Secrets.Buy LedgerPages free`: six pins. `Looter.Secrets.Forget`.
6. Only the first six before Main 3: `Looter.Mission.Complete Main3` (or play) brings the Tollman, the Hoard, the Miser,
   the Ledger pages; Main 4 the South Fields; Main 5 the Gravemother's barrow; Dead Lift the mountain.
7. Walk off the landing up the ladder with the screen open: it closes at 6.5 m.
8. `Looter.Mission.Complete Main6` (or play it): Paid on Delivery appears (Missions page, "Pay the Ferryman"), the
   manifest in Carried if Main 6 gave it. Climb down: the step ticks on the landing, the tracker turns to Sexton. Tap him:
   "Turn in" (not the shop): the card, his three lines, the Rare gun. Tap again: "Buy a secret"; the greeting now says
   paid in full; prices a quarter lower and A QUARTER OFF under the purse.
9. Save & Quit, load the session: bought secrets still MARKED, pins still on the map.

## Risks I couldn't check

- Compile: written against the headers, not built. Watch: `UWorld::RemovePIEPrefix`, `ScrollWidgetIntoView`'s signature,
  `FInteractionOptions` copy from the story character's, `TObjectPtr` in a `mutable` UPROPERTY, `FStoryCondition::IsMet`
  with a null runner (used in the rules test), the `EMapPinKind` switch defaults.
- `AMisterSexton` gains a default subobject: his placed instance picks it up on load (no rebuild needed); if the editor
  shows him without it, re-run `build_area.py RansomsRest gameplay`.
- The shop takes Sexton's tap after Main 2, so his after-topic lines (`DA_Lines_SextonAfter`) are no longer heard: the
  screen's greeting says the same (the manifest). His talk comes back whenever a mission waits at him.
- Paid on Delivery starts when Main 6 is done, not at the manifest's pickup (Main 6 isn't rebuilt yet, §12's "starts when
  the manifest is picked up" needs an event from it): switch to `start='ON_EVENT', start_event='<Main 6's manifest event>'`,
  `prerequisites=['BackPay']` once Main 6 sends one. Main 6's own items are taken away as it finishes; the manifest is safe
  because it's Paid on Delivery's.
- Reach 4 m with height from the landing's middle: the ladder's foot is about 4 m along; the step may tick a step after
  the ladder, not on it. The deck (3.8 m up) never counts.
- A find not placed in the level hides its secret; if the finds' builder logs a spot left out, that secret silently
  isn't sold (by design).
- The legend gains a row: the map page's side column grows 27 px (check it still fits at 1080p with the fast-travel card).
- `UI.Open`/`UI.Close`/`Vendor.Buy`/`World.Find.Glint` are existing cues; no new sound.

## Numbers to tune

Prices 40 / 100 / 150 (Lore and Story 60), +8% a level compounding, 25% off after Paid on Delivery. Screen: 900 px wide,
list 460 px tall then scrolls, closes at 6.5 m. Flash 0.7 s, stamp 0.35 s. Minimap: six markers, two on the rim within
150 m. Paid on Delivery: 300 XP, Rare, reach 4 m.
