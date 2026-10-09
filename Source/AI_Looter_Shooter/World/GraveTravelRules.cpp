#include "World/GraveTravelRules.h"

#define LOCTEXT_NAMESPACE "GraveTravel"

namespace GraveTravelRules
{
	EGraveTravelBlock CheckNow(const FGraveTravelSenses& Senses)
	{
		// The most final reason first: a dead player can't be in a fight worth naming, and a scene outranks the fight it
		// may be showing (a boss's entrance).
		if (!Senses.bHasPlayer)
		{
			return EGraveTravelBlock::NoPlayer;
		}
		if (Senses.bDying)
		{
			return EGraveTravelBlock::Dying;
		}
		if (Senses.bTravelling)
		{
			return EGraveTravelBlock::Travelling;
		}
		if (Senses.bInScene)
		{
			return EGraveTravelBlock::Scene;
		}
		if (Senses.bBossFight)
		{
			return EGraveTravelBlock::BossFight;
		}
		if (Senses.Hunters > 0)
		{
			return EGraveTravelBlock::Hunted;
		}
		return EGraveTravelBlock::None;
	}

	EGraveTravelBlock CheckGrave(const FGraveTravelSenses& Senses, bool bGraveOpen, double DistanceToGrave)
	{
		const EGraveTravelBlock Now = CheckNow(Senses);
		if (Now != EGraveTravelBlock::None)
		{
			return Now;
		}
		if (!bGraveOpen)
		{
			return EGraveTravelBlock::GraveClosed;
		}
		if (DistanceToGrave < AlreadyThereDistance)
		{
			return EGraveTravelBlock::AlreadyThere;
		}
		return EGraveTravelBlock::None;
	}

	FText Reason(EGraveTravelBlock Block)
	{
		switch (Block)
		{
		case EGraveTravelBlock::NoPlayer:     return LOCTEXT("NoPlayer", "There's nobody to send.");
		case EGraveTravelBlock::Dying:        return LOCTEXT("Dying", "Not while you're down.");
		case EGraveTravelBlock::Scene:        return LOCTEXT("Scene", "Not now: the story has you here.");
		case EGraveTravelBlock::BossFight:    return LOCTEXT("BossFight", "Not in the middle of a boss fight.");
		case EGraveTravelBlock::Hunted:       return LOCTEXT("Hunted", "Something is hunting you. Lose it or put it down first.");
		case EGraveTravelBlock::Travelling:   return LOCTEXT("Travelling", "You're already on your way.");
		case EGraveTravelBlock::GraveClosed:  return LOCTEXT("GraveClosed", "This grave hasn't opened to you yet.");
		case EGraveTravelBlock::AlreadyThere: return LOCTEXT("AlreadyThere", "You're standing at it.");
		case EGraveTravelBlock::None:         break;
		}
		return FText::GetEmpty();
	}

	FVector ArrivalLocation(const FVector& GraveSpot, float HalfHeight)
	{
		return GraveSpot + FVector(0.0, 0.0, HalfHeight);
	}
}

#undef LOCTEXT_NAMESPACE
