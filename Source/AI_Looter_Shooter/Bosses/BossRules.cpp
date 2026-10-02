#include "Bosses/BossRules.h"

TArray<FBossPhase> BossRules::Ordered(const TArray<FBossPhase>& Phases)
{
	TArray<FBossPhase> Result = Phases;
	for (FBossPhase& Phase : Result)
	{
		Phase.HealthShare = FMath::Clamp(Phase.HealthShare, 0.f, 1.f);
	}
	// Stable, so two phases on the same line keep the order they were written in (the second follows at once).
	Result.StableSort([](const FBossPhase& A, const FBossPhase& B) { return A.HealthShare > B.HealthShare; });
	if (Result.IsEmpty())
	{
		Result.AddDefaulted();
	}
	// The fight always starts in the first phase, whatever share it was written with.
	Result[0].HealthShare = 1.f;
	return Result;
}

int32 BossRules::PhaseAt(const TArray<FBossPhase>& OrderedPhases, float HealthShare)
{
	int32 Phase = 0;
	for (int32 Index = 1; Index < OrderedPhases.Num(); ++Index)
	{
		if (HealthShare <= OrderedPhases[Index].HealthShare + ShareTolerance)
		{
			Phase = Index;
		}
	}
	return Phase;
}

int32 BossRules::AddsToSpawn(int32 Wanted, int32 Alive, int32 WaveCap, int32 BossCap)
{
	const int32 Boss = FMath::Clamp(BossCap, 0, MaxAliveAdds);
	const int32 Cap = WaveCap > 0 ? FMath::Min(WaveCap, Boss) : Boss;
	return FMath::Clamp(FMath::Min(Wanted, Cap - Alive), 0, MaxAliveAdds);
}

bool BossRules::UntargetableEnds(const FBossUntargetable& Spell, float Elapsed, int32 AliveAdds, bool bWavesPending)
{
	const bool bTimeUp = Spell.Seconds > 0.f && Elapsed >= Spell.Seconds;
	const bool bAddsGone = Spell.bUntilAddsDie && AliveAdds <= 0 && !bWavesPending;
	return bTimeUp || bAddsGone;
}

TArray<FVector> BossRules::RingPoints(const FVector& Center, float Radius, int32 Count, float StartDegrees)
{
	TArray<FVector> Points;
	Points.Reserve(FMath::Max(Count, 0));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Degrees = StartDegrees + 360.f * static_cast<float>(Index) / static_cast<float>(Count);
		const float Radians = FMath::DegreesToRadians(Degrees);
		Points.Add(Center + FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f) * Radius);
	}
	return Points;
}
