// Developer console command that photographs the cast, creatures and characters, and measures what pictures miss (not in
// shipping builds).
//
// Looter.CastShots [quit] [only=spider,unpaid,...] [flat=X,Y] [slope=X,Y] (from -ExecCmds, '+' and ':' for the commas)
// spawns each creature kind on open flat ground
// and on a slope (found near the player, or given), puts it in each of its states through the game's own paths (idle, a
// stroll, a chase, an attack's wind-up, a hit, its death), lets it run, stops time and photographs it from three angles
// into Saved/Screenshots/CastShots/<NNN>_<who>-<place>_<state>_<angle>.png (no UI); then packs closing on the player, the
// player's own body in third person, and the story's characters where they stand. Beside the pictures it writes
// numbers.csv: feet in or over the ground, limbs through the body, feet sliding, pops, bodies in each other, characters in
// the level, each marked "!" past what a player would notice. Tools/castshots.ps1 runs it standalone. FCastShotRun
// (CastShotRun.h) runs the steps; CastShotScene says who, where and how; CastShotProbe measures.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Dev/CastShotRun.h"
#include "AI_Looter_Shooter.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"

namespace
{
	TSharedPtr<FCastShotRun> ActiveCastShotRun;

	/** "X,Y" or "X:Y" as a point (its height comes from the ground there). -ExecCmds splits commands at commas, so ':' too. */
	bool ParsePoint(const FString& Value, FVector& Out)
	{
		FString X;
		FString Y;
		if (!Value.Split(TEXT(","), &X, &Y) && !Value.Split(TEXT(":"), &X, &Y))
		{
			return false;
		}
		Out = FVector(FCString::Atof(*X), FCString::Atof(*Y), 0.0);
		return true;
	}

	/** Looter.CastShots [quit] [only=a,b] [flat=X,Y] [slope=X,Y] [tag=name] */
	void CastShotsCommand(const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (ActiveCastShotRun.IsValid())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: already running."));
			return;
		}
		bool bQuit = false;
		TArray<FString> Only;
		TOptional<FVector> Flat;
		TOptional<FVector> Slope;
		FString Tag;
		for (const FString& Arg : Args)
		{
			FString Key;
			FString Value;
			FVector Point;
			if (Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase))
			{
				bQuit = true;
			}
			else if (!Arg.Split(TEXT("="), &Key, &Value))
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: '%s' isn't quit, only=, flat=, slope= or tag=; ignored."), *Arg);
			}
			else if (Key.Equals(TEXT("tag"), ESearchCase::IgnoreCase))
			{
				// Its own folder, so a run before a change and one after sit side by side.
				Tag = FPaths::MakeValidFileName(Value);
			}
			else if (Key.Equals(TEXT("only"), ESearchCase::IgnoreCase))
			{
				// Names apart by commas, or by '+' from the command line (-ExecCmds splits commands at commas).
				static const TCHAR* const Separators[] = { TEXT(","), TEXT("+") };
				Value.ParseIntoArray(Only, Separators, UE_ARRAY_COUNT(Separators), /*bCullEmpty*/ true);
			}
			else if (Key.Equals(TEXT("flat"), ESearchCase::IgnoreCase) && ParsePoint(Value, Point))
			{
				Flat = Point;
			}
			else if (Key.Equals(TEXT("slope"), ESearchCase::IgnoreCase) && ParsePoint(Value, Point))
			{
				Slope = Point;
			}
		}
		IFileManager::Get().MakeDirectory(*FCastShotRun::Directory(Tag), /*Tree*/ true);
		ActiveCastShotRun = MakeShared<FCastShotRun>(bQuit, MoveTemp(Only), Flat, Slope, MoveTemp(Tag));
		const TSharedPtr<FCastShotRun> Run = ActiveCastShotRun;
		FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.CastShots"), 0.f, [Run](float DeltaTime)
			{
				const bool bGoOn = Run->Tick(DeltaTime);
				if (!bGoOn)
				{
					ActiveCastShotRun.Reset();
				}
				return bGoOn;
			});
	}

	FAutoConsoleCommandWithWorldAndArgs CastShotsCommandRegistration(
		TEXT("Looter.CastShots"),
		TEXT("Looter.CastShots [quit] [only=spider,unpaid,...] [flat=X,Y] [slope=X,Y] [tag=name]: photographs every creature kind on flat ground and ")
		TEXT("a slope in each state (idle, walk, chase, wind-up, hurt, death), packs, the player's body and the story's characters, from ")
		TEXT("three angles each, into Saved/Screenshots/CastShots/<NNN>_<who>-<place>_<state>_<angle>.png, and writes numbers.csv beside ")
		TEXT("them (feet in the ground, limbs through bodies, sliding feet, pops, overlaps), '!' past what a player would notice. ")
		TEXT("only= keeps subjects whose names start so (spider, spiderling, gravemother, slime, unpaid, player, sexton, amos, hob, ")
		TEXT("abel...); tag= puts the run in a folder of that name. Tools/castshots.ps1 runs it standalone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CastShotsCommand));
}

#endif
