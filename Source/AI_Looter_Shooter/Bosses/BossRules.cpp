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

float BossRules::AddCritToStagger(float BuildUp, float CritDamage, float MaxHealth, const FBossStagger& Stagger)
{
	const float Needed = Stagger.CritShare * MaxHealth;
	if (Needed <= 0.f || CritDamage <= 0.f)
	{
		return FMath::Clamp(BuildUp, 0.f, 1.f);
	}
	return FMath::Clamp(BuildUp + CritDamage / Needed, 0.f, 1.f);
}

float BossRules::DrainStagger(float BuildUp, float DeltaSeconds, const FBossStagger& Stagger)
{
	return FMath::Max(0.f, BuildUp - FMath::Max(DeltaSeconds, 0.f) / FMath::Max(Stagger.DrainSeconds, 0.1f));
}

float BossRules::ShowerReach(int32 Index, const FBossLootShowerSettings& Shower)
{
	// The golden ratio's steps round the unit interval: each lands well away from the one before, and together they fill
	// the ring evenly from its inside to its outside.
	const float Low = FMath::Min(Shower.MinReach, Shower.MaxReach);
	const float High = FMath::Max(Shower.MinReach, Shower.MaxReach);
	const float Step = FMath::Frac(0.5f + static_cast<float>(FMath::Max(Index, 0)) * 0.6180340f);
	return FMath::Lerp(Low, High, Step);
}

FVector BossRules::ShowerThrow(int32 Index, float StartYaw, const FBossLootShowerSettings& Shower, float Gravity)
{
	// Up and back down to the same height takes 2U/g: out at the speed that covers the reach in that time.
	constexpr float GoldenAngle = 137.50776f;
	const float Up = FMath::Max(Shower.UpSpeed, 100.f);
	const float Fall = FMath::Max(Gravity, 1.f);
	const float Out = ShowerReach(Index, Shower) * Fall / (2.f * Up);
	const float Radians = FMath::DegreesToRadians(StartYaw + GoldenAngle * static_cast<float>(FMath::Max(Index, 0)));
	return FVector(FMath::Cos(Radians) * Out, FMath::Sin(Radians) * Out, Up);
}
