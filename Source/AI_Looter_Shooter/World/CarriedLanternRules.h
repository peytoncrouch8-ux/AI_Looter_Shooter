#pragma once

#include "CoreMinimal.h"

/**
 * Main 7, "Given to the Wind" (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 11; lane MAIN67's CONTRACT67.md): its names
 * and step numbers, in one place for the C++ that reads them (the carried lantern, the soul coal, the Reliquary's lantern,
 * SaintAdaWakes, Abel's corruption's SixSaved event). The mission's data (Tools/Unreal/create_mission_assets.py) counts the
 * same steps from 0; never reorder.
 */
namespace GivenToTheWind
{
	/** The mission's id, and its steps (AAbelKeeper keeps HangStep 1, FightStep 2, SceneStep 3). */
	AI_LOOTER_SHOOTER_API FName Mission();
	inline constexpr int32 PointStep = 0;
	inline constexpr int32 CarryStep = 4;
	inline constexpr int32 ReliquaryStep = 5;
	/** The step past the last: ready to turn in to Delia at the chapel door. */
	inline constexpr int32 ReadyStep = 6;

	/** Abel's fight won with none of the six drunk (AbelCorruption sends it as an event and sets a flag of the same name). */
	AI_LOOTER_SHOOTER_API FName SixSavedEvent();
	/** The Reliquary's lantern rest (AReliquaryLantern), the coal lumps (ASoulCoalLump), Delia at the chapel door. */
	AI_LOOTER_SHOOTER_API FName ReliquaryLanternTag();
	AI_LOOTER_SHOOTER_API FName SoulCoalTag();
	AI_LOOTER_SHOOTER_API FName DeliaChapelTag();
	/** The west road's Company men off the barge (build_area_main7.py's spawner id). */
	AI_LOOTER_SHOOTER_API FName WestRoadCrewId();
	/** Where the freed souls wait while Abel is fought: Gravewind Point's place at the Keeper's Gate. */
	AI_LOOTER_SHOOTER_API FName WaitPlaceTag();

	/**
	 * Saint Ada woke (SaintAdaWakes played or skipped, or its end state put in place): set for good. The Reliquary's lit
	 * state, Abel's soul by it, the coal gone, the followers gone, all read it; it outlives the mission's own steps.
	 */
	AI_LOOTER_SHOOTER_API FName SaintAdaAwakeFlag();
	/** A coal lump cracked and its soul freed ("SoulCoal.<Id>"): kept by the campaign, so a lump stays cracked. */
	AI_LOOTER_SHOOTER_API FName SoulCoalFlag(FName LumpId);
	/** Said once for good (the campaign's flags): the first coal's word, the first latch's word, the road's word. */
	AI_LOOTER_SHOOTER_API FName CoalWordFlag();
	AI_LOOTER_SHOOTER_API FName LatchWordFlag();

	/** Line sets (Tools/Unreal/create_story_lines.py). */
	AI_LOOTER_SHOOTER_API FName CoalLines();
	AI_LOOTER_SHOOTER_API FName LatchLines();
	AI_LOOTER_SHOOTER_API FName RoadLines();
	AI_LOOTER_SHOOTER_API FName ArriveLines();
	AI_LOOTER_SHOOTER_API FName WakesLines();
}

/**
 * Abel's light in his lantern while Sully carries it home (Main 7 step 4 on, until it's set on the Reliquary), and the
 * freed souls' trail behind it, as plain math (the tests check it; UCarriedLanternSubsystem and ASoulFollowers play it).
 * A Company hose-man's line on the lantern drains the light; free of lines a moment, it fills again. It never goes out:
 * nothing fails, the light is what's at stake (the design: "nothing fails, the light refills once free").
 */
namespace CarriedLanternRules
{
	/** Each line drains this share of the light a second; it never sinks under MinLight. */
	inline constexpr float DrainPerLine = 0.07f;
	inline constexpr float MinLight = 0.15f;
	/** Free of every line this long (s), it fills again at this share a second. */
	inline constexpr float RefillDelay = 1.5f;
	inline constexpr float RefillPerSecond = 0.22f;

	/** The light after DeltaSeconds with Lines on it. */
	AI_LOOTER_SHOOTER_API float Drained(float Light, int32 Lines, float DeltaSeconds);

	/** The light after DeltaSeconds free of lines, SinceFree seconds since the last let go. */
	AI_LOOTER_SHOOTER_API float Refilled(float Light, float SinceFree, float DeltaSeconds);

	/** The lantern hangs at the carrier's right hip: this far right, ahead, and down from their middle (cm). */
	inline constexpr float HipRight = 30.f;
	inline constexpr float HipAhead = 12.f;
	inline constexpr float HipDown = 18.f;

	/** The freed souls (one a coal lump), at most this many following at once. */
	inline constexpr int32 MaxFollowers = 12;

	/**
	 * Follower Index's place behind its leader at Time (the leader's frame: X ahead, Y right, Z up, cm): a loose, swaying
	 * line back from the lantern, each a little farther back and higher, never in the player's view ahead.
	 */
	AI_LOOTER_SHOOTER_API FVector FollowerSlot(int32 Index, float Time);

	/** Waiting by the Keeper's Gate: Index's place round the wait spot (a slow ring, cm). */
	AI_LOOTER_SHOOTER_API FVector WaitSlot(int32 Index, int32 Count, float Time);

	/** A released soul's way west Seconds into its flight from its start (the Gravewind: rising, west, a little south). */
	AI_LOOTER_SHOOTER_API FVector ReleasedAt(const FVector& From, float Seconds, int32 Index);
	/** How long a released soul is seen (s), and its glow at Seconds (rising in, fading out far off). */
	inline constexpr float ReleaseSeconds = 14.f;
	AI_LOOTER_SHOOTER_API float ReleasedGlow(float Seconds);
}
