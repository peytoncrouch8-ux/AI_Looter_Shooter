// UFightLogSubsystem's report (FightLog.h): what a measured fight looked like, as log lines and a CSV.

#include "Dev/FightLog.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/** The report as it is built: every row goes to the log (lines start with FIGHT) and to the CSV. */
	struct FRows
	{
		FString Csv = TEXT("section,key,value\n");

		void Add(const TCHAR* Section, const FString& Key, const FString& Value)
		{
			UE_LOG(LogLooter, Display, TEXT("FIGHT %s %s: %s"), Section, *Key, *Value);
			Csv += FString::Printf(TEXT("%s,%s,%s\n"), Section, *Key, *Value);
		}
	};

	/** The middle value of Values (the mean of the two middle ones for an even count); 0 for none. */
	float Median(TArray<float> Values)
	{
		if (Values.IsEmpty())
		{
			return 0.f;
		}
		Values.Sort();
		const int32 Middle = Values.Num() / 2;
		return Values.Num() % 2 ? Values[Middle] : (Values[Middle - 1] + Values[Middle]) * 0.5f;
	}
}

FString UFightLogSubsystem::Report(bool bWriteCsv)
{
	FRows Rows;
	const double Length = GetFightSeconds();
	if (FightStart < 0.0)
	{
		UE_LOG(LogLooter, Display, TEXT("FIGHT report: nothing seen yet (no creature fought the player and nothing hurt them)."));
	}

	// The fight as a whole.
	Rows.Add(TEXT("fight"), TEXT("length_s"), FString::Printf(TEXT("%.1f"), Length));
	Rows.Add(TEXT("fight"), TEXT("kills"), FString::Printf(TEXT("%d of %d seen"), GetKills(), Watched.Num()));
	Rows.Add(TEXT("fight"), TEXT("peak_fighting"), FString::Printf(TEXT("%d"), PeakFighting));
	Rows.Add(TEXT("fight"), TEXT("peak_tokens"), FString::Printf(TEXT("melee %d, ranged %d"), PeakMeleeTokens, PeakRangedTokens));
	Rows.Add(TEXT("fight"), TEXT("player_deaths"), FString::Printf(TEXT("%d"), PlayerDeaths));
	Rows.Add(TEXT("fight"), TEXT("lowest_ehp_share"), FString::Printf(TEXT("%.0f%%"), LowestShare * 100.f));
	Rows.Add(TEXT("fight"), TEXT("peak_dps_in_5s"), FString::Printf(TEXT("%.1f"), PeakDps));
	Rows.Add(TEXT("fight"), TEXT("damage_in"), FString::Printf(TEXT("%.0f (%.0f of it to health)"), TotalDamage, TotalToHealth));

	// What dealt it: biggest first.
	TArray<TPair<FString, FDamageIn>> Dealers;
	for (const TPair<FString, FDamageIn>& Pair : DamageByKind)
	{
		Dealers.Emplace(Pair.Key, Pair.Value);
	}
	Dealers.Sort([](const TPair<FString, FDamageIn>& A, const TPair<FString, FDamageIn>& B) { return A.Value.Total > B.Value.Total; });
	for (const TPair<FString, FDamageIn>& Pair : Dealers)
	{
		Rows.Add(TEXT("damage_in"), Pair.Key, FString::Printf(TEXT("%.0f in %d hits (%.1f a hit), %.0f to health"), Pair.Value.Total, Pair.Value.Hits,
			Pair.Value.Hits > 0 ? Pair.Value.Total / Pair.Value.Hits : 0.f, Pair.Value.ToHealth));
	}

	// Attacks started and landed, by kind (the director's events; none without one).
	TSet<FString> Attackers;
	for (const TPair<FString, int32>& Pair : StartedByKind)
	{
		Attackers.Add(Pair.Key);
	}
	for (const TPair<FString, int32>& Pair : LandedByKind)
	{
		Attackers.Add(Pair.Key);
	}
	int32 AllStarted = 0;
	int32 AllLanded = 0;
	for (const FString& Kind : Attackers)
	{
		const int32 Started = StartedByKind.FindRef(Kind);
		const int32 Landed = LandedByKind.FindRef(Kind);
		AllStarted += Started;
		AllLanded += Landed;
		Rows.Add(TEXT("attacks"), Kind, FString::Printf(TEXT("%d started, %d landed (%.0f%%)"), Started, Landed,
			Started > 0 ? 100.f * Landed / Started : 0.f));
	}
	if (Attackers.Num() > 1)
	{
		Rows.Add(TEXT("attacks"), TEXT("all"), FString::Printf(TEXT("%d started, %d landed (%.0f%%)"), AllStarted, AllLanded,
			AllStarted > 0 ? 100.f * AllLanded / AllStarted : 0.f));
	}

	// Time to kill, first hit to death, median by kind, rank and level.
	TMap<FString, TArray<float>> KillTimes;
	for (const TPair<uint32, FWatch>& Pair : Watched)
	{
		const FWatch& Watch = Pair.Value;
		if (Watch.Died >= 0.0 && Watch.FirstHit >= 0.0)
		{
			KillTimes.FindOrAdd(FString::Printf(TEXT("%s/%s/L%d"), *Watch.Kind, *UCreatureRankSettings::GetTierName(Watch.Rank), Watch.Level))
				.Add(static_cast<float>(Watch.Died - Watch.FirstHit));
		}
	}
	KillTimes.KeySort([](const FString& A, const FString& B) { return A < B; });
	for (const TPair<FString, TArray<float>>& Pair : KillTimes)
	{
		Rows.Add(TEXT("ttk"), Pair.Key, FString::Printf(TEXT("median %.2f s of %d"), Median(Pair.Value), Pair.Value.Num()));
	}

	// Every event the director gave, by name (the new moves among them).
	TArray<FString> Names;
	EventsByName.GenerateKeyArray(Names);
	Names.Sort();
	for (const FString& Name : Names)
	{
		Rows.Add(TEXT("events"), Name, FString::Printf(TEXT("%d"), EventsByName.FindRef(Name)));
	}

	if (!bWriteCsv)
	{
		return FString();
	}
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("FightLog"));
	IFileManager::Get().MakeDirectory(*Folder, true);
	const FString Path = Folder / (FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT(".csv"));
	if (!FFileHelper::SaveStringToFile(Rows.Csv, *Path))
	{
		UE_LOG(LogLooter, Warning, TEXT("FIGHT report: could not write %s."), *Path);
		return FString();
	}
	UE_LOG(LogLooter, Display, TEXT("FIGHT report: written to %s."), *Path);
	return Path;
}
