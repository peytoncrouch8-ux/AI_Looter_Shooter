#pragma once

#include "CoreMinimal.h"

/**
 * Swimming's plain rules (round 4, the user's pick 2026-10-27: "swim in deep water"), apart from the world so the tests read
 * them (Looter.Swim.Rules, .Shore). Lengths are the full-size body's (Player/PlayerSize.h) and take the body's scale
 * (BodyScale; 0.85 for the player), cm and seconds. Every number is a cvar (Looter.Swim.*) read here.
 *
 *  - Deep enough water swims: more than chest deep over the feet (StartDepth); shallower stays wading. Out of the swim
 *    again where the bed under the feet lies less than StandDepth under the surface (a little shallower than the start, so
 *    the edge of a pool never flickers between the two).
 *  - Floating, the eye rides FloatEye over the surface, the body eased there on a spring (FloatSpeed); no key held under
 *    water, the body drifts gently up (buoyancy), never shooting out.
 *  - Breath (cheap and fair, the contract's "only if fair"): a full breath lasts BreathSeconds with the head under and comes
 *    back over RefillSeconds in the air. Run out and nothing hurts: the body is carried up to the surface and can't dive
 *    again until it has breathed (OutOfBreath). Surfacing short of breath gasps (GaspOnSurfacing).
 *  - Diving: the crouch key sinks, the jump key rises, and swimming forward with the look pitched past the dead zone goes
 *    the look's way (DiveShare).
 */
namespace SwimRules
{
	/** Water this deep over the feet (cm) swims; less wades. */
	AI_LOOTER_SHOOTER_API float StartDepth(float BodyScale);
	/** A swimmer stands again where the bed lies less than this under the surface (cm): StartDepth less the margin. */
	AI_LOOTER_SHOOTER_API float StandDepth(float BodyScale);
	AI_LOOTER_SHOOTER_API bool ShouldSwim(float WaterOverFeet, float BodyScale);
	AI_LOOTER_SHOOTER_API bool CanStand(float WaterOverBed, float BodyScale);

	/** How far down a swimmer feels for the bed under the feet (cm). */
	AI_LOOTER_SHOOTER_API float BedReach(float BodyScale);

	/** Where the feet float: the eye FloatEye over the surface (EyeAboveFeet: the eye's height over the feet, world cm). */
	AI_LOOTER_SHOOTER_API float FloatFeetZ(float SurfaceZ, float EyeAboveFeet, float BodyScale);
	/** The up speed (cm/s; down negative) the float spring asks for, the feet at FeetZ, heading for TargetFeetZ. */
	AI_LOOTER_SHOOTER_API float FloatSpeed(float FeetZ, float TargetFeetZ, float BodyScale);

	/** Swimming speed (cm/s) for a walk speed, faster with the sprint key held. */
	AI_LOOTER_SHOOTER_API float SwimSpeed(float WalkSpeed, bool bFast);
	/** Up or down at full key (cm/s). */
	AI_LOOTER_SHOOTER_API float VerticalSpeed(float BodyScale);
	/** Slowing with no key held (cm/s^2): the water's drag. */
	AI_LOOTER_SHOOTER_API float Braking();
	/** Share of a fall's speed kept on hitting the water (the water takes the rest). */
	AI_LOOTER_SHOOTER_API float EntryKeep();
	/** The jump key at the surface: up this fast (cm/s), to reach a bank's lip (a ledge in front is climbed). */
	AI_LOOTER_SHOOTER_API float JumpOutSpeed(float BodyScale);

	/**
	 * Swimming forward with the look at PitchDegrees (up positive): the share of the stroke that goes up (positive) or down,
	 * 0 inside the dead zone (a glance down never dives), full past DiveFull.
	 */
	AI_LOOTER_SHOOTER_API float DiveShare(float PitchDegrees);

	/** Strokes a second at a speed share (0 treading .. 1 full swim). */
	AI_LOOTER_SHOOTER_API float StrokeRate(float SpeedShare);
	/** Where in a stroke (0..1) its push comes (the hands sweeping out): its sound and bubbles. */
	inline constexpr float StrokePushAt = 0.3f;

	/** The breath after Dt seconds with the head under or in the air (1 full, 0 out). */
	AI_LOOTER_SHOOTER_API float BreathAfter(float Breath, float Dt, bool bHeadUnder);
	AI_LOOTER_SHOOTER_API bool OutOfBreath(float Breath);
	/** Out of breath, the dive comes back once the breath is up to this again. */
	AI_LOOTER_SHOOTER_API float BreathBack();
	AI_LOOTER_SHOOTER_API bool GaspOnSurfacing(float Breath);
	/** The head counts as under when the eye is this far under the surface (cm): a bob at the surface isn't a dive. */
	inline constexpr float HeadUnderMargin = 3.f;
}

/** Plain polygon tests on world XY (cm) for the waters' shores and deep zones (ASwimWater). */
namespace SwimShore
{
	/** Inside the polygon (even-odd); fewer than three corners holds nothing. */
	AI_LOOTER_SHOOTER_API bool Contains(const TArray<FVector2D>& Polygon, const FVector2D& Point);
	/** The polygon's box (empty for none). */
	AI_LOOTER_SHOOTER_API FBox2D BoundsOf(const TArray<FVector2D>& Polygon);
	/** An ellipse's outline: Count corners round Center with radii Radii (the build script makes ponds' shores this way). */
	AI_LOOTER_SHOOTER_API TArray<FVector2D> Ellipse(const FVector2D& Center, const FVector2D& Radii, int32 Count);
}
