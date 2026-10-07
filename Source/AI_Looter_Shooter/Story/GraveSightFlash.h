#pragma once

#include "CoreMinimal.h"

/**
 * A Grave Sight flash's rules, apart from the world (Docs/Story.md: "The cyan lines are Grave Sight: Ellis sees souls, weak
 * points and the Unpaid"; Docs/Areas/RansomsRest.md, Main 4: "a two-second Grave Sight flash of the ember lifting off the
 * lid"). Over its seconds it says how strongly the screen's cyan overlay shows (in quickly as a flash comes, held, out a
 * little slower so the sight lingers) and where an ember seen through it has got to (lifting off what held it, flaring,
 * then fading as it goes). The vision mode itself, a post-process, stays a later, measured option (Docs/Story.md): the
 * flash needs no rendering pass of its own. UGraveSightSubsystem plays one on the screen; AChapelReliquary plays its ember.
 */
struct AI_LOOTER_SHOOTER_API FGraveSightFlash
{
	/** A flash's length (s): the Reliquary's two seconds. */
	static constexpr float DefaultSeconds = 2.f;

	/** The shortest flash there is (s). */
	static constexpr float MinSeconds = 0.1f;

	/** Starts it over from its beginning, Seconds long (at least MinSeconds). */
	void Start(float InSeconds = DefaultSeconds);

	/** Moves it on by DeltaSeconds. True on the step it ends, once. */
	bool Advance(float DeltaSeconds);

	/** Ends it at once (its level going, a test). */
	void Stop();

	bool IsPlaying() const { return bPlaying; }

	/** Seconds into it, and its length. */
	float GetElapsed() const { return Elapsed; }
	float GetSeconds() const { return Seconds; }

	/** The share of it gone, 0 to 1 (1 once it has ended). */
	float GetProgress() const;

	/** The overlay's strength now, 0 to 1 (0 while none plays). */
	float GetOverlayAlpha() const { return bPlaying ? OverlayAlphaAt(GetProgress()) : 0.f; }

	/** How far the ember has risen now, 0 to 1 of its rise. */
	float GetEmberRise() const { return EmberRiseAt(GetProgress()); }

	/** How brightly the ember burns now, 0 to 1 (0 while none plays). */
	float GetEmberGlow() const { return bPlaying ? EmberGlowAt(GetProgress()) : 0.f; }

	/** The overlay at a share of the flash: in over its first 15%, held to 60%, out by its end. */
	static float OverlayAlphaAt(float Progress);

	/** The ember's rise at a share of the flash: quick off the lid, slowing as it goes. */
	static float EmberRiseAt(float Progress);

	/** The ember's glow at a share of the flash: flaring up over its first tenth, fading out over its second half. */
	static float EmberGlowAt(float Progress);

private:
	float Seconds = DefaultSeconds;
	float Elapsed = 0.f;
	bool bPlaying = false;
};
