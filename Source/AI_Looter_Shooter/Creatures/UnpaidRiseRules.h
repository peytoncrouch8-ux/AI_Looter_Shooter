#pragma once

#include "CoreMinimal.h"

/**
 * How the Unpaid rise out of the ground (2026-10-31, the user's pick for their AI: "Unpaid rise from the ground near the
 * player"; UnpaidCreatureRise.cpp plays it). The rules as plain functions, so the tests check them without a world.
 *
 * A rise goes Buried (unseen under the dirt; the last RumbleSeconds of it the ground stirs: dust breathing up, grit hopping, the
 * coal's light seeping through a crack, a low rumble), Breach (the dirt bursts, one hand claws up out of it and grasps at the
 * air), Haul (it slams that hand down, the other comes up and plants, and it drags itself up and out in two heaves, bent over,
 * dirt pouring off it) and Stand (it straightens, throws its head back for its first breath, its coal catching, and turns on
 * the player). An ambush's rise (RiseAmbush) is the whole of it, staggered across its pack; a boss's adds, a toll's dead and
 * the miners' quick rise (RiseIn) skip the wait and the rumble and play the rest quicker.
 *
 * Surround: an ambush that rises round the player (EAmbushEntrance::RiseAround) gives each of its dead a slot in a burst at
 * that player: the first where they look (they see the first hand come up: the read), the next behind them (turn round!),
 * then their sides, then the gaps between, each a little off the even ring, each rising a beat after the one before.
 */
enum class EUnpaidRisePhase : uint8
{
	None,
	Buried,
	Breach,
	Haul,
	Stand,
};

/** Which rise it plays. */
enum class EUnpaidRiseStyle : uint8
{
	None,
	/** RiseIn: no wait, no rumble; a quicker breach, haul and stand (a boss's adds, a toll's dead, the miners). */
	Quick,
	/** RiseAmbush: buried for its stagger, the ground stirring, then the whole climb out. */
	Ambush,
};

/** One rise's timings (s). */
struct AI_LOOTER_SHOOTER_API FUnpaidRiseTiming
{
	float Buried = 0.f;
	/** The ground stirs over this much of the end of its wait (never longer than the wait). */
	float Rumble = 0.f;
	float Breach = 0.25f;
	float Haul = 0.45f;
	float Stand = 0.35f;

	float Length() const { return Buried + Breach + Haul + Stand; }
	/** When its brain may go (s from the start): a short way into its stand, so it turns on them as it straightens. */
	float BrainFree() const { return Buried + Breach + Haul + 0.3f * Stand; }
};

/** A burst of the dead rising round one player (UUnpaidPackSubsystem keeps one per player): its start, its slots, its spots. */
struct AI_LOOTER_SHOOTER_API FUnpaidRiseBurst
{
	/** When it began and when it last gave a slot (world seconds), the slots given so far, its turn off the player's view
	 * (degrees) and that view (degrees). */
	double Start = -1000.0;
	double Last = -1000.0;
	int32 Given = 0;
	float Turn = 0.f;
	float ViewYaw = 0.f;
	/** Where its dead came up (world), so the next keeps its distance. */
	TArray<FVector> Spots;
};

namespace UnpaidRiseRules
{
	/** A quick rise's breach, haul and stand (s): RiseIn's whole rise is their sum (AUnpaidCreature::RiseSeconds). */
	inline constexpr float QuickBreach = 0.25f;
	inline constexpr float QuickHaul = 0.45f;
	inline constexpr float QuickStand = 0.35f;
	/** An ambush's (s). */
	inline constexpr float AmbushBreach = 0.55f;
	inline constexpr float AmbushHaul = 0.8f;
	inline constexpr float AmbushStand = 0.5f;

	/** How deep it lies buried (cm under its hang, at size 1): a hand at full stretch up only just breaks the ground. */
	inline constexpr float BuriedDepth = 230.f;
	/** A quick rise's start (cm under its hang): its head and shoulders come up with its hands. */
	inline constexpr float QuickDepth = 150.f;

	/** Its haul: its lead hand planted this far into it, the other hand here, the second heave from here (its share, 0-1). */
	inline constexpr float LeadPlantAt = 0.06f;
	inline constexpr float OtherPlantAt = 0.34f;
	inline constexpr float SecondHeaveAt = 0.55f;

	/** At most this many slots a burst (its pattern's length); more go round again a little farther out. */
	inline constexpr int32 PatternSlots = 8;

	/** The timings of a Style rise that waits Delay seconds buried (an ambush's) with Rumble of that stirring (both 0 quick). */
	AI_LOOTER_SHOOTER_API FUnpaidRiseTiming Timing(EUnpaidRiseStyle Style, float Delay, float Rumble);

	/** Its phase Time seconds into a rise of Timing, and how far into that phase (OutInto, 0-1). None past its end. */
	AI_LOOTER_SHOOTER_API EUnpaidRisePhase PhaseAt(float Time, const FUnpaidRiseTiming& Timing, float& OutInto);

	/** Whether the ground stirs over it Time seconds in (the rumble: the end of its wait). */
	AI_LOOTER_SHOOTER_API bool IsRumbling(float Time, const FUnpaidRiseTiming& Timing);

	/**
	 * How far under its hang it is (cm at size 1, 0 or below; a touch over in the stand's settle) in Phase, Into through it:
	 * buried deep, the breach jerking up a little, the haul's two heaves, the stand's straightening.
	 */
	AI_LOOTER_SHOOTER_API float Sink(EUnpaidRiseStyle Style, EUnpaidRisePhase Phase, float Into);

	/** Slot's bearing round the player (degrees, world): their view's yaw plus the pattern's offset and the burst's turn. */
	AI_LOOTER_SHOOTER_API float SurroundBearing(int32 Slot, float ViewYaw, float Turn);

	/** The pattern's offset from the player's view for Slot (degrees): 0 ahead, then behind, the sides, the gaps between. */
	AI_LOOTER_SHOOTER_API float PatternOffset(int32 Slot);

	/** Slot's distance from the player (cm at size 1): every other one a little farther, and farther again each time round. */
	AI_LOOTER_SHOOTER_API float SurroundRadius(int32 Slot, float Radius);

	/** Seconds Slot lies buried before its hand breaks the ground: the first after First, each next one Gap later (Roll: its jitter, 0-1). */
	AI_LOOTER_SHOOTER_API float StaggerDelay(int32 Slot, float First, float Gap, float Roll);

	/** Where Slot may rise, best first: its own spot, then a little round either way, then nearer and farther (Center: the player's feet). */
	AI_LOOTER_SHOOTER_API TArray<FVector> SurroundCandidates(const FVector& Center, float Bearing, float Radius);

	/** Spot keeps Spacing (cm, flat) from every one of Taken, and ClearOfPlayer from Center: no two come up in one grave, none under their feet. */
	AI_LOOTER_SHOOTER_API bool IsClear(const FVector& Spot, const TArray<FVector>& Taken, float Spacing, const FVector& Center, float ClearOfPlayer);

	/**
	 * The next slot of Burst at Now (world s): a burst older than Window starts afresh at slot 0 with the player's ViewYaw and a
	 * turn of TurnRoll (0-1 across MaxTurn either way); else the next slot of it.
	 */
	AI_LOOTER_SHOOTER_API int32 ClaimSlot(FUnpaidRiseBurst& Burst, double Now, float ViewYaw, float Window, float TurnRoll, float MaxTurn);
}

class APawn;

/** An Unpaid's rise as it goes (AUnpaidCreature's Kit.Rise; UnpaidCreatureRise.cpp). */
struct AI_LOOTER_SHOOTER_API FUnpaidRiseState
{
	EUnpaidRiseStyle Style = EUnpaidRiseStyle::None;
	FUnpaidRiseTiming Timing;
	/** Seconds into it, its phase as last ticked, and how far into that phase (0-1). */
	float Time = 0.f;
	EUnpaidRisePhase Phase = EUnpaidRisePhase::None;
	float Into = 0.f;
	/** Its slot in its burst (-1: none), and the arm that breaks the ground first (0 left, 1 right). */
	int32 Slot = -1;
	int32 LeadArm = 1;
	/** Its moments so far: the rumble's sound, the burst, each hand planted, the second heave's dirt, its first breath. */
	bool bRumbled = false;
	bool bBurst = false;
	bool bPlanted[2] = { false, false };
	bool bHeaved = false;
	bool bStood = false;
	/** Seconds to the next stirring of the dirt over it while the ground rumbles. */
	float StirIn = 0.f;
	/** Its capsule lets pawns through while it's under the ground (and what it answered pawns with before). */
	bool bPawnsThrough = false;
	uint8 PawnResponse = 0;
	/** Killed mid-rise: how far under its hang it was (cm at size 1), so it sinks back into its grave rather than popping up. */
	bool bDiedRising = false;
	float DiedAt = 0.f;
	/** Who it rises for: its turn and its first breath face them. */
	TWeakObjectPtr<const APawn> For;

	bool IsRising() const { return Style != EUnpaidRiseStyle::None; }
};
