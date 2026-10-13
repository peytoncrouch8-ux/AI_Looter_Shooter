#include "Progression/XPCurve.h"

namespace
{
	/**
	 * No climb ever asks for more than this in all. It only matters for wild settings (a steep power over hundreds of
	 * levels) and keeps every total, and a player's XP with a huge gain added, far inside int64.
	 */
	constexpr double MaxTotalXP = 1e15;
}

int64 FXPCurve::TotalXPToReach(int32 Level) const
{
	const int32 Reached = FMath::Clamp(Level, 1, FMath::Max(MaxLevel, 1));
	if (Reached <= 1)
	{
		return 0;
	}
	// Never flatter than a straight line, never under a point a level: later levels always take at least as long.
	const double Scale = FMath::Max(CurveScale, 1.0);
	const double Power = FMath::Max(CurvePower, 1.0);
	const double Total = Scale * (FMath::Pow(static_cast<double>(Reached), Power) - 1.0);
	return static_cast<int64>(FMath::RoundToDouble(FMath::Min(Total, MaxTotalXP)));
}

int64 FXPCurve::XPToNextLevel(int32 Level) const
{
	if (IsMaxLevel(Level))
	{
		return 0;
	}
	// The difference of two rounded totals, so the steps always add up to the totals exactly.
	const int32 From = FMath::Max(Level, 1);
	return FMath::Max<int64>(TotalXPToReach(From + 1) - TotalXPToReach(From), 1);
}

float FXPCurve::LevelProgress(int32 Level, int64 XP) const
{
	const int64 Needed = XPToNextLevel(Level);
	return Needed > 0 ? FMath::Clamp(static_cast<float>(static_cast<double>(XP) / Needed), 0.f, 1.f) : 1.f;
}

int32 FXPCurve::ApplyXP(int32& Level, int64& XP, int64 Amount) const
{
	Clamp(Level, XP);
	if (Amount <= 0 || IsMaxLevel(Level))
	{
		return 0;
	}

	const int32 StartLevel = Level;
	// Even an absurd amount (a typo in a console command) can't overflow: no climb takes anywhere near this much.
	XP += FMath::Min<int64>(Amount, static_cast<int64>(MaxTotalXP));
	// One step per level crossed: a big reward (or a console command) can cover several at once.
	for (int64 Needed = XPToNextLevel(Level); Needed > 0 && XP >= Needed; Needed = XPToNextLevel(Level))
	{
		XP -= Needed;
		++Level;
	}
	if (IsMaxLevel(Level))
	{
		XP = 0;
	}
	return Level - StartLevel;
}

void FXPCurve::Clamp(int32& Level, int64& XP) const
{
	Level = FMath::Clamp(Level, 1, FMath::Max(MaxLevel, 1));
	const int64 Needed = XPToNextLevel(Level);
	XP = Needed > 0 ? FMath::Clamp<int64>(XP, 0, Needed - 1) : 0;
}
