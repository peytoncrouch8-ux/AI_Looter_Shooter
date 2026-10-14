#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * A flame's light, as plain rules (ULightFlickerSubsystem plays them on the lights near the player; the tests run them
 * without a world). A paraffin lamp or a lantern never burns quite still: its light breathes a few percent either way
 * (three layers of smooth noise, slow to quick, so it never pulses on a beat), and now and then the flame gutters, a
 * quick dip and back. A house's lamp is also carried about the house now and then: the lamp, and the windows it shows
 * through, dim slowly as it goes and brighten as it comes back.
 */
namespace LightFlickerRules
{
	/** Lights within this of the view flicker (cm); past it they rest at their own intensity and cost nothing. */
	inline constexpr float Radius = 4000.f;

	/** How often the lights coming near or going far are looked for (s). */
	inline constexpr float LookSeconds = 0.5f;

	/** How often a near light is written (s): a flame needs no more than 30 a second. */
	inline constexpr float StepSeconds = 1.f / 30.f;

	/** The breath: at most this share of the light's own intensity either way. */
	inline constexpr float Breath = 0.08f;

	/** A gutter: how deep its dip goes (a share), how long it lasts (s), and the wait between two (s). */
	inline constexpr float GutterDepth = 0.18f;
	inline constexpr float GutterSeconds = 0.45f;
	inline constexpr float GutterGapMin = 14.f;
	inline constexpr float GutterGapMax = 45.f;

	/**
	 * A lamp carried off and back (someone moving about the house): the wait between two (s), how low the light goes
	 * (a share), how long it eases down, stays down and eases back (s).
	 */
	inline constexpr float CarryGapMin = 45.f;
	inline constexpr float CarryGapMax = 120.f;
	inline constexpr float CarryLowMin = 0.55f;
	inline constexpr float CarryLowMax = 0.75f;
	inline constexpr float CarryOutSeconds = 1.8f;
	inline constexpr float CarryHoldMin = 3.f;
	inline constexpr float CarryHoldMax = 8.f;
	inline constexpr float CarryBackSeconds = 2.4f;

	/** A lamp's glass (a house's windows) follows its flame's flicker this much: the windows are the lamp seen through curtains. */
	inline constexpr float GlassShare = 0.85f;

	/** The breath at Seconds for a flame of its own Seed: smooth, within [-1, 1]. */
	AI_LOOTER_SHOOTER_API float BreathAt(float Seconds, float Seed);

	/** A gutter's dip Since seconds into it: 0 before and after, 1 at its deepest. */
	AI_LOOTER_SHOOTER_API float GutterAt(float Since);

	/** A carry's level Since seconds into it, going down to Low and staying Hold seconds: 1 before and after. */
	AI_LOOTER_SHOOTER_API float CarryAt(float Since, float Low, float Hold);

	/** How long a carry that stays down Hold seconds lasts, all told (s). */
	AI_LOOTER_SHOOTER_API float CarrySeconds(float Hold);
}

/** One flame's flicker: its own clock, its next gutter and (a carried lamp's) its next carry. */
struct AI_LOOTER_SHOOTER_API FFlameFlicker
{
	/** A fresh flame of its own (seed, clock, first gutter and carry) from Random; bInCarried: a house lamp, carried now and then. */
	void Start(FRandomStream& Random, bool bInCarried);

	/** Moves on by DeltaSeconds: the next gutter or carry comes when its wait is over. */
	void Advance(float DeltaSeconds, FRandomStream& Random);

	/** The breath and the gutter: the light's share of its own intensity from the flame alone. */
	float GetFlame() const;

	/** The carry: 1, or less while the lamp is carried off. */
	float GetCarry() const;

	/** All of it: the light's share of its own intensity now. */
	float GetFactor() const { return GetFlame() * GetCarry(); }

	/** The glass's share of its own glow now: the flame's flicker a little softened (GlassShare), and the carry. */
	float GetGlassFactor() const;

	bool IsGuttering() const;
	bool IsCarried() const;

	float Seed = 0.f;
	float Clock = 0.f;
	bool bCarries = false;
	float NextGutter = 0.f;
	/** When the gutter under way began (on Clock), or below 0 for none. */
	float GutterStart = -1.f;
	float NextCarry = 0.f;
	float CarryStart = -1.f;
	float CarryLow = 1.f;
	float CarryHold = 0.f;
};
