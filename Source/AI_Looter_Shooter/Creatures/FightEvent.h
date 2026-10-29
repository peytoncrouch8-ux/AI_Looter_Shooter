#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"

class ACreatureBase;

/**
 * The moments of a fight the fight log counts (FIGHTS.md section 2, 2026-10-28; Dev/FightLog): the director and the kinds
 * note them through UFightDirectorSubsystem::NoteEvent, which hands them to OnFightEvent. Value is the event's own number
 * where it has one (AttackLanded: the damage dealt; Ease: the player's health share when it opened), else 0.
 */
enum class EFightEvent : uint8
{
	/** A creature was given a turn (melee or ranged) at its target (the director). */
	TurnTaken,
	/** A creature began an attack with its turn (CreatureBaseFight.cpp). */
	AttackStarted,
	/** A creature's attack hurt its victim (ACreatureBase::HitWithAttack; Value: the damage). */
	AttackLanded,
	/** A spider leapt at its target (lane BEASTS). */
	Pounce,
	/** A spider skittered back after its bite (lane BEASTS). */
	Skitter,
	/** A Spitter lobbed behind the player's cover (lane BEASTS). */
	Flush,
	/** A pack's leader died (PackLeaderRules::OnLeaderDown). */
	LeaderDown,
	/** A slime split into slimelets (lane SLIME). */
	Split,
	/** A slime hopped aside from fire (lane SLIME). */
	Dodge,
	/** A hurt Unpaid faded back out of the fight for a moment (lane UNPAID). */
	FadeBack,
	/** The close-call ease opened: fewer turns at a nearly dead player for a while (the director; Value: their health share). */
	Ease,
};

/** How many EFightEvent values there are (counts per event are kept in arrays this long). */
constexpr int32 NumFightEvents = static_cast<int32>(EFightEvent::Ease) + 1;

/** The event's name, for logs and the fight log's CSV. */
inline const TCHAR* FightEventName(EFightEvent Event)
{
	switch (Event)
	{
	case EFightEvent::TurnTaken: return TEXT("TurnTaken");
	case EFightEvent::AttackStarted: return TEXT("AttackStarted");
	case EFightEvent::AttackLanded: return TEXT("AttackLanded");
	case EFightEvent::Pounce: return TEXT("Pounce");
	case EFightEvent::Skitter: return TEXT("Skitter");
	case EFightEvent::Flush: return TEXT("Flush");
	case EFightEvent::LeaderDown: return TEXT("LeaderDown");
	case EFightEvent::Split: return TEXT("Split");
	case EFightEvent::Dodge: return TEXT("Dodge");
	case EFightEvent::FadeBack: return TEXT("FadeBack");
	case EFightEvent::Ease: return TEXT("Ease");
	}
	return TEXT("Unknown");
}

/** A fight's moment: who, what, and its number (native, so nothing is spent on it while nobody listens). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnFightEvent, const ACreatureBase& /*Creature*/, EFightEvent /*Event*/, float /*Value*/);
