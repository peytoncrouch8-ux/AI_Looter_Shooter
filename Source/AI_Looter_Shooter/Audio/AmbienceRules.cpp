#include "Audio/AmbienceRules.h"
#include "Audio/LooterSoundCues.h"
#include "Misc/Paths.h"
#include "World/LightingStateSubsystem.h"

namespace
{
	const FName DuskName(TEXT("Dusk"));

	FAmbienceSweetener Make(const TCHAR* Cue, float Weight, float MinGap, float MaxGap, float Cooldown, float MinMetres,
		float MaxMetres, float MinUp, float MaxUp, bool bFlat = false)
	{
		FAmbienceSweetener S;
		S.Cue = FName(Cue);
		S.Weight = Weight;
		S.MinGap = MinGap;
		S.MaxGap = MaxGap;
		S.Cooldown = Cooldown;
		S.MinDistance = MinMetres * 100.f;
		S.MaxDistance = MaxMetres * 100.f;
		S.MinHeight = MinUp * 100.f;
		S.MaxHeight = MaxUp * 100.f;
		S.bFlat = bFlat;
		return S;
	}

	/**
	 * The tables. The calls' distances sit inside their attenuation's reach (the birds' is the Gun's, about 60 m, falling
	 * off fast), so a far call is near enough to hear and made far in its recipe (the hawk, the coyote). A sweetener
	 * comes every 5-15 s on average, a few a minute, never so often that the loop shows.
	 */
	const TArray<FAmbienceSweetener>& SkyreachDay()
	{
		using namespace LooterSoundCue;
		static const TArray<FAmbienceSweetener> Table = {
			Make(Songbird, 5.f, 4.f, 9.f, 0.f, 6.f, 16.f, 2.f, 8.f),
			Make(Crow, 1.f, 8.f, 15.f, 25.f, 14.f, 26.f, 4.f, 14.f),
			Make(Hawk, 0.6f, 10.f, 18.f, 70.f, 18.f, 30.f, 18.f, 32.f),
			Make(Bee, 1.5f, 6.f, 12.f, 12.f, 2.f, 6.f, 0.5f, 2.f),
			Make(InsectChirp, 1.2f, 5.f, 10.f, 8.f, 2.f, 6.f, 0.f, 0.5f),
		};
		return Table;
	}

	const TArray<FAmbienceSweetener>& RansomsRestDay()
	{
		using namespace LooterSoundCue;
		static const TArray<FAmbienceSweetener> Table = {
			Make(Songbird, 3.f, 5.f, 11.f, 0.f, 6.f, 18.f, 2.f, 8.f),
			Make(Crow, 1.6f, 7.f, 14.f, 20.f, 12.f, 26.f, 4.f, 14.f),
			Make(Hawk, 1.f, 10.f, 18.f, 50.f, 18.f, 30.f, 20.f, 35.f),
			Make(InsectChirp, 1.5f, 5.f, 10.f, 8.f, 2.f, 6.f, 0.f, 0.5f),
		};
		return Table;
	}

	const TArray<FAmbienceSweetener>& Dusk()
	{
		using namespace LooterSoundCue;
		static const TArray<FAmbienceSweetener> Table = {
			Make(Owl, 1.2f, 8.f, 16.f, 40.f, 10.f, 22.f, 3.f, 10.f),
			Make(Crow, 0.6f, 8.f, 16.f, 45.f, 14.f, 26.f, 4.f, 14.f),
			Make(InsectChirp, 2.5f, 4.f, 9.f, 6.f, 2.f, 6.f, 0.f, 0.5f),
			Make(CoyoteFar, 0.7f, 12.f, 22.f, 80.f, 22.f, 34.f, 0.f, 6.f),
			Make(ThunderFar, 0.5f, 14.f, 24.f, 90.f, 0.f, 0.f, 0.f, 0.f, /*bFlat*/ true),
		};
		return Table;
	}
}

ELooterAudioArea AmbienceRules::AreaForMap(const FString& MapName)
{
	// "/Game/Maps/Lvl_RansomsRest", "UEDPIE_0_Lvl_RansomsRest": the last path part, with any play-in-editor prefix.
	FString Short = FPaths::GetBaseFilename(MapName);
	if (Short.IsEmpty())
	{
		Short = MapName;
	}
	if (Short.Contains(TEXT("RansomsRest")))
	{
		return ELooterAudioArea::RansomsRest;
	}
	if (Short.Contains(TEXT("TutorialIsland")) || Short.Contains(TEXT("Skyreach")))
	{
		return ELooterAudioArea::Skyreach;
	}
	return ELooterAudioArea::Unknown;
}

bool AmbienceRules::IsDusk(FName LightingState)
{
	return LightingState == DuskName;
}

FAmbienceBed AmbienceRules::BedFor(ELooterAudioArea Area, FName LightingState)
{
	using namespace LooterSoundCue;
	FAmbienceBed Bed;
	switch (Area)
	{
	case ELooterAudioArea::RansomsRest:
		Bed.Air = IsDusk(LightingState) ? FName(RansomsRestDuskAir) : FName(RansomsRestDayAir);
		Bed.Life = IsDusk(LightingState) ? FName(RansomsRestDuskLife) : FName(RansomsRestDayLife);
		break;
	case ELooterAudioArea::Skyreach:
	case ELooterAudioArea::Unknown:
	default:
		// The island has one light; if it's ever given a dusk, its breeze carries the Rest's crickets.
		Bed.Air = FName(SkyreachAir);
		Bed.Life = IsDusk(LightingState) ? FName(RansomsRestDuskLife) : FName(SkyreachLife);
		break;
	}
	return Bed;
}

const TArray<FAmbienceSweetener>& AmbienceRules::SweetenersFor(ELooterAudioArea Area, FName LightingState)
{
	if (IsDusk(LightingState))
	{
		return Dusk();
	}
	return Area == ELooterAudioArea::RansomsRest ? RansomsRestDay() : SkyreachDay();
}

int32 AmbienceRules::PickSweetener(TArrayView<const FAmbienceSweetener> Options, int32 Last, TArrayView<const double> ReadyAt,
	double Now, float Roll)
{
	TArray<int32, TInlineAllocator<8>> Ready;
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const bool bCooled = !ReadyAt.IsValidIndex(Index) || ReadyAt[Index] <= Now;
		if (bCooled && Options[Index].Weight > 0.f)
		{
			Ready.Add(Index);
		}
	}
	// Never the same call twice running while there's another: two crows in a row read as one sound on a loop.
	if (Ready.Num() > 1)
	{
		Ready.Remove(Last);
	}
	if (Ready.IsEmpty())
	{
		return INDEX_NONE;
	}
	float Total = 0.f;
	for (const int32 Index : Ready)
	{
		Total += Options[Index].Weight;
	}
	float Pick = FMath::Clamp(Roll, 0.f, 1.f) * Total;
	for (const int32 Index : Ready)
	{
		Pick -= Options[Index].Weight;
		if (Pick <= 0.f)
		{
			return Index;
		}
	}
	return Ready.Last();
}

float AmbienceRules::GapAfter(const FAmbienceSweetener& Sweetener, float Roll)
{
	const float Low = FMath::Max(0.5f, FMath::Min(Sweetener.MinGap, Sweetener.MaxGap));
	const float High = FMath::Max(Low, FMath::Max(Sweetener.MinGap, Sweetener.MaxGap));
	return FMath::Lerp(Low, High, FMath::Clamp(Roll, 0.f, 1.f));
}

FVector AmbienceRules::SpotAround(const FVector& Listener, const FAmbienceSweetener& Sweetener, float YawRoll, float DistanceRoll,
	float HeightRoll)
{
	const float Yaw = FMath::Clamp(YawRoll, 0.f, 1.f) * UE_TWO_PI;
	const float Distance = FMath::Lerp(Sweetener.MinDistance, Sweetener.MaxDistance, FMath::Clamp(DistanceRoll, 0.f, 1.f));
	const float Height = FMath::Lerp(Sweetener.MinHeight, Sweetener.MaxHeight, FMath::Clamp(HeightRoll, 0.f, 1.f));
	return Listener + FVector(FMath::Cos(Yaw) * Distance, FMath::Sin(Yaw) * Distance, Height);
}

FVector AmbienceRules::NearestOnPath(TArrayView<const FVector> Path, bool bClosed, const FVector& Point)
{
	if (Path.IsEmpty())
	{
		return Point;
	}
	if (Path.Num() == 1)
	{
		return Path[0];
	}
	FVector Best = Path[0];
	double BestDistance = TNumericLimits<double>::Max();
	const int32 Segments = bClosed ? Path.Num() : Path.Num() - 1;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const FVector Candidate = FMath::ClosestPointOnSegment(Point, Path[Index], Path[(Index + 1) % Path.Num()]);
		const double Distance = FVector::DistSquared(Candidate, Point);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Candidate;
		}
	}
	return Best;
}

float AmbienceRules::BedCrossfadeSeconds(ELightingSwitch How)
{
	switch (How)
	{
	case ELightingSwitch::Fade:
		// The screen is black for under a second and fades back over 0.6: the new air is up as the picture is.
		return 1.2f;
	case ELightingSwitch::Instant:
		return 0.6f;
	case ELightingSwitch::Blend:
	default:
		// The sun goes down while the player watches: the evening comes in with it.
		return 6.f;
	}
}
