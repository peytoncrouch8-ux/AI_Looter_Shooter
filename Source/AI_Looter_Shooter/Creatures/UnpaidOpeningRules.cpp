#include "Creatures/UnpaidOpeningRules.h"

// The Unpaid's punish of an opening (a reload, a swap, an empty gun, the flask at the lips) as plain rules (2026-10-31): the
// gate per player, who may claim it, and how the moves weigh while the player is open. UnpaidCreatureOpening.cpp plays it.

namespace
{
	/** At an open player: the lunge and the phase-strike this many times likelier, the feint this share, no hang-back. */
	constexpr float OpenLunge = 1.5f;
	constexpr float OpenStrike = 1.4f;
	constexpr float OpenFeint = 0.2f;
	/** The punisher: in front of their view it blinks round to their back; behind them it lunges. */
	constexpr float PunisherStrike = 3.f;
	constexpr float PunisherLunge = 2.2f;
}

bool UnpaidOpeningRules::Step(FUnpaidOpeningGate& Gate, bool bOpenNow, double Now, float Gap, float ClaimWindow)
{
	const bool bBegan = bOpenNow && !Gate.bOpen;
	Gate.bOpen = bOpenNow;
	if (bBegan)
	{
		Gate.OpenSince = Now;
		Gate.bDue = Now - Gate.LastPunish >= FMath::Max(Gap, 0.f);
	}
	else if (!bOpenNow)
	{
		Gate.OpenSince = -1.0;
		Gate.bDue = false;
	}
	// Only its first moments: a long reload isn't punished late, out of nowhere.
	if (Gate.bDue && Now - Gate.OpenSince > FMath::Max(ClaimWindow, 0.f))
	{
		Gate.bDue = false;
	}
	Gate.ReadAt = Now;
	return bBegan;
}

bool UnpaidOpeningRules::MayClaim(float Frontness, float Distance, float Range, float Near, float MaxFront)
{
	if (Distance > Range)
	{
		return false;
	}
	// Behind them or at a side: where they aren't looking while their hands are busy. Right on them, anywhere.
	return Distance <= Near || Frontness <= MaxFront;
}

bool UnpaidOpeningRules::Claim(FUnpaidOpeningGate& Gate, uint32 Id, double Now)
{
	if (!Gate.bDue || Id == 0)
	{
		return false;
	}
	Gate.bDue = false;
	Gate.Punisher = Id;
	Gate.LastPunish = Now;
	return true;
}

bool UnpaidOpeningRules::IsPunisher(const FUnpaidOpeningGate& Gate, uint32 Id, double Now, float Seconds)
{
	return Id != 0 && Gate.Punisher == Id && Now - Gate.LastPunish <= Seconds;
}

float UnpaidOpeningRules::WeightScale(EUnpaidMove Move, bool bOpen, bool bPunisher, bool bInFront)
{
	if (!bOpen && !bPunisher)
	{
		return 1.f;
	}
	switch (Move)
	{
	case EUnpaidMove::Lunge:
		return bPunisher && !bInFront ? PunisherLunge : OpenLunge;
	case EUnpaidMove::PhaseStrike:
		return bPunisher && bInFront ? PunisherStrike : OpenStrike;
	case EUnpaidMove::Feint:
		return OpenFeint;
	case EUnpaidMove::HangBack:
		return 0.f;
	default:
		return 1.f;
	}
}
