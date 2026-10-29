#include "World/FindRules.h"
#include "Economy/EconomyRules.h"
#include "Session/CampaignRecord.h"
#include "Algo/StableSort.h"
#include "Math/RandomStream.h"

#define LOCTEXT_NAMESPACE "LooterFinds"

FName FindRules::FlagFor(FName Id)
{
	return Id.IsNone() ? NAME_None : FName(*FString::Printf(TEXT("Find.%s"), *Id.ToString()));
}

int64 FindRules::BaseXp(EFindKind Kind)
{
	// The harder the reach, the more it pays: a walk round a rock, a dig, a climb or a swim, a named elite (whose kill's own
	// experience comes on top). A page read is a small thanks for looking.
	switch (Kind)
	{
	case EFindKind::Cache: return 100;
	case EFindKind::Grave: return 160;
	case EFindKind::Hidden: return 220;
	case EFindKind::Elite: return 300;
	case EFindKind::Lore: return 40;
	}
	return 0;
}

int64 FindRules::XpFor(EFindKind Kind, int32 ZoneLevel)
{
	// Grown with the zone's level, gentler than kills grow (FINDS.md §3 pacing: the full clear stays under level 7.8).
	const double Scale = 1.0 + 0.35 * static_cast<double>(FMath::Max(ZoneLevel, 1) - 1);
	return static_cast<int64>(FMath::RoundToDouble(static_cast<double>(BaseXp(Kind)) * Scale));
}

bool FindRules::CountsInFinds(EFindKind Kind)
{
	return Kind != EFindKind::Lore;
}

const TArray<FString>& FindRules::RegionOrder()
{
	// The map's rows, as the area doc walks Ransom's Rest: from the farm out west, through town, south and up the mountain.
	static const TArray<FString> Order = {
		TEXT("Farm & Point"),
		TEXT("West Road"),
		TEXT("Town & Boot Hill"),
		TEXT("South Fields"),
		TEXT("The Mountain"),
	};
	return Order;
}

FFindTally FindRules::Tally(const TArray<FFindEntry>& Entries, const FCampaignRecord& Record, bool bLore)
{
	FFindTally Tally;
	// Regions keyed by their words; the known ones first in their order, the rest in the order met.
	TArray<FString> Keys;
	for (const FFindEntry& Entry : Entries)
	{
		if (Entry.Id.IsNone() || CountsInFinds(Entry.Kind) == bLore)
		{
			continue;
		}
		const bool bFound = Record.HasFlag(FlagFor(Entry.Id));
		++Tally.Total;
		Tally.Found += bFound ? 1 : 0;
		const FString Key = Entry.Region.ToString();
		int32 Index = Keys.IndexOfByKey(Key);
		if (Index == INDEX_NONE)
		{
			Index = Keys.Add(Key);
			FFindRegionCount& Row = Tally.ByRegion.AddDefaulted_GetRef();
			Row.Region = Entry.Region;
		}
		FFindRegionCount& Row = Tally.ByRegion[Index];
		++Row.Total;
		Row.Found += bFound ? 1 : 0;
	}
	const TArray<FString>& Order = RegionOrder();
	auto Rank = [&Order](const FFindRegionCount& Row)
	{
		const int32 Known = Order.IndexOfByKey(Row.Region.ToString());
		return Known == INDEX_NONE ? Order.Num() : Known;
	};
	// Stable, so unknown regions keep the order they were met in.
	Algo::StableSortBy(Tally.ByRegion, Rank);
	return Tally;
}

FText FindRules::BannerCount(int32 Found, int32 Total)
{
	return FText::Format(LOCTEXT("Count", "{0} / {1}"), FText::AsNumber(Found), FText::AsNumber(Total));
}

FText FindRules::CountLine(int32 Found, int32 Total, const FText& AreaName)
{
	if (AreaName.IsEmptyOrWhitespace())
	{
		return BannerCount(Found, Total);
	}
	return FText::Format(LOCTEXT("CountLine", "{0} · {1}"), BannerCount(Found, Total), AreaName.ToUpper());
}

FText FindRules::LoreLine(int32 Found, int32 Total)
{
	return FText::Format(LOCTEXT("LoreLine", "LEDGER PAGE · {0}"), BannerCount(Found, Total));
}

FText FindRules::CompleteTitle(const FText& AreaName)
{
	if (AreaName.IsEmptyOrWhitespace())
	{
		return LOCTEXT("CompleteNoArea", "EVERY FIND");
	}
	return FText::Format(LOCTEXT("Complete", "EVERY FIND IN {0}"), AreaName.ToUpper());
}

FText FindRules::CollapsedTitle(int32 Count)
{
	return FText::Format(LOCTEXT("Collapsed", "{0} FOUND"), FText::AsNumber(Count));
}

FText FindRules::CompletionWords()
{
	return LOCTEXT("CompletionWords", "That's all of it, sunshine. Even I didn't know about the tarn.");
}

int32 FindRules::SilverAt(int32 Base, int32 ZoneLevel)
{
	return LooterEconomy::AtLevel(Base, FMath::Max(ZoneLevel, 1));
}

bool FindRules::CanGlint(double Distance, bool bClearLine, bool bSunSide)
{
	// Too near and the player is already on it (a star in their face reads as a bug); too far and it's a speck.
	return Distance >= GlintNear && Distance <= GlintFar && bClearLine && bSunSide;
}

bool FindRules::IsSunSide(const FVector& FindToPlayer, const FVector& SunToward)
{
	const FVector Sun = SunToward.GetSafeNormal();
	// The sun down: nothing catches it. Overhead: every side is lit. Otherwise the player stands on the sun's half (a
	// little past the edge: brass is round, it catches light from the side too).
	if (Sun.IsNearlyZero() || Sun.Z <= 0.02)
	{
		return false;
	}
	const FVector Flat = FVector(Sun.X, Sun.Y, 0.0);
	if (Flat.SizeSquared() < 0.01)
	{
		return true;
	}
	const FVector ToPlayer = FVector(FindToPlayer.X, FindToPlayer.Y, 0.0).GetSafeNormal();
	return ToPlayer.IsNearlyZero() || FVector::DotProduct(ToPlayer, Flat.GetSafeNormal()) > -0.2;
}

float FindRules::NextGlintDelay(FRandomStream& Random)
{
	return Random.FRandRange(GlintEveryMin, GlintEveryMax);
}

int32 FindRules::SeedFor(FName Id)
{
	// The id's words, not the FName's number: the same rhythm every play and every machine.
	return static_cast<int32>(GetTypeHash(Id.ToString()) | 1u);
}

float FindRules::GlintEnvelope(float Age)
{
	if (Age < 0.f || Age > GlintSeconds)
	{
		return 0.f;
	}
	// A third of its life to flare, the rest to fade: a catch of light, not a blink.
	const float Rise = GlintSeconds / 3.f;
	return Age < Rise ? Age / Rise : FMath::Square(1.f - (Age - Rise) / (GlintSeconds - Rise));
}

float FindRules::UnderGlowPulse(float Time, float Seed)
{
	// A slow breath, never out: something down there catching what light reaches it, not a lamp blinking.
	const float Wave = 0.5f + 0.5f * FMath::Sin(UE_TWO_PI * (UnderGlowRate * Time + Seed));
	return FMath::Lerp(UnderGlowLeast, 1.f, Wave);
}

float FindRules::UnderGlowScale(double Distance)
{
	// Under 2 m a third of a glint, a glint's own size by 12 m (the fog has most of it by then).
	return static_cast<float>(FMath::GetMappedRangeValueClamped(FVector2D(200.0, 1200.0), FVector2D(0.35, 1.0), Distance));
}

#undef LOCTEXT_NAMESPACE
