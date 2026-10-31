// FControlHintRules: the hints' table, each one's key, words, saved name and how often it may show. When it comes and goes
// is ControlHintRules.cpp's.

#include "Tutorial/ControlHintRules.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "InputCoreTypes.h"

// The same namespace as the words always had, so a translation keeps finding them.
#define LOCTEXT_NAMESPACE "ControlHints"

namespace
{
	/** The number key that turns the inventory to Page ("3"), from the tabs' own table. */
	FText PageKeyText(EInventoryPage Page)
	{
		return LoadoutParts::PageNumberKey(static_cast<int32>(Page)).GetDisplayName(/*bLongDisplayName*/ false);
	}
}

int32 FControlHintRules::MaxShows(EControlHint Hint)
{
	switch (Hint)
	{
	case EControlHint::Jump:
	case EControlHint::Slide:
	// The flask's: the first time health drops low in a fight (the contract's "first time"); its key tab sits on the HUD
	// by the flask besides.
	case EControlHint::Flask:
		return 1;
	case EControlHint::Crouch:
	case EControlHint::Hold:
		return 2;
	default:
		return 3;
	}
}

FName FControlHintRules::Action(EControlHint Hint)
{
	switch (Hint)
	{
	case EControlHint::Move:      return TEXT("Move");
	// The soul flask's binding id (the contract's "Flask", in UKeyBindingSubsystem).
	case EControlHint::Flask:     return TEXT("Flask");
	case EControlHint::Reload:    return TEXT("Reload");
	case EControlHint::Fire:      return TEXT("Fire");
	// UKeyBindingSubsystem::MeleeBindingId(), spelled out here as the others are so the rules need no subsystem.
	case EControlHint::Melee:     return TEXT("Melee");
	// UKeyBindingSubsystem::GrenadeBindingId().
	case EControlHint::Grenade:   return TEXT("Grenade");
	case EControlHint::Aim:       return TEXT("Aim");
	case EControlHint::Crouch:    return TEXT("Crouch");
	case EControlHint::Jump:      return TEXT("Jump");
	case EControlHint::Interact:  return TEXT("Interact");
	case EControlHint::Hold:      return TEXT("Interact");
	case EControlHint::Sprint:    return TEXT("Sprint");
	case EControlHint::Slide:     return TEXT("Crouch");
	case EControlHint::Swap:      return TEXT("NextWeapon");
	case EControlHint::Inventory: return TEXT("Inventory");
	// The Ledger and the Missions page are the inventory's pages: its key opens them, the words say which.
	case EControlHint::Ledger:    return TEXT("Inventory");
	case EControlHint::Missions:  return TEXT("Inventory");
	// UKeyBindingSubsystem::MapBindingId().
	case EControlHint::Map:       return TEXT("Map");
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
	case EControlHint::Flask:     return LOCTEXT("Flask", "Soul flask");
	case EControlHint::Reload:    return LOCTEXT("Reload", "Reload");
	case EControlHint::Fire:      return LOCTEXT("Fire", "Fire");
	case EControlHint::Melee:     return LOCTEXT("Melee", "Melee: strike what gets too close");
	case EControlHint::Grenade:   return LOCTEXT("Grenade", "Grenade: salt the crowd");
	case EControlHint::Aim:       return LOCTEXT("Aim", "Aim down the sights for a long shot");
	// The game's own rule (UPlayerLocomotionComponent::CrouchSpreadMultiplier): a crouch tightens the spread.
	case EControlHint::Crouch:    return LOCTEXT("Crouch", "Crouch for a steadier aim");
	// Where the mantle is, a jump at a ledge climbs it; where it isn't, a ledge this low still takes a jump.
	case EControlHint::Jump:      return LOCTEXT("Jump", "Jump up onto ledges");
	case EControlHint::Interact:  return LOCTEXT("Interact", "Use it: read, open, pick up");
	case EControlHint::Hold:      return LOCTEXT("Hold", "Hold until the ring fills");
	case EControlHint::Sprint:    return bToggle ? LOCTEXT("SprintToggle", "Press to sprint") : LOCTEXT("SprintHold", "Hold to sprint");
	case EControlHint::Slide:     return LOCTEXT("Slide", "Crouch while sprinting to slide");
	case EControlHint::Swap:      return LOCTEXT("Swap", "Swap guns");
	case EControlHint::Inventory: return LOCTEXT("Inventory", "Inventory: compare and equip");
	// After the inventory's keycap, the page's number inside it from the tabs' own order, as the missions' words give it
	// (UMissionOpenPageObjective: "{Inventory}, then {Page.Bestiary}"), so neither drifts when the tabs move.
	case EControlHint::Ledger:
		return FText::Format(LOCTEXT("Ledger", "Then {0}: the Ledger"), PageKeyText(EInventoryPage::Bestiary));
	case EControlHint::Missions:
		return FText::Format(LOCTEXT("Missions", "Then {0}: your missions, to choose which to follow"), PageKeyText(EInventoryPage::Missions));
	case EControlHint::Map:       return LOCTEXT("Map", "Map: where you are, and where you're headed");
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
	case EControlHint::Flask:     return TEXT("Flask");
	case EControlHint::Reload:    return TEXT("Reload");
	case EControlHint::Fire:      return TEXT("Fire");
	case EControlHint::Melee:     return TEXT("Melee");
	case EControlHint::Grenade:   return TEXT("Grenade");
	case EControlHint::Aim:       return TEXT("Aim");
	case EControlHint::Crouch:    return TEXT("Crouch");
	case EControlHint::Jump:      return TEXT("Jump");
	case EControlHint::Interact:  return TEXT("Interact");
	case EControlHint::Hold:      return TEXT("Hold");
	case EControlHint::Sprint:    return TEXT("Sprint");
	case EControlHint::Slide:     return TEXT("Slide");
	case EControlHint::Swap:      return TEXT("Swap");
	case EControlHint::Inventory: return TEXT("Inventory");
	case EControlHint::Ledger:    return TEXT("Ledger");
	case EControlHint::Missions:  return TEXT("Missions");
	case EControlHint::Map:       return TEXT("Map");
	case EControlHint::Bench:     return TEXT("Bench");
	case EControlHint::Count:     break;
	}
	return NAME_None;
}

EControlHint FControlHintRules::FromId(FName InId)
{
	for (int32 Index = 0; Index < static_cast<int32>(EControlHint::Count); ++Index)
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
	switch (Hint)
	{
	case EControlHint::Flask:
	case EControlHint::Reload:
	case EControlHint::Fire:
	case EControlHint::Melee:
	case EControlHint::Grenade:
	case EControlHint::Aim:
	case EControlHint::Crouch:
	case EControlHint::Jump:
	case EControlHint::Interact:
	case EControlHint::Hold:
	case EControlHint::Bench:
		return true;
	default:
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
