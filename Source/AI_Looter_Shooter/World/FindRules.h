#pragma once

#include "CoreMinimal.h"
#include "FindRules.generated.h"

struct FCampaignRecord;
struct FRandomStream;

/**
 * What kind of find a spot is (Docs/Work/2026-10-28-Core/FINDS.md §3): it sets the experience it pays (FindRules::XpFor)
 * and whether it counts among the area's finds. Lore (the Ledger pages) is counted on its own line, never in "4 / 15".
 */
UENUM(BlueprintType)
enum class EFindKind : uint8
{
	/** A chest put out of the way: walk round the rock to it (the footlockers, the Widow's strongbox). */
	Cache,
	/** Reached only by a climb, a crawl or a swim (the tarn's lockbox, the bolt-hole, Ranger Post Three). */
	Hidden,
	/** Dug up: a grave chest held open (Pike's cache, the miser's grave). */
	Grave,
	/** A named wandering creature: found by killing it (the Tollman). Its kill's own experience comes on top. */
	Elite,
	/** A page to read (the Ledger pages): its own count, "LEDGER PAGE · 2 / 6". */
	Lore,
};

/**
 * The finds' sound cues (Art/Sounds/recipes/finds.py makes them; with none a cue plays nothing). Kept here, not in the shared
 * LooterSoundCues.h, which other lanes edit.
 */
namespace FindCue
{
	/** A find made: a struck tin bell over a low plucked open fifth and a crow's far caw (2D, with the banner). */
	inline constexpr const TCHAR* Discover = TEXT("World.Find.Discover");
	/** Every find in the area: the same a fourth up with a second bell (2D, with the last banner). */
	inline constexpr const TCHAR* Complete = TEXT("World.Find.Complete");
	/** A glint off an unfound find's brass: a soft high tink (3D at the glint; the first flash within GlintSoundRange). */
	inline constexpr const TCHAR* Glint = TEXT("World.Find.Glint");
}

/** One find as the rules count it: the spot's id, words and kind (UFindSubsystem builds these from its AFindSpot actors). */
struct AI_LOOTER_SHOOTER_API FFindEntry
{
	FName Id;
	FText Name;
	/** The map's row it counts in ("Farm & Point"). */
	FText Region;
	EFindKind Kind = EFindKind::Cache;
};

/** One region's line on the map: "Farm & Point 2 / 4" (cyan with a tick once done). */
struct AI_LOOTER_SHOOTER_API FFindRegionCount
{
	FText Region;
	int32 Found = 0;
	int32 Total = 0;

	bool IsDone() const { return Total > 0 && Found >= Total; }
};

/** How many of a level's finds are found: in all, and per region in FindRules::RegionOrder's order (unknown regions after). */
struct AI_LOOTER_SHOOTER_API FFindTally
{
	int32 Found = 0;
	int32 Total = 0;
	TArray<FFindRegionCount> ByRegion;

	bool IsComplete() const { return Total > 0 && Found >= Total; }
};

/** The finds' plain rules (Looter.Finds.Rules tests them): flags, experience, the tally, the banner's words, the glint's timing. */
namespace FindRules
{
	/** The banner's length (UHudFindBannerWidget::ShowSeconds): without a HUD (a test level) XP waits this long. */
	inline constexpr float BannerSeconds = 2.8f;
	/** At most this long waiting for a HUD's BannerDone (queued behind menus and other banners) before paying anyway. */
	inline constexpr float BannerWaitLimit = 45.f;
	/** Every find in the area: this much on top (FINDS.md §3 pacing: drop to 500 if Looter.Progression.Pacing says over). */
	inline constexpr int64 CompletionXP = 1000;
	/** The legacy pass (an old save's opened chests and read pages, credited silently) runs this long into play. */
	inline constexpr float LegacyDelay = 1.f;
	/** Seconds between the subsystem's looks (chest lids, distances): 4 Hz. */
	inline constexpr float PollInterval = 0.25f;

	/** "Find.<Id>": the story flag a find is saved as (FCampaignRecord::StoryFlags). Never rename an id. None for None. */
	AI_LOOTER_SHOOTER_API FName FlagFor(FName Id);

	/** A kind's experience at zone level 1: Cache 100, Grave 160, Hidden 220, Elite 300, Lore 40. */
	AI_LOOTER_SHOOTER_API int64 BaseXp(EFindKind Kind);

	/** BaseXp x (1 + 0.35 x (ZoneLevel - 1)), rounded; a level under 1 reads as 1. */
	AI_LOOTER_SHOOTER_API int64 XpFor(EFindKind Kind, int32 ZoneLevel);

	/** Counts in the area's finds ("4 / 15"): every kind but Lore. */
	AI_LOOTER_SHOOTER_API bool CountsInFinds(EFindKind Kind);

	/** The map's region rows in order: Farm & Point, West Road, Town & Boot Hill, South Fields, The Mountain. */
	AI_LOOTER_SHOOTER_API const TArray<FString>& RegionOrder();

	/**
	 * The tally of Entries against Record's flags: the finds (bLore false: every kind but Lore) or the Lore pages (bLore true).
	 * ByRegion in RegionOrder's order, regions it doesn't know after them in the order met; a region with none is left out.
	 */
	AI_LOOTER_SHOOTER_API FFindTally Tally(const TArray<FFindEntry>& Entries, const FCampaignRecord& Record, bool bLore = false);

	/** "4 / 15". */
	AI_LOOTER_SHOOTER_API FText BannerCount(int32 Found, int32 Total);

	/** "4 / 15 · RANSOM'S REST" (the area's name in capitals); "4 / 15" without one. */
	AI_LOOTER_SHOOTER_API FText CountLine(int32 Found, int32 Total, const FText& AreaName);

	/** "LEDGER PAGE · 2 / 6". */
	AI_LOOTER_SHOOTER_API FText LoreLine(int32 Found, int32 Total);

	/** "EVERY FIND IN RANSOM'S REST"; "EVERY FIND" without a name. */
	AI_LOOTER_SHOOTER_API FText CompleteTitle(const FText& AreaName);

	/** "3 FOUND". */
	AI_LOOTER_SHOOTER_API FText CollapsedTitle(int32 Count);

	/** Hob's word when the last find is made: "That's all of it, sunshine. Even I didn't know about the tarn." */
	AI_LOOTER_SHOOTER_API FText CompletionWords();

	/** A find's silver at its zone level: its level-1 Base grown as kill silver grows (LooterEconomy::AtLevel). */
	AI_LOOTER_SHOOTER_API int32 SilverAt(int32 Base, int32 ZoneLevel);

	// --- The glint (an unfound find's brass catching the sun) ---

	/** The player this far off (cm) at least, and at most, sees it. */
	inline constexpr double GlintNear = 800.0;
	inline constexpr double GlintFar = 4500.0;
	/** The first flash within this far (cm) is heard (FindCue::Glint). */
	inline constexpr double GlintSoundRange = 1500.0;
	/** One star's life (s), and the wait between stars (s, seeded per find). */
	inline constexpr float GlintSeconds = 0.3f;
	inline constexpr float GlintEveryMin = 3.5f;
	inline constexpr float GlintEveryMax = 5.f;
	/** At most this many stars at once over the whole level. */
	inline constexpr int32 MaxGlints = 2;
	/** The star's size (cm across, M_FX_Shape quad) and strength (additive). */
	inline constexpr float GlintSize = 70.f;
	inline constexpr float GlintStrength = 6.f;

	/** A glint may show: within GlintNear..GlintFar, a clear line from the eyes, and the player on the sun's side of it. */
	AI_LOOTER_SHOOTER_API bool CanGlint(double Distance, bool bClearLine, bool bSunSide);

	/** The player is on the lit side: the sun (SunToward: from the find toward the sun, any length) up and on their side. */
	AI_LOOTER_SHOOTER_API bool IsSunSide(const FVector& FindToPlayer, const FVector& SunToward);

	/** The wait before a find's next star (s): GlintEveryMin..GlintEveryMax from Random. */
	AI_LOOTER_SHOOTER_API float NextGlintDelay(FRandomStream& Random);

	/** A find's own seed (its glints' rhythm), from its id: the same every play. */
	AI_LOOTER_SHOOTER_API int32 SeedFor(FName Id);

	/** How bright a star is Age seconds into its GlintSeconds (0 to 1: a quick rise, a slower fall; 0 outside it). */
	AI_LOOTER_SHOOTER_API float GlintEnvelope(float Age);
}
