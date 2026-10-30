#pragma once

#include "CoreMinimal.h"

class AUnpaidCreature;
class APawn;
struct FUnpaidMoveTuning;
enum class EUnpaidMove : uint8;
enum class EUnpaidShriekPhase : uint8;
namespace UnpaidBody
{
	struct FPoseChannels;
}

/**
 * AUnpaidCreature's varied moves (2026-10-29, the user's "Unpaid fights: variety"; UnpaidCreatureVariety.cpp). Friends of the
 * class, so they reach its kit, body and brain as its own files do, while its header (past ~500 lines) gains only the
 * friendship. Only the Unpaid's own files call them; each is a no-op unless it fights with its kit and Looter.Unpaid.Variety is
 * on with a fight director (IsOn): bosses, a boss's adds, the miners, the Bell-Ringer and Abel keep their own fights.
 */
struct FUnpaidMoveWork
{
	/** Its kit, its variety on, not a boss's add, and a fight director in its world. */
	static bool IsOn(const AUnpaidCreature& Unpaid);

	/** Its turn at Victim (StartsRingMove): picks its move, closes in for a close one, or hangs back. True when it moved itself this frame. */
	static bool PlanTurn(AUnpaidCreature& Unpaid, const APawn& Victim, float Distance);
	/** As its wind-up begins (ChooseKitMove): its planned move (or one picked now), its timings, the blink of a phase-strike. False: the old way. */
	static bool Choose(AUnpaidCreature& Unpaid);
	/** Its strike for its own moves (a feint's or low swipe's flight, a phase-strike's claw); true when it handled it. */
	static bool Strike(AUnpaidCreature& Unpaid);
	/** Each attack frame: the feint's slide, the phase-strike's tell. */
	static void TickAttack(AUnpaidCreature& Unpaid, float DeltaSeconds);
	/**
	 * Each frame with its kit (TickFightKit): clocks, the pack's board, the rally, the hang-back's circling, an owed shriek
	 * started. True when it began a shriek this frame (TickFightKit then does no more).
	 */
	static bool TickKit(AUnpaidCreature& Unpaid, float DeltaSeconds, const APawn& Victim);
	/** A low swipe's flight reaching its target: whether it caught their feet (TryLungeHit's, for a low swipe). */
	static bool TryLowSwipeHit(AUnpaidCreature& Unpaid);
	/** A flight's end (EndLunge): a feint slides aside; a lunge that missed may turn into a shriek. */
	static void OnFlightEnded(AUnpaidCreature& Unpaid);
	/** A lunge spent without landing (in flight, struck where it stood, or its target gone behind cover): the shriek's roll. */
	static void NoteLungeMissed(AUnpaidCreature& Unpaid);
	/** Its turn played out: true when a shriek is owed (no fade-back now). */
	static bool OnTurnPlayedOut(AUnpaidCreature& Unpaid);
	/** Its flight's speed (cm/s): a feint's or a low swipe's own, else the lunge's. */
	static float FlightSpeed(const AUnpaidCreature& Unpaid);
	/** Holding its attack (CanStartAttack): hanging back. */
	static bool HoldsAttack(const AUnpaidCreature& Unpaid);
	/** Rallied by a pack-mate's shriek now (its chase comes at the frenzy's pace, its coal high). */
	static bool IsRallied(const AUnpaidCreature& Unpaid);
	/** Its moves' pose on the channels' goals (PoseTargets, before its statuses'). */
	static void Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal);
	/** Death, coming back, leaving the world: its board entry gone, its state back to calm, its light given back. */
	static void Forget(AUnpaidCreature& Unpaid);

private:
	/** Blinks it to the player's side or behind them for a phase-strike: false with nowhere to land. OutArm: the arm it claws with. */
	static bool BlinkToStrike(AUnpaidCreature& Unpaid, const APawn& Victim, const FUnpaidMoveTuning& Tuning, int32& OutArm);
	/** A feint's or a low swipe's flight (Move), like the lunge's with its own length and pace. */
	static void StartFlight(AUnpaidCreature& Unpaid, EUnpaidMove Move);
	/** The phase-strike's claw across its front: whether it caught its target. */
	static bool Claw(AUnpaidCreature& Unpaid);
	/** What it commits at its target now, for the pack's board. */
	static EUnpaidMove CommittedNow(const AUnpaidCreature& Unpaid);
};

/**
 * AUnpaidCreature's failed-lunge shriek (UnpaidCreatureShriek.cpp; its pose, UnpaidShriekPose.cpp). A lunge that misses
 * may turn into a shriek (FUnpaidMoveWork::NoteLungeMissed): it stops, turns on the player and screams. Its coal shot in the
 * tell chokes it off (a stagger, its coal exposed a moment); the scream rattles players near it (UPlayerRattleSubsystem) and
 * rallies its pack.
 */
struct FUnpaidShriekWork
{
	/** Starts the shriek now (its owed shriek, or the tests and dev tools): false when it can't (no target, busy, dead). */
	static bool Start(AUnpaidCreature& Unpaid);
	/** Each frame of it (TickKitAction, while its Action is the shriek). */
	static void Tick(AUnpaidCreature& Unpaid, float DeltaSeconds, const APawn& Victim);
	/** Its end or calling off (EndKitAction): its light back, its hat back on its head. */
	static void CleanUp(AUnpaidCreature& Unpaid, bool bCalledOff);
	/** A hit on it (OnHurt): its coal shot in the tell interrupts the shriek. True when it did. */
	static bool OnHurt(AUnpaidCreature& Unpaid, bool bCritical, const FVector& HitLocation);
	/** How much bigger its coal's crit spot is now (1; more while it is exposed after an interrupt). */
	static float CoalExposure(const AUnpaidCreature& Unpaid);

	// --- Its body (UnpaidShriekPose.cpp) ---
	/** The shriek's pose on the channels' goals: reared, head up, jaw wide, arms spread; the scream's thrust; the hunch. */
	static void Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal);
	/** Its quick motions on the trunk's bones after the channels: the head's violent shake, the shudder, the panting. */
	static void LayBones(AUnpaidCreature& Unpaid);
	/** A shake on the drift its shroud swings to (cm/s, its own frame): the flare and the shudder. */
	static FVector ShroudShake(const AUnpaidCreature& Unpaid);
	/** Its hat lifting off its head through the scream and settling back, and the shroud's flare (each frame of it). */
	static void TickBody(AUnpaidCreature& Unpaid, float DeltaSeconds);
	/** Its hat back where it sat (the shriek's end, a death). */
	static void SettleHat(AUnpaidCreature& Unpaid);

private:
	/** The scream's moment (its first frame): its cry, its rings and ground wave, the players rattled, its pack rallied. */
	static void Scream(AUnpaidCreature& Unpaid, const FUnpaidMoveTuning& Tuning);
	/** Its coal's heat, its light and its particles this frame, at its phase's rates. */
	static void Show(AUnpaidCreature& Unpaid, float DeltaSeconds, EUnpaidShriekPhase Phase, const FUnpaidMoveTuning& Tuning);
	/** Where its mouth is now (world), and the way the scream leaves it (OutWay, a unit vector). */
	static FVector MouthOf(const AUnpaidCreature& Unpaid, FVector& OutWay);
};
