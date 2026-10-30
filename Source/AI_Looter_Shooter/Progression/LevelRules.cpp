#include "Progression/LevelRules.h"

namespace
{
	/**
	 * No single kill gives more than this. It only matters for wild settings (a steep power on a high level) and keeps the
	 * result far inside int64.
	 */
	constexpr double MaxKillXP = 1e12;
}

float FLevelRules::EnemyScale(int32 Level) const
{
	return Compounded(EnemyGrowth, Level);
}

float FLevelRules::Compounded(float Growth, int32 Level)
{
	// Each level a share of the one below (the user's +9% a level, compounding), so the step between two levels is the same
	// share however high they are.
	return FMath::Pow(1.f + FMath::Max(Growth, 0.f), static_cast<float>(FMath::Max(Level, 1) - 1));
}

float FLevelRules::PlayerHealthScale(int32 Level) const
{
	return Compounded(HealthPerLevel, Level);
}

double FLevelRules::KillXPShare(int32 EnemyLevel, int32 PlayerLevel) const
{
	const int32 LevelsBelow = PlayerLevel - EnemyLevel;
	if (LevelsBelow <= 0)
	{
		return 1.0;
	}
	const double LeastShare = FMath::Clamp(KillXPFloor, 0.0, 1.0);
	return FMath::Max(LeastShare, 1.0 - FMath::Max(KillXPFalloff, 0.0) * LevelsBelow);
}

int64 FLevelRules::KillXP(int32 BaseXP, int32 EnemyLevel, int32 PlayerLevel) const
{
	if (BaseXP <= 0)
	{
		return 0;
	}
	// A negative power would make tougher enemies worth less; never below flat.
	const double Grown = BaseXP * FMath::Max(KillXPScale, 0.0)
		* FMath::Pow(static_cast<double>(FMath::Max(EnemyLevel, 1)), FMath::Max(KillXPPower, 0.0));
	const double Earned = FMath::Min(Grown * KillXPShare(EnemyLevel, PlayerLevel), MaxKillXP);
	return FMath::Max<int64>(FMath::RoundToInt64(Earned), 1);
}
