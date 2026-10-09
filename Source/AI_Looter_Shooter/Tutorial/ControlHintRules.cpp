#include "Tutorial/ControlHintRules.h"

#define LOCTEXT_NAMESPACE "ControlHints"

namespace
{
	constexpr int32 HintCount = static_cast<int32>(EControlHint::Count);

	/** A step this small (cm) between looks is standing still: a capsule settling, a nudge from a creature. */
	constexpr float StillStep = 1.f;
}

// ---------------------------------------------------------------------------
// The hints' table
// ---------------------------------------------------------------------------

int32 FControlHintRules::MaxShows(EControlHint Hint)
{
	return Hint == EControlHint::Jump || Hint == EControlHint::Slide ? 1 : 3;
}

FName FControlHintRules::Action(EControlHint Hint)
{
	switch (Hint)
	{
	case EControlHint::Move:      return TEXT("Move");
	case EControlHint::Reload:    return TEXT("Reload");
	// UKeyBindingSubsystem::MeleeBindingId(), spelled out here as the others are so the rules need no subsystem.
	case EControlHint::Melee:     return TEXT("Melee");
	// UKeyBindingSubsystem::GrenadeBindingId().
	case EControlHint::Grenade:   return TEXT("Grenade");
	case EControlHint::Aim:       return TEXT("Aim");
	case EControlHint::Jump:      return TEXT("Jump");
	case EControlHint::Sprint:    return TEXT("Sprint");
	case EControlHint::Slide:     return TEXT("Crouch");
	case EControlHint::Swap:      return TEXT("NextWeapon");
	case EControlHint::Inventory: return TEXT("Inventory");
	case EControlHint::Bench:     return TEXT("Interact");
	case EControlHint::Count:     break;
	}
	return NAME_None;
}

FText FControlHintRules::Words(EControlHint Hint, bool bToggle)
{
	// Short, as the HUD's floating lines are: the keycap says which key, these say what it does.
	switch (Hint)
	{
	case EControlHint::Move:      return LOCTEXT("Move", "Move, and look around with the mouse");
	case EControlHint::Reload:    return LOCTEXT("Reload", "Reload");
	case EControlHint::Melee:     return LOCTEXT("Melee", "Melee: strike what gets too close");
	case EControlHint::Grenade:   return LOCTEXT("Grenade", "Grenade: salt the crowd");
	case EControlHint::Aim:       return LOCTEXT("Aim", "Aim down the sights for a long shot");
	// Where the mantle is, a jump at a ledge climbs it; where it isn't, a ledge this low still takes a jump.
	case EControlHint::Jump:      return LOCTEXT("Jump", "Jump up onto ledges");
	case EControlHint::Sprint:    return bToggle ? LOCTEXT("SprintToggle", "Press to sprint") : LOCTEXT("SprintHold", "Hold to sprint");
	case EControlHint::Slide:     return LOCTEXT("Slide", "Crouch while sprinting to slide");
	case EControlHint::Swap:      return LOCTEXT("Swap", "Swap guns");
	case EControlHint::Inventory: return LOCTEXT("Inventory", "Inventory: compare and equip");
	case EControlHint::Bench:     return LOCTEXT("Bench", "Use the bench: two guns of a kind can trade parts");
	case EControlHint::Count:     break;
	}
	return FText::GetEmpty();
}

FName FControlHintRules::Id(EControlHint Hint)
{
	switch (Hint)
	{
	case EControlHint::Move:      return TEXT("Move");
	case EControlHint::Reload:    return TEXT("Reload");
	case EControlHint::Melee:     return TEXT("Melee");
	case EControlHint::Grenade:   return TEXT("Grenade");
	case EControlHint::Aim:       return TEXT("Aim");
	case EControlHint::Jump:      return TEXT("Jump");
	case EControlHint::Sprint:    return TEXT("Sprint");
	case EControlHint::Slide:     return TEXT("Slide");
	case EControlHint::Swap:      return TEXT("Swap");
	case EControlHint::Inventory: return TEXT("Inventory");
	case EControlHint::Bench:     return TEXT("Bench");
	case EControlHint::Count:     break;
	}
	return NAME_None;
}

EControlHint FControlHintRules::FromId(FName InId)
{
	for (int32 Index = 0; Index < HintCount; ++Index)
	{
		const EControlHint Hint = static_cast<EControlHint>(Index);
		if (!InId.IsNone() && Id(Hint) == InId)
		{
			return Hint;
		}
	}
	return EControlHint::Count;
}

bool FControlHintRules::IsMomentary(EControlHint Hint)
{
	return Hint == EControlHint::Reload || Hint == EControlHint::Melee || Hint == EControlHint::Grenade || Hint == EControlHint::Aim
		|| Hint == EControlHint::Jump || Hint == EControlHint::Bench;
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
	if (In.bJumped)
	{
		Learn(EControlHint::Jump);
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
	if (In.bSwapped)
	{
		Learn(EControlHint::Swap);
	}
	if (In.bInventoryOpen)
	{
		Learn(EControlHint::Inventory);
		bInventoryDue = false;
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
	if (In.bPickedUpGun && !GetMemory(EControlHint::Inventory).bLearned)
	{
		bInventoryDue = true;
	}
}

bool FControlHintRules::Wants(EControlHint Hint, const FControlHintInput& In) const
{
	switch (Hint)
	{
	case EControlHint::Move:      return IdleFor >= MoveIdleSeconds;
	case EControlHint::Reload:    return In.bGunInHand && In.bMagazineLow && !In.bReloading;
	// Fists work too, so no gun needed. The sense only reports a close creature to a body that hasn't struck yet.
	case EControlHint::Melee:     return In.bCloseTarget;
	// The sense only reports a crowd to a body with a grenade in hand that hasn't thrown one yet.
	case EControlHint::Grenade:   return In.bCrowdAhead;
	case EControlHint::Aim:      return In.bGunInHand && In.bFarTarget && !In.bAiming;
	case EControlHint::Jump:      return In.bLedgeAhead;
	case EControlHint::Sprint:    return WalkFor >= WalkSeconds;
	case EControlHint::Slide:     return SprintFor >= SlideSprintSeconds;
	case EControlHint::Swap:      return In.GunsEquipped >= 2;
	case EControlHint::Inventory: return bInventoryDue;
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
}

void FControlHintRules::End(EControlHintEnd How)
{
	if (!IsShowing())
	{
		return;
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
	bInventoryDue = false;
	bChanged = true;
}

bool FControlHintRules::ConsumeChanged()
{
	const bool bWas = bChanged;
	bChanged = false;
	return bWas;
}

#undef LOCTEXT_NAMESPACE
