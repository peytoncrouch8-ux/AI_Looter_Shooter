#include "Creatures/CreatureReactRules.h"

// ---------------------------------------------------------------------------
// The notice beat
// ---------------------------------------------------------------------------

float FCreatureNotice::LengthFor(float Seed01, bool bPointBlank)
{
	if (bPointBlank)
	{
		// Walked up on: it reacts, but no free half second for whoever is already at its fangs.
		return PointBlankSeconds;
	}
	return FMath::Lerp(ShortestSeconds, LongestSeconds, FMath::Clamp(Seed01, 0.f, 1.f));
}

bool FCreatureNotice::MayNotice(ECreatureState From, bool bHurt, bool bBoss, bool bChasedThisHunt)
{
	// Calm, walking home, or watching from its edge without ever having come at them: its first sight of them.
	const bool bFirstSight = From == ECreatureState::Idle || From == ECreatureState::Wander || From == ECreatureState::Return
		|| From == ECreatureState::Search;
	return bFirstSight && !bHurt && !bBoss && !bChasedThisHunt;
}

void FCreatureNotice::Begin(float Seconds)
{
	Length = FMath::Max(Seconds, 0.05f);
	Time = 0.f;
	bHolding = true;
	++Count;
}

void FCreatureNotice::Advance(float DeltaSeconds)
{
	if (!bHolding)
	{
		return;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	if (Time >= Length)
	{
		bHolding = false;
	}
}

void FCreatureNotice::Stop()
{
	bHolding = false;
}

void FCreatureNotice::EndHunt()
{
	bHolding = false;
	bChasedThisHunt = false;
}

float FCreatureNotice::GetAlpha() const
{
	return bHolding ? FMath::Clamp(Time / Length, 0.f, 1.f) : 0.f;
}

float FCreatureNotice::GetStrength() const
{
	if (!bHolding)
	{
		return 0.f;
	}
	const float Alpha = GetAlpha();
	const float In = FMath::SmoothStep(0.f, RiseShare, Alpha);
	const float Out = 1.f - FMath::SmoothStep(1.f - FallShare, 1.f, Alpha);
	return In * Out;
}

// ---------------------------------------------------------------------------
// The hit flash
// ---------------------------------------------------------------------------

float FCreatureHitFlash::ShareFor(float GapSeconds, bool bInCritical)
{
	const float Repeat = bInCritical ? CriticalRepeatShare : RepeatShare;
	return FMath::Lerp(Repeat, 1.f, FMath::SmoothStep(RepeatSeconds, QuietSeconds, GapSeconds));
}

void FCreatureHitFlash::Start(bool bInCritical)
{
	// Time is the gap since the last hit: steady fire only nudges the flash, a shot after a pause flashes in full.
	// Crits streaming into a head (a rifle on a spider's eyes: every round a crit) are steady fire too; only a crit among body
	// hits keeps its stronger repeat (the play-check, 2026-10-23: a crit stream washed the legs pale).
	const bool bCritRun = bInCritical && bLastHitCritical;
	bLastHitCritical = bInCritical;
	const float New = (bInCritical ? CriticalStrength : NormalStrength) * ShareFor(Time, bInCritical && !bCritRun);
	// A shotgun's pellets land together: the strongest flash stands, they don't add up.
	Strength = IsOn() ? FMath::Max(Strength * FMath::Square(1.f - Time / Seconds), New) : New;
	bCritical = bInCritical || (IsOn() && bCritical);
	Time = 0.f;
}

void FCreatureHitFlash::Advance(float DeltaSeconds)
{
	Time = FMath::Min(Time + FMath::Max(DeltaSeconds, 0.f), 1000.f);
}

float FCreatureHitFlash::GetIntensity() const
{
	if (!IsOn())
	{
		return 0.f;
	}
	return Glow * Strength * FMath::Square(1.f - Time / Seconds);
}

// ---------------------------------------------------------------------------
// The level's allowance for dust and ash
// ---------------------------------------------------------------------------

int32 FCreatureDustBudget::Take(double Now, int32 Wanted)
{
	if (Tokens < 0.f)
	{
		Tokens = Burst;
		LastTime = Now;
	}
	const double Since = FMath::Max(Now - LastTime, 0.0);
	Tokens = FMath::Min(Burst, Tokens + static_cast<float>(Since) * PerSecond);
	LastTime = Now;
	const int32 Granted = FMath::Clamp(Wanted, 0, FMath::FloorToInt32(Tokens));
	Tokens -= static_cast<float>(Granted);
	return Granted;
}
