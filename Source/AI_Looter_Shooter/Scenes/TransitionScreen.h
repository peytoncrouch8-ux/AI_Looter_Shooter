#pragma once

#include "CoreMinimal.h"

/** Where the transition screen is. */
enum class ETransitionPhase : uint8
{
	/** Nothing on screen. */
	Clear,
	/** A scene sets the white frame by frame (the skiff ride's last seconds). */
	Driven,
	/** Coming up to full white by itself. */
	Fading,
	/** Full white, waiting through a level load for the arrival to reveal it. */
	Held,
	/** The title rising through the white, then the white thinning. */
	Revealing,
};

/**
 * The transition screen's rules, apart from the screen (UTransitionScreenSubsystem draws them; tests drive them): how
 * white it is, the title's rise and fade, and the timeout that keeps a white from ever staying for good.
 *
 * A reveal with a title: the white alone for a beat, the title rising into place through it with its accent line drawing
 * out, a moment to read it, then the white thinning while the title fades and drifts up. Without one, a short beat and the
 * white thins. A hold, or a scene's white left alone, thins by itself after HoldTimeout seconds. Steps are counted at most
 * LongestStep each, so a level load's one long frame counts as one ordinary frame.
 */
class AI_LOOTER_SHOOTER_API FTransitionScreen
{
public:
	/** A held white nothing reveals thins by itself after this long (seconds of play, not of loading). */
	static constexpr float HoldTimeout = 20.f;
	static constexpr float LongestStep = 0.1f;

	/** With a title: the white alone, the title rising, held to be read, the white thinning, the title fading out. */
	static constexpr float TitleDelay = 0.5f;
	static constexpr float TitleRiseSeconds = 1.6f;
	static constexpr float TitleHoldSeconds = 1.4f;
	static constexpr float ThinSeconds = 2.5f;
	/** The title starts to fade this long after the white starts thinning, and is gone this long after that. */
	static constexpr float TitleFadeDelay = 1.2f;
	static constexpr float TitleFadeSeconds = 1.6f;

	/** Without a title: a short beat, then the white thins. */
	static constexpr float PlainDelay = 0.25f;
	static constexpr float PlainThinSeconds = 2.f;

	/** A scene's white, this white (0-1) now. Not held; nothing at 0. */
	void Drive(float InWhite);

	/** Up to full white over Seconds from where it is, then held. */
	void FadeIn(float Seconds);

	/** Full white at once, held. */
	void Hold();

	/** Thins whatever white there is, the title rising through it first (bWithTitle). A title shows even with no white. */
	void Reveal(bool bWithTitle);

	/** Nothing on screen, at once. */
	void Clear();

	/** Moves on by DeltaSeconds. True when a white nobody revealed just timed out (it's thinning now). */
	bool Advance(float DeltaSeconds);

	ETransitionPhase GetPhase() const { return Phase; }
	bool IsHeld() const { return Phase == ETransitionPhase::Held; }

	/** Something is on screen: some white, or a title. */
	bool IsShowing() const;

	/** How white the screen is, 0-1. */
	float GetWhite() const;

	/** The title's opacity, 0-1. */
	float GetTitleOpacity() const;

	/** Where the title is on its rise: 1 below its place, 0 in it, a little under 0 as it drifts up fading out. */
	float GetTitleRise() const;

	/** How much of the accent line under the title has drawn out, 0-1. */
	float GetTitleLine() const;

	/** Seconds from Reveal until the screen is clear. */
	static float RevealSeconds(bool bWithTitle);

private:
	/** Seconds into the reveal when the white starts thinning, and how long it takes. */
	float ThinStart() const;
	float ThinDuration() const;

	ETransitionPhase Phase = ETransitionPhase::Clear;
	/** Driven, Fading and Held: the white; Revealing: the white it thins from. */
	float White = 0.f;
	/** Seconds into the fade or the reveal. */
	float Clock = 0.f;
	float FadeFrom = 0.f;
	float FadeSeconds = 0.f;
	/** Seconds held or driven with nobody touching it. */
	float Unattended = 0.f;
	bool bTitle = false;
};
