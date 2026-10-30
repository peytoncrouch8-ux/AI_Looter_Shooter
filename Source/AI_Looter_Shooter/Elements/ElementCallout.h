#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementRules.h"
#include "Elements/LooterElement.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;
class UDamageType;
class UObject;

/**
 * How one floating damage number shows its element (UHealthComponent::SpawnDamageNumber works it out, ADamageNumberActor and
 * UDamageNumberWidget play it): an element status's piece or a smite takes the element's colour; a critical hit of an
 * element gun keeps its crit look, tinted toward the element's; a plain hit of one is unchanged. A hit on a creature weak to
 * the element, once a fight, pops a "WEAK" tag beside a slightly bigger number; a resisted one pops a greyed "RESIST", an immune one a greyed "NO EFFECT".
 */
struct FElementNumberLook
{
	/** The element the hit carried (None: an ordinary number). */
	ELooterElement Element = ELooterElement::None;
	/** The number takes the element's colour (a status's piece or smite; a crit's settled colour is tinted toward it). */
	bool bTint = false;
	/** Weak or Resist: the tag popped beside the number now (Neutral: none). */
	EElementAffinity Callout = EElementAffinity::Neutral;
	/** The number's size times this (the weak hit's, a little bigger). */
	float SizeScale = 1.f;
};

/**
 * The element a gun's hit carries while its damage is being dealt (UBulletSubsystem::ResolveHit, a rocket's blast): the
 * damage type is the same for every gun's hit, so the number has no other way to know. Set around ApplyPointDamage.
 */
struct AI_LOOTER_SHOOTER_API FHitElementScope
{
	explicit FHitElementScope(ELooterElement Element);
	~FHitElementScope();

	/** The element of the hit being dealt now (None outside any scope, or for a gun with no element). */
	static ELooterElement Current() { return Active; }

private:
	ELooterElement Previous = ELooterElement::None;
	static ELooterElement Active;
};

/**
 * Which creatures have had their weakness (or resistance) called out in the fight they are in, so a number says WEAK once
 * and a rifle's stream of hits doesn't repeat it. A fight goes on while element hits keep coming; FightSeconds without one
 * ends it, and the next calls it out again. Plain numbers and weak pointers (the tests drive it without a world).
 */
class AI_LOOTER_SHOOTER_API FElementCalloutLog
{
public:
	/**
	 * Whether Affinity of Element on Creature is called out now (the fight's first such hit); the hit counts as part of the
	 * fight either way. Never for a neutral hit or no element. Now is the world's time: a clock that went back (a new world)
	 * starts over.
	 */
	bool TryClaim(const UObject* Creature, ELooterElement Element, EElementAffinity Affinity, double Now, float FightSeconds);

	void Reset() { Entries.Reset(); }
	int32 Num() const { return Entries.Num(); }

private:
	struct FEntry
	{
		TWeakObjectPtr<const UObject> Creature;
		ELooterElement Element = ELooterElement::None;
		EElementAffinity Affinity = EElementAffinity::Neutral;
		double Last = 0.0;
	};

	void Prune(double Now, float FightSeconds);

	TArray<FEntry> Entries;
	static constexpr int32 MaxEntries = 48;
};

/** The damage numbers' element rules. */
namespace ElementCallout
{
	/** A weak hit's number is this much bigger, once a fight. */
	inline constexpr float WeakSize = 1.2f;
	/** A fight ends this long after the last element hit on a creature. */
	inline constexpr float FightSeconds = 20.f;
	/** How far a crit's settled colour leans from the crit orange to its element's colour. */
	inline constexpr float CritTint = 0.65f;

	/**
	 * A number's look from plain facts: the element a gun's hit carried (GunElement), the one a status dealt (StatusElement,
	 * which wins), whether it was critical, where the target stands against the element, and whether the weakness or
	 * resistance has not been called out yet this fight.
	 */
	AI_LOOTER_SHOOTER_API FElementNumberLook Judge(ELooterElement GunElement, ELooterElement StatusElement, bool bCritical,
		EElementAffinity Affinity, bool bFirstInFight);

	/** The element of a status's damage: what UElementStatusComponent is dealing now, or an element damage type's own. None for the rest. */
	AI_LOOTER_SHOOTER_API ELooterElement StatusElementOf(const UDamageType* DamageType);

	/** The look of a number for damage of DamageType on Target at world time Now (the gun's hit element from FHitElementScope), logging the call-out. */
	AI_LOOTER_SHOOTER_API FElementNumberLook LookFor(const UDamageType* DamageType, const AActor* Target, bool bCritical, double Now);

	/** The log LookFor uses. */
	AI_LOOTER_SHOOTER_API FElementCalloutLog& Log();

	/**
	 * A number's colour: the element's for a status's piece, the crit orange tinted toward it for a crit, the crit's (heat:
	 * 1 white-hot to 0 settled) or the plain text colour otherwise.
	 */
	AI_LOOTER_SHOOTER_API FLinearColor NumberColor(const FElementNumberLook& Look, bool bCritical, float Heat);

	/** The tag's text ("WEAK", "RESIST", "NO EFFECT"; empty for Neutral) and colour (the element's for weak, greyed otherwise). */
	AI_LOOTER_SHOOTER_API FText TagText(EElementAffinity Affinity);
	AI_LOOTER_SHOOTER_API FLinearColor TagColor(ELooterElement Element, EElementAffinity Affinity);
}
