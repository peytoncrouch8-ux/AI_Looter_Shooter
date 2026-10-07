#pragma once

#include "CoreMinimal.h"

/**
 * What the player is asking for with the sprint and crouch keys, independent of whether the character can
 * actually do it right now. Each key works in hold mode (active while held) or toggle mode (press on, press off).
 *
 * When both are asked for, the most recent press wins: holding crouch then pressing sprint stands up and sprints,
 * and letting go of sprint drops back into the crouch that is still held. Pressing one also clears the other's
 * toggle, so a toggled crouch doesn't come back after a sprint. Jump while crouched stands up (StandUp), and a slide
 * ending with forward held goes back into the sprint (ResumeSprint). Pure logic, so the rules are unit tested.
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
		if (Stance == EStance::Sprint)
		{
			// The key itself takes over a sprint kept on without it (ResumeSprint).
			bSprintLatched = false;
		}
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
		// A sprint kept on without its key has no key to let go of (the safety net for missed releases asks anyway).
		if (bToggle || (Stance == EStance::Sprint && bSprintLatched))
		{
			return;
		}
		(Stance == EStance::Sprint ? bSprintActive : bCrouchActive) = false;
		if (Latest == Stance)
		{
			Latest = Other(Stance);
		}
	}

	/**
	 * Ends a toggled sprint, or one kept on out of a slide (the player stopped moving, shot or aimed). A held key keeps
	 * its intent.
	 */
	void CancelSprintToggle()
	{
		if (bSprintToggle || bSprintLatched)
		{
			bSprintActive = false;
			bSprintLatched = false;
		}
	}

	/**
	 * A slide ending with forward held (the user's rule): back into the sprint as if its key were down, and out of any
	 * crouch, a toggled one too. A sprint key still held keeps working as a held key; one already let go (hold mode) is
	 * kept on without it, like a toggled sprint, until the player stops, shoots or aims.
	 */
	void ResumeSprint()
	{
		bSprintLatched = !bSprintToggle && !bSprintActive;
		bSprintActive = true;
		bCrouchActive = false;
		Latest = EStance::Sprint;
	}

	/**
	 * The jump key while crouched: stand up, in either mode. A held crouch key counts as let go until it is pressed
	 * again, and a held sprint key takes over.
	 */
	void StandUp()
	{
		bCrouchActive = false;
		if (Latest == EStance::Crouch)
		{
			Latest = Other(EStance::Crouch);
		}
	}

	bool WantsSprint() const { return bSprintActive && (Latest == EStance::Sprint || !bCrouchActive); }
	bool WantsCrouch() const { return bCrouchActive && (Latest == EStance::Crouch || !bSprintActive); }

	bool IsHeldMode(EStance Stance) const { return Stance == EStance::Sprint ? !bSprintToggle : !bCrouchToggle; }
	bool IsActive(EStance Stance) const { return Stance == EStance::Sprint ? bSprintActive : bCrouchActive; }

	void Reset()
	{
		bSprintActive = false;
		bSprintLatched = false;
		bCrouchActive = false;
		Latest = EStance::None;
	}

private:
	static EStance Other(EStance Stance) { return Stance == EStance::Sprint ? EStance::Crouch : EStance::Sprint; }

	bool bSprintToggle = false;
	bool bCrouchToggle = false;
	bool bSprintActive = false;
	/** The sprint is on without its key (hold mode, out of a slide): it behaves as a toggled one until it ends. */
	bool bSprintLatched = false;
	bool bCrouchActive = false;
	EStance Latest = EStance::None;
};
