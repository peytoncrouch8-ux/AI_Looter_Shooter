#pragma once

// BargeFightRules: the deck fight's plan on her zones (Main 6, "The Pilot Light"; the user's "Bigger", 2026-11-04: "more
// pushes, Dredgers dropping from the house roof and the tower, a third push from the stern deck"): its three pushes and their
// spots, the pacing between them, the Company's horns, the lanes the player keeps to the walks. Part of BargeRules (included
// at the end of BargeRules.h).

#include "CoreMinimal.h"
#include "World/BargeRules.h"

namespace BargeRules
{
	/**
	 * The three pushes, each its own encounter (ABargeBoarding's fight names; the build script places one spawner each):
	 *  1. BargeDeck, the nine below: on the well deck behind cover facing the landing, on the forecastle's high ground, in
	 *     the walks' mouths; it comes as the Lamplighter draws alongside (its own two waves).
	 *  2. BargeDeckRoof and BargeDeckTower together: Dredgers sliding down ropes (ARappelSpawner: they can't step off a drop
	 *     of more than about 3.5 m) off the house's fore face under its roof onto the well deck's aft half, and off the bow's
	 *     scoop onto the forecastle's fore end: a push from above, in front of and behind the player at once.
	 *  3. BargeDeckStern: from aft, up both house walks behind the player (the walks' mouths are where it shows): it starts in
	 *     the cross walk, the walks' aft junction (the stern deck itself lies past the Dredgers' LoseInterestRadius, 80 m,
	 *     from a player on the forecastle).
	 * She starts the second and third herself (ABargeBoarding::AdvancePushes) while the player fights across her deck (Main 6
	 * step 3): the next when the last has brought all its waves and is down to PushNextAlive, PushBeatSeconds after; or
	 * PushNextSeconds after the last began, whatever is left of it; only while the player is up and within PushPlayerReach
	 * of the deck's middle (a push whose encounters aren't placed counts as down).
	 */
	inline constexpr int32 NumDeckPushes = 3;
	inline constexpr int32 PushNextAlive = 2;
	inline constexpr float PushBeatSeconds = 4.f;
	inline constexpr float PushNextSeconds = 75.f;
	inline constexpr float PushPlayerReach = 8000.f;
	/** The next push is due: the last one mostly down for a beat, or on long enough. Pure. */
	inline bool IsNextPushDue(bool bMostlyDown, float MostlyDownSeconds, float SinceStarted)
	{
		return (bMostlyDown && MostlyDownSeconds >= PushBeatSeconds) || SinceStarted >= PushNextSeconds;
	}
	/** The Dredgers each push brings (the build script's groups and the mission's counts must match: BARGE.md "Round 14"). */
	inline constexpr int32 DeckPushDredgers[NumDeckPushes] = { 9, 6, 4 };

	/** The first push's nine spots (BargeDeck): four on the well deck behind cover facing the landing (one aft of the
	 *  gantry), three on the forecastle (by the stairs' heads, one fore of the frame), one in each house walk's mouth. */
	inline const FVector DeckFightSpots[] = { FVector(3700.0, 300.0, WellDeckZ), FVector(3750.0, -350.0, WellDeckZ), FVector(2700.0, 180.0, WellDeckZ),
		FVector(2290.0, -380.0, WellDeckZ), FVector(4250.0, -440.0, ForecastleZ), FVector(4250.0, 380.0, ForecastleZ),
		FVector(4885.0, 150.0, ForecastleZ), FVector(1850.0, -757.0, WalkZ), FVector(1850.0, 780.0, WalkZ) };

	/**
	 * A dropper's rope (her frame): its top, where he appears and hangs (a hand off the face he comes over, under its top),
	 * and his feet's spot, where he lands (the spawner's spot; his rope's turn is his spot's). The roof's: off the house's
	 * fore face (its upper storey's hull to x 2238, its roof at 3200) down to the well deck's aft half, slanted out over the
	 * horns, the vent and the stack. The tower's: off the bow scoop's aft face (its hull from x 5090, its top 3129) down onto
	 * the forecastle's fore end either side of the winch and the barrels.
	 */
	struct FDropRope
	{
		FVector Top;
		FVector Feet;
	};
	inline const FDropRope RoofDrops[] = {
		{ FVector(2262.0, -380.0, 3190.0), FVector(2880.0, -350.0, WellDeckZ) },
		{ FVector(2262.0, 0.0, 3190.0), FVector(2660.0, -80.0, WellDeckZ) },
		{ FVector(2262.0, 600.0, 3190.0), FVector(2800.0, 560.0, WellDeckZ) },
	};
	inline const FDropRope TowerDrops[] = {
		{ FVector(5070.0, -250.0, 3100.0), FVector(4960.0, -300.0, ForecastleZ) },
		{ FVector(5070.0, 250.0, 3100.0), FVector(4960.0, 280.0, ForecastleZ) },
	};
	/** How far out of the face a rope's top hangs (the tests find the face that close behind it). */
	inline constexpr float DropTopOff = 60.f;

	/** The third push's spots (BargeDeckStern) in the cross walk either side of the hatch, each at least the spawner's
	 *  Spacing (250) from the next, off the coaming and the restart: two at its aft corners, two by the house's walks. */
	inline const FVector SternFightSpots[] = { FVector(-600.0, -820.0, CrossWalkZ), FVector(-600.0, 820.0, CrossWalkZ), FVector(-400.0, -560.0, CrossWalkZ),
		FVector(-400.0, 560.0, CrossWalkZ) };

	/**
	 * The Company's horns (ACompanyLoudspeaker posts, the user's Tripo post: 4 m, 2.1 m across, its horns toward its +X;
	 * the build script attaches one at each), off every lane the player runs (the PIE walk, 2026-11-04: on the well deck
	 * they left only a knife-edge to the walks): all four on the starboard bulwark's rim facing in (the port rim is the
	 * Lamplighter's side; against the house they met the gantry and the stack), clear of the rim's work lamps, the bow-most
	 * first (Vane's deck cue is the first's). Each: its foot, its yaw (her frame).
	 */
	struct FHornSpot
	{
		FVector At;
		float Yaw;
	};
	inline const FHornSpot HornSpots[] = { { FVector(4050.0, 792.0, WellDeckZ + RailHeight), -90.f }, { FVector(3800.0, 792.0, WellDeckZ + RailHeight), -90.f },
		{ FVector(3100.0, 792.0, WellDeckZ + RailHeight), -90.f }, { FVector(2700.0, 792.0, WellDeckZ + RailHeight), -90.f } };
	/** Half a horn post's width across its horns (the model's 211 cm). */
	inline constexpr float HornHalfWidth = 110.f;

	/**
	 * The lanes (the PIE walk's): from the plank's landing and from the well deck's middle to both walks' mouths, this wide
	 * on the well deck: along the port rail at PortLaneY and the starboard rail at StarboardLaneY, aft to LaneAftX, then into
	 * each walk's mouth (WalkMouthX: as wide as the walk) past the house's fore corners.
	 */
	inline constexpr float LaneWidth = 150.f;
	inline constexpr float PortLaneY = -610.f;
	inline constexpr float StarboardLaneY = 650.f;
	inline constexpr float LaneAftX = 2320.f;
	inline constexpr float WalkMouthX = 1850.f;
	/** How far outboard of a walk's strip the way into its mouth may swing (the tests' search band). */
	inline constexpr float MouthBandOut = 120.f;

	/** Every spawn spot stands this far over its floor (a hand: the spawner's probes find the floor under it). */
	inline constexpr float SpotUp = 20.f;
	/** The plan's lines of sight (the tests hold it): from the plank's landing the player sees at least this many of the
	 *  nine over the cover (the walks' flankers and the one behind the frame are hidden by design), and from each forecastle
	 *  stair's top at least this many; an eye this high, a chest this high. */
	inline constexpr int32 PlankSightLines = 4;
	inline constexpr int32 StairSightLines = 4;
	inline constexpr float EyeUp = 160.f;
	inline constexpr float ChestUp = 120.f;
	/** The plank's landing keeps this much deck clear round it (no cover or vent in the player's first step). */
	inline constexpr float LandingApron = 200.f;
	/** The well deck's middle (the deck fight's spawner stands there). */
	AI_LOOTER_SHOOTER_API FVector DeckMiddle();
}
