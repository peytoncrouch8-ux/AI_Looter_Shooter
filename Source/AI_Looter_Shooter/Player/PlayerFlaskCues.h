#pragma once

#include "CoreMinimal.h"

/**
 * The soul flask's sound cues (Art/Sounds/recipes/flask.py makes them; Tools/Unreal/build_sound_bank.py brings them in; with
 * none a cue plays nothing). All the player's own, 2D. FlaskFX plays them; nothing else needs to.
 */
namespace FlaskCue
{
	/** The press: the cork twisted in the glass neck with a squeak, then the pop (the flask's air ringing) and its tock. */
	inline constexpr const TCHAR* Uncork = TEXT("Player.Flask.Uncork");
	/** The sip: the glass at the lips, the flask glugging, two swallows, a breath out. */
	inline constexpr const TCHAR* Drink = TEXT("Player.Flask.Drink");
	/** With the sip: the soul taken in, a warm major chord of glass rising and blooming (about the heal's length). */
	inline constexpr const TCHAR* Heal = TEXT("Player.Flask.Heal");
	/** A charge filled by soul: a clear little glass chime and its fifth. */
	inline constexpr const TCHAR* ChargeFilled = TEXT("Player.Flask.ChargeFilled");
	/** Refused, no charges: a dry glass clink, the empty flask knocked twice on its tin cup. */
	inline constexpr const TCHAR* Empty = TEXT("Player.Flask.Empty");
	/** Refused, health full: a soft muted tap on the cork and the full flask's dull slosh. */
	inline constexpr const TCHAR* Full = TEXT("Player.Flask.Full");
	/** The respawn's top-up: three soft glass notes rising, quiet. */
	inline constexpr const TCHAR* Refilled = TEXT("Player.Flask.Refilled");
}
