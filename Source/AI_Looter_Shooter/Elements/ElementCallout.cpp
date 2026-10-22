#include "Elements/ElementCallout.h"
#include "Elements/ElementDamageType.h"
#include "Elements/ElementStatusComponent.h"
#include "UI/Style/LooterUIStyle.h"
#include "GameFramework/DamageType.h"

// The damage numbers' element rules: what a number shows of the element it carried, and the once-a-fight call-out of a
// weakness. Plain rules first; LookFor is the one place that reads the world's facts.

ELooterElement FHitElementScope::Active = ELooterElement::None;

FHitElementScope::FHitElementScope(ELooterElement Element)
	: Previous(Active)
{
	Active = Element;
}

FHitElementScope::~FHitElementScope()
{
	Active = Previous;
}

void FElementCalloutLog::Prune(double Now, float FightSeconds)
{
	Entries.RemoveAllSwap([Now, FightSeconds](const FEntry& Entry)
	{
		// Gone, over (a pause longer than a fight), or from a clock that has since started again.
		return !Entry.Creature.IsValid() || Now < Entry.Last || Now - Entry.Last > FightSeconds;
	});
}

bool FElementCalloutLog::TryClaim(const UObject* Creature, ELooterElement Element, EElementAffinity Affinity, double Now, float FightSeconds)
{
	if (!Creature || Element == ELooterElement::None || Affinity == EElementAffinity::Neutral)
	{
		return false;
	}
	Prune(Now, FightSeconds);
	for (FEntry& Entry : Entries)
	{
		if (Entry.Creature.Get() == Creature && Entry.Element == Element && Entry.Affinity == Affinity)
		{
			// The same fight going on: already said.
			Entry.Last = Now;
			return false;
		}
	}
	if (Entries.Num() >= MaxEntries)
	{
		// A crowd: the oldest fight's entry goes.
		int32 Oldest = 0;
		for (int32 Index = 1; Index < Entries.Num(); ++Index)
		{
			Oldest = Entries[Index].Last < Entries[Oldest].Last ? Index : Oldest;
		}
		Entries.RemoveAtSwap(Oldest);
	}
	FEntry& Added = Entries.AddDefaulted_GetRef();
	Added.Creature = Creature;
	Added.Element = Element;
	Added.Affinity = Affinity;
	Added.Last = Now;
	return true;
}

FElementNumberLook ElementCallout::Judge(ELooterElement GunElement, ELooterElement StatusElement, bool bCritical,
	EElementAffinity Affinity, bool bFirstInFight)
{
	FElementNumberLook Look;
	Look.Element = StatusElement != ELooterElement::None ? StatusElement : GunElement;
	if (Look.Element == ELooterElement::None)
	{
		return Look;
	}
	// A status's piece is the element's colour; an element gun's plain hit stays as it was, its crit only tinted.
	Look.bTint = StatusElement != ELooterElement::None || bCritical;
	if (Affinity != EElementAffinity::Neutral && bFirstInFight)
	{
		Look.Callout = Affinity;
		Look.SizeScale = Affinity == EElementAffinity::Weak ? WeakSize : 1.f;
	}
	return Look;
}

ELooterElement ElementCallout::StatusElementOf(const UDamageType* DamageType)
{
	// Grave-salt's piece deals the grenade's damage type (so it can't be told by type): the status component says.
	const ELooterElement Dealing = UElementStatusComponent::DealingElement();
	if (Dealing != ELooterElement::None)
	{
		return Dealing;
	}
	const UElementDamageType* Element = Cast<UElementDamageType>(DamageType);
	return Element ? Element->Element : ELooterElement::None;
}

FElementCalloutLog& ElementCallout::Log()
{
	static FElementCalloutLog TheLog;
	return TheLog;
}

FElementNumberLook ElementCallout::LookFor(const UDamageType* DamageType, const AActor* Target, bool bCritical, double Now)
{
	const ELooterElement Status = StatusElementOf(DamageType);
	const ELooterElement Gun = FHitElementScope::Current();
	const ELooterElement Element = Status != ELooterElement::None ? Status : Gun;
	if (Element == ELooterElement::None)
	{
		return FElementNumberLook();
	}
	// A critical hit finds an Unpaid's coal (Grave-salt's weak spot); a status's piece is never critical.
	const EElementAffinity Affinity = ElementRules::Affinity(Element, ElementRules::TargetOf(Target), bCritical && Status == ELooterElement::None);
	const bool bFirst = Affinity != EElementAffinity::Neutral && Log().TryClaim(Target, Element, Affinity, Now, FightSeconds);
	return Judge(Gun, Status, bCritical, Affinity, bFirst);
}

FLinearColor ElementCallout::NumberColor(const FElementNumberLook& Look, bool bCritical, float Heat)
{
	const float Hot = FMath::Clamp(Heat, 0.f, 1.f);
	const bool bTinted = Look.bTint && Look.Element != ELooterElement::None;
	if (bCritical)
	{
		const FLinearColor Settled = bTinted ? FMath::Lerp(LooterUI::Color::Accent(), ElementRules::UiColor(Look.Element), CritTint) : LooterUI::Color::Accent();
		return FMath::Lerp(Settled, FLinearColor::White, Hot);
	}
	return bTinted ? ElementRules::UiColor(Look.Element) : LooterUI::Color::Text();
}

FText ElementCallout::TagText(EElementAffinity Affinity)
{
	switch (Affinity)
	{
	case EElementAffinity::Weak: return NSLOCTEXT("Looter", "ElementWeakTag", "WEAK");
	case EElementAffinity::Resist: return NSLOCTEXT("Looter", "ElementResistTag", "RESIST");
	default: return FText::GetEmpty();
	}
}

FLinearColor ElementCallout::TagColor(ELooterElement Element, EElementAffinity Affinity)
{
	if (Affinity == EElementAffinity::Weak)
	{
		return ElementRules::UiColor(Element);
	}
	// A resist is greyed: the dim text colour, faded.
	FLinearColor Grey = LooterUI::Color::TextDim();
	Grey.A *= 0.75f;
	return Grey;
}
