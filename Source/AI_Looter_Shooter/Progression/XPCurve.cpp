#include "Progression/XPCurve.h"

namespace
{
	/**
	 * No single level ever asks for more than this. It only matters for wild settings (a steep growth over hundreds of
	 * levels) and keeps every total far inside int64.
	 */
	constexpr double MaxStepXP = 1e12;
}

int64 FXPCurve::XPToNextLevel(int32 Level) const
{
	if (IsMaxLevel(Level))
	{
		return 0;
	}
	// A flat or shrinking curve would let late levels come as fast as early ones; never less than one point per level.
	const double Step = FMath::Max<int64>(BaseXP, 1) * FMath::Pow(FMath::Max(Growth, 1.0), static_cast<double>(FMath::Max(Level, 1) - 1));
	return static_cast<int64>(FMath::Max(FMath::RoundToDouble(FMath::Min(Step, MaxStepXP)), 1.0));
}

int64 FXPCurve::TotalXPToReach(int32 Level) const
{
	int64 Total = 0;
	for (int32 From = 1; From < FMath::Min(Level, MaxLevel); ++From)
	{
		Total += XPToNextLevel(From);
	}
	return Total;
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
	XP += FMath::Min<int64>(Amount, static_cast<int64>(MaxStepXP) * 1000);
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
