#include "Combat/RecoverySettings.h"

float FRecoverySettings::RegenShareBetween(float From, float To) const
{
	const float Ramp = FMath::Max(RegenRampSeconds, 0.f);
	const float Rate = FMath::Max(RegenRatePerSecond, 0.f);

	// The share healed from the run's start up to T: the area under a rate that rises in a straight line over the ramp,
	// then stays level.
	const auto Healed = [Ramp, Rate](float T)
	{
		T = FMath::Max(T, 0.f);
		if (Ramp <= UE_KINDA_SMALL_NUMBER)
		{
			return Rate * T;
		}
		return T < Ramp ? Rate * T * T / (2.f * Ramp) : Rate * (Ramp * 0.5f + (T - Ramp));
	};
	return FMath::Max(Healed(To) - Healed(From), 0.f);
}

float FRecoverySettings::DropChance(ECreatureRank Rank) const
{
	switch (Rank)
	{
	case ECreatureRank::Basic:
		return DropChanceBasic;
	case ECreatureRank::Rare:
		return DropChanceRestless;
	case ECreatureRank::Epic:
	case ECreatureRank::Legendary:
		return DropChanceGravebound;
	case ECreatureRank::Boss:
		return 1.f;
	}
	return DropChanceBasic;
}

int32 FRecoverySettings::RollMotes(ECreatureRank Rank, FRandomStream& Random) const
{
	if (Rank == ECreatureRank::Boss)
	{
		const int32 Fewest = FMath::Max(BossMotesMin, 0);
		return Random.RandRange(Fewest, FMath::Max(BossMotesMax, Fewest));
	}
	return Random.FRand() < DropChance(Rank) ? 1 : 0;
}

float FRecoverySettings::MoteVisible(float Age) const
{
	const float Fade = FMath::Max(MoteFadeSeconds, 0.f);
	const float Left = MoteLifeSeconds - Age;
	if (Left >= Fade)
	{
		return 1.f;
	}
	if (Left <= 0.f || Fade <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	// Slowly at first and quicker at the end, like a flame guttering out.
	return FMath::InterpEaseOut(0.f, 1.f, Left / Fade, 1.6f);
}
