#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class UHeroLineTable;
struct FHeroLineVariant;

/**
 * The story's words for the hero being played (Docs/Story.md, "The heroes": one story; whoever you are takes Ellis's
 * place). Every line, page, prompt and bark is written as Ellis's and stays so in the scripts, the assets and the
 * LOCTEXT. One table, /Game/Data/Story/DA_HeroLines (UHeroLineTable, from Tools/Unreal/hero_lines.json), maps those of
 * Ellis's words that name Ellis's family or Ellis's own life to what any other hero reads, and holds a hero's own voice
 * where they have one. Where words reach the screen one call swaps them: every caption (UCaptionSubsystem::Play), the
 * mission cards and pages, the objectives, mission items, readables, the Ledger, the boss bar's phase, the barks.
 *
 * Precedence for hero H and Ellis's words S:
 *  1. H is Ellis (UHeroDefinition::DefaultId), or S has no row: S, unchanged.
 *  2. The row's Heroes[H]: that.
 *  3. The row's bDropForOthers: nothing (a caption with no words is skipped).
 *  4. The row's Other: that; empty, S (a row only for some heroes' voices).
 * Tokens are filled in the chosen words only: {Hero} the short name ("Odessa"), {HeroFull} the display name ("Odessa
 * Lark"), {HERO} that in capitals, {Initialed} "O. Lark" (one word stays as it is: "Gauge").
 *
 * The played hero is a static: one player, one hero, and code with no world (LooterEconomy) resolves too. The session sets
 * it as each level or the menu begins, and Looter.Hero at once. While the automated tests run every text reads as written
 * (Ellis's), so a Looter.Hero left set never fails a test that checks Ellis's words; a test that wants another hero's
 * words says so with FScopedHero.
 */
namespace HeroLines
{
	// --- The hero ---

	/** The hero whose words are shown: a scoped test hero, else Ellis during the automated tests, else the session's. */
	AI_LOOTER_SHOOTER_API FName GetPlayedHeroId();

	/** USessionSubsystem sets it (None: Ellis). The table is looked up afresh, so a newly made one is found. */
	AI_LOOTER_SHOOTER_API void SetPlayedHero(FName HeroId);

	/** Ellis is played: everything as written. */
	AI_LOOTER_SHOOTER_API bool IsDefaultHero();

	/** Id is Ellis's (or None). */
	AI_LOOTER_SHOOTER_API bool IsDefaultHeroId(FName Id);

	// --- The words ---

	/** The words for the played hero (precedence above). Cheap: Ellis returns Text itself; others one map find. */
	AI_LOOTER_SHOOTER_API FText Resolve(const FText& Text);
	AI_LOOTER_SHOOTER_API FString ResolveString(const FString& Text);

	/** A line's Speaker and Text resolved; Seconds kept (scenes time their lines; 0 reads the new words' length). */
	AI_LOOTER_SHOOTER_API FStoryLine ResolveLine(const FStoryLine& Line);
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> ResolveLines(const TArray<FStoryLine>& Lines);

	/** The same for a given hero rather than the played one (the tests). */
	AI_LOOTER_SHOOTER_API FText ResolveFor(FName HeroId, const FText& Text);
	AI_LOOTER_SHOOTER_API FString ResolveStringFor(FName HeroId, const FString& Text);
	AI_LOOTER_SHOOTER_API FStoryLine ResolveLineFor(FName HeroId, const FStoryLine& Line);

	/**
	 * Precedence 2-4 on one row: true with OutWords the words HeroId reads (empty: dropped), false when the source stands
	 * (Ellis, or no words for this hero). Tokens filled.
	 */
	AI_LOOTER_SHOOTER_API bool PickWords(const FHeroLineVariant& Row, FName HeroId, FString& OutWords);

	/** What HeroId reads for Row: PickWords' words, or the row's Source. */
	AI_LOOTER_SHOOTER_API FText Pick(const FHeroLineVariant& Row, FName HeroId);

	/** The key a text is found by: its source string (culture-proof), else its display string. */
	AI_LOOTER_SHOOTER_API FString SourceOf(const FText& Text);

	// --- Tokens ---

	/** "{Hero}", "{HeroFull}", "{HERO}", "{Initialed}": what a variant may name the hero by. */
	AI_LOOTER_SHOOTER_API const TArray<FString>& TokenNames();

	/** Words with the four tokens filled for HeroId (their definition's names; the id with none). */
	AI_LOOTER_SHOOTER_API FString FillTokens(const FString& Words, FName HeroId);

	/** "Odessa Lark" -> "O. Lark"; one word stays: "Gauge". */
	AI_LOOTER_SHOOTER_API FString Initialed(const FString& FullName);

	// --- Painted words ---

	/**
	 * A hero's own version of an asset, by the naming rule: "/Game/Art/Materials/MI_PosterDecal.MI_PosterDecal" for Odessa
	 * is ".../MI_PosterDecal_Odessa.MI_PosterDecal_Odessa". Empty for Ellis (the asset itself is Ellis's).
	 */
	AI_LOOTER_SHOOTER_API FString HeroVariantPath(const FString& ObjectPath, FName HeroId);

	// --- The table ---

	/** DA_HeroLines, loaded once and kept (null when not made yet: everything reads as written). */
	AI_LOOTER_SHOOTER_API const UHeroLineTable* GetTable();

	/** The row for Ellis's words, or null. */
	AI_LOOTER_SHOOTER_API const FHeroLineVariant* FindRow(const FString& Source);

	/** Looks the table and the heroes' names up afresh on next use (its rows changed, or a level began). */
	AI_LOOTER_SHOOTER_API void ForgetTable();

	// --- For tests ---

	/** The played hero for a scope, then back (nests). */
	struct AI_LOOTER_SHOOTER_API FScopedHero
	{
		explicit FScopedHero(FName Id);
		~FScopedHero();
		FScopedHero(const FScopedHero&) = delete;
		FScopedHero& operator=(const FScopedHero&) = delete;

		FName Was;
	};

	/** Table in place of DA_HeroLines for a scope (null: no table at all), then back. The caller keeps Table alive. */
	struct AI_LOOTER_SHOOTER_API FScopedTable
	{
		explicit FScopedTable(const UHeroLineTable* Table);
		~FScopedTable();
		FScopedTable(const FScopedTable&) = delete;
		FScopedTable& operator=(const FScopedTable&) = delete;

		const UHeroLineTable* Was = nullptr;
		bool bWasActive = false;
		TMap<FString, int32> WasIndex;
	};
}
