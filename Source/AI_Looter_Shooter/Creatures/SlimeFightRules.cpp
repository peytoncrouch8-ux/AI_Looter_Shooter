#include "Creatures/SlimeFightRules.h"
#include "HAL/IConsoleManager.h"

// The Looter.Slime.* cvars of the slime's fight (FIGHTS.md section 3). Their defaults are FSlimeFightTuning's own field values.

namespace
{
	const FSlimeFightTuning SlimeDefaults;

	TAutoConsoleVariable<int32> CVarSlimeSplit(TEXT("Looter.Slime.Split"), SlimeDefaults.bSplit ? 1 : 0,
		TEXT("1 lets a Restless or better slime die into two slimelets (under the caps); 0 is a plain death."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarSlimeSplitCap(TEXT("Looter.Slime.SplitCap"), SlimeDefaults.SplitCap,
		TEXT("A split is refused when it would leave more than this many slimes (slimelets too) alive in the level."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSlimeMeleeGap(TEXT("Looter.Slime.MeleeGap"), SlimeDefaults.MeleeGap,
		TEXT("Two slimes' leaps at one target begin at least this far apart (s); 0 is off."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSlimeDodge(TEXT("Looter.Slime.Dodge"), SlimeDefaults.DodgeChance,
		TEXT("The chance that three quick hits, while a slime waits its turn, make it hop aside (0 never, 1 always)."), ECVF_Default);
}

FSlimeFightTuning FSlimeFightTuning::Live()
{
	FSlimeFightTuning Tuning;
	Tuning.bSplit = CVarSlimeSplit.GetValueOnGameThread() != 0;
	Tuning.SplitCap = CVarSlimeSplitCap.GetValueOnGameThread();
	Tuning.MeleeGap = FMath::Max(CVarSlimeMeleeGap.GetValueOnGameThread(), 0.f);
	Tuning.DodgeChance = FMath::Clamp(CVarSlimeDodge.GetValueOnGameThread(), 0.f, 1.f);
	return Tuning;
}

bool FSlimeDodgeTracker::Note(double Now, const FSlimeFightTuning& Tuning, bool bWaiting, bool bBusy, float Roll)
{
	// Only the hits inside the window count.
	Hits.RemoveAll([Now, &Tuning](double At) { return Now - At > Tuning.DodgeWindow; });
	Hits.Add(Now);
	if (Tuning.DodgeChance <= 0.f || Hits.Num() < Tuning.DodgeHits || !bWaiting || bBusy || Now < ReadyAt)
	{
		return false;
	}
	// A burst rolls once, win or lose.
	Hits.Reset();
	if (Roll >= Tuning.DodgeChance)
	{
		return false;
	}
	ReadyAt = Now + Tuning.DodgeCooldown;
	return true;
}

int32 SlimeFightRules::SplitPieces(const FSlimeFightTuning& Tuning, ECreatureRank Rank, bool bSlimelet, int32 SlimesAlive,
	int32 NearAlive, int32 NearCap)
{
	const bool bRanked = LooterRanks::IsTier(Rank) && Rank != ECreatureRank::Basic;
	if (!Tuning.bSplit || bSlimelet || !bRanked || Tuning.SplitPieces <= 0)
	{
		return 0;
	}
	if (SlimesAlive + Tuning.SplitPieces > Tuning.SplitCap)
	{
		return 0;
	}
	if (NearCap > 0 && NearAlive + Tuning.SplitPieces > NearCap)
	{
		return 0;
	}
	return Tuning.SplitPieces;
}

int32 SlimeFightRules::SlimeletLevel(int32 ParentLevel, int32 ParentRankOffset)
{
	return FMath::Max(ParentLevel - ParentRankOffset, 1);
}

FCreatureRuntimeSpawn SlimeFightRules::SlimeletSpawn(const FSlimeFightTuning& Tuning, int32 ParentLevel, int32 ParentRankOffset)
{
	FCreatureRuntimeSpawn Spawn;
	Spawn.Rank = ECreatureRank::Basic;
	Spawn.Level = SlimeletLevel(ParentLevel, ParentRankOffset);
	Spawn.BodyScale = Tuning.SplitSize;
	Spawn.HealthScale = Tuning.SplitHealth;
	return Spawn;
}

TArray<FVector> SlimeFightRules::SplitOffsets(int32 Pieces, float Spread, float StartYawDegrees)
{
	TArray<FVector> Offsets;
	if (Pieces <= 1)
	{
		Offsets.Add(FVector::ZeroVector);
		return Offsets;
	}
	// On a circle whose neighbouring points are Spread apart (two pieces: opposite sides, Spread apart).
	const float Radius = Spread / (2.f * FMath::Sin(UE_PI / Pieces));
	for (int32 Index = 0; Index < Pieces; ++Index)
	{
		const float Yaw = FMath::DegreesToRadians(StartYawDegrees + 360.f * Index / Pieces);
		Offsets.Add(FVector(FMath::Cos(Yaw) * Radius, FMath::Sin(Yaw) * Radius, 0.f));
	}
	return Offsets;
}

FVector SlimeFightRules::DodgeDirection(const FVector& ToTarget, bool bRight)
{
	FVector Forward = ToTarget.GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}
	// Right of forward in Unreal's left-handed, Z-up space: (-Y, X).
	const FVector Right(-Forward.Y, Forward.X, 0.f);
	return bRight ? Right : -Right;
}
