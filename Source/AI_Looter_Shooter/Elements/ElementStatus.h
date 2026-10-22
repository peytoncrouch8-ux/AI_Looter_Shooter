#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Elements/LooterElement.h"

/** A piece of damage a status lets go of this step (FElementStatus::Advance). */
struct FElementTick
{
	ELooterElement Element = ELooterElement::None;
	float Damage = 0.f;
	/** A smite's burst (Soul-light): it leaps to the next of the dead. */
	bool bSmite = false;
};

/**
 * The statuses one creature carries, as plain numbers (the rules the tests drive; UElementStatusComponent plays them in the
 * world): its burns (Ember, up to EmberMaxStacks at once), its waiting smites (Soul-light) and its salt (Grave-salt, one at
 * a time: a new one keeps whichever is more). Damage over time builds continuously and comes off in pieces every
 * TickSeconds, the last piece when a status runs out, so what it deals in all is exactly its share of the hit. Bosses carry
 * each status for BossDuration of its time (a smite at BossDuration of its burst).
 */
class AI_LOOTER_SHOOTER_API FElementStatus
{
public:
	/** A hit of HitDamage (as it was dealt) set Element on it, where it stands Affinity to it. */
	void Apply(ELooterElement Element, float HitDamage, EElementAffinity Affinity, bool bBoss, const FElementTuning& Tuning);

	/** Moves its statuses on by DeltaSeconds; the damage due now is added to Out (at most one piece an element, plus smites). */
	void Advance(float DeltaSeconds, const FElementTuning& Tuning, TArray<FElementTick>& Out);

	/** Everything off at once (its owner died, or came back for a new life). */
	void Clear();

	bool IsActive() const;
	int32 NumBurns() const { return Burns.Num(); }
	int32 NumSmites() const { return Smites.Num(); }
	bool IsSalted() const { return Salt.TimeLeft > 0.f; }
	float GetSaltTimeLeft() const { return Salt.TimeLeft; }
	/** The soonest smite's wait so far (0 just set, 1 bursting); 0 with none waiting. */
	float GetSmiteCharge(const FElementTuning& Tuning) const;
	/** Damage still to come from Element's status (what's built up but not dealt yet included). */
	float GetDamageLeft(ELooterElement Element) const;
	/** Of the burns, the longest time left (s); 0 when not burning. */
	float GetBurnTimeLeft() const;

private:
	struct FOverTime
	{
		float DamageLeft = 0.f;
		float PerSecond = 0.f;
		float TimeLeft = 0.f;
	};
	struct FSmite
	{
		float Damage = 0.f;
		float Wait = 0.f;
	};

	/** Takes one step of an over-time status: what it deals in it, its time and damage counted down. */
	static float Drain(FOverTime& Status, float DeltaSeconds);

	TArray<FOverTime, TInlineAllocator<3>> Burns;
	TArray<FSmite, TInlineAllocator<4>> Smites;
	FOverTime Salt;
	/** Built up since the last piece came off. */
	float EmberOwed = 0.f;
	float SaltOwed = 0.f;
	float SinceTick = 0.f;
};
