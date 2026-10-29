#include "Creatures/FightPressureRules.h"

// ---------------------------------------------------------------------------
// The watches
// ---------------------------------------------------------------------------

void FFightStillWatch::Note(const FVector& Where, double Now, float Radius)
{
	// A circle round where they were when they last left one: wandering inside it (a step, a turn, a crouch) is still standing
	// there; leaving it starts a new one.
	if (!bStarted || FVector::DistSquared2D(Where, Anchor) > FMath::Square(static_cast<double>(FMath::Max(Radius, 0.f))))
	{
		Anchor = Where;
		Since = Now;
		bStarted = true;
	}
}

float FFightStillWatch::StillFor(double Now) const
{
	return bStarted ? static_cast<float>(FMath::Max(Now - Since, 0.0)) : 0.f;
}

void FFightSightWatch::Note(bool bSeen, double Now)
{
	if (bSeen || !bStarted)
	{
		LastSeen = Now;
		bStarted = true;
	}
}

float FFightSightWatch::HiddenFor(double Now) const
{
	return bStarted ? static_cast<float>(FMath::Max(Now - LastSeen, 0.0)) : 0.f;
}

bool FFightEaseWatch::TryOpen(float HealthShare, double Now, bool bOn, float Below, float Seconds, float Gap)
{
	if (!bOn || Seconds <= 0.f || HealthShare >= Below || IsEasing(Now) || Now - Last < FMath::Max(Gap, 0.f))
	{
		return false;
	}
	Last = Now;
	Until = Now + Seconds;
	return true;
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

bool FightPressureRules::IsCamping(float StillFor, float HiddenFor, int32 Fighters, float StillSeconds, float HiddenSeconds, int32 MinFighters)
{
	return Fighters >= FMath::Max(MinFighters, 1) && (StillFor >= StillSeconds || HiddenFor >= HiddenSeconds);
}

float FightPressureRules::HealthShare(float Health, float MaxHealth, float Ward, float WardCapacity)
{
	const float Most = FMath::Max(MaxHealth, 0.f) + FMath::Max(WardCapacity, 0.f);
	if (Most <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp((FMath::Max(Health, 0.f) + FMath::Max(Ward, 0.f)) / Most, 0.f, 1.f);
}

int32 FightPressureRules::EasedTokens(int32 Tokens, bool bEasing)
{
	return bEasing ? FMath::Max(Tokens - 1, 1) : Tokens;
}
