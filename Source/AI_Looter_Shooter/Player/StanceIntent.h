#pragma once

#include "CoreMinimal.h"

/**
 * What the player is asking for with the sprint and crouch keys, independent of whether the character can
 * actually do it right now. Each key works in hold mode (active while held) or toggle mode (press on, press off).
 *
 * When both are asked for, the most recent press wins: holding crouch then pressing sprint stands up and sprints,
 * and letting go of sprint drops back into the crouch that is still held. Pressing one also clears the other's
 * toggle, so a toggled crouch doesn't come back after a sprint. Pure logic, so the rules are unit tested.
 */
struct FStanceIntent
{
	enum class EStance : uint8 { None, Sprint, Crouch };

	void SetModes(bool bInSprintToggle, bool bInCrouchToggle)
	{
		bSprintToggle = bInSprintToggle;
		bCrouchToggle = bInCrouchToggle;
	}

	void Press(EStance Stance)
	{
		bool& bActive = Stance == EStance::Sprint ? bSprintActive : bCrouchActive;
		const bool bToggle = Stance == EStance::Sprint ? bSprintToggle : bCrouchToggle;
		bActive = bToggle ? !bActive : true;
		if (!bActive)
		{
			Latest = Other(Stance);
			return;
		}
		Latest = Stance;

		// A latched toggle on the other stance would otherwise come back as soon as this one ends.
		const bool bOtherToggle = Stance == EStance::Sprint ? bCrouchToggle : bSprintToggle;
		if (bOtherToggle)
		{
			(Stance == EStance::Sprint ? bCrouchActive : bSprintActive) = false;
		}
	}

	void Release(EStance Stance)
	{
		const bool bToggle = Stance == EStance::Sprint ? bSprintToggle : bCrouchToggle;
		if (bToggle)
		{
			return;
		}
		(Stance == EStance::Sprint ? bSprintActive : bCrouchActive) = false;
		if (Latest == Stance)
		{
			Latest = Other(Stance);
		}
	}

	/** Ends a toggled sprint (the player stopped moving or started shooting). A held key keeps its intent. */
	void CancelSprintToggle()
	{
		if (bSprintToggle)
		{
			bSprintActive = false;
		}
	}

	bool WantsSprint() const { return bSprintActive && (Latest == EStance::Sprint || !bCrouchActive); }
	bool WantsCrouch() const { return bCrouchActive && (Latest == EStance::Crouch || !bSprintActive); }

	bool IsHeldMode(EStance Stance) const { return Stance == EStance::Sprint ? !bSprintToggle : !bCrouchToggle; }
	bool IsActive(EStance Stance) const { return Stance == EStance::Sprint ? bSprintActive : bCrouchActive; }

	void Reset()
	{
		bSprintActive = false;
		bCrouchActive = false;
		Latest = EStance::None;
	}

private:
	static EStance Other(EStance Stance) { return Stance == EStance::Sprint ? EStance::Crouch : EStance::Sprint; }

	bool bSprintToggle = false;
	bool bCrouchToggle = false;
	bool bSprintActive = false;
	bool bCrouchActive = false;
	EStance Latest = EStance::None;
};
