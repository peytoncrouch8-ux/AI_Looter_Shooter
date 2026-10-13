// FControlHintRules: when hints come and go, and what they remember. The hints' table (keys, words, saved names, how
// often each may show) is ControlHintRulesTable.cpp's.

#include "Tutorial/ControlHintRules.h"

namespace
{
	constexpr int32 HintCount = static_cast<int32>(EControlHint::Count);

	/** A step this small (cm) between looks is standing still: a capsule settling, a nudge from a creature. */
	constexpr float StillStep = 1.f;
}

// ---------------------------------------------------------------------------
// Each look
// ---------------------------------------------------------------------------

void FControlHintRules::Update(const FControlHintInput& In, float DeltaSeconds)
{
	// What the player did counts whenever it happens, under a menu too: the inventory hint's control is opening one.
	LearnFrom(In);
	if (!In.bEnabled)
	{
		if (IsShowing())
		{
			End(EControlHintEnd::Hidden);
		}
		return;
	}
	// A menu, a scene or death holds the player: nothing moves on, and the hint on show waits (the widget hides it).
	if (!In.bInControl)
	{
		return;
	}
	Notice(In, DeltaSeconds);
	for (float& Cooldown : Cooldowns)
	{
		Cooldown = FMath::Max(Cooldown - DeltaSeconds, 0.f);
	}
	// Someone is talking: nothing new comes over the conversation, and the hint on show waits it out with its time
	// stopped (the widget steps aside meanwhile).
	if (In.bCaptionsOn)
	{
		return;
	}

	if (IsShowing())
	{
		ShownFor += DeltaSeconds;
		const bool bMomentary = IsMomentary(Shown);
		MomentGone = bMomentary && !Wants(Shown, In) ? MomentGone + DeltaSeconds : 0.f;
		if (ShownFor >= ShowSeconds || (bMomentary && MomentGone >= LingerSeconds))
		{
			// Unheeded: it may come again later, until it has shown its most.
			Cooldowns[IndexOf(Shown)] = RetrySeconds;
			End(EControlHintEnd::TimedOut);
		}
		return;
	}

	SinceEnd += DeltaSeconds;
	if (SinceEnd < GapSeconds)
	{
		return;
	}
	// The most pressing hint due comes first (the enum's order).
	for (int32 Index = 0; Index < HintCount; ++Index)
	{
		const EControlHint Hint = static_cast<EControlHint>(Index);
		if (!IsRetired(Hint) && Cooldowns[Index] <= 0.f && Wants(Hint, In))
		{
			Show(Hint);
			return;
		}
	}
}

void FControlHintRules::LearnFrom(const FControlHintInput& In)
{
	MovedTotal += FMath::Max(In.Moved, 0.f);
	if (MovedTotal >= LearnMoveDistance)
	{
		Learn(EControlHint::Move);
	}
	if (In.bSprinting)
	{
		Learn(EControlHint::Sprint);
	}
	if (In.bSliding)
	{
		Learn(EControlHint::Slide);
	}
	if (In.bCrouching)
	{
		Learn(EControlHint::Crouch);
	}
	if (In.bJumped)
	{
		Learn(EControlHint::Jump);
	}
	if (In.bFired)
	{
		Learn(EControlHint::Fire);
	}
	if (In.bAiming)
	{
		Learn(EControlHint::Aim);
	}
	if (In.bReloading)
	{
		Learn(EControlHint::Reload);
	}
	if (In.bMeleed)
	{
		Learn(EControlHint::Melee);
	}
	if (In.bThrew)
	{
		Learn(EControlHint::Grenade);
	}
	if (In.bInteracted || In.bHeldUse)
	{
		Learn(EControlHint::Interact);
	}
	if (In.bHeldUse)
	{
		Learn(EControlHint::Hold);
	}
	if (In.bSwapped)
	{
		Learn(EControlHint::Swap);
	}
	if (In.bInventoryOpen)
	{
		Learn(EControlHint::Inventory);
		bInventoryDue = false;
	}
	// The inventory's pages: each one opened is its lesson (the inventory's own comes with it).
	if (In.bLedgerOpen)
	{
		Learn(EControlHint::Ledger);
	}
	if (In.bMissionsOpen)
	{
		Learn(EControlHint::Missions);
	}
	if (In.bMapOpen)
	{
		Learn(EControlHint::Map);
	}
	if (In.bBenchOpen)
	{
		Learn(EControlHint::Bench);
	}
}

void FControlHintRules::Notice(const FControlHintInput& In, float DeltaSeconds)
{
	// Standing still since control came is the move hint's clock; any real step starts it over (and soon learns it).
	IdleFor = In.Moved > StillStep ? 0.f : IdleFor + DeltaSeconds;
	// Walking time since the last sprint, kept through stops (a player who never sprints still gets there).
	if (In.bSprinting)
	{
		WalkFor = 0.f;
	}
	else if (In.bWalking)
	{
		WalkFor += DeltaSeconds;
	}
	// One unbroken sprint's time.
	SprintFor = In.bSprinting ? SprintFor + DeltaSeconds : 0.f;
	// One unbroken aim at a target while standing; one unbroken look at something to press, or to hold.
	AimAtTargetFor = In.bAiming && In.bTargetInSights && !In.bCrouching ? AimAtTargetFor + DeltaSeconds : 0.f;
	TappableFor = In.bTapFocused ? TappableFor + DeltaSeconds : 0.f;
	HoldableFor = In.bHoldFocused ? HoldableFor + DeltaSeconds : 0.f;
	// Two missions going at once (past the new one's banner).
	TwoMissionsFor = In.MissionsGoing >= 2 ? TwoMissionsFor + DeltaSeconds : 0.f;
	// Walking toward a far objective, kept through stops (a fight on the way); an objective near at hand starts it over.
	if (!In.bFarWaypoint)
	{
		FarWalkFor = 0.f;
	}
	else if (In.bWalking || In.bSprinting)
	{
		FarWalkFor += DeltaSeconds;
	}
	if (In.bPickedUpGun && !GetMemory(EControlHint::Inventory).bLearned)
	{
		bInventoryDue = true;
	}
	if (In.bGrenadesGained && !GetMemory(EControlHint::Grenade).bLearned)
	{
		bGrenadeDue = true;
	}
}

bool FControlHintRules::Wants(EControlHint Hint, const FControlHintInput& In) const
{
	switch (Hint)
	{
	case EControlHint::Move:      return IdleFor >= MoveIdleSeconds;
	case EControlHint::Reload:    return In.bGunInHand && In.bMagazineLow && !In.bReloading;
	case EControlHint::Fire:      return In.bGunInHand && In.bTargetInSights;
	// Fists work too, so no gun needed. The sense only reports a close creature to a body that hasn't struck yet.
	case EControlHint::Melee:     return In.bCloseTarget;
	// The sense only reports a crowd to a body with a grenade in hand that hasn't thrown one yet; a pickup asks once.
	case EControlHint::Grenade:   return In.bCrowdAhead || bGrenadeDue;
	case EControlHint::Aim:       return In.bGunInHand && In.bFarTarget && !In.bAiming;
	case EControlHint::Crouch:    return AimAtTargetFor >= CrouchAimSeconds;
	case EControlHint::Jump:      return In.bLedgeAhead;
	case EControlHint::Interact:  return TappableFor >= InteractDwellSeconds;
	// Loot takes a tap and a hold: pressing comes first, the hold (taking it in hand) once pressing is known.
	case EControlHint::Hold:      return HoldableFor >= HoldDwellSeconds && (!In.bTapFocused || GetMemory(EControlHint::Interact).bLearned);
	case EControlHint::Sprint:    return WalkFor >= WalkSeconds;
	case EControlHint::Slide:     return SprintFor >= SlideSprintSeconds;
	case EControlHint::Swap:      return In.GunsEquipped >= 2;
	case EControlHint::Inventory: return bInventoryDue || In.bAskedInventory;
	case EControlHint::Ledger:    return In.bAskedLedger;
	case EControlHint::Missions:  return TwoMissionsFor >= MissionsSettleSeconds || In.bAskedMissions;
	case EControlHint::Map:       return FarWalkFor >= MapWalkSeconds;
	case EControlHint::Bench:     return In.bAtBenchWithPair;
	case EControlHint::Count:     break;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Showing and ending
// ---------------------------------------------------------------------------

void FControlHintRules::Show(EControlHint Hint)
{
	Shown = Hint;
	ShownFor = 0.f;
	MomentGone = 0.f;
	++Memories[IndexOf(Hint)].Shows;
	bChanged = true;
	// A habit's clock starts over, so a hint that times out isn't due again the moment its cooldown ends.
	if (Hint == EControlHint::Move)
	{
		IdleFor = 0.f;
	}
	else if (Hint == EControlHint::Sprint)
	{
		WalkFor = 0.f;
	}
	else if (Hint == EControlHint::Slide)
	{
		SprintFor = 0.f;
	}
	else if (Hint == EControlHint::Missions)
	{
		TwoMissionsFor = 0.f;
	}
	else if (Hint == EControlHint::Map)
	{
		FarWalkFor = 0.f;
	}
}

void FControlHintRules::End(EControlHintEnd How)
{
	if (!IsShowing())
	{
		return;
	}
	// A pickup's grenade hint has had its show: only a crowd brings it back.
	if (Shown == EControlHint::Grenade)
	{
		bGrenadeDue = false;
	}
	LastEnded = Shown;
	LastEnd = How;
	++EndCount;
	Shown = EControlHint::Count;
	ShownFor = 0.f;
	MomentGone = 0.f;
	SinceEnd = 0.f;
}

// ---------------------------------------------------------------------------
// Memory
// ---------------------------------------------------------------------------

const FControlHintMemory& FControlHintRules::GetMemory(EControlHint Hint) const
{
	static const FControlHintMemory Blank;
	return Hint == EControlHint::Count ? Blank : Memories[IndexOf(Hint)];
}

void FControlHintRules::SetMemory(EControlHint Hint, const FControlHintMemory& InMemory)
{
	if (Hint != EControlHint::Count)
	{
		Memories[IndexOf(Hint)] = InMemory;
	}
}

bool FControlHintRules::IsRetired(EControlHint Hint) const
{
	const FControlHintMemory& Memory = GetMemory(Hint);
	return Memory.bLearned || Memory.Shows >= MaxShows(Hint);
}

void FControlHintRules::Learn(EControlHint Hint)
{
	if (Hint == EControlHint::Count)
	{
		return;
	}
	FControlHintMemory& Memory = Memories[IndexOf(Hint)];
	if (!Memory.bLearned)
	{
		Memory.bLearned = true;
		bChanged = true;
	}
	if (Shown == Hint)
	{
		End(EControlHintEnd::Done);
	}
}

void FControlHintRules::ForceShow(EControlHint Hint)
{
	if (Hint == EControlHint::Count)
	{
		return;
	}
	End(EControlHintEnd::Hidden);
	Show(Hint);
}

void FControlHintRules::ResetMemory()
{
	End(EControlHintEnd::Hidden);
	for (int32 Index = 0; Index < HintCount; ++Index)
	{
		Memories[Index] = FControlHintMemory();
		Cooldowns[Index] = 0.f;
	}
	IdleFor = 0.f;
	MovedTotal = 0.f;
	WalkFor = 0.f;
	SprintFor = 0.f;
	AimAtTargetFor = 0.f;
	TappableFor = 0.f;
	HoldableFor = 0.f;
	TwoMissionsFor = 0.f;
	FarWalkFor = 0.f;
	bInventoryDue = false;
	bGrenadeDue = false;
	bChanged = true;
}

bool FControlHintRules::ConsumeChanged()
{
	const bool bWas = bChanged;
	bChanged = false;
	return bWas;
}
