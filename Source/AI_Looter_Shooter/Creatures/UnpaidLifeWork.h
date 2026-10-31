#pragma once

#include "CoreMinimal.h"
#include "Creatures/UnpaidOpeningRules.h"
#include "Creatures/UnpaidRiseRules.h"

class AUnpaidCreature;
class APawn;
namespace UnpaidBody
{
	struct FPoseChannels;
}

/**
 * The Unpaid's 2026-10-31 round, as friends of AUnpaidCreature (its header past ~500 lines gains only the friendships, as
 * UnpaidKitWork.h's do): its body language (FUnpaidGaitWork, UnpaidCreatureGait.cpp), its rise out of the ground
 * (FUnpaidRiseWork: UnpaidCreatureRise.cpp, its pose UnpaidCreatureRisePose.cpp, its look UnpaidCreatureRiseLook.cpp) and the
 * pack's punish of an opening (FUnpaidOpeningWork, UnpaidCreatureOpening.cpp). The numbers: FUnpaidLifeTuning
 * (Looter.Unpaid.Gait.*, .Rise.*, .Opening.*).
 */
struct FUnpaidGaitWork
{
	/** Looter.Unpaid.Gait on, its rig ready, and no boss (a boss's body is its fight's show). */
	static bool IsOn(const AUnpaidCreature& Unpaid);
	/** As play begins: its limp and its strong side, its own for its life. */
	static void Seed(AUnpaidCreature& Unpaid);
	/** Each posed frame (AnimateBody), before the bones: its lurch's clock, its head's tilt, a hit's loll, the wind-up's coil. Speed: its drift against its chase. */
	static void Step(AUnpaidCreature& Unpaid, float DeltaSeconds, float Speed);
	/** On the trunk's bones, after the rig lays its own (AnimateBody): the lurch, the breath, the tilt and loll, the coil and shiver, a rake's twist, a fall. */
	static void LayBones(AUnpaidCreature& Unpaid);
	/** On one arm's angles (PoseArms, before the keep-out): the lurch's swing, a fall's last reach. Side 0 left, 1 right. */
	static void ArmPose(const AUnpaidCreature& Unpaid, int32 Side, float& Swing, float& Out, float& Bend, float& Wrist, float& Curl);
	/** A fresh hit (OnHurt) of Strength, pushing it Away (its own frame): its head lolls, its lurch stumbles. */
	static void OnHurt(AUnpaidCreature& Unpaid, float Strength, const FVector& Away);
	/** Its death (OnDied, after its kill style is known): how a plain kill falls. */
	static void OnDied(AUnpaidCreature& Unpaid);
	/** Back as it was made (a respawn). */
	static void Reset(AUnpaidCreature& Unpaid);
};

struct FUnpaidRiseWork
{
	/**
	 * Starts its rise of Style: Quick (RiseIn) where it stands; Ambush buried for its slot's stagger in its burst at Around (the
	 * player who sprang it), round them with bSurround (Radius cm at size 1; 0: Looter.Unpaid.Rise.Radius), else where it stands.
	 */
	static void Begin(AUnpaidCreature& Unpaid, const APawn* Around, EUnpaidRiseStyle Style, bool bSurround, float Radius);
	/** Each frame of its rise (Tick, after its brain): its moments (rumble, burst, hands planted, heaves, breath), its turn to its prey. */
	static void Tick(AUnpaidCreature& Unpaid, float DeltaSeconds);
	/** Buried: its dissolve stays whole and its health tag hidden (TickPhaseStep leaves the dissolve to it). */
	static bool HoldsPhase(const AUnpaidCreature& Unpaid);
	/** How far under its hang it is now (cm in the mesh's space; 0 once up): AUnpaidCreature::RiseDrop. */
	static float Drop(const AUnpaidCreature& Unpaid);
	/** Its rise's pose on the channels' goals (PoseTargets). */
	static void Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal);
	/** Its rise on the trunk's bones (AnimateBody): bent over its hands through the haul, its head thrown back as it stands. */
	static void LayBones(AUnpaidCreature& Unpaid);
	/** Its arms through the rise (PoseArms, before the keep-out): the hand up out of the dirt, then both planted for the haul. */
	static void ArmPose(const AUnpaidCreature& Unpaid, int32 Side, float& Swing, float& Out, float& Bend, float& Wrist, float& Curl);
	/** Its rise ends early: killed (bDied: it sinks back into its grave), come back, or gone. Its capsule answers pawns again. */
	static void Stop(AUnpaidCreature& Unpaid, bool bDied);

private:
	/**
	 * A spot round Around for slot Slot of Round (OutFeet: the ground), checked as a phase-step's are; false: nowhere, and
	 * OutWhy says what turned each candidate down (counted by check, for the log).
	 */
	static bool FindSurroundSpot(AUnpaidCreature& Unpaid, const APawn& Around, FUnpaidRiseBurst& Round, int32 Slot, float Radius,
		FVector& OutFeet, FString* OutWhy = nullptr);
	/** Its capsule lets pawns through (bThrough) while it's under the ground, or answers them as before. */
	static void LetPawnsThrough(AUnpaidCreature& Unpaid, bool bThrough);

	// --- Its look (UnpaidCreatureRiseLook.cpp) ---
	/** The ground stirring over it (each so often while it rumbles): dust breathing up, grit hopping, its coal's light in a crack. */
	static void Stir(const AUnpaidCreature& Unpaid);
	/** The dirt bursting as its hand breaks the ground (Strength 0-1: an ambush's 1, a quick rise's less). */
	static void Burst(const AUnpaidCreature& Unpaid, float Strength);
	/** A hand slammed down on the ground (Side 0 left, 1 right): a puff and grit where it lands. */
	static void Plant(const AUnpaidCreature& Unpaid, int32 Side);
	/** Its feet: the ground it rises out of. */
	static FVector FeetOf(const AUnpaidCreature& Unpaid);
};

struct FUnpaidOpeningWork
{
	/**
	 * Each frame of its kit at Victim (TickFightKit): the pack's look at Victim (once a frame, whoever asks: their gun, their
	 * flask), and this one's claim of the punish when it's the one to (behind them or at their side, near, free). True when it
	 * claimed it now: its hiss, its coal, its pack's frenzy.
	 */
	static bool Tick(AUnpaidCreature& Unpaid, const APawn& Victim);
	/** Victim is open now (reloading, swapping, an empty gun, the flask), by the pack's last look. */
	static bool IsOpen(const AUnpaidCreature& Unpaid, const APawn& Victim);
	/** This one claimed the punish at Victim a moment ago (its next turn favours it). */
	static bool IsPunisher(const AUnpaidCreature& Unpaid, const APawn& Victim);
	/** What Victim's hands are doing now: their gun (reloading, swapping, empty) and their flask. */
	static FUnpaidOpeningRead Read(const APawn& Victim);
};
