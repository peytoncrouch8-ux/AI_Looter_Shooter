// HeroLines: the played hero, the table of Ellis's words and their variants (loaded once and kept), the precedence and
// the tokens, and the painted words' naming rule.

#include "Story/HeroLines.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Story/HeroLineTable.h"
#include "CoreGlobals.h"
#include "Misc/PackageName.h"

namespace
{
	/** The hero the session plays (None until it says: Ellis). */
	FName PlayedHero;

	/** FScopedHero's hero while one is in scope (the tests). */
	FName ScopedHero;
	int32 ScopeDepth = 0;

	/** A hero's names as the tokens fill them. */
	struct FHeroNames
	{
		FString Short;
		FString Full;
	};

	/**
	 * DA_HeroLines and its lookup. The asset is kept loaded for the run once found (rooted, like the UI's fonts): it is
	 * asked about on every caption, and a garbage collection between two would load it from disk again mid-play.
	 */
	struct FTableCache
	{
		const UHeroLineTable* Table = nullptr;
		/** Source -> row. FString keys match ignoring case, so a find is checked again with case (FindRow). */
		TMap<FString, int32> Index;
		TMap<FName, FHeroNames> Names;
		bool bLooked = false;
	};

	FTableCache& Cache()
	{
		static FTableCache Made;
		return Made;
	}

	/** FScopedTable's table while one is in scope. */
	const UHeroLineTable* OverrideTable = nullptr;
	TMap<FString, int32> OverrideIndex;
	bool bOverride = false;

	void BuildIndex(const UHeroLineTable& Table, TMap<FString, int32>& Index)
	{
		Index.Reset();
		for (int32 Row = 0; Row < Table.Lines.Num(); ++Row)
		{
			const FString& Source = Table.Lines[Row].Source;
			if (Source.IsEmpty())
			{
				continue;
			}
			if (Index.Contains(Source))
			{
				// create_hero_lines.py refuses a duplicate: only an asset edited by hand gets here.
				UE_LOG(LogLooter, Warning, TEXT("HeroLines: %s has \"%s\" twice (row %d); the first row is used."), *Table.GetName(),
					*Source.Left(60), Row);
				continue;
			}
			Index.Add(Source, Row);
		}
	}

	/** The asset when its package is in the game, without a load warning while it isn't made yet. */
	const UHeroLineTable* LoadTable()
	{
		const FString Path = UHeroLineTable::AssetPath();
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			return nullptr;
		}
		UHeroLineTable* Loaded = LoadObject<UHeroLineTable>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Loaded)
		{
			Loaded->AddToRoot();
		}
		return Loaded;
	}

	const FHeroNames& NamesOf(FName HeroId)
	{
		FTableCache& Made = Cache();
		if (const FHeroNames* Known = Made.Names.Find(HeroId))
		{
			return *Known;
		}
		FHeroNames Names;
		const UHeroDefinition* Hero = UHeroDefinition::Load(HeroId);
		Names.Full = Hero && !Hero->DisplayName.IsEmpty() ? Hero->DisplayName.ToString() : HeroId.ToString();
		Names.Short = Hero ? Hero->GetShortName().ToString() : HeroId.ToString();
		return Made.Names.Add(HeroId, MoveTemp(Names));
	}
}

// ---------------------------------------------------------------------------
// The hero
// ---------------------------------------------------------------------------

FName HeroLines::GetPlayedHeroId()
{
	if (ScopeDepth > 0)
	{
		return ScopedHero.IsNone() ? UHeroDefinition::DefaultId() : ScopedHero;
	}
	// The tests check Ellis's words as written, whatever hero Looter.Hero left set for the next play.
	if (GIsAutomationTesting)
	{
		return UHeroDefinition::DefaultId();
	}
	return PlayedHero.IsNone() ? UHeroDefinition::DefaultId() : PlayedHero;
}

void HeroLines::SetPlayedHero(FName HeroId)
{
	if (PlayedHero != HeroId)
	{
		UE_LOG(LogLooter, Log, TEXT("HeroLines: the words are %s's."), *(IsDefaultHeroId(HeroId) ? UHeroDefinition::DefaultId() : HeroId).ToString());
	}
	PlayedHero = HeroId;
	// A level is beginning: the table made since the last one (create_hero_lines.py) is found, the names read again.
	ForgetTable();
}

bool HeroLines::IsDefaultHeroId(FName Id)
{
	return Id.IsNone() || Id == UHeroDefinition::DefaultId();
}

bool HeroLines::IsDefaultHero()
{
	return IsDefaultHeroId(GetPlayedHeroId());
}

// ---------------------------------------------------------------------------
// The words
// ---------------------------------------------------------------------------

FString HeroLines::SourceOf(const FText& Text)
{
	const FString* Source = FTextInspector::GetSourceString(Text);
	return Source ? *Source : Text.ToString();
}

bool HeroLines::PickWords(const FHeroLineVariant& Row, FName HeroId, FString& OutWords)
{
	if (IsDefaultHeroId(HeroId))
	{
		return false;
	}
	if (const FText* Own = Row.Heroes.Find(HeroId); Own && !Own->IsEmpty())
	{
		OutWords = FillTokens(Own->ToString(), HeroId);
		return true;
	}
	if (Row.bDropForOthers)
	{
		OutWords.Reset();
		return true;
	}
	if (!Row.Other.IsEmpty())
	{
		OutWords = FillTokens(Row.Other.ToString(), HeroId);
		return true;
	}
	return false;
}

FText HeroLines::Pick(const FHeroLineVariant& Row, FName HeroId)
{
	FString Words;
	return PickWords(Row, HeroId, Words) ? FText::FromString(Words) : FText::FromString(Row.Source);
}

FText HeroLines::ResolveFor(FName HeroId, const FText& Text)
{
	if (Text.IsEmpty() || IsDefaultHeroId(HeroId))
	{
		return Text;
	}
	const FHeroLineVariant* Row = FindRow(SourceOf(Text));
	FString Words;
	// The source standing, the text itself (its translation too); a variant is the hero's own words.
	return Row && PickWords(*Row, HeroId, Words) ? FText::FromString(Words) : Text;
}

FString HeroLines::ResolveStringFor(FName HeroId, const FString& Text)
{
	if (Text.IsEmpty() || IsDefaultHeroId(HeroId))
	{
		return Text;
	}
	const FHeroLineVariant* Row = FindRow(Text);
	FString Words;
	return Row && PickWords(*Row, HeroId, Words) ? Words : Text;
}

FStoryLine HeroLines::ResolveLineFor(FName HeroId, const FStoryLine& Line)
{
	if (IsDefaultHeroId(HeroId))
	{
		return Line;
	}
	FStoryLine Said = Line;
	Said.Speaker = ResolveFor(HeroId, Line.Speaker);
	const FHeroLineVariant* Row = Line.Text.IsEmpty() ? nullptr : FindRow(SourceOf(Line.Text));
	// A row for one speaker's words leaves the same words in anyone else's mouth as they are.
	const bool bSpeakerFits = Row && (Row->Speaker.IsEmpty() || Line.Speaker.IsEmpty()
		|| Row->Speaker.Equals(SourceOf(Line.Speaker), ESearchCase::CaseSensitive));
	FString Words;
	if (bSpeakerFits && PickWords(*Row, HeroId, Words))
	{
		Said.Text = FText::FromString(Words);
	}
	return Said;
}

FText HeroLines::Resolve(const FText& Text)
{
	return ResolveFor(GetPlayedHeroId(), Text);
}

FString HeroLines::ResolveString(const FString& Text)
{
	return ResolveStringFor(GetPlayedHeroId(), Text);
}

FStoryLine HeroLines::ResolveLine(const FStoryLine& Line)
{
	return ResolveLineFor(GetPlayedHeroId(), Line);
}

TArray<FStoryLine> HeroLines::ResolveLines(const TArray<FStoryLine>& Lines)
{
	const FName HeroId = GetPlayedHeroId();
	if (IsDefaultHeroId(HeroId))
	{
		return Lines;
	}
	TArray<FStoryLine> Said;
	Said.Reserve(Lines.Num());
	for (const FStoryLine& Line : Lines)
	{
		Said.Add(ResolveLineFor(HeroId, Line));
	}
	return Said;
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const TArray<FString>& HeroLines::TokenNames()
{
	static const TArray<FString> Names = { TEXT("{Hero}"), TEXT("{HeroFull}"), TEXT("{HERO}"), TEXT("{Initialed}") };
	return Names;
}

FString HeroLines::Initialed(const FString& FullName)
{
	TArray<FString> Words;
	FullName.ParseIntoArrayWS(Words);
	if (Words.Num() < 2)
	{
		return FullName.TrimStartAndEnd();
	}
	return Words[0].Left(1) + TEXT(". ") + Words.Last();
}

FString HeroLines::FillTokens(const FString& Words, FName HeroId)
{
	if (!Words.Contains(TEXT("{"), ESearchCase::CaseSensitive))
	{
		return Words;
	}
	const FHeroNames& Names = NamesOf(HeroId);
	// Case matters: {Hero} and {HERO} are different tokens (and ReplaceInline ignores case unless told).
	FString Filled = Words;
	Filled.ReplaceInline(TEXT("{HeroFull}"), *Names.Full, ESearchCase::CaseSensitive);
	Filled.ReplaceInline(TEXT("{HERO}"), *Names.Full.ToUpper(), ESearchCase::CaseSensitive);
	Filled.ReplaceInline(TEXT("{Initialed}"), *Initialed(Names.Full), ESearchCase::CaseSensitive);
	Filled.ReplaceInline(TEXT("{Hero}"), *Names.Short, ESearchCase::CaseSensitive);
	return Filled;
}

// ---------------------------------------------------------------------------
// Painted words
// ---------------------------------------------------------------------------

FString HeroLines::HeroVariantPath(const FString& ObjectPath, FName HeroId)
{
	if (IsDefaultHeroId(HeroId) || ObjectPath.IsEmpty())
	{
		return FString();
	}
	FString Package;
	FString Asset;
	if (!ObjectPath.Split(TEXT("."), &Package, &Asset, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		Package = ObjectPath;
		Asset = FPackageName::GetShortName(ObjectPath);
	}
	const FString Suffix = TEXT("_") + HeroId.ToString();
	return FString::Printf(TEXT("%s%s.%s%s"), *Package, *Suffix, *Asset, *Suffix);
}

// ---------------------------------------------------------------------------
// The table
// ---------------------------------------------------------------------------

const UHeroLineTable* HeroLines::GetTable()
{
	if (bOverride)
	{
		return OverrideTable;
	}
	FTableCache& Made = Cache();
	if (!Made.bLooked)
	{
		Made.bLooked = true;
		Made.Table = LoadTable();
		Made.Index.Reset();
		if (Made.Table)
		{
			BuildIndex(*Made.Table, Made.Index);
		}
	}
	return Made.Table;
}

const FHeroLineVariant* HeroLines::FindRow(const FString& Source)
{
	const UHeroLineTable* Table = GetTable();
	if (!Table || Source.IsEmpty())
	{
		return nullptr;
	}
	const int32* Found = (bOverride ? OverrideIndex : Cache().Index).Find(Source);
	if (!Found || !Table->Lines.IsValidIndex(*Found))
	{
		return nullptr;
	}
	// The map matched ignoring case; Ellis's words are exact.
	const FHeroLineVariant& Row = Table->Lines[*Found];
	return Row.Source.Equals(Source, ESearchCase::CaseSensitive) ? &Row : nullptr;
}

void HeroLines::ForgetTable()
{
	FTableCache& Made = Cache();
	Made.bLooked = false;
	Made.Index.Reset();
	Made.Names.Reset();
}

// ---------------------------------------------------------------------------
// For tests
// ---------------------------------------------------------------------------

HeroLines::FScopedHero::FScopedHero(FName Id)
	: Was(ScopedHero)
{
	ScopedHero = Id;
	++ScopeDepth;
}

HeroLines::FScopedHero::~FScopedHero()
{
	ScopedHero = Was;
	--ScopeDepth;
}

HeroLines::FScopedTable::FScopedTable(const UHeroLineTable* Table)
	: Was(OverrideTable)
	, bWasActive(bOverride)
	, WasIndex(MoveTemp(OverrideIndex))
{
	OverrideTable = Table;
	bOverride = true;
	OverrideIndex.Reset();
	if (Table)
	{
		BuildIndex(*Table, OverrideIndex);
	}
}

HeroLines::FScopedTable::~FScopedTable()
{
	OverrideTable = Was;
	bOverride = bWasActive;
	OverrideIndex = MoveTemp(WasIndex);
}
