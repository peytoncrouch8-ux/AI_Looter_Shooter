#pragma once

#include "CoreMinimal.h"

/** The volume sliders in Settings > Audio, each the slider's share from 0 (silent) to 1 (as the sounds were made). */
struct FLooterVolumes
{
	float Master = 1.f;
	float Effects = 1.f;
	float Interface = 1.f;
	float Music = 1.f;
};

/**
 * The gain each sound class gets from the sliders (/Game/Audio/Mix: SC_Master over SC_Effects, SC_Interface, SC_Ambience
 * and SC_Music). Master's gain is set on SC_Master, which hands it down to the four under it.
 */
struct FLooterClassGains
{
	float Master = 1.f;
	float Effects = 1.f;
	float Interface = 1.f;
	float Ambience = 1.f;
	float Music = 1.f;
};

/** The sound system's rules as plain functions, apart from the engine's audio so the tests can run them. */
namespace LooterSoundRules
{
	/**
	 * Which of a cue's Count sounds plays, from Roll (0 to 1): any of them the first time (Last is INDEX_NONE), and never
	 * Last again when there is another, so the same sound never plays twice in a row. INDEX_NONE when Count is 0.
	 */
	AI_LOOTER_SHOOTER_API int32 PickVariation(int32 Count, int32 Last, float Roll);

	/** Decibels as a gain: 0 dB is 1, -6 dB about a half. */
	AI_LOOTER_SHOOTER_API float DbToGain(float Db);

	/** PitchScale with the cue's jitter: Roll 0 is Jitter lower, 1 is Jitter higher (0.04: within 4% either way). */
	AI_LOOTER_SHOOTER_API float JitteredPitch(float PitchScale, float Jitter, float Roll);

	/**
	 * A slider's share as a gain. Squared, because hearing is logarithmic: half way sounds about half as loud, where a
	 * straight line would leave the bottom half of the slider nearly as loud as the top.
	 */
	AI_LOOTER_SHOOTER_API float SliderToGain(float Share);

	/** The gain of each sound class for the sliders. Ambience (wind, water, the world's own sounds) follows Effects. */
	AI_LOOTER_SHOOTER_API FLooterClassGains ClassGains(const FLooterVolumes& Volumes);

	/**
	 * A creature's voice for its size: a bigger body sounds lower (the Gravemother is the spider at 1.8x, Abel the Unpaid at
	 * 1.3x), a smaller one higher (a spiderling at 0.45x). One over the size's square root, kept within 0.6-1.6.
	 */
	AI_LOOTER_SHOOTER_API float PitchForSize(float SizeScale);

	/**
	 * Seconds within which a cue doesn't start again: a shotgun's pellets landing together are heard as one hit, and a
	 * pack's alerts as separate cries rather than one loud, phasing one.
	 */
	inline constexpr float RetriggerSeconds = 0.03f;

	/**
	 * A critical hit's hit marker: the same tick a little higher (the user's call, 2026-10-08: crits need no sound of
	 * their own), so it reads as a better hit without a second sound in every firefight.
	 */
	inline constexpr float CritPitch = 1.12f;
}
