#pragma once

#include "CoreMinimal.h"

/**
 * The ward's sounds, by cue (Art/Sounds/recipes/shields.py; all script-made). They live here until Main moves them into
 * Audio/LooterSoundCues.h for the bank's check (LooterSoundCue::Shield).
 */
namespace ShieldCue
{
	/** A hit soaked (2D, the player's own, ~0.25 s): a glassy crackle of the ward taking it, a soft low thud under it. */
	inline constexpr const TCHAR* Hit = TEXT("Shield.Hit");
	/** The ward breaking (2D, ~0.9 s): a pane of light shattering, the coal's hum dropping out, a hollow rush after. */
	inline constexpr const TCHAR* Break = TEXT("Shield.Break");
	/** It starts to refill (2D, ~0.6 s): the coal catching again, a rising charge whine and a click of the box's catch. */
	inline constexpr const TCHAR* RechargeStart = TEXT("Shield.RechargeStart");
	/** Full again (2D, ~0.5 s): a clean bright chime and a settling hum. */
	inline constexpr const TCHAR* Full = TEXT("Shield.Full");
	/** Last Breath's burst (3D at the player, ~1.2 s): a deep breath of air thrown out, the element's colour of crackle on it. */
	inline constexpr const TCHAR* Nova = TEXT("Shield.Nova");
	/** Saint's Grace answering (2D, ~0.9 s): a warm choir swell and the ward's chime. */
	inline constexpr const TCHAR* Grace = TEXT("Shield.Grace");
	/** A ward picked up or worn (2D): the box's catch and its strap. */
	inline constexpr const TCHAR* Pickup = TEXT("Shield.Pickup");
}
