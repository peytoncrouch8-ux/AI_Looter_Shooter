#pragma once

#include "CoreMinimal.h"

enum class ELightingSwitch : uint8;

/** Which area's sound world a level is: its beds, its sweeteners and its exploration theme. */
enum class ELooterAudioArea : uint8
{
	/** A level with no sound world of its own (a test map): it borrows Skyreach's. */
	Unknown,
	/** The tutorial and practice island (Lvl_TutorialIsland, and the old Lvl_Skyreach). */
	Skyreach,
	/** The campaign's first area and hub. */
	RansomsRest,
};

/** An area's bed in one light: two loops of different lengths played together, so the pair never audibly repeats. */
struct FAmbienceBed
{
	/** The air: wind, grass, the canyon (19 s). */
	FName Air;
	/** The life: insects, birds far off (13 s). */
	FName Life;

	bool IsSet() const { return !Air.IsNone() || !Life.IsNone(); }
	bool operator==(const FAmbienceBed& Other) const { return Air == Other.Air && Life == Other.Life; }
};

/**
 * One kind of sweetener: a single call (a bird, an insect, far thunder) the ambience places round the listener now and
 * then. Distances and heights are cm from the listener; gaps and cooldowns are seconds.
 */
struct FAmbienceSweetener
{
	FName Cue;
	/** How often it's picked against the others that are ready. */
	float Weight = 1.f;
	/** After it plays, the next sweetener of any kind waits this long (a random time between the two). */
	float MinGap = 5.f;
	float MaxGap = 12.f;
	/** It doesn't play again for this long (a hawk every few seconds would give the loop away). */
	float Cooldown = 0.f;
	/** Where it is: this far away on the ground, this high up. */
	float MinDistance = 600.f;
	float MaxDistance = 1600.f;
	float MinHeight = 0.f;
	float MaxHeight = 600.f;
	/** Heard flat from everywhere (far thunder rolls in from all round), not placed. */
	bool bFlat = false;
};

/** The ambience's rules as plain functions, apart from the engine's audio so the tests can run them. */
namespace AmbienceRules
{
	/** The area a map plays (its name, with or without a play-in-editor prefix or a path). */
	AI_LOOTER_SHOOTER_API ELooterAudioArea AreaForMap(const FString& MapName);

	/** Whether a lighting state is an evening one (Dusk): crickets and owls instead of cicadas and hawks. */
	AI_LOOTER_SHOOTER_API bool IsDusk(FName LightingState);

	/** The area's bed in a light. An area with no bed of its own for the light keeps its day bed. */
	AI_LOOTER_SHOOTER_API FAmbienceBed BedFor(ELooterAudioArea Area, FName LightingState);

	/** The sweeteners the area has in a light (empty for none). */
	AI_LOOTER_SHOOTER_API const TArray<FAmbienceSweetener>& SweetenersFor(ELooterAudioArea Area, FName LightingState);

	/**
	 * Which sweetener plays next: one of those whose cooldown has passed (ReadyAt[i] <= Now), by weight from Roll (0 to 1),
	 * never Last again while another is ready. INDEX_NONE when none is ready.
	 */
	AI_LOOTER_SHOOTER_API int32 PickSweetener(TArrayView<const FAmbienceSweetener> Options, int32 Last, TArrayView<const double> ReadyAt,
		double Now, float Roll);

	/** The wait after a sweetener (Roll 0 to 1 across its gap). */
	AI_LOOTER_SHOOTER_API float GapAfter(const FAmbienceSweetener& Sweetener, float Roll);

	/** Where a sweetener plays round the listener: a direction (YawRoll), a distance and a height across their ranges. */
	AI_LOOTER_SHOOTER_API FVector SpotAround(const FVector& Listener, const FAmbienceSweetener& Sweetener, float YawRoll, float DistanceRoll,
		float HeightRoll);

	/** The point of a line of points (a creek, a street; closed: a pond's shore) nearest Point. Point itself for no line. */
	AI_LOOTER_SHOOTER_API FVector NearestOnPath(TArrayView<const FVector> Path, bool bClosed, const FVector& Point);

	/** How long the bed crossfades when the light changes: under a fade it's done by the time the screen comes back. */
	AI_LOOTER_SHOOTER_API float BedCrossfadeSeconds(ELightingSwitch How);

	/** The first sweetener waits this long after the level begins (the bed comes up first). */
	inline constexpr float FirstSweetenerDelay = 5.f;

	/** The bed's fade in as a level begins (s). */
	inline constexpr float BedFadeInSeconds = 3.f;
}
